#ifndef SLAVECOMMUNICATIONTASK_HPP
#define SLAVECOMMUNICATIONTASK_HPP
#include "Task.hpp"

#include <string>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>

#include "../config.hpp"
#include "../modules/communication/Message.hpp"
#include "../modules/communication/IPCMessage.hpp"
#include "../utils/timing.h"
#include "../utils/container.h"
#include "../utils/types.h"
#include "../utils/miscs.h"


using namespace LibSerial;

namespace tasks{
    class SlaveCommunicationTask: public Task{
        
        private:
            std::string portName = SLAVE_PORTNAME;
            BaudRate baudrate;
            SerialPort *serialPort = new SerialPort();
            bool isConnected;
            int timeoutSlave = SLAVE_CON_TIMEOUT;

            container::LinkedList<UINT8> rx_remaining;

        public:
            SlaveCommunicationTask(std::string task_name, SCHEDULE_POLICY policy, TASK_PRIORITY task_priority, int64_t period_ns=0, int64_t runtime_ns=0, int64_t deadline_ns=0, std::vector<size_t> cpu_affinity={});

            bool connectPort();
            void disconnectPort();
            void setPortName(std::string _portName);
            void setBaudrate(BaudRate _baudrate);
            std::pair<container::LinkedList<uint8_t>, comm::serial::SerialMessagePacket> 
            parseIncommingBytes(container::LinkedList<uint8_t> &rx_bytes, int byte_count);
        
        protected:
            void beforeTask() override;  // overwrite
            void runTask() override;  // overwrite
            void afterTask() override;  // overwrite
            
    };
}


#endif  //SLAVECOMMUNICATIONTASK_HPP