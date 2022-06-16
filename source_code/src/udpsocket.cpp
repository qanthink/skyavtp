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
#include "udpsocket.h"

using namespace std;

/*
	功能：	创建UDP 套接字。
	返回：	返回0.
	注意：	通常使用带参构造函数完成对象的定义和初始化。而该函数用于对象定义和初始化分离的情况。
*/
int UdpSocket::create(bool bBind, const char *_hostIP, const char *_destIP, unsigned short _ipPort)
{
	cout << "Call UdpSocket::create()." << endl;
	UdpSocket(bBind, _hostIP, _destIP, _ipPort);
	cout << "Call UdpSocket::create() end." << endl;
	return 0;
}

/*
	功能：	创建UDP 套接字。
	返回：	返回0.
	注意：	通常用此带参构造函数完成对象的定义和初始化。
*/
UdpSocket::UdpSocket(bool bBind, const char *_hostIP, const char *_destIP, unsigned short _ipPort)
{
	cout << "Call UdpSocket::UdpSocket()." << endl;

	sfd = -1;
	ipPort = _ipPort;
	hostIP = _hostIP;
	destIP = _destIP;

	memset(&stSockAddrHost, 0, sizeof(struct sockaddr_in));
	stSockAddrHost.sin_family = AF_INET;
	stSockAddrHost.sin_port = htons(ipPort);
	//stSockAddrHost.sin_addr.S_un.S_addr = INADDR_ANY;
	inet_pton(AF_INET, hostIP, (void *)&stSockAddrHost.sin_addr.s_addr);

	memset(&stSockAddrDst, 0, sizeof(struct sockaddr_in));
	stSockAddrDst.sin_family = AF_INET;
	stSockAddrDst.sin_port = htons(ipPort);
	inet_pton(AF_INET, destIP, (void *)&stSockAddrDst.sin_addr.s_addr);

	int ret = 0;
#ifdef _WIN64
	// Windows 初始化网络
	WSADATA wsadata;					// 套接字信息结
	WORD sockVersion = MAKEWORD(2, 2);	// 设置版本号
	ret = WSAStartup(sockVersion, &wsadata);
	if(0 != ret)
	{
		cerr << "Fail to call WSAStartup() in UdpSocket::UdpSocket(). Error = " << WSAGetLastError() << endl;
		return;
	}
#endif

	// 创建套接字，AP_INET: TCP/IP IPV4; SOCK_DGRAM: UDP
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(-1 == sfd || 0 == sfd)
	{
		cerr << "Fail to call socket()." << endl;
	}

	if(bBind)
	{
		// bind();	// 主动端可省略绑定。
		ret = bind(sfd, (struct sockaddr *)&stSockAddrHost, sizeof(struct sockaddr));
		if(-1 == ret)
		{
			cerr << "Fail to call bind()." << endl;
			return;
		}
	}
#if 0
#ifdef _WIN64
	/* 设置socket IO 为阻塞/非阻塞模式 */
	unsigned long noBlock = 1;
	ioctlsocket(sfd, FIONBIO, &noBlock);
#elif defined(__linux__)
	int flags = 0;
	flags = fcntl(sfd, F_GETFL, 0);
	if(flags < 0)
	{
		cerr << "Fail to call fcntl(2) with F_GETFL in UdpSocket::UdpSocket()." << endl;
	}

	flags = fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
	if(flags < 0)
	{
		cerr << "Fail to call fcntl(2) with F_SETFL in UdpSocket::UdpSocket()." << endl;
	}
	cout << "Set O_NONBLOCK ok." << endl;
#endif
#endif

	bInit = true;

	/*	套接字缓存区对UDP 性能影响很大，系统默认只有65536Bytes = 64KB. 不足以缓存数秒堆积的视频数据。
		从而导致UDP 局域网点对点有线传输也丢包。
		目前针对FHD@30 H.265 设置的1 MB缓冲区，未来做4K@60 可能需要再适配。
	*/
#if 1
	setRecvBufLen();
	setSendBufLen();
#endif

	cout << "Call UdpSocket::UdpSocket() end." << endl;
}

/*
	功能：	析构UDP 套接字。
	返回：	
	注意：	
*/
UdpSocket::~UdpSocket()
{
	cout << "Call UdpSocket::~UdpSocket()." << endl;
	if(-1 != sfd)
	{
#ifdef _WIN64
		closesocket(sfd);
#elif defined (__linux__)
		close(sfd);
#endif
		sfd = -1;
	}

	ipPort = 0;
	destIP = NULL;
	hostIP = NULL;
	bInit = false;
	memset(&stSockAddrDst, 0, sizeof(struct sockaddr_in));
	memset(&stSockAddrHost, 0, sizeof(struct sockaddr_in));
	cout << "Call UdpSocket::~UdpSocket() end." << endl;
}

/*
	功能：	以非阻塞形式发送UDP 数据。
	返回：	成功，返回字节数；失败，返回-1；
	注意：	
*/
int UdpSocket::sendTo(const void *const dataBuf, const int dataSize, const char *ipAddr)
{
	//cout << "Call UdpSocket::send()." << endl;

	if(NULL == dataBuf)
	{
		cerr << "Fail to call UdpSocket::send(). Arugument has null value." << endl;
		return -1;
	}

	if(!bInit)
	{
		cerr << "Fail to call UdpSocket::send(). UdpSocket is not initialized." << endl;
		return -1;
	}

	int ret = 0;
	memset(&stSockAddrDst, 0, sizeof(struct sockaddr_in));
	stSockAddrDst.sin_family = AF_INET;
	stSockAddrDst.sin_port = htons(ipPort);
	inet_pton(AF_INET, ipAddr, (void *)&stSockAddrDst.sin_addr.s_addr);
	ret = sendto(sfd, (char *)dataBuf, dataSize, 0, (struct sockaddr*)&stSockAddrDst, sizeof(struct sockaddr));
	if(-1 == ret)
	{
#ifdef _WIN64
		const unsigned int wsaErrno = WSAGetLastError();
		switch(wsaErrno)
		{
			case 10040:
				break;
			default:
				cerr << "Fail to call UdpSocket::send(). errno = " << wsaErrno << endl;
				break;
		}
#elif defined(__linux__)
		cerr << "Fail to call UdpSocket::send(). " << strerror(errno) << endl;
#endif
	}

	//cout << "Call UdpSocket::send() end." << endl;
	return ret;
}

/*
	功能：	以阻塞形式接收UDP 数据。
	返回：	成功，返回字节数；失败，返回-1；
	注意：	
*/
int UdpSocket::recvFrom(void *const dataBuf, const int dataSize)
{
	//cout << "Call UdpSocket::recv()." << endl;
	//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
	int ret = 0;
#ifdef _WIN64
	int sockLen = 0;
#elif defined(__linux__)
	socklen_t sockLen = 0;
#endif
	sockLen = sizeof(struct sockaddr_in);
	ret = recvfrom(sfd, (char *)dataBuf, dataSize, 0, (struct sockaddr *)&stSockAddrDst, &sockLen);
	if(-1 == ret)
	{
#ifdef _WIN64
		const unsigned int wsaErrno = WSAGetLastError();
		switch(wsaErrno)
		{
			case 10040:
				break;
			default:
				cerr << "Fail to call UdpSocket::recv(). errno = " << wsaErrno << endl;
				break;
		}
#elif defined(__linux__)
		cerr << "Fail to call UdpSocket::recv(). " << strerror(errno) << endl;
#endif
	}

	//cout << "Call UdpSocket::recv() end." << endl;
	return ret;
}

/*
	功能：	以非阻塞形式接收UDP 数据。
	返回：	成功，返回字节数；失败，返回-1；
	注意：	
*/
int UdpSocket::recvNonblock(void *const dataBuf, const int dataSize)
{
	//cout << "Call UdpSocket::recvNonblock()." << endl;
	//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
	int ret = 0;
#ifdef _WIN64
	unsigned long noBlock = 1;
	ioctlsocket(sfd, FIONBIO, &noBlock);
	int sockLen = 0;
	sockLen = sizeof(struct sockaddr_in);
	ret = recvfrom(sfd, (char *)dataBuf, dataSize, 0, (struct sockaddr *)&stSockAddrDst, &sockLen);
#elif defined(__linux__)
	socklen_t sockLen = 0;
	sockLen = sizeof(struct sockaddr_in);
	ret = recvfrom(sfd, (char *)dataBuf, dataSize, MSG_DONTWAIT, (struct sockaddr *)&stSockAddrDst, &sockLen);
#endif
	if(-1 == ret)
	{
#ifdef _WIN64
		const unsigned int wsaErrno = WSAGetLastError();
		switch(wsaErrno)
		{
			case 10040:
				break;
			default:
				//cerr << "Fail to call UdpSocket::recvNonblock(). errno = " << wsaErrno << endl;
				break;
		}
#elif defined(__linux__)
		//cerr << "Fail to call UdpSocket::recvNonblock(). " << strerror(errno) << endl;
#endif
	}

#ifdef _WIN64
	noBlock = 0;
	ioctlsocket(sfd, FIONBIO, &noBlock);
#elif defined(__linux__)
#endif
	//cout << "Call UdpSocket::recvNonblock() end." << endl;
	return ret;
}

/*
	功能：	以非阻塞形式从队列缓存中窥探一帧UDP 数据，而不从队列中移除。从而后续的recv() 调用可以从队列中再次获取数据。
	返回：	成功，返回字节数；超时无数据，-2；失败，返回-1.
	注意：	
*/
int UdpSocket::peek(void *const dataBuf, const int dataSize, char *ipAddr, const unsigned int ipAddrLen)
{
	//cout << "Call UdpSocket::peek()." << endl;
	//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

	int ret = 0;
#ifdef _WIN64
	unsigned int wsaErrno = 0;
	unsigned long noBlock = 1;
	ioctlsocket(sfd, FIONBIO, &noBlock);
	int sockLen = 0;
	sockLen = sizeof(struct sockaddr_in);
	ret = recvfrom(sfd, (char *)dataBuf, dataSize, MSG_PEEK, (struct sockaddr *)&stSockAddrDst, &sockLen);
#elif defined(__linux__)
	socklen_t sockLen = 0;
	sockLen = sizeof(struct sockaddr_in);
	ret = recvfrom(sfd, (char *)dataBuf, dataSize, MSG_PEEK | MSG_DONTWAIT, (struct sockaddr *)&stSockAddrDst, &sockLen);
#endif
	memset(ipAddr, 0, ipAddrLen);
	inet_ntop(AF_INET, &stSockAddrDst.sin_addr.s_addr, ipAddr, ipAddrLen);
	//cout << stSockAddrDst.sin_addr.s_addr << ", " << ntohs(stSockAddrDst.sin_port) << endl;
	//cout << "ipAddr = " << ipAddr << endl;
	if(-1 == ret)
	{
#ifdef _WIN64
		wsaErrno = WSAGetLastError();
		switch(wsaErrno)
		{
			case 10040:
				//cerr << "Fail to call UdpSocket::peek(). errno = " << wsaErrno << endl;
				break;
			case 10035:		// 无数据
				//cerr << "Fail to call UdpSocket::peek(). errno = " << wsaErrno << endl;
				ret = -2;
				break;
			default:
				cerr << "Fail to call UdpSocket::peek(). errno = " << wsaErrno << endl;
				break;
		}
#elif defined(__linux__)
		switch(errno)
		{
			case 11:
				//cerr << "Fail to call UdpSocket::peek(). errno = " << errno << ", " << strerror(errno) << endl;
				ret = -2;
				break;
			default:
				cerr << "Fail to call UdpSocket::peek(). errno = " << errno << ", " << strerror(errno) << endl;
				break;
		}

#endif
	}

#ifdef _WIN64
	noBlock = 0;
	ioctlsocket(sfd, FIONBIO, &noBlock);
#elif defined(__linux__)
#endif

	//cout << "Call UdpSocket::peek() end." << endl;
#ifdef _WIN64
	return ret;
#elif defined(__linux__)
	return ret;
#endif

}

/*
	功能：	以非阻塞形式从队列缓存中移除一帧UDP 数据。
	返回：	成功，返回字节数；失败，返回-1；
	注意：	
*/
int UdpSocket::relinquish()
{
	//cout << "Call UdpSocket::relinquish()." << endl;
	//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
	int ret = 0;
	char tmp[65536] = {0};
#ifdef _WIN64
	unsigned long noBlock = 1;
	ioctlsocket(sfd, FIONBIO, &noBlock);
	int sockLen = 0;
	sockLen = sizeof(struct sockaddr_in);
	ret = recvfrom(sfd, tmp, 65536, 0, (struct sockaddr *)&stSockAddrDst, &sockLen);
#elif defined __linux__
	socklen_t sockLen = 0;
	sockLen = sizeof(struct sockaddr_in);
	ret = recvfrom(sfd, tmp, 65536, MSG_DONTWAIT, (struct sockaddr *)&stSockAddrDst, &sockLen);
#endif
	if(-1 == ret)
	{
		//cerr << "Fail to call UdpSocket::relinquish(). " << strerror(errno) << endl;
	}

#ifdef _WIN64
	noBlock = 0;
	ioctlsocket(sfd, FIONBIO, &noBlock);
#endif
	//cout << "Call UdpSocket::relinquish() end." << endl;
	return ret;
}

/*
	功能：	检测套接字是否超时仍未收到数据。
	返回：	超时，返回1; 未超时，返回0.
	注意：	
*/
int UdpSocket::checkTimeOutMs(unsigned int millisecond)
{
	struct timeval stTimeout;
	memset(&stTimeout, 0, sizeof(struct timeval));
#if 1
	stTimeout.tv_sec = millisecond / 1000;				// 秒
	stTimeout.tv_usec = (millisecond % 1000) * 1000;	// 微秒
#else
	stTimeout.tv_sec = 1;		// 秒
	stTimeout.tv_usec = 0;		// 微秒
#endif

	fd_set fdSet;
	FD_ZERO(&fdSet);
	FD_SET(sfd, &fdSet);

	// 在windows 中，第一个参数被忽略，可以传为0. 但在linux 必须为文件描述符最大值+1, 曾因传0 而造成程序逻辑不通。
	int ret = 0;
	ret = select(sfd + 1, &fdSet, NULL, NULL, &stTimeout);
	if(-1 == ret)		// 出错
	{
		cerr << "select error." << endl;
		return 0;
	}
	else if(0 == ret)	// 超时
	{
		return 1;
	}
	else				// 有文件描述符可读写
	{
		return 0;
	}
}

/*
	功能：	设置套接字接收缓存区大小。
	返回：	成功，返回0; 失败，返回非0.
	注意：	
*/
int UdpSocket::setRecvBufLen(const unsigned int recvBufLen)
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
		cerr << "Fail to get default receive buf size in UdpSocket::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default receive buf size in UdpSocket::setRecvBufLen(). errno = " << errno << endl;
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
		cerr << "Fail to set receive buf size in UdpSocket::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to set receive buf size in UdpSocket::setRecvBufLen(). errno = " << errno << endl;
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
		cerr << "Fail to get default receive buf size in UdpSocket::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default receive buf size in UdpSocket::setRecvBufLen(). errno = " << errno << endl;
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
int UdpSocket::setSendBufLen(const unsigned int sendBufLen)
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
		cerr << "Fail to get default send buf size in UdpSocket::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default send buf size in UdpSocket::setRecvBufLen(). errno = " << errno << endl;
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
		cerr << "Fail to set send buf size in UdpSocket::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to set send buf size in UdpSocket::setRecvBufLen(). errno = " << errno << endl;
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
		cerr << "Fail to get default send buf size in UdpSocket::setRecvBufLen(). errno = " << WSAGetLastError() << endl;
#elif defined(__linux__)
		cerr << "Fail to get default send buf size in UdpSocket::setRecvBufLen(). errno = " << errno << endl;
#endif
	}
	else
	{
		cout << "Now socket send buffer size = " << defaultBufSize << endl;
	}

	return ret;
}

