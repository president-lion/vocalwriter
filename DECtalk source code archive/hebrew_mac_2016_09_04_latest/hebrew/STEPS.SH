#!/bin/sh
# Matthew Schnee
# 04/15/1998
# this file is for making the necessary changes to the files for them to work in the build
#
# 001 MGS 04/15/1998  added code for the new html files
# 002 MGS 05/21/1998  added code for new ph files
# 003 ETT 11/24/1998  added chmod for autoconf build process.
# 004 MGS 02/01/1999  Added chmod for configure  
# 005 MGS 04/17/2001  Changes for the 4.61 documentation
#

unzip -L -a dapi.zip

rm -rf dapi/src/plat

unzip -a other.zip

DAPI_API=dapi/src
DAPI_DIR=dapicore
CMD_DIR=$DAPI_DIR/cmd
DIC_DIR=dic
INC_DIR=$DAPI_DIR/include
LTS_DIR=$DAPI_DIR/lts
PH_DIR=$DAPI_DIR/ph

NT_DIR=$DAPI_DIR/nt
OSF_DIR=$DAPI_DIR/osf

VTM_DIR=$DAPI_DIR/vtm

cd $DAPI_DIR

mv CMD cmd
mv INCLUDE include
mv KERNEL kernel
mv LTS lts
mv NT nt
mv PH ph
mv VTM vtm

#cd api
#mv CRYPT2.C crypt2.c
#mv DECSTD97.C decstd97.c
#mv TTSAPI.H ttsapi.h
#mv Tts.h tts.h

cd ../cmd
mv C_US_CDE.H c_us_cde.h 
mv Cmd_init.c cmd_init.c
mv PAR_AMBI.TAB par_ambi.tab
mv PAR_RULE.H par_rule.h 
mv PAR_TEST.H par_test.h 
mv Par_test.par par_test.par 


cd ../include
mv IMPLODE.H implode.h 
mv LOADER.H loader.h 
mv PDROM.H pdrom.h 
mv VERSION.ENG version.eng 
mv VERSION.H version.h 
mv VMS_PORT.H vms_port.h 

cd ../kernel
mv EXPRESS.C express.c    
mv LOADHELP.ASM loadhelp.asm 
mv Usa_init.c usa_init.c

cd ../lts
mv LSA_ADJU.C lsa_adju.c

cd ../ph
mv P_UK_SR1.C p_uk_sr1.c
mv P_UK_SY1.C p_uk_sy1.c
mv Ph_drwt02.c ph_drwt02.c
mv Ph_inton2.c ph_inton2.c
mv P_gr_vdf1.h p_gr_vdf1.h
mv P_us_vdf1.h p_us_vdf1.h
mv P_fr_vdf1.h p_fr_vdf1.h
mv Ph_fr_inton2.c ph_fr_inton2.c
mv Ph_drwt02it.c ph_drwt02it.c
mv Ph_inton2it.c ph_inton2it.c

cd ../../dic
mv DIC.H dic.h 
mv DIC_UK.ZIP dic_uk.zip
mv DIC_LA.ZIP dic_la.zip 
mv DIC_USS.ZIP dic_uss.zip 
mv USER.TAB user.tab 


cd ../..

find . -name '*.sh' -exec chmod +x {} \;
find . -name 'config.sub' -exec chmod +x {} \;
find . -name 'config.guess' -exec chmod +x {} \;
find . -name 'configure' -exec chmod +x {} \;


