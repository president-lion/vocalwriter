/*C_HEADER_FILE****************************************************************
FILE			:	XVocabLib.c
DESC			:	Functions needed to build a binary recognizer for the XBox
TABS			:	3
OWNER			:	Fonix
DATE CREATED:	19 April 2002

(C) Copyright 2001 All rights reserved.
This is an unpublished work, and is confidential and proprietary: 
technology and information of fonix corporation.  No part of this
code may be reproduced, used or disclosed without written consent of 
fonix corporation in each and every instance.

  $Date: 7/30/03 6:15p $
  $Revision: 8 $

*END_HEADER*******************************************************************/

#include <stdio.h>
#include <string.h>
#include "FnxPlatform.h"
#include "FnxMisc.h"
#include "language.h"
#include "FnxByteOrder.h"
//#include "KoreanIntonation.h"

#define MAX_PHRASE_LENGTH 2048 // Maximum length a phrase for recognition is allowed to be.

/* In Unicode 2.0, Hanguel syllables go from 0xAC00-0xD7A3. 
   Each value represents one syllable, which consists of
   an initial consonant, a vowel, and possibly a final consonant.*/
#define HANGEUL_BASE 0xAC00 // Beginning of Hangeul characters
#define HANGEUL_END  0xD7A3 // Last Hangeul character

#define NUM_HANGEUL_INITIAL 19
#define NUM_HANGEUL_VOWEL 21
#define NUM_HANGEUL_FINAL 28


/* In Unicode 2.0, Hanguel syllables go from 0xAC00-0xD7A3. 
   Each value represents one syllable, which consists of
   an initial consonant, a vowel, and possibly a final consonant.
   The possible initial consonants, vowels and final consonants
   are defined in the arrays psInitial[], psVowel[] and psFinal[],
   respectively.  The syllables are arranged such that all
   combinations of these are possible. */
static char *psInitial[NUM_HANGEUL_INITIAL] 
  = {"g", "kk", "n", "d", "tt", "r", "m", "b", "pp", 
     "s", "ss", "-", "j", "jj", "ch", "k", "t", "p", "h"};
static char *psVowel[NUM_HANGEUL_VOWEL]
  = {"a", "ae", "ya", "yae", "eo", "e", "yeo", "ye", "o",
     "wa", "wae", "oe", "yo", "u", "wo", "we", "wi", "yu",
     "eu", "ui", "i"};
static char *psFinal[NUM_HANGEUL_FINAL]
  = {"", "g", "kk", "gs", "n", "nj", "nh", "d", "l", "lg",
     "lm", "lb", "ls", "lt", "lp", "lh", "m", "b", "bs",
     "s", "ss", "ng", "j", "ch", "k", "t", "p", "h"};

/* Compare two SWORDS pointed to by void pointers */
int CmpSWORD(const void *a, const void *b)
{
   SWORD *pwA = (SWORD *)a;
   SWORD *pwB = (SWORD *)b;
   if (*pwA < *pwB)
      return -1;
   if (*pwA > *pwB)
      return 1;
   return 0;
}

#define NUM_AMBIGUOUS_HANGEUL 27

 /* Following is a list of {final<<8+initial} indices of consonants
    which are ambiguous and thus need a '-' between them.  This
    list must be sorted by value so that a bsearch() is possible. */
static short pwAmbiguous[NUM_AMBIGUOUS_HANGEUL]=
   {0x0001 /* -kk*/, 0x0004 /* -tt*/, 0x0008 /* -pp*/, 0x000a /* -ss*/, 
    0x000d /* -jj*/, 0x010a /*g-ss*/, 0x020f /*kk-k*/, 0x0309 /*gs-s*/,
    0x0400 /* n-g (not truly ambiguous, but it's misleading to see "ng" together when n-g is meant)*/,
    0x040d /*n-jj*/, 0x050c /*nj-j*/, 0x0804 /*l-tt*/, 0x0808 /*l-pp*/, 
    0x080a /*l-ss*/, 0x0c09 /*ls-s*/, 0x0d10 /*lt-t*/, 0x0e11 /*lp-p*/, 
    0x110a /*b-ss*/, 0x1209 /*bs-s*/, 0x1309 /*s-s*/,  0x130a /*s-ss*/, 
    0x1409 /*ss-s*/, 0x160c /*j-j*/,  0x1801 /*k-kk*/, 0x180f /*k-k*/, 
    0x1910 /*t-t*/,  0x1a11 /*p-p*/};


/*FUNCTION_HEADER**********************
 * NAME:    ;LongestMatch
 * DESC:    Find the longest string in a list of strings that match the beginning of a given string.
 * IN:      s - String to try to find a match in.
            psStringList - Array of strings to try to match to the beginning of 's'.
            iNumStrings - Number of strings in psStringList[].
 * OUT:     n/a
 * RETURN:  Index of the longest string in psStringList that 's' begins with.
            Returns a -1 if no match is found.
 * NOTES:   For example, if s="eon-o" and the list contains {"a", "eo", "e", "o", and "yeo"},
               then elements [1] ("eo") and [2] ("e") would both match the beginning
               of the string, but [1] is longer, and thus its index would be returned.
 *END_HEADER***************************/
int LongestMatch(char *s, char **psStringList, int iNumStrings)
{
   int i;
   int iLen, iBestLen=-1;
   int iBestIndex = -1; /* Haven't found a match yet */
   char *pc1, *pc2;

   for (i=0; i<iNumStrings; i++)
   {
      iLen=0;
      for (pc1=s, pc2=psStringList[i]; *pc1 && *pc2 && *pc1==*pc2; pc1++, pc2++)
         iLen++;
      if (*pc2==0) /* Reached the end of psStringList[i], indicating that it matched the beginning of s */
      {
         if (iLen > iBestLen)
         {
            iBestLen = iLen;
            iBestIndex = i;
         }
      }
   }  /* for i */
   return iBestIndex;
}  /* LongestMatch */


/*FUNCTION_HEADER**********************
 * NAME:    ;HangeulToRoman
 * DESC:    Convert a Unicode Hangeul string to a romanized narrow-character string.
 * IN:      wsHangeul - A wide-character string of Unicode 2.0 Hangeul characters.
            sAscii    - Buffer to put romanized characters into (NULL=>calculate size and allocate it).
 * OUT:     An ASCII string is allocated if necessary, and then 
               filled with a romanized version of the string.
 * RETURN:  Size of string to pass in, or 0 on error.
 * NOTES:   Any unicode characters in the range 0x0000-0x00FF (i.e., 0-255)
              are copied over directly as ASCII characters.
            Unicode characters not in the ASCII (0-255) or Hangeul (0xAC00-0xD7A3) ranges
              cause a '?' to be output.
            The resulting romanized string can be converted back to Hangeul Unicode
              by calling RomanToHangeul().
 *END_HEADER***************************/
int HangeulToRoman(wchar_t *wsHangeul, char *sAscii)
{
   int iOffset;  /* Index into array of Unicode Hangeul characters */
   int iInitial; /* Index into psInitial[] array */
   int iVowel;   /* Index into psVowel[] array */
   int iFinal;   /* Index into psFinal[] array */
   int iLength;  /* Total number of characters for entire array */
   int iSyllablesPerInitial = NUM_HANGEUL_VOWEL*NUM_HANGEUL_FINAL;
   wchar_t *ws;
   BOOL bNewWord=TRUE; /* Flag for whether the next symbol begins a new word */
   char *sRoman, *pc;
   int iPrevFinal; /* Index in psFinal[] of the final consonant of the previous syllable */
   SWORD wPair;    /* Temporarily value consisting of (iFinalConsonant<<8) + iInitialConsonant */
   
   if (wsHangeul==NULL)
      return 0;

   if (sAscii==NULL)
   {
	   /* Determine the length of the array to allocate */
	   iLength = 0;
	   for (ws = wsHangeul; *ws; ws++)
	   {
		   if (*ws < HANGEUL_BASE || *ws > HANGEUL_END)
		   {
			   iLength++; // Use an ASCII symbol or '?' for any non-Hangeul characters
			   bNewWord=TRUE;
		   }
		   else
		   {
			   /* Get the offset of the hangul character in the Hangeul section of the Unicode space. */
			   iOffset = (int)*ws - HANGEUL_BASE;
			   
			   /* Get the offset of the three parts of the syllable */
			   iInitial = iOffset / iSyllablesPerInitial;
			   iVowel	= (iOffset%iSyllablesPerInitial) / 28;
			   iFinal	= iOffset%28;
			   
			   // Insert a syllable break
			   if( !bNewWord )
			   {
				   iLength++;
			   }

			   /* The initial symbol should only be a '-' if this is the beginning of a new word */
			   if ((bNewWord && iInitial!=11) || (!bNewWord && iInitial==11))
			   {
				   iLength += strlen(psInitial[iInitial]);
			   }
			   else if (!bNewWord) // i.e., skip an initial 11 of a new word
			   {
				   wPair = (((SWORD)iPrevFinal) << 8) + (SWORD)iInitial;
				   if (bsearch(&wPair, pwAmbiguous, NUM_AMBIGUOUS_HANGEUL, sizeof(SWORD), CmpSWORD))
					   iLength++; // Add one for '-'
				   iLength += strlen(psInitial[iInitial]);
			   }
			   // ...(else this is a new word with an initial silent consonant (#11), so add nothing).
			   iLength += strlen(psVowel[iVowel]) + strlen(psFinal[iFinal]);
			   iPrevFinal = iFinal;
			   bNewWord=FALSE;
		   }
	   }  /* for ws */
	   return iLength+1;
	   
   }
   else 
	   sRoman = sAscii;

   bNewWord=TRUE;
   for (pc=sRoman, ws = wsHangeul; *ws; ws++)
   {
	   if (*ws < HANGEUL_BASE || *ws > HANGEUL_END)
	   {
		   if (*ws<255)
			   *pc++ = (char)*ws;	// Copy any ASCII characters straight across (e.g., digits, punctuation, etc.)
		   else 
			   *pc++ = '?'; 	// Use a '?' for any unknown Unicode characters. 
		   bNewWord=TRUE;
	   }
	   else // Hangeul
	   {
		   /* Get the offset of the hangul character in the Hangeul section of the Unicode space. */
		   iOffset = (int)*ws - HANGEUL_BASE;
		   
		   /* Get the offset of the three parts of the syllable */
		   iInitial = iOffset / iSyllablesPerInitial;
		   iVowel	= (iOffset%iSyllablesPerInitial) / NUM_HANGEUL_FINAL;
		   iFinal	= iOffset%NUM_HANGEUL_FINAL;
		   assert(iInitial>=0 && iInitial < NUM_HANGEUL_INITIAL && 
			   iVowel>=0 && iVowel< NUM_HANGEUL_VOWEL && 
			   iFinal>=0 && iFinal < NUM_HANGEUL_FINAL);
		   
		   // Insert a syllable break
		   if( !bNewWord )
		   {
			   pc = StrCopyCat(pc, SYLLABLE_MARKER_STR);
		   }
		   
		   /* If there is no initial consonant (i.e., #11), and this is not
			  the beginning of a new word, then a '-' is needed.  For example, 
			  "hage" could be "ha-ge" or "hag-e", in the latter case, the initial 
			  silent consonant is represented by a '-'.  We never need a '-' at
			  the beginning of a word, though. */
		   if ((bNewWord && iInitial!=11) || (!bNewWord && iInitial==11))
		   {
			   pc = StrCopyCat(pc, psInitial[iInitial]);
		   }
		   else if (!bNewWord) // i.e., skip an initial 11 of a new word
		   {
			   /* This is not the beginning of a word, and the initial consonant
			   is not the silent one, so we must check for ambiguities that
			   can arise with double consonants.  For example, "mokki" could 
			   be "mok-ki" or "mo-kki" [but not "mokk-i", since the initial 
			   silent consonant would have been taken care of in the "if" 
			   statement above].*/
			   /* Do a binary search into the {final,initial} pairs stored in
			   pwAmbiguous[] to see if the current pair is in there.  The table
			   consists of final<<8+initial, so make such a value that we can use. */
			   SWORD wPair = (((SWORD)iPrevFinal) << 8) + (SWORD)iInitial;
			   if (bsearch(&wPair, pwAmbiguous, NUM_AMBIGUOUS_HANGEUL, sizeof(SWORD), CmpSWORD))
				   *pc++ = '-'; /* Insert a '-' between ambiguous final/initial consonant pairs */
			   pc = StrCopyCat(pc, psInitial[iInitial]);
		   }
		   // ...(else this is a new word with an initial silent consonant (#11), so add nothing).
		   
		   /* Copy the vowel */
		   pc = StrCopyCat(pc, psVowel[iVowel]);
		   
		   /* Copy the final consonant (which may be the empty string) */
		   iPrevFinal = iFinal; /* Remember the previous consonant ID so as to check for ambiguity. */
		   pc = StrCopyCat(pc, psFinal[iFinal]);
		   bNewWord=FALSE;
	   }
   }

   return strlen(sRoman);

}  /* HangeulToRoman */
 

/*FUNCTION_HEADER**********************
 * NAME:	;ConvertToAscii
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
int ConvertToAscii(LPVOID phTTS, wchar_t *wstr, char *szbuf, BOOL bUnicode)
{
	return HangeulToRoman(wstr, szbuf);
}


/*FUNCTION_HEADER*******************
 * NAME:	  ;CheckUnicodeHeader
 * DESC:	  Read the first two or three bytes of a Unicode file to see what kind it is.
 * IN:     fp - File to check.
           pbLittleEndian - Address in which to return a flag indicating whether the unicode
             file is little-endian or not.
 * OUT:    *pbLittleEndian is modified (IF the file is unicode).
           The file is advanced beyond the 2- or 3-byte header, if there is one,
           or is rewound to the beginning, if there is not.
 * RETURN: TRUE if the file is unicode, FALSE otherwise.
 * NOTES:  File should be opened and positioned at the very beginning.
 *END_HEADER***************************/
BOOL CheckUnicodeHeader(FILE *fp, BOOL *pbLittleEndian)
{
   BOOL bUnicode=FALSE;
   unsigned char sBuf[4];

   /* Unicode files usually begin with "FF FE" (little-endian), or "FE FF" (big-endian).
      So check to see if we have one of those cases. */
   fread(sBuf, 2, 1, fp);
   if (sBuf[0]==0xFF && sBuf[1]==0xFE)
   {
	   bUnicode = TRUE;
	   *pbLittleEndian=TRUE;
   }
   else if (sBuf[0]==0xFE && sBuf[1]==0xFF)
   {
	   bUnicode = TRUE;
	   *pbLittleEndian=FALSE;
   }
   else
   {
      /* Compressed unicode files begin with a byte before the FFFE/FEFF bytes
         that inidcate what kind of compression is used.  We'll ignore most kinds,
         but two standard kinds we can handle:
           0x0E => This is a regular (uncompressed, UTF-16) unicode file.
           0x0F => This is a UTF-8 unicode file, i.e., it uses 1 byte per character, 
                   probably in ASCII, so just treat it as though it was not unicode.
         See http://www.unicode.org/unicode/reports/tr6/index.html#Signature
      */
      fread(&sBuf[2], 1, 1, fp);
      if (sBuf[1]==0xFF && sBuf[2]==0xFE)
         *pbLittleEndian=TRUE;
      else if (sBuf[1]==0xFE && sBuf[2]==0xFF)
         *pbLittleEndian=FALSE;
      else fseek(fp, 0, SEEK_SET);
      bUnicode=FALSE;
   }
   return bUnicode;
}  /* CheckUnicodeHeader */


/*FUNCTION_HEADER*******************
 * NAME:	  ;ReadWordList
 * DESC:	  Read a file containing lines of the form "<ID#><tab><word or phrase>\n"
 * IN:     sWordList  - Filename of word list file containing lines with 
                          "<ID#><tab><word or phrase>\n" (Ignore any lines beginning with non-numeric characters).
           ppsWords   - Address in which to return a pointer to an array of 
                          strings with the words (or phrases).
           ppiWordIDs - Address in which to return a pointer to an array of 
                          ID#s associated with each element in (*ppsPhrases)[].
 * OUT:    *ppsPhrases and *ppiPhraseID are modified.
 * RETURN: Number of words or phrases read (or -1 on error).
 * NOTES:
 *END_HEADER***************************/
int FnxReadWordList(char *wsWordFile, char *wsWordConvertionFile)
{
   FILE *fp, *fpWrite;                     /* Word list file */
   wchar_t wsBuf[MAX_PHRASE_LENGTH*sizeof(wchar_t)] = {0}; /* Character array to read each line into */
   unsigned char sBuf[MAX_PHRASE_LENGTH] = {'\0'}; /* Character array to read each line into */
   unsigned char sBufRomanji[MAX_PHRASE_LENGTH] = {'\0'}; /* Character array to read each line into */
   int  iNumWords=0;        /* Number of words in pWordList and pIDList */
   SDWORD iNumber = 0;
   BOOL bUnicode;           /* Flag for whether the text file is unicode */
   BOOL bLittleEndian;      /* Flag for whether the unicode file is little-endian */
   BOOL bGrammar = FALSE;

   /* Open the word list file */
   fp = fopen(wsWordFile, "rb");
   if (fp==NULL)
   {
//      Message(NULL,"Error: Could not open word list file '%S'\n", wsWordList);
      return -1;
   }

   fpWrite = fopen(wsWordConvertionFile, "w");

   bUnicode = CheckUnicodeHeader(fp, &bLittleEndian);

   bUnicode = TRUE;
   while (bUnicode ? (int)fgetws(wsBuf, MAX_PHRASE_LENGTH, fp) : (int)fgets(sBuf, MAX_PHRASE_LENGTH, fp))
   {
	   if (bUnicode)
      {
#ifdef WORDS_LITTLEENDIAN
         if (!bLittleEndian)
#else
         if (bLittleEndian)
#endif
         {  /* Swap the bytes if different from the host */
            BlockByteSwap16(wsBuf, wcslen(wsBuf));
         }
         ConvertToAscii(NULL, wsBuf, sBufRomanji, bUnicode);
      }
		else
		{
//			int strLength;
//			wchar_t * wTemp;
			// We need to check each line to see if it is Unicode or not.
//			if (sBuf[0] < 0x80)
//			{

//				strLength = strlen(sBuf);
//				memset(wsBuf, 0, MAX_PHRASE_LENGTH*sizeof(wchar_t));
				// We need to convert the sentence to wide for the convertion to romongi.
//				mbstowcs( wsBuf,  sBuf, strLength );
//				wTemp = wsBuf;
//			}
//			else
//			{
//				wTemp = (wchar_t *)sBuf;
//				BlockByteSwap16(wTemp, wcslen(wTemp));
//			}
			// sBuf will be over written.
			ConvertToAscii(NULL, (wchar_t*)sBuf, sBufRomanji, FALSE);
		}

	  if (sBufRomanji[0]==0)
	  {
//		   printf("\n");
         fwrite("\n", sizeof(char), 1, fpWrite);
		   continue; /* Skip blank lines */
	  }

//     printf("Romongi String = %s\n", sBufRomanji);
     fwrite(sBufRomanji, sizeof(char), strlen(sBufRomanji), fpWrite);

	  memset( sBufRomanji, 0, MAX_PHRASE_LENGTH);
	  sBuf[0] = 0;
	  wsBuf[0] = 0;
  
	}  /* while not at end of file */

   /* Close the word list file */
   fclose(fpWrite);
   fclose(fp);
   return 0;
}  /* ReadWordList */

