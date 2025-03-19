# Lightweight KCP Session Implementation - kcpp

`kcpp` truly enables you to use KCP just by including a single header file and writing a few lines of code, without having to worry about how to organize the code to adapt to KCP.

- You only need to include the single header file `kcpp.h`.
- You only need to call `KcpSession::Send`, `KcpSession::Recv`, and `KcpSession::Update` to complete UDP connection state management, session control, and RUDP protocol scheduling.

# Features

- Single-header-only
- Session implementation
- Dynamic redundancy
- Two-channel
  - Reliable
  - Unreliable

# kcpp Examples

- [realtime-server](https://github.com/no5ix/realtime-server): A real-time dedicated game server (FPS / MOBA).
- [realtime-server-ue4-demo](https://github.com/no5ix/realtime-server-ue4-demo): A UE4 state synchronization demo for the real-time server. [Video Preview](https://hulinhong.com)

# kcpp Usage

The main loop is supposed to be as follows:

```c++
Game.Init();

// kcpp initialization
kcpp::KcpSession myKcpSess(
    KcpSession::RoleTypeE,
    std::bind(udp_output, _1, _2),
    std::bind(udp_input),
    std::bind(timer)
);

while (!isGameOver) {
    myKcpSess.Update();

    while (myKcpSess.Recv(data, len)) {
        if (len > 0) {
            Game.HandleRecvData(data, len);
        } else if (len < 0) {
            Game.HandleRecvError(len);
        }
    }

    if (myKcpSess.CheckCanSend()) {
        myKcpSess.Send(data, len);
    } else {
        Game.HandleCanNotSendForNow();
    }

    Game.Logic();
    Game.Render();
}
```

The `Recv`, `Send`, and `Update` functions of kcpp are guaranteed to be non-blocking.
Please read [TestKcppClient.cpp](https://github.com/no5ix/kcpp/blob/master/TestKcppClient.cpp) and [TestKcppServer.cpp](https://github.com/no5ix/kcpp/blob/master/TestKcppServer.cpp) for some basic usage examples.

# KCP Source Code Annotations

This project also comes with an annotated version of the KCP source code, `ikcp.h` and `ikcp.c`. It can be regarded as another detailed explanation of KCP, which is convenient for self-study and helps others get started more quickly. The original code is from: https://github.com/skywind3000/kcp. Thanks to skywind3000 for bringing such a concise and excellent project.

Readers interested in open-source projects for real-time combat games like FPS / MOBA can also visit [realtime-server](https://github.com/no5ix/realtime-server). Welcome to communicate.

Note: Tabs are used for indentation in the project, and tab is set to 2 spaces.

Almost every paragraph has annotations, and key data structures are accompanied by diagrams. For example:

```
...
//
// The data packets sent by KCP have their own packet structure. The packet header is 24 bytes in total and contains some necessary information. The specific content and size are as follows:
//
// |<------------ 4 bytes ------------>|
// +--------+--------+--------+--------+
// |  conv                             | conv: Conversation, the session serial number used to identify whether the sent and received data packets are consistent.
// +--------+--------+--------+--------+ cmd: Command, the instruction type, representing the type of this Segment.
// |  cmd   |  frg   |       wnd       | frg: Fragment, the segmentation serial number. The segments are numbered from large to small, and 0 indicates that the data packet has been received completely.
// +--------+--------+--------+--------+ wnd: Window, the window size.
// |                ts                 | ts: Timestamp, the sending timestamp.
// +--------+--------+--------+--------+
// |                 sn                | sn: Sequence Number, the Segment serial number.
// +--------+--------+--------+--------+
// |                una                | una: Unacknowledged, the current unacknowledged serial number,
// +--------+--------+--------+--------+      which means that all packets before this serial number have been received.
// |                len                | len: Length, the length of the subsequent data.
// +--------+--------+--------+--------+
//
...

//---------------------------------------------------------------------
// ...
// rcv_queue: The queue for receiving messages. The data in rcv_queue is continuous, while the data in rcv_buf may be intermittent.
// nrcv_que: The number of Segments in the receiving queue rcv_queue, which needs to be less than rcv_wnd.
// rcv_queue is shown in the following diagram:
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
// ... | 2 | 3 | 4 | ............................................... 
// +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//              ^              	        ^                      	^        
//              |                      	|                      	|        
//           rcv_nxt           	rcv_nxt + nrcv_que      rcv_nxt + rcv_wnd		
//
// snd_buf: The buffer for sending messages.
// snd_buf is shown in the following diagram:
// +---+---+---+---+---+---+---+---+---+---+---+---+---+
// ... | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | ...........
// +---+---+---+---+---+---+---+---+---+---+---+---+---+
//              ^               ^               ^
//              |               |               |
//           snd_una         snd_nxt    snd_una + snd_wnd	
//
//
// rcv_buf: The buffer for receiving messages.
// rcv_buf is shown in the following diagram. The data in rcv_queue is continuous, while the data in rcv_buf may be intermittent.
// +---+---+---+---+---+---+---+---+---+---+---+---+---+
// ... | 2 | 4 | 6 | 7 | 8 | 9 | ...........
// +---+---+---+---+---+---+---+---+---+---+---+---+---+	
//
...
```





# 轻量级的kcp会话实现-kcpp

`kcpp`真正实现了只需要包含一个头文件再随意写几行代码就可以用上kcp, 而无需烦心如何组织代码来适配kcp

- 只需包含 `kcpp.h` 这一个头文件即可
- 只需调用 `KcpSession::Send` 和 `KcpSession::Recv` 和 `KcpSession::Update` 即可完成UDP的链接状态管理、会话控制、 RUDP协议调度



# kcp源码注释

本项目还附了一个注释版的kcp源码 `ikcp.h` 和 `ikcp.c`， 算是另一种的 kcp详解, 方便自己学习也为大家更快的上手, 原始代码来自： https://github.com/skywind3000/kcp , 感谢 skywind3000 带来这么短小精悍的好项目

对 FPS / MOBA 类实时对战游戏开源项目感兴趣的读者还可以移步 [realtime-server](https://github.com/no5ix/realtime-server) , 欢迎交流

注 : 项目中使用 tab 缩进且设置了tab = 2 space

几乎每个段落都有注释, 且关键数据结构还带有图解, 比如 : 

```
...
//
//	kcp发送的数据包设计了自己的包结构，包头一共24bytes，包含了一些必要的信息，具体内容和大小如下：
//	
//	|<------------ 4 bytes ------------>|
//	+--------+--------+--------+--------+
//	|  conv                             | conv：Conversation, 会话序号，用于标识收发数据包是否一致
//	+--------+--------+--------+--------+ cmd: Command, 指令类型，代表这个Segment的类型
//	|  cmd   |  frg   |       wnd       | frg: Fragment, 分段序号，分段从大到小，0代表数据包接收完毕
//	+--------+--------+--------+--------+ wnd: Window, 窗口大小
//	|                ts                 | ts: Timestamp, 发送的时间戳
//	+--------+--------+--------+--------+
//	|                 sn                | sn: Sequence Number, Segment序号
//	+--------+--------+--------+--------+
//	|                una                | una: Unacknowledged, 当前未收到的序号，
//	+--------+--------+--------+--------+      即代表这个序号之前的包均收到
//	|                len                | len: Length, 后续数据的长度
//	+--------+--------+--------+--------+
//
...

//---------------------------------------------------------------------
// ...
//	rcv_queue	接收消息的队列, rcv_queue的数据是连续的，rcv_buf可能是间隔的
//	nrcv_que // 接收队列rcv_queue中的Segment数量, 需要小于 rcv_wnd
//	rcv_queue 如下图所示
//	+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//	... | 2 | 3 | 4 | ............................................... 
//	+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//              ^              	        ^                      	^        
//              |                      	|                      	|        
//           rcv_nxt           	rcv_nxt + nrcv_que      rcv_nxt + rcv_wnd		
//
//	snd_buf 发送消息的缓存
//	snd_buf 如下图所示
//	+---+---+---+---+---+---+---+---+---+---+---+---+---+
//	... | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | ...........
//	+---+---+---+---+---+---+---+---+---+---+---+---+---+
//              ^               ^               ^
//              |               |               |
//           snd_una         snd_nxt    snd_una + snd_wnd	
//
//
//	rcv_buf 接收消息的缓存
//	rcv_buf 如下图所示, rcv_queue的数据是连续的，rcv_buf可能是间隔的
//	+---+---+---+---+---+---+---+---+---+---+---+---+---+
//	... | 2 | 4 | 6 | 7 | 8 | 9 | ...........
//	+---+---+---+---+---+---+---+---+---+---+---+---+---+	
//
...
```
