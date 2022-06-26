/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
*/

#include <iostream>
#include <string.h>
#include "avtp_server.h"

using namespace std;

/*
	功能：	构造函数。
	注意：	
*/
AvtpVideoServer::AvtpVideoServer(const char * serverIP)
{
	cout << "Call AvtpVideoServer::AvtpVideoServer()." << endl;
	pUdpServer = make_shared<UdpServer>(serverIP, avtpPort);
	pThServerRecv = make_shared<thread>(thListening, this);
	cout << "Call AvtpVideoServer::AvtpVideoServer() end." << endl;
}

/*
	功能：	发送视频帧。
	注意：	
*/
int AvtpVideoServer::recvVideoFrame(string clientIp, void *frameBuf, const unsigned int frameBufSize)
{
	//cout << "Call AvtpVideoServer::recvVideoFrame()." << endl;

	//requestNxtFrame(clientIp, clientPool[clientIp].frameIdRequest);
	//requestNxtFrame(clientIp, clientPool[clientIp].frameIdRequest);

	int frameRealSize = 0;
	//while(true)
	{
		//if(!isGroupFull(clientIp))
		{
			//this_thread::sleep_for(chrono::nanoseconds(1));
			//continue;
		}

		int i = 0;
		for(i = 0; i < 5; ++i)
		{
			//cout << "frameID = " << clientPool[clientIp].videoSliceGroup.videoSlice[i].frameID << ", seq: " << clientPool[clientIp].videoSliceGroup.videoSlice[i].frameID
		}
		
		//frameRealSize = popFrameFromGrup(clientIp, frameBuf, frameBufSize);
		frameRealSize = clientPool2[clientIp]->pop(frameBuf, frameBufSize);
		//clearGroup(clientIp);
		//++clientPool[clientIp].frameIdRequest;
		//mLock.clear();
		//break;
	}
	
	//cout << "Call AvtpVideoServer::recvVideoFrame() end." << endl;
	return frameRealSize;
}

bool AvtpVideoServer::addClient(string clientIP)
{
	cout << "Call AvtpVideoServer::addClient()." << endl;

	while(mLock.test_and_set()); 	// 获得锁;
	shared_ptr<ClientProc> pClientProc(new ClientProc);
	pair<map<string, shared_ptr<ClientProc>>::iterator, bool> ret;
	ret = clientPool2.insert(make_pair(clientIP, pClientProc));
	pClientProc->create();
	mLock.clear();

	#if 0
	if(false == ret.second)
	{
		cerr << "Fail to add client." << endl;
		return false;
	}
	else
	{
		return true;
	}
	#endif
	return true;
}

bool AvtpVideoServer::deleteClient(string clientIP)
{
	while(mLock.test_and_set()); 	// 获得锁;
	videoSliceGroup_t videoSliceGroup;
	clientPool.erase(clientIP);
	mLock.clear();

	return 0;
}

bool AvtpVideoServer::queryClient(string clientIP)
{
	//cout << "Call AvtpVideoServer::queryClient()." << endl;

	while(mLock.test_and_set()); 	// 获得锁;
	map<string , shared_ptr<ClientProc>>::iterator it;
	it = clientPool2.find(clientIP);
	mLock.clear();
	
	if(clientPool2.end() == it)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int AvtpVideoServer::pushSliceIntoGroup(string clientIp, const videoSlice_t *pVideoSlice)
{
	cout << "Call AvtpVideoServer::pushSliceIntoGroup()." << endl;
	
	while(mLock.test_and_set()); 	// 获得锁;
	clientPool[clientIp].videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
	mLock.clear();

	cout << "Call AvtpVideoServer::pushSliceIntoGroup() end." << endl;
	return 0;
}

int AvtpVideoServer::popFrameFromGrup(string clientIp, void *frameBuf, const unsigned int frameSzie)
{
	//cout << "Call AvtpVideoServer::popFrameFromGrup()." << endl;
	
	//while(mLock.test_and_set()); 	// 获得锁;
	int i = 0;
	unsigned int cpyBytes = 0;
	const videoSlice_t *pVideoSlice = NULL;
	pVideoSlice = clientPool[clientIp].videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		memcpy(frameBuf + cpyBytes, pVideoSlice->sliceBuf, pVideoSlice->sliceSize);
		cpyBytes += pVideoSlice->sliceSize;
		++pVideoSlice;
	}
	//mLock.clear();

	//cout << "Call AvtpVideoServer::popFrameFromGrup() end." << endl;
	return cpyBytes;
}

int AvtpVideoServer::clearGroup(std::string clientIp)
{
	//while(mLock.test_and_set()); 	// 获得锁;
	memset(&clientPool[clientIp], 0, sizeof(videoSliceGroup_t));
	//mLock.clear();
	return 0;
}

int AvtpVideoServer::requestNxtFrame(string clientIp, unsigned int frameID)
{
	avtpCmd_t avtpCmd;
	memset(&avtpCmd, 0, sizeof(avtpCmd_t));
	avtpCmd.avtpDataType = avtpDataType::TYPE_CMD_ReqNextFrm;
	avtpCmd.avtpData[0] = clientPool[clientIp].frameIdRequest;

	struct sockaddr_in stAddrClient;
	memset(&stAddrClient, 0, sizeof(struct sockaddr_in));
	stAddrClient.sin_family = AF_INET;
	stAddrClient.sin_port = htons(AVTP_PORT);
	stAddrClient.sin_addr.s_addr = inet_addr(clientIp.c_str());

	pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);

	return 0;
}

bool AvtpVideoServer::isGroupFull(std::string clientIp)
{
	//while(mLock.test_and_set()); 	// 获得锁;
	videoSlice_t *pSlice = NULL;
	pSlice = clientPool[clientIp].videoSliceGroup.videoSlice;
	if(0 == pSlice->frameSize)
	{
		//mLock.clear();
		return false;
	}

	int i = 0;
	unsigned int frameSize = 0;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		frameSize += pSlice->sliceSize;
		++pSlice;
	}

	if(clientPool[clientIp].videoSliceGroup.videoSlice[0].frameSize == frameSize)
	{
		//mLock.clear();
		return true;
	}
	else
	{
		//mLock.clear();
		return false;
	}
}


int AvtpVideoServer::thListening(void *pThis)
{
	return ((AvtpVideoServer *)pThis)->listening();
}

int AvtpVideoServer::listening()
{
	int sfd = 0;
	sfd = pUdpServer->getSocketFd();

	sleep(0.5);
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
		videoSlice_t videoSlice;
		memset(&videoSlice, 0, sizeof(videoSlice_t));
		struct sockaddr_in stAddrClient;
		memset(&stAddrClient, 0, sizeof(struct sockaddr));

		ret = pUdpServer->recv(&videoSlice, sizeof(videoSlice_t), &stAddrClient);
		if(0 == ret)	// 需要完善异常处理。
		{
			
		}

		// 从数据中，解析IP 并查询是否在IP 池中。
		const unsigned int ipLen = 16;
		char clientIp[ipLen] = {0};
		strcpy(clientIp, inet_ntoa(stAddrClient.sin_addr));
		//cout << "client IP = " << clientIp << endl;

		bool bInClientPool = false;
		//bInClientPool = queryClient(clientIp);
		if(!bInClientPool)
		{
			//cerr << "This ip is not in client pool: " << clientIp << endl;
			//continue;
		}

		// 从数据中，解析数据头。
		// 如果是视频数据，则回复ACK, 并放入本地缓存。
		if(avtpDataType::TYPE_AV_VIDEO == videoSlice.avtpDataType)
		{
			//cout << "IP: " << clientIp << ", freame ID = " << videoSlice.frameID << ", seqence = " << videoSlice.sliceSeq << endl;
			//cout << "frame len = " << videoSlice.frameSize << ", slice len = " << videoSlice.sliceSize << endl;
			avtpCmd_t avtpCmd;
			memset(&avtpCmd, 0, sizeof(avtpCmd_t));
			avtpCmd.avtpDataType = avtpDataType::TYPE_CMD_ACK;
			avtpCmd.avtpData[0] = videoSlice.frameID;
			avtpCmd.avtpData[1] = videoSlice.sliceSeq;
			pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
			//pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
		}

		clientPool2[clientIp]->push(&videoSlice);
		//pushSliceIntoGroup(clientIp, &videoSlice);
	}
}

ClientProc::ClientProc()
{
	cout << "Call AvtpVideoServer::ClientProc()." << endl;
	cout << "Call AvtpVideoServer::ClientProc() end." << endl;
}

ClientProc::~ClientProc()
{
	
}

int ClientProc::create()
{
	cout << "Call ClientProc::create()." << endl;
	pThRecvHandler = make_shared<thread>(thRecvHandler, this);
	cout << "Call ClientProc::create() end." << endl;
	return 0;
}

int ClientProc::thRecvHandler(void *pThis)
{
	return ((ClientProc *)pThis)->recvHandler();
}

int ClientProc::recvHandler()
{
	cout << "Call ClientProc::thRecvHandler()." << endl;
	while(true)
	{
		//cout << "a" << endl;
		this_thread::sleep_for(chrono::seconds(1));
	}
	//while(mLock.test_and_set()); 	// 获得锁;
	//mLock.clear();
	cout << "Call ClientProc::thRecvHandler() end." << endl;
	return 0;
}

int ClientProc::push(const videoSlice_t *pVideoSlice)
{
	//cout << "Call ClientProc::push()." << endl;
	#if 0
	while(mLock.test_and_set()); 	// 获得锁;
	videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
	mLock.clear();
	#else
	unique_lock<mutex> lock(mMtx);
	videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
	lock.unlock();
	mCondVar.notify_one();
	#endif

	//cout << "Call ClientProc::push() end." << endl;
	return 0;
}

int ClientProc::pop(void *frameBuf, const unsigned int frameBufLen)
{
	//cout << "Call ClientProc::pop()." << endl;
#if 0
	while(mLock.test_and_set());	// 获得锁;
	int i = 0;
	unsigned int cpyBytes = 0;
	const videoSlice_t *pVideoSlice = NULL;
	pVideoSlice = videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		memcpy(frameBuf + cpyBytes, pVideoSlice->sliceBuf, pVideoSlice->sliceSize);
		cpyBytes += pVideoSlice->sliceSize;
		++pVideoSlice;
	}
	mLock.clear();
#else
	unique_lock<mutex> lock(mMtx);
	while(!isGroupFull())	// 队列不满则循环等待。
	{
		mCondVar.wait(lock);
	}

	int i = 0;
	unsigned int cpyBytes = 0;
	const videoSlice_t *pVideoSlice = NULL;
	pVideoSlice = videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		memcpy(frameBuf + cpyBytes, pVideoSlice->sliceBuf, pVideoSlice->sliceSize);
		cpyBytes += pVideoSlice->sliceSize;
		++pVideoSlice;
	}

	memset(&videoSliceGroup, 0, sizeof(videoSliceGroup_t));
	lock.unlock();
#endif
	//cout << "Call ClientProc::pop() end." << endl;
	return cpyBytes;
}

bool ClientProc::isGroupFull() const
{
	//cout << "Call ClientProc::isGroupFull()." << endl;
	const videoSlice_t *pSlice = NULL;
	pSlice = videoSliceGroup.videoSlice;
	if(0 == pSlice->frameSize)
	{
		return false;
	}

	int i = 0;
	unsigned int frameSize = 0;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		frameSize += pSlice->sliceSize;
		++pSlice;
	}

	if(videoSliceGroup.videoSlice[0].frameSize == frameSize)
	{
		return true;
	}
	else
	{
		return false;
	}

	return false;
}


