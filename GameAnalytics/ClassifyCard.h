#include "stdafx.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "Globals.h"

const int RESIZED_TYPE_WIDTH = 40;
const int RESIZED_TYPE_HEIGHT = 50;
const int MIN_CONTOUR_AREA = 80;

class ClassifyCard
{
public:
	ClassifyCard();
	~ClassifyCard();

	// INITIALIZATION
	void generateTrainingData(const cv::Mat & trainingImage, const std::string & outputPreName);	// generating knn data if it doesn't exist yet
	void generateImageVector();										// initialization of the image vector used for the subtraction method

	// MAIN FUNCTIONS
	std::vector<Card> classifyExtractedCards(std::vector<cv::Mat> extractedImages);
	std::vector<std::vector<Card>> classifyAllExtractedCards(std::vector<std::vector<cv::Mat>> extractedImages);
	std::pair<cv::Mat, cv::Mat> segmentRankAndSuitFromCard(const cv::Mat & aCard);
	Card classifyCard(std::pair<cv::Mat, cv::Mat> cardCharacteristics);		// main function for classification of the rank/suit images
	
	// CLASSIFICATION METHODS
	int classifyTypeUsingSubtraction(										// return: the index of image_list of the best match
		std::vector<std::pair<char, cv::Mat>> &image_list,			// input: list of known (already classified) images
		cv::Mat &resizedROI);

	char classifyTypeUsingKnn(										// return: the classified type
	const cv::Mat & image, const cv::Ptr<cv::ml::KNearest> & kNearest);		// input: the unknown image // input: the correct trained knn algorithm

							

private:
	// hardcoded values, but thanks to the resizing and consistent cardextraction, that is possible
	cv::Rect myRankROI = cv::Rect(4, 3, 22, 27);	
	cv::Rect mySuitROI = cv::Rect(4, 30, 22, 21);

	// images used for subtraction method
	std::vector<std::pair<char, cv::Mat>> rankImages;
	std::vector<std::pair<char, cv::Mat>> suitImages;

	// knn data
	cv::Ptr<cv::ml::KNearest>  kNearest_suit;
	cv::Ptr<cv::ml::KNearest>  kNearest_rank;
};

inline bool fileExists(const std::string& name) {
	std::ifstream f(name.c_str());
	return f.good();
}
