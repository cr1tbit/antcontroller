
#include "arduino_utils.h"


// void scan_i2c_rail(TwoWire& _wire)
// {
//   Serial.println ();
//   Serial.println ("Scanning I2C rail...");
//   byte count = 0;

//   for (byte i = 8; i < 120; i++)
//   {
//     _wire.beginTransmission (i); 
//     if (_wire.endTransmission () == 0) 
//     {
//       Serial.print ("Found address: ");
//       Serial.print (i, DEC);
//       Serial.print (" (0x");
//       Serial.print (i, HEX);
//       Serial.println (")");
//       count++;
//     }
//   }
//   Serial.print ("Found ");      
//   Serial.print (count, DEC);        // numbers of devices
//   Serial.println (" device(s).\n");
// };