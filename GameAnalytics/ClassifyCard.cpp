#include "stdafx.h"
#include "ClassifyCard.h"

ClassifyCard::ClassifyCard()
{
	generateImageVector();	// initialize the images used for classification using comparison
	std::vector<std::string> type = { "rank", "suit" };
	for (int i = 0; i < type.size(); i++)
	{
		std::string name = "../GameAnalytics/knnData/trained_" + type.at(i) + ".yml";	// get the trained data from the knn classifier
		if (!fileExists(name))
		{
			cv::Mat trainingImg = cv::imread("../GameAnalytics/knnData/" + type.at(i) + "TrainingImg.png");
			generateTrainingData(trainingImg, type.at(i));
		}
	}
	kNearest_suit = cv::ml::KNearest::load<cv::ml::KNearest>("../GameAnalytics/knnData/trained_suit.yml");	// load frequently used knn algorithms as member variables
	kNearest_rank = cv::ml::KNearest::load<cv::ml::KNearest>("../GameAnalytics/knnData/trained_rank.yml");
}

ClassifyCard::~ClassifyCard()
{
}

/****************************************************
 *	INITIALIZATIONS
 ****************************************************/

std::vector<Card> ClassifyCard::classifyExtractedCards(std::vector<cv::Mat> extractedImages)
{
	Card card;
	std::pair<cv::Mat, cv::Mat> cardCharacteristics;
	std::vector<Card> classifiedCardsFromPlayingBoard;
	classifiedCardsFromPlayingBoard.reserve(12);
	for_each(extractedImages.begin(), extractedImages.end(),
		[this, &card, &cardCharacteristics, &classifiedCardsFromPlayingBoard](cv::Mat mat) {
		if (mat.empty())	// extracted card was an empty image -> no card on this location
		{
			card.setEmptyCard(true);
		}
		else	// segment the rank and suit + classify this rank and suit
		{
			cardCharacteristics = segmentRankAndSuitFromCard(mat);
			card = classifyCard(cardCharacteristics);
		}
		classifiedCardsFromPlayingBoard.push_back(card);	// push the classified card to the variable
	});
	return classifiedCardsFromPlayingBoard;
}

std::vector<std::vector<Card>> ClassifyCard::classifyAllExtractedCards(std::vector<std::vector<cv::Mat>> extractedImages) 
{
	Card card;
	std::pair<cv::Mat, cv::Mat> cardCharacteristics;
	std::vector<std::vector<Card>> allBoardCards;
	allBoardCards.resize(16, std::vector<Card>());//Every stack
	for (int i = 0; i < extractedImages.size(); i++) {
		for (int j = 0; j < extractedImages.at(i).size(); j++) {
			if (extractedImages.at(i).at(j).empty())	// extracted card was an empty image -> no card on this location
			{
				card.setEmptyCard(true);
			}
			else	// segment the rank and suit + classify this rank and suit
			{
				cardCharacteristics = segmentRankAndSuitFromCard(extractedImages.at(i).at(j));
				card = classifyCard(cardCharacteristics);
			}
			allBoardCards.at(i).push_back(card);
		}
	}
	return allBoardCards;
}

void ClassifyCard::generateTrainingData(const cv::Mat & trainingImage, const std::string & outputPreName) {

	// initialize variables
	cv::Mat grayImg, threshImg;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::Mat classificationInts, trainingImagesAsFlattenedFloats;
	std::vector<int> intValidChars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'J', 'Q', 'K', 'A',	// ranks
		'S', 'C', 'H', 'D' };	// suits (spades, clubs, hearts, diamonds)
	cv::Mat trainingNumbersImg = trainingImage;

	// change the training image to black/white and find the contours of characters
	cv::cvtColor(trainingNumbersImg, grayImg, CV_BGR2GRAY);
	cv::threshold(grayImg, threshImg, 130, 255, cv::THRESH_BINARY_INV);
	cv::findContours(threshImg, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);	// get each rank/suit from the image, one by one

	// show each character from the training image and let the user input which character it is
	for (int i = 0; i < contours.size(); i++)
	{
		if (contourArea(contours.at(i)) > 60)
		{
			cv::Rect boundingRect = cv::boundingRect(contours[i]);
			cv::rectangle(trainingNumbersImg, boundingRect, cv::Scalar(0, 0, 255), 2);

			cv::Mat ROI = threshImg(boundingRect);	// process each segmented rank/suit before saving it and asking for user input
			cv::Mat ROIResized;
			if (outputPreName == "suit")
			{
				cv::resize(ROI, ROIResized, cv::Size(RESIZED_TYPE_HEIGHT, RESIZED_TYPE_HEIGHT));
			}
			else
			{
				cv::resize(ROI, ROIResized, cv::Size(RESIZED_TYPE_WIDTH, RESIZED_TYPE_HEIGHT));
			}
			imshow("ROIResized", ROIResized);
			imshow("TrainingsNumbers", trainingNumbersImg);

			int intChar = cv::waitKey(0);	// get user input
			if (intChar == 27)	// if esc key was pressed
			{
				return;
			}
			else if (find(intValidChars.begin(), intValidChars.end(), intChar) != intValidChars.end())	// check if the user input is valid
			{	
				classificationInts.push_back(intChar);	// push the classified rank/suit to the classification list
				cv::Mat imageFloat;
				ROIResized.convertTo(imageFloat, CV_32FC1);	// convert the image to a binary float image
				cv::Mat imageFlattenedFloat = imageFloat.reshape(1, 1);	// reshape the image to one long line (row after row)
				trainingImagesAsFlattenedFloats.push_back(imageFlattenedFloat);	// push the resulting image to the image list
																				// knn requires flattened float images
			}
		}
	}

	std::cout << "training complete" << std::endl;

	cv::Ptr<cv::ml::KNearest> kNearest(cv::ml::KNearest::create());
	kNearest->train(trainingImagesAsFlattenedFloats, cv::ml::ROW_SAMPLE, classificationInts);	// train the knn classifier
	kNearest->save("../GameAnalytics/knnData/trained_" + outputPreName + ".yml");	// store the trained classifier in a YML file for future use
}

void ClassifyCard::generateImageVector()
{
	std::vector<std::string> suitClassifiersList = { "S", "C", "D", "H" };
	std::vector<std::string> rankClassifiersList = { "2", "3", "4", "5", "6", "7", "8", "9", "J", "Q", "K", "A" };
	std::vector<std::string> black_suitClassifiersList = { "S", "C" };
	std::vector<std::string> red_suitClassifiersList = { "D", "H" };

	for (int i = 0; i < rankClassifiersList.size(); i++)	// get the images from the map to use them for classification using comparison
	{
		cv::Mat src = cv::imread("../GameAnalytics/compareImages/" + rankClassifiersList.at(i) + ".png");
		if (!src.data)	// check for invalid input
		{
			std::cerr << "Could not open or find the image" << std::endl;
			exit(EXIT_FAILURE);
		}
		cv::Mat grayImg, threshImg;	// identical processing as the segmented images for improved robustness
		cv::cvtColor(src, grayImg, cv::COLOR_BGR2GRAY);
		cv::threshold(grayImg, threshImg, 140, 255, cv::THRESH_BINARY);
		std::pair<char, cv::Mat> pair;
		pair.first = char(rankClassifiersList.at(i).at(0));
		pair.second = threshImg.clone();
		rankImages.push_back(pair);
	}
	for (int i = 0; i < suitClassifiersList.size(); i++)	// get the images from the map to use them for classification using comparison
	{
		cv::Mat src = cv::imread("../GameAnalytics/compareImages/" + suitClassifiersList.at(i) + ".png");
		if (!src.data)	// check for invalid input
		{
			std::cerr << "Could not open or find the image" << std::endl;
			exit(EXIT_FAILURE);
		}
		cv::Mat grayImg, threshImg;	// identical processing as the segmented images for improved robustness
		cv::cvtColor(src, grayImg, cv::COLOR_BGR2GRAY);
		cv::threshold(grayImg, threshImg, 140, 255, cv::THRESH_BINARY);
		std::pair<char, cv::Mat> pair;
		pair.first = char(suitClassifiersList.at(i).at(0));
		pair.second = threshImg.clone();
		suitImages.push_back(pair);
	}
}

/****************************************************
 *	MAIN FUNCTIONS
 ****************************************************/

std::pair<cv::Mat, cv::Mat> ClassifyCard::segmentRankAndSuitFromCard(const cv::Mat & aCard)
{
	// Get the rank and suit from the resized card
	cv::Mat grayImg, blurredImg, threshImg, blobImg;
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::Rect rankAndSuitROI(0, 2, 28, 55);
	cv::Mat rankAndSuit(aCard, rankAndSuitROI);

	cv::cvtColor(rankAndSuit, grayImg, cv::COLOR_BGR2GRAY);
	cv::GaussianBlur(grayImg, blurredImg, cv::Size(5, 5), 0);
	cv::threshold(blurredImg, threshImg, 150, 255, cv::THRESH_BINARY_INV);
	cv::findContours(threshImg, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	if (contours.size() == 3)
	{
		std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2)
			-> bool { return cv::contourArea(c1, false) > contourArea(c2, false); });
		
		// bounding rect of smallest contour
		for (int i = 0; i < contours.size(); i++) 
		{
			cv::Rect boundRect = cv::boundingRect(contours.at(i));
			if (boundRect.y + boundRect.height == rankAndSuit.size().height) //intersects with bottom border of image
			{
				contours.erase(contours.begin() + i);
			}
		}
	}
	//if (contours.size() > 2) contours = myOpencv::removeSmallerContours(contours, 0, 50);
	
	int lowestContourY = 0;
	for (int i = 0; i < contours.size(); i++) 
	{
		cv::Rect boundRect = cv::boundingRect(contours.at(i));
		//cv::rectangle(rankAndSuit, boundRect, cv::Scalar(0, 255, 255), 1, 8, 0);
		if (boundRect.y > lowestContourY) lowestContourY = boundRect.y;
	}

	cv::Rect rankROI(0, 0, rankAndSuit.size().width, abs(lowestContourY - 1));
	cv::Rect suitROI(0, lowestContourY, rankAndSuit.size().width, rankAndSuit.size().height - lowestContourY);

	cv::Mat rank(aCard, rankROI);
	cv::resize(rank, rank, cv::Size(RESIZED_TYPE_HEIGHT, RESIZED_TYPE_HEIGHT));	// resize a first time to increase pixeldensity
	cv::Mat suit(aCard, suitROI);
	cv::resize(suit, suit, cv::Size(RESIZED_TYPE_HEIGHT, RESIZED_TYPE_HEIGHT));	// resize a first time to increase pixeldensity
	std::pair<cv::Mat, cv::Mat> cardCharacteristics = std::make_pair(rank, suit); // package as a pair of rank and suit for classification
	return cardCharacteristics;
}

Card ClassifyCard::classifyCard(std::pair<cv::Mat, cv::Mat> cardCharacteristics)
{
	// initialize variables
	Card card;
	cv::Mat src, blurredImg, grayImg, threshImg, resizedBlurredImg, resizedThreshImg;
	std::string type = "rank";	// first classify the rank
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	src = cardCharacteristics.first;

	for (int i = 0; i < 2; i++)
	{		
		// process the src
		cv::cvtColor(src, grayImg, cv::COLOR_BGR2GRAY);
		//cv::GaussianBlur(grayImg, blurredImg, cv::Size(5, 5), 0);
		cv::threshold(grayImg, threshImg, 200, 255, cv::THRESH_BINARY_INV); //150 //145
		cv::findContours(threshImg, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		if (contours.empty())	// no contour visible, so there is no image - extra verification to avoid potentially missed errors
		{
			card.setEmptyCard(true);
			return card;
		}

		// sort the contours on contourArea
		std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2)
			-> bool { return cv::contourArea(c1, false) > contourArea(c2, false); });

		if (type == "rank" && contours.size() > 1 && contourArea(contours.at(1), false) > 250.0)	// multiple contours and the second contour isn't small (noise)
		{
			card.rank = TEN;
		}
		else
		{
			cv::Mat ROI, resizedROI;
			ROI = threshImg(boundingRect(contours.at(0))); // extract the biggest contour from the image
			(type == "rank") ?	// resize to standard size (necessary for classification)
				cv::resize(ROI, resizedROI, cv::Size(RESIZED_TYPE_WIDTH, RESIZED_TYPE_HEIGHT)) :	// numbers are naturally smaller in width
				cv::resize(ROI, resizedROI, cv::Size(RESIZED_TYPE_HEIGHT, RESIZED_TYPE_HEIGHT));	// suits are rather squared, keep this characteristic
			cv::GaussianBlur(resizedROI, resizedBlurredImg, cv::Size(3, 3), 0);	// used for shape analysis
			cv::threshold(resizedBlurredImg, resizedThreshImg, 140, 255, cv::THRESH_BINARY);


			// COMPARISON METHOD
			/*(type == "rank") ?
				cardType.rank = Rank(rankImages.at(classifyTypeUsingSubtraction(rankImages, resizedThreshImg)).first) :
				cardType.suit = Suit(suitImages.at(classifyTypeUsingSubtraction(suitImages, resizedThreshImg)).first;*/

			// KNN METHOD
			
			(type == "rank") ?
				card.rank = Rank(classifyTypeUsingKnn(resizedROI, kNearest_rank)):
				card.suit = Suit(classifyTypeUsingKnn(resizedROI, kNearest_suit));

		}
		type = "suit";	// next classify the suit, start from a black_suit, if it's red, this will get detected at the beginning of the second loop
		src = cardCharacteristics.second;
	}
	return card;
}

/****************************************************
 *	CLASSIFICATION METHODS
 ****************************************************/

int ClassifyCard::classifyTypeUsingSubtraction(std::vector<std::pair<char, cv::Mat>> &image_list, cv::Mat &resizedROI)
{
	cv::Mat diff;
	int lowestValue = INT_MAX;
	int lowestIndex = INT_MAX;
	int nonZero;
	for (int i = 0; i < image_list.size(); i++)	// absolute comparison between 2 binary images
	{
		cv::compare(image_list.at(i).second, resizedROI, diff, cv::CMP_NE);
		nonZero = cv::countNonZero(diff);
		if (nonZero < lowestValue)	// the lower the amount of nonzero pixels (parts that don't match), the better the classification
		{
			lowestIndex = i;
			lowestValue = nonZero;
		}
	}
	return lowestIndex;
}

char ClassifyCard::classifyTypeUsingKnn(const cv::Mat & image, const cv::Ptr<cv::ml::KNearest> & kNearest)
{
	cv::Mat ROIFloat, ROIFlattenedFloat;
	image.convertTo(ROIFloat, CV_32FC1);	// converts 8 bit int gray image to binary float image
	ROIFlattenedFloat = ROIFloat.reshape(1, 1);	// reshape the image to 1 line (all rows pasted behind each other)
	cv::Mat CurrentChar(0, 0, CV_32F);	// output array char that corresponds to the best match (nearest neighbor)
	kNearest->findNearest(ROIFlattenedFloat, 3, CurrentChar);	// calculate the best match
	return char(CurrentChar.at<float>(0, 0));	// convert the float to a char, and finally to classifiers to find the closest match
}
