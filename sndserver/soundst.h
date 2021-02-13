// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: soundst.h,v 1.3 1997/01/29 22:40:45 b1 Exp $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//
// $Log: soundst.h,v $
// Revision 1.3  1997/01/29 22:40:45  b1
// Reformatting, S (sound) module files.
//
// Revision 1.2  1997/01/21 19:00:07  b1
// First formatting run:
//  using Emacs cc-mode.el indentation for C++ now.
//
// Revision 1.1  1997/01/19 17:22:50  b1
// Initial check in DOOM sources as of Jan. 10th, 1997
//
//
// DESCRIPTION:
//	Sound (utility) related. Hang on.
//	See gensounds.h and gensounds.c for what soundst.h is made of.
//
//-----------------------------------------------------------------------------

#ifndef __SOUNDSTH__
#define __SOUNDSTH__

//
// MusicInfo struct.
//
typedef struct
{
    // up to 6-character name
    char*	name;

    // lump number of music
    int		lumpnum;
    
    // music data
    void*	data;

    // music handle once registered
    int handle;
    
} musicinfo_t;



//
// SoundFX struct.
//
typedef struct sfxinfo_struct	sfxinfo_t;

struct sfxinfo_struct
{
    // up to 6-character name
    char*	name;

    // Sfx singularity (only one at a time)
    int		singularity;

    // Sfx priority
    int		priority;

    // referenced sound if a link
    sfxinfo_t*	link;

    // pitch if a link
    int		pitch;

    // volume if a link
    int		volume;

    // sound data
    void*	data;

    // this is checked every second to see if sound
    // can be thrown out (if 0, then decrement, if -1,
    // then throw out, if > 0, then it's in use)
    int		usefulness;

    // lump number of sfx
    int		lumpnum;		
};

#endif
