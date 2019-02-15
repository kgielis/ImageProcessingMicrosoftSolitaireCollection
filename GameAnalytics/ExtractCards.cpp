#include "stdafx.h"
#include "ExtractCards.h"

ExtractCards::ExtractCards()
{
}

ExtractCards::~ExtractCards()
{
}

/****************************************************
 *	INITIALIZATIONS
 ****************************************************/

void ExtractCards::determineROI(const cv::Mat & boardImage, int threshold)
{
	cv::Mat adaptedSrc, src, hsv, mask;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;

	src = boardImage.clone();
	cv::cvtColor(src, adaptedSrc, cv::COLOR_BGR2GRAY);	// convert the image to gray
	cv::threshold(adaptedSrc, adaptedSrc, threshold, 255, cv::THRESH_BINARY);	// threshold the image to keep only brighter regions (cards are white)										
	cv::findContours(adaptedSrc, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));	// find all the contours using the thresholded image

	auto new_end = std::remove_if(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1) {	// remove all small contours
		double area = contourArea(c1, false);	// make sure the contourArea is big enough to be a card
		cv::Rect bounding_rect = boundingRect(c1);
		float aspectRatio = (float)bounding_rect.width / (float)bounding_rect.height;	// make sure the contours have an aspect ratio of an average card (to filter out navbar)
		return ((aspectRatio < 0.1) || (aspectRatio > 10) || (area < 10000)); });
	contours.erase(new_end, contours.end());

	calculateOuterRect(contours, boardImage.rows);	// calculate the outer rectangle enclosing all cards + a margin at the bottom for stacked cards
}

void ExtractCards::calculateOuterRect(std::vector<std::vector<cv::Point>> &contours, int srcHeight)
{
	cv::Rect tempRect = boundingRect(contours.at(0));	// calculate the min and max area that encloses all cards
	int xmin = tempRect.x;
	int xmax = tempRect.x + tempRect.width;
	int ymin = tempRect.y;
	int ymax = tempRect.y + tempRect.height;
	topCardsHeight = ymin;
	for (int i = 1; i < contours.size(); i++)
	{
		tempRect = boundingRect(contours.at(i));
		if (xmin > tempRect.x) { xmin = tempRect.x; }
		if (xmax < tempRect.x + tempRect.width) { xmax = tempRect.x + tempRect.width; }
		if (ymin > tempRect.y)
		{
			ymin = tempRect.y;
			topCardsHeight = tempRect.height;
		}
	}
	ROI = cv::Rect(xmin - 10, ymin - 10, xmax - xmin + 30, srcHeight - ymin);	// add a small margin on each side and a large margin on the bottom for stacked cards later on
	topCardsHeight += 20;	// value used for the extraction of cardregions later on, namingly to split the top cards and bottom cards
}

/****************************************************
 *	MAIN FUNCTIONS
 ****************************************************/

std::vector<cv::Mat> ExtractCards::findCardsFromBoardImage(cv::Mat const & boardImage, SolitaireGame gameType)
{
	extractedCards.clear();
	cv::Mat adaptedSrc, src, hsv, mask, croppedSrc;
	try
	{
		src = boardImage.clone();
		croppedSrc = src(ROI);	// get the region of interest (area of cards) using the Rectangle calculated at the initialization
		if (gameType == KLONDIKE)
		{
			extractCardRegionsKlondike(croppedSrc);	// extract the regions of the cards
		}
		else if (gameType == FREECELL)
		{
			extractCardRegionsFreecell(croppedSrc);
		}
		extractCards();	// extract the topcard from the regions
	}
	catch (const std::exception&)	// if the board image wasn't captured correctly
	{		
		std::cerr << std::endl;
		std::cerr << "The image of the board wasn't captured correctly, trying again" << std::endl;
		std::cerr << std::endl;
	}
	return extractedCards;
}

std::vector<std::vector<cv::Mat>> ExtractCards::findAllCardsFromBoardImage(cv::Mat const & boardImage)
{
	cv::Mat adaptedSrc, src, hsv, mask, croppedSrc;
	try
	{
		src = boardImage.clone();
		croppedSrc = src(ROI);	// get the region of interest (area of cards) using the Rectangle calculated at the initialization
		
		extractCardRegionsFreecell(croppedSrc);
		extractAllCards();	// extract the topcard from the regions
	}
	catch (const std::exception&)	// if the board image wasn't captured correctly
	{
		std::cerr << std::endl;
		std::cerr << "The image of the board wasn't captured correctly, trying again" << std::endl;
		std::cerr << std::endl;
	}
	return allExtractedCards;
}

void ExtractCards::extractCardRegionsKlondike(const cv::Mat &src)
{
	cardRegions.clear();
	cv::Mat croppedtopCards = extractTopScreen(src);
	cv::Mat croppedbottomCards = extractBottomScreen(src);
	cv::Size topCardsSize = croppedtopCards.size();
	cv::Size bottomCardsSize = croppedbottomCards.size();

	for (int i = 0; i < 7; i++)	// build stack: split in 7 equidistance regions of cards
	{
		cv::Rect cardLocationRect = cv::Rect((int) bottomCardsSize.width / 7.1 * i, 0, (int) (bottomCardsSize.width / 7 - 1), bottomCardsSize.height);
		cv::Mat croppedCard(croppedbottomCards, cardLocationRect);
		cardRegions.push_back(croppedCard.clone());
	}

	cv::Rect talonCardsRect = cv::Rect(topCardsSize.width / 7, 0, topCardsSize.width / 5, topCardsSize.height);	
	// talon: slightly bigger width than other cards, take a bigger margin
	cv::Mat croppedCard(croppedtopCards, talonCardsRect);
	cardRegions.push_back(croppedCard.clone());

	for (int i = 3; i < 7; i++)	// suit stack: take the last 4 equidistance regions of cards
	{
		cv::Rect cardLocationRect = cv::Rect((int)(topCardsSize.width / 7.1 * i), 0, (int)(topCardsSize.width / 7 - 1), topCardsSize.height);
		cv::Mat croppedCard(croppedtopCards, cardLocationRect);
		cardRegions.push_back(croppedCard.clone());
	}
}

cv::Mat ExtractCards::extractTopScreen(const cv::Mat &src)
{
	// take the top of the screen until topCardsHeight (calculated in calculateOuterRect)
	cv::Mat croppedtopCards(src, cv::Rect(0, 0, (int)src.size().width, topCardsHeight));
	return croppedtopCards;
}

cv::Mat ExtractCards::extractBottomScreen(const cv::Mat & src)
{
	// take the rest of the image as bottomcards
	cv::Mat croppedbottomCards(src, cv::Rect(0, topCardsHeight, (int)src.size().width, (int)(src.size().height - topCardsHeight - 1)));	
	return croppedbottomCards;
}

void ExtractCards::extractCardRegionsFreecell(const cv::Mat & src)
{
	cardRegions.clear();
	cv::Size srcSize = src.size();
	// split the topcards (talon and suit stack) and the bottom cards (the build stack)
	cv::Mat croppedtopCards(src, cv::Rect(0, 0, srcSize.width, topCardsHeight));
	cv::Mat croppedbottomCards(src, cv::Rect(0, topCardsHeight, srcSize.width, (int)(srcSize.height - topCardsHeight - 1)));
	cv::Size topCardsSize = croppedtopCards.size();
	cv::Size bottomCardsSize = croppedbottomCards.size();

	int cardLocOffset = (int)round(0.035 * bottomCardsSize.width);
	int cardLocRemain = bottomCardsSize.width - (2 * cardLocOffset);
	for (int i = 0; i < 8; i++)	// build stack: split in 8 equidistance regions of cards
	{
		cv::Rect cardLocationRect = cv::Rect(((cardLocRemain / 8) * i) + cardLocOffset, 0, (cardLocRemain / 8), bottomCardsSize.height);
		cv::Mat croppedCard(croppedbottomCards, cardLocationRect);
		cardRegions.push_back(croppedCard.clone());
	}

	int cardLocSpaceHalf = topCardsSize.width / 2;
	for (int i = 0; i < 4; i++)	// 4 storage spaces
	{
		cv::Rect cardLocationRect2 = cv::Rect((int)((cardLocSpaceHalf / 4.3) * i), 0, cardLocSpaceHalf / 4.2, topCardsSize.height);
		cv::Mat croppedCard2(croppedtopCards, cardLocationRect2);
		cardRegions.push_back(croppedCard2.clone());
	}
	for (int i = 0; i < 4; i++)	// 4 suit spaces
	{
		cv::Rect cardLocationRect3 = cv::Rect((int)(((cardLocSpaceHalf / 4.3) * i) + cardLocSpaceHalf + (int)(cardLocSpaceHalf * 0.06)), 0, cardLocSpaceHalf / 4.2, topCardsSize.height);
		cv::Mat croppedCard3(croppedtopCards, cardLocationRect3);
		cardRegions.push_back(croppedCard3.clone());
	}
	assert(cardRegions.size() == 16);
}

std::vector<cv::Mat> ExtractCards::extractTalonCards(const cv::Mat &src)
{
	extractedCards.clear();
	cv::Mat graySrc, blurredSrc, threshSrc;
	std::vector<cv::Mat> talonCards;
	cv::cvtColor(cardRegions.at(7), graySrc, cv::COLOR_BGR2GRAY);
	cv::GaussianBlur(graySrc, blurredSrc, cv::Size(5, 5), 0);
	cv::threshold(blurredSrc, threshSrc, 220, 255, cv::THRESH_BINARY);

	auto contours = myOpencv::findContoursBySize(threshSrc);
	if (contours.size() > 0)
	{
		cv::Rect br = boundingRect(contours.front());
		if (br.height < 20 & br.width < 20) return extractedCards; //solve button
		cv::Mat card = cv::Mat(cardRegions[7], br);
		extractTalonCardsUsingSobel(card, talonCards);
	}
	return extractedCards;
}

void ExtractCards::extractCards()
{
	extractedCards.clear();
	extractedCards.resize(cardRegions.size());
	for (int i = 0; i < cardRegions.size(); i++)
	{
		cv::Mat graySrc, blurredSrc, threshSrc;
		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;

		cv::cvtColor(cardRegions.at(i), graySrc, cv::COLOR_BGR2GRAY);
		cv::GaussianBlur(graySrc, blurredSrc, cv::Size(5, 5), 0);
		cv::threshold(blurredSrc, threshSrc, 220, 255, cv::THRESH_BINARY);	// apply a string threshold to keep only white cards (no blue facedown cards)
																	    // by applying a strong threshold, the gray lines between stacked cards are also removed																
																		    // -> the topcard will be accessible by taking the largest area
		findContours(threshSrc, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));	// find the contours in the region

		if (contours.size() > 1)	// remove potential noise
		{
			// remove all contours that have an area smaller than 10000 pixels
			auto new_end = std::remove_if(contours.begin(), contours.end(), [](const std::vector<cv::Point> & c1)
			{ return (contourArea(c1, false) < 10000); });
			contours.erase(new_end, contours.end());
			// the biggest contour is the biggest card, place at the bottom
			std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2)
				-> bool { return contourArea(c1, false) > contourArea(c2, false); });
		}

		if (contours.size() > 0)	// if the size is zero, then there is no card at that cardlocation
		{
			cv::Rect br = boundingRect(contours.back());
			if (contours.size() == 2)
			{
				cv::Rect br2 = boundingRect(contours.at(0));
				if (br2.y > br.y) br = br2;
			}
			if (br.height < 20 && br.width < 20) {
				//not a card
				cv::Mat empty;
				extractedCards.at(i) = empty;
			}
			else
			{
				cv::Mat card = cv::Mat(cardRegions[i], br);
				cv::Mat croppedRef, resizedCardImage;
				//extractTopCardUsingSobel(card, croppedRef, i);	// if the strong threshold doesn't extract the card correctly, the card can be extracted using sobel edge detection
				extractTopCardUsingAspectRatio(card, croppedRef);
				/*Size cardSize = croppedRef.size();	// finally, if sobel edge doesn't extract the card correctly, try using hardcoded values (cardheight = 1.33 * cardwidth)
				if (cardSize.width * 1.3 > cardSize.height || cardSize.width * 1.4 < cardSize.height)
				{
				extractTopCardUsingAspectRatio(card, croppedRef);
				}*/
				croppedTopCardToStandardSize(croppedRef, resizedCardImage);	// resize the card to 150x200 for consistency
				extractedCards.at(i) = resizedCardImage.clone();
			}
		}
		else
		{
			cv::Mat empty;
			extractedCards.at(i) = empty;
		}
	}
}

void ExtractCards::extractAllCards()
{
	allExtractedCards.clear();
	allExtractedCards.resize(cardRegions.size());
	for (int i = 0; i < cardRegions.size(); i++)
	{
		cv::Mat graySrc, blurredSrc, threshSrc;
		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;

		cv::cvtColor(cardRegions.at(i), graySrc, cv::COLOR_BGR2GRAY);
		cv::GaussianBlur(graySrc, blurredSrc, cv::Size(5, 5), 0);
		cv::threshold(blurredSrc, threshSrc, 220, 255, cv::THRESH_BINARY);	// apply a string threshold to keep only white cards (no blue facedown cards)
																		// by applying a strong threshold, the gray lines between stacked cards are also removed																// -> the topcard will be accessible by taking the largest area
		findContours(threshSrc, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));	// find the contours in the region

		if (contours.size() > 1)	// remove potential noise
		{
			// remove all contours that have an area smaller than 10000 pixels
			auto new_end = std::remove_if(contours.begin(), contours.end(),
				[](const std::vector<cv::Point> & c1) { return (contourArea(c1, false) < 10000); });
			contours.erase(new_end, contours.end());
			// the biggest contour is the biggest card, place at the bottom
			std::sort(contours.begin(), contours.end(),
				[](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2) -> bool 
			{ return contourArea(c1, false) > contourArea(c2, false); });
		}

		if (contours.size() > 0)	// if the size is zero, then there is no card at that cardlocation
		{
			cv::Rect br = boundingRect(contours.at(0));
			cv::Mat card = cv::Mat(cardRegions[i], br);
			cv::Mat croppedRef, resizedCardImage;
			extractAllCardsUsingAspectRatio(card, allExtractedCards.at(i));
			//extractAllCardsUsingSobel(card, allExtractedCards.at(i));
		}
		else
		{
			cv::Mat empty;
			allExtractedCards.at(i).push_back(empty);
		}
	}
	assert(allExtractedCards.size() > 0);
}

/****************************************************
 *	PROCESSING OF STACK CARDS WITHIN extractCards()
 ****************************************************/

void ExtractCards::extractTalonCardsUsingSobel(const cv::Mat &src, std::vector<cv::Mat> dest)
{
	extractedCards.clear();
	cv::Mat gray, grad, abs_grad, thresh_grad;
	std::vector<cv::Point> lines;
	std::vector<cv::Vec4i> linesP;
	cv::Mat cdst = src.clone();
	/// Gradient X
	// calculate the vertical edges using Sobel edge detection
	cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
	cv::Sobel(gray, grad, CV_16S, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);
	cv::convertScaleAbs(grad, abs_grad);
	cv::threshold(abs_grad, thresh_grad, 70, 255, cv::THRESH_BINARY);
	// calculate lines of these edges that are at least 90% of the cardwidth 
	// and have maximum 10 pixels of not corresponding with the line
	HoughLinesP(thresh_grad, linesP, 1, CV_PI / 72, 30, src.size().height * 0.88, 15);	
	std::sort(linesP.begin(), linesP.end(), [](cv::Vec4i l1, cv::Vec4i l2) {return l1[0] > l2[0]; });
	for (size_t i = 0; i < linesP.size(); i++)
	{
		cv::Vec4i l = linesP.at(i);
		line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);
		//detect number of lines
		if (lines.size() > 0)
		{
			if (((abs(l[0] - l[2])) <= 2) && (abs(l[0] - lines.back().x) > 20))
			{
				lines.push_back(cv::Point(l[0], l[1]));
				line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);
			}
		}
		else 
		{
			if (((abs(l[0] - l[2])) <= 2) && (src.size().width - l[0] > (standardCardWidth * 0.7)) && l[0] > 20) 
			{
				lines.push_back(cv::Point(l[0], l[1]));
				line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);
			}
		}
	}

	if (lines.size() == 0)
	{
		extractedCards.push_back(src);
	}
	else if (lines.size() > 0)
	{
		int max_x = src.size().width;
		for (int i = 0; i < lines.size(); i++)
		{
			int min_x = lines.at(i).x;
			cv::Rect myROI(min_x, 0, max_x - min_x, src.size().height);
			max_x = min_x;
			cv::Mat card(src, myROI);
			cv::Mat cropped;
			if (card.size().width < 5)
			{
				std::cout << "error" << std::endl;
			}
			croppedTopCardToStandardSize(card, cropped);
			extractedCards.push_back(cropped);
		}
		cv::Rect myROI(0, 0, max_x, src.size().height);
		cv::Mat lastCard(src, myROI);
		cv::Mat cropped;
		if (lastCard.size().width < 5)
		{
			std::cout << "error" << std::endl;
		}
		croppedTopCardToStandardSize(lastCard, cropped);
		extractedCards.push_back(cropped);		
	}
}

void ExtractCards::extractTopCardUsingSobel(const cv::Mat &src, cv::Mat& dest, int i)
{
	cv::Mat gray, grad, abs_grad, thresh_grad;
	cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
	std::vector<cv::Vec4i> linesP;
	cv::Vec4i l;
	cv::Point lowest_pt1, lowest_pt2;
	cv::Point pt1, pt2;
	cv::Size cardSize = src.size();
	cv::Rect myROI;
	cv::Mat cdst = src.clone();

	if (i != 7)
	{
		/// Gradient Y
		cv::Sobel(gray, grad, CV_16S, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT);	// calculate the horizontal edges using Sobel edge detection
		cv::convertScaleAbs(grad, abs_grad);
		cv::threshold(abs_grad, thresh_grad, 80, 255, cv::THRESH_BINARY);
		lowest_pt1.y = 0;
		HoughLinesP(thresh_grad, linesP, 1, CV_PI / 72, 30, cardSize.width * 0.88, 15);	// calculate lines of these edges that are at least 90% of the cardwidth
																						//  and have maximum 10 pixels of not corresponding with the line
		for (size_t i = 0; i < linesP.size(); i++)
		{
			l = linesP[i];
			if (abs(l[1] - l[3]) <= 2 && l[1] > lowest_pt1.y && l[1] < cardSize.height * 0.9)	// the line needs to be perfectly horizontal, the lowest line and not at the bottom
			{
				lowest_pt1 = cv::Point(l[0], l[1]);
				lowest_pt2 = cv::Point(l[2], l[3]);
				line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);	// drawing the line for debugging purposes
			}

		}
		myROI.x = 0, myROI.width = cardSize.width;	// calculate the region of the card
		if (lowest_pt1.y > lowest_pt2.y)
		{
			myROI.y = lowest_pt1.y;
			myROI.height = cardSize.height - lowest_pt1.y;
		}
		else
		{
			myROI.y = lowest_pt2.y;
			myROI.height = cardSize.height - lowest_pt2.y;
		}

	}
	else
	{
		/// Gradient X
		// calculate the vertical edges using Sobel edge detection
		cv::Sobel(gray, grad, CV_16S, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);	
		cv::convertScaleAbs(grad, abs_grad);
		cv::threshold(abs_grad, thresh_grad, 80, 255, cv::THRESH_BINARY);
		lowest_pt1.y = 0;
		HoughLinesP(thresh_grad, linesP, 1, CV_PI / 72, 30, cardSize.height * 0.88, 15);	// calculate lines of these edges that are at least 90% of the cardwidth
																							//  and have maximum 10 pixels of not corresponding with the line
		for (size_t i = 0; i < linesP.size(); i++)
		{
			l = linesP.at(i);
			// the line needs to be perfectly horizontal, the lowest line and not at the right
			if (((abs(l[0] - l[2])) <= 2) && (l[0] > lowest_pt1.x) && l[0] < (cardSize.width * 0.9))	
			{
				lowest_pt1 = cv::Point(l[0], l[1]);
				lowest_pt2 = cv::Point(l[2], l[3]);
				// drawing the line for debugging purposes
				line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);	
			}
		}
		if (lowest_pt1.x > lowest_pt2.x)	// calculate the region of the card
		{
			myROI.x = lowest_pt1.x;
			myROI.width = cardSize.width - lowest_pt1.x;
		}
		else
		{
			myROI.x = lowest_pt2.x;
			myROI.width = cardSize.width - lowest_pt2.x;
		}
		myROI.y = 0, myROI.height = cardSize.height;
	}
	cv::Mat croppedRef(src, myROI);	// extract the card using the region
	dest = croppedRef.clone();
}

void ExtractCards::croppedTopCardToStandardSize(const cv::Mat &croppedRef, cv::Mat &resizedCardImage)
{
	int width = croppedRef.cols, height = croppedRef.rows;
	if (height < 0.5 * width) //not topcard
	{
		resizedCardImage = cv::Mat::zeros(60, standardCardWidth, croppedRef.type());	// initialize an empty image of size 150x200 (standard card size)
		float scale = ((float)60) / height;
		cv::Rect roi(0, 0, width * scale - 2, 60);
		cv::resize(croppedRef, resizedCardImage(roi), roi.size());
	}
	else 
	{
	resizedCardImage = cv::Mat::zeros(standardCardHeight, standardCardWidth, croppedRef.type());	// initialize an empty image of size 150x200 (standard card size)
	cv::Rect roi;
	float scale = ((float)standardCardHeight) / height;
	roi.y = 0;
	roi.height = standardCardHeight;	// take the maximum height
	roi.width = width * scale - 1;	// scale the width according to the height
	roi.x = 0;
	cv::resize(croppedRef, resizedCardImage(roi), roi.size());
	}
}

void ExtractCards::extractTopCardUsingAspectRatio(const cv::Mat & src, cv::Mat & dest)
{
	cv::Size cardSize = src.size();
	cv::Rect myROI;
	if (cardSize.width * 1.35 > cardSize.height)	// talon cards are wider than build stack cards
	{
		myROI.y = 0;
		myROI.height = cardSize.height;	// talon: height is ok
		myROI.x = cardSize.width - cardSize.height / 1.34 + 1;	// scale the width according to the height
		myROI.width = cardSize.width - myROI.x;
	}
	else	// build stack cards
	{
		myROI.x = 0;
		myROI.y = cardSize.height - cardSize.width * 1.32 - 1;	// scale the height according to the width and calculate from the bottom
		myROI.height = cardSize.height - myROI.y;	// take the height from the bottom
		myROI.width = cardSize.width;	// build stack: width is ok
	}

	cv::Mat croppedRef(src, myROI);
	dest = croppedRef.clone();
}

void ExtractCards::extractAllCardsUsingAspectRatio(const cv::Mat &src, std::vector<cv::Mat> &dest) 
{
	cv::Size cardSize = src.size();
	cv::Mat newSrc, src2;
	src2 = src;
	newSrc = src;
	while (cardSize.height > cardSize.width * 1.5) //cards above the top card (1.35 ratio + some bit)
	{
		cv::Rect myROI(0, 0, cardSize.width, cardSize.width * 0.4);
		//cv::rectangle(newSrc, myROI, cv::Scalar(0,255, 255), 3, 8, 0);
		cv::Rect myROIRemain(0, myROI.height, cardSize.width, cardSize.height - myROI.height);
		cv::Mat croppedRef1(src2, myROI);
		cv::Mat resizedCardImage1;
		// resize the card to 150x200 for consistency or topcards to 150*60
		croppedTopCardToStandardSize(croppedRef1, resizedCardImage1);	
		dest.push_back(resizedCardImage1.clone());
		src2 = src2(myROIRemain);
		cardSize = src2.size();
	}
	//no more cards above
	cv::Mat croppedRef(src2);
	cv::Mat resizedCardImage;
	croppedTopCardToStandardSize(croppedRef, resizedCardImage);
	dest.push_back(resizedCardImage.clone());
}

void ExtractCards::extractAllCardsUsingSobel(const cv::Mat &src, std::vector<cv::Mat> &dest)
{
	cv::Mat grad, abs_grad, thresh_grad, grayImg;
	cv::cvtColor(src, grayImg, cv::COLOR_BGR2GRAY);
	cv::Size cardSize = src.size();
	cv::Mat cdst = src.clone();
	std::vector<cv::Vec4i> linesP;
	std::vector<int> linesY;
	cv::Vec4i l;
	cv::Sobel(grayImg, grad, CV_16S, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT);	// calculate the horizontal edges using Sobel edge detection
	cv::convertScaleAbs(grad, abs_grad);
	cv::threshold(abs_grad, thresh_grad, 80, 255, cv::THRESH_BINARY);
	HoughLinesP(thresh_grad, linesP, 1, CV_PI / 60, 45, cardSize.width * 0.88, 15);
	std::sort(linesP.begin(), linesP.end(), [](cv::Vec4i v1, cv::Vec4i v2) { return v1[1] < v2[1]; });
	for (size_t i = 0; i < linesP.size(); i++)
	{
		l = linesP[i];
		if (abs(l[1] - l[3]) <= 2 && l[1] < cardSize.height * 0.9)	// the line needs to be horizontal and wide enough
		{
			if (linesY.empty()) linesY.push_back(l[1]);
			else if (l[1] - linesY.back() > 10) linesY.push_back(l[1]); //exclude double lines;
			line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);
			//line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);	// drawing the line for debugging purposes
		}
	}
	std::sort(linesY.begin(), linesY.end(), [](int l1, int l2) { return l1 < l2; });
	int prevY = 0;
	for (int i = 0; i < linesY.size(); i++)
	{
		if (i > 0) prevY = linesY.at(i - 1);
		cv::Rect ROI(0, prevY, cardSize.width, linesY.at(i) - prevY);
		cv::Mat card(src, ROI);
		dest.push_back(card.clone());
	}
	cv::Rect ROI(0, linesY.back(), cardSize.width, cardSize.height - linesY.back());
	cv::Mat card(src, ROI);
	dest.push_back(card.clone());
}

/****************************************************
 *	PROCESSING OF SELECTED CARDS BY THE PLAYER
 ****************************************************/
int ExtractCards::getIndexOfSelectedCard(int i) // use the cardlocation that has been clicked on (using coordinates)
{
	cv::Mat selectedCard = cardRegions.at(i).clone();
	cv::Mat hsv, mask;
	cv::cvtColor(selectedCard, hsv, cv::COLOR_BGR2HSV);	// convert the RBG spectrum (BGR for opencv) to HSV spectrum
	cv::Scalar lo_int(91, 95, 214);	// filter light blue (color of a selected card)
	cv::Scalar hi_int(96, 160, 255);
	inRange(hsv, lo_int, hi_int, mask);	// a mask of all the pixels that contain the light blue color, useful to see how many cards are selected
	cv::GaussianBlur(mask, mask, cv::Size(5, 5), 0);
	std::vector<std::vector<cv::Point>> selected_contours;
	std::vector<cv::Vec4i> selected_hierarchy;
	findContours(mask, selected_contours, selected_hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	if (selected_contours.size() > 1)	// remove potential noise
	{
		auto new_end = std::remove_if(selected_contours.begin(), selected_contours.end(), [](const std::vector<cv::Point> & c1) { return (contourArea(c1, false) < 1000); });
		selected_contours.erase(new_end, selected_contours.end());
	}
	if (selected_contours.size() > 1)	// multiple cards were selected: outeredge is one contours + inneredge for other contours
										// so if one card was selected (the topcard), we see 2 contours: the outer- and innercontour
										// if three cards were selected, we see 4 contours: 1 outercontour, 3 innercontours
	{
		int index = selected_contours.size() - 2;	// to get the index (from topcard to bottomcard, we need to substract 1 for the outercontour and again one for the index (size 1 = index 0)
		return index;
	}
	else
	{
		return -1;
	}
}

/*
*	returns a pair: clicked index (bottom == index 0) && number of cards with blue border
*/
std::pair<int, int> ExtractCards::findSelectedCards(POINT pt, int cardLocation)
{
	//take card stack
	if (cardLocation == -1) return std::pair<int, int>(-1, -1);
	cv::Mat selectedCardLocation = cardRegions.at(cardLocation).clone();
	
	//crop so no green
	cv::Mat hsv, mask;
	cv::cvtColor(selectedCardLocation, hsv, cv::COLOR_BGR2HSV);
	cv::Scalar lo_int(72, 150, 65);
	cv::Scalar hi_int(78, 255, 160);
	inRange(hsv, lo_int, hi_int, mask);
	cv::Mat invSrc = cv::Scalar::all(255) - mask;
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(invSrc);
	if (contours.size() == 0) return std::pair<int, int>(0, -1); //empty card stack
	cv::Rect cropped_br = boundingRect(contours.at(0));
	cv::Mat croppedImg(selectedCardLocation, cropped_br);

	//find click index through blue border
	cv::Mat grayImg, blurImg, binaryImg;
	cv::cvtColor(croppedImg, grayImg, cv::COLOR_BGR2GRAY);
	cv::GaussianBlur(grayImg, blurImg, cv::Size(5, 5), 0);
	cv::threshold(blurImg, binaryImg, 220, 255, cv::THRESH_BINARY);
	contours = myOpencv::findContoursBySize(binaryImg);
	contours = myOpencv::removeSmallerContours(contours, 0, 1000);
	myOpencv::orderContoursFromBottomToTop(&contours);

	if (contours.empty()) return std::pair<int, int>(0, -1);
	if (contours.size() > 1) //multiple blue borders detected
	{
		for (int i = 0; i < contours.size(); i++)
		{
			cv::Rect br = cv::boundingRect(contours.at(i));
			cv::Mat image(croppedImg, br);
			if (br.y < pt.y && pt.y < br.y + br.height)
			{
				std::cout << "blue border" << getIndexOfSelectedCard(cardLocation) << std::endl;
				return  std::pair<int, int>(i, getIndexOfSelectedCard(cardLocation) + 1);
			}
		}
	}
	if (contours.size() == 1) //one or none blue borders 
	{
		cv::Rect br = cv::boundingRect(contours.at(0));
		if (br.height > 130 && br.height < 240) return std::pair<int, int>(0, -1); //topcard with blue border 
		else if (br.height < 130) //smaller than one card -> unmovable card has been selected
		{
			return std::pair<int, int>(-98, -1);
		
		}
	}
	//find click index through edge detection (all cards needs to be visible, so no blue border and no unmovable selected cards
	cv::Rect br = cv::boundingRect(contours.at(0));
	cv::Mat cardStack(croppedImg, br);
	cv::Mat grad, abs_grad, thresh_grad;
	cv::cvtColor(cardStack, grayImg, cv::COLOR_BGR2GRAY);
	cv::Size cardSize = cardStack.size();
	cv::Mat cdst = cardStack.clone();
	std::vector<cv::Vec4i> linesP;
	std::vector<int> linesY;
	cv::Vec4i l;
	cv::Sobel(grayImg, grad, CV_16S, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT);	// calculate the horizontal edges using Sobel edge detection
	cv::convertScaleAbs(grad, abs_grad);
	cv::threshold(abs_grad, thresh_grad, 80, 255, cv::THRESH_BINARY);
	HoughLinesP(thresh_grad, linesP, 1, CV_PI / 60, 45, cardSize.width * 0.88, 15);
	std::sort(linesP.begin(), linesP.end(), [](cv::Vec4i v1, cv::Vec4i v2) { return v1[1] < v2[1]; });
	for (size_t i = 0; i < linesP.size(); i++)
	{
		l = linesP[i];
		if (abs(l[1] - l[3]) <= 2 && l[1] < cardSize.height * 0.9)	// the line needs to be horizontal and wide enough
		{
			if (linesY.empty()) linesY.push_back(l[1]);
			else if (l[1] - linesY.back() > 10) linesY.push_back(l[1]); //exclude double lines;
			line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);
			//line(cdst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 255), 3, CV_AA);	// drawing the line for debugging purposes
		}
	}
	std::sort(linesY.begin(), linesY.end(), [](int l1, int l2) { return l1 > l2; });
	for (int i = 0; i < linesY.size(); i++)
	{
		if (linesY.at(i) < pt.y) 
		{
			return  std::pair<int, int>(i, -1);
		}
	}
	return std::pair<int, int>(-1, -1);;
}

/****************************************************
 *	EXTRACT LOCATIONS OF CARD REGIONS
 ****************************************************/
std::vector<cv::Rect>  ExtractCards::extractCardRegionLocations(cv::Mat src)
{
	cv::Mat grayImg, binaryImg;

	cv::cvtColor(src, grayImg, cv::COLOR_BGR2GRAY);
	cv::GaussianBlur(grayImg, grayImg, cv::Size(5, 5), 0);
	cv::threshold(grayImg, binaryImg, 115, 255, cv::THRESH_BINARY);
	auto contours = myOpencv::findContoursBySize(binaryImg);
	contours = myOpencv::removeSmallerContours(contours, 0, 10000);
	myOpencv::orderContoursFromBottomToTop(&contours);
	cv::Rect bottom = cv::boundingRect(contours.at(0));
	myOpencv::orderContoursFromLeftToRight(&contours);

	std::vector<cv::Rect> cardStacks;
	std::vector<cv::Rect> topStacks;
	for each (std::vector<cv::Point> contour in contours)
	{
		cv::Rect br = cv::boundingRect(contour);
		if (br.y > bottom.y - 10) 
		{
			br.height = src.rows - br.y - 20;
			cv::Mat image(src, br);
			cardStacks.push_back(br);
		}
		else
		{
			cv::Mat image2(src, br);
			topStacks.push_back(br);
		}
	}
	cardStacks.insert(cardStacks.end(), topStacks.begin(), topStacks.end());

	return cardStacks;
}
