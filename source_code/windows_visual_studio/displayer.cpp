#include <iostream>
#include "displayer.h"

using namespace std;
using namespace cv;		// opencv

Displayer::Displayer(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	cout << "Displayer::Displayer()." << endl;

	namedWindow(windowName, WINDOW_KEEPRATIO | WINDOW_NORMAL);
	moveWindow(windowName, x, y);
	resizeWindow(windowName, width, height);

	cout << "Displayer::Displayer() end." << endl;
}

Displayer::~Displayer()
{
	cout << "Displayer::~Displayer()." << endl;
	destroyWindow(windowName);
	cout << "Displayer::~Displayer() end." << endl;
}

int Displayer::showRGBImg(unsigned int width, unsigned int height, unsigned char *pRGBBuf)
{
	//cout << "Displayer::showRGBImg()." << endl;
	Mat rgbMat(height, width, CV_8UC3, pRGBBuf);
	imshow(windowName, rgbMat);
	waitKey(1);
	rgbMat.release();

	//cout << "Displayer::showRGBImg() end." << endl;
	return 0;
}

