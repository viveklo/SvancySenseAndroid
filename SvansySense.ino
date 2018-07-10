//#include <DS3231.h>
#include <DS3231_Simple.h>
#include <Thread.h>
#include <ArduinoJson.h>

#include "Sensor.h"
#include "MotionIRSensor.h"
#include "UltraSonicSensor.h"
#include "WiFiEsp.h"


#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(10, 11); // RX, TX, Wifi
#endif

const int motionPin = 2;
const int cameraPin = 4;
const int usTrigPin = 7;
const int usEchoPin = 8;
int count = 0;
int duration;
int distance;

// for ultrasonic sensor (0 or 1)
int ultrasonic = 0;
// for motion sensor (0 or 1)
int motion = 0;

//wifi parameters
char ssid[] = "TwimEsp";         // your network SSID (name)
char pass[] = "12345678";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

//Config parameter - later to be read from from RTC/WIfi module (EEPROM) card or( SD card - SD card not in plan)


// Ultrasonic sensor distance in cm
int animalDist;
//confirm animal is present by repeating animalDist readings 
int repeatAnimalReading;
//number of capture from camera (number of shots/images)
int numCapture;
//time between capture so that camera flash etc gets settled (secs)
int timebetnCapture;
//time to get next reading of the ultrasonic sensor (secs)
int timebetnNextReading;

//default values for above variable
const  int defaultanimalDist = 150;
const  unsigned int defaultanimalDist1 = 150;
const int defaultrepeatAnimalReading = 5;
const int defaultnumCapture = 5;
const int defaulttimebetnCapture = 10;
const int defaulttimebetnNextReading = 20;

//writetoeeprom
bool writetoeeprom = false;



UltraSonicSensor svansyUltraSonic(usTrigPin, usEchoPin, animalDist, repeatAnimalReading);
MotionIRSensor svansyMotion(motionPin);

Thread senseThread = Thread();
const int senseThreadInterval = 5; // in ms

WiFiEspServer svancyserver(80);
Thread wifiThread = Thread();
const int wifiThreadInterval = 50; // in ms
// use a ring buffer to increase speed and reduce memory allocation
RingBuffer buf(8);

int reqCount = 0;                // number of requests received may not be required
char reqParam[30];

//DS3231  svancyRtc(SDA, SCL);

DS3231_Simple svancyClock; //this is to write to eeprom


void setup() {

  Serial.println("setup()");
  // put your setup code here, to run once:
  svancySetup();

  //bind the call back function
  wifiThread.onRun(wifiCallback);
  wifiThread.setInterval(50);
  
  senseThread.onRun(senseCallback);
  senseThread.setInterval(senseThreadInterval);

  delay(1000); //this delay is to let the sensor settle down before taking a reading
}

void loop() {

  // checks if thread should run & start a thread to listen on wifi server
  if(wifiThread.shouldRun())
    wifiThread.run();
  
  //Thread to read value from sensors and do the processing. This is the main processing loop
  if (senseThread.shouldRun())
    senseThread.run();
    
} // main loop

void svancySetup(){

    unsigned int eepromData[3], i;
    DateTime     eepromTime;
    bool setdefaultparams = true;
      
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode (motionPin, INPUT);
    pinMode(usTrigPin, OUTPUT); // ultrasonic trigger Sets the trigPin as an Output
    pinMode(usEchoPin, INPUT); // ultrasonic echo  Sets the echoPin as an Input
    pinMode (cameraPin, OUTPUT); // out put to camera 
    
    // Setup Serial connection
    Serial.begin(115200);   // initialize serial for debugging
    Serial.println("svancySetup()");

    // Initialize the rtc object
    //rtc date and time will be set through mobile app, if not set will default to ds3231 default time
    svancyClock.begin();

    //does eeprom contain valid values
    // read entries from eeprom
    i = 0;
    while(svancyClock.readLog(eepromTime,eepromData[i])){
      Serial.println((unsigned int)eepromData[i]);
      i = i + 1;
      setdefaultparams = false;
      writetoeeprom = true;
      if (i>4) {// just  a mechanism to avoid any issues of infinite loop
        setdefaultparams = true;
        break; 
      }   
    }

    /*eepromData[0] = (unsigned int)defaultanimalDist  ;
     eepromData[1] = (unsigned int) defaultnumCapture;
      eepromData[2] = (unsigned int) defaulttimebetnCapture;
       eepromData[3] = (unsigned int) defaulttimebetnNextReading;



       Serial.print("eepromData[0] - ");
       Serial.println(eepromData[0]);

              Serial.print("eepromData[1] - ");
       Serial.println(eepromData[1]);

              Serial.print("eepromData[2] - ");
       Serial.println(eepromData[2]);

              Serial.print("eepromData[3] - ");
       Serial.println(eepromData[3]);*/
       
    //writetoeeprom = false;
    //svancyClock.formatEEPROM();

    if(writetoeeprom) { // we lose values once we read the buffer, we will write the same values back again

      //write the values back again
      //first format eprom
      svancyClock.formatEEPROM();
      //write animalDist
      svancyClock.writeLog(eepromData[0]);
      //numCapture
      svancyClock.writeLog(eepromData[1]);
      //timebetnCapture
      svancyClock.writeLog(eepromData[2]);
      //timebetnNextReading
      svancyClock.writeLog(eepromData[3]);
    }
          
    if(!(setdefaultparams)){  
      Serial.println("Get values from eeprom");
      animalDist = (int) eepromData[0];
      Serial.println(animalDist);
      //check range
      if (!((animalDist >=30) && (animalDist <=240) && (animalDist%30 == 0))) // some issue in receving (actual value in ft - 1-8, data set default value
         animalDist = defaultanimalDist;
         
      numCapture = (int) eepromData[1];
      Serial.println(numCapture);
      //check range - 10
      if (!((numCapture >=1) && (numCapture <=10))) 
          numCapture = defaultnumCapture;
                
      timebetnCapture = (int) eepromData[2];
      Serial.println(timebetnCapture);
      //check range 1 - 10
      if (!((timebetnCapture >=1) && (timebetnCapture <=10))) 
          timebetnCapture = defaulttimebetnCapture;
                
      timebetnNextReading = (int) eepromData[3];
      Serial.println(timebetnNextReading);
      //check range range 10 to 120
      if (!((timebetnNextReading >=10) && (timebetnNextReading <=120))) 
          timebetnNextReading = defaulttimebetnNextReading;
    }
    else {
      Serial.println("Set defaultparams");
      // there is no valid data in eeprom, revert to default values
      // valid values in feet between 1(*30) to 8 (*30) default value 5*30
      animalDist = defaultanimalDist;
      // this is not set by the user.. we will keep it at 5 currently
      repeatAnimalReading = defaultrepeatAnimalReading;
      // number of photos - range 1 to 10
      numCapture = defaultnumCapture;
      // (wait) time between photos  - range 1 to 10
      timebetnCapture = defaulttimebetnCapture;
      // (wait) time between photos  - range 10 to 120
      timebetnNextReading = defaulttimebetnNextReading; 
      
    }
    //svancyRtc.begin();

    //setup wifi
    wifiSetup();
    
     // The following lines can be uncommented to set the date and time through mobile app

    //svancyRtc.setDOW(THURSDAY);     // Set Day-of-Week e.g. SUNDAY
    //svancyRtc.setTime(20, 48, 0);     // Set the time to 12:00:00 (24hr format)
    //svancyRtc.setDate(19, 4, 2018);   // Set the date to dd mm yyy

 } //svancySetup end
  
void senseCallback() {
  //initialize variable
  int sumDistance = 0;
  
  ultrasonic = 0;
  motion = 0;
  distance = 0;
  
  digitalWrite (LED_BUILTIN, LOW);
  digitalWrite (cameraPin, LOW);

  //check ultrasonic sensor if there is an animal (values 0 or 1)
  ultrasonic = svansyUltraSonic.checkAvgThreshold();
  //Serial.println("checkAvgThreshold");

  //check if motion is there (values 0 or 1)
  motion = svansyMotion.getMotion();
  //Serial.println("checkAvgThreshold");
  
  if (ultrasonic == 1  || motion == 1) {
    //Serial.println("ultrasonic ");
        //Serial.println(ultrasonic);
    //Serial.println("motion ");
        //Serial.println(motion);
    // currently triggering LED, need to trigger camera and capture images/shots
    for (int i=1; i <= numCapture; i++){
      digitalWrite(LED_BUILTIN, HIGH); //this needs to be replaced
      digitalWrite (cameraPin, HIGH); //this needs to be replaced
      delay(1000);//this needs to be removed
      digitalWrite (cameraPin, LOW); //this needs to be replaced
      digitalWrite (LED_BUILTIN, LOW); //this needs to be replaced
      //delay(timebetnCapture*1000); //convert to secs
      senseThread.setInterval(timebetnCapture*1000);
    }//for loop

    ultrasonic = 0;
    senseThread.setInterval(timebetnNextReading*1000);  // change wait interval for senseThread to waitnextanimalDist milli secs	
  } // if ultrasonic and motion
  else {
    // keep the wait interval the standard one 
    senseThread.setInterval(senseThreadInterval);
  } 

}//senseCallback



void  wifiSetup() {

   int  a;
   Serial1.begin(9600);    // initialize serial for ESP module

   //WiFi.reset();

   Serial.println("wifiSetup()");
   
   //attenpt 3 times to initialze wifi 
   for( a = 1; a <= 3; a = a + 1 ){
      WiFi.init(&Serial1);    // initialize ESP module
      if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
      }
      else {
        break; // break the loop
      }
   }
    
    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD) {
      Serial.println("WiFi not up after 3 attempts");
      while (true); // don't continue.. should display error via blinking led
    } // end if

    Serial.print("Attempting to start AP ");
    Serial.println(ssid);

    // uncomment these two lines if you want to set the IP address of the AP
    IPAddress localIp(192, 168, 1, 1);
    WiFi.configAP(localIp);

    status = WiFi.beginAP(ssid, 10, pass, ENC_TYPE_WPA2_PSK); 
    
    svancyserver.begin();
     
}//wifiSetup end


void wifiCallback(){
  
  WiFiEspClient client = svancyserver.available();  // listen for incoming clients

  int i = 0;

  if (client) {                               // if you get a client,
    Serial.println("New client");             // print a message out the serial port
    buf.init();                               // initialize the circular buffer
    while (client.connected()) {              // loop while the client's connected
      if (client.available()) {               // if there's bytes to read from the client,
        char c = client.read();               // read a byte, then       
        buf.push(c);                          // push it to the ring buffer
         //i = client.read(reqParam, 499);
         //reqParam[i] = '\n';
         reqParam[i] = c;
         i = i +1;
         //Serial.println(c);
         //Serial.println(i);
        
        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
        if (buf.endsWith("\r\n\r\n")|| (i == 18) ) {
          //if (buf.endsWith("\n")) {
          reqParam[i] = '\0';
          
          //process the parameters
          processGetRequest();

          //send http response
          sendHttpResponse(client);         
          break;
        }
      }
    }
        // give the web browser time to receive the data
    delay(10);

    // close the connection
    client.flush();
    client.stop();
    Serial.println("Client disconnected");
    
  }
}

//need to modify based on the application developed
void sendHttpResponse(WiFiEspClient client)
{
  client.print(
    "HTTP/1.1 200 OK\r\n"
    //"Content-Type: text/html\r\n"
    "Connection: close\r\n"  // the connection will be closed after completion of the response
    //"Refresh: 20\r\n"        // refresh the page automatically every 20 sec
    "\r\n");

    //send the paramters value back 

    StaticJsonBuffer<120> JSONbuffer;
    JsonObject& root = JSONbuffer.createObject();
    JsonArray& paramValues = root.createNestedArray("p");
    
    paramValues.add(animalDist/30);
    paramValues.add(numCapture);
    paramValues.add(timebetnCapture);
    paramValues.add(timebetnNextReading);
    
    if(writetoeeprom)
      paramValues.add(1);
    else
      paramValues.add(0);

    //send the current time back
    DateTime currentDateAndTime = svancyClock.read();
    //Time st = svancyRtc.getTime();
    JsonArray& timeValues = root.createNestedArray("t");

    timeValues.add(currentDateAndTime.Hour);
    timeValues.add(currentDateAndTime.Minute);
    timeValues.add(currentDateAndTime.Second);
    timeValues.add(currentDateAndTime.Day);
    timeValues.add(currentDateAndTime.Month);
    timeValues.add(currentDateAndTime.Year);
      
    root.prettyPrintTo(client);
    
    //Serial.print(F("Sending: "));
    //root.printTo(Serial);
    //Serial.println();
    
  /*client.print("<!DOCTYPE HTML>\r\n");
  client.print("<html>\r\n");
  client.print("<h1>Hello World!</h1>\r\n");
  client.print("Requests received: ");
  client.print(++reqCount);
  client.print("<br>\r\n");
  client.print("Analog input A0: ");
  client.print(analogRead(0));
  client.print("<br>\r\n");
  client.print("</html>\r\n"); */
}

void processGetRequest()
{
  char *processParam;
  char param[5];

  Serial.print("processGetRequest() - ");
  
  
  processParam = strstr(reqParam, "s");

  Serial.println(reqParam);
  
  char command = processParam[1];

  if (processParam != NULL){

      switch (command)
      {
        case 'p':
        
            //format aannttTTT
            strncpy(param, processParam+2, 2); //aa in feet - distance
            param[2] = '\0';
            animalDist = atoi(param) * 30; //convert to cm
            //check range
            if (!((animalDist >=30) && (animalDist <=240) && (animalDist%30 == 0))) // some issue in receving (actual value in ft - 1-8, data set default value
               animalDist = defaultanimalDist;
               
            strncpy(param, processParam+4, 2); //nn - number of photos - range 1 to 10
            param[2] = '\0';
            numCapture = atoi(param);
            //check range
            if (!((numCapture >=1) && (numCapture <=10))) 
                numCapture = defaultnumCapture;
              
            strncpy(param, processParam+6, 2);//tt range 1 to 10  
            param[2] = '\0';   
            timebetnCapture = atoi(param);
            //check range
            if (!((timebetnCapture >=1) && (timebetnCapture <=10))) 
                timebetnCapture = defaulttimebetnCapture;
             
            strncpy(param, processParam+8, 3); //TTT range 10 to 120
            param[3] = '\0';
            timebetnNextReading = atoi(param);      
            //check range range 10 to 120
            if (!((timebetnNextReading >=10) && (timebetnNextReading <=120))) 
                timebetnNextReading = defaulttimebetnNextReading;

            strncpy(param, processParam+11, 1); //eeprom bool 'T' or 'F'
            param[1] = '\0';
            if (param[0] == 'T')
              writetoeeprom = true;
            else
              writetoeeprom = false;
             

            //write parameters in eeprom
            if (writetoeeprom){
              Serial.println("saving to eeprom");
              // remove all earlier entries of eeprom 
              svancyClock.formatEEPROM();
              //write animalDist
              svancyClock.writeLog((unsigned int)animalDist);
              //numCapture
              svancyClock.writeLog((unsigned int)numCapture);
              //timebetnCapture
              svancyClock.writeLog((unsigned int)timebetnCapture);
              //timebetnNextReading
              svancyClock.writeLog((unsigned int)timebetnNextReading);
            }
            svansyUltraSonic.updateParameter(animalDist, repeatAnimalReading);

            break;

        case 'd':
            DateTime newDate;
            newDate = svancyClock.read(); //get current date and time
            //format wddmmyyyy
            //int svdow, svday, svmonth, svyear;
            
            strncpy(param, processParam+2, 1); //w - day of week monday etc..
            param[1] = '\0';
            newDate.Dow = atoi(param);
            
            
            strncpy(param, processParam+3, 2); //dd
            param[2] = '\0';
            newDate.Day = atoi(param);
            
            strncpy(param, processParam+5, 2); //mm
            param[2] = '\0';
            newDate.Month = atoi(param);
            
            //strncpy(param, processParam+7, 4);//yyyy
            strncpy(param, processParam+9, 2);//yy
            param[2] = '\0';
            newDate.Year = atoi(param); 

            svancyClock.write(newDate);
            //svancyRtc.setDOW(svdow);     // Set Day-of-Week e.g. SUNDAY
            //svancyRtc.setDate(svday, svmonth, svyear);   // Set the date to dd mm yyy

            break;

         case 'g':
          // get parameters - already sent through sendhttpresponse
          break;

        case 't':
            //format hhmmss
            //int svhh, svmm, svss;
            DateTime newTime; 
            newTime = svancyClock.read(); //get current date and time
            
            strncpy(param, processParam+2, 2); //hh
            param[2] = '\0';
            newTime.Hour = atoi(param);
            
            strncpy(param, processParam+4, 2); //mm
            param[2] = '\0';
            newTime.Minute = atoi(param);
            
            strncpy(param, processParam+6, 2);//ss
            param[2] = '\0';
            newTime.Second = atoi(param); 

            svancyClock.write(newTime);
            //svancyRtc.setTime(svhh, svmm, svss);     // Set the time to 12:00:00 (24hr format) 
            
            break;
         }             
    }   
}




