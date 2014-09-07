/*
  Web Server
 
 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)
  
 */

#include <SPI.h>
#include <Ethernet.h>
#include <RCSwitch.h>

#define SWITCH_CONTROL_PIN 11
#define SWITCH_INPUT_PIN 2

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x2B, 0x41 };
IPAddress ip(192,168,1,50);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);


String readString = ""; //string for fetching data from address
String urlReadSwitch11 = "/readSwitch11"; 
String urlSetSwitch11On = "/setSwitch11On";
String urlSetSwitch11Off = "/setSwitch11Off";

RCSwitch mySwitch = RCSwitch();
  
int state = HIGH;      // the current state of the output pin
int reading;           // the current reading from the input pin
int previous = LOW;    // the previous reading from the input pin
long time = 0;         // the last time the output pin was toggled
long debounce = 200;   // the debounce time, increase if the output flickers


void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  //toggle button
  pinMode(SWITCH_INPUT_PIN, INPUT);
  // Transmitter is connected to Arduino Pin  
  mySwitch.enableTransmit(SWITCH_INPUT_PIN);

}


void loop() {

  reading = digitalRead(SWITCH_INPUT_PIN);

  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (reading == HIGH && previous == LOW && millis() - time > debounce) {
    if (state == HIGH){
      state = LOW;
      mySwitch.switchOff(1,1);
    }else{
      state = HIGH;
      mySwitch.switchOn(1,1);
    }  
    time = millis();    
  }

  previous = reading;


  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (readString.length() < 200) {

          //store characters to string 
          readString+=c; 
        } 
        //Serial.write(c);

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          
          Serial.println(readString);

          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
	        //client.println("Refresh: 30");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");


          if (readString.substring(4,4+urlReadSwitch11.length()) == urlReadSwitch11) {
            Serial.println("read Switch 1 1 status"); 
            client.print("Switch 1-1 Status:");
            client.print(state);
            client.println("<br />");  
          }

          if (readString.substring(4,4+urlSetSwitch11On.length()) == urlSetSwitch11On) {
            Serial.println("Set Switch 1 1 ON"); 
            client.print("Switch 1-1 Status changed to ON:");
            state = HIGH;
            mySwitch.switchOn(1,1);
            client.print(state);
            client.println("<br />");  
          }

          if (readString.substring(4,4+urlSetSwitch11Off.length()) == urlSetSwitch11Off) {
            Serial.println("Set Switch 1 1 OFF"); 
            client.print("Switch 1-1 Status changed to OFF:");
            state = LOW;
            mySwitch.switchOff(1,1);            
            client.print(state);
            client.println("<br />");  
          }          

    
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

