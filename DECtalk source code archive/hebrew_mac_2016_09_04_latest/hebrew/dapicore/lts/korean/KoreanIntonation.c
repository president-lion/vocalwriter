/*C_HEADER_FILE****************************************************************
FILE			:	KoreanIntonation.c
DESC			:	Add Intonation information to phrase.
TABS			:	4
OWNER			:	Fonix
DATE CREATED	:	13 Sept 2005	

(C) Copyright 2005 All rights reserved.
This is an unpublished work, and is confidential and proprietary: 
technology and information of fonix corporation.  No part of this
code may be reproduced, used or disclosed without written consent of 
fonix corporation in each and every instance.

  $Date:  $
  $Revision:  $

*END_HEADER*******************************************************************/
#include "FnxStdlib.h"
#include "KoreanIntonation.h"


static char *sQuestionWords[] = {
	"nugu",			// who
	"nuga",			// cumulation (?)
	"mueo",			// what
	"eodi",			// where
	"eoddeohge",	// how
	"eoneu",		// which
	"eoddeon",		// what kind of
	"myeoc",		// some (?)
	""
};

static char *sYesNoWords[] = {
	"yes",
	"no",
	""
};

/*FUNCTION_HEADER**********************
 * NAME:	;KrFreePhrases
 * DESC: 	Free all of the phrase/word/syllable data
 * IN:		ppPhrases - address of the pointer to the head of the list of phrases
 * OUT:		all memory used by ppPhrases in released
 * RETURN:	none
 * NOTES:	
 *END_HEADER***************************/
void KrFreePhrases(KOREAN_PHRASE **ppPhrases)
{
	KOREAN_PHRASE	*pTmpPhrase, *pPhrase;
	KOREAN_WORD		*pTmpWord, *pWord;
	KOREAN_SYLLABLE *pTmpSyl, *pSyl;

	pPhrase = *ppPhrases;
	while( pPhrase )
	{
		pTmpPhrase = pPhrase;
		pPhrase = pPhrase->pNext;

		pWord = pTmpPhrase->pWords;					// Free all words for this phrase
		while( pWord )
		{
			pTmpWord = pWord;
			pWord = pWord->pNext;

			pSyl = pTmpWord->pSyl;					// Free all syllables for this word
			while( pSyl )
			{
				pTmpSyl = pSyl;
				pSyl = pSyl->pNext;
				
				if( pTmpSyl->sSyllable )
					FNX_FREE(pTmpSyl->sSyllable);

				FNX_FREE(pTmpSyl);
				pTmpSyl = NULL;
			}

			if( pTmpWord->sWord )	FNX_FREE(pTmpWord->sWord);
			FNX_FREE(pTmpWord);
			pTmpWord = NULL;
		}

		if( pTmpPhrase->sPhrase )	FNX_FREE(pTmpPhrase->sPhrase);
		FNX_FREE(pTmpPhrase);
		pTmpPhrase = NULL;
	}

	*ppPhrases = NULL;

}	// KrFreePhrases

/*FUNCTION_HEADER**********************
 * NAME:	;KrParsePhrase
 * DESC: 	Parse one or more Korean phrases into words and syllables 
			to make it easier to further analyze.
 * IN:		sPhrase - string containing one or more phrases
 * OUT:		none
 * RETURN:	pointer to KOREAN_PHRASE structure on success, NULL on failure
 * NOTES:	
 *END_HEADER***************************/
KOREAN_PHRASE *KrParsePhrase(char *sPhrase)
{
	char *sTok,
		 *sLocal,
		 *sPhraseSeps = ".,;:?!(){}[]\n\r\0",	// Should these be user configurable?
		 *sWordSeps   = " \n\r\0",
		 *sSylSeps	  = "-\n\r\0";
	int	 iPos, nWords=0;
	KOREAN_PHRASE *pHeadPhrase=NULL, *pPhrase;
	KOREAN_WORD	  *pWord;
	KOREAN_SYLLABLE *pSyl;

	if( sPhrase == NULL || 
		FNX_STRLEN(sPhrase) == 0 )
		return NULL;

	// Get a local copy of sPhrase because we are going to mangle it
	if( (sLocal = FNX_STRDUP(sPhrase)) == NULL )
		return NULL;

	// ------------------------------------------
	// Parse all of the phrases
	// ------------------------------------------
	sTok = FNX_STRTOK(sLocal, sPhraseSeps);
	while( sTok )
	{
		if( pHeadPhrase == NULL )
		{
			pHeadPhrase = pPhrase = (KOREAN_PHRASE *)FNX_CALLOC(1, sizeof(KOREAN_PHRASE));
		}
		else
		{
			pPhrase->pNext = (KOREAN_PHRASE *)FNX_CALLOC(1, sizeof(KOREAN_PHRASE));
			pPhrase = pPhrase->pNext;
		}

		// Get a copy of the this phrase
		pPhrase->sPhrase = FNX_STRDUP(sTok);

		// Get the character that marks the end of this phrase
		iPos = (sTok-sLocal) + FNX_STRLEN(sTok);
		pPhrase->cEnd = sPhrase[iPos];

		switch(pPhrase->cEnd)
		{
		case '?':
			pPhrase->iType |= KR_PHRS_QUESTION;
			break;

		case '!':
			pPhrase->iType |= KR_PHRS_EXCLAMATION;
			break;

		default:
			pPhrase->iType |= KR_PHRS_NEUTRAL;
			break;
		}

		sTok = FNX_STRTOK(NULL, sPhraseSeps);
	}
	FNX_FREE(sLocal);
	
	// ---------------------------------------------
	// Parse the words from each phrase
	// ---------------------------------------------
	for(pPhrase=pHeadPhrase; pPhrase; pPhrase=pPhrase->pNext)
	{
		// Get a local copy of sPhrase because we are going to mangle it
		if( (sLocal = FNX_STRDUP(pPhrase->sPhrase)) == NULL )
			return NULL;

		// Parse all of the phrases
		sTok = FNX_STRTOK(sLocal, sWordSeps);
		while( sTok )
		{
			if( pPhrase->pWords == NULL )
			{
				pPhrase->pWords = pWord = (KOREAN_WORD *)FNX_CALLOC(1, sizeof(KOREAN_WORD));
				pWord->iPos = KR_POS_FIRST;
			}
			else
			{
				pWord->pNext = (KOREAN_WORD *)FNX_CALLOC(1, sizeof(KOREAN_WORD));
				pWord = pWord->pNext;
				pWord->iPos = KR_POS_MID;
			}

			// Get a copy of the this word
			pWord->sWord = FNX_STRDUP(sTok);

			sTok = FNX_STRTOK(NULL, sWordSeps);
		}
		if( pWord )
		{
			if( pWord->iPos == KR_POS_FIRST )
				pWord->iPos |= KR_POS_LAST;
			else
				pWord->iPos = KR_POS_LAST;
		}
		FNX_FREE(sLocal);
	}

	// ---------------------------------------------
	// Parse the syllables from each word
	// ---------------------------------------------
	for(pPhrase=pHeadPhrase; pPhrase; pPhrase=pPhrase->pNext)
	{
		for(pWord=pPhrase->pWords; pWord; pWord=pWord->pNext)
		{

			// Get a local copy of sPhrase because we are going to mangle it
			if( (sLocal = FNX_STRDUP(pWord->sWord)) == NULL )
				return NULL;

			// Parse all of the syllables
			sTok = FNX_STRTOK(sLocal, sSylSeps);
			while( sTok )
			{
				if( pWord->pSyl == NULL )
				{
					pWord->pSyl = pSyl = (KOREAN_SYLLABLE *)FNX_CALLOC(1, sizeof(KOREAN_SYLLABLE));
					pSyl->iPos = KR_POS_FIRST;
				}
				else
				{
					pSyl->pNext = (KOREAN_SYLLABLE *)FNX_CALLOC(1, sizeof(KOREAN_SYLLABLE));
					pSyl = pSyl->pNext;
					pSyl->iPos = KR_POS_MID;
				}

				// Get a copy of the this syllable
				pSyl->sSyllable = FNX_STRDUP(sTok);

				sTok = FNX_STRTOK(NULL, sSylSeps);
			}
			if( pSyl )
			{
				if( pSyl->iPos == KR_POS_FIRST )
					pSyl->iPos |= KR_POS_LAST;
				else
					pSyl->iPos = KR_POS_LAST;
			}
			
			// Recompose the word without syllable boundry markers
			pSyl=pWord->pSyl;
			FNX_STRCPY(sLocal, pSyl->sSyllable);
			pSyl=pSyl->pNext;
			while( pSyl )
			{
				FNX_STRCAT(sLocal, pSyl->sSyllable);
				pSyl=pSyl->pNext;
			}
			FNX_STRCPY(pWord->sWord, sLocal);

			FNX_FREE(sLocal);
		}
	}

	return pHeadPhrase;

}	// KrParsePhrase


/*FUNCTION_HEADER**********************
 * NAME:	;KrApplyIntonation
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
void KrApplyIntonation(char *sText)
{
	KOREAN_PHRASE *pPhrases;
	
	pPhrases = KrParsePhrase(sText);


	// The first syllable of a word has a slightly higher pitch.
	// Put [/] in front of each word with multiple syllables

	// The final syllable of a phrase is longer.
	// How do you elongate a syllable?

	// Question words, when used in a question, will be focused and have a higher pitch.
	// Put ["] in front of each syllable of the question word.

	// Question phrases should have a slightly rising intonation.
	// How do you affect a rising pitch across the entire phrase?

	// Command phrases have a falling intonation pattern.
	// Are there specific words in Korean that can be used to denote a command phrase?

	// Phrases with an exclamation point should have a dramatic tone
	// ["] on the first syllable of the phrase, ['] on the second to last syllable
	// [/]on the last syllable (final syllable has rising pitch (%50) and is held a little longer)


	// The end of a thought group is usually marked by a short pause.
	// Replace double spaces "  " with [:cp 200] ... ", " ... [:cp 0].  
	// The pause should be shorter than a standard comma pause.



	KrFreePhrases(&pPhrases);
}