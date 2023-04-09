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
	testQueue();
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
	返回：	成功，返回帧数据长度；失败，返回-1.
	注意：	
*/
int AvtpVideoServer::recvVideoFrame(string clientIp, void *frameBuf, const unsigned int frameBufSize)
{
	//cout << "Call AvtpVideoServer::recvVideoFrame()." << endl;
	//cout << "recvVideoFrame." << clientIp << endl;

	int frameRealSize = 0;
	frameRealSize = clientPool[clientIp]->popFrame(frameBuf, frameBufSize);
	//clientPool[clientIp]->lock.unlock();
	
	//cout << "recvVideoFrame end." << clientIp << endl;
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
		if(-1 == ret)			// Fail
		{
			cerr << "In AvtpVideoServer::listening(). Fail to call select(2), " << strerror(errno) << endl;
			continue;
		}
		else if(0 == ret)		// Timeout
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
				#if 0
				// 2023.4.6 先push 再send ACK. 避免没来得及处理，TX 端根据ACK 又发下一帧数据。
				int ret = 0;
				ret = clientPool[clientIp]->pushSlice(&videoSlice);		// push 动作可能存在延时。
				if(0 == ret)		// 成功
				{
					pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
				}
				else if(-1 == ret)
				{
					// 数据已存在在，依然回复ACK.
					pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
				}
				else if(-2 == ret)
				{
					// RX 重启，丢弃无用数据，依然回复ACK.
					pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
				}
				else if(-3 == ret)
				{
					// 过时的数据，依然回复ACK.
					pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
				}
				else
				{
					cerr << "push fail" << endl;
					// pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
				}
				#else
				if(clientPool[clientIp]->sliceQueue.isFull())
				{
					// 队列满，donothing. Wait queue empty and TX retransmit.
					break;
				}
				else
				{
					pUdpServer->send(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
					clientPool[clientIp]->sliceQueue.push(&videoSlice);
				}
				#endif
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
	sliceQueue.setQueueDepth(queueDepths);
	cout << "Call AvtpVideoServer::ClientProc() end." << endl;
}

ClientProc::~ClientProc()
{
	cout << "Call AvtpVideoServer::~ClientProc()." << endl;
	cout << "Call AvtpVideoServer::~ClientProc() end." << endl;
}

/*
	功能：	将Slicen 数据放入Group 中。
	返回：成功，反馈0; 失败：-1, 数据已存在；-2, RX 重连后舍弃不完整数据；-3, 过时的数据；
	注意：	slice, group 概念参考AVTP 数据类型。
*/
int ClientProc::pushSlice(const videoSlice_t *pVideoSlice)
{
	//cout << "Call ClientProc::pushSlice()." << endl;
	//cout << "pushSlice" << endl;
	unique_lock<mutex> lock(mMtx);
	#if 0
	if(pVideoSlice->frameID >= expFrameID)
	{
		cout << ">=" << endl;
		videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
	}
	else if(pVideoSlice->frameID != expFrameID - 1)
	{
		cout << "!= -1" << endl;
		videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
	}
	else if(pVideoSlice->frameID == expFrameID - 1)
	{
		cerr << "not expFrameID" << endl;
	}
	else
	{
		cerr << "expFrameID out of range" << endl;
	}
	lock.unlock();
	mCondVar.notify_one();
	#else
	if(pVideoSlice->frameID == expFrameID)	// 如果是期待的frameID, 则保存。
	{
		// 如果slice 数据不为空，说明slice 没有被清除、没有被使用，则新的数据不要进来。
		if(avtpDataType::TYPE_INVALID != videoSliceGroup.videoSlice[pVideoSlice->sliceSeq].avtpDataType)
		{
			lock.unlock();
			mCondVar.notify_one();
			return -1;
		}
		else
		{
			videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
			lock.unlock();
			mCondVar.notify_one();
			return 0;
		}
	}
	else		// 如果不是期待的frameID, 则要分情况。
	{
		if(0 == pVideoSlice->frameID)		// TX 断连重启的情况。
		{
			expFrameID = 0;
			videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = *pVideoSlice;
			lock.unlock();
			mCondVar.notify_one();
			return 0;
		}
		else if(0 == expFrameID)			// RX 断联重启的情况。
		{
			expFrameID = pVideoSlice->frameID + 1;
			videoSliceGroup.videoSlice[pVideoSlice->sliceSeq] = {0};
			lock.unlock();
			mCondVar.notify_one();
			return -2;
		}
		else if(pVideoSlice->frameID < expFrameID)
		{
			cerr << "old slice." << endl;
			return -3;
		}
		else if(pVideoSlice->frameID > expFrameID)
		{
			cerr << "Maybe not pop" << endl;
			cerr << "frameID, expFrameID = " << pVideoSlice->frameID << ", " << expFrameID << endl;
			return -4;
		}

		lock.unlock();
		mCondVar.notify_one();
		return 0;
	}
	#endif

	//cout << "pushSlice end." << endl;
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
	//cout << "popFrame." << endl;
	#if 0
	unique_lock<mutex> lock(mMtx);
	//std::lock(mMtx, std::defer_lock);
	while(!isGroupFull())	// 队列不满则循环等待。
	{
		//cout << "wait" << endl;
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

	expFrameID = videoSliceGroup.videoSlice[0].frameID + 1;
	memset(&videoSliceGroup, 0, sizeof(videoSliceGroup_t));
	// 此处unlock 做的太早，且pop 的后续动作执行太慢，导致了多次push.
	// push/pop/recv 需要做到同步。
	lock.unlock();

	//cout << "Call ClientProc::popFrame() end." << endl;
	//cout << "popFrame end." << endl;
	return cpyBytes;
	#else
	// 步骤一：从sliceQueue 中取数据出来。
	int ret = 0;
	videoSlice_t videoSlice = {0};
	ret = sliceQueue.pop(&videoSlice);
	if(0 != ret)		// 队列异常或队列空，取数据失败，返回-1.
	{
		cerr << "sliceQueue is empty or abnormal." << endl;
		return -1;
	}

	cout << "videoSlice.frameID, expFrameID = " << videoSlice.frameID << ", " << expFrameID << endl;
	// 步骤二：判断slice 要不要放入sliceGroup, 以及sliceGroup 是否要清除数据。
	// 如果是期待的frameID, 则保存。
	if(videoSlice.frameID == expFrameID)
	{
		cout << "frameID == expFrame" << endl;
		// 如果slice 数据不为空，说明slice 没有被清除、没有被使用，是重传的，不处理。并且返回-2.
		if(avtpDataType::TYPE_INVALID != videoSliceGroup.videoSlice[videoSlice.sliceSeq].avtpDataType)
		{
			return -2;
		}
		else
		{
			videoSliceGroup.videoSlice[videoSlice.sliceSeq] = videoSlice;
		}
	}
	// 如果不是期待的frameID, 则要分情况。
	else
	{
		cout << "frameID != expFrame" << endl;
		if(0 == videoSlice.frameID)			// TX 断连重启的情况。
		{
			cout << "frameID == 0" << endl;
			expFrameID = 0;
			videoSliceGroup.videoSlice[videoSlice.sliceSeq] = videoSlice;
		}
		else if(0 == expFrameID)			// RX 断联重启的情况。
		{
			cout << "expFrameID == 0" << endl;
			expFrameID = videoSlice.frameID + 2;
			videoSliceGroup.videoSlice[videoSlice.sliceSeq] = {0};
			return -3;
		}
		else if(videoSlice.frameID < expFrameID)
		{
			cerr << "old slice." << endl;
			return -4;
		}
		else if(videoSlice.frameID > expFrameID)
		{
			cerr << "Maybe not pop" << endl;
			cerr << "frameID, expFrameID = " << videoSlice.frameID << ", " << expFrameID << endl;
			return -5;
		}
	}

	// 步骤三：从sliceGroup 中判断数据是否已满。
	int i = 0;
	videoSlice_t *pSlice = NULL;
	pSlice = videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		// if(0 == pSlice->sliceSize) break; 缩短了轮询时间，对即时任务处理很重要。
		if(0 == pSlice->sliceSize)
		{
			break;
		}
		else
		{
			//do next;
		}

		if(pSlice->frameID == expFrameID)	// 包含两边同时重连重启的情况，ID == 0.
		{
			++pSlice;
			continue;
		}
		else								// ID 不同。
		{
			// do next
		}

		if(0 == pSlice->frameID)			// TX 端重启重连
		{
			*pSlice = {0};
			++pSlice;
			continue;
		}
		// 不存在该if 分支，如果RX 端重启重连，push 中已经对expFrameID 进行了重置。
		else if(0 == expFrameID) 			// RX 端重启重连
		{
			cerr << "0 == expFrameID. unexpected" << endl;
			continue;
		}
		else		// 0 != expFrameID && 0 != frameID && expFrameID != frameID.
		{
			if(pSlice->frameID < expFrameID)
			{
				*pSlice = {0};
				continue;
			}
			else	// frameID > expFrameID.
			{
				cerr << "frameID > expFrameID. unexpected" << endl;
				*pSlice = {0};
				continue;
			}
		}
	}

	// 第四步：从sliceGroup 中打包出frame.
	unsigned int frameSize = 0;
	pSlice = videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		// if(0 == pSlice->sliceSize) break; 缩短了轮询时间，对及时任务处理很重要。
		if(0 == pSlice->sliceSize)
		{
			break;
		}
		else
		{
			frameSize += pSlice->sliceSize;
			++pSlice;
		}
	}

	//cout << "frameSize = " << frameSize << endl;
	//cout << "[0].frameSize = " << videoSliceGroup.videoSlice[0].frameSize << endl;

	if((frameSize == videoSliceGroup.videoSlice[0].frameSize) && 
		(0 != frameSize))
	{
		cout << "frameSize = " << frameSize << endl;
		expFrameID = videoSliceGroup.videoSlice[0].frameID + 1;
		//memset(&videoSliceGroup, 0, sizeof(videoSliceGroup_t));
		//return frameSize;
	}
	else
	{
		cout << "frameSize != [0].frameSize, frameSize = " << frameSize << endl;
		return -6;
	}

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
	
	return cpyBytes;
	#endif
}

/*
	功能：	判断Group 是否为满。
	返回：
	注意：
*/
bool ClientProc::isGroupFull()
{
	//cout << "Call ClientProc::isGroupFull()." << endl;
	videoSlice_t *pSlice = NULL;
	pSlice = videoSliceGroup.videoSlice;

	// 第一轮，整理数据。
	int i = 0;
	pSlice = videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		// if(0 == pSlice->sliceSize) break; 缩短了轮询时间，对及时任务处理很重要。
		if(0 == pSlice->sliceSize)
		{
			break;
		}
		else
		{
			//do next;
		}

		if(pSlice->frameID == expFrameID)	// 包含两边同时重连重启的情况，ID == 0.
		{
			++pSlice;
			continue;
		}
		else								// ID 不同。
		{
			// do next
		}

		if(0 == pSlice->frameID)			// TX 端重启重连
		{
			// push 的时候只对sliceSeq 位置的数据进行重置，其它位置无法重置，只能在pop 中检查数据并重置。
			*pSlice = {0};
			++pSlice;
			continue;
		}
		// 不存在该if 分支，如果RX 端重启重连，push 中已经对expFrameID 进行了重置。
		else if(0 == expFrameID) 			// RX 端重启重连
		{
			cerr << "0 == expFrameID. unexpected" << endl;
			continue;
		}
		else		// 0 != expFrameID && 0 != frameID && expFrameID != frameID.
		{
			if(pSlice->frameID < expFrameID)
			{
				*pSlice = {0};
				continue;
			}
			else	// frameID > expFrameID.
			{
				cerr << "frameID > expFrameID. unexpected" << endl;
				*pSlice = {0};
				continue;
			}
		}
	}

	unsigned int frameSize = 0;
	pSlice = videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		// if(0 == pSlice->sliceSize) break; 缩短了轮询时间，对及时任务处理很重要。
		if(0 == pSlice->sliceSize)
		{
			break;
		}
		else
		{
			frameSize += pSlice->sliceSize;
			++pSlice;
		}
	}
	
	//cout << "frameSize = " << frameSize << endl;
	//cout << "[0].frameSize = " << videoSliceGroup.videoSlice[0].frameSize << endl;

	if((frameSize == videoSliceGroup.videoSlice[0].frameSize) && 
		(0 != frameSize))
	{
		cout << "frameSize = " << frameSize << endl;
		return true;
	}
	else
	{
		return false;
	}

	return false;
}

