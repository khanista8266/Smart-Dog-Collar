#include <SoftwareSerial.h>

// RX YX5300 MP3 Player Module <-----> TX(GPIO 4 / D2) SoftwareSerial NodeMCU ESP8266.
// TX YX5300 MP3 Player Module <-----> RX(GPIO 5 / D1) SoftwareSerial NodeMCU ESP8266.
SoftwareSerial mp3(5,4); //-> SoftwareSerial mp3 (RX_Pin, TX_Pin);

// ------------------------------------------------------- The byte commands are based on the table contained in the MP3 Player Module Document.
// Link to download the MP3 Player Module Document is available in the video description.

#define CMD_NEXT_SONG         0X01    //-> Play next song.
#define CMD_PREV_SONG         0X02    //-> Play previous song.
#define CMD_PLAY_W_INDEX      0X03    //-> Play with index.
// Play with index : 7E FF 06 03 00 00 01 EF -> Play the first song.
// Play with index : 7E FF 06 03 00 00 02 EF -> Play the second song.

#define CMD_VOLUME_UP         0X04    //-> Volume increased one.
#define CMD_VOLUME_DOWN       0X05    //-> Volume decrease one.
#define CMD_SET_VOLUME        0X06    //-> Set volume.
// Set volume : 7E FF 06 06 00 00 1E EF -> Set the volume to 30 (0x1E is 30) (Set volume is 06).

#define CMD_SNG_CYCL_PLAY     0X08    //-> Single Cycle Play.
// Single Cycle Play : 7E FF 06 08 00 00 01 EF -> Single cycle play the first song.

#define CMD_SEL_DEV           0X09    //-> Select device.
#define DEV_TF                0X02    //-> 02 for TF card.
// Select device : 7E FF 06 09 00 00 02 EF -> Select storage device to TF card.

#define CMD_SLEEP_MODE        0X0A    //-> Chip enters sleep mode
#define CMD_WAKE_UP           0X0B    //-> Chip wakes up
#define CMD_RESET             0X0C    //-> Chip reset
#define CMD_PLAY              0X0D    //-> Resume playback
#define CMD_PAUSE             0X0E    //-> Playback is paused
#define CMD_PLAY_FOLDER_FILE  0X0F    //-> Play with folder and file name.
// Play with folder and file name : 7E FF 06 0F 00 01 01 EF -> Play the song with the directory: /01/001xxx.mp3
// Play with folder and file name : 7E FF 06 0F 00 01 02 EF -> Play the song with the directory: /01/002xxx.mp3

#define CMD_STOP_PLAY         0X16    //-> Stop playing continuously. 
#define CMD_FOLDER_CYCLE      0X17    //-> Cycle play with folder name.
// Cycle play with folder name : 7E FF 06 17 00 01 02 EF -> 01 folder cycle play

#define CMD_SHUFFLE_PLAY      0x18    //-> Shuffle play.
#define CMD_SET_SNGL_CYCL     0X19    //-> Set single cycle play.
// Set single cycle play : 7E FF 06 19 00 00 00 EF -> Start up single cycle play.
// Set single cycle play : 7E FF 06 19 00 00 01 EF -> Close single cycle play.

#define CMD_SET_DAC           0X1A    //-> Set DAC.
#define DAC_ON                0X00    //-> DAC On
#define DAC_OFF               0X01    //-> DAC Off
// Set DAC : 7E FF 06 1A 00 00 00 EF -> Start up DAC output.
// Set DAC : 7E FF 06 1A 00 00 01 EF -> DAC no output.

#define CMD_PLAY_W_VOL        0X22    //-> Play with volume.
// Play with volume : 7E FF 06 22 00 1E 01 EF -> Set the volume to 30 (0x1E is 30) and play the first song
// Play with volume : 7E FF 06 22 00 0F 02 EF -> Set the volume to 15(0x0f is 15) and play the second song

// Additional byte commands from several references. But I tried it on my module and it doesn't work. Maybe the module I'm using doesn't support these commands. 
// Because the module that I use uses a chip AA19HFA440-94 (Although in the shop where this module was purchased writing YX5300 chip). 
// Maybe these commands can work in modules with the YX5300 chip.
// Please comment in the comments column of this project video if these commands work or not in your module.
#define CMD_PLAYING_N         0x4C
#define CMD_QUERY_STATUS      0x42
#define CMD_QUERY_VOLUME      0x43
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS  0x48
#define CMD_QUERY_FLDR_COUNT  0x4f
// -------------------------------------------------------

static int8_t Send_buf[8] = {0}; //-> Buffer to send commands. 
static uint8_t ansbuf[10] = {0}; //-> Buffer to receive response.  

// ------------------------------------------------------- Millis variables for receiving feedback data from the Serial MP3 Player Module.
unsigned long previousMillisCR = 0; //-> save the last time the data was received from the serial mp3 module.
const long intervalCR = 100; //-> interval for updating data received from serial mp3 player.


// IR Sensor variables
int IrPin = D5;
int val = 0;


void setup() {
  //Read IR Sensor
  pinMode(IrPin, INPUT); // sets the pin as input
  Serial.begin(115200);

  mp3.begin(9600);
  delay(500);
  
  Serial.println("Hardware Serial dan Software Serial begin.");
  delay(500);

  Serial.println();

  Serial.println("Select storage device to TF card.");
  sendCommand(CMD_SEL_DEV, 0, DEV_TF);
  delay(500); 
  
}

void loop() {
  val = digitalRead(IrPin);  //อ่านค่าสัญญาณ digital ขาD5 ที่ต่อกับ เซ็นเซอร์ตรวจจับวัตถุ IR Infrared
  Serial.print("val = "); // พิมพ์ข้อมความส่งเข้าคอมพิวเตอร์ "val = "
  Serial.println(val); // พิมพ์ค่าของตัวแปร val
  if (val == 0) { // ค่า เป็น 0  Object is detected
     Serial.println("Object Detected!!");
     sendCommand(CMD_PLAY); //Play MP3

     unsigned long currentMillisCR = millis();
    if (currentMillisCR - previousMillisCR >= intervalCR) {
    // save the last time the data was received from the serial mp3 module.
    previousMillisCR = currentMillisCR;
  
    // Check response.
    if (mp3.available()){
      Serial.println(decodeMP3Answer());
    }
   }
  } 
  else {
    //digitalWrite(ledPin, LOW); // สั่งให้ LED ดับ
  }
  delay(100);
}

// ******************************************************************************************************* decodeMP3Answer()
// The meaning of the mp3 player module response
String decodeMP3Answer() {
  String decodedMP3Answer = "";

  decodedMP3Answer += sanswer();

  switch (ansbuf[3]) {
    case 0x3A:
      decodedMP3Answer += " -> Memory card inserted.";
      break;

    case 0x3D:
      decodedMP3Answer += " -> Completed play num " + String(ansbuf[6], DEC);
      break;

    case 0x40:
      decodedMP3Answer += " -> Error";
      break;

    case 0x41:
      decodedMP3Answer += " -> Data recived correctly. ";
      break;

    case 0x42:
      decodedMP3Answer += " -> Status playing: " + String(ansbuf[6], DEC);
      break;

    case 0x48:
      decodedMP3Answer += " -> File count: " + String(ansbuf[6], DEC);
      break;

    case 0x4C:
      decodedMP3Answer += " -> Playing: " + String(ansbuf[6], DEC);
      break;

    case 0x4E:
      decodedMP3Answer += " -> Folder file count: " + String(ansbuf[6], DEC);
      break;

    case 0x4F:
      decodedMP3Answer += " -> Folder count: " + String(ansbuf[6], DEC);
      break;
  }

  return decodedMP3Answer;
}

// ******************************************************************************************************* sendCommand
// Function for sending commands.
void sendCommand(byte command){
  sendCommand(command, 0, 0);
}

void sendCommand(byte command, byte dat1, byte dat2){
  delay(20);
  Send_buf[0] = 0x7E;    //-> Every command should start with $(0x7E)
  Send_buf[1] = 0xFF;    //-> Version information
  Send_buf[2] = 0x06;    //-> The number of bytes of the command without starting byte and ending byte
  Send_buf[3] = command; //-> Such as PLAY and PAUSE and so on
  Send_buf[4] = 0x01;    //-> 0x00 = not feedback, 0x01 = feedback
  Send_buf[5] = dat1;    //-> data1
  Send_buf[6] = dat2;    //-> data2
  Send_buf[7] = 0xEF;    //-> Ending byte of the command
  
  Serial.print("Sending: ");
  
  for (uint8_t i = 0; i < 8; i++){
    mp3.write(Send_buf[i]) ;
    Serial.print(sbyte2hex(Send_buf[i]));
  }
  
  Serial.println();
}
// *******************************************************************************************************

// ******************************************************************************************************* Functions for conversion
// For byte to hex conversion.
String sbyte2hex(uint8_t b){
  String shex;

  shex = "0X";

  if (b < 16) shex += "0";
  shex += String(b, HEX);
  shex += " ";
  return shex;
}

// For hex to integer conversion.
int shex2int(char *s, int n){
  int r = 0;
  for (int i=0; i<n; i++){
     if(s[i]>='0' && s[i]<='9'){
      r *= 16; 
      r +=s[i]-'0';
     }else if(s[i]>='A' && s[i]<='F'){
      r *= 16;
      r += (s[i] - 'A') + 10;
     }
  }
  return r;
}

// ******************************************************************************************************* 

// ******************************************************************************************************* sanswer(void)
// Function: sanswer. Returns a String answer from mp3 UART module.          
// Parameter:- uint8_t b. void.                                                  
// Return: String. If the answer is well formated answer.                        

String sanswer(void){
  uint8_t i = 0;
  String mp3answer = ""; 

  // Get only 10 Bytes
  while (mp3.available() && (i < 10)){
    uint8_t b = mp3.read();
    ansbuf[i] = b;
    i++;

    mp3answer += sbyte2hex(b);
  }

  // if the answer format is correct.
  if ((ansbuf[0] == 0x7E) && (ansbuf[9] == 0xEF)){
    return mp3answer;
  }

  return "???: " + mp3answer;
}
// *******************************************************************************************************
