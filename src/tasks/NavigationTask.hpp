#ifndef NAVIGATIONTASK_HPP
#define NAVIGATIONTASK_HPP

#include "Task.hpp"

namespace tasks{
    class NavigationTask: public Task{
        public:
            NavigationTask(): Task(){};
            NavigationTask(int taskID): Task(taskID){};
            NavigationTask(int taskID, int taskPeriod): Task(taskID, taskPeriod){};
            NavigationTask(int taskID, int taskPeriod, PRIORITY taskPriority, int estimatedTaskDuration, bool rtEnabled, bool cpuAffinityEnabled): 
            Task(taskID, taskPeriod, taskPriority, estimatedTaskDuration, rtEnabled, cpuAffinityEnabled){};

            void run() override{
                std::cout << "NavigationTask running..." << std::endl;
            };

    };
}


#endif  //NAVIGATIONTASK_HPP