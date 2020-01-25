#include "cpuClass.h"
#include "cliClass.h"
#include "SndDetect.h"

CLI cli;
CPU cpu;
SNDETECT sndet;

void setup()
{
	cpu.init(9600);
	SHOWDIAG("Hi There");
	cli.init( ECHO_ON, "cmd: " );
	oled.dsp( O_LED096 );	
	oled.dsp( 0, "\bPRESS BT");
	cli.prompt();
	sndet.resetT1();
}
#include <ticClass.h>
TICsec tic( 3600 );		// every hour
TICsec beat( 60 );		// every 60 sec
static int hour = 0;

void loop()
{
	if( tic.ready() )
	{		
		PF("\r\nHour %d, Total duration = %.1fs\r\n", hour++ );
        PF("Total duration = %.1fs\r\n", toSec( sndet.getT1() ) );
		sndet.resetT1();
	}
//	if( beat.ready() )
//		PF("-");
	
	if( sndet.stateChanged() )
	{
		if( sndet.stateON() )
		{
			oled.dsp( 4, "\t\bSet");
			PF("Started | ");
		}
		else
		{
			oled.dsp( 4, "\t\b%.1fs", ((float)sndet.getCycleON())/1000.0 );
			PF("Ended %.1fs | ", toSec( sndet.getCycleON() ) );
			PF("T1 %.1fs\r\n", toSec(sndet.getT1() ) );	
		}
	}
	if( cli.ready() )
	{
		char *p = cli.gets();
		if( *p == 'r' )
		{
			sndet.resetT1();
			PFN("T1 RESET");
		}
		else if( *p == 't' )
		{
			PFN("T1 = %.1f", toSec(sndet.getT1()) );
		}
		cli.prompt();
	}
}
