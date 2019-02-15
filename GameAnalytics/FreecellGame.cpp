#include "stdafx.h"
#include "FreecellGame.h"

FreecellGame::FreecellGame(ScreenCaptureService sc)	: Game(sc)
{
	std::vector<int> emptyVector(16, 0);
	metrics.numberOfPresses = emptyVector;
}

FreecellGame::~FreecellGame()
{
}

void FreecellGame::initPlayingBoard()
{
	//capture screen
	cv::Mat screenCapture = screenCaptureService.captureScreen();
	while (!screenCaptureService.checkForPlayingScreen(screenCapture))
	{
		screenCapture = screenCaptureService.captureScreen();
	}
	
	//Fetch dificulty
	ExtractButtonsAndLabels extractLabels = ExtractButtonsAndLabels();
	std::vector<std::string> topBarData = extractLabels.getTopBarData(screenCapture);
	metrics.gameType = topBarData.at(0);
	if (topBarData.size() > 1) metrics.gameDifficulty = topBarData.at(1);
	metrics.gameSeed = extractLabels.extractSeed(screenCapture, classifyCard);

	//extract card region coordinates
	cardRegions = extractCards.extractCardRegionLocations(screenCapture);

	//extract cards
	extractCards.determineROI(screenCapture, 118);
	std::vector<std::vector<cv::Mat>> extractedImagesFromPlayingBoard = extractCards.findAllCardsFromBoardImage(screenCapture);
	assert(extractedImagesFromPlayingBoard.size() == 16);

	//classify cards
	std::vector<std::vector<Card>> classifiedCardsFromPlayingBoard = classifyCard.classifyAllExtractedCards(extractedImagesFromPlayingBoard);
	assert(classifiedCardsFromPlayingBoard.size() == 16);
	//init playingboard
	currentPlayingBoard.resize(16);

	// build stack
	for (int i = 0; i < 8; i++)	// add the classified cards as the only item of known cards and set the amount of unknown cards of each location
	{
		CardLocation newLocation(classifiedCardsFromPlayingBoard.at(i));
		currentPlayingBoard.at(i) = newLocation;
	}

	// Storage space
	for (int i = 8; i < 12; i++)	// add the classified cards as the only item of known cards and set the amount of unknown cards of each location
	{
		CardLocation newLocation(classifiedCardsFromPlayingBoard.at(i));
		currentPlayingBoard.at(i) = newLocation;
	}

	// suit stack
	for (int i = 12; i < 16; i++)
	{
		CardLocation newLocation(classifiedCardsFromPlayingBoard.at(i));
		currentPlayingBoard.at(i) = newLocation;
	}
	previousPlayingBoards.push_back(currentPlayingBoard);	// add the first game state to the list of all game states
	printPlayingBoardState();	// print the board
	startTimers();
}

void FreecellGame::printPlayingBoardState()
{
	std::cout << "Storage: " << std::endl;	// print the current topcard from deck
	for (int i = 8; i < 12; i++)
	{
		std::cout << "   Pos " << i << ": ";
		if (currentPlayingBoard.at(i).knownCards.empty())
		{
			std::cout << "// ";
		}
		else {
			std::cout << static_cast<char>(currentPlayingBoard.at(i).knownCards.back().rank);
			std::cout << static_cast<char>(currentPlayingBoard.at(i).knownCards.back().suit);
		}

		std::cout << std::endl;
	}


	std::cout << "Suit stack: " << std::endl;		// print all cards from the suit stack
	for (int i = 12; i < currentPlayingBoard.size(); i++)
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
		std::cout << std::endl;
	}

	std::cout << "Build stack: " << std::endl;		// print all cards from the build stack
	for (int i = 0; i < 8; i++)
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
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

std::vector<Card> FreecellGame::findCards(cv::Mat image, POINT pt)
{
	//extract cards
	std::vector<cv::Mat> extractedImagesFromPlayingBoard = extractCards.findCardsFromBoardImage(image, FREECELL);

	//classify cards
	std::vector<Card> newTopCards = classifyCard.classifyExtractedCards(extractedImagesFromPlayingBoard);
	return newTopCards;
}

Click FreecellGame::findClick(const POINT &pt)
{
	//find Selected card
	int cardLocation = determineIndexOfPressedCard(pt.x, pt.y);
	Click click = Click(pt, cardLocation);
	if (cardLocation != -1)
	{
		std::pair<int, int> selected = extractCards.findSelectedCards(locationOfLastPress, cardLocation);
		int selectedCardIndex = selected.first;
		click.setNumberOfCardsWithBlueBorder(selected.second);
		if (selectedCardIndex == -98)
		{
			std::cout << "unmovable card selected" << std::endl;
			click.setNotmovableCardSelected(true);
			selectedCardIndex = 0;
		}
		click.setSelectedIndex(selectedCardIndex);
		int size = currentPlayingBoard.at(cardLocation).knownCards.size();
		if (cardLocation < 8 && selectedCardIndex > 0 && size > selectedCardIndex + 1)
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
		if (cardLocation > 7) addClickCoordOnCard(locationOfLastPress);
		else if (click.getSelectedIndex() == 0 && !click.getSelectedCard().isEmptyCard())
		{
			addClickCoordOnCard(locationOfLastPress);
		}
	}
	return click;
}

bool FreecellGame::updateBoard(const std::vector<Card> & newTopCards, POINT pt, Timepoint timestamp, bool drag)
{
	//load board to process if necessary 
	std::vector<Card> extractedTopCards;
	if (previousExtractedTopCards.empty()) extractedTopCards = newTopCards;
	else { extractedTopCards = previousExtractedTopCards.front();  previousExtractedTopCards.pop(); }
	
	//find pressed location index
	currentClick = findClick(pt);
	std::cout << "clicked at location index: " << currentClick.getLocationIndex() << std::endl;
	incrementPressesAtLoc(currentClick.getLocationIndex());
	currentClick.setTimestamp(timestamp);
	currentClick.setDragClick(drag);
	if (currentClick.isNotMovableCardSelected())
	{
		previousBoardClicks.push_back(currentClick);
		return false;
	}

	//find changedLocations
	std::vector<int> indexOfChangedLocation = findChangedCardLocations(extractedTopCards);

	//updateBoard
	if (currentClick.getLocationIndex() == -1 && indexOfChangedLocation.size() == 0) //no update of board
	{
		std::cout << "NO UPDATE, no card selected" << std::endl;
		return false;
	}
	else if (currentClick.getLocationIndex() != -1)
	{
		if (indexOfChangedLocation.size() > 0)
		{
			// board changed at currentlySelectedIndex
			if (previousBoardClicks.empty())
			{
				previousBoardClicks.push_back(currentClick);
				std::cout << "NO UPDATE, no previous clicks;" << std::endl;
				return false;
			}
			else
			{
				int sourceLocation = previousBoardClicks.back().getLocationIndex();
				int destLocation = currentClick.getLocationIndex();
				//std::cout << "try move: " << sourceLocation << " -> " << destLocation << std::endl;
				// if sourceLocation == destLocation ==> double click, so card moved to suit/or storage
				if (sourceLocation != destLocation)
				{
					bool moveHappened = moveAllPossibleCards(sourceLocation, destLocation, extractedTopCards, false);
					if (moveHappened)
					{
						processAutoplayCards(extractedTopCards);
						addSuccesMoveMetrics();
						previousBoardClicks.clear();
					}
					else 
					{
						if (previousBoardClicks.size() > 0) checkErrors(previousBoardClicks.back(), currentClick);
						previousBoardClicks.clear();
						previousBoardClicks.push_back(currentClick);
						std::cout << "NO UPDATE, it was not a move" << std::endl;
					}
				}
				else if (std::chrono::duration_cast<std::chrono::milliseconds>
					(currentClick.getTimestamp() - previousBoardClicks.back().getTimestamp()).count() <= GetDoubleClickTime() + 100 
					&& !drag)
				{
					//double click  (card can move to suit stack or storage)
					//find where card is now
					//std::cout << "sourceLocation == destLocation" << std::endl;
					processDoubleClickMoves(sourceLocation, extractedTopCards);
				}
				else if (drag)
				{
					std::cout << "drag click detected" << std::endl;
					if (previousBoardClicks.size() > 0) checkErrors(previousBoardClicks.back(), currentClick);
					previousBoardClicks.push_back(currentClick);
					if (previousBoardClicks.back().isDragClick())
					{
						previousBoardClicks.clear();
					}
					std::cout << "NO UPDATE, no double click, it was not a move" << std::endl;
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
	if (!compareTopCards(extractedTopCards) && !compareTopCards(newTopCards))
	{
		//save topCards For Later
		std::cout << "extracted cards: ";
		for (int i = 0; i < extractedTopCards.size(); i++)
		{
			std::cout << static_cast<char>(extractedTopCards.at(i).getRank());
			std::cout << static_cast<char>(extractedTopCards.at(i).getSuit()) << " ";
		}
		std::cout << std::endl;
		previousExtractedTopCards.push(extractedTopCards);
		std::cout << "board update not complete, save extractedTopCards" << std::endl;
	}
	else
	{
		std::queue<std::vector<Card>> empty;
		std::swap(previousExtractedTopCards, empty);
	}
	std::cout << "topCards: ";
	for (int i = 0; i < newTopCards.size(); i++)
	{
		std::cout << static_cast<char>(newTopCards.at(i).getRank());
		std::cout << static_cast<char>(newTopCards.at(i).getSuit()) << " ";
	}
	std::cout << std::endl;

	printPlayingBoardState();
	return true;
}

void FreecellGame::processDoubleClickMoves(int sourceLocation, std::vector<Card> &extractedTopCards)
{
	int destLocation = sourceLocation;
	Card movedCard = currentPlayingBoard.at(sourceLocation).getLastCard();
	auto cardIt = std::find(extractedTopCards.begin(), extractedTopCards.end(), movedCard);
	if (cardIt != extractedTopCards.end())
	{
		destLocation = (int)std::distance(extractedTopCards.begin(), cardIt);
		//std::cout << "destLocation: " << destLocation << std::endl;
		if (destLocation > 7 && destLocation < 12) //card moved to storage
		{
			if (!moveAllPossibleCards(sourceLocation, destLocation, extractedTopCards, true))
			{
				std::cout << "double click: not a move" << std::endl;
			}
			processAutoplayCards(extractedTopCards);
			addSuccesMoveMetrics();
			previousBoardClicks.clear();
		}
		else if (destLocation < 8)
		{
			previousBoardClicks.clear();
			previousBoardClicks.push_back(currentClick);
			std::cout << "NO UPDATE, double click, no move to suit stack or storage" << std::endl;
		}

	}
	if (cardIt == extractedTopCards.end() || destLocation > 11)
	{
		//if card is not topcard anymore, it moved to suit
		//check if clicked autoplay card wasn't first placed to storage 
		extractedTopCards = moveAutoPlayCardToStorageIfNeeded(sourceLocation, extractedTopCards);
		if (currentPlayingBoard.at(sourceLocation).getLastCard() == movedCard) //no move to storage
		{
			destLocation = suitToValueMap.at(movedCard.getSuit());
			if (!moveAllPossibleCards(sourceLocation, destLocation, extractedTopCards, true))
			{
				std::cout << "double click: not a move" << std::endl;
			}
		}
		processAutoplayCards(extractedTopCards);
		addSuccesMoveMetrics();
		previousBoardClicks.clear();
	}
	else if (!previousBoardClicks.empty()) //no move happend yet
	{
		previousBoardClicks.push_back(currentClick);
		std::cout << "NO UPDATE, double click, it was not a move" << std::endl;
	}
}

void FreecellGame::addSuccesMoveMetrics()
{
	//if last click of move is same as first click of this move, last move is invallid (not a move)
	Move move;
	Click firstClickOfMove = previousBoardClicks.back();
	Click secondClickOfMove = currentClick;
	Timepoint prevMoveTime;
	if (metrics.listOfMoves.empty()) prevMoveTime = metrics.startOfGame;
	else prevMoveTime = metrics.listOfMoves.back().endOfMove;

	move.thinkDuration = timeDifferenceBetween(firstClickOfMove.getTimestamp(), prevMoveTime);
	move.startOfMove = firstClickOfMove.getTimestamp();
	move.endOfMove = secondClickOfMove.getTimestamp();
	move.moveDuration = timeDifferenceBetween(secondClickOfMove.getTimestamp(), firstClickOfMove.getTimestamp());
	move.succesfull = true;
	move.movedCard = firstClickOfMove.getSelectedCard();
	move.destCard = secondClickOfMove.getSelectedCard();
	move.sourceLocation = firstClickOfMove.getLocationIndex();
	move.destLocation = secondClickOfMove.getLocationIndex();
	std::cout << "movedCard: " << static_cast<char>(move.movedCard.getRank());
	std::cout << static_cast<char>(move.movedCard.getSuit()) << " ";
	std::cout << "destCard: " << static_cast<char>(move.destCard.getRank());
	std::cout << static_cast<char>(move.destCard.getSuit()) << std::endl;
	move.numberOfCardsMoved = firstClickOfMove.getNumberOfSelectedCards();
	move.timeOfMove = std::time(0);
	metrics.listOfMoves.push_back(move);
	removeFalseMoves();
}
	
void FreecellGame::checkErrors(Click firstClick, Click secondClick)
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
	Card sourceSelectedCard = Card(EMPTY_RANK, EMPTY_SUIT);
	Card destSelectedCard = Card(EMPTY_RANK, EMPTY_SUIT);
	if (firstClick.getSelectedIndex() != -1)
	{
		if (currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.size() != 0)
		{
			int index = (int)currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.size() - firstClick.getSelectedIndex() - 1;
			try 
			{
				if (index > 0 && index < currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.size())
				{
					sourceSelectedCard = currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.at(index);
				}
			}
			catch (const std::out_of_range& oor) 
			{
				sourceSelectedCard = currentPlayingBoard.at(firstClick.getLocationIndex()).getLastCard();
			}
		}
	}
	else {
		std::cout << "-1 source location!!!" << std::endl;
	}
	destSelectedCard = currentPlayingBoard.at(secondClick.getLocationIndex()).getLastCard();

	move.movedCard = sourceSelectedCard;
	move.destCard = destSelectedCard;
	move.numberOfCardsMoved = firstClick.getSelectedIndex() + 1;

	//check if card was movable
	if (firstClick.isNotMovableCardSelected())
	{
		std::cout << "not movable card bool was true"<< std::endl;
		move.movedCard = Card(UNKNOWN_RANK, UNKNOWN_SUIT);
		move.unmovableErr = true;
		metrics.listOfMoves.push_back(move);
		return;
	}
	else 
	{
		//if card is not last one
		move = checkIfCardWasMovable(firstClick, move);
	}
	
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

	//check rank errors
	if (sourceSelectedCard.getRankValue() + 1 != destSelectedCard.getRankValue()
		&& !destSelectedCard.isEmptyCard()) 
	{
		std::cout << "Incompatible rank! " << static_cast<char>(sourceSelectedCard.getRank()) << " can't go on ";
		std::cout << static_cast<char>(destSelectedCard.getRank()) << std::endl;
		incrementRankErrors();
		move.rankErr = true; 
	}

	//check moved to many cards error
	int	maxMovableCards = getMaxMovableCards(firstClick, secondClick);
	if (firstClick.getSelectedIndex() + 1 >= maxMovableCards)
	{
		std::cout << "Tried to move too many cards! " << firstClick.getSelectedIndex() + 1 << ", you can move only ";
		std::cout << maxMovableCards << " cards" << std::endl;
		metrics.numberOfTooManyCardsMovedErrors++;
		move.tooManyCardsMovementErr = true;
	}
	metrics.listOfMoves.push_back(move);
}

int FreecellGame::getMaxMovableCards(Click &firstClick, Click &secondClick)
{
	int nrFreeCells = std::count_if(currentPlayingBoard.begin() + 8, currentPlayingBoard.begin() + 12,
		[](CardLocation cl) { return cl.getLastCard().isEmptyCard(); });
	int index = 0;
	int nrFreeSpots = std::count_if(currentPlayingBoard.begin(), currentPlayingBoard.begin() + 8,
		[&secondClick, &index](CardLocation cl)
	{
		bool isFreeSpot = cl.knownCards.empty() && index != secondClick.getLocationIndex();
		index++;
		return isFreeSpot;
	});
	return (1 + nrFreeCells) * pow(2, nrFreeSpots);
}

Move FreecellGame::checkIfCardWasMovable(Click &firstClick, Move move)
{
	int selectedIndex = firstClick.getSelectedIndex();
	Card selectedCard = firstClick.getSelectedCard();
	while (selectedIndex > 0)
	{
		int index = (int)currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.size() - selectedIndex - 1;
		if (index < 0 || index >= currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.size())
		{
			return move;
		}
		Card cardUnder = currentPlayingBoard.at(firstClick.getLocationIndex()).knownCards.at(index + 1);
		if (cardUnder.getRankValue() + 1 != selectedCard.getRankValue())
		{
			Suit prevSuit = selectedCard.getSuit();
			Suit nextSuit = cardUnder.getSuit();
			if (((prevSuit == 'H' || prevSuit == 'D') && (nextSuit != 'H' || nextSuit != 'D'))	// check for color error
				|| ((prevSuit == 'S' || prevSuit == 'C') && (nextSuit != 'S' || nextSuit != 'C')))
			{
				// not movable 
				std::cout << "found that card was not movable" << std::endl;
				move.unmovableErr = true;
				break;
			}
		}
		selectedCard = cardUnder;
		selectedIndex--;
	}
	return move;
}

bool FreecellGame::checkIfAutoPlayIsPossible(Card autoplayCard) 
{
	int suitIndex = suitToValueMap.at(autoplayCard.getSuit());
	Card topSuitCard = currentPlayingBoard.at(suitIndex).getLastCard();
	int topCardValue = topSuitCard.getRankValue();
	if (topCardValue > 1)
	{
		//check if next card is allowed
		if (suitIndex == 12 || suitIndex == 14) //card is red
		{
			int clubsValue = rankValueMap.at(currentPlayingBoard.at(13).getLastCard().getRank());
			int spadesValue = rankValueMap.at(currentPlayingBoard.at(15).getLastCard().getRank());
			if (clubsValue >= topCardValue && spadesValue >= topCardValue)
			{
				//you are able to add the next card
				return true;
			}
		}
		else if (suitIndex == 13 || suitIndex == 15) //card is black
		{
			int heartsValue = rankValueMap.at(currentPlayingBoard.at(12).getLastCard().getRank());
			int diamondsValue = rankValueMap.at(currentPlayingBoard.at(14).getLastCard().getRank());
			if (heartsValue >= topCardValue && diamondsValue >= topCardValue)
			{
				//you are able to add the next card
				return true;
			}
		}
	}
	else 
	{
		//card move allowed, A or 2 always allowed
		return true;
	}
	return false;

}

std::vector<Card> FreecellGame::moveAutoPlayCardToStorageIfNeeded(int sourceIndex, std::vector<Card> newTopCards)
{
	auto autoplayCards = checkAutoPlay(newTopCards);
	//find autoplayCard at top of sourceLocation, which is the card that moved first
	auto it = find_if(autoplayCards.begin(), autoplayCards.end(), [&sourceIndex, this](AutoplayCard autoplayCard) 
	{
		return autoplayCard.locationIndex == sourceIndex
			&& autoplayCard.cardIndex == currentPlayingBoard.at(sourceIndex).knownCards.size() - 1;
	});
	if (it != autoplayCards.end())
	{
		//if card cannot directly be placed on suit stack, it first moved to storage
		Card firstMovedCard = it->card;
		Card topCardSuit = currentPlayingBoard.at(suitToValueMap.at(firstMovedCard.getSuit())).getLastCard();
		int topCardSuitValue = rankValueMap.at(topCardSuit.getRank());
		if (rankValueMap.at(firstMovedCard.getRank()) != topCardSuitValue + 1)
		{
			//move card to storage 
			for (int i = 8; i < 12; i++)
			{
				if (currentPlayingBoard.at(i).getLastCard().isEmptyCard())
				{
					moveAllPossibleCards(sourceIndex, i, newTopCards, false);
					//update newtopCards/extractedTopCards
					newTopCards.at(sourceIndex) = currentPlayingBoard.at(sourceIndex).getLastCard();
					newTopCards.at(i) = currentPlayingBoard.at(i).getLastCard();
					return newTopCards;
				}
			}
		}
	}
	return newTopCards;
}

bool FreecellGame::moveAllPossibleCards(int sourceIndex, int destIndex, std::vector<Card> newTopCards, bool doubleClick)
{
	//take all moveable cards from source location
	int indexTop = currentPlayingBoard.at(sourceIndex).knownCards.size() - 2;
	if (currentPlayingBoard.at(sourceIndex).knownCards.empty()) return false;
	if (previousBoardClicks.back().isNotMovableCardSelected()) return false;
	Card lastMovableCard = currentPlayingBoard.at(sourceIndex).getLastCard();
	Card destTopCard = currentPlayingBoard.at(destIndex).getLastCard();
	if (currentPlayingBoard.at(destIndex).knownCards.size() == 0) destTopCard = Card(EMPTY_RANK, EMPTY_SUIT);
	if (destIndex < 8)
	{
		for (int i = indexTop; i >= 0; i--)
		{
			int rankValue = rankValueMap.at(lastMovableCard.getRank());
			Card nextCard = currentPlayingBoard.at(sourceIndex).knownCards.at(i);
			int rankValueNextCard = rankValueMap.at(nextCard.getRank());
			if (rankValueNextCard == rankValue + 1)
			{
				if ((lastMovableCard.getSuit() == 'H' || lastMovableCard.getSuit() == 'D') &&
					(nextCard.getSuit() == 'C' || nextCard.getSuit() == 'S'))
				{
					if (destTopCard.isEmptyCard() || rankValueMap.at(destTopCard.getRank()) > rankValueMap.at(nextCard.getRank()))
					{
						// nextCard is moveable
						lastMovableCard = nextCard;
					}

				}
				else if ((lastMovableCard.getSuit() == 'C' || lastMovableCard.getSuit() == 'S') &&
					(nextCard.getSuit() == 'H' || nextCard.getSuit() == 'D'))
				{
					if (destTopCard.isEmptyCard() || rankValueMap.at(destTopCard.getRank()) > rankValueMap.at(nextCard.getRank()))
					{
						// nextCard is movable
						lastMovableCard = nextCard;
					}
				}
				else break;
			}
			else break;
		}
	}
	else if (destIndex > 7)
	{
		//card moved to storage or suit
		if (destIndex < 12 && currentPlayingBoard.at(destIndex).getLastCard().isEmptyCard())
		{
			currentClick.setSelectedCard(currentPlayingBoard.at(destIndex).getLastCard());
			currentClick.setSelectedIndex(0);
			previousBoardClicks.back().setNumberOfSelectedCards(1);
			previousBoardClicks.back().setSelectedCard(currentPlayingBoard.at(sourceIndex).getLastCard());
			currentPlayingBoard.at(destIndex).knownCards.push_back(currentPlayingBoard.at(sourceIndex).getLastCard());
			currentPlayingBoard.at(sourceIndex).knownCards.pop_back();
			previousPlayingBoards.push_back(currentPlayingBoard);
			return true;
		}
		else if (destIndex > 11)
		{
			if (currentPlayingBoard.at(sourceIndex).getLastCard().getRankValue()
				== currentPlayingBoard.at(destIndex).getLastCard().getRankValue() + 1)
			{
				currentClick.setSelectedCard(currentPlayingBoard.at(destIndex).getLastCard());
				currentClick.setSelectedIndex(0);
				previousBoardClicks.back().setNumberOfSelectedCards(1);
				previousBoardClicks.back().setSelectedCard(currentPlayingBoard.at(sourceIndex).getLastCard());
				currentPlayingBoard.at(destIndex).knownCards.push_back(currentPlayingBoard.at(sourceIndex).getLastCard());
				currentPlayingBoard.at(sourceIndex).knownCards.pop_back();
				previousPlayingBoards.push_back(currentPlayingBoard);
				return true;
			}
		}
		return false; 
	}
	
	//check if you can place the card on the destination, if not, skip because not real move
	if (destTopCard.isEmptyCard() || isCardMoveAllowed(lastMovableCard, destTopCard))
	{
		if (destTopCard.isEmptyCard())
		{
			std::cout << previousBoardClicks.back().getNumberOfCardsWithBlueBorder() << std::endl;
			if (previousBoardClicks.back().getNumberOfCardsWithBlueBorder() != -1)
			{
				int index = currentPlayingBoard.at(sourceIndex).knownCards.size() - previousBoardClicks.back().getNumberOfCardsWithBlueBorder();
				std::cout << index << std::endl;
				lastMovableCard = currentPlayingBoard.at(sourceIndex).knownCards.at(index);
			}
			std::cout << static_cast<char>(lastMovableCard.getRank()) << static_cast<char>(lastMovableCard.getSuit()) << std::endl;
		}
		//find iterator to lastMovableCard
		auto lastMovableCardIt = find(currentPlayingBoard.at(sourceIndex).knownCards.begin(),
			currentPlayingBoard.at(sourceIndex).knownCards.end(), lastMovableCard);

		std::cout << "lastMovableCard: " << static_cast<char>(lastMovableCard.getRank());
		std::cout << static_cast<char>(lastMovableCard.getSuit()) << std::endl; 

		//do move
		currentClick.setSelectedCard(currentPlayingBoard.at(destIndex).getLastCard());
		currentClick.setSelectedIndex(0);
		previousBoardClicks.back().setSelectedCard(lastMovableCard);
		int numberOfCardSelected = std::distance(lastMovableCardIt, currentPlayingBoard.at(sourceIndex).knownCards.end());
		previousBoardClicks.back().setNumberOfSelectedCards(numberOfCardSelected);
		if (getMaxMovableCards(previousBoardClicks.back(), currentClick) >= numberOfCardSelected)
		{
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
	}	
	return false;
}

bool FreecellGame::isCardMoveAllowed(Card moveCard, Card destTopCard) 
{
	//check rank
	if (rankValueMap.at(destTopCard.getRank()) != rankValueMap.at(moveCard.getRank()) + 1) return false;

	//check suit
	if ((moveCard.getSuit() == 'H' || moveCard.getSuit() == 'D') &&
		(destTopCard.getSuit() == 'H' || destTopCard.getSuit() == 'D'))
	{
		return false;
	}

	else if ((moveCard.getSuit() == 'C' || moveCard.getSuit() == 'S') &&
		(destTopCard.getSuit() == 'C' || destTopCard.getSuit() == 'S'))
	{
		return false;
	}
	return true;
}

std::vector<int> FreecellGame::findChangedCardLocations(const std::vector<Card>& newTopCards)
{
	std::vector<int> indexOfChangedLocation;
	for (int i = 0; i < newTopCards.size(); i++)
	{
		if (currentPlayingBoard.at(i).knownCards.empty())
		{
			if (!newTopCards.at(i).isEmptyCard())
			{
				indexOfChangedLocation.push_back(i);
			}
		}
		else if (currentPlayingBoard.at(i).knownCards.back() != newTopCards.at(i))
		{
			indexOfChangedLocation.push_back(i);
		}
	}
	return indexOfChangedLocation;
}

std::vector<AutoplayCard> FreecellGame::checkAutoPlay(const std::vector<Card> & newTopCards)
{
	// check if new cards was added to the suit stack
	std::vector<AutoplayCard> movedCards;
	for (int i = 12; i < 16; i++)
	{
		Suit suit = newTopCards.at(i).suit;
		if (currentPlayingBoard.at(i).knownCards.back() != newTopCards.at(i))
		{
			//this location has been changed, find how many cards
			int valuePrevCard = rankValueMap.at(currentPlayingBoard.at(i).getLastCard().rank);	//value of prev top card
			int valueNewCard = rankValueMap.at(newTopCards.at(i).rank);							//value of new top card
			int numberOfCardsMovedToSuitDeck = abs(valueNewCard - valuePrevCard);

			for (int k = valuePrevCard + 1; k <= numberOfCardsMovedToSuitDeck + valuePrevCard; k++)
			{
				//find location of card change, 
				Rank rank = valueRankMap.at(k);
				std::pair<int, int> location = findCardLocationOnBoard(currentPlayingBoard, rank, suit);
				AutoplayCard autoplayCard;
				autoplayCard.card = Card(rank, suit);
				autoplayCard.locationIndex = location.first;
				autoplayCard.cardIndex = location.second;
				movedCards.push_back(autoplayCard);	
			}
		}
	}
	return movedCards;
}

void FreecellGame::moveCardToSuit(int suitLocation, int locationIndex, int cardIndex)
{
	//add card to suit
	Card movedCard = currentPlayingBoard.at(locationIndex).knownCards.at(cardIndex);
	currentPlayingBoard.at(suitLocation).knownCards.push_back(movedCard);

	//remove card
	currentPlayingBoard.at(locationIndex).knownCards
		.erase(currentPlayingBoard.at(locationIndex).knownCards.begin() + cardIndex);
	
	//save move
	previousPlayingBoards.push_back(currentPlayingBoard);
}

bool FreecellGame::processAutoplayCards(const std::vector<Card>& newTopCards)
{
	bool exit = false;
	while (!exit)
	{
		for (int i = 0; i < 12; i++)
		{
			Card topCard = currentPlayingBoard.at(i).getLastCard();
			if (!topCard.isEmptyCard())
			{
				//check if can move to suit && check if automove is allowed
				int suitIndex = suitToValueMap.at(topCard.getSuit());
				Card suitTopCard = currentPlayingBoard.at(suitIndex).getLastCard();
				if (suitTopCard.getRankValue() + 1 == topCard.getRankValue() && checkIfAutoPlayIsPossible(topCard))
				{
					//check if move actually happend
					if (newTopCards.at(suitIndex).getRankValue() >= topCard.getRankValue())
					{
						/*std::cout << "moved autplay card " << static_cast<char>(topCard.rank) << static_cast<char>(topCard.suit);
						std::cout << " at " << i << std::endl;*/
						moveCardToSuit(suitIndex, i, currentPlayingBoard.at(i).knownCards.size() - 1);
						break;
					}
				}
			}
			if (i == 11) exit = true;
		}
	}
	return true;
}

bool FreecellGame::compareTopCards(const std::vector<Card> newTopCards)
{
	//if all topcards are empty you are in end game situation
	if (all_of(currentPlayingBoard.begin() + 12, currentPlayingBoard.end(), [](CardLocation cardlocation)
	{ return cardlocation.getLastCard().getRank() == KING; })) return true;
	for (int i = 0; i < newTopCards.size(); i++)
	{
		if (newTopCards.at(i).isEmptyCard())
		{
			if (i < 8)
			{
				if (currentPlayingBoard.at(i).knownCards.size() != 0) return false;
			}
			else {
				if (!currentPlayingBoard.at(i).knownCards.back().isEmptyCard()) return false;
			}
		}
		else if (currentPlayingBoard.at(i).knownCards.size() > 0)
		{
			if (newTopCards.at(i) != currentPlayingBoard.at(i).knownCards.back()) return false;
		}
	}
	return true;
}

int FreecellGame::determineIndexOfPressedCard(const int & x, const int & y)
{
	// check if a cardlocation has been pressed and remap the coordinate to that cardlocation
	
	for (int i = 0; i < cardRegions.size(); i++) 
	{
		if (cardRegions.at(i).y <= y && y <= cardRegions.at(i).y + cardRegions.at(i).height
			&& cardRegions.at(i).x <= x && x <= cardRegions.at(i).x + cardRegions.at(i).width)
		{
			locationOfLastPress.x = x - cardRegions.at(i).x;
			locationOfLastPress.y = y - cardRegions.at(i).y;
			return i;
		}
	}
	return -1;
}

