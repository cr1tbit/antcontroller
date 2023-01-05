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


#define FW_REV "0.1.0"

AsyncWebServer server(80);

IoController ioController;

void handle_io_pattern(uint8_t pin){
  static uint32_t pattern_counter = 0;
  static uint8_t heartbeat_pattern[] = {1,0,0,1,0,0,0,0,0,0,0,0,0};
  
  digitalWrite(pin, heartbeat_pattern[
    pattern_counter % sizeof(heartbeat_pattern)
  ]);
    
  pattern_counter++;
}

void scan_i2c_rail()
{
  Serial.println ();
  Serial.println ("Scanning I2C rail...");
  byte count = 0;

  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission (i); 
    if (Wire.endTransmission () == 0) 
    {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
    }
  }
  Serial.print ("Found ");      
  Serial.print (count, DEC);        // numbers of devices
  Serial.println (" device(s).\n");
}


int analyze_path(String subpath){
  Serial.println("Analyzing subpath: " + subpath);
  //schema is /api/<CMD>/<INDEX>/<VALUE>

  int cmd_pos = subpath.indexOf('/');
  if (cmd_pos < 0){
    Serial.println("no slashes?");
    return 400;
  } else {
    String command = subpath.substring(0,cmd_pos);

    int index_pos = subpath.indexOf('/',cmd_pos+1);
    if (index_pos < 0){
      // Serial.println("no slashes?");
      String dummy("");
      return api_operation(command,-1,dummy);
    } else {
      int index = subpath.substring(cmd_pos+1,index_pos).toInt();
      String value = subpath.substring(index_pos+1);
      return api_operation(command,index,value);      
    }  
  }
}

int api_operation(String& command, int index, String& value) {
  Serial.printf("API call: ");
  Serial.print(command);
  Serial.printf("/%d/", index);
  Serial.println(value);

  if (index < 0){
    return 400;
  }

  bool b_value = value.indexOf("on") >= 0 ? true : false;

  if (command == "MOSbits") {
    ioController.set_output_bits(MOSFET, index);
  } else if (command == "RELbits") {
    ioController.set_output_bits(RELAY, index);
  } else if (command == "OPTbits") {
    ioController.set_output_bits(OPTO, index);
  } else if (command == "TTLbits") {
    ioController.set_output_bits(TTL, index);
  } else if (command == "MOS") {
    ioController.set_output(MOSFET, index, b_value);
  } else if (command == "REL") {
    ioController.set_output(RELAY, index, b_value);
  } else if (command == "OPT") {
    ioController.set_output(OPTO, index, b_value);
  } else if (command == "TTL") {
    ioController.set_output(TTL, index, b_value);
  } else {
    Serial.print("unrecognised command: " + command);
    return 400;
  }

  return 200;
}

void initialize_http_server(){
  if(!LittleFS.begin(false)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    Serial.println("Is filesystem properly generated and uploaded?");
    return;
  } else {
    Serial.println("LittleFS initialized. Files:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
  
    while(file){
      Serial.println(file.name());
      file = root.openNextFile();
    }
    Serial.println("");
  }

  server.serveStatic("/", LittleFS, "/");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false);
  });
  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request){
    int ret_code = analyze_path(request->url().substring(5));
    request->send(ret_code, "text/plain", "Request ok");
  });

  server.addHandler(new SPIFFSEditor(LittleFS, "test","test"));
  
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
}

void setup(){
  Serial.begin(115200);
  Serial.println("============================");
  Serial.println("AntController fw rev. "FW_REV);
  Serial.println("Compiled " __DATE__ " " __TIME__);

  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS,HIGH);
  pinMode(PIN_BOOT_BUT1, INPUT);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
  ioController.begin(Wire);
  ioController.init_inputs(
    PIN_IN_BUFF_ENA,
    (const uint8_t*)input_pins_array,
    input_pins_array_len
  );

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(3000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  initialize_http_server();
}


void loop()
{
  handle_io_pattern(PIN_LED_STATUS);  
  Serial.printf("bits: %04x\n",ioController.get_input_bits());
  
  delay(500);
}