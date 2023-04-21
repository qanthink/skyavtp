/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#include <thread>
#include <condition_variable>

#include "udp_socket.h"
#include "avtp_datatype.h"

class AvtpVideoClient
{
public:
	AvtpVideoClient(const char *serverIP);
	~AvtpVideoClient();

	double getLossRate();
	int changeFps10s(unsigned int fps);
	int sendVideoFrame(const void *buf, size_t len);

private:
	const char *mServerIP = NULL;					// 服务器地址。
	const unsigned short mAvtpPort = VTP_PORT;		// 传输协议端口。
	std::shared_ptr<UdpSocket> pUdpSocket = NULL;	// 指向UDP Socket 对象。

	// 客户端线程，用于监听服务器发出的指令。
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThClientRecv = NULL;

	const unsigned int calculateSliceNum(const unsigned int frameSize) const;
	bool isGroupEmpty(const unsigned int sliceNum = videoSliceGroup_t::groupMaxSize);
	int videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq);
	int videoSliceGroupClear();
	int pushFrameIntoGroup(const void *frameBuf, const unsigned int frameBufLen);

	bool bRunning = false;				// 运行状态
	unsigned int mFps10s = 100;			// 10 秒内传输的帧数。
	unsigned int mTimeOutMs = 5000;		// 超时时间。
	unsigned int mFrameID = 0;			// 帧ID.
	videoSliceGroup_t videoSliceGroup;	// video slice group.

	unsigned int sendCnt = 0;
	unsigned int resendCnt = 0;
	double lossRate = 0.0;
	
	std::mutex mMtx;					// 互斥量。
};

