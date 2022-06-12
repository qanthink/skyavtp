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
		memset(&avtpCmd, 0, sizeof(avtpCmd_t));

		if(!bConnected && bRunning)
		{
#if 1
			/*	遇到过windows 和linux 网络都正常，但socket 无法建立通信的奇怪情况。此时需要windows 先给linux发个信息。
				但也不要频繁发，导致重连失败问题。当server 的bConnected == false 而client 的bConnected == true 时，
				重复发送导致client 的peek() 函数能够接收到数据，则client 始终判断没有掉线。
			*/
			static int sk = 0;
			if(500 == ++sk)
			{
				sk = 0;
				pUdpSocket->send(&avtpCmd, sizeof(avtpCmd_t));
			}
#endif
			this_thread::sleep_for(chrono::microseconds(10 * 1000));
			ret = pUdpSocket->recvNonblock(&avtpCmd, sizeof(avtpCmd_t));
			if(-1 == ret)
			{
				static int sj = 0;
				if(50 == ++sj)
				{
					sj = 0;
					cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoServer::stateMachine()." << endl;
				}
				continue;
			}

			cout << strDestIP << ", judge: TYPE_CMD_ReqHand != avtpCmd.avtpDataType ?" << endl;
			if(avtpDataType::TYPE_CMD_ReqHand != avtpCmd.avtpDataType)	// 没有收到连接请求，继续等待。
			{
				cout << "TYPE_CMD_ReqHand != avtpCmd.avtpDataType" << endl;
				continue;
			}

			agreeHandShake();
			curFrameID++;
			reqNextFrm();
		}

#if 1	// 超时重连机制
		static unsigned int sTimeOutCnt = 0;					// 超时次数。
		const unsigned int timeOutMs = 100;						// 单次超时时长。
		const unsigned int totalTimeOutMs = 5 * 1000;			// 总超时时长。

		//ret = pUdpSocket->checkTimeOut(2 * 1000 * 1000);	// 在有线网络下，测试过2秒效果不错，暂时保留。
		ret = pUdpSocket->checkTimeOutMs(timeOutMs);			// n * 1000 = n MS.
		if(1 == ret)
		{
			//cout << "Avtp server timeout." << endl;
			reqNextFrm();
			sTimeOutCnt++;
			if(timeOutMs * sTimeOutCnt > totalTimeOutMs)		// 超时重连
			{
				cerr << "timeOutMs * sTimeOutCnt > totalTimeOutMs. bConnected = 0." << endl;
				sTimeOutCnt = 0;
				bConnected = false;
				videoSliceGroupClear();
				curFrameID = 0;
				curFrameSize = 0;
				continue;
			}

			continue;
		}
		else
		{	// 未超时则清空计数器，并继续执行剩余业务逻辑。
			//cout << "Reset timeout counter." << endl;
			sTimeOutCnt = 0;
		}
#endif

		// 窥探报头而不从缓存中移除数据。
		ret = pUdpSocket->peek(&avtpCmd, sizeof(avtpCmd_t));
		if(-1 == ret)
		{
			//cerr << "Fail to call peek()." << endl;
		}
		else if(-2 == ret)
		{
			//cout << "Peek nothing." << endl;
			continue;
		}
		//cout << "Peek Data. Type = " << avtpCmd.avtpDataType << endl;

		switch(avtpCmd.avtpDataType)
		{
			case avtpDataType::TYPE_CMD_ReqHand:	// server 未判断为断开，但client 判断为断开。此时会存在client 请求握手的现象。
			{
				ret = pUdpSocket->recvNonblock(&avtpCmd, sizeof(avtpCmd_t));
				if(-1 == ret)
				{
					cerr << "Fail to call pUdpSocket->recvNonblock() in AvtpVideoServer::stateMachine()." << endl;
					break;
				}

				agreeHandShake();
				reqNextFrm();
				break;
			}
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
				ret = answerSliceAck(videoSlice.frameID, videoSlice.sliceSeq);
				if(-1 == ret)
				{
					cerr << "Fail to call answerSliceAck() in AvtpVideoServer::stateMachine()." << endl;
				}

				/* 如果不是期待的帧数据则丢弃,不处理即可 */
				if(videoSlice.frameID != curFrameID)
				{
					//cout << "frameID != curFrameID." << endl;
					break;
				}

				/*	如果是期待的帧数据，则放入slice group 中。
					注意对slice group 的原子操作。			*/
				while(lock.test_and_set(std::memory_order_acquire)); 	// 获得锁;

				int sliceSeq = 0;
				sliceSeq = videoSlice.sliceSeq;
				videoSliceGroup.videoSliceGroup[sliceSeq] = videoSlice;

				if(videoSliceGroupIsFull())		// 如果slice group 已经打包好一组slice, 则通知其它线程使用。
				{
					bAllowPacking = true;
				}
				else
				{
					bAllowPacking = false;
				}

				lock.clear();	// 释放锁
				break;
			}
			default:
			{
				//cout << "Receive unrecognized type! Type = " << avtpCmd.avtpDataType << endl;
				pUdpSocket->relinquish();
				break;
			}
		}
	}
	cout << "Call stateMachine() end." << endl;
	return NULL;
}

#if 0
int AvtpVideoServer::videoSliceClear(const unsigned int frameID, unsigned int sliceSeq)
{
	//cout << "Call AvtpVideoServer::videoSliceClear()." << endl;

	if(sliceSeq > videoSliceGroup_t::groupMaxSize)
	{
		cerr << "Fail to call AvtpVideoServer::videoSliceClear(). Argument is out of range()." << endl;
		cerr << "sliceSeq = " << sliceSeq << ", videoSliceGroup_t::groupMaxSize = " << videoSliceGroup_t::groupMaxSize << endl;
	}

	while(lock.test_and_set(std::memory_order_acquire));	// 获得锁;

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

	lock.clear();// 释放锁

	//cout << "Call AvtpVideoServer::videoSliceClear() end." << endl;
	return 0;
}
#endif

#if 0
int AvtpVideoServer::videoSliceGroupClear()
{
	//cout << "Call AvtpVideoServer::videoSliceGroupClear()." << endl;

	int i = 0;
	for (i = 0; i < videoSliceGroup.groupMaxSize; ++i)
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

	//cout << "Call AvtpVideoServer::videoSliceGroupClear() end." << endl;
	return 0;
}
#endif

