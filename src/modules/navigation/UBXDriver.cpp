#include "UBXDriver.hpp"

using namespace navigation;

bool UBXDriver::connect(std::string portName, BaudRate baudrate){
    this->portName = portName;
    this->baudrate = baudrate;

    bool is_connected = connect();
    if (is_connected)
        serialPort->SetBaudRate(baudrate);
    return is_connected;
}

bool UBXDriver::connect(){
    try{
        if (serialPort == NULL)
            return false;
        serialPort->Open(portName);
        return true;
    }
    catch (LibSerial::OpenFailed &ex){
        std::string exc_msg = "Cannot connect to port" + (std::string) ex.what();
        SPDLOG_ERROR(exc_msg);
        return false;
    }
}

void UBXDriver::disconnect(){
    if (serialPort == NULL)
        return;

    if (serialPort->IsOpen())
        serialPort->Close();
}

bool UBXDriver::IsOpen(){
    return serialPort->IsOpen();
}

std::pair<container::LinkedList<UINT8>, container::LinkedList<UINT8>> UBXDriver::parse_ubx(container::LinkedList<UINT8> &buf){
    if (buf.Count() > 4){
        if (buf[0] == 0xB5 && buf[1] == 0x62){
            UINT16 msg_length = buf(4,6).toUint16() + 8;  // add header and checksum
            if (buf.Count() >= msg_length){
                return std::pair<container::LinkedList<UINT8>, container::LinkedList<UINT8>>(buf(0, msg_length), buf(msg_length, buf.Count()));
            }
            else{
                container::LinkedList<UINT8> null_list;
                return std::pair<container::LinkedList<UINT8>, container::LinkedList<UINT8>>(null_list ,buf);
            }
        }
        else{
            container::LinkedList<UINT8> null_list;
            buf.removeFirst();
            return std::pair<container::LinkedList<UINT8>, container::LinkedList<UINT8>>(null_list ,buf);
        }
    }
    else{
        container::LinkedList<UINT8> null_list;
        return std::pair<container::LinkedList<UINT8>, container::LinkedList<UINT8>>(null_list ,buf);
    }
}

UINT16 UBXDriver::calc_checksum(container::LinkedList<UINT8> &rx_bytes){
    UINT8 CK_A = 0;
    UINT8 CK_B = 0;
    
    for (int i=4; i<rx_bytes.Count()-2; i++){
        CK_A = (CK_A + rx_bytes[i]) % 0xFF;
        CK_B = (CK_B + CK_A) % 0xFF;
        
    }
    return (UINT16) (CK_A << 8) || CK_B;
}

void UBXDriver::read_ubx(UBX &ubx){
        //Read received serial messages
    container::LinkedList<UINT8> rx_buffer;
    try
    {
        int available_byte_count = serialPort->GetNumberOfBytesAvailable();

        for (size_t i = 0; i < available_byte_count; i++)
        {
            char rx_byte;
            serialPort->ReadByte(rx_byte, timeoutSlave);
            rx_buffer.add((UINT8) rx_byte);
        }
        
        auto pair = parse_ubx(rx_buffer);
        rx_remaining += pair.second;

        if (pair.first.Count() > 0){
            ubx.header = pair.first(0,2).toUint16L();
            ubx.id = pair.first(2,4).toUint16L();
            ubx.size = pair.first(4,6).toUint16L();
            pair.first(6,-2, ubx.payload);
            ubx.checksum = pair.first(-2,pair.first.Count()).toUint16L();
        }
        else{
            
        }        

    }
    catch (ReadTimeout &err)
    {
        err.what();
    }
}

UBX_NAV_PVT UBXDriver::read_ubx_nav_pvt(UBX &ubx){
    UBX_NAV_PVT ubx_nav_pvt;
    if (ubx.id == 0x0107){  // NAV_PVT
                
        ubx_nav_pvt.iTOW = ubx.payload(0,4).toUint32();  //  ms GPS time of week of the navigation epoch.
        ubx_nav_pvt.year = ubx.payload(4,6).toUint16();  //  Year (UTC)
        ubx_nav_pvt.month = ubx.payload(6,7).toUint8();  //  Month, range 1..12 (UTC)
        ubx_nav_pvt.day = ubx.payload(7,8).toUint8();  //  Day of month, range 1..31 (UTC)
        ubx_nav_pvt.hour = ubx.payload(8,9).toUint8();  //  Hour of day, range 0..23 (UTC)
        ubx_nav_pvt.min = ubx.payload(9,10).toUint8();  //  Minute of hour, range 0..59 (UTC)
        ubx_nav_pvt.sec = ubx.payload(10,11).toUint8();  //  Seconds of minute, range 0..60 (UTC)
        ubx_nav_pvt.valid = ubx.payload(11,12).toUint8();  //  Validity Flags (see graphic below)
        ubx_nav_pvt.tAcc = ubx.payload(12,16).toUint32();  //  ns Time accuracy estimate (UTC)
        ubx_nav_pvt.nano = ubx.payload(16,20).toUint32();  //  ns Fraction of second, range -1e9 .. 1e9 (UTC)
        ubx_nav_pvt.fixType = ubx.payload(20,21).toUint8();  //  GNSSfix Type, range 0..5
        ubx_nav_pvt.flags = ubx.payload(21,22).toUint8();  //  Fix Status Flags (see graphic below)
        ubx_nav_pvt.reserved1 = ubx.payload(22,23).toUint8();  //  Reserved
        ubx_nav_pvt.numSV = ubx.payload(23,24).toUint8();  //  Number of satellites used in Nav Solution
        ubx_nav_pvt.lon = ((float) ubx.payload(24,28).toUint32()) * (float) 1e-7; // deg Longitude
        ubx_nav_pvt.lat = ((float) ubx.payload(28,32).toUint32()) * (float) 1e-7;  // deg Latitude
        ubx_nav_pvt.height = (float) ubx.payload(32,36).toUint32();  // mm Height above Ellipsoid
        ubx_nav_pvt.hMSL = (float) ubx.payload(36,40).toUint32();  // mm Height above mean sea level
        ubx_nav_pvt.hAcc = (float) ubx.payload(40,44).toUint32();  // mm Horizontal Accuracy Estimate
        ubx_nav_pvt.vAcc = (float) ubx.payload(44,48).toUint32();  // mm Vertical Accuracy Estimate
        ubx_nav_pvt.velN = (float) ubx.payload(48,52).toUint32();  // mm/s NED north velocity
        ubx_nav_pvt.velE = (float) ubx.payload(52,56).toUint32();  // mm/s NED north velocity
        ubx_nav_pvt.velD = (float) ubx.payload(56,60).toUint32();  // mm/s NED north velocity
        ubx_nav_pvt.gSpeed = (float) ubx.payload(60,64).toUint32();  //  mm/s Ground Speed (2-D)
        ubx_nav_pvt.heading = ((float) ubx.payload(64,68).toUint32()) * (float) 1e-5;  // #deg Heading of motion 2-D
        ubx_nav_pvt.sAcc = (float) ubx.payload(68,72).toUint32();  // mm/s Speed Accuracy Estimate
        ubx_nav_pvt.headingAcc = ((float) ubx.payload(72,76).toUint32()) * (float) 1e-5;  // #deg Heading Accuracy Estimate
        ubx_nav_pvt.pDOP = (float) ubx.payload(76,78).toUint16() * 0.01;  // #Position DOP
        ubx_nav_pvt.reserved2 = ubx.payload(78,80).toUint16();  //
        ubx_nav_pvt.reserved3 = ubx.payload(80,84).toUint32();  //
    }
    return ubx_nav_pvt;
}