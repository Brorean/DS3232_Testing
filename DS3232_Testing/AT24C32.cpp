#include <Arduino.h>
#include <Wire.h>
#include "AT24C32.h"

#define min(a,b) ((a)<(b)?(a):(b))

const uint8_t EEPROM_ADDRESS = 0x57;

AT24C32::AT24C32()
{
  Wire.begin();
}

uint8_t AT24C32::is_present(void)      // check if the device is present
{
  Wire.beginTransmission(EEPROM_ADDRESS);
  return Wire.endTransmission();
}

int AT24C32::read_mem(int addr, uint8_t *buff, uint8_t count) {
  int index = 0;
  uint8_t bytes;

  while (count > 0) {
    Wire.beginTransmission(EEPROM_ADDRESS);
#if ARDUINO >= 100
    Wire.write(addr>>8);   // Address MSB
    Wire.write(addr&0xff); // Address LSB
#else
    Wire.send(addr>>8);   // Address MSB
    Wire.send(addr&0xff); // Address LSB
#endif
    Wire.endTransmission();

    bytes = min(count, 128);
    Wire.requestFrom(EEPROM_ADDRESS, bytes);

    while (Wire.available() && count>0) {
#if ARDUINO >= 100
      buff[index] = Wire.read();
#else
      buff[index] = Wire.receive();
#endif
      index++; count--; addr++;
    }  /* while */  
  }
  return index;
}

uint8_t AT24C32::write_mem(int addr, uint8_t value) {
  uint8_t retval=0;
  Wire.beginTransmission(EEPROM_ADDRESS);
#if ARDUINO >= 100
    Wire.write(addr>>8);   // Address MSB
    Wire.write(addr&0xff); // Address LSB
    Wire.write(value);
#else
    Wire.send(addr>>8);   // Address MSB
    Wire.send(addr&0xff); // Address LSB
    Wire.send(value);
#endif
  retval = Wire.endTransmission();

  return retval;
}

