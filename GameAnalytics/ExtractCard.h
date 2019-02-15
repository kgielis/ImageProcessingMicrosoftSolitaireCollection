#pragma once

#include "stdafx.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include <opencv2/opencv.hpp>
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"

#include <vector>
#include <iostream>
#include <string>

#define standardBoardWidth 1600
#define standardBoardHeight 900
#define standardCardWidth 150
#define standardCardHeight 200

using namespace std;
using namespace cv;

class ExtractCard
{
public:
	ExtractCard();
	~ExtractCard();
	void findCardsFromBoardImage(Mat const & boardImage);
	void determineROI(const Mat & boardImage);
	void calculateOuterRect(std::vector<std::vector<cv::Point>> &contours);
	void resizeBoardImage(Mat const & boardImage, Mat & resizedBoardImage);
	void extractCardRegions(const cv::Mat & src);
	void extractCards();
	int getIndexOfSelectedCard(int i);
	void extractTopCardUsingSobel(const cv::Mat & src, cv::Mat & dest, int i);
	void croppedTopCardToStandardSize(const cv::Mat &croppedRef, cv::Mat &resizedCardImage);
	void extractTopCardUsingAspectRatio(const cv::Mat & src, cv::Mat & dest);
	bool checkForOutOfMovesState(const cv::Mat &src);
	const std::vector<cv::Mat> & getCards();

private:
	std::vector<cv::Mat> cards;
	std::vector<cv::Mat> cardRegions;
	Rect ROI;
	int topCardsHeight;
};
