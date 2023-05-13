#ifndef SLAVECOMMUNICATIONTASK_HPP
#define SLAVECOMMUNICATIONTASK_HPP

#include  <string>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>

#include "../config.hpp"
#include "Task.hpp"
#include "../modules/communication/messages/Message.hpp"

using namespace LibSerial ;

namespace tasks{
    class SlaveCommunicationTask: public Task{
        
        private:
            std::string portName = SLAVE_PORTNAME;
            BaudRate baudrate;
            SerialPort *serialPort = new SerialPort();
            bool isConnected;
            int timeoutSlave = SLAVE_CON_TIMEOUT;


        public:
            SlaveCommunicationTask();
            SlaveCommunicationTask(int taskID): Task(taskID){};
            SlaveCommunicationTask(int taskID, int taskPeriod): Task(taskID, taskPeriod){};
            SlaveCommunicationTask(int taskID, int taskPeriod, PRIORITY taskPriority, int estimatedTaskDuration, bool rtEnabled, bool cpuAffinityEnabled): 
            Task(taskID, taskPeriod, taskPriority, estimatedTaskDuration, rtEnabled, cpuAffinityEnabled){};

            ~SlaveCommunicationTask();

            bool connectPort();
            void disconnectPort();
            void setPortName(std::string _portName);
            void setBaudrate(BaudRate _baudrate);
            std::pair<communication::messages::LinkedBytes<uint8_t>, communication::messages::MessagePacket> 
            parseIncommingBytes(communication::messages::LinkedBytes<uint8_t> &rx_bytes, int byte_count);

            void run();
    };
}


#endif  //SLAVECOMMUNICATIONTASK_HPP