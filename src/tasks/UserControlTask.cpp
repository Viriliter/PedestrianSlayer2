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
    //std::cout << "Read operation started" << std::endl;
    //std::cout << "----" << std::endl;
    while (getAvailableMsgQueue("/SlaveCommunicationTask_out")>0){
        char buf[MAX_MQ_MSG_SIZE+1] = "";
        
        readMsgQueue("/SlaveCommunicationTask_out", buf, MAX_MQ_MSG_SIZE+1);
        std::string r = "";
        for(int i=0; i<sizeof(buf); i++){
            r += std::to_string((uint8_t) buf[i]) + " ";
        }
        //std::cout << r << std::endl;
        SPDLOG_INFO("Received message: " + r);

        /*
        std::cout << "Received message: ";
        for(int i=0; i<sizeof(buf); i++){
        {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << " ";
        }
        std::cout << std::endl;
        */

    }
    //std::cout << "----" << std::endl;
    //std::cout << "Read operation finished" << std::endl;

}


void UserControlTask::afterTask(){
    std::cout << "UserControlTask is stopped" << std::endl;
    
    closeMsgQueue();
    unlinkMsgQueue("/UserControlTask");
};