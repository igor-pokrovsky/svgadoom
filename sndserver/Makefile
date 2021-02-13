##########################################################
#
# $Id:$
#
# $Log:$
#
#

CC=	gcc
CFLAGS=	-s -O3 -mpentium -I/usr/lib/oss --fast-math --unroll-loops --omit-frame-pointer --expensive-optimizations
LIBS=	-lm

O=	Unix

all:	 $(O)/sndserver

clean:
	rm -f $(O)/*.o 
	rm -f $(O)/sndserver

# Target
$(O)/sndserver: \
	$(O)/soundsrv.o \
	$(O)/sounds.o \
	$(O)/wadread.o \
	$(O)/linux.o
	$(CC) $(CFLAGS) $(LDFLAGS) \
	$(O)/soundsrv.o \
	$(O)/sounds.o \
	$(O)/wadread.o \
	$(O)/linux.o -o $(O)/sndserver $(LIBS)
	echo make complete.

# Rule
$(O)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


