/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
	video 收发有明显的服务器、客户端工作差异。是CS 模型。
	audio 收发没有明显的服务器、客户端工作差异。不是CS 模型。
*/

#include <iostream>
#include "atp_client.h"

using namespace std;

/*
	功能：	构造函数。
	注意：	
*/
AvtpAudioClient::AvtpAudioClient(const std::string serverIP)
{
	cout << "Call AvtpAudioClient::AvtpAudioClient()." << endl;
	
	bRunning = true;
	pUdpSocket = make_shared<UdpSocket>(serverIP, avtpPort);
	pThServerRecv = make_shared<thread>(thListening, this);
	mServerIP = serverIP;
	
	cout << "Call AvtpAudioClient::AvtpAudioClient() end." << endl;
}

AvtpAudioClient::~AvtpAudioClient()
{
	cout << "Call AvtpAudioClient::~AvtpAudioClient()." << endl;
	
	bRunning = false;
	
	if(!pThServerRecv)
	{
		pThServerRecv->join();
		pThServerRecv = NULL;
	}

	pUdpSocket = NULL;
	mServerIP = "0.0.0.0";
	cout << "Call AvtpAudioClient::~AvtpAudioClient() end." << endl;
}

/*
	功能：	发送音频帧。
	返回：	
	注意：	
*/
int AvtpAudioClient::sendAudioFrame(const void *buf, size_t len)
{
	//cout << "Call AvtpAudioClient::sendAudioFrame()." << endl;
	audioFrame.avtpDataType = avtpDataType::TYPE_AV_AUDIO;
	audioFrame.frameID = 0;
	audioFrame.frameSize = len;
	memcpy(audioFrame.dataBuf, buf, len);

	struct sockaddr_in stAddrServer;
	memset(&stAddrServer, 0, sizeof(struct sockaddr_in));
	stAddrServer.sin_family = AF_INET;
	stAddrServer.sin_port = htons(avtpPort);
	stAddrServer.sin_addr.s_addr = inet_addr(mServerIP.c_str());
	pUdpSocket->sendTo(&audioFrame, sizeof(audioFrame_t), &stAddrServer);

	//cout << "Call AvtpAudioClient::sendAudioFrame() end." << endl;
	return 0;
}

/*
	功能：	接收音频帧。
	返回：	返回音频帧实际的长度。
	注意：	
*/
int AvtpAudioClient::recvAudioFrame(void *buf, size_t len)
{
	//cout << "Call AvtpAudioClient::recvAudioFrame()." << endl;
	int ret = 0;
	audioFrame_t audioFrame = {0};
	ret = audioQueue.pop(&audioFrame);
	if(0 != ret)
	{
		return 0;
	}

	unsigned min = 0;
	min = ((len > audioFrame.frameSize) ? audioFrame.frameSize : len);
	memcpy(buf, audioFrame.dataBuf, min);
	
	//cout << "Call AvtpAudioClient::recvAudioFrame() end." << endl;
	return min;
}

int AvtpAudioClient::thListening(void *pThis)
{
	return ((AvtpAudioClient *)pThis)->listening();
}

/*
	功能：	服务器接收线程，用于监听客户端的消息。
	返回：	返回0.
	注意：	
*/
int AvtpAudioClient::listening()
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
			cerr << "In AvtpAudioClient::listening(). Fail to call select(2), " << strerror(errno) << endl;
			continue;
		}
		else if(0 == ret)		// Timeout
		{
			cout << "In AvtpAudioClient::listening(). Timeout!" << endl;
			continue;
		}
		else
		{
			// do next;
		}

		// step2 接收数据。
		memset(&audioFrame, 0, sizeof(videoSlice_t));
		memset(&stAddrClient, 0, sizeof(struct sockaddr));		// 必须进行memset 以便更新client 信息。避免掉线不能重连。
		ret = pUdpSocket->recvFrom(&audioFrame, sizeof(audioFrame_t), &stAddrClient);
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpSocket->recv() in AvtpAudioClient::listening()." << endl;
			continue;
		}
		else if(ret < sizeof(avtpCmd_t))
		{
			cout << "In AvtpAudioClient::listening(). Received data length is not as expected." << endl;
			continue;
		}

		// 处理数据。要尽可能快，保障最短时间内再次调用select. 避免UDP 数据堆积导致丢包。
		switch(audioFrame.avtpDataType)
		{
			case avtpDataType::TYPE_AV_AUDIO:
			{
				//cout << "IP: " << clientIp << ", freame ID = " << videoSlice.frameID << ", seqence = " << videoSlice.sliceSeq << endl;
				audioQueue.push(&audioFrame);
				break;
			}
			case avtpDataType::TYPE_CMD_ReqStartTalk:
			{
				cout << "AvtpAudioClient::listening(). Recv TYPE_CMD_ReqStartTalk." << endl;
				bAllowTalking = true;
				break;
			}
			case avtpDataType::TYPE_CMD_ReqStopTalk:
			{
				cout << "AvtpAudioClient::listening(). Recv TYPE_CMD_ReqStopTalk." << endl;
				bAllowTalking = false;
				break;
			}
			case avtpDataType::TYPE_AV_VIDEO:
			{
				cerr << "Receive video data in AvtpAudioServer." << endl;
				break;
			}
			default:
			{
				cout << "In AvtpAudioClient::listening(). Received datetype = " << audioFrame.avtpDataType << endl;
				break;
			}
		}
	}

	return 0;
}

