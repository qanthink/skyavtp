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

class AudioClientProc
{
public:
	AudioClientProc();
	~AudioClientProc();

	sem_t sem;
	MyQueue<audioFrame_t> frameQueue;
	const unsigned int queueDepths = 8;

	int popFrame(void *frameBuf, const unsigned int frameBufLen);

private:
};


class AvtpAudioServer
{
public:
	AvtpAudioServer(const char *serverIp);
	~AvtpAudioServer();

	bool addClient(std::string clientIp);
	bool deleteClient(std::string clientIp);
	bool queryClient(std::string clientIp);

	int sendMessage(std::string clientIp, avtpCmd_t *pAvtpCmd);
	int sendAudioFrame(std::string clientIp, const void *frameBuf, const unsigned int frameBufLen);
	int recvAudioFrame(std::string clientIp, void *frameBuf, const unsigned int frameBufSize);

private:
	std::shared_ptr<UdpServer> pUdpServer = NULL;	// 指向UDP Socket 对象。
	const unsigned short avtpPort = ATP_PORT;		// AVTP 端口号
	std::map<std::string, std::shared_ptr<AudioClientProc>> clientPool;	// 客户端池。
	
	// 服务器接收线程，用于监听客户端的消息。
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThServerRecv = NULL;

	bool bRunning = false;							// 运行状态标志。
	unsigned int mTimeOutMs = 5000;					// 超时时间。
	std::mutex mMtx;								// 互斥量
	audioFrame_t audioFrame;						// 音频帧数据
};

