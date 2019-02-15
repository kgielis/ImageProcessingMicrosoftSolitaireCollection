#include "stdafx.h"
#include "myOpencvFunctions.h"

//find contours ordened from large to small
std::vector<std::vector<cv::Point>> myOpencv::findContoursBySize(cv::Mat src)
{
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(src, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2)
		-> bool { return contourArea(c1, false) > contourArea(c2, false); });

	return contours;
}

std::vector<std::vector<cv::Point>> myOpencv::removeSmallerContours(std::vector<std::vector<cv::Point>> contours, int min_ratio, int min_area)
{
	auto new_end = std::remove_if(contours.begin(), contours.end(), 
		[&min_ratio, min_area](const std::vector<cv::Point>& c1) {	
		double area = contourArea(c1, false);
		cv::Rect bounding_rect = boundingRect(c1);
		float aspectRatio = (float)bounding_rect.width / (float)bounding_rect.height;
		return ((aspectRatio < min_ratio) || (area < min_area)); });
	contours.erase(new_end, contours.end());
	return contours;
}

void myOpencv::orderContoursFromLeftToRight(std::vector<std::vector<cv::Point>> *contours)
{
	std::sort(contours->begin(), contours->end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2) 
	{
		cv::Rect r1 = cv::boundingRect(c1);
		cv::Rect r2 = cv::boundingRect(c2);
		return r1.x < r2.x;
	});
}

void myOpencv::orderContoursFromBottomToTop(std::vector<std::vector<cv::Point>> *contours)
{
	std::sort(contours->begin(), contours->end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2)
	{
		cv::Rect r1 = cv::boundingRect(c1);
		cv::Rect r2 = cv::boundingRect(c2);
		return r1.y > r2.y;
	});
}
