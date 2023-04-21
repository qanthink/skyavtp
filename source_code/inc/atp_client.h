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

class AvtpAudioClient
{
public:
	AvtpAudioClient(const char *serverIp);
	~AvtpAudioClient();

	int sendAudioFrame(const void *frameBuf, const unsigned int frameBufLen);
	int recvAudioFrame(void *frameBuf, const unsigned int frameBufLen);
	bool isAllowTalking(){return bAllowTalking;};
	AvtpAudioClient *getInstance(){return this;};

private:
	std::shared_ptr<UdpClient> pUdpClient = NULL;	// 指向UDP Socket 对象。
	const unsigned short avtpPort = ATP_PORT;		// AVTP 端口号
	
	// 服务器接收线程，用于监听客户端的消息。
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThServerRecv = NULL;

	bool bRunning = false;							// 运行状态标志。
	bool bAllowTalking = false;						// 允许对讲。
	
	unsigned int mTimeOutMs = 5000;					// 超时时间。
	std::mutex mMtx;								// 互斥量
	audioFrame_t audioFrame;						// 音频帧数据
	
	const unsigned int queueDepths = 8;				// 音频队列
	MyQueue<audioFrame_t> audioQueue;
};

