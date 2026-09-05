# Makefile for dapi/src/osf/ for Dectalk software for x86_64-apple-macos 
#
# 001 ETT 10/22/1998 moved to autoconf system. 
#

#### Start of system configuration section. ####  

# where is the standard include dir? (cross compile critical)
STD_INCLUDE=/usr/include/
# which compiler do we use?
CC=/usr/bin/gcc
# to debug or not to debug?
DEBUG_FLAGS= -g -arch x86_64
# how much optomization do we use?
OPTO_FLAGS= -O2
# how many silly compile warnings do we want?
WARN_FLAGS= -Wall
# what do we define?
DEFINES= -DLTSSIM -D_APPLE_MAC_ -D_MAC_OSX_ -D_APPLE_MAC_64BIT_ -DNO_SESSION -DTTSSIM -DANSI -DBLD_DECTALK_DLL -D$(LANGUAGE) -DACCESS32 -DTYPING_MODE -DNO_INCLUDED_C_FILES -DNEW_MULTI_LANG_ARCH -DMULTIPLE_LANGUAGES_LOADED -DHLSYN -DMACOS_STATIC -dynamic
# include mme stuff?
MME_DIR=.
# add -shared for qnx
LIB_FLAGS=


#### End of system configuration section. #### 

RM=rm -f

API_DIR     = ../../../dapi
CMD_DIR     = ../../../dapicore/cmd
DIC_DIR     = ../../../dic
INCLUDE_DIR = ../../../dapicore/include
KERNEL_DIR  = ../../../dapicore/kernel
LTS_DIR     = ../../../dapicore/lts
NT_DIR      = ../../../shared/include
OSF_DIR     = ../../../shared/include/osf
PH_DIR      = ../../../dapicore/ph
PROTOS_DIR  = ../../../plat/unix/include
VTM_DIR     = ../../../dapicore/vtm
HL_DIR      = ../../../dapicore/hlsyn
DECTALKF_H  = ../../../sdk/include/unix

INCLUDES= -I$(INCLUDE_DIR) -I$(API_DIR) -I$(CMD_DIR) -I$(DIC_DIR) -I$(KERNEL_DIR) \
	  -I$(LTS_DIR) -I$(NT_DIR) -I$(OSF_DIR) -I$(PH_DIR) -I$(DECTALKF_H)\
          -I$(PROTOS_DIR) -I$(VTM_DIR) -I$(MME_DIR) -I$(STD_INCLUDE) 

CFLAGS= $(DEBUG_FLAGS) $(OPTO_FLAGS) $(WARN_FLAGS) $(INCLUDES) $(DEFINES) $(LIB_FLAGS)

OSF_SRC= playstub.c loadable.c dtmmio.c 

OSF_OBJ= $(OUTPUT_DIR)/playstub.o $(OUTPUT_DIR)/dtmmio.o $(OUTPUT_DIR)/loadable.o

all:: $(OSF_OBJ)

$(OUTPUT_DIR)/playstub.o : playstub.c
	$(CC) -c -DDEC $(CFLAGS) -o $@ $? 

$(OUTPUT_DIR)/dtmmio.o:dtmmio.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/loadable.o : loadable.c
	$(CC) -c -DDEC $(CFLAGS) -o $@ $?

clean::
	rm $(OUTPUT_DIR)/*.o
