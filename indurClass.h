#pragma once

#include "cpuClass.h"
#include "oledClass.h"

extern OLED oled;

enum snstate_t
{
	STABLE_OFF  = 0,
	CHANGED_ON  = 1,	
	CHANGED_OFF = 2,
	STABLE_ON   = 3
};

#define SHOWDIAG( A ) if( op ) op->dsp( 0, "\t\b"A )
class INDUR
{
	int dpin;			// pin number. Optionally with ORed NAGATIVE_LOGIC
	OLED *op;           // pointer to OLED. Not used if NULL
    
	snstate_t state;	// one of the above states			
	uint32 T0;			// time tic at the time of CHANGED_ON or _OFF
	bool onoff;	        // debounced state ON or OFF
    bool iochange;
    
	uint32 Tstart;		// point of time of stable detection (in msec)
	uint32 Tcycle;		// duration ON (from Tstart until first low)

    bool simactive;     // simulation is active or not
    bool simulpin;      // if active, is state ON or OFF
	uint32 Sdur[2]; 	// final duration (in seconds)
    uint32 Sold[2];     // previous duration (in seconds)
    uint32 Tref[2];      // portion of Tcycle when resetT1();
    
	bool pinON();		// sample of the pin state
    void pollPin();
    
public:	
	INDUR( int iopin = 0+NEGATIVE_LOGIC, OLED *oledp = NULL );

    void simulate( bool simon );        // enable or disable simulation
    bool isSimulON();
    void setSimul( bool simstate1 );    // set simulation ON or OFF
    bool getSimul();                    // get simulated state
    bool runSimul();

	bool stateChanged();		// True if change. Includes debouncing
	bool stateON();				// Reports state. Use after stateChange()
	uint32 getCycleON();		// Reports cycle duration starting from previous
    uint32 getCycleSec();
        
	void resetT( int i );		// Resets/starts duration timer T1
    uint32 getTsec( int i ); 
    uint32 getPriorT( int i );           
};
//float toSec( uint32 u );
//uint32 sub( uint32 u1, uint32 u2 );
