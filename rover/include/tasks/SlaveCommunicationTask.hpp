#ifndef SLAVECOMMUNICATIONTASK_HPP
#define SLAVECOMMUNICATIONTASK_HPP

#include <string>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>

#include "config.hpp"
#include "tasks/Task.hpp"
#include "modules/communication/Message.hpp"
#include "modules/communication/IPCMessage.hpp"
#include "utils/Timing.hpp"
#include "utils/Container.hpp"
#include "utils/Types.hpp"
#include "utils/Miscs.hpp"


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
            SlaveCommunicationTask(TaskConfig *taskConfig=NULL);

            bool connectPort();
            void disconnectPort();
            void setPortName(std::string _portName);
            void setBaudrate(BaudRate _baudrate);
            std::pair<container::LinkedList<uint8_t>, comm::serial::SerialMessagePacket> 
            parseIncommingBytes(container::LinkedList<uint8_t> &rx_bytes, int byte_count);
        
        protected:
            virtual void beforeTask() noexcept override;  // overwrite
            virtual void myTask() noexcept override;  // overwrite
            virtual void afterTask() noexcept override;  // overwrite
            
    };
}


#endif  //SLAVECOMMUNICATIONTASK_HPP