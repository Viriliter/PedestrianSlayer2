#include "LaneDetector.hpp"

LaneDetector::LaneDetector(bool is_test): is_test(is_test){
    if (!is_test){
        capture = cv::VideoCapture(0);
        capture.set(3,360);
        capture.set(4,240);
    }
    else{
        capture = cv::VideoCapture("/home/linux/Videos/test-1.mp4");
    }
}

cv::Mat LaneDetector::get_frame(){
    cv::Mat frame;
    capture >> frame;
    return frame;
}

void LaneDetector::set_color_correction(cv::Mat frame, cv::Mat &correctedFrame){
    cv::Mat HLS; 
    cv::cvtColor(frame, HLS, cv::COLOR_BGR2HLS);
    cv::Mat channels[3];
    cv::split(HLS, channels);
    
    std::vector<cv::Mat> h_channels;
    h_channels.push_back(channels[2]);
    h_channels.push_back(channels[2]);
    h_channels.push_back(channels[2]);

    cv::Mat new_layer;
    cv::merge(h_channels, new_layer);
    //cv::add(h_channels, new_layer, correctedFrame);
    //correctedFrame = new_layer;
    cv::add(frame, new_layer, correctedFrame);
}

std::pair<std::array<cv::Point2f, 4>, std::array<cv::Point2f, 4>> LaneDetector::createTrapzoid(cv::Mat frame, 
                                                                                               int bottomWidth,
                                                                                               int upperWidth, 
                                                                                               int height){
    int xbias = 0;
    int ybias = 0;
    
    std::array<cv::Point2f, 4> src;
    src = {cv::Point2f((float)frame.cols/2 - (float)upperWidth/2 + xbias, (float)frame.rows - height + (float)ybias),
           cv::Point2f((float)frame.cols/2 + (float)upperWidth/2 + xbias, (float)frame.rows - height + (float)ybias),
           cv::Point2f((float)frame.cols/2 + (float)bottomWidth/2 + xbias, (float)frame.rows + (float)ybias),
           cv::Point2f((float)frame.cols/2 - (float)bottomWidth/2 + xbias, (float)frame.rows + (float)ybias)};

    if (bottomWidth > upperWidth){
        int maxWidth = bottomWidth;
    }
    else{
        int maxWidth = upperWidth;
    }
    std::array<cv::Point2f, 4> dst;
    dst = {cv::Point2f((float)frame.cols/2 - (float)bottomWidth/2, (float)0),
           cv::Point2f((float)frame.cols/2 + (float)bottomWidth/2, (float)0),
           cv::Point2f((float)frame.cols/2 + (float)bottomWidth/2, (float)frame.rows),
           cv::Point2f((float)frame.cols/2 - (float)bottomWidth/2, (float)frame.rows)};
    
    return std::make_pair(src, dst);
}

cv::Mat LaneDetector::warp_frame_perspective(cv::Mat frame, cv::Mat &output){
    // frame.shape[0]: number of row
    // frame.shape[1]: number of col
    
    cv::Size frame_size{frame.cols,frame.rows};
    int bottom = frame.cols;
    int upper = (int) (frame.cols * 1);
    int height = (int) (frame.rows * 0.4);
    
    std::pair<std::array<cv::Point2f, 4>, std::array<cv::Point2f, 4>> pairs;
    pairs = createTrapzoid(frame, bottom, upper, height);

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
    auto M = cv::getPerspectiveTransform(src, dst);
    
    cv::warpPerspective(frame, output, M, frame_size, cv::INTER_LINEAR);
    
    return M;
}

Line LaneDetector::findLines(cv::Mat frame, int nwindows){
    std::vector<float> left_fit;
    std::vector<float> right_fit;
    std::vector<float> left_fit_m;
    std::vector<float> right_fit_m;
    // Create empty lists to receive left and right lane pixel indices
    std::vector<int> left_lane_inds;
    std::vector<int> right_lane_inds;
    cv::Mat out_img = frame;
    cv::Mat nonzerox;
    cv::Mat nonzeroy;
    
    int margin=110, minpix=50;

    // Take a histogram of the bottom half of the image
    cv::Mat histogram;
    for (int i=frame.size().height/2; i< frame.size().height; i++){
        frame.row(i).copyTo(histogram.row(i));
    }

    // Find the peak of the left and right halves of the histogram
    // These will be the starting point for the left and right lines
    int midpoint = (int) histogram.rows/2;
    
    cv::Point maxLoc;
    int leftx_base = 0;
    int rightx_base = 0;
    cv::minMaxLoc(histogram(cv::Rect(0,0,histogram.size().height,midpoint)), nullptr, nullptr, nullptr, &maxLoc);  //[:midpoint]
    leftx_base = maxLoc.x;
    cv::minMaxLoc(histogram(cv::Rect(midpoint,0,histogram.size().height,histogram.size().height)), nullptr, nullptr, nullptr, &maxLoc); // [midpoint:]
    rightx_base = maxLoc.x + midpoint;

    // Set height of windows
    int window_height = (int) frame.rows/nwindows;
    // Identify the x and y positions of all nonzero pixels in the image
    std::vector<cv::Point> nonzero;
    cv::findNonZero(frame, nonzero);
    for(auto iter = nonzero.begin(); iter==nonzero.end(); iter++){
        nonzeroy.row(0).push_back(iter->y);
        nonzerox.row(0).push_back(iter->x);
    };       

    // Current positions to be updated for each window
    float leftx_current = leftx_base;
    float rightx_current = rightx_base;
    
    // Step through the windows one by one
    for (int window=0; window<nwindows; window++){
        // Identify window boundaries in x and y (and right and left)
        int win_y_low = frame.rows - (window+1)*window_height;
        int win_y_high = frame.rows - window*window_height;
        int win_xleft_low = leftx_current - margin;
        int win_xleft_high = leftx_current + margin;
        float win_xright_low = rightx_current - (float) margin;
        float win_xright_high = rightx_current + (float) margin;
        // Draw the windows on the visualization image
        cv::rectangle(out_img, cv::Point(win_xleft_low, win_y_low), cv::Point(win_xleft_high, win_y_high), cv::Scalar(0,255,0), 2);
        cv::rectangle(out_img, cv::Point(win_xright_low, win_y_low), cv::Point(win_xright_high, win_y_high), cv::Scalar(0,255,0), 2);
        // Identify the nonzero pixels in x and y within the window
        cv::Mat good_left_inds;  // 1d array
        cv::Mat good_right_inds;  // 1d array
        cv::Mat mask1{(nonzeroy.row(0) >= win_y_low) & (nonzeroy.row(0) < win_y_high) & (nonzerox.row(0) >= win_xleft_low) & (nonzerox.row(0) < win_xleft_high)};
        cv::Mat mask2{(nonzeroy.row(0) >= win_y_low) & (nonzeroy.row(0) < win_y_high) & (nonzerox.row(0) >= win_xright_low) & (nonzerox.row(0) < win_xright_high)};
        cv::findNonZero(mask1, good_left_inds);
        cv::findNonZero(mask2, good_right_inds);

        for(int i=0; i< good_left_inds.cols; i++){
            left_lane_inds.push_back(good_left_inds.at<int>(0,i));    
        }
        for(int i=0; i< good_right_inds.cols; i++){
            right_lane_inds.push_back(good_right_inds.at<int>(0,i));    
        }

        //temp_left_lane_inds.push_back(good_left_inds);
        //temp_right_lane_inds.push_back(good_right_inds);

        // If you found > minpix pixels, recenter next window on their mean position
        if (good_left_inds.rows > minpix){
            leftx_current = (int) (cv::mean(nonzerox.row(0), good_left_inds.row(0)))[0];
        }
        if (good_right_inds.rows > minpix){
            rightx_current = (int) (cv::mean(nonzerox.row(0), good_right_inds.row(0)))[0];
        }
    }
    
    // Concatenate the arrays of indices
    //left_lane_inds.insert(temp_left_lane_inds.end(), temp_left_lane_inds.begin(), temp_left_lane_inds.end());
    //right_lane_inds.insert(temp_right_lane_inds.end(), temp_right_lane_inds.begin(), temp_right_lane_inds.end());

    // Extract left and right line pixel positions
    std::vector<float> leftx;
    std::vector<float> lefty;
    std::vector<float> rightx;
    std::vector<float> righty;
    for(int i=0; i<left_lane_inds.size(); i++){
        leftx.push_back(nonzerox.row(0).at<float>(left_lane_inds.at(i)));    
        lefty.push_back(nonzeroy.row(0).at<float>(left_lane_inds.at(i)));    
    }
    for(int i=0; i<right_lane_inds.size(); i++){
        rightx.push_back(nonzerox.row(0).at<float>(right_lane_inds.at(i)));    
        righty.push_back(nonzeroy.row(0).at<float>(right_lane_inds.at(i)));    
    }
    
    // Fit a second order polynomial to each
    left_fit = math_fun::polyfit(lefty, leftx, 2);
    right_fit = math_fun::polyfit(righty, rightx, 2);
    
    for(int i=0; i<lefty.size(); i++){
        leftx.at(i) *= XM_PER_PIX;
        lefty.at(i) *= YM_PER_PIX;
    }

    for(int i=0; i<righty.size(); i++){
        rightx.at(i) *= XM_PER_PIX;
        righty.at(i) *= YM_PER_PIX;
    }

    // Fit a second order polynomial to each
    
    left_fit_m = math_fun::polyfit(lefty, leftx, 2);
    right_fit_m = math_fun::polyfit(righty, rightx, 2);
    
    Line line;
    line.left_fit = left_fit;
    line.right_fit = right_fit;
    line.left_fit_m = left_fit_m;
    line.right_fit_m = right_fit_m;
    line.left_lane_inds = left_lane_inds;
    line.right_lane_inds = right_lane_inds;
    line.out_img = out_img;
    line.nonzerox = nonzerox;
    line.nonzeroy = nonzeroy;
    
    return line;
}

float LaneDetector::calculateCurvature(float yRange, std::vector<float>  left_fit_cr){
    /*
    Returns the curvature of the polynomial `fit` on the y range `yRange`.
    */
    return (std::pow(1 + std::pow(2*left_fit_cr[0]*yRange*YM_PER_PIX + left_fit_cr[1], 2), 1.5)) / std::abs(2*left_fit_cr[0]);
}

void LaneDetector::drawLines(cv::Mat img, cv::Mat M, std::vector<float> left_fit, std::vector<float> right_fit, cv::Mat &out_img){
    /*
    Draw the lane lines on the image `img` using the poly `left_fit` and `right_fit`.
    */
    int yMax = img.rows;
    std::vector<double> ploty;
    ploty = math_fun::linspace<int>(0, yMax - 1, yMax);
    cv::Mat color_warp(cv::Size(img.rows, img.cols), 0);

    // Calculate points.
    std::vector<double> left_fitx;
    std::vector<double> right_fitx;
    
    for (auto &val_ploty: ploty){
        left_fitx.push_back(left_fit[0]*std::pow(val_ploty, 2) + left_fit[1]*val_ploty + left_fit[2]);
        right_fitx.push_back(right_fit[0]*std::pow(val_ploty, 2) + right_fit[1]*val_ploty + right_fit[2]);
    }

    // Recast the x and y points into usable format for cv2.fillPoly()
    std::vector<std::vector<float>> pts;
       
    for(int i=0; i<left_fitx.size(); i++){
        std::vector<float> temp;
        temp.push_back(left_fitx[i]);
        temp.push_back(ploty[i]);
        pts[i] = temp;
    }
    
    for(int i=0; i<right_fitx.size(); i++){
        std::vector<float> temp;
        temp.push_back(right_fitx[i]);
        temp.push_back(ploty[i]);
        pts[i] = temp;
    }

    // Draw the lane onto the warped blank image
    cv::fillPoly(color_warp, pts, cv::Scalar(0,255, 0));
    
    // Warp the blank back to original image space using inverse perspective matrix (Minv)
    cv::Mat newWarp;
    cv::warpPerspective(color_warp, newWarp, M, cv::Size(img.cols, img.rows));
    cv::addWeighted(img, 1, newWarp, 0.3, 0, out_img);
}

void LaneDetector::visualizeLanes(cv::Mat frame, cv::Mat M){
    /*
    Visualize the windows and fitted lines for `image`.
    Returns (`left_fit` and `right_fit`)
    */

    Line line = findLines(frame, 9);
    cv::Mat out_img;
    drawLines(frame, M, line.left_fit, line.right_fit, out_img);
    
    int yRange = 719;
    float leftCurvature = calculateCurvature(yRange, line.left_fit_m);
    float rightCurvature = calculateCurvature(yRange, line.right_fit_m);

    float xMax = frame.cols*YM_PER_PIX;
    float yMax = frame.rows*XM_PER_PIX;
    float vehicleCenter = xMax / (float)2;

    float lineLeft = line.left_fit_m[0]*std::pow(yMax,2) + line.left_fit_m[1]*yMax + line.left_fit_m[2];
    float lineRight = line.right_fit_m[0]*std::pow(yMax,2) + line.right_fit_m[1]*yMax + line.right_fit_m[2];
    float lineMiddle = lineLeft + (lineRight - lineLeft)/2;
    float diffFromVehicle = lineMiddle - vehicleCenter;
    
    std::string message;
    if (diffFromVehicle > 0)
        message = "%.2f m right", (diffFromVehicle);
    else
        message = "%.2f m left", (-diffFromVehicle);
    
    // Draw info
    int fontScale = 2;
    std::string text1, text2, text3;
    text1 = "Left curvature: %.2f m", leftCurvature;
    text2 = "Right curvature: %.2f m", rightCurvature;
    text3 = "Vehicle is %s of center", message;
    
    cv::putText(out_img, text1, cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(255, 255, 255), 2);
    cv::putText(out_img, text2, cv::Point(50, 120), cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(255, 255, 255), 2);
    cv::putText(out_img, text3, cv::Point(50, 190), cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(255, 255, 255), 2);

}

void LaneDetector::find_lane(cv::Mat frame){
    int nbins = 9;
    Line line = findLines(frame, nbins);

    float yRange = 719;
    float leftCurvature, rightCurvature;
    leftCurvature = calculateCurvature(yRange, line.left_fit_m) / 1000;
    rightCurvature = calculateCurvature(yRange, line.right_fit_m) / 1000;
}

void LaneDetector::get_lane_params(){
    cv::Mat frame = get_frame();
    cv::imshow("Original", frame);
    
    //Apply color filter to the frame
    cv::Mat corrected_frame;
    set_color_correction(frame, corrected_frame);               
    cv::imshow("Corrected Frame", corrected_frame);
    
    cv::Mat canny_frame;
    cv::Canny(frame, canny_frame, 200, 255);
    cv::imshow("Canny Frame", canny_frame);
    
    cv::Mat warped_frame;
    cv::Mat M = warp_frame_perspective(canny_frame, warped_frame);

    cv::imshow("Warped Frame", warped_frame);

    visualizeLanes(frame, M);

    cv::waitKey(100);
}
