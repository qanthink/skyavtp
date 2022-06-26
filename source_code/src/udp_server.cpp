/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
*/

#ifdef _WIN64
#include <Ws2tcpip.h>
#elif defined(__linux__)
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#endif
#include <iostream>
#include "udp_server.h"

using namespace std;

/*
	功能：	创建UDP 套接字。
	返回：	返回0.
	注意：	通常用此带参构造函数完成对象的定义和初始化。
*/
UdpServer::UdpServer(const char *serverIP, const unsigned short ipPort)
{
	cout << "Call UdpServer::UdpServer()." << endl;

	sfd = -1;
	mIpPort = ipPort;
	mServerIP = serverIP;

	memset(&stAddrServer, 0, sizeof(struct sockaddr_in));
	stAddrServer.sin_family = AF_INET;
	stAddrServer.sin_port = htons(ipPort);
	stAddrServer.sin_addr.s_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, hostIP, (void *)&stAddrServer.sin_addr.s_addr);

	int ret = 0;
#ifdef _WIN64
	// Windows 初始化网络
	WSADATA wsadata;					// 套接字信息结
	WORD sockVersion = MAKEWORD(2, 2);	// 设置版本号
	ret = WSAStartup(sockVersion, &wsadata);
	if(0 != ret)
	{
		cerr << "Fail to call WSAStartup() in UdpServer::UdpServer(). Error = " << WSAGetLastError() << endl;
		return;
	}
#endif

	// 创建套接字，AP_INET: TCP/IP IPV4; SOCK_DGRAM: UDP
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(-1 == sfd || 0 == sfd)
	{
		cerr << "Fail to call socket()." << endl;
	}

	// bind();	// 主动端可省略绑定。
	ret = bind(sfd, (struct sockaddr *)&stAddrServer, sizeof(struct sockaddr));
	if(-1 == ret)
	{
		cerr << "Fail to call bind()." << endl;
		cerr << " " << strerror(errno) << endl;
		return;
	}
		
	bInit = true;

	/*	套接字缓存区对UDP 性能影响很大，系统默认只有65536Bytes = 64KB. 不足以缓存数秒堆积的视频数据。
		从而导致UDP 局域网点对点有线传输也丢包。
		目前针对FHD@30 H.265 设置的1 MB缓冲区，未来做4K@60 可能需要再适配。
	*/
#if 1
	setSendBufLen();
	setRecvBufLen();
#endif

	cout << "Call UdpServer::UdpServer() end." << endl;
}

/*
	功能：	析构UDP 套接字。
	返回：	
	注意：	
*/
UdpServer::~UdpServer()
{
	cout << "Call UdpServer::~UdpServer()." << endl;
	if(-1 != sfd)
	{
#ifdef _WIN64
		closesocket(sfd);
#elif defined (__linux__)
		close(sfd);
#endif
		sfd = -1;
	}

	mIpPort = 0;
	mServerIP = NULL;
	bInit = false;
	memset(&stAddrServer, 0, sizeof(struct sockaddr_in));
	
	cout << "Call UdpServer::~UdpServer() end." << endl;
}

/*
	功能：	以非阻塞形式发送UDP 数据。
	返回：	成功，返回字节数；失败，返回-1；
	注意：	
*/
int UdpServer::send(const void *const dataBuf, const int dataSize, const struct sockaddr_in *pstAddrClient)
{
	//cout << "Call UdpServer::send()." << endl;

	if(NULL == dataBuf || NULL == pstAddrClient)
	{
		cerr << "Fail to call UdpServer::send(). Arugument has null value." << endl;
		return -1;
	}

	if(!bInit)
	{
		cerr << "Fail to call UdpServer::send(). UdpServer is not initialized." << endl;
		return -1;
	}

	int ret = 0;
	ret = sendto(sfd, (char *)dataBuf, dataSize, 0, (struct sockaddr *)pstAddrClient, sizeof(struct sockaddr));
	if(-1 == ret)
	{
#ifdef _WIN64
		const unsigned int wsaErrno = WSAGetLastError();
		switch(wsaErrno)
		{
			case 10040:
				break;
			default:
				cerr << "Fail to call UdpServer::send(). errno = " << wsaErrno << endl;
				break;
		}
#elif defined(__linux__)
		cerr << "Fail to call UdpServer::send(). " << strerror(errno) << endl;
#endif
	}

	//cout << "Call UdpServer::send() end." << endl;
	return ret;
}

/*
	功能：	以阻塞形式接收UDP 数据。
	返回：	成功，返回字节数；失败，返回-1；
	注意：	
*/
int UdpServer::recv(void *const dataBuf, const int dataSize, struct sockaddr_in *pstAddrClient)
{
	//cout << "Call UdpServer::recv()." << endl;
	//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
	int ret = 0;
#ifdef _WIN64
	int sockLen = 0;
#elif defined(__linux__)
	socklen_t sockLen = 0;
#endif
	sockLen = sizeof(struct sockaddr);
	ret = recvfrom(sfd, (char *)dataBuf, dataSize, 0, (struct sockaddr *)pstAddrClient, &sockLen);
	if(-1 == ret)
	{
#ifdef _WIN64
		const unsigned int wsaErrno = WSAGetLastError();
		switch(wsaErrno)
		{
			case 10040:
				break;
			default:
				cerr << "Fail to call UdpServer::recv(). errno = " << wsaErrno << endl;
				break;
		}
#elif defined(__linux__)
		cerr << "Fail to call UdpServer::recv(). " << strerror(errno) << endl;
#endif
	}

	//cout << "Call UdpServer::recv() end." << endl;
	return ret;
}

/*
	功能：	设置套接字接收缓存区大小。
	返回：	成功，返回0; 失败，返回非0.
	注意：	
*/
int UdpServer::setRecvBufLen(const unsigned int recvBufLen)
{
	int ret = 0;
	unsigned int defaultBufSize = 0;
#ifdef _WIN64
	int optLen = sizeof(unsigned int);
#elif defined(__linux__)
	unsigned int optLen = sizeof(unsigned int);
#endif

	ret = getsockopt(sfd, SOL_SOCKET, SO_RCVBUF, (char *)&defaultBufSize, &optLen);
	if(0 != ret)
	{
#ifdef _WIN64
		cerr << "Fail to get default receive buf size in UdpServer::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default receive buf size in UdpServer::setRecvBufLen(). errno = " << errno << endl;
#endif
	}
	else
	{
		cout << "Default socket receive buffer size = " << defaultBufSize << endl;
	}

	ret = setsockopt(sfd, SOL_SOCKET, SO_RCVBUF, (char *)&recvBufLen, optLen);
	if(0 != ret)
	{
#ifdef _WIN64
		cerr << "Fail to set receive buf size in UdpServer::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to set receive buf size in UdpServer::setRecvBufLen(). errno = " << errno << endl;
#endif
	}
	else
	{
		cout << "Success to set socket receive buffer size to " << recvBufLen << endl;
	}

	ret = getsockopt(sfd, SOL_SOCKET, SO_RCVBUF, (char *)&defaultBufSize, &optLen);
	if(0 != ret)
	{
#ifdef _WIN64
		cerr << "Fail to get default receive buf size in UdpServer::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default receive buf size in UdpServer::setRecvBufLen(). errno = " << errno << endl;
#endif
	}
	else
	{
		cout << "Now socket receive buffer size = " << defaultBufSize << endl;
	}

	return ret;
}

/*
	功能：	设置套接字发送缓存区大小。
	返回：	成功，返回0; 失败，返回非0.
	注意：	
*/
int UdpServer::setSendBufLen(const unsigned int sendBufLen)
{
	int ret = 0;
	unsigned int defaultBufSize = 0;
#ifdef _WIN64
		int optLen = sizeof(unsigned int);
#elif defined(__linux__)
		unsigned int optLen = sizeof(unsigned int);
#endif

	ret = getsockopt(sfd, SOL_SOCKET, SO_SNDBUF, (char *)&defaultBufSize, &optLen);
	if(0 != ret)
	{
#ifdef _WIN64
		cerr << "Fail to get default send buf size in UdpServer::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default send buf size in UdpServer::setRecvBufLen(). errno = " << errno << endl;
#endif
	}
	else
	{
		cout << "Default socket send buffer size = " << defaultBufSize << endl;
	}

	ret = setsockopt(sfd, SOL_SOCKET, SO_SNDBUF, (char *)&sendBufLen, optLen);
	if(0 != ret)
	{
#ifdef _WIN64
		cerr << "Fail to set send buf size in UdpServer::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to set send buf size in UdpServer::setRecvBufLen(). errno = " << errno << endl;
#endif
	}
	else
	{
		cout << "Success to set socket send buffer size to " << sendBufLen << endl;
	}
	
	ret = getsockopt(sfd, SOL_SOCKET, SO_SNDBUF, (char *)&defaultBufSize, &optLen);
	if(0 != ret)
	{
#ifdef _WIN64
		cerr << "Fail to get default send buf size in UdpServer::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default send buf size in UdpServer::setRecvBufLen(). errno = " << errno << endl;
#endif
	}
	else
	{
		cout << "Now socket send buffer size = " << defaultBufSize << endl;
	}

	return ret;
}

