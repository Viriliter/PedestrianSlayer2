#include "tasks/UserControlTask.hpp"

using namespace sw::redis;

using namespace tasks;

UserControlTask::UserControlTask(TaskConfig *taskConfig): Task(taskConfig){
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

void UserControlTask::beforeTask() noexcept{
    std::cout << REDIS_URI << std::endl;
    std::cout << redis.ping() << std::endl;

};

void UserControlTask::myTask() noexcept{

    //SPDLOG_INFO("Thrust: " + redis.get("thrust").value());
    //SPDLOG_INFO("Steering: " + redis.get("steering").value());

    //std::cin >> in;
    //std::cout << ">>>>> " << in << std::endl;
};


void UserControlTask::afterTask() noexcept{
    std::cout << "UserControlTask is stopped" << std::endl;
    
    closeMsgQueue();
    unlinkMsgQueue("/UserControlTask");
};