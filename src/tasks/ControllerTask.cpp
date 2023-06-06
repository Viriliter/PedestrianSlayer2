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
    // Get user inputs
    char queue_user_ctrl[MAX_MQ_MSG_SIZE+1];
    UINT16 queueSizeUserCtrl = readMsgQueue("/UserControlTask_out", queue_user_ctrl);
    if (queueSizeUserCtrl>0){
        // Start from index 2 since first two bytes are length of queue message
        comm::ipc::IPCMessage pMsg2 = comm::ipc::deserialize(queue_user_ctrl+2, queueSizeUserCtrl);
        auto raw_bytes = pMsg2.getPackageValue<std::vector<UINT8>>("Raw");
        
        std::cout << "----";
        for(auto &raw_byte:raw_bytes)
        {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int) raw_byte << " ";
        }
        std::cout << std::endl;
    }
    
    char queue_slave_comm_out[MAX_MQ_MSG_SIZE+1];
    UINT16 queueSizeSlaveCommOut = readMsgQueue("/SlaveCommunicationTask_out", queue_slave_comm_out);
    if (queueSizeSlaveCommOut>0){
        // Start from index 2 since first two bytes are length of queue message
        comm::ipc::IPCMessage pMsg2 = comm::ipc::deserialize(queue_slave_comm_out+2, queueSizeSlaveCommOut);
        auto raw_bytes = pMsg2.getPackageValue<std::vector<UINT8>>("Raw");
        
        std::cout << "----";
        for(auto &raw_byte:raw_bytes)
        {
            std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int) raw_byte << " ";
        }
        std::cout << std::endl;
    }

    if (lastMode!=currentMode){  // Mode transition
        switch(currentMode){
            case(IdleMode): myMode = modes::IdleMode(); break;
            case(StartupMode): myMode = modes::StartupMode(); break;
            case(TestMode): myMode = modes::TestMode(); break;
            case(UserMode): myMode = modes::UserMode(); break;
            case(AutoMode): myMode = modes::AutoMode(); break;
            case(TerminationMode): myMode = modes::TerminationMode(); break;
        }
    }
    else{
        myMode.do_job();
    }
    
    // Create IPC message for posix queue
    std::vector<UINT8> txMsg;
    comm::ipc::PayloadType in_payload;
    comm::ipc::insert_package<std::vector<UINT8>>(in_payload, "Raw", txMsg);

    comm::ipc::IPCMessage pMsg("TxMsg", in_payload);
    std::vector<UINT8> serializedMsg = pMsg.serialize();
    
    writeMsgQueue("/SlaveCommunicationTask_in", serializedMsg, serializedMsg.size());
};

void ControllerTask::afterTask(){
    SPDLOG_INFO("Task has been stopped.");

    closeMsgQueue();
    unlinkMsgQueue("/ControllerTask");
};