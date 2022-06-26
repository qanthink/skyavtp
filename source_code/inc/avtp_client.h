/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#include "udp_client.h"
#include "avtp_datatype.h"

#include <memory>
#include <mutex>
#include <condition_variable>

class AvtpVideoClient
{
public:
	AvtpVideoClient(const char *serverIP);
	~AvtpVideoClient();

	int sendVideoFrame(const void *frameBuf, const unsigned int frameBufLen);
	int changeFps10s(unsigned int fps);
	
private:

	std::shared_ptr<UdpClient> pUdpClient = NULL;	// UDP Socket.
	const unsigned short avtpPort = AVTP_PORT;		// 传输协议端口。

	// 客户端线程，用于监听服务器发出的指令。
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThClientRecv = NULL;

	const unsigned int calculateSliceNum(const unsigned int frameSize) const;
	bool isGroupEmpty(const unsigned int sliceNum = videoSliceGroup_t::groupMaxSize);
	int videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq);
	int videoSliceGroupClear();
	int pushFrameIntoGroup(const void *frameBuf, const unsigned int frameBufLen);

	bool bRunning = false;
	unsigned int mFps10s = 100;			// 10 秒内传输的帧数。
	unsigned int mTimeOutMs = 5000;
	unsigned int mFrameID = 0;
	//std::atomic_flag mLock = ATOMIC_FLAG_INIT;		// 原子对象，保障原子操作。
	std::mutex mMtx;
	//std::condition_variable mCondVar;
	
	videoSliceGroup_t videoSliceGroup;
};

