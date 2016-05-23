# DO NOT FORGET to define BITBOX environment variable 

# allow use of SD card for io:
USE_SDCARD = 1

NAME = roktpong
GAME_C_FILES = main.c nonsimple.c
GAME_H_FILES = nonsimple.h

GAME_C_OPTS += -DVGAMODE_320

# see this file for options
include $(BITBOX)/lib/bitbox.mk


clean::
	rm -rf hello.ppm
