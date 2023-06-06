#include "UserControlTask.hpp"

#include <string>

#include "../utils/timing.h"
#include "../utils/container.h"

using namespace tasks;

UserControlTask::UserControlTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns, int64_t runtime_ns, int64_t deadline_ns, std::vector<size_t> cpu_affinity)
: Task(task_name, policy, task_priority, period_ns, runtime_ns, deadline_ns, cpu_affinity){
    mq_attr msg_attr;

    msg_attr.mq_flags = 0;
    msg_attr.mq_maxmsg = 10;
    msg_attr.mq_msgsize = MAX_MQ_MSG_SIZE;
    msg_attr.mq_curmsgs = 0;

    // epoch(uint64_t) - sourceTask - 
    
    // First delete then create new  message queue
    unlinkMsgQueue("/UserControlTask");
    createMsgQueue("/UserControlTask", msg_attr, msg_attr);
};

void UserControlTask::runTask(){
    //char in;
    std::cout << "Enter input:"<< std::endl;
    //std::cin >> in;
    //std::cout << ">>>>> " << in << std::endl;
}


void UserControlTask::afterTask(){
    std::cout << "UserControlTask is stopped" << std::endl;
    
    closeMsgQueue();
    unlinkMsgQueue("/UserControlTask");
};