#include "lightingLib.h"
#include "RGBDriver.h"

// original logical channels on UI
//right 10,11,12,13,14,15
//left 4,5,6,7,8,9
//focus 2,3,0,1



//mapping from logical channel to physical channel
int ledMap[] = { 2,5,0, 13,12,9, 3,1,4, 11,8,10,14,  6,7,15 };

//watts output max for each logical channel 0-15
//int ledPowerMap[] = { 12,6,6, 6,9,9, 6,9,9, 6,6,6,6, 3,3,6 };
int ledPowerMap[] = { 12,6,6, 6,12,9, 6,6,9, 6,6,6,6, 3,3,6 };

volatile int pauseLED=0;
float target[LEDS];
float origin[LEDS];
float offset[LEDS];
float stepIndex=0;
float stepCount=1;
int atHome=true;

struct ScheduleRow
{
    int home;
    int time;
    int endTime;
    String timeString;
    String color;
    int fade;
};
int scheduleCount;
ScheduleRow schedule[20];
int currentSchedule;

Adafruit_PCA9685 ledDriver = Adafruit_PCA9685(0x40, true);  // Use the default address, but also turn on debugging

void initializeLEDDriver(int aFreq)
{
    ledDriver.begin();    // This calls Wire.begin()
    ledDriver.setPWMFreq(aFreq); // Maximum PWM frequency is 1600
}




void colorMultiply(float v[], float f)
{   for (int i=0; i<LEDS; i++) 
    {   v[i]*=f;
    }
}

/* when m==1 v is unchanged, when m==0, v is multipled by f */
void colorMultiplyVector(float v[], const float f[], float m)
{   for (int i=0; i<LEDS; i++) 
    {   v[i]*=(m + f[i]*(1-m));
    }
}


void colorMultiplyRange(float v[], float f, int first, int last)
{   for (int i=first; i<last; i++)
    {   v[i]*=f;
    }
}

void copyColourArray(float source[], float target[])
{   for (int i=0; i<LEDS; i++) 
    {   target[i]=source[i];
    }
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

/* set a single LED brighness to val. mid is middle of pulse in PWM */
void setLED(int ledNum, int val, int mid)
{
    if (val >= 4095) {
      // Special value for signal fully on.
      ledDriver.setPWM(ledNum, 4096, 0);
    }
    else if (val <= 0) {
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

/* update LEDs right now to the array of brighnesses */
void setLEDs(float v[])
{
    int offset=0;
    int div=4096/LEDS;
    float x=4095.0f/255.0f;
    for (int i=0; i<LEDS; i++) 
    {
        int dutyCycle=(int)roundf(v[i]*x);
        setLED(ledMap[i],dutyCycle,offset);
        offset+=div; 
    }
}




/* called by timer every 20ms */
void adjustLights()
{
    if (!pauseLED)
    {   if (stepIndex<stepCount)
        {   stepIndex++;
            float v[LEDS];
            morph(v);
            for (int i=0; i<LEDS; i++)
            {   v[i]+=offset[i];
            }
            setLEDs(v);
        }
    }
}


void parseColorString(const char* str, float result[])
{   char c[3]; c[2]=0;
    int len=strlen(str);
    int n=0;
    for (int i=len-1; i>0; i-=2)
    {   
        while(str[i]==' ' && i>0) i--;
        if (i>0)
        {
            c[0]=str[i-1];
            c[1]=str[i];
            long color = strtol(c, NULL, 16);
            result[n++]=(float)color;
            if (n>=LEDS) break;
        }
    }
    for (int i=n; i<LEDS; i++)
    {   result[i]=0.0f;
    }
}

String parseField(String line, int* i)
{
    int j=line.indexOf(",",*i);
    String result="";
    if (j>0)
    {   result=line.substring(*i,j).trim();
        *i=j+1;
    }
    else if (((int)line.length())>*i)
    {   result=line.substring(*i);
        *i=line.length();
    }
    //Serial.println(result);
    return result;
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

void parseSchedule() {
    
    String s=   "1,00:00,0,1 \n"
    
                //"1,14:00,D0C0D0 00000000 A0C0C0 A0C0C0 000040,1 \n" // super bright
    
                "1,07:30,040002 00000000 020403 020403 000001,1 \n"
                "1,08:00,000000 300D1525 040E0F 030E0F 000002,1 \n"
                "1,10:00,4A0020 400D1540 2C2210 2C2210 000020,1 \n"
                "1,12:00,9D0035 390A163A 614420 57441C 000020,1 \n"
                "1,13:00,C00073 60303060 B68054 B6B154 000040,1 \n" // peak day - better colors
                "1,14:00,D000C0 FF4040B8 B09080 B0B080 000080,1 \n" // super bright
                "1,14:30,000000 05000005 000093 000000 500000,1 \n" // siesta
                //"1,14:30,000000 0C00000C 040000 040000 180000,1 \n" // siesta
                "1,15:45,D000C0 FF3F3FB4 A8C0D1 A8DFC8 0001DF,1 \n" //extra super bright
                //"1,15:30,D000C0 FF4040B8 B0B080 B0B080 000080,1 \n" // super bright
                "1,18:00,C00073 60303060 A0A054 A0B154 000040,1 \n" // peak day - better colors
                "1,19:30,B10026 30000040 194F20 194F20 00001B,1 \n"
                "1,20:30,35405F 08000000 1B080B 1B080B 0A0131,1 \n" // early sunset
                //"1,20:20,03005D 08000000 1A070A 1A070A 090130,1 \n" // late sunset
                "1,20:50,34615B 17000000 190009 190009 3A460C,1 \n" // late sunset
                "1,21:10,14400E 08000000 060805 060805 10060D,1 \n" // midway between sunset and dusk
                "1,21:30,04001D 04000004 020413 010424 0F0505,1 \n" // dusk2
                "1,22:00,000000 00030300 010B0A 010D0A 000001,1 \n" // bright moonlight
                "1,22:15,000000 00030300 010804 010804 000001,1 \n" // moonlight
                "1,22:30,000000 00020300 000000 010603 000000,1 \n"
                "1,22:45,000000 00000000 000000 000000 000000,1 \n"    
    
    
    
    
    /*
                "1,07:30,0400 0200 020403000001 020403000001,1 \n"
                "1,08:00,1200 1008 100810040006 100810040006,1 \n"
                //"1,10:00,2D00 2000 184810000020 184810000020,1 \n"
                "1,10:00,4A26 2000 2E2210000020 2C2210000020,1 \n"
                
                "1,12:00,A070 4000 60A020000060 60A020000060,1 \n"
                //"1,15:00,E800 815F FFFFD30000FF FFFFDF0000FF,1 \n" // peak day 
                //"1,15:00,6F0A 0009 7EA9A0000CA9 81F2BC0000CA,1 \n" // peak day - less bright
                "1,13:00,C07C 736F A08B40000080 B6B154000080,1 \n" // peak day - better colors
                
                //"1,14:00,E0D0 D0E0 C0FFFF000080 C0FFFF000080,1 \n" // super bright
                //on holiday reduce lighting
                "1,14:00,D0C0 C0D0 A0C0C0000040 A0C0C0000040,1 \n" // super bright
                
                
                "1,17:00,C07C 736F A0B140000060 A0B154000060,1 \n" // peak day - better colors
                
                //"1,19:00,6B50 6043 605020000040 605020000040,1 \n"
                "1,19:00,B15A 2672 0F4F2000001E 194F2000001B,1 \n"
                
                "1,20:00,3500 5F7A 16081D0F0335 1B080B0A0131,1 \n" // early sunset
        
                "1,20:20,2030 0020 0C01032A2426 0D0101282225,1 \n" // late sunset
                "1,20:40,1400 0E2A 06080711070D 06080510060D,1 \n" // midway between sunset and dusk
                "1,21:00,0F02 0519 01080F070000 010419050000,1 \n" // dusk2
                //"1,20:40,1000 0E1B 012010080000 012010080000,1 \n" // dusk
                "1,21:30,0600 0007 010804000001 010804000001,1 \n" // moonlight
                "1,22:00,0000 0003 000002000000 000002000000,1 \n"
                "1,22:15,0000 0000 000000000000 000000000000,1 \n"
                //"1,22:30,00,1 \n"
    */
                "1,23:59,00,1 \n"
                
                //"0,12:00,2700 0000 3FE94B0000F4 17FF480000FF,1 \n" // early afternoon AWAY
                //"0,15:00,2700 0000 3FE94B0000F4 17FF480000FF,1 \n" // peak day AWAY
                
                ;
    
    scheduleCount=0;
    int lineStart=0;
    for (int i=0; i<((int)s.length()); i++)
    {   char c=s.charAt(i);
        if (c=='\n')
        {   String line=s.substring(lineStart,i);
            //Serial.println(line);
            parseLine(line);
            lineStart=i+1;
        }
    }
}

// if strict then only return a schedule when a schedule starts in this exact minute
int getScheduleRowForTime(int now, int strict)
{
    
    int found=-1;
    
    //find schedule for home/away status
    for (int i=0; i<scheduleCount-1; i++)
    {
        if (strict)
        {   if (schedule[i].time==now && schedule[i].home==atHome)
            {   found=i;
                break;
            }
        }
        else
        {   if (now>=schedule[i].time && now<=schedule[i+1].time && schedule[i].home==atHome)
            {   found=i;
                break;
            }
        }
    }
    
    
    if (found==-1)
    {   //look for any schedule regardless of home/away status
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
                    break;
                }
            }
        }
    }
    
    
    return found;
}

/* sets the new target brightness of LEDs */
void setTarget2(float color[], int fadeMS)
{
    pauseLED=1;
    
    morph(origin); //seed origin with current colour
    copyColourArray(color,target); //seed target with passed colour

    stepIndex=0;
    stepCount=fadeMS/MORPH_TIMER_MS;
    pauseLED=0;
}

void initiateSchedule(int i, int fadeMS)
{   currentSchedule=i;
    float a[LEDS];
    parseColorString(schedule[i].color,a);
    setTarget2(a,fadeMS);
}




/* read current LED state from hardware and initialise in memory state */
void restoreExistingLEDState()
{

    //on off
    //0 4096 = fully off
    //4096 0 = fully on
    float x=255.0f/4095.0f;
    for (int i=0; i<LEDS; i++) 
    {
        int on=ledDriver.readPWMOn(ledMap[i]);
        int off=ledDriver.readPWMOff(ledMap[i]);
        float v;
        if (on==4096 && off==0) v=255.0f;
        else if (on==0 && off==4096) v=0.0f;
        else
        {   v=((float)(off-on))*x;
        }
        origin[i]=v;
    }
    
    for (int i=0; i<LEDS; i++)
    {   target[i]=origin[i];
        offset[i]=0.0f;
    }
}

//calculate led power usage
float getTotalLEDPower()
{   
    float v[LEDS];
    morph(v);
    
    float totalPower=0;
    float divisor=1.0f/256.0f;
    for (int i=0; i<LEDS; i++)
    {   totalPower += v[i]*(float)ledPowerMap[i]*divisor;
    }
    return totalPower;
}

char* getCurrentScheduleString()
{
    static char currentScheduleString[12];
    char sched0[6];
    schedule[currentSchedule].timeString.toCharArray(sched0,6);
    char sched1[6];
    schedule[currentSchedule+1].timeString.toCharArray(sched1,6);
    sprintf(currentScheduleString,"%s-%s",sched0,sched1);
    return currentScheduleString;
}

int isAtHome()
{   return atHome;
}

void getCurrentScheduleLighting(float result[])
{   int now=Time.hour()*60+Time.minute();
    int i=getScheduleRowForTime(now,0);
    parseColorString(schedule[i].color,result);
}

void resetToSchedule()
{
    int now=Time.hour()*60+Time.minute();
    int i=getScheduleRowForTime(now,0);
    initiateSchedule(i,NATURAL_TRANSITION_PERIOD);
}

void setLEDOffset(int aLED, float aValue)
{   offset[aLED]=aValue;
}



