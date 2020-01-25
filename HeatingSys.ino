//  **** Enable or disable Thinger.io in myThinger.h ****

    #define METER_READING_PERIOD 2                          // seconds. How often to read the meter
    #define ALERT_REPEAT_PERIOD (60/METER_READING_PERIOD )  // seconds. How long to wait for IFTTT-NOTIFYrepeats

// ----------------------------------------------------------------------------
    #include <FS.h>
    #include <ArduinoOTA.h>
    #include <Ticker.h>         // needed to RESET statistics
    
    #include "mgnClass.h"
    #include "SimpleSTA.h"      // in GKE-Lw
    #include "CommonCLI.h"      // in GKE-Lw
                                
    #include "myGlobals.h"      // in this project. This includes externIO.h
    #include "myCliHandlers.h"
    #include "mySupport.h"
    #include "myEngClasses.h"   // this also includes pz16Class.h
    #include "myThinger.h"  
    
//------------------ References and Class Allocations ------------------------

    Ticker tk;
    CLI cli;
    EXE exe;
    EEP eep;
    BUF buffer( 1600 );         // buffer to hold the response from the local or remote CLI commands.

// ------------------------------------ MAIN SETUP ----------------------------------

    static int line = 0;                        // oled line counter
    #define DSP_CLEAR   oled.dsp( O_CLEAR ); line=0
    #define DSP(A, ...) oled.dsp( line++, A, ##__VA_ARGS__)

    void setupOTA( char *name );                // forward reference

void setup() 
{
    int runcount = setjmp( myp.env );           // env is allocated in myGlobals
    cpu.init( 9600 );
    pinMode( SWINP, INPUT_PULLUP );

    ASSERT( SPIFFS.begin() );                   // start the filesystem;
    
    char *name = getDevName( "Heat" );           // make the name for this device

    oled.dsp( O_LED096 );                       // initialize OLED
    DSP_CLEAR;                                  

// *********************************            // Revise this!
    DSP( "\t%s (Jan 2020)", name  );
    PFN( "mDNS: %s.local", name );
// Use 
//      dns-sd -Z _arduino._tcp
//      dns-sd -G v4 name.local
//      mobile ZeroConfig
// *********************************

    myp.initAllParms();                         // initialize volatile & user EEPROM parameters
    DSP( "Read %d parms", nmp.getParmCount() );
    oled.dsp( BRIGHT( myp.gp.obright ) );       // adjust OLED brightness
  
    linkParms2cmnTable( &myp );                 // Initialize CLI tables
    exe.initTables();                           // clear all tables
    exe.registerTable( cmnTable );              // register common CLI tables. See GKE-Lw
    exe.registerTable( mypTable );              // register tables from myCliHandlers.cpp
                                                // See myThinger.cpp for thsTable[]
    DSP( "Waiting for CLI" );           
    DSP( "or BTN SmartConf" );           
    startCLIAfter( 5/*sec*/, &buffer );         // this also initializes cli(). See SimpleSTA.cpp    
                                                // If button is pressed, does SmartConfig of WiFi
    DSP( "Waiting for WiFi" );

    setupWiFi();                                // use the EEP to start the WiFi. If no connection
    setupOTA( name );                           // prepares and starts OTA. mDNS is GkePwr-MAC05
    
    DSP( "%s", WiFi.SSID().c_str() );
    DSP( "RSSI:%ddBm", WiFi.RSSI() );
    IPAddress ip=WiFi.localIP();
    DSP( "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3] );
    
    SETUP_THINGER();                            // if thinger is not included, it does nothing

    // ------------- Other initialization --------------------------------

    secTic.setSecCount( myp.gp.scansec );       // update the tic used to read PZEM
  
    idur.resetT(0);                             // reset hourly statistics
    idur.resetT(1);                             // reset daily statistics
    
    tmb.setHMS( 20, 40, 0 );                    // Set current time. Use CLI setTime
    tk.attach_ms(1000,[](){tmb.updSeconds();}); // Normal operation
//  tk.attach_ms( 500,[](){tmb.updMinutes();}); // Use this to test in "fast speed" x120 (1hr in 30sec, 1day in 12min)

    showMegAllParms();                          // located in mySupport.cpp
    delay( 4000 );
    DSP_CLEAR;
    cli.prompt();
}

// ------------------------- OTA PREP -------------------------------------------------------

void setupOTA( char *devname )                          // generic OTA handler
{
    // ArduinoOTA.setPort(8266);                        // this is the default port for OTA
  
    ArduinoOTA.setHostname( devname );                  // set mDNS name

    ArduinoOTA.onStart([]() { oled.dsp( O_CLEAR ); oled.dsp( 0,"\t\bOTA"); });
    ArduinoOTA.onEnd([]()   { oled.dsp( 6, "\t\bDONE"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        oled.dsp(3, "\t\b%u%%", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) 
    { 
        oled.dsp( O_CLEAR );
        oled.dsp( 0,"\t\bOTA Err%u", error);
        if (error == OTA_AUTH_ERROR)            oled.dsp(3, "\tAuth Failed");
        else if (error == OTA_BEGIN_ERROR)      oled.dsp(3, "\tBegin Failed");
        else if (error == OTA_CONNECT_ERROR)    oled.dsp(3, "\tConnect Failed");
        else if (error == OTA_RECEIVE_ERROR)    oled.dsp(3, "\tReceive Failed");
        else if (error == OTA_END_ERROR)        oled.dsp(3, "\tEnd Failed");
    });
    ArduinoOTA.begin();    
}
// ------------------------ MAIN LOOP -------------------------------------------------------

static int process_withWiFi = 0;                // this flag is set by stdLoop() to indicate to
                                                // main loop() that WiFi processing is necessary
                                                // 1 = stream real time measurements 
                                                // Flag is reset by the main loop()
                                                
void stdLoop()                                  // This is always executed regardless if WiFi connected or not
{
    ArduinoOTA.handle();
   
    if( cli.ready() )                           // handle serial interactions
    {
        char *p = cli.gets();
        exe.dispatchBuf( p, buffer );           // required by Meguno
        buffer.print();
        cli.prompt();
    }
    if( idur.stateChanged() )                   // check if io-pin has changed state
    {
        MGN mgn;
        if( idur.stateON() )
            cpu.led( ON );
        else
            cpu.led( OFF );
        reportINP( myp.gp.display );            // report changes
    }
    if(  secTic.ready() && myp.gp.scanON )      // secTic is allocated in mySupport.cpp
    { 
        reportINP( myp.gp.display );            // show to console accordingly
        process_withWiFi |= 1;                  // show to Thinger if activated and WiFi OK
    }
//    if( tmb.hrsWrap() )                         // reset T0 statistics every hour
//    {        
//        static uint32 priorDaily = 0;
//        updateTankGallons( idur.getTsec(0) );   // update tank level
//
//        // Prep data for Thinger 
//        myp.reportHourSec   = idur.getTsec(0);
//        myp.reportTodaySec  = idur.getTsec(1);       // This did not work! idur.getPriorT(1);
//        myp.reportYesterSec = priorDaily;           // This did not work! idur.getPriorT(1);
//        myp.reportTankLevel = myp.gp.tankEst;
//        myp.reportGalUsed   = myp.gp.galperhr * ((float) idur.getTsec(0))/ 3600.0;
//
//        PFN( "HourON=%ds, TodayON=%ds, YesterON=%ds, GalUsed=%.3f, TankLevel=%.2f",
//            myp.reportHourSec,
//            myp.reportTodaySec,
//            myp.reportYesterSec,                    
//            myp.reportGalUsed,
//            myp.reportTankLevel );
//        
//        idur.resetT(0);
//        if( tmb.dayWrap() )
//        {
//            priorDaily = idur.getTsec(1);
//            idur.resetT(1);                     // reset T1 statistics every day
//        }
//        process_withWiFi |= 2;
//    }
}

TICsec streamTic( 30 );                         // 30 second reporting, regardless if streaming is ON or OFF

void loop()                                     // Main loop
{
    stdLoop();
    if( checkWiFi() )                           // Good WiFi connection?
    {    
        if( process_withWiFi & 1 )
        {
            process_withWiFi &= 2;                          // flip bit 0 to zero
            if( myp.streamON /* ||streamTic.ready() */ )    // myp.streamON is the only place that is used. 
                THING_STREAM("reading");                    // Option to force streaming every 30 sec (by streamTic) was removed
                                                            // to allow selective store to a HEAT_RT bucket
        }
//        if( process_withWiFi & 2 )
//        {
//            process_withWiFi &= 1;              // flip bit 1 to zero
//            THING_STREAM("HourStats");    
//            if( myp.gp.dbMinut )
//                sendToDropbox();
//        }
        THING_HANDLE();                         // Continue polling. If WiFi is diconnected
    }
    else
        reconnectWiFi();                            // reconnect to WiFi (non-blocking function)
}
