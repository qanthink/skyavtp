#pragma once

#include <atomic>
#include <thread>
#include <string>
#include "udpsocket.h"

#define AVTP_PORT 1995

/*
	概念约定：
	====帧Frame ：H.26X 的I 帧和P 帧，以及MJPEG 的每个帧，都为帧。
	====片Slice ：把帧数据分割成若干份，每份中再补充与协议相关的私有数据，则形成了一个一个片数据Slice.
	====片组SliceGroup: 一帧Frame 分割出来的若干片Slice, 形成了一个片组SliceGroup.
*/

/*
	传输协议的数据类型，位于一包数据中的第一个int 字节中。
	不同类型对应不同的数据包格式，需要不同的解析策略。
*/
class avtpDataType{
public:
	const static unsigned int TYPE_INVALID = 0;
	
	const static unsigned int TYPE_CMD_ACK = 1;
	const static unsigned int TYPE_CMD_ReqHand = 2;
	const static unsigned int TYPE_CMD_AgeHand = 3;
	const static unsigned int TYPE_CMD_ReqNextFrm = 4;
	
	const static unsigned int TYPE_AV_AUDIO = 8;
	const static unsigned int TYPE_AV_VIDEO = 9;
};

/*	TYPE 决定了后续数据的排列格式：
	TYPE_INVALID:
		后续数据无效，直接丢弃。
		
	TYPE_CMD_ACK:
		avtpData[0] 为frameID.
		avtpData[1] 为sliceSeq.
		avtpData[2] 预留.

	TYPE_CMD_ReqHand:
		avtpData[0] 预留.
		avtpData[1] 预留.
		avtpData[2] 预留.

	TYPE_CMD_AgeHand:
		avtpData[0] 预留.
		avtpData[1] 预留.
		avtpData[2] 预留.

	TYPE_CMD_ReqNextFrm:
		avtpData[0] frameID.
		avtpData[1] 预留.
		avtpData[2] 预留.
*/

class avtpCmd_t{
public:
	unsigned int avtpDataType = avtpDataType::TYPE_INVALID;
	unsigned int avtpData[3] = {0};
};

/*
	case TYPE_AV_VIDEO:
	
	UDP包的正文内容，需要限制在548 Bytes以内(Internet环境)，或1472 Bytes 以内(局域网环境)。
	在一片Slice 中，一部分字节用作了私有协议头，所以留给视频数据的空间不足548(或1472) Bytes.
	本协议约定私有数据占36 Bytes字节，则视频数据的长度为512(或1436) Bytes.
*/
class videoSlice_t{
public:
	//const static unsigned int sliceBufMaxSize = 512;
	//const static unsigned int sliceBufMaxSize = 1436;
	//const static unsigned int sliceBufMaxSize = 2000;
	const static unsigned int sliceBufMaxSize = 4000;
	//const static unsigned int sliceBufMaxSize = 6100;
	//const static unsigned int sliceBufMaxSize = 8100;
	//const static unsigned int sliceBufMaxSize = 12200;
	//const static unsigned int sliceBufMaxSize = 16300;

public:
	unsigned int avtpDataType = avtpDataType::TYPE_INVALID;	// 第1个int: 数据类型。
	unsigned int frameID = 0;								// 第2个int: 帧ID, 唯一标识一个Frame.
	unsigned int frameSize = 0;								// 第3个int: 帧长度。
	unsigned int sliceSeq = 0;								// 第4个int: 片序号，决定了在一个Frame 中的位置。
	unsigned int sliceSize = 0;								// 第5个int: 片大小。
	unsigned int reserve0 = 0;								// 第6个int: 保留。
	unsigned int reserve1 = 0;								// 第7个int: 保留。
	unsigned int reserve2 = 0;								// 第8个int: 保留。
	unsigned int reserve3 = 0;								// 第9个int: 保留。
	unsigned char sliceBuf[sliceBufMaxSize];				// 第10个int起: 视频数据。
};

/*
	片组：包含多个slice, 在接收端可以解析重组为一帧Frame.
	
	H.265 FHD30 的码率通常在2Mbps. I帧大小通常不超过64KB.
	H.265 4K30 对应8Mbps, I帧 256KB. 故而暂且认为512KB 的空间足够容纳一帧图像。
	把一帧Frame图像打包，拆分成若干Slice, 每个Slice中的有效数据为512B(或1436B), 
	则需要512KB * 1024 / 512B(或1436B) = 1024(366)个Slice.
*/
class videoSliceGroup_t{
public:
	const static unsigned int h26xMaxFrameBytes = 512 * 1024;
	//const static unsigned int groupMaxSize = ceil((double)h26xMaxFrameBytes / videoSlice_t::sliceBufMaxSize);	// 取整
	const static unsigned int groupMaxSize = h26xMaxFrameBytes / videoSlice_t::sliceBufMaxSize + 1;

public:
	videoSlice_t videoSliceGroup[groupMaxSize];
};

/*
	视频传输协议抽象类，作为父类，不可被实例化。
	继承后可以形成子类，server 端和 client 端。server 端用于接收视频数据，client 端发送视频数据。
*/
class AvtpVideoBase {
public:
	AvtpVideoBase(bool bBind, const char *hostIP, const char *destIP, const unsigned short port);
	~AvtpVideoBase();

public:
	int stop();
	int start();
	bool isRunning();						// 运行状态。
	bool isConnected();						// 是否处于连接状态
	
	int recvVideoFrame(void *const frameBuff, const unsigned int frameSize, const char *ipAddr);			// Server: 以阻塞形式从传输协议中获取一帧视频数据。
	int sendVideoFrame(const void *const frameBuff, const unsigned int frameSize, const char *ipAddr);		// Client: 以阻塞形式将一帧视频数据送入传输协议中。
	double getLossRate() const;

protected:
	/* 握手协议、传输应答协议相关函数 */
	int requestHandShake(const char *ipAddr);					// Client: 请求握手。
	int waitHandShakeAgree(unsigned int waitTimeMs = 15, unsigned int waitCnt = 40);				// Client: 等待对方同意握手。
	int agreeHandShake(const char *ipAddr);					// Server: 同意握手。
	
	int reqNextFrm(const char *ipAddr);						// Server: 请求下一帧数据
	int answerSliceAck(const unsigned int frameID, const unsigned sliceSeq, const char *ipAddr);			// Server: 回应ACK.

	/* 打包Frame 和slice 的相关函数，需要原子操作 */
	const unsigned int calculateSliceNum(const unsigned int frameSize) const;			// Server + Client: 计算片组中，片的数量。
	int videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq);		// Server + Client: 清空片数据。
	int videoSliceGroupClear();				// Server + Client: 清空片组数据。
	int videoSliceGroupIsFull();			// Server + Client: 片组判满。
	int videoSliceGroupIsEmpty();			// Server + Client: 片组判空。
	int packingSlice2Frame(void *const frameBuff, const unsigned int bufSize);			// Server: 将片重组为帧。
	int packingFrame2Slice(const void *const frameBuff, const unsigned int frameSize);	// Client: 将帧分解为片。
	int sendSliceGroup(const unsigned int groupSize, const char *ipAddr);									// Client: 发送片组。

	double lossRateCalculator(bool bResend);

	/* 网络收发相关函数 */
	// send();
	// recv();
	// peekfrom();

	/* 状态机和状态打印 */
	void *stateShow(void *arg);
	static void *thStateShow(void *arg);
	virtual void *stateMachine(void *arg) = 0;											// 子类必须实现状态机。
	static void *thSatateMachine(void *arg);

protected:
	const int udpPort = 1989;				// AVTP 协议约定端口号。
	const unsigned int ssTimeMS = 3000;		// 状态显示间隔时间。
	std::string strHostIP;
	std::string strDestIP;
	
	bool bRunning = false;					// Server + Client: 运行状态
	bool bConnected = true;				// Server + Client: 连接状态
	volatile bool bAllowPacking = true;	// Client: 是否允许打包
	unsigned int curFrameID = 0;			// Server + Client: 帧ID
	unsigned int curFrameSize = 0;			// Server + Client: 帧尺寸
	videoSliceGroup_t videoSliceGroup;		// Server + Client: 片组
	float lossRate = 0;

	std::atomic_flag lock = ATOMIC_FLAG_INIT;				// Server + Client: 原子对象，保障原子操作。
	std::shared_ptr<std::thread> pThSM = NULL;				// Server + Client: 状态机线程指针
	std::shared_ptr<std::thread> pThSS = NULL;				// Server + Client: 状态打印线程指针
	std::shared_ptr<UdpSocket> pUdpSocket = NULL;			// Server + Client: UDP 套接字指针
};
