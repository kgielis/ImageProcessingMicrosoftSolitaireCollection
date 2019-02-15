#include "stdafx.h"
#include "GameAnalytics.h"

CRITICAL_SECTION threadLock;
GameAnalytics ga;

int main(int argc, char** argv)
{
	changeConsoleFontSize(0.5);
	InitializeCriticalSection(&threadLock);
	
	#ifdef NDEBUG 
		DatabaseService * dBService = new DatabaseService();
		dBService->initDBConn();
		ga.setPlayerId(dBService->initLogin());
		ga.setDatabaseService(dBService);
	#endif // NDEBUG 

	ScreenCaptureService screenCaptureService;
	std::pair<int, int> window = screenCaptureService.initScreenCapture();
	ga.setWindow(window);
	ga.setScreenCaptureService(screenCaptureService);
	ExtractButtonsAndLabels extractButtonsAndLabels;
	ga.setExtractButtonsAndLabels(extractButtonsAndLabels);

	// initializing thread to capture mouseclicks and a thread dedicated to capturing the screen of the game 
	std::thread clickThread(&GameAnalytics::hookMouseClicks, &ga);
	std::thread srcGrabber(&GameAnalytics::grabSrc, &ga);

	// main processing function of the main thread
	cv::Mat src = screenCaptureService.captureScreen();
	ga.handleMainMenuState(src);
	ga.process();

	// terminate threads
	srcGrabber.join();
	clickThread.join();
	return EXIT_SUCCESS;
}

GameAnalytics::GameAnalytics()
{
}

GameAnalytics::~GameAnalytics()
{
	#ifdef NDEBUG 
	delete dBService;
	#endif
	delete currentGame;
}

#ifdef NDEBUG
void GameAnalytics::setDatabaseService(DatabaseService *db)
{
	dBService = db;
}
#endif

void GameAnalytics::setPlayerId(int playerId)
{
	playerID = playerId;
}

void GameAnalytics::setScreenCaptureService(ScreenCaptureService sc) 
{
	screenCaptureService = sc;
}

ScreenCaptureService GameAnalytics::getScreenCapterService()
{
	return screenCaptureService;
}

bool GameAnalytics::getEndOfGameBool()
{
	return endOfGameBool;
}

void GameAnalytics::setWindow(std::pair<int, int> win) {
	window.height = win.second;
	window.width = win.first;
}

void GameAnalytics::setExtractButtonsAndLabels(ExtractButtonsAndLabels ex)
{
	extractButtons = ex;
}

void GameAnalytics::startNewGame(SolitaireGame game) 
{
	delete currentGame;
	switch (game)
	{
	case KLONDIKE:
	{
		currentGame = new KlondikeGame(screenCaptureService);
		currentGame->initPlayingBoard();
		currentGameType = KLONDIKE;
		std::cout << "NEW KLONDIKE GAME CREATED" << std::endl;
		break;
	}
	case FREECELL:
		currentGame = new FreecellGame(screenCaptureService);
		currentGame->initPlayingBoard();
		currentGameType = FREECELL;
		std::cout << "NEW FREECELL GAME CREATED" << std::endl;
		break;
	default:
		break;
	}
}

/****************************************************
 *	STATE MACHINE								
 ****************************************************/

void GameAnalytics::process()
{
	bool exit = false;
	while (!exit)
	{
		if (!srcBuffer.empty())	// click registered
		{
			EnterCriticalSection(&threadLock);	// get the coordinates of the click and image of the board
			cv::Mat src = srcBuffer.front(); srcBuffer.pop();
			POINT pt = posBuffer2.front(); posBuffer2.pop();
			bool isDoubleClick = doubleClickBuffer.front(); doubleClickBuffer.pop();
			bool drag = dragBuffer.front(); dragBuffer.pop();
			Timepoint timestamp = timestampBuffer.front(); timestampBuffer.pop();
			LeaveCriticalSection(&threadLock);
			// recalculate the coordinates to the correct window and rescale to the standardBoard
			POINT scaled = screenCaptureService.scaleCoordinates(pt, window);
			//std::cout << "scaled click position (" << scaled.x << "," << scaled.y << ")" << std::endl;

			determineNextState(pt, scaled, src, isDoubleClick);
			switch (currentState)
			{
			case PLAYING:
				handlePlayingState(src, pt, timestamp, drag);
				break;
			case UNDO:
				if (drag) {currentState = PLAYING; break;}
				handleUndoState(src);
				break;
			case HINT:
				currentGame->incrementHints();
				currentState = PLAYING;
				break;
			case QUIT:
				endOfGameBool = true;
				break;
			case WON:
				{
					if (screenCaptureService.checkForGameWonScreen(src))
					{
						extractedButtons = extractButtons.extractEndGameButtons(src);
						finishGame(true, true);
						currentState = ENDGAME;
					}
				}
				break;
			case AUTOCOMPLETE:
				{
					cv::Mat screenCapture = screenCaptureService.captureScreen();
					if (screenCaptureService.checkForGameWonScreen(screenCapture))
					{
						finishGame(true, true);
						extractedButtons = extractButtons.extractEndGameButtons(src);
					}
				}
				break;
			case NEWGAME:
				break;
			case MENU:
				break;
			case MAINMENU:
				handleMainMenuState(src);
				break;
			case OUTOFMOVES:
				break;
			case SELECTDIFICULTY:
				break;
			case ENDGAME:
				break;
			case GAMEOVER:
				break;
			default:
				std::cerr << "Error: currentState is not defined!" << std::endl;
				break;
			}
		}
		else if (currentState == WON) //waiting for endgame screen
		{
			cv::Mat screenCapture = screenCaptureService.captureScreen();
			std::cout << "check if endGame screen is visible" << std::endl;
			if (screenCaptureService.checkForGameWonScreen(screenCapture))
			{
				extractedButtons = extractButtons.extractEndGameButtons(screenCapture);
				finishGame(true, true);
				currentState = ENDGAME;
			}
		}
		else if (GetAsyncKeyState(VK_ESCAPE))
		{
			finishGame(false, false);
			endOfGameBool = true;
			exit = true;
		}
		else
		{
			Sleep(10);
		}
	}
}

void GameAnalytics::finishGame(bool gameWon, bool endOfGame)
{
	if (currentGame != nullptr)
	{
		currentGame->calculateFinalScore();
		if (gameWon) currentGame->setGameWon();
		currentGame->printEndOfGame();
		#ifdef NDEBUG
		if (playerID != -1 && currentGame->isFirstMovePlayed()) 
		{
			std::cout << "enter game in database..." << std::endl;
			dBService->insertMetricsDB(playerID, currentGame->getGameMetrics(), endOfGame);
		}
		currentGame = nullptr;
		#endif
		//currentGame->showPressLocations();
	}
}

void GameAnalytics::determineNextState(const POINT pt, const POINT scaled, cv::Mat src, bool isDoubleClick)	
{	
	switch (currentState)
	{
	case PLAYING:
		if ((20 <= pt.x && pt.x <= 150) && (window.height - 95 <= pt.y && pt.y <= window.height - 25))
		{
			if (currentGame->isFirstMovePlayed()) //check if one move was made
			{
				std::cout << "NEWGAME PRESSED!" << std::endl;
				finishGame(false, false);
				currentState = NEWGAME;
			}
			else 
			{
				std::cout << "NEWGAME PRESSED!" << std::endl;
				std::cout << "SELECT DIFICULTY!" << std::endl;
				
				cv::Mat screenCapture = screenCaptureService.captureScreen();
				while (!screenCaptureService.checkForSelectDifficultyScreen(screenCapture))
				{
					screenCapture = screenCaptureService.captureScreen();
				}
				extractedButtons = extractButtons.extractDifficultyButtons(screenCapture);
				finishGame(false, false);
				currentState = SELECTDIFICULTY;
			}	
		}
		else if ((window.width - 150 <= pt.x && pt.x <= window.width - 25) && (window.height - 95 <= pt.y && pt.y <= window.height - 25))
		{
			if (currentGame->previousPlayingBoards.size() > 1) //check if there is a move you can undo
			{
				std::cout << "UNDO PRESSED!" << std::endl;
				currentState = UNDO;
			}
		}
		else if ((1 <= pt.x && pt.x <= 70) && (1 <= pt.y && pt.y <= 70))
		{
			std::cout << "MENU PRESSED!" << std::endl;
			currentState = MENU;
		}
		else if ((85 <= pt.x && pt.x <= 155) && (1 <= pt.y && pt.y <= 70))
		{
			std::cout << "BACK PRESSED!" << std::endl;
			currentState = MAINMENU;
		}
		else if (currentGameType == KLONDIKE && currentGame->checkAutoComplete() && !isDoubleClick)
		{
			if ((530 <= pt.x && pt.x <= 655) && (145 <= pt.y && pt.y <= 210))
			{
				std::cout << "AUTOCOMPLETE PRESSED" << std::endl;
				currentState = WON;
			}
		}
		else if (screenCaptureService.checkForGameOverScreen(src))
		{
			std::cout << "GAME OVER, OUT OF MOVES" << std::endl;
			extractedButtons = extractButtons.extractGameOverButtons(src);
			currentState = GAMEOVER;
		}
		else if (currentGame->checkEndGame()) 
		{
			std::cout << "all cards played" << std::endl;
			currentState = WON;
		}
		break;
	case MENU:
		if ((1 <= scaled.x && scaled.x <= 340) && (60 <= scaled.y && scaled.y <= 117))
		{
			std::cout << "HINT PRESSED!" << std::endl;
			currentState = HINT;
		}
		else if (!((1 <= scaled.x && scaled.x <= 340) && (60 <= scaled.y && scaled.y <= window.height)))
		{
			std::cout << "PLAYING!" << std::endl;
			currentState = PLAYING;
		}
		break;
	case NEWGAME:
		if ((733 <= scaled.x && scaled.x <= 951) && (535 <= scaled.y && scaled.y <= 586))
		{
			std::cout << "OK PRESSED!" << std::endl;
			std::cout << "SELECT DIFICULTY!" << std::endl;
			cv::Mat screenCapture = screenCaptureService.captureScreen();
			while (!screenCaptureService.checkForSelectDifficultyScreen(screenCapture))
			{
				screenCapture = screenCaptureService.captureScreen();
			}
			extractedButtons = extractButtons.extractDifficultyButtons(screenCapture);
			currentState = SELECTDIFICULTY;
		}
		else if ((968 <= scaled.x && scaled.x <= 1187) && (535 <= scaled.y && scaled.y <= 586))
		{
			std::cout << "CANCEL PRESSED!" << std::endl;
			currentState = QUIT;
		}
		break;
	case OUTOFMOVES:
		{
			cv::Rect ok_button = extractedButtons.at(0);
			cv::Rect undo_button = extractedButtons.at(1);
			if (ok_button.x <= pt.x && pt.x <= ok_button.x + ok_button.width
				&& ok_button.y <= pt.y && pt.y <= ok_button.y + ok_button.height)
			{
				std::cout << "CONTINUE PRESSED!" << std::endl;
				std::cout << "GAMEOVER" << std::endl;
				extractedButtons = extractButtons.extractGameOverButtons(src);
				currentState = GAMEOVER;
			}
			if (undo_button.x <= pt.x && pt.x <= undo_button.x + undo_button.width
				&& undo_button.y <= pt.y && pt.y <= undo_button.y + undo_button.height)
			{
				std::cout << "UNDO PRESSED!" << std::endl;
				currentState = UNDO;
			}
		}
		break;
	case MAINMENU:
		break;
	case WON:
		break;
	case SELECTDIFICULTY:
		{
			cv::Rect button = extractedButtons.at(0);
			if (button.x <= pt.x && pt.x <= button.x + button.width
				&& button.y <= pt.y && pt.y <= button.y + button.height)
			{
				std::cout << "PLAY PRESSED!" << std::endl;
				std::cout << "START NEW GAME" << std::endl;
				startNewGame(currentGameType);
				currentState = PLAYING;
			}
		}
		break;

	case GAMEOVER:
		{
			finishGame(false, true);
			cv::Rect newGameButton = extractedButtons.at(0);
			cv::Rect menuButton = extractedButtons.at(1);
			cv::Rect againButton = extractedButtons.at(2);
			if (newGameButton.x <= pt.x && pt.x <= newGameButton.x + newGameButton.width
				&& newGameButton.y <= pt.y && pt.y <= newGameButton.y + newGameButton.height)
			{
				std::cout << "NEW GAME PRESSED!" << std::endl;
				cv::Mat screenCapture = screenCaptureService.captureScreen();
				while (!screenCaptureService.checkForSelectDifficultyScreen(screenCapture))
				{
					screenCapture = screenCaptureService.captureScreen();
				}
				extractedButtons = extractButtons.extractDifficultyButtons(screenCapture);
				currentState = SELECTDIFICULTY;
			}
			if (menuButton.x <= pt.x && pt.x <= menuButton.x + menuButton.width
				&& menuButton.y <= pt.y && pt.y <= menuButton.y + menuButton.height)
			{
				std::cout << "MENU PRESSED!" << std::endl;
				currentState = MAINMENU;
			}
			if (againButton.x <= pt.x && pt.x <= againButton.x + againButton.width
				&& againButton.y <= pt.y && pt.y <= againButton.y + againButton.height)
			{
				std::cout << "AGAIN PRESSED!" << std::endl;
				startNewGame(currentGameType);
				currentState = PLAYING;
			}
		}
		break;
	case ENDGAME:
		{
			cv::Rect newGameButton = extractedButtons.at(0);
			cv::Rect menuButton = extractedButtons.at(1);
			if (newGameButton.x <= pt.x && pt.x <= newGameButton.x + newGameButton.width
				&& newGameButton.y <= pt.y && pt.y <= newGameButton.y + newGameButton.height)
			{
				std::cout << "NEW GAME PRESSED!" << std::endl;
				cv::Mat screenCapture = screenCaptureService.captureScreen();
				while (!screenCaptureService.checkForSelectDifficultyScreen(screenCapture))
				{
					screenCapture = screenCaptureService.captureScreen();
				}
				extractedButtons = extractButtons.extractDifficultyButtons(screenCapture);
				currentState = SELECTDIFICULTY;
			}
			if (menuButton.x <= pt.x && pt.x <= menuButton.x + menuButton.width
				&& menuButton.y <= pt.y && pt.y <= menuButton.y + menuButton.height)
			{
				std::cout << "MENU PRESSED!" << std::endl;
				currentState = MAINMENU;
			}
		}
		break;
	default:
		std::cerr << "Error: currentState is not defined!" << std::endl;
		break;
	}
}

void GameAnalytics::handlePlayingState(cv::Mat src, POINT pt, Timepoint timestamp, bool drag)
{
	std::vector<Card> classifiedTopCards;
	if (screenCaptureService.checkForPlayingScreen(src))
	{
		classifiedTopCards = currentGame->findCards(src, pt);
	}
	else if (screenCaptureService.checkForGameOverScreen(src))
	{
		std::cout << "GAME OVER, OUT OF MOVES" << std::endl;
		extractedButtons = extractButtons.extractGameOverButtons(src);
		currentState = GAMEOVER;
		return;
	}
	else if (screenCaptureService.checkForGameWonScreen(src))
	{
		classifiedTopCards.resize(currentGame->currentPlayingBoard.size());
		// endscreen so all kings are on suit deck
		for (int i = 0; i < classifiedTopCards.size(); i++)
		{
			//suit card
			if (i >= classifiedTopCards.size() - 4)
			{
				Suit suit = currentGame->currentPlayingBoard.at(i).getLastCard().getSuit();
				classifiedTopCards.at(i) = Card(KING, suit);
			}
			else 
			{
				classifiedTopCards.at(i) = Card(EMPTY_RANK, EMPTY_SUIT);
			}
		}
		extractedButtons = extractButtons.extractEndGameButtons(src);
	}
	else return; 
	if (currentGame->updateBoard(classifiedTopCards, pt, timestamp, drag))
	{
		if (currentGame->allKingsOnSuitStack())
		{
			currentState = WON;
			//wait until endgame shows endgame screen
			cv::Mat screenCapture = screenCaptureService.captureScreen();
			while (!screenCaptureService.checkForGameWonScreen(screenCapture))
			{
				Sleep(60);
				screenCapture = screenCaptureService.captureScreen();
			}
			extractedButtons = extractButtons.extractEndGameButtons(screenCapture);
		}
	}
}

void GameAnalytics::handleUndoState(cv::Mat src)
{
	currentGame->previousPlayingBoards.pop_back();	// take the previous board state as the current board state
	currentGame->currentPlayingBoard = currentGame->previousPlayingBoards.back();
	currentGame->printPlayingBoardState();
	currentGame->incrementUndos();
	currentState = PLAYING;
}

void GameAnalytics::handleMainMenuState(cv::Mat src) 
{
	if (screenCaptureService.checkForPlayingScreen(src))
	{
		std::string gameType = extractButtons.getTopBarData(src).front();
		if (gameType == "Freecell") startNewGame(FREECELL);
		else if (gameType == "Klondike") startNewGame(KLONDIKE);
		currentState = PLAYING;
	}
}

/****************************************************
 *	MULTITHREADED CLICK FUNCTIONS + SCREEN CAPTURE								
 ****************************************************/

void GameAnalytics::hookMouseClicks()
{
	// installing the click hook
	ClicksHooks::Instance().InstallHook();

	// handeling the incoming mouse data
	ClicksHooks::Instance().Messages();
}

void GameAnalytics::toggleClickDownBool()
{
	if (waitForStableImageBool)	// if the main process is waiting for a stable image (no animations), but a new click comes in
	{
		//std::cout << "take instant screenshot" << std::endl;	// take the image right on the new click as the image of the previous move
		cv::Mat screenshot = screenCaptureService.hwnd2Mat();

		std::thread takeScreenShotThread(&ScreenCaptureService::takeInstantScreenShot, screenCaptureService, screenshot);
		takeScreenShotThread.detach();
	}
}

void GameAnalytics::grabSrc()
{
	SetThreadPriority(this, THREAD_PRIORITY_TIME_CRITICAL);
	while (!endOfGameBool)	// while game not over
	{
		while(!endOfGameBool && posBuffer1.empty()) std::this_thread::yield();
	
		//Click registered
		waitForStableImageBool = true;
		cv::Mat img = screenCaptureService.captureScreen();
		waitForStableImageBool = false;

		if (!img.empty())	// empty image if the animation takes longer than 3 seconds (= game won animation)
		{
			EnterCriticalSection(&threadLock);
			if (!endOfGameBool) 
			{
				posBuffer2.push(posBuffer1.front()); posBuffer1.pop();
				srcBuffer.push(img.clone());
			}
			LeaveCriticalSection(&threadLock);
		}
	}
}

void GameAnalytics::addCoordinatesToBuffer(const int x, const int y, const bool isDoubleClick, const bool drag) {
	EnterCriticalSection(&threadLock);	// function called by the clickHooksThread, pushes the coordinates of a click to the first buffer
	POINT position = {x, y};
	posBuffer1.push(position);
	doubleClickBuffer.push(isDoubleClick);
	dragBuffer.push(drag);
	LeaveCriticalSection(&threadLock);
}

void GameAnalytics::addTimeStampToBuffer(Timepoint timestamp) {
	EnterCriticalSection(&threadLock);	// function called by the clickHooksThread
	timestampBuffer.push(timestamp);
	LeaveCriticalSection(&threadLock);
}

/****************************************************
 *	OTHER FUNCTIONS
 ****************************************************/

void changeConsoleFontSize(const double & percentageIncrease)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX font = { sizeof(CONSOLE_FONT_INFOEX) };

	if (!GetCurrentConsoleFontEx(hConsole, 0, &font))
	{
		exit(EXIT_FAILURE);
	}

	COORD size = font.dwFontSize;
	size.X += (SHORT)(size.X * percentageIncrease);
	size.Y += (SHORT)(size.Y * percentageIncrease);
	font.dwFontSize = size;

	if (!SetCurrentConsoleFontEx(hConsole, 0, &font))
	{
		exit(EXIT_FAILURE);
	}
}

