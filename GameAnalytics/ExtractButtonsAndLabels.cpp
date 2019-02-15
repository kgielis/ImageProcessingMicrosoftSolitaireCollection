#include "stdafx.h"
#include "ExtractButtonsAndLabels.h"

ExtractButtonsAndLabels::ExtractButtonsAndLabels()
{
}

ExtractButtonsAndLabels::~ExtractButtonsAndLabels()
{
}

std::vector<cv::Rect> ExtractButtonsAndLabels::extractEndGameButtons(cv::Mat src)
{
	std::vector<cv::Rect> buttons;

	//convert to hsv
	cv::Mat hsv;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);

	//find yellow button (new Game)
	cv::Rect boundRectYellow = findYellowButton(hsv);
	buttons.push_back(boundRectYellow);

	//find blue button (main menu)
	std::vector<cv::Rect> blueButton = findBlueButtons(hsv);
	buttons.push_back(blueButton.at(0));

	return buttons;
}

std::vector<cv::Rect> ExtractButtonsAndLabels::extractGameOverButtons(cv::Mat src)
{
	std::vector<cv::Rect> buttons;
	//convert to hsv
	cv::Mat hsv;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);

	//find yellow button (new Game)
	cv::Rect yellowButton = findYellowButton(hsv);
	buttons.push_back(yellowButton);

	//find Blue buttons (Menu and again)
	std::vector<cv::Rect> blueButtons = findBlueButtons(hsv);
	buttons.push_back(blueButtons.at(0));
	buttons.push_back(blueButtons.at(1));

	return buttons;	
}

std::vector<cv::Rect> ExtractButtonsAndLabels::extractDifficultyButtons(cv::Mat src)
{
	//convert to hsv
	cv::Mat hsv;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);

	//find yellow button (new Game)
	std::vector<cv::Rect> buttons;
	cv::Rect boundRectYellow = findYellowDificultyButton(hsv);
	buttons.push_back(boundRectYellow);
	return buttons;
}

std::vector<cv::Rect> ExtractButtonsAndLabels::extractOutOfMovesButtons(cv::Mat src) 
{
	std::vector<cv::Rect> buttons;

	//convert to hsv
	cv::Mat hsv;
	cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);

	//find blue OK-button
	cv::Mat blueMask;
	cv::Scalar lo_int_b(107, 213, 104);
	cv::Scalar hi_int_b(107, 213, 104);
	inRange(hsv, lo_int_b, hi_int_b, blueMask);
	std::vector<std::vector<cv::Point>> contours_ok = myOpencv::findContoursBySize(blueMask);
	if (contours_ok.size() > 0) buttons.push_back(cv::boundingRect(contours_ok.at(0)));

	//find gray UNDO-button
	cv::Mat grayMask;
	cv::Scalar lo_int_g(0, 0, 204);
	cv::Scalar hi_int_g(0, 0, 204);
	inRange(hsv, lo_int_g, hi_int_g, grayMask);
	std::vector<std::vector<cv::Point>> contours_undo = myOpencv::findContoursBySize(grayMask);
	if (contours_undo.size() > 0) buttons.push_back(cv::boundingRect(contours_undo.at(0)));

	return buttons;
}

std::vector<std::string> ExtractButtonsAndLabels::getTopBarData(const cv::Mat & boardImage) 
{
	cv::Mat binaryImg, blobImg, grayImg;
	cv::Mat topbar(boardImage, cv::Rect(0, 0, boardImage.cols / 2, 64));
	cvtColor(topbar, grayImg, CV_BGR2GRAY);
	cv::threshold(grayImg, binaryImg, 150, 255, cv::THRESH_BINARY);

	cv::dilate(binaryImg, blobImg, cv::Mat(), cv::Point(-1, -1), 6, 0);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(blobImg);
	contours = myOpencv::removeSmallerContours(contours, 2, 3000);
	myOpencv::orderContoursFromLeftToRight(&contours);
	std::vector<std::string> topBarData;
	//Images containing the data
	
	cv::Rect gameRect = cv::boundingRect(contours.at(0));
	cv::Mat gameImg(binaryImg, gameRect);
	std::vector<std::string> gameTypes = { "Klondike", "Freecell" };
	std::string matchTextGame = compareImages(gameTypes, gameImg);
	std::cout << "You are playing: " << matchTextGame;
	topBarData.push_back(matchTextGame);

	if (contours.size() >= 2)
	{
		cv::Rect diffRect = cv::boundingRect(contours.at(1));
		cv::Mat diffImg(binaryImg, diffRect);
		std::vector<std::string> difficulties = { "Makkelijk", "Normaal", "Moeilijk", "Expert", "Meester", "Grootmeester", "Willekeurig" };
		std::string matchTextdiff = compareImages(difficulties, diffImg);
		std::cout << " on difficulty: " << matchTextdiff;
		topBarData.push_back(matchTextdiff);
	}
	std::cout << std::endl;
	return topBarData;
}

std::string ExtractButtonsAndLabels::extractSeed(cv::Mat boardImage, ClassifyCard cc) 
{
	cv::Mat binaryImg, grayImg, blobImg;
	cv::Mat topBar2ndPart(boardImage, cv::Rect(boardImage.cols / 2, 0, (boardImage.cols / 2) - 1, 64));
	cvtColor(topBar2ndPart, grayImg, CV_BGR2GRAY);
	cv::threshold(grayImg, binaryImg, 150, 255, cv::THRESH_BINARY);
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;

	//find seed in topbar
	cv::dilate(binaryImg, blobImg, cv::Mat(), cv::Point(-1, -1), 6, 0);
	cv::findContours(blobImg, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& lhs, const std::vector<cv::Point>& rhs)
	{ return cv::boundingRect(rhs).width < cv::boundingRect(lhs).width; });
	cv::Rect seedRect = cv::boundingRect(contours.at(0));
	cv::Mat seedImg(binaryImg, seedRect);

	//find individual numbers of seed
	cv::findContours(seedImg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	myOpencv::orderContoursFromLeftToRight(&contours);
	std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& lhs, const std::vector<cv::Point>& rhs)
	{ return cv::boundingRect(lhs).x < cv::boundingRect(rhs).x; });

	std::string seedText = "";
	for (int i = 1; i < contours.size(); i++) {
		cv::Rect numberRect = cv::boundingRect(contours.at(i));
		cv::Mat numberImg(seedImg, numberRect);
		cv::resize(numberImg, numberImg, cv::Size(40, 50));
		char seedChar = cc.classifyTypeUsingKnn(numberImg, cv::ml::KNearest::load<cv::ml::KNearest>("../GameAnalytics/knnData/trained_number.yml"));
		seedText.push_back(seedChar);
	}
	std::cout << "This is the seed: " << seedText << std::endl;
	return seedText;	
}

std::string ExtractButtonsAndLabels::compareImages(std::vector<std::string> fileNameVector, cv::Mat src) 
{
	float matchingFactor = 0; // The higher this number, the higher the matching pixels.
	std::string matchTextGame = "";
	for each (std::string fileName in fileNameVector)
	{
		cv::Size stdSize = cv::Size(150, 50);
		cv::Mat toCompare = cv::imread("../GameAnalytics/CompareImages/" + fileName + ".png"), result, resizedOriginal, graytoComp;
		cv::cvtColor(toCompare, toCompare, cv::COLOR_BGR2GRAY);
		cv::resize(toCompare, toCompare, stdSize);
		cv::resize(src, resizedOriginal, stdSize);
		cv::compare(resizedOriginal, toCompare, result, cv::CMP_EQ);
		//compare result
		int newfactor = countNonZero(result);
		if (newfactor > matchingFactor) {
			matchTextGame = fileName;
			matchingFactor = newfactor;
		}
	}
	return matchTextGame;
}

cv::Rect ExtractButtonsAndLabels::findYellowButton(cv::Mat &hsv)
{
	cv::Mat yellowMask;
	cv::Scalar lo_int_y(13, 236, 206);
	cv::Scalar hi_int_y(21, 255, 225);
	inRange(hsv, lo_int_y, hi_int_y, yellowMask);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(yellowMask);
	if (contours.size() > 0) return cv::boundingRect(contours.at(0));
	return cv::Rect();
}

cv::Rect ExtractButtonsAndLabels::findYellowDificultyButton(cv::Mat &hsv)
{
	cv::Mat yellowMask;
	cv::Scalar lo_int_y(15, 150, 214);
	cv::Scalar hi_int_y(19, 235, 247);
	inRange(hsv, lo_int_y, hi_int_y, yellowMask);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(yellowMask);
	if (contours.size() > 0) return cv::boundingRect(contours.at(0));
	return cv::Rect();
}

std::vector<cv::Rect> ExtractButtonsAndLabels::findBlueButtons(cv::Mat &hsv)
{
	std::vector<cv::Rect> blueButtons;
	cv::Mat blueMask;
	cv::Scalar lo_int(104, 200, 162);
	cv::Scalar hi_int(109, 220, 198);
	inRange(hsv, lo_int, hi_int, blueMask);
	std::vector<std::vector<cv::Point>> contours = myOpencv::findContoursBySize(blueMask);
	if (contours.size() > 0) blueButtons.push_back(cv::boundingRect(contours.at(0)));
	if (contours.size() > 1)
	{
		blueButtons.push_back(cv::boundingRect(contours.at(1)));
		//sort from left to right
		std::sort(blueButtons.begin(), blueButtons.end(), [](const cv::Rect& r1, const cv::Rect& r2)
			-> bool { return r1.x < r2.x; });
		
	}
	return blueButtons;
}
