#ifndef TASK_SCHEDULER
#define TASK_SCHEDULER

#include <vector>
#include <stack>
#include <algorithm> 

#include "tasks/Task.hpp"

namespace tasks {

enum class SCHEDULER_STATUS{
    STARTED,
    STOPPED
};

class TaskScheduler{
    private:
        SCHEDULER_STATUS mSchedulerStatus = SCHEDULER_STATUS::STOPPED;
        std::vector<AbstractTask*> mTaskList;

    public:
        TaskScheduler() = default;
        ~TaskScheduler() 
        { 

        };

        /**
         * @brief This function adds Task.
         * @param[in] task Task 
        */
        void addTask(AbstractTask *task);

        /**
         * @brief This function removes Task according to provided taskID.
         * @param[in] taskID taskID of Task that will be removed
        */
        void removeTask(size_t taskID);

        /**
         * @brief This function starts tasks in the list.
        */
        void startTasks();

        /**
         * @brief This function terminates all tasks in the list.
        */
        void terminateTasks();
};
}

#endif  // TASK_SCHEDULER