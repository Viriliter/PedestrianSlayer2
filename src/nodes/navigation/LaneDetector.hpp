#ifndef LANEDETECTOR_HPP
#define LANEDETECTOR_HPP

#include <math.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include "../../utils/math_fun.h"

       
// Define conversions in x and y from pixels space to meters
float const YM_PER_PIX = 30.0/720; // meters per pixel in y dimension
float const XM_PER_PIX = 3.7/700; // meters per pixel in x dimension

struct Window{
    std::vector<float> left_fit;
    std::vector<float> right_fit;
    float left_curverad;
    float right_curverad;
    cv::Mat out_img;
};

struct Line{
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
    bool is_test=false;
    cv::VideoCapture capture;
    int prevErrorCnt;
    int errorCnt;
    Line left_line;
    Line right_line;
        
    cv::Mat get_frame();
    void set_color_correction(cv::Mat frame, cv::Mat &correctedFrame);

    std::pair<std::array<cv::Point2f, 4>, std::array<cv::Point2f, 4>>createTrapzoid(cv::Mat frame,
                                                                                    int bottomWidth,
                                                                                    int upperWidth,
                                                                                    int height);
    cv::Mat warp_frame_perspective(cv::Mat frame, cv::Mat &output);
    //Window get_sliding_window(cv::Mat frame, int nbins);
    //Window get_non_sliding_window(cv::Mat frame, int nbins);
    Line findLines(cv::Mat frame, int nwindows);
    float calculateCurvature(float yRange, std::vector<float> left_fit_cr);
    void drawLines(cv::Mat img, cv::Mat M, std::vector<float> left_fit, std::vector<float> right_fit, cv::Mat &out_img);
    void visualizeLanes(cv::Mat frame, cv::Mat M);
    void find_lane(cv::Mat frame);

public:
    LaneDetector(bool is_test=false);

    void get_lane_params();
};

#endif  // LANEDETECTOR_HPP