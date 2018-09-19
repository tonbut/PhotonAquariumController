#ifndef WEATHER_H
#define WEATHER_H
#include "application.h"
#include "lightingLib.h"

void initiateWeather(int aType, int aDuration);
void resetWeather();
void processWeather();
char* getWeatherMsg();

class WeatherPattern
{
  public:
    WeatherPattern() {}
    virtual void process() = 0;
    virtual int isFinished() = 0;
    virtual void setWeatherMessage(char* msg)=0;
    virtual ~WeatherPattern() {};
};


#endif