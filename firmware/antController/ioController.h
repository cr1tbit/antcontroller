#pragma once 

#include <Arduino.h>
#include <Wire.h>
#include <PCA95x5.h>


const int EXP_MOS_ADDR = 0x20;
const int EXP_REL_ADDR = 0x21;
const int EXP_OPTO_ADDR = 0x22;


typedef enum {
  RET_OK = 0,
  RET_ERR = -1
} ret_code_t;

typedef enum {
  MOSFET = 0,
  RELAY,
  OPTO,
  TTL,
  OUT_TYPE_COUNT
} output_group_type_t;

typedef enum {
  EXP_MOSFETS = 0,
  EXP_RELAYS,
  EXP_OPTO_TTL,
  EXP_COUNT
} exp_index_t;

typedef struct {
  PCA9555* p_exp;
  int out_num;
  int out_offs;
} output_group_t;

bool pins_changed = false;

void IRAM_ATTR input_pins_isr() {
  pins_changed = true;
  // digitalWrite(PIN_LED_STATUS,~digitalRead(PIN_LED_STATUS));
}

class IoController_ {

  private:
    IoController_() = default;

    TwoWire* _wire;
    PCA9555 expanders[EXP_COUNT];
    output_group_t o_group[OUT_TYPE_COUNT];
    const uint8_t* pins_array {0x00};
    int pins_array_len {0};

    ret_code_t init_controller_objects(){
      init_expander(&expanders[EXP_MOSFETS], EXP_MOS_ADDR );
      init_expander(&expanders[EXP_RELAYS],  EXP_REL_ADDR );
      init_expander(&expanders[EXP_OPTO_TTL],EXP_OPTO_ADDR);

      o_group[MOSFET].p_exp = &expanders[EXP_MOSFETS];
      o_group[MOSFET].out_num = 16;
      o_group[MOSFET].out_offs = 0;

      o_group[RELAY].p_exp = &expanders[EXP_RELAYS];
      o_group[RELAY].out_num = 16;
      o_group[RELAY].out_offs = 0;

      o_group[OPTO].p_exp = &expanders[EXP_OPTO_TTL];
      o_group[OPTO].out_num = 8;
      o_group[OPTO].out_offs = 8;

      o_group[TTL].p_exp = &expanders[EXP_OPTO_TTL];
      o_group[TTL].out_num = 8;
      o_group[TTL].out_offs = 0;

      return RET_OK;
    }

    ret_code_t init_expander(PCA9555* p_exp, int addr){
      p_exp->attach(*_wire,addr);

      p_exp->polarity(PCA95x5::Polarity::ORIGINAL_ALL);
      p_exp->direction(PCA95x5::Direction::OUT_ALL);
      bool write_status = p_exp->write(PCA95x5::Level::L_ALL);

      Serial.printf("init exp %02x ",addr);

      if (write_status){
        Serial.println("OK!");
        return RET_OK;
      }
      Serial.println("ERR!");
      return RET_ERR;
    }

  public:

    //singleton pattern as from https://forum.arduino.cc/t/how-to-write-an-arduino-library-with-a-singleton-object/666625
    static IoController_ &getInstance(){
        static IoController_ instance;
        return instance;
    }

    IoController_(const IoController_ &) = delete; // no copying
    IoController_ &operator=(const IoController_ &) = delete;

    void begin(TwoWire& wire){
      _wire = &wire;
      init_controller_objects();
    }

    ret_code_t init_inputs(uint8_t pin_in_buff_ena, const uint8_t* const pins_array, int pins_array_len){
      this->pins_array = pins_array;
      this->pins_array_len = pins_array_len;

      for (int iInput = 0; iInput < pins_array_len; iInput++){  
        //Serial.printf("p %d!\n\r",(*p_pins_array)[iInput]);              
        pinMode(pins_array[iInput], INPUT_PULLDOWN);
        attachInterrupt(iInput, input_pins_isr, CHANGE);
      }

      pinMode(pin_in_buff_ena,OUTPUT);
      digitalWrite(pin_in_buff_ena,0);
      return RET_OK;
    }

    bool set_output(output_group_type_t group, int pin_num, bool val){
      
      if (pin_num > o_group[group].out_num){
        return false;
      }

      int offs_pin = pin_num + o_group[group].out_offs;

      Serial.printf(
        "set pin %d %s on group %d\n\r",
        offs_pin,
        val ? "on" : "off",
        (int)group);

      o_group[group].p_exp->write(
        (PCA95x5::Port::Port) offs_pin,
        (PCA95x5::Level::Level )val
      );
      return true;
    }

    bool set_output_bits(output_group_type_t group, uint16_t bits){
      
      bits &= (0xFFFF >> 16-o_group[group].out_num);      
      bits <<= o_group[group].out_offs;

      Serial.printf("Write bits %04x on group %d\n\r",bits,(int)group);

      o_group[group].p_exp->write(bits);
      return true;
    }

    bool get_input_bits(uint16_t* res){
      *res = 0;

      for (int iInput = 0; iInput < pins_array_len; iInput++){
        uint8_t pin_to_read = pins_array[iInput];
        // Serial.printf("read %d!\n\r",pin_to_read);
        if (digitalRead(pin_to_read) == true){
          // Serial.printf("%d ishigh !\n\r",pin_to_read);
          *res |= (uint16_t)0x01<<iInput;
        }
      }
      return true;
    }
};

IoController_ &IoController = IoController.getInstance();

// extern IoController_ &IoController;
