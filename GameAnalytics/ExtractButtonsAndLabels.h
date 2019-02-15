#pragma once
#include "Globals.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "myOpencvFunctions.h"
#include "ClassifyCard.h"


class ExtractButtonsAndLabels
{
public:
	ExtractButtonsAndLabels();
	~ExtractButtonsAndLabels();

	std::vector<cv::Rect> extractEndGameButtons(cv::Mat src);
	std::vector<cv::Rect> extractGameOverButtons(cv::Mat src);
	std::vector<cv::Rect> extractDifficultyButtons(cv::Mat src);
	std::vector<cv::Rect> extractOutOfMovesButtons(cv::Mat src);

	std::vector<std::string> getTopBarData(const cv::Mat & boardImage);
	std::string extractSeed(cv::Mat boardImage, ClassifyCard cc);

private:
	cv::Rect findYellowButton(cv::Mat &hsv);
	std::vector<cv::Rect> findBlueButtons(cv::Mat &hsv);
	cv::Rect findYellowDificultyButton(cv::Mat &hsv);

	std::string compareImages(std::vector<std::string> fileNameVector, cv::Mat src);
};