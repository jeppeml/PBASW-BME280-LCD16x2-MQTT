#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

unsigned long delayTime;
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

const char* ssid = "<your ssid>";
const char* password = "<your password for wifi>";
const int wifiConnectRetries = 3;
const int wifiConnectDelayBetweenRetries = 2000;

const char* mqtt_server = "mqtt.flespi.io";
const int mqtt_port = 1883;
const char* mqtt_user = "<token generated from flespi.io>";
const char* mqtt_pass = "";

const int screenDelay = 4000; // Screen LCD update interval
const int mqttPublishDelay = 60000; // Send to MQTT with this interval
const int mqttConnectRetries = 3;
const int mqttConnectDelayBetweenRetries = 1000;
 
int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

float tempC = 0.0;
float humidity = 0.0;
float airPressure = 0.0;
float cToFRate = 1.8;
float cToF32 = 32;
int countDelays = 0;

 
void setup() {
  Serial.begin(115200);
  delay(10);
 
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  init_lcd();
  init_bme280();
 
  connectWifi(wifiConnectRetries, wifiConnectDelayBetweenRetries);
  initMQTT();
}

void init_bme280(){
  Serial.println(F("BME280 test"));
  
  bool status;
  status = bme.begin(0x76);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  Serial.println();
}

void init_lcd(){
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

  delayTime = 1000;
}
 
void loop() {
  readBME280Values(); // read values into variables
  printValues(screenDelay); // Print values from variables to LCD 
  
  client.loop(); // loop the pubsub client for callbacks (subscribes)
  countDelays++;
  if(countDelays>=(mqttPublishDelay/(screenDelay*2))){
    countDelays=0;
    connectPublishMQTT(); // Connect to MQTT and publish data
  }
}

void connectPublishMQTT(){
  if(WiFi.status() != WL_CONNECTED){
    connectWifi(wifiConnectRetries, wifiConnectDelayBetweenRetries);
  } else {
    if(!client.connected()){
      connectMQTT(mqttConnectRetries, mqttConnectDelayBetweenRetries);
    } else {
      publishData();
    }
  }
}

void publishData(){
    client.publish("easv/weather/humid", String(humidity).c_str());
    client.publish("easv/weather/tempC", String(tempC).c_str());
    client.publish("easv/weather/presshpa", String(airPressure).c_str());
}

void initMQTT(){
  Serial.println("Initializing MQTT");
   
  randomSeed(micros());
  client.setServer(mqtt_server, mqtt_port);
  
  connectMQTT(mqttConnectRetries, mqttConnectDelayBetweenRetries);
}
 
void connectMQTT(int retries, int delayBetweenRetries){
  while(!client.connected() && retries>=0){
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);
    if(client.connect(clientId.c_str(), mqtt_user, mqtt_pass)){
      Serial.println("Successfully connected MQTT");
    } else {
      Serial.println("Error!");
      Serial.println(client.state());
      delay(delayBetweenRetries);
      retries--;
    }
  }
}
 
void connectWifi(int retries, int delayBetweenRetries){
  WiFi.begin(ssid, password);
   
  while(status != WL_CONNECTED && retries>=0){
    status = WiFi.status();
    delay(delayBetweenRetries);
    Serial.print(".");
    retries--;
  }
}

void readBME280Values(){
  tempC = bme.readTemperature();
  humidity = bme.readHumidity();
  airPressure = bme.readPressure() / 100.0F;
}

void printValues(int screenDelay) {
  lcd.setCursor(0,0);
  lcd.print("Temp ");
  lcd.print(bme.readTemperature());
  lcd.print("*C");
  
  lcd.setCursor(0,1);
  lcd.print("Pres ");
  lcd.print(airPressure);
  lcd.print(" hPa");
  delay(screenDelay);
  
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("Hum ");
  lcd.print(humidity);
  lcd.print("%");
  
  lcd.setCursor(0,1);
  lcd.print("Net ");
  lcd.print(netStateToString(client.state()));
  delay(screenDelay);
  
  lcd.clear();
}

/*
 * https://pubsubclient.knolleary.net/api#state
Returns
-4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
-3 : MQTT_CONNECTION_LOST - the network connection was broken
-2 : MQTT_CONNECT_FAILED - the network connection failed
-1 : MQTT_DISCONNECTED - the client is disconnected cleanly
0 : MQTT_CONNECTED - the client is connected
1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
*/

String netStateToString(int state){ // LCD short friendly string
    switch(state){
      case MQTT_CONNECTION_TIMEOUT:
        return "Timeout con";
        break;
      case MQTT_CONNECTION_LOST:
        return "Lost con";
        break;
      case MQTT_CONNECT_FAILED:
        return "Con fail";
        break;
      case MQTT_DISCONNECTED:
        return "Disconnect";
        break;
      case MQTT_CONNECTED:
        return "Connected";
        break;
      case MQTT_CONNECT_BAD_PROTOCOL:
        return "Bad protocol";
        break;  
      case MQTT_CONNECT_BAD_CLIENT_ID:
        return "Bad Client Id";
        break;
      case MQTT_CONNECT_UNAVAILABLE:
        return "Unavailable";
        break;
      case MQTT_CONNECT_BAD_CREDENTIALS:
        return "Bad Creds";
        break;  
      case MQTT_CONNECT_UNAUTHORIZED:
        return "Unauthorized";
        break;  
      default:
        return "Unknown ERROR!";
    }
}
