#include "stdafx.h"
#include "Card.h"

Card::Card()
{
}

Card::Card(Rank r, Suit s) : rank{ r }, suit{ s }
{
	if (r == EMPTY_RANK && s == EMPTY_SUIT)
	{
		emptyCard = true;
	}
}

Card::Card(Rank r, Suit s, bool empty) : rank{ r }, suit{ s }, emptyCard{ empty }
{
}

Card::~Card()
{
}

Rank Card::getRank() const
{
	return rank;
}

Suit Card::getSuit() const
{
	return suit;
}

int Card::getRankValue() const 
{
	return rankValueMap.at(rank);
}

bool Card::isEmptyCard() const
{
	return emptyCard;
}

void Card::setEmptyCard(bool empty)
{
	rank = EMPTY_RANK;
	suit = EMPTY_SUIT;
	emptyCard = empty;
}

bool Card::isUnknownCard()
{
	return rank == UNKNOWN_RANK && suit == UNKNOWN_SUIT;
}

bool operator==(const Card& lhs, const Card& rhs)
{
	return lhs.getRank() == rhs.getRank() && lhs.getSuit() == rhs.getSuit();
}

bool operator!=(const Card& lhs, const Card& rhs)
{
	return lhs.getRank() != rhs.getRank() || lhs.getSuit() != rhs.getSuit();
}

