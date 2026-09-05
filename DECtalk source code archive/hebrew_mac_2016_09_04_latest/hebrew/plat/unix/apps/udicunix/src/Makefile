# Makefile for samplosf/src/windict/ for Dectalk software for x86_64-apple-macos 
#
# 001 MGS 05/22/1998 added include for dectalkf.h
# 002 ETT 10/22/1998 moved to autoconf system. 
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
#DT_LIB=-L../../../../build/$(OS_VERSION)/dectalk -ltts
LIBS= -lpthread -framework CoreAudio -framework AudioToolbox -framework AudioUnit
DEFINES= -DLTSSIM -D_APPLE_MAC_ -D_MAC_OSX_ -D_APPLE_MAC_64BIT_ -DNO_SESSION -DTTSSIM -DANSI -DBLD_DECTALK_DLL -D$(LANGUAGE) -DACCESS32 -DTYPING_MODE -DNO_INCLUDED_C_FILES -DNEW_MULTI_LANG_ARCH -DMULTIPLE_LANGUAGES_LOADED -DHLSYN -DMACOS_STATIC -dynamic
PLATFORM_DEFS=
MMELIBS= -lpthread
LINKFLAGS= -ldl -arch x86_64
MME_DIR=.
#### End of system configuration section. #### 

RM=rm -f
LD=ld

INCLUDE_DIR=../../dapi/src/include
API_DIR=    ../../dapi/src/api
NT_DIR=     ../../dapi/src/nt
LTS_DIR=    ../../dapi/src/lts
OSF_DIR=    ../../dapi/src/osf
DECTALKF_DIR = ../../
CURRENT_DIR=`pwd`

INCLUDES= -I$(INCLUDE_DIR) -I$(API_DIR) -I$(LTS_DIR) -I$(NT_DIR) -I$(DECTALKF_DIR) -I$(CURRENT_DIR) -I$(MME_DIR) -I$(OSF_DIR)

CDEBUGFLAGS = $(DEBUG_FLAGS) 
LINKFLAGS=  $(DEBUG_FLAGS)

CFLAGS= $(INCLUDES) $(CDEBUGFLAGS) $(WARN_FLAGS) -D$(LANGUAGE) $(DEFINES)

#DT_LIB= -lttsmme

LIBS=

XLIBS= 

all::$(OUTPUT_DIR)/udic_$(LANG_CODE)

UDICT_SRCS = udic_com.c alphabet.c 

UDICT_OBJS= $(OUTPUT_DIR)/link/udic_com.o $(OUTPUT_DIR)/link/alphabet.o 

#all: $(OUTPUT_DIR)/windict $(OUTPUT_DIR)/windict.uid

$(OUTPUT_DIR)/link/udic_com.o:udic_com.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/link/alphabet.o:alphabet.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/udic_$(LANG_CODE):$(UDICT_OBJS)
	$(RM) $@
	$(CC) -o $@ $(UDICT_OBJS) $(LINKFLAGS) $(LIBS)


clean:
	$(RM) $(OUTPUT_DIR)/link/*.o
	$(RM) $(OUTPUT_DIR)/udic_$(LANG_CODE)
	$(RM) $(OUTPUT_DIR)/core
	$(RM) core *~
