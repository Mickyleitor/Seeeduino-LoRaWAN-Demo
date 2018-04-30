
#include <LoRaWan.h>

#define MAX_PAYLOAD 6
#define ENQ 0x05
#define ACK 0X06
#define NAK 0X15
#define CMD1 0x11
#define NUM_DEVICES 1
#define NUM_CMD 1
#define T_10SEG 10
#define T_5SEG 5

void decode_msg();
void writeLogMessage(char * text);
void writeLogMessage(char * text, unsigned char value1);
void writeLogMessage(char * text, unsigned char value1,unsigned char value2);
void build_MessageResponseCMD(unsigned char command);
void build_MessageAskCMD(unsigned char DeviceID,unsigned char command);

uint8_t status = 0;
uint8_t currentAskingCMD = 0;
uint8_t currentAskingID = 0;
unsigned char myID = 0;
unsigned char message[MAX_PAYLOAD];
unsigned char buffer [256];
short length = 0;
short rssi = 0;


void setup(void)
{
    SerialUSB.begin(115200);
    randomSeed(analogRead(0));
    if(myID == 0) status = 3;
    lora.init();
    lora.initP2PMode(433, SF12, BW125, 8, 8, 20);
    delay(5000);
    writeLogMessage("Starting device...");
}

void loop(void)
{
  switch (status){
    case 0 : {
      memset(buffer, 0, 32);
      if(lora.receivePacketP2PMode(buffer, MAX_PAYLOAD,  &rssi, T_5SEG)){
        decode_msg();
      }
      break;
    }
    case 1 : {
      switch(buffer[2]){
        case CMD1 : {
          build_MessageResponseCMD(CMD1);
          writeLogMessage("Sending the analogical value of A0 [%d], frame number [%d]",message[5],message[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
          break;
        }
        default : {
          build_MessageResponseCMD(buffer[2]);
          writeLogMessage("Unknown command resquested, sending a NACK command with fram number [%d]",buffer[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
        }
      }
      status = 0;
      break;
    }
    case 2 : {
      currentAskingID++;
      if(currentAskingID > NUM_DEVICES){
        currentAskingID = 1;
        currentAskingCMD++;
      }
      if(currentAskingCMD >= NUM_CMD) currentAskingCMD = 0;
      build_MessageAskCMD(currentAskingID + myID,currentAskingCMD + CMD1);
      
      writeLogMessage("Asking to device[%d] the command[%d]",message[1],message[2]);
      writeLogMessage("The frame counter is [%d]",message[4]);
      lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
      memset(buffer, 0, 32);
      delay(1000);
      if(lora.receivePacketP2PMode(buffer, MAX_PAYLOAD,  &rssi, T_5SEG)){
        decode_msg();
      }else{
        writeLogMessage("Time out or no response from device[%d]",currentAskingID + myID);
      }
      break;
    }
    case 3 : {
      switch(buffer[2]){
        case CMD1 : {
          writeLogMessage("The analogical value of A0 from device[%d] is %d",buffer[0],buffer[5]);
          break;
        }
      }
      status = 2;
      break;
    }
  }
}

void decode_msg(){
  if( buffer[1]==myID && buffer[3]==ENQ ) {
    writeLogMessage("The device[%d] has asked the command[%d] to me",buffer[0],buffer[2]);
    status = 1;
  }else if( buffer[1]==myID && buffer[3]==ACK ) {
    writeLogMessage("The device[%d] has answered the command with frame number [%d] to me",buffer[0],buffer[4]);
    status = 3;
  }else{
    writeLogMessage("A message from device[%d] have been ignored.",buffer[0]);
  }
}

void build_MessageResponseCMD(unsigned char command){
    message[0]= myID;
    message[1]= buffer[0];
    message[2]= command;
    message[3]= NAK;
    switch (command) {
      case CMD1 : {
          message[3]= ACK;
          message[4]= buffer[4];
          message[5]= analogRead(A0);
      }
      break;
    }
}

void build_MessageAskCMD(unsigned char DeviceID,unsigned char command){
    message[0]= myID;
    message[1]= DeviceID;
    message[2]= command;
    message[3]= ENQ;
    message[4]= random(256);
}

void writeLogMessage(char * text){
    SerialUSB.print(myID);
    SerialUSB.print(" : ");
    SerialUSB.println(text);
}

void writeLogMessage(char * text, unsigned char value1){
    char LogMessages [256];
    memset(LogMessages, 0, 256);
    sprintf(LogMessages,text, value1);
    SerialUSB.print(myID);
    SerialUSB.print(" : ");
    SerialUSB.println(LogMessages);
}
void writeLogMessage(char * text, unsigned char value1,unsigned char value2){
    char LogMessages [256];
    memset(LogMessages, 0, 256);
    sprintf(LogMessages,text, value1,value2);
    SerialUSB.print(myID);
    SerialUSB.print(" : ");
    SerialUSB.println(LogMessages);
}
