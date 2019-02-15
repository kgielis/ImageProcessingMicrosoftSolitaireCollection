#include "stdafx.h"
#include "ClicksHooks.h"
#include "GameAnalytics.h"

//extern CONDITION_VARIABLE mouseclick;
extern GameAnalytics ga;
std::chrono::time_point<std::chrono::steady_clock> clickUpTimer;
std::chrono::time_point<std::chrono::steady_clock> clickDownTimer;
bool clickDown = false;
bool doubleClick = false;
int clickDown_x = -1;
int clickDown_y = -1;
long long timer = 0; //See if it is a double click
long long timer2 = 0; // Used for Duration of a move

//sources: https://www.unknowncheats.me/wiki/C%2B%2B:WindowsHookEx_Mouse

ClicksHooks::ClicksHooks()
{
}

ClicksHooks::~ClicksHooks()
{
}

int ClicksHooks::Messages() {
	 while (msg.message != WM_QUIT && !ga.getEndOfGameBool())
	 {
		 // while we do not close our application
		 if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}	
		Sleep(1);
	} 
	UninstallHook(); // if we close, let's uninstall our hook
	return (int) msg.wParam; // return the messages
}

void ClicksHooks::InstallHook()
{
	if (!(hook = SetWindowsHookEx(WH_MOUSE_LL, MyMouseCallback, NULL, 0)))
	{
		std::cout << "Error:" << GetLastError() << std::endl;
	}
}

void ClicksHooks::UninstallHook()
{
	UnhookWindowsHookEx(hook);	// Uninstall our hook using the hook handle
}

LRESULT WINAPI MyMouseCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	HWND handle = ga.getScreenCapterService().getHwnd();
	if (nCode == 0) {

		MSLLHOOKSTRUCT * pMouseStruct = (MSLLHOOKSTRUCT *)lParam;

		switch (wParam) {
		case WM_LBUTTONUP:
			if (GetForegroundWindow() == handle)
			{
				bool drag = false;
				if (abs(pMouseStruct->pt.x - clickDown_x) > 15 || abs(pMouseStruct->pt.y - clickDown_y) > 15) //card has been draged
				{
					drag = true;
					ga.addTimeStampToBuffer(clickDownTimer); //time between click up and click down --> think time
					ga.addCoordinatesToBuffer(clickDown_x, clickDown_y, false, true);
				}
				ga.addTimeStampToBuffer(Clock::now());
				ga.addCoordinatesToBuffer(pMouseStruct->pt.x, pMouseStruct->pt.y, doubleClick, drag);
				std::cout << "Click at position (" << pMouseStruct->pt.x << "," << pMouseStruct->pt.y << ")" << std::endl;
				clickUpTimer = Clock::now();
			}
			break;
		case WM_LBUTTONDOWN:
			if (GetForegroundWindow() == handle)
			{
				doubleClick = false;
				timer = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - clickDownTimer).count();
				if (clickDown_x == pMouseStruct->pt.x && clickDown_x == pMouseStruct->pt.y && timer < 300)
				{
					std::cout << "double click" << std::endl;
					doubleClick = true;
				}
				else
				{
					//new action pressed, click down registerde
					ga.toggleClickDownBool();
				}
				clickDown_x = pMouseStruct->pt.x;
				clickDown_y = pMouseStruct->pt.y;
				clickDownTimer = Clock::now();
			}
			break;

		case WM_RBUTTONUP:
			break;

		case WM_RBUTTONDOWN:
			break;
		}
	}

	/*
	Every time that the nCode is less than 0 we need to CallNextHookEx:
	-> Pass to the next hook
	MSDN: Calling CallNextHookEx is optional, but it is highly recommended;
	otherwise, other applications that have installed hooks will not receive hook notifications and may behave incorrectly as a result.
	*/
	return CallNextHookEx(ClicksHooks::Instance().hook, nCode, wParam, lParam);
}