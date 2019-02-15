#include "stdafx.h"
#include "Click.h"


Click::Click()
{
}

Click::Click(const POINT coord, int locationIndex) 
	: coord{ coord }, locationIndex{ locationIndex }
{
}


Click::~Click()
{
}

void Click::setLocationIndex(int locationIndex)
{
	this->locationIndex = locationIndex;
}

int Click::getLocationIndex()
{
	return locationIndex;
}

void Click::setSelectedIndex(int selectedIndex)
{
	this->selectedIndex = selectedIndex;
}

int Click::getSelectedIndex()
{
	return selectedIndex;
}

void Click::setSelectedCard(Card selectedCard)
{
	this->selectedCard = selectedCard;
}

Card Click::getSelectedCard()
{
	return selectedCard;
}

void Click::setNumberOfSelectedCards(int number)
{
	this->numberOfSelectedCards = number;
}

int Click::getNumberOfSelectedCards()
{
	return numberOfSelectedCards;
}

void Click::setTimestamp(Timepoint timestamp)
{
	this->timestamp = timestamp;
}

Timepoint Click::getTimestamp()
{
	return timestamp;
}

void Click::setNotmovableCardSelected(bool isNotMovable)
{
	isNotMovableCard = isNotMovable;
}

bool Click::isNotMovableCardSelected()
{
	return isNotMovableCard;
}

void Click::setNumberOfCardsWithBlueBorder(int number)
{
	numberOfCardsWithBlueBorder = number;
}

int Click::getNumberOfCardsWithBlueBorder()
{
	return numberOfCardsWithBlueBorder;
}

bool Click::isDragClick()
{
	return dragClick;
}

void Click::setDragClick(bool dragClick)
{
	this->dragClick = dragClick;
}

void Click::print()
{

	std::tm tm;
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch());
	std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(ms);
	std::time_t t = s.count(); localtime_s(&tm, &t);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%F %T");
	std::cout << "CLICK: " << locationIndex << ", " << selectedIndex << ", " << oss.str() << " ";
	std::cout << static_cast<char>(selectedCard.rank) << static_cast<char>(selectedCard.suit) << " ";
	std::cout << numberOfSelectedCards << std::endl;
}
