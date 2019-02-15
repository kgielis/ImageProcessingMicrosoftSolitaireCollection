#include "stdafx.h"
#include "CardLocation.h"


CardLocation::CardLocation()
	: knownCards{}, remainingCards{}
{
}

CardLocation::CardLocation(Card knownCard, int remainingCards)
	: knownCards{}, remainingCards{ remainingCards }
{
	knownCards.push_back(knownCard);
}

CardLocation::CardLocation(std::vector<Card> cards)
	: knownCards{ cards }, remainingCards{ 0 }
{
	if (knownCards.empty())
	{
		knownCards.push_back(Card(EMPTY_RANK, EMPTY_SUIT, true));
	}
}

CardLocation::CardLocation(std::vector<Card> knownCards, int remainingCards)
	: knownCards{ knownCards }, remainingCards{ remainingCards }
{
}

CardLocation::~CardLocation()
{
}

Card CardLocation::getLastCard() const
{
	//if (knownCards.empty() && !topCard.isEmptyCard()) return topCard;	// return last card of talon location
	if (knownCards.empty()) return Card(EMPTY_RANK, EMPTY_SUIT, true);
	return knownCards.back();
}
