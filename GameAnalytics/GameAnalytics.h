#pragma once
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "shcore.lib")

#include "stdafx.h"
#include "shcore.h"
#include <mutex>
#include <condition_variable>
#include "ClicksHooks.h"
#include "Globals.h"
#include "KlondikeGame.h"
#include "FreecellGame.h"
#include "ScreenCaptureService.h"
#include "ExtractButtonsAndLabels.h"
#ifdef NDEBUG
#include "databaseService.h"
#endif

class GameAnalytics
{
public:
	GameAnalytics();
	~GameAnalytics();

	// GETTERS AND SETTERS
	#ifdef NDEBUG
	void setDatabaseService(DatabaseService *db);
	#endif
	void setPlayerId(int playerId);
	ScreenCaptureService getScreenCapterService();
	void setScreenCaptureService(ScreenCaptureService sc);
	bool getEndOfGameBool();
	void setWindow(std::pair<int, int> window);
	void setExtractButtonsAndLabels(ExtractButtonsAndLabels ex);
	void startNewGame(SolitaireGame game);

	// MAIN FUNCTIONS
	void process();	// main function of the application
	void finishGame(bool gameWon, bool endOfGame);
	void determineNextState(const POINT pt, const POINT scaled, cv::Mat src, bool isDoubleClick);	
	void handleUndoState(cv::Mat src);	// take the previous state of the playing board if undo was pressed
	void handleMainMenuState(cv::Mat src);
	void handlePlayingState(cv::Mat src, POINT pt, Timepoint timestamp, bool drag);	// extract cards, classify them and check for changes in the playing board

	// MULTITHREADED FUNCTIONS + SCREEN CAPTURE
	void hookMouseClicks();	// multithreaded function for capturing mouse clicks
	void grabSrc();	// multithreaded function that only captures screens after mouse clicks
	void addCoordinatesToBuffer(const int x, const int y, const bool isDoubleClick, const bool drag);
	void toggleClickDownBool();	// on click before previous screen is captured, notify for immediate screengrab
	void addTimeStampToBuffer(Timepoint timestamp);

private:
	SolitaireState currentState = MAINMENU;
	ScreenCaptureService screenCaptureService;
	Game *currentGame = nullptr;
	SolitaireGame currentGameType;
	ExtractButtonsAndLabels extractButtons;
	#ifdef NDEBUG
		DatabaseService *dBService;
	#endif
	Window window;

	// GAME METRICS
	int playerID = 0;
	bool endOfGameBool = false;

	// SCREENSHOT BUFFERS AND DATA
	std::queue<cv::Mat> srcBuffer;
	std::queue<POINT> posBuffer1;
	std::queue<POINT> posBuffer2;
	std::queue<bool> doubleClickBuffer;
	std::queue<bool> dragBuffer;
	std::queue<Timepoint> timestampBuffer;

	bool waitForStableImageBool = false;
	std::vector<cv::Rect> extractedButtons; // these are the buttonlocations extracted the decide next state

};

void changeConsoleFontSize(const double & percentageIncrease);
