/*---------------------------------------------------------------- 
版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

/*
*/

#include <iostream>
#include <string.h>
#include "avtp_client.h"

using namespace std;

/*
	功能：	客户端状态机。根据服务器反馈的ACK 等信息，控制发送行为。
	注意：	
*/
void *AvtpVideoClient::stateMachine(void *arg)
{
	cout << "Call client stateMachine()." << endl;

	while(bRunning)
	{
		int ret = 0;
		avtpCmd_t avtpCmd;

		if(!bConnected)
		{
			requestHandShake();
			waitHandShakeAgree();
			continue;
		}

		//this_thread::sleep_for(chrono::microseconds(1));	// sleep 影响UDP 接收效率。
		memset(&avtpCmd, 0, sizeof(avtpCmd_t));
		#if 0	// old 无重连
		ret = pUdpSocket->peek(&avtpCmd, sizeof(avtpCmd_t));
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpSocket->peek() in AvtpVideoClient::stateMachine()." << endl;
			continue;
		}
		//cout << "Peek Data. Type = " << avtpCmd.avtpDataType << endl;
		#else	// new 有重连
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

		this_thread::sleep_for(chrono::nanoseconds(1));	// sleep 影响UDP 接收效率，但如果没有的话，CPU 负载又太高。
		memset(&avtpCmd, 0, sizeof(avtpCmd_t));
		ret = pUdpSocket->peek(&avtpCmd, sizeof(avtpCmd_t));
		if(-1 == ret)
		{
			cerr << "Fail to call pUdpSocket->peek() in AvtpVideoClient::stateMachine()." << endl;
			//continue;
		}
		else if(-2 == ret)
		{
			//cout << "Peek nothing." << endl;
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
					cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoClient::stateMachine()." << endl;
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
					//videoSliceGroupClear();		// 应该需要注释掉。不注释会怎样不知道。
					curFrameID = avtpCmd.avtpData[0];
					bAllowPacking = true;
					//cout << "avtpCmd.avtpData[0] = " << avtpCmd.avtpData[0] << endl;
				}
				break;
			}
			case avtpDataType::TYPE_CMD_ACK:
			{
				ret = pUdpSocket->recvNonblock(&avtpCmd, sizeof(avtpCmd_t));
				if(-1 == ret)
				{
					cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoClient::stateMachine()." << endl;
					break;
				}

				#if 0
				//cout << "avtpCmd.avtpDataType = " << avtpCmd.avtpDataType << endl;
				cout << "avtpCmd.avtpData[0], avtpCmd.avtpData[1] = " << avtpCmd.avtpData[0] << ", " << avtpCmd.avtpData[1]<< endl;
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
				//cerr << "Error in AvtpVideoClient::stateMachine(). AVTP_TYPE is unrecognized. TYPE = " << avtpCmd.avtpDataType << endl;
				pUdpSocket->relinquish();
				break;
		}
	}

	return NULL;
}

#if 0
int AvtpVideoClient::videoSliceClear(const unsigned int frameID, const unsigned int sliceSeq)
{
	//cout << "Call AvtpVideoClient::videoSliceClear()." << endl;

	if(sliceSeq > videoSliceGroup_t::groupMaxSize)
	{
		cerr << "Fail to call AvtpVideoClient::videoSliceClear(). Argument is out of range()." << endl;
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

	lock.clear();	// 释放锁
	
	//cout << "Call AvtpVideoClient::videoSliceClear() end." << endl;
	return 0;
}
#endif

#if 0
int AvtpVideoClient::videoSliceGroupClear()
{
	//cout << "Call AvtpVideoClient::videoSliceGroupClear()." << endl;
	while(lock.test_and_set(std::memory_order_acquire));		// 获得锁;
	
	int i = 0;
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

	lock.clear();	// 释放锁
	
	//cout << "Call AvtpVideoClient::videoSliceGroupClear() end." << endl;
	return 0;
}
#endif

