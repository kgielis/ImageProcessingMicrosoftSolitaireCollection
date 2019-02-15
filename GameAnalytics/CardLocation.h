#pragma once

#include <vector>
#include "Card.h" 

class CardLocation
{
public:
	CardLocation();
	CardLocation(Card card, int remainingCards);
	CardLocation(std::vector<Card> knownCards); //used for Freecell
	CardLocation(std::vector<Card> knownCards, int remainingCards);
	~CardLocation();

	Card getLastCard() const;

	std::vector<Card> knownCards;	// used for the build- and suit stack
	int remainingCards;
};
