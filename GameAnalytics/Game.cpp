#include "stdafx.h"
#include "Game.h"

Game::Game()
{
}

Game::Game(ExtractCards ex, ClassifyCard cc, ScreenCaptureService sc)
	: extractCards(ex), classifyCard(cc), screenCaptureService(sc)
{
}

Game::Game(ScreenCaptureService sc) 
	: extractCards(), classifyCard(), screenCaptureService(sc)
{
}

Game::~Game()
{
}

void Game::incrementHints()
{
	metrics.numberOfHints++;
}

void Game::incrementScore(int value)
{
	metrics.score += value;
}

void Game::incrementUndos()
{
	metrics.numberOfUndos++;
}

void Game::incrementSuitErrors()
{
	metrics.numberOfSuitErrors++;
}

void Game::incrementRankErrors()
{
	metrics.numberOfRankErrors++;
}

void Game::incrementPilePresses()
{
	metrics.numberOfPilePresses++;
}

void Game::setGameWon()
{
	metrics.gameWon = true;
}

void Game::addClickCoordOnCard(POINT location)
{
	metrics.locationOfPresses.push_back(location);
}

void Game::incrementPressesAtLoc(int locationIndex)
{
	if (locationIndex >= 0 && locationIndex != 99) ++metrics.numberOfPresses.at(locationIndex);
}

bool Game::checkAutoComplete()
{
	auto it = find_if(currentPlayingBoard.begin(), currentPlayingBoard.end(), [](CardLocation cardLocation) 
	{
		return cardLocation.remainingCards != 0;
	});
	if (it == currentPlayingBoard.end()) return true;
	return false;
}

bool Game::checkEndGame()
{
	for (int i = 0; i < currentPlayingBoard.size() - 4; i++)
	{
		if (currentPlayingBoard.at(i).knownCards.size() > 0) return false;
	}
	return true;
}

void Game::recalculateErrors()
{
	metrics.numberOfRankErrors = 0;
	metrics.numberOfSuitErrors = 0;
	metrics.numberOfTooManyCardsMovedErrors = 0;
	for each (Move move in metrics.listOfMoves)
	{
		metrics.numberOfRankErrors += move.rankErr;
		metrics.numberOfSuitErrors += move.suitErr;
		metrics.numberOfTooManyCardsMovedErrors += move.tooManyCardsMovementErr;
	}
}

void Game::printEndOfGame()
{
	recalculateErrors();

	std::cout << "--------------------------------------------------------" << std::endl;
	(metrics.gameWon) ? std::cout << "Game won!" << std::endl : std::cout << "Game over!" << std::endl;
	std::cout << "--------------------------------------------------------" << std::endl;
	std::cout << "Game solved: " << std::boolalpha << metrics.gameWon << std::endl;
	std::cout << "Total time: " << getGameDuration() << " s" << std::endl;
	std::cout << "Points scored: " << metrics.score << std::endl;
	std::cout << "Average time per move = " << getAverageDuration("move") << "ms" << std::endl;
	std::cout << "Number of moves = " << metrics.listOfMoves.size() << " moves" << std::endl;
	std::cout << "Hints requested = " << metrics.numberOfHints << std::endl;
	std::cout << "Times undo = " << metrics.numberOfUndos << std::endl;
	std::cout << "Number of rank errors = " << metrics.numberOfRankErrors << std::endl;
	std::cout << "Number of suit errors = " << metrics.numberOfSuitErrors << std::endl;
	std::cout << "--------------------------------------------------------" << std::endl;
	for (int i = 0; i < 7; i++)
	{
		std::cout << "Number of build stack " << i << " presses = " << metrics.numberOfPresses.at(i) << std::endl;
	}
	std::cout << "Number of pile presses = " << metrics.numberOfPilePresses << std::endl;
	std::cout << "Number of talon presses = " << metrics.numberOfPresses.at(7) << std::endl;
	for (int i = 8; i < 12; i++)
	{
		std::cout << "Number of suit stack " << i << " presses = " << metrics.numberOfPresses.at(i) << std::endl;
	}
	std::cout << "--------------------------------------------------------" << std::endl;

	//add to metrics
	metrics.avgThinkDuration = getAverageDuration("think");
	metrics.avgMoveDuration = getAverageDuration("move");
	metrics.duration = getGameDuration();
}

bool Game::isFirstMovePlayed()
{
	return previousPlayingBoards.size() > 1;
}

int Game::timeDifferenceBetween(Timepoint t1, Timepoint t2)
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t2).count();
}

GameMetrics Game::getGameMetrics()
{
	return metrics;
}

void Game::showPressLocations()
{
	cv::Mat pressLocations = cv::Mat(176 * 2, 133 * 2, CV_8UC3, cv::Scalar(255, 255, 255));	// output an image with location of presses on a topcard
	for (int i = 0; i < metrics.locationOfPresses.size(); i++)
	{
		cv::Point point = cv::Point(metrics.locationOfPresses.at(i).x * 2, metrics.locationOfPresses.at(i).y * 2);
		cv::circle(pressLocations, point, 2, cv::Scalar(0, 0, 255), 2);
	}

	cv::namedWindow("clicklocations", cv::WINDOW_NORMAL);
	resizeWindow("clicklocations", cv::Size(131 * 2, 174 * 2));
	imshow("clicklocations", pressLocations);
	cv::waitKey(0);
}

int Game::getGameDuration()
{
	std::chrono::time_point<std::chrono::steady_clock> endOfGame = Clock::now();
	return std::chrono::duration_cast<std::chrono::seconds>(endOfGame - metrics.startOfGame).count();
}

int Game::getAverageDuration(std::string cat)
{
	int sum = 0;
	for each (Move move in metrics.listOfMoves)
	{
		if (cat.compare("move") == 0) sum += move.moveDuration;
		else sum += move.thinkDuration;
	}
	if (metrics.listOfMoves.empty()) return 0;
	return  sum / metrics.listOfMoves.size();
}

bool Game::allKingsOnSuitStack()
{
	for (int i = 1; i <= 4; i++)
	{
		if (currentPlayingBoard.at(currentPlayingBoard.size() - i).getLastCard().getRank() != KING) return false;
	}
	return true;
}

void Game::calculateFinalScore() 
{
	int remainingCards = 0;
	for (int i = 0; i < 7; ++i)
	{
		remainingCards += (int)currentPlayingBoard.at(i).knownCards.size();
	}
	incrementScore(remainingCards * 10);
}

void Game::startTimers() 
{
	metrics.startOfGame = Clock::now();
}

void Game::moveCard(int sourceIndex, int destinationIndex, Card movedCard)
{
	currentPlayingBoard.at(sourceIndex).knownCards.pop_back();
	currentPlayingBoard.at(destinationIndex).knownCards.push_back(movedCard);
}

void Game::removeFalseMoves()
{
	int endLoop = false;
	if (metrics.listOfMoves.size() <= 1) return;
	while (!endLoop)
	{
		for (int i = metrics.listOfMoves.size() - 1; i >= 0; i--)
		{
			if (i < metrics.listOfMoves.size() - 1)
			{
				if (metrics.listOfMoves.at(i).succesfull)  endLoop = true;;
				if (metrics.listOfMoves.at(i).endOfMove.time_since_epoch() == metrics.listOfMoves.at(i + 1).startOfMove.time_since_epoch())
				{
					metrics.listOfMoves.erase(metrics.listOfMoves.begin() + i);
					if (i == 0) endLoop = true;
					break;
				}
			}
		}
	}
	//recalculate think time
	for (int i = 0; i < metrics.listOfMoves.size(); i++)
	{
		if (metrics.listOfMoves.at(i).thinkDuration == 0)
		{
			if (i == 0)
			{
				metrics.listOfMoves.at(i).thinkDuration = timeDifferenceBetween(metrics.listOfMoves.at(i).startOfMove, metrics.startOfGame);
			}
			else 
			{
				metrics.listOfMoves.at(i).thinkDuration = timeDifferenceBetween(metrics.listOfMoves.at(i).startOfMove, metrics.listOfMoves.at(i - 1).endOfMove);
			}
			
		}
	}
}

std::pair<int, int> Game::findCardLocationOnBoard(std::vector<CardLocation> board, Rank rank, Suit suit)
{
	for (int index = 0; index < board.size(); index++)
	{
		for (int cardIndex = 0; cardIndex < board.at(index).knownCards.size(); cardIndex++)
		{
			Rank cardRank = board.at(index).knownCards.at(cardIndex).getRank();
			Suit cardSuit = board.at(index).knownCards.at(cardIndex).getSuit();
			if (cardRank == rank && cardSuit == suit)
			{
				std::cout << "found card " << static_cast<char>(cardRank) << static_cast<char>(cardSuit);
				std::cout << " at (" << index << "," << cardIndex << ")" << std::endl;
				return std::pair<int, int>(index, cardIndex);
			}
		}
	}
}


