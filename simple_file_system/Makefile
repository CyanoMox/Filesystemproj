CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes
LIBS= 
CC=gcc
AR=ar


BINS= simplefs_test\
	lowlevel_test\
	simplefs_editor

OBJS = #add here your object files

HEADERS= disk_driver.h\
	 simplefs.h

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 

so_game: simplefs_test.c $(OBJS) 
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)
