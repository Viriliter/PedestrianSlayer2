#include "Message.h"

using namespace message;

SerialMessage *SerialMessage::decodeMessage(SerialMessagePacket &messagePacket){
    if (msgSize==0) throw "Empty message cannot be decoded";

    // Check Header
    if (msgSOM != messagePacket.SOM) throw("SOM does not match");
    msgID = messagePacket.MsgID;

    if (msgSize != messagePacket.MsgSize) throw("MsgSize does not match");

    // Check Checksum
    if (!messagePacket.checkChecksum()) throw ("CRC dismatch");               

    // Parse payload
    SerialMessage *msg;
    // TODO implement visitor pattern for message specific decoding
    switch(msgID){
        case(SerialMessageID::ControlRoverID): msg = new ControlRover(messagePacket.MessagePayload); break;
        case(SerialMessageID::CalibrateRoverID): msg = new CalibrateRover(messagePacket.MessagePayload); break;
        case(SerialMessageID::RoverStateID): msg = new RoverState(messagePacket.MessagePayload); break;
        case(SerialMessageID::RoverIMUID): msg = new RoverIMU(messagePacket.MessagePayload); break;
        case(SerialMessageID::RoverErrorID): msg = new RoverError(messagePacket.MessagePayload); break;
        default: SerialMessage *msg = NULL; break;
    }
    return msg;

};

std::vector<UINT8> SerialMessage::encodeMessage(){
    if (msgSize==0) throw ("Empty message cannot be encoded");

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


void SerialMessage::encodeMessage(container::LinkedList<UINT8> &out){
    if (msgSize==0) throw ("Empty message cannot be encoded");

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
    
    message.toLinkedList(out);
};
