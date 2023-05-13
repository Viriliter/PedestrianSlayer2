#include "LaneDetector.hpp"

std::string type2strr(int type)
{
    std::string r;

    uchar depth = type & CV_MAT_DEPTH_MASK;
    uchar chans = 1 + (type >> CV_CN_SHIFT);

    switch (depth)
    {
    case CV_8U:
        r = "8U";
        break;
    case CV_8S:
        r = "8S";
        break;
    case CV_16U:
        r = "16U";
        break;
    case CV_16S:
        r = "16S";
        break;
    case CV_32S:
        r = "32S";
        break;
    case CV_32F:
        r = "32F";
        break;
    case CV_64F:
        r = "64F";
        break;
    default:
        r = "User";
        break;
    }

    r += "C";
    r += (chans + '0');

    return r;
}

std::string isBinaryMatrix(cv::Mat mat){
    for (int i = 0; i < mat.cols; i++)
    {
        for (int j = 0; j < mat.rows; j++)
        {
            int val = mat.at<uchar>(j,i);
            if (val != 1 && val != 0){
                return "Not Binary Matrix (" + std::to_string(val) + " value exist)";
            }
        }
    }
    return "Binary Matrix";
}

std::string isZeroMatrix(cv::Mat mat){
    for (int i = 0; i < mat.cols; i++)
    {
        for (int j = 0; j < mat.rows; j++)
        {
            int val = mat.at<uchar>(j,i);
            if (val != 0){
                return "Not Zero Matrix (" + std::to_string(val) + " value exist)";
            }
        }
    }
    return "Zero Matrix";
}

template <typename T>
void printVector(std::vector<T> &vec)
{
    for (auto &value : vec)
    {
        std::cout << value << ", ";
    }
    std::cout << std::endl;
}

void printMat(const cv::Mat &matrix)
{
    std::cout << cv::format(matrix, Formatter::FMT_PYTHON) << std::endl;
}

cv::Mat threshIt(cv::Mat in_frame, int thresh_min, int thresh_max)
{
    /*
    Applies a threshold to the `img` using [`thresh_min`, `thresh_max`] returning a binary image [0, 1]
    */
    cv::Mat xbinary(in_frame.size(), CV_8U, cv::Scalar(0));

    for (int i=0; i<in_frame.rows; i++){
        uchar *pImg = in_frame.ptr<uchar>(i);
        uchar *pBin = xbinary.ptr<uchar>(i);
        for (int j=0; j<in_frame.cols; j++){
            if (pImg[j] >= thresh_min && pImg[j] <= thresh_max){
                pBin[j] = 1;
            }
        }
    }
    return xbinary;
}

cv::Mat absSobelThresh(cv::Mat in_frame, char orient, int sobel_kernel, int thresh_min, int thresh_max)
{
    /*
    Calculate the Sobel gradient on the direction `orient` and return a binary thresholded image
    on [`thresh_min`, `thresh_max`]. Using `sobel_kernel` as Sobel kernel size.
    */
    int xorder, yorder;

    if (orient == 'x')
    {
        yorder = 0;
        xorder = 1;
    }
    else
    {
        yorder = 1;
        xorder = 0;
    }
    cv::Mat sobel;
    cv::Sobel(in_frame, sobel, CV_64F, xorder, yorder, sobel_kernel);
    cv::Mat abs_sobel = cv::abs(sobel);
    
    double min_sobel, max_sobel;
    cv::minMaxLoc(abs_sobel, NULL, &max_sobel, NULL, NULL);
    cv::Mat scaled = ((255.0 * abs_sobel) / max_sobel);  
    cv::Mat mScaled;
    scaled.assignTo(mScaled, CV_8U);

    return threshIt(mScaled, thresh_min, thresh_max);
    
}

LaneDetector::LaneDetector(bool is_test) : is_test(is_test)
{
    if (!is_test)
    {
        capture = cv::VideoCapture("/home/linux/Videos/test-0.mp4");
        //capture = cv::VideoCapture(0);
        //capture.set(3, 360);
        //capture.set(4, 240);
    }
    else
    {
        capture = cv::VideoCapture("/home/linux/Videos/test-0.mp4");
    }
}

cv::Mat LaneDetector::getFrame()
{
    cv::Mat frame;
    capture >> frame;
    return frame;
}

void LaneDetector::applyCameraCalibration(const cv::Mat &in_frame, cv::Mat &out_frame){
    cv::undistort(in_frame, out_frame, CAM_MAT, DIST_MAT, CAM_MAT);
}

void LaneDetector::applyColorFilter(const cv::Mat &in_frame, cv::Mat &out_frame){
    cv::cvtColor(in_frame, out_frame, cv::COLOR_BGR2HLS);
}

cv::Mat LaneDetector::splitColorChannel(const cv::Mat &in_frame, int channel_id){
    cv::Mat color_channels[3];
    cv::split(in_frame, color_channels);
    return color_channels[channel_id];

}

cv::Mat LaneDetector::combineColorChannel(const cv::Mat &in_frame){
    cv::Mat out_frame;
    cv::Mat cloned_frame = in_frame.clone()*255;
    cv::Mat in[] = {cloned_frame, cloned_frame, cloned_frame};
    cv::merge(in, 3, out_frame);
    return out_frame;

}

void LaneDetector::combineGradients(cv::Mat in_frame, cv::Mat &out_frame)
{
    /*
    Compute the combination of Sobel X and Sobel Y or Magnitude and Direction
    */
    auto withSobelX = [](cv::Mat corrected_img)
    { return absSobelThresh(corrected_img, 'x', 3, 10, 160); };
    auto withSobelY = [](cv::Mat corrected_img)
    { return absSobelThresh(corrected_img, 'y', 3, 10, 160); };

    cv::Mat sobelX = withSobelX(in_frame);
    cv::Mat sobelY = withSobelY(in_frame);   

    cv::bitwise_and(sobelX, sobelY, out_frame);
    // output[((sobelX == 1) & (sobelY == 1))] = 1;
}

cv::Mat LaneDetector::warpFrame(const cv::Mat &in_frame, cv::Mat &out_frame)
{
    /*
    std::pair<cv::Mat, cv::Mat> pair_m = calculatePerspectiveMatrix(const cv::Mat &in_frame);
    */

    cv::Size frame_size{in_frame.cols, in_frame.rows};
    cv::warpPerspective(in_frame, out_frame, M, frame_size);

    return M;
}

std::pair<std::array<cv::Point, 4>, std::array<cv::Point, 4>> LaneDetector::createTrapzoid(cv::Mat frame,
                                                                                               int bottomWidth,
                                                                                               int upperWidth,
                                                                                               int height)
{
    int xbias = 0;
    int ybias = 30;

    std::array<cv::Point, 4> src;
    std::array<cv::Point, 4> dst;

    if (bottomWidth > upperWidth)
    {
        int maxWidth = bottomWidth;
    }
    else
    {
        int maxWidth = upperWidth;
    }

    src = {cv::Point(frame.cols / 2 - bottomWidth / 2, 0),
           cv::Point(frame.cols / 2 + bottomWidth / 2, 0),
           cv::Point(frame.cols / 2 + bottomWidth / 2, frame.rows),
           cv::Point(frame.cols / 2 - bottomWidth / 2, frame.rows)};

    dst = {cv::Point(frame.cols / 2 - upperWidth / 2 + xbias, frame.rows - height + ybias),
           cv::Point(frame.cols / 2 + upperWidth / 2 + xbias, frame.rows - height + ybias),
           cv::Point(frame.cols / 2 + bottomWidth / 2 + xbias, frame.rows + ybias),
           cv::Point(frame.cols / 2 - bottomWidth / 2 + xbias, frame.rows + ybias)};
    /*
    cv::Point point0(frame.cols / 2 - upperWidth / 2 + xbias, frame.rows - height + ybias);
    cv::Point point1(frame.cols / 2 + upperWidth / 2 + xbias, frame.rows - height + ybias);
    cv::Point point2(frame.cols / 2 + bottomWidth / 2 + xbias, frame.rows + ybias);
    cv::Point point3(frame.cols / 2 - bottomWidth / 2 + xbias, frame.rows + ybias);

    cv::lane(frame, point0, point1, cv::Scalar(255, 255, 255), 2);
    cv::lane(frame, point1, point2, cv::Scalar(255, 255, 255), 2);
    cv::lane(frame, point2, point3, cv::Scalar(255, 255, 255), 2);
    cv::lane(frame, point3, point0, cv::Scalar(255, 255, 255), 2);
    cv::imshow("Perspective Trapzoid", frame);
    cv::waitKey(1000);
    */

    return std::make_pair(src, dst);
}

std::pair<cv::Mat, cv::Mat> LaneDetector::calculatePerspectiveMatrix(const cv::Mat &in_frame){
    int bottom = in_frame.cols;
    int upper = (int)(in_frame.cols * 0.2);
    int height = (int)(in_frame.rows * 0.4);

    std::pair<std::array<cv::Point, 4>, std::array<cv::Point, 4>> pairs;
    pairs = createTrapzoid(in_frame, bottom, upper, height);

    cv::Point2f src[4];
    src[0] = pairs.first[0];
    src[1] = pairs.first[1];
    src[2] = pairs.first[2];
    src[3] = pairs.first[3];

    cv::Point2f dst[4];
    dst[0] = pairs.second[0];
    dst[1] = pairs.second[1];
    dst[2] = pairs.second[2];
    dst[3] = pairs.second[3];

    cv::Mat Mnew = cv::getPerspectiveTransform(src, dst);
    cv::Mat Mnew_inv = Mnew.inv();
    std::cout << "##########################################" << std::endl;
    std::cout << "New Transformation Matrices" << std::endl;
    std::cout << "M:";
    printMat(Mnew);
    std::cout << std::endl;
    std::cout << "Minv:";
    printMat(Mnew_inv);
    std::cout << std::endl;
    std::cout << "##########################################" << std::endl;
    return std::make_pair(Mnew, Mnew_inv);
}

template<typename T>
void LaneDetector::calculateHistogram(cv::Mat &in_frame, std::vector<T> &histogram){
    for (int i = 0; i < in_frame.cols; i++)
    {
        int columnSum = 0;
        // Take a histogram of the bottom half of the image
        for (int j = in_frame.rows / 2; j < in_frame.rows; j++)
        {
            uchar &pFrame = in_frame.ptr<uchar>(j)[i];
            columnSum += (int) pFrame;
        }
        histogram.push_back(columnSum);
    }
}

Lane LaneDetector::findLaneParameters(const cv::Mat &raw_frame)
{
    std::vector<float> left_fit;
    std::vector<float> right_fit;
    std::vector<float> left_fit_m;
    std::vector<float> right_fit_m;
    // Create empty lists to receive left and right lane pixel indices
    std::vector<int> left_lane_inds;
    std::vector<int> right_lane_inds;
    cv::Mat out_img = raw_frame.clone();  //720x1280
    cv::Mat nonzerox;
    cv::Mat nonzeroy;

    int margin = 110;

    // Apply HLS color filter
    cv::Mat corrected_frame, filtered_frame;
    applyCameraCalibration(raw_frame, corrected_frame);
    applyColorFilter(corrected_frame, filtered_frame);
  
    // Convert to 1 channel image
    // Channel 0: Hue, Channel 1: Limunosity, Channel 2: Saturation
    cv::Mat s_channel = splitColorChannel(filtered_frame, 2);
    //showFrame("S Channel", s_channel);

    cv::Mat combined_frame(s_channel.size(), CV_8U);
    combineGradients(s_channel, combined_frame);
    //showFrame("Combined Gradient", combined_frame.clone()*255);

    // Apply a perspective transform
    cv::Mat frame(combined_frame.size(), CV_8U);
    warpFrame(combined_frame, frame);
    //showFrame("Perspective Frame", frame.clone()*255);

    // Convert to 3 channel color space
    if (is_test){
        out_img = combineColorChannel(frame);
    }

    // Calculate histogram of the image
    std::vector<int> histogram;
    calculateHistogram<int>(frame, histogram);

    // Create histogram chart to visualize
    //cv::Mat histogram_chart = createHistogramChart(histogram, frame.rows);
    //showFrame("Histogram", histogram_chart);

    // Find the peak of the left and right halves of the histogram
    // These will be the starting point for the left and right lines
    int midpoint = (int)histogram.size() / 2;

    cv::Point maxLoc;
    int leftx_base = 0;
    int rightx_base = 0;

    leftx_base, rightx_base = 0;
    for(int i=0; i<midpoint; i++){
        leftx_base = (histogram.at(i)>histogram.at(leftx_base))? i: leftx_base;
    }

    for(int i=midpoint; i<histogram.size(); i++){
        rightx_base = (histogram.at(i)>histogram.at(rightx_base))? i: rightx_base;
    }

    // Set height of windows
    int window_height = (int)frame.rows / N_WINDOWS;
    // Identify the x and y positions of all nonzero pixels in the image
    std::vector<cv::Point> nonzero;
    cv::findNonZero(frame, nonzero);
    for (auto iter = nonzero.begin(); iter != nonzero.end(); iter++)
    {
        nonzeroy.push_back(iter->y);
        nonzerox.push_back(iter->x);
    };

    // Current positions to be updated for each window
    float leftx_current = leftx_base;
    float rightx_current = rightx_base;

    // Step through the windows one by one
    for (int window = 0; window < N_WINDOWS; window++)
    {
        // Identify window boundaries in x and y (and right and left)
        int win_y_low = frame.rows - (window + 1) * window_height;
        int win_y_high = frame.rows - window * window_height;
        int win_xleft_low = leftx_current - margin;
        int win_xleft_high = leftx_current + margin;
        float win_xright_low = rightx_current - (float)margin;
        float win_xright_high = rightx_current + (float)margin;

        // Draw the windows on the visualization image
        if (is_test){
            cv::rectangle(out_img, cv::Point(win_xleft_low, win_y_low), cv::Point(win_xleft_high, win_y_high), LEFT_WINDOW_COLOR, 2);
            cv::rectangle(out_img, cv::Point(win_xright_low, win_y_low), cv::Point(win_xright_high, win_y_high), RIGHT_WINDOW_COLOR, 2);
        }

        // Identify the nonzero pixels in x and y within the window
        cv::Mat good_left_inds[2];    // 1d array
        cv::Mat good_right_inds[2];   // 1d array
        cv::Mat temp_good_left_inds;  // 1d array
        cv::Mat temp_good_right_inds; // 1d array

        cv::Mat mask1((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & (nonzerox >= win_xleft_low) & (nonzerox < win_xleft_high));
        cv::Mat mask2((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & (nonzerox >= win_xright_low) & (nonzerox < win_xright_high));
        cv::findNonZero(mask1, temp_good_left_inds);
        cv::findNonZero(mask2, temp_good_right_inds);   
        rightx_base += midpoint;

        cv::split(temp_good_left_inds, good_left_inds);
        cv::split(temp_good_right_inds, good_right_inds);

        for (int i = 0; i < good_left_inds[1].rows; i++)
        {
            left_lane_inds.push_back(good_left_inds[1].at<int>(i, 0));
        }
        for (int i = 0; i < good_right_inds[1].rows; i++)
        {
            right_lane_inds.push_back(good_right_inds[1].at<int>(i, 0));
        }

        // If you found > MIN_PIXEL pixels, recenter next window on their mean position
        if (good_left_inds[1].rows > MIN_PIXEL)
        {
            leftx_current = 0;
            for (int i = 0; i < good_left_inds[1].rows; i++)
            {
                leftx_current += nonzerox.at<int>(good_left_inds[1].at<int>(i));
            }
            leftx_current /= good_left_inds[1].rows;
        }
        if (good_right_inds[1].rows > MIN_PIXEL)
        {
            rightx_current = 0;
            for (int i = 0; i < good_right_inds[1].rows; i++)
            {
                rightx_current += nonzerox.at<int>(good_right_inds[1].at<int>(i));
            }
            rightx_current /= good_right_inds[1].rows;
        }
    }
    // Extract left and right lane pixel positions
    std::vector<float> leftx;
    std::vector<float> lefty;
    std::vector<float> rightx;
    std::vector<float> righty;

    for (int i = 0; i < left_lane_inds.size(); i++)
    {
        leftx.push_back((int) nonzerox.at<int>(left_lane_inds.at(i)));
        lefty.push_back((int) nonzeroy.at<int>(left_lane_inds.at(i)));
    }
    for (int i = 0; i < right_lane_inds.size(); i++)
    {
        rightx.push_back((int) nonzerox.at<int>(right_lane_inds.at(i)));
        righty.push_back((int) nonzeroy.at<int>(right_lane_inds.at(i)));
    }
    // Fit a second order polynomial to each
    left_fit = math_fun::polyfit(lefty, leftx, 2);
    right_fit = math_fun::polyfit(righty, rightx, 2);

    for (int i = 0; i < lefty.size(); i++)
    {
        leftx.at(i) *= XM_PER_PIX;
        lefty.at(i) *= YM_PER_PIX;
    }

    for (int i = 0; i < righty.size(); i++)
    {
        rightx.at(i) *= XM_PER_PIX;
        righty.at(i) *= YM_PER_PIX;
    }

    // Fit a second order polynomial to each  
    left_fit_m = math_fun::polyfit(lefty, leftx, 2);
    right_fit_m = math_fun::polyfit(righty, rightx, 2);

    if (is_test){
        for(int i=0; i<left_lane_inds.size(); i++){
            int y = nonzeroy.at<int>(left_lane_inds.at(i));
            int x = nonzerox.at<int>(left_lane_inds.at(i));
            BGR &color = out_img.ptr<BGR>(y)[x];
            color = LEFT_LANE_CLUSTER_COLOR;
        }
        for(int i=0; i<right_lane_inds.size(); i++){
            int y = nonzeroy.at<int>(right_lane_inds.at(i));
            int x = nonzerox.at<int>(right_lane_inds.at(i));
            BGR &color = out_img.ptr<BGR>(y)[x];
            color = RIGHT_LANE_CLUSTER_COLOR;
        }
    }

    Lane lane;
    lane.left_fit = left_fit;
    lane.right_fit = right_fit;
    lane.left_fit_m = left_fit_m;
    lane.right_fit_m = right_fit_m;
    lane.left_lane_inds = left_lane_inds;
    lane.right_lane_inds = right_lane_inds;
    lane.out_img = out_img;
    lane.nonzerox = nonzerox;
    lane.nonzeroy = nonzeroy;
    
    return lane;
}

void LaneDetector::showFrame(std::string title, const cv::Mat &frame){
    if (!is_test) return;
    cv::imshow(title, frame);
}

cv::Mat LaneDetector::createHistogramChart(const std::vector<int> &histogram, int height){
    cv::Mat histogram_chart(cv::Size(histogram.size(), height), CV_8UC1, cv::Scalar(0));
    int indexx = 0;
    for (auto &v:histogram){
        for(int i=0; i<v; i++){
            if (i>=histogram_chart.rows) break;
            int val = (histogram_chart.rows-1)-i;
            uchar &pHist = histogram_chart.ptr<uchar>(val)[indexx];
            pHist = (int) 255;
        }
        indexx++;
    }
    return histogram_chart;
}

void LaneDetector::drawLines(cv::Mat img, Lane &lane, cv::Mat &out_img)
{
    /*
    Draw the lane lines on the image `img` using the poly `left_fit` and `right_fit`.
    */
    if (!is_test) return;

    int yMax = img.rows;
    std::vector<double> ploty = math_fun::linspace<int>(0, yMax - 1, yMax);
    cv::Mat color_warp(img.size(), CV_8UC3, cv::Scalar(0, 0, 0));

    // Calculate points.
    std::vector<double> left_fitx;
    std::vector<double> right_fitx;

    for (auto &val_ploty : ploty)
    {
        left_fitx.push_back(lane.left_fit[0] * std::pow(val_ploty, 2) + lane.left_fit[1] * val_ploty + lane.left_fit[2]);
        right_fitx.push_back(lane.right_fit[0] * std::pow(val_ploty, 2) + lane.right_fit[1] * val_ploty + lane.right_fit[2]);
    }

    // Recast the x and y points into usable format for polylines
    std::vector<cv::Point> left_pts, right_pts;

    for (int i = 0; i < left_fitx.size(); i++)
    {
        left_pts.push_back(cv::Point(left_fitx[i], ploty[i]));
    }

    for (int i = 0; i < right_fitx.size(); i++)
    {
        right_pts.push_back(cv::Point(right_fitx[i], ploty[i]));
    }

    // Draw the lane onto the warped blank image
    cv::polylines(color_warp, left_pts, false, LEFT_LINE_COLOR, LINE_THICKNESS);
    cv::polylines(color_warp, right_pts, false, RIGHT_LINE_COLOR, LINE_THICKNESS);

    // Warp the blank back to original image space using inverse perspective matrix (Minv)
    cv::Mat newWarp;
    cv::warpPerspective(color_warp, newWarp, M_INV, img.size());
    cv::addWeighted(img, 1, newWarp, 1, 0, out_img);
}

float LaneDetector::calculateCurvature(std::vector<float> left_fit_cr)
{
    /*
    Returns the curvature of the polynomial `fit` on the y range `yRange`.
    */
    return (std::pow(1 + std::pow(2 * left_fit_cr[0] * Y_RANGE * YM_PER_PIX + left_fit_cr[1], 2), 1.5)) / std::abs(2 * left_fit_cr[0]);
}

float LaneDetector::calculateVehiclePosition(const cv::Mat &frame, Lane &lane){
    float xMax = frame.cols * XM_PER_PIX;
    float yMax = frame.rows * YM_PER_PIX;

    float vehicleCenter = xMax / (float)2;
    float lineLeft = lane.left_fit_m[0] * std::pow(yMax, 2) + lane.left_fit_m[1] * yMax + lane.left_fit_m[2];
    float lineRight = lane.right_fit_m[0] * std::pow(yMax, 2) + lane.right_fit_m[1] * yMax + lane.right_fit_m[2];
    float lineMiddle = lineLeft + (lineRight - lineLeft) / 2;
    return lineMiddle - vehicleCenter;  //  diffFromVehicle
}

void LaneDetector::visualizeLanes(const cv::Mat &frame, cv::Mat &out_img)
{
    /*
    Visualize the windows and fitted lines for `image`.
    Returns (`left_fit` and `right_fit`)
    */

    if (!is_test) return;

    Lane lane = findLaneParameters(frame);
    //showFrame("Windows on Line", lane.out_img);

    drawLines(frame, lane, out_img);
    //showFrame("Lines on Lane", lane.out_img);

    float leftCurvature = calculateCurvature(lane.left_fit_m);
    float rightCurvature = calculateCurvature(lane.right_fit_m);
    float diffFromVehicle = calculateVehiclePosition(frame, lane);

    std::string message;
    if (diffFromVehicle > 0)
        message = std::to_string(diffFromVehicle) + " m right";
    else
        message = std::to_string(-diffFromVehicle) + " m left";

    // Draw info
    int fontScale = 1;
    std::string text1, text2, text3;
    text1 = "Left curvature: " + std::to_string(leftCurvature) + " m";
    text2 = "Right curvature: " + std::to_string(rightCurvature) + " m";
    text3 = "Vehicle is " + message + " of center";

    cv::putText(out_img, text1, cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, fontScale, TEXT_COLOR, 2);
    cv::putText(out_img, text2, cv::Point(50, 120), cv::FONT_HERSHEY_SIMPLEX, fontScale, TEXT_COLOR, 2);
    cv::putText(out_img, text3, cv::Point(50, 190), cv::FONT_HERSHEY_SIMPLEX, fontScale, TEXT_COLOR, 2);
}

void LaneDetector::findVehiclePosition(cv::Mat frame)
{
    Lane lane = findLaneParameters(frame);
    float diffFromVehicle = calculateVehiclePosition(frame, lane);    
}

void LaneDetector::getLaneParams()
{
    cv::Mat frame = getFrame();

    //miscs::Time time1 = miscs::pickTime();
    //findVehiclePosition(frame);
    //miscs::Time time2 = miscs::pickTime();
    //miscs::showElapsedTime(time1, time2, (int) timing::timeFormat::formatMiliseconds);
    
    cv::imshow("Original", frame);
    cv::Mat visualized_frame = frame;
    visualizeLanes(frame, visualized_frame);
    cv::imshow("Visualized Frame", visualized_frame);
    cv::waitKey(1);
    
}

/*
void LaneDetector::getLaneParams()
{
    // cv::Mat frame = getFrame();
    std::vector<std::string> imgs_path;
    imgs_path.push_back("/home/linux/Pictures/test_images/test6.jpg");
    imgs_path.push_back("/home/linux/Pictures/test_images/straight_lines1.jpg");
    imgs_path.push_back("/home/linux/Pictures/test_images/test1.jpg");
    imgs_path.push_back("/home/linux/Pictures/test_images/test5.jpg");
    imgs_path.push_back("/home/linux/Pictures/test_images/straight_lines2.jpg");
    imgs_path.push_back("/home/linux/Pictures/test_images/test3.jpg");
    imgs_path.push_back("/home/linux/Pictures/test_images/test4.jpg");
    imgs_path.push_back("/home/linux/Pictures/test_images/test2.jpg");
    cv::Mat frame;
    for (auto &img_path : imgs_path)
    {
        frame = cv::imread(img_path);

        cv::imshow("Original", frame);
        
        cv::Mat visualized_frame;
        visualizeLanes(frame, visualized_frame);
        cv::imshow("Visualized Frame", visualized_frame);

        cv::waitKey(1000);
        std::cout << "Wait " << std::endl;

    }
}
*/
