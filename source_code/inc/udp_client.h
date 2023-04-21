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

class UdpClient{
public:
	UdpClient(const char *serverIP, const unsigned short ipPort);		// 用构造函数创建套接字。
	~UdpClient();

	int sendto1(const void *const dataBuf, const int dataSize, const struct sockaddr_in *pstAddrClient);			// 发送UDP 数据。
	int recvFrom(void *const dataBuf, const int dataSize, struct sockaddr_in *pstAddrClient);					// 以阻塞方式接收UDP 数据。

	int setSendBufLen(const unsigned int sendBufLen = 2 * 1024 * 1024);	// 设置UDP 发送缓冲区的大小。
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
	const char *mServerIP = NULL;
	struct sockaddr_in stAddrServer;
};

