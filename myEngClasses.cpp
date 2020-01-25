#include "myEngClasses.h"          // this also include myGlobals.h

// ----------------------------------- global allocations -----------------------------
    
    CPU  cpu;                    // needed for Sec UART
    OLED oled;
    INDUR idur( SWINP, &oled );  // allocation of Inp Duration Class
    
    TMBASE tmb;
/* 
 *  To use synchronized Tics,use:
 *    TOCsec ticMin(60);            // every min
 *    TOCsec ticHr (60*60);         // every hour
 *
 *    tmb.setHMS( 20, 40, 0 );                    // set current time
 *    ticMin.setSecCount( tmb.getSecCount() );
 *    ticHr.setSecCount( tmb.getSecCount() );
 *    tk.attach_ms( 1000, [](){ tmb.updSeconds(), ticMin.update(); ticHr.update(); } );  
 */

// --------------------------------- Time Base -------------------------

    TOCsec::TOCsec( uint32 sec )
    {
        _seccnt = 0;
        _secrdy = false;
        _secmax = sec;
    }
    void TOCsec::setModulo( uint32 sec )
    {
        _secmax = sec;
    }
    void TOCsec::update()       // to be executed every sec
    {
        _seccnt ++;
        if( _seccnt >= _secmax )
        {
            _seccnt = 0;
            _secrdy = true;
        }
    }
    void TOCsec::setSecCount( uint32 secref )
    {
        _seccnt = secref % _secmax;     // sets count to modulo
    }
    bool TOCsec::ready()
    {
        if( _secrdy )
            return !(_secrdy = false);
        return false;
    }    
// -------------------------------------------------------
    
    TMBASE::TMBASE()
    {
        setHMS();   // all zeros by default
    }        
    void TMBASE::setHMS( int hrs0, int min0, int sec0 )
    {
        _sec = sec0; _min=min0; _hrs=hrs0;
        _hrsWrap = _dayWrap = false;
    }
    uint32 TMBASE::getSecCount()
    {        
        return 3600*_hrs + 60*_min + _sec;
    }
    void TMBASE::updMinutes()       // this is called every minute
    {
        _sec = 0;
        _min++;
        if( _min >= 60 )
        {
            _min = 0;
            _hrs++;
            _hrsWrap = true;
            if( _hrs >= 24 )
            {
                _hrs = 0;
                _dayWrap = true;
            }
        }
    }  
    void TMBASE::updSeconds()        // this is called every second
    {
        _sec ++;
        if( _sec >= 60 )
            updMinutes();
    }
    bool TMBASE::hrsWrap()
    {
        if( _hrsWrap )
            return !(_hrsWrap = false);
        return false;
    }
    bool TMBASE::dayWrap()
    {
        if( _dayWrap )
            return !(_dayWrap = false);
        return false;
    }
    const char *TMBASE::getTime()
    {
        static char temp[32];   // must be static
        int hrs;
        char *ds;
        hrs = _hrs;
        ds = "am";
        if( _hrs>12 )
        {
            hrs = _hrs-12;
            ds = "pm";
        }    
        return (const char *)sf( temp, sizeof( temp ), "%02d:%02d:%02d %s", hrs, _min, _sec, ds );
    }
