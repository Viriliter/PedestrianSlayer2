#ifndef MESSAGE_HPP
#define MESSAGE_HPP 

#include <string>
#include <iostream>
#include <memory>
#include <sstream>

#include "../utils/container.h"

namespace communication::messages{
    class MessagePacket{
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
                char *buf = new char[MessagePayload.Count()];
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
    };
    
    class Message{
        public:
            std::string msgName;
            uint8_t msgSOM = 0xAA;
            uint8_t msgSize;
            uint8_t msgID;
            
            
            void decodeMessage(MessagePacket &messagePacket){
                messagePacket.MessagePayload;
            };

            void encodeMessage(){
                container::LinkedList<uint8_t> *tx_message = new container::LinkedList<uint8_t>();
                MessagePacket message{};
                message.SOM = msgSOM;
                message.MsgSize = msgSize;
                message.MsgID = msgID;
                message.Checksum = message.calcChecksum();
                message.toLinkedBytes(*tx_message);
                delete tx_message;

            };
    };

    class ControlRover: public Message{
        ControlRover(){
            msgName = "ControlRover";
            uint8_t msgSize;
            uint8_t msgID;
        };
    };

    class CalibrateRover: public Message{
        CalibrateRover(){
            msgName = "CalibrateRover";
        };     
    };

    class RoverState: public Message{
        RoverState(){
            msgName = "RoverState";
        };      
    };

    class RoverIMU: public Message{
        RoverIMU(){
            msgName = "RoverIMU";
        };
    };

    class RoverError: public Message{
        RoverError(){
            msgName = "RoverError";
        };
    };
}

#endif  // MESSAGE_HPP