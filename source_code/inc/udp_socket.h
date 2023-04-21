/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#ifdef _WIN64
// winsocket2.h 和windows.h 顺序不能错，否则编译出错，提示重定义信息。
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#elif defined(__linux__)
#include <arpa/inet.h>
#endif

#ifdef _WIN64
#include <Ws2tcpip.h>
#elif defined(__linux__)
#endif

class UdpSocket{
public:
	UdpSocket(const char *localIP, const unsigned short ipPort);				// 用构造函数创建套接字。
	~UdpSocket();

	// 发送UDP 数据。
	int sendTo(const void *buf, size_t len, const struct sockaddr_in *pstDstAddr);
	// 以阻塞方式接收UDP 数据。
	int recvFrom(void *buf, size_t len, struct sockaddr_in *pstSrcAddr);

	int setSendBufLen(const unsigned int sendBufLen = 1 * 1024 * 1024);	// 设置UDP 发送缓冲区的大小。
	int setRecvBufLen(const unsigned int recvBufLen = 1 * 1024 * 1024);	// 设置UDP 接收缓冲区的大小。

	const int getSocketFd() const {return sfd;};

private:
#ifdef _WIN64
	SOCKET sfd = -1;
#elif defined(__linux__)
	int sfd = -1;
#endif
	bool bInit = false;
	unsigned short mIpPort = 0;
	const char *mLocalIP = NULL;
};

