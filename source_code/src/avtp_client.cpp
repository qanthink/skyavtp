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

		this_thread::sleep_for(chrono::nanoseconds(1));	// sleep 影响UDP 接收效率，但如果没有的话，CPU 负载又太高。
		memset(&avtpCmd, 0, sizeof(avtpCmd_t));
		const unsigned int ipAddrLen = 16;
		char ipAddr[ipAddrLen] = {0};
		// 窥探报头而不从缓存中移除数据。
		//cout << "peek" << endl;
		ret = pUdpSocket->peek(&avtpCmd, sizeof(avtpCmd_t), ipAddr, ipAddrLen);
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
		//cout << ipAddr << endl;
		
		switch(avtpCmd.avtpDataType)
		{
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

					//if(videoSliceGroupIsEmpty())
					//{
					//	bAllowPacking = true;
					//}
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

