#include "myThinger.h"
#ifdef INC_THINGER

    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "mySupport.h"
    #include "myEngClasses.h"   // this also includes indurClass
    
    #define USERNAME "GeoKon"
    #define DEVICE_ID "HEATSYS"
    #define DEVICE_CREDENTIAL "success"
//  #define _DEBUG_
    #define _DISABLE_TLS_           // very important for the Thinger.io 

    #include <ThingerESP8266.h>     // always included even if not #ifdef
       
//------------------ Class Allocations ------------------------------------------------------------------

    ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);   // changed in the setupThinger();

// ------------------------------- THINGER INITIALIZATION -----------------------------------------------
    void thingHandle()
    {
        thing.handle();
    }
    void thingStream( const char *type )
    {
        thing.stream( thing[type] );
    }    
    
    static char exrsp[1500];        // Buffer used for Thinger CLI. Must be static
    BUF exbuf( exrsp, 1500 );       // BUF wrapper of exstr[] 

    void setupThinger()
    {
        PF("----- Initializing THING\r\n");
        thing.add_wifi( eep.wifi.ssid, eep.wifi.pwd );  // initialize Thinker WiFi

        thing.set_credentials( USERNAME, DEVICE_ID, DEVICE_CREDENTIAL );
        
        exe.registerTable( thsTable );              // register Thinger Tables
        
// ----------------------------------------------------------------------------
//                   Thinger.io CLI
// ----------------------------------------------------------------------------
        thing["CliCmd"] << [](pson &in)
        {   
            const char *p = (const char *)in["extCmd"];
            PFN("Thinger cmd [%s]", p );
            exe.dispatchBuf( p, exbuf );            // execute command
            PR( exrsp );                            // print results to console     
        };        
        thing["CliRsp"] >> [](pson& ot )                          
        {   
            //PFN( "Thinger rsp");
            PR( exrsp );
            ot["value1"] = (const char *)exrsp;         
        };
        thing["help"] << [](pson &inp)
        {   
			if( inp.is_empty() ) 
			{
				inp=0;
				return;
			}
			int sel = inp;
            char *p;
            switch( sel )
            {
            	default:
            	case 1: p="h"; break;
            	case 2: p="Parms"; break;
            	case 3: p="Wifi"; break; 
            }
            exe.dispatchBuf( p, exbuf );         	// execute command
            PR( exrsp );                            // print results to console     
        };

/* --------------------------------------- HELP & DIAGNOSTICS ------------ ------------------------------------- 
  * 
  * ------------------------------------------------------------------------------------------------------------ */
        thing["led"]        << invertedDigitalPin( MYLED ); 

 /* --------------------------------------- ACQUISITION & STREAMING CONTROL ------------------------------------- 
  * 'scanON is saved in EEPROM (non-volatile). 'streamON' starts as OFF (volatile).
  * ------------------------------------------------------------------------------------------------------------ */
         thing["scanON"]     << [](pson &inp)   {   if( inp.is_empty() ) 
                                                    {
                                                        inp = (bool) myp.gp.scanON;     // return simulated kW
                                                    }
                                                    else                                        // set simulated watts
                                                    {
                                                        char value[8];                                                            
                                                        sf( value,8,"%d", (bool) inp );
                                                        if( myp.gp.display & 2 ) setAnyMegParm( "scanON", value, &exbuf );
                                                        else                     setAnyEEParm ( "scanON", value, &exbuf );   
                                                    }
                                                };
                                                 
        thing["streamON"] << [](pson &inp)    {   if( inp.is_empty() ) 
                                                    inp = myp.streamON ? true : false;     // get state of streamON
                                                else                            
                                                {
                                                    myp.streamON = (bool) inp;
                                                    refreshState( myp.gp.display, &exbuf);
                                                }
                                              };                                                                                                      

/* --------------------------------------- SIMULATION ---------- ------------------------------------------------
 * 'simulV' is set by a slider to values 90V-140V. 'simulW' is also set by a slider to values 0-10kW. 
 * -------------------------------------------------------------------------------------------------------------- */
 
        thing["simONF"] << [](pson &inp)  {   
                                            if( inp.is_empty() )                        // if simulation is enabled,
                                                inp =  idur.isSimulON() ?               //   show simulated state
                                                        idur.getSimul() : false;              
                                            else                                        // set simulated state
                                                idur.setSimul( idur.isSimulON() ? 
                                                                     (bool) inp : false );
                                          };
       thing["simul"] << [](pson &inp)    {   
                                            if( inp.is_empty() ) 
                                                inp=idur.isSimulON();                   // show simulation activation state
                                            else                                        
                                                idur.simulate( (bool) inp );            // enable or disable simulation
                                            thing.stream("simONF");                     // update the simulated state
                                          };

/* --------------------------------------- Notification HANDLERS ------------------------------------------------
 * notfyV is entered as a slide in Thinger. It indicates the undervoltage condition to trigger IFTTT notification
 * notfyW is entered as a slide in Thinger. It indicates the overpower condition to trigger IFTTT notification
 * The 'notfy'-bits 1, 2, 4 eeprom parameter are used as indicators/masks of which condition were set
 * -------------------------------------------------------------------------------------------------------------- */
        static int tankLevel = 0;
        thing["tankLevel"]  << inputValue( tankLevel );
        thing["tankCal"]    << [](pson &inp)  { if( inp.is_empty() )       // resets T0 (hourly) ON time
                                                    inp = false;
                                                else                            
                                                {
                                                    inp = true;
                                                    setTankGallons( tankLevel );
                                                }
                                              }; 
        thing["compGPM"]    << [](pson &inp)  { if( inp.is_empty() )       // resets T0 (hourly) ON time
                                                    inp = false;
                                                else                            
                                                {
                                                    inp = true;
                                                    computeGPMCoef( tankLevel );
                                                }
                                              }; 

/* --------------------------------------- RESET kWh Energy -----------------------------------------------*/

       thing["reset0"]    << [](pson &inp)    {  if( inp.is_empty() )       // resets T0 (hourly) ON time
                                                    inp = false;
                                                else                            
                                                {
                                                    inp = true;
                                                    idur.resetT(0);
                                                }
                                              }; 
       thing["reset1"]    << [](pson &inp)    {  if( inp.is_empty() )       // resets T1 (daily) ON time
                                                    inp = false;
                                                else                            
                                                {
                                                    inp = true;
                                                    idur.resetT(1);
                                                }
                                              }; 

/* --------------------------------------- DISPLAY CONTROL ----------------------------------------------------- */

        thing["display"]     << [](pson &inp)   {   if( inp.is_empty() ) 
                                                    {
                                                        inp = (int) myp.gp.display;     // return simulated kW
                                                    }
                                                    else                                        // set simulated watts
                                                    {   
                                                        char value[8];                                                            
                                                        sf( value,8,"%d", (int) inp );
                                                        if( myp.gp.display & 2 ) setAnyMegParm( "display", value, &exbuf );
                                                        else                     setAnyEEParm ( "display", value, &exbuf ); 
                                                         
                                                    }
                                                };
 
/* --------------------------------------- STREAMING OF MEASUREMENTS ---------------------------------------------
 * "reading" is updated by the DEVICE every N-seconds or so. The DEVICE updates the "myp.vap[]" (volts, amps, etc)
 *  Rotates one type of measurement at a time, i.e. all of them are done in two cycles
 * -------------------------------------------------------------------------------------------------------------- */

        static char T0dur[32], T1dur[32];
        thing["reading"] >> [](pson& ot )       // meter is read asynchronously by the loop() every N-seconds
        {   
            strcpy( T0dur, minsec( idur.getTsec(0) ));
            strcpy( T1dur, minsec( idur.getTsec(1) ));
            ot["T0dur"]     = (const char *)T0dur;
            ot["T1dur"]     = (const char *)T1dur;
            ot["onoff"]     = (int) idur.stateON();
            ot["cycle"]     = idur.stateON() ? (int) idur.getCycleSec() : 0;
            ot["status"]    = idur.stateON() ? "ON" : "OFF";
            ot["gallons"]   = myp.gp.tankEst;
            ot["myTime" ]   = tmb.getTime();
            ot["DaysToGo"]  = computeDaysToGo();
        };

/* --------------------------------------- STATISTICS (for the BUCKETS)------------------------------------------
 * [HourStats] is used by THINGER to save stats into HEAT_HOURLY bucket.
 * This is triggerred periodically by this bucket every hour. Use a shorter period (say 30sec) to test proper operation
 * 
 * It reports:
 *      [Hourly_ON]     number of seconds the heater was on for the past hour
 *      [Today_ON]      number of seconds the heater was on since midnight
 *      [Gallons_Used]  Gallons the past hour
 *      [Tank_Gallons]  amount of oil in the tank at the end of this hour
 *  
 * Hourly statistics are reset at the end of reporting.
 * -------------------------------------------------------------------------------------------------------------- */
        
        thing["HourStats"] >> [](pson& ot )         // Bucket HEAT_HOURLY calls this EVERY HOUR for 24+ hours. 
        {            
            updateTankGallons( idur.getTsec(0) );
            ot["Seconds_ON"  ]  = idur.getTsec(0);
            ot["TodaySec_ON" ]  = idur.getTsec(1); 
            ot["Gallons_Used"]  = myp.gp.galperhr * ((float) idur.getTsec(0))/ 3600.0; 
            ot["Tank_Gallons"]  = myp.gp.tankEst;
            sendToDropbox( false );                 // send the above to dropbox
            idur.resetT(0);                         // reset the above statistics
        };
        thing["DayStats"] >> [](pson& ot )          // Bucket HEAT_DAILY calls this EVERY DAY for 30+ days. 
        {            
            ot["Seconds_ON"  ] = idur.getTsec(1); 
            ot["Gallons_Used"]  = myp.gp.galperhr * ((float) idur.getTsec(1))/ 3600.0; 
            ot["Tank_Gallons"]  = myp.gp.tankEst;            
            idur.resetT(1);                        // reset the above statistics
            sendToDropbox( true );                 // send dropbox header
        };
    }                                               // end of setupThinker()

/* ----------------------------- NOTIFICATION HANDLERS ---------------------------- 
 * Includes:
 *               sending to Dropbox     Thinger(Web hooks)->Dropbox
 * -------------------------------------------------------------------------------- */

BUF ntfy(256);  // Common buffer used for all notifications.                           


void sendToDropbox( bool header ) 
{
    #ifdef INC_DROPBOX
        pson datn;
		datn["value2"] = "";
	#endif
	
	ntfy.init();
    if( header )
    {
        ntfy.set( "ThisHrON(sec), GallonsUsedThisHr, ON_Today(sec), GallonsUsedToday, TankRef, GPH, TankLevel");
    #ifdef INC_DROPBOX
        datn["value1"] = (const char *)ntfy.c_str();
        thing.call_endpoint( "DBox_HeatSys", datn );
        exbuf.set( "Sent to Dropbox\r\n" );  
    #endif
    }
    else
    {
        ntfy.set( "%d, %.3f, %d, %.1f, %.1f, %.2f, %.1f",
            idur.getTsec(0), 
            myp.gp.galperhr * ((float) idur.getTsec(0))/ 3600.0,
            idur.getTsec(1),        
            myp.gp.galperhr * ((float) idur.getTsec(1))/ 3600.0,        
            myp.gp.tankCal,
            myp.gp.galperhr,
            myp.gp.tankEst );
            
        PF("DROPBOX: %s\r\n", ntfy.c_str() );
            
        #ifdef INC_DROPBOX
            datn["value1"] = (const char *)ntfy.c_str();
            thing.call_endpoint( "DBox_HeatSys", datn );
            exbuf.set( "Sent to Dropbox\r\n" );  
        #endif
    }
}
CMDTABLE thsTable[]=            // functions to test IFTTT endpoints
{
    #define PROTO []( int n, char **arg )
    { "dropbox", "Send Measurements to Drobox",             PROTO { sendToDropbox(); } },
    {NULL, NULL, NULL}
};

#endif // INC_THINGER
