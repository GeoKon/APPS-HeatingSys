#include "indurClass.h"

	bool INDUR::pinON()
	{
        if( simactive )
            return simulpin;
        else
    		return (dpin & NEGATIVE_LOGIC) ? !digitalRead( dpin&0x7FFF ) 
    									   :  digitalRead( dpin&0x7FFF );
	}
	INDUR::INDUR( int iopin, OLED *oledp )
	{
		dpin = iopin;
        op = oledp;
		state = STABLE_OFF;
		onoff = false;
        iochange = false;
		T0 = Tstart = Tcycle = 0L;
	    simactive = simulpin = false;
		Sdur[0] = Sdur[1] = Sold[0] = Sold[1] = Tref[0] = Tref[1] = 0L;
	}	
	void INDUR::simulate( bool simon )      // activates-deactivates dimulation
    {
        simactive = simon;
        simulpin = false;                   // force simulation state OFF  
        onoff    = false;
        iochange = false;
    }
    bool INDUR::isSimulON()                 { return simactive; }
    void INDUR::setSimul( bool simstate )   { simulpin = simstate; }    
    bool INDUR::getSimul()                  { return simulpin; }    
/*  
 *  pollPin() is a 'private' function.
 *  Call frequently to check the iopin state.
 *  if state changes to ON, sets the Tstart timer, sets onoff, and set 'iochange' to TRUE
 *  if state changes to OFF, computes Tcycle, resets 'onoff', and also sets 'iochange' to TRUE
 *  While state is ON, continues updating Tcycle and returns setting 'iochange' to FALSE
 */
	void INDUR::pollPin()
    {
        switch( state   )                   // if was OFF...
        {
            default:
            case STABLE_OFF:
                if( pinON() )               // detected ON
                {
                    state = CHANGED_ON;
                    T0 = millis();
                    SHOWDIAG("ON?");
                }               
                break;                      // else, stay in the same state
            
            case CHANGED_ON:
                if( pinON() )
                {
                    Tstart = millis();
                    Tcycle = 0;
                    if( Tstart-T0 > 1000L )
                    {
                        state = STABLE_ON;
                        onoff = true;
                        SHOWDIAG("ON OK");
                        iochange = true;
                        return;
                    }
                    break;                  // else, stay in the same state
                }
                state = STABLE_OFF;         // if OFF, go back to idle state
                SHOWDIAG("OFF");
                break;
            
            case STABLE_ON:
                Tcycle = millis() - Tstart;     // tracks duration
                if( !pinON() )                  // just went off
                {   
                    state = CHANGED_OFF;
                    T0 = millis();
                    SHOWDIAG("OFF?");
                    break;                  // return false in all cases
                }
                break;                      // remain in the same state
            
            case CHANGED_OFF:
                if( !pinON() )
                {
                    if( millis()-T0 > 500L )
                    {
                        state = STABLE_OFF;
                        onoff = false;
                        SHOWDIAG("IDLE");
                        iochange = true;        
                        return;
                    }
                    break;                  // not enough debouncing off
                }
                state = STABLE_ON;
                SHOWDIAG("ON");
                break;
        }
        iochange = false;
    }
	bool INDUR::stateChanged()
	{
        pollPin();
        if( iochange && (onoff == false) )  // cycle just ended
        {
            Sdur[0] += Tcycle/1000L;          // add the duration of this cycle
            Sdur[0] -= Tref[0]/1000L;         // and subtract the initial segment
            Tref[0] = 0;                      // set Tref1 to zero for the next cycle

            Sdur[1] += Tcycle/1000L;          // add the duration of this cycle
            Sdur[1] -= Tref[1]/1000L;         // and subtract the initial segment
            Tref[1] = 0;                      // set Tref1 to zero for the next cycle
        }
        return iochange;
	}
	bool INDUR::stateON()
	{
 	    return onoff;
	}
/*
 *  Return the Tcycle duration in msec.
 *  If onoff is true, the Tcycle is incremented by stateChanged()
 *  If false, the Tcycle indicated the previous duration
 */
	uint32 INDUR::getCycleON()
	{
		return Tcycle;
	}
    uint32 INDUR::getCycleSec()
    {
        return Tcycle/1000;
    }
	void INDUR::resetT( int i )
	{
		Sold[i] = Sdur[i];                  // save old value
		Sdur[i] = 0;
        if( onoff ) 
            Tref[i]  = Tcycle;              // save for later the ms from the beginning of the cycle
        else
            Tref[i] = 0;
 	}
	uint32 INDUR::getTsec(int i)	        // Returns T1 duration since resetT1()
	{							        
		if( onoff )                         // if in the middle of a cycle, return adjusted duration
        {
            return Sdur[i] + (Tcycle-Tref[i])/1000;
        }
		else
		    return Sdur[i];			        // Duration was added by stateChanged() if cycle is completed
	}
    uint32 INDUR::getPriorT(int i)          // Returns Ti duration of previous interval
    {                                   
        return Sold[i];                     // Duration was added by stateChanged() if cycle is completed
    }

//   float toSec( uint32 u )
//   {
//        return ((float) (u/100))/10.0;
//   }
//    uint32 sub( uint32 u1, uint32 u2 )
//    {
//        if( u1 > u2 )
//            return u1-u2;
//        
//        uint32 y = 0xFFFF-u2;
//        return u1 + y + 1;
//    }
