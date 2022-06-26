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

	int sendVideoFrame(const void *frameBuf, const unsigned int frameBufLen);
	
private:
	std::shared_ptr<UdpClient> pUdpClient = NULL;
	const unsigned short avtpPort = AVTP_PORT;

	// 客户端接收线程
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThClientRecv = NULL;

	const unsigned int calculateSliceNum(const unsigned int frameSize) const;
	bool isGroupEmpty();
	int videoSliceGroupClear();
	int videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq);
	int pushFrameIntoGroup(const void *frameBuf, const unsigned int frameBufLen);

	unsigned int mFrameID = 0;
	bool bAllowNxtFrame = false;
	std::atomic_flag mLock = ATOMIC_FLAG_INIT;		// 原子对象，保障原子操作。
	std::mutex mMtx;
	std::condition_variable mCondVar;
	
	videoSliceGroup_t videoSliceGroup;
};

