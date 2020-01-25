#pragma once
#include <cliClass.h>

    #define INC_THINGER                 // select this to enable THINGER interface
    //#undef  INC_THINGER               // select this to enable THINGER interface

    #ifdef INC_THINGER
        #define INC_DROPBOX                 // select this to inclue Thinger Dropbox via IFTTT
        //#define INC_EMAIL                   // select this to inclue Thinger EMail
        //#define INC_TEXT                    // select this to inclue Thinger ClickSend
        //#define INC_NOTIFY                  // select this to inclue Thinger IFTTT notifications. DOES NOT WORK -- GETS STUCK
        //#define INC_ALARM
        
        void setupThinger();
        void thingHandle();
        void thingStream( const char *type );
        
        #define SETUP_THINGER() setupThinger()
        #define THING_HANDLE()  thingHandle()
        #define THING_STREAM(A) thingStream(A)
        
        void sendToDropbox(bool header = false );    
        
        extern CMDTABLE thsTable[];
    #else
        #undef INC_DROPBOX                 // select this to inclue Thinger Dropbox via IFTTT
        #undef INC_EMAIL                   // select this to inclue Thinger EMail
        #undef INC_TEXT                    // select this to inclue Thinger ClickSend
        #undef INC_NOTIFY                  // select this to inclue Thinger IFTTT notifications. DOES NOT WORK -- GETS STUCK
        #undef INC_ALARM

        #define SETUP_THINGER() 
        #define THING_HANDLE()  
        #define THING_STREAM(A)  
    #endif
