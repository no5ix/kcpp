
# QQ群

因为 KCP 官方群已经满了, 可以加群 496687140


# 轻量级的kcp会话实现-kcpsess

`kcpsess`真正实现了只需要包含一个头文件再随意写几行代码就可以用上kcp, 而无需烦心如何组织代码来适配kcp

- 只需包含 `kcpsess.h` 这一个头文件即可
- 只需调用 `KcpSession::Send` 和 `KcpSession::Recv` 和 `KcpSession::Update` 即可完成UDP的链接状态管理、会话控制、 RUDP协议调度

# Features

- single-header-only
- session implementation
- fec support
- two-channel
   - reliable
   - unreliable

# kcpsess Examples

- [realtime-server](https://github.com/no5ix/realtime-server) : A realtime dedicated game server ( FPS / MOBA ). 一个实时的专用游戏服务器.
- [realtime-server-ue4-demo](https://github.com/no5ix/realtime-server-ue4-demo) :  A UE4 State Synchronization demo for realtime-server. 为realtime-server而写的一个UE4状态同步demo, [Video Preview 视频演示](https://hulinhong.com)
- [TestKcpSessionServer.cpp](https://github.com/no5ix/kcpsess/blob/master/TestKcpSessionServer.cpp)
- [TestKcpSessionClient.cpp](https://github.com/no5ix/kcpsess/blob/master/TestKcpSessionClient.cpp)


# kcpsess Usage

the main loop was supposed as:

``` c++
Game.Init()

// kcpsess init
KcpSession myKcpSess(
    KcpSession::RoleTypeE,
    std::bind(udp_output, _1, _2),
    std::bind(udp_input),
    std::bind(timer));

while (!isGameOver)

    while (myKcpSess.Recv(data, len))
        if (len > 0)
            Game.HandleRecvData(data, len)
        else if (len < 0)
            Game.HandleRecvError(len);

        if (myKcpSess.CheckCanSend())
            myKcpSess.Send(data, len)
        else
            Game.HandleCanNotSendForNow()

    myKcpSess.Update()
    Game.Logic()
    Game.Render()
```

The Recv/Send/Update functions of kcpsess are guaranteed to be non-blocking.
Please read [TestKcpSessionClient.cpp](https://github.com/no5ix/kcpsess/blob/master/TestKcpSessionClient.cpp) and [TestKcpSessionServer.cpp](https://github.com/no5ix/kcpsess/blob/master/TestKcpSessionServer.cpp) for some basic usage.


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

在注释的过程中， 除了少量空格和换行以及一处有无符号比较的调整(为保证高警告级别可编译过)外 :    
`if ((IUINT32)count >= IKCP_WND_RCV) return -2;`   
没有对原始代码进行任何其他改动， 最大程度地保证了代码的“原汁原味”。