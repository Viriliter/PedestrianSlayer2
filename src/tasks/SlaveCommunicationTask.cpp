#include "SlaveCommunicationTask.hpp"

#include <string>
#include "../utils/timing.h"
#include "../utils/container.h"

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
        throw std::runtime_error("Cannot connect to port");
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

std::pair<container::LinkedList<uint8_t>, communication::serial_messages::SerialMessagePacket> 
SlaveCommunicationTask::parseIncommingBytes(container::LinkedList<uint8_t> &rx_bytes, int byte_count)
{
    //Parses message according to predefined message structure. Message structure can be found in ../doc path
    communication::serial_messages::SerialMessagePacket packet;
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
        container::LinkedList<uint8_t> rx_buffer;
        try
        {
            int available_byte_count = serialPort->GetNumberOfBytesAvailable();
            for (int i = 0; i < available_byte_count; i++)
            {
                char rx_byte;
                serialPort->ReadByte(rx_byte, timeoutSlave);
                rx_buffer.add((uint8_t) rx_byte);
            }
            auto pair = parseIncommingBytes(rx_buffer, available_byte_count);
            rx_remaining += pair.first;

            if (pair.second.MsgSize>0){
                //SPDLOG_INFO(pair.second.toString());
                //std::cout << "Write operation started" << std::endl;
                std::string rx_msg = pair.second.toChar();
                
                //char *msg = new char[MAX_MQ_MSG_SIZE-strlen(buf)]{1};
                //strcat(msg, buf);

                  // Create a posix message to send to message queue
                std::vector<uint8_t> rxMsg = pair.second.toVector();

                communication::posix_messages::PayloadType in_payload{{"Raw", rxMsg}};
                communication::posix_messages::PosixMessage pMsg("RxMsg", in_payload);

                std::vector<uint8_t> serializedMsg = pMsg.serialize();
                for(auto &v:serializedMsg)
                {
                    std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int) v << " ";
                }
                std::cout << std::endl;

                communication::posix_messages::PosixMessage pMsg2 = communication::posix_messages::deserialize(serializedMsg);
                std::cout << "Epoch: " << std::to_string(pMsg2.epoch) << std::endl;
                std::cout << "Keyword: " << pMsg2.keyword << std::endl;

                std::vector<uint8_t> raw_bytes = pMsg2.getPackageValue<std::vector<uint8_t>>("Raw");
                for(auto &byte:raw_bytes)
                {
                    std::cout << " ---" << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int) byte << " ";
                }

                std::cout << std::endl;
                std::cout << "==========" << std::endl;

                //strcat(buf, msg, strlen(buf));
                //writeMsgQueue("/SlaveCommunicationTask_out", msg, MAX_MQ_MSG_SIZE);
                // Delete the pointer since the array is created on heap
                //delete buf;
            }
        }
        catch (ReadTimeout &err)
        {
            err.what();
        }
        //timing::sleep(100, timing::timeFormat::formatMiliseconds);
    }
    else{
        terminateTask();
    }
};

void SlaveCommunicationTask::afterTask(){
    std::cout << "SlaveCommunicationTask is stopped" << std::endl;
    disconnectPort();
    delete serialPort;

    closeMsgQueue();
    unlinkMsgQueue("/SlaveCommunicationTask");
};