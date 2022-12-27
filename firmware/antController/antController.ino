// Ant controller firmware
// (C) cr1tbit 2023

#include <Wire.h>
#include <PCA95x5.h>

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#include "board_config.h"
#include "secrets.h"

PCA9555 exp_mosfets;
PCA9555 exp_relays;
PCA9555 exp_opto_io;

#define FW_REV "0.1.0"

AsyncWebServer server(80);


typedef enum {
  PATTERN_NONE,
  PATTERN_HBEAT,
  PATTERN_ERR  
} led_patterns_type_t;

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

  Wire.begin();
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

void initialize_io(){
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS,HIGH);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  scan_i2c_rail();

  exp_mosfets.attach(Wire,0x20);
  exp_mosfets.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  exp_mosfets.direction(PCA95x5::Direction::OUT_ALL);
  exp_mosfets.write(PCA95x5::Level::L_ALL);

  exp_relays.attach(Wire,0x21);
  exp_relays.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  exp_relays.direction(PCA95x5::Direction::OUT_ALL);
  exp_relays.write(PCA95x5::Level::L_ALL);

  exp_opto_io.attach(Wire,0x22);
  exp_opto_io.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  exp_opto_io.direction(PCA95x5::Direction::OUT_ALL);
  exp_opto_io.write(PCA95x5::Level::L_ALL);

  for (int iInput = 0; iInput < input_pins_array_len; iInput++){
    int current_pin_no = input_pins_array[iInput];

    pinMode(current_pin_no, INPUT_PULLDOWN);
	  attachInterrupt(current_pin_no, input_pins_isr, CHANGE);
  }
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
  server.begin();
}






void setup()
{
  Serial.begin(115200);
  Serial.println("AntController fw rev. "FW_REV);
  Serial.println("Compiled " __DATE__ " " __TIME__);

  initialize_io();

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
  delay(100);
}