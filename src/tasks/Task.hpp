#ifndef TASK_HPP
#define TASK_HPP

#include <pthread.h>
#include <sys/mman.h>  // necessary for mlockall

#include <cstring>
#include <stdexcept>
#include <string>
#include <iostream>

namespace tasks{
    enum PRIORITY{
        VERY_LOW_PRIORITY_TASK,
        LOW_PRIORITY_TASK,
        NORMAL_PRIORITY_TASK,
        HIGH_PRIORITY_TASK,
        VERY_HIGH_PRIORITY_TASK
    };

    enum TASK_STATUS{
        IDLE_TASK,  // The task has not been run before 
        RUNNING_TASK,  // The task is running
        STOPPED_TASK,  // The task has been run at least once before but it is not running now.
    };

    class Task{
        public:
            size_t taskID = 0;
            int taskPeriod = -1;  // Defines period of task cycle
            PRIORITY taskPriority = PRIORITY::NORMAL_PRIORITY_TASK;  // Priorizes task respect to other tasks. Note that RT tasks have higher priorty regardless of the taskPriority
            int estimatedTaskDuration = -1;  // Defines estimated task duration of task in single cycle
            TASK_STATUS taskStatus = TASK_STATUS::IDLE_TASK;  // Defines task status 
            bool rtEnabled = false;  // Enables real-time task features
            bool cpuAffinityEnabled = false;  // Enables cpu affinity

            // Apply Singleton Pattern
            static Task& getInstance(){
                static Task instance; 
                return instance;
            };

            // Delete Copy constructor and assignment operator to prevent copy of object
            Task(Task const&) = delete;
            Task &operator=(Task const&) = delete;

            Task(){
            };

            Task(int taskID): taskID(taskID){};

            Task(int taskID, int taskPeriod): taskID(taskID), taskPeriod(taskPeriod){};

            Task(int taskID, int taskPeriod, PRIORITY taskPriority, int estimatedTaskDuration, bool rtEnabled, bool cpuAffinityEnabled): 
            taskID(taskID), taskPeriod(taskPeriod), taskPriority(taskPriority), estimatedTaskDuration(estimatedTaskDuration), rtEnabled(rtEnabled), cpuAffinityEnabled(cpuAffinityEnabled){};

            ~Task(){
                std::cout << "Task-" << std::to_string(taskID) << " is destroyed" << std::endl;
            }

            TASK_STATUS start(){};
            TASK_STATUS stop(){};
            virtual void run(){
                std::cout << "Base Run" << std::endl;
            };
    };

}


#endif  //TASK_HPP