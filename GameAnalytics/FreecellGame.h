#pragma once
#include "game.h"

const std::map<Suit, int> suitToValueMap{
{Suit::HEARTS, 12},
{Suit::CLUBS, 13},
{Suit::DIAMONDS, 14},
{Suit::SPADES, 15}};

struct AutoplayCard {
	Card card;
	int locationIndex;
	int cardIndex;
};


class FreecellGame : public Game
{
public:
	FreecellGame(ScreenCaptureService sc);
	~FreecellGame();
	
	void initPlayingBoard();
	void printPlayingBoardState();

	//update board
	std::vector<Card> findCards(cv::Mat image, POINT pt);
	Click findClick(const POINT &pt);
	bool updateBoard(const std::vector<Card> & newTopCards, POINT pt, Timepoint timestamp, bool drag);
	void processDoubleClickMoves(int sourceLocation, std::vector<Card> &extractedTopCards);

	// detect which cards have been moved
	std::vector<int> findChangedCardLocations(const std::vector<Card> &newTopCards);
	std::vector<AutoplayCard> checkAutoPlay(const std::vector<Card> &newTopCards); 
	int determineIndexOfPressedCard(const int & x, const int & y);

	//move card
	void moveCardToSuit(int suitLocation, int locationIndex, int cardIndex);
	bool processAutoplayCards(const std::vector<Card>& newTopCards);
	bool moveAllPossibleCards(int sourceIndex, int destIndex, std::vector<Card> newTopCards, bool doubleClick);
	std::vector<Card> moveAutoPlayCardToStorageIfNeeded(int sourceIndex, std::vector<Card> newTopCards);

	//add move metrics
	void addSuccesMoveMetrics();
	void checkErrors(Click prevClick, Click currentClick);
	int getMaxMovableCards(Click & firstClick, Click & secondClick);
	Move checkIfCardWasMovable(Click &firstClick, Move move);

	// control check
	bool compareTopCards(const std::vector<Card> newTopCards);
	bool isCardMoveAllowed(Card moveCard, Card destTopCard);
	bool checkIfAutoPlayIsPossible(Card autoplayCard);
};
