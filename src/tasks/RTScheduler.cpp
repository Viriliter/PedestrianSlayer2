#include "RTScheduler.hpp"

using namespace tasks;

RTScheduler::RTScheduler(){
    // Create Tasks
    //taskID | taskPeriod | taskPriority | estimatedTaskDuration | rtEnabled | cpuAffinityEnabled
    NavigationTask *navigationTask = new NavigationTask(0);
    RoverDriveTask *roverDriveTask = new RoverDriveTask(1);
    SlaveCommunicationTask *slaveCommunicationTask = new SlaveCommunicationTask(2);
    UserControlTask *userControlTask = new UserControlTask(3);
    
    // Add tasks into list
    
    addTask(navigationTask);
    addTask(roverDriveTask);
    addTask(slaveCommunicationTask);
    addTask(userControlTask);
    
};

RTScheduler::~RTScheduler(){
    std::cout << "RTScheduler destructor is called" << std::endl;
    // Destroys tasks in reverse order (Last added task removes first)
    for(int i=TASK_COUNT-1; i>=0; i--){
        removeTask(taskList[i]);
    }
    delete[] taskList;
};

SCHEDULER_STATUS RTScheduler::startScheduler(){
    bool flag = true;
    for(int i=0; i<TASK_COUNT; i++){
        Task *ptrTask = taskList[i]; 
        if (ptrTask == NULL) continue;

        taskList[i]->run();
        //flag &= (bool) (startTask(taskList[i]) == TASK_STATUS::RUNNING_TASK);
    }
    schedulerStatus = (flag)? SCHEDULER_STATUS::Started: schedulerStatus;
    return schedulerStatus;
};

SCHEDULER_STATUS RTScheduler::stopScheduler(){
    bool flag = true;
    // Stops tasks in reverse order (Last added task stops first)
    for(int i=TASK_COUNT-1; i>=0; i--){
        if (taskList[i] == NULL) continue;

        flag &= (bool) (stopTask(taskList[i]) != TASK_STATUS::RUNNING_TASK);
    }
    schedulerStatus = (flag)? SCHEDULER_STATUS::Stopped: schedulerStatus;
    return schedulerStatus;
};

TASK_STATUS RTScheduler::startTask(Task *task){
    return task->start();
};

TASK_STATUS RTScheduler::stopTask(Task *task){
    return task->stop();   
};

void RTScheduler::addTask(Task *task){
    if (availableTaskSpace<0) throw "Task list is full. Either delete unncessary tasks or increase TASK_COUNT";
    for(int i=0; i<TASK_COUNT; i++){
        //Check whether task is NULL and add to NULL pointer
        if (taskList[i] == NULL) {
            taskList[i] = task;
            break;
        }
    }
    availableTaskSpace--;
};

void RTScheduler::removeTask(Task *task){
    for(int i=0; i<TASK_COUNT; i++){
        //Check whether task is NULL 
        if (taskList[i] == NULL) {
            delete taskList[i];
             return;
        } 

        // If the task is still running, stop it first
        //if (taskList[i]->taskStatus == TASK_STATUS::RUNNING_TASK) taskList[i]->stop();
        delete taskList[i];
    }
    availableTaskSpace++;
};

void RTScheduler::removeTask(int taskID){
    for(int i=0; i<TASK_COUNT; i++){
        if (taskList[i] == NULL) {
            continue;
        }

        if (taskList[i]->taskID == taskID){
            // If the task is still running, stop it first
            //if (taskList[i]->taskStatus == TASK_STATUS::RUNNING_TASK) taskList[i]->stop();
            delete taskList[i];
        }
    }
    availableTaskSpace++;
};
