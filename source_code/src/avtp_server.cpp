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

#ifdef _WIN64
#include <windows.h>
#else
#include <sys/time.h>
#endif

/*
	功能：	服务器状态机。回应客户端发来的信息。
	注意：	
*/
void *AvtpVideoServer::stateMachine(void *arg)
{
	cout << "Call server stateMachine()." << endl;

	while(bRunning)
	{
		int ret = 0;

		avtpCmd_t avtpCmd;
		const unsigned int ipAddrLen = 16;
		char ipAddr[ipAddrLen] = {0};
		// 窥探报头而不从缓存中移除数据。
		ret = pUdpSocket->peek(&avtpCmd, sizeof(avtpCmd_t), ipAddr, ipAddrLen);
		if(-1 == ret)
		{
			cerr << "Fail to call peek()." << endl;
		}
		else if(-2 == ret)
		{
			//cout << "Peek nothing." << endl;
			//this_thread::sleep_for(chrono::microseconds(100));
			continue;
		}
		//cout << "Peek Data. Type = " << avtpCmd.avtpDataType << endl;

		//cout << ipAddr << endl;

		switch(avtpCmd.avtpDataType)
		{
			case avtpDataType::TYPE_AV_VIDEO:		// 收到client 的视频流数据。
			{
				videoSlice_t videoSlice;
				ret = pUdpSocket->recvNonblock(&videoSlice, sizeof(videoSlice_t));
				if(-1 == ret)
				{
					cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoServer::stateMachine()." << endl;
					break;
				}
#if 0		// debug. 报头打印
				cout << "videoSlice.frameID = " << videoSlice.frameID << ", videoSlice.frameSize = " << videoSlice.frameSize
					<< ", videoSlice.sliceSeq = " << videoSlice.sliceSeq << ", videoSlice.sliceSize = " << videoSlice.sliceSize << endl;
#endif
				/*
					先回复ACK, 优先保障client 收到回应，减小延时。再进行本地数据处理。
					收到的每个Slice, 无论是首次收到，还是重复收到，都要回复ACK. 避免由于ACK 丢失，client 循环发送。
				*/
				cout << "frameID = " << videoSlice.frameID << endl;
				ret = answerSliceAck(videoSlice.frameID, videoSlice.sliceSeq, ipAddr);
				if(-1 == ret)
				{
					cerr << "Fail to call answerSliceAck() in AvtpVideoServer::stateMachine()." << endl;
				}
				
				break;
			}
			default:
			{
				cout << "Receive unrecognized type! Type = " << avtpCmd.avtpDataType << endl;
				pUdpSocket->relinquish();
				break;
			}
		}

		//this_thread::sleep_for(chrono::microseconds(1));
	}
	cout << "Call stateMachine() end." << endl;
	return NULL;
}
