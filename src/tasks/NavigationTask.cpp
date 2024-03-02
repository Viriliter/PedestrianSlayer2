#include "NavigationTask.hpp"

using namespace LibSerial;
using namespace tasks;

NavigationTask::NavigationTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns, int64_t runtime_ns, int64_t deadline_ns, std::vector<size_t> cpu_affinity)
: Task(task_name, policy, task_priority, period_ns, runtime_ns, deadline_ns, cpu_affinity){

    mq_attr msg_attr;

    msg_attr.mq_flags = 0;
    msg_attr.mq_maxmsg = 10;
    msg_attr.mq_msgsize = MAX_MQ_MSG_SIZE;
    msg_attr.mq_curmsgs = 0;

    // epoch(uint64_t) - sourceTask - 

    // First delete then create new  message queue
    unlinkMsgQueue("/NavigationTask");
    createMsgQueue("/NavigationTask", msg_attr, msg_attr);

    BaudRate baudrate;
    switch (UBX_BAUD)
    {
    case (9600):
        baudrate = BaudRate::BAUD_9600;
        break;
    case (115200):
        baudrate = BaudRate::BAUD_115200;
        break;
    case (230400):
        baudrate = BaudRate::BAUD_230400;
        break;
    default:
        baudrate = BaudRate::BAUD_9600;
        break;
    };

    bool is_ubx_connected = ubx_driver.connect(UBX_PORT, baudrate);
    if (!is_ubx_connected){
        //throw std::runtime_error("Cannot connect to port");
    }

};


void NavigationTask::beforeTask(){
    SPDLOG_INFO("NavigationTask has just started.");
};

void NavigationTask::runTask(){
    if (ubx_driver.IsOpen())
    {
        UBX ubx;
        ubx_driver.read_ubx(ubx);
        
        UBX_NAV_PVT ubx_nav_pvt = ubx_driver.read_ubx_nav_pvt(ubx);
        
        SPDLOG_INFO("numSV: " + std::to_string(ubx_nav_pvt.numSV) + 
                    " lat: " + std::to_string(ubx_nav_pvt.lat) +
                    " lon: " + std::to_string(ubx_nav_pvt.lon) +
                    " height: " + std::to_string(ubx_nav_pvt.height));
        
    }
    else{
        terminateTask();
    }
};

void NavigationTask::afterTask(){
    SPDLOG_INFO("Task has been stopped.");

    ubx_driver.disconnect();

    closeMsgQueue();
    unlinkMsgQueue("/SlaveCommunicationTask");
};