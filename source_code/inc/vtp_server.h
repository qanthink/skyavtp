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
#include "udp_socket.h"
#include "avtp_datatype.h"

class VideoClientProc
{
public:
	VideoClientProc();
	~VideoClientProc();

	sem_t sem;
	MyQueue<videoSlice_t> sliceQueue;

	int popFrame(void *buf, size_t len);

private:
	unsigned int expFrameID = 0;
	videoSliceGroup_t videoSliceGroup;

	// FHD20, bitRata=800Kbps, maxIframeSize=131KB.
	// queueDepths = maxIframeSize / videoSlice_t::sliceBufMaxSize;
	const unsigned int maxIframeSize = 64 * 1024;
	const unsigned int queueDepths = maxIframeSize / videoSlice_t::sliceBufMaxSize;
};

class AvtpVideoServer
{
public:
	AvtpVideoServer(const char *serverIP);
	~AvtpVideoServer();

	bool addClient(std::string clientIp);
	bool deleteClient(std::string clientIp);
	bool queryClient(std::string clientIp);

	int recvVideoFrame(std::string clientIp, void *buf, size_t len);

private:
	const unsigned short avtpPort = VTP_PORT;		// AVTP 端口号
	std::shared_ptr<UdpSocket> pUdpSocket = NULL;	// 指向UDP Socket 对象。
	std::map<std::string, std::shared_ptr<VideoClientProc>> clientPool;	// 客户端池。
	
	// 服务器接收线程，用于监听客户端的消息。
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThServerRecv = NULL;

	std::mutex mMtx;								// 互斥量
	bool bRunning = false;							// 运行状态标志。
	unsigned int mTimeOutMs = 5000;					// 超时时间。
};


