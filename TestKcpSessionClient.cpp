#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#ifndef _WIN32 
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/time.h>
	#include <unistd.h>
#else
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <time.h>
#endif

#include "kcpsess.h"

#define SERVER_PORT 8888

// if u modify this `TEST_APPLICATION_LEVEL_CONGESTION_CONTROL`,
// u have to update this var of the server side to have the same value.
#define TEST_APPLICATION_LEVEL_CONGESTION_CONTROL 1

#define SND_BUFF_LEN 1500
#define RCV_BUFF_LEN 1500

// #define SERVER_IP "172.96.239.56"
#define SERVER_IP "127.0.0.1"

using kcpsess::KcpSession;

#ifdef WIN32
inline int
gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);
	tp->tv_sec = static_cast<long>(clock);
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}
#endif

IUINT32 iclock()
{
	long s, u;
	IUINT64 value;

	struct timeval time;
	gettimeofday(&time, NULL);
	s = time.tv_sec;
	u = time.tv_usec;

	value = ((IUINT64)s) * 1000 + (u / 1000);
	return (IUINT32)(value & 0xfffffffful);
}

void udp_output(const void *buf, int len, int fd, struct sockaddr* dst)
{
	::sendto(fd, (const char*)buf, len, 0, dst, sizeof(*dst));
}

KcpSession::InputData udp_input(char *buf, int len, int fd, struct sockaddr_in from)
{
	socklen_t fromAddrLen = sizeof(from);
	int recvLen = ::recvfrom(fd, buf, len, 0,
		(struct sockaddr*)&from, &fromAddrLen);
	return KcpSession::InputData(buf, recvLen);
}



void udp_msg_sender(int fd, struct sockaddr* dst)
{
	char sndBuf[SND_BUFF_LEN];
	char rcvBuf[RCV_BUFF_LEN];
	kcpsess::Buf kcpsessRcvBuf; // cause we don't know how big the recv_data is

	struct sockaddr_in from;
	int len = 0;
	uint32_t index = 11;

	KcpSession kcpClient(
		KcpSession::RoleTypeE::kCli,
		std::bind(udp_output, std::placeholders::_1, std::placeholders::_2, fd, dst),
		std::bind(udp_input, rcvBuf, RCV_BUFF_LEN, fd, std::ref(from)),
		std::bind(iclock));

#if TEST_APPLICATION_LEVEL_CONGESTION_CONTROL

	const uint32_t testPassIndex = 66666;
	kcpClient.SetKcpConfig(1024, 1024, 4096, 1, 1, 1, 1, 0, 300, 5);
	while (1)
	{

#else

	const uint32_t testPassIndex = 666;
	while (1)
	{
	#ifndef _WIN32
		usleep(16666); // 60fps
	#else
		Sleep(16666 / 1000);
	#endif // !_WIN32

#endif // TEST_APPLICATION_LEVEL_CONGESTION_CONTROL

		kcpClient.Update();
		if (kcpClient.CheckCanSend())
		{
			memset(sndBuf, 0, SND_BUFF_LEN);
			((uint32_t*)sndBuf)[0] = index++;

			len = kcpClient.Send(sndBuf, SND_BUFF_LEN);
			if (len < 0)
			{
				printf("kcpSession Send failed\n");
				return;
			}
		}

		//memset(rcvBuf, 0, RCV_BUFF_LEN);
		//while (kcpClient.Recv(rcvBuf, len))
		while (kcpClient.Recv(&kcpsessRcvBuf, len))
		{
			if (len < 0)
			{
				printf("kcpSession Recv failed, Recv() = %d \n", len);
			}
			else if (len > 0)
			{
				//uint32_t srvRcvMaxIndex = *(uint32_t*)(rcvBuf + 0);
				uint32_t srvRcvMaxIndex = *(uint32_t*)(kcpsessRcvBuf.peek() + 0);
				kcpsessRcvBuf.retrieveAll();
				printf("unreliable msg from server: have recieved the max index = %d\n", (int)srvRcvMaxIndex);
				if (srvRcvMaxIndex >= testPassIndex)
				{
					printf("test passes, yay! \n");
					return;
				}
			}
		}

	}
}


int main(int argc, char* argv[])
{
	int client_fd;
	struct sockaddr_in ser_addr;

#ifdef _WIN32
	WSADATA  Ws;
	//Init Windows Socket
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
	{
		printf("Init Windows Socket Failed");
		return -1;
	}
#endif

	client_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_fd < 0)
	{
		printf("create socket fail!\n");
		return -1;
	}

#ifndef _WIN32
	// set socket non-blocking
	{
		int flags = fcntl(client_fd, F_GETFL, 0);
		fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
	}
#else
	unsigned long flags = 1; /* 这里根据需要设置成0或1 */
	ioctlsocket(client_fd, FIONBIO, &flags);
#endif

	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	ser_addr.sin_port = htons(SERVER_PORT);

	udp_msg_sender(client_fd, (struct sockaddr*)&ser_addr);

#ifndef _WIN32
	close(client_fd);
#else
	closesocket(client_fd);
#endif // !_WIN32

	return 0;
}