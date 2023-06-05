#include "ControllerTask.hpp"

using namespace tasks;

ControllerTask::ControllerTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns, int64_t runtime_ns, int64_t deadline_ns, std::vector<size_t> cpu_affinity)
: Task(task_name, policy, task_priority, period_ns, runtime_ns, deadline_ns, cpu_affinity){
    mq_attr msg_attr;

    msg_attr.mq_flags = 0;
    msg_attr.mq_maxmsg = 10;
    msg_attr.mq_msgsize = 8192;
    msg_attr.mq_curmsgs = 0;

    // epoch(uint64_t) - sourceTask - 
    
    // First delete then create new  message queue
    unlinkMsgQueue("/ControllerTask");
    createMsgQueue("/ControllerTask", msg_attr, msg_attr);
};

void ControllerTask::beforeTask(){
 
};

void ControllerTask::runTask(){
    char queue[MAX_MQ_MSG_SIZE+1];
    UINT16 queueSize = readMsgQueue("/SlaveCommunicationTask_out", queue);
    if (queueSize>0){
        // Start from index 2 since first two bytes are length of queue message
        comm::ipc::IPCMessage pMsg2 = comm::ipc::deserialize(queue+2, queueSize);
        auto raw_bytes = pMsg2.getPackageValue<std::vector<UINT8>>("Raw");
        
        std::cout << "----";
        for(auto &raw_byte:raw_bytes)
        {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int) raw_byte << " ";
        }
        std::cout << std::endl;
    }
};

void ControllerTask::afterTask(){
    SPDLOG_INFO("Task has been stopped.");

    closeMsgQueue();
    unlinkMsgQueue("/ControllerTask");
};