/*
  XGZP6897D.cpp - Library for using a familly of pressure sensors from CFSensor.com
  I2C sensors
  Created by Francis Sourbier
  GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  This library is free software; 
  Released into the public domain
*/
#include <XGZP6828D.h>
#include <Wire.h>
//  Descriptor. K depends on the exact model of the sensor. See datasheet and documentation
XGZP6828D::XGZP6828D(uint16_t K)
{
  _K = K;
  _I2C_address = I2C_device_address;
}
//
//  
bool XGZP6828D::begin()
{
  Wire.begin(0,1);
  // A basic scanner, see if it ACK's
  Wire.beginTransmission(_I2C_address);
  if (Wire.endTransmission() == 0) {
    return true;  // Ok device is responding
  }
  return false; // device not responding
}
//  Return raw integer values for temperature and pressure.
//  The raw integer of temperature must be devided by 256 to convert in degree Celsius
//  The raw integer of pressure must be devided by the K factor to convert in Pa
void XGZP6828D::readSensor(float &temperature, float &pressure)
 {
  int32_t pressure_adc;
  int32_t  temperature_adc ;
  uint8_t pressure_H, pressure_M, pressure_L, temperature_H, temperature_L;
  uint8_t CMD_reg;
  // start conversion
  Wire.beginTransmission(_I2C_address);
  Wire.write(0x30);
  Wire.write(0x0A);   //start combined conversion pressure and temperature
  Wire.endTransmission();
  // wait until the end of conversion (Sco bit in CMD_reg. bit 3)
  do {
    Wire.beginTransmission(_I2C_address);
    Wire.write(0x30);                       //send 0x30 CMD register address
    Wire.endTransmission();
    Wire.requestFrom(_I2C_address, byte(1));
    CMD_reg = Wire.read();                //read 0x30 register value
  } while ((CMD_reg & 0x08) > 0);        //loop while bit 3 of 0x30 register copy is 1
  // read temperature and pressure
  Wire.beginTransmission(_I2C_address);
  Wire.write(0x06);                        //send pressure high byte register address
  Wire.endTransmission();
  Wire.requestFrom(_I2C_address, byte(5)); // read 3 bytes for pressure and 2 for temperature
  pressure_H = Wire.read();
  pressure_M = Wire.read();
  pressure_L = Wire.read();
  pressure_adc = ((uint32_t)(pressure_H * 65536)) + (uint32_t) (pressure_M*256) +(uint32_t) pressure_L;
  if (pressure_adc > 8388608)
  pressure = (pressure_adc - 16777216); //unit is Pa
  else
  pressure = pressure_adc;
  
  temperature_H = Wire.read();
  temperature_L = Wire.read();
  temperature_adc = ((uint16_t)temperature_H *256) + (uint16_t) temperature_L;
  if (temperature_adc > 131072)
  temperature = (temperature_adc - 65536) / 256; //unit is ℃
  else
  temperature = temperature_adc / 256; //unit is ℃
#ifdef debugFS
  Serial.print(String(pressure_H, HEX));
  Serial.print("," + String(pressure_M, HEX));
  Serial.print("," + String(pressure_L, HEX));
  Serial.print(":" + String(pressure_adc, HEX));
  Serial.print(" – " + String(temperature_H, HEX));
  Serial.print("," + String(temperature_L, HEX));
  Serial.print(":" + String(temperature_adc, HEX));
  Serial.println();
#endif
  // return ;
} 
//  Read temperature (degree Celsius), and pressure (PA)
//  Return float values
// void XGZP6828D::readSensor(float &temperature, float &pressure)
// {
//   int32_t rawPressure;
//   int16_t  rawTemperature ;
//   readRawSensor(rawsure);
//   pressure = (float)rawPressure;
//   temperature = float(rawTemperatTemperature,rawPresure);
//   #ifdef debugFS
//   Serial.print(" - " + String(temperature) + ":" + String(pressure));
//   Serial.println();
// #endif
// }
