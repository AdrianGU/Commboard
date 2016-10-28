#include <UartEvent.h>
#include <DynamixelMessage.h>

DynamixelMessage Test(0X02,0X04,false,false,0X03,0X01);
Uart1Event Event1;//UART A of the Teensy
Uart2Event Event2;//UART B of the Teensy
Uart3Event Event3;//UART C of the Teensy


volatile bool print_flag = false;
volatile bool switch_flag = false;
volatile uint8_t rcvdPkt[256];
volatile uint16_t posInArray = 0;
volatile uint16_t lengthOfCurrentPkt = 0;
volatile bool Sync = true;
volatile uint16_t numOfBytesToRead = 4;
volatile uint8_t toSerialQueue[255];
volatile uint8_t toPortAQueue[255];
volatile uint8_t toPortBQueue[255];
volatile uint8_t toPortCQueue[255];
void tx1Event();
void rx1Event();
Vector<uint8_t> MessageVector;
void sendPkt(Vector<uint8_t>* packetToSend);


void setup()
{
  Event1.clear();
  Event1.txEventHandler = tx1Event;
  Event1.rxEventHandler = rx1Event;
  Event1.rxBufferSizeTrigger = 1;
  Event1.begin(1000000);
  Event1.clear();
  pinMode(13, OUTPUT);

}

void loop()
{
  digitalWrite(13, HIGH);
  delay(500);
  Test.assemblePacket(&MessageVector);
  sendPkt(&MessageVector);
  digitalWrite(13, LOW);
  delay(500);
}

void tx1Event(void)
{
  //Serial.println("tx1Event triggered");
}

void scanPort(int portNr)
{
  if (portNr == 1)
  {
    //Event1.rxEventHandler=rx1scanEvent;
    for (int i = 0; i < 255; i++)
    {
    //  sendPkt(Test.assemblePacket());
      delayMicroseconds(300);
    }
  }
}


void rx1Event()
{
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
        //Event1.rxEventHandler = rx1Resync;
        posInArray = 0;
        Event1.rxBufferSizeTrigger = 1;
      }
    }
    //sendPkt(4, 0X24, 4);
  }

  Serial.println("BUFFERTRIGGER");
  Serial.println(Event1.rxBufferSizeTrigger);
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
      Serial.println("Sent off array was:");
      Serial.println(rcvdPkt[0]);
      Serial.println(rcvdPkt[1]);
      Serial.println(rcvdPkt[2]);
      Serial.println(rcvdPkt[3]);
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
      Serial.println(rcvdPkt[0]);
      Serial.println(rcvdPkt[1]);
      Serial.println(rcvdPkt[2]);
      //Serial.println(rcvdPkt[3]);
      posInArray--;
    }
  }
}

void sendPkt(Vector<uint8_t>* packetToSend)
{

  Event1.write(packetToSend->data(),packetToSend->size());
  Event1.flush();
  Serial.println("Sent off");
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
