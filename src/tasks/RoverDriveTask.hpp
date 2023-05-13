#ifndef ROVERDRIVETASK_HPP
#define ROVERDRIVETASK_HPP

#include "Task.hpp"

namespace tasks{
    class RoverDriveTask: public Task{
        public:
            RoverDriveTask(): Task(){};
            RoverDriveTask(int taskID): Task(taskID){};
            RoverDriveTask(int taskID, int taskPeriod): Task(taskID, taskPeriod){};
            RoverDriveTask(int taskID, int taskPeriod, PRIORITY taskPriority, int estimatedTaskDuration, bool rtEnabled, bool cpuAffinityEnabled): 
            Task(taskID, taskPeriod, taskPriority, estimatedTaskDuration, rtEnabled, cpuAffinityEnabled){};

            void run() override{
                std::cout << "RoverDriveTask running..." << std::endl;
            };

    };
}

#endif  //ROVERDRIVETASK_HPP