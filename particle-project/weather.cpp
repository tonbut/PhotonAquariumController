#include "weather.h"

WeatherPattern* currentWeather=NULL;

//***********************************
// Fast changing light dark
//***********************************
class NullWeather : public WeatherPattern
{   private:
        int mRemainingDuration;
    public:
    NullWeather(int aDuration)
    {   mRemainingDuration=aDuration;
    }
    void process()
    {   if (mRemainingDuration>0)
        {   mRemainingDuration--;
        }
    }
    int isFinished()
    {   return mRemainingDuration<=0;
    }
    
    void setWeatherMessage(char* msg)
    {   sprintf(msg,"null weather remaining=%d",+mRemainingDuration);
    }
};




//***********************************
// Flat overcast with darker bluish light fade in and out
//***********************************
class OvercastWeather : public WeatherPattern
{
    private:
        int mRemainingDuration;
        int mTimeSinceStart;
        int mFade;
        
    public:
    OvercastWeather(int aDuration, int aFade)
    {   mRemainingDuration=aDuration;
        mFade=aFade;
        mTimeSinceStart=0;
    }
    
    void process()
    {   
        //static const float overcastFilter[] = { 0.5, 0.5, 0.6, 0.9, 0.4, 0.7,  0.5, 0.5, 0.6, 0.9, 0.4, 0.7, 0,0,0,0  };
        static const float overcastFilter[] = { 0.2, 0.3, 0.4, 0.7, 0.4, 0.4,  0.7, 0.5, 0.4, 0.6,0.6,0.6,0.6, 0,0,0  };
        
        if (mRemainingDuration>0)
        {   mRemainingDuration--;
            mTimeSinceStart++;
            if ((mRemainingDuration%5)==0)
            {
                float intensity;
                if (mTimeSinceStart<mFade)
                {   intensity=((float)mTimeSinceStart)/((float)mFade);
                }
                else if (mRemainingDuration<mFade)
                {   intensity=((float)mRemainingDuration)/((float)mFade);
                }
                else
                {   intensity=1.0f;
                }
            
                float a[LEDS];
                getCurrentScheduleLighting(a);
                float multiplier=1.0f-intensity;
                colorMultiplyVector(a,overcastFilter,multiplier);
                setTarget2(a,100);
            }
            
        }
    }
    
    int isFinished()
    {   return mRemainingDuration<=0;
    }
    
    void setWeatherMessage(char* msg)
    {   sprintf(msg,"overcast %d of %d",mTimeSinceStart,(mTimeSinceStart+mRemainingDuration));
    }
};


//***********************************
// Fast changing light dark
//***********************************

class SCSection
{   private:
        int mRemainingDuration;
        float mBrightness;
    public:
    SCSection()
    {   mRemainingDuration=0;
    }
    int process()
    {
        if (mRemainingDuration==0)
        {
            mRemainingDuration=25+random(50);
            mBrightness=(512+(float)random(1024))/1024.0f;
            return true;
        }
        else
        {   mRemainingDuration--;
            return false;
        }
    }
    float getBrightness()
    {
        return mBrightness;
    }
    int getDuration()
    {   return mRemainingDuration;
    }
};


class ScatteredClouds2 : public WeatherPattern
{   private:
        SCSection mSections[8];
        int mRemainingDuration;
        float mScheduledLighting[LEDS];
        float mCurrentFade[LEDS];
    public:
    ScatteredClouds2(int aDuration)
    {   mRemainingDuration=aDuration;
        getCurrentScheduleLighting(mScheduledLighting);
        for (int i=0; i<LEDS; i++)
        {   mCurrentFade[i]=mScheduledLighting[i];
        }
    }
    void process()
    {   if (mRemainingDuration>0)
        {   mRemainingDuration--;
            int flag=0;
            for (int i=0; i<8; i++)
            {
                if (mSections[i].process())
                {   flag=1;
                    
                    int a=3,b=6;
                    if (i==1)
                    {   a=6; b=9;
                    }
                    else if (i==2)
                    {   a=9; b=10;
                    }
                    else if (i==3)
                    {   a=10; b=11;
                    }
                    else if (i==4)
                    {   a=11; b=12;
                    }
                    else if (i==5)
                    {   a=12; b=13;
                    }
                    else if (i==6)
                    {   a=13; b=14;
                    }
                    else if (i==7)
                    {   a=15; b=16;
                    }
                    
                    for (int i=a; i<b; i++) mCurrentFade[i]=mScheduledLighting[i];
                    colorMultiplyRange(mCurrentFade,mSections[i].getBrightness(),a,b);
                    
                    
                }
            }
            if (flag)
            {
                setTarget2(mCurrentFade,750);
            }
        }
    }
    int isFinished()
    {   return mRemainingDuration<=0;
    }
    
    void setWeatherMessage(char* msg)
    {   sprintf(msg,"scattered clouds 2 remaining=%d",+mRemainingDuration);
    }
};



class ScatteredClouds : public WeatherPattern
{   private:
        int mRemainingDuration;
        int cloudCycleRemaining;
    public:
    ScatteredClouds(int aDuration)
    {   mRemainingDuration=aDuration;
        cloudCycleRemaining=0;
    }
    void process()
    {   if (mRemainingDuration>0)
        {   mRemainingDuration--;
        
            if (cloudCycleRemaining==0)
            {   int t=250+random(5000);
                
                cloudCycleRemaining = t/MORPH_TIMER_MS;
                
                float f1=(512+(float)random(512))/1024.0f;
                float f2=(512+(float)random(512))/1024.0f;
                float f3=(512+(float)random(512))/1024.0f;
                float a[LEDS];
                getCurrentScheduleLighting(a);
                colorMultiplyRange(a,f1,0,6);
                colorMultiplyRange(a,f2,6,12);
                colorMultiplyRange(a,f3,12,16);
                setTarget2(a,t);
            
            }
            else
            {   cloudCycleRemaining--;
            }
        }
    }
    int isFinished()
    {   return mRemainingDuration<=0;
    }
    
    void setWeatherMessage(char* msg)
    {   sprintf(msg,"scattered clouds remaining=%d",+mRemainingDuration);
    }
};

//***********************************
// Lightning storm
//***********************************
class StormWeather : public WeatherPattern
{   private:
        int mRemainingDuration;
        int mTimeSinceStart;
        int mFade;
        
        int delayMedian=delayMax; //current avg time between flash sequences
        int delayRemaining=0; //number of cycles to next flash sequence
        int innerDelayRemaining=-1; // number of cycles to next flash within sequence
        int preFlashRemaining=0; // position in flash sequence
        float brightness=0.0f; // current instantaneous flash brightness
        float dayLightIntensity=1.0f;
        
        int delayMin=4;
        int delayMax=22;
        int preFlashMin=3;
        int preFlashMax=8;
        int preFlashPeriodMin=40/MORPH_TIMER_MS;
        int preFlashPeriodMax=200/MORPH_TIMER_MS;
        float fadeFactor=0.5f;

    public:
    StormWeather(int aDuration, int aFade)
    {   mRemainingDuration=aDuration;
        mFade=aFade;
        mTimeSinceStart=0;
        delayMedian=(delayMax+delayMin)/2;
        delayRemaining=0;
        innerDelayRemaining=-1;
        preFlashRemaining=0;
        brightness=0.0f;
        dayLightIntensity=1.0f;
        
    }
    void process()
    {   if (mRemainingDuration>0)
        {   mRemainingDuration--;
            mTimeSinceStart++;
            processStorm();
        }
    }
    void processStorm()
    {
        if (delayRemaining>0)
        {   delayRemaining--;
        }
        else
        {   
            float stormIntensity;
            if (mTimeSinceStart<mFade)
            {   stormIntensity=((float)mTimeSinceStart)/((float)mFade);
            }
            else if (mRemainingDuration<mFade)
            {   stormIntensity=((float)mRemainingDuration)/((float)mFade);
                
                if (stormIntensity<0.5f) stormIntensity=0.0f;
                else stormIntensity=(stormIntensity-0.5f)*2.0f;
            }
            else
            {   stormIntensity=1.0f;
            }
            dayLightIntensity=stormIntensity;
            

            if (preFlashRemaining==0 && innerDelayRemaining<0)
            {   preFlashRemaining=random(preFlashMin,preFlashMax);
                innerDelayRemaining=0;
            }
            
            if (innerDelayRemaining==0 && preFlashRemaining>0)
            {   //initiate new flash
            
                brightness=(preFlashRemaining==1)?stormIntensity:stormIntensity*0.2f;
                preFlashRemaining--;
                innerDelayRemaining=random(preFlashPeriodMin,preFlashPeriodMax);
            }
            if (innerDelayRemaining>0)
            {   
                innerDelayRemaining--;
            }
            if (preFlashRemaining==0 && innerDelayRemaining==0)
            {   
                int dmin=delayMedian/2;
                int dmax=delayMedian*2;
                int d=random(dmin,dmax);
                delayRemaining=d*d;
                int dmm=delayMin+(int)(((float)delayMin)*(1.0f-stormIntensity));
                delayMedian=(delayMedian*2+random(dmm,delayMax))/3;
                innerDelayRemaining=-1;
            }
        }
        
        if (brightness>0.01f)
        {
            setColor(brightness,dayLightIntensity);
            brightness=brightness*fadeFactor;
        }
        else if (brightness>0.0f)
        {   
            brightness=0.0f;
            setColor(brightness,dayLightIntensity);
        }
    }
    void setColor(float lightningBrightness,float daylightFade)
    {
        if ((mRemainingDuration%5)==0)
        {
            static const float stormLightFilter[] = { 0.1, 0.1, 0.3, 0.9, 0.7, 0.3,  0.9, 0.7, 0.3,  0.3, 1.0, 1.0, 0.3, 0,0,0,0  };
            //static const float stormLightFilter[] = { 0.1, 0.1, 0.3, 1.0, 0.3, 0.2,  0.1, 0.1, 0.3, 1.0, 0.3, 0.2, 0,0,0,0  };

            float a[LEDS];
            getCurrentScheduleLighting(a);
            float multiplier=1.0f-daylightFade;
            colorMultiplyVector(a,stormLightFilter,multiplier);
            setTarget2(a,1000);
        }
        float brightness=lightningBrightness*128.0f;
        setLEDOffset(12,brightness);
        setLEDOffset(11,brightness);
        setLEDOffset(10,brightness);
        setLEDOffset(9,brightness);
        setLEDOffset(5,brightness);
        setLEDOffset(8,brightness);
        
        
    }
    
    
    int isFinished()
    {   return mRemainingDuration<=0;
    }
    
    void setWeatherMessage(char* msg)
    {   sprintf(msg,"storm %d of %d",mTimeSinceStart,(mTimeSinceStart+mRemainingDuration));
    }
};





void resetWeather()
{
    if (currentWeather!=NULL)
    {   delete currentWeather;
        currentWeather=NULL;
        resetToSchedule();
    }
    
}


void initiateWeather(int aType, int aDuration)
{
    if (currentWeather!=NULL)
    {   delete currentWeather;
        currentWeather=NULL;
    }
    if (aType==0)
    {   int fade=aDuration/4;
        if (fade>60*MORPH_TIMER_MS) fade=60*MORPH_TIMER_MS;
        currentWeather=new OvercastWeather(aDuration, fade);
    }
    if (aType==1)
    {   currentWeather=new ScatteredClouds2(aDuration);
    }
    if (aType==2)
    {   int fade=aDuration/4;
        if (fade>120*MORPH_TIMER_MS) fade=120*MORPH_TIMER_MS;
        currentWeather=new StormWeather(aDuration, fade);
    }
}

void processWeather()
{
    if (currentWeather!=NULL)
    {   currentWeather->process();
        if (currentWeather->isFinished())
        {   delete currentWeather;
            currentWeather=NULL;
            resetToSchedule();
        }
    }
}

char* getWeatherMsg()
{
    static char b[128];
    WeatherPattern* cw=currentWeather;
    if (cw!=NULL)
    {   cw->setWeatherMessage(b);
    }
    else
    {   sprintf(b,"none");
    }
    return b;
}