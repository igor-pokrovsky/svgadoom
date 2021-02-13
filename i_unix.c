#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>

#include <vga.h>
#include <vgakeyboard.h>
#include <vgamouse.h>

// Needed for alternative mouse handling stuff
#include <sys/mouse.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "d_main.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_misc.h"
#include "v_video.h"
#include "doomdef.h"

#include "i_sound.h"
#include "i_unix.h"

// How many megabytes DOOM will allocate for himself
#define MB_USED         4

byte*           pagebase;		// video memory page base address

boolean         video_inited;
boolean         keyboard_inited;
boolean         mouse_inited;

static int	md;		// descriptor to access to mouse ioctl's

byte        scantokey[128] =
{
        0,      KEY_ESCAPE,'1', '2',    '3',    '4',    '5',    '6',
        '7',    '8',    '9',    '0',    '-',    '=',    KEY_BACKSPACE,KEY_TAB,
        'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
        'o',    'p',    '[',    ']',    KEY_ENTER,KEY_LCTRL,'a','s',
        'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
        39,     '`',    KEY_LSHIFT,92,  'z',    'x',    'c',    'v',
        'b',    'n',    'm',    ',',    '.',    '/',    KEY_RSHIFT,'*',
        KEY_LALT,' ',   0  ,    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
        KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,0,      0,      0,
        KEY_UPARROW,0,  '-',    KEY_LEFTARROW,'5',KEY_RIGHTARROW,'+',0,
        KEY_DOWNARROW,0,0,      0,      0,      0,      0,      KEY_F11,
        KEY_F12,0,      KEY_RCTRL,0,    0,      KEY_RALT,0,     KEY_UPARROW,
        0,      KEY_LEFTARROW,KEY_RIGHTARROW,KEY_PGUP,KEY_DOWNARROW,KEY_PGDN,KEY_INS,KEY_DEL,
        KEY_PAUSE,0,	0,	0, 	0,  	0,	0,     	 0,
        0,      0,      0,      0,      0,      0,      0,       0,
        0,      0,      0,      0,      0,      0,      0,       0
};
        


void I_Tactile (int on, int off, int total)
{
  // UNUSED.
  on = off = total = 0;
}

ticcmd_t        emptycmd;

ticcmd_t*       I_BaseTiccmd(void)
{
    return &emptycmd;
}

int  I_GetHeapSize (void)
{
    return MB_USED*1024*1024;
}

byte* I_ZoneBase (int* size)
{
    *size = MB_USED*1024*1024;
    return (byte *) malloc (*size);
}

//
// I_GetTime
// returns time in 1/70th second tics
//
int  I_GetTime (void)
{
    struct timeval      tp;
    struct timezone     tzp;
    int                 newtics;
    static int          basetime=0;
    gettimeofday(&tp, &tzp);
    if (!basetime)
        basetime = tp.tv_sec;
    newtics = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;
    return newtics;
}

void I_WaitVBL (int count)
{
#ifdef SGI
    sginap(1);
#else
#ifdef SUN
    sleep(0);
#else
    usleep (count * (1000000/70) );
#endif
#endif
}


void I_BeginRead (void) {}
void I_EndRead (void) {}

byte* I_AllocLow (int length)
{
    byte*       mem;
    
    mem = (byte *) malloc (length);
    memset (mem,0,length);
    return mem;
}

//
// I_Error
//
extern boolean demorecording;

void I_Error (char *error, ...)
{
    va_list     argptr;

    va_start (argptr,error);
    vfprintf (stderr,error,argptr);
    fprintf (stderr, "\n");
    va_end (argptr);
    fflush( stderr );

    // Shutdown. Here might be other errors.
    if (demorecording)
        G_CheckDemoStatus();

    D_QuitNetGame ();
    I_Shutdown();

    exit (-1);
}    

void Debug (char* message, ...)
{
	FILE*	fp;
	va_list argptr;

	fp = fopen ("debug","a");

	va_start (argptr, message);
	vfprintf (fp, message, argptr);
	va_end (argptr);

	fclose (fp);
}

//
// I_Quit
//
void I_Quit (void)
{
    D_QuitNetGame ();
    M_SaveDefaults ();
    I_Shutdown ();
    exit(0);
}

/*
============
= I_Init
============
*/
void I_Init (void)
{
	I_InitSound ();
	I_InitMusic ();

	// Here init stuff, based on SVGAlib
    	vga_init ();
    	I_StartupKeyboard ();
    	I_StartupMouse ();
    	I_InitGraphics();
}

/*
==============
= I_Shutdown
==============
*/
void I_Shutdown (void)
{
        I_ShutdownSound ();
        I_ShutdownMusic ();

        I_ShutdownKeyboard ();
        I_ShutdownMouse ();
        I_ShutdownGraphics ();
        
}

/*
==============================================================================
= 				Video
==============================================================================
*/

void I_InitGraphics (void)
{
	int	r;

	r = vga_setmode (G320x200x256);
	if (r == -1)
		I_Error ("I_InitGraphics: Unable to init video mode.\n");
	else
		video_inited = true;		

	pagebase = vga_getgraphmem ();
	screens[0] = I_AllocLow (SCREENWIDTH*SCREENHEIGHT);
}

void I_ShutdownGraphics (void)
{
	if (video_inited)
        	vga_setmode (TEXT);
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
        static int      tmppal[256*3];
        int*            tp;
        int             i;

	if (video_inited)
	{
        	tp = tmppal;
        	for (i=256*3; i; i--)
                	*tp++ = gammatable[usegamma][*palette++] >> 2;

        	vga_setpalvec (0, 256, tmppal);
        }	
}


void I_StartFrame (void)	{}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    static int  lasttic;
    int         tics;
    int         i;

    // draws little dots on the bottom of the screen
    if (devparm)
    {
        i = I_GetTime();
        tics = i - lasttic;
        lasttic = i;
        if (tics > 20) tics = 20;
        for (i=0 ; i<tics*2 ; i+=2)
            screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
        for ( ; i<20*2 ; i+=2)
            screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

    if (video_inited)	
    {
    	vga_waitretrace ();
    	memcpy (pagebase, screens[0], SCREENWIDTH*SCREENHEIGHT);
    }	
}    

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}
    
/*
===============================================================================
= 				Keyboard
===============================================================================
*/
void keyboard_handler (int scancode, int state)
{
        event_t         ev;

        if (state == KEY_EVENTPRESS)
                ev.type = ev_keydown;
        else
                ev.type = ev_keyup;

        ev.data1 = scantokey [scancode];
        D_PostEvent (&ev);
}

void I_StartupKeyboard (void)
{
	int	r;

       	r = keyboard_init ();
       	if (r == -1)
       		I_Error ("I_StartupKeyboard: Unable to init keyboard.\n");
       	else
       		keyboard_inited = true;
        		
       	keyboard_seteventhandler (keyboard_handler);
}

void I_ShutdownKeyboard (void)
{
	if (keyboard_inited)
        	keyboard_close ();
}

/*
===============================================================================
= 				Mouse
===============================================================================
*/

int	mx, my;
int	mouse_buttonstate;

void mouse_handler (int buttonstate, int dx, int dy)
{
	mouse_buttonstate = buttonstate;
	mx += dx;
	my += dy;	
}

static int	md;

void I_StartupMouse (void)
{
/*
        if (mouse_init ("/dev/sysmouse", vga_getmousetype(), 1200) != -1)
	       	mouse_inited = true;
    	        
        mouse_seteventhandler (mouse_handler);
*/        
      
	md = open ("/dev/sysmouse", O_RDONLY);
	if (md < 0)
		I_Error ("I_StartupMouse: unable to open device.\n");
	else
		mouse_inited = true;	
		
}


void I_ShutdownMouse (void)
{
/*
	if (mouse_inited)
        	mouse_close ();
*/        	
      
	if (mouse_inited)  	
		close (md);
		
}


/*
=============
= I_GetEvent
=============
*/
void   I_GetEvent (void)
{
	int		r;
	mousestatus_t	ms;
        event_t         ev;
	

        if (keyboard_inited)
        	keyboard_update ();

	if (mouse_inited)
	{
        	r = ioctl (md, MOUSE_GETSTATUS, &ms);
		if (r < 0)
			I_Error ("I_GetEvent: Unable to read data from mouse.\n");

        	ev.type = ev_mouse;

     		ev.data1 =(ms.flags & MOUSE_BUTTON1DOWN)
     			| (ms.flags & MOUSE_BUTTON2DOWN ? 2 : 0)
     			| (ms.flags & MOUSE_BUTTON3DOWN ? 2 : 0) 
     			| (ms.button & MOUSE_BUTTON1DOWN)
     			| (ms.button & MOUSE_BUTTON2DOWN ? 2 : 0)
     			| (ms.button & MOUSE_BUTTON3DOWN ? 2 : 0);	
        	
        	ev.data2 = ms.dx<<3;
        	ev.data3 = -ms.dy<<3;

        	D_PostEvent (&ev);
/*        	
		if (mouse_update ())
			;

		ev.type = ev_mouse;

		ev.data1 = (mouse_buttonstate & MOUSE_LEFTBUTTON)
			|  (mouse_buttonstate & MOUSE_RIGHTBUTTON ? 2 : 0)
			|  (mouse_buttonstate & MOUSE_MIDDLEBUTTON ? 2 : 0);
			
//		ev.data1 = 0;

		ev.data2 = mx;
		ev.data3 = my;
		
		mx=my=0;

		D_PostEvent (&ev);
*/		
        }	

        
}

void I_StartTic (void)
{
    I_GetEvent();
}

void I_UpdateNoBlit (void)	{}


/*
==============================================================================
==============================================================================
*/

int main (int argc, char** argv)
{
	myargc = argc;
	myargv = argv;

	D_DoomMain ();

	return 0;
}	
