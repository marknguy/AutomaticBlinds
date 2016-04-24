/*
 *  This sketch controls servos which open and close blinds.  An ESP8266-01 with four GPIOs
 *  is used to control up to four servos.  GPIO 0 and 2 are standard.  GPIO 3 (which is 
 *  also RX) and GPIO 1 (which is also TX) are also used.
 *  The servos are continuous rotating.  Since servos may vary, you may need to adjust the 
 *  position variable for OPEN (openPosS) and CLOSE (closePosS) for speed and direction.
 *  
 *  The servos can be controlled using a RESTful GET call
 *  http://<ip_address>/gpioXposYYYdurZZZZ
 *  where X = the gpio number.  should be 0, 1, 2, or 3
 *                   note: actual blind number is X+11 (i.e 1,2,3,4)
 *      YYY = position (this must be 3 characters, e.g. 040 or 170)
 *     ZZZZ = duration to rotate in milliseconds (must be 4 characters.  0000 - 9999)
 *     
 *  you can also do http://<ip_address>/allon or http://<ip_address>/alloff and it will
 *  use the default on position of 180 and off position of 20 and duration of 2000ms.
 *  
 *  Created by Mark Nguyen.  Last modified: 24apr2016
 */

#include <ESP8266WiFi.h>
#include <Servo.h>


const char* ssid = "mySSID";
const char* password = "myPassword";
Servo myservo;
boolean status[4] = {false,false,false,false};     // open = true, closed = false
const char* openPosS = "150";      // adjust this as needed for your servo
const char* closePosS = "110";   // adjust this as needed for your servo
const char* stillPosS = "130";   // adjust this as needed for your servo
int openPos = atoi(openPosS);
int closePos = atoi(closePosS);   
int stillPos = atoi(stillPosS);
int stillPosA[4] = {130,125,130,134};   // since every servo is different, this value calibrates the still position


// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);

// prepare GPIO2 and GPIO3
//  pinMode(3, INPUT);
//  pinMode(2, OUTPUT);
//  digitalWrite(2, 0);
  
  
// Connect to WiFi network

  WiFi.mode(WIFI_STA);

// Connect to WiFi network
//  Serial.println();
//  Serial.println();
//  Serial.print("Connecting to ");
//  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

//  Serial.println("");
//  Serial.println("WiFi connected");
  
  
  
// Start the server
  server.begin();
//  Serial.println("Server started");
//  myservo.attach(2);

  // Print the IP address
//  Serial.println(WiFi.localIP());

  Initialize(closePos);  // initialize with all Blinds closed

}

void loop() {

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  while(!client.available()){
    delay(10);
  }
  
// Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  int gpio;
  int pos;
  int duration;
  duration = 1000;    // default 1000ms
  int index;
// activates the door signal if called from Web page
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    s += "\r\n<!DOCTYPE HTML>\r\n<html>\r\n<body>\r\n<meta name=viewport content='width=400'>\r\n";
//    s += "</body></html>\n";
  pos = 135;
  if (req.indexOf("/allopen") != -1) {
    AllBlinds(openPos);
    for (index = 0; index <= 3; index += 1) {
      status[index]=true;
    }
    s += "Please hit BACK on your browser and REFRESH<br><br>\n";
    s += "</body></html>\n";
    client.print(s);
    client.stop();
    return;
  }
  else if (req.indexOf("/allclose") != -1) {
    AllBlinds(closePos);
    for (index = 0; index <= 3; index += 1) {
      status[index]=false;
    }
    s += "Please hit BACK on your browser and REFRESH<br><br>\n";
    s += "</body></html>\n";
    client.print(s);
    client.stop();
    return;
  }
  else if ((req.indexOf("/gpio") != -1) && (req.indexOf("pos") != -1) && (req.indexOf("dur") != -1)) {
    index = req.indexOf("/gpio");
//    Serial.println(index);
    String gpioS = req.substring(index+5,index+6);
    String posS = req.substring(index+9,index+12);
    String durS = req.substring(index+15,index+19);
    String order = "GPIO: " + gpioS + " Position: " + posS + " Duration: " + durS;
//    Serial.println(order);
//    Serial.println(posi);
//    Serial.println(dura);
    gpio = gpioS.toInt();
    pos = posS.toInt();
    duration = durS.toInt();
//    Serial.println(pos);
//    Serial.println(duration);
    if (status[gpio] && pos < 125) {
      while (!myservo.attached()) {
        myservo.attach(gpio);
      }
      myservo.write(pos);                   // tell servo to go to position in variable 'pos'
      delay(duration);
      myservo.write(stillPosA[gpio]);              // tell servo to go to position 
      delay(2000);   // waits 2000ms for the servo to reach the position
   while (myservo.attached()) {
        myservo.detach();
      }
      status[gpio]=false;
    }
    else if (!status[gpio] && pos > 145) {
      while (!myservo.attached()) {
        myservo.attach(gpio);
      }
      myservo.write(pos);                   // tell servo to go to position in variable 'pos'
      delay(duration);
      myservo.write(stillPosA[gpio]);              // tell servo to go to stop position 
      delay(2000);   // waits 2000ms for the servo to reach the position
      while (myservo.attached()) {
        myservo.detach();
      }
      status[gpio]=true;
    }
    else {
          s += "Invalid request.  Trying to open blinds that are already OPEN or close blinds that are already CLOSED.<br><br>\n";
    }
    s += "Command sent to blinds:  " + order + "<br><br>\n";
    s += "Please hit BACK on your browser and REFRESH<br><br>\n";
    s += "</body></html>\n";
    delay(500);
    client.print(s);
    client.stop();
    delay(50);
    return;
  }  

  
// Main page
  String m = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>Mark's Servos</title>\r\n";
  m += "<h1>Mark's Servos</h1>\n";
  m += "The format should be <bold>gpio[x]pos[yyy]dur[zzzz]</bold><br><br>\n";
  m += "<meta name=viewport content='width=400'>\n";
  for (index = 0; index <= 3; index += 1) { // goes from 1 to 4
    String n = String(index+1);
    m += "Blind " + n + " status: ";
    if (status[index]) {
      m+= "OPEN\n";
    }
    else {
      m+= "CLOSED\n";
    }
    m += "<TABLE BORDER=\"0\"><TR>\n";
    m += "<TD><form method=\"get\" action=\"/gpio" + String(index) + "pos" + openPosS + "dur1000\"><button type=\"submit\" style='height:40px; width:100px'>Open</button></form></TD> \n";
    m += "<TD><form method=\"get\" action=\"/gpio" + String(index) + "pos" + closePosS + "dur1000\"><button type=\"submit\" style='height:40px; width:100px'>Close</button></form></TD>\n";
    m += "</TR><BR></TABLE><BR>\n";
  }

  m += "<hr>";
  m += "<TABLE BORDER=\"0\"><TR>\n";
  m += "All Blinds <td><form method=\"get\" action=\"/allopen\"><button type=\"submit\" style='height:40px; width:100px'>Open</button></form></TD> \n";
  m += "<td><form method=\"get\" action=\"/allclose\"><button type=\"submit\" style='height:40px; width:100px'>Close</button></form></TD><br><br>\n";
  m += "</TR></TABLE><hr>";
  
  m += "</html>\n";
  client.print(m);
  client.stop();
  delay(10);

}

void AllBlinds(int pos) {
  int servo = 0;
  for (servo = 0; servo <= 3; servo += 1) { // goes from 0 to 3
    if (status[servo] && pos < 125) {
      myservo.attach(servo);
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(1000);   // waits 2000ms for the servo to reach the position
      myservo.write(stillPosA[servo]);              // tell servo to go to stop position 
      delay(2000);   // waits 2000ms for the servo to reach the position
      myservo.detach();
      status[servo]=false;
    }
    else if (!status[servo] && pos > 145) {
      myservo.attach(servo);
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(1000);   // waits 2000ms for the servo to reach the position
      myservo.write(stillPosA[servo]);              // tell servo to go to stop position
      delay(2000);   // waits 2000ms for the servo to reach the position
      myservo.detach();
      status[servo]=true;
    }
    delay(200);  
  }
}

void Initialize(int pos) {
  int servo = 0;
  for (servo = 0; servo <= 3; servo += 1) { // goes from 0 to 3
    if (pos < 125) {
//      while (!myservo.attached()) {
        myservo.attach(servo);
//      }
      myservo.write(pos);             // tell servo to go to position in variable 'pos'
      delay(1000);                // waits 2000ms for the servo to reach the end
      myservo.write(stillPosA[servo]);              // tell servo to go to stop position
      delay(2000);                // waits 2000ms for the servo to reach the end
//      myservo.write(160);          // reverse direction
//      delay(600);                 // for half a second
//      while (myservo.attached()) {
        myservo.detach();
//      }
    }
    else if (pos > 145) {
//      while (!myservo.attached()) {
        myservo.attach(servo);
//      }
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(1000);                     // waits 2000ms for the servo to reach the position
      myservo.write(stillPosA[servo]);              // tell servo to go to stop position
      delay(2000);                // waits 2000ms for the servo to reach the end
//      myservo.write(110);          // reverse direction
//      delay(600);                 // for half a second
//      while (myservo.attached()) {
        myservo.detach();
//      }
    }
    delay(1000);  
  }
}
