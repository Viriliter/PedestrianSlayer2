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
    msg_attr.mq_msgsize = 8192;
    msg_attr.mq_curmsgs = 0;
    
    std::cout << "createMsgQueue? " << std::endl;
    std::cout << createMsgQueue("/SlaveCommunicationTask", msg_attr) << std::endl;

    connectPort();
    serialPort->SetBaudRate(baudrate);
};

SlaveCommunicationTask::~SlaveCommunicationTask()
{
    std::cout << "Destructor of SlaveCommunicationTask" << std::endl;
    terminateTask();
    std::cout << "Task termination called" << std::endl;
    disconnectPort();
    std::cout << "Port disconnected" << std::endl;
    delete serialPort;
    std::cout << "Port deleted" << std::endl;
};

bool SlaveCommunicationTask::connectPort()
{
    if (serialPort == NULL)
        return false;
    serialPort->Open(portName);
    return isConnected;
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

std::pair<container::LinkedList<uint8_t>, communication::messages::MessagePacket> 
SlaveCommunicationTask::parseIncommingBytes(container::LinkedList<uint8_t> &rx_bytes, int byte_count)
{
    //Parses message according to predefined message structure. Message structure can be found in ../doc path
    communication::messages::MessagePacket packet;
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
                SPDLOG_INFO(pair.second.toString());
                char *buf = pair.second.toChar();

                SPDLOG_INFO(writeMsgQueue("/SlaveCommunicationTask", buf, pair.second.MsgSize));
                delete buf;
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