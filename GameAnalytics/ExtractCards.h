#pragma once
#include "stdafx.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "Globals.h"

class ExtractCards
{
public:
	ExtractCards();
	~ExtractCards();

	// INITIALIZATION
	void determineROI(const cv::Mat & boardImage, int threshold);	// find the general location (roi) that contains all cards during the initialization
	void calculateOuterRect(std::vector<std::vector<cv::Point>> &contours, int sourceHeight);	// calculate the outer rect (roi) using the contours of all cards

	// MAIN FUNCTIONS
	std::vector<cv::Mat> findCardsFromBoardImage(cv::Mat const & boardImage, SolitaireGame gameType);	// main extraction function
	std::vector<std::vector<cv::Mat>> findAllCardsFromBoardImage(cv::Mat const & boardImage);
	cv::Mat extractTopScreen(const cv::Mat &src);
	cv::Mat extractBottomScreen(const cv::Mat &src);
	void extractCardRegionsFreecell(const cv::Mat & src);
	std::vector<cv::Mat> extractTalonCards(const cv::Mat & src);
	void extractCardRegionsKlondike(const cv::Mat & src);	// extract each region that contains a card
	void extractCards();	// extract the top card from a card region
	void extractAllCards();

	void extractTalonCardsUsingSobel(const cv::Mat & src, std::vector<cv::Mat> dest);

	// PROCESSING OF STACK CARDS WITHIN extractCards()
	void extractTopCardUsingSobel(const cv::Mat & src, cv::Mat & dest, int i);	// extract the top card from a stack of cards using Sobel edge detection
	void croppedTopCardToStandardSize(const cv::Mat &croppedRef, cv::Mat &resizedCardImage);	// resize the extracted top card to standardCardWidth/Height 
	void extractTopCardUsingAspectRatio(const cv::Mat & src, cv::Mat & dest);	// extract the top card from a stack of cards using the standard size of a card
	void extractAllCardsUsingAspectRatio(const cv::Mat &src, std::vector<cv::Mat> &dest);
	void extractAllCardsUsingSobel(const cv::Mat & src, std::vector<cv::Mat>& dest);
	// PROCESSING OF SELECTED CARDS BY THE PLAYER
		// find the index of the deepest card that was selected by the player (topcard = 0, below top card = 1, etc.)
	int getIndexOfSelectedCard(int i);
	std::pair<int, int> findSelectedCards(POINT pt, int cardLocation);
	std::vector<cv::Rect> extractCardRegionLocations(cv::Mat src);



private:
	std::vector<cv::Mat> extractedCards;	// extracted cards from card regions
	std::vector<std::vector<cv::Mat>> allExtractedCards;
	std::vector<cv::Mat> cardRegions;	// extracted card regions
	cv::Rect ROI;	// roi calculated by determineROI during the initialization that contains the general region of all cards
	int topCardsHeight;	// separation of the talon and suit stack from the build stack within the ROI
};
