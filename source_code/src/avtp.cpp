/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
*/

#include <iostream>
#include <iomanip>
#include <string.h>
#include <thread>
#include "avtp.h"

using namespace std;

/* 
	功能：	构造抽象类，初始化参数。
	注意：	
*/
AvtpVideoBase::AvtpVideoBase(bool bBind, const char *hostIP, const char *destIP, const unsigned short port)
{
	cout << "Call AvtpVideoBase::AvtpVideoBase()." << endl;
	bRunning = false;
	bConnected = false;
	bAllowPacking = false;
	curFrameID = 0;
	curFrameSize = 0;
	pThSM = NULL;
	pThSS = NULL;
	pUdpSocket = make_shared<UdpSocket>(bBind, hostIP, destIP, port);
	start();
	strHostIP = hostIP;
	strDestIP = destIP;
	cout << "Call AvtpVideoBase::AvtpVideoBase() end." << endl;
}

/*
	功能：	构造抽象类，初始化参数。
	注意：
*/
AvtpVideoBase::~AvtpVideoBase()
{
	cout << "Call AvtpVideoBase::~AvtpVideoBase()." << endl;

	stop();
	bRunning = false;
	bConnected = false;
	bAllowPacking = false;
	curFrameID = 0;
	curFrameSize = 0;
	pThSM = NULL;
	pThSS = NULL;
	lock.clear();
	pUdpSocket = NULL;

	cout << "Call AvtpVideoBase::~AvtpVideoBase() end." << endl;
}

/*
	功能：	开始运行音视频传输协议。
	注意：	内部新建了两个线程，一个线程用于状态机，一个线程用于显示运行状态。
*/
int AvtpVideoBase::start()
{
	cout << "Call AvtpVideoBase::start()." << endl;

	if(bRunning)
	{
		return 0;
	}
	
	bRunning = 1;
	pThSS = make_shared<thread>(thStateShow, this);
	pThSM = make_shared<thread>(thSatateMachine, this);

	cout << "Call AvtpVideoBase::start() end." << endl;
	return 0;
}

/*
	功能：	开始运行音视频传输协议。
	注意：	结束了两个线程，一个线程是状态机，一个线程是用于显示运行状态的。
*/
int AvtpVideoBase::stop()
{
	cout << "Call AvtpVideoBase::stop()." << endl;
	if(!bRunning)
	{
		return 0;
	}

	bRunning = false;
	if(NULL != pThSM)
	{
		pThSM->join();
		pThSM = NULL;
	}

	if(NULL != pThSS)
	{
		pThSS->join();
		pThSS = NULL;
	}

	cout << "Call AvtpVideoBase::stop()." << endl;
	return 0;
}

/*
	功能：	音视频传输协议是否在运行。
	返回：	true, 运行中；false, 未运行；
	注意：	
*/
bool AvtpVideoBase::isRunning()
{
	return bRunning;
}

/*
	功能：	客户端和服务器是否在连接状态。
	返回：	true, 已连接；false, 未连接。
	注意：
*/
bool AvtpVideoBase::isConnected()
{
	return bConnected;
}

/*
	功能：	以阻塞形式将一帧视频数据送入传输协议中。客户端操作。
	返回：	成功，返回一帧数据分成的片数量；失败，返回-1.
*/
int AvtpVideoBase::sendVideoFrame(const void *const frameBuff, const unsigned int frameSize)
{
	//cout << "Call AvtpVideoBase::sendVideoFrame()." << endl;

	if(NULL == frameBuff || 0 == frameSize)
	{
		cerr << "Fail to call AvtpVideoBase::sendVideoFrame()." << endl;
		return -1;
	}

	while(!bAllowPacking)
	{
		this_thread::sleep_for(std::chrono::nanoseconds(1));
	}
	bAllowPacking = false;

	int sliceNum = 0;
	sliceNum = packingFrame2Slice(frameBuff, frameSize);
	if(0 == sliceNum)
	{
		cerr << "Fail to call AvtpVideoBase::packingFrame2Slice()." << endl;
		return -1;
	}

	int ret = 0;
	ret = sendSliceGroup(sliceNum);
	if(0 != ret)
	{
		cerr << "Fail to call AvtpVideoBase::sendSliceGroup()." << endl;
		return -1;
	}

	//cout << "Call AvtpVideoBase::sendVideoFrame() end." << endl;
	return sliceNum;
}

/*
	功能：	以阻塞形式从视频传输协议中获取一帧数据。服务器操作。
	返回：	成功，返回帧大小；失败，返回-1.
*/
int AvtpVideoBase::recvVideoFrame(void *const frameBuff, const unsigned int frameSize)
{
	//cout << "Call AvtpVideoBase::recvVideoFrame()." << endl;

	if(NULL == frameBuff || 0 == frameSize)
	{
		cerr << "Fail to call AvtpVideoBase::recvVideoFrame()." << endl;
		return -1;
	}

	int ret = 0;
	while(bRunning)
	{
		while(lock.test_and_set(std::memory_order_acquire));	// 获得锁;
		if(!bAllowPacking)
		{
			lock.clear();	// 释放锁
			//this_thread::sleep_for(chrono::microseconds(1));	// 尽可能减少等待，避免丢包。
			//this_thread::sleep_for(chrono::nanoseconds(1));	// 尽可能减少等待，避免丢包。
			continue;
		}
		bAllowPacking = false;

		#if 1
		ret = packingSlice2Frame(frameBuff, frameSize);
		if(-1 == ret)
		{
			cerr << "Call packingSlice2Frame(). Out of range." << endl;
			ret = frameSize;
		}
		#endif

		videoSliceGroupClear();
		curFrameID++;
		reqNextFrm();
		lock.clear();	// 释放锁

		break;
	}

	//cout << "Call AvtpVideoBase::recvVideoFrame() end." << endl;
	return ret;
}

/*
	功能：	发送握手包，请求握手。客户端操作。
	返回：	成功，返回0. 失败，返回非0.
	注意：	为了避免打印信息过多，所以将若干次相同的打印信息合并为一次打印。
*/
int AvtpVideoBase::requestHandShake()
{
	static int sk = 0;
	if(25 == ++sk)
	{
		sk = 0;
		cout << "Call AvtpVideoBase::requestHandShake()." << endl;
	}

	int ret = 0;
	avtpCmd_t avtpCmd;
	avtpCmd.avtpDataType = avtpDataType::TYPE_CMD_ReqHand;
	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::requestHandShake()." << endl;
		return ret;
	}

	//cout << "Call AvtpVideoBase::requestHandShake() end." << endl;
	return 0;
}

/*
	功能：	等待握手。客户端操作。
	返回：	成功握手，返回1. 未能握手，返回0.
	注意：	为了避免打印信息过多，所以将若干次相同的打印信息合并为一次打印。
			在发送完握手信号requestHandShake() 之后，等待若干毫秒再接收数据，连续接收若干次。
			关于等待时间，过长会导致数据包丢失；过短，则在网络环境不良时，容易漏收数据包。
			有线局域网下，等待时长的典型值为15ms. 连续等待次数为40 次。
			无线局域网下，尚未测试。
*/
int AvtpVideoBase::waitHandShakeAgree(unsigned int waitTimeMs, unsigned int waitCnt)
{
	static int sk = 0;
	if(1 * 1000 == ++sk * waitTimeMs)
	{
		sk = 0;
		cout << "Call AvtpVideoBase::waitHandShakeAgree()." << endl;
	}

	this_thread::sleep_for(chrono::microseconds(waitTimeMs * 1000));

	int ret = 0;
	avtpCmd_t avtpCmd;
	unsigned int i = 0;
	for(i = 0; i < waitCnt; ++i)
	{
		//this_thread::sleep_for(chrono::milliseconds(100));
		this_thread::sleep_for(chrono::microseconds(waitTimeMs * 1000));
		//cout << "for i = " << i << endl;
		ret = pUdpSocket->recvNonblock(&avtpCmd, sizeof(avtpCmd_t));
		if(-1 == ret)
		{
			static int sj = 0;
			if(1 * 1000 == ++sj * waitTimeMs)
			{
				sj = 0;
				cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoBase::waitHandShakeAgree()." << endl;
			}
			continue;
		}

		if(avtpDataType::TYPE_CMD_AgeHand == avtpCmd.avtpDataType)
		{
			cout << "!!!Shake hand OK!!!\n!!!Shake hand OK!!!" << endl;
			//videoSliceGroupClear();
			bAllowPacking = true;
			bConnected = true;
			return 1;
		}
		else
		{
			//cout << "receive type: " << avtpCmd.avtpDataType << endl;
		}
	}

	//cout << "Call AvtpVideoBase::waitHandShakeAgree() end." << endl;
	return 0;
}

/*
	功能：	同意握手，发送同意握手数据包。服务器操作。
	返回：	返回0.
	注意：	
*/
int AvtpVideoBase::agreeHandShake()
{
	cout << "Call AvtpVideoBase::agreeHandShake()." << endl;

	int ret = 0;
	avtpCmd_t avtpCmd;
	avtpCmd.avtpDataType = avtpDataType::TYPE_CMD_AgeHand;

	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::agreeHandShake()." << endl;
	}

	// 担心响应包丢失，故而两次回应。
	//this_thread::sleep_for(chrono::nanoseconds(1));	// 此处sleep, 导致slice 重复传输。
	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::agreeHandShake()." << endl;
	}

	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::agreeHandShake()." << endl;
	}

	bConnected = true;

	cout << "Call AvtpVideoBase::agreeHandShake() end." << endl;
	return 0;
}

/*
	功能：	请求下一帧。服务器操作。
	返回：	成功发出数据，返回0; 否则返回-1.
	注意：	网络状况不良时，请求包容易丢失，导致客户端停止发送新数据。
			解决措施有二，1. 连续两次发送请求包，增加发送成功率；
			2. 在一段时间未接收到数据时，再次主动发送请求包。
*/
int AvtpVideoBase::reqNextFrm()
{
	//cout << "Call AvtpVideoBase::reqNextFrm()." << endl;
	int ret = 0;
	avtpCmd_t avtpCmd;
	avtpCmd.avtpDataType = avtpDataType::TYPE_CMD_ReqNextFrm;
	avtpCmd.avtpData[0] = curFrameID;

	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::reqNextFrm()." << endl;
		return -1;
	}

	// 实践证明，两次回应能够有效保障Client 收到报文。
	//this_thread::sleep_for(chrono::microseconds(1));	// 此处sleep 导致某一帧重复传输。不清楚为什么不能sleep.
	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::reqNextFrm()." << endl;
		return -1;
	}

	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::reqNextFrm()." << endl;
		return -1;
	}

	//cout << "Call AvtpVideoBase::reqNextFrm() end." << endl;
	return 0;
}

/*
	功能：	回应ACK, 表示已经收到了一片Slice 数据。服务器操作。
	返回：	成功发出数据，返回0; 否则返回-1.
	注意：	网络状况不良时，ACK 包容易丢失，导致客户端重复发送相同Slice 数据。
			解决措施：连续两次发送ACK 包，增加发送成功率；
*/
int AvtpVideoBase::answerSliceAck(const unsigned int frameID, const unsigned sliceSeq)
{
	//cout << "Call AvtpVideoBase::sendSliceAck()." << endl;
	avtpCmd_t avtpCmd;
	memset(&avtpCmd, 0, sizeof(avtpCmd_t));
	avtpCmd.avtpDataType = avtpDataType::TYPE_CMD_ACK;
	avtpCmd.avtpData[0] = frameID;
	avtpCmd.avtpData[1] = sliceSeq;

	int ret = 0;
	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::sendSliceAck()." << endl;
		return -1;
	}

	// 相比重复传包，多传ACK 的效率更好。
	// 实践证明，两次回应能够有效保障Client 收到报文。但也不要三次传ACK.
	//this_thread::sleep_for(chrono::nanoseconds(1));	// 此处sleep, 导致slice 重复传输。不清楚为什么不能sleep 后再重传。
	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::sendSliceAck()." << endl;
		return -1;
	}
#if 0
	ret = pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
	if(-1 == ret)
	{
		cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::sendSliceAck()." << endl;
		return -1;
	}
#endif
	return 0;
	//cout << "Call AvtpVideoBase::sendSliceAck() end." << endl;
}

/*
	功能：	将帧数据打包为片数据。
	返回：	返回片数据的数量。
	注意：	原子操作。
			存在如下异常状况：当frameSize 过大，无法全部存放于sliceGroup 中时，多出数据会被截断。客户端可能收到不完整I 帧。
*/
int AvtpVideoBase::packingFrame2Slice(const void *const frameBuff, const unsigned int frameSize)
{
	//cout << "Call AvtpVideoBase::packingFrame2Slice()." << endl;
	
	unsigned int i = 0;
	unsigned int sliceNum = 0;
	sliceNum = calculateSliceNum(frameSize);
	
	while(lock.test_and_set(std::memory_order_acquire)); 	// 获得锁;	
	for(i = 0; i < sliceNum; ++i)
	{
		videoSliceGroup.videoSliceGroup[i].avtpDataType = avtpDataType::TYPE_AV_VIDEO;
		videoSliceGroup.videoSliceGroup[i].frameID = curFrameID;
		videoSliceGroup.videoSliceGroup[i].frameSize = frameSize;
		videoSliceGroup.videoSliceGroup[i].sliceSeq = i;
		if(sliceNum - 1 == i)
		{
			videoSliceGroup.videoSliceGroup[i].sliceSize = frameSize - videoSlice_t::sliceBufMaxSize * i;
		}
		else
		{
			videoSliceGroup.videoSliceGroup[i].sliceSize = videoSlice_t::sliceBufMaxSize;
		}
		memcpy(videoSliceGroup.videoSliceGroup[i].sliceBuf, (char*)frameBuff + i * videoSlice_t::sliceBufMaxSize, videoSliceGroup.videoSliceGroup[i].sliceSize);
	}
	//videoSliceGroup.videoSliceGroup[i].sliceSize = videoSlice_t::sliceBufMaxSize;
	lock.clear();	// 释放锁

	//cout << "Call AvtpVideoBase::packingFrame2Slice() end." << endl;
	return sliceNum;
}

/*
	功能：	将片数据打包为帧数据。
	返回：	返回帧数据的大小。
	注意：	原子操作。
			存在如下异常状况：当Frame 的长度大于bufSize 时，多出数据会被截断，导致收到不完整的帧数据。
*/
int AvtpVideoBase::packingSlice2Frame(void *const frameBuff, const unsigned int bufSize)
{
	//cout << "Call AvtpVideoBase::packingSlice2Frame()." << endl;

	unsigned int i = 0;
	unsigned int sliceNum = 0;
	sliceNum = calculateSliceNum(curFrameSize);

	unsigned int frameSize = 0;
	for(i = 0; i < sliceNum; ++i)
	{
		frameSize += videoSliceGroup.videoSliceGroup[i].sliceSize;
		//cout << "videoSliceGroup.videoSliceGroup[i].sliceSize = " << videoSliceGroup.videoSliceGroup[i].sliceSize << endl;
		if(frameSize > bufSize)
		{
			unsigned int cpySize = 0;
			cpySize = videoSliceGroup.videoSliceGroup[i].sliceSize - (frameSize - bufSize);
			memcpy((char*)frameBuff + i * videoSlice_t::sliceBufMaxSize, videoSliceGroup.videoSliceGroup[i].sliceBuf, cpySize);

			if(i + 1 == sliceNum)	// 结束时依然没能存下。
			{
				return -1;
			}
			else
			{
				return bufSize;
			}
		}
		memcpy((char*)frameBuff + i * videoSlice_t::sliceBufMaxSize, videoSliceGroup.videoSliceGroup[i].sliceBuf, videoSliceGroup.videoSliceGroup[i].sliceSize);
	}

	//cout << "Call AvtpVideoBase::packingSlice2Frame() end." << endl;
	return frameSize;
}

/*
	功能：	根据Frame 长度和单片slice 的最大长度，计算片数量。
	返回：	返回片数量。
	注意：	原子操作。
*/
const unsigned int AvtpVideoBase::calculateSliceNum(const unsigned int frameSize) const
{
	unsigned int sliceNum = 0;
	sliceNum = frameSize / videoSlice_t::sliceBufMaxSize;

	if(0 == frameSize % videoSlice_t::sliceBufMaxSize)
	{
		return sliceNum;
	}
	else
	{
		return sliceNum + 1;
	}
}

/*
	功能：	根据帧ID 和片序列，清除片组中的片数据。
	返回：	返回0.
	注意：	客户端需要原子操作；服务器端暂时没有用到该操作，如果用到，可能不需要原子操作。
			清空时，原则上只需要将slice 的avtpDataType 成员，置为avtpDataType::TYPE_INVALID. 
			不建议用memset 清空整个slice. 有时间消耗。
*/
#if 1
int AvtpVideoBase::videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq)
{
	//cout << "Call AvtpVideoBase::videoSliceClear()." << endl;

	if(sliceSeq > videoSliceGroup_t::groupMaxSize)
	{
		cerr << "Fail to call AvtpVideoBase::videoSliceClear(). Argument is out of range()." << endl;
		cerr << "sliceSeq = " << sliceSeq << ", videoSliceGroup_t::groupMaxSize = " << videoSliceGroup_t::groupMaxSize << endl;
	}
	
	while(lock.test_and_set(std::memory_order_acquire));		// 获得锁;

	if(frameID == videoSliceGroup.videoSliceGroup[sliceSeq].frameID)
	{
		videoSliceGroup.videoSliceGroup[sliceSeq].avtpDataType = avtpDataType::TYPE_INVALID;
		videoSliceGroup.videoSliceGroup[sliceSeq].frameID = 0;
		videoSliceGroup.videoSliceGroup[sliceSeq].frameSize = 0;
		videoSliceGroup.videoSliceGroup[sliceSeq].sliceSeq = 0;
		videoSliceGroup.videoSliceGroup[sliceSeq].sliceSize = 0;
		#if 0
		memset(videoSliceGroup.videoSliceGroup[i].sliceBuf, 0, videoSlice_t::sliceBufMaxSize);
		#endif
	}

	lock.clear();// 释放锁
	
	//cout << "Call AvtpVideoBase::videoSliceClear() end." << endl;
	return 0;
}
#endif

/*
	功能：	清空片组。
	返回：	返回0.
	注意：	客户端不需要原子操作；服务器端则需要原子操作，但由于优化了逻辑，故而暂时可以不用原子操作。
			清空时，原则上只需要将slice 的avtpDataType 成员，置为avtpDataType::TYPE_INVALID. 
			不建议用memset 清空整个slice. 有时间消耗。
*/
#if 1
int AvtpVideoBase::videoSliceGroupClear()
{
	//cout << "Call AvtpVideoBase::videoSliceGroupClear()." << endl;
	//while(lock.test_and_set(std::memory_order_acquire));		// 获得锁;
	
	unsigned int i = 0;
	for(i = 0; i < videoSliceGroup.groupMaxSize; ++i)
	{
		videoSliceGroup.videoSliceGroup[i].avtpDataType = avtpDataType::TYPE_INVALID;
		videoSliceGroup.videoSliceGroup[i].frameID = 0;
		videoSliceGroup.videoSliceGroup[i].frameSize = 0;
		videoSliceGroup.videoSliceGroup[i].sliceSeq = 0;
		videoSliceGroup.videoSliceGroup[i].sliceSize = 0;
		#if 0
		memset(videoSliceGroup.videoSliceGroup[i].sliceBuf, 0, videoSlice_t::sliceBufMaxSize);
		#endif
	}

	//lock.clear();// 释放锁
	
	//cout << "Call AvtpVideoBase::videoSliceGroupClear() end." << endl;
	return 0;
}
#endif

/*
	功能：	片组判满。片组中的slice 能够组成一帧Frame 时，则为满。
	返回：	满，返回true; 未满，返回false.
	注意：	并非判断片组中的所有片都为TYPE_AV_VIDEO, 而是判断若干slice 能否组成完整的一帧Frame.
*/
int AvtpVideoBase::videoSliceGroupIsFull()
{
	//cout << "Call AvtpVideoClient::videoSliceGroupIsFull()." << endl;

	if(avtpDataType::TYPE_AV_VIDEO != videoSliceGroup.videoSliceGroup[0].avtpDataType)
	{
		//lock.clear();// 释放锁
		return 0;
	}

	curFrameSize = videoSliceGroup.videoSliceGroup[0].frameSize;

	int i = 0;
	int sliceNum = 0;
	sliceNum = calculateSliceNum(curFrameSize);
	for(i = 0; i < sliceNum; ++i)
	{
		if(avtpDataType::TYPE_AV_VIDEO != videoSliceGroup.videoSliceGroup[i].avtpDataType)
		{
			//cout << "false." << endl;
			return false;
		}
	}

	//cout << "true." << endl;
	//cout << "Call AvtpVideoClient::videoSliceGroupIsFull() end." << endl;
	return true;
}

/*
	功能：	片组判空。所有的slice 都没有装载TYPE_AV_VIDEO 类型的数据，则为空。
	返回：	空，返回true; 非空，返回false.
	注意：	
*/
int AvtpVideoBase::videoSliceGroupIsEmpty()
{
	//cout << "Call AvtpVideoBase::videoSliceGroupIsEmpty()." << endl;

	while(lock.test_and_set(std::memory_order_acquire));		// 获得锁;
	
	unsigned int i = 0;
	for(i = 0; i < videoSliceGroup.groupMaxSize; ++i)
	{
		if(avtpDataType::TYPE_AV_VIDEO == videoSliceGroup.videoSliceGroup[i].avtpDataType)
		{
			//cout << "false." << endl;
			lock.clear();// 释放锁
			return 0;
		}
	}

	lock.clear();	// 释放锁

	//cout << "true." << endl;
	//cout << "Call AvtpVideoBase::videoSliceGroupIsEmpty() end." << endl;
	return 1;
}

/*
	功能：	使用网络套接字发送片组。
	返回：	返回0.
	注意：
*/
int AvtpVideoBase::sendSliceGroup(const unsigned int groupSize)
{
	//cout << "Call AvtpVideoBase::sendSliceGroup()." << endl;

	bool bResend = false;
	while(!videoSliceGroupIsEmpty())
	{
		if(!bConnected)
		{
			this_thread::sleep_for(chrono::microseconds(100));
			continue;
		}
	
		unsigned int i = 0;
		while(lock.test_and_set(std::memory_order_acquire));		// 获得锁;
		//cout << "for, group Size = " << groupSize << endl;
		for(i = 0; i < groupSize; ++i)
		{
			if(bAllowPacking)	// 意味着允许打包下一帧数据，则跳出循环，放弃本次发送。
			{
				cout << "In AvtpVideoBase::sendSliceGroup(), 'bAllowPacking == true' means we can send next frame." << endl;
				lock.clear();	// 释放锁
				return 0;
			}
		
			if(avtpDataType::TYPE_AV_VIDEO == videoSliceGroup.videoSliceGroup[i].avtpDataType)
			{
				int ret = 0;
				ret = pUdpSocket->send(videoSliceGroup.videoSliceGroup + i, sizeof(videoSlice_t));
				if(-1 == ret)
				{
					cerr << "Fail to call pUdpSocket->send() in AvtpVideoBase::sendSliceGroup()." << endl;
				}

				lossRateCalculator(bResend);
				this_thread::sleep_for(chrono::microseconds(1));	// 间隔发送，减小丢包率。
				if(bResend)
				{
					//cout << "i = " << i << ", size = " << videoSliceGroup.videoSliceGroup[i].frameSize << endl;
				}
			}
			else
			{
				//cout << "TYPE_AV_VIDEO != videoSliceGroup[i].avtpDataType. i = " << i << endl;
			}
		}
		lock.clear();	// 释放锁
		// 帧率需要与sleep 时间匹配，也即帧率需要与网络环境关联。如果收发延迟峰值60ms, 则最大帧率为100/60=15
		//this_thread::sleep_for(chrono::milliseconds(250));	// test, ubuntu ok
		//this_thread::sleep_for(chrono::milliseconds(125));	// test, ubuntu ok
		//this_thread::sleep_for(chrono::milliseconds(75));	// test, ubuntu lossRate = 0.0195122
		//this_thread::sleep_for(chrono::milliseconds(30));	// test, ubuntu lossRate = 0.0043
		this_thread::sleep_for(chrono::milliseconds(50));	// FPS = 1000 / 50 = 20.
		//this_thread::sleep_for(chrono::milliseconds(15));	// FPS = 1000 / 15 = 66.67.
		//this_thread::sleep_for(chrono::milliseconds(30));		// FPS = 1000 / 30 = 33.33
		//this_thread::sleep_for(chrono::milliseconds(5));		// FPS = 1000 / 5 = 200.

		bResend = true;
	}
	
	//cout << "Call AvtpVideoBase::sendSliceGroup() end." << endl;
	return 0;
}

/*
	功能：	丢包率计算器。
	返回：	返回丢包率.
	注意：
*/
double AvtpVideoBase::lossRateCalculator(bool bResend)
{
	static unsigned int sendCnt = 0;
	static unsigned int resendCnt = 0;

	if(bResend)
	{
		resendCnt++;
	}

	sendCnt++;
	if(sendCnt > 1024)
	{
		lossRate = (float)((double)resendCnt / sendCnt);
		sendCnt = 0;
		resendCnt = 0;
	}

	return lossRate;
}

/*
	功能：	获取丢包率
	返回：	
	注意：
*/
double AvtpVideoBase::getLossRate() const
{
	return lossRate;
}

#if 0
void *AvtpVideoBase::stateMachine(void *arg)
{
	while(bRunning)
	{
		if(!bConnected)
		{
			requestHandShake();
			waitHandShakeAgree();
			continue;
		}

		this_thread::sleep_for(chrono::microseconds(1));
		
		int ret = 0;
		avtpCmd_t avtpCmd;
		memset(&avtpCmd, 0, sizeof(avtpCmd_t));
		#if 0	// old 无重连
		ret = pUdpSocket->peek(&avtpCmd, sizeof(avtpCmd_t));
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpSocket->peek() in AvtpVideoBase::stateMachine()." << endl;
			continue;
		}
		//cout << "Peek Data. Type = " << avtpCmd.avtpDataType << endl;
		#endif	// new reconect
		#if 1
		static unsigned int sTimeOutCnt = 0;					// 超时次数。
		const unsigned int timeOutMs = 100;						// 单次超时时长。
		const unsigned int totalTimeOutMs = 5 * 1000;			// 总超时时长。

		#if 1
		ret = pUdpSocket->checkTimeOutMs(timeOutMs);			// n * 1000 = n MS.
		if(1 == ret)
		{
			//cout << "Avtp client timeout." << endl;
			sTimeOutCnt++;
			if(timeOutMs * sTimeOutCnt > totalTimeOutMs)		// 超时重连
			{
				cerr << "timeOutMs * sTimeOutCnt > totalTimeOutMs. bConnected = 0." << endl;
				bConnected = false;
				sTimeOutCnt = 0;
				curFrameID = 0;
				continue;
			}

			//continue;
		}
		else
		{	// 未超时则清空计数器，并继续执行剩余业务逻辑。
			//cout << "Reset timeout counter." << endl;
			sTimeOutCnt = 0;
		}
		#endif
		#endif

		ret = pUdpSocket->peek(&avtpCmd, sizeof(avtpCmd_t));
		if(-1 == ret)
		{
			//cerr << "Fail to call pUdpSocket->peek() in AvtpVideoBase::stateMachine()." << endl;
			continue;
		}
		//cout << "Peek Data. Type = " << avtpCmd.avtpDataType << endl;
		
		switch(avtpCmd.avtpDataType)
		{
			case avtpDataType::TYPE_CMD_ReqNextFrm:
			{
				// 若以阻塞方式接收，可能导致server 掉线后，client 一直阻塞。
				ret = pUdpSocket->recvNonblock(&avtpCmd, sizeof(avtpCmd_t));
				if(-1 == ret)
				{
					cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoBase::stateMachine()." << endl;
					break;
				}
				
				if(avtpCmd.avtpData[0] == curFrameID)
				{
					#if 0
					cout << "avtpCmd.avtpData[0] == curFrameID" << endl;
					//videoSliceGroupClear();
					#endif
				}
				else
				{
					//cout << "avtpCmd.avtpData[0] != curFrameID, avtpData[0] = " << avtpCmd.avtpData[0] << endl;
					//videoSliceGroupClear();
					curFrameID = avtpCmd.avtpData[0];
					bAllowPacking = true;
				}
				break;
			}
			case avtpDataType::TYPE_CMD_ACK:
			{
				ret = pUdpSocket->recvNonblock(&avtpCmd, sizeof(avtpCmd_t));
				if(-1 == ret)
				{
					cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoBase::stateMachine()." << endl;
					break;
				}

				#if 0
				cout << "avtpCmd.avtpDataType = " << avtpCmd.avtpDataType << endl;
				cout << "avtpCmd.avtpData[0] = " << avtpCmd.avtpData[0] << endl;
				cout << "avtpCmd.avtpData[1] = " << avtpCmd.avtpData[1] << endl;
				cout << "avtpCmd.avtpData[2] = " << avtpCmd.avtpData[2] << endl;
				#endif
				
				//if(avtpCmd.avtpData[0] == curFrameID)
				{
					unsigned int sliceSeq = 0;
					unsigned int frameID = 0;
					frameID = avtpCmd.avtpData[0];
					sliceSeq = avtpCmd.avtpData[1];
					videoSliceClear(frameID, sliceSeq);
				}
				//else
				{
					// 丢弃。
				}
				break;
			}

			default:
				//cerr << "Error in AvtpVideoBase::stateMachine(). AVTP_TYPE is unrecognized. TYPE = " << avtpCmd.avtpDataType << endl;
				pUdpSocket->relinquish();
				break;
		}
	}

	return NULL;
}
#endif

/*
	功能：	启用状态机的线程。
	返回：	
	注意：	为了在类内建立线程，并且线程能够访问类成员，故而需要两步操作。
*/
void *AvtpVideoBase::thSatateMachine(void *arg)
{
	AvtpVideoBase *pThis = (AvtpVideoBase *)arg;
	this_thread::sleep_for(chrono::microseconds(10));	// 如果不sleep, windows server 端总是运行崩溃，不清楚原因。sleep 时间不足也会崩溃。
	return pThis->stateMachine(NULL);
}

/*
	功能：	启用状态打印线程。
	返回：
	注意：	为了在类内建立线程，并且线程能够访问类成员，故而需要两步操作。
*/
void *AvtpVideoBase::thStateShow(void *arg)
{
	AvtpVideoBase *pThis = (AvtpVideoBase *)arg;
	this_thread::sleep_for(chrono::microseconds(10));
	return pThis->stateShow(NULL);
}

/*
	功能：	状态打印线程。
	返回：
	注意：	为了在类内建立线程，并且线程能够访问类成员，故而需要两步操作。
*/
void *AvtpVideoBase::stateShow(void *arg)
{
	cout << "Call stateShow()." << endl;
	while(bRunning)
	{
		cout << "Avtp running, connnected, allowpacking status = " << (int)bRunning << ", " << (int)bConnected << ", " << (int)bAllowPacking << endl;
		cout << "Avtp curFrameID = " << curFrameID << ", curFrameSize = " << curFrameSize << ", lossRate = " << setprecision(6) << lossRate << endl;
		cout << "hostIP = " << strHostIP << ", destIP = " << strDestIP << endl;
		this_thread::sleep_for(chrono::microseconds(ssTimeMS * 1000));
	}
	cout << "Call stateShow() end." << endl;
	return NULL;
}
