#include "modules/communication/Message.hpp"

using namespace comm::serial;

void SerialMessage::decodeMessage(SerialMessagePacket &messagePacket){
    if (msgSize==0) throw "Empty message cannot be decoded";

    // Check Header
    if (msgSOM != messagePacket.SOM) throw("SOM does not match");
    msgID = messagePacket.MsgID;
    
    SerialMessage *msg;
    // TODO implement visitor pattern for message specific decoding
    /*
    switch(msgID){
        case(SerialMessageID::ControlRoverID): msg = ControlRover(); break;
        case(SerialMessageID::CalibrateRoverID): msg = CalibrateRover(); break;
        case(SerialMessageID::RoverStateID): msg = RoverState(); break;
        case(SerialMessageID::RoverIMUID): msg = RoverIMU(); break;
        case(SerialMessageID::RoverErrorID): msg = RoverError(); break;
        default: throw("Undefined MsgID"); break;
    }
    */
    

    if (msgSize != messagePacket.MsgSize) throw("MsgSize does not match");

    // Check Checksum
    if (!messagePacket.checkChecksum()) throw ("CRC dismatch");               

    // Parse payload

    parsePayload();
};

UINT8 decodeMessage(SerialMessagePacket &messagePacket, SerialMessageTypes &decodedMsg){
    if (messagePacket.MsgSize==0) throw "Empty message cannot be decoded";
    
    // Check Header
    if (messagePacket.SOM != 0xAA) throw("SOM does not match");
    
    // Check Checksum
    if (!messagePacket.checkChecksum()) throw ("CRC dismatch");

    // TODO implement visitor pattern for message specific decoding
    if(messagePacket.MsgID == SerialMessageID::ControlRoverID){
        ControlRover *msg = new ControlRover(messagePacket.MessagePayload); decodedMsg = msg;
    }
    else if(messagePacket.MsgID == SerialMessageID::CalibrateRoverID){
        CalibrateRover *msg = new CalibrateRover(messagePacket.MessagePayload); decodedMsg = msg;
    }
    else if(messagePacket.MsgID == SerialMessageID::RoverStateID){
        RoverState *msg = new RoverState(messagePacket.MessagePayload); decodedMsg = msg;
    }
    else if(messagePacket.MsgID == SerialMessageID::RoverIMUID){
        RoverIMU *msg = new RoverIMU(messagePacket.MessagePayload); decodedMsg = msg;
    }
    else if(messagePacket.MsgID == SerialMessageID::RoverErrorID){
        RoverError *msg = new RoverError(messagePacket.MessagePayload); decodedMsg = msg;
    }
    else{
        SerialMessage *msg = NULL; decodedMsg = msg;
    }
    return messagePacket.MsgID;
}

std::vector<UINT8> SerialMessage::encodeMessage(){
    if (msgSize==0) throw ("Empty message cannot be encoded");

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
    return message.toVector();
};
