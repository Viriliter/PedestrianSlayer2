#ifndef LANEDETECTOR_HPP
#define LANEDETECTOR_HPP

#include <math.h>
#include <vector>
#include <opencv2/opencv.hpp>

#include "utils/MathFun.hpp"
#include "utils/Timing.hpp"

using namespace cv;

// CAMERA SPECIFIC PARAMETERS
//Camera Calibration Parameters
const cv::Mat CAM_MAT = (Mat_<float>(3,3) << 1.15777930e+03, 0.00000000e+00, 6.67111054e+02, 0.00000000e+00, 1.15282291e+03, 3.86128937e+02, 0.00000000e+00, 0.00000000e+00, 1.00000000e+00);

const cv::Mat DIST_MAT = (Mat_<float>(1, 5) << -0.24688775, -0.02373133, -0.00109842,  0.00035108, -0.0025857);

//Perspective Transformation Parameters
const cv::Mat M = (Mat_<double>(3,3) <<  -6.83269851e-01, -1.49897451e+00, 1.06311163e+03, -1.20729385e-15, -1.98300615e+00, 9.02267800e+02, -1.15194662e-18, -2.40257838e-03, 1.00000000e+00);
//const cv::Mat M = (Mat_<double>(3,3) <<  0.2, -0.7111111111111111, 512, 0, -0.4333333333333333, 462, 0, -0.001111111111111111, 1);

const cv::Mat M_INV = (Mat_<double>(3, 3) <<  1.36363636e-01, -7.78812057e-01, 5.57727273e+02, -5.26327952e-17, -5.04284870e-01, 4.55000000e+02, 0.00000000e+00, -1.21158392e-03, 1.00000000e+00);
//const cv::Mat M_INV = (Mat_<double>(3, 3) <<  5, 8.888888888888895, -6666.666666666671, 0, 12.50000000000001, -5775.000000000003, 0, 0.0138888888888889, -5.41666666666667);

// Define conversions in x and y from pixels space to meters
const float YM_PER_PIX = 30.0/720; // meters per pixel in y dimension
const float XM_PER_PIX = 3.7/700; // meters per pixel in x dimension

const int N_WINDOWS = 9;  // Number of windows to find each line

const int Y_RANGE = 719;

const int MIN_PIXEL = 50;  // Number of minimum pixel that should be in the each window


struct BGR {
    uchar blue;
    uchar green;
    uchar red;  };

// VISUALIZATION PARAMETERS
// Note that OpenCV color space is BGR in default
const BGR LEFT_LANE_CLUSTER_COLOR{255, 0, 0};  // Blue
const BGR RIGHT_LANE_CLUSTER_COLOR{0, 0, 255};  // Red

const cv::Scalar LEFT_WINDOW_COLOR = cv::Scalar(0, 255, 0);  // Green
const cv::Scalar RIGHT_WINDOW_COLOR = cv::Scalar(0, 255, 255);  // Yellow

const cv::Scalar LEFT_LINE_COLOR = cv::Scalar(0, 0, 255);  // Red
const cv::Scalar RIGHT_LINE_COLOR = cv::Scalar(0, 0, 255);  // Red
const int LINE_THICKNESS = 5;

const cv::Scalar TEXT_COLOR = cv::Scalar(255, 255, 255);  // White

struct Lane{
std::vector<float> left_fit;
std::vector<float> right_fit;
std::vector<float> left_fit_m;
std::vector<float> right_fit_m;
std::vector<int> left_lane_inds;
std::vector<int> right_lane_inds;
cv::Mat out_img;
cv::Mat nonzerox;
cv::Mat nonzeroy;
};

class LaneDetector
{
private:
    const bool is_test=false;
    cv::VideoCapture capture;
     
    cv::Mat getFrame();
    void applyCameraCalibration(const cv::Mat &in_frame, cv::Mat &out_frame);
    void applyColorFilter(const cv::Mat &in_frame, cv::Mat &out_frame);
    cv::Mat splitColorChannel(const cv::Mat &in_frame, int channel_id);
    cv::Mat combineColorChannel(const cv::Mat &in_frame);
    void combineGradients(cv::Mat in_frame, cv::Mat &output);
    cv::Mat warpFrame(const cv::Mat &in_frame, cv::Mat &out_frame);
    std::pair<std::array<cv::Point, 4>, std::array<cv::Point, 4>>createTrapzoid(cv::Mat frame,
                                                                                    int bottomWidth,
                                                                                    int upperWidth,
                                                                                    int height);
    std::pair<cv::Mat, cv::Mat> calculatePerspectiveMatrix(const cv::Mat &in_frame);
    template<typename T>
    void calculateHistogram(cv::Mat &in_frame, std::vector<T> &histogram);

    Lane findLaneParameters(const cv::Mat &raw_frame);
    float calculateCurvature(std::vector<float> left_fit_cr);
    float calculateVehiclePosition(const cv::Mat &frame, Lane &lane);

    void showFrame(std::string title, const cv::Mat &frame);
    cv::Mat createHistogramChart(const std::vector<int> &histogram, int height);
    void drawLines(cv::Mat img, Lane &line, cv::Mat &out_img);
    void visualizeLanes(const cv::Mat &frame, cv::Mat &out_img);

    void findVehiclePosition(cv::Mat frame);

public:
    LaneDetector(bool is_test=false);

    void getLaneParams();
};

#endif  // LANEDETECTOR_HPP