#ifndef LIGHTINGLIB_H
#define LIGHTINGLIB_H
#include "application.h"
#include <math.h>
#include <Adafruit_PCA9685.h>

const int MORPH_TIMER_MS=20;
const int LEDS=16;
const int NATURAL_TRANSITION_PERIOD=30000; // start and end
const int NATURAL_SLOW_TRANSITION_PERIOD=360000; // between times of day

void initializeLEDDriver(int aFreq);
void restoreExistingLEDState();
void parseSchedule();
void adjustLights();
void colorMultiply(float v[], float f);
void colorMultiplyVector(float v[], const float n[], float m);
void colorMultiplyRange(float v[], float f, int first, int last);
void parseColorString(const char* str, float result[]);
void setTarget2(float color[], int fadeMS);
int getScheduleRowForTime(int now, int strict);
void initiateSchedule(int i, int fadeMS);
float getTotalLEDPower();
void morph(float v[]);
char* getCurrentScheduleString();
int isAtHome();
void getCurrentScheduleLighting(float result[]);
void resetToSchedule();
void setLEDOffset(int aLED, float aValue);
#endif