TARGET = colditz
OBJS = main.o low-level.o utilities.o getopt.o 

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
FLAGS =
LIBS += -lglut -lGLU -lGL -lpspgu -lm -lc -lpsprtc 
#-lpspvfpu

BUILD_PRX=1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Colditz

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
