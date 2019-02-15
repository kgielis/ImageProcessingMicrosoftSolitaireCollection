#include "stdafx.h"
#include "KlondikeGame.h"

KlondikeGame::KlondikeGame(ExtractCards ex, ClassifyCard cc, ScreenCaptureService sc)
		: Game(ex, cc, sc)
	{
	std::vector<int> emptyVector(12, 0);
	metrics.numberOfPresses = emptyVector;
}

KlondikeGame::KlondikeGame(ScreenCaptureService sc) 
	: Game(sc)
{
	std::vector<int> emptyVector(12, 0);
	metrics.numberOfPresses = emptyVector;
}

KlondikeGame::~KlondikeGame()
{
}

void KlondikeGame::initPlayingBoard()
{
	//capture screen
	cv::Mat screencapture = screenCaptureService.captureScreen();
	while (!screenCaptureService.checkForPlayingScreen(screencapture))
	{
		screencapture = screenCaptureService.captureScreen();
	}

	//Fetch dificulty
	ExtractButtonsAndLabels extractLabels = ExtractButtonsAndLabels();
	std::vector<std::string> topBarData = extractLabels.getTopBarData(screencapture);
	metrics.gameDifficulty = topBarData.at(0);
	metrics.gameType = topBarData.at(1);

	//extract card region locations
	cardRegions = extractCards.extractCardRegionLocations(screencapture);

	//extract cards
	extractCards.determineROI(screencapture, 210);
	std::vector<cv::Mat> extractedImagesFromPlayingBoard = extractCards.findCardsFromBoardImage(screencapture, KLONDIKE);

	//classify cards
	auto classifiedCardsFromPlayingBoard = classifyCard.classifyExtractedCards(extractedImagesFromPlayingBoard);

	//init playingboard
	currentPlayingBoard.resize(12);

	// build stack
	for (int i = 0; i < 7; i++)	// add the classified cards as the only item of known cards and set the amount of unknown cards of each location
	{
		CardLocation newLocation(classifiedCardsFromPlayingBoard.at(i), i);
		currentPlayingBoard.at(i) = newLocation;
	}

	// talon
	CardLocation talon(classifiedCardsFromPlayingBoard.at(7), 24);
	currentPlayingBoard.at(7) = talon;
	
	// suit stack
	for (int i = 8; i < currentPlayingBoard.size(); i++)
	{
		CardLocation newSuitStackLocation(classifiedCardsFromPlayingBoard.at(i), 0);
		currentPlayingBoard.at(i) = newSuitStackLocation;
	}
	previousPlayingBoards.push_back(currentPlayingBoard);	// add the first game state to the list of all game states
	printPlayingBoardState();	// print the board
}

void KlondikeGame::printPlayingBoardState()
{
	std::cout << "Talon: ";	// print the current topcard from deck
	if (currentPlayingBoard.at(7).getLastCard().isEmptyCard())
	{
		std::cout << "// ";
	}
	else
	{
		for (int j = 0; j < currentPlayingBoard.at(7).knownCards.size(); j++)
		{
			std::cout << static_cast<char>(currentPlayingBoard.at(7).knownCards.at(j).rank);
			std::cout << static_cast<char>(currentPlayingBoard.at(7).knownCards.at(j).suit) << " ";
		}
	}
	std::cout << "	size: " << currentPlayingBoard.at(7).knownCards.size();
	std::cout << "	Remaining: " << currentPlayingBoard.at(7).remainingCards << std::endl;

	std::cout << "Suit stack: " << std::endl;		// print all cards from the suit stack
	for (int i = 8; i < currentPlayingBoard.size(); i++)
	{
		std::cout << "   Pos " << i - 8 << ": ";
		if (currentPlayingBoard.at(i).knownCards.empty())
		{
			std::cout << "// ";
		}
		for (int j = 0; j < currentPlayingBoard.at(i).knownCards.size(); j++)
		{
			std::cout << static_cast<char>(currentPlayingBoard.at(i).knownCards.at(j).rank);
			std::cout << static_cast<char>(currentPlayingBoard.at(i).knownCards.at(j).suit) << " ";
		}
		std::cout << std::endl;
	}

	std::cout << "Build stack: " << std::endl;		// print all cards from the build stack
	for (int i = 0; i < 7; i++)
	{
		std::cout << "   Pos " << i << ": ";
		if (currentPlayingBoard.at(i).knownCards.empty())
		{
			std::cout << "// ";
		}
		for (int j = 0; j < currentPlayingBoard.at(i).knownCards.size(); j++)
		{
			std::cout << static_cast<char>(currentPlayingBoard.at(i).knownCards.at(j).rank);
			std::cout << static_cast<char>(currentPlayingBoard.at(i).knownCards.at(j).suit) << " ";
		}
		std::cout << "	Hidden cards = " << currentPlayingBoard.at(i).remainingCards << std::endl;
	}

	std::cout << std::endl;
}

std::vector<Card> KlondikeGame::findCards(cv::Mat screenCapture, POINT pt)
{
	//extract cards
	std::vector<cv::Mat> extractedImagesFromPlayingBoard = extractCards.findCardsFromBoardImage(screenCapture, KLONDIKE);

	//classify cards
	std::vector<Card> classifiedCardsFromPlayingBoard = classifyCard.classifyExtractedCards(extractedImagesFromPlayingBoard);

	//extract talon
	std::vector<cv::Mat> extractedTalonCards = extractCards.extractTalonCards(screenCapture);
	classifiedTalonCards = classifyCard.classifyExtractedCards(extractedTalonCards);

	//find Selected card
	int cardLocation = determineIndexOfPressedCard(pt.x, pt.y);
	if (cardLocation != -1 && cardLocation != 99)
	{
		currentClick = findClick(cardLocation, pt);
		if (cardLocation > 6) addClickCoordOnCard(locationOfLastPress);
		else if (currentClick.getSelectedIndex() == 0 && !currentClick.getSelectedCard().isEmptyCard())
		{
			addClickCoordOnCard(locationOfLastPress);
		}
	}
	else if (cardLocation == 99) currentClick = Click(pt, cardLocation);

	return classifiedCardsFromPlayingBoard;
}

Click KlondikeGame::findClick(int cardLocation, const POINT &pt)
{
	Click click(pt, cardLocation);
	int selectedCardIndex = 0;
	if (cardLocation != 7)
	{
		std::pair<int, int> selected = extractCards.findSelectedCards(locationOfLastPress, cardLocation);
		selectedCardIndex = selected.first;
	}
	click.setSelectedIndex(selectedCardIndex);
	int size = currentPlayingBoard.at(cardLocation).knownCards.size();
	if (cardLocation < 7 && selectedCardIndex != -1 && size > selectedCardIndex + 1)
	{
		if (size == 0)
		{
			click.setSelectedCard(Card(EMPTY_RANK, EMPTY_SUIT));
			click.setNumberOfSelectedCards(0);
		}
		else
		{
			int index = size - selectedCardIndex - 1;
			click.setSelectedCard(currentPlayingBoard.at(cardLocation).knownCards.at(index));
			click.setNumberOfSelectedCards(selectedCardIndex + 1);
		}
	}
	else
	{
		click.setSelectedCard(currentPlayingBoard.at(cardLocation).getLastCard());
		click.setNumberOfSelectedCards(1);
	}
	return click;
}

bool KlondikeGame::updateBoard(const std::vector<Card> & newTopCards, POINT pt, Timepoint timestamp, bool drag)
{
	int sourceIndex = -1;

	//load board to process if necessary 
	std::vector<Card> extractedTopCards;
	if (previousExtractedTopCards.empty()) extractedTopCards = newTopCards;
	else { extractedTopCards = previousExtractedTopCards.front();  previousExtractedTopCards.pop(); }

	//find pressed location index
	//std::cout << "clicked at location index: " << currentClick.getLocationIndex() << std::endl;
	incrementPressesAtLoc(currentClick.getLocationIndex());
	currentClick.setTimestamp(timestamp);
	
	//find changedLocations
	std::vector<int> indexOfChangedLocation = findChangedCardLocations(extractedTopCards);

	//update board
	if (currentClick.getLocationIndex() == -1 && indexOfChangedLocation.size() == 0)
	{
		//no update of board
		std::cout << "NO UPDATE, no card selected" << std::endl;
		return false;
	}
	else if (currentClick.getLocationIndex() == 99)
	{
		//pile clicked
		std::cout << "Pile pressed" << std::endl;
		if (currentPlayingBoard.at(7).knownCards.size() == currentPlayingBoard.at(7).remainingCards)
		{
			currentPlayingBoard.at(7).knownCards.clear();
		}
		int addedCards = 0;
		for_each(classifiedTalonCards.rbegin(), classifiedTalonCards.rend(), [this, &addedCards](Card newTalonCard) 
		{
			std::vector<Card> currentTalonCards = currentPlayingBoard.at(7).knownCards;
			bool alreadyInTalon = false;
			for (int i = 0; i < currentTalonCards.size(); i++)
			{
				if (currentTalonCards.at(i) == newTalonCard) alreadyInTalon = true;
			}
			if (!alreadyInTalon) 
			{
				currentPlayingBoard.at(7).knownCards.push_back(newTalonCard);
				addedCards++;
			}
		});
		previousPlayingBoards.push_back(currentPlayingBoard);
		previousBoardClicks.clear();
	}
	else if (currentClick.getLocationIndex() != -1)
	{
		if (indexOfChangedLocation.size() > 0)
		{
			//the board has changed
			if (previousBoardClicks.empty())
			{
				previousBoardClicks.push_back(currentClick);
				std::cout << "NO UPDATE, no previous clicks;" << std::endl;
				return false;
			}
			else 
			{
				sourceIndex = previousBoardClicks.back().getLocationIndex();
				int destIndex = currentClick.getLocationIndex();
				//std::cout << "try move: " << sourceIndex << " -> " << destIndex << std::endl;
				if (sourceIndex != destIndex && currentPlayingBoard.at(destIndex).getLastCard().isUnknownCard())
				{
					//find how many cards moved by looking at the newTopCards
					auto it = find(currentPlayingBoard.at(sourceIndex).knownCards.begin(),
						currentPlayingBoard.at(sourceIndex).knownCards.end(), extractedTopCards.at(sourceIndex));
					if (it == currentPlayingBoard.at(sourceIndex).knownCards.end())
					{
						it = currentPlayingBoard.at(sourceIndex).knownCards.begin(); // all cards moved to destination 
					}
					else it++;
					//do move
					currentClick.setSelectedCard(currentPlayingBoard.at(destIndex).getLastCard());
					currentClick.setSelectedIndex(0);
					previousBoardClicks.back().setSelectedCard(Card(UNKNOWN_RANK, UNKNOWN_SUIT));
					previousBoardClicks.back().setNumberOfSelectedCards(std::distance(it, currentPlayingBoard.at(sourceIndex).knownCards.end()));
					currentPlayingBoard.at(destIndex).knownCards.insert(
						currentPlayingBoard.at(destIndex).knownCards.end(),
						it,
						currentPlayingBoard.at(sourceIndex).knownCards.end());
					currentPlayingBoard.at(sourceIndex).knownCards.erase(
						it,
						currentPlayingBoard.at(sourceIndex).knownCards.end());
					checkTurnAroundCard(sourceIndex, extractedTopCards);
					previousPlayingBoards.push_back(currentPlayingBoard);
					addSuccesMoveMetrics();
					previousBoardClicks.clear();
				}
				else if (sourceIndex != destIndex)
				{
					if (moveAllPossibleCards(sourceIndex, destIndex))
					{
						checkTurnAroundCard(sourceIndex, extractedTopCards);
						previousPlayingBoards.push_back(currentPlayingBoard);
						addSuccesMoveMetrics();
						previousBoardClicks.clear();
					}
					else
					{
						std::cout << "NO UPDATE" << std::endl;
						if (previousBoardClicks.size() > 0) checkErrors(previousBoardClicks.back(), currentClick);

						previousBoardClicks.push_back(currentClick);
					}
				} 
				else if (std::chrono::duration_cast<std::chrono::milliseconds>
					(currentClick.getTimestamp() - previousBoardClicks.back().getTimestamp()).count() <= 500)
				{
					processDoubleClickMoves(sourceIndex, extractedTopCards);

				}
				else 
				{
					if (previousBoardClicks.size() > 0) checkErrors(previousBoardClicks.back(), currentClick);
					previousBoardClicks.push_back(currentClick);
					std::cout << "NO UPDATE, no double click, it was not a move" << std::endl;
				}
			}
		}
		else 
		{
			if (previousBoardClicks.size() > 0) checkErrors(previousBoardClicks.back(), currentClick);
			previousBoardClicks.push_back(currentClick);
			std::cout << "NO UPDATE, no changedIndeces" << std::endl;
			return false;
		}
	}	
	if (!compareTopCards(extractedTopCards))
	{
		//save topCards For Later
		previousExtractedTopCards.push(extractedTopCards);
		std::cout << "board update not complete, save extractedTopCards" << std::endl;
	}
	else
	{
		resolveUnkownCards(extractedTopCards);
		std::queue<std::vector<Card>> empty;
		std::swap(previousExtractedTopCards, empty);
	}
	printPlayingBoardState();
	return true;
}

void KlondikeGame::processDoubleClickMoves(int sourceIndex, std::vector<Card> &extractedTopCards)
{
	// double click, card moved to the suit stack
	Card movedCard = currentPlayingBoard.at(sourceIndex).getLastCard();
	std::cout << static_cast<char>(movedCard.getRank()) << static_cast<char>(movedCard.getSuit()) << std::endl;
	bool updated = false;
	for (int i = 8; i < 12; i++)
	{
		if (currentPlayingBoard.at(i).getLastCard().isEmptyCard()
			&& movedCard.getRank() == ACE)
		{
			moveCard(sourceIndex, i, movedCard);
			checkTurnAroundCard(sourceIndex, extractedTopCards);
			previousPlayingBoards.push_back(currentPlayingBoard);
			previousBoardClicks.back().setSelectedCard(movedCard);
			previousBoardClicks.back().setSelectedIndex(0);
			currentClick.setSelectedCard(Card(EMPTY_RANK, EMPTY_SUIT));
			addSuccesMoveMetrics();
			previousBoardClicks.clear();
			updated = true;
			break;
		}
		else if ((currentPlayingBoard.at(i).getLastCard().getSuit() == movedCard.getSuit())
			&& (rankValueMap.at(currentPlayingBoard.at(i).getLastCard().getRank()) + 1
				== rankValueMap.at(movedCard.getRank())))
		{
			moveCard(sourceIndex, i, movedCard);
			checkTurnAroundCard(sourceIndex, extractedTopCards);
			previousPlayingBoards.push_back(currentPlayingBoard);
			previousBoardClicks.back().setSelectedCard(movedCard);
			previousBoardClicks.back().setSelectedIndex(0);
			currentClick.setSelectedCard(movedCard);
			addSuccesMoveMetrics();
			previousBoardClicks.clear();
			updated = true;
			break;
		}
		else if (movedCard.isUnknownCard())
		{
			std::vector<int> changedindexes = findChangedCardLocations(extractedTopCards);
			auto it = find(changedindexes.begin(), changedindexes.end(), i);
			if (it != changedindexes.end())
			{
				Rank rank = valueRankMap.at(currentPlayingBoard.at(i).knownCards.size() + 1);
				Suit suit = extractedTopCards.at(i).getSuit();
				moveCard(sourceIndex, i, Card(rank, suit));
				checkTurnAroundCard(sourceIndex, extractedTopCards);
				previousPlayingBoards.push_back(currentPlayingBoard);
				previousBoardClicks.back().setSelectedCard(movedCard);
				previousBoardClicks.back().setSelectedIndex(0);
				currentClick.setSelectedCard(Card(rank, suit));
				addSuccesMoveMetrics();
				previousBoardClicks.clear();
				updated = true;
				break;
			}
		}
	}
	if (!updated)
	{
		std::cout << "NO UPDATE" << std::endl;
		if (previousBoardClicks.size() > 0) checkErrors(previousBoardClicks.back(), currentClick);
		previousBoardClicks.push_back(currentClick);
	}
}

bool KlondikeGame::moveAllPossibleCards(int sourceIndex, int destIndex)
{
	// check if source is not empty --> NO MOVE
	CardLocation sourceLocation = currentPlayingBoard.at(sourceIndex);
	if (sourceLocation.knownCards.empty() && sourceLocation.remainingCards == 0) return false;

	int indexOfLastMovableCard = sourceLocation.knownCards.size() - 1;
	// get destination TOPCard;
	Card destTopCard = currentPlayingBoard.at(destIndex).getLastCard();
	if (currentPlayingBoard.at(destIndex).knownCards.size() == 0 && sourceIndex != 7)
	{
		//only king can be moved here
		if (sourceLocation.knownCards.front().getRank() == KING) indexOfLastMovableCard = 0;
		else if (sourceLocation.knownCards.front().getRank() == UNKNOWN_RANK) indexOfLastMovableCard = 0;
	}
	else if (sourceIndex == 7) 
	{
		Card moveCard = sourceLocation.getLastCard();
		if (moveCard.getRank() == KING && destTopCard.isEmptyCard())
		{
			currentPlayingBoard.at(7).knownCards.pop_back();
			currentPlayingBoard.at(destIndex).knownCards.push_back(moveCard);
			return true;
		}
		else if (rankValueMap.at(moveCard.getRank()) == rankValueMap.at(destTopCard.getRank()) - 1)
		{
			if (((destTopCard.getSuit() == 'H' || destTopCard.getSuit() == 'D')
				&& (moveCard.getSuit() == 'C' || moveCard.getSuit() == 'S')) 
				|| ((destTopCard.getSuit() == 'C' || destTopCard.getSuit() == 'S')
					&& (moveCard.getSuit() == 'H' || moveCard.getSuit() == 'D')))
			{
				currentPlayingBoard.at(7).knownCards.pop_back();
				currentPlayingBoard.at(destIndex).knownCards.push_back(moveCard);
				return true;
			}
			return false;
		}
		return false;

	}
	else 
	{
		//find next card until movable on destTopCard
		auto it = find_if(sourceLocation.knownCards.begin(), sourceLocation.knownCards.end(), [&destTopCard](Card card)
		{
			if (card.isUnknownCard()) return false;
			return rankValueMap.at(destTopCard.getRank()) == rankValueMap.at(card.getRank()) + 1;
		});
		// lastmovable card found
		if (it != sourceLocation.knownCards.end())
		{
			indexOfLastMovableCard = std::distance(sourceLocation.knownCards.begin(), it);
		}
		else // not found 
		{
			auto rit = find(sourceLocation.knownCards.rbegin(), sourceLocation.knownCards.rend(), Card(UNKNOWN_RANK, UNKNOWN_SUIT));
			if (rit != sourceLocation.knownCards.rend())
			{
				int index = sourceLocation.knownCards.size() - 1 - std::distance(sourceLocation.knownCards.rbegin(), rit);
				if (index != sourceLocation.knownCards.size() - 1)
				{
					Card cardAboveUnknown = sourceLocation.knownCards.at(index + 1);
					if (rankValueMap.at(cardAboveUnknown.getRank()) + 2 == rankValueMap.at(destTopCard.getRank()))
					{
						indexOfLastMovableCard = index;
					}
					else return false;
				}
			}
			else return false;
		}
	}

	Card lastMovableCard = currentPlayingBoard.at(sourceIndex).knownCards.at(indexOfLastMovableCard);
	/*std::cout << "lastMovableCard: " << static_cast<char>(lastMovableCard.getRank());
	std::cout << static_cast<char>(lastMovableCard.getSuit()) << std::endl;*/

	auto lastMovableCardIt = currentPlayingBoard.at(sourceIndex).knownCards.begin() + indexOfLastMovableCard;
	//do move
	currentClick.setSelectedCard(currentPlayingBoard.at(destIndex).getLastCard());
	currentClick.setSelectedIndex(0);
	previousBoardClicks.back().setSelectedCard(lastMovableCard);
	previousBoardClicks.back().setNumberOfSelectedCards(std::distance(lastMovableCardIt, currentPlayingBoard.at(sourceIndex).knownCards.end()));
	currentPlayingBoard.at(destIndex).knownCards.insert(
		currentPlayingBoard.at(destIndex).knownCards.end(),
		lastMovableCardIt,
		currentPlayingBoard.at(sourceIndex).knownCards.end());
	currentPlayingBoard.at(sourceIndex).knownCards.erase(
		lastMovableCardIt,
		currentPlayingBoard.at(sourceIndex).knownCards.end());
	previousPlayingBoards.push_back(currentPlayingBoard);
	return true;
}

void KlondikeGame::addSuccesMoveMetrics() {
	removeFalseMoves();
	Click firstClickOfMove = previousBoardClicks.back();
	Click secondClickOfMove = currentClick;
	Move move;
	Timepoint prevMoveTime;
	if (metrics.listOfMoves.empty()) prevMoveTime = metrics.startOfGame;
	else prevMoveTime = metrics.listOfMoves.back().endOfMove;

	move.thinkDuration = timeDifferenceBetween(firstClickOfMove.getTimestamp(), prevMoveTime);;
	move.startOfMove = firstClickOfMove.getTimestamp();
	move.endOfMove = secondClickOfMove.getTimestamp();
	move.moveDuration = timeDifferenceBetween(secondClickOfMove.getTimestamp(), firstClickOfMove.getTimestamp());
	move.succesfull = true;
	move.movedCard = firstClickOfMove.getSelectedCard();
	move.destCard = secondClickOfMove.getSelectedCard();
	move.sourceLocation = firstClickOfMove.getLocationIndex();
	move.destLocation = secondClickOfMove.getLocationIndex();
	move.numberOfCardsMoved = firstClickOfMove.getNumberOfSelectedCards();
	move.timeOfMove = std::time(0);
	metrics.listOfMoves.push_back(move);

	std::cout << "movedCard: " << static_cast<char>(move.movedCard.getRank());
	std::cout << static_cast<char>(move.movedCard.getSuit()) << " ";
	std::cout << "destCard: " << static_cast<char>(move.destCard.getRank());
	std::cout << static_cast<char>(move.destCard.getSuit()) << std::endl;
}

void KlondikeGame::checkErrors(Click firstClick, Click secondClick)
{
	//other metrics of move
	Move move;
	Timepoint prevMoveTime;
	if (metrics.listOfMoves.empty()) prevMoveTime = metrics.startOfGame;
	else prevMoveTime = metrics.listOfMoves.back().endOfMove;

	move.thinkDuration = timeDifferenceBetween(firstClick.getTimestamp(), prevMoveTime);
	move.moveDuration = timeDifferenceBetween(secondClick.getTimestamp(), firstClick.getTimestamp());
	move.startOfMove = firstClick.getTimestamp();
	move.endOfMove = secondClick.getTimestamp();
	move.succesfull = false;
	move.sourceLocation = firstClick.getLocationIndex();
	move.destLocation = secondClick.getLocationIndex();
	move.timeOfMove = std::time(0);

	//find selectedCards
	std::cout << "preClick: (" << firstClick.getLocationIndex() << "," << firstClick.getSelectedIndex() << ")" << std::endl;
	std::cout << "currentClick: (" << secondClick.getLocationIndex() << "," << secondClick.getSelectedIndex() << ")" << std::endl;
	int index = (int)currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.size() - firstClick.getSelectedIndex() - 1;
	Card sourceSelectedCard = currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.at(index);
	Card destSelectedCard = currentPlayingBoard.at(secondClick.getLocationIndex()).getLastCard();
	move.movedCard = sourceSelectedCard;
	move.destCard = destSelectedCard;
	move.numberOfCardsMoved = firstClick.getSelectedIndex() + 1;

	//check rank errors
	Suit prevSuit = sourceSelectedCard.getSuit();
	Suit newSuit = destSelectedCard.getSuit();
	if (((prevSuit == 'H' || prevSuit == 'D') && (newSuit == 'H' || newSuit == 'D'))	// check for color error
		|| ((prevSuit == 'S' || prevSuit == 'C') && (newSuit == 'S' || newSuit == 'C')))
	{
		std::cout << "Incompatible suit! " << static_cast<char>(prevSuit) << " can't go on ";
		std::cout << static_cast<char>(newSuit) << " to build the build stack" << std::endl;
		incrementSuitErrors();
		move.suitErr = true;
	}

	//check suit errors
	if (sourceSelectedCard.getRankValue() + 1 != destSelectedCard.getRankValue())
	{
		std::cout << "Incompatible rank! " << sourceSelectedCard.getRankValue() << " can't go on ";
		std::cout << destSelectedCard.getRankValue() << std::endl;
		incrementRankErrors();
		move.rankErr = true;
	}

	metrics.listOfMoves.push_back(move);
}

bool KlondikeGame::compareTopCards(const std::vector<Card> newTopCards)
{
	//if all topcards are empty you are in end game situation
	if (all_of(currentPlayingBoard.begin() + 8, currentPlayingBoard.end(), [](CardLocation cardlocation)
	{ return cardlocation.getLastCard().getRank() == KING; })) return true;
	for (int i = 0; i < newTopCards.size(); i++)
	{
		if (newTopCards.at(i).isEmptyCard())
		{
			if (i < 7)
			{
				if (currentPlayingBoard.at(i).knownCards.size() != 0
					&& !currentPlayingBoard.at(i).getLastCard().isUnknownCard()) return false;
			}
			else {
				if (!currentPlayingBoard.at(i).knownCards.back().isEmptyCard()
					&& !currentPlayingBoard.at(i).getLastCard().isUnknownCard()) return false;
			}
		}
		else if (currentPlayingBoard.at(i).knownCards.size() > 0)
		{
			if (newTopCards.at(i) != currentPlayingBoard.at(i).knownCards.back()
				&& !currentPlayingBoard.at(i).getLastCard().isUnknownCard()) return false;
		}
	}
	return true;
}

bool KlondikeGame::checkTurnAroundCard(int location, const std::vector<Card> & newTopCards)
{
	if (location == 7) 
	{
		currentPlayingBoard.at(location).remainingCards--;
	}
	else if (currentPlayingBoard.at(location).knownCards.size() == 0 && 
		currentPlayingBoard.at(location).remainingCards != 0)
	{
		//turn around card
		currentPlayingBoard.at(location).remainingCards--;
		currentPlayingBoard.at(location).knownCards.push_back(Card(UNKNOWN_RANK, UNKNOWN_SUIT));
		std::cout << "turn around card" << std::endl;
		return true;
	}
	return false;
}

void KlondikeGame::resolveUnkownCards(std::vector<Card> newTopCards)
{
	for (int i = 0; i < 7; i++)
	{
		if (currentPlayingBoard.at(i).getLastCard().isUnknownCard())
		{
			currentPlayingBoard.at(i).knownCards.pop_back();
			currentPlayingBoard.at(i).knownCards.push_back(newTopCards.at(i));
		}
	}
}

std::vector<int> KlondikeGame::findChangedSuitLocations(std::vector<int> &changedIndex)
{
	std::vector<int> changedSuitLocations;
	for (int i = 0; i < changedIndex.size(); i++)
	{
		if (changedIndex.at(i) > 7)
		{
			changedSuitLocations.push_back(changedIndex.at(i));
		}
	}
	return changedSuitLocations;
}

std::vector<int> KlondikeGame::findChangedCardLocations(const std::vector<Card> &newTopCards) 
{
	std::vector<int> indexOfChangedLocation;
	for (int i = 0; i < currentPlayingBoard.size(); i++)
	{
		if (i == 7)	// card is different from the topcard of the talon AND the card isn't empty
		{
			if (newTopCards.at(7) != currentPlayingBoard.at(7).getLastCard())
			{
				indexOfChangedLocation.push_back(7);
			}
		}
		else if (currentPlayingBoard.at(i).knownCards.empty())	// adding card to an empty location
		{
			if (!newTopCards.at(i).isEmptyCard())	// new card isn't empty
			{
				indexOfChangedLocation.push_back(i);
			}
		}
		else
		{
			if (currentPlayingBoard.at(i).getLastCard() != newTopCards.at(i))
			{
				indexOfChangedLocation.push_back(i);
			}
		}
	}
	return indexOfChangedLocation;
}

int KlondikeGame::determineIndexOfPressedCard(const int & x, const int & y)
{	
	for (int i = 0; i < cardRegions.size(); i++)
	{
		if (cardRegions.at(i).y <= y && y <= cardRegions.at(i).y + cardRegions.at(i).height
			&& cardRegions.at(i).x <= x && x <= cardRegions.at(i).x + cardRegions.at(i).width)
		{
			locationOfLastPress.x = x - cardRegions.at(i).x;
			locationOfLastPress.y = y - cardRegions.at(i).y;
			if (i == 7) //pile clicked
			{
				incrementPilePresses();
				return 99;

			}
			return i;
		}
		else if (cardRegions.at(7).y <= y && y <= cardRegions.at(7).y + cardRegions.at(7).height
			&& cardRegions.at(1).x <= x && x <= (cardRegions.at(1).x + cardRegions.at(1).width) * 1.43) 
		{
			//talon clicked, talon is 1.43 times wider than regular card
			locationOfLastPress.x = x - cardRegions.at(1).x;
			locationOfLastPress.y = y - cardRegions.at(7).y;
			return 7;
		}
	}
	return -1;
}

