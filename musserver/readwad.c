/*************************************************************************
 *  readwad.c
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
#include <string.h>
#include "musserver.h"

extern unsigned long lumpsize[35];	/* size in bytes of data lumps */
extern unsigned long lumppos[35];	/* positions of lumps in wad file */
extern unsigned char *musicdata;
extern struct opl_instr fm_instruments[175];
extern struct sbi_instrument fm_sbi[175];
void cleanup(int status);


#define WADHEADER 8	/* The wad header is 12 bytes, but we only care about
			   the last 4.  Thus we skip the first 8 */

static char *doom2names[] = {
        "D_RUNNIN", "D_STALKS", "D_COUNTD", "D_BETWEE", "D_DOOM", "D_THE_DA",
        "D_SHAWN", "D_DDTBLU", "D_IN_CIT", "D_DEAD", "D_STLKS2", "D_THEDA2",
        "D_DOOM2", "D_DDTBL2", "D_RUNNI2", "D_DEAD2", "D_STLKS3", "D_ROMERO",
        "D_SHAWN2", "D_MESSAG", "D_COUNT2", "D_DDTBL3", "D_AMPIE", "D_THEDA3",
        "D_ADRIAN", "D_MESSG2", "D_ROMER2", "D_TENSE", "D_SHAWN3", "D_OPENIN",
        "D_EVIL", "D_ULTIMA", "D_READ_M", "D_DM2TTL", "D_DM2INT" };

static char *doom1names[] = {
        "D_E1M1", "D_E1M2", "D_E1M3", "D_E1M4", "D_E1M5", "D_E1M6", "D_E1M7",
        "D_E1M8", "D_E1M9", "D_E2M1", "D_E2M2", "D_E2M3", "D_E2M4", "D_E2M5",
        "D_E2M6", "D_E2M7", "D_E2M8", "D_E2M9", "D_E3M1", "D_E3M2", "D_E3M3",
        "D_E3M4", "D_E3M5", "D_E3M6", "D_E3M7", "D_E3M8", "D_E3M9", "D_INTROA",
        "D_INTRO", "D_INTER", "D_VICTOR", "D_BUNNY" };

unsigned long genmidipos;

void readdir(FILE *wadfile, int version)
{

int inmus = 0;		/* flag */
long count = 0;		/* counting variable */
long dirpos;		/* position of directory in wad file */
int lumpindex = 0;	/* index (0 to 34) of current lump */
char lumpname[8];	/* name of current lump */
int musiclump;		/* tells whether current lump contains music */

/* skip first 8 bytes to find and read the wad directory offset */
  fseek(wadfile, WADHEADER, SEEK_SET);
  fread((char*)&dirpos, 4, 1, wadfile);

/* seek to the name of the first wad directory entry */
  fseek(wadfile, dirpos + 8, SEEK_SET);

/* loop to find positions of all the music entries */
  while (!inmus)
    {
    fread(&lumpname, 8, 1, wadfile);
    if (!strncmp(lumpname, "D_", 2))
      {
      musiclump = 1;
        switch (version)
          {
          case 0: case 1:
            for (lumpindex = 0; lumpindex <= 31; lumpindex++)
              if (!strncmp(lumpname, doom1names[lumpindex], strlen(doom1names[lumpindex])))
                break;
            if (lumpindex == 32)
              {
              printf("musserver: unrecognized music entry (%s)\n", lumpname);
              musiclump = 0;
              }
            break;

          case 2:
            for (; lumpindex <= 34; lumpindex++)
              if (!strncmp(lumpname, doom2names[lumpindex], strlen(doom2names[lumpindex])))
                break;
            if (lumpindex >= 35)
              {
              printf("musserver: unrecognized music entry (%s)\n", lumpname);
              musiclump = 0;
              }
            break;
          }

        if (musiclump)
          {
          fseek(wadfile, -16, SEEK_CUR);
          fread((char*)&lumppos[lumpindex], 4, 1, wadfile);
          fread((char*)&lumpsize[lumpindex], 4, 1, wadfile);
          fseek(wadfile, 16, SEEK_CUR);
          lumpindex++;
          }
      }
    else if (!strncmp(lumpname, "GENMIDI", 7))
      {
      fseek(wadfile, -16, SEEK_CUR);
      fread((char*)&genmidipos, 4, 1, wadfile);
      fseek(wadfile, 20, SEEK_CUR);
      }
    else
      if (!strcmp(lumpname, "F_END"))
        {
        break;
        }
    count++;
    if (feof(wadfile))
      inmus = 1;
    }
}


int readmus(FILE *wadfile, int lumpnum)
{
struct mus_header {			/* header of music lump */
	char		id[4];
	unsigned short	music_size;
	unsigned short	header_size;
	unsigned short	channels;
	unsigned short	sec_channels;
	unsigned short	instrnum;
	unsigned short	dummy;
} header;

  fseek(wadfile, lumppos[lumpnum], SEEK_SET);
  fread(&header, 1, sizeof(header), wadfile);

  fseek(wadfile, lumppos[lumpnum], SEEK_SET);

/* skip instrument data for now */
  fseek(wadfile, header.header_size, SEEK_CUR);

  musicdata = malloc(header.music_size);
  if (musicdata == NULL)
    {
    printf("musserver: could not allocate %d bytes for music data, exiting.\n", header.music_size);
    cleanup(1);
    }
  fread(musicdata, 1, header.music_size, wadfile);
  return header.music_size;
}

void read_genmidi(FILE *wadfile)
{
char header[9];

  fseek(wadfile, genmidipos, SEEK_SET);
  fread(&header, 1, 8, wadfile);
  if (strncmp(header, "#OPL_II#", 8))
    {
    printf("musserver: couldn't find GENMIDI entry in wadfile, exiting.\n");
    cleanup(1);
    }
  fread(&fm_instruments, sizeof(struct opl_instr), 175, wadfile);
}
