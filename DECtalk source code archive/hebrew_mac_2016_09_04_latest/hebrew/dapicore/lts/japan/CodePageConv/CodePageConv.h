/*H_HEADER_FILE***************************************************************
FILE			: CodePageConv.h
DESC			: 
TABS			: 4
OWNER			: Fonix
DATE CREATED	: 15 Sept 2005 

(C) Copyright 2005 All rights reserved.
This is an unpublished work, and is confidential and proprietary: 
technology and information of fonix corporation.  No part of this
code may be reproduced, used or disclosed without written consent of 
fonix corporation in each and every instance.

  $Date:  $
  $Revision:  $

*END_HEADER******************************************************************/
#ifndef	CODEPAGECONV_H
#define	CODEPAGECONV_H

typedef enum _tagCodeSort
{
	eUtf16=0,
	eSjis,
	eEuc

}CODE_TYPE;

unsigned short Convert(unsigned short code, CODE_TYPE eIn, CODE_TYPE eOut);

#if defined(__cplusplus)
extern "C" {
#endif

extern void u2e(unsigned short *str);		// Unicode to EUC
extern void u2s(unsigned short *str);		// Unicode to Shift-Jis
extern void s2u(unsigned short *str);		// Shift-Jis to Unicode
extern void s2e(unsigned short *str);		// Shift-Jis to EUC
extern void e2u(unsigned short *str);		// EUC to Unicode
extern void e2s(unsigned short *str);		// EUC to Shift-Jis

#if defined(__cplusplus)
}
#endif

#endif
