/*************************************************************************
 *  musserver.c
 *
 *  Copyright (C) 1995 Michael Heasley (mheasley@hmc.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#ifdef __FreeBSD__
 #include <errno.h>
#endif 

#include "musserver.h"

extern int use_synth;
extern int seqfd;
extern int mixfd;
unsigned long lumpsize[35];	/* size in bytes of data lumps */
unsigned long lumppos[35];	/* position of lumps in wad file */
unsigned char *musicdata;
int doomver;
int verbose = 1;
int qid;
int changevol = 1;
FILE *infile;

void readdir(FILE *wadfile, int version);
int readmus(FILE *wadfile, int lumpnum);
int playmus(char *musdata, unsigned int mussize, int play_once);
void midi_setup(int pref_dev, int dev_num);
void read_genmidi(FILE *wadfile);
void fmload(void);
void cleanup_midi(void);


void show_help(void)
{
  printf("Usage: musserver [options]\n\n");
  printf("  -d directory     Look in 'directory' for DOOM wad file\n");
  printf("  -f               Use FM synth device for music playback\n");
  printf("  -h               Print this message and exit\n");
  printf("  -l               List detected music devices and exit\n");
  printf("  -m               Use general midi device for music playback\n");
  printf("  -t number        Timeout after 'number' seconds when getting IPC message\n");
  printf("                   queue id\n");
  printf("  -u number        Use device of type 'number' where 'number' is the type\n");
  printf("                   reported by 'musserver -l'.  Requires -f or -m option.\n");
  printf("  -V               Ignore volume change messages from Doom\n");
  printf("  -v               Verbose\n\n");
}
 
void cleanup(int status)
{
struct msqid_ds *dummy;

  cleanup_midi();
  dummy = malloc(sizeof(struct msqid_ds));
  msgctl(qid, IPC_RMID, dummy);
  free(dummy);
  fclose(infile);
  exit(status);
}

void main(int argc, char **argv)
{
int done = 0;
int lump = -1;
int intro = 0;
int introa = 0;
int result;
char *wadfilename;
char *waddir;
unsigned int musicsize;
int x;
int outtro = 31;
int opt_dev = 0;
int list_dev = 0;
int num_dev = -1;
int playarg;
unsigned int timeout = 1500;


  waddir = getenv("DOOMWADDIR");
  if (waddir == NULL)
    {
    waddir = malloc(1);
    sprintf(waddir,  ".");
    }
  while ((x = getopt(argc, argv, "d:fhlmt:u:Vv")) != -1)
    switch (x)
      {
      case 'd':
        waddir = optarg;
        break;
      case 'f':
        if (!opt_dev)
          opt_dev = FM_SYNTH;
        else
          {
          printf("musserver: you may not specify both the -m and -f options, exiting.\n");
          exit(1);
          }
        break;
      case 'h':
        show_help();
        exit(0);
        break;
      case 'l':
        list_dev = 1;
        break;
      case 'm':
        if (!opt_dev)
          opt_dev = EXT_MIDI;
        else
          {
          printf("musserver: you may not specify both the -m and -f options, exiting.\n");
          exit(1);
          }
        break;
      case 't':
        timeout = 5 * atoi(optarg);
        break;
      case 'u':
        num_dev = atoi(optarg);
        break;
      case 'V':
        changevol = 0;
        break;
      case 'v':
        verbose++;
        break;
      case '?': case ':':
        show_help();
        exit(1);
        break;
      }


  if (list_dev)
    opt_dev = opt_dev + 100;

  if (((opt_dev == 0) || (opt_dev == 100)) && (num_dev != -1))
    {
    printf("musserver: the -u option requires either the -m or the -f option, exiting.\n");
    exit(1);
    }

  midi_setup(opt_dev, num_dev);

  wadfilename = malloc(strlen(waddir) + strlen("/doom1.wad") + 1);
  sprintf(wadfilename, "%s/doom1.wad", waddir);
  infile = fopen(wadfilename, "r");
  if (infile == NULL)
    {
    free(wadfilename);
    wadfilename = malloc(strlen(waddir) + strlen("/doom.wad") + 1);
    sprintf(wadfilename, "%s/doom.wad", waddir);
    infile = fopen(wadfilename, "r");
    if (infile == NULL)
      {
      free(wadfilename);
      wadfilename = malloc(strlen(waddir) + strlen("/doom2.wad") + 1);
      sprintf(wadfilename, "%s/doom2.wad", waddir);
      infile = fopen(wadfilename, "r");
      if (infile == NULL)
        {
        free(wadfilename);
        printf("musserver: game mode indeterminate.\n");
        exit(1);
        }
      else
        doomver = 2;
      }
    else
      doomver = 1;
    }
  else
    doomver = 0;

  if (verbose)
    switch (doomver)
      {
      case 0:
        printf("Playing music for shareware DOOM\n");
        break;
      case 1:
        printf("Playing music for registered DOOM\n");
        break;
      case 2:
        printf("Playing music for DOOM II\n");
        break;
      }

  switch(doomver)
    {
    case 0: case 1:
      intro = 28;
      introa = 27;
      break;
    case 2:
      intro = 33;
      introa = 100;
      outtro = 100;
      break;
    }

/* read the wadfile, get music data */
  readdir(infile, doomver);

  if (use_synth)
    {
    read_genmidi(infile);
    fmload();
    }

  qid = -1;
  x = 0;
  if (verbose)
    printf("getting message queue id...\n");
  while (qid == -1)
    {
    x++;
    qid = msgget((key_t)53075, 0);
    if (verbose > 1)
      printf("qid: %d\n",qid);
    if (x > timeout)
      qid = -2;
    if (qid == -1)
      if (errno == ENOENT)
        usleep(200000);
      else
        qid = -2;
    }
  if (qid == -2)
    {
    printf("musserver: could not get IPC message queue id, exiting.\n");
    cleanup(1);
    }

  printf("musserver: using %s\nmusserver: ready\n", wadfilename);
  free(wadfilename);

/* fork to background */
  if (!verbose)
    if((x = fork()) > 0)
      exit(0);
    else if(x == -1)
      {
      fprintf(stderr,"musserver: unable to fork new process, exiting\n");
      perror("fork");
      fclose(infile);
      exit(1);
      }

  playarg = 2;
  if (verbose)
    printf("Waiting for first message from Doom...\n");
  while (!done)
    {
    qid = msgget((key_t)53075, 0);
    if ((lump == outtro) || (lump == intro) || (lump == introa))
      playarg = 1;
    else if (lump >= 0)
      playarg = 0;
    else
      lump = intro;
    if ((verbose) && (playarg != 2))
      printf("Playing music resource number %d\n", lump + 1);
    musicsize = readmus(infile, lump);
    result = playmus(musicdata, musicsize, playarg);
    free(musicdata);
    switch (result)
      {
      case TERMINATED:
        done = 1;
        if (verbose)
          printf("Terminated\n");
        break;
      default:
        if (result >= 500)
          lump = result - 500;
        else
          {
          done = 1;
          printf("musserver: unknown error in music playing, exiting\n");
          }
        break;
      }
    if (playarg == 2)
      playarg = 1;
    }

  cleanup(0);
}
