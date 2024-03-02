#ifndef UBXDRIVER_HPP
#define UBXDRIVER_HPP

#include <string>
#include <utility>

#include <spdlog/spdlog.h>

#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>

#include "config.hpp"
#include "utils/Timing.hpp"
#include "utils/Container.hpp"
#include "utils/Types.hpp"
#include "utils/Miscs.hpp"

using namespace LibSerial;

namespace navigation{

    struct UBX{
        UINT16 header = 0x0000;
        UINT16 id = 0x0000;
        UINT16 size = 0x0000;
        container::LinkedList<UINT8> payload;
        UINT16 checksum = 0x0000;
    };

    struct UBX_NAV_PVT{
        UINT32 iTOW = 0;
        UINT16 year = 0;
        UINT8 month = 0;
        UINT8 day = 0;
        UINT8 hour = 0;
        UINT8 min = 0;
        UINT8 sec = 0;
        UINT8 valid = 0;
        UINT32 tAcc = 0;
        UINT32 nano = 0;
        UINT8 fixType = 0;
        UINT8 flags = 0;
        UINT8 reserved1 = 0;
        UINT8 numSV = 0;
        float lon = 0;
        float lat = 0;
        float height = 0;
        float hMSL = 0;
        float hAcc = 0;
        float vAcc = 0;
        float velN = 0;
        float velE = 0;
        float velD = 0;
        float gSpeed = 0;
        float heading = 0;
        float sAcc = 0;
        float headingAcc = 0;
        float pDOP = 0;
        UINT16 reserved2 = 0;
        UINT32 reserved3 = 0;
    };

    class UBXDriver{
        private:
            std::string portName = SLAVE_PORTNAME;
            BaudRate baudrate;
            SerialPort *serialPort = new SerialPort();
            bool isConnected;
            
            container::LinkedList<UINT8> rx_remaining;
            int timeoutSlave = 100;  // ms

        public:
            UBXDriver(){

            };
            
            UBXDriver(std::string portName, BaudRate baudrate): portName(portName), baudrate(baudrate){

            };

            ~UBXDriver(){
                delete serialPort;
            };

            bool connect(std::string portName, BaudRate baudrate);
            
            bool connect();

            void disconnect();

            bool IsOpen();

            std::pair<container::LinkedList<UINT8>, container::LinkedList<UINT8>> parse_ubx(container::LinkedList<UINT8> &buf);

            UINT16 calc_checksum(container::LinkedList<UINT8> &rx_bytes);

            void read_ubx(UBX &ubx);

            UBX_NAV_PVT read_ubx_nav_pvt(UBX &ubx);
    };

}

#endif  // UBXDRIVER_HPP