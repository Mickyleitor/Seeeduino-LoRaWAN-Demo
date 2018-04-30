
#include <LoRaWan.h>

#define MAX_PAYLOAD 6
#define ENQ 0x05
#define ACK 0X06
#define CMD1 0x11
#define NUM_DEVICES 1
#define NUM_CMD 1
#define T_10SEG 10000
#define T_5SEG 5000

void decode_msg();
void writeLogMessage(char * text, unsigned char value);
void writeLogMessage(char * text);
void blink_t(int times);

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
    writeLogMessage("Device RESET");
    if(myID == 0) status = 3;
    lora.init();
    lora.initP2PMode(433, SF12, BW125, 8, 8, 20);
}

void loop(void)
{
  memset(buffer, 0, 32);
  switch (status){
    case 0 : {
      if(lora.receivePacketP2PMode(buffer, MAX_PAYLOAD,  &rssi, T_5SEG)){
        decode_msg();
      }
      break;
    }
    case 1 : {
      switch(buffer[2]){
        case CMD1 : {
          build_MessageCommandA0();
          writeLogMessage("Sending the analogical value of A0 [%d] to the requester.",message[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
          break;
        }
      }
      status = 0;
      break;
    }
    case 2 : {
      switch(buffer[2]){
        case CMD1 : {
          writeLogMessage("It's analogical value of A0 is %d",buffer[4]);
          break;
        }
      }
      status = 3;
      break;
    }
    case 3 : {
      if(currentAskingID > NUM_DEVICES) currentAskingID = 0;
      if(currentAskingCMD > NUM_CMD) currentAskingCMD = 0;
      build_MessageAskCMD();
      writeLogMessage("Asking the %d device a command",message[1]);
      lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
      if(lora.receivePacketP2PMode(buffer, MAX_PAYLOAD,  &rssi, T_5SEG)){
        decode_msg();
      }
      break;
    }
  }
}

void decode_msg(){
  if( buffer[1]==myID && buffer[3]==ENQ ) {
    writeLogMessage("The %d device has asked a command to me.",buffer[0]);
    status = 1;
  }else if( buffer[1]==myID && buffer[3]==ACK ) {
    writeLogMessage("The %d device has answered the command to me.",buffer[0]);
    status = 2;
  }else{
    writeLogMessage("A message from %d device have been ignored.",buffer[0]);
  }
}

void writeLogMessage(char * text){
    SerialUSB.print(myID);
    SerialUSB.print(" : ");
    SerialUSB.println(text);
}

void writeLogMessage(char * text, unsigned char value){
    char LogMessages [256];
    memset(LogMessages, 0, 256);
    sprintf(LogMessages,text, value);
    SerialUSB.print(myID);
    SerialUSB.print(" : ");
    SerialUSB.println(LogMessages);
}

void blink_t(int times){
    int y;
    for(y = 0; y < times ; y ++ ){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
  }
}

void build_MessageCommandA0(){
    message[0]= myID;
    message[1]= buffer[0];
    message[2]= CMD1;
    message[3]= ACK;
    message[4]= analogRead(0);
}

void build_MessageAskCMD(){
    message[0]= myID;
    message[1]= myID + currentAskingID;
    message[2]= CMD1 + currentAskingCMD;
    message[3]= ENQ;
}
