// This #include statement was automatically added by the Particle IDE.
#include "DS18B20/DS18B20.h"

// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_DHT/Adafruit_DHT.h"

// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_PCA9685.h"


/**
 * This example waves the brightness of a row of LEDs
 */
 
 #include <math.h>

const int MORPH_TIMER_MS=20;
const int LEDS=6;


volatile int pauseLED=0;
float target[LEDS];
float origin[LEDS];
float stepIndex=1;
float stepCount=1;


struct ScheduleRow
{
    int home;
    int time;
    String timeString;
    String color;
    int fade;
};
int scheduleCount;
ScheduleRow schedule[16];
int currentSchedule;

DHT dht(D2, DHT22);
DS18B20 ds18b20(D3);


String lastData;


Adafruit_PCA9685 ledDriver = Adafruit_PCA9685(0x40, true);  // Use the default address, but also turn on debugging
Timer morphTimer(MORPH_TIMER_MS, adjustLights);
Timer minuteTimer(1000*60, eachMinute);


double getDSTemp() {
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
      return celsius;
    }
}


static float lastGoodAirTemp=0.0;
static float lastGoodAirHumi=0.0;
static float lastGoodWaterTemp=0.0;
static long lastTempReading=0;
static long lastWaterReading=0;

/* called every minute to update status data */
void updateData()
{
    float v[LEDS];
    morph(v);
    char sched0[6]; schedule[currentSchedule].timeString.toCharArray(sched0,6);
    char sched1[6]; schedule[currentSchedule+1].timeString.toCharArray(sched1,6);
    
    //get light level
    int a0=analogRead(A0);
	float light=1.0f-((float)a0)/4095.0f;
    
    //get DHT22 stuff
	float temp = dht.getTempCelcius();
    float humi = dht.getHumidity();
    float dsTemp = getDSTemp();
    long now = Time.now();
    if (temp>0.1)
    {   lastGoodAirTemp=temp;
        lastGoodAirHumi=humi;
        lastTempReading=now;
    }
    if (dsTemp>0.1)
    {   lastGoodWaterTemp=dsTemp;
        lastWaterReading=now;
    }
    
    
    char c[256];
    sprintf(c,"<data time=\"%02d:%02d\" schedule=\"%s - %s\" white=\"%.1f\" violet=\"%.1f\" cyan=\"%.1f\" green=\"%.1f\" red=\"%.1f\" deepRed=\"%.1f\" light=\"%.3f\" temp=\"%.2f\" humi=\"%.2f\" waterTemp=\"%.2f\" tempReadingAge=\"%d\" waterReadingAge=\"%d\" />",
        Time.hour(),
        Time.minute(),
        sched0,
        sched1,
        v[4], v[1], v[3], v[0], v[5], v[2],
        light,
        lastGoodAirTemp,
        lastGoodAirHumi,
        lastGoodWaterTemp,
        now-lastTempReading,
        now-lastWaterReading
        );
    lastData=c;
}


/* called by timer every 20ms */
void adjustLights()
{
    if (!pauseLED)
    {   if (stepIndex<stepCount)
        {   stepIndex++;
            float v[LEDS];
            morph(v);
            setLEDs(v);
        }
        processWeather();
    }
}


void eachMinute() {
    int now=Time.hour()*60+Time.minute();
    int i=getScheduleRowForTime(now,1);
    if (i>=0)
    {   initiateSchedule(i,180000);
    }
    //updateData();
}





void parseSchedule() {
    
    String s=   "0,00:00,000000000000,1 \n"
                "0,07:00,000410002004,1 \n"
                "0,08:00,001008080804,1 \n"
                "0,10:00,001810202000,1 \n"
                "0,12:00,004020403800,1 \n"
                "0,15:00,106040607050,1 \n" // peak day
                "0,19:00,004020403800,1 \n"
                "0,20:00,103004200820,1 \n" //early sunset
                "0,20:20,200800200020,1 \n" // late sunset
                "0,20:30,000210004008,1 \n" // moonlight
                "0,21:00,000104010800,1 \n"
                "0,22:00,000000000000,1 \n"
                "0,23:59,000000000000,1 \n";
    
    
    int lineStart=0;
    for (int i=0; i<s.length(); i++)
    {   char c=s.charAt(i);
        if (c=='\n')
        {   String line=s.substring(lineStart,i);
            //Serial.println(line);
            parseLine(line);
            lineStart=i+1;
        }
    }
}

void parseLine(String line) {
    int i=0;
    String home=parseField(line,&i);
    String time=parseField(line,&i);
    String hour=time.substring(0,2);
    String min=time.substring(3,5);
    int minsSinceMidnight=strtol(hour, NULL, 10)*60+strtol(min, NULL, 10);
    String color=parseField(line,&i);
    String fade="300"; //parseField(line,&i);
    schedule[scheduleCount].home=(int)strtol(home,NULL,10);
    schedule[scheduleCount].timeString=time;
    schedule[scheduleCount].time=minsSinceMidnight;
    schedule[scheduleCount].color=color;
    schedule[scheduleCount].fade=strtol(fade, NULL, 10)*1000/MORPH_TIMER_MS;
    scheduleCount++;
}

String parseField(String line, int* i)
{
    int j=line.indexOf(",",*i);
    String result="";
    if (j>0)
    {   result=line.substring(*i,j).trim();
        *i=j+1;
    }
    else if (line.length()>*i)
    {   result=line.substring(*i);
        *i=line.length();
    }
    //Serial.println(result);
    return result;
}

int getScheduleRowForTime(int now, int strict)
{
    
    int found=-1;
    for (int i=0; i<scheduleCount-1; i++)
    {
        if (strict)
        {   if (schedule[i].time==now)
            {   found=i;
                break;
            }
        }
        else
        {   if (now>=schedule[i].time && now<=schedule[i+1].time)
            {   found=i;
            }
        }
    }
    return found;
}

void initiateSchedule(int i, int fadeMS)
{
    currentSchedule=i;
    long long color = strtoll(schedule[i].color, NULL, 16);
    setTarget(color,fadeMS);
    
}


/* external call to set color */
int setColour(String data)
{
    int period;
    long long number;
    int commaIndex=data.indexOf(",");
    if (commaIndex==-1)
    {   period=100;
        number = (int)strtol(data, NULL, 16);
    }
    else
    {   period = (int)strtol(data.substring(commaIndex+1), NULL, 10);
        number = strtoll(data.substring(0,commaIndex), NULL, 16);
    }

    setTarget(number,period);
    return 0;
}


int weatherType;
int weatherDurationRemaining=-1;

int cloudToggle;
int cloudCycleRemaining;

/* external call to initiate weather 
type: 0 none, 1 cloudy
*/
int setWeather(String data)
{
    
    int commaIndex=data.indexOf(",");
    int type = (int)strtoll(data.substring(0,commaIndex), NULL, 10);
    int duration = (int)strtol(data.substring(commaIndex+1), NULL, 10);

    char ch[80];
    sprintf(ch,"weather %d %d",type, duration);
    Serial.println(ch);

    
    weatherType=type;
    weatherDurationRemaining=duration*1000/MORPH_TIMER_MS;
    
    if (type==1 || type==2 || type==3 || type==4)
    {   cloudCycleRemaining=0;
        cloudToggle=0;
    }

    return 0;
}

void processWeather()
{
    if (weatherDurationRemaining>0)
    {   weatherDurationRemaining--;
    
        if (weatherType==1)
        {
            if (cloudCycleRemaining==0)
            {   cloudCycleRemaining = random(5, 30)*1000/MORPH_TIMER_MS;
                cloudToggle=!cloudToggle;
            
                int now=Time.hour()*60+Time.minute();
                int i=getScheduleRowForTime(now,0);
                long long color = strtoll(schedule[i].color, NULL, 16);
                if (cloudToggle)
                {   //half brightness
                    color=colorMultiply(color,0.25f);
                    
                }
                setTarget(color,500);
            
            
            }
            else
            {   cloudCycleRemaining--;
            }
        }
        else if (weatherType==2)
        {
            if (cloudCycleRemaining==0)
            {   cloudCycleRemaining = 5000/MORPH_TIMER_MS;

                int now=Time.hour()*60+Time.minute();
                int i=getScheduleRowForTime(now,0);
                long long color = strtoll(schedule[i].color, NULL, 16);
                float f=(256+(float)random(768))/1024.0f;
                color=colorMultiply(color,f);
                setTarget(color,5000);
            
            
            }
            else
            {   cloudCycleRemaining--;
            }
        }
        else if (weatherType==3)
        {
            if (cloudCycleRemaining==0)
            {   int t=250+random(500);
                
                cloudCycleRemaining = t/MORPH_TIMER_MS;

                int now=Time.hour()*60+Time.minute();
                int i=getScheduleRowForTime(now,0);
                long long color = strtoll(schedule[i].color, NULL, 16);
                float f=(512+(float)random(512))/1024.0f;
                color=colorMultiply(color,f);
                setTarget(color,t);
            
            
            }
            else
            {   cloudCycleRemaining--;
            }
        }
        else if (weatherType==4)
        {
            if (cloudCycleRemaining==0)
            {   int t=250+random(5000);
                
                cloudCycleRemaining = t/MORPH_TIMER_MS;

                int now=Time.hour()*60+Time.minute();
                int i=getScheduleRowForTime(now,0);
                long long color = strtoll(schedule[i].color, NULL, 16);
                float f=(512+(float)random(512))/1024.0f;
                color=colorMultiply(color,f);
                setTarget(color,t);
            
            
            }
            else
            {   cloudCycleRemaining--;
            }
        }
        
        
        
    }
    else if (weatherDurationRemaining==0)
    {   //restore
        Serial.println("End of weather");
        int now=Time.hour()*60+Time.minute();
        int i=getScheduleRowForTime(now,0);
        initiateSchedule(i,10000);
        weatherDurationRemaining--;
    }
    
}

long long colorMultiply(long long color, float f)
{
    int v[LEDS];
    for (int i=0; i<LEDS; i++)
    {   v[i]=color & 0xFF;
        color>>=8;
    }
    
    for (int i=0; i<LEDS; i++)
    {   v[i]= (int)( ((float)v[i])*f );
    }
    
    long long result=0;
    for (int i=0; i<LEDS; i++)
    {   result<<=8;
        result+=v[LEDS-1-i];
        
    }
    return result;
}


/* update LEDs right now to the array of brighnesses */
void setLEDs(float v[])
{
    int offset=0;
    for (int i=0; i<LEDS; i++) 
    {
        int dutyCycle=(int)(v[i]*(4095/256));
        //ledDriver.setVal(i, dutyCycle);
        setLED(i,dutyCycle,offset);
        offset+=682; // about 1/6 of cycle
    }
}

/* called before reset to turn lights off */
void reset_handler()
{
    int N=20;
    float v[LEDS];
    setTarget(0,N);
    for (int i=0; i<N; i++)
    {   morph(v);
        setLEDs(v);
        delay(20);
    }
    //setTarget(0,1000/MORPH_TIMER_MS);
    //delay(1000);
    //setLEDs(target);
}

/* set a single LED brighness to val. mid is middle of pulse in PWM */
void setLED(int ledNum, int val, int mid)
{
    if (val == 4095) {
      // Special value for signal fully on.
      ledDriver.setPWM(ledNum, 4096, 0);
    }
    else if (val == 0) {
      // Special value for signal fully off.
      ledDriver.setPWM(ledNum, 0, 4096);
    }
    else {
      int h1=val/2;
      int h2=val-h1;
      int s=mid-h1;
      int e=mid+h2;
      if (s<0)
      { e-=s;
        s=0;
      }
      else if (e>4095)
      { int d=e-4095;
        s-=d;
        e=4095;
      }
      ledDriver.setPWM(ledNum, s, e);
    }
}

void colorToArray(long long number, float v[])
{
    v[5]=(number>>40)&0xFF;
    v[4]=(number>>32)&0xFF;
    v[3]=(number>>24)&0xFF;
    v[2]=(number>>16)&0xFF;
    v[1]=(number>>8)&0xFF;
    v[0]=(number)&0xFF;
}


/* sets the new target brightness of LEDs */
void setTarget(long long number, int fadeMS)
{
    float v[LEDS];
    colorToArray(number,v);
    
    //char ch[80];
    //sprintf(ch,"target f=%f e=%f d=%f c=%f b=%f a=%f fadeDuration=%d",v[5],v[4],v[3],v[2],v[1],v[0],period);
    //Serial.println(ch);
    
    pauseLED=1;
    
    morph(origin);

    for (int i=0; i<LEDS; i++) 
    {   target[i]=v[i];
    }

    stepIndex=0;
    stepCount=fadeMS/MORPH_TIMER_MS;
    pauseLED=0;
}

/* function to smooth transitions at each end */
float smootherstep(float x)
{   return x*x*x*(x*(x*6 - 15) + 10);
}

/* smooth iteration between origin and target on each step */
void morph(float v[])
{
    float ratio=stepIndex/stepCount;
    if (ratio>1) ratio=1;
    if (ratio<0) ratio=0;
    ratio=smootherstep(ratio);
    for (int i=0; i<LEDS; i++)
    {
        float gamma=0.5;
        float o=origin[i];
        float origin=pow(o,gamma);
        float t=target[i];
        float target=pow(t,gamma);
        float lg=origin+(target-origin)*ratio;
        v[i]=(float)pow(lg,1.0/gamma);
    }
}

void setup()
{
    ledDriver.begin();    // This calls Wire.begin()
    ledDriver.setPWMFreq(1000);     // Maximum PWM frequency is 1600
    Particle.function("setColour", setColour);
    Particle.function("setWeather", setWeather);
    lastData="";
    Particle.variable("data",lastData);
    
    Serial.begin(9600);
    Serial.println("Initialising");
    parseSchedule();
    
    Time.zone(0);
    Particle.syncTime();
    
    dht.begin();
    pinMode(D3, INPUT_PULLUP);
    ds18b20.setResolution(12);
  
    reset_handler();
    System.on(reset, reset_handler);  
    morphTimer.start();
    minuteTimer.start();
    
    delay(1000);
    int now=Time.hour()*60+Time.minute();
    int i=getScheduleRowForTime(now,0);
    initiateSchedule(i,10000);
    updateData();
}

void loop() {
    updateData();
    delay(10000);
}

