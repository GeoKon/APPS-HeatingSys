/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
// Minimum necessary for this file to compile

#include <externIO.h>               // IO includes and cpu...exe extern declarations
#include <mgnClass.h>

#include "myGlobals.h"
#include "mySupport.h"
#include "myEngClasses.h"           // this also includes pz16Class.h
#include "myCliHandlers.h"

static MGN mgn;                     // temp usage of Meguno structure

// ----------------------- 1. CLI DIRECT PZM Access -----------------------------------

void requireOFF( BUF *bp )                           // helper function
{
    RESPOND( bp, "Direct access to PZEM requires 'scanON' 0\r\n");
}

// ----------------------- 2. CLI SIMULATION -----------------------------------
// --------------------- 3. Global volatile VARIABLES -------------------------

static void cliSetTime( int n, char *arg[])
{
    BINIT( bp, arg );
    int tmin, thrs, tsec;
    thrs = tmin = tsec = 0;
    
    if( n>1 ) thrs = atoi( arg[1] );
    if( n>2 ) tmin = atoi( arg[2] );
    if( n>3 ) tsec = atoi( arg[3] );
    
    tmb.setHMS( thrs, tmin, tsec );
//    ticMin.setSecCount( tmb.getSecCount() );
//    ticHr.setSecCount( tmb.getSecCount() );
    RESPOND( bp, "Time is: %s", tmb.getTime() ); 
}
static void cliPrintTime( int n, char *arg[])
{
    BINIT( bp, arg );
    RESPOND( bp, "Time is: %s", tmb.getTime() ); 
}
// --------------------- 4. Measurements & Actions -------------------------

static void cliReport( int n, char *arg[] )
{
    reportINP( exe.cmd1stUpper() ? STREAM_ALL : STREAM_CONSOLE );       
}
static void cliResetT( int n, char *arg[])
{
    BINIT( bp, arg );
    int i = (n>=1) ? atoi( arg[1] ) : 0;
    
    idur.resetT( i );
    if( exe.cmd1stUpper() )
        mgn.controlVarArgs( i ? "T1dur" : "T0dur", "T%d RESET", i);        
    else
        RESPOND( bp, "T%d RESET\r\n", i );
}

static void cliShowState( int n, char *arg[])
{
    BINIT( bp, arg );
    refreshState( exe.cmd1stUpper() ? STREAM_MEGUNO : STREAM_CONSOLE, bp ); // in mySupport.cpp
}
static void cliThisHelp( int n, char *arg[])
{
    BINIT( bp, arg );
    CMDTABLE *row = mypTable;
    for( int j=0; row->cmd ; j++, row++ )
    {
        RESPOND( bp,  "\t%s\t%s\r\n", row->cmd, row->help );
    }
}
// ----------------------- 5. SetEE IMPLEMENTATION -----------------------------------

void cliSetMyParm( int n, char **arg )
{
    BINIT( bp, arg );
    if( exe.missingArgs( 2 ) )
        return;
    if( exe.cmd1stUpper() )
        setAnyMegParm( arg[1], arg[2], bp );
    else
        setAnyEEParm ( arg[1], arg[2], bp );
}  

// ----------------------- 6. SIMULATIONS  -----------------------------------

static void cliSimulate( int n, char **arg )
{
    BINIT( bp, arg );
//    if( exe.cmd1stUpper() )
//        setAnyMegParm( arg[1], arg[2], bp );
//    else
//        setAnyEEParm ( arg[1], arg[2], bp );
    int v = (n>=1) ? atoi(arg[1]) : 0;
    idur.simulate( (bool) v );
    RESPOND( bp, "Simulation %s\r\n", v ?"ENABLED":"DISABLED" );
}  
static void cliSetSimul( int n, char **arg )
{
    BINIT( bp, arg );
//    if( exe.cmd1stUpper() )
//        setAnyMegParm( arg[1], arg[2], bp );
//    else
//        setAnyEEParm ( arg[1], arg[2], bp );
    int v = (n>=1) ? atoi(arg[1]) : 0;
    idur.setSimul( (bool) v );
    RESPOND( bp, "Simulated %s\r\n", v ?"ON":"OFF" );
}  

static void cliSetTankGallons( int n, char **arg )
{
    BINIT( bp, arg );
    if( exe.missingArgs(1) )
        return;
    int gallons = atoi( arg[1] );
    setTankGallons( gallons );
    RESPOND( bp, "TankCal, TankEst set to %d. HrsOper zeroed\r\n", gallons );
}  
static void cliComputeGPH( int n, char **arg )
{
    BINIT( bp, arg );
    if( exe.missingArgs(1) )
        return;
    int tankLevel = atoi( arg[1] );
    float newcoef = computeGPMCoef( tankLevel );
    RESPOND( bp, "GPH=%.3f (Duration %.3fhrs, delta oil in tank %.1f\r\n",
            newcoef, myp.gp.hrsOper, myp.gp.tankCal-(float)tankLevel );
} 
// ----------------- CLI command table ----------------------------------------------

/* To program a new PZEM-016, use "flash addr". Test using "v addr".
 */
    CMDTABLE mypTable[]= 
    {
        {"t",         "This help",                                  cliThisHelp },    

        {"setTime", "hrs min sec. Set current time",                cliSetTime},
        {"time",    "prints current time",                          cliPrintTime},
        
// 4. High Level Measurements/Actions

        {"report", "[or Report]. Display as in main loop",          cliReport},
        {"reset",   "[or Reset] 0|1. Zero T0 or T1 timer",          cliResetT},

// 5. Sets a user EE Parameter
        {"ShowAll", "Clears and redraws Meguno Parm Table",   [](int, char**){showMegAllParms();} },
        {"state",   "[or State]. Show state",                       cliShowState },
        {"setEE",   "[or SetEE] parm value. Sets any parameter",    cliSetMyParm},

        {"simul",   "0|1 Activate/deactivate simulation",           cliSimulate },
        {"setSim",  "0|1 Set simulated value 1 of 0",               cliSetSimul },

        {"setTank", "gallons. Set initial oil level",               cliSetTankGallons },
        {"compGPH", "gallons. Compute & print GPH based.",          cliComputeGPH },
        
        {NULL, NULL, NULL}
    };
 #undef RESPONSE
 
