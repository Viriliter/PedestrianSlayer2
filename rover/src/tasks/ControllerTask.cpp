#include "tasks/ControllerTask.hpp"

using namespace tasks;

ControllerTask::ControllerTask(TaskConfig *taskConfig): Task(taskConfig){
    mq_attr msg_attr;

    msg_attr.mq_flags = 0;
    msg_attr.mq_maxmsg = 10;
    msg_attr.mq_msgsize = MAX_MQ_MSG_SIZE;
    msg_attr.mq_curmsgs = 0;

    // epoch(uint64_t) - sourceTask - 
    
    // First delete then create new  message queue
    unlinkMsgQueue("/ControllerTask");
    createMsgQueue("/ControllerTask", msg_attr, msg_attr);
};

void ControllerTask::beforeTask() noexcept{
    std::cout << REDIS_URI << std::endl;
    std::cout << redis.ping() << std::endl;
    //redis.set("key", "val");
};

void ControllerTask::myTask() noexcept{
    // Get user inputs over redis
    auto inMode = redis.get("mode");
    auto inThrust = redis.get("thrust");
    auto inSteering = redis.get("steering");

    if (inThrust && inSteering){
        //SPDLOG_INFO("Thrust: " + *inThrust);
        //SPDLOG_INFO("Steering: " + *inSteering);
        
        UINT8 roverControl = 0b00000111;
        UINT8 thrust = (std::stoi(inThrust.value()) + 100) * 1.27;
        UINT8 steering = (std::stoi(inSteering.value()) + 100) * 1.27;

        // Create Serial message
        auto msgControlRover = comm::serial::ControlRover();
        msgControlRover.addPayload(roverControl, steering, thrust);
        auto txMsg = msgControlRover.encodeMessage(); 


        // Create IPC message for posix queue
        comm::ipc::PayloadType in_payload;
        comm::ipc::insert_package<std::vector<UINT8>>(in_payload, "Raw", txMsg);
        comm::ipc::IPCMessage ipcMsgControlRover("TxMsg", in_payload);
        std::vector<UINT8> serializedMsg = ipcMsgControlRover.serialize();

        writeMsgQueue("/SlaveCommunicationTask_in", serializedMsg, serializedMsg.size());
    }

    // Get serial rx messages from slave board
    while (true){
        char queue_slave_comm_out[MAX_MQ_MSG_SIZE+1];
        UINT16 queueSizeSlaveCommOut = readMsgQueue("/SlaveCommunicationTask_out", queue_slave_comm_out);
        if (queueSizeSlaveCommOut>0){
            // Start from index 2 since first two bytes are length of queue message
            comm::ipc::IPCMessage ipcMsgRxSerial = comm::ipc::deserialize(queue_slave_comm_out+2, queueSizeSlaveCommOut);
            auto raw_rx_serial = ipcMsgRxSerial.getPackageValue<std::vector<UINT8>>("Raw");
            
            std::string hex_string = miscs::Dec2HexString<uint8_t>(raw_rx_serial);
            //SPDLOG_INFO("Rx----> " + hex_string);
            comm::serial::SerialMessagePacket rx_serial_package(raw_rx_serial);

            switch (rx_serial_package.MsgID){
                case(comm::serial::SerialMessageID::ControlRoverID): 
                    //comm::serial::ControlRover msg = comm::serial::ControlRover(rx_serial_package.MessagePayload); 
                    break;
                case(comm::serial::SerialMessageID::CalibrateRoverID): 
                    //comm::serial::CalibrateRoverID msg = comm::serial::CalibrateRoverID(rx_serial_package.MessagePayload); 
                    break;
                case(comm::serial::SerialMessageID::RoverStateID): 
                    //comm::serial::RoverStateID msg = comm::serial::RoverStateID(rx_serial_package.MessagePayload); 
                    break;
                case(comm::serial::SerialMessageID::RoverIMUID): 
                    //comm::serial::RoverIMUID msg = comm::serial::RoverIMUID(rx_serial_package.MessagePayload); 
                    break;
                case(comm::serial::SerialMessageID::RoverErrorID): 
                    //comm::serial::RoverErrorID msg = comm::serial::RoverErrorID(rx_serial_package.MessagePayload); 
                    break;
            }

        }
        else break;
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

void ControllerTask::afterTask() noexcept{
    //SPDLOG_INFO("Task has been stopped.");

    closeMsgQueue();
    unlinkMsgQueue("/ControllerTask");
};