#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp8266.h>
#include <TaskScheduler.h>
#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"

//=========================================================================

void taskWrapper();
void getHeartRate();
void getTemp();
void sendDatatoBlynk();
void sendDatatoGs();
void connectWifi();


//Tasks

Task t1(10000,TASK_FOREVER,&taskWrapper); 
Task t2(30000,TASK_FOREVER, &sendDatatoBlynk);
Task t3(30000,TASK_FOREVER, &sendDatatoGs); 


Scheduler runner;
MAX30105 particleSensor;

//================================================================================
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).

char auth[] = ""; //Bynk Auth Token
char ssid[] = ""; //Wifi SSD
char pass[] = ""; //Wifi Password


char server[] = ""; // Blynk server
int port = 8080;    // Blynk server port   

//Host & httpsPort to connect to Google Site script
const char* host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------
//--> Google Sheet script ID
String GAS_ID = ""; 


//============================================================================
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute, tempVal=0;
int beatAvg, bpmVal=0;

//============================================================================
// t1 = taskWrapper()

void taskWrapper(){

    getHeartRate();
    delay(500);
    getTemp();
}

//============================================================================

void getHeartRate()
{
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.getINT1(); // clear the status registers by reading
  //particleSensor.getINT2();
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running

  for (int i=0; i < 400; i++) { 
  long irValue = particleSensor.getIR();
 
  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // 
  
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000)
    Serial.print(" No finger?");

  Serial.println();

  }
  bpmVal = beatAvg;
  
}

//=============================================================

void getTemp(){
   float temperature;

  for(int i=0; i < 5; i++) { 
  particleSensor.setup(0);
  particleSensor.getINT2();
  temperature = particleSensor.readTemperature();
  ///particleSensor.setPulseAmplitudeRed(255);
  tempVal = temperature - 1;
  tempVal = roundf(tempVal * 10) / 10;
  Serial.print("temperature C = ");
  Serial.print(tempVal, 2);
  Serial.println();
  delay(100);
  }
  
}

//=============================================================

void sendDatatoBlynk() 
{
  // sending sensor value to Blynk App.
  Blynk.virtualWrite(V1, bpmVal);  
  Blynk.virtualWrite(V2, tempVal);
  Blynk.run();
  
}
//==============================================================================

void connectWifi(){

  WiFi.begin(ssid, pass); //--> Connect to your WiFi router
  Serial.println("");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
//==============================================================================

void sendDatatoGs() {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  //----------------------------------------Connect to Google host
  WiFiClientSecure client; //--> Create a WiFiClientSecure object.
  client.setInsecure(); // this is the magical line that makes everything work
  
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  //----------------------------------------

  float string_temp = tempVal; 
  int string_bpm = bpmVal;
  String url = "/macros/s/" + GAS_ID + "/exec?temp=" + string_temp + "&bpm="+string_bpm; //  2 variables 
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  //---------------------------------------
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  //----------------------------------------
} 

//=========================== SETUP ==================================
void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    //while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  //Create connection to Blynk and Google site script
  Blynk.begin(auth, ssid, pass, server, port);
  connectWifi();

  //particleSensor.setup(); //Configure sensor with default settings
  //particleSensor.getINT1(); // clear the status registers by reading
  //particleSensor.getINT2();
  //particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  //particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED


//================ Task Scheduler ========================================
  runner.init();
  Serial.println("Initialized scheduler");
  
  runner.addTask(t1);
  Serial.println("added t1 taskWrapper()");

  runner.addTask(t2);
  Serial.println("added t2");

  runner.addTask(t3);
  Serial.println("added t3");

  delay(1000);
  
  t1.enable();
  Serial.println("Enabled t1");
  t2.enable();
  Serial.println("Enabled t2");
  t3.enable();
  Serial.println("Enabled t3");
}

//==========================================================================

void loop () {
  runner.execute();
}
