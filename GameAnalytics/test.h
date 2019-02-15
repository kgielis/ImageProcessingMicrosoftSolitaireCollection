#pragma once

#include "globals.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "ExtractCards.h"
#include "classifyCard.h"

class Test
{
public:
	Test(ExtractCards ec, ClassifyCard cc);
	~Test();

	void doTest();	// test function for card extraction and classification
	bool writeTestData(const std::vector<std::vector<Card> > &classifiedBoards, const std::string & file);	// easily write new testdata to a textfile for later use
	bool readTestData(std::vector<std::vector<Card>> &classifiedBoards, const std::string &file);	// read in previously saved testdata from a textfile
private:
	ExtractCards ec;
	ClassifyCard cc;
};

