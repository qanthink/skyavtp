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
#include <stddef.h>
#endif

class UdpSocket {
public:
	int create(bool bBind, const char *_hostIP, const char *_destIP, unsigned short _ipPort);		// 创建套接字，与构造函数具有相同功能。

	UdpSocket(){};
	UdpSocket(bool bBind, const char *_hostIP, const char *_destIP, unsigned short _ipPort);		// 用构造函数创建套接字。
	~UdpSocket();

	int sendTo(const void *const dataBuf, const int dataSize, const char *ipAddr);			// 发送UDP 数据。
	int recvFrom(void *const dataBuf, const int dataSize);					// 以阻塞方式接收UDP 数据。
	int recvNonblock(void *const dataBuf, const int dataSize);			// 以非阻塞方式接收UDP 数据。
	int peek(void *const dataBuf, const int dataSize, char *ipAddr, const unsigned int ipAddrLen);					// 以非阻塞方式窥探UDP 数据，而不将数据从缓冲中移除。从而后续的recv() 可以再次获取数据。
	int relinquish();													// 以非阻塞方式从缓冲中移除UDP 数据。

	int checkTimeOutMs(unsigned int millisecond);						// 检测超时。
	int setRecvBufLen(const unsigned int recvBufLen = 1 * 1024 * 1024);	// 设置UDP 接收缓冲区的大小。
	int setSendBufLen(const unsigned int sendBufLen = 1 * 1024 * 1024);	// 设置UDP 发送缓冲区的大小。

private:
#ifdef _WIN64
	SOCKET sfd = -1;
#elif defined(__linux__)
	int sfd = -1;
#endif
	bool bInit = false;
	unsigned short ipPort = 0;
	const char *destIP = NULL;
	const char *hostIP = NULL;
	struct sockaddr_in stSockAddrDst;
	struct sockaddr_in stSockAddrHost;
};

