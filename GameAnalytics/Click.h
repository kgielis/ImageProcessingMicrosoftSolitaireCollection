#pragma once
#include "globals.h"
class Click
{
public:
	Click();
	Click(POINT coord, int locationIndex);
	~Click();
	
	void setLocationIndex(int locationIndex);
	int getLocationIndex();
	void setSelectedIndex(int selectedIndex);
	int getSelectedIndex();
	void setSelectedCard(Card selectedCard);
	Card getSelectedCard();
	void setNumberOfSelectedCards(int number);
	int getNumberOfSelectedCards();
	void setTimestamp(Timepoint timestamp);
	Timepoint getTimestamp();
	void setNotmovableCardSelected(bool isNotMovable);
	bool isNotMovableCardSelected();
	void setNumberOfCardsWithBlueBorder(int number);
	int getNumberOfCardsWithBlueBorder();
	bool isDragClick();
	void setDragClick(bool dragClick);
	void Click::print();

private:
	POINT coord;
	int locationIndex = -1;
	int selectedIndex = -1;
	Card selectedCard = Card(UNKNOWN_RANK, UNKNOWN_SUIT);
	int numberOfSelectedCards = 0;
	Timepoint timestamp;
	bool isNotMovableCard = false;
	int numberOfCardsWithBlueBorder = -1;
	bool dragClick = false;
};

