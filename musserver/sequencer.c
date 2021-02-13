/*************************************************************************
 *  sequencer.c
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
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "musserver.h"


struct synth_info sinfo[5];
struct midi_info minfo[5];
int seqfd;
int mixfd;
int use_midi = 0;
int use_synth = 0;
int seq_dev = -1;
int fmpatches[16];
int chanvol[16];
int volscale = 100;
unsigned int voxdate;
struct fm_voice *fmvoices;
struct opl_instr fm_instruments[175];
struct sbi_instrument fm_sbi[175];
extern int verbose;
void cleanup(int status);

SEQ_DEFINEBUF(2048);

void seqbuf_dump(void)
{
  if (_seqbufptr)
  if (write(seqfd, _seqbuf, _seqbufptr) == -1)
    {
     perror("write /dev/sequencer");
     cleanup(-1);
     }
  _seqbufptr = 0;
}

void fmload(void)
{
int x;

  for (x = 0; x < 175; x++)
    {
    fm_sbi[x].key = FM_PATCH;
    fm_sbi[x].device = seq_dev;
    fm_sbi[x].channel = x;
    fm_sbi[x].operators[0] = fm_instruments[x].patchdata[0];
    fm_sbi[x].operators[1] = fm_instruments[x].patchdata[7];
    fm_sbi[x].operators[2] = fm_instruments[x].patchdata[4] + fm_instruments[x].patchdata[5];
    fm_sbi[x].operators[3] = fm_instruments[x].patchdata[11] + fm_instruments[x].patchdata[12];
    fm_sbi[x].operators[4] = fm_instruments[x].patchdata[1];
    fm_sbi[x].operators[5] = fm_instruments[x].patchdata[8];
    fm_sbi[x].operators[6] = fm_instruments[x].patchdata[2];
    fm_sbi[x].operators[7] = fm_instruments[x].patchdata[9];
    fm_sbi[x].operators[8] = fm_instruments[x].patchdata[3];
    fm_sbi[x].operators[9] = fm_instruments[x].patchdata[10];
    fm_sbi[x].operators[10] = fm_instruments[x].patchdata[6];
    fm_sbi[x].operators[11] = fm_instruments[x].patchdata[16];
    fm_sbi[x].operators[12] = fm_instruments[x].patchdata[23];
    fm_sbi[x].operators[13] = fm_instruments[x].patchdata[20] + fm_instruments[x].patchdata[21];
    fm_sbi[x].operators[14] = fm_instruments[x].patchdata[27] + fm_instruments[x].patchdata[28];
    fm_sbi[x].operators[15] = fm_instruments[x].patchdata[17];
    fm_sbi[x].operators[16] = fm_instruments[x].patchdata[24];
    fm_sbi[x].operators[17] = fm_instruments[x].patchdata[18];
    fm_sbi[x].operators[18] = fm_instruments[x].patchdata[25];
    fm_sbi[x].operators[19] = fm_instruments[x].patchdata[19];
    fm_sbi[x].operators[20] = fm_instruments[x].patchdata[26];
    fm_sbi[x].operators[21] = fm_instruments[x].patchdata[22];
    SEQ_WRPATCH(&fm_sbi[x], sizeof(fm_sbi[x]));
    }
}

void reset_midi(void)
{
unsigned int channel;

  if (use_midi)
    for (channel = 0; channel < 16; channel++)
      {
      /* all notes off */
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE + channel);
      SEQ_MIDIOUT(seq_dev, 0x7b);
      SEQ_MIDIOUT(seq_dev, 0);
      /* reset pitch bender */
      SEQ_MIDIOUT(seq_dev, MIDI_PITCH_BEND + channel);
      SEQ_MIDIOUT(seq_dev, 64 >> 7);
      SEQ_MIDIOUT(seq_dev, 64 & 127);
      /* reset volume to 100 */
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE + channel);
      SEQ_MIDIOUT(seq_dev, CTL_MAIN_VOLUME);
      SEQ_MIDIOUT(seq_dev, volscale);
      chanvol[channel] = 100;
      /* reset pan */
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE + channel);
      SEQ_MIDIOUT(seq_dev, CTL_PAN);
      SEQ_MIDIOUT(seq_dev, 64);

      SEQ_DUMPBUF();
      }
  else
    for (channel = 0; channel < sinfo[seq_dev].nr_voices; channel++)
      {
        SEQ_STOP_NOTE(seq_dev, channel, fmvoices[channel].note, 64);
        SEQ_BENDER_RANGE(seq_dev, channel, 200);
        fmvoices[channel].note = -1;
        fmvoices[channel].channel = -1;
        SEQ_DUMPBUF();
      }
}

void midi_setup(int pref_dev, int num_dev)
{
int numsynths;
int m = 0;
int s = 0;
int have_synth = 1;
int have_midi = 1;
int listonly = 0;
int x;
int y;
FILE *sndstat;
char sndver[100];
char snddate[100];

  if (pref_dev >= 100)
    {
    pref_dev = pref_dev - 100;
    listonly = 1;
    }

  seqfd = open("/dev/sequencer", O_WRONLY, 0);

  if (pref_dev != EXT_MIDI)
    if (ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &numsynths) == -1)
      if (pref_dev == FM_SYNTH)
        {
        printf("musserver: no synth devices found, exiting.\n");
        exit(1);
        }
      else
        have_synth = 0;
    else
      for (s = 0; s < numsynths; s++)
        {
        sinfo[s].device = s;
        ioctl(seqfd, SNDCTL_SYNTH_INFO, &sinfo[s]);
        if (verbose || listonly)
          printf("Found a synth device: type %d (%s)\n", sinfo[s].synth_type, sinfo[s].name);
        if (sinfo[s].synth_type == num_dev)
          seq_dev = s;
        }
  else
    have_synth = 0;

  if (pref_dev != FM_SYNTH)
    if (ioctl(seqfd, SNDCTL_SEQ_NRMIDIS, &numsynths) == -1)
      {
      if (pref_dev == EXT_MIDI)
        {
        printf("musserver: no midi devices found, exiting.\n");
        exit(1);
        }
      else if (have_synth == 0)
        {
        printf("musserver: no synth or midi devices found, exiting.\n");
        exit(1);
        }
      else
        have_midi = 0;
      }
    else
      for (m = 0; m < numsynths; m++)
        {
        minfo[m].device = m;
        ioctl(seqfd, SNDCTL_MIDI_INFO, &minfo[m]);
        if (verbose || listonly)
          printf("Found a midi device: type %d (%s)\n", minfo[m].dev_type, minfo[m].name);
        if (minfo[m].dev_type == num_dev)
          seq_dev = m;
        }
  else
    have_midi = 0;

  if ((num_dev != -1) && (seq_dev == -1))
    {
    printf("musserver: could not find a device of type %d, exiting.\n", num_dev);
    exit(1);
    }

  if (have_midi == 1)
    {
    use_midi = 1;
    if (seq_dev == -1)
      seq_dev = m - 1;
    if (verbose || listonly)
      printf("Using midi device number %d (%s)\n", m, minfo[m - 1].name);
    }
  else
    {
    use_synth = 1;
    if (seq_dev == -1)
      seq_dev = s - 1;

/* this bit of code needs a little more error checking... */
    sndstat = fopen("/dev/sndstat", "r");
    if (sndstat == NULL)
      {
      printf("musserver: could not open /dev/sndstat, exiting.\n");
      exit(1);
      }
    fgets(sndver, 100, sndstat);
    for (x = 0; x < strlen(sndver); x++)
      if (sndver[x] == '-')
        {
        x++;
        for (y = x; y < strlen(sndver); y++)
          if (sndver[y] != ' ')
            snddate[y-x] = sndver[y];
          else
            {
            snddate[y-x] = 0;
            break;
            }
        break;
        }
    voxdate = atoi(snddate);

    if (verbose || listonly)
      printf("Using synth device number %d (%s)\n", s, sinfo[s - 1].name);
    fmvoices = malloc(sinfo[seq_dev].nr_voices * sizeof(struct fm_voice));
    for (x = 0; x < sinfo[seq_dev].nr_voices; x++)
      {
      fmvoices[x].note = -1;
      fmvoices[x].channel = -1;
      }
    for (x = 0; x < 16; x++)
      fmpatches[x] = -1;
    mixfd = open("/dev/mixer", O_WRONLY, 0);
    }
  if (listonly)
    exit(0);
  else
    reset_midi();
}

void cleanup_midi(void)
{
  reset_midi();
  close(seqfd);
  if (use_synth)
    close(mixfd);
}

void note_off(int note, int channel, int volume)
{
int x = 0;

  if (use_midi)
    {
    SEQ_MIDIOUT(seq_dev, MIDI_NOTEOFF + channel);
    SEQ_MIDIOUT(seq_dev, note);
    SEQ_MIDIOUT(seq_dev, volume);
    SEQ_DUMPBUF();
    }
  else
    {
    for (x = 0; x < sinfo[seq_dev].nr_voices; x++)
      if ((fmvoices[x].note == note) && (fmvoices[x].channel == channel))
        break;
    if (x != sinfo[seq_dev].nr_voices)
      {
      fmvoices[x].note = -1;
      fmvoices[x].channel = -1;
      SEQ_STOP_NOTE(seq_dev, x, note, volume);
      SEQ_DUMPBUF();
      }
    }
}

void note_on(int note, int channel, int volume)
{
int x = 0;

  if (use_midi)
    {
    SEQ_MIDIOUT(seq_dev, MIDI_NOTEON + channel);
    SEQ_MIDIOUT(seq_dev, note);
    SEQ_MIDIOUT(seq_dev, volume);
    SEQ_DUMPBUF();
    }
  else
    {
    for (x = 0; x < sinfo[seq_dev].nr_voices; x++)
      if ((fmvoices[x].note == -1) && (fmvoices[x].channel == -1))
        break;
    if (x != sinfo[seq_dev].nr_voices)
      {
      fmvoices[x].note = note;
      fmvoices[x].channel = channel;
      if (channel == 9)		/* drum note */
        {
        SEQ_SET_PATCH(seq_dev, x, note + 93);
        note = fm_instruments[note + 93].note;
        }
      else
        {
        SEQ_SET_PATCH(seq_dev, x, fmpatches[channel]);
        if (voxdate != 950728)
          note = note + 12;
        }
      SEQ_START_NOTE(seq_dev, x, note, volume);
      SEQ_DUMPBUF();
      }
    }
}

void pitch_bend(int channel, signed int value)
{
int x;

  if (use_midi)
    {
    SEQ_MIDIOUT(seq_dev, MIDI_PITCH_BEND + channel);
    SEQ_MIDIOUT(seq_dev, value >> 7);
    SEQ_MIDIOUT(seq_dev, value & 127);
    SEQ_DUMPBUF();
    }
  else
    {
    for (x = 0; x < sinfo[seq_dev].nr_voices; x++)
      if (fmvoices[x].channel == channel)
        {
        SEQ_BENDER_RANGE(seq_dev, x, 200);
        SEQ_BENDER(seq_dev, x, 128*value);
        SEQ_DUMPBUF();
        }
    }
}

void control_change(int controller, int channel, int value)
{
int x;

  if (controller == CTL_MAIN_VOLUME)
    {
    chanvol[channel] = value;
    value = value * volscale / 100;
    }

  if (use_midi)
    {
    SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE + channel);
    SEQ_MIDIOUT(seq_dev, controller);
    SEQ_MIDIOUT(seq_dev, value);
    SEQ_DUMPBUF();
    }
  else
    {
    for (x = 0; x < sinfo[seq_dev].nr_voices; x++)
      if (fmvoices[x].channel == channel)
        if (controller == CTL_MAIN_VOLUME)
/*          SEQ_START_NOTE(seq_dev, x, 255, value); */
          SEQ_MAIN_VOLUME(seq_dev, x, value);
      SEQ_DUMPBUF();
/*        SEQ_CONTROL(seq_dev, x, controller, value); */
    }
}

void patch_change(int patch, int channel)
{
int x;

  if (use_midi)
    {
    SEQ_MIDIOUT(seq_dev, MIDI_PGM_CHANGE + channel);
    SEQ_MIDIOUT(seq_dev, patch);
    SEQ_DUMPBUF();
    }
  else
    for (x = 0; x < sinfo[seq_dev].nr_voices; x++)
      if (((fmvoices[x].channel == -1) && (fmvoices[x].note == -1)) || (fmvoices[x].channel == channel))
        fmpatches[channel] = patch;
}

void midi_wait(float time)
{
  ioctl(seqfd, SNDCTL_SEQ_SYNC);
  SEQ_WAIT_TIME((int) time);
  SEQ_DUMPBUF();
}

void midi_timer(int action)
{
  switch (action)
    {
    case 0:
      SEQ_START_TIMER();
      break;
    case 1:
      SEQ_STOP_TIMER();
      break;
    case 2:
      SEQ_CONTINUE_TIMER();
      break;
    }
}

void vol_change(int volume)
{
int x;
int logscale[16] = {0, 25, 40, 50, 58, 65, 70, 75, 79, 83, 86, 90, 93, 95,
                    98, 100};

  volume = (volume < 0 ? 0 : (volume > 15 ? 15 : volume));
  volscale = logscale[volume];
  if (use_midi)
    {
    for (x = 0; x < 16; x++)
      {
      SEQ_MIDIOUT(seq_dev, MIDI_CTL_CHANGE + x);
      SEQ_MIDIOUT(seq_dev, CTL_MAIN_VOLUME);
      SEQ_MIDIOUT(seq_dev, chanvol[x] * volscale / 100);
      SEQ_DUMPBUF();
      }
    }
  else
    {
    volume = volscale;
    volume |= (volume << 8);
    if (-1 == ioctl(mixfd, SOUND_MIXER_WRITE_SYNTH, &volume))
      perror("volume change");
    }
}
