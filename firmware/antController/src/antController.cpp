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
AsyncEventSource events("/events");

String api_operation(String& command, int index, String& value, int* ret_code);

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


String analyze_path(String subpath, int* ret_code){
  Serial.println("Analyzing subpath: " + subpath);
  //schema is /api/<CMD>/<INDEX>/<VALUE>

  int cmd_pos = subpath.indexOf('/');
  if (cmd_pos < 0){
    *ret_code = 400;
    return String("no slashes?");
  } else {
    String command = subpath.substring(0,cmd_pos);

    int index_pos = subpath.indexOf('/',cmd_pos+1);
    if (index_pos < 0){
      String dummy("");
      return api_operation(command,-1,dummy, ret_code);
    } else {
      int index = subpath.substring(cmd_pos+1,index_pos).toInt();
      String value = subpath.substring(index_pos+1);
      return api_operation(command,index,value,ret_code);      
    }  
  }
}

String api_operation(String& command, int index, String& value, int* ret_code) {
  Serial.printf("API call: ");
  Serial.print(command);
  Serial.printf("/%d/", index);
  Serial.println(value);

  if (index < 0){
    if (command == "INPbits") {
      uint16_t bits;
      IoController.get_input_bits(&bits);
      *ret_code = 200; 
      return String("Inputs ") + String(bits);
    } else {
      *ret_code = 400; 
      return String("invalid index");
    }
  }

  bool b_value = value.indexOf("on") >= 0 ? true : false;
  bool is_succ = false;
  

  if (command == "MOSbits") {
    is_succ = IoController.set_output_bits(MOSFET, index);
  } else if (command == "RELbits") {
    is_succ = IoController.set_output_bits(RELAY, index);
  } else if (command == "OPTbits") {
    is_succ = IoController.set_output_bits(OPTO, index);
  } else if (command == "TTLbits") {
    is_succ = IoController.set_output_bits(TTL, index);
  } else if (command == "MOS") {
    is_succ = IoController.set_output(MOSFET, index, b_value);
  } else if (command == "REL") {
    is_succ = IoController.set_output(RELAY, index, b_value);
  } else if (command == "OPT") {
    is_succ = IoController.set_output(OPTO, index, b_value);
  } else if (command == "TTL") {
    is_succ = IoController.set_output(TTL, index, b_value);
  } else {
    *ret_code = 400; 
    return String("unrecognised command: " + command);    
  }

  if (is_succ){
    *ret_code = 200;
    return String("OK: " + command);   
  } else {
    *ret_code = 500;
    return String("ERR executing: " + command);   
  }  
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

  server.serveStatic("/", LittleFS, "/");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false);
  });
  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request){
    int ret_code = 418;
    String api_result = analyze_path(request->url().substring(5),&ret_code);
    Serial.printf("API call result:\n%s\n",api_result.c_str());
    request->send(ret_code, "text/plain", api_result);
  });

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

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
  IoController.begin(Wire);
  IoController.init_inputs(
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

int counter = 0;
void loop()
{
  handle_io_pattern(PIN_LED_STATUS);
  // uint16_t inp_bits;
  // IoController.get_input_bits(&inp_bits);
  // Serial.printf("bits: %04x\n",inp_bits);
  events.send("dupa","myevent",millis());
  delay(1000);
}