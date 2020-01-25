#pragma once
#include <ticClass.h>
#include "myGlobals.h"          // needed for VAP definition (only)
#include "indurClass.h"

#define STREAM_CONSOLE 1
#define STREAM_MEGUNO  2
#define STREAM_ALL     3
#define STREAM_NONE    0

    extern TICsec secTic; 
    
    void reportINP( int mask );     // reports streaming values
    
    bool setAnyMegParm( char *parm, char *value, BUF *bp );
    bool setAnyEEParm ( char *parm, char *value, BUF *bp );
    
    char *getAllState( BUF *s );
    void showMegAllParms();    
    
    void refreshState( int mask, BUF *bp );
    char *minsec( uint32 t );

    void updateTankGallons( uint32 durSec );
    void setTankGallons( int gallons );
    float computeGPMCoef( int tankLevel );
    const char *computeDaysToGo();
