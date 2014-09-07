/*
 * Time_NTP.pde
 * Example showing time sync to NTP time source
 *
 * This sketch uses the Ethenet library with the user contributed UdpBytewise extension
 */

#include <SPI.h>      
#include <Time.h> 
#include <TimeAlarms.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <stdlib.h>

//for temperature reading DS10B20
#include <OneWire.h>
#include <DallasTemperature.h>

//for LCD Display LCM1602
#include <Wire.h>  
#include <LiquidCrystal_I2C.h>

/*-------- LCD display code ----------*/

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

/*-------- Temperature reading code ----------*/
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

//for xively client
#define APIKEY         "PCFjN3QoIAF9EMqXfQEdKVq9TT2y67KwRj4mtHtwyYiuFXZJ" // replace your xively api key here
#define FEEDID         1877155116 // replace your feed ID 1877155116
#define USERAGENT      "My Project" // user agent is the project name

int buttonPin = 7;
boolean currentLCDState = LOW;//stroage for current button state
boolean lastLCDState = LOW;//storage for last button state

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;


/* -------xiveli code - begin ------ */
// initialize the library instance:
EthernetClient client;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
IPAddress xivelyServer(216,52,233,122);      // numeric IP for api.xively.com
//char server[] = "api.xively.com";   // name address for xively API

unsigned long xivelyLastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean xivelyLastConnected = false;                 // state of the connection last time through the main loop
const unsigned long xivelyPostingInterval = 300000; //delay between updates to Xively.com


//for ethernet connection

byte ip[] = { 192, 168, 1, 50 }; // set the IP address to an unused address on your network
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x2B, 0x41 }; //updated to my mac



IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
//byte SNTP_server_IP[] = { 130,149,17,21};    // ntps1-0.cs.tu-berlin.de
//byte SNTP_server_IP[] = { 192,53,103,108};   // ptbtime1.ptb.de

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

unsigned int localPort = 8888;      // local port to listen for UDP packets

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

time_t prevDisplay = 0; // when the digital clock was displayed
const  long timeZoneOffset = 0L; // set this to the offset in seconds to your local time;

void setup() 
{
  Serial.begin(9600);
  
  /*-------- LCD display code ----------*/
  /*16 character 2 line I2C Display
   Backpack Interface labelled "YwRobot Arduino LCM1602 IIC V1"
    Get the LCD I2C Library here: 
    https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
   */
   
    lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
    
    pinMode(buttonPin, INPUT);//Set LCD Backlight button pin as INPUT

// ------- Quick 3 blinks of backlight  -------------
  for(int i = 0; i< 3; i++)
  {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight(); // finish with backlight on  
  currentLCDState = HIGH;
  lastLCDState = HIGH;

//-------- Write characters on the display ------------------
// NOTE: Cursor Position: (CHAR, LINE) start at 0  
  lcd.setCursor(0,0); //Start at character 4 on line 0
  lcd.print("Ola");
  delay(1000);
  lcd.setCursor(0,1);
  lcd.print("Mario Rovisco");
  delay(2000);  
  lcd.clear();
 

  /*-------- Temperature sensor code ----------*/
  // locate devices on the OneWire bus
  Serial.print("Locating OneWire devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
    // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 12);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
  
  /*-------- Ethernet code ----------*/
  
  //Ethernet.begin(mac,ip);  
 
  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Offline mode
   while (1==1) ; 
  }else{
    //online mode
    Udp.begin(localPort);
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);
    while(timeStatus()== timeNotSet)   
       ; // wait until the time is set by the sync provider
    Alarm.timerRepeat(10, TimeTemperatureAlarm); 
  }
}
/* --- xively code begin ----*/
void loop(){  
  
    // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
/* --- xively code end ----*/
  /*
  if( now() != prevDisplay) //update the display only if the time has changed
  {
    prevDisplay = now();
    digitalClockDisplay();  
  }
  */
  //digitalClockDisplay();
  readLcdBacklightButton();
  Alarm.delay(100); // wait one second between clock display
}

// functions to be called when an alarm triggers:
void TimeTemperatureAlarm(){
  Serial.println("Alarm time");  
  digitalClockDisplay();
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
  // It responds almost immediately. Let's print out the data
  printTemperature(insideThermometer); // Use a simple function to print out the data
  lcd.clear();
  printTemperatureOnLCD(insideThermometer,lcd); 
  
  /*-------xively code begin ------*/
  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && xivelyLastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  
  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - xivelyLastConnectionTime > xivelyPostingInterval)) {
    float tempC = sensors.getTempC(insideThermometer);
    String dataString = "temperature,";
    //Serial.print("Temp read: ");
    //Serial.println(tempC);
    
    char buffer[6];
    dtostrf(tempC,5,2,buffer);
    dataString += String(buffer);  
    Serial.print("Post to xively: ");
    Serial.println(dataString);
    sendData(dataString);
  }
  // store the state of the connection for next time through
  // the loop:
  xivelyLastConnected = client.connected();
  
}
/*------ xively code begin --------*/

// this method makes a HTTP connection to the server:
void sendData(String thisData) {
  // if there's a successful connection:
  if (client.connect(xivelyServer, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    client.print(FEEDID);
    client.println(".csv HTTP/1.1");
    client.println("Host: api.xively.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    client.println(thisData.length());

    // last pieces of the HTTP PUT request:
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.println(thisData);
    
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
   // note the time that the connection was made or attempted:
  xivelyLastConnectionTime = millis();
}


// This method calculates the number of digits in the
// sensor reading.  Since each digit of the ASCII decimal
// representation is a byte, the number of digits equals
// the number of bytes:

int getLength(int someValue) {
  // there's at least one byte:
  int digits = 1;
  // continually divide the value by ten, 
  // adding one to the digit count for each
  // time you divide, until you're at 0:
  int dividend = someValue /10;
  while (dividend > 0) {
    dividend = dividend /10;
    digits++;
  }
  // return the number of digits:
  return digits;
}

/*----------- xively code end -----------*/

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


/*-------- LCD Display Code ----------*/
void printTemperatureOnLCD(DeviceAddress deviceAddress,LiquidCrystal_I2C lcd)
{
  float tempC = sensors.getTempC(deviceAddress);
  lcd.setCursor(0,0);
  //lcd.write("Hora");
  digitalClockDisplayOnLCD();
  lcd.setCursor(0,1);
  lcd.write("Temp C: ");
  lcd.print(tempC);
}


void digitalClockDisplayOnLCD(){

  // digital clock display of the time 
  lcd.print(hour());
  printDigitsOnLCD(minute());
  printDigitsOnLCD(second());
  lcd.print(" ");
  lcd.print(day());
  lcd.print("/");
  lcd.print(month());
  //lcd.print(" ");
  //lcd.print(year()); 
  
}

void printDigitsOnLCD(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  lcd.print(":");
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

//Toogle Button to turn LCD Backlight ON/OFF
void readLcdBacklightButton(){
  currentLCDState = digitalRead(buttonPin);  
  if (currentLCDState == HIGH && lastLCDState == LOW){//if button has just been pressed    
    Serial.println("LCD BackLigth ON");    
    lcd.backlight();
    Alarm.delay(1);//crude form of button debouncing  
  } else if(currentLCDState == LOW && lastLCDState == HIGH){
    Serial.println("LCD BackLigth OFF");  
    lcd.noBacklight();  
    Alarm.delay(1);//crude form of button debouncing 
  } 
  lastLCDState = currentLCDState;
  
}

/*-------- Temperature sensor code ----------*/

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

/*-------- NTP code ----------*/

unsigned long getNtpTime()
{
  Serial.println("start getNtpTime");
  sendNTPpacket(timeServer);
  Alarm.delay(2000);
 
  if ( Udp.parsePacket() ) {  
    Serial.println("packer received");
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    //Serial.print("Seconds since Jan 1 1900 = " );
    //Serial.println(secsSince1900);               

    // now convert NTP time into everyday time:
    //Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    // print Unix time:
    //Serial.println(epoch);     
    int timezone = 3600;  
    epoch =epoch + timezone;

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');  
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':'); 
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch %60); // print the second
    return epoch;
  }
  return 0; // return 0 if unable to get the time
}

unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:       
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();   
}
