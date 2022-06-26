/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
*/

#include "avtp_client.h"

#include <iostream>
#include <string.h>

using namespace std;

/*
	功能：	构造函数。
	注意：	
*/
AvtpVideoClient::AvtpVideoClient(const char * serverIP)
{
	bRunning = true;
	pUdpClient = make_shared<UdpClient>(serverIP, avtpPort);
	pThClientRecv = make_shared<thread>(thListening, this);
}

AvtpVideoClient::~AvtpVideoClient()
{
	bRunning = false;

	if(!pThClientRecv)
	{
		pThClientRecv->join();
		pThClientRecv = NULL;
	}
	
	pUdpClient = NULL;
}

int AvtpVideoClient::thListening(void *pThis)
{
	return ((AvtpVideoClient *)pThis)->listening();
}

/*
	功能：	客户端线程，用于监听服务器发出的指令。
	返回：	返回0.
	注意：	
*/
int AvtpVideoClient::listening()
{
	// 获取套接字文件描述，并放入监听列表。
	int sfd = 0;
	sfd = pUdpClient->getSocketFd();
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sfd, &fdset);
	FD_SET(sfd, &fdset);

	// 设置超时时间。
	struct timeval stTimeOut;
	memset(&stTimeOut, 0, sizeof(struct timeval));

	// 避免在循环体内初始化数据，避免时间开销。
	int ret = 0;
	avtpCmd_t avtpCmd;
	struct sockaddr_in stAddrClient;
	memset(&stAddrClient, 0, sizeof(struct sockaddr));
	
	while(bRunning)
	{
		// step1.1 设置监听的套接字
		// step1.2 设置超时时长
		stTimeOut.tv_sec = mTimeOutMs / 1000;
		stTimeOut.tv_usec = mTimeOutMs % 1000 * 1000; // N * 1000 = N ms
		// step1.3 监听套接字，等待客户端数据到来。
		ret = select(sfd + 1, &fdset, NULL, NULL, &stTimeOut);
		if(-1 == ret)
		{
			cerr << "In AvtpVideoClient::listening(). Fail to call select(2), " << strerror(errno) << endl;
			continue;
		}
		else if(0 == ret)
		{
			cout << "In AvtpVideoClient::listening(). Timeout!" << endl;
			continue;
		}
		else
		{
			// do next;
		}

		// step2 接收数据。
		memset(&avtpCmd, 0, sizeof(avtpCmd_t));
		//memset(&stAddrClient, 0, sizeof(struct sockaddr));
		ret = pUdpClient->recv(&avtpCmd, sizeof(avtpCmd_t));
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpClient->recv() in AvtpVideoClient::listening()." << endl;
			continue;
		}
		else if(ret < sizeof(avtpCmd_t))
		{
			cout << "In AvtpVideoClient::listening(). Received data length is not as expected." << endl;
			continue;
		}

		// step3 处理数据。要尽可能快，保障最短时间内再次调用select. 避免UDP 数据堆积导致丢包。
		switch(avtpCmd.avtpDataType)
		{
			case avtpDataType::TYPE_CMD_ACK:
			{
				unsigned int frameID = 0;
				unsigned int seqence = 0;
				frameID = avtpCmd.avtpData[0];
				seqence = avtpCmd.avtpData[1];
				cout << "avtpDataType: " << avtpCmd.avtpDataType << ", frame ID: " << frameID << ", sequence: " << seqence << endl;
				videoSliceClear(frameID, seqence);
				break;
			}
			default:
			{
				cout << "In AvtpVideoClient::listening(). Received bad date." << endl;
				continue;
			}
		}
	}

	return 0;
}

/*
	功能：	清除Video Slice.
	返回：返回0
	注意：	Video Slice 的概念请参考AVTP 数据类型中的描述。
*/
int AvtpVideoClient::videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq)
{
	//cout << "Call AvtpVideoClient::videoSliceClear()." << endl;

	mMtx.lock();
	if(frameID == videoSliceGroup.videoSlice[sliceSeq].frameID)
	{
		videoSliceGroup.videoSlice[sliceSeq].avtpDataType = avtpDataType::TYPE_INVALID;
		//memset(videoSliceGroup.videoSlice + sliceSeq, 0, sizeof(videoSlice_t));
	}
	mMtx.unlock();

	//cout << "Call AvtpVideoClient::videoSliceClear() end." << endl;
	return 0;
}

/*
	功能：	清除Video Slice Group
	返回：返回0
	注意：	Video Slice Group 的概念请参考AVTP 数据类型中的描述。
*/
int AvtpVideoClient::videoSliceGroupClear()
{
	//cout << "Call AvtpVideoClient::videoSliceGroupClear()." << endl;

	mMtx.lock();
	memset(videoSliceGroup.videoSlice, 0, sizeof(videoSlice_t) * videoSliceGroup_t::groupMaxSize);
	mMtx.unlock();
	
	//cout << "Call AvtpVideoClient::videoSliceGroupClear() end." << endl;
	return 0;
}

/*
	功能：	发送视频帧。
	注意：	
*/
int AvtpVideoClient::sendVideoFrame(const void *frameBuf, const unsigned int frameBufLen)
{
	//cout << "Call AvtpVideoClient::sendVideoFrame()." << endl;
	
	//memset(&videoSliceGroup, 0, sizeof(videoSliceGroup_t));	// 为了缩短时间，暂时不做memset.
	unsigned sliceNum = 0;
	sliceNum = pushFrameIntoGroup(frameBuf, frameBufLen);

	int i = 0;
	do{
		for(i = 0; i < sliceNum; ++i)
		{
			mMtx.lock();
			if(avtpDataType::TYPE_AV_VIDEO == videoSliceGroup.videoSlice[i].avtpDataType)
			{
				cout << "send. " << i << endl;
				pUdpClient->send(videoSliceGroup.videoSlice + i, sizeof(videoSlice_t));
			}
			mMtx.unlock();
			//this_thread::sleep_for(chrono::nanoseconds(1));
		}

		this_thread::sleep_for(chrono::milliseconds(10000 / mFps10s));
	}while(!isGroupEmpty(sliceNum));
	
	mFrameID++;

	//cout << "Call AvtpVideoClient::sendVideoFrame() end." << endl;
	return 0;
}

bool AvtpVideoClient::isGroupEmpty(const unsigned int sliceNum)
{
	//cout << "Call AvtpVideoClient::isGroupEmpty()." << endl;
	int i = 0;
	for(i = 0; i < sliceNum; ++i)
	{
		mMtx.lock();
		if(avtpDataType::TYPE_AV_VIDEO == videoSliceGroup.videoSlice[i].avtpDataType)
		{
			mMtx.unlock();
			cout << "Call AvtpVideoClient::isGroupEmpty() end. false" << endl;
			return false;
		}
		mMtx.unlock();
	}
	cout << "Call AvtpVideoClient::isGroupEmpty() end. true" << endl;
	return true;
}

/*
	功能：	根据Frame 长度和单片slice 的最大长度，计算片数量。
	返回：	返回片数量。
	注意：	
*/
const unsigned int AvtpVideoClient::calculateSliceNum(const unsigned int frameSize) const
{
	unsigned int sliceNum = 0;
	sliceNum = frameSize / videoSlice_t::sliceBufMaxSize;

	if(0 != frameSize % videoSlice_t::sliceBufMaxSize)
	{
		++sliceNum;
	}

	if(sliceNum > videoSliceGroup_t::groupMaxSize)
	{
		cerr << "In AvtpVideoClient::calculateSliceNum(). Frame size exceeds limit." << endl;
		sliceNum = videoSliceGroup_t::groupMaxSize;
	}

	return sliceNum;
}

/*
	功能：	将frame 帧放入 slice group.
	返回：	返回片数量。
	注意：	frame, slice 的概念，请参考AVTP 数据类型中的说明。
*/
int AvtpVideoClient::pushFrameIntoGroup(const void *frameBuf, const unsigned int frameBufLen)
{
	//cout << "Call AvtpVideoClient::pushFrameIntoGroup()." << endl;

	int i = 0;
	unsigned int cpyBytes = 0;
	const unsigned int sliceNum = calculateSliceNum(frameBufLen);

	for(i = 0; i < sliceNum; ++i)
	{
		unsigned int thisCpyBytes = 0;
		if(frameBufLen - cpyBytes < videoSlice_t::sliceBufMaxSize)
		{
			thisCpyBytes = frameBufLen - cpyBytes;
		}
		else
		{
			thisCpyBytes = videoSlice_t::sliceBufMaxSize;
		}

		mMtx.lock();
		videoSliceGroup.videoSlice[i].avtpDataType = avtpDataType::TYPE_AV_VIDEO;
		videoSliceGroup.videoSlice[i].frameID = mFrameID;
		videoSliceGroup.videoSlice[i].frameSize = frameBufLen;
		videoSliceGroup.videoSlice[i].sliceSeq = i;
		videoSliceGroup.videoSlice[i].sliceSize = thisCpyBytes;
		memcpy(videoSliceGroup.videoSlice[i].sliceBuf, frameBuf + cpyBytes, thisCpyBytes);
		mMtx.unlock();

		cpyBytes += thisCpyBytes;
	}

	//cout << "Call AvtpVideoClient::pushFrameIntoGroup() end." << endl;
	return sliceNum;
}

/*
	功能：	改变帧率。
	返回：	返回改变后的帧率。
	注意：	1. 该帧率为10秒内帧率，例如5FPS/S -> 50FPS/10S.
			2. 帧率上限为1200 帧每10 秒。
			3. 该帧率为传输协议允许的最大传输帧率，由于帧率还受网络影响，故而实际帧率会小于该值。
*/
int AvtpVideoClient::changeFps10s(unsigned int fps10s)
{
	if(fps10s > 1200)
	{
		mFps10s = 1200;
	}

	return mFps10s;
}

