/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#include <map>
#include <thread>
#include <semaphore.h>
#include <condition_variable>

#include "myqueue.h"
#include "udp_server.h"
#include "avtp_datatype.h"

class ClientProc
{
public:
	ClientProc();
	~ClientProc();

	sem_t sem;
	// FHD20, bitRata=800Kbps, maxIframeSize=73KB.
	const unsigned int maxIframeSize = 64 * 1024;
	//const unsigned int maxIframeSize = 128 * 1024;
	//const unsigned int maxIframeSize = 256 * 1024;
	// queueDepths = maxIframeSize / videoSlice_t::sliceBufMaxSize;
	const unsigned int queueDepths = maxIframeSize / videoSlice_t::sliceBufMaxSize;
	MyQueue<videoSlice_t> sliceQueue;

	//int pushSlice(const videoSlice_t *pVideoSlice);
	int popFrame(void *frameBuf, const unsigned int frameBufLen);
	//bool isGroupFull();

private:
	videoSliceGroup_t videoSliceGroup;
	unsigned int expFrameID = 0;
};


class AvtpVideoServer
{
public:
	AvtpVideoServer(const char *serverIp);
	~AvtpVideoServer();

	bool addClient(std::string clientIp);
	bool deleteClient(std::string clientIp);
	bool queryClient(std::string clientIp);

	int recvVideoFrame(std::string clientIp, void *frameBuf, const unsigned int frameBufSize);

private:
	std::shared_ptr<UdpServer> pUdpServer = NULL;	// 指向UDP Socket 对象。
	const unsigned short avtpPort = AVTP_PORT;		// AVTP 端口号
	std::map<std::string, std::shared_ptr<ClientProc>> clientPool;	// 客户端池。
	
	// 服务器接收线程，用于监听客户端的消息。
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThServerRecv = NULL;

	bool bRunning = false;							// 运行状态标志。
	unsigned int mTimeOutMs = 5000;					// 超时时间。
	std::mutex mMtx;								// 互斥量
};


