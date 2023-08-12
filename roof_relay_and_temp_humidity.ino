
// Load Wi-Fi library
#include <ESP8266WiFi.h>

//library for sensor DHT22
#include <DHT.h>                
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

//HDC1080 Sensor

#include <Wire.h>
#include "ClosedCube_HDC1080.h"

ClosedCube_HDC1080 hdc1080;


#include "uptime.h"

// Replace with your network credentials
//const char* ssid     = "NAO Rozhen 2.4G";
//const char* password = "RozhenZO2021";

const char* ssid     = "Mravcho";
const char* password = "bulsatcom";
/*
IPAddress ip(192, 168, 99, 88);
IPAddress gateway(192,168,99,1);   
IPAddress subnet(255,255,255,0);  
*/ 

IPAddress ip(192, 168, 201, 88);
IPAddress gateway(192,168,201,254);   
IPAddress subnet(255,255,255,0); 

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String open_relay_state = "off";
String close_relay_state = "off";

bool moving = false;

// Assign output variables to GPIO pins
const int open_relay = 4;
const int close_relay = 5;
const int stop_relay = 0;
const int DHTPin = 14;  //Pin D5
const int SDA_pin = 12; //Pin D6
const int SCL_pin = 13; //Pin D7
//const int open_relay = 12;
//const int close_relay = 13;


//sensors variables
double t = 0;
double h = 0;
double gamma_dht = 0;
double dp = 0;
double t_hdc = 0;
double h_hdc = 0;
double gamma_hdc = 0;
double dp_hdc = 0;

long timenow = 0;
long next_sens_read = 0;
const long sens_delay_time = 30000;


// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);


void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(open_relay, OUTPUT);
  pinMode(close_relay, OUTPUT);
  pinMode(stop_relay, OUTPUT);
  // Set outputs to LOW
  digitalWrite(open_relay, LOW);
  digitalWrite(close_relay, LOW);
  digitalWrite(stop_relay, LOW);

  Wire.begin(SDA_pin ,SCL_pin);

  hdc1080.begin(0x40);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

// set static IP of WiFi  
WiFi.config(ip, gateway, subnet);


  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void closeDoor() {
  digitalWrite(close_relay, HIGH);
  delay(500);
  digitalWrite(close_relay, LOW);
}

void openDoor() {
  digitalWrite(open_relay, HIGH);
  delay(500);
  digitalWrite(open_relay, LOW);
}

void stopDoor() {
  digitalWrite(stop_relay, HIGH); 
  delay(500);
  digitalWrite(stop_relay, LOW);
}

void read_sensors() {

//Reading from DHT22 
      
      t = dht.readTemperature();       // Gets the values of the temperature
      h = dht.readHumidity();         // Gets the values of the humidity

      //Calculation of Dew Point
      
      gamma_dht = log(h / 100) + ((17.62 * t) / (243.5 + t));
      dp = 243.5 * gamma_dht / (17.62 - gamma_dht);

//Reading from HDC1080

      t_hdc = hdc1080.readTemperature();
      h_hdc = hdc1080.readHumidity();

      //Calculation of Dew Point from HDC1080
      gamma_hdc = log(h / 100) + ((17.62 * t_hdc) / (243.5 + t_hdc));
      dp_hdc = 243.5 * gamma_hdc / (17.62 - gamma_hdc);
  
  
}

void loop(){

  uptime::calculateUptime();
  read_sensors();
   
  WiFiClient client = server.available();   // Listen for incoming clients
  
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /open") >= 0) {
              openDoor();
            } else if (header.indexOf("GET /close") >= 0) {
              closeDoor();
            } else if (header.indexOf("GET /stop") >= 0) {
              stopDoor();
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"30\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Stellar Society's Observatory Roof Control</h1>");
            
            client.println("<p><a href=\"/open\"><button class=\"button\">Open Roof</button></a></p>");
            client.println("<p><a href=\"/close\"><button class=\"button\">Close Roof</button></a></p>");
            client.println("<p><a href=\"/stop\"><button class=\"button\">Stop &nbsp Roof</button></a></p>");

            client.println("<br />");
            client.print("Temperature DHT22 = ");
            client.print(t);
            client.print(" *C ");
            client.println("<br />");
            client.print("Humidity DHT22 = ");
            client.print(h);
            client.print(" % ");
            client.println("<br />");
            client.print("Dew Point  DHT22 = ");
            client.print(dp);
            client.print(" *C "); 
            client.println("<br />");
            client.println("<br />");
                        
            if ((t < (dp+3))&&(t > (dp+0.25))) {
               client.print("<p style=\"color:orange\";><b>Warning ! Dew point is near according least one of the sensors !</b></p>");
               client.println("<br />");
            }
            else if (t <= (dp+0.25)) {
               client.print("<p style=\"color:red\";><b>Warning ! Dew point is reached according least one of the sensors !</b></p>");
               client.println("<br />");
            }
            
            client.print("Temperature HDC1080 = ");
            client.print(t_hdc);
            client.print(" *C ");
            client.println("<br />");
            client.print("Humidity HDC1080 = ");
            client.print(h_hdc);
            client.print(" % ");
            client.println("<br />");
            client.print("Dew Point  HDC1080 = ");
            client.print(dp_hdc);
            client.print(" *C ");
            client.println("<br />");
            client.println("<br />"); 
            client.print("Controller uptime d:h:m:s - "); 
            client.print(uptime::getDays()); 
            client.print(":");
            client.print(uptime::getHours());
            client.print(":");
            client.print(uptime::getMinutes());
            client.print(":");
            client.print(uptime::getSeconds());

            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
