#pragma once
#include "Globals.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgcodecs.hpp"

namespace myOpencv 
{
	std::vector<std::vector<cv::Point>> findContoursBySize(cv::Mat src);
	std::vector<std::vector<cv::Point>> removeSmallerContours(std::vector<std::vector<cv::Point>> contours, int min_ratio, int min_area);
	void orderContoursFromLeftToRight(std::vector<std::vector<cv::Point>>* contours);
	void orderContoursFromBottomToTop(std::vector<std::vector<cv::Point>> *contours);
}