#pragma once 

const int EXP_MOS_ADDR = 0x20;
const int EXP_REL_ADDR = 0x21;
const int EXP_OPTO_ADDR = 0x22;

const int PIN_I2C_SCL = 4;
const int PIN_I2C_SDA = 5;
const int PIN_I2C_RST = 12;


// board inputs

const int PIN_INPUT_1 = 14;
const int PIN_INPUT_2 = 15;
const int PIN_INPUT_3 = 16;
const int PIN_INPUT_4 = 17;

const int PIN_INPUT_5 = 18;
const int PIN_INPUT_6 = 19;
const int PIN_INPUT_7 = 21;
const int PIN_INPUT_8 = 22;

const int PIN_INPUT_9 = 23;
const int PIN_INPUT_10 = 25;
const int PIN_INPUT_11 = 26;
const int PIN_INPUT_12 = 27;

const int input_pins_array[] = {
  PIN_INPUT_1,
  PIN_INPUT_2,
  PIN_INPUT_3,
  PIN_INPUT_4,
  PIN_INPUT_5,
  PIN_INPUT_6,
  PIN_INPUT_7,
  PIN_INPUT_8,
  PIN_INPUT_9,
  PIN_INPUT_10,
  PIN_INPUT_11,
  PIN_INPUT_12
};

const int input_pins_array_len = sizeof(input_pins_array)/sizeof(input_pins_array[0]);

// user interface
const int PIN_BOOT_BUT1 = 0;
const int PIN_BUT2 = 33;
const int PIN_BUT3 = 34;
const int PIN_BUT4 = 35;

const int PIN_LED_STATUS = 2;



// trash

/*
for (size_t i = 0; i < 16; ++i) {
    Serial.print("set port high: ");
    Serial.println(i);

    exp_mosfets.write((PCA95x5::Port::Port)i, PCA95x5::Level::H);
    exp_relays.write((PCA95x5::Port::Port)i, PCA95x5::Level::H);
    exp_opto_io.write((PCA95x5::Port::Port)i, PCA95x5::Level::H);
    // Serial.println(ioex.read(), BIN);
    delay(500);
  }

  for (size_t i = 0; i < 16; ++i) {
    Serial.print("set port low: ");
    Serial.println(i);

    exp_mosfets.write((PCA95x5::Port::Port)i, PCA95x5::Level::L);
    exp_relays.write((PCA95x5::Port::Port)i, PCA95x5::Level::L);
    exp_opto_io.write((PCA95x5::Port::Port)i, PCA95x5::Level::L);

    // Serial.println(ioex.read(), BIN);
    delay(500);
  }


typedef {
  int esp_pin_num;
  uint32_t pin_bitmask;
  bool is_enabled;
  const char[20] name_verbose;
} board_input_t;


*/

