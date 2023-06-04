// Ant controller firmware
// (C) cr1tbit 2023

#include <Wire.h>

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h"
#include <SPIFFSEditor.h>

#include "board_config.h"
#include "secrets.h"

#include "ioController.h"

#include <sstream>

#define FW_REV "0.3.0"

AsyncWebServer server(80);
AsyncEventSource events("/events");

void handle_io_pattern(uint8_t pin){
  static uint32_t pattern_counter = 0;
  static uint8_t heartbeat_pattern[] = {1,0,0,1,0,0,0,0,0,0,0,0,0};
  
  digitalWrite(pin, heartbeat_pattern[
    pattern_counter % sizeof(heartbeat_pattern)
  ]);
    
  pattern_counter++;
}


String handle_api_call(const String &subpath, int* ret_code){
  Serial.println("Analyzing subpath: " + subpath);
  //schema is /api/<CMD>/<INDEX>/<VALUE>

  ///api/INP/ <- reads in
  ///api/MOS/ <- reads out
  ///api/REL/8/on <- sets state of 8 to high
  ///api/OPT/bits/ <- reads all bits of a channel

  *ret_code = 200; 

  output_group_type_t output;

  std::vector<String>api_split;

  int delimiterIndex = 0;
  int previousDelimiterIndex = 0;
  
  while ((delimiterIndex = subpath.indexOf("/", previousDelimiterIndex)) != -1) {
    api_split.push_back(subpath.substring(previousDelimiterIndex, delimiterIndex));
    previousDelimiterIndex = delimiterIndex + 1;
  }  
  api_split.push_back(subpath.substring(previousDelimiterIndex));
  //now api_split contains all the parts of the path

  if (api_split.size() == 0){
    return "ivnalid API call: " + subpath;
  }  

  return IoController.handleApiCall(api_split);
}

void initialize_http_server(){
  if(!LittleFS.begin(false)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    Serial.println("Is filesystem properly generated and uploaded?");
    Serial.println("(Note, in debug build filesystem does not autoformate)");
    return;
  } else {
    Serial.println("LittleFS initialized.");
    Serial.printf(
      "Using %d/%d kb.\nFiles:\n",
      LittleFS.usedBytes()/1024,
      LittleFS.totalBytes()/1024
    );

    File root = LittleFS.open("/");
    File file = root.openNextFile();  
    while(file){
      Serial.printf("%s: %db\n",file.name(),file.size());
      file = root.openNextFile();
    }
    Serial.println("");
  }

  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request){
    int ret_code = 418;
    String api_result = handle_api_call(request->url().substring(5),&ret_code);
    Serial.printf("API call result:\n%s\n",api_result.c_str());
    request->send(ret_code, "text/plain", api_result);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false);
  });
  server.serveStatic("/", LittleFS, "/");

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it gat is: %u\n", client->lastId());
    }
    //send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);

  server.addHandler(new SPIFFSEditor(LittleFS, "test","test"));
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
}

void setup(){
  Serial.begin(115200);
  Serial.println("============================");
  Serial.println("AntController fw rev. " FW_REV);
  Serial.println("Compiled " __DATE__ " " __TIME__);

  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS,HIGH);
  pinMode(PIN_BOOT_BUT1, INPUT);
  pinMode(PIN_BUT2, INPUT);
  pinMode(PIN_BUT3, INPUT);
  pinMode(PIN_BUT4, INPUT);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
  IoController.begin(Wire);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(3000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  initialize_http_server();
}

int counter = 0;
void loop()
{
  int dummy_ret;
  handle_io_pattern(PIN_LED_STATUS);
  events.send("dupa","myevent",millis());
  delay(1000);
  if (digitalRead(PIN_BUT4) == LOW){
    Serial.println("Button 4 is pressed - doing test call");
    handle_api_call("REL/bits/"+String(counter++),&dummy_ret);
  }
}