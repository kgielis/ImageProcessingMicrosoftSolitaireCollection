#pragma once

#include "Globals.h"
#include "ExtractCards.h"
#include "ScreenCaptureService.h"
#include "ExtractButtonsAndLabels.h"
#include "Click.h"

class Game
{
public:
	Game();
	Game(ExtractCards ex, ClassifyCard cc, ScreenCaptureService sc);
	Game(ScreenCaptureService sc);
	~Game();

	virtual void initPlayingBoard() = 0;
	virtual void printPlayingBoardState() = 0;
	virtual std::vector<Card> findCards(cv::Mat image, POINT pt) = 0;
	virtual bool updateBoard(const std::vector<Card>& newTopCards, POINT pt, Timepoint timestamp, bool drag) = 0;
	virtual bool compareTopCards(const std::vector<Card> newTopCards) = 0;

	bool allKingsOnSuitStack();
	void calculateFinalScore();
	void startTimers();
	std::pair<int, int> findCardLocationOnBoard(std::vector<CardLocation> board, Rank rank, Suit suit);
	
	void moveCard(const int sourceIndex, const int destinationIndex, const Card movedCard);
	void removeFalseMoves();

	bool checkAutoComplete();
	bool checkEndGame();
	void recalculateErrors();
	void printEndOfGame();
	void showPressLocations();
	bool isFirstMovePlayed();
	int timeDifferenceBetween(Timepoint t1, Timepoint t2);
	
	//setters GameMetrics
	void incrementHints();
	void incrementScore(int value);
	void incrementUndos();
	void incrementSuitErrors();
	void incrementRankErrors();
	void incrementPilePresses();
	void setGameWon();
	void addClickCoordOnCard(POINT location);
	void incrementPressesAtLoc(int locationIndex);
	GameMetrics getGameMetrics();


	ExtractCards extractCards;
	ClassifyCard classifyCard;
	std::vector<CardLocation> currentPlayingBoard;
	std::vector<std::vector<CardLocation>> previousPlayingBoards;

protected:
	int getGameDuration();
	int getAverageDuration(std::string cat);
	ScreenCaptureService screenCaptureService;

	bool firstMove = false;
	GameMetrics metrics;
	std::vector<cv::Rect> cardRegions;

	//to update the board
	std::queue<std::vector<Card>> previousExtractedTopCards;
	std::vector<Click> previousBoardClicks;
	Click currentClick;
	POINT locationOfLastPress;
};