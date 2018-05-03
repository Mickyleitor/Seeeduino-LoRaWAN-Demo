#include <LoRaWan.h>

#define MAX_PAYLOAD 6
#define ENQ 0x05
#define ACK 0X06
#define NAK 0X15
#define CMD1 'a'
#define CMD2 'b'
#define CMD3 'c'
#define CMD4 'd'
#define T_10SEG 10
#define T_5SEG 5

void decode_msg();
void writeLogMessage(char * text);
void writeLogMessage(char * text, unsigned char value1);
void writeLogMessage(char * text, unsigned char value1,unsigned char value2);
void build_MessageResponseCMD(unsigned char command);
void build_MessageAskCMD(unsigned char DeviceID,unsigned char command);
void read_commandFromSerial();
unsigned char get_LATposition ();
unsigned char get_LONposition ();

// Current state of the FSM of the system
uint8_t status = 0;

// This value represent a counter in the communication
// It's used to avoid duplicated messages (and visualize it) in case a bunch of commands is requested.
uint8_t frame_counter = 0;

// This value should be changed, it's the physical address
// Example: '1' will be device 1.
unsigned char myID = '0';

unsigned char message[MAX_PAYLOAD];
unsigned char buffer [256];
short length = 0;
short rssi = 0;


void setup(void)
{
    Serial.begin(9600);
    SerialUSB.begin(115200);
    randomSeed(analogRead(0));
    if((myID == 0) || (myID == '0')){
      digitalWrite(LED_BUILTIN,HIGH);
      status = 3;
    }
    lora.init();
    lora.initP2PMode(433, SF12, BW125, 8, 8, 20);
    delay(5000);
    writeLogMessage("Starting device...");
}

void loop(void)
{
  switch (status){
    // Wait for remote request
    case 0 : {
      memset(buffer, 0, 32);
      if(lora.receivePacketP2PMode(buffer, MAX_PAYLOAD,  &rssi, T_5SEG)){
        // If message arrive, decode it.
        decode_msg();
      }
      break;
    }
    case 1 : {
      switch(buffer[2]){
        // If requested command is get analogical value, build the message and send it
        case CMD1 : {
          build_MessageResponseCMD(CMD1);
          writeLogMessage("Sending the analogical value of A0 [%d], frame number [%d]",message[5],message[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
          break;
        }
        // If requested command is the RSSI, build the message and send it.
        case CMD2 : {
          build_MessageResponseCMD(CMD2);
          writeLogMessage("Sending the RRSI (Received Signal Strength Indicator) [%d], frame number [%d]",message[5],message[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
          break;
        }
        case CMD3 : {
          // If requested command is the GPRMC NMEA LAT, build message and send it.
          build_MessageResponseCMD(CMD3);
          writeLogMessage("Getting data and sending from GPRMC NMEA, LAT is: [%d], frame number [%d]",message[5],message[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
          break;
        }
        // If requested command is the GPRMC NMEA LON, build message and send it.
        case CMD4 : {
          build_MessageResponseCMD(CMD4);
          writeLogMessage("Getting data and sending from GPRMC NMEA, LON is: [%d], frame number [%d]",message[5],message[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
          break;
        }
        // If Unknown command (or unimplemented) is received, send a NACK command
        default : {
          build_MessageResponseCMD(buffer[2]);
          writeLogMessage("Unknown command resquested, sending a NACK command with frame number [%d]",buffer[4]);
          lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);
        }
      }
      status = 0;
      break;
    }
    case 2 : {
      // Read command from serial
      read_commandFromSerial();
      // Print log messages
      writeLogMessage("Asking to device[%d] the command[%d]",message[1],message[2]);
      writeLogMessage("The frame counter is [%d]",message[4]);
      // Send message via P2P Lora
      lora.transferPacketP2PMode(message,MAX_PAYLOAD,T_5SEG);

      // Reset buffer to 0
      memset(buffer, 0, 32);
      // Wait for answer 10 seconds maximum
      if(lora.receivePacketP2PMode(buffer, MAX_PAYLOAD,  &rssi, T_10SEG)){
        // Decode message
        decode_msg();
      }else{
        // Print log message that the message hasn't been answered
        writeLogMessage("Time out or no response from device[%d]",message[1]);
      }
      break;
    }
    case 3 : {
      // Print answered command (depending on the command, obviously)
      switch(buffer[2]){
        case CMD1 : {
          writeLogMessage("The analogical value of A0 from device[%d] is %d",buffer[0],buffer[5]);
          break;
        }
        case CMD2 : {
          writeLogMessage("The RSSI from this device to device[%d] is [%d]",buffer[0],buffer[5]);
          writeLogMessage("The RSSI from device[%d] to this device is [%d]",buffer[0],rssi);
          break;
        }
        case CMD3 : {
          writeLogMessage("GPRMC NMEA, LAT is: [%d], from device [%d]",buffer[5],buffer[0]);
          break;
        }
        case CMD4 : {
          writeLogMessage("GPRMC NMEA, LON is: [%d], from device [%d]",buffer[5],buffer[0]);
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
    // If the message is for me and its a requesting-type message,
    // Go to status 1 and print a log message
    writeLogMessage("The device[%d] has asked the command[%d] to me",buffer[0],buffer[2]);
    status = 1;
  }else if( buffer[1]==myID && buffer[3]==ACK ) {
    // If the message is for me and its a answering-type message,
    // Go to status 3 and print a log message
    writeLogMessage("The device[%d] has answered the command with frame number [%d]",buffer[0],buffer[4]);
    status = 3;
  }else if( buffer[1]==myID && buffer[3]==NAK ){
    // If the message is for me and says the command it's not implemeted,
    // Do nothing and print a log message reporting the NACK answer
    writeLogMessage("The device[%d] has answered NACK (Unknown command requested) frame number [%d]",buffer[0],buffer[4]);
  }else{
    // If the message is unknown, ignore it and print a log message
    writeLogMessage("A message from device[%d] have been ignored.",buffer[0]);
  }
}

// Build a message with a specific command
void build_MessageResponseCMD(unsigned char command){
    message[0]= myID;
    message[1]= buffer[0];
    message[2]= command;
    message[3]= ACK;
    message[4]= buffer[4];
    switch (command) {
      case CMD1 : {
          // Read analog message and store it on the message
          message[5]= analogRead(A0);
      }
      break;
      case CMD2 : {
           // Store the last RSSI value in the message
          message[5]= rssi;
      }
      break;
      case CMD3 : {
          // Get Latitude position (via function) and store it
          message[5]= get_LATposition();
      }
      break;
      case CMD4 : {
        // Get Longitude position (via function) and store it
          message[5]= get_LONposition();
      }
      break;
      default : {
        // If unknown command is requested, store a NACK value
        message[3] = NAK;
      }
    }
}

// Build a message depending on the Device and command
void build_MessageAskCMD(unsigned char DeviceID,unsigned char command){
    message[0]= myID;
    message[1]= DeviceID;
    message[2]= command;
    message[3]= ENQ;
    message[4]= frame_counter++;
}

// Writting log functions.
// A few versions is provided, supporting up to 2 values to be formatted.

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

// This functions read streams from serial and decode it
void read_commandFromSerial(){
  String buffering = "";
  bool isInValid = true;
  SerialUSB.println("Insert command :");
  while(isInValid){
    // If there is something coming from the serialUSB stream
    if(SerialUSB.available() > 0){
      // Read it and store in buffering string
      buffering = SerialUSB.readString();
      buffering.toLowerCase();

      // If it's a valid format (cmd:ADDR@COMMAND)
      if((buffering.substring(0,4)).compareTo("cmd:")==0 && buffering.charAt(5)=='@'){
        // Print the command and build the message,
        // then exit the while loop
        SerialUSB.print("Valid command - [");
        SerialUSB.print(buffering.substring(0));
        SerialUSB.println("]");
        build_MessageAskCMD(buffering.charAt(4),buffering.charAt(6));        
        isInValid = false;
      }else{
        // If it's not valid, print the error.
        SerialUSB.print("Invalid command [");
        SerialUSB.print(buffering);
        SerialUSB.println("]");
        SerialUSB.println("Format is [cmd:ADDR@COMMAND]");
        SerialUSB.println("Valid commands:\n'a' : Get analogical value of A0\n'b' : Get RSSI from both devices\n'c' : Get GPS based latitude position\n'd' : Get GPS based longitude position");
        
      }
    }
  }
}

unsigned char get_LATposition (){
  bool isInValid = true;
  int counter;
  String buffering = "";
  while(isInValid)
  {
    // Read until '$' character is found.
    buffering = Serial.readStringUntil('$');
    // Read again until ',' character is found
    buffering = Serial.readStringUntil(',');
    // Now we should have in the buffering the "GPRMC" string
    if(buffering.substring(0,5).compareTo("GPRMC")==0){
      // Count until field 3 (which is LAT value in GPRMC NMEA payload)
      for(counter = 0 ; counter < 3 ; counter ++) {
        // Store it
        buffering = Serial.readStringUntil(',');
      }
      isInValid = false;
    }
  }
  // return the value as a int
  return buffering.toInt();
}


unsigned char get_LONposition (){
  bool isInValid = true;
  int counter;
  String buffering = "";
  while(isInValid)
  {
    // Read until '$' character is found.
    buffering = Serial.readStringUntil('$');
    // Read again until ',' character is found
    buffering = Serial.readStringUntil(',');
    // Now we should have in the buffering the "GPRMC" string
    if(buffering.substring(0,5).compareTo("GPRMC")==0){
      // Count until field 5 (which is LAT value in GPRMC NMEA payload)
      for(counter = 0 ; counter < 5 ; counter ++) {
        // Store it
        buffering = Serial.readStringUntil(',');
      }
      isInValid = false;
    }
  }
  // return the value as a int
  return buffering.toInt();
}
