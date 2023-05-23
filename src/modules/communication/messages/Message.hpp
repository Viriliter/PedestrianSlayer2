#ifndef MESSAGE_HPP
#define MESSAGE_HPP 

#include <string>
#include <iostream>
#include <memory>
#include <sstream>

#include <map>
#include <any>
//#include "simdjson.h"
#include "../../../third-party/nlohmann/json.hpp"
#include "../../../utils/container.h"
#include "../../../utils/timing.h"

namespace communication::messages{
    class SerialMessagePacket{
        public:
            uint8_t SOM = 0;
            uint8_t MsgSize = 0;
            uint8_t MsgID = 0;
            container::LinkedList<uint8_t> MessagePayload{};  // Store message payload in Linked List since its size may differ
            uint8_t Checksum = 0;

            std::string toString(){
                std::string result;

                result = "SOM: " + std::to_string(SOM) + " " +
                         "MsgSize: " + std::to_string(MsgSize) + " " +
                         "MsgID: " + std::to_string(MsgID) + " " +
                         "Payload: " + MessagePayload.toString() +
                         "Checksum: " + std::to_string(Checksum);
                return result;
            };

            uint8_t calcChecksum(){
                // Calculates checksum of the message by summing bytes. Note that in checksum calculation "Checksum" field is not included.
                int checksum = 0;
                checksum = SOM + MsgSize + MsgID;

                for (size_t i=0; i< MessagePayload.Count(); i++){
                    checksum += (int) MessagePayload[i];
                }
                return checksum % 255;
            }

            bool checkChecksum(){
                // Calculates the checksum and compares with the "Checksum" field.
                uint8_t calculatedChecksum = calcChecksum();
                return Checksum  == calculatedChecksum;
            }

            void toLinkedBytes(container::LinkedList<uint8_t> &linkedBytes){
                // Deep copy of MessagePayload to referenced linked list
                linkedBytes = MessagePayload;
                linkedBytes.push(MsgID);  // Push value to tail
                linkedBytes.push(MsgSize);  // Push value to tail
                linkedBytes.push(SOM);  // Push value to tail
                linkedBytes.add(Checksum);  // Add value to header
            };

            char *toChar(){
                char *buf = new char[MessagePayload.Count()+1];
                buf[0] = (char) SOM;
                buf[1] = (char) MsgSize;
                buf[2] = (char) MsgID;
                int i;
                for (i=0; i<MessagePayload.Count(); i++){
                    buf[i+3] += (char) MessagePayload[i];
                }
                buf[i+3] = (char) Checksum;
                return buf;
            };

            uint8_t *toUint8(){
                uint8_t *buf = new uint8_t[MessagePayload.Count()+1];
                buf[0] = (uint8_t) SOM;
                buf[1] = (uint8_t) MsgSize;
                buf[2] = (uint8_t) MsgID;
                int i;
                for (i=0; i<MessagePayload.Count(); i++){
                    buf[i+3] += (uint8_t) MessagePayload[i];
                }
                buf[i+3] = (uint8_t) Checksum;
                return buf;
            };
 
    };
    
    class SerialMessage{
        public:
            std::string msgName;
            uint8_t msgSOM = 0xAA;
            uint8_t msgSize;
            uint8_t msgID;
            
            
            void decodeMessage(SerialMessagePacket &messagePacket){
                messagePacket.MessagePayload;
            };

            void encodeMessage(){
                container::LinkedList<uint8_t> *tx_message = new container::LinkedList<uint8_t>();
                SerialMessagePacket message{};
                message.SOM = msgSOM;
                message.MsgSize = msgSize;
                message.MsgID = msgID;
                message.Checksum = message.calcChecksum();
                message.toLinkedBytes(*tx_message);
                delete tx_message;

            };
    };

    class ControlRover: public SerialMessage{
        ControlRover(){
            msgName = "ControlRover";
            uint8_t msgSize;
            uint8_t msgID;
        };
    };

    class CalibrateRover: public SerialMessage{
        CalibrateRover(){
            msgName = "CalibrateRover";
        };     
    };

    class RoverState: public SerialMessage{
        RoverState(){
            msgName = "RoverState";
        };      
    };

    class RoverIMU: public SerialMessage{
        RoverIMU(){
            msgName = "RoverIMU";
        };
    };

    class RoverError: public SerialMessage{
        RoverError(){
            msgName = "RoverError";

        };
    };

    struct messageQueue{
        unsigned long epoch = 0;
        std::string source = "";
        nlohmann::json data;
    };

    template<typename T>
    class PosixMessage{
    private:
        messageQueue message;

    public:
        nlohmann::json j;

        PosixMessage(std::map<std::string, T> &m_){
            message.epoch = timing::getEpoch();
            message.source = "Source";
            message.data = m_;
            
            // {"epoch": {}, "source": {}, "data": {}}  
        };

        std::string serialize(){
            /*
            std::string j_string = nlohmann::to_string(j);
            std::cout << "JSON OBJECT" << std::endl;
            std::cout << j_string << std::endl;
            std::cout << "===========" << std::endl;
            return j_string;
            */
           return "";
        };

        void deserialize(std::string message){

        };


    };

}

#endif  // MESSAGE_HPP