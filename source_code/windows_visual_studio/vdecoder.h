#pragma once
/*
影响解析时间的几个参数：
fps_analyze_framecount:
	通常可以看到count值大于等于fps_analyze_framecount时，才不在解析码流中的数据。
	如果想降低解析的延迟，可以减小fps_analyze_framecount的初始值。
	但是有一个风险，以rtmp流为例，有可能连续出现多次audio后再出现video包，就有可能解析不到video的编解码格式以及相应的参数。

probesize:
	当读取的数据大于等于probesize值时，才停止解析码流中的数据。

eof_reached:
	读文件出错，或者文件结束。
*/

// ffmpeg include and lib
#if 1
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
#include "libswscale/swscale.h"
};

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "swscale.lib")
#endif

class VDecoder {
public:
	VDecoder();
	~VDecoder();

	/* initWithArg() 和 initWithFile() 只需要二选一调用。在创建对象后执行。*/
	int deInit();
	int initWithArg(AVCodecID avCodecID);			// 对应的获取解码数据的方法为sendData() + recvData().
	int initWithFile(const char *filePath);			// 对应的获取解码数据的方法为recvDataFromFile();

	void stop();
	void begin();
	bool isRunning() const;

	int sendData(const unsigned char *pData, unsigned int size);	// 向解码器中添加已编码的数据。
	int recvData(unsigned char *pData, unsigned int size);			// 从解码器中获取解码后的数据。
	int recvDataFromFile(unsigned char *pData, unsigned int size);	// 转换图像色域空间。
	int colorConvert(AVPixelFormat avDstFmt, unsigned char* pDstData, AVPixelFormat avSrcFmt, unsigned char* pSrcData, unsigned int width, unsigned int height);
	unsigned int saveVideoPacket(const char *outFileNamePrefix);	// 将解析出的视频帧数据，一帧一帧保存到本地文件。仅对initWithFile() 初始化的数据有效。

	unsigned int widths() const;		// 图像宽。
	unsigned int heights() const;		// 图像高。
	double yuvPixelBytes() const;		// 单个像素YUV 占的空间大小。YUV420 为1.5Bytes.

	int __receive(unsigned char *pData, unsigned int size);			// 用户静止调用

private:
	bool bInited = false;
	bool bRunning = false;
	bool bWHInited = false;

	int videoIndex = -1;
	unsigned int width = 0;
	unsigned int height = 0;
	double yuvPixelByte = 1.5;

	AVFrame *avFrame = NULL;
	AVCodec *avCodec = NULL;
	AVPacket *avPacket = NULL;
	AVCodecContext *avCodecCtx = NULL;
	AVFormatContext	*avFormatCtx = NULL;
};
