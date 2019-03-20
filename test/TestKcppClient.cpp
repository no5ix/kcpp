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

#include "../kcpp.h"


#define SERVER_PORT 6666

// if u modify this `PRACTICAL_CONDITION`,
// u have to update this var of the server side to have the same value.
#define PRACTICAL_CONDITION 1

#define SND_BUFF_LEN 200
#define RCV_BUFF_LEN 1500

 //#define SERVER_IP "172.96.239.56"
#define SERVER_IP "127.0.0.1"
//#define SERVER_IP "192.168.46.125"

using kcpp::KcpSession;

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

kcpp::UserInputData udp_input(char *buf, int len, int fd, struct sockaddr_in from)
{
	socklen_t fromAddrLen = sizeof(from);
	int recvLen = ::recvfrom(fd, buf, len, 0,
		(struct sockaddr*)&from, &fromAddrLen);
	return kcpp::UserInputData(buf, recvLen);
}

void error_pause()
{
	printf("press any key to quit ...\n");
	char ch; scanf("%c", &ch);
}

void udp_msg_sender(int fd, struct sockaddr* dst)
{
	char sndBuf[SND_BUFF_LEN];
	char rcvBuf[RCV_BUFF_LEN];

	// we can't use char array, cause we don't know how big the recv_data is
	kcpp::Buf kcppRcvBuf;

	struct sockaddr_in from;
	int len = 0;
	uint32_t initIndex = 11;
	uint32_t nextSndIndex = initIndex;
	int64_t nextKcppUpdateTs = 0;
	int64_t nextSendTs = 0; 

	KcpSession kcppClient(
		kcpp::RoleTypeE::kCli,
		std::bind(udp_output, std::placeholders::_1, std::placeholders::_2, fd, dst),
		std::bind(udp_input, rcvBuf, RCV_BUFF_LEN, fd, std::ref(from)),
		std::bind(iclock));


#if !PRACTICAL_CONDITION

	static const int64_t kSendInterval = 0;
	const uint32_t testPassIndex = 66666;
	kcppClient.SetConfig(666, 1024, 1024, 4096, 1, 1, 1, 1, 0, 5);

	while (1)
	{

#else

	//kcppClient.SetConfig(1500, 32, 128, 128, 0, 100, 2, 0, 0, 100);
	static const int64_t kSendInterval = 33; // 30fps
	const uint32_t testPassIndex = 666;
	while (1)
	{

#endif // PRACTICAL_CONDITION

		int64_t now = static_cast<int64_t>(iclock());
		if (now >= nextKcppUpdateTs)
			nextKcppUpdateTs = kcppClient.Update();

		if (kcppClient.CheckCanSend() && now >= nextSendTs)
		{
			nextSendTs = now + kSendInterval;
			memset(sndBuf, 0, SND_BUFF_LEN);
			((uint32_t*)sndBuf)[0] = nextSndIndex++;

			len = kcppClient.Send(sndBuf, SND_BUFF_LEN);
			if (len < 0)
			{
				printf("kcpSession Send failed\n");
				error_pause();
				return;
			}
		}

		while (kcppClient.Recv(&kcppRcvBuf, len))
		{
			if (len < 0)
			{
				printf("kcpSession Recv failed, Recv() = %d \n", len);
				error_pause();
				return;
			}
			else if (len > 0)
			{
				uint32_t srvRcvMaxIndex = *(uint32_t*)(kcppRcvBuf.peek() + 0);
				kcppRcvBuf.retrieveAll();
				printf("msg from server: have recieved the max index = %d\n", (int)srvRcvMaxIndex);
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