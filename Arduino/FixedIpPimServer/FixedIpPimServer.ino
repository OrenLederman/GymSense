/*

 Based om Web server by by Tom Igoe
 Based on ChatServer example by David A. Mellis
 Based on Web Server well structured. by Alessandro Calzavara, alessandro(dot)calzavara(at)gmail(dot)com 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include <Time.h>  
#include <Timezone.h>    //https://github.com/JChristensen/Timezone
#include <string.h>

/**********************************************************************************************************************
*                                   Network definitions
***********************************************************************************************************************/

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
// use this for final version (is configured for the the ethernet port donwstairs)
//byte mac[] = { 
//  0x00, 0xFE, 0xFE, 0xFE, 0xAA, 0x01 };

// this one is just for testing
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress ip(18,139,1, 86);
IPAddress gateway(18,139,0, 1);
IPAddress subnet(255, 255, 0, 0);

// telnet defaults to port 23
EthernetServer server(80);

// A UDP instance to let us send and receive packets over UDP. Needed for NTP
EthernetUDP Udp;

// client defs
EthernetClient client;
char serverName[] = "orenled-inno.media.mit.edu"; 

/**********************************************************************************************************************
*                                   Memory space for string management and setup WebServer service
***********************************************************************************************************************/

// For sending HTML to the client
#define STRING_BUFFER_SIZE 128
char buffer[STRING_BUFFER_SIZE];

// to store data from HTTP request
#define STRING_LOAD_SIZE 128
char load[STRING_LOAD_SIZE];

// POST and GET variables
#define STRING_VARS_SIZE 128
char vars[STRING_VARS_SIZE];

/**********************************************************************************************************************
*                                        define HTTP return structure ID for parsing HTTP header request
***********************************************************************************************************************/
struct HTTP_DEF {
  int pages;
  char vars[STRING_VARS_SIZE]; //size needs to match the global variable 'vars' otherwise program crashes due to index overrun
} ;


/**********************************************************************************************************************
*                                   NTP and time definitions
***********************************************************************************************************************/

unsigned int localPort = 8888;      // local port to listen for UDP packets

IPAddress timeServer(18, 7 ,33 , 13); // time.mit.edu
//IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov NTP server

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

//US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

const  long timeZoneOffset = 0L; // set this to the offset in seconds to your local time;


/**********************************************************************************************************************
*                                   input/output, variables
***********************************************************************************************************************/

// Pins used by ethernet shield - 10,11,12,13 and probably 4
// Pin names ending with2 are used by the second sensor
int pimPin1 = 9; // elliptical
int pimPin2 = 8; // treadmill
int statusLedPin = 7;
int movementLedPin1 = 5;
int movementLedPin2 = 6;

// global variables
int movement1 = 0;
int movement2 = 0;

time_t lastMovementTime1 = 0; // last time there was movment
time_t lastMovementTime2 = 0; // last time there was movment

// temp vars used for calcualtion (I think)
time_t utc = 0; //

long lastReadingTime = 0;
/**********************************************************************************************************************
*                                   Code
***********************************************************************************************************************/

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  
  // setup ports
  pinMode(pimPin1, INPUT);
  pinMode(movementLedPin1, OUTPUT);  
  pinMode(pimPin2, INPUT);
  pinMode(movementLedPin2, OUTPUT);  
  
  pinMode(statusLedPin, OUTPUT); 
  
  digitalWrite(statusLedPin, HIGH); 
  
  // this check is only needed on the Leonardo:
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // start the Ethernet connection:
  Serial.println("Trying to get an IP address using DHCP");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    //show error (and enter infinite loop...)
    blinkErrorInfi();
    // initialize the ethernet device not using DHCP:
    Ethernet.begin(mac, ip, gateway, subnet);
  }

  /*
  // use fixed IP given by MIT
  Serial.println("Setting IP");  
  // The original example doesn't provide the "dns". I'm 
  // not sure how DNS is set here... but it seems to be working
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  */
  
  // give the Ethernet shield a second to initialize:
  delay(1000);
  
  // print your local IP address:
  Serial.print("My IP address: ");
  ip = Ethernet.localIP();
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(ip[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();
  
  // start listening for clients
  server.begin();

  // setup NTP stuff
  Udp.begin(localPort);
  
  // Get time set upped
  Serial.println("Setting time sync function");
  setSyncProvider(getNtpTime);  //set function to call when sync required
  while(timeStatus()== timeNotSet)
   ; // wait until the time is set by the sync provider
  Serial.println("Done setting time sync function");   

  // finished init
  digitalWrite(statusLedPin, LOW); 
  Serial.println("turned off led");   
}

/*********************************************************************************************
Main loop
*********************************************************************************************/
void loop() {
   // perform a sensor reading no more than once a second.
  if (millis() - lastReadingTime > 1000){
      getData1();
      getData2();
      // timestamp the last time you got a reading:
      lastReadingTime = millis();
      //writeMovement(1);
  }
  // listen for incoming Ethernet connections:
  listenForEthernetClients();
}

/*********************************************************************************************
Read data from Sensor
*********************************************************************************************/

void getData1() {
  movement1 = digitalRead(pimPin1);
  
  if (movement1 == HIGH) {
    lastMovementTime1 = now();
    time_t local = myTZ.toLocal(lastMovementTime1, &tcr);
    Serial.print("Sensor1 ");    
    printTime(local, tcr -> abbrev);
    digitalWrite(movementLedPin1, HIGH); 
  } else {
    digitalWrite(movementLedPin1, LOW);     
  }
  //Serial.print("Movement? ");
  //Serial.println(movement);


}

void getData2() {
  movement2 = digitalRead(pimPin2);
  
  if (movement2 == HIGH) {
    lastMovementTime2 = now();
    time_t local = myTZ.toLocal(lastMovementTime2, &tcr);
    Serial.print("Sensor2 ");
    printTime(local, tcr -> abbrev);
    
    digitalWrite(movementLedPin2, HIGH); 
  } else {
    digitalWrite(movementLedPin2, LOW);     
  }
}

/**********************************************************************************************************************
*                                              Update movement in database
***********************************************************************************************************************/
void writeMovement(int sensorId) {
  Serial.println("Lets try to write to web server");
   // if you get a connection, report back via serial:
  if (client.connect(serverName, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    client.println("GET /updateStat?sensorId=1 HTTP/1.1");
    client.print("Host: ");
    client.println(serverName);
    client.println("Connection: close");
    client.println();    
  } else { //cant connect
        Serial.println("cant make HTTP request");
  }
  
  if (client.connected()) { 
    Serial.println("Disconnect");
    client.stop();	// DISCONNECT FROM THE SERVER
  }
}

/**********************************************************************************************************************
*                                              Answering clients
***********************************************************************************************************************/

// Provide data to clients
void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    Serial.println(lastMovementTime1);    
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.0 200 OK\nServer: arduino\nCache-Control: no-store, no-cache, must-revalidate\nPragma: no-cache\nConnection: close\nContent-Type: text/html\n");
          client.println("<html><head><title>The Warehouse Gym status</title><style type=\"text/css\">table{border-collapse:collapse;}td{padding:0.25em 0.5em;border:0.5em solid #C8C8C8;}</style></head><body><h1>The Warehouse Gym status</h1>");
          client.println("<h2>Status</h2>");

          client.print("Elliptical trainer: Last movement detected at ");           
          //this works outside the loop...
//          time_t local = myTZ.toLocal(lastMovementTime1, &tcr);
//          client.println(lastMovementTime1);
//          clientPrintTime(client,lastMovementTime1, tcr -> abbrev);
          
          //client.print(timeToString(local, tcr -> abbrev));// doesn't work 
          time_t t = myTZ.toLocal(lastMovementTime1, &tcr);;
          client.print(sPrintI00(hour(t)));
          client.print(sPrintDigits(minute(t)));
          client.print(sPrintDigits(second(t)));
          client.print(" ");
          client.print(dayShortStr(weekday(t)));
          client.print(" ");
          client.print(sPrintI00(day(t)));
          client.print(" ");
          client.print(monthShortStr(month(t)));
          client.print(" ");
          client.print(year(t));
          client.print(" ");
      //    client.print(tz);


          client.println("<br><br>");  


          client.print("Treadmill: Last movement detected at ");           
          //client.println(lastMovementTime2);          
          //this works outside the loop...
//          time_t local2 = myTZ.toLocal(lastMovementTime2, &tcr);
          //clientPrintTime(client,lastMovementTime2, tcr -> abbrev);
          
          //client.print(timeToString(local2, tcr -> abbrev));// doesn't work 
//          client.print("DONE!");
          t = myTZ.toLocal(lastMovementTime2, &tcr);;
          client.print(sPrintI00(hour(t)));
          client.print(sPrintDigits(minute(t)));
          client.print(sPrintDigits(second(t)));
          client.print(" ");
          client.print(dayShortStr(weekday(t)));
          client.print(" ");
          client.print(sPrintI00(day(t)));
          client.print(" ");
          client.print(monthShortStr(month(t)));
          client.print(" ");
          client.print(year(t));
          client.print(" ");
      //    client.print(tz);
  
          
          client.println("</body></html>");  
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
      //Serial.println("done client");
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("Closed client");
  }
} 

/**********************************************************************************************************************
*                                                              Misc
***********************************************************************************************************************/
// control status LED
void blinkError(int times) {
  // finished init
  for (int i=0; i < times; i++) {
    digitalWrite(statusLedPin, HIGH);
    delay(500);
    digitalWrite(statusLedPin, LOW);  
    delay(500);
  }
}

// control status LED
void blinkErrorInfi() {
  // finished init
  while (1==1) {
    digitalWrite(statusLedPin, HIGH);
    delay(100);
    digitalWrite(statusLedPin, LOW);  
    delay(100);
  }
}

/*********************************************************************************************
NTP and time zone
Sources:
* http://playground.arduino.cc/Code/time
* http://arduino.cc/en/Tutorial/UdpNtpClient
* http://forum.arduino.cc/index.php?topic=96891.0
*********************************************************************************************/

// send an NTP request to the time server at the given address 
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

/*
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
*/

//Function to print time with time zone
void printTime(time_t t, char *tz)
{
    Serial.println(timeToString(t, tz));
}

String clientPrintTime(EthernetClient client, time_t t, char *tz) {
    client.print(sPrintI00(hour(t)));
    client.print(sPrintDigits(minute(t)));
    client.print(sPrintDigits(second(t)));
    client.print(" ");
    client.print(dayShortStr(weekday(t)));
    client.print(" ");
    client.print(sPrintI00(day(t)));
    client.print(" ");
    client.print(monthShortStr(month(t)));
    client.print(" ");
    client.print(year(t));
    client.print(" ");
    client.print(tz);

}

String timeToString(time_t t, char *tz) {
    String s = "";
    s = s + sPrintI00(hour(t));
    s = s + sPrintDigits(minute(t));
    s = s + sPrintDigits(second(t));
    s = s + " ";
    s = s + dayShortStr(weekday(t));
    s = s + " ";
    s = s + sPrintI00(day(t));
    s = s + " ";
    s = s + monthShortStr(month(t));
    s = s + " ";
    s = s + year(t);
    s = s + " ";
    s = s + tz;
  return s;
}

//Print an integer in "00" format (with leading zero).
//Input value assumed to be between 0 and 99.
String sPrintI00(int val)
{
    String s1 = "";
    if (val < 10) s1 = s1 + "0";
    s1 = s1 + val;
    return s1;
}

//Print an integer in ":00" format (with leading zero).
//Input value assumed to be between 0 and 99.
String sPrintDigits(int val)
{
    String s2 = "";
    s2=s2+":";
    if(val < 10) s2 = s2 + "0";
    s2= s2 + val;
    return s2;
}

unsigned long getNtpTime()
{
  sendNTPpacket(timeServer);
  delay(1000);

  Serial.println("Reading time from NTP server");
  unsigned long secsSince1900 = 0;
  unsigned long epoch = 0;
  if ( Udp.parsePacket() ) {  
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    secsSince1900 = highWord << 16 | lowWord;  
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);               
    
    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;  
    // print Unix time:
    Serial.println(epoch);                               
  }
  
  if (secsSince1900 != 0) {
    return epoch;
  } else {  
    Serial.println("Could not read time from NTP server!");
    blinkError(5);
    // will never get here. will keep blinking error
    return 0; // return 0 if unable to get the time
  }
}

