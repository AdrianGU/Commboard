#include "DynamixelMessage.h"



DynamixelMessage::DynamixelMessage(uint8_t id, uint8_t length, bool write, bool syncwrite, uint8_t reg, uint8_t value)
{
  //When calling the constructor, all given variables are written to private variables in the class.
    DynamixelMessage::_id=id;
    //DynamixelMessage::_length=length;
    DynamixelMessage::_write=write;
    DynamixelMessage::_syncwrite=syncwrite;
    DynamixelMessage::_reg=reg;
    DynamixelMessage::_value=value;
}


void DynamixelMessage::assemblePacket(Vector<uint8_t>* assembledPacket)
//This function will assemble a packet correctly so that it can be sent to, and be unterstood by a Dynamixel Servo-Drive.
//The function will use the private variables that were given when calling the constructor
//of the class to assemble the packet.
//The constructor of the Vector library is buggy currently. Do not remove the argument (Vector<uint8_t>-Type pointer) from the function
//unless you know what you are doing.
//IMPORTANT NOTE: assemblePacket() is currently implemented for the regular Dynamixel Series only and will not work with the Dynamixel Pro Series.

{
    uint8_t pkt[255];
    uint8_t checksumResult =0;

    for (int t = 0; t < 255; t++)
    {
        pkt[t] = 0;
    }

    pkt[0] = 0XFF;
    pkt[1] = 0XFF;
    pkt[2] = DynamixelMessage::_id;
    pkt[3] = 1 + 1 + 2;
    //The length is determined as "number of parameters(N) +2". For the case of just reading one byte from one register this will result in
    // a length of 4 (register to read from (1st parameter) + payload of how many bytes to read from that register on (2nd parameter) + 2 )
    // This needs to be adjusted for more complex request packets

    //Determining what type of message(READ,WRITE,SYNCWRITE) to send to the Dynamixel.
    if(is_write())
    {
        pkt[4]=_WRITE_SERVO_DATA;

    }else if(is_syncwrite())
    {
        pkt[4] = _WRITE_SYNC_SERVO_DATA;

    }else
    {
        pkt[4] = _READ_SERVO_DATA;

    }
    pkt[5] =DynamixelMessage::_reg;
    pkt[6] =DynamixelMessage::_value;

    //checksum calculation = The checksum is calculated by bit-wise inverting the sum of all parameters from the message except for the
    //first two bytes ;
    for(int p=2;p<=pkt[3]+2;p++)
    {

        checksumResult=checksumResult+pkt[p];
    }
    checksumResult=~(checksumResult)&255;

    pkt[pkt[3]+3]=checksumResult;
    for(int k =0;k<pkt[3]+4;k++)
    {
      //Serial.println(pkt[k]);
    }

    //Pushing the message into the vector pointer that got passed when calling
    //the function.
    //This will dynamically allocate the space in the Vector in main.cpp depending on how big pkt[3] is e.g. how big the packet is.
    //Serial.println("Beginning of Message");
    for (int j=0;j<pkt[3]+4;j++)
    {

      assembledPacket->push_back(pkt[j]);
    }
    //Serial.println("End of Message");
}

//Definition of Getter-/Setter-Methods for the private variables
uint8_t DynamixelMessage::get_id() const {
    return _id;
}

void DynamixelMessage::set_id(uint8_t _id) {
    DynamixelMessage::_id = _id;
}

uint8_t DynamixelMessage::get_length() const {
    return _length;
}

void DynamixelMessage::set_length(uint8_t _length) {
    DynamixelMessage::_length = _length;
}

bool DynamixelMessage::is_write() const {
    return _write;
}

void DynamixelMessage::set_write(bool _write) {
    DynamixelMessage::_write = _write;
}

bool DynamixelMessage::is_syncwrite() const {
    return _syncwrite;
}

void DynamixelMessage::set_syncwrite(bool _syncwrite) {
    DynamixelMessage::_syncwrite = _syncwrite;
}

uint8_t DynamixelMessage::get_reg() const {
    return _reg;
}

void DynamixelMessage::set_reg(uint8_t _reg) {
    DynamixelMessage::_reg = _reg;
}

uint8_t DynamixelMessage::get_value() const {
    return _value;
}

void DynamixelMessage::set_value(uint8_t _value) {
    DynamixelMessage::_value = _value;
}
