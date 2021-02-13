/*************************************************************************
 *  musserver.h
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

#include <sys/soundcard.h>

struct mus_header {			/* header of music lump */
	char		id[4];
	unsigned short	music_size;
	unsigned short	header_size;
	unsigned short	channels;
	unsigned short	sec_channels;
	unsigned short	instrnum;
	unsigned short	dummy;
}; 

struct opl_instr {
	unsigned short   flags;
	unsigned char    finetune;
	unsigned char    note;
	sbi_instr_data   patchdata;
};

struct fm_voice {
	signed int    note;
	signed int    channel;
};

#define FM_SYNTH 1
#define EXT_MIDI 2
#define LIST_DEV -1
#define TERMINATED 4
#define MSG_WAIT 0
