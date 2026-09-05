/*C_HEADER_FILE****************************************************************
FILE			:	stdonsei.c 
DESC			:	contains the APIs for the onsei functions
TABS			:	4
OWNER			:	
DATE CREATED:	

(C) Copyright 2005 All rights reserved.
This is an unpublished work, and is confidential and proprietary: 
technology and information of fonix corporation.  No part of this
code may be reproduced, used or disclosed without written consent of 
fonix corporation in each and every instance.

  $Date:  $
  $Revision:  $

*END_HEADER*******************************************************************/
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "onsei.h"
#include "entrypt.h"
#include "onseiapi.h"
//#include "ls_def.h"
//#include "ls_rule.h"

#define PAGESIZE 1024

static STRUCT_ONSEI OnseiParam = {0};
static uint8_t outbuf[2+MAX_OMEGA*3+3+1] = {0};		/* 出力データエリア */
static int jfd = 0; /* Japanese file descriptor ? */
static const char* dictionary = "/usr/share/dict/onsei.dic";

static int u_alloc(void** addr, size_t size);
static void u_dealloc(void** addr);
static int u_write(int page, size_t size, void *addr);
static int u_read(int page, size_t size, void* addr);
static int onsei_checkdata(int16_t fall);
int hatsuon_mora(uint8_t* ptr, uint8_t* buf);

extern void check_X(uint8_t* , int16_t);
extern int dispchar_to_code(uint8_t* , uint8_t*);

/*FUNCTION_HEADER**********************
 * NAME:	;onsei_init
 * DESC: 	Performs initialization for the conversion processing.
 * IN:		jname - Pointer to the dictionary path
			inbuf - Pointer to Kana and Chinese character (Kanji) mixture sentence (JIS X 0208)
			talkbuf - Pointer to the place where the phonogram string is stored
			sentbuf - Pointer to the place where word or phrase information is stored
			kousei - when 0, disregard signs, when 1, read all signs
			u_stop - function that discontinues conversion
 * OUT:		
 * RETURN:	0:					Normal termination
			Err_Fatal_DIC:		Dictionary (onsei.dic) can't be opened
			Err_Broken:			Dictionary is broken
			Err_Fatal_Alloc:	Insufficient memory
 * NOTES:	
 *  -- Secures the place for Kana Chinese character (Kanji) mixture sentences, for the phonogram string,
 *     and for the word or phrase information, and calls the pointer for each.
 *  -- When you can not wait for the analysis to end, you define the function u_stop
 *     which forces it to end and call that function's far pointer.
 *  -- Calls the dictionary path and symbolic reading flag.
 *  -- The parameters above can not be modified in the middle of processing.
 *     When wanting to modify, call the onsei_term, and then call the onsei_init again. 
*END_HEADER***************************/
int onsei_init(const char* jname, uint8_t* inbuf, uint8_t* talkbuf, uint16_t* sentbuf, 
			   int16_t kousei, int (*u_stop)(void*))
{
    int status;

    dictionary = jname;
    OnseiParam.onsei_inert = inbuf;
    OnseiParam.onsei_speak = talkbuf;
    OnseiParam.onsei_sent  = sentbuf;
    OnseiParam.onsei_alloc = u_alloc;
    OnseiParam.onsei_dealloc = u_dealloc;
    OnseiParam.onsei_read  = u_read;
    OnseiParam.onsei_write = u_write;
    OnseiParam.onsei_stop  = u_stop;			// simple function that just returns 0 (?)
    OnseiParam.onsei_kouseisystem = kousei;		// Correction system (0)

    OnseiParam.onsei_omega = outbuf;
    OnseiParam.onsei_usetankan = 1;		/* １に固定 */
    OnseiParam.onsei_readcrlf = 0;		/* ０に固定 */
    OnseiParam.onsei_crlfexist = 0;		/* ０に固定 */
    
    dictionary=jname;

    if ((jfd=open(dictionary,O_RDONLY | O_BINARY))==-1) 
	{
        return Err_Fatal_DIC;	/* 辞書がない */
    }

    OnseiParam.onsei_procno = PROC_ENV_INIT;
    status=entrypt(&OnseiParam);
    return status;
}

/*FUNCTION_HEADER**********************
 * NAME:	;onsei_henkan
 * DESC: 	Performs all the types of conversion processing.
 * IN:		sw - type of processing
 * OUT:		
 * RETURN:	
 * NOTES:	
 * In the types of processing (sw), you can define any of the following:
 *
 *   PROC_FIRST:		First conversion
 *   PROC_NEXT:			Attention word or phrase next candidate
 *   PROC_TERM:			Decides on learned information
 *   PROC_RIGHT:		Attention word or phrase move to the right
 *   PROC_LEFT:			Attention word or phrase move to the left
 *   PROC_SHORT:		Splits into the direction of short word or phrase
 *	 PROC_LONG:			Splits into the direction of long word or phrase
 *   PROC_SENT_NO:		Takes out the attention word or phrase number
 *   PROC_ACON:			Combines the accent of attention word or phrase and the next word or phrase
 *   PROC_ACOFF:		Does not combine the accent of attention word or phrase to the next word or phrase
 *   PROC_PAUSE_INS:	Inserts a pause directly after the attention word or phrase.
 *   PROC_PAUSE_DEL:	Takes out the puase directly after the attention word or phrase
 * 
 *
 * When the sw is anything but PROC_FIRST, PROC_NEXT, or PROC_SENT_NO,  the value is as follows:
 *    0:				Normal termination
 *    Err_NaibuOVF:		The word or phrase number is over limit (too big) or the phonogram string is too long
 *    Err_Chuudan:		Stopped at u_stop()
 *    Err_Fatal_Edic:	Dictionary malfunction
 *    Err_Fatal_Idic:	Dictionary malfunction
 *    Err_IdouFunou:	Word or phrase movement is not possible
 *    Err_NextNone:		There is no next candidate
 *    Err_RestrictCNT:	Timed out
 *
 * When the sw is PROC_FIRST or PROC_NEXT, then 
 *    15 or less:		Normal termination. The first word is positioned at the top.
 *    Err_NaibuOVF:		The word or phrase number is over or the phonogram string is too long
 *    Err_Chuudan:		Stopped at at u_stop()
 *    Err_Fatal_Edic:	Dictionary malfunction
 *    Err_Fatal_Idic:	Dictionary malfunction
 *    Err_IdouFunou:	Word or phrase movement is not possible
 *    Err_NextNone:		There is no next candidate
 *    Err_HenkanTochuu:	Conversion of a word that is not in the dictionary
 *    Err_RestrictCNT:	Timed out
 *
 * When the sw is PROC_SENT_NO: then return the current word or phrase number
 *END_HEADER***************************/
int onsei_henkan(int16_t sw)
{
    int status;
	
    if (sw == PROC_SENT_NO) 
	{
        return OnseiParam.onsei_wsno;
    } 
	else if (sw == PROC_FIRST) 
	{
        uint8_t* c;
        for (c = outbuf; *c; ++c) 
		{
            *c = 0;
        }
    }
    OnseiParam.onsei_procno = sw;
    status=entrypt(&OnseiParam);		// This is where the rough conversion from JIS to Romaji happens when sw == 3
    /*
	* 正常終了なら、１つ目の単語の滝の位置を返す。
	* PROC_FIRST,PROC_NEXTのみ
	*/
    switch (sw) 
	{
	case PROC_FIRST:
	case PROC_NEXT:
		if(status==0) 
		{
			/* 滝の位置 */
			return OnseiParam.onsei_acctype;
		} 
		else 
		{
			if (status<0x80) 
			{
				/* 文節移動・次候補なし・タイムアウト */
				return status | 0x70;
			} 
			else 
			{
				return status;
			}
		}
    }
    return status;
}

/*FUNCTION_HEADER**********************
 * NAME:	;onsei_term
 * DESC: 	Completes the conversion processing. 
			Writes out the learned information, etc.
 * IN:		
 * OUT:		
 * RETURN:	0:		Normal termination
 *			else:	Dictionary malfunction
 * NOTES:	
 *END_HEADER***************************/
int onsei_term(void)
{
    int status;
    OnseiParam.onsei_procno = PROC_ENV_TERM;
    status=entrypt(&OnseiParam);
    close(jfd);
    return status;
}

/*FUNCTION_HEADER**********************
 * NAME:	;onsei_add
 * DESC: 	Performs registering of the dictionary
 * IN:		talkbuf
			fall - The position of the top of the accent
 * OUT:		
 * RETURN:	
 *    0:				Normal termination
 *    1:				The way it is read or the top position of the accent is not correct
 *    Err_EdicTooLong:	The written item or the reading is too long
 *    Err_TourokuSumi:	The same one is already registered
 *    Err_EdicOVF:		Can not be registered -> if the dictionary readjusts, then it will be OK
 * NOTES:	
 *END_HEADER***************************/
int onsei_add(int16_t fall)
{
    int status;

	if(onsei_checkdata(fall)==1) 
	{
        return 1;
    }
    status=onsei_henkan(PROC_EDIC_ADD);
    return status;
}

/*FUNCTION_HEADER**********************
 * NAME:	;onsei_del
 * DESC: 	Performs dictionary deletion
 * IN:		talkbuf
			fall - The position of the top of the accent
 * OUT:		
 * RETURN:	
 *    0:				Normal termination 
 *    1:				The way it is read or the top position of the accent is not correct
 *    Err_EdicTooLong:	The written item or the reading is too long
 *    Err_MiTouroku:	Word not registered 
 * NOTES:	
 *END_HEADER***************************/
int onsei_del(int16_t fall)
{
    int status;

	if(onsei_checkdata(fall)==1) 
	{
        return 1; 
    }
    status=onsei_henkan(PROC_EDIC_DELETE2);
    return status;
}


/*FUNCTION_HEADER**********************
 * NAME:	;onsei_del2
 * DESC: 	Perform dictionary deletion of the words that are in the process of currently being converted.
			Before calling this function, it is absolutely necessary to call the first conversion and 
			then the next candidate.  When the return value of the first conversion and the next 
			candidate is 16 or more, (except when the accent is at the top position), you must not call it.
 * IN:		talkbuf
 * OUT:		
 * RETURN:	 
 *    0:				Normal termination 
 *    Err_MiTouroku:	It is not the user's registration word
 * NOTES:	
 *END_HEADER***************************/
int onsei_del2()
{
    int status;

    status=onsei_henkan(PROC_EDIC_DELETE);
    return status;
}


/*FUNCTION_HEADER**********************
 * NAME:	;onsei_checkdata
 * DESC: 	Copies talkbuf to outbuf
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
int onsei_checkdata(int16_t fall)
{
    /* input: fall */
    /*	return 0: OK	*/
    /*	return 1: Error	*/
    uint16_t nbuf,i; 
	uint8_t *si,*di;
  
	strcpy(outbuf, OnseiParam.onsei_speak);

    /* ROMA字なら大文字に変換 */
    {
        uint8_t* c;
        for (c = outbuf; *c; ++c) 
		{
            *c = toupper(*c);
        }
    }

    /* 発音データチェック */
    if (hatsuon_mora(outbuf,&outbuf[20])==0) 
	{
        return 1;
    }
    nbuf=outbuf[20];
    if (nbuf>15 || fall>nbuf) 
	{ 
        return 1;
    }
    /* 撥音と促音の分類をする。*/
    check_X(&outbuf[21],outbuf[20]);
    
	/* アクセント情報 */
    outbuf[1] = 4;
    outbuf[2] = (uint8_t)fall; 
	outbuf[3] = 0x11; 
	outbuf[4] = 0x21;
    outbuf[5] = (fall) ? (0x40|(uint8_t)fall) : 0x41;

    /*
     *	explanation of outbuf[]
     *
     * from
     *	 0  1  2...				  20 21...
     *	+--+--+--------------+---+--+--------------------+
     *	| ?|L1| accent codes |   |L2| pronunciation data |
     *	+--+--+--------------+---+--+--------------------+
     *	      |<-----L1----->|      |<--------L2-------->|
     *	      |<------ 18 ------>|
     *
     * to
     *   0		 1	   2...			  L1+2...
     *	+-------+-----+--------------+--------------------+
     *	|L1+L2+1|L1/L2| accent codes | pronunciation data |
     *	+-------+-----+--------------+--------------------+
     *	              |<-----L1----->|<--------L2-------->|
     *
     *	outbuf[0]				: number of total length (L1+L2+1)
     *	outbuf[1] upper 4 bits	: number of accent code
     *	outbuf[1] lower 4 bits	: mora number of pronunciation data
     *	outbuf[2]..				: accent codes
     *	outbuf[following]..		: pronunciation data
     */
    nbuf=outbuf[20]; 
	i=outbuf[1]; 
	di=outbuf+i+2; 
	si=(&outbuf[21]);
    outbuf[0]=nbuf+i+1; 
	outbuf[1]=nbuf+(i<<4);
    for (;nbuf--;) 
	{
        *di++=(*si++);
    }
    return 0;
}

/*FUNCTION_HEADER**********************
 * NAME:	;hatsuon_mora
 * DESC: 	pronunciation ... mora
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
int hatsuon_mora(uint8_t* ptr, uint8_t* buf)
{
    int n; uint8_t *top,len,a[3],ch;
    for (top=buf,len=0,buf++;*ptr;) 
	{
        a[0]=(*ptr); 
		a[1]=(*(ptr+1)); 
		a[2]=(*(ptr+2));
        /*
		*   ptrが文字列を越えていると、0x00が入ってしまう。
		* そのとき、マッチングが異常終了し、nが１多くなって、ptrがずれる。
		* すると、*ptrが0x00でなくなって、ループが終了しなくなる。
		*   そうならないように、0x00から後を0xFFで詰めておく。
		* このルーチンは辞書登録でのみコールされる為、
		* スピードは考慮しないで良い。
		*/
        if(a[1]==0x00) 
		{
            a[1]=a[2]=0xFF;
        } 
		else 
		{
            if (a[2]==0x00) 
			{
                a[2]=0xFF;
            }
        }
        for(n=3;n;) 
		{
            if(dispchar_to_code(a, &ch)) 
			{
                ptr += n; 
				goto hatsuon_mora100;
            }
            a[--n]=0x00;
        }
        return 0;
hatsuon_mora100:
		*buf++=ch; 
		++len;
    }
    *top=len;
    return 1;
}


/*FUNCTION_HEADER**********************
 * NAME:	;u_read
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
static int u_read(int page, size_t size, void *addr)
{
    if (lseek(jfd,((long)(page-1)*(long)PAGESIZE),0)==-1) 
	{
        perror("seek error");
        return errno;
    }
    if(read(jfd,addr,size*PAGESIZE)==-1) 
	{
        perror("read error");
        return errno;
    }
    return 0;
}

/*FUNCTION_HEADER**********************
 * NAME:	;u_write
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
static int u_write(int page, size_t size, void* addr)
{
    if(lseek(jfd,((long)(page-1)*(long)PAGESIZE),0)==-1) 
	{
        perror("seek error");
        return errno;
    }
    if(write(jfd,addr,size*PAGESIZE)==-1) 
	{
        perror("write error\n");
        return errno;
    }
    return 0;
}

/*FUNCTION_HEADER**********************
 * NAME:	;u_alloc
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
static int u_alloc(void** addr, size_t size)
{
    size_t pages = size*PAGESIZE;
    if((*addr=malloc(pages))==NULL) 
	{
        fprintf(stderr, "alloc error:%d\n", pages);
        return errno;
    }
    return 0;
}

/*FUNCTION_HEADER**********************
 * NAME:	;u_dealloc
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
static void u_dealloc(void** addr)
{
    free(*addr);
    *addr = NULL;
}
#if 0 // cant be here like this
/*FUNCTION_HEADER**********************
 * NAME:	;u_stop
 * DESC: 	function that discontinues conversion
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
static int u_stop(void*p) 
{
    return 0;
}

/*FUNCTION_HEADER**********************
 * NAME:	;load_dictionary
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
int load_dictionary( LPTTS_HANDLE_T phTTS, void **dict_index, void **dict_data,
				     unsigned int *dict_siz, unsigned int *dict_bytes, 
#ifdef CHEESY_DICT_COMPRESSION
					  void **dict_fc_entry, unsigned int *dict_fc_entries, 
#endif
					  char *dict_nam,
					  BOOL bRequired, BOOL bReportToWindow,
					  DT_HANDLE *dicMapObject,				// Handle for mapped object
					  DT_HANDLE *dicFileHandle,				// File Handle	
					  LPVOID *dicMapStartAddr,				// Starting address of mapped view
					  MEMMAP_T dict_map)
{
	int status;
	PLTS_T pLts_t=phTTS->pLTSThreadData;

	strcpy(phTTS->dictionary_file_name, "onsei.dic");
	phTTS->pKernelShareData->fdic_entries[phTTS->cur_lts_lang]++;

	status = onsei_init(phTTS->dictionary_file_name, pLts_t->j_inbuf, pLts_t->j_talkbuf, (uint16_t *)pLts_t->j_sentbuf, 0, u_stop);
	if( status == Err_Fatal_DIC )
		return MMSYSERR_INVALPARAM;
	else
		return MMSYSERR_NOERROR;

}

/*FUNCTION_HEADER**********************
 * NAME:	;unload_dictionary
 * DESC: 	
 * IN:		
 * OUT:		
 * RETURN:	
 * NOTES:	
 *END_HEADER***************************/
void unload_dictionary( void **dict_index, void **dict_data, unsigned int *dict_siz,
						unsigned int * dict_bytes, LPVOID *dicMapStartAddr, DT_HANDLE *dicMapObject,
						DT_HANDLE *dicFileHandle, MEMMAP_T dict_map )
{
	onsei_term();
}
#endif
/* End of stdonsei.c */
