/*H_HEADER_FILE***************************************************************
FILE			: KoreanIntonaton.h
DESC			: 
TABS			: 4
OWNER			: Fonix
DATE CREATED: 14 Sept 2005

(C) Copyright 2005 All rights reserved.
This is an unpublished work, and is confidential and proprietary: 
technology and information of fonix corporation.  No part of this
code may be reproduced, used or disclosed without written consent of 
fonix corporation in each and every instance.

  $Date:  $
  $Revision:  $

*END_HEADER******************************************************************/
#ifndef	KR_INTON_H
#define	KR_INTON_H


// Intonation start flags
#define KR_INTON_SON1		0x0001	// First sonorant
#define KR_INTON_SYL1		0x0002	// First syllabic nucleus (Vowel or Syllabic n)

// Intonation patterns
#define KR_INTON_NEUTRAL	0x0000	// ----
#define KR_INTON_FALL1		0x0001	// vvvv
#define KR_INTON_FALL2		0x0002	// vvv^
#define KR_INTON_FALL3		0x0004	// vv^^
#define KR_INTON_FALL4		0x0008	// v^^^
#define KR_INTON_RISE1		0x0010	// ^^^^
#define KR_INTON_RISE2		0x0020	// ^^^v
#define KR_INTON_RISE3		0x0040	// ^^vv
#define KR_INTON_RISE4		0x0080	// ^vvv

// Syllable/Word position flags
#define KR_POS_FIRST		0x0001	
#define KR_POS_MID			0x0002	
#define KR_POS_LAST			0x0004	

// Phrase type flags
#define KR_PHRS_NEUTRAL		0x0001
#define KR_PHRS_QUESTION	0x0002
#define KR_PHRS_EXCLAMATION	0x0004


typedef struct _tagKrSyllable
{
	int  iIntStart,					// Start location for intonation command
		 iIntLength,				// Length of intonation command (-100% to 300%)
		 iIntTarget,
		 iDuration;
	int	 iPos;						// Syllable position flag
	char *sSyllable;				// Copy of the syllable
	struct _tagKrSyllable *pNext;	// next syllable in the word

}KOREAN_SYLLABLE;

typedef struct _tagKrWord
{
	int  iIntStart,					// Start location for intonation command
		 iIntLength,				// Length of intonation command (-100% to 300%)
		 iIntTarget,
		 iDuration;
	int	 iPos;						// Word position flag
	char *sWord;					// copy of the word
	KOREAN_SYLLABLE *pSyl;			// pointer to individual syllable information
	struct _tagKrWord *pNext;		// next word in the phrase

}KOREAN_WORD;

typedef struct _tagKrPhrase
{
	int	nWords;					// Number of words in phrase
	int iType;					// Phrase type (Ex. KR_PHRS_NEUTRAL, KR_PHRS_QUESTION, KR_PHRS_EXCLAMATION)
	int iInton;					// Phrase intonation flag
	char cEnd;					// End of phrase character
	char *sPhrase;				// copy of the phrase
	KOREAN_WORD *pWords;		// pointer to individual word information
	struct _tagKrPhrase *pNext;	// pointer to the next phrase

}KOREAN_PHRASE;

void KrFreePhrases(KOREAN_PHRASE **ppPhrases);
KOREAN_PHRASE *KrParsePhrase(char *sPhrase);


#endif