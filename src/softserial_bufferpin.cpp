#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include <SoftwareSerial.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>

#define debug  // for serial debug messages

String status = "";
String buffer_space = "";
String buffer_size = "";
String page_size = "";
String error_message = "";
const byte DNS_PORT = 53;
IPAddress apIP (192, 168, 4, 1);
DNSServer dnsServer;
AsyncWebServer server (80);
SoftwareSerial s (D4, D3);

String processor (const String &template_variable) {
  if (template_variable == "ERROR_MESSAGE")
    return error_message;
  return String ();
}
void check_status() {
  status = "";
  s.print ("\x1B.O");
  char st = s.read (); // read until one byte at a time
  int timeout = 0  ;
  while (st != '\r' && timeout <2000) {// read until \r one byte at a time check for timeout
    ++timeout ;
    if (isDigit (st)) {
      status += st; // concat value to buffer_space
    }
    st = s.read (); // reset byte value
  }
  if (timeout == 2000) {
 //-----------------------------------------------------------------------------//
 //---------------------Disable santity check here (chnage "offiline" to 8 )-----//   
  //  status = "offline";
     status = 8;
  //-----------------------------------------------------------------------------//
  //-----------------------------------------------------------------------------//   
  }
  #ifdef debug
  Serial.print ("Status : " + status + " -> ");
  #endif
}

void check_buffer_size() {  
  buffer_size = "";
  s.print ("\x1B.L"); // get buffer size 
  char sz = s.read (); // read one byte at a time
  while (sz != '\r') { // read until \r one byte at a time + if no reply, timeout
    if (isDigit (sz)) {
      buffer_size += sz; // concat value to buffer_size
    }
    sz = s.read (); // reset byte value
  }
  #ifdef debug
  Serial.println ("Buffer size : " + buffer_size);
  #endif
}

void check_buffer_space() {
  buffer_space = "";
  s.print ("\x1B.B"); // get buffer space
  char sp = s.read (); // read one byte at a time
  while (sp != '\r') { // read until \r one byte at a time + if no reply, timeout
    if (isDigit (sp)) {
      buffer_space += sp; // concat value to buffer_space
    }
    sp = s.read (); // reset byte value
  }
  #ifdef debug
  Serial.println ("Buffer space : " + buffer_space);
  #endif
}

void check_page_size() {
  page_size = "";
  s.print ("OH"); // get hard clip limits
  char ps = s.read (); // read one byte at a time
  while (ps != '\r') { // read until \r one byte at a time + if no reply, timeout
    if (isAscii (ps)) {
      page_size += ps; // concat value to page size
    }
    ps = s.read (); // reset byte value
  }
    #ifdef debug
  Serial.println ("Page size : " + page_size);
  #endif
}

void handleHome (AsyncWebServerRequest *request) {
  request->send (LittleFS, "/wsHome.html");
}
void handlePrint (AsyncWebServerRequest *request) { //receive message,serve printing site
  if (request->hasParam ("text", true) == false) { // Check if body received   
    request->send (LittleFS, "/wsHome.html");
    return;
  }

  check_status();
  if (status == "offline") {
    error_message = " Plotter <br> not <br> connected";
    request->send (LittleFS, "/wsError.html", String (), false, processor);
    #ifdef debug 
    Serial.println (" Plotter not connected");
    #endif
  }
  else if (status == "0") {
    error_message = " Plotter <br> in action <br> Wait !";
    request->send (LittleFS, "/wsError.html", String (), false, processor);
    #ifdef debug
    Serial.println (" Plotter in action");
    #endif
  }
  else if (status == "8") {
    #ifdef debug
    Serial.println ("Plotter is ready and waiting");
    #endif
    AsyncWebParameter *p = request->getParam ("text", true);
    String message = p->value ().c_str ();
    message.replace ("%2C", ",");
    message.replace ("%3B", ";");
    message.replace ("LB", "SI0.5,0.5;DTELBXE");

    if (message.substring (0, 11) == "LINEDRAWING" && message.length () > 100) {  // check header - if it's correct, send hpgl via serial:        
      message.replace ("LINEDRAWING-","IN;PS4;SC0,1000,0,1000;SP1"); // new HPGL header               
      request->send (LittleFS, "/wsPrint.html");

      check_buffer_size ();      
    for (int i = 0; i < message.length () ; i += 20) { // send coordinates in blocks of 80                  
      check_buffer_space();                    
      if (buffer_space.toInt () > buffer_size.toInt () * 0.5) { // if bufferspace is les that 20% of total
        s.print (message.substring (i, i + 20));
      }
        else {
          #ifdef debug
          Serial.println ("Delay ");
          #endif

// ----- NEED A DELAY FUNCNTION THAT WORKS WITH ESPASYNCWEBSERVER HERE
           
          // int timeout = 0  ;
          // while (timeout <200000) { 
          //   ++timeout ;
          //   if (timeout == 200000) { 
          //     Serial.println (timeout);         
              s.print (message.substring (i, i + 80));

        //     }
        //  }
        } 
        s.print ("PU;PU1000,1000;IN;"); // add end note
      }
    }
    else {
      error_message = "DRAW SOMETHING <br> FIRST!";
      request->send (LittleFS, "/wsError.html", String (), false, processor);
      #ifdef debug
      Serial.write ( "No lines drawn");
      #endif
    }   
  } 
  else if (status == "16") {
    error_message = "Plot paused <br> with <br> view button";
    request->send (LittleFS, "/wsError.html", String (), false, processor);
    #ifdef debug
    Serial.println ("Plot paused with view button");
    #endif
    status = "";
    error_message = "";
  }
  else if (status == "24") {
    error_message = ("View button <br> pressed");
    request->send (LittleFS, "/wsError.html", String (), false, processor);
    #ifdef debug
    Serial.println ("View button pressed");
    #endif
    status = "";
    error_message = "";
  }
  else if (status == "32") {
    error_message = "Buffer <br> has info,<br> paper <br> not <br> loaded";
    request->send (LittleFS, "/wsError.html", String (), false, processor);
    #ifdef debug
    Serial.println ("Buffer has info, paper not loaded");
    #endif
    status = "";
    error_message = "";
  }
  else if (status == "40") {
    error_message = "Paper <br> not <br> loaded";
    request->send (LittleFS, "/wsError.html", String (), false, processor);
    #ifdef debug
    Serial.println ("Buffer is empty, paper not loaded");
    #endif
    status = "";
    error_message = "";
  }
  else {
    error_message = ("Unknown status <br>" + status);
    request->send (LittleFS, "/wsError.html", String (), false, processor);
    #ifdef debug
    Serial.println ("Status unknown: " + status);
    #endif
    status = "";
    error_message = "";
  }
} 
 
void setup () {
  s.begin (9600);      // open serial connection to plotter
  Serial.begin (9600); //  open serial for debug  
  s.print ("IN;"); // reset plotter after start up serial junk

  if (!LittleFS.begin ()) { // Initialize LittleFS   
    #ifdef debug
    Serial.println ("An Error has occurred while mounting LittleFS");
    #endif
    return;
  } 
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("DRAW ME!");
  dnsServer.start(DNS_PORT, "*", apIP);

  server.onNotFound (handleHome);
  server.on ("/", HTTP_ANY, handleHome);
  server.on ("/print", HTTP_ANY, handlePrint);
  server.begin ();
}

void loop () {
  dnsServer.processNextRequest ();
}