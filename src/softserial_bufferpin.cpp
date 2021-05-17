#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>


String status = "";
String buffer_space = "";
String buffer_size = "";
int buffer_state = 0;
String page_size = "";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);
SoftwareSerial s(D4, 1);

#define buffer_pin D1

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
}

void check_status() {
  status = "";
  s.print ("\x1B.O");
  char st = s.read (); // read until one byte at a time
  int timeout = 0  ;
  while (st != '\r' && timeout <20) {// read until \r one byte at a time check for timeout
    delay (10);
    ++timeout ;
    if (isDigit (st)) {
      status += st; // concat value to buffer_space
    }
    st = s.read (); // reset byte value
  }
  if (timeout == 20) {
    status = "offline";
  }
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
}

void handleHome() {
  File wsHomeFile = LittleFS.open("/wsHome.html", "r");
  server.streamFile(wsHomeFile, "text/html");
  wsHomeFile.close();
}

void handlePrint() {
  if (server.hasArg("plain") == false) {      // Check if body received
    File wsHomeFile = LittleFS.open("/wsHome.html", "r");
    server.streamFile(wsHomeFile, "text/html");
    wsHomeFile.close();
    return;
  }

  check_status();
  if (status == "offline") {
    File wsPlotterErrorFile = LittleFS.open("/wsPlotterError.html", "r");
    server.streamFile(wsPlotterErrorFile, "text/html");
    wsPlotterErrorFile.close();
  }
  else if (status == "0") {
    //    error_message = " Plotter <br> in action <br> Wait !";
    File wsPlotterErrorFile = LittleFS.open("/wsPlotterError.html", "r");
    server.streamFile(wsPlotterErrorFile, "text/html");
    wsPlotterErrorFile.close();   
  }      
  else if (status == "8") {
    String message = server.arg("plain");
    message.replace("%2C", ",");
    message.replace("%3B", ";");
    message.replace("LB", "SI0.5,0.5;CP-0.5,-0.5;DTX;LBX");

    if (message.substring(5, 16) =="LINEDRAWING" && message.length() > 100) { // check header - if it's correct, send hpgl via serial:
      message.replace("text=LINEDRAWING-","IN;PS4;IP2520,1061,8520,7061;SC0,1000,0,1000;SP1;VS20;PU-90,-90;EA1090,1090;PU-90,-90;EA1090,1090;PU-80,-80;EA1080,1080;"); // new HPGL header

      File wsPrintFile = LittleFS.open("/wsPrint.html", "r");
      server.streamFile(wsPrintFile, "text/html");    // serve thank you message
      wsPrintFile.close();

      for (int i = 0; i < message.length();i += 40) {     // send coordinates in blocks
        buffer_state = digitalRead(buffer_pin);
        if (buffer_state == 0) {
          s.print(message.substring(i, i + 40));
        } 
        else {         
          delay(6000);
          s.print(message.substring(i, i + 40));                
        }
      }
      s.print("PU-10,-10;EA1010,1010;VS;PU-75,500;SI0.5,0.75;DI0,1;CP-7.5,0.5;DT!;LBDRAW SOMETHING !;PU1200,0;DI0,1;CP0,0.5;SI0.2,0.3;DI0,1;CP1,0;DT.;LBxx PLUG 'n' PLOT by Jason xx.SP0;PU1250,1050;IN;"); // add end note
    } 
    else {
      File wsDrawErrorFile = LittleFS.open("/wsDrawError.html", "r");
      server.streamFile(wsDrawErrorFile, "text/html");
      wsDrawErrorFile.close();
    }
  }
  else if (status == "16") {
    //   error_message = "Plot paused <br> with <br> view button";
    File wsPlotterErrorFile = LittleFS.open("/wsPlotterError.html", "r");
    server.streamFile(wsPlotterErrorFile, "text/html");
    wsPlotterErrorFile.close();   
  }
  else if (status == "24") {
    //    error_message = ("View button <br> pressed");
    File wsPlotterErrorFile = LittleFS.open("/wsPlotterError.html", "r");
    server.streamFile(wsPlotterErrorFile, "text/html");
    wsPlotterErrorFile.close();     
  }
  else if (status == "32") {
    //    error_message = "Buffer <br> has info,<br> paper <br> not <br> loaded";
    File wsPlotterErrorFile = LittleFS.open("/wsPlotterError.html", "r");
    server.streamFile(wsPlotterErrorFile, "text/html");
    wsPlotterErrorFile.close();     
  } 
  else if (status == "40") {
    //    error_message = "Paper <br> not <br> loaded";
    File wsPlotterErrorFile = LittleFS.open("/wsPlotterError.html", "r");
    server.streamFile(wsPlotterErrorFile, "text/html");
    wsPlotterErrorFile.close();   
  } 
  else {
    //    error_message = ("Unknown status <br>" + status);
    File wsPlotterErrorFile = LittleFS.open("/wsPlotterError.html", "r");
    server.streamFile(wsPlotterErrorFile, "text/html");
    wsPlotterErrorFile.close();     
  }     
}

void setup() {

  s.begin(9600);
  s.print("IN;"); 

  if (!LittleFS.begin()) {   // Initialize LittleFS
  return;
  }
  pinMode(buffer_pin, INPUT);
  WiFi.softAP("DRAW ME!");
  dnsServer.start(DNS_PORT, "*", apIP);
  server.onNotFound(handleHome);
  server.on("/", handleHome);
  server.on("/print", handlePrint);
  server.begin();
}

void loop() {
  // ................................................... webserver
  server.handleClient();
  dnsServer.processNextRequest();
}