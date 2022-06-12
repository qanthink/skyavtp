#pragma once

/* OpenCV includes and libraries */
#include "opencv2/opencv.hpp"

#if 1
#pragma comment(lib, "opencv_world3415.lib")		// release
#else
#pragma comment(lib, "opencv_world3415d.lib")		// debug
#endif

class Displayer {
public:
	Displayer(unsigned int x = 0, unsigned int y = 0, unsigned int width = 0, unsigned int height = 0);
	~Displayer();

	int showRGBImg(unsigned int width, unsigned int height, unsigned char *pRGBBuf);

private:
	cv::Mat rgbMat;
	const char *windowName = "opencv window";

};
