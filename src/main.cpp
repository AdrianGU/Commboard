#include <UartEvent.h>
#include <DynamixelMessage.h>
#include <QueueArray.h>

DynamixelMessage* TestMessage1 = new DynamixelMessage(0X02,0X04,0X02,0X24,0X01);
DynamixelMessage* TestMessage2 = new DynamixelMessage(0X04,0X04,0X02,0X24,0X01);
Uart1Event Event1;//UART A of the Teensy
Uart2Event Event2;//UART B of the Teensy
Uart3Event Event3;//UART C of the Teensy
IntervalTimer txTimer;

volatile bool print_flag = false;
volatile bool switch_flag = false;
volatile uint8_t rcvdPkt[255];
volatile uint16_t posInArray = 0;
volatile uint16_t lengthOfCurrentPkt = 0;
volatile bool Sync = true;
volatile uint16_t numOfBytesToRead = 4;
volatile uint8_t toSerialQueue[255];
//Vector<DynamixelMessage*> toPortAQueue;
volatile bool scanMode=false;
volatile bool USBmode=false;
QueueArray <DynamixelMessage*> Queue_A(50);
uint8_t USBrcvdPkt[255];
uint8_t USBposInArray;
uint8_t IdMap[255];
uint8_t pktFromSerial[255];
uint8_t serialCounter =0;
void tx1Event();
void tx2Event();
void rx1Event();
void rx2Event();
void rx1Resync();
void rx1SerialResync();
void rx1SerialEvent();
void fetchSerial();
void messageReceival();

Vector<uint8_t> MessageVector;
void sendPkt(Vector<uint8_t>* packetToSend);
void scanPort(Vector<uint8_t>* packetToSend);


void setup()
{
  //toPortAQueue=new Vector<DynamixelMessage*>;
  Event1.txEventHandler = tx1Event;
  Event1.rxEventHandler = rx1Event;
  Event1.rxBufferSizeTrigger = 1;
  Event1.begin(1000000);
  Event1.clear();
  pinMode(13, OUTPUT);


}

void loop()
{
  Queue_A.push(TestMessage1);
  Queue_A.push(TestMessage2);
  Queue_A.pop()->assemblePacket(&MessageVector);
  sendPkt(&MessageVector);
  delay(1000);
  Queue_A.pop()->assemblePacket(&MessageVector);
  sendPkt(&MessageVector);
  delay(1000);

//rx1SerialEvent();
//Serial.println("Calling assemblePacket()");

//Queue_A.peek();
/*
delay(100);
Serial.println("Calling assemblePacket");
Queue_A.pop()->assemblePacket(&MessageVector);
*/
//delay(100);
//Serial.println("Sending Packet");
//sendPkt(&MessageVector);


/*
  one.assemblePacket(&MessageVector);
  Serial.println(sizeof(one));
  delay(1000);
  sendPkt(&MessageVector);
  delay(1000);//
  two.assemblePacket(&MessageVector);
  Serial.println(sizeof(two));
  sendPkt(&MessageVector);
  delay(1000);
*/
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
    DynamixelMessage Scan(i,0X04,0X02,0X03,0X01);
    Scan.assemblePacket(&MessageVector);
    sendPkt(&MessageVector);
  }
  scanMode=false;
}
void tx1Event(void)
{
}


void rx1SerialEvent()
{
  USBposInArray=0;
  if (!Serial.available())
  {
    return;
  }
    while(Serial.available())
    {
      //Serial.println("Incoming Bytes from USB");
      if(USBposInArray==0)
      {
        USBrcvdPkt[USBposInArray]=Serial.read();
        USBposInArray++;
      }
      if(USBposInArray>3)
      {
        if(USBrcvdPkt[0] == 255 && USBrcvdPkt[1] == 255)
        {
          //Serial.println("Found packet prefix, id and length!Reading rest of Packet");
          for(int i=USBrcvdPkt[3];i<=USBrcvdPkt[3]+3;i++)
          {
            USBrcvdPkt[USBposInArray]=Serial.read();
            USBposInArray++;
          }
          //Serial.println("The currently read packet is: ");
          /*for (int j=0;j<USBrcvdPkt[3]+4;j++)
          {
            Serial.println(USBrcvdPkt[j]);
          }*/
          if ((USBposInArray-1)==(USBrcvdPkt[3]+3))
          {
            uint8_t testchksum=0;
            for(int p=2;p<=USBrcvdPkt[3]+2;p++)
            {
                testchksum=testchksum+USBrcvdPkt[p];
            }
            testchksum=~(testchksum)&255;
            if(testchksum==USBrcvdPkt[USBposInArray-1])
            {
              /*Serial.println("Message looks good! Putting this into a Dynamixel Object and sending to its destination : ");
              for(int k=0;k<USBposInArray;k++)
              {
                  Serial.println(USBrcvdPkt[k]);
              }*/
              DynamixelMessage* USBMessage=new DynamixelMessage(USBrcvdPkt[2],USBrcvdPkt[3],USBrcvdPkt[4],USBrcvdPkt[5],USBrcvdPkt[6]);
              Queue_A.push(USBMessage);
              //USBMessage.assemblePacket(&MessageVector);
              //sendPkt(&MessageVector);

              USBposInArray=0;
            }
          }
        }
      }else
      {
        if(USBposInArray<3)
        {
          USBrcvdPkt[USBposInArray]=Serial.read();
          USBposInArray++;
          if(USBrcvdPkt[0] == 255 && USBrcvdPkt[1] == 255)
          {
            //Serial.println("Found Packet prefix, trying to retreive ID+Length ");
            USBrcvdPkt[USBposInArray]=Serial.read();
            USBposInArray++;
            USBrcvdPkt[USBposInArray]=Serial.read();
            USBposInArray++;
          }
        }
      }
    }
  }


void rx1Event()
{

  //Serial.println("ping");
  //Serial.println("Incoming Bytes from Port A");
  if (Sync == true)
  {
    //Serial.println("IN SYNC");
    if (posInArray >= 4)
    {
      numOfBytesToRead = rcvdPkt[3] + 4;
    } else {
      numOfBytesToRead = 4;
    }
    while (Event1.available() && posInArray < numOfBytesToRead)
    {
      rcvdPkt[posInArray] = Event1.read();
      //Serial.println(rcvdPkt[posInArray]);
      posInArray++;
    }

    if (posInArray > 4 && posInArray >= rcvdPkt[3] + 4) {
      //received a complete packet from Dynamixel

      if(scanMode)
      {
        //Serial.println("Hey I found Id :");
        //Serial.println(rcvdPkt[2]);
        IdMap[rcvdPkt[2]]=1;
      }else if (USBmode)
      {
        Serial.println("Sending back this Message to USB :");
        for (int i=0;i<posInArray;i++)
        {
          Serial.println(rcvdPkt[i]);
        }
      }
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
        //Serial.println("Weird buffersize, setting sync to false");
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
      Serial.println(rcvdPkt[0]);
      Serial.println(rcvdPkt[1]);
      Serial.println(rcvdPkt[2]);
      Serial.println(rcvdPkt[3]);
      posInArray--;
    }
  }
}
void sendMessage(DynamixelMessage* messageToSend)
{

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
