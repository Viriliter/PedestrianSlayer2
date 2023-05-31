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
#include "../../../third-party/nlohmann/json.hpp"
#include "../../../utils/container.h"
#include "../../../utils/timing.h"
#include "../../../utils/types.h"

namespace communication::serial_messages{
    class SerialMessagePacket{
        public:
            UINT8 SOM = 0x00;
            UINT8 MsgSize = 0x00;
            UINT8 MsgID = 0x00;
            container::LinkedList<UINT8> MessagePayload{};  // Store message payload in Linked List since its size may differ
            UINT8 Checksum = 0x00;

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

    class SerialMessage{
        UINT8 msgSize = 0x00;
        const UINT8 headerSize = 0x03;
        const UINT8 checksumSize = 0x01 ;
        
        public:
            std::string msgName;
            UINT8 msgSOM = 0xAA;
            UINT8 msgID = 0x00;
            UINT8 payloadSize = 0x00;
            
            container::LinkedList<uint8_t> msgPayload;

            void decodeMessage(SerialMessagePacket &messagePacket){
                if (msgSize==0) throw "Empty message cannot be decoded";

            };

            void encodeMessage(){
                if (msgSize==0) throw "Empty message cannot be encoded";

                //std::unique_ptr<container::LinkedList<UINT8>> tx_message(new container::LinkedList<UINT8>());
                SerialMessagePacket message{};

                // Header
                message.SOM = msgSOM;
                msgSize = headerSize + payloadSize + checksumSize;
                message.MsgSize = msgSize;
                message.MsgID = msgID;
                // Payload
                message.MessagePayload = msgPayload;

                // Checksum
                message.Checksum = message.calcChecksum();
            };
    };

    class ControlRover: public SerialMessage{
    public:
        // Message Payload
        UINT8 roverControl;
        INT8 steeringAngle;
        INT8 thrustDutyCycle;
        
        ControlRover(){
            // Message Header
            msgName = "ControlRover";
            msgID = 0x01;
            payloadSize = 3;
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
        // Message Payload
        FLOAT thrustP;
        FLOAT thrustI;
        FLOAT thrustD;
        INT8 steeringOffset;

        CalibrateRover(){
            msgName = "CalibrateRover";
            msgID = 0x02;
            payloadSize = 13;
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
        // Message Payload
        UINT8 roverState;
        UINT8 msgCounter;

        RoverState(){
            msgName = "RoverState";
            msgID = 0x81;
            payloadSize = 2;
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
            msgID = 0x82;
            payloadSize = 36;
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
        // Message Payload
        UINT16 errorCode;

        RoverError(){
            msgName = "RoverError";
            msgID = 0xFF;
            payloadSize = 2;
        };

        void addPayload(UINT8 roverControl, UINT8 steeringAngle, UINT8 thrustDutyCycle){
            msgPayload.add(errorCode & 0x00FF);
            msgPayload.add((errorCode & 0xFF00)>>8);
        };

        void parsePayload(){
            errorCode = msgPayload(0, 2).toUint16();
        }

    };
}

namespace communication::posix_messages{
    enum PosixDataTypes{
        pUINT8,
        pUINT16,
        pUINT32,
        pUINT64,
        pINT8,
        pINT16,
        pINT32,
        pINT64,
        pFLOAT,
        pDOUBLE,
        pLONG,
        pLONG_LONG,
        pVECTOR_UINT8        
    };

    typedef std::string PackageValueType;
    typedef std::vector<UINT8> PackageValue;
    //typedef std::variant<std::vector<UINT8>> PackageValue;

    //typedef std::pair<PackageValueType, PackageValue> PackageType; // <data type, data value> 
    typedef PackageValue PackageType; // <data type, data value> 
    typedef std::map<std::string, PackageType> PayloadType; // <data key, data pair>

    //std::map<std::string, std::vector<uint8_t>> posixDataType;

    struct messageStruct{
        unsigned long epoch;
        std::string keyword;
        PayloadType payload;
    };

    class PosixMessage{
    private:
        nlohmann::json obj_;
    public:
        unsigned long epoch;
        std::string keyword;
        PayloadType payload;

        
        PosixMessage(nlohmann::json &obj): obj_(obj){};

        PosixMessage(std::string keyword, PayloadType payload): keyword(keyword), payload(payload){
            epoch = timing::getEpoch();
        };

        std::vector<uint8_t> serialize(){
            obj_["_epoch"] = epoch;
            obj_["_keyword"] = keyword;
        
            for(auto const &package:payload){
                std::string key = (std::string) package.first;
                // Check whether first character is underscore. (Underscore keys are omitted by default)
                if (key[0] == '_') continue;

                obj_[key] = package.second;
                /*
                PackageType value = std::make_pair("-", package.second);
                obj_[key] = value;
                */
            }
            std::vector<uint8_t> v = nlohmann::json::to_msgpack(obj_);

            return v;
            //return nlohmann::to_string(obj_);
        };
        
        template<typename T>
        T getPackageValue(const std::string &packageName){
            for(auto &item:obj_.items()){
                if (item.key()[0] == '_') continue;

                if (packageName == item.key()){
                    //obj_.get<decltype(T)>();
                    return static_cast<T>(obj_[packageName]);
                }
            }
            throw "No Package named '" + packageName + "' exists in " +  keyword + " message";
        }
    };
    /*
    static std::any dataConversion(PosixDataTypes type, std::any value){
        switch(type){
            case(pUINT8): return std::any_cast<UINT8>(value);
            case(pUINT16): return std::any_cast<UINT16>(value);
            case(pUINT32): return std::any_cast<UINT32>(value);
            case(pUINT64): return std::any_cast<UINT64>(value);
            case(pINT8): return std::any_cast<INT8>(value);
            case(pINT16): return std::any_cast<INT16>(value);
            case(pINT32): return std::any_cast<INT32>(value);
            case(pINT64): return std::any_cast<INT64>(value);
            case(pFLOAT): return std::any_cast<FLOAT>(value);
            case(pDOUBLE): return std::any_cast<DOUBLE>(value);
            case(pLONG): return std::any_cast<LONG>(value);
            case(pLONG_LONG): return std::any_cast<LONG_LONG>(value);
            case(pVECTOR_UINT8): return std::any_cast<std::vector<UINT8>>(value);
            default: throw "Invalid type keyword";
        };
    };
    */
   
    inline PosixMessage deserialize(std::vector<UINT8> rawMessage){
        nlohmann::json obj_ = nlohmann::json::from_msgpack(rawMessage);

        PosixMessage message(obj_);
        for (auto &item: obj_.items()){
            if (item.key() == "_epoch") message.epoch = static_cast<unsigned long>(item.value());
            else if (item.key() == "_keyword") message.keyword = item.value();
            else continue;
        }
        /*
        for (auto &item: obj_.items()){
            if (item.key() == "_epoch") message.epoch = static_cast<unsigned long>(item.value());
            else if (item.key() == "_keyword") message.keyword = item.value();
            else {
                message.payload.insert(std::pair<std::string, PackageType>(item.key(), get_to(item.value())));

                    
                //nlohmann::json packages = nlohmann::json::from_msgpack(item.value());
                //for(auto const &package:packages.items()){
                //    PackageType my_package = std::make_pair(package.key(), package.value());
                //    message.payload.insert(std::pair<std::string, PackageType>(item.key(), my_package));
                //    //message.data[el.key()] = el.value();
                //    //message.data.insert(std::pair<std::string, VariantTypes>(el.key(), el.value()));
                //}
            }
        }
        */
        return message;
    };
}

#endif  // MESSAGE_HPP