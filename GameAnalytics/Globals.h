#pragma once
#include <vector>
#include <utility>
#include <Windows.h>
#include <stdlib.h>
#include <string>
#include <cstdio>
#include <chrono>
#include <numeric>
#include <thread>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <ctime>
#include <iostream>
#include <limits>
#include <cstddef>
#include <sstream>
#include <iomanip>
#include <map>
#include "CardLocation.h"
#include "myOpencvFunctions.h"

#define standardBoardWidth 1600
#define standardBoardHeight 900
#define standardCardWidth 150
#define standardCardHeight 200
typedef std::chrono::time_point<std::chrono::steady_clock> Timepoint;
typedef std::chrono::high_resolution_clock Clock;

enum SolitaireState { PLAYING, UNDO, QUIT, NEWGAME, MENU, MAINMENU, OUTOFMOVES, WON, HINT,
	AUTOCOMPLETE, SELECTDIFICULTY, ENDGAME, GAMEOVER, NONE };	// possible game states

enum SolitaireGame { KLONDIKE, FREECELL };

const std::map<int, Rank> valueRankMap{
{0, Rank::EMPTY_RANK},
{1, Rank::ACE},
{2, Rank::TWO},
{3, Rank::THREE},
{4, Rank::FOUR},
{5, Rank::FIVE},
{6, Rank::SIX},
{7, Rank::SEVEN},
{8, Rank::EIGHT},
{9, Rank::NINE},
{10, Rank::TEN},
{11, Rank::JACK},
{12, Rank::QUEEN},
{13, Rank::KING}};

struct Move {
	long long thinkDuration;
	long long moveDuration;
	bool succesfull;
	bool rankErr = false;
	bool suitErr = false;
	bool tooManyCardsMovementErr = false;
	bool unmovableErr = false;
	Timepoint startOfMove;
	Timepoint endOfMove;
	std::time_t timeOfMove = std::time(0);
	int numberOfCardsMoved;
	Card movedCard;
	Card destCard;
	int sourceLocation;
	int destLocation;
};

struct GameMetrics {
	bool gameWon = false;
	int numberOfUndos = 0;
	int numberOfHints = 0;
	int numberOfSuitErrors = 0;
	int numberOfRankErrors = 0;
	int numberOfPilePresses = 0;
	int numberOfTooManyCardsMovedErrors = 0;
	int score = 0;
	int duration;
	int avgThinkDuration;
	int avgMoveDuration;
	std::vector<Move> listOfMoves;
	std::vector<int> numberOfPresses;
	std::vector<POINT> locationOfPresses;
	Timepoint startOfGame;
	const std::time_t startingTime = std::time(0);
	std::string gameDifficulty = "";
	std::string gameSeed = "";
	std::string gameType = "";
};

struct Window {
	int height;
	int width;
};