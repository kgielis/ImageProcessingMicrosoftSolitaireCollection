#include "stdafx.h"
#include "test.h"

Test::Test(ExtractCards ec, ClassifyCard cc) : ec(ec), cc(cc)
{
}

Test::~Test()
{
}

void Test::doTest()
{
	// PREPARATION
	std::vector<cv::Mat> testImages;
	std::vector<std::vector<Card>> correctClassifiedOutputVector;

	for (int i = 0; i < 10; i++)	// read in all testimages and push them to the vector
	{
		std::stringstream ss;
		ss << i;
		cv::Mat src = cv::imread("../GameAnalytics/test/noMovesPlayed/" + ss.str() + ".png");
		if (!src.data)	// check for invalid input
		{
			std::cout << "Could not open or find testimage " << ss.str() << std::endl;
			exit(EXIT_FAILURE);
		}
		testImages.push_back(src.clone());
	}

	/*for (int i = 0; i < testImages.size(); i++)		// ---> used to save testdata of the classified images to a txt file
	{
	ec.findCardsFromBoardImage(testImages.at(i)); // -> average 38ms
	extractedImagesFromPlayingBoard = ec.getCards();
	classifyExtractedCards();	// -> average d133ms and 550ms
	correctClassifiedOutputVector.push_back(classifiedCardsFromPlayingBoard);
	}
	if (!writeTestData(correctClassifiedOutputVector, "../GameAnalytics/test/someMovesPlayed/correctClassifiedOutputVector.txt"))
	{
	std::cout << "Error writing testdata to txt file" << std::endl;
	exit(EXIT_FAILURE);
	}*/
	if (!readTestData(correctClassifiedOutputVector, "../GameAnalytics/test/noMovesPlayed/correctClassifiedOutputVector.txt"))	// read in the correct classified output
	{
		std::cout << "Error reading testdata from txt file" << std::endl;
		exit(EXIT_FAILURE);
	}


	// ACTUAL TESTING

	// 1. Test for card extraction

	/*int wrongExtraction = 0;
	std::chrono::time_point<std::chrono::steady_clock> test1 = Clock::now();
	for (int k = 0; k < 100; k++)	// repeat for k loops
	{
	for (int i = 0; i < testImages.size(); i++)	// repeat for all testimages
	{
	ec.findCardsFromBoardImage(testImages.at(i));
	extractedImagesFromPlayingBoard = ec.getCards();
	for (int j = 0; j < extractedImagesFromPlayingBoard.size(); j++)
	{
	cv::Mat test = extractedImagesFromPlayingBoard.at(j);
	Size cardSize = extractedImagesFromPlayingBoard.at(j).size();	// finally, if sobel edge doesn't extract the card correctly, try using hardcoded values (cardheight = 1.33 * cardwidth)
	if (cardSize.width * 1.3 > cardSize.height || cardSize.width * 1.4 < cardSize.height)
	{
	++wrongExtraction;
	}
	}
	}
	}
	std::chrono::time_point<std::chrono::steady_clock> test2 = Clock::now();	// test the duration of the classification of 10*100 loops
	std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(test2 - test1).count() << " ns" << std::endl;
	std::cout << "Wrong extraction counter: " << wrongExtraction << std::endl;
	Sleep(10000);*/


	// 1. Test for card classification

	int wrongRankCounter = 0;
	int wrongSuitCounter = 0;
	std::vector<std::vector<cv::Mat>> allExtractedImages;
	allExtractedImages.resize(testImages.size());
	for (int i = 0; i < testImages.size(); i++)	// first, get all cards extracted correctly
	{
		allExtractedImages.at(i) = ec.findCardsFromBoardImage(testImages.at(i), KLONDIKE);
	}
	std::pair<cv::Mat, cv::Mat> cardCharacteristics;
	Card cardType;

	std::chrono::time_point<std::chrono::steady_clock> test1 = Clock::now();
	for (int k = 0; k < 100; k++)	// repeat for k loops
	{
		for (int i = 0; i < allExtractedImages.size(); i++)
		{
			std::vector<Card> classifiedCardsFromPlayingBoard;
			classifiedCardsFromPlayingBoard.clear();	// reset the variable
			for_each(allExtractedImages.at(i).begin(), allExtractedImages.at(i).end(), [this, &cardCharacteristics, &cardType, &classifiedCardsFromPlayingBoard](cv::Mat mat) {
				if (mat.empty())	// extracted card was an empty image -> no card on this location
				{
					cardType.setEmptyCard(true);
				}
				else	// segment the rank and suit + classify this rank and suit
				{
					cardCharacteristics = cc.segmentRankAndSuitFromCard(mat);
					cardType = cc.classifyCard(cardCharacteristics);
				}
				classifiedCardsFromPlayingBoard.push_back(cardType);	// push the classified card to the variable
			});
			for (int j = 0; j < classifiedCardsFromPlayingBoard.size(); j++)
			{
				if (correctClassifiedOutputVector.at(i).at(j).rank != classifiedCardsFromPlayingBoard.at(j).rank)	// compare the classified output with the correct classified output
				{
					++wrongRankCounter;
				}
				if (correctClassifiedOutputVector.at(i).at(j).suit != classifiedCardsFromPlayingBoard.at(j).suit)
				{
					++wrongSuitCounter;
				}
			}
		}
	}

	std::chrono::time_point<std::chrono::steady_clock> test2 = Clock::now();	// test the duration of the classification of 10*100 loops
	std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::milliseconds>(test2 - test1).count() << " ms" << std::endl;
	std::cout << "Rank error counter: " << wrongRankCounter << std::endl;	// print the amount of faulty classifications
	std::cout << "Suit error counter: " << wrongSuitCounter << std::endl;
	Sleep(10000);
}

bool Test::writeTestData(const std::vector<std::vector<Card>>& classifiedBoards, const std::string & file)
{
	if (classifiedBoards.empty())
		return false;

	if (file != "")
	{
		std::stringstream ss;
		for (int k = 0; k < classifiedBoards.size(); k++)	// push each classified card as rank + space + suit, each card on seperate lines, to a stringstream
		{
			for (int i = 0; i < classifiedBoards.at(k).size(); i++)
			{
				ss << static_cast<char>(classifiedBoards.at(k).at(i).rank) << " " << static_cast<char>(classifiedBoards.at(k).at(i).suit) << "\n";	// cast the classified rank/suit to a char for readability
			}
			ss << "\n";	// leave a whitespace for a new classified board
		}

		std::ofstream out(file.c_str());
		if (out.fail())
		{
			out.close();
			return false;
		}
		out << ss.str();	// push the stringstream of all classified cards to the file
		out.close();
	}
	return true;
}

bool Test::readTestData(std::vector<std::vector<Card>>& classifiedBoards, const std::string & file)
{
	if (file != "")
	{
		std::stringstream ss;
		std::ifstream in(file.c_str());
		if (in.fail())
		{
			in.close();
			return false;
		}
		std::string str;
		classifiedBoards.resize(10);
		for (int i = 0; i < classifiedBoards.size(); i++)	// prepare the classifiedBoards vector
		{
			classifiedBoards.at(i).resize(12);
		}
		for (int k = 0; k < 10; k++)
		{
			for (int i = 0; i < 12; i++)
			{
				std::getline(in, str);	// read line by line
				std::istringstream iss(str);	// separates each word in the string
				std::string subs, subs2;
				iss >> subs;	// push the rank to a temporary string
				iss >> subs2;	// push the suit to a temporary string
				classifiedBoards.at(k).at(i).rank = static_cast<Rank>(subs[0]);	// cast the chars to classifiers and push them to the classifiedBoards vector
				classifiedBoards.at(k).at(i).suit = static_cast<Suit>(subs2[0]);
			}
			std::getline(in, str);	// at the end of a board, bump one extra line for a new board
		}
		in.close();
	}
	return true;
}
