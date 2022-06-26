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
	pUdpClient = make_shared<UdpClient>(serverIP, avtpPort);
	pThClientRecv = make_shared<thread>(thListening, this);
}

int AvtpVideoClient::thListening(void *pThis)
{
	return ((AvtpVideoClient *)pThis)->listening();
}

int AvtpVideoClient::listening()
{
	int sfd = 0;
	sfd = pUdpClient->getSocketFd();

	while(true)
	{
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(sfd, &fdset);

		// 监听套接字，等待客户端数据到来。
		int ret = 0;
		ret = select(sfd + 1, &fdset, NULL, NULL, NULL);
		if(-1 == ret)
		{
			cerr << "Fail to call select(2), " << strerror(errno) << endl;
			return -3;
		}
		else if(0 == ret)
		{
			cout << "select(2) timeout!" << endl;
			return -4;
		}
		else
		{
			// do next;
		}

		// 接收数据。
		avtpCmd_t avtpCmd;
		memset(&avtpCmd, 0, sizeof(avtpCmd_t));
		struct sockaddr_in stAddrClient;
		memset(&stAddrClient, 0, sizeof(struct sockaddr));

		ret = pUdpClient->recv(&avtpCmd, sizeof(avtpCmd_t));
		if(0 == ret)	// 需要完善异常处理。
		{
			
		}

		unsigned int frameID = 0;
		unsigned int seqence = 0;
		if(avtpDataType::TYPE_CMD_ACK == avtpCmd.avtpDataType)
		{
			frameID = avtpCmd.avtpData[0];
			seqence = avtpCmd.avtpData[1];
			cout << "avtpDataType: " << avtpCmd.avtpDataType << ", frame ID: " << frameID << ", sequence: "  << seqence << endl;
			videoSliceClear(frameID, seqence);
		}

		#if 0
		if(avtpDataType::TYPE_CMD_ReqNextFrm == avtpCmd.avtpDataType)
		{
			frameID = avtpCmd.avtpData[0];
			videoSliceGroupClear();
		}
		#endif
	}
}

int AvtpVideoClient::videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq)
{
	//cout << "Call AvtpVideoClient::videoSliceClear()." << endl;

	mMtx.lock();
	if(frameID == videoSliceGroup.videoSlice[sliceSeq].frameID)
	{
		memset(videoSliceGroup.videoSlice + sliceSeq, 0, sizeof(videoSlice_t));
	}
	mMtx.unlock();

	//cout << "Call AvtpVideoClient::videoSliceClear() end." << endl;
	return 0;
}

int AvtpVideoClient::videoSliceGroupClear()
{
	//cout << "Call AvtpVideoClient::videoSliceGroupClear()." << endl;

	while(mLock.test_and_set());		// 获得锁;
	memset(videoSliceGroup.videoSlice, 0, sizeof(videoSlice_t) * videoSliceGroup_t::groupMaxSize);
	mLock.clear();// 释放锁
	
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
	
	memset(&videoSliceGroup, 0, sizeof(videoSliceGroup_t));

	unsigned sliceNum = 0;
	sliceNum = pushFrameIntoGroup(frameBuf, frameBufLen);

	do{
		int i = 0;
		for(i = 0; i < sliceNum; ++i)
		{
			mMtx.lock();
			if(avtpDataType::TYPE_AV_VIDEO == videoSliceGroup.videoSlice[i].avtpDataType)
			{
				cout << "send. " << i << endl;
				pUdpClient->send(videoSliceGroup.videoSlice + i, sizeof(videoSlice_t));
			}
			mMtx.unlock();
			this_thread::sleep_for(chrono::nanoseconds(1));
		}

		this_thread::sleep_for(chrono::milliseconds(100));
	}while(!isGroupEmpty());
	
	mFrameID++;

	//cout << "Call AvtpVideoClient::sendVideoFrame() end." << endl;
	return 0;
}

bool AvtpVideoClient::isGroupEmpty()
{
	cout << "Call AvtpVideoClient::isGroupEmpty()." << endl;
	
	int i = 0;

	unique_lock<mutex> lock(mMtx);
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		if(avtpDataType::TYPE_AV_VIDEO == videoSliceGroup.videoSlice[i].avtpDataType)
		{
			lock.unlock();
			cout << "Call AvtpVideoClient::isGroupEmpty() end. false" << endl;
			return false;
		}
	}
	lock.unlock();
	cout << "Call AvtpVideoClient::isGroupEmpty() end." << endl;
	return true;
}

/*
	功能：	根据Frame 长度和单片slice 的最大长度，计算片数量。
	返回：	返回片数量。
	注意：	原子操作。
*/
const unsigned int AvtpVideoClient::calculateSliceNum(const unsigned int frameSize) const
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

int AvtpVideoClient::pushFrameIntoGroup(const void *frameBuf, const unsigned int frameBufLen)
{
	//cout << "Call AvtpVideoClient::pushFrameIntoGroup()." << endl;

	int i = 0;
	unsigned int cpyBytes = 0;
	const unsigned int sliceNum = calculateSliceNum(frameBufLen);

	unique_lock<mutex> lock(mMtx);
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

		videoSliceGroup.videoSlice[i].avtpDataType = avtpDataType::TYPE_AV_VIDEO;
		videoSliceGroup.videoSlice[i].frameID = mFrameID;
		videoSliceGroup.videoSlice[i].frameSize = frameBufLen;
		videoSliceGroup.videoSlice[i].sliceSeq = i;
		videoSliceGroup.videoSlice[i].sliceSize = thisCpyBytes;
		memcpy(videoSliceGroup.videoSlice[i].sliceBuf, frameBuf + cpyBytes, thisCpyBytes);
		cpyBytes += thisCpyBytes;
	}
	lock.unlock();

	//cout << "Call AvtpVideoClient::pushFrameIntoGroup() end." << endl;
	return sliceNum;
}


