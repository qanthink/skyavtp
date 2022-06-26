/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#include "udp_server.h"
#include "avtp_datatype.h"

#include <memory>
#include <map>
#include <mutex>
#include <condition_variable>


class ClientProc
{
public:
	ClientProc();
	int create();
	~ClientProc();

	int push(const videoSlice_t *pVideoSlice);
	int pop(void *frameBuf, const unsigned int frameBufLen);
	bool isGroupFull() const;

private:
	std::atomic_flag mLock = ATOMIC_FLAG_INIT;		// 原子对象，保障原子操作。
	std::mutex mMtx;
	std::condition_variable mCondVar;
	
	// 数据处理线程
	int recvHandler();
	static int thRecvHandler(void *pThis);
	std::shared_ptr<std::thread> pThRecvHandler = NULL;

	videoSliceGroup_t videoSliceGroup;
};

class clientInf_t{
public:
	clientInf_t(){};
	unsigned int frameIdRequest;
	videoSliceGroup_t videoSliceGroup;
	ClientProc clientProc;
	std::atomic_flag mLock;
private:
};

class AvtpVideoServer
{
public:
	AvtpVideoServer(const char *serverIp);

	bool addClient(std::string clientIp);
	bool deleteClient(std::string clientIp);
	bool queryClient(std::string clientIp);

	int recvVideoFrame(std::string clientIp, void *frameBuf, const unsigned int frameBufSize);

private:
	std::shared_ptr<UdpServer> pUdpServer = NULL;			// UDP 服务器
	const unsigned short avtpPort = AVTP_PORT;				// 端口号
	std::map<std::string, clientInf_t> clientPool;	// 客户端IP 池。
	std::map<std::string, std::shared_ptr<ClientProc>> clientPool2;	// 客户端IP 池。
	unsigned int mFrameID = 0;
	
	// 服务器接收线程
	int listening();
	static int thListening(void *pThis);
	std::shared_ptr<std::thread> pThServerRecv = NULL;
	std::atomic_flag mLock = ATOMIC_FLAG_INIT;				// 原子对象，保障原子操作。

	int pushSliceIntoGroup(std::string clientIp, const videoSlice_t *pVideoSlice);
	int popFrameFromGrup(std::string clientIp, void *frameBuf, const unsigned int frameBufSize);
	int clearGroup(std::string clientIp);
	bool isGroupFull(std::string clientIp);
	int requestNxtFrame(std::string clientIp, unsigned int frameID);
};


