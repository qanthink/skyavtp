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

class AudioClientProc
{
public:
	AudioClientProc();
	~AudioClientProc();

	sem_t sem;
	bool bAllowTalking = false;			// 允许对讲状态位。最好通过函数来设置变量。
	MyQueue<audioFrame_t> frameQueue;	// 音频队列。

	bool isAllowTalking(){return bAllowTalking;};
	int popFrame(void *frameBuf, const unsigned int frameBufLen);

private:
	const unsigned int queueDepths = 8;	// 音频队列深度
};

class AvtpAudioServer
{
public:
	AvtpAudioServer(const std::string serverIP);
	~AvtpAudioServer();

	bool addClient(std::string clientIP);
	bool deleteClient(std::string clientIP);
	bool queryClient(std::string clientIP);

	int sendMessage(std::string clientIP, const avtpCmd_t *pAvtpCmd);
	int sendAudioFrame(std::string clientIP, const void *buf, size_t len);
	int recvAudioFrame(std::string clientIP, void *buf, size_t len);

	bool isRunning(){return bRunning;};

private:
	const unsigned short avtpPort = ATP_PORT;		// AVTP 端口号
	std::shared_ptr<UdpSocket> pUdpSocket = NULL;	// 指向UDP Socket 对象。
	std::map<std::string, std::shared_ptr<AudioClientProc>> clientPool;	// 客户端池。
	
	// 服务器接收线程，用于监听客户端的消息。
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThServerRecv = NULL;

	std::mutex mMtx;								// 互斥量
	bool bRunning = false;							// 运行状态标志。
	unsigned int mTimeOutMs = 5000;					// 超时时间。
	audioFrame_t audioFrame;						// 音频帧数据

public:
	decltype(clientPool) *pClientPool = &clientPool;
};

