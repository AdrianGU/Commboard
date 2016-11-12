#include <UartEvent.h>
#include <DynamixelMessage.h>
#include <QueueArray.h>

//DynamixelMessage four(0X04,0X04,0X02,0X24,0X01);
//DynamixelMessage thirteen(0X0D,0X04,0X02,0X24,0X01);

Uart1Event Event1;//UART A of the Teensy
Uart2Event Event2;//UART B of the Teensy
Uart3Event Event3;//UART C of the Teensy

IntervalTimer txATimer;
IntervalTimer txBTimer;
IntervalTimer txCTimer;

volatile uint8_t rcvdPkt1[255];
volatile uint8_t rcvdPkt2[255];
volatile uint8_t rcvdPkt3[255];

volatile uint16_t posInArray1 = 0;
volatile uint16_t posInArray2 = 0;
volatile uint16_t posInArray3 = 0;

volatile uint16_t lengthOfCurrentPkt = 0;

volatile bool Sync1 = true;
volatile bool Sync2 = true;
volatile bool Sync3 = true;

volatile bool blockedA = false;
volatile bool blockedB = false;
volatile bool blockedC = false;

volatile uint16_t numOfBytesToRead1 = 4;
volatile uint16_t numOfBytesToRead2 = 4;
volatile uint16_t numOfBytesToRead3 = 4;

volatile uint8_t toSerialQueue[255];

volatile bool scanMode=false;

volatile uint8_t resendCounterA=0;
volatile uint8_t resendCounterB=0;
volatile uint8_t resendCounterC=0;

volatile bool USBmode=false;

QueueArray <DynamixelMessage*> Queue_A(50);
QueueArray <DynamixelMessage*> Queue_B(50);
QueueArray <DynamixelMessage*> Queue_C(50);

uint8_t USBrcvdPkt[255];
uint8_t USBposInArray=0;


volatile uint8_t IdMap[255];

uint8_t pktFromSerial[255];
uint8_t serialCounter =0;

void rx1Event();
void rx2Event();
void rx3Event();

void tx1Event();
void tx2Event();
void tx3Event();


void rx1Resync();
void rx1SerialResync();
void rx1SerialEvent();

void rx2Resync();
void rx2SerialResync();

void rx3Resync();
void rx3SerialResync();



void noMessageReceivalA();
void noMessageReceivalB();
void noMessageReceivalC();

Vector<uint8_t> MessageVector;
void sendA();
void sendB();
void sendC();

void scanPort();

void sendPkt(Vector<uint8_t>* packetToSend);



void setup()
{

  Event1.txEventHandler = tx1Event;
  Event1.rxEventHandler = rx1Event;
  Event1.rxBufferSizeTrigger = 1;
  Event1.begin(1000000);
  Event1.clear();
  pinMode(13, OUTPUT);

  Event2.txEventHandler = tx2Event;
  Event2.rxEventHandler = rx2Event;
  Event2.rxBufferSizeTrigger = 1;
  Event2.begin(1000000);
  Event2.clear();

  Event3.txEventHandler = tx3Event;
  Event3.rxEventHandler = rx3Event;
  Event3.rxBufferSizeTrigger=1;
  Event3.begin(1000000);
  Event3.clear();

  IdMap[2]=1;
  IdMap[13]=2;

  //delay(2000);
  //scanPort();
  delay(2000);
}

void loop()
{
  /*
  four.assemblePacket(&MessageVector);
  sendPkt(&MessageVector);
  delay(100);
  thirteen.assemblePacket(&MessageVector);
  sendPkt(&MessageVector);
  delay(100);
*/
  rx1SerialEvent();

/*
  rx1SerialEvent();
  if (!Queue_A.isEmpty())
  {
    DynamixelMessage* message=Queue_A.pop();
    message->assemblePacket(&MessageVector);
    sendPkt(&MessageVector);
    resendCounter=0;
    delete message;
    */

  /*
  delay(100);
  Serial.println("Calling assemblePacket");
  Queue_A.pop()->assemblePacket(&MessageVector);
  */
  //delay(100);
  //Serial.println("Sending Packet");
  //sendPkt(&MessageVector);
}



void noMessageReceivalA()
{

    txATimer.end();
    blockedA=false;
    Serial.print("No Reply from :");
    Serial.println(MessageVector.at(2));
    if(resendCounterA<3)
    {
      Event1.write(MessageVector.data(),MessageVector.size());
      Event1.flush();
      resendCounterA++;
    }
}
void noMessageReceivalB()
{

    txBTimer.end();
    blockedB=false;
    Serial.print("No Reply from :");
    Serial.println(MessageVector.at(2));
    while(resendCounterB<3)
    {
      Event2.write(MessageVector.data(),MessageVector.size());
      Event2.flush();
      resendCounterB++;
    }
}

void noMessageReceivalC()
{

    txCTimer.end();
    blockedC=false;
    //Serial.print("No Reply from :");
    //Serial.println(MessageVector.at(2));
    while(resendCounterC<3)
    {
      Event3.write(MessageVector.data(),MessageVector.size());
      Event3.flush();
      resendCounterC++;
    }
}

void scanPort()
{
  scanMode=true;
  for (int i=0;i<254;i++)
  {
    IdMap[i]=0;
  }

  for (int j =0;j<20;j++)
  {

    DynamixelMessage* ScanMessage=new DynamixelMessage(j,0X04,0X02,0X03,0X01);
    ScanMessage->assemblePacket(&MessageVector);

    Queue_A.push(ScanMessage);
    Queue_B.push(ScanMessage);
    //Queue_C.push(ScanMessage);


  }
  delay(100);
  while(!Queue_A.isEmpty())
  {

    sendA();
  }

  while(!Queue_B.isEmpty())
  {
    sendB();
  }
  /*
  while(!Queue_C.isEmpty())
  {
    sendC();
  }*/

    Serial.println("Ports scanned, IdMap looks like this: ");
    for (int i=0;i<254;i++)
    {
      Serial.print(i+": ");
      Serial.println(IdMap[i]);
    }
    scanMode=false;
  }

void tx1Event(void)
{
  blockedA=true;
  txATimer.priority(255);
  txATimer.begin(noMessageReceivalA,200);
}
void tx2Event()
{
  blockedB=true;
  txBTimer.priority(255);
  txBTimer.begin(noMessageReceivalB,200);
}

void tx3Event()
{
  /*
  blockedC=true;
  txCTimer.priority(255);
  txCTimer.begin(noMessageReceivalC,200);
  */
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
      Serial.println("Incoming Bytes from USB");
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

              if (IdMap[USBrcvdPkt[2]] == 1  &&(scanMode==false))
              {
                Queue_A.push(USBMessage);
                sendA();
              }
              else if((IdMap[USBrcvdPkt[2]] == 2)  &&(scanMode==false))
              {
                Queue_B.push(USBMessage);
                sendB();
              }
              else if((IdMap[USBrcvdPkt[2]] == 3) &&(scanMode==false))
              {

                //Queue_C.push(USBMessage);
                //sendC();

              }
              else if(IdMap[USBrcvdPkt[2]] == 0)
              {
                Serial.println("Could not find this Servo!");
              }

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



  Serial.println("Incoming Bytes from Port A");
  if (Sync1 == true)
  {
    //Serial.println("IN Sync1");
    if (posInArray1 >= 4)
    {
      numOfBytesToRead1 = rcvdPkt1[3] + 4;
    } else {
      numOfBytesToRead1 = 4;
    }
    while (Event1.available() && posInArray1 < numOfBytesToRead1)
    {
      rcvdPkt1[posInArray1] = Event1.read();
      //Serial.println(rcvdPkt[posInArray]);
      posInArray1++;
    }

    if (posInArray1 > 4 && posInArray1 >= rcvdPkt1[3] + 4) {
      //received a complete packet from Dynamixel
      Serial.print("Received a Message from :");
      Serial.println(rcvdPkt1[2]);
      Serial.println("Packet is:");
      for(int i =0;i<rcvdPkt1[3]+4;i++)
      {
        Serial.println(rcvdPkt1[i]);
      }
      blockedA=false;
      txATimer.end();
      if(scanMode)
      {
        //Serial.println("Hey I found Id :");
        //Serial.println(rcvdPkt[2]);
        IdMap[rcvdPkt1[2]]=1;
      }else if (USBmode)
      {
        Serial.println("Sending back this Message to USB :");
        for (int i=0;i<posInArray1;i++)
        {
          Serial.println(rcvdPkt1[i]);
        }
      }
      Event1.rxBufferSizeTrigger = 4;
      posInArray1 = 0;

    } else {
      if (posInArray1 >= 4)
      {
        numOfBytesToRead1 = rcvdPkt1[3] + 4;
      }
      Event1.rxBufferSizeTrigger = (numOfBytesToRead1 - posInArray1);
      if ((Event1.rxBufferSizeTrigger > 100) | (Event1.rxBufferSizeTrigger <= 0))
      {
        //Serial.println("Weird buffersize, setting sync to false");
        Sync1 = false;
        Event1.rxEventHandler = rx1Resync;
        posInArray1 = 0;
        Event1.rxBufferSizeTrigger = 1;
      }
    }
  }
  //Serial.println("BUFFERTRIGGER");
  //Serial.println(Event1.rxBufferSizeTrigger);

}

void rx2Event()
{



  Serial.println("Incoming Bytes from Port B");
  if (Sync2 == true)
  {
    //Serial.println("IN Sync1");
    if (posInArray2 >= 4)
    {
      numOfBytesToRead2 = rcvdPkt2[3] + 4;
    } else {
      numOfBytesToRead2 = 4;
    }
    while (Event2.available() && posInArray2 < numOfBytesToRead2)
    {
      rcvdPkt2[posInArray2] = Event2.read();
      //Serial.println(rcvdPkt[posInArray]);
      posInArray2++;
    }

    if (posInArray2 > 4 && posInArray2 >= rcvdPkt2[3] + 4) {
      //received a complete packet from Dynamixel
      Serial.print("Received a Message from :");
      Serial.println(rcvdPkt2[2]);
      Serial.println("Packet is:");
      for(int i =0;i<rcvdPkt2[3]+4;i++)
      {
        Serial.println(rcvdPkt2[i]);
      }
      blockedB=false;
      txBTimer.end();

      if(scanMode)
      {
        //Serial.println("Hey I found Id :");
        //Serial.println(rcvdPkt[2]);
        IdMap[rcvdPkt2[2]]=2;
      }else if (USBmode)
      {
        Serial.println("Sending back this Message to USB :");
        for (int i=0;i<posInArray2;i++)
        {
          Serial.println(rcvdPkt2[i]);
        }
      }
      Event2.rxBufferSizeTrigger = 4;
      posInArray2 = 0;

    } else {
      if (posInArray2 >= 4)
      {
        numOfBytesToRead2 = rcvdPkt2[3] + 4;
      }
      Event2.rxBufferSizeTrigger = (numOfBytesToRead2 - posInArray2);
      if ((Event2.rxBufferSizeTrigger > 100) | (Event2.rxBufferSizeTrigger <= 0))
      {
        //Serial.println("Weird buffersize, setting sync to false");
        Sync2 = false;
        Event2.rxEventHandler = rx2Resync;
        posInArray2 = 0;
        Event2.rxBufferSizeTrigger = 1;
      }
    }
  }
  //Serial.println("BUFFERTRIGGER");
  //Serial.println(Event1.rxBufferSizeTrigger);

}

void rx3Event()
{



  Serial.println("Incoming Bytes from Port C");
  if (Sync3 == true)
  {
    //Serial.println("IN Sync1");
    if (posInArray3 >= 4)
    {
      numOfBytesToRead3 = rcvdPkt3[3] + 4;
    } else {
      numOfBytesToRead3 = 4;
    }
    while (Event3.available() && posInArray3 < numOfBytesToRead3)
    {
      rcvdPkt3[posInArray3] = Event3.read();
      //Serial.println(rcvdPkt[posInArray]);
      posInArray3++;
    }

    if (posInArray3 > 4 && posInArray3 >= rcvdPkt3[3] + 4) {
      //received a complete packet from Dynamixel
      Serial.print("Received a Message from :");
      Serial.println(rcvdPkt3[2]);
      Serial.println("Packet is:");
      for(int i =0;i<rcvdPkt3[3]+4;i++)
      {
        Serial.println(rcvdPkt3[i]);
      }
      blockedC=false;
      txCTimer.end();
      if(scanMode)
      {
        //Serial.println("Hey I found Id :");
        //Serial.println(rcvdPkt[2]);
        IdMap[rcvdPkt3[2]]=3;
      }else if (USBmode)
      {
        Serial.println("Sending back this Message to USB :");
        for (int i=0;i<posInArray3;i++)
        {
          Serial.println(rcvdPkt3[i]);
        }
      }
      Event3.rxBufferSizeTrigger = 4;
      posInArray3 = 0;

    } else {
      if (posInArray3 >= 4)
      {
        numOfBytesToRead3 = rcvdPkt3[3] + 4;
      }
      Event3.rxBufferSizeTrigger = (numOfBytesToRead3 - posInArray3);
      if ((Event3.rxBufferSizeTrigger > 100) | (Event3.rxBufferSizeTrigger <= 0))
      {
        //Serial.println("Weird buffersize, setting sync to false");
        Sync3 = false;
        Event3.rxEventHandler = rx3Resync;
        posInArray3 = 0;
        Event3.rxBufferSizeTrigger = 1;
      }
    }
  }
  //Serial.println("BUFFERTRIGGER");
  //Serial.println(Event1.rxBufferSizeTrigger);

}


void rx1Resync()
{
  if (!Event1.available()) {
    return;
  }
  rcvdPkt1[posInArray1] = ((uint8_t) Event1.read());
  //println(rcvdPkt[posInArray]);
  posInArray1++;
  if (posInArray1 >= 4)
  {
    if (rcvdPkt1[0] == 255 && rcvdPkt1[1] == 255)
    {
      //Serial.println("Sent off array was:");
      //Serial.println(rcvdPkt[0]);
      //Serial.println(rcvdPkt[1]);
      //Serial.println(rcvdPkt[2]);
      //Serial.println(rcvdPkt[3]);
      Sync1 = true;
      Event1.rxBufferSizeTrigger = rcvdPkt1[3] + 4;
      //numOfBytesToRead=rcvdPkt[3]+4;
      //posInArray++;
      Event1.rxEventHandler = rx1Event;
    } else {
      Serial.println("rolling array");
      rcvdPkt1[0] = rcvdPkt1[1];
      rcvdPkt1[1] = rcvdPkt1[2];
      rcvdPkt1[2] = rcvdPkt1[3];
      Serial.println(rcvdPkt1[0]);
      Serial.println(rcvdPkt1[1]);
      Serial.println(rcvdPkt1[2]);
      Serial.println(rcvdPkt1[3]);
      posInArray1--;
    }
  }
}
void rx2Resync()
{
  if (!Event2.available()) {
    return;
  }
  rcvdPkt2[posInArray2] = ((uint8_t) Event2.read());
  //println(rcvdPkt[posInArray]);
  posInArray2++;
  if (posInArray2 >= 4)
  {
    if (rcvdPkt2[0] == 255 && rcvdPkt2[1] == 255)
    {
      //Serial.println("Sent off array was:");
      //Serial.println(rcvdPkt[0]);
      //Serial.println(rcvdPkt[1]);
      //Serial.println(rcvdPkt[2]);
      //Serial.println(rcvdPkt[3]);
      Sync2 = true;
      Event2.rxBufferSizeTrigger = rcvdPkt2[3] + 4;
      //numOfBytesToRead=rcvdPkt[3]+4;
      //posInArray++;
      Event2.rxEventHandler = rx2Event;
    } else {
      Serial.println("rolling array");
      rcvdPkt2[0] = rcvdPkt2[1];
      rcvdPkt2[1] = rcvdPkt2[2];
      rcvdPkt2[2] = rcvdPkt2[3];
      Serial.println(rcvdPkt2[0]);
      Serial.println(rcvdPkt2[1]);
      Serial.println(rcvdPkt2[2]);
      Serial.println(rcvdPkt2[3]);
      posInArray2--;
    }
  }
}

void rx3Resync()
{
  if (!Event3.available()) {
    return;
  }
  rcvdPkt3[posInArray3] = ((uint8_t) Event3.read());
  //println(rcvdPkt[posInArray]);
  posInArray3++;
  if (posInArray3 >= 4)
  {
    if (rcvdPkt3[0] == 255 && rcvdPkt3[1] == 255)
    {
      //Serial.println("Sent off array was:");
      //Serial.println(rcvdPkt[0]);
      //Serial.println(rcvdPkt[1]);
      //Serial.println(rcvdPkt[2]);
      //Serial.println(rcvdPkt[3]);
      Sync3 = true;
      Event3.rxBufferSizeTrigger = rcvdPkt3[3] + 4;
      //numOfBytesToRead=rcvdPkt[3]+4;
      //posInArray++;
      Event3.rxEventHandler = rx2Event;
    } else {
      Serial.println("rolling array");
      rcvdPkt3[0] = rcvdPkt3[1];
      rcvdPkt3[1] = rcvdPkt3[2];
      rcvdPkt3[2] = rcvdPkt3[3];
      Serial.println(rcvdPkt3[0]);
      Serial.println(rcvdPkt3[1]);
      Serial.println(rcvdPkt3[2]);
      Serial.println(rcvdPkt3[3]);
      posInArray3--;
    }
  }
}


void sendA()
{
  while(!blockedA && !Queue_A.isEmpty())
  {
    DynamixelMessage* message=Queue_A.pop();
    message->assemblePacket(&MessageVector);
    Event1.write(MessageVector.data(),MessageVector.size());
    Event1.flush();
    delete message;
  }
}

void sendB()
{
  while(!blockedB && !Queue_B.isEmpty())
  {
    DynamixelMessage* message=Queue_B.pop();
    message->assemblePacket(&MessageVector);
    Event2.write(MessageVector.data(),MessageVector.size());
    Event2.flush();
    delete message;
  }
}

void sendC()
{
  /*
  while(!blockedC && !Queue_C.isEmpty())
  {

    DynamixelMessage* message=Queue_C.pop();
    message->assemblePacket(&MessageVector);
    Event3.write(MessageVector.data(),MessageVector.size());
    Event3.flush();
    delete message;
  }*/
}
void sendPkt(Vector<uint8_t>* packetToSend)
{



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


  */
