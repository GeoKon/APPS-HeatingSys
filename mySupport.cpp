/*
 * Used by:
 *      PwrMonitor      -- to report to console when streaming is ON
 *      myCliHandlers   -- to set parameters
 *      myThinger       -- to set parameters
 * 
 * Depends on:
 *      myEngClasses
 *      myGlobals
 */

#include <mgnClass.h>
#include <SimpleSTA.h>      // needed for devName()

#include "myEngClasses.h"   // this already includes myGlobals, indurClass, oledClass
#include "mySupport.h"

// ----------------------------- CLASS ALLOCATIONS ---------------------------------

static MGN mgn("CONFIG");                       // used for this module
TICsec secTic( 2 );                             // every second. Used by main loop() but modified here

/* --------------- Used by main loop() to show data if scan is on ------------------
 *  See header for mask values
 *  --------------------------------------------------------------------------------
 */
    char *minsec( uint32 t )
    {
        int hours = t/3600;
        t -= hours * 3600;
        int minu = t/60;
        t -= minu * 60;
        int sec = t;
        static char tim[32];
        if( hours > 0 )
            sf( tim, 32, "%dhrs %02d:%02d", hours, minu, sec );
        else if( minu > 0 )
            sf( tim, 32, "%02d:%02d (%ds)", minu, sec, minu*60+sec );
        else
            sf( tim, 32, "%ds", t );
        return tim;
    }
    void reportINP( int mask )                      // reports whatever the current values are
    {
        if( mask & STREAM_CONSOLE )                 // report to console
        {
            if( idur.stateON() )
                PF( "ON for %ds. T0:%ds T1:%ds\r\n", idur.getCycleSec(), idur.getTsec(0), idur.getTsec(1)  );
            else
                PF( "OFF. T0:%ds T1:%ds\r\n", idur.getTsec(0), idur.getTsec(1)   );
        }
        if( mask & STREAM_MEGUNO )
        {
            if( idur.stateON() )
                mgn.controlVarArgs( "TStatus", "Heater is ON for %ds", idur.getCycleSec()  );
            else
                mgn.controlVarArgs( "TStatus", "Heater is OFF");
    
            mgn.controlVarArgs( "T0dur",   "%s", minsec(idur.getTsec(0)) );
            mgn.controlVarArgs( "T1dur",   "%s", minsec(idur.getTsec(1)) );
            
            mgn.tplotData( "TmLine",  idur.stateON()  );
        }   
        
        // --------------- always display OLED ------------------------------
    
        if( idur.stateON() )
            oled.dsp( 2, "\t%ds", idur.getCycleSec() );
        else
            oled.dsp( 2, "\t " );
        
        oled.dsp( 4, "\tT0:%d", idur.getTsec(0) );
        oled.dsp( 5, "\tT1:%d", idur.getTsec(1) );
        oled.dsp( 6, "\tHr:%.3f", myp.gp.hrsOper );
        oled.dsp( 7, "\t%s", tmb.getTime() );
    }

    // This routine is called if ANY PARAMETER has changed
                                                       
    #define IF(A) if( !strcasecmp( A, parm ) )
    static void doParmFunc( char *parm, char *value )
    {
        // Here, parameter has been updated. Save in EEPROM, update Meguno, and 
        //  selectively call initialization functions.
                 
        PFN( "Calling function associated to 'set %s %s'", parm, value );

        IF( "scanON" )
            mgn.controlSetCheck( "cScan", atoi(value) );      // update check box
        IF( "scanSec" )
            secTic.setSecCount( atoi( value ) );
        IF( "obright" )
        {
            oled.dsp( O_LED130 );
            oled.dsp( BRIGHT(myp.gp.obright) );
        }
    }    
    #undef IF

    bool setAnyEEParm( char *parm, char *value, BUF *bp )
    {        
        if( !nmp.setParmByStr( parm, value ) )          // if EEPROM User parm not found
        {
            RESPOND( bp, "%s not found\r\n", parm );
            return false;
        }        
        // Here, parameter has been updated. Save in EEPROM, update Meguno, and 
        //  selectively call initialization functions.

        myp.saveMyEEParms();                            // save to EEPROM
        doParmFunc( parm, value );                      // perform function associated with this parameter
        RESPOND( bp, "User parm %s updated\r\n", parm ); 
        return true;
    }   
    bool setAnyMegParm( char *parm, char *value, BUF *bp )
    {
        char *channel="CONFIG";
        if( !nmp.setParmByStr( parm, value ) )          // if EEPROM User parm not found
        {
            nmp.printMgnInfo( channel, parm, "is unknown" );     
            RESPOND( bp, "%s not found\r\n", parm );
            return false;
        }         
        myp.saveMyEEParms();                            // save to EEPROM
        nmp.printMgnParm( channel, parm );              // update the table
        nmp.printMgnInfo( channel, parm, "updated" );   // update the INFO  

        doParmFunc( parm, value );                      // perform function associated with this parameter
        RESPOND( bp, "User parm %s updated\r\n", parm ); 
        return true;
    }
    
    void showMegAllParms()
    {
        mgn.tableClear();
        mgn.tableSet( "INFO",       " ", "All EEP Parms" );
        mgn.tableSet( "WIFI PARMS", " ", "----------------------------" );
        eep.printMgnWiFiParms( "" ); 
        mgn.tableSet( "USER PARMS", " ", "----------------------------" );
        nmp.printMgnAllParms( "" );
    }

    char *getAllState( BUF *s )
    {
        s->init();
        s->add("streamON:%d\r\n", myp.streamON );
        return s->c_str();
    }
    // refreshes state based on display setting
    void refreshState( int mask, BUF *bp )
    {
        BUF sb(200);
        char *p = getAllState( &sb );
        if( mask & STREAM_MEGUNO )
            mgn.controlSetText( "state", p );
        if( mask & STREAM_CONSOLE )
            RESPOND( bp, "%s", p );
    }
    /* --------------------------------- OIL & TANK calculations --------------------------
 *  Set 'hrsOper' (in hours) to zero when tank is Tc gallons.
 *  'hrsOper' increments as heater is running, and Tc decreases.
 *  Tank gallons are estimated as Te using the formula:
 *  
 *          Te = Tc - hrsOper * c, or
 *          Tc - Te = hrsOper * c
 *          
 *  where c is the coeficient in gallons per hour. 
 *  If the actual tank indicator is Ta, the correct coefficient would be
 *  
 *          (Tc - Ta) = hrsOper * c', or
 *          c' = (Tc-Ta)/hrsOper
 *          
 * Use Slider and press TANK LEVEL. This sets hrsOper<-0 and tankCal<-given level
 * tankEst is shown in display.
 * Press CALIBRATE. This uses tankEst, tankCal, hrsOper and c to compute the new c'
 * 
 * You can also use the CLI 'compute GAL hrsOper' to compute c'
 * 
 */
        void updateTankGallons( uint32 durSec )
        {
            myp.gp.hrsOper += ((float) durSec)/3600.0;
            myp.gp.tankEst  = myp.gp.tankCal - myp.gp.hrsOper * myp.gp.galperhr;
            
            myp.saveMyEEParms();                            // save to EEPROM
            if( myp.gp.display & 2 ) 
                showMegAllParms();  
        }
        void setTankGallons( int gallons )
        {
            myp.gp.hrsOper = 0.0;
            myp.gp.tankCal = (float) gallons;
            myp.gp.tankEst = myp.gp.tankCal;
            myp.saveMyEEParms();                            // save to EEPROM
            if( myp.gp.display & 2 ) 
                showMegAllParms();  
        }
        float computeGPMCoef( int gallons )
        {
            if( myp.gp.hrsOper >= (1.0/60.0) )              // at least one minute of operation
            {
                return (myp.gp.tankCal - (float) gallons) / myp.gp.hrsOper;
            }
            return myp.gp.galperhr;                         // if not, report previous coef
        }
        const char *computeDaysToGo()
        {
            static char est[16];
            float gallonsused = myp.gp.tankCal - myp.gp.tankEst;
            if( gallonsused <= 1.0 )
                return "Unknown";
            else
            {
                float T = myp.gp.tankEst * myp.gp.hrsOper / gallonsused;  
                T /= 24.0;
                if( T < 1.0 )
                    return "< 1";
                else if( T > 30.0 )
                    return "> 30";
                else
                {
                    sf( est, 16, "%.0f", T);
                    return (const char *)est;
                }
            }
        }    
