/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#include "avtp.h"

/* 通过继承抽象类形成Server */
class AvtpVideoServer : public AvtpVideoBase
{
public:
	/* 借助父类构造子类 */
	AvtpVideoServer(const char *hostIP, const char *destIP, const unsigned short port) : AvtpVideoBase(true, hostIP, destIP, port){}
private:
	/* 子类必须重写纯虚函数 */
	void *stateMachine(void *arg);
};

/*
	WSAError:
	10035 - WSAEWOULDBLOCK
		资源暂时不可用。对非阻塞套接字来说，如果请求操作不能立即执行的话，通常会返回这个错误。
		比如说，在一个非阻塞套接字上调用connect，就会返回这个错误。因为连接请求不能立即执行。

	10040 - WSAEMSGSIZE
		消息过长。这个错误的含义很多。如果在一个数据报套接字上发送一条消息，
		这条消息对内部缓冲区而言太大的话，就会产生这个错误。
		再比如，由于网络自身的限制，使一条消息过长，也会产生这个错误。
		最后，如果收到数据报之后，缓冲区太小，不能接收消息时，也会产生这个错误。
*/

