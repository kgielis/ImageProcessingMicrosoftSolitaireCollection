#include "stdafx.h"
#include "ScreenCaptureService.h"

ScreenCaptureService::ScreenCaptureService()
{
}

ScreenCaptureService::~ScreenCaptureService()
{
}

std::pair<int, int> ScreenCaptureService::initScreenCapture()
{

	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);	// making sure the screensize isn't altered automatically by Windows

	//hwnd = FindWindow(NULL, L"Microsoft Solitaire Collection - Mozilla Firefox");	// find the handle of a window using the FULL name
	hwnd = FindWindow(L"ApplicationFrameWindow", L"Microsoft Solitaire Collection");

	if (hwnd == NULL)
	{
		std::cout << "start Microsoft Solitaire" << std::endl;
		startMicrosoftSolitaire();
		hwnd = FindWindow(L"ApplicationFrameWindow", L"Microsoft Solitaire Collection");
	}

	setToForeground();

	HMONITOR appMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);	// check on which monitor Solitaire is being played
	MONITORINFO appMonitorInfo;
	appMonitorInfo.cbSize = sizeof(appMonitorInfo);
	GetMonitorInfo(appMonitor, &appMonitorInfo);	// get the location of that monitor in respect to the primary window
	RECT appRect = appMonitorInfo.rcMonitor;	// getting the data of the monitor on which Solitaire is being played

	double windowWidth = abs(appRect.right - appRect.left);
	double windowHeight = abs(appRect.bottom - appRect.top);
	std::pair<int, int> windowsize(windowWidth, windowHeight);
	return windowsize;
}

POINT ScreenCaptureService::scaleCoordinates(POINT pt, Window window)
{
	double aspectRatio = window.width / window.height;
	POINT scaled;
	scaled.x = pt.x * standardBoardWidth / window.width;
	scaled.y = pt.y * standardBoardHeight / window.height;
	return scaled;
}

cv::Mat ScreenCaptureService::captureScreen()
{
	cv::Mat image = waitForStableImage();
	if (!image.empty() && checkForLoadingScreen(image)) 
	{
		std::cout << "loading screen captured, trying again" << std::endl;
		image = waitForStableImage();
	}
	return image;
}

void ScreenCaptureService::takeInstantScreenShot(cv::Mat image)
{
	if (image.empty()) 
	{
		std::cout << "image is empty" << std::endl;
		takeInstantScreenShot(hwnd2Mat());
	}
	clickDownBuffer.push(image);
	clickDownBool = true;
}

cv::Mat ScreenCaptureService::waitForStableImage()
{
	cv::Mat src1, src2, graySrc1, graySrc2;
	double norm = DBL_MAX;
	std::chrono::time_point<std::chrono::steady_clock> duration1 = Clock::now();

	src2 = hwnd2Mat();
	while (norm > 0.0)
	{
		// function takes longer than 4 seconds -> end of game animation
		if (std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - duration1).count() > 4)	
		{
			cv::Mat empty;
			return empty;
		}
		src1 = src2;
		cvtColor(src1, graySrc1, cv::COLOR_BGR2GRAY);
		Sleep(20);	// wait for a certain duration to check for a difference (animation)
					// -> too short? issue: first animation of cardmove, second animation of new card turning around
					//					the second animation takes a small duration to kick in, which will be missed if the Sleep duration is too short
					// -> too long? the process takes longer, which can give issues when a player plays fast (more exception cases using clickDownBuffer)
		src2 = hwnd2Mat();
		cvtColor(src2, graySrc2, cv::COLOR_BGR2GRAY);
		norm = cv::norm(graySrc1, graySrc2, cv::NORM_L1);	// calculates the manhattan distance (sum of absolute values) of two grayimages
		if (checkForGameWonScreen(src2))
		{
			std::cout << "found end game" << std::endl;
			return src2; //changing elements so not possible to take stable screenshot
		}
		if (clickDownBool)
		{
			std::cout << "Click registerd when waiting for stable image " << clickDownBuffer.size() << std::endl;
			clickDownBool = false;	// new click registered while waitForStableImage isn't done yet
									//  -> use the image at the moment of the new click (just before the new animation) for the previous move
			  
			src1 = clickDownBuffer.front(); clickDownBuffer.pop();
			return src1;
		}
	}
	if (src2.empty()) waitForStableImage();
	return src2;
}

cv::Mat ScreenCaptureService::hwnd2Mat()
{

	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	cv::Mat src;
	BITMAPINFOHEADER  bi;

	setToForeground();
	

	hwindowDC = GetDC(NULL);	// get the device context of the window handle 
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);	// get a handle to the memory of the device context
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);	// set the stretching mode of the bitmap so that when the image gets resized to a smaller size, the eliminated pixels get deleted w/o preserving information

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;	// get the screensize of the window
	srcwidth = windowsize.right;
	height = windowsize.bottom;
	width = windowsize.right;

	src.create(height, width, CV_8UC4);	// create an a color image (R,G,B and alpha for transparency)
										// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow
																									   // avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}

void ScreenCaptureService::setToForeground() {
	if (GetForegroundWindow() != hwnd)
	{
		SetForegroundWindow(hwnd);
	}
}

HWND ScreenCaptureService::getHwnd() {
	return hwnd;
}

bool ScreenCaptureService::checkForOutOfMovesState()
{
	// split the image in 3 horizontal parts and take the middle part and 4 vertical parts and take the 2 in the middle
	// if the middle part is mostly white (at least 84%), then the board image shows an outOfMoves image
	cv::Mat src = hwnd2Mat();
	cv::Mat gray, threshold;
	cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
	// threshold the image to keep only brighter regions (cards are white)	
	cv::threshold(gray, threshold, 240, 255, cv::THRESH_BINARY);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(threshold);
	if (contours.size() > 0)
	{
		cv::Rect boundRect = cv::boundingRect(contours.at(0));
		cv::Mat outofmovesScreen(src, boundRect);
		double ratio = (double)outofmovesScreen.cols / (double)outofmovesScreen.rows;
		int area = outofmovesScreen.cols * outofmovesScreen.rows;
		if (ratio > 2.5 && area > 100000)
		{
			return true;
		}
	}
	return false;
}

bool ScreenCaptureService::checkForGameOverScreen(cv::Mat src)
{
	int simularAreas = checkForEndGameScreen(src);
	if (simularAreas == 3) return true;
	return false;

}

bool ScreenCaptureService::checkForGameWonScreen(cv::Mat src)
{
	int simularAreas = checkForEndGameScreen(src);
	if (simularAreas == 2) return true;
	return false;
}

int ScreenCaptureService::checkForEndGameScreen(cv::Mat src)
{
	if (src.empty()) return false; //animation returns empty src

	cv::Mat hsv, mask, resizedEndGame;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
	//find all blue pixels in range
	cv::Scalar lo_int(100, 196, 135);
	cv::Scalar hi_int(110, 217, 200);
	inRange(hsv, lo_int, hi_int, mask);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(mask);

	if (contours.size() > 0)
	{
		if (src.empty()) return -1; //animation returns empty src

		cv::Mat hsv, mask, resizedEndGame;
		cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
		//find all blue pixels in range
		cv::Scalar lo_int(100, 196, 135);
		cv::Scalar hi_int(110, 217, 200);
		inRange(hsv, lo_int, hi_int, mask);
		std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(mask);

		if (contours.size() > 0)
		{
			cv::Rect boundRect = cv::boundingRect(contours.at(0));
			cv::Mat endGame(src, boundRect);
			double ratio = (double)endGame.cols / (double)endGame.rows;
			//look if buttons are visible and no end game animation sparkles are on top
			cv::Mat binaryScreen(mask, boundRect);
			cv::Mat invSrc = cv::Scalar::all(255) - binaryScreen;
			std::vector<std::vector<cv::Point>> smaller_contours = myOpencv::findContoursBySize(invSrc);
			int previousArea = 0, simularAreas = 1;
			for (int i = 0; i < smaller_contours.size(); i++)
			{
				cv::Rect rect = cv::boundingRect(smaller_contours.at(i));
				int area = rect.area();
				if (abs(area - previousArea) < 300 && area > 100 && rect.width < src.size().width / 3) simularAreas++; //navigation buttons
				previousArea = area;
			}
			if (smaller_contours.size() > 30)
			{
				//sparkles too many countours
				return -1;
			}
			if (ratio > 1.9 && ratio < 1.92)
			{
				return simularAreas;
			}
		}
		return -1;
	}
	return -1;
}

bool ScreenCaptureService::checkForLoadingScreen(cv::Mat src) 
{
	cv::Mat hsv, mask, resizedEndGame;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
	//find all blue pixels in range
	cv::Scalar lo_int(106, 215, 102);
	cv::Scalar hi_int(106, 215, 102);
	inRange(hsv, lo_int, hi_int, mask);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(mask);
	if (contours.size() > 0) 
	{
		cv::Rect boundRect = cv::boundingRect(contours.at(0));
		if (boundRect.height == src.size().height && boundRect.width == src.size().width)
		{
			return true;
		}
	}
	return false;
}

bool ScreenCaptureService::checkForPlayingScreen(cv::Mat src) 
{
	if (src.empty()) return false;
	cv::Mat hsv, mask;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
	//find all green pixels in range
	cv::Scalar lo_int(72, 150, 65);
	cv::Scalar hi_int(78, 255, 160);
	inRange(hsv, lo_int, hi_int, mask);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(mask);
	if (contours.size() > 0)
	{
		cv::Rect boundRect = cv::boundingRect(contours.at(0));
		if (boundRect.height == src.size().height && boundRect.width == src.size().width)
		{
			return true;
		}
	}
	return false;
}

bool ScreenCaptureService::checkForSelectDifficultyScreen(cv::Mat src)
{
	if (src.empty()) return false;

	cv::Mat hsv, mask, resizedEndGame;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
	//find all blue pixels in range
	cv::Scalar lo_int(106, 196, 74);
	cv::Scalar hi_int(108, 231, 200);
	inRange(hsv, lo_int, hi_int, mask);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(mask);

	if (contours.size() > 0)
	{
		cv::Rect boundRect = cv::boundingRect(contours.at(0));
		cv::Mat screen(src, boundRect);
		double ratio = (double)screen.cols / (double)screen.rows;
		if (ratio < 1) return true; // length > width execept for freecell
		else if (ratio > 1.1 && ratio < 1.2) return true; // for freecell
	}
	return false;
}

int ScreenCaptureService::startMicrosoftSolitaire()
{
	//source : https://gist.github.com/MattHarrington/4508572
	std::wstring myApp(L"Microsoft.MicrosoftSolitaireCollection_8wekyb3d8bbwe!App"); // Hard-code your AppUserModelId here.
	int argc = 2; 
	HRESULT hrResult = S_OK;
	if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		if (argc == 2)
		{
			DWORD dwProcessId = 0;
			hrResult = LaunchApp(myApp, &dwProcessId); // changed argument to myApp from *argv
		}
		else
		{
			hrResult = E_INVALIDARG;
		}

		CoUninitialize();
	}

	return hrResult;
}

HRESULT ScreenCaptureService::LaunchApp(const std::wstring& strAppUserModelId, PDWORD pdwProcessId)
{
	CComPtr<IApplicationActivationManager> spAppActivationManager;
	HRESULT hrResult = E_INVALIDARG;
	if (!strAppUserModelId.empty())
	{
		// Instantiate IApplicationActivationManager
		hrResult = CoCreateInstance(CLSID_ApplicationActivationManager,
			NULL,
			CLSCTX_LOCAL_SERVER,
			IID_IApplicationActivationManager,
			(LPVOID*)&spAppActivationManager);

		if (SUCCEEDED(hrResult))
		{
			// This call ensures that the app is launched as the foreground window
			hrResult = CoAllowSetForegroundWindow(spAppActivationManager, NULL);

			// Launch the app
			if (SUCCEEDED(hrResult))
			{
				hrResult = spAppActivationManager->ActivateApplication(strAppUserModelId.c_str(),
					NULL,
					AO_NONE,
					pdwProcessId);
			}
		}
	}

	return hrResult;
}
