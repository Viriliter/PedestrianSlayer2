#include <iostream>
#include <string>
#include <signal.h>

#include <opencv2/opencv.hpp> 
#include "spdlog/spdlog.h"
//#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
//#include "spdlog/fmt/ostr.h" // support for user defined types

#include "tasks/RTScheduler.hpp"
#include "modules/navigation/LaneDetector.hpp"

tasks::RTScheduler *scheduler = new tasks::RTScheduler();

void signal_callback_handler(int signum){
    std::cout << "Exiting..." << std::endl;
    //scheduler = NULL;
    delete scheduler;
    exit(signum);
}

int main(){
    signal(SIGINT, signal_callback_handler);
    SPDLOG_DEBUG("Starting application");

    //spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    std::cout << "Starting..." << std::endl;

    scheduler->startScheduler();

    /*
    LaneDetector ld = LaneDetector(true);


    while (1){
        ld.getLaneParams();
    }
    */

    return EXIT_SUCCESS;
}