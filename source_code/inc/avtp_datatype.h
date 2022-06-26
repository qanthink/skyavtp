#pragma once

#include "udp_client.h"

#define AVTP_PORT 1000
#define SLICE_HEAD_SIZE (8 * 4)

/*
	概念约定：
	====帧Frame ：H.26X 的I 帧和P 帧，以及MJPEG 的每个帧，都为帧，记作Frame.
	====片Slice ：把帧数据分割成若干份，每份中再补充与协议相关的私有数据，则形成了多份片数据Slice.
	====片组SliceGroup: 一帧Frame 分割出来的若干片Slice, 形成了一个片组SliceGroup.
*/

/*
	传输协议的数据类型，位于一包数据中的第一个int 字节中。
	不同类型对应不同的数据包格式，需要不同的解析策略。
*/
class avtpDataType{
public:
	const static unsigned int TYPE_INVALID = 0;
	
	const static unsigned int TYPE_CMD_ACK = 1;
	const static unsigned int TYPE_CMD_ReqHand = 2;
	const static unsigned int TYPE_CMD_AgeHand = 3;
	const static unsigned int TYPE_CMD_ReqNextFrm = 4;
	
	const static unsigned int TYPE_AV_AUDIO = 8;
	const static unsigned int TYPE_AV_VIDEO = 9;
};

class avtpCmd_t{
public:
	unsigned int avtpDataType = avtpDataType::TYPE_INVALID;
	unsigned int avtpData[3] = {0};
};

/*	TYPE 决定了后续数据的排列格式：
	TYPE_INVALID:
		后续数据无效，直接丢弃。
		
	TYPE_CMD_ACK:
		avtpData[0] 为frameID.
		avtpData[1] 为sliceSeq.
		avtpData[2] 预留.

	TYPE_CMD_ReqHand:
		avtpData[0] 预留.
		avtpData[1] 预留.
		avtpData[2] 预留.

	TYPE_CMD_AgeHand:
		avtpData[0] 预留.
		avtpData[1] 预留.
		avtpData[2] 预留.

	TYPE_CMD_ReqNextFrm:
		avtpData[0] frameID.
		avtpData[1] 预留.
		avtpData[2] 预留.
*/

/*
	TYPE_AV_VIDEO:
	
	UDP包的正文内容，需要限制在548 Bytes以内(Internet环境)，或1472 Bytes 以内(局域网环境)。
	在一片Slice 中，一部分字节用作了私有协议头，所以留给视频数据的空间不足548(或1472) Bytes.
	本协议约定数据头占32 Bytes字节，则视频数据的长度为516(或1440) Bytes.
*/
class videoSlice_t{
public:
	//const static unsigned int sliceBufMaxSize = 516;
	const static unsigned int sliceBufMaxSize = 1440;
	//const static unsigned int sliceBufMaxSize = 1440 * 2;		// 2880
	//const static unsigned int sliceBufMaxSize = 1440 * 3;		// 4320
	//const static unsigned int sliceBufMaxSize = 1440 * 4;		// 5600
	//const static unsigned int sliceBufMaxSize = 1440 * 5;		// 7000
	//const static unsigned int sliceBufMaxSize = 1440 * 6;		// 8400
	//const static unsigned int sliceBufMaxSize = 1440 * 7;		// 9800
	//const static unsigned int sliceBufMaxSize = 1440 * 8;		// 11200
	//const static unsigned int sliceBufMaxSize = 1440 * 12;	// 16800
	//const static unsigned int sliceBufMaxSize = 1440 * 16;	// 22400
	//const static unsigned int sliceBufMaxSize = 1440 * 24;	// 33600

public:
	unsigned int avtpDataType = avtpDataType::TYPE_INVALID;	// 第0个int: 数据类型。
	unsigned int frameID = 0;								// 第1个int: 帧ID, 唯一标识一个Frame.
	unsigned int sliceSeq = 0;								// 第2个int: 片序号，决定了在一个Frame 中的位置。
	unsigned int frameSize = 0;								// 第3个int: 帧长度。
	unsigned int sliceSize = 0;								// 第4个int: 片大小。
	unsigned int reserve0 = 0;								// 第5个int: 保留。
	unsigned int reserve1 = 0;								// 第6个int: 保留。
	unsigned int reserve2 = 0;								// 第7个int: 保留。
	unsigned char sliceBuf[sliceBufMaxSize];				// 第8个int起: 视频数据。
};

/*
	片组：包含多个slice, 在接收端可以解析重组为一帧Frame.
	
	H.265 FHD30 的码率通常在2Mbps. I帧大小通常不超过64KB.
	H.265 4K30 对应8Mbps, I帧 256KB. 故而暂且认为512KB 的空间足够容纳一帧图像。
	把一帧Frame图像打包，拆分成若干Slice, 每个Slice中的有效数据为512B(或1436B), 
	则需要512KB * 1024 / 512B(或1436B) = 1024(366)个Slice.
*/
class videoSliceGroup_t{
public:
	const static unsigned int h26xFrameMaxBytes = 128 * 1024;	// FHD 32KB, UHD 128KB.
	//const static unsigned int groupMaxSize = ceil((double)h26xMaxFrameBytes / videoSlice_t::sliceBufMaxSize);	// 取整
	const static unsigned int groupMaxSize = h26xFrameMaxBytes / videoSlice_t::sliceBufMaxSize + 1;

public:
	videoSlice_t videoSlice[groupMaxSize];
};


