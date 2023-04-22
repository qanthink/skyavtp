/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
*/

#include <iostream>
#include "vtp_server.h"

using namespace std;

/*
	功能：	构造函数。
	注意：	
*/
AvtpVideoServer::AvtpVideoServer(const std::string serverIP)
{
	cout << "Call AvtpVideoServer::AvtpVideoServer()." << endl;
	
	bRunning = true;
	pUdpSocket = make_shared<UdpSocket>(serverIP, avtpPort);
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

	pUdpSocket = NULL;
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
	pair<map<string, shared_ptr<VideoClientProc>>::iterator, bool> ret;
	shared_ptr<VideoClientProc> pVideoClientProc(new VideoClientProc);
	ret = clientPool.insert(make_pair(clientIP, pVideoClientProc));
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
	map<string , shared_ptr<VideoClientProc>>::iterator it;
	it = clientPool.find(clientIP);
	
	if(clientPool.end() == it)
	{
		mMtx.unlock();
		return false;
	}
	else
	{
		mMtx.unlock();
		return true;
	}
}

/*
	功能：	接收视频帧。
	返回：	返回帧数据长度。
	注意：	
*/
int AvtpVideoServer::recvVideoFrame(string clientIp, void *buf, size_t len)
{
	//cout << "Call AvtpVideoServer::recvVideoFrame()." << endl;
	
	int frameRealSize = 0;
	frameRealSize = clientPool[clientIp]->popFrame(buf, len);
	
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
	sfd = pUdpSocket->getSocketFd();
	fd_set fdset;
	FD_ZERO(&fdset);
	
	// 设置超时时长
	struct timeval stTimeOut;
	memset(&stTimeOut, 0, sizeof(struct timeval));

	// 避免在循环体内初始化数据，减小时间开销。
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
		memset(&stAddrClient, 0, sizeof(struct sockaddr));
		ret = pUdpSocket->recvFrom(&videoSlice, sizeof(videoSlice_t), &stAddrClient);
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpSocket->recvFrom() in AvtpVideoServer::listening()." << endl;
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

				if(!clientPool[clientIp]->sliceQueue.isFull())
				{
					pUdpSocket->sendTo(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
					clientPool[clientIp]->sliceQueue.push(&videoSlice);
					sem_post(&clientPool[clientIp]->sem);
					//cout << "post end." << endl;
					break;
				}
				else
				{
					// 队列满，donothing. Wait queue empty and TX retransmit.
					cerr << clientIp << ": queue full" << endl;
					break;
				}
				break;
			}
			case avtpDataType::TYPE_AV_AUDIO:
			{
				cerr << "Receive audio data in AvtpVideoServer." << endl;
				break;
			}
			default:
			{
				//cout << "In AvtpVideoClient::listening(). Received datetype = " << videoSlice.avtpDataType << endl;
				break;
			}
		}
	}

	return 0;
}

VideoClientProc::VideoClientProc()
{
	cout << "Call AvtpVideoServer::VideoClientProc()." << endl;
	sliceQueue.setQueueDepth(queueDepths);
	sem_init(&sem, 0, 0);
	cout << "Call AvtpVideoServer::VideoClientProc() end." << endl;
}

VideoClientProc::~VideoClientProc()
{
	cout << "Call AvtpVideoServer::~VideoClientProc()." << endl;
	sem_destroy(&sem);
	cout << "Call AvtpVideoServer::~VideoClientProc() end." << endl;
}


/*
	功能：	将Frame 数据从Group 取出。
	返回：	返回帧的长度。
	注意：	Frame, group 概念参考AVTP 数据类型。
*/
int VideoClientProc::popFrame(void *buf, size_t len)
{
	//cout << "Call VideoClientProc::popFrame()." << endl;

	unsigned int frameSize = 0;
	while(!((frameSize == videoSliceGroup.videoSlice[0].frameSize) && 
		(0 != frameSize)))		// 未来需要考虑停用协议时，如何跳出while 循环。
	{
		frameSize = 0;
		// 步骤一：从sliceQueue 中取数据出来。
		int ret = 0;
		videoSlice_t videoSlice = {0};
		sem_wait(&sem);
		//cout << "wait end." << endl;
		ret = sliceQueue.pop(&videoSlice);
		if(0 != ret)		// 队列异常或队列空，取数据失败，返回-1.
		{
			//cerr << "sliceQueue is empty or abnormal." << endl;
			continue;
		}

		//cout << "videoSlice.frameID, expFrameID = " << videoSlice.frameID << ", " << expFrameID << endl;
		// 步骤二：判断slice 要不要放入sliceGroup, 以及sliceGroup 是否要清除数据。
		// 如果是期待的frameID, 则保存。
		if(videoSlice.frameID == expFrameID)
		{
			//cout << "frameID == expFrame" << endl;
			// 如果slice 数据为空，则把新数据放进去。
			if(avtpDataType::TYPE_INVALID == videoSliceGroup.videoSlice[videoSlice.sliceSeq].avtpDataType)
			{
				videoSliceGroup.videoSlice[videoSlice.sliceSeq] = videoSlice;
			}
			else	// 否则说明slice 没被清除、没被使用，是重传的，不处理。并且返回-2.
			{
				continue;
			}
		}
		// 如果不是期待的frameID, 则要分情况。
		else		// videoSlice.frameID != expFrameID
		{
			//cout << "frameID != expFrame" << endl;
			if(0 == videoSlice.frameID)			// TX 断连重启的情况。0 == frameID && 0 != expFrameID
			{
				//cout << "frameID == 0" << endl;
				expFrameID = 0;
				videoSliceGroup.videoSlice[videoSlice.sliceSeq] = videoSlice;
			}
			else if(0 == expFrameID)			// RX 断联重启的情况。0 != frameID && 0 == expFrameID
			{
				//cout << "expFrameID == 0" << endl;
				expFrameID = videoSlice.frameID + 2;	// +1 好像有问题，存在脏数据。
				//videoSliceGroup.videoSlice[videoSlice.sliceSeq] = {0};
				continue;
			}
			else if(videoSlice.frameID < expFrameID)	// 舍弃脏数据。frameID < expFrameID && != 0
			{
				//cerr << "old slice." << endl;
				continue;
			}
			else if(videoSlice.frameID > expFrameID)	// frameID > expFrameID && != 0
			{
				cerr << "frameID > expFrameID: " << videoSlice.frameID << ", " << expFrameID << endl;
				continue;
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

			// frameID != expFrameID. 不用判断frameID == 0 和 expFrameID == 0.
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

		// 第四步：从sliceGroup 中打包出frame.
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
	}

	int i = 0;
	unsigned int cpyBytes = 0;
	videoSlice_t const *pSlice = NULL;
	pSlice = videoSliceGroup.videoSlice;
	for(i = 0; i < videoSliceGroup_t::groupMaxSize; ++i)
	{
		if(0 == pSlice->frameSize)
		{
			break;
		}
		memcpy(buf + cpyBytes, pSlice->sliceBuf, pSlice->sliceSize);
		cpyBytes += pSlice->sliceSize;
		++pSlice;
	}
	expFrameID = videoSliceGroup.videoSlice[0].frameID + 1;
	memset(&videoSliceGroup, 0, sizeof(videoSliceGroup_t));
	
	return cpyBytes;
}

