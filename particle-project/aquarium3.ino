// This #include statement was automatically added by the Particle IDE.
#include <HttpClient.h>

// This #include statement was automatically added by the Particle IDE.
#include "sensors.h"

// This #include statement was automatically added by the Particle IDE.
//#include <Adafruit_DHT.h>

// This #include statement was automatically added by the Particle IDE.
//#include <DS18B20.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_PCA9685.h>

// This #include statement was automatically added by the Particle IDE.
#include "weather.h"

// This #include statement was automatically added by the Particle IDE.
#include "RGBDriver.h"

// This #include statement was automatically added by the Particle IDE.
#include "lightingLib.h"

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

String lastData;
void morphCallback();
void minuteCallback();
Timer morphTimer(MORPH_TIMER_MS, morphCallback);
Timer minuteTimer(1000*60, minuteCallback);
int minuteOffset=0;


HttpClient http;
http_request_t request;
http_response_t response;
http_header_t headers[] = {
    { "Accept" , "*/*"},
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

void updateData()
{
    float v[LEDS];
    morph(v);
    long now = Time.now();
    
    int flapOpen=getFlapStatus();
    
    char c[512];
    
    sprintf(c,"<data time=\"%02d:%02d\" atHome=\"%d\" schedule=\"%s\" weather=\"%s\" "
        "colors=\"(%02X %02X) (%02X %02X) (%02X %02X %02X %02X %02X %02X) (%02X %02X %02X %02X %02X %02X)\" "
        "light=\"%.3f\" lux=\"%.3f\" lightPower=\"%.2f\" temp=\"%.2f\" humi=\"%.2f\" waterTemp=\"%.3f\" "
        "tempReadingAge=\"%ld\" waterReadingAge=\"%ld\" fanSpeed=\"%d\" flap=\"%d\" "
        "motion=\"%d\" pHVoltage=\"%0.4f\" pHVoltageFiltered=\"%0.4f\" pH=\"%0.3f\" />",
        Time.hour(),
        Time.minute(),
        isAtHome(),
        getCurrentScheduleString(),
        getWeatherMsg(),
        (int)roundf(v[15]), (int)roundf(v[14]),
        (int)roundf(v[13]), (int)roundf(v[12]),
        (int)roundf(v[11]), (int)roundf(v[10]), (int)roundf(v[9]), (int)roundf(v[8]), (int)roundf(v[7]), (int)roundf(v[6]),
        (int)roundf(v[5]), (int)roundf(v[4]), (int)roundf(v[3]), (int)roundf(v[2]), (int)roundf(v[1]), (int)roundf(v[0]),
        getLightLevel(),
        getLightLux(),
        getTotalLEDPower(),
        getAirTemp(),
        getHumidity(),
        getWaterTemp(),
        now-getLastTempReading(),
        now-getLastWaterReading(),
        getFan(),
        flapOpen,
        getMotionStatus(),
        getPHVoltage(),
        getPHVoltageFiltered(),
        getPH()
        );
    lastData=c;
}

/* external call to set color */
int setColour(String data)
{
    int commaIndex=data.indexOf(",");
    if (commaIndex>=0)
    {   int period = (int)strtol(data.substring(commaIndex+1), NULL, 10);
        float a[LEDS];
        parseColorString(data.substring(0,commaIndex),a);
        setTarget2(a,period);
    }

    return 0;
}

int resetToProgramme(String data)
{   resetWeather();
    resetToSchedule();
    return 0;
}

int setWeather(String data)
{   int commaIndex=data.indexOf(",");
    int type = (int)strtoll(data.substring(0,commaIndex), NULL, 10);
    int duration = (int)strtol(data.substring(commaIndex+1), NULL, 10);
    initiateWeather(type,duration*1000/MORPH_TIMER_MS);
}

int setTime(String data)
{   int commaIndex=data.indexOf(":");
    int hour = (int)strtoll(data.substring(0,commaIndex), NULL, 10);
    int min = (int)strtol(data.substring(commaIndex+1), NULL, 10);
    int now=hour*60+min;
    int i=getScheduleRowForTime(now,0);
    initiateSchedule(i,10000);
}

int setTimeOffset(String data)
{   minuteOffset = (int)strtoll(data, NULL, 10);
    int now=Time.hour()*60+Time.minute()+minuteOffset;
    int i=getScheduleRowForTime(now,0);
    initiateSchedule(i,10000);
}


void setup() {

    initializeLEDDriver(800);
    
    Particle.function("setColour", setColour);
    Particle.function("reset", resetToProgramme);
    Particle.function("setWeather", setWeather);
    Particle.function("setTime", setTime); //for demo purposes
    Particle.function("timeOffset", setTimeOffset);
    //Particle.function("setAtHome", setAtHome);
    lastData="";
    Particle.variable("data",lastData);
    
    parseSchedule();
    
    Time.zone(0);
    //Time.endDST();
    Time.beginDST();
    Particle.syncTime();
    
    initializeSensors();
    
    restoreExistingLEDState();
  
    morphTimer.start();
    minuteTimer.start();
    
    delay(1000);
    int now=Time.hour()*60+Time.minute();
    int i=getScheduleRowForTime(now,0);
    initiateSchedule(i,NATURAL_TRANSITION_PERIOD);
    updateData();
    
    request.hostname = "192.168.1.1";
    request.port = 8080;

}

int updateDataCounter=0;
int lastFlap=0;
int lastMotion=0;
void loop()
{   
    if (updateDataCounter==0)
    {   updateData();
        updateDataCounter=4;
    }
    else
    {   updateDataCounter--;
    }
    delay(500);
    int flap=getFlapStatus();
    if (flap && !lastFlap)
    {
        request.path = "/polestar/scripts/webhook/90697B320A017A92";
        request.body="urn:door:aquarium";
        http.post(request, response, headers);
    }
    lastFlap=flap;
    
    int motion=getMotionStatus();
    if (motion && !lastMotion)
    {
        request.path = "/polestar/scripts/webhook/90697B320A017A92";
        request.body="urn:motion:aquarium";
        http.post(request, response, headers);
    }
    lastMotion=motion;
    
}


void morphCallback()
{   adjustLights();
    processWeather();
}

void minuteCallback()
{
    int now=Time.hour()*60+Time.minute()+minuteOffset;
    if (now<60)
    {   minuteOffset=0;
    }
    
    int i=getScheduleRowForTime(now,1);
    if (i>=0)
    {   initiateSchedule(i,NATURAL_SLOW_TRANSITION_PERIOD);
    }
    
    float airTemp=getAirTemp();
    if (airTemp>28)
    {   setFan(255);
    }
    else if (airTemp<27)
    {   setFan(0);
    }
}