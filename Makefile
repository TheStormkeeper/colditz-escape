TARGET = colditz
OBJS = psp/psp-setup.o low-level.o soundplayer.o videoplayer.o md5.o game.o graphics.o eschew/ConvertUTF.o eschew/eschew.o conf.o main.o

INCDIR = 
CFLAGS = -O3 -Wall -Wshadow -Wundef -Wunused -G0 -Xlinker -S -Xlinker -x
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = ./libs
FLAGS =
LIBS += -lglut -lGLU -lGL -lpmp -lexpat -lpspgum -lpspgu -lpsprtc -lm -lc -lpspaudiolib -lpspaudio -lpspaudiocodec -lpspmpeg -lpsppower -lpspvfpu
BUILD_PRX = 1
EXTRA_TARGETS = EBOOT.PBP

PSP_DIR_NAME = Colditz
PSP_EBOOT_SFO = param.sfo
PSP_EBOOT_TITLE = Colditz Escape! PSP v0.9.4
PSP_EBOOT = EBOOT.PBP
PSP_EBOOT_ICON = icon1.png
PSP_EBOOT_ICON1 = NULL
PSP_EBOOT_PIC0 = NULL
PSP_EBOOT_PIC1 = NULL
PSP_EBOOT_SND0 = NULL
PSP_EBOOT_PSAR = NULL
PSP_FW_VERSION = 371

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

