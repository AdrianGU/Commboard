#include <UartEvent.h>
#include <DynamixelMessage.h>

DynamixelMessage one(0X02,0X04,false,false,0X24,0X01);
DynamixelMessage two(0X04,0X04,false,false,0X24,0X01);
Uart1Event Event1;//UART A of the Teensy
Uart2Event Event2;//UART B of the Teensy
Uart3Event Event3;//UART C of the Teensy


volatile bool print_flag = false;
volatile bool switch_flag = false;
volatile uint8_t rcvdPkt[255];
volatile uint16_t posInArray = 0;
volatile uint16_t lengthOfCurrentPkt = 0;
volatile bool Sync = true;
volatile uint16_t numOfBytesToRead = 4;
volatile uint8_t toSerialQueue[255];
volatile Vector<uint8_t> toPortAQueue;
volatile bool scanMode=false;
uint8_t IdMap[255];
uint8_t pktFromSerial[255];
uint8_t serialCounter =0;
void tx1Event();
void tx2Event();
void rx1Event();
void rx2Event();
void rx1Resync();
void fetchSerial();

Vector<uint8_t> MessageVector;
void sendPkt(Vector<uint8_t>* packetToSend);
void scanPort(Vector<uint8_t>* packetToSend);


void setup()
{

  Event1.txEventHandler = tx1Event;
  Event1.rxEventHandler = rx1Event;
  Event1.rxBufferSizeTrigger = 1;
  Event1.begin(1000000);
  Event1.clear();
  pinMode(13, OUTPUT);


}

void loop()
{

fetchSerial();
delay(10);
  /*
  one.assemblePacket(&MessageVector);
  sendPkt(&MessageVector);
  delay(100);
  two.assemblePacket(&MessageVector);
  sendPkt(&MessageVector);
  delay(100);*/
}


void scanPort(Vector<uint8_t>* packetToSend)
{
  scanMode=true;
  for (int j=0;j<254;j++)
  {
    IdMap[j]=0;
  }
  for(int i = 0;i<254;i++)
  {
    delay(1);
    DynamixelMessage Scan(i,0X04,false,false,0X03,0X01);
    Scan.assemblePacket(&MessageVector);
    sendPkt(&MessageVector);
  }
  scanMode=false;
}
void tx1Event(void)
{
  Serial.println("txEvent from Port A triggered");
}

void fetchSerial()
{
    while(Serial.available())
    {
      Serial.println("Theres stuff in the USB buffer!");
      pktFromSerial[serialCounter]=Serial.read();
      serialCounter++;
      if (serialCounter>6)
      {
        Serial.println("Resetting Counter, the read packet was:");
        for(int j=0;j<7;j++)
        {
            Serial.println(pktFromSerial[j]);
            serialCounter=0;
        }
      }
    }
  }


void rx1Event()
{
  Serial.println("Incoming Bytes from Port A");
  if (Sync == true)
  {
    Serial.println("IN SYNC");
    if (posInArray >= 4)
    {
      numOfBytesToRead = rcvdPkt[3] + 4;
    } else {
      numOfBytesToRead = 4;
    }

    while (Event1.available() && posInArray < numOfBytesToRead)
    {

      rcvdPkt[posInArray] = Event1.read();
      Serial.println(rcvdPkt[posInArray]);
      posInArray++;
    }


    if (posInArray > 4 && posInArray >= rcvdPkt[3] + 4) {
      //received a complete packet from Dynamixel
      Event1.rxBufferSizeTrigger = 4;
      posInArray = 0;
      if(scanMode)
      {
        Serial.println("Hey I found Id :");
        Serial.println(rcvdPkt[2]);
        IdMap[rcvdPkt[2]]=1;
        /*for (int k=0;k<100;k++)
        {
          Serial.println("Your Id Map looks like this:");
          Serial.println(k);
          Serial.println("this Id is connected to the following Port:");
          Serial.print(IdMap[k]);
        }*/
      }
      //Test.assemblePacket(&MessageVector);
      //sendPkt(&MessageVector);
    } else {
      if (posInArray >= 4)
      {
        numOfBytesToRead = rcvdPkt[3] + 4;
      }
      Event1.rxBufferSizeTrigger = (numOfBytesToRead - posInArray);
      if ((Event1.rxBufferSizeTrigger > 100) | (Event1.rxBufferSizeTrigger <= 0))
      {
        Serial.println("Weird buffersize, setting sync to false");
        Sync = false;
        Event1.rxEventHandler = rx1Resync;
        posInArray = 0;
        Event1.rxBufferSizeTrigger = 1;
      }
    }
  }
  //Serial.println("BUFFERTRIGGER");
  //Serial.println(Event1.rxBufferSizeTrigger);
  print_flag = true;
}


void rx1Resync()
{

  if (!Event1.available()) {
    return;
  }
  rcvdPkt[posInArray] = ((uint8_t) Event1.read());
  //println(rcvdPkt[posInArray]);
  posInArray++;
  if (posInArray >= 4)
  {
    if (rcvdPkt[0] == 255 && rcvdPkt[1] == 255)
    {
      //Serial.println("Sent off array was:");
      //Serial.println(rcvdPkt[0]);
      //Serial.println(rcvdPkt[1]);
      //Serial.println(rcvdPkt[2]);
      //Serial.println(rcvdPkt[3]);
      Sync = true;
      Event1.rxBufferSizeTrigger = rcvdPkt[3] + 4;
      //numOfBytesToRead=rcvdPkt[3]+4;
      //posInArray++;
      Event1.rxEventHandler = rx1Event;
    } else {
      Serial.println("rolling array");
      rcvdPkt[0] = rcvdPkt[1];
      rcvdPkt[1] = rcvdPkt[2];
      rcvdPkt[2] = rcvdPkt[3];
      //Serial.println(rcvdPkt[0]);
      //Serial.println(rcvdPkt[1]);
      //Serial.println(rcvdPkt[2]);
      //Serial.println(rcvdPkt[3]);
      posInArray--;
    }
  }
}

void sendPkt(Vector<uint8_t>* packetToSend)
{



Event1.write(packetToSend->data(),packetToSend->size());
Event1.flush();
packetToSend->clear();


//digitalWrite(3,LOW);
}


/*
  void pro_test()
  {
  int counter;
  uint8_t pkt[15];
  uint8_t checksum;
  for (int t = 0; t < 15; t++)
  {
    pkt[t] = 0;
    //rcvdPkt[t] = 0;
  }
  pkt[0] = 255;
  pkt[1] = 255;
  pkt[2] = 253;
  pkt[3] = 0;
  pkt[4] = 1;
  pkt[5] = 8;
  pkt[6] = 0;
  pkt[7] = 3;
  pkt[8] = 51;
  pkt[9] = 2;
  pkt[10] = 255;
  pkt[11] = 255;
  pkt[12] = 255;
  pkt[13] = 229;
  pkt[14] = 4;
  counter = 0;
  int i = 0;
  Event1.write(pkt, 15);
  while (Event1.available() && counter < 8)
  {
    rcvdPkt[i] = Event1.read();
    Serial.println(Event1.read());
    counter = counter + 1;
    i = i + 1;
  }
  Event1.flush();

  }
*/
