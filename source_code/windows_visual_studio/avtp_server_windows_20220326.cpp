// avtp_server_windows_20220326.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <fstream>
#include <iostream>
#include "vdecoder.h"
#include "avtp_server.h"
#include "displayer.h"

void YUV420_to_RGB(unsigned char* pYUV, unsigned char* pRGB, int width, int height);

using namespace std;

int main()
{
	cout << "Please enter address << " << endl;
	char ipAddr[128] = { 0 };
	cin >> ipAddr;

	cout << "Please enter address << " << endl;
	unsigned int port = 0;
	cin >> port;

	cout << ipAddr << endl;
	cout << port << endl;

	/* 1.init video decoder. */
	VDecoder vdecoder;
#if 1
	/* 2.avtp connection. */
	AvtpVideoServer avtpServer("192.168.0.5", "192.168.0.100", port);
	vdecoder.initWithArg(AV_CODEC_ID_H265);
#else
	vdecoder.initWithFile("C:\\Users\\zgxxg\\Documents\\Visual Studio 2015\\Projects\\ffmpeg_opcv\\samples\\test1.265");
#endif
	//int ret = vdecoder.saveVideoPacket("C:\\Users\\zgxxg\\Documents\\Visual Studio 2015\\Projects\\ffmpeg_opcv\\samples\\h265-03\\h265-");
	/* 3.init opencv windows */
	const unsigned int screenW = 1920;
	const unsigned int screenH = 1080;
	//Displayer displayer(screenW * 1.2, 0 + 200, screenW / 3, screenH / 3);
	Displayer displayer(screenW * 0.7, 0 + 200, screenW / 3, screenH / 3);

	/* 4. read file and output h26x frames. */
#if 0
	static unsigned int suCnt = 0;
	while(30 * 1 != suCnt++)
#else
	while(true)
#endif
	{
		/* 4.1从音视频协议中获取一帧数据。 */
		int realSize = 0;
		const unsigned int bufSize = 128 * 1024;
		unsigned char buf[bufSize] = { 0 };

#if 1
		realSize = avtpServer.recvVideoFrame(buf, bufSize);
		if(-1 == realSize)
		{
			cerr << "Fail to call avtpServer.recvVideoFrame() in main()." << endl;
			continue;
		}
		//cout << "realSize = " << realSize << endl;

		/* 4.2 将一帧H.26X 数据送入解码器。*/
		realSize = vdecoder.sendData(buf, realSize);
		if(-1 == realSize)
		{
			cerr << "Fail to call vdecoder.sendData() in main()." << endl;
			continue;
		}
#endif

		/* 4.3 获取解码后的YUV 数据。*/
		const unsigned int width = vdecoder.widths();
		const unsigned int height = vdecoder.heights();
		const unsigned int YUV420BufLen = vdecoder.widths() * vdecoder.heights() * vdecoder.yuvPixelBytes();
		//cout << "width, height, yuvBufLen = " << width << ", " << height << ", " << YUV420BufLen << endl;

		unsigned char *pYUV420Buf = NULL;
		pYUV420Buf = (unsigned char *)malloc(YUV420BufLen);
		if(NULL == pYUV420Buf)
		{
			cerr << "Fail to call malloc(3) in main()." << endl;
			continue;
		}
		memset(pYUV420Buf, 0, YUV420BufLen);

#if 1
		realSize = vdecoder.recvData(pYUV420Buf, YUV420BufLen);
		if(-1 == realSize)
		{
			cerr << "Fail to call vdecoder.recvData() in main()." << endl;
			free(pYUV420Buf);
			pYUV420Buf = NULL;
			continue;
		}
#else
		realSize = vdecoder.recvDataFromFile(pYUV420Buf, YUV420BufLen);
		if(-1 == realSize)
		{
			cerr << "Fail to call vdecoder.recvDataFromFile() in main()." << endl;
			free(pYUV420Buf);
			pYUV420Buf = NULL;
			break;
		}
#endif

		/* 4.4 对图像进行色彩域的转换，YUV 转RGB. */
		const unsigned int RGBBufLen = width * height * 3;		// RGB 需要3 个字节存放一个Pixel.
		unsigned char *pRGBBuf = NULL;
		pRGBBuf = (unsigned char *)malloc(RGBBufLen);
		if(NULL == pRGBBuf)
		{
			cerr << "Fail to call malloc(3) in main()." << endl;
			free(pYUV420Buf);
			pYUV420Buf = NULL;
			continue;
		}
		memset(pRGBBuf, 0, RGBBufLen);
		//cout << "width, height, RGBBufLen = " << width << ", " << height << ", " << RGBBufLen << endl;

		realSize = vdecoder.colorConvert(AV_PIX_FMT_RGB24, pRGBBuf, AV_PIX_FMT_YUV420P, pYUV420Buf, width, height);
		if(-1 == realSize)
		{
			cerr << "Fail to call vdecoder.colorConvert() in main()." << endl;
			continue;
		}

		///* 4.4 openCV 显示图像。 */
		displayer.showRGBImg(width, height, pRGBBuf);

		free(pYUV420Buf);
		pYUV420Buf = NULL;
		free(pRGBBuf);
		pRGBBuf = NULL;
	}

	system("pause");
	avtpServer.~AvtpVideoServer();

	malloc(7);
	_CrtDumpMemoryLeaks();
	return 0;
}


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
