#include "Message.hpp"

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
