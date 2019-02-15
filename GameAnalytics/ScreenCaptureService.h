#pragma once

#include "Globals.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"

//launch application
#include <shobjidl.h>
#include <shlobj.h>
#include <objbase.h>
#include <atlbase.h>

class ScreenCaptureService
{
public:
	ScreenCaptureService();
	~ScreenCaptureService();
	
	std::pair<int, int> initScreenCapture();
	POINT scaleCoordinates(POINT pt, Window window);
	cv::Mat captureScreen();
	void takeInstantScreenShot(cv::Mat image);
	cv::Mat hwnd2Mat();
	void setToForeground();
	HWND getHwnd();
	bool checkForOutOfMovesState();
	bool checkForGameOverScreen(cv::Mat src);
	bool checkForGameWonScreen(cv::Mat src);
	bool checkForLoadingScreen(cv::Mat src);
	bool checkForPlayingScreen(cv::Mat src);
	bool checkForSelectDifficultyScreen(cv::Mat src);
	
private:
	cv::Mat waitForStableImage();
	int checkForEndGameScreen(cv::Mat src);

	int startMicrosoftSolitaire();
	HRESULT LaunchApp(const std::wstring& strAppUserModelId, PDWORD pdwProcessId);

	HWND hwnd;
	bool clickDownBool = false;
	std::queue<cv::Mat> clickDownBuffer;

};

