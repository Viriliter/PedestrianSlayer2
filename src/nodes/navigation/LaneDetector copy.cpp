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

cv::Mat LaneDetector::warp_frame_perspective(cv::Mat frame){
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
    
    cv::Mat warped_frame;
    cv::warpPerspective(frame, warped_frame, M, frame_size, cv::INTER_LINEAR);
    
    return warped_frame;
}

Window LaneDetector::get_sliding_window(cv::Mat frame, int nbins){
        std::vector<float> left_fit;
        std::vector<float> right_fit;
        float left_curverad;
        float right_curverad;
        cv::Mat out_img;

        std::vector<cv::Mat> out_img_channels;
        out_img_channels.push_back(frame*255);
        out_img_channels.push_back(frame*255);
        out_img_channels.push_back(frame*255);
        cv::merge(out_img, out_img_channels);


        cv::Mat histogram( 1, frame.size().height, CV_32FC1, 0.0);

        for(int i=frame.size().height/2; i<frame.size().height; i++){
            for(int j=0; j<frame.size().width; j++){
                histogram[i] += frame[i][j];
            }
        }
        
        // Find the peak of the left and right halves of the histogram
        // These will be the starting point for the left and right lines
        int midpoint = (int) (histogram.rows/2);
        
        float leftx_base = 0;
        for (int i=0; i<midpoint; i++){
            leftx_base = ((histogram[i]>leftx_base)? histogram[i] : leftx_base);
        }

        float rightx_base = 0;
        for (int i=midpoint; i<histogram.rows; i++){
            rightx_base = ((histogram[i][0]>rightx_base)? histogram[i] : rightx_base) + midpoint;
        }

        std::vector<float> leftx;
        std::vector<float> lefty;
        std::vector<float> rightx;
        std::vector<float> righty;
        
        // Choose the number of sliding windows
        int nwindows = 15;
        // Set height of windows
        int window_height = (int) frame.rows/nwindows;

        // Identify the x and y positions of all nonzero pixels in the image
        std::vector<cv::Point> nonzero;
        cv::findNonZero(frame, nonzero);

        std::vector<int> nonzeroy;
        std::vector<int> nonzerox;
        for (int i=0; i< nonzero.size(); i++){
            nonzerox.at(i) = nonzero.at(i).x;
            nonzeroy.at(i) = nonzero.at(i).y;
        }
        
        // Find the pe.array(nonzero[1])
        // Current positions to be updated for each window
        int leftx_current = leftx_base;
        int rightx_current = rightx_base;
        
        // Set the width of the windows +/- margin
        int margin = 70;
        
        // Set minimum number of pixels found to recenter window
        int minpix = 60;
        
        // Create empty lists to receive left and right lane pixel indices
        std::vector<float> left_lane_inds;
        std::vector<float> right_lane_inds;
        std::vector<float> leftdelete;
        std::vector<float> rightdelete;
        
        // Step through the windows one by one
        for (int window=0; window<nwindows; window++){
            // Identify window boundaries in x and y (and right and left)
            float win_y_low = frame.rows - (window+1)*window_height;
            float win_y_high = frame.rows - window*window_height;
            float win_xleft_low = leftx_current - margin;
            float win_xleft_high = leftx_current + margin;
            float win_xright_low = rightx_current - margin;
            float win_xright_high = rightx_current + margin;

            // Draw the windows on the visualization image
            if (is_test){
                cv::rectangle(out_img, cv::Point(win_xleft_low, win_y_low), cv::Point(win_xleft_high,win_y_high), cv::Scalar(0,255,0), 2);  // Green
                cv::rectangle(out_img, cv::Point(win_xright_low, win_y_low), cv::Point(win_xright_high,win_y_high), cv::Scalar(0,255,0), 2);  // Green
                cv::imshow("kk", out_img);
            }

            // Identify the nonzero pixels in x and y within the window
            std::vector<int> good_left_inds;
            std::vector<int> good_right_inds;
            
            for(int i=0; i< nonzerox.size(); i++){
                
            }
            good_left_inds = ((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & (nonzerox >= win_xleft_low) & (nonzerox < win_xleft_high)).nonzero()[0];
            good_right_inds = ((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & (nonzerox >= win_xright_low) & (nonzerox < win_xright_high)).nonzero()[0];
            
            // Append these indices to the lists
            left_lane_inds.push_back(good_left_inds);
            right_lane_inds.push_back(good_right_inds);

            // If you found > minpix pixels, recenter next window on their mean position
            if (good_left_inds.size() > minpix)
                leftx_current =  (int) (np.mean(nonzerox[good_left_inds]));
            if (good_right_inds.size() > minpix)
                rightx_current = (int) (np.mean(nonzerox[good_right_inds]));
        }
        
        if (len(left_lane_inds[nwindows-1]) < minpix)
            leftdelete.push_back(nwindows-1);

        if (len(right_lane_inds[nwindows-1]) < minpix)
            rightdelete.push_back(nwindows-1);

        for (int y=2;  y<nwindows-1; y++){
            int x = nwindows - y;
            int l1 = int(len(left_lane_inds[x-1]) > minpix);
            int l2 = int(len(left_lane_inds[x]) > minpix);
            int l3 = int(len(left_lane_inds[x+1]) > minpix);
            int r1 = int(len(right_lane_inds[x-1]) > minpix);
            int r2 = int(len(right_lane_inds[x]) > minpix);
            int r3 = int(len(right_lane_inds[x+1]) > minpix);
            
            if ((l1+l2+l3)<2)
                leftdelete.push_back(x);
            if ((r1+r2+r3)<2)
                rightdelete.push_back(x);
        }

        if (len(left_lane_inds[1]) < minpix)
            leftdelete.push_back(1);
    
        if (len(right_lane_inds[1]) < minpix)
            rightdelete.push_back(1);
        
        /*
        // print len(leftdelete)
        for h in range (len(leftdelete)-1){
            print (leftdelete[h+1]>1+leftdelete[h])
            print str(leftdelete[h+1]) + "***" + str(leftdelete[h])
            if leftdelete[h+1]>leftdelete[h]{}
        }
        */
       
        for (int i=0; i<leftdelete.size(); i++){
            int k = leftdelete[i];
            int p = len(left_lane_inds[k]);
            
            int window = k;
            int win_y_low = frame.rows - (window+1)*window_height;
            int win_y_high = frame.rows - window*window_height;
            int avg = (int) (win_y_low + win_y_high)/2;
            
            lefty.extend([avg, avg, avg]);
            leftx.extend([0, 1, 2]);

            for (int n=win_y_low; n<win_y_high; n++){
                for (int g=0; g<2; g++){
                   out_img[n, g] = 255;
                }
            }
            /*
            if(p>0){
                for t in range (p){
                    out_img[nonzeroy[left_lane_inds[k][t]],nonzerox[left_lane_inds[k][t]]]=0;
                }
            }
            */     
          
           delete left_lane_inds[k];
        }

        for (int j=0; j<rightdelete.size(); j++){
            int l = rightdelete[j];
            int u = len(right_lane_inds[l]);
            int window = l;
            int win_y_low = frame.rows - (window+1)*window_height;
            int win_y_high = frame.rows - window*window_height;
            int avg = (int) (win_y_low+win_y_high)/2;

            righty.extend([avg,avg,avg]);

            int outlane = frame.cols;
            rightx.extend([outlane-2, outlane-1, outlane]);

            for (int n=win_y_low; n<win_y_high; n++){
                // righty.extend([n])
                for (int g=outlane-2; g<outlane; g++){
                    out_img[n, g] = 255;
               }
            }
            /*
            if(u>0){
                for m in range (u){
                    koor1=int(nonzerox[right_lane_inds[l][m]])
                    koor2=int(nonzeroy[right_lane_inds[l][m]])
                    out_img[koor2,koor1]=0
                }
            }
            */
            delete right_lane_inds[l];
        }
           
        cv::imshow("yy", out_img);
                
        // Generate x and y values for plotting
        std::vector<double> ploty = linspace(0, frame.rows-1, frame.rows);
        
        // Calculate radii of curvature in meters
        float y_eval = np.max(ploty);  // Where radius of curvature is measured

        try{
            // Concatenate the arrays of indices
            left_lane_inds = np.concatenate(left_lane_inds);

            // Extract left and right line pixel positions
            leftx.extend(nonzerox[left_lane_inds]);
            lefty.extend(nonzeroy[left_lane_inds]);

            // Fit a second order polynomial to each
            left_fit = np.polyfit(lefty, leftx, 2);

            // Stash away polynomials
            left_line.current_fit = left_fit;

            // Generate x and y values for plotting
            left_fitx = left_fit[0]*ploty**2 + left_fit[1]*ploty + left_fit[2];
            
            out_img[nonzeroy[left_lane_inds], nonzerox[left_lane_inds]] = [255, 0, 0];
            out_img[ploty.astype('int'),left_fitx.astype('int')] = [0, 255, 255];

            // Fit new polynomials to x,y in world space
            lefty = np.array(lefty);
            leftx = np.array(leftx);

            left_fit_cr = np.polyfit(lefty*YM_PER_PIX, leftx*XM_PER_PIX, 2);

            // Calculate radii of curvature in meters
            left_curverad = ((1 + (2*left_fit_cr[0]*y_eval*YM_PER_PIX + left_fit_cr[1])**2)**1.5) / np.absolute(2*left_fit_cr[0]);
            
            // Stash away the curvatures  
            left_line.radius_of_curvature = left_curverad;
        } 
        
        catch (...){
            left_line.radius_of_curvature = 0;
            left_curverad = 0;
        }
        
        try{
            // Concatenate the arrays of indices
            right_lane_inds = np.concatenate(right_lane_inds);
            
            // Extract left and right line pixel positions
            rightx.extend(nonzerox[right_lane_inds]);
            righty.extend(nonzeroy[right_lane_inds]);
            // Fit a second order polynomial to each
            right_fit = np.polyfit(righty, rightx, 2);

            // Stash away polynomials
            right_line.current_fit = right_fit;
            
            // Generate x and y values for plotting
            right_fitx = right_fit[0]*ploty**2 + right_fit[1]*ploty + right_fit[2];
            out_img[nonzeroy[right_lane_inds], nonzerox[right_lane_inds]] = [0, 0, 255];
            out_img[ploty.astype('int'),right_fitx.astype('int')] = [0, 255, 255];
            
            // Fit new polynomials to x,y in world space
            righty = np.array(righty);
            rightx = np.array(rightx);
                    
            right_fit_cr = np.polyfit(righty*YM_PER_PIX, rightx*XM_PER_PIX, deg=2);
            // Calculate radii of curvature in meters
            right_curverad = ((1 + (2*right_fit_cr[0]*y_eval*YM_PER_PIX + right_fit_cr[1])**2)**1.5) / np.absolute(2*right_fit_cr[0]);
                    
            // Stash away the curvatures
            right_line.radius_of_curvature = right_curverad;
        }                   
          
        catch (...){
            right_line.radius_of_curvature = 0;
            right_curverad = 0;
        }

        Window window;
        window.left_fit = left_fit;
        window.right_fit = right_fit;
        window.left_curverad = left_curverad;
        window.right_curverad = right_curverad;
        window.out_img = out_img;
        
        return window;
}

Window LaneDetector::get_non_sliding_window(cv::Mat frame, int nbins){}

void LaneDetector::find_lane(cv::Mat frame){
    int nbins = 10;

    if (prevErrorCnt != errorCnt){
        Window window = get_sliding_window(frame, nbins);

    }       
    else{
        Window window = get_non_sliding_window(frame,
                                               np.average(l_params,0,weights[-len(l_params):]),
                                               np.average(r_params,0,weights[-len(l_params):]),
                                               nbins);

    }
    
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
    
    cv::Mat warped_frame = warp_frame_perspective(canny_frame);
    cv::imshow("Warped Frame", warped_frame);
    
    cv::waitKey(100);
}