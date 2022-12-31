// Ant controller firmware
// (C) cr1tbit 2023

#include <Wire.h>

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

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

void IRAM_ATTR input_pins_isr() {
  digitalWrite(PIN_LED_STATUS,~digitalRead(PIN_LED_STATUS));
}


int analyze_path(String subpath){
  Serial.println("Analyzing subpath: " + subpath);
  //schema is /api/<CMD>/<INDEX>/<VALUE>

  int cmd_pos = subpath.indexOf('/');
  if (cmd_pos < 0){
    Serial.println("no slashes?");
    return 404;
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

int api_operation(String& command, int index, String& value){
  Serial.printf("API call: ");
  Serial.print(command);
  Serial.printf("/%d/",index);
  Serial.println(value);

  if (command == "MOSf"){
    ioController.set_output_bits(MOSFET, index);
  }

  return 200;
}

void initialize_http_server(){
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    Serial.println("Is filesystem properly generated and uploaded?");
    return;
  } else {
    Serial.println("SPIFFS initialized. Files:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
  
    while(file){
      Serial.println(file.name());
      file = root.openNextFile();
    }
    Serial.println("");
  }

  server.serveStatic("/", SPIFFS, "/");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request){
    int ret_code = analyze_path(request->url().substring(5));
    request->send(ret_code, "text/plain", "Request ok");
  });
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


uint16_t t = 0x01;

void loop()
{
  handle_io_pattern(PIN_LED_STATUS);
  // Serial.println("Main loop...");

  // ioController.set_output(RELAY, 0, true);
  // ioController.set_output(MOSFET, 1, true);
  // ioController.set_output(OPTO, 2, true);
  // ioController.set_output(TTL, 3, true);

  // t++;

  // ioController.set_output_bits(RELAY, t);
  // ioController.set_output_bits(MOSFET, t);
  // ioController.set_output_bits(OPTO, t);
  // ioController.set_output_bits(TTL, t);
  // Serial.printf(
  //   "read %04x\n",
  //   ioController.get_input_bits()
  // );
  
  delay(200);


}