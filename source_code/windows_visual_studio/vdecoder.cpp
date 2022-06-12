#include <iostream>
#include <fstream>
#include <thread>
#include "vdecoder.h"

using namespace std;

/*	该回调函数，如果没有一次性用完buffer 空间，则ffmpeg 不会分配新空间。
	曾经出现的错误是，用了一个128KB 的缓存空间，每次只接收20KB 左右的视频帧。
	导致buffSize 递减，在剩余的最后10KB 空间中，无法完整拷贝20KB 的视频帧。
*/
static int readPacket(void *opaque, uint8_t *buf, int bufSize)
{
	cout << "Call readPacket()." << endl;
	cout << "bufSize = " << bufSize << endl;

	int realSize = 0;
	VDecoder *pThis = (VDecoder*)opaque;
	realSize = pThis->__receive(buf, bufSize);
	
	realSize = realSize < bufSize ? realSize : bufSize;
	if(0 == realSize)
	{
		cerr << "Fail to call pThis->__receive() in readPacket(). 0 == realSize." << endl;
		return AVERROR_EOF;
	}
	cout << "Call readPacket() end. realSize = " << realSize << endl;

#if 0	// debug. Print context.
	int i = 0;
	for(i = 0; i < stH26xFrame.frameCurSize; ++i)
	{
		cout.fill(' ');
		cout.width(4);
		cout << hex << (int)(unsigned char)stH26xFrame.frameBuf[i];

		if(15 == i % 16)
		{
			cout << endl;
		}
	}
#endif

	return bufSize;
}

int VDecoder::__receive(unsigned char *pData, unsigned int size)
{
	//cout << "Call  VDecoder::__receive()" << endl;
	unsigned int realSize = 0;
	do
	{
		//memcpy(pData, ___, realSize);
		this_thread::sleep_for(chrono::microseconds(1));
	} while(bRunning);

	return realSize;
}

VDecoder::VDecoder()
{
	cout << "Call VDecoder::VDecoder()." << endl;
	av_register_all();
	bRunning = true;
	cout << "Call VDecoder::VDecoder() end." << endl;
}

VDecoder::~VDecoder()
{
	cout << "Call VDecoder::~VDecoder()." << endl;
	deInit();
	cout << "Call VDecoder::~VDecoder() end." << endl;
}

void VDecoder::stop()
{
	cout << "Call VDecoder::stop()." << endl;
	bRunning = false;
	cout << "Call VDecoder::stop() end." << endl;
}

void VDecoder::begin()
{
	cout << "Call VDecoder::begin()." << endl;
	bRunning = true;
	cout << "Call VDecoder::begin() end." << endl;
}

bool VDecoder::isRunning() const
{
	return bRunning;
}

/*
	功能：	反初始化。
*/
int VDecoder::deInit()
{
	cout << "Call VDecoder::deInit()." << endl;
	
	bool bWHInited = false;
	bool bRunning = false;
	bool bInited = false;

	//	av_frame_free(&avFrame);
	//	av_packet_free(&avPacket);
	//	avcodec_free_context(&avCodecCtx);
	//	avformat_close_input(&avFormatCtx);
	//	avformat_free_context(avFormatCtx);
	if(NULL != avFrame)
	{
		av_frame_free(&avFrame);
		avFrame = NULL;
	}

	if(NULL != avPacket)
	{
		av_packet_free(&avPacket);
		avPacket = NULL;
	}

	if(NULL != avCodecCtx)
	{
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
	}

	if(NULL != avCodec)
	{
		avCodec = NULL;
	}

	if(NULL != avFormatCtx)
	{
		//avformat_close_input(&avFormatCtx);
		//avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
	}

	width = 0;
	height = 0;
	videoIndex = -1;

	cout << "Call VDecoder::deInit() end." << endl;
	return 0;
}

/*
	功能：	使用指定参数初始化解码器，参数为解码类型。
			取值有AV_CODEC_ID_H264, AV_CODEC_ID_H265 等。参考"libavcodec/codec_id.h".
	返回：	成功，返回0；失败，返回-1.
	注意：	1.为若干结构体分配了空间，为了避免内存泄漏，请在使用结束时逐一释放。
			2.初始化成功后，需要通过sendData() 为解码器输送一帧完整视频帧，从而在解码器内部获取解码分辨率。
			3.注意2 中提到的完整视频帧，针对H.26X 是I 帧。
			4.后续对应的获取解码数据的方法为recvDataFromFile()
*/
int VDecoder::initWithArg(AVCodecID avCodecID)
{
	// 寻找解码器。
	avCodec = avcodec_find_decoder(avCodecID);
	if(NULL == avCodec)
	{
		cerr << "Fail to call avcodec_find_decoder() in VDecoder::initWithArg(). Can't find codec." << endl;
		return -1;
	}

	// 为解码器分配空间。
	avCodecCtx = avcodec_alloc_context3(avCodec);
	if(NULL == avCodecCtx)
	{
		cerr << "Fail to call avcodec_alloc_context3() in VDecoder::initWithArg()." << endl;
		return -1;
	}

	// 打开解码器。
	int ret = 0;
	ret = avcodec_open2(avCodecCtx, avCodec, NULL);
	if(0 != ret)
	{
		cerr << "Fail to call avcodec_open2() in VDecoder::initWithArg()." << endl;
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
		return -1;
	}

	// 为数据包分配空间。
	avPacket = av_packet_alloc();
	if(NULL == avPacket)
	{
		cerr << "Fail to call av_packet_alloc() in VDecoder::initWithArg()." << endl;
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
		return -1;
	}

	// 为帧数据分配空间。
	avFrame = av_frame_alloc();
	if(NULL == avFrame)
	{
		cerr << "Fail to call av_packet_alloc() in VDecoder::initWithArg()." << endl;
		av_packet_free(&avPacket);
		avPacket = NULL;
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
		return -1;
	}

	//av_frame_free(&avFrame);
	//av_packet_free(&avPacket);
	//avcodec_free_context(&avCodecCtx);

	bInited = true;
	return 0;
}

/*
	功能：	用文件初始化解码器。
	返回：	成功，返回0；失败，返回错误码。
	注意：	对应的获取解码数据的方法为recvDataFromFile();
*/
int VDecoder::initWithFile(const char *filePath)
{
	if(NULL == filePath)
	{
		cerr << "Fail to call VDecoder::initWithFile(). Argument has null value." << endl;
		return -1;
	}

	// step1: init ffmpeg
	av_register_all();
	avFormatCtx = avformat_alloc_context();
	if(NULL == avFormatCtx)
	{
		cerr << "Couldn't open input stream." << endl;
		return -1;
	}

	// step2: open file and find codec information.
	int ret = 0;
	ret = avformat_open_input(&avFormatCtx, filePath, NULL, NULL);
	if(ret != 0)
	{
		cerr << "Couldn't open input stream." << endl;
		avFormatCtx = NULL;
		return -2;
	}

	// step3: 寻找流信息并显示。
	ret = avformat_find_stream_info(avFormatCtx, NULL);
	if(ret < 0)
	{
		cerr << "Couldn't find stream information." << endl;
		avformat_close_input(&avFormatCtx);
		avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
		return -3;
	}
	av_dump_format(avFormatCtx, 0, NULL, 0);

	// step4: 在流信息中寻找音、视频流的索引值。
	int i = 0;
	for(i = 0; i < avFormatCtx->nb_streams; ++i)
	{
		if(AVMEDIA_TYPE_VIDEO == avFormatCtx->streams[i]->codec->codec_type)
		{
			videoIndex = i;
		}
	}

	if(-1 == videoIndex)
	{
		cerr << "Didn't find any audio or video stream." << endl;
		avformat_close_input(&avFormatCtx);
		avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
		return -3;
	}
	cout << "videoIndex = " << videoIndex << endl;
	cout << "width = " << width << ", height = " << height << endl;
	width = avFormatCtx->streams[videoIndex]->codec->width;
	height = avFormatCtx->streams[videoIndex]->codec->height;

	// step5: 为音视频寻找合适的解码器，并打开解码器。
	avCodecCtx = avFormatCtx->streams[videoIndex]->codec;
	if(NULL == avCodecCtx)
	{
		cerr << "Video codec contex not found." << endl;
		avformat_close_input(&avFormatCtx);
		avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
		return -1;
	}

	avCodec = avcodec_find_decoder(avCodecCtx->codec_id);
	if(NULL == avCodec)
	{
		cerr << "Video codec not found." << endl;
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
		avCodec = NULL;
		avformat_close_input(&avFormatCtx);
		avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
		return -1;
	}

	//avcodec_alloc_context3();
	ret = avcodec_open2(avCodecCtx, avCodec, NULL);
	if(ret < 0)
	{
		cerr << "Could not open video codec." << endl;
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
		avCodec = NULL;
		avformat_close_input(&avFormatCtx);
		avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
	}

	// step5: 为AVPacket 和 AVFrame 对象分配空间。
	avPacket = av_packet_alloc();
	if(NULL == avPacket)
	{
		cerr << "Fail to call av_packet_alloc() in VDecoder::recvDataFromFile()." << endl;
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
		avCodec = NULL;
		avformat_close_input(&avFormatCtx);
		avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
		return -1;
	}

	avFrame = av_frame_alloc();
	if(NULL == avFrame)
	{
		cerr << "Fail to call av_frame_alloc() in Decoder::recvDataFromFile()." << endl;
		av_packet_free(&avPacket);
		avPacket = NULL;
		avcodec_free_context(&avCodecCtx);
		avCodecCtx = NULL;
		avCodec = NULL;
		avformat_close_input(&avFormatCtx);
		avformat_free_context(avFormatCtx);
		avFormatCtx = NULL;
		return -1;
	}

	bInited = true;
	return 0;
}

/*	
	功能：	向解码器中添加已编码的数据。
	返回：	成功，返回0；失败，返回-1
	注意：	如果送进去的第一帧不是I 帧，则有可能出现如下打印，是正常现象：
		[hevc @ 000001ed18500b80] PPS id out of range: 0
		[hevc @ 000001ed18500b80] Error parsing NAL unit #0.
*/
int VDecoder::sendData(const unsigned char *pData, unsigned int size)
{
	// 拷贝数据到AVPacket 包。
	avPacket->size = size;
	avPacket->data = (unsigned char*)av_malloc(avPacket->size);
	memcpy(avPacket->data, pData, size);

	// 初始化AVPacket 包（数据会被送到ffmpeg 底层）。
	int ret = 0;
	ret = av_packet_from_data(avPacket, avPacket->data, avPacket->size);
	if(0 != ret)
	{
		cerr << "Fail to call av_packet_from_data() in VDecoder::sendData()." << endl;
		av_free(avPacket->data);
		return -1;
	}

	// 将打包的数据送进解码器。
	ret = avcodec_send_packet(avCodecCtx, avPacket);
	if(0 != ret)
	{
		cerr << "Fail to call avcodec_send_packet() in VDecoder::sendData()." << endl;
		av_packet_unref(avPacket);
		return -1;
	}
	av_packet_unref(avPacket);

	if(!bWHInited)
	{
		width = avCodecCtx->width;
		height = avCodecCtx->height;
		cout << "Decoder initialized succeded. Resolution = " << width << "x" << height << endl;

		bWHInited = true;
	}

	return 0;
}

/*	
	功能：	从解码器中获取解码后的数据。
	返回：	失败，返回-1；成功，返回YUV 的长度。
*/
int VDecoder::recvData(unsigned char *pData, unsigned int size)
{
	if(NULL == pData || NULL == size)
	{
		cerr << "Fail to call VDecoder::recvData(). Argument has null value." << endl;
		return -1;
	}

	// 从ffmpeg 获取解码后的YUV 数据。
	int ret = 0;
	ret = avcodec_receive_frame(avCodecCtx, avFrame);
	if(0 != ret)
	{
		cerr << "Fail to call avcodec_receive_frame() in VDecoder::recvData()." << endl;
		return -1;
	}

	// 判断上层空间是否足够存放YUV 数据。
	unsigned int yuvLen = 0;
	yuvLen = avFrame->width * avFrame->height * this->yuvPixelByte;
	if(size < yuvLen)
	{
		cerr << "Fail to call VDecoder::recvData(). No enough space for receiving YUV data. The minimum space requirement is " << yuvLen << endl;
		return -1;
	}

	// 写入数据到上层
	int i = 0;
	int j = 0;
	for(i = 0; i < avFrame->height; ++i)
	{
		memcpy(pData + j, avFrame->data[0] + i * avFrame->linesize[0], avFrame->width);
		j += avFrame->width;
	}
	for(i = 0; i < avFrame->height / 2; ++i)
	{
		memcpy(pData + j, avFrame->data[1] + i * avFrame->linesize[1], avFrame->width / 2);
		j += avFrame->width / 2;
	}
	for(i = 0; i < avFrame->height / 2; ++i)
	{
		memcpy(pData + j, avFrame->data[2] + i * avFrame->linesize[2], avFrame->width / 2);
		j += avFrame->width / 2;
	}

	return yuvLen;
}

int VDecoder::recvDataFromFile(unsigned char *pData, unsigned int size)
{
	if(NULL == pData || NULL == size)
	{
		cerr << "Fail to call VDecoder::recvData(). Argument has null value." << endl;
		return -1;
	}

	int ret;
	ret = av_read_frame(avFormatCtx, avPacket);
	if(0 != ret)
	{
		cerr << "Fail to call av_read_frame(). Read packet is NULL." << endl;
		return -1;
	}
	cout << "Read success. avPacket.size = " << avPacket->size << endl;

	if(videoIndex != avPacket->stream_index)
	{
		cerr << "Error in Decoder::recvDataFromFile(). videoIndex != videoPacket.stream_index." << endl;
		return -1;
	}

	ret = avcodec_send_packet(avCodecCtx, avPacket);
	if(0 != ret)
	{
		cerr << "Fail to call avcodec_send_packet() in Decoder::recvDataFromFile()." << endl;
		return -1;
	}
	
	ret = avcodec_receive_frame(avCodecCtx, avFrame);
	if(0 != ret)
	{
		cerr << "Fail to call avcodec_receive_frame() in Decoder::recvDataFromFile()." << endl;
		return -1;
	}
	cout << "frame->width = " << avFrame->width << ", frame->height = " << avFrame->height << endl;

	const unsigned int yuvLen = avFrame->width * avFrame->height * yuvPixelBytes();
	if(yuvLen > size)
	{
		cerr << "Fail to call Decoder::getYUV420Frame(). Argument yuvBuf and yuvBufLen are less than need." << endl;
		return -1;
	}

	int i = 0;
	int a = 0;
	for(i = 0; i < avFrame->height; i++)
	{
		memcpy(pData + a, avFrame->data[0] + i * avFrame->linesize[0], avFrame->width);
		a += avFrame->width;
	}
	for(i = 0; i < avFrame->height / 2; i++)
	{
		memcpy(pData + a, avFrame->data[1] + i * avFrame->linesize[1], avFrame->width / 2);
		a += avFrame->width / 2;
	}
	for(i = 0; i < avFrame->height / 2; i++)
	{
		memcpy(pData + a, avFrame->data[2] + i * avFrame->linesize[2], avFrame->width / 2);
		a += avFrame->width / 2;
	}

	return 0;
}

/*
	功能：	返回解码视频的宽。
*/
unsigned int VDecoder::widths() const
{
	return width;
}

/*
	功能：	返回解码视频的高。
*/
unsigned int VDecoder::heights() const
{
	return height;
}

/*
	功能：	返回YUV 单个像素的空间需求。
		一般YUV420 是1.5Bytes/Pixel.
*/
double VDecoder::yuvPixelBytes() const
{
	return yuvPixelByte;
}

/*
	功能：	将解析出的视频帧数据，一帧一帧保存到本地文件
	返回：	成功返回保存的帧数量。
	注意：	1.仅对initWithFile() 初始化的数据有效。
			2.保存的是已编码数据，非解码后的数据。

*/
unsigned int VDecoder::saveVideoPacket(const char *outFileNamePrefix)
{
	if(NULL == outFileNamePrefix)
	{
		cerr << "Fail to call VDecoder::saveVideoPacket(). Argument has null value." << endl;
		return 0;
	}

	// step1: 从音视频格式上下文中获取一包数据。
	bool flag = true;
	unsigned int cnt = 0;
	do
	{
		int ret = 0;
		AVPacket videoPacket;
		//memset(&videoPacket, 0, sizeof(AVPacket));
		ret = av_read_frame(avFormatCtx, &videoPacket);
		if(ret < 0)
		{
			cerr << "Fail to call av_read_frame(). Read packet is NULL." << endl;
			break;
		}

		if(videoIndex != videoPacket.stream_index)
		{
			cerr << "Fail to call VDecoder::getYUV420Frame(). videoIndex != videoPacket.stream_index." << endl;
			av_packet_unref(&videoPacket);
		}
		cout << "Read success. Size = " << videoPacket.size << endl;

		printf("data[0-9]=%x, %x, %x, %x, %x, %x, %x, %x, %x, %x\n", videoPacket.data[0], videoPacket.data[1], videoPacket.data[2], videoPacket.data[3], videoPacket.data[4], videoPacket.data[5], videoPacket.data[6], videoPacket.data[7], videoPacket.data[8], videoPacket.data[9]);
		cout << "size = " << videoPacket.size << endl;
		//const char *prefix = "C:\\Users\\zgxxg\\Documents\\Visual Studio 2015\\Projects\\ffmpeg_opcv\\samples\\h265\\h265-";
		const unsigned int fileNameLen = 1024;
		char fileName[fileNameLen] = { 0 };
		snprintf(fileName, fileNameLen, "%s%d", outFileNamePrefix, cnt);
		fstream fout;
		fout.open(fileName, ios_base::out | ios_base::binary | ios_base::trunc);
		if(fout.fail())
		{
			cerr << "Fail to open file: " << fileName << endl;
			continue;
		}

		fout << videoPacket.data << endl;
		fout.write((const char *)videoPacket.data, videoPacket.size);
		fout.close();
		cnt++;

		AVFrame *avFrame = NULL;
		avFrame = av_frame_alloc();
		if(NULL == avFrame)
		{
			cerr << "Fail to call av_frame_alloc()." << endl;
			break;
		}

		int got_frame = 0;
		ret = avcodec_decode_video2(avFormatCtx->streams[videoIndex]->codec, avFrame, &got_frame, &videoPacket);

		av_frame_free(&avFrame);
		av_packet_unref(&videoPacket);
	} while(flag);

	return cnt;
}

/*
	功能：	图像色彩域转换。
	返回：	成功，返回0; 失败，返回-1.
*/
int VDecoder::colorConvert(AVPixelFormat avDstFmt, unsigned char* pDstData, AVPixelFormat avSrcFmt, unsigned char* pSrcData, unsigned int width, unsigned int height)
{
	if(NULL == pDstData || NULL == pSrcData || 0 == width || 0 == height)
	{
		cerr << "Fail to call VDecoder::colorConvert(). Argument has null or zero value." << endl;
		return -1;
	}

	AVPicture avPicSrc;
	avpicture_fill(&avPicSrc, pSrcData, avSrcFmt, width, height);

	if(avSrcFmt == AV_PIX_FMT_YUV420P || avSrcFmt == AV_PIX_FMT_YUYV422)
	{
		//U,V互换，不换会偏紫。
#if 1
		unsigned char *pTmp = NULL;
		pTmp = avPicSrc.data[1];
		avPicSrc.data[1] = avPicSrc.data[2];
		avPicSrc.data[2] = pTmp;
#endif
	}

	AVPicture avPicDst;
	avpicture_fill(&avPicDst, pDstData, avDstFmt, width, height);

	struct SwsContext *imgCtx = NULL;
	imgCtx = sws_getContext(width, height, avSrcFmt, width, height, avDstFmt, SWS_BILINEAR, 0, 0, 0);		// SWS_BILINEAR 双线性过滤
	if(NULL == imgCtx)
	{
		sws_freeContext(imgCtx);
		imgCtx = NULL;
		return -1;
	}

	sws_scale(imgCtx, avPicSrc.data, avPicSrc.linesize, 0, height, avPicDst.data, avPicDst.linesize);

	if(NULL != imgCtx)
	{
		sws_freeContext(imgCtx);
		imgCtx = NULL;
	}

	return 0;
}

#if 0
int VDecoder::initWithBuf()
{
	av_register_all();

	unsigned char *avioBuf = NULL;
	const unsigned avioBufSize = 128 * 1024;	// n * 1024 = n KiB
	avioBuf = (unsigned char *)av_malloc(avioBufSize);		// av_freep(&avio_ctx->buffer);
	if(NULL == avioBuf)
	{
		cerr << "Fail to call av_malloc() in VDecoder::initWithBuf()." << endl;
		return -1;
	}

	AVIOContext *avioCtx = NULL;
	avioCtx = avio_alloc_context(avioBuf, avioBufSize, 0, this, readPacket, NULL, NULL);	// avio_context_free
	if(NULL == avioCtx)
	{
		cerr << "Fail to call avio_alloc_context() in VDecoder::initWithBuf()." << endl;
		av_freep(&avioCtx->buffer);
		return -1;
	}

	avFormatCtx = avformat_alloc_context();
	if(NULL == avFormatCtx)
	{
		cerr << "Fail to call avformat_alloc_context() in VDecoder::initWithBuf()." << endl;
		av_freep(&avioCtx->buffer);
		avio_context_free(&avioCtx);
		return -1;
	}
	avFormatCtx->pb = avioCtx;
	avFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;
	cout << "avFormatCtx->format_probesize = " << avFormatCtx->format_probesize << endl;			// 1048576
	cout << "avFormatCtx->format_probesize = " << avFormatCtx->max_analyze_duration << endl;		// 0

	cout << "Call avformat_open_input()" << endl;
	int ret = 0;
	ret = avformat_open_input(&avFormatCtx, "whatever", NULL, NULL);
	if(0 != ret)
	{
		cerr << "Couldn't open input stream." << endl;
		av_freep(&avioCtx->buffer);
		avio_context_free(&avioCtx);
		avformat_close_input(&avFormatCtx);
		avFormatCtx = NULL;
		return -1;
	}

	// step3: 寻找流信息并显示。
	cout << "Call avformat_find_stream_info()" << endl;
	ret = avformat_find_stream_info(avFormatCtx, NULL);
	if(ret < 0)
	{
		cerr << "Couldn't find stream information." << endl;
		av_freep(&avioCtx->buffer);
		avio_context_free(&avioCtx);
		avformat_close_input(&avFormatCtx);
		avFormatCtx = NULL;
		return -1;
	}

	cout << "Call av_dump_format()" << endl;
	av_dump_format(avFormatCtx, 0, NULL, 0);

	// step4: 在流信息中寻找音、视频流的索引值。
	cout << "Find audio and video type." << endl;
	int i = 0;
	for(i = 0; i < avFormatCtx->nb_streams; ++i)
	{
		if(AVMEDIA_TYPE_VIDEO == avFormatCtx->streams[i]->codec->codec_type)
		{
			videoIndex = i;
		}
	}
	cout << "Find audio and video type end." << endl;

	if(-1 == videoIndex)
	{
		cerr << "Didn't find any audio or video stream." << endl;
		av_freep(&avioCtx->buffer);
		avio_context_free(&avioCtx);
		avformat_close_input(&avFormatCtx);
		avFormatCtx = NULL;
		return -3;
	}
	cout << "videoIndex = " << videoIndex << endl;

	// step5.1: 为音视频寻找合适的解码器，并打开解码器。
	avCodecCtx = avFormatCtx->streams[videoIndex]->codec;
	if(NULL == avCodecCtx)
	{
		cerr << "Video codec contex not found." << endl;
	}

	AVCodec *videoCodec = NULL;
	videoCodec = avcodec_find_decoder(avCodecCtx->codec_id);
	if(NULL == videoCodec)
	{
		cerr << "Video codec not found." << endl;
	}

	ret = avcodec_open2(avCodecCtx, videoCodec, NULL);
	if(ret < 0)
	{
		cerr << "Could not open video codec." << endl;
	}

	width = avFormatCtx->streams[videoIndex]->codec->width;
	height = avFormatCtx->streams[videoIndex]->codec->height;
	cout << "width = " << width << ", height = " << height << endl;

	return 0;
}
#endif

#if 0
void YUV420_to_RGB_FFMPEG(unsigned char* pYUV, unsigned char* pBGR24, int width, int height)
{
	//int srcNumBytes,dstNumBytes;
	//uint8_t *pSrc,*pDst;
	AVPicture pFrameYUV, pFrameBGR;
	avpicture_fill(&pFrameYUV, pYUV, AV_PIX_FMT_YUV420P, width, height);

	//U,V互换
	uint8_t * ptmp = pFrameYUV.data[1];
	pFrameYUV.data[1] = pFrameYUV.data[2];
	pFrameYUV.data[2] = ptmp;

	//avpicture_fill(&pFrameBGR, pBGR24, AV_PIX_FMT_BGR24, width, height);
	avpicture_fill(&pFrameBGR, pBGR24, AV_PIX_FMT_RGB24, width, height);

	struct SwsContext* imgCtx = NULL;
	//imgCtx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_BGR24, SWS_BILINEAR, 0, 0, 0);
	imgCtx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, 0, 0, 0);

	if(imgCtx != NULL)
	{
		sws_scale(imgCtx, pFrameYUV.data, pFrameYUV.linesize, 0, height, pFrameBGR.data, pFrameBGR.linesize);
		if(imgCtx)
		{
			sws_freeContext(imgCtx);
			imgCtx = NULL;
		}
	}
	else
	{
		sws_freeContext(imgCtx);
		imgCtx = NULL;
	}
}
#endif

#if 0	// 公式法将YUV 转化为RGB, 效率不如移位法（FFMPEG 自带的）。
void YUV420_to_RGB(unsigned char* pYUV, unsigned char* pRGB, int width, int height)
{
	unsigned char* point_Y = pYUV; //找到Y、U、V在内存中的首地址
	unsigned char* point_U = pYUV + height * width;
	unsigned char* point_V = point_U + (height * width / 4);
	unsigned char* pBGR = NULL;
	unsigned char R = 0;
	unsigned char G = 0;
	unsigned char B = 0;
	unsigned char Y = 0;
	unsigned char U = 0;
	unsigned char V = 0;

	double temp = 0;
	int i = 0;
	for(i = 0; i < height; ++i)
	{
		for(int j = 0; j < width; ++j)
		{
			pBGR = pRGB + i * width * 3 + j * 3;	// 找到相应的RGB首地址
			Y = *(point_Y + i * width + j);		// 取Y、U、V的数据值
			U = *point_U;
			V = *point_V;
			//yuv转rgb公式
			temp = Y + ((1.773) * (U - 128));
			if(temp < 0)
				B = 0;
			else if(temp > 255)
				B = 255;
			else
				B = (unsigned char)temp;
			temp = (Y - (0.344) * (U - 128) - (0.714) * (V - 128));
			if(temp < 0)
				G = 0;
			else if(temp > 255)
				G = 255;
			else
				G = (unsigned char)temp;
			temp = (Y + (1.403) * (V - 128));
			if(temp < 0)
				R = 0;
			else if(temp > 255)
				R = 255;
			else
				R = (unsigned char)temp;
			*pBGR = B; //将转化后的rgb保存在rgb内存中，b存放在最低位
			*(pBGR + 1) = G;
			*(pBGR + 2) = R;
			if(j % 2 != 0)
			{
				++point_U;
				++point_V;
			}
		}
		if(0 == i % 2)
		{
			point_U = point_U - width / 2;
			point_V = point_V - width / 2;
		}
	}
}
#endif