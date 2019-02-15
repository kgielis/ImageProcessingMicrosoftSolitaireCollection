#pragma once
#include "Game.h"

class KlondikeGame : public Game
{
public:
	KlondikeGame(ExtractCards ex, ClassifyCard cc, ScreenCaptureService sc);
	KlondikeGame(ScreenCaptureService sc);
	~KlondikeGame();

	void initPlayingBoard();
	void printPlayingBoardState();

	//update board
	std::vector<Card> findCards(cv::Mat image, POINT pt);
	Click findClick(int cardLocation, const POINT &pt);
	bool updateBoard(const std::vector<Card>& newTopCards, POINT pt, Timepoint timestamp, bool drag);
	void processDoubleClickMoves(int sourceIndex, std::vector<Card> &extractedTopCards);
	bool checkTurnAroundCard(const int location, const std::vector<Card> & newTopCards);
	void resolveUnkownCards(std::vector<Card> newTopCards);

	//detect which cards have been moved
	int determineIndexOfPressedCard(const int & x, const int & y);
	std::vector<int> findChangedCardLocations(const std::vector<Card>& classifiedCardsFromPlayingBoard);
	std::vector<int> findChangedSuitLocations(std::vector<int> &changedIndex);

	//move cards
	bool moveAllPossibleCards(int sourceIndex, int destIndex);

	//metrics
	void addSuccesMoveMetrics();
	void checkErrors(Click firstClick, Click secondClick);


	// control check
	bool compareTopCards(const std::vector<Card> newTopCards);

private:
	std::vector<Card> classifiedTalonCards;
};

