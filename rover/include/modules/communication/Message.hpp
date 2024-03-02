#ifndef MESSAGE_HPP
#define MESSAGE_HPP 

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include <map>
#include <any>
#include <variant>
//#include "simdjson.h"
#include "nlohmann/json.hpp"
#include "utils/Container.hpp"
#include "utils/Types.hpp"

namespace comm::serial{
    class SerialMessagePacket{
        public:
            UINT8 SOM = 0x00;
            UINT8 MsgSize = 0x00;
            UINT8 MsgID = 0x00;
            container::LinkedList<UINT8> MessagePayload{};  // Store message payload in Linked List since its size may differ
            UINT8 Checksum = 0x00;

            SerialMessagePacket(){};

            SerialMessagePacket(std::vector<UINT8> &raw_message){
                SOM = raw_message.at(0);
                MsgSize = raw_message.at(1);
                MsgID = raw_message.at(2);
                for(int i=3; i<raw_message.size()-1; i++){
                    MessagePayload.push(raw_message.at(i));
                }
                Checksum = raw_message.at(raw_message.size()-1);
            }

            std::string toString(){
                std::string result;

                result = "SOM: " + std::to_string(SOM) + " " +
                         "MsgSize: " + std::to_string(MsgSize) + " " +
                         "MsgID: " + std::to_string(MsgID) + " " +
                         "Payload: " + MessagePayload.toString() +
                         "Checksum: " + std::to_string(Checksum);
                return result;
            };

            UINT8 calcChecksum(){
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

            void toLinkedBytes(container::LinkedList<UINT8> &linkedBytes){
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

            UINT8 *toUint8(){
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

            std::vector<UINT8> toVector(){
                std::vector<UINT8> buf;
                buf.push_back((UINT8) SOM);
                buf.push_back((UINT8) MsgSize);
                buf.push_back((UINT8) MsgID);
                std::vector<UINT8> vMessagePayload = MessagePayload.toVector<UINT8>();
                buf.insert(buf.end(), vMessagePayload.begin(), vMessagePayload.end());
                buf.push_back((UINT8) Checksum);
                return buf;
            };

    };

    enum SerialMessageID{
        ControlRoverID = 0x01,
        CalibrateRoverID = 0x02,
        RoverStateID = 0x81,
        RoverIMUID = 0x82,
        RoverErrorID = 0xFF
    };

    class SerialMessage{
        private:
            UINT8 payloadSize = 0x00;

        protected:
            UINT8 msgSize = 0x00;
            const UINT8 headerSize = 0x03;
            const UINT8 checksumSize = 0x01 ;
        
        public:
            std::string msgName;
            UINT8 msgSOM = 0xAA;
            UINT8 msgID = 0x00;
            
            container::LinkedList<uint8_t> msgPayload;

            virtual void parsePayload() = 0;

            void decodeMessage(SerialMessagePacket &messagePacket);

            std::vector<UINT8> encodeMessage();

            void setPayloadSize(UINT8 payloadSize_){
                payloadSize = payloadSize_;
                msgSize = headerSize + payloadSize + checksumSize;
            }

            UINT8 getPayloadSize(){
                return payloadSize;
            }
    };

    // region SerialMessages

    class ControlRover: public SerialMessage{
    public:
        // Message Payload
        UINT8 roverControl;
        INT8 steeringAngle;
        INT8 thrustDutyCycle;
        
        ControlRover(){
            // Message Header
            msgName = "ControlRover";
            msgID = SerialMessageID::ControlRoverID;
            setPayloadSize(3);
        };

        ControlRover(container::LinkedList<UINT8> &payload){
            // Message Header
            msgName = "ControlRover";
            msgID = SerialMessageID::ControlRoverID;
            setPayloadSize(3);
            msgPayload = payload;
            parsePayload();
        };

        void addPayload(UINT8 roverControl, INT8 steeringAngle, INT8 thrustDutyCycle){
            msgPayload.add(roverControl); 
            msgPayload.add(steeringAngle); 
            msgPayload.add(thrustDutyCycle);
        };
        
        void parsePayload(){
            roverControl = (UINT8) msgPayload[0];
            steeringAngle = (INT8) msgPayload[1];
            thrustDutyCycle = (INT8) msgPayload[2];
        };
    };

    class CalibrateRover: public SerialMessage{
    public:
        // Message Payload
        FLOAT thrustP;
        FLOAT thrustI;
        FLOAT thrustD;
        INT8 steeringOffset;

        CalibrateRover(){
            msgName = "CalibrateRover";
            msgID = SerialMessageID::CalibrateRoverID;
            setPayloadSize(13);
        };
        
                
        CalibrateRover(container::LinkedList<UINT8> &payload){
            msgName = "CalibrateRover";
            msgID = SerialMessageID::CalibrateRoverID;
            setPayloadSize(13);
            msgPayload = payload;
            parsePayload();
        };

        void addPayload(FLOAT thrustP, FLOAT thrustI, FLOAT thrustD, INT8 steeringOffset){
            msgPayload.add(thrustP); 
            msgPayload.add(thrustI);
            msgPayload.add(thrustD);
            msgPayload.add(steeringOffset);
        };

        void parsePayload(){
            thrustP = msgPayload(0, 4).toFloat();
            thrustI = msgPayload(4, 8).toFloat();
            thrustD = msgPayload(8, 12).toFloat();
            steeringOffset = msgPayload(12, 13).toInt8(); 
        };
    };

    class RoverState: public SerialMessage{
    public:
        // Message Payload
        UINT8 roverState;
        UINT8 msgCounter;

        RoverState(){
            msgName = "RoverState";
            msgID = SerialMessageID::RoverStateID;
            setPayloadSize(2);
        };
        
        RoverState(container::LinkedList<UINT8> &payload){
            msgName = "RoverState";
            msgID = SerialMessageID::RoverStateID;
            setPayloadSize(2);
            msgPayload = payload;
            parsePayload();
        };

        void addPayload(UINT8 roverControl, UINT8 steeringAngle, UINT8 thrustDutyCycle){
            msgPayload.add(roverState); 
            msgPayload.add(msgCounter); 
        };
        
        void parsePayload(){
            roverState = (UINT8) msgPayload[0];
            msgCounter = (UINT8) msgPayload[1];
        };  
    };

    class RoverIMU: public SerialMessage{
    public:
        // Message Payload
        FLOAT accelX;
        FLOAT accelY;
        FLOAT accelZ;
        FLOAT gyroX;
        FLOAT gyroY;
        FLOAT gyroZ;
        FLOAT magX;
        FLOAT magY;
        FLOAT magZ;

        RoverIMU(){
            msgName = "RoverIMU";
            msgID = SerialMessageID::RoverIMUID;
            setPayloadSize(36);
        };

        RoverIMU(container::LinkedList<UINT8> &payload){
            msgName = "RoverIMU";
            msgID = SerialMessageID::RoverIMUID;
            setPayloadSize(36);
            msgPayload = payload;
            parsePayload();
        };

        void addPayload(UINT8 roverControl, UINT8 steeringAngle, UINT8 thrustDutyCycle){
            msgPayload.add(accelX); 
            msgPayload.add(accelY); 
            msgPayload.add(accelZ);
            msgPayload.add(gyroX); 
            msgPayload.add(gyroY); 
            msgPayload.add(gyroZ);
            msgPayload.add(magX); 
            msgPayload.add(magY); 
            msgPayload.add(magZ);
        };
                
        void parsePayload(){
            accelX = msgPayload(0, 4).toFloat();
            accelY = msgPayload(4, 8).toFloat();
            accelZ = msgPayload(8, 12).toFloat();
            gyroX = msgPayload(12, 16).toFloat();
            gyroY = msgPayload(16, 20).toFloat();
            gyroZ = msgPayload(20, 24).toFloat();
            magX = msgPayload(24, 28).toFloat();
            magY = msgPayload(28, 32).toFloat();
            magZ = msgPayload(32, 36).toFloat();
        };  
    };

    class RoverError: public SerialMessage{
    public:
        // Message Payload
        UINT16 errorCode;
        
        RoverError(){
            msgName = "RoverError";
            msgID = SerialMessageID::RoverErrorID;
            setPayloadSize(2);
        };

        RoverError(container::LinkedList<UINT8> &payload){
            msgName = "RoverError";
            msgID = SerialMessageID::RoverErrorID;
            setPayloadSize(2);
            msgPayload = payload;
            parsePayload();
        };

        void addPayload(UINT8 roverControl, UINT8 steeringAngle, UINT8 thrustDutyCycle){
            msgPayload.add(errorCode & 0x00FF);
            msgPayload.add((errorCode & 0xFF00)>>8);
        };

        void parsePayload(){
            errorCode = msgPayload(0, 2).toUint16();
        }
    };

    // endregion
    typedef std::variant<SerialMessage*, CalibrateRover*, RoverState*, RoverIMU*, RoverError*> SerialMessageTypes;
}

#endif  // MESSAGE_HPP