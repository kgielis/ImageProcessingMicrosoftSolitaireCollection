#pragma once
#include <map>
#include <assert.h>

enum Rank : char {
	ACE = 'A', TWO = '2', THREE = '3', FOUR = '4', FIVE = '5', SIX = '6', SEVEN = '7', EIGHT = '8', NINE = '9', TEN,
	JACK = 'J', QUEEN = 'Q', KING = 'K', EMPTY_RANK, UNKNOWN_RANK = '?'
};

enum Suit : char {
	CLUBS = 'C', SPADES = 'S', HEARTS = 'H', DIAMONDS = 'D', EMPTY_SUIT, UNKNOWN_SUIT = '?'
};

const std::map<Rank, int> rankValueMap{
{Rank::EMPTY_RANK, 0},
{Rank::ACE, 1},
{Rank::TWO, 2},
{Rank::THREE, 3},
{Rank::FOUR, 4},
{Rank::FIVE, 5},
{Rank::SIX, 6},
{Rank::SEVEN, 7},
{Rank::EIGHT, 8},
{Rank::NINE, 9},
{Rank::TEN, 10},
{Rank::JACK, 11},
{Rank::QUEEN, 12},
{Rank::KING, 13} };

class Card
{
public:
	Card();
	Card(Rank rank, Suit suit);
	Card(Rank r, Suit s, bool empty);
	~Card();

	Rank getRank() const;
	Suit getSuit() const;

	int getRankValue() const;

	Rank rank;
	Suit suit;
	bool isEmptyCard() const;
	void setEmptyCard(bool empty);
	bool isUnknownCard();

private:
	bool emptyCard = false;
};

bool operator==(const Card & lhs, const Card & rhs);
bool operator!=(const Card & lhs, const Card & rhs);
