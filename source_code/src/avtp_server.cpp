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
	
	bRunning = true;
	pUdpServer = make_shared<UdpServer>(serverIP, avtpPort);
	pThServerRecv = make_shared<thread>(thListening, this);
	
	cout << "Call AvtpVideoServer::AvtpVideoServer() end." << endl;
}

AvtpVideoServer::~AvtpVideoServer()
{
	cout << "Call AvtpVideoServer::~AvtpVideoServer()." << endl;
	
	bRunning = false;
	
	if(!pThServerRecv)
	{
		pThServerRecv->join();
		pThServerRecv = NULL;
	}

	pUdpServer = NULL;
	cout << "Call AvtpVideoServer::~AvtpVideoServer() end." << endl;
}

/*
	功能：	增加客户端。
	返回：	成功返回true, 失败返回false.
	注意：	如果客户端已经存在，则会添加失败。
*/
bool AvtpVideoServer::addClient(string clientIP)
{
	cout << "Call AvtpVideoServer::addClient()." << endl;

	mMtx.lock(); 	// 获得锁;
	pair<map<string, shared_ptr<ClientProc>>::iterator, bool> ret;
	shared_ptr<ClientProc> pClientProc(new ClientProc);
	ret = clientPool.insert(make_pair(clientIP, pClientProc));
	mMtx.unlock();

	return true;
}

/*
	功能：	删除客户端。
	返回：	成功返回true, 失败返回false.
	注意：	如果客户端不存在，则会删除失败。
*/
bool AvtpVideoServer::deleteClient(string clientIP)
{
	mMtx.lock(); 	// 获得锁;
	videoSliceGroup_t videoSliceGroup;
	clientPool.erase(clientIP);
	mMtx.unlock();

	return 0;
}

/*
	功能：	查询客户端。
	返回：	存在返回true, 不存在返回false.
	注意：	
*/
bool AvtpVideoServer::queryClient(string clientIP)
{
	//cout << "Call AvtpVideoServer::queryClient()." << endl;

	mMtx.lock(); 	// 获得锁;
	map<string , shared_ptr<ClientProc>>::iterator it;
	it = clientPool.find(clientIP);
	mMtx.unlock();
	
	if(clientPool.end() == it)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/*
	功能：	接收视频帧。
	注意：	
*/
int AvtpVideoServer::recvVideoFrame(string clientIp, void *frameBuf, const unsigned int frameBufSize)
{
	//cout << "Call AvtpVideoServer::recvVideoFrame()." << endl;

	int frameRealSize = 0;
	frameRealSize = clientPool[clientIp]->popFrame(frameBuf, frameBufSize);
	
	//cout << "Call AvtpVideoServer::recvVideoFrame() end." << endl;
	return frameRealSize;
}

int AvtpVideoServer::thListening(void *pThis)
{
	return ((AvtpVideoServer *)pThis)->listening();
}

/*
	功能：	服务器接收线程，用于监听客户端的消息。
	返回：	返回0.
	注意：	
*/
int AvtpVideoServer::listening()
{
	// 设置要监听的套接字。
	int sfd = 0;
	sfd = pUdpServer->getSocketFd();
	fd_set fdset;
	FD_ZERO(&fdset);
	
	// 设置超时时长
	struct timeval stTimeOut;
	memset(&stTimeOut, 0, sizeof(struct timeval));

	// 避免在循环体内初始化数据，避免时间开销。
	avtpCmd_t avtpCmd;
	videoSlice_t videoSlice;
	struct sockaddr_in stAddrClient;

	int ret = 0;
	const unsigned int ipLen = 16;
	char clientIp[ipLen] = {0};

	while(bRunning)
	{
		// step1.1 设置监听的套接字
		FD_SET(sfd, &fdset);
		// step1.2 设置超时时长
		stTimeOut.tv_sec = mTimeOutMs / 1000;
		stTimeOut.tv_usec = mTimeOutMs % 1000 * 1000; // N * 1000 = N ms
		// step1.3 监听套接字，等待客户端数据到来。
		ret = select(sfd + 1, &fdset, NULL, NULL, &stTimeOut);
		if(-1 == ret)
		{
			cerr << "In AvtpVideoServer::listening(). Fail to call select(2), " << strerror(errno) << endl;
			continue;
		}
		else if(0 == ret)
		{
			cout << "In AvtpVideoServer::listening(). Timeout!" << endl;
			continue;
		}
		else
		{
			// do next;
		}

		// step2 接收数据。
		memset(&videoSlice, 0, sizeof(videoSlice_t));
		memset(&stAddrClient, 0, sizeof(struct sockaddr));		// 必须进行memset 以便更新client 信息。避免掉线不能重连。
		ret = pUdpServer->recv(&videoSlice, sizeof(videoSlice_t), &stAddrClient);
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpServer->recv() in AvtpVideoServer::listening()." << endl;
			continue;
		}
		else if(ret < sizeof(avtpCmd_t))
		{
			cout << "In AvtpVideoServer::listening(). Received data length is not as expected." << endl;
			continue;
		}

		// 处理数据。要尽可能快，保障最短时间内再次调用select. 避免UDP 数据堆积导致丢包。
		// step3.1 解析IP 并查询是否在IP 池中。
		strcpy(clientIp, inet_ntoa(stAddrClient.sin_addr));
		//cout << "client IP = " << clientIp << endl;

		bool bInClientPool = false;
		bInClientPool = queryClient(clientIp);
		if(!bInClientPool)
		{
			cerr << "This ip is not in client pool: " << clientIp << endl;
			continue;
		}

		// step3.2 从数据中，解析数据头。
		// 如果是视频数据，则回复ACK, 并放入内存缓存。
		switch(videoSlice.avtpDataType)
		{
			case avtpDataType::TYPE_AV_VIDEO:
			{
				//cout << "IP: " << clientIp << ", freame ID = " << videoSlice.frameID << ", seqence = " << videoSlice.sliceSeq << endl;
				//cout << "frame len = " << videoSlice.frameSize << ", slice len = " << videoSlice.sliceSize << endl;
				memset(&avtpCmd, 0, sizeof(avtpCmd_t));
				avtpCmd.avtpDataType = avtpDataType::TYPE_CMD_ACK;
				avtpCmd.avtpData[0] = videoSlice.frameID;
				avtpCmd.avtpData[1] = videoSlice.sliceSeq;
				pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
				clientPool[clientIp]->pushSlice(&videoSlice);
				break;
			}
			default:
			{
				cout << "In AvtpVideoServer::listening(). Received bad date." << endl;
				break;
			}
		}
	}

	return 0;
}

ClientProc::ClientProc()
{
	cout << "Call AvtpVideoServer::ClientProc()." << endl;
	cout << "Call AvtpVideoServer::ClientProc() end." << endl;
}

ClientProc::~ClientProc()
{
	cout << "Call AvtpVideoServer::~ClientProc()." << endl;
	cout << "Call AvtpVideoServer::~ClientProc() end." << endl;
}

/*
	功能：	将Slicen 数据放入Group 中。
	返回：
	注意：	slice, group 概念参考AVTP 数据类型。
*/
int ClientProc::pushSlice(const videoSlice_t *pVideoSlice)
{
	//cout << "Call ClientProc::pushSlice()." << endl;
	unique_lock<mutex> lock(mMtx);
	videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
	lock.unlock();
	mCondVar.notify_one();

	//cout << "Call ClientProc::pushSlice() end." << endl;
	return 0;
}

/*
	功能：	将Frame 数据从Group 取出。
	返回：
	注意：	Frame, group 概念参考AVTP 数据类型。
*/
int ClientProc::popFrame(void *frameBuf, const unsigned int frameBufLen)
{
	//cout << "Call ClientProc::popFrame()." << endl;
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

	//cout << "Call ClientProc::popFrame() end." << endl;
	return cpyBytes;
}

/*
	功能：	判断Group 是否为满。
	返回：
	注意：
*/
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
	#if 1
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
	#else
	判断Group 是否为满的方法，有优化空间。
	#endif

	return false;
}

