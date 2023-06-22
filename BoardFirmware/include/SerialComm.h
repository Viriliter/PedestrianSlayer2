#ifndef SERIALCOMM_H
#define SERIALCOMM_H

#include <string>
#include <stdexcept>
#include <map>

#include "UARTDriver.h"
#include "Message.h"
#include "container.h"
#include "types.h"

using namespace HAL;
namespace comm{
    class SerialComm{
    private:
        uart_inst_t *portName = NULL;
        HAL::BaudrateEnum baudrate;
        bool isConnected;

        container::LinkedList<UINT8> rx_remaining;

        bool connect(){
            driver.setUartID(portName);
            driver.setBaudrate(baudrate);
            driver.connect();
            isConnected = true;
            return true;
        };

        bool reconnect(){
            driver.setUartID(portName);
            driver.setBaudrate(baudrate);
            driver.reconnect();
            isConnected = true;
            return true;
        };

    public:
        HAL::UARTDriver driver;
        SerialComm(){};

        ~SerialComm(){};

        bool connectPort(uart_inst_t *_portName, HAL::BaudrateEnum _baudrate){
            portName = (uart_inst_t *)_portName;
            baudrate = _baudrate;
            return connect();
        };

        bool reconnectPort(uart_inst_t *_portName, HAL::BaudrateEnum _baudrate){
            portName = (uart_inst_t *) _portName;
            baudrate = _baudrate;
            return reconnect();
        };

        void disconnectPort(){
            // No implementation
        };

        void setPortName(uart_inst_t *_portName){
            portName = (uart_inst_t *) _portName;
            driver.setUartID(portName);

        };

        void setBaudrate(HAL::BaudrateEnum _baudrate){
            baudrate = _baudrate;
            driver.setBaudrate(baudrate);
        };

        std::pair<container::LinkedList<UINT8>, message::SerialMessagePacket> 
        parseIncommingBytes(container::LinkedList<UINT8> &rx_bytes)
        {
            //Parses message according to predefined message structure. Message structure can be found in ../doc path
            message::SerialMessagePacket packet;
            container::LinkedList<UINT8> remaining;
            if (rx_bytes.Count() > 3){  // Check header
                if (rx_bytes[0] == 0xAA)
                { // Check SoM
                    UINT8 msgSize = rx_bytes[1];
                    if (msgSize <= rx_bytes.Count()){
                        packet.SOM = rx_bytes[0];
                        packet.MsgSize = msgSize;
                        packet.MsgID = rx_bytes[2];
                        packet.Checksum = rx_bytes[msgSize-1];
                        // Fill payload
                        for (size_t i = 3; i < packet.MsgSize-1; i++)
                        {
                            packet.MessagePayload.add(rx_bytes[i]);
                        }
                        // Fill remaining bytes (Store excessive bytes for next parsing)
                        for (size_t i = packet.MsgSize; i < rx_bytes.Count(); i++)
                        {
                            remaining.add(rx_bytes[i]);
                        }
                        return std::make_pair(remaining, packet);
                    }
                    else{
                        // Fill remaining bytes (Store bytes for other parse)
                        for (size_t i = 0; i < rx_bytes.Count(); i++)
                        {
                            remaining.add(rx_bytes[i]);
                        }
                        return std::make_pair(remaining, packet);
                    }
                }
                else{
                    // Fill remaining bytes (Omit first byte since it is not SoM)
                    for (size_t i = 1; i < rx_bytes.Count(); i++)
                    {
                        remaining.add(rx_bytes[i]);
                    }
                    return std::make_pair(remaining, packet);
                }
            }
            else{  // Fill remaining bytes
                for (size_t i = 0; i < rx_bytes.Count(); i++)
                {
                    remaining.add(rx_bytes[i]);
                }
                return std::make_pair(remaining, packet);
            }
        };

        void readIncomming(std::vector<message::SerialMessage*> &rx_msgs){
            
            if (driver.isAvailable())
            {
                //Read received serial messages
                container::LinkedList<UINT8> rx_buffer;
                try
                {
                    while(driver.isAvailable()){
                        driver.read(rx_buffer, 1);

                        auto pair = parseIncommingBytes(rx_buffer);
                        rx_remaining += pair.first;

                        if (pair.second.MsgSize>0){
                            message::SerialMessage *ptr_message;
                            // Decode serial message package
                            ptr_message->decodeMessage(pair.second);
                            rx_msgs.push_back(ptr_message);
                        }
                    }
                }
                catch (...)
                {
                    
                }
            }
        };

        void writeOutgoing(message::SerialMessage &tx_msg){
            container::LinkedList<UINT8> tx_bytes;
            tx_msg.encodeMessage(tx_bytes);
            driver.write(tx_bytes);
        };

    };
};


#endif  // SERIALCOMM_H