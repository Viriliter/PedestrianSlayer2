#include "tasks/TaskScheduler.hpp"

using namespace tasks;

void TaskScheduler::addTask(AbstractTask *task){
    mTaskList.push_back(task);
};

void TaskScheduler::removeTask(size_t taskID) {

    int i = 0;
    AbstractTask *ptrTask = NULL;
    for(auto &task: mTaskList){
        if (task == NULL) continue;
        
        if (task->taskID() == taskID){
            ptrTask = task;
            task->terminateTask();
            break;
        }
    }
    // Remove task from the list
    mTaskList.erase(std::remove_if(mTaskList.begin(), mTaskList.end(), 
                                    [ptrTask] (AbstractTask* ptr) { return ptr == ptrTask; }),
                                    mTaskList.end());
};

void TaskScheduler::startTasks() {
    mSchedulerStatus = SCHEDULER_STATUS::STARTED;

    for( auto &task: mTaskList){
        if (task==NULL) continue;
        task->start();
    }

    for( auto &task: mTaskList){
        if (task==NULL) continue;
        task->join();
    }
};

void TaskScheduler::terminateTasks() {
    for( auto &task: mTaskList){
        if (task==NULL) continue;
        task->terminateTask();
        
        while(task->TaskStatus() != TASK_STATUS::STOPPED_TASK){};
        task = NULL;
    }
    mSchedulerStatus = SCHEDULER_STATUS::STOPPED;
};