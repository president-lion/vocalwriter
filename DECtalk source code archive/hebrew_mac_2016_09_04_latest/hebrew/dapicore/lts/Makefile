# Makefile for dapi/src/lts/ for Dectalk software for x86_64-apple-macos 
#
# 001 ETT 10/22/1998 moved to autoconf system. 
# 002 NAL 04/07/2000 added gspeak demo info
# 003 MGS 06/19/2001 Solaris Port BATS#972
#

#### Start of system configuration section. ####  

# where is the standard include dir? (cross compile critical)
STD_INCLUDE=/usr/include/
# which compiler do we use?
AR=ar ruvs
LD=ld
CC=/usr/bin/gcc
LN_S=ln -f -s
RANLIB=ranlib
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
SHLIB_SUFFIX=dylib
LANG_DAPI_LD_SHARED=$(CC) -arch x86_64 -dynamic -dynamiclib -dylib 		-o $@ $($(U_LANG_CODE)_OBJ)
LANG_DAPI_LD_STUB=$(CC) -arch x86_64 -dynamic -dynamiclib -dylib 		-o $@ $($(U_LANG_CODE)_STUB_OBJ)

#### End of system configuration section. #### 

RM=rm -f

API_DIR     = ../../dapi
CMD_DIR     = ../cmd
DIC_DIR     = ../../dic
INCLUDE_DIR = ../include
KERNEL_DIR  = ../kernel
LTS_DIR     = ../lts
NT_DIR      = ../../shared/include
OSF_DIR     = ../../shared/include/osf
PH_DIR      = ../ph
PROTOS_DIR  = ../../plat/unix/include
VTM_DIR     = ../vtm
HL_DIR      = ../hlsyn
DECTALKF_H  = ../../sdk/include/unix
LANG_INC    = korean -Ijapan -Ichinese


INCLUDES= -I$(INCLUDE_DIR) -I$(API_DIR) -I$(CMD_DIR) -I$(DIC_DIR) -I$(KERNEL_DIR) \
	  -I$(LTS_DIR) -I$(NT_DIR) -I$(OSF_DIR) -I$(PH_DIR) -I$(DECTALKF_H)\
          -I$(PROTOS_DIR) -I$(VTM_DIR) -I$(MME_DIR) -I$(STD_INCLUDE) -I$(LANG_INC)

CFLAGS= $(DEBUG_FLAGS) $(OPTO_FLAGS) $(WARN_FLAGS) $(INCLUDES) $(DEFINES) $(LIB_FLAGS)

LTS_SRC = lsa_adju.c lsa_coni.c lsa_fr.c   lsa_gr.c   lsa_ir.c   lsa_it.c \
          lsa_ja.c   lsa_rtbi.c lsa_rule.c lsa_sl.c   lsa_sp.c   lsa_task.c \
          lsa_us.c   lsa_util.c lsw_main.c ls_chari.c ls_dict.c  ls_homo.c \
          ls_math.c  ls_proc.c  ls_spel.c  ls_speli.c ls_suff.c  ls_suffi.c \
          loaddict.c

LTS_OBJ = $(OUTPUT_DIR)/lsa_adju.o $(OUTPUT_DIR)/lsa_coni.o \
	  $(OUTPUT_DIR)/lsa_fr.o   $(OUTPUT_DIR)/lsa_gr.o   \
	  $(OUTPUT_DIR)/lsa_ir.o   $(OUTPUT_DIR)/lsa_it.o \
          $(OUTPUT_DIR)/lsa_ja.o   $(OUTPUT_DIR)/lsa_rtbi.o \
	  $(OUTPUT_DIR)/lsa_rule.o $(OUTPUT_DIR)/lsa_sl.o   \
	  $(OUTPUT_DIR)/lsa_sp.o   $(OUTPUT_DIR)/lsa_task.o \
          $(OUTPUT_DIR)/lsa_us.o   $(OUTPUT_DIR)/lsa_util.o \
	  $(OUTPUT_DIR)/lsw_main.o $(OUTPUT_DIR)/ls_chari.o \
	  $(OUTPUT_DIR)/ls_dict.o  $(OUTPUT_DIR)/ls_homo.o \
          $(OUTPUT_DIR)/ls_math.o  $(OUTPUT_DIR)/ls_proc.o  \
	  $(OUTPUT_DIR)/ls_spel.o  $(OUTPUT_DIR)/ls_speli.o \
	  $(OUTPUT_DIR)/ls_suff.o  $(OUTPUT_DIR)/ls_suffi.o \
          $(OUTPUT_DIR)/loaddict.o $(OUTPUT_DIR)/lsw_main_demo.o


COMMON_LTS_SRC = loaddict.c ls_adju.c  ls_dict.c  \
		 ls_homo.c  ls_math.c  ls_rule.c  \
	         ls_rule2.c ls_spel.c  ls_suff.c  \
		 ls_task.c  ls_util.c  lsw_main.c

COMMON_LTS_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/loaddict.o \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_adju.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_dict.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_homo.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_math.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_rule.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_rule2.o \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_spel.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_suff.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_task.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/ls_util.o  \
		 $(OUTPUT_DIR)/$(LANG_CODE)/lsw_main.o

ACNA_SRC = lsa_rta.c  lsa_us.c   lsa_util.c 

ACNA_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lsa_rta.o  \
	   $(OUTPUT_DIR)/$(LANG_CODE)/lsa_us.o   \
	   $(OUTPUT_DIR)/$(LANG_CODE)/lsa_util.o \

US_SRC = l_ac_con.c l_us_ad1.c l_us_con.c l_us_ma1.c l_us_pr1.c \
	 l_us_rta.c l_us_sp1.c l_us_spe.c l_us_suf.c $(COMMON_LTS_SRC) \
	 $(ACNA_SRC)

US_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/l_ac_con.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_ad1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_con.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_ma1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_pr1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_rta.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_sp1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_spe.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_us_suf.o \
	 $(COMMON_LTS_OBJ) $(ACNA_OBJ)

US_STUB_SRC = lts_dummy_us.c

US_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_us.o

HB_SRC = l_hb_ad1.c l_hb_con.c l_hb_ma1.c l_hb_pr1.c l_hb_ru1.c \
	 l_hb_sp1.c $(COMMON_LTS_SRC)

HB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/l_hb_ad1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_hb_con.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_hb_ma1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_hb_pr1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_hb_ru1.o \
	 $(OUTPUT_DIR)/$(LANG_CODE)/l_hb_sp1.o \
	 $(COMMON_LTS_OBJ)

HB_STUB_SRC = lts_dummy_hb.c

HB_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_hb.o

CH_SRC = lts_dummy_ch.c
CH_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_ch.o
CH_STUB_SRC = lts_dummy_ch.c
CH_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_ch.o

FR_SRC = lts_dummy_fr.c
FR_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_fr.o
FR_STUB_SRC = lts_dummy_fr.c
FR_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_fr.o

GR_SRC = lts_dummy_gr.c
GR_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_gr.o
GR_STUB_SRC = lts_dummy_gr.c
GR_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_gr.o

IT_SRC = lts_dummy_it.c
IT_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_it.o
IT_STUB_SRC = lts_dummy_it.c
IT_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_it.o

KR_SRC = lts_dummy_kr.c
KR_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_kr.o
KR_STUB_SRC = lts_dummy_kr.c
KR_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_kr.o

JP_SRC = lts_dummy_jp.c
JP_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_jp.o
JP_STUB_SRC = lts_dummy_jp.c
JP_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_jp.o

LA_SRC = lts_dummy_la.c
LA_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_la.o
LA_STUB_SRC = lts_dummy_la.c
LA_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_la.o

SP_SRC = lts_dummy_sp.c
SP_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_sp.o
SP_STUB_SRC = lts_dummy_sp.c
SP_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_sp.o

UK_SRC = lts_dummy_uk.c
UK_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_uk.o
UK_STUB_SRC = lts_dummy_uk.c
UK_STUB_OBJ = $(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_uk.o



all::  $(OUTPUT_DIR)/libtts_$(LANG_CODE).$(SHLIB_SUFFIX) $(OUTPUT_DIR)/libtts_$(LANG_CODE)_stub.$(SHLIB_SUFFIX) 

otherfiles::$(OUTPUT_DIR)/l_us_cha.o

$(OUTPUT_DIR)/libtts_$(LANG_CODE).$(SHLIB_SUFFIX):$(OUTPUT_DIR)/libtts_$(LANG_CODE).a
	#$(LANG_DAPI_LD_SHARED)

$(OUTPUT_DIR)/libtts_$(LANG_CODE)_stub.$(SHLIB_SUFFIX):$(OUTPUT_DIR)/libtts_$(LANG_CODE)_stub.a 
	#$(LANG_DAPI_LD_STUB)

$(OUTPUT_DIR)/libtts_$(LANG_CODE).a:$($(U_LANG_CODE)_OBJ)
	$(RM) $@
	$(AR) $@ $($(U_LANG_CODE)_OBJ)

$(OUTPUT_DIR)/libtts_$(LANG_CODE)_stub.a:$($(U_LANG_CODE)_STUB_OBJ)
	$(RM) $@
	$(AR) $@ $($(U_LANG_CODE)_STUB_OBJ)


$(OUTPUT_DIR)/l_us_cha.o:l_us_cha.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_ac_con.o:l_ac_con.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_ad1.o:l_us_ad1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_con.o:l_us_con.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_ma1.o:l_us_ma1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_pr1.o:l_us_pr1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_rta.o:l_us_rta.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_sp1.o:l_us_sp1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_spe.o:l_us_spe.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_us_suf.o:l_us_suf.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/loaddict.o:loaddict.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_adju.o:ls_adju.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_dict.o:ls_dict.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_homo.o:ls_homo.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_math.o:ls_math.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_rule.o:ls_rule.c 
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_rule2.o:ls_rule2.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_spel.o:ls_spel.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_suff.o:ls_suff.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_task.o:ls_task.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/ls_util.o:ls_util.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/lsa_rta.o:lsa_rta.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/lsa_us.o:lsa_us.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/lsa_util.o:lsa_util.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/lsw_main.o:lsw_main.c 
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_hb_ad1.o:l_hb_ad1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_hb_con.o:l_hb_con.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_hb_ma1.o:l_hb_ma1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_hb_pr1.o:l_hb_pr1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_hb_ru1.o:l_hb_ru1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/l_hb_sp1.o:l_hb_sp1.c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/lts_dummy_$(LANG_CODE).o:lts_dummy_$(LANG_CODE).c
	$(CC) -c $(CFLAGS) -o $@ $?

$(OUTPUT_DIR)/$(LANG_CODE)/lsw_main_demo.o:lsw_main.c
	$(CC) -c -DDEMO $(CFLAGS) -o $@ $?

clean::
	rm $(OUTPUT_DIR)/*.o
