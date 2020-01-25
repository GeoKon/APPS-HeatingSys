/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#pragma once

/*
 * The Meguno "TState" is updated every time there is a state change.
 * 
 * scanON (and scanSec) are used to periodically sample the state of the heating system
 * Both are EEProm parameters. 
 * Use CLI "setEE scanON 0|1" or the Meguno button to control this. 
 * 
 * When scanON is true, the Meguno "TmLine" is updated periodically
 *
 */

// ----------------------------------- Structures -------------------------------------

typedef struct eng_t
{

} ENG;

#include "IGlobal.h"        // in GKE-Lw. Includes and externs of cpu,...,eep
#include <nmpClass.h>

// ----------------- Exported by this module ---------------------------------------

    extern NMP nmp;             // allocated in myGlobals.cpp; used only by this                             

    #define BUTTON       0      // INPUTS
    #define MYLED       16      // 2 for old MCU, 16 for new ones
    
    #define SWINP       12+NEGATIVE_LOGIC      // 12=D6, 2=D4 (not good for input), 14=D5 
    
                                // GPIO-4 (aka D2) is SDA used by OLED
                                // GPIO-5  (aka D3) is SCL used by OLED        
#define MAGIC_CODE 0x1556

// --------------- Definition of Global parameters ---------------------------------

    class Global: public IGlobal
    {
      public:												// ======= A1. Add here all volatile parameters 
        
        bool streamON;              // streaming enabled or disabled. Used by Thinger.io only

                                    // SET BY MAIN LOOP. USED BY THINGER ONLY
        uint32 reportHourSec;       // seconds heater is ON during past hour
        uint32 reportTodaySec;        // seconds heater is ON during today
        uint32 reportYesterSec;        // seconds heater is ON during past day
        float reportGalUsed;        // gallons used the past hour
        float reportTankLevel;      // estimated gallons left in the tank

        void initVolatile()                                 // ======= A2. Initialize here the volatile parameters
		{
            streamON    = false;
            reportHourSec = 0;     
            reportTodaySec = 0;    
            reportYesterSec = 0;     
            reportGalUsed = 0.0;        // gallons used the past hour
            reportTankLevel = 0.0;
		}    
		void printVolatile( char *prompt="",                // ======= A3. Add to buffer (or print) all volatile parms
		                    BUF *bp=NULL ){;}
		struct gp_t                                         // ======= B1. Add here all non-volatile parameters into a structure
		{                           
            int scanON;             // enables or disables scanning
            int scansec;            // n>0 is number of seconds per scan
            
            int display;            // 1=show console, 2=show Meguno every scan, 4=OLED
            int obright;			// oled brightness 
            int dbMinut;            // how often to save to dropbox

            float galperhr;         // coefficient: gallons consumed per hour
            float tankCal;          // tank indication when durtotal set to zero
            float hrsOper;          // counts hours of operation after the calibration point
            float tankEst;          // estimated gallons in the tank
 		} gp;
        
		void initMyEEParms()                                // ======= B2. Initialize here the non-volatile parameters
		{
            gp.scanON       = 1;
            gp.scansec      = 2;
            gp.display      = 1;
            gp.obright      = 1;           
            gp.dbMinut      = 0;                           // on or off 
            
            gp.galperhr     = 3.5;
            gp.tankCal      = 150.0;
            gp.tankEst      = 150.0;
            gp.hrsOper      = 0.0;            
		}		
        void registerMyEEParms()                           // ======= B3. Register parameters by name
        {
            nmp.resetRegistry();
            nmp.registerParm( "scanON",    'd', &gp.scanON,     "0=no scanning, 1=scanning ON" );
            nmp.registerParm( "scanSec",   'd', &gp.scansec,    "Number of seconds per scan" );
            nmp.registerParm( "display",   'd', &gp.display,    "Show to 1=console, 2=Meguno" );
            nmp.registerParm( "obright",   'd', &gp.obright,    "OLED brightness 1...255" );
          
            nmp.registerParm( "dbMinut",   'd', &gp.dbMinut,    "0|1 send to Drobox hourly stats" );
            
            nmp.registerParm( "GalPerHr",  'f', &gp.galperhr,   "Coef: Gallons per Hour" );
            nmp.registerParm( "TankCal",   'f', &gp.tankCal,    "Gallons indicator since RESET" );
            nmp.registerParm( "TankEst",   'f', &gp.tankEst,    "Estimated Gallons in Tank" );
            nmp.registerParm( "hrsOper",   'f', &gp.hrsOper,    "Number of hours operating" );

			PF("%d named parameters registed\r\n", nmp.getParmCount() );
			ASSERT( nmp.getSize() == sizeof( gp_t ) );                             
        }
        void printMyEEParms( char *prompt="",               // ======= B4. Add to buffer (or print) all volatile parms
                             BUF *bp=NULL ) 
		{
			nmp.printAllParms( prompt );
		}
        void initAllParms()
        {
            initTheseParms( MAGIC_CODE, (byte *)&gp, sizeof( gp ) );
        }
	};
    
    extern Global myp;                                      // exported class by this module
