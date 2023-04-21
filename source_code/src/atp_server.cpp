/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
	video 收发有明显的服务器、客户端工作差异。是CS 模型。
	audio 收发没有明显的服务器、客户端工作差异，是全双工的。不是CS 模型。
*/

#include <iostream>
#include "atp_server.h"

using namespace std;

/*
	功能：	构造函数。
	注意：	
*/
AvtpAudioServer::AvtpAudioServer(const char * serverIP)
{
	cout << "Call AvtpAudioServer::AvtpAudioServer()." << endl;
	
	bRunning = true;
	pUdpSocket = make_shared<UdpSocket>(serverIP, avtpPort);
	pThServerRecv = make_shared<thread>(thListening, this);
	
	cout << "Call AvtpAudioServer::AvtpAudioServer() end." << endl;
}

AvtpAudioServer::~AvtpAudioServer()
{
	cout << "Call AvtpAudioServer::~AvtpAudioServer()." << endl;
	
	bRunning = false;
	
	if(!pThServerRecv)
	{
		pThServerRecv->join();
		pThServerRecv = NULL;
	}

	pUdpSocket = NULL;
	cout << "Call AvtpAudioServer::~AvtpAudioServer() end." << endl;
}

/*
	功能：	增加客户端。
	返回：	成功返回true, 失败返回false.
	注意：	如果客户端已经存在，则会添加失败。
*/
bool AvtpAudioServer::addClient(string clientIP)
{
	cout << "Call AvtpAudioServer::addClient()." << endl;

	mMtx.lock(); 	// 获得锁;
	pair<map<string, shared_ptr<AudioClientProc>>::iterator, bool> ret;
	shared_ptr<AudioClientProc> pAudioClientProc(new AudioClientProc);
	ret = clientPool.insert(make_pair(clientIP, pAudioClientProc));
	mMtx.unlock();

	return true;
}

/*
	功能：	删除客户端。
	返回：	成功返回true, 失败返回false.
	注意：	如果客户端不存在，则会删除失败。
*/
bool AvtpAudioServer::deleteClient(string clientIP)
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
bool AvtpAudioServer::queryClient(string clientIP)
{
	//cout << "Call AvtpAudioServer::queryClient()." << endl;

	mMtx.lock(); 	// 获得锁;
	map<string , shared_ptr<AudioClientProc>>::iterator it;
	it = clientPool.find(clientIP);
	mMtx.unlock();
	
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
	功能：	发送消息。
	返回：	
	注意：	
*/
int AvtpAudioServer::sendMessage(std::string clientIp, const avtpCmd_t *pAvtpCmd)
{
	struct sockaddr_in stAddrClient;
	memset(&stAddrClient, 0, sizeof(struct sockaddr_in));
	stAddrClient.sin_family = AF_INET;
	stAddrClient.sin_port = htons(avtpPort);
	stAddrClient.sin_addr.s_addr = inet_addr(clientIp.c_str());

	pUdpSocket->sendTo(pAvtpCmd, sizeof(avtpCmd_t), &stAddrClient);
	return 0;
}

/*
	功能：	发送音频帧。
	返回：	
	注意：	
*/
int AvtpAudioServer::sendAudioFrame(std::string clientIp, const void *buf, size_t len)
{
	//cout << "Call AvtpAudioServer::sendAudioFrame()." << endl;
	mMtx.lock();
	audioFrame.avtpDataType = avtpDataType::TYPE_AV_AUDIO;
	audioFrame.frameID = 0;
	audioFrame.frameSize = len;
	memcpy(audioFrame.dataBuf, buf, len);
	mMtx.unlock();

	struct sockaddr_in stAddrClient;
	memset(&stAddrClient, 0, sizeof(struct sockaddr));
	stAddrClient.sin_port = htons(avtpPort);
	stAddrClient.sin_family = AF_INET;								// 设置地址家族
	stAddrClient.sin_addr.s_addr = inet_addr(clientIp.c_str());		//设置
	pUdpSocket->sendTo(&audioFrame, sizeof(audioFrame_t), &stAddrClient);

	//cout << "Call AvtpAudioServer::sendAudioFrame() end." << endl;
	return 0;
}

/*
	功能：	接收音频频帧。
	返回：	返回音频帧数据长度。
	注意：	
*/
int AvtpAudioServer::recvAudioFrame(string clientIp, void *buf, size_t len)
{
	//cout << "Call AvtpAudioServer::recvAudioFrame()." << endl;
	
	int frameRealSize = 0;
	frameRealSize = clientPool[clientIp]->popFrame(buf, len);
	
	//cout << "Call AvtpAudioServer::recvAudioFrame() end." << endl;
	return frameRealSize;
}

int AvtpAudioServer::thListening(void *pThis)
{
	return ((AvtpAudioServer *)pThis)->listening();
}

/*
	功能：	服务器接收线程，用于监听客户端的消息。
	返回：	返回0.
	注意：	
*/
int AvtpAudioServer::listening()
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
	audioFrame_t audioFrame;
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
			cerr << "In AvtpAudioServer::listening(). Fail to call select(2), " << strerror(errno) << endl;
			continue;
		}
		else if(0 == ret)		// Timeout
		{
			cout << "In AvtpAudioServer::listening(). Timeout!" << endl;
			continue;
		}
		else
		{
			// do next;
		}

		// step2 接收数据。
		memset(&audioFrame, 0, sizeof(audioFrame_t));
		memset(&stAddrClient, 0, sizeof(struct sockaddr));		// 必须进行memset 以便更新client 信息。避免掉线不能重连。
		ret = pUdpSocket->recvFrom(&audioFrame, sizeof(audioFrame_t), &stAddrClient);
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpSocket->recvFrom() in AvtpAudioServer::listening()." << endl;
			continue;
		}
		else if(ret < sizeof(avtpCmd_t))
		{
			cout << "In AvtpAudioServer::listening(). Received data length is not as expected." << endl;
			continue;
		}

		//cout << ", " << stAddrClient.sin_port << ", " << stAddrClient.sin_addr << endl;

		// 处理数据。要尽可能快，保障最短时间内再次调用select. 避免UDP 数据堆积导致丢包。
		// step3.1 解析IP 并查询是否在IP 池中。
		strcpy(clientIp, inet_ntoa(stAddrClient.sin_addr));
		//cout << "client IP = " << clientIp << endl;

		bool bInClientPool = false;
		bInClientPool = queryClient(clientIp);
		if(!bInClientPool)
		{
			//cerr << "This ip is not in client pool: " << clientIp << endl;
			continue;
		}

		// step3.2 从数据中，解析数据头。
		// 如果是视频数据，则回复ACK, 并放入内存缓存。
		switch(audioFrame.avtpDataType)
		{
			case avtpDataType::TYPE_AV_AUDIO:
			{
				if(!clientPool[clientIp]->frameQueue.isFull())
				{
					pUdpSocket->sendTo(&avtpCmd, sizeof(avtpCmd_t), &stAddrClient);
					clientPool[clientIp]->frameQueue.push(&audioFrame);
					sem_post(&clientPool[clientIp]->sem);
					//cout << "post end." << endl;
					break;
				}
				else
				{
					cerr << "frameQueue is full. IP: " << clientIp << endl;
				}
				break;
			}
			case avtpDataType::TYPE_CMD_ReqStartTalk:
			{
				//bAllowTalking = true;
				break;
			}
			case avtpDataType::TYPE_CMD_ReqStopTalk:
			{
				//bAllowTalking = false;
				break;
			}
			case avtpDataType::TYPE_AV_VIDEO:
			{
				cerr << "Receive video data in AvtpAudioServer." << endl;
				break;
			}
			default:
			{
				//cout << "In AvtpAudioServer::listening(). Received datetype = " << audioFrame.avtpDataType << endl;
				break;
			}
		}
	}

	return 0;
}

AudioClientProc::AudioClientProc()
{
	cout << "Call AvtpAudioServer::AudioClientProc()." << endl;
	frameQueue.setQueueDepth(queueDepths);
	sem_init(&sem, 0, 0);
	cout << "Call AvtpAudioServer::AudioClientProc() end." << endl;
}

AudioClientProc::~AudioClientProc()
{
	cout << "Call AvtpAudioServer::~AudioClientProc()." << endl;
	sem_destroy(&sem);
	cout << "Call AvtpAudioServer::~AudioClientProc() end." << endl;
}

/*
	功能：	将Frame 数据从队列中取出来。
	返回：	返回帧的长度。
	注意：	
*/
int AudioClientProc::popFrame(void *buf, const unsigned int frameBufLen)
{
	//cout << "Call AudioClientProc::popFrame()." << endl;
	audioFrame_t audioFrame = {0};
	while(1)		// 未来需要考虑停用协议时，如何跳出while 循环。
	{
		sem_wait(&sem);
		//cout << "wait end." << endl;
		
		int ret = 0;
		ret = frameQueue.pop(&audioFrame);
		if(0 != ret)		// 队列异常或队列空，取数据失败，返回-1.
		{
			//cerr << "sliceQueue is empty or abnormal." << endl;
			continue;
		}
		else
		{
			//cout << "In AudioClientProc::popFrame(). audioFrame.frameSize = " << audioFrame.frameSize << endl;
			break;
		}
	}

	unsigned int frameSize = 0;
	if(frameBufLen < audioFrame.frameSize)
	{
		cerr << "Fail to call AudioClientProc::popFrame(). Argument buf has no enough space." << endl;
		return 0;
	}
	
	memcpy(buf, audioFrame.dataBuf, audioFrame.frameSize);
	return audioFrame.frameSize;
}

