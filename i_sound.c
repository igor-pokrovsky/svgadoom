#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/time.h>
#include <sys/types.h>

/* these one are for musserver only */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <fcntl.h>
#include <unistd.h>

#include "z_zone.h"

#include "i_unix.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"

// Separate sound server process.
FILE*	sndserver=0;
char*	sndserver_filename = "./sndserver";

FILE*	musserver=0; 
char*	musserver_filename = "./musserver";

struct msg_buffer_s {
	long msg_type;
	char msg_text[12];
};

void I_SetChannels()
{
}	

 
void I_SetSfxVolume(int volume)
{
  snd_SfxVolume = volume;
}

// MUSIC API - dummy. Some code from DOS version.
void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  snd_MusicVolume = volume;
  // Now set volume on output device.
  // Whatever( snd_MusciVolume );
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//
int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{
    priority = 0;	

    if (sndserver)
    {
    	vol<<=2;
	fprintf (sndserver, "p%2.2x%2.2x%2.2x%2.2x\n", id, pitch, vol, sep);
	fflush (sndserver);
    }
    // warning: control reaches end of non-void function.
    return id;
}



void I_StopSound (int handle)
{
  handle = 0;
}


int I_SoundIsPlaying(int handle)
{
    // Ouch.
    return gametic < handle;
}


void
I_UpdateSoundParams
( int	handle,
  int	vol,
  int	sep,
  int	pitch)
{
  // UNUSED.
  handle = vol = sep = pitch = 0;
}




void I_ShutdownSound(void)
{    
  if (sndserver)
  {
    // Send a "quit" command.
    fprintf(sndserver, "q\n");
    fflush(sndserver);
  }
}



void
I_InitSound()
{ 
  char buffer[256];

  
  if (getenv("DOOMWADDIR"))
    sprintf(buffer, "%s/%s",
	    getenv("DOOMWADDIR"),
	    sndserver_filename);
  else
    sprintf(buffer, "%s", sndserver_filename);
  
  // start sound process
  if ( !access(buffer, X_OK) )
  {
    strcat(buffer, " -quiet");	

    fprintf (stderr,"Trying to connect to sndserver ");
    sndserver = popen(buffer, "w");
    if (!sndserver)
    	fprintf (stderr, "Failed.\n"); 
    else
    	fprintf (stderr, "Ok.\n");	
  }
  else
    fprintf(stderr, "Could not start sound server [%s]\n", buffer);
}


        int     msg_id;

//
// MUSIC API.
// Still no music done.
// Remains. Dummies.
//
void I_InitMusic(void)		
{ 
	char	buffer[256];
    	struct msg_buffer_s msg_buffer;	
    	

  	if (getenv("DOOMWADDIR"))
    		sprintf(buffer, "%s/%s",
            		getenv("DOOMWADDIR"),
            		musserver_filename);
  	else
    		sprintf(buffer, "%s", musserver_filename);
	

  	if(!access(buffer, X_OK)) {

		msg_id = -1;

    		fprintf (stderr,"Trying to connect to musserver ");  	
    		musserver = popen (buffer, "w");
	    	if (!musserver)
        		fprintf (stderr, "Failed.\n");
    		else
        		fprintf (stderr, "Ok.\n");

    		/* I give all permissions I can... */
    		msg_id = msgget(53075, IPC_CREAT | 0777);
  	}
  	else {
    		fprintf (stderr, "Could not start music server [%s]\n", buffer);
  	}

  if(msg_id != -1) {
    msg_buffer.msg_type = 6;
    memset(msg_buffer.msg_text, 0, 12);
    msg_buffer.msg_text[0] = 'v';
    msg_buffer.msg_text[1] = 100;
    msgsnd(msg_id, (struct msgbuf *) &msg_buffer, 12, IPC_NOWAIT);
  }


  if(msg_id != -1) {
    msg_buffer.msg_type = 6; 
    memset(msg_buffer.msg_text, 0, 12);    
    memcpy (msg_buffer.msg_text, "D_E1M1", 12);
    msgsnd (msg_id, (struct msgbuf *) &msg_buffer, 12, IPC_NOWAIT);
  }
  
}  

void I_ShutdownMusic(void)	
{ 
 if(msg_id != -1) {
   msgctl(msg_id, IPC_RMID, (struct msgid_ds *) NULL);
 }
}

static int	looping=0;
static int	musicdies=-1;

void I_PlaySong(int handle, int looping)
{
  // UNUSED.
  handle = looping = 0;
  musicdies = gametic + TICRATE*30;
}

void I_PauseSong (int handle)
{
  // UNUSED.
  handle = 0;
}

void I_ResumeSong (int handle)
{
  // UNUSED.
  handle = 0;
}

void I_StopSong(int handle)
{
  // UNUSED.
  handle = 0;
  
  looping = 0;
  musicdies = 0;
}

void I_UnRegisterSong(int handle)
{
  // UNUSED.
  handle = 0;
}

int I_RegisterSong(void* data)
{
  // UNUSED.
  data = NULL;
  
  return 1;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
  // UNUSED.
  handle = 0;
  return looping || musicdies > gametic;
}


