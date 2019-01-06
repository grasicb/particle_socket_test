// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>

// This #include statement was automatically added by the Particle IDE.
#include <PietteTech_DHT.h>

// This #include statement was automatically added by the Particle IDE.
#include "application.h"
#include "CloudConnect.h"
#include <string>

SerialLogHandler logHandler(LOG_LEVEL_ALL);

//SYSTEM_THREAD(ENABLED);
//SYSTEM_MODE(MANUAL);

// Server connection
CloudConnect *cc;
byte server[] = { 192, 168, 1, 27 };
int port = 8081;
void handleCloudEvent(JsonObject& event); // Forward declaration

//Other vars
int led2 = D7; 
int led = D2; 
int photosensor = A0;
long lastBlink;
long lastSensorRead;
long lastDimmStep;
short dimmStep = 5;
int dimmValue = 0;
boolean ledValue = false;
int analogvalue; 

//DHT Sensor
#define DHTPIN D6
#define DHTTYPE DHT22
PietteTech_DHT dht(DHTPIN, DHTTYPE);
long lastDhtRead;
float temperature  = 0;
float humidity  = 0;
bool bDHTstarted = false;


void setup() {
    Serial.begin(57600);
    
    // Wait for a USB serial connection for up to 30 seconds
    delay(10000);
    waitFor(Particle.connected, 30000);
    /*
    WiFi.on();
    WiFi.connect();
    waitFor(WiFi.ready, 30000);
    */
    cc = new CloudConnect(server, port);
    cc->registerListener(handleCloudEvent);
    
    //Init leds
    pinMode(led, OUTPUT);
    pinMode(led2, OUTPUT);
    digitalWrite(led, HIGH);

    Log.info("Application loaded");
}



void loop() {
    //Process cloud messages
    cc->process();
    
    /*
    // Dimm LED
    if (millis() > lastDimmStep+50) {
        dimmValue += dimmStep;
        if (dimmValue > 255) {
            dimmValue = 0;
        }
        analogWrite(led, dimmValue);
        lastDimmStep = millis();
    }
    */

    
    // Blink LED
    if (millis() > lastBlink+1000) {
      ledValue = !ledValue;
      
      if (ledValue) {
          digitalWrite(led2, HIGH);
      }else{
          digitalWrite(led2, LOW);
      }
      lastBlink = millis();
    }
    
    // Read light sensor
    if (millis() > lastSensorRead+500) {
        analogvalue = analogRead(photosensor);
        //Serial.printlnf("Light sensor: %d", analogvalue);
        lastSensorRead = millis();
        //client.printf("Light sensor: %d", analogvalue);
    }
    
    // Read temperature and humidity
    if (millis() > lastDhtRead+2000) {
        int result = dht.acquireAndWait(2000); // wait up to 2 sec (default indefinitely)
        
        if(result == DHTLIB_OK) {
            float h = dht.getHumidity();
            float t = dht.getCelsius(); 
            
            //if there is change in values read from sensor, then it emits the event to the server
            if(h != humidity || t != temperature) {
                humidity = h;
                temperature = t;
                
                long ttime = Time.now()*1000;
                const int capacity = JSON_OBJECT_SIZE(8);
                StaticJsonBuffer<capacity> jsonBuffer;
                JsonObject& root = jsonBuffer.createObject();
                root["type"] = "event";
                root["event"] = "temperature";
                root["timestamp"] = ttime;
                std::string str (Time.format(ttime, TIME_FORMAT_ISO8601_FULL).c_str());
                root["timestamp_human"] = str;
                JsonObject& payload = root.createNestedObject("payload");
                payload["temperature"] = t;
                payload["humidity"] = h;
                
                /*
                std::string str1;
                root.printTo(str1);
                Log.trace(str1.c_str());
                */
                
                cc->emitEvent(root);
            }
            
            Log.trace(String::format("DHT sensor || temperature: %.2f humidity: %.2f", t, h));
        }else{
            printDhtError(result);
        }
        
        lastDhtRead = millis();
    }

}

void printDhtError(int status) {
    switch (status) {
          case DHTLIB_OK:
            Log.info("DHT sensor read successfull.");
            break;
          case DHTLIB_ERROR_CHECKSUM:
            Log.warn("DHT Sensor Error \tChecksum error");
            break;
          case DHTLIB_ERROR_ISR_TIMEOUT:
            Log.warn("DHT Sensor Error \tISR time out error");
            break;
          case DHTLIB_ERROR_RESPONSE_TIMEOUT:
            Log.warn("DHT Sensor Error \tResponse time out error");
            break;
          case DHTLIB_ERROR_DATA_TIMEOUT:
            Log.warn("DHT Sensor Error \tData time out error");
            break;
          case DHTLIB_ERROR_ACQUIRING:
            Log.warn("DHT Sensor Error \tAcquiring");
            break;
          case DHTLIB_ERROR_DELTA:
            Log.warn("DHT Sensor Error \tDelta time to small");
            break;
          case DHTLIB_ERROR_NOTSTARTED:
            Log.warn("DHT Sensor Error \tNot started");
            break;
          default:
            Log.warn("DHT Sensor Error \tUnknown error");
            break;
          }
}

int ledToggle(String command) {

    if (command=="on") {
        digitalWrite(led,HIGH);
        return 1;
    }
    else if (command=="off") {
        digitalWrite(led,LOW);
        return 0;
    }
    else {
        return -1;
    }
}


void handleCloudEvent(JsonObject& event) {
    Log.trace("handleCloudEvent: %s", event["event"]);
    
    if (event["event"] == "ledDimm") {
        int val = event["payload"]["value"];
        Log.trace(String::format("Change led brightness to %d%c", val), '%');
        analogWrite(led, val*255/100);
    }

    /*
    if(input.startsWith("led:")) {
        //String ledVal = input.substring(4);
        Log.trace(String::format("Change led brightness to %d%c", input.substring(4).toInt()), '%');
        analogWrite(led, input.substring(4).toInt()*255/100);
    }
    */
}