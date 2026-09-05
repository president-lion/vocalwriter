#define IN_DECTALK

#ifdef IN_DECTALK
#include "dectalkf.h"
#include "port.h"
#include "ls_def.h"
#include "ls_defs.h"
#include "ls_data.h"
#include  "ph_def.h"

#endif


#include "chin_bachus.h"
#include <string.h>
#include <stdio.h>
#include "chin_bachdata.h"

#ifndef IN_DECTALK
#define userulesdebug
// #define LINUXSYNTHESIS
#define userules_is_main
#endif

#ifdef userulesdebug
#define debug_printf(fmt,args...) printf(fmt,## args)
#else
#ifdef _UNIX_LIKE_
#define debug_printf(fmt,args...)
#else
#define debug_printf(fmt,args)
#endif
#endif

#define MAXRULEBUFLEN 100       
#define MAXRULEALLBUFFERLENGTH (6*MAXRULEBUFLEN)          /* do not change the 6 */



void f0target(LPTTS_HANDLE_T phTTS,int dur, int df0, int delay)
{
#ifndef IN_DECTALK
    CHAR buff[20];

    sprintf(buff, "<%d,%d,%d>",dur,df0,delay);

#else
    short buf[4];
    buf[0]=phTTS->pKernelShareData->reverse_ascky['<']+(3 << PSNEXTRA);
//	buf[0]=(3 << PSNEXTRA);
    buf[1]=dur;
    buf[2]=df0;
    buf[3]=delay;
    ls_util_write_pipe(phTTS->pKernelShareData,&buf[0],4);
//	buf[0]=phTTS->pKernelShareData->reverse_ascky[phoneme]+(0 << PSNEXTRA);
//	ls_util_write_pipe(phTTS->pKernelShareData,&buf[0],1);
#endif

}

void f0glideup(LPTTS_HANDLE_T phTTS, int duration, int delta, int delay)
{
#ifndef IN_DECTALK
    CHAR buff[20];
    sprintf(buff,"/<%d,%d,%d>",len,delta,delay);
#else
    short buf[4];
    buf[0]=phTTS->pKernelShareData->reverse_ascky['/']+(3 << PSNEXTRA);
    buf[1]=duration;
    buf[2]=delta;
    buf[3]=delay;
    ls_util_write_pipe(phTTS->pKernelShareData,&buf[0],4);
#endif
}

void f0glidedwn(LPTTS_HANDLE_T phTTS, int duration, int delta, int delay)
{
#ifndef IN_DECTALK
    CHAR buff[20];
    sprintf(buff,"\\<%d,%d,%d>",len,delta,delay);
#else
    short buf[4];
    buf[0]=phTTS->pKernelShareData->reverse_ascky['\\']+(3 << PSNEXTRA);
    buf[1]=duration;
    buf[2]=delta;
    buf[3]=delay;
    ls_util_write_pipe(phTTS->pKernelShareData,&buf[0],4);
#endif
}


void make_mand_tone(LPTTS_HANDLE_T phTTS,CHAR phoneme, int tone, int len);

WalkCNTXT Walkercntxt;   // not constant.



/* THE JUNK BUFFER SHOULD BE 6 TIMES THIS LENGTH. */


/* 
   subroutine to match a seperator string pattern, containing '-' for 
   seperation to an input word string. copies the '-' to the output
   so that word gets seperated by hyphens
*/



typedef struct _mandarinsyll
{
    char syll[6];        // asky
    int  tone;
    int  toneseq;        // things like 214 or 35
	int  durscale;
}
mandsyll;
  

int tone_to_toneseq(int tone)
{
    int ret;

    switch (tone)
    {
	case 1:  ret = 0x0055; break;
	case 2:  ret = 0x0335; break;
	case 3:  ret = 0x0214; break;
	case 4:  ret = 0x0051; break;
	case 5:  ret = 0x0000; break; 
	default: ret = 0x0000; break;
    }
    return ret;
}


 
int chinese_pinyin2arpa(CHAR *inword, CHAR *outphonemes, CHAR *junk, PLTS_T pLts_t)
{
    int ii;
    
    CHAR  *word, *outword,  *phonoword;
    CHAR  *class_string,  *pattern;

    word = junk;
    outword=junk+MAXRULEBUFLEN;
    phonoword=junk+(2*MAXRULEBUFLEN);
    class_string=junk+(3*MAXRULEBUFLEN);
    pattern = junk+(4*MAXRULEBUFLEN);
    
    //  pLts_t->magic_environ=1;        //


    ii = analyse_word(inword, outword, outphonemes, &Walkercntxt, &CHIN_WordGrammar);

    if (ii!=0) return (1);

    else return (0);
}

int look_for_next_syllable(CHAR *inphones, int *point, CHAR *syll, int *tone)
{
    int k;
    char c;

    if (inphones[*point]=='\0') return 0;

    *tone =  inphones[*point] - '1' + 1;
    k=0;

    if (*tone >= 1 && *tone <= 5)
    {
	*point += 1;

	for (c = inphones[*point]; !(c == '-' || c == '\0'); *point = *point +1)
	{
	    c = inphones[*point];
	    if (c=='_') c = ' ';
	    if (c != '-') syll[k++] = c;  // don't transport syll boundaries.
	}

	syll[k]='\0';
	return 1;
    }
    else
    {
	return 0;
    }
}



/*

214 + 55  ->  21  + 55
214 + 35  ->  21  + 35
214 + 51  ->  21  + 51

214 + 214 ->  35  + 214
51 + 51 -> 53 + 51

*/

int twotone_sandhi(mandsyll *mand, int nsyll, int t1, int t2, int t1new, int t2new)
{

    int ret;

    ret = 0;

    if (mand[nsyll].toneseq == t1 && 
	mand[nsyll+1].toneseq == t2)
    {
	mand[nsyll].toneseq = t1new;
	mand[nsyll+1].toneseq = t2new;
	ret = 1;
    }
    return ret;
}

void apply_sandhi_rules(mandsyll *mand, int nsyll)
{
    int ksyll;

    for (ksyll=0; ksyll < (nsyll-1) ;ksyll++)
    {
	twotone_sandhi(mand, ksyll, 0x0214, 0x0055, 0x0021, 0x0055);
	twotone_sandhi(mand, ksyll, 0x0214, 0x0035, 0x0021, 0x0035);
	twotone_sandhi(mand, ksyll, 0x0214, 0x0051, 0x0021, 0x0051);
	twotone_sandhi(mand, ksyll, 0x0214, 0x0214, 0x0323, 0x0214);
	twotone_sandhi(mand, ksyll, 0x0051, 0x0051, 0x0053, 0x0051);
	twotone_sandhi(mand, ksyll, 0x0055, 0x0000, 0x0055, 0x0043);  /* may have to be modified */
	twotone_sandhi(mand, ksyll, 0x0035, 0x0000, 0x0035, 0x0044);
	twotone_sandhi(mand, ksyll, 0x0213, 0x0000, 0x0213, 0x0033);
	twotone_sandhi(mand, ksyll, 0x0051, 0x0051, 0x0051, 0x0041);
	twotone_sandhi(mand, ksyll, 0x0051, 0x0000, 0x0051, 0x0012);
    }
}

/* Explanation: There is a state variable current_tone_level which will 
   contained a value 1 - 5. If it is initially 0, it will be set to 3 
   the first time it is accessed. 
   The setting of tones is going to be in multiple half tone steps up and 
   down relative to the base F0. The base F0 can then be changing independently
   outside of this program. Ed can have a falling general pitch 
   contour, and I use it as a reference here. REW
*/

void set_current_tone(int tonel,LPTTS_HANDLE_T phTTS)
{
    /* tonel (tone level) is a number between 1 and 5 */
    PKSD_T pKsd_t;
    pKsd_t = phTTS->pKernelShareData;
    phTTS->current_tone_level = tonel;
}


void set_virtual_f0(int f0,LPTTS_HANDLE_T phTTS)
{
    /* tonel (tone level) is a number between 1 and 5 */
    PKSD_T pKsd_t;
    pKsd_t = phTTS->pKernelShareData;
    phTTS->current_f0_level = f0;
}

int get_base_f0(LPTTS_HANDLE_T phTTS)  
{     
    PDPH_T pDph_t = phTTS->pPHThreadData;
//    if (pDph_t->f0minimum==0) return 130;
//	else
//    return  (pDph_t->f0minimum/10);    /* F0 in Hz */
    return 115;                          // REW 
}


int get_current_tone(LPTTS_HANDLE_T phTTS)        // mock subroutine REWT.
{
    /* tonel is a number between 1 and 5 */
    int tonel;
    PKSD_T pKsd_t;
    pKsd_t = phTTS->pKernelShareData;

    tonel = phTTS->current_tone_level;
    if (tonel<=0) 
    {
	tonel = 3; 
	set_current_tone(tonel,phTTS);
    }
    return (tonel);
}

int get_virtual_f0(LPTTS_HANDLE_T phTTS)        // mock subroutine REWT.
{
    /* tonel is a number between 1 and 5 */
    int f0;
    PKSD_T pKsd_t;
    pKsd_t = phTTS->pKernelShareData;

    f0 = phTTS->current_f0_level;
    if (f0<=0) 
    {
	f0 = get_base_f0(phTTS); 
	set_virtual_f0(f0,phTTS);
    }
    return (f0);
}


 
  
/* look up the tone difference: diff is the number of half tones up or
   down. basef0 is the reference F0. The multiplication is
   in integer arithmetic based on 1 corresponding to 256.A half tone
   interval is 2^(1/12) (12th root of 2), since an octave has 12 half 
   tones. REW
*/

int  lookup_f0_difference(int basef0, int tnew, int told)
{ 
    int mul1, mul2;
    /* table based on half tones */

    switch (tnew)      
    {
	case  1:  mul1 = 194; break;
	case  2:  mul1 = 221; break;
	case  3:  mul1 = 256; break;
	case  4:  mul1 = 298; break;
	case  5:  mul1 = 358; break;
	default:  mul1 = 256; break;
    }

    switch (told)      
    {
	case  1:  mul2 = 194; break;
	case  2:  mul2 = 221; break;
	case  3:  mul2 = 256; break;
	case  4:  mul2 = 298; break;
	case  5:  mul2 = 358; break;
	default:  mul2 = 256; break;
    }

	if (mul1>=mul2) return (basef0*(mul1-mul2)/256);
	else            return (-(basef0*(mul2-mul1)/256));

    //return ((basef0*(mul1-mul2))/256);
}



void mandarin_f0_command(LPTTS_HANDLE_T phTTS, int duration, int change, int delay, int type)
{

#if !defined(UNDER_CE) && defined(PH_DEBUG)	
    PKSD_T pKsd_t = phTTS->pKernelShareData;
    //CAB 03/15/00 WINprintf not supported under Windows CE
    if (DT_DBG(PH_DBG,0x010))
	switch(type)
	{
	    case 0:
		printf("** mandarin_f0_command  TOWARDS dur=%i, change=%i, delay=%i, type=%i\n",
		       duration,change,delay,type);
		break;
	    case -1:
		printf("** mandarin_f0_command  DOWN:   dur=%i, change=%i, delay=%i, type=%i\n",
		       duration,change,delay,type);
		break;
	    case 1:
		printf("** mandarin_f0_command  UP:     dur=%i, change=%i, delay=%i, type=%i\n",
		       duration,change,delay,type);
		break;
	    default:
		printf("** mandarin_f0_command illegal type\n");
		break;
	}
#endif

    if (type==0) 
    {
	//printf("<%d,%d,%d>\n",duration,change,delay);
	f0target(phTTS, duration, change, delay);
    }

    if (type==(-1)) 
    {
	//printf("\\<%d,%d,%d>\n",duration,change,delay);
	f0glidedwn(phTTS, duration, change, delay);
    }
    if (type==1) 
    {
	//printf("/<%d,%d,%d>\n",duration,change,delay);
	f0glideup(phTTS, duration, change, delay); 
    }
}

void mandarin_stretch_phone(LPTTS_HANDLE_T phTTS, 
			    char phone,
			    int stretch,
			    int durfact)
{
    short buf[6];
    char  bufchar[6];
    int   fact;

#if !defined(UNDER_CE) && defined(PH_DEBUG)	
    PKSD_T pKsd_t = phTTS->pKernelShareData;
    //CAB 03/15/00 WINprintf not supported under Windows CE
#endif

    if(stretch == 0) 
    {
	
#if !defined(UNDER_CE) && defined(PH_DEBUG)	
	if (DT_DBG(PH_DBG,0x010))
	{
	    printf("**sending /%c/\n", phone);
	}
#endif
	bufchar[0]=phTTS->pKernelShareData->reverse_ascky[phone];
	ls_util_send_phone(phTTS,bufchar[0]);
    }
    else
    {
/* durfact refers to syllable duration
   stretch refers to stretchability of a sound:
   a stretch of 256 means stiff: little or no duration 
   change possible.
*/
	fact = (((durfact-256)*(stretch-256))/256 + 256);   // REW: EXPERIMENTAL
	fact = durfact;
#if !defined(UNDER_CE) && defined(PH_DEBUG)	
	if (DT_DBG(PH_DBG,0x010))
	{	
	    printf("** sending /%c/ scaled duration by (%i/256)\n", phone ,fact);
	}
#endif
	buf[0]=phTTS->pKernelShareData->reverse_ascky[phone]+(1 << PSNEXTRA);
	buf[1]= -fact; 
	ls_util_write_pipe(phTTS->pKernelShareData,&buf[0],2); 
    }
}


    

int is_mand_vowel(char c)
{
	//	"iUeEAI2u53oO4a678"
    if (c == 'i' ||
		c == 'U' ||
		c == 'e' ||
		c == 'E' ||
		c == 'A' ||
		c == 'I' ||
		c == '2' ||
		c == 'u' ||
		c == '5' ||
		c == '3' ||
		c == '0' ||
		c == 'O' ||
		c == 'o' ||
		c == '4' ||
		c == 'a' ||
		c == '6' ||
		c == '7' ||
		c == '8' ||
		c == '^' ||
		c == 'X'  ) return 1;
    else return 0;
}

int is_mand_voicedcons(char c)
{
    if (c == 'b' ||
		c == 'd' ||
		c == 'g' ||
		c == 'C' ||
		c == 'Z'   ) return 1;
    else return 0;
}

int is_mand_voicelesscons(char c)
{
    if (c == 'f' ||
		c == 'c' ||
		c == 's' ||
		c == 'S' ||
		c == 'p' ||
		c == 't' ||
		c == 'k' ||
		c == 'h' ||
		c == 'q' ||
		c == 'x'    ) return 1;
    else return 0;
}

int is_mand_nasal(char c)
{
    if (c == 'n' ||
		c == 'm' ||
		c == 'N' ) return 1;
    else return 0;
}

int is_mand_semivowel(char c)
{
    if (c == 'J' || 
		c == 'W' ||
		c == '9'  ) return 1;
    else return 0;
}

int is_mand_glide(char c)
{
    if (c == 'w' || 
		c == 'y'   ) return 1;
    else return 0;
}


int is_mand_liquid(char c)
{
    if (c == 'l' || 
		c == 'r'   ) return 1;
    else return 0;
}


void  three_level_tone(LPTTS_HANDLE_T phTTS, 
		       char *syll,
		       int level1,
		       int level2,
		       int level3,
			   int durfact
    )
{
    int len, previous_tone;
    int basef0;
    int deltaf0A, deltaf0B;

#if !defined(UNDER_CE) && defined(PH_DEBUG)	
    PKSD_T pKsd_t = phTTS->pKernelShareData;
#endif


 	
    /* What is needed: 
       (1) move from previous_tone level to the first F0 level1.
       This movement has to be completed once the first sonorant or semivowel
       is sent, so the first voiced sound comes out at the target frequency
       of the first level.
       (2) move from level1 to level2: 
       a) if a semivowel is present (before the vowel), 
       reach target level2 shortly after the beginning of
       the vowel, say 25% of the duration of the vowel.
       b) if there is no semivowel before the vowel, 
       reach the target level2 at about 40% of the duration of the 
       vowel.
       (3) move to level3 (from level2) during the rest of the sonorants of the
       syllable.
    */
//    len = fricativeonset+semivowel+numsonorants;    // total number of phonems
 //   finalnasal =  last_asky_is_nasal(syll,len);

    previous_tone = get_current_tone(phTTS);
    basef0 = get_base_f0(phTTS);

    deltaf0A =  lookup_f0_difference(basef0,level1 , previous_tone);
    mandarin_f0_command(phTTS,20,deltaf0A, 0, 0);

    deltaf0A =  lookup_f0_difference(basef0,level2 , level1);
    deltaf0B =  lookup_f0_difference(basef0,level3 , level2);


	len = strlen(syll);

    /*
      mandarin_f0_command(phTTS,20,deltaf0, 0, 0);
      deltaf0 =  lookup_f0_difference(basef0,level2 , level1);
      delay = 20+timeshift;
      dur = 30*numsonorants;
      mandarin_f0_command(phTTS,dur, deltaf0, delay, -1);
      deltaf0 =  lookup_f0_difference(basef0,level3 , level2);
      delay = 100*numsonorants+timeshift;
      dur = 90*numsonorants;
      mandarin_f0_command(phTTS,dur, deltaf0, delay, 1);
    */

    //mand_phoneme_output(phTTS,syll,1,fricativeonset);
    //mand_phoneme_stretched(phTTS, syll, 1, 1, 320, durfact);
    
    if (len==1)
    {
	/* 
	   1  one vowel only (open syllable)     
	
	*/
	mandarin_f0_command(phTTS,  40, deltaf0A, 10, -1);
	mandarin_f0_command(phTTS, 100, deltaf0B, 45,  1);
	mandarin_stretch_phone(phTTS, syll[0], 512, durfact);
    }
	
	
	
    else if (len==2)
    {
		
	if (is_mand_vowel(syll[0]) && is_mand_nasal(syll[1]))
	{
	    /*
	      2 vowel + nasal coda
	    */
	    mandarin_f0_command(phTTS,  60, deltaf0A, 10, -1);
	    mandarin_f0_command(phTTS, 140, deltaf0B, 80,  1);
	    mandarin_stretch_phone(phTTS, syll[0], 384, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
	}
	else if (is_mand_nasal(syll[0]) && is_mand_vowel(syll[1]))
	{	
	    /*
	      2 nasal + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS,  30, deltaf0A,-40, -1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_f0_command(phTTS, 100, deltaf0B, 35,  1);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_voicedcons(syll[0]) && is_mand_vowel(syll[1]))
	{
	    /*
	      2 voiced consonant + vowel
	    */
	    mandarin_f0_command(phTTS,  40, deltaf0A, -30, -1);
	    mandarin_f0_command(phTTS, 100, deltaf0B,  45,  1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_voicelesscons(syll[0]) && is_mand_vowel(syll[1]))
	{
	    /*
	      2 voiceless cons + one vowel only (open syllable)     
	    */
	    mandarin_f0_command(phTTS,  40, deltaf0A, 10, -1);
	    mandarin_f0_command(phTTS, 100, deltaf0B, 45,  1);
	    mandarin_stretch_phone(phTTS, syll[0], 0  , durfact);    /* just send */
	    mandarin_stretch_phone(phTTS, syll[1], 512, durfact);
	}
		
	else if (is_mand_semivowel(syll[0]))
	{
	    /*
	      2 semivowel + vowel (open syllable)
	    */
	    mandarin_f0_command(phTTS, 110, deltaf0A,  30, -1);
	    mandarin_f0_command(phTTS, 180, deltaf0B, 120,  1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_liquid(syll[0]))
	{
	    /*
	      2 liquid + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS,  30, deltaf0A, -50, -1);
	    mandarin_f0_command(phTTS, 100, deltaf0B,  35,  1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_glide(syll[0]))
	{
	    /*
	      2 glide + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS, 110, deltaf0A,  30, -1);
	    mandarin_f0_command(phTTS, 180, deltaf0B, 120,  1);
	    //mandarin_f0_command(phTTS,  30, deltaf0A, -50, -1);
	    //mandarin_f0_command(phTTS,  100, deltaf0B,  35,  1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
	}
		
	else
	{
	    /*
	      2 two vowels only (should be all that's left):
	    */
	    mandarin_f0_command(phTTS,  80, deltaf0A, 10, -1);
	    mandarin_f0_command(phTTS, 180, deltaf0B, 90,  1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
    }  /* end of case len == 2 */
	
    else if (len==3)
    {
	if (is_mand_vowel(syll[1]) && is_mand_nasal(syll[2]))
	{
	    if (is_mand_semivowel(syll[0]))
	    {
		/*
		  3 semivowel + vowel + nasal
		*/
		mandarin_f0_command(phTTS, 110, deltaf0A, 30, -1);
		mandarin_f0_command(phTTS, 180, deltaf0B,120,  1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else if (is_mand_liquid(syll[0]))
	    {
		/*
		  3 liquid + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  30, deltaf0A, -50, -1);
		mandarin_f0_command(phTTS, 140, deltaf0B,  40,  1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else if (is_mand_glide(syll[0]))
	    {
		/*
		  3 glide + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  30, deltaf0A, -50, -1);
		mandarin_f0_command(phTTS, 140, deltaf0B,  40,  1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
			
	    else if (is_mand_nasal(syll[0]))
	    {
				
		/*
		  3 nasal + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  30, deltaf0A, -50, -1);
		mandarin_f0_command(phTTS, 160, deltaf0B,  40,  1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
			
	    else if (is_mand_voicedcons(syll[0]))
	    {
		/*
		  3 voiced consonant + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  30, deltaf0A, -30, -1);
		mandarin_f0_command(phTTS, 140, deltaf0B,  40,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);         /* just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else
	    {
		/*
		  3 voiceless cons + vowel + nasal coda
		*/
		mandarin_f0_command(phTTS,  60, deltaf0A, 10, -1);
		mandarin_f0_command(phTTS, 100, deltaf0B, 80,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	}
	else if ((is_mand_semivowel(syll[1]) || is_mand_vowel(syll[1])) && is_mand_vowel(syll[2]))
	{
	    if (is_mand_voicedcons(syll[0]))
	    {
		/*
		  3 voiced consonant + semivowel + vowel
		*/
		mandarin_f0_command(phTTS,  70, deltaf0A, -25, -1);
		mandarin_f0_command(phTTS, 160, deltaf0B,  75,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else if (is_mand_liquid(syll[0]))
		{
		mandarin_f0_command(phTTS,  70, deltaf0A, -25, -1);
		mandarin_f0_command(phTTS, 160, deltaf0B,  75,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
		}
		else if (is_mand_nasal(syll[0]))
		{
		mandarin_f0_command(phTTS,  70, deltaf0A, -25, -1);
		mandarin_f0_command(phTTS, 160, deltaf0B,  75,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
		}
		else
	    {
		/*
		  3 voiceless cons + semivowel + vowel (open syllable)
		*/
		mandarin_f0_command(phTTS, 110, deltaf0A, 30, -1);
		mandarin_f0_command(phTTS, 180, deltaf0B,120,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 384, durfact);
	    }
	}
	else
	{
	    if (is_mand_voicelesscons(syll[0]))
	    {
		/*
		  3 voiceless cons + two vowels only:
		*/
		mandarin_f0_command(phTTS,  80, deltaf0A, 10, -1);
		mandarin_f0_command(phTTS, 180, deltaf0B, 90,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	    else if(is_mand_voicedcons(syll[0]))
	    {
		/*
		  3 voiced cons + two vowels.
		*/
		mandarin_f0_command(phTTS,  70, deltaf0A, -25, -1);
		mandarin_f0_command(phTTS, 160, deltaf0B,  75,  1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	}
    }
    else if (len==4)
    {
	if (is_mand_voicedcons(syll[0]))
	{
	    /*
	      4 voiceless cons + semivowel + vowel + nasal
	    */
	    mandarin_f0_command(phTTS, 110, deltaf0A, 30, -1);
	    mandarin_f0_command(phTTS, 240, deltaf0B,120,  1);
	    mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
	    mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[3], 256, durfact);
	}
	else 
	{
	    /*
	      4 voiced consonant + semivowel+vowel+nasal
	      or others with voiced start.
	    */
	    mandarin_f0_command(phTTS, 110, deltaf0A, -25, -1);
	    mandarin_f0_command(phTTS, 240, deltaf0B,  120,  1);
	    mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
	    mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[3], 256, durfact);				
	}
    }
	
/*	
  stretch_a_syllable(phTTS,syll,
  fricativeonset,
  semivowel,
  numsonorants,
  512);         // stretch duration to 200%
*/

    set_current_tone(level3,phTTS);
}

void  raising_tone(LPTTS_HANDLE_T phTTS, 
		     char *syll,
		     int level1,
		     int level2,
			 int durfact
		   )
{
    int len,previous_tone,basef0,deltaf0;

/* What is needed:

1) move from previous_tone level to the first level level1.
This movement has to be completed once the first sonorant or semivowel
is sent, so the first voiced sound comes out at the target frequency

2) move to level2 according to the following rule:
	
since level2 is higher than level1:
remain for 40% of the total duration of the sonorants of the syllable
on level1 and then move up to level2 during the remaining time of
the syllable. Take into account the durationfactor.

*/

    len = strlen(syll);
 
    previous_tone = get_current_tone(phTTS);

    previous_tone = get_current_tone(phTTS);
    basef0 = get_base_f0(phTTS);
 
    deltaf0 =  lookup_f0_difference(basef0,level1 , previous_tone);
    //  /\ movement towards a target level 
    mandarin_f0_command(phTTS,20,deltaf0,0,0);  // task (1) above.
    deltaf0 =  lookup_f0_difference(basef0,level2 , level1);

    if (len==1)
    {
	/* 
	   1  one vowel only (open syllable)     
	*/
	mandarin_f0_command(phTTS,  100 , deltaf0, 40 , -1);
	mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
    }
	
    else if (len==2)
    {
		
	if (is_mand_vowel(syll[0]) && is_mand_nasal(syll[1]))
	{
	    /*
	      2 vowel + nasal coda
	    */
	    mandarin_f0_command(phTTS,  120, deltaf0, 40 , 1);
	    mandarin_stretch_phone(phTTS, syll[0], 384, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
	}
	else if (is_mand_nasal(syll[0]) && is_mand_vowel(syll[1]))
	{	
	    /*
	      2 nasal + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS, 80, deltaf0, 60 , 1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_voicedcons(syll[0]) && is_mand_vowel(syll[1]))
	{
	    /*
	      2 voiced consonant + vowel
	    */
	    mandarin_f0_command(phTTS,  100, deltaf0, -20, 1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_voicelesscons(syll[0]) && is_mand_vowel(syll[1]))
	{
	    /*
	      2 voiceless cons + one vowel only (open syllable)     
	    */
	    mandarin_f0_command(phTTS,  100 , deltaf0, 30, 1);
	    mandarin_stretch_phone(phTTS, syll[0], 0  , durfact);    /* just send */
	    mandarin_stretch_phone(phTTS, syll[1], 512, durfact);
	}
		
	else if (is_mand_semivowel(syll[0]))
	{
	    /*
	      2 semivowel + vowel (open syllable)
	    */
	    mandarin_f0_command(phTTS, 140, deltaf0, 60,  1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_liquid(syll[0]))
	{
	    /*
	      2 liquid + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS, 100, deltaf0, -30, 1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_glide(syll[0]))
	{
	    /*
	      2 glide + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS,  100, deltaf0, -30, 1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else
	{
	    /*
	      2 two vowels only (should be all that's left):
	    */
	    mandarin_f0_command(phTTS, 180 , deltaf0, 60, 1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
    }  /* end of case len == 2 */
	
    else if (len==3)
    {
	if (is_mand_vowel(syll[1]) && is_mand_nasal(syll[2]))
	{
	    if (is_mand_semivowel(syll[0]))
	    {
		/*
		  3 semivowel + vowel + nasal
		*/
		mandarin_f0_command(phTTS, 190, deltaf0, 60, 1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	    else if (is_mand_liquid(syll[0]))
	    {
		/*
		  3 liquid + vowel + nasal
		*/
		mandarin_f0_command(phTTS, 110, deltaf0, -20, 1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	    else if (is_mand_glide(syll[0]))
	    {
		/*
		  3 glide + vowel + nasal
		*/
		mandarin_f0_command(phTTS, 120 , deltaf0, -20, 1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
			
	    else if (is_mand_nasal(syll[0]))
	    {
				
		/*
		  3 nasal + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  120, deltaf0, 20, 1);
		mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
			
	    else if (is_mand_voicedcons(syll[0]))
	    {
		/*
		  3 voiced consonant + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  180, deltaf0, 50, 1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);         /* just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else
	    {
		/*
		  3 voiceless cons + vowel + nasal coda
		*/
		mandarin_f0_command(phTTS,  180, deltaf0, 50, 1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	}
	else if ((is_mand_semivowel(syll[1]) || is_mand_vowel(syll[1])) && is_mand_vowel(syll[2]))
	{
	    if (is_mand_voicedcons(syll[0]) || 
			is_mand_liquid(syll[0])     ||
			is_mand_nasal(syll[0])         )
	    {
		/*
		  3 voiced consonant + semivowel + vowel
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 60 , 1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else
	    {
		/*
		  3 voiceless cons + semivowel + vowel (open syllable)
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 60, 1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 384, durfact);
	    }
	}
	else
	{
	    if (is_mand_voicelesscons(syll[0]))
	    {
		/*
		  3 voiceless cons + two vowels only:
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 60, 1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	    else if(is_mand_voicedcons(syll[0]))
	    {
		/*
		  3 voiced cons + two vowels.
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 60, 1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	}
    }
    else if (len==4)
    {
	if (is_mand_voicedcons(syll[0]))
	{
	    /*
	      4 voiceless cons + semivowel + vowel + nasal
	    */
	    mandarin_f0_command(phTTS, 220, deltaf0, 70, 1);
	    mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
	    mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[3], 256, durfact);
	}
	else 
	{
	    /*
	      4 voiced consonant + semivowel+vowel+nasal
	      or others with voiced start.
	    */
	    mandarin_f0_command(phTTS, 220, deltaf0, 70, 1);
	    mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
	    mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[3], 256, durfact);				
	}
    }
/*
    stretch_a_syllable(phTTS,syll,
		       fricativeonset,
		       semivowel,
		       numsonorants,
		       durationfactor);

*/
//	mand_phoneme_output(phTTS,syll,1,len);

    set_current_tone(level2,phTTS);
}


void  falling_tone(LPTTS_HANDLE_T phTTS, 
		     char *syll,
		     int level1,
		     int level2,
			 int durfact
		   )
{
    int len,previous_tone,basef0,deltaf0;
 
/* What is needed:

1) move from previous_tone level to the first level level1.
This movement has to be completed once the first sonorant or semivowel
is sent, so the first voiced sound comes out at the target frequency

2) 
since level2 is lower than level1:
move immediately down to level 2.
*/

    len = strlen(syll);

    previous_tone = get_current_tone(phTTS);
    basef0 = get_base_f0(phTTS);
                 
    deltaf0 =  lookup_f0_difference(basef0,level1 , previous_tone);
    //  /\ movement towards a target level 
    mandarin_f0_command(phTTS,20,deltaf0,0,0);  // task (1) above.
    deltaf0 =  lookup_f0_difference(basef0,level2 , level1);

    if (len==1)
    {
	/* 
	   1  one vowel only (open syllable)     
	*/
	mandarin_f0_command(phTTS,  100 , deltaf0, 10 , -1);
	mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
    }
	
    else if (len==2)
    {
		
	if (is_mand_vowel(syll[0]) && is_mand_nasal(syll[1]))
	{
	    /*
	      2 vowel + nasal coda
	    */
	    mandarin_f0_command(phTTS,  120, deltaf0, 10 , -1);
	    mandarin_stretch_phone(phTTS, syll[0], 384, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
	}
	else if (is_mand_nasal(syll[0]) && is_mand_vowel(syll[1]))
	{	
	    /*
	      2 nasal + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS, 100, deltaf0, -50 , -1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_voicedcons(syll[0]) && is_mand_vowel(syll[1]))
	{
	    /*
	      2 voiced consonant + vowel
	    */
	    mandarin_f0_command(phTTS,  100, deltaf0, -20, -1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_voicelesscons(syll[0]) && is_mand_vowel(syll[1]))
	{
	    /*
	      2 voiceless cons + one vowel only (open syllable)     
	    */
	    mandarin_f0_command(phTTS,  100 , deltaf0, 10, -1);
	    mandarin_stretch_phone(phTTS, syll[0], 0  , durfact);    /* just send */
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_semivowel(syll[0]))
	{
	    /*
	      2 semivowel + vowel (open syllable)
	    */
	    mandarin_f0_command(phTTS, 180, deltaf0, 20,  -1);
	    mandarin_stretch_phone(phTTS, syll[0], 256, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_liquid(syll[0]))
	{
	    /*
	      2 liquid + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS, 100, deltaf0, -50, -1);
	    mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else if (is_mand_glide(syll[0]))
	{
	    /*
	      2 glide + vowel  (open syllable)
	    */
	    mandarin_f0_command(phTTS,  100, deltaf0, -50, -1);
	    mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
		
	else
	{
	    /*
	      2 two vowels only (should be all that's left):
	    */
	    mandarin_f0_command(phTTS, 180 , deltaf0, 20, -1);
	    mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
	}
    }  /* end of case len == 2 */
	
    else if (len==3)
    {
	if (is_mand_vowel(syll[1]) && is_mand_nasal(syll[2]))
	{
	    if (is_mand_semivowel(syll[0]))
	    {
		/*
		  3 semivowel + vowel + nasal
		*/
		mandarin_f0_command(phTTS, 190, deltaf0, 20, -1);
		mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else if (is_mand_liquid(syll[0]))
	    {
		/*
		  3 liquid + vowel + nasal
		*/
		mandarin_f0_command(phTTS, 110, deltaf0, -50, -1);
		mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	    else if (is_mand_glide(syll[0]))
	    {
		/*
		  3 glide + vowel + nasal
		*/
		mandarin_f0_command(phTTS, 120 , deltaf0, -50, -1);
		mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
			
	    else if (is_mand_nasal(syll[0]))
	    {
				  
		/*
		  3 nasal + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  120, deltaf0, -20, -1);
		mandarin_stretch_phone(phTTS, syll[0], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
			
	    else if (is_mand_voicedcons(syll[0]))
	    {
		/*
		  3 voiced consonant + vowel + nasal
		*/
		mandarin_f0_command(phTTS,  120, deltaf0, -30, -1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);         /* just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else
	    {
		/*
		  3 voiceless cons + vowel + nasal coda
		*/
		mandarin_f0_command(phTTS,  120, deltaf0, 10, -1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	}
	else if ((is_mand_semivowel(syll[1]) || is_mand_vowel(syll[1])) && is_mand_vowel(syll[2]))
	{
	    if (is_mand_voicedcons(syll[0]) ||
			is_mand_liquid(syll[0])     ||
			is_mand_nasal(syll[0])        )
	    {
		/*
		  3 voiced consonant + semivowel + vowel
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 20 , -1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 384, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 256, durfact);
	    }
	    else
	    {
		/*
		  3 voiceless cons + semivowel + vowel (open syllable)
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 20, -1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 256, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 384, durfact);
	    }
	}
	else
	{
	    if (is_mand_voicelesscons(syll[0]))
	    {
		/*
		  3 voiceless cons + two vowels only:
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 20, -1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	    else if(is_mand_voicedcons(syll[0]))
	    {
		/*
		  3 voiced cons + two vowels.
		*/
		mandarin_f0_command(phTTS, 180, deltaf0, 20, -1);
		mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
		mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
		mandarin_stretch_phone(phTTS, syll[2], 320, durfact);
	    }
	}
    }
    else if (len==4)
    {
	if (is_mand_voicedcons(syll[0]))
	{
	    /*
	      4 voiceless cons + semivowel + vowel + nasal
	    */
	    mandarin_f0_command(phTTS, 220, deltaf0, 10, -1);
	    mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
	    mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[2], 384, durfact);
	    mandarin_stretch_phone(phTTS, syll[3], 256, durfact);
	}
	else 
	{
	    /*
	      4 voiced consonant + semivowel+vowel+nasal
	      or others with voiced start.
	    */
	    mandarin_f0_command(phTTS, 220, deltaf0, 10, -1);
	    mandarin_stretch_phone(phTTS, syll[0],   0, durfact);  /* 0: just send */
	    mandarin_stretch_phone(phTTS, syll[1], 320, durfact);
	    mandarin_stretch_phone(phTTS, syll[2], 384, durfact);
	    mandarin_stretch_phone(phTTS, syll[3], 256, durfact);				
	}
    }
/*
    stretch_a_syllable(phTTS,syll,
		       fricativeonset,
		       semivowel,
		       numsonorants,
		       durationfactor);

*/
//	mand_phoneme_output(phTTS,syll,1,len);

    set_current_tone(level2,phTTS);
}


void  two_level_tone(LPTTS_HANDLE_T phTTS, 
		     char *syll,
		     int level1,
		     int level2,
		     int durfact
    )
{
#if !defined(UNDER_CE) && defined(PH_DEBUG)	
    PKSD_T pKsd_t = phTTS->pKernelShareData;
#endif

/* What is needed:

1) move from previous_tone level to the first level level1.
This movement has to be completed once the first sonorant or semivowel
is sent, so the first voiced sound comes out at the target frequency

2) move to level2 according to the following rule:
	
a) if level2 is higher than level1:
remain for 40% of the total duration of the sonorants of the syllable
on level1 and then move up to level2 during the remaining time of
the syllable. Take into account the durationfactor.

b) if level2 is lower than level1, the durationfactor will be small and
the syllable duration is reduced. remain only very short time (20% of the
syllable duration) on level1, then move to level2. This is a rapid down
movement of F0.

*/

    if (level2 >= level1) raising_tone(phTTS,syll,level1,level2, durfact);
    else                  falling_tone(phTTS,syll,level1,level2, durfact);  

}




void make_f0_contour(LPTTS_HANDLE_T phTTS, CHAR *syll, int toneseq, int durfact)
{
    
    //int dur;
    int first, second, third;
#if !defined(UNDER_CE) && defined(PH_DEBUG)	
    PKSD_T pKsd_t = phTTS->pKernelShareData;
#endif


    if (toneseq > 0x0055)  // for example: 3rd tone
    {
	first = (int) toneseq/256;
	second = (int) (toneseq - first*256)/16;
	third = (toneseq - first*256 - second*16);
	
	three_level_tone(phTTS,
			 syll,
			 first,
			 second,
			 third,
			 durfact
	    );
	set_current_tone(third,phTTS);
    }
    else //if (toneseq>0)
    {
	first = (int) toneseq/16;
	second = (int) (toneseq - first*16);
//	printf("B fricativeonset: %i eins: %d zwei %d\n",fricativeonset, first, second);
//	printf("B fricativeonset: %i, Sonorants %i.  1st: %d 2nd %d\n",
//	       fricativeonset, numsonorants,first, second);

	//dur = duration_factor(second-first);     // duration in parts per 256

	two_level_tone(phTTS,
		       syll,
		       first,
		       second,
		       durfact);
	
	set_current_tone(second, phTTS);
    }
}



  
int mandarin_syllables(CHAR *inphones, CHAR *outphones, mandsyll *msyll)
{
    /* First find the next number in the string that is in between 1 and 5. */
    
    int point;
    int tone;
    int len,i;
    int nsyll=0;

    
    CHAR syll[20];

    point = 0;
    
    while (look_for_next_syllable(inphones, &point, syll, &tone))
    {
	len = strlen(syll);

	if (len<6) for(i=0;i<len;i++) msyll[nsyll].syll[i]=syll[i];
        msyll[nsyll].syll[len] = '\0';
	msyll[nsyll].tone = tone;
	switch (tone)
	{
	case 1: msyll[nsyll].durscale = 240; break;
	case 2: msyll[nsyll].durscale = 240; break;
	case 3: msyll[nsyll].durscale = 280; break;
	case 4: msyll[nsyll].durscale = 140; break;
	case 5: msyll[nsyll].durscale = 130; break;
	default: msyll[nsyll].durscale = 256; break;
	}

	msyll[nsyll].toneseq = tone_to_toneseq(tone);

	// make_mand_tone(syll, tone, len);
	// send_phone_string(syll);

	nsyll += 1;
    }
    return nsyll;
}

void mandarin_tone_process(LPTTS_HANDLE_T phTTS, char *outphonemes)
{
    int nsyll;
    int k;
    mandsyll mandsylls[50];  
#if !defined(UNDER_CE) && defined(PH_DEBUG)	
    PKSD_T pKsd_t = phTTS->pKernelShareData;
#endif

    nsyll = mandarin_syllables(outphonemes, outphonemes, mandsylls);

#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows CE
    if (DT_DBG(PH_DBG,0x010))
    {
	printf("******** Mandarin input with tones ***********\n");
	for (k=0;k<nsyll;k++)
	{
	    printf("** Syll %i: %s tone %i. Initial tone level sequence: %x\n",
		   k+1, 
		   mandsylls[k].syll,
		   mandsylls[k].tone,
		   mandsylls[k].toneseq);
	}
    }
#endif

    apply_sandhi_rules(mandsylls , nsyll);

#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows CE
    if (DT_DBG(PH_DBG,0x010))
    {
	printf("** After application of Sandhi rules:\n");

	for (k=0;k<nsyll;k++)
	{
	    printf("** Syll %i: %s tone %i. Altered tone level sequence: %x\n",
		   k+1, 
		   mandsylls[k].syll,
		   mandsylls[k].tone,
		   mandsylls[k].toneseq);
	}
    }
#endif

    for (k=0;k<nsyll;k++)
    {
	make_f0_contour(phTTS, mandsylls[k].syll, mandsylls[k].toneseq, mandsylls[k].durscale);
    }
    set_current_tone(3, phTTS);
}

