#pragma once

#include <ESP8266WiFi.h>
#include "myGlobals.h"
#include "indurClass.h"             // this already includes OLED Class

// ----------------------------------- Timer Class -------------------------------------

class TMBASE
{
private:
    int _sec;           // 0,10,...50
    int _min;           // 0, 1, ..., 59
    int _hrs;           // 0, 1, ..., 23
    bool _dayWrap;      // set to true, every day 00:00:00
    bool _hrsWrap;      // set to true, every hour XX:00:00
        
public:
    TMBASE();
    void setHMS( int h=0, int m=0, int s=0 );     // sets time
    uint32 getSecCount();
    void updSeconds();   // hook in Ticker every second
    void updMinutes();   // hook in Ticker every second for x60 fast
    
    const char *getTime();
    bool dayWrap();     // returns true every day and resets the flag
    bool hrsWrap();     // returns true every hour and resets the flag
};
// ---------------------------------
class TOCsec
{
private:
public:
    uint32 _secmax;                     // upper limit before warps to 0 
    uint32 _seccnt;                     // counts 0...secmax-1
    bool _secrdy;                       // true if warp
public:
    TOCsec( uint32 period = 1);         // can be used to set timeout
    void update();                      // call this every "sec"    
    void setModulo( uint32 period );    // optional call 
    void setSecCount( uint32 sec );
    bool ready();                       // ready every SEC_COUNT
};

// ----------------------------------- External References ---------------------------------
    extern CPU cpu;
    extern OLED oled;
    extern INDUR idur;
    
//    extern PZENG   eng;    
//    extern VWSTATS shortT;      // short time averages
//    extern VWSTATS longT;       // long term averages    

    extern TOCsec ticMin;       // every hour
    extern TOCsec ticHr;        // every day
    extern TMBASE tmb;          // every second
