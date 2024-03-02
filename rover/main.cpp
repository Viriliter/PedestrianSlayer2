#include <iostream>
#include <string>
#include <signal.h>
#include <memory>

#include <opencv2/opencv.hpp> 
#include "spdlog/spdlog.h"

#include "tasks/TaskScheduler.hpp"
#include "modules/navigation/LaneDetector.hpp"

tasks::TaskScheduler *scheduler = new tasks::TaskScheduler();

void signal_callback_handler(int signum){
    SPDLOG_INFO("Exiting application...");
    //scheduler = NULL;
    delete scheduler;
    SPDLOG_INFO("Exitted.");
    
    exit(signum);
}

int main(){
    signal(SIGINT, signal_callback_handler);
    int heap_size_ = 5;



    SPDLOG_INFO(__cplusplus);
    SPDLOG_INFO("Starting application...");

    //spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    scheduler->startTasks();

    /*
    LaneDetector ld = LaneDetector(true);


    while (1){
        ld.getLaneParams();
    }
    */

    return EXIT_SUCCESS;
}