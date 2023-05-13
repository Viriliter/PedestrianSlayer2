#ifndef RTSCHEDULER_HPP
#define RTSCHEDULER_HPP

#include <vector>

#include "../config.hpp"
#include "Task.hpp"
#include "../utils/cactus_rt/app.h"

// Include Tasks
#include "NavigationTask.hpp"
#include "RoverDriveTask.hpp"
#include "SlaveCommunicationTask.hpp"
#include "UserControlTask.hpp"
#include "NavigationTask.hpp"

namespace tasks{

    enum SCHEDULER_STATUS{
        Started,
        Stopped
    };

    class RTScheduler{
        private:
            int availableTaskSpace = TASK_COUNT;
            Task **taskList = new Task*[TASK_COUNT];

            void setTaskID(Task &task);
            void setTaskPriority(Task &task);

            void addTask(Task *task);

            void removeTask(Task *task);
            void removeTask(int TaskID);

            TASK_STATUS startTask(Task *task);
            TASK_STATUS stopTask(Task *task);

        public:
            SCHEDULER_STATUS schedulerStatus;
            RTScheduler();
            ~RTScheduler();

            RTScheduler(const RTScheduler&) = delete;
            RTScheduler &operator=(const RTScheduler&) = delete;

            SCHEDULER_STATUS startScheduler();
            SCHEDULER_STATUS stopScheduler();

    };

}


#endif  //RTSCHEDULER_HPP