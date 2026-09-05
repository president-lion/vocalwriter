# Makefile for samplosf/src/dtsamples/ for Dectalk software for x86_64-apple-macos 
#
# 001 ETT 10/22/1998 moved to autoconf system. 
# 002 ETT 12/04/1998 changed dtsample_dtlib to be 
#					 used in all samples as sample_dtlib
# 003 MGS 06/19/2001 Solaris Port BATS#972
#

#### Start of system configuration section. ####  

# where is the standard include dir? (cross compile critical)
STD_INCLUDE=/usr/include/
# which compiler do we use?
CC=/usr/bin/gcc
RANLIB=ranlib
# support for symbolic links?
LN_S=ln -f -s
# to debug or not to debug?
DEBUG_FLAGS= -g -arch x86_64
# how much optomization do we use?
OPTO_FLAGS= -O2
# how many silly compile warnings do we want?
WARN_FLAGS= -Wall
OS_VERSION=intel64-14.1.0
DTSAMPLE_ALL=$(OUTPUT_DIR)/say
DT_LIB=-L../../../../build/$(OS_VERSION)/dectalk -ltts
LIBS= -lpthread -framework CoreAudio -framework AudioToolbox -framework AudioUnit
DEFINES= -DLTSSIM -D_APPLE_MAC_ -D_MAC_OSX_ -D_APPLE_MAC_64BIT_ -DNO_SESSION -DTTSSIM -DANSI -DBLD_DECTALK_DLL -D$(LANGUAGE) -DACCESS32 -DTYPING_MODE -DNO_INCLUDED_C_FILES -DNEW_MULTI_LANG_ARCH -DMULTIPLE_LANGUAGES_LOADED -DHLSYN -DMACOS_STATIC -dynamic
MMELIBS= -lpthread
LINKFLAGS= -ldl -arch x86_64
OS_VERSION=intel64-14.1.0
#### End of system configuration section. #### 

RM=rm -f

INCLUDES= -I$(STD_INCLUDE) -I$(STD_INCLUDE)/dtk -I../../../../dapicore/include -I../../../include/unix

#DEBUGFLAGS= -G3
#OTHER_FLAGS=-threads 
#LFLAGS=-G3 $(DEBUG_FLAGS)

CFLAGS= $(DEBUG_FLAGS) $(OPTO_FLAGS) $(WARN_FLAGS) $(INCLUDES) $(DEFINES)
#LINKFLAGS= -Bdynamic

XLIBS= -lMrm -lXm -lXt -lXext -lX11
#MMELIBS= -lmme -lc_r -lmach -lrt -llmf

#DT_LIB=-lttsmme_$(LANG_CODE)

SAMPLES_SRC=say.c dtmemory.c aclock.c xmsay.c

SAMPLES_OBJ=$(SAMPLES_SRC:.c=.o)

all: $(DTSAMPLE_ALL)
#$(OUTPUT_DIR)/say $(OUTPUT_DIR)/dtmemory \
#     $(OUTPUT_DIR)/aclock $(OUTPUT_DIR)/xmsay $(OUTPUT_DIR)/xmsay.uid 

$(OUTPUT_DIR)/say:say.o
	$(CC) $(LINKFLAGS) -o $@ $? $(DT_LIB) $(LIBS) $(MMELIBS)

$(OUTPUT_DIR)/say_demo_$(LANG_CODE):say.o
	$(CC) $(LINKFLAGS) -o $@ $? -ltts_demo_$(LANG_CODE) $(LIBS) $(MMELIBS)

$(OUTPUT_DIR)/dtmemory:dtmemory.o
	$(CC) $(LINKFLAGS) -o $@ $? $(DT_LIB) $(LIBS) $(MMELIBS)

$(OUTPUT_DIR)/dtmemory_demo_$(LANG_CODE):dtmemory.o
	$(CC) $(LINKFLAGS) -o $@ $? -ltts_demo_$(LANG_CODE) $(LIBS) $(MMELIBS)

$(OUTPUT_DIR)/aclock:aclock.o
	$(CC) $(LINKFLAGS) -o $@ $? $(DT_LIB) $(LIBS) $(MMELIBS)

$(OUTPUT_DIR)/xmsay.uid:xmsay.uil 
	/usr/bin/X11/uil -o $@ $?

$(OUTPUT_DIR)/xmsay:xmsay.o
	$(CC) $(LINKFLAGS) -o $@ $? $(DT_LIB) $(LIBS) $(XLIBS) $(MMELIBS)

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $?

clean:
	$(RM) $(OUTPUT_DIR)/* 
	$(RM) $(SAMPLES_OBJ) *~ core
