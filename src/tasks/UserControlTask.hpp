#ifndef USERCONTROLTASK_HPP
#define USERCONTROLTASK_HPP


#include "Task.hpp"

namespace tasks{
    class UserControlTask: public Task{
        public:
            UserControlTask(): Task(){};
            UserControlTask(int taskID): Task(taskID){};
            UserControlTask(int taskID, int taskPeriod): Task(taskID, taskPeriod){};
            UserControlTask(int taskID, int taskPeriod, PRIORITY taskPriority, int estimatedTaskDuration, bool rtEnabled, bool cpuAffinityEnabled): 
            Task(taskID, taskPeriod, taskPriority, estimatedTaskDuration, rtEnabled, cpuAffinityEnabled){};

            void run() override{
                std::cout << "UserControlTask running..." << std::endl;
            };

    };
}


#endif  //USERCONTROLTASK_HPP