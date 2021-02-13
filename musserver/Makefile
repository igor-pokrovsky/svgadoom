#
# Makefile for musserver 0.9
#

# Change these to fit your system
CC     = gcc

# Uncomment this line for extra debugging output
#CFLAGS = -g -DDEBUG -I. -Wall -O2 -m486
CFLAGS = -g -I/usr/lib/oss/include -Wall -O2 -mpentiumpro
#CFLAGS = -I. -Wall
LIBS   = -s

INSTALL = install
DESTDIR = /usr/local/bin
SHELL = /bin/sh

#############################################
# Nothing below this line should be changed #
#############################################

OBJS = musserver.o readwad.o playmus.o sequencer.o

musserver: $(OBJS)
	   $(CC) $(LIBS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS) ipc.o ipc musserver

all: musserver

install: all
	$(INSTALL) musserver $(DESTDIR)

#ipc:	ipc.o
#	$(CC) -o $@ ipc.o
