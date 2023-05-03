#include <iostream>
#include <string>
#include <signal.h>

#include <opencv2/opencv.hpp> 

#include "nodes/navigation/LaneDetector.hpp"

void signal_callback_handler(int signum){
    std::cout << "Exiting..." << std::endl;
    exit(signum);
}

int main(){
    std::cout << "Starting..." << std::endl;

    LaneDetector ld = LaneDetector(true);

    signal(SIGINT, signal_callback_handler);

    while (1){
        ld.get_lane_params();
    }

    return EXIT_SUCCESS;
}