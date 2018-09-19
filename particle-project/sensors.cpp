#include "sensors.h"

DHT dht(D2, DHT22);
DS18B20 ds18b20(D3);

#define CLK 4 //pins definitions for the driver        
#define DIO 5
RGBdriver Driver(CLK,DIO);

float lastGoodAirTemp=0.0;
float lastGoodAirHumi=0.0;
float lastGoodWaterTemp=0.0;
long lastTempReading=0;
long lastWaterReading=0;
float lastLight=-1.0f;
int fanSpeed2=0;

void initializeSensors()
{
    dht.begin();
    pinMode(D3, INPUT_PULLUP);
    ds18b20.setResolution(TEMP_12_BIT);
    
    //flap switch
    pinMode(D6, INPUT_PULLUP);
    
    pinMode(A1, INPUT);
    pinMode(A2, INPUT);
}

float getWaterTemp() {
    int dsAttempts=0;
    double celsius=0.0;
    if(!ds18b20.search())
    {
        ds18b20.resetsearch();
      
        celsius = ds18b20.getTemperature();
        while (!ds18b20.crcCheck() && dsAttempts < 4)
        {
            Serial.println("Caught bad value.");
            dsAttempts++;
            Serial.print("Attempts to Read: ");
            Serial.println(dsAttempts);
            if (dsAttempts == 3)
            {
                delay(1000);
            }
            ds18b20.resetsearch();
            celsius = ds18b20.getTemperature();
            continue;
        }
        if (celsius>0.1 && celsius<50.0)
        {   if (lastGoodWaterTemp==0)
                lastGoodWaterTemp=celsius;
            else if (abs(lastGoodWaterTemp-celsius)<5.0)
                lastGoodWaterTemp=lastGoodWaterTemp*0.9f + celsius*0.1f;
            long now = Time.now();
            lastWaterReading=now;
        }
    }
    return lastGoodWaterTemp;
}

float getLightLevel()
{
    //get light level 0=light 4095=dark
    int a0=analogRead(A0);
	float light=1.0f-((float)a0)/4095.0f;
	if (lastLight==-1)
	{   lastLight=light;
	}
	else
	{   lastLight=lastLight*0.9f+light*0.1f;
	}
	return lastLight;
    
}

float getLightLux()
{
    int a0=analogRead(A0);
    float Vout=a0*0.001220703125f;
    //float lux1=500/(10*((5-Vout)/Vout));//use this equation if the LDR is in the upper part of the divider
    float lux=(2500/Vout-500); ///10;
    return lux;
}


float getAirTemp()
{
    //get DHT22 stuff
	float temp = dht.getTempCelcius();
    float humi = dht.getHumidity();
    if (temp>0.1 && temp<50)
    {   
        if (lastGoodAirTemp==0.0 || abs(lastGoodAirTemp-temp)<10.0)
        {
            lastGoodAirTemp=temp;
            lastGoodAirHumi=humi;
            long now = Time.now();
            lastTempReading=now;
        }
    }
    return lastGoodAirTemp;
}

float getHumidity()
{   return lastGoodAirHumi;
}

long getLastTempReading()
{   return lastTempReading;
}

long getLastWaterReading()
{   return lastWaterReading;
}

void setFan(int fan)
{
    if (fan<0) fan=0;
    if (fan>255) fan=255;
    fanSpeed2=fan;
    Driver.begin(); // begin
    Driver.SetColor(0,fan,0); //Red. first node data
    Driver.end();
}

int getFan()
{   return fanSpeed2;
}

int getFlapStatus()
{
    return !digitalRead(D6);
}

float lastPHVoltage=-1;

float getPHVoltageFiltered()
{   return lastPHVoltage;
}

float getPHVoltage()
{   int r=analogRead(A1);
    float v=(3.3f*r)/4095.0f;
    if (lastPHVoltage<0)
    {   lastPHVoltage=v;
    }
    else
    {   lastPHVoltage=lastPHVoltage*0.98f+v*0.02f;
    }
    return v;
}
float getPH()
{
    return 7.0f;
}
int getMotionStatus()
{
    return digitalRead(A2);
}


