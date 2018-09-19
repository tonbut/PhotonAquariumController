#ifndef SENSORS_H
#define SENSORS_H
#include "application.h"
#include "RGBDriver.h"
#include <DS18B20.h>
#include <Adafruit_DHT.h>

void initializeSensors();
float getWaterTemp();
float getLightLevel();
float getLightLux();
float getAirTemp();
float getHumidity();
long getLastTempReading();
long getLastWaterReading();
void setFan(int speed);
int getFan();
int getFlapStatus();
float getPHVoltage();
float getPHVoltageFiltered();
float getPH();
int getMotionStatus();

#endif