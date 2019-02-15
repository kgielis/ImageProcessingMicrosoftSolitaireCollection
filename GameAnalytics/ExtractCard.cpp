#include "stdafx.h"
#include "ExtractCard.h"

ExtractCard::ExtractCard()
{
	cards.resize(12);
}

ExtractCard::~ExtractCard()
{
}

void ExtractCard::findCardsFromBoardImage(Mat const & boardImage)
{
	cv::Mat adaptedSrc, src, hsv, mask, croppedSrc;
	try
	{
		resizeBoardImage(boardImage, src);	// resize the board to 1600x900 for consistency
		croppedSrc = src(ROI);	// get the region of interest (area of cards) using the Rectangle calculated at the initialization
		extractCardRegions(croppedSrc);	// extract the regions of the cards
		extractCards();	// extract the topcard from the regions
	}
	catch (const std::exception&)	// if the board image wasn't captured correctly
	{
		cv::Mat test;
		cvtColor(boardImage, test, COLOR_BGR2GRAY);
		threshold(test, test, 0, 255, THRESH_BINARY);	// threshold the image to keep only brighter regions (cards are white)										
		std::cerr << "ERROR: Amount of not captured pixels in the image = " << cv::countNonZero(test) << std::endl;
	}
}

void ExtractCard::determineROI(const Mat & boardImage)
{
	cv::Mat adaptedSrc, src, hsv, mask;
	resizeBoardImage(boardImage, src);
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	cvtColor(src, adaptedSrc, COLOR_BGR2GRAY);	// convert the image to gray
	threshold(adaptedSrc, adaptedSrc, 120, 255, THRESH_BINARY);	// threshold the image to keep only brighter regions (cards are white)										
	findContours(adaptedSrc, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));	// find all the contours using the thresholded image

	auto new_end = std::remove_if(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1) {	// remove all small contours
		double area = contourArea(c1, false);	// make sure the contourArea is big enough to be a card
		Rect bounding_rect = boundingRect(c1);
		float aspectRatio = (float)bounding_rect.width / (float)bounding_rect.height;	// make sure the contours have an aspect ratio of an average card (to filter out navbar)
		return ((aspectRatio < 0.1) || (aspectRatio > 10) || (area < 10000)); });
	contours.erase(new_end, contours.end());

	calculateOuterRect(contours);	// calculate the outer rectangle enclosing all cards + a margin at the bottom for stacked cards
}

void ExtractCard::calculateOuterRect(std::vector<std::vector<cv::Point>> &contours)
{
	Rect tempRect = boundingRect(contours.at(0));	// calculate the min and max area that encloses all cards
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
	ROI = Rect(xmin - 10, ymin - 10, xmax - xmin + 30, standardBoardHeight - ymin);	// add a small margin on each side and a large margin on the bottom for stacked cards later on
	topCardsHeight += 20;	// value used for the extraction of cardregions later on, namingly to split the top cards and bottom cards
}

void ExtractCard::resizeBoardImage(Mat const & boardImage, Mat & resizedBoardImage)
{
	// certain resolutions return an image with black borders, we have to remove these for our processing
	Mat gray, thresh;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	cv::cvtColor(boardImage, gray, COLOR_BGR2GRAY);
	cv::threshold(gray, thresh, 1, 255, THRESH_BINARY);	// filter capturing every colour except pure black
	cv::findContours(thresh, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	Rect board = boundingRect(contours.at(0));
	Mat crop(boardImage, board);


	int width = crop.cols,
		height = crop.rows;
	

	cv::Mat targetImage = cv::Mat::zeros(standardBoardHeight, standardBoardWidth, crop.type());	// setup an empty image (zeros) using the standard height and width (1600x900)

	cv::Rect roi;
	// the width is always bigger than the height (the board always has 16:9 aspect ratio)
	float scale = ((float)standardBoardWidth) / width;
	roi.width = standardBoardWidth;	// take the maximum possible width
	roi.x = 0;
	roi.height = height * scale - 1;	// scale the height according to the aspect ratio with a maximum width
	roi.y = 0;	// start the image in the origin (0,0)

	cv::resize(crop, targetImage(roi), roi.size());
	resizedBoardImage = targetImage.clone();
}

void ExtractCard::extractCardRegions(const cv::Mat &src)
{
	cardRegions.clear();
	cv::Size srcSize = src.size();
	// split the topcards (talon and suit stack) and the bottom cards (the build stack)
	Mat croppedtopCards(src, Rect(0, 0, (int) srcSize.width, topCardsHeight));	// take the top of the screen until topCardsHeight (calculated in calculateOuterRect)
	Mat croppedbottomCards(src, Rect(0, topCardsHeight, (int)srcSize.width, (int) (srcSize.height - topCardsHeight - 1)));	// take the rest of the image as bottomcards
	Size topCardsSize = croppedtopCards.size();
	Size bottomCardsSize = croppedbottomCards.size();

	for (int i = 0; i < 7; i++)	// build stack: split in 7 equidistance regions of cards
	{
		Rect cardLocationRect = Rect((int) bottomCardsSize.width / 7.1 * i, 0, (int) (bottomCardsSize.width / 7 - 1), bottomCardsSize.height);
		Mat croppedCard(croppedbottomCards, cardLocationRect);
		cardRegions.push_back(croppedCard.clone());
	}

	Rect talonCardsRect = Rect(topCardsSize.width / 7, 0, topCardsSize.width / 5, topCardsSize.height);	// talon: slightly bigger width than other cards, take a bigger margin
	Mat croppedCard(croppedtopCards, talonCardsRect);
	cardRegions.push_back(croppedCard.clone());

	for (int i = 3; i < 7; i++)	// suit stack: take the last 4 equidistance regions of cards
	{
		Rect cardLocationRect = Rect((int)(topCardsSize.width / 7.1 * i), 0, (int)(topCardsSize.width / 7 - 1), topCardsSize.height);
		Mat croppedCard(croppedtopCards, cardLocationRect);
		cardRegions.push_back(croppedCard.clone());
	}
}

bool ExtractCard::checkForOutOfMovesState(const cv::Mat &src)
{
	// split the image in 3 horizontal parts and take the middle part
	// if the middle part is mostly white (at least 70%), then the board image shows an outOfMoves image (big horizontal white bar in the middle)
	Size imageSize = src.size();
	Rect middle = Rect(0, imageSize.height / 3, imageSize.width, imageSize.height / 3);
	Mat croppedSrc(src, middle);
	cvtColor(croppedSrc, croppedSrc, COLOR_BGR2GRAY);
	threshold(croppedSrc, croppedSrc, 240, 255, THRESH_BINARY);	// threshold the image to keep only brighter regions (cards are white)										
	if (cv::countNonZero(croppedSrc) > croppedSrc.rows * croppedSrc.cols * 0.7)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ExtractCard::extractCards()
{
	for (int i = 0; i < cardRegions.size(); i++)
	{		
		Mat adaptedSrc;
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;

		cv::cvtColor(cardRegions.at(i), adaptedSrc, COLOR_BGR2GRAY);
		cv::GaussianBlur(adaptedSrc, adaptedSrc, cv::Size(5, 5), 0);
		cv::threshold(adaptedSrc, adaptedSrc, 240, 255, THRESH_BINARY);	// apply a string threshold to keep only white cards (no blue facedown cards)
																		// by applying a strong threshold, the gray lines between stacked cards are also removed
																		// -> the topcard will be accessible by taking the largest area
		findContours(adaptedSrc, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));	// find the contours in the region

		if (contours.size() > 1)	// remove potential noise
		{
			// remove all contours that have an area smaller than 10000 pixels
			auto new_end = std::remove_if(contours.begin(), contours.end(), [](const std::vector<cv::Point> & c1) { return (contourArea(c1, false) < 10000); });
			contours.erase(new_end, contours.end());
			// the biggest contour is the biggest card, place at the bottom
			std::sort(contours.begin(), contours.end(), [](const vector<Point>& c1, const vector<Point>& c2) -> bool { return contourArea(c1, false) > contourArea(c2, false); });
		}
		
		if ( contours.size() > 0 )	// if the size is zero, then there is no card at that cardlocation
		{
			Rect br = boundingRect(contours.at(0));
			
			Mat card = Mat(cardRegions[i], br);		
			Mat croppedRef, resizedCardImage;
			//extractTopCardUsingSobel(card, croppedRef, i);	// if the strong threshold doesn't extract the card correctly, the card can be extracted using sobel edge detection

			/*Size cardSize = croppedRef.size();	// finally, if sobel edge doesn't extract the card correctly, try using hardcoded values (cardheight = 1.33 * cardwidth)
			if (cardSize.width * 1.3 > cardSize.height || cardSize.width * 1.4 < cardSize.height)
			{
				extractTopCardUsingAspectRatio(card, croppedRef);
			}*/
			croppedTopCardToStandardSize(card, resizedCardImage);	// resize the card to 150x200 for consistency
			cards.at(i) = resizedCardImage.clone();
		}
		else
		{
			Mat empty;
			cards.at(i) = empty;
		}

	}
}

int ExtractCard::getIndexOfSelectedCard(int i)	// use the cardlocation that has been clicked on (using coordinates)
{
	Mat selectedCard = cardRegions.at(i).clone();
	Mat hsv, mask;
	cv::cvtColor(selectedCard, hsv, COLOR_BGR2HSV);	// convert the RBG spectrum (BGR for opencv) to HSV spectrum
	Scalar lo_int(91, 95, 214);	// filter light blue (color of a selected card)
	Scalar hi_int(96, 160, 255);
	inRange(hsv, lo_int, hi_int, mask);	// a mask of all the pixels that contain the light blue color, useful to see how many cards are selected
	cv::GaussianBlur(mask, mask, cv::Size(5, 5), 0);
	vector<vector<Point>> selected_contours;
	vector<Vec4i> selected_hierarchy;
	findContours(mask, selected_contours, selected_hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE, Point(0, 0));
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

void ExtractCard::extractTopCardUsingSobel(const cv::Mat &src, cv::Mat& dest, int i)
{
	Mat gray, grad, abs_grad, thresh_grad;
	cv::cvtColor(src, gray, COLOR_BGR2GRAY);
	vector<Vec4i> linesP;
	Vec4i l;	
	cv::Point lowest_pt1, lowest_pt2;
	cv::Point pt1, pt2;
	float rho, theta;
	double a, b, x0, y0;
	Size cardSize = src.size();
	Rect myROI;
	Mat cdst = src.clone();

	if (i != 7)
	{
		/// Gradient Y
		cv::Sobel(gray, grad, CV_16S, 0, 1, 3, 1, 0, BORDER_DEFAULT);	// calculate the horizontal edges using Sobel edge detection
		cv::convertScaleAbs(grad, abs_grad);
		cv::threshold(abs_grad, thresh_grad, 90, 255, THRESH_BINARY);
		lowest_pt1.y = 0;
		HoughLinesP(thresh_grad, linesP, 1, CV_PI / 72, 30, cardSize.width * 0.88, 15);	// calculate lines of these edges that are at least 90% of the cardwidth
																						//  and have maximum 10 pixels of not corresponding with the line
		for (size_t i = 0; i < linesP.size(); i++)
		{
			l = linesP[i];
			if (abs(l[1] - l[3]) <= 2 && l[1] > lowest_pt1.y && l[1] < cardSize.height * 0.9)	// the line needs to be perfectly horizontal, the lowest line and not at the bottom
			{
				lowest_pt1 = Point(l[0], l[1]);
				lowest_pt2 = Point(l[2], l[3]);
				line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 255), 3, CV_AA);	// drawing the line for debugging purposes
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
		cv::Sobel(gray, grad, CV_16S, 1, 0, 3, 1, 0, BORDER_DEFAULT);	// calculate the vertical edges using Sobel edge detection
		cv::convertScaleAbs(grad, abs_grad);
		cv::threshold(abs_grad, thresh_grad, 70, 255, THRESH_BINARY);
		lowest_pt1.y = 0;
		HoughLinesP(thresh_grad, linesP, 1, CV_PI / 72, 30, cardSize.height * 0.88, 15);	// calculate lines of these edges that are at least 90% of the cardwidth
																							//  and have maximum 10 pixels of not corresponding with the line
		for (size_t i = 0; i < linesP.size(); i++)
		{
			l = linesP.at(i);
			if (((abs(l[0] - l[2])) <= 2) && (l[0] > lowest_pt1.x) && l[0] < (cardSize.width * 0.9))	// the line needs to be perfectly horizontal, the lowest line and not at the right
			{
				lowest_pt1 = Point(l[0], l[1]);
				lowest_pt2 = Point(l[2], l[3]);
				line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 255), 3, CV_AA);	// drawing the line for debugging purposes
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
	Mat croppedRef(src, myROI);	// extract the card using the region
	dest = croppedRef.clone();
}

void ExtractCard::croppedTopCardToStandardSize(const cv::Mat &croppedRef, cv::Mat &resizedCardImage)
{
	int width = croppedRef.cols,
		height = croppedRef.rows;
	resizedCardImage = cv::Mat::zeros(standardCardHeight, standardCardWidth, croppedRef.type());	// initialize an empty image of size 150x200 (standard card size)
	cv::Rect roi;
	float scale = ((float)standardCardHeight) / height;
	roi.y = 0;
	roi.height = standardCardHeight;	// take the maximum height
	roi.width = width * scale - 1;	// scale the width according to the height
	roi.x = 0;
	cv::resize(croppedRef, resizedCardImage(roi), roi.size());
}

void ExtractCard::extractTopCardUsingAspectRatio(const cv::Mat & src, cv::Mat & dest)
{
	Size cardSize = src.size();
	Rect myROI;
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

	Mat croppedRef(src, myROI);
	dest = croppedRef.clone();
}

const std::vector<cv::Mat> & ExtractCard::getCards()
{
	return cards;
}
