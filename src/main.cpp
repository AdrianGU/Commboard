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

uint8_t uartInt1=1;
uint8_t uartInt2=2;
uint8_t uartInt3=3;

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

volatile bool busyA=false;
volatile bool busyB=false;
volatile bool busyC=false;

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

Vector<uint8_t> MessageVectorA;
Vector<uint8_t> MessageVectorB;
Vector<uint8_t> MessageVectorC;
Vector<uint8_t> MessageVectorUSB;

void sendA();
void sendB();
void sendC();

void pushToQueue1(DynamixelMessage* messageToPush1);
void pushToQueue2(DynamixelMessage* messageToPush2);
void pushToQueue3(DynamixelMessage* messageToPush3);

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

  //IdMap[2]=1;
  //IdMap[13]=1;

  delay(2000);
  scanPort();
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

void noMessageReceival( Stream*             event,
                        IntervalTimer*      timer,
                        volatile bool*      blocked,
                        volatile uint8_t*   resendCounter,
                        void(*sendFunctionPointer)(void),
                        Vector<uint8_t>*    messagevector){
    timer->end();
    (*blocked)=false;
    Serial.print("No Reply from :");
    Serial.println(messagevector->at(2));
    if((*resendCounter)<3 && *blocked==false)
    {
      event->write(messagevector->data(),messagevector->size());
      event->flush();
      (*resendCounter)+=1;
    }else{
      sendFunctionPointer();
    }
}

void noMessageReceivalA(){
  noMessageReceival(&Event1, &txATimer, &blockedA, &resendCounterA,sendA, &MessageVectorA);
    /*
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
    */
}

void noMessageReceivalB(){
    noMessageReceival(&Event2, &txBTimer, &blockedB, &resendCounterB,sendB, &MessageVectorB);
    /*
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
    */
}

void noMessageReceivalC(){
    noMessageReceival(&Event3, &txCTimer, &blockedC, &resendCounterC,sendC, &MessageVectorC);
    /*
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
    */
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

    DynamixelMessage* ScanMessageA= new DynamixelMessage(j,0X04,0X02,0X03,0X01);
    DynamixelMessage* ScanMessageB= new DynamixelMessage(j,0X04,0X02,0X03,0X01);
    DynamixelMessage* ScanMessageC= new DynamixelMessage(j,0X04,0X02,0X03,0X01);


    Queue_A.push(ScanMessageA);
    Queue_B.push(ScanMessageB);
    Queue_C.push(ScanMessageC);


  }
    sendA();
    sendB();
    sendC();


    while(busyA && busyB && busyC)
    {
      delay(10);
    }
    Serial.println("Ports scanned, IdMap looks like this: ");
    for (int i=0;i<254;i++)
    {
      //Serial.print(i+": ");
      Serial.println(IdMap[i]);
    }

    scanMode=false;
    }


void txEvent( IntervalTimer* timer,
              volatile bool* blocked,
              volatile bool* busy,
              void (*noMessageReceivalFunctionPointer)(void)){
  (*blocked)=true;
  (*busy)=true;
  timer->priority(255);
  timer->begin(noMessageReceivalFunctionPointer,300);
}

void tx1Event()
{
  txEvent(&txATimer, &blockedA,&busyA, noMessageReceivalA);
  /*
  blockedA=true;
  txATimer.priority(255);
  txATimer.begin(noMessageReceivalA,200);
  */
}

void tx2Event()
{
  txEvent(&txBTimer, &blockedB,&busyB, noMessageReceivalB);
  /*
  blockedB=true;
  txBTimer.priority(255);
  txBTimer.begin(noMessageReceivalB,200);
  */
}

void tx3Event()
{
  txEvent(&txCTimer, &blockedC,&busyC, noMessageReceivalC);
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
                pushToQueue1(USBMessage);
              }
              else if((IdMap[USBrcvdPkt[2]] == 2)  &&(scanMode==false))
              {
                pushToQueue2(USBMessage);
              }
              else if((IdMap[USBrcvdPkt[2]] == 3) &&(scanMode==false))
              {
                pushToQueue3(USBMessage);
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


void rxEvent( UartEvent*          event,
              IntervalTimer*      timer,
              volatile bool*      blocked,
              volatile bool*      sync,
              volatile uint16_t*  posInArray,
              volatile uint16_t*  numOfBytesToRead,
              volatile uint8_t*   rcvdPkt,
              uint8_t*            uartInt,
              volatile uint8_t*   resendCounter,
              void (*rxResyncFunctionPointer)(void)){


    if ((*sync) == true)
    {
      //Serial.println("IN Sync1");
      if ((*posInArray) >= 4)
      {
        (*numOfBytesToRead) = rcvdPkt[3] + 4;
      } else {
        (*numOfBytesToRead) = 4;
      }
      while (event->available() && (*posInArray) < (*numOfBytesToRead))
      {
        rcvdPkt[*posInArray] = event->read();
        //Serial.println(rcvdPkt[posInArray]);
        (*posInArray)+=1;
      }

      if ((*posInArray) > 4 && (*posInArray) >= rcvdPkt[3] + 4) {
        //received a complete packet from Dynamixel
        Serial.print("Received a Message from :");
        Serial.println(rcvdPkt[2]);
        Serial.println("Packet is:");
        for(int i =0;i<rcvdPkt[3]+4;i++)
        {
          Serial.println(rcvdPkt[i]);
        }
        (*blocked)=false;
        resendCounter=0;
        timer->end();
        if(scanMode)
        {
          //Serial.println("Hey I found Id :");
          //Serial.println(rcvdPkt[2]);
          IdMap[rcvdPkt[2]]=*uartInt;
        }else if (USBmode)
        {
          Serial.println("Sending back this Message to USB :");
          for (int i=0;i<(*posInArray);i++)
          {
            Serial.println(rcvdPkt[i]);
          }
        }
        event->setRxBufferSizeTrigger(4);
        (*posInArray) = 0;

      } else {
        if ((*posInArray) >= 4)
        {
          (*numOfBytesToRead) = rcvdPkt[3] + 4;
        }
        event->setRxBufferSizeTrigger((*numOfBytesToRead) - (*posInArray));
        if ((event->getRxBufferSizeTrigger() > 100) | (Event1.getRxBufferSizeTrigger() <= 0))
        {
          //Serial.println("Weird buffersize, setting sync to false");
          (*sync) = false;
          event->setRxEventHandler(rxResyncFunctionPointer);
          (*posInArray) = 0;
          event->setRxBufferSizeTrigger(1);
        }
      }
    }


  }


void rx1Event()
{
  rxEvent(&Event1, &txATimer, &blockedA, &Sync1, &posInArray1, &numOfBytesToRead1, rcvdPkt1,&uartInt1,&resendCounterA, rx1Resync);
}

void rx2Event()
{
  rxEvent(&Event2, &txBTimer, &blockedB, &Sync2, &posInArray2, &numOfBytesToRead2, rcvdPkt2,&uartInt2,&resendCounterC, rx2Resync);

}

void rx3Event()
{
  rxEvent(&Event3, &txCTimer, &blockedC, &Sync3, &posInArray3, &numOfBytesToRead3, rcvdPkt3,&uartInt3,&resendCounterC, rx3Resync);

}


void rxResync( UartEvent*          event,
              volatile bool*      sync,
              volatile uint16_t*   posInArray,
              volatile uint8_t*   rcvdPkt,
              void (*rxEventFunctionPointer)(void)){
  if (!event->available()) {
    return;
  }
  rcvdPkt[*posInArray] = ((uint8_t) event->read());
  //println(rcvdPkt[posInArray]);
  (*posInArray)+=1;
  if ((*posInArray) >= 4)
  {
    if (rcvdPkt[0] == 255 && rcvdPkt[1] == 255)
    {
      //Serial.println("Sent off array was:");
      //Serial.println(rcvdPkt[0]);
      //Serial.println(rcvdPkt[1]);
      //Serial.println(rcvdPkt[2]);
      //Serial.println(rcvdPkt[3]);
      (*sync) = true;
      event->setRxBufferSizeTrigger(rcvdPkt[3] + 4);
      //numOfBytesToRead=rcvdPkt[3]+4;
      //posInArray++;
      event->setRxEventHandler(rxEventFunctionPointer);
    } else {
      Serial.println("rolling array");
      rcvdPkt[0] = rcvdPkt[1];
      rcvdPkt[1] = rcvdPkt[2];
      rcvdPkt[2] = rcvdPkt[3];
      Serial.println(rcvdPkt[0]);
      Serial.println(rcvdPkt[1]);
      Serial.println(rcvdPkt[2]);
      Serial.println(rcvdPkt[3]);
      (*posInArray)-=1;
    }
  }
}


void rx1Resync()
{
  rxResync(&Event1, &Sync1, &posInArray1, rcvdPkt1, rx1Event);
  /*
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
  */
}

void rx2Resync()
{
  rxResync(&Event2, &Sync2, &posInArray2, rcvdPkt2, rx2Event);
  /*
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
  */
}

void rx3Resync()
{
  rxResync(&Event3, &Sync3, &posInArray3, rcvdPkt3, rx3Event);
  /*
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
  */
}

void send(Stream* event, QueueArray<DynamixelMessage*>* queue, volatile bool* blocked, volatile uint8_t* resendCounter,volatile bool* busy, Vector<uint8_t>* messagevector)
{
    *busy=true;
    DynamixelMessage* message=queue->pop();
    message->assemblePacket(messagevector);
    event->write(messagevector->data(),messagevector->size());
    event->flush();
    delete message;
}

void sendA()
{
  send(&Event1, &Queue_A, &blockedA, &resendCounterA,&busyA, &MessageVectorA);
  /*
  while(!blockedA && !Queue_A.isEmpty())
  {
    DynamixelMessage* message=Queue_A.pop();
    message->assemblePacket(&MessageVector);
    Event1.write(MessageVector.data(),MessageVector.size());
    Event1.flush();
    delete message;
  }
  */
}

void sendB()
{
  send(&Event2, &Queue_B, &blockedB, &resendCounterB,&busyB, &MessageVectorB);
  /*
  while(!blockedB && !Queue_B.isEmpty())
  {
    DynamixelMessage* message=Queue_B.pop();
    message->assemblePacket(&MessageVector);
    Event2.write(MessageVector.data(),MessageVector.size());
    Event2.flush();
    delete message;
  }
  */
}

void sendC()
{
  send(&Event3, &Queue_C, &blockedC, &resendCounterC,&busyC, &MessageVectorC);
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

void pushToQueue( QueueArray<DynamixelMessage*>*  queue,
                  DynamixelMessage*               messageToPush,
                  volatile bool*                  busy,
                  void (*sendFunctionPointer)(void))
  {
    queue->push(messageToPush);
    if((*busy) == false)
    {
      sendFunctionPointer();
    }
  }
void pushToQueue1(DynamixelMessage* messageToPush1)
{
    pushToQueue(&Queue_A,messageToPush1,&busyA,sendA);
}



void pushToQueue2(DynamixelMessage* messageToPush2)
{
    pushToQueue(&Queue_B,messageToPush2,&busyB,sendB);
}

void pushToQueue3(DynamixelMessage* messageToPush3)
{
    pushToQueue(&Queue_C,messageToPush3,&busyC,sendC);
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
