#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

Mat find_ramp(Mat source)
{
	int iLowH = 15;
	int iHighH = 80;

	int iLowS = 145;
	int iHighS = 200;

	int iLowV = 200;
	int iHighV = 255;

	Mat imgHSV;
	cvtColor(source, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	Mat imgThresholded;
	inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

	//morphological opening (remove small objects from the foreground)
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	//morphological closing (fill small holes in the foreground)
	dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	return imgThresholded;
}

void MyLine(Mat img, Point start, Point end)
{
	int thickness = 2;
	int lineType = 8;
	line(img,
		start,
		end,
		Scalar(0, 0, 0),
		thickness,
		lineType);
}

int main(int argc, char** argv)
{
	Mat image;
	image = imread("E:\\36.png", CV_LOAD_IMAGE_COLOR);


	Mat ramp = find_ramp(image);

	Moments m = moments(ramp, false);
	Point p1(m.m10 / m.m00, m.m01 / m.m00);
	circle(image, p1, 5, Scalar(128, 0, 0), -1);
	cout << Mat(p1) << endl;

	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(ramp.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	std::vector<cv::Point> approx;
	cv::approxPolyDP(
		cv::Mat(contours[0]),
		approx,
		cv::arcLength(cv::Mat(contours[0]), true) * 0.02,
		true
		);
	for (int i = 0; i < approx.size(); i++)
	{
		MyLine(image, approx[i % approx.size()], approx[(i + 1) % approx.size()]);
	}

	imshow("Thresholded Image", ramp); //show the thresholded image
	imshow("Final_image", image); //show the original image

	while (waitKey(500) == -1) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
	{
	}

	return 0;
}
