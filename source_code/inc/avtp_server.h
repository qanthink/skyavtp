/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "udp_server.h"
#include "avtp_datatype.h"

class ClientProc
{
public:
	ClientProc();
	~ClientProc();

	int pushSlice(const videoSlice_t *pVideoSlice);
	int popFrame(void *frameBuf, const unsigned int frameBufLen);
	bool isGroupFull();

private:
	std::mutex mMtx;
	//std::unique_lock<std::mutex> lock;
	std::condition_variable mCondVar;
	//std::atomic_flag mLock = ATOMIC_FLAG_INIT;		// 原子对象，保障原子操作。

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
	//std::atomic_flag mLock = ATOMIC_FLAG_INIT;	// 原子对象，保障原子操作。
	std::mutex mMtx;								// 互斥量
};


