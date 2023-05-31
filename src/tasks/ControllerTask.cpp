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
    if (getAvailableMsgQueue("/SlaveCommunicationTask_out")>0){
        char buf[MAX_MQ_MSG_SIZE+1] = "";
        
        readMsgQueue("/SlaveCommunicationTask_out", buf, MAX_MQ_MSG_SIZE+1);
        std::string r = "";

        SPDLOG_INFO("Received message: " + r);
    }
    /*    
    std::map<std::string, int> my_map{{"CPU", 10}, {"GPU", 15}, {"RAM", 20}};
    communication::messages::PosixMessage<int> m(my_map);
    
    std::string my_string = m.serialize();
    SPDLOG_INFO(my_string);
    */
};

void ControllerTask::afterTask(){
    std::cout << "ControllerTask is stopped" << std::endl;
    
    closeMsgQueue();
    unlinkMsgQueue("/ControllerTask");
};