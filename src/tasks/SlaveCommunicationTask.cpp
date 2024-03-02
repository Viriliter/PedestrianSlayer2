#include "SlaveCommunicationTask.hpp"

using namespace LibSerial;
using namespace tasks;

SlaveCommunicationTask::SlaveCommunicationTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns, int64_t runtime_ns, int64_t deadline_ns, std::vector<size_t> cpu_affinity)
: Task(task_name, policy, task_priority, period_ns, runtime_ns, deadline_ns, cpu_affinity){
    switch (SLAVE_BAUD)
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
    
    mq_attr msg_attr;

    msg_attr.mq_flags = 0;
    msg_attr.mq_maxmsg = 10;
    msg_attr.mq_msgsize = MAX_MQ_MSG_SIZE;
    msg_attr.mq_curmsgs = 0;

    // epoch(uint64_t) - sourceTask - 

    // First delete then create new  message queue
    unlinkMsgQueue("/SlaveCommunicationTask");
    createMsgQueue("/SlaveCommunicationTask", msg_attr, msg_attr);

    // Connect to serial port
    bool is_connected = connectPort();
    if (is_connected){
        serialPort->SetBaudRate(baudrate);
    }
    else{
        std::string exc_msg = "Cannot connect to port" + (std::string) portName;
        SPDLOG_ERROR(exc_msg);
        //throw std::runtime_error("Cannot connect to port");
    }
};

bool SlaveCommunicationTask::connectPort()
{
    try{
        if (serialPort == NULL)
            return false;
        serialPort->Open(portName);
        return true;
    }
    catch (LibSerial::OpenFailed &ex){
        std::string exc_msg = "Cannot connect to port" + (std::string) ex.what();
        SPDLOG_ERROR(exc_msg);
        return false;
    }
};

void SlaveCommunicationTask::disconnectPort()
{
    if (serialPort == NULL)
        return;

    if (serialPort->IsOpen())
        serialPort->Close();
};

void SlaveCommunicationTask::setPortName(std::string _portName)
{
    std::string portName = _portName;
};

void SlaveCommunicationTask::setBaudrate(BaudRate _baudrate)
{
    BaudRate baudrate = _baudrate;
};

std::pair<container::LinkedList<UINT8>, comm::serial::SerialMessagePacket> 
SlaveCommunicationTask::parseIncommingBytes(container::LinkedList<UINT8> &rx_bytes, int byte_count)
{
    //Parses message according to predefined message structure. Message structure can be found in ../doc path
    comm::serial::SerialMessagePacket packet;
    container::LinkedList<uint8_t> remaining;
    if (rx_bytes.Count() > 3){  // Check header
        if (rx_bytes[0] == 0xAA)
        { // Check SoM
            uint8_t msgSize = rx_bytes[1];
            if (msgSize <= rx_bytes.Count()){
                packet.SOM = rx_bytes[0];
                packet.MsgSize = msgSize;
                packet.MsgID = rx_bytes[2];
                packet.Checksum = rx_bytes[msgSize-1];
                // Fill payload
                for (int i = 3; i < packet.MsgSize-1; i++)
                {
                    packet.MessagePayload.add(rx_bytes[i]);
                }
                // Fill remaining bytes (Store excessive bytes for next parsing)
                for (int i = packet.MsgSize; i < rx_bytes.Count(); i++)
                {
                    remaining.add(rx_bytes[i]);
                }
                return std::make_pair(remaining, packet);
            }
            else{
                // Fill remaining bytes (Store bytes for other parse)
                for (int i = 0; i < rx_bytes.Count(); i++)
                {
                    remaining.add(rx_bytes[i]);
                }
                return std::make_pair(remaining, packet);
            }
        }
        else{
            // Fill remaining bytes (Omit first byte since it is not SoM)
            for (int i = 1; i < rx_bytes.Count(); i++)
            {
                remaining.add(rx_bytes[i]);
            }
            return std::make_pair(remaining, packet);
        }
    }
    else{  // Fill remaining bytes
        for (int i = 0; i < byte_count; i++)
        {
            remaining.add(rx_bytes[i]);
        }
        return std::make_pair(remaining, packet);
    }
};

void SlaveCommunicationTask::beforeTask(){
    SPDLOG_INFO("SlaveCommunicationTask has just started.");
};

void SlaveCommunicationTask::runTask(){
    if (serialPort->IsOpen())
    {
        //Read received serial messages
        container::LinkedList<UINT8> rx_buffer;
        try
        {
            int available_byte_count = serialPort->GetNumberOfBytesAvailable();

            for (size_t i = 0; i < available_byte_count; i++)
            {
                char rx_byte;
                serialPort->ReadByte(rx_byte, timeoutSlave);
                rx_buffer.add((UINT8) rx_byte);
            }
            auto pair = parseIncommingBytes(rx_buffer, available_byte_count);
            rx_remaining += pair.first;

            if (pair.second.MsgSize>0){
                  // Create a posix message to send to message queue
                std::vector<UINT8> rxMsg = pair.second.toVector();

                // Create IPC message for posix queue
                comm::ipc::PayloadType in_payload;
                comm::ipc::insert_package<std::vector<UINT8>>(in_payload, "Raw", rxMsg);
                comm::ipc::IPCMessage pMsg("RxMsg", in_payload);
                std::vector<UINT8> serializedMsg = pMsg.serialize();

                writeMsgQueue("/SlaveCommunicationTask_out", serializedMsg, serializedMsg.size());
            }
        }
        catch (ReadTimeout &err)
        {
            err.what();
        }

        //Send serial messages
        
        try{
            while (true){
                char queue_slave_comm_in[MAX_MQ_MSG_SIZE+1];
                UINT16 queueSize = readMsgQueue("/SlaveCommunicationTask_in", queue_slave_comm_in);
                if (queueSize>0){
                    // Start from index 2 since first two bytes are length of queue message
                    comm::ipc::IPCMessage pMsg2 = comm::ipc::deserialize(queue_slave_comm_in+2, queueSize);
                    auto tx_bytes = pMsg2.getPackageValue<std::vector<UINT8>>("Raw");
                    for (auto const &tx_byte: tx_bytes){
                        serialPort->Write(tx_bytes);
                    }

                    serialPort->FlushOutputBuffer();

                    std::string hex_string = miscs::Dec2HexString<uint8_t>(tx_bytes);
                    //SPDLOG_INFO("Tx----> " + hex_string);

                }
                else break;
            }
        }
        catch (std::exception &err){
            err.what();
        }
        
        //timing::sleep(100, timing::timeFormat::formatMiliseconds);
    }
    else{
        terminateTask();
    }
};

void SlaveCommunicationTask::afterTask(){
    SPDLOG_INFO("Task has been stopped.");

    disconnectPort();
    delete serialPort;

    closeMsgQueue();
    unlinkMsgQueue("/SlaveCommunicationTask");
};