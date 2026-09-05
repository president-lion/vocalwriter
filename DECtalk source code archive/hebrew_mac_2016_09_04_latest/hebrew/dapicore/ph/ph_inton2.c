/* ww_02_27_01
 ***********************************************************************
 *
 *                           Copyright ©
 *    Copyright © 2002 Fonix Corporation. All rights reserved.
 *    Copyright © 2000-2001 Force Computers, Inc., a Solectron company. All rights reserved.
 *    © SMART Modular Technologies 1999, 2000. All rights reserved.
 *    © Digital Equipment Corporation 1996, 1997, 1998, 1999 All rights reserved.
 *
 *     U.S. Government Rights: Consistent with FAR 12.211 and 12.212, 
 *     Commercial Computer Software, Computer Software Documentation, 
 *     and Technical Data for Commercial Items are licensed to the U.S. 
 *     Government under vendor's standard commercial license.
 *
 *
 *    This software is proprietary to and embodies the confidential
 *    technology of Fonix Corporation and other parties.
 *    Possession, use, or copying of this software and media is authorized
 *    only pursuant to a valid written license from Fonix or an
 *    authorized sublicensor.
 *
 ***********************************************************************
 *    File Name:    ph_inton.c
 *    Author:
 *    Creation Date:
 *
 *    Functionality:
 *    FUNDAMENTAL FREQUENCY RULES
 *
 ***********************************************************************
 *    Revision History:
 * Rev  Who     Date            Description
 * ---  -----   -----------     -------------------------------------------- 
 * 0001 DK 		12/16/1984		Initial creation
 * 0002 DGC		12/27/1984		Version for 80186, use mstofr()
 * 0003 DK		01/09/1985		Fix handling of user inputted f0 commands
 * 0004 DK		01/21/1985		Fix bug causing f0 to drift down in long sentences
 * 0005 DK		03/01/1985		Reduce pitch gestures in a clause ended by "?"
 * 0006 DK		04/18/1985		Try releasing final nasals into silence for intellig.
 * 0007 DK		04/25/1985		Fix continuation rise so not too early
 * 0008 DK		05/17/1985		F0 fall cannot occur too early in a long vowel
 * 0009 DK		06/14/1985		Fix f0 fall for f0mode=HAT_LOCATIONS_SPECIFIED
 * 0010 DK		06/19/1985		Fix insertion of dummy-vowel phoneme
*/
/* #define MSDBG4 */
#include "dectalkf.h"


//BTS10192 Fixes some phoneme based on MRT
#include "ph_def.h"

#define BEFORE_HAT_RISE			0
#define ON_TOP_OF_HAT			1
#define AFTER_FINAL_FALL		2
#define AFTER_NONFINAL_FALL		3
#define DONTKNOW				0
#define QUESTCLAUSE				1
#define VERBPHRASE				2
#define PERIODCLAUSE			3

/* 
* Duration of the "dummy vowel" inserted after a clause-final stop.
*/
/* #define  DUMMY_V_DUR NF25MS      -- seems a bit long */
#define DUMMY_V_DUR   NF15MS



/***************************************************************************/
/* MVP : The following extern variables are now become elements of instance */
/* specific PH thread data structure DPH_T.                                */
/* extern short allophons[];    Integer rep of phonetic string             */
/* extern short allofeats[];    Structural features                        */
/* extern short allodurs[];     Duration in frames for each phone          */
/* extern short nallotot;       Number of phones in phonetic string        */
/* extern short arg1,arg2,arg3; Used for muldv(pDph_t->arg1,pDph_t->arg2,pDph_t->arg3) */
/* extern short *user_f0;       User-specified f0 commands, optional       */
/* extern short *user_offset;   User-specified f0 command time offset      */
/* extern FLAG  newparagsw;    Make f0 higher initially if =1              */
/* Set to 0 here after raising f0              							   */
/* extern short f0mode;        State variable determine if rules used      */
/* extern short cbsymbol;      Equals QUEST if clause ends in ?            */
/*   1/17/99 EAB Note cbcymbol is either 0 or the phonemic value of a ?    */
/*	so if(cbsymbol == Quest) and if(cbsymbol) are equivalent               */
/* extern short assertiveness; Speaker def parameter                       */
/* extern short size_hat_rise; Speaker def parameter                       */
/* extern short scale_str_rise;Speaker def parameter                       */
/* extern short f0tar[];       F0 target commands, in Hz*10                */
/* extern short f0tim[];       Times between commands, in frames           */
/* extern short nf0tot;        Number of commands for cur clause           */
/***************************************************************************/

/* Input variables:                                                    */

/* Output variables:                                                   */

/* TABLES located in PH_ROM.C                                          */

//extern short f0_stress_level[];			/* F0 rise as f(stress-level)  	 */

//extern __inline short phone_feature(PDPH_T pDph_t, int phone);	   /* Phonetic features            */
//extern __inline short begtyp(int phone);
//extern short *begtyp;

#define phone_feature(a,b) (pDph_t->cur_speaker[(b)>>8]->featb[(b)&0x00FF])
#define begtyp(b) (pDph_t->cur_speaker[(b)>>8]->begtyp[(b)&0x00FF])

/* MVP : Static function declarations */
void make_f0_command (LPTTS_HANDLE_T phTTS,short type, short rulenumber, short tar, short delay,
					  short length,short *psCumdur,short nphon);
void do_by_rule_command(LPTTS_HANDLE_T phTTS,short *mf0, short *cumdur,short nphon);
void do_british_inton(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon, short *n_last_syl_nuc);
void do_italian_stress(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon,short *targf0);
void do_onset_conditions(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon);
void do_phrase_dip(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon);
void do_phrase_final(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon);
void do_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon);
void do_sp_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon);
short firstfeature(LPTTS_HANDLE_T phTTS, short fea1,short fea2,short nseg);

#define EMPH_FALL		  1				/* stress reduce shift for emph. stress 	*/
#define DELTAEMPH_SPEC  505				/* special value for fast emphatic str 	*/
#define DELTAEMPH		501				/* normal value for emphatic stress 		*/
#define DELTARISE		200				/* rise for continuing cadence */
#define DELTAFINAL		100				/* delta to remain at top in final cad. */
#define FINAL_FALL		  1				/* stress reduce shift for str. at top */


					  /*nd, ... accent in a phrase
					  * Check against MAX_NRISES in phinton.c
*/

const short gr_f0_mphrase_position[] =
{
	/* First clause    Second clause  */
	/* 300, 275, 250, 200, 175, 150, 50,50 last pos=0 causes bad problems */
		160, 110, 90, 85, 75, 65 , 65, 65};
	const short gr_f0_fphrase_position[] =
	{
		/* First clause    Second clause  */
		/* 300, 275, 250, 200, 175, 150, 50,50 last pos=0 causes bad problems */
		120, 110, 90, 85, 75, 65 , 65,65};
		/* 
		* F0 rise as f(stress-level); Order is
		* FNOSTRESS  unstressed
		* FSTRESS_1  primary stress
		* FSTRESS_2  secondary stress
		* FEMPHASIS  emphatic stress
		*/
		
		/* 0,       71,     31,    261 eab original stress */
		
		const short gr_f0_mstress_level[] =
		{
			/* 'stress `stress "stress                      */
		0, 81, 51, 261					   /* eab original stress */
				
		};
		const short gr_f0_fstress_level[] =
		{
			/* 'stress `stress "stress                      */
		0, 111, 91, 261					   /* eab original stress */
				
		};
		
		/* Stress-related rise/fall amount in Hz*10 for first, second, ... accent *  in a phrase */
		
		//BATS 776 EAB 10/22/98 add values to arrays
		

	const short us_f0_mphrase_position[] = 
	{ 120,  90,  60,  40,  20,  0,  0,  0};
		
	const short us_f0_mstress_level[] =
		{1, 81, 61, 261 };		
	const short us_f0_fphrase_position[] =
	{  210,  90,  60,  40,  0,  0,  0,  0 };
		
	const short us_f0_fstress_level[] =	{1, 80, 70, 161};
		
	const short he_f0_mphrase_position[] = 
	{ 220,  100,  90,  80,  60,  60,  60,  60};
		
	const short he_f0_mstress_level[] =
		{1, 141,91, 261 };		
	const short he_f0_fphrase_position[] =
	{  260, 120,  90,  80,  80,  60,  60,  60 };
		
	const short he_f0_fstress_level[] =	{1, 80, 70, 161};
		


		
		const short uk_f0_mphrase_position[] = { 100,  90,  80,  60,  40,  20,  0,  0 };
		const short uk_f0_mstress_level[] = { 1,  85,  51,  190,  0,  0,  0,  0 };
		const short uk_f0_fphrase_position[] = { 100,  90,  80,  60,  40,  20,  0,  0 };
		const short uk_f0_fstress_level[] = { 1,  90,  68,  210,  0,  0,  0,  0 };
			
		const short it_f0_mphrase_position[] =
		{
			
			100, 100, 90, 85, 80, 75 , 70, 65  
				
		};
		
		/* 
		* F0 rise as f(stress-level); Order is
		*      FNOSTRESS       unstressed
		*      FSTRESS_1       primary stress
		*      FSTRESS_2       secondary stress
		*      FEMPHASIS       emphatic stress
		*/
		
		const short it_f0_mstress_level[] =
		{
			/* 'stress `stress "stress                      */
			0, 181, 81, 281					   /* eab original stress */
				
		};
		/* 
		* Stress-related rise/fall amount in Hz*10 for
		* first, second, ... accent in a phrase
		* Check against MAX_NRISES in phinton.c
		*/
		
		const short it_f0_fphrase_position[] =
		{
			//eab gafingerpoken 9/21/04
			130, 120, 110, 100, 90, 80 , 70, 60 
		};
		const short it_f0_fstress_level[] =
		{
			/* 'stress `stress "stress                      */
			0, 251, 91, 291	
		};
		
		
		const short la_f0_mphrase_position[] =
		{
			
			/* First clause    Second clause  */
			/* 300, 275, 250, 200, 175, 150, 50,50 last pos=0 causes bad problems */
			
	 180, 160, 140, 120, 120, 80 , 70, 60  
				
		};
		
		/* 
		* F0 rise as f(stress-level); Order is
		*      FNOSTRESS       unstressed
		*      FSTRESS_1       primary stress
		*      FSTRESS_2       secondary stress
		*      FEMPHASIS       emphatic stress
		*/
		
		
		/* 0,       71,     31,    261 eab original stress */
		
		const short la_f0_mstress_level[] =
		{
			/* 'stress `stress "stress                      */
	0, 161, 81, 261					   /* eab original stress */
				
		};
		/* 
		* Stress-related rise/fall amount in Hz*10 for
		* first, second, ... accent in a phrase
		* Check against MAX_NRISES in phinton.c
		*/
		
		const short la_f0_fphrase_position[] =
		{
			/* First clause    Second clause  */
	220, 190, 160, 155, 140, 100, 90	   /* last pos=0 causes bad problems */
				
		};
		
		/* 
		* F0 rise as f(stress-level); Order is
		*      FNOSTRESS       unstressed
		*      FSTRESS_1       primary stress
		*      FSTRESS_2       secondary stress
		*      FEMPHASIS       emphatic stress
		*/
		const short la_f0_fstress_level[] =
		{
			/* 'stress `stress "stress                      */
  	0, 151, 81, 261	
				
				
		};
		
		/* F0 rise as f(stress-level); Order is unstr, primary, secondary, emphasis */
		
		/* WARNING eab f0_stress_level + f0_phrase_pos must add up to an odd number or you will be creating a step function instead of the desired IMPULSE
		* function */
		
		//const short f0_stress_level [] = { 01, 71, 51, 281 };
		const short sp_f0_mphrase_position[] =
		{
			/* First clause    Second clause  */
			/* 300, 275, 250, 200, 175, 150, 50,50 last pos=0 causes bad problems */
			//300,275, 250, 200, 175, 150, 50, 50	   /* last pos=0 causes bad problems */
	211,160,60, 40, 40, 40, 40, 40
		};
		
		/* 
		* F0 rise as f(stress-level); Order is
		*      FNOSTRESS       unstressed
		*      FSTRESS_1       primary stress
		*      FSTRESS_2       secondary stress
		*      FEMPHASIS       emphatic stress
		*/
		
		
		/* 0,       71,     31,    261 eab original stress */
		
		const short sp_f0_mstress_level[] =
		{
			/* 'stress `stress "stress                      */
			/* 0,       51-10,     31-10,    161-50 */   /*eab  */
	0, 101, 61, 281					   /* eab original stress */
		};
		/* 
		* Stress-related rise/fall amount in Hz*10 for
		* first, second, ... accent in a phrase
		* Check against MAX_NRISES in phinton.c
		*/
		const short sp_f0_fphrase_position[] =
		{
			/* First clause    Second clause  */
/* 300, 275, 250, 200, 175, 150, 50,50 last pos=0 causes bad problems */
	200, 180, 160, 160, 150, 150, 140	   /* last pos=0 causes bad problems */
	//140,105, 90, 70, 60, 50, 50, 50
		};
		/* 
		* F0 rise as f(stress-level); Order is
		*      FNOSTRESS       unstressed
		*      FSTRESS_1       primary stress
		*      FSTRESS_2       secondary stress
		*      FEMPHASIS       emphatic stress
		*/
		/* 0,       71,     31,    261 eab original stress */
		const short sp_f0_fstress_level[] =
		{
			/* 'stress `stress "stress                      */
			/* 0,       51-10,     31-10,    161-50 */  /*eab  */
	0, 160, 81, 251					   /* eab original stress */
				
		};
		
		
		
		/*
		*      Function Name: phinton()      
		*
		*  	Description: 
		*
		*      Arguments: LPTTS_HANDLE_T phTTS		Text-To-speech handle
		*
		*      Return Value: 
		*
		*      Comments:
		*
		*/
		/* GL 03/25/1998,  BATS#639 use phTTS argument instead of pDph_t */
		void phinton (LPTTS_HANDLE_T phTTS)
		{
			PKSD_T	pKsd_t = phTTS->pKernelShareData;
			PDPH_T	pDph_t = phTTS->pPHThreadData;
			PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;

			int		temp;
			short F0_CBOUND_PULSE;
			short SCHWA1;
			short SCHWA2;
			short F0_QGesture1;
			short  F0_QGesture2;
			short F0_CGesture1;
			short F0_CGesture2;
			short GEST_SHIFT;
			short  MAX_NRISES;
			short F0_FINAL_FALL;
			short F0_NON_FINAL_FALL;
			short  F0_QSYLL_FALL;
			short F0_GLOTTALIZE;
			short Reduce_last;
			short F0_COMMA_FALL;
			
			
			
			short *f0_mstress_level;
			short *f0_fstress_level;
			short *f0_mphrase_position;
			short *f0_fphrase_position;
			short phrase_pos[6]= {350,150,130,100,70,50};			
			
			/* eab 2/26/97 EAB Group hardcoded items into one file to be 
			handled differently later */
			/* Note F0_Final _fall is the number of hertz *10 that it
			will try to fall below the baseline on a declarative stressed final syllable.
			*/
			
			
			
			
			
			/* Automatic variables */
			short nphon = 0, mf0 = 0,tmp=0, vot;
			short length=0;
			short n_last_syl_nuc;
			short pholas = 0, struclas = 0;
			int		fealas = 0;
			U32  struccur = 0, feacur = 0;
			U32  stresscur = 0;
			short phonex = 0, strucnex = 0;
			int		feanex = 0;
			short targf0 = 0, delayf0 = 0,vowdelay=0;
			short f0fall = 0;		/* Extra fall below baseline at end of clause */
			short nphonx = 0,step_time;		/* short temp is never used MVP */
			short f0_incr =0 ,cumdur = 0, phocur = 0;		/* MVP : made local */
			short stepcount;
			short inputscrewup = 0;	/* MVP : was of type FLAG */
			short nextsylbou =0 ,nextwrdbou =0 ,nextphrbou =0;
			short issubclause = 0;   /* TRUE signals subordinate clause */
			short NotQuest = 1;
			short n=0;
			short oddeven = -1;
			short f0_accent_table[] = { 250, 150, 100, 80, 70 };
			pDph_t->hatpos = 0;
			pDph_t->delta_special=0;
			pDphsettar->nrises_sofar = 0;
			pDphsettar->number_halves = 0;
			pDphsettar->hatsize = 0;
			pDphsettar->hat_loc_re_baseline = 0;
			
			/* Beginning of initialization */
			//	pDph_t->commacnt =0; //BATS709
			pDph_t->last_sonorant = 999;
			inputscrewup = FALSE;
			cumdur = 0;
			stepcount=0;
			pDph_t->had_hatbegin=0;
			pDph_t->had_hatend=0;
			pDph_t->in_phrase_final = 0;
			pDph_t->nf0tot = 0;
			pholas = GEN_SIL;
			fealas = phone_feature( pDph_t,GEN_SIL);
			struclas = 0;
			mf0 = 0;
			pDph_t->prevtargf0 = -1; /* EAB 1/13/98 Want first target to end up plus
			also initalize completion flag */
			pDph_t->done =0;
		
			
			
			if(pKsd_t->lang_curr == LANG_german)
			{
				f0_mstress_level = (short *)gr_f0_mstress_level;
				f0_fstress_level = (short *)gr_f0_fstress_level;
				f0_mphrase_position = (short *)gr_f0_mphrase_position;
				f0_fphrase_position = (short *)gr_f0_fphrase_position;
				
				
				
				SCHWA1 = GRP_EX;
				SCHWA2 = GRP_EX;
				F0_QGesture1 = -120;
 F0_QGesture2 = 451;
				F0_CGesture1 = -120;
 F0_CGesture2 = 140;
				GEST_SHIFT = 1;
				MAX_NRISES = 6;
				F0_FINAL_FALL = 60;
				F0_NON_FINAL_FALL = 80;
				F0_QSYLL_FALL	= 80;
				F0_GLOTTALIZE  = -100;
				Reduce_last = 50;
				
			}
			else if(pKsd_t->lang_curr == LANG_latin_american)
			{
				f0_mstress_level = (short *)la_f0_mstress_level;
				f0_fstress_level = (short *)la_f0_fstress_level;
				f0_mphrase_position = (short *)la_f0_mphrase_position;
				f0_fphrase_position = (short *)la_f0_fphrase_position;
				
				
				F0_CBOUND_PULSE  = 700;
				F0_GLOTTALIZE  = -60; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				SCHWA1 = LAP_IX;
				SCHWA2 = LAP_IX;
 F0_QGesture1 = -180;
 F0_QGesture2 = 450;
				F0_CGesture1 = -81;
				F0_CGesture2  = 171;
				GEST_SHIFT = 2;
				MAX_NRISES  = 6;
				F0_FINAL_FALL  =	140;
				F0_NON_FINAL_FALL  =	150;
				F0_QSYLL_FALL	= 80;
				Reduce_last = 70; 
				
			}
			else if(pKsd_t->lang_curr == LANG_spanish)
			{
				f0_mstress_level = (short *)sp_f0_mstress_level;
				f0_fstress_level = (short *)sp_f0_fstress_level;
				f0_mphrase_position = (short *)sp_f0_mphrase_position;
				f0_fphrase_position = (short *)sp_f0_fphrase_position;
				
				
				F0_CBOUND_PULSE = 700;
				F0_GLOTTALIZE   = -60; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				SCHWA1 = SPP_IX;
				SCHWA2 = SPP_IX;
 F0_QGesture1 = -220;
 F0_QGesture2 = 450;
				F0_CGesture1 = -81;
				F0_CGesture2 = 171;
				GEST_SHIFT = 2; 
				MAX_NRISES = 6;
				F0_FINAL_FALL = 140;
				F0_NON_FINAL_FALL = 150;
				F0_QSYLL_FALL	 = 80;
				Reduce_last = 70;
				
			}
			
			else if(pKsd_t->lang_curr == LANG_english)
			{ 
				
				
				f0_mstress_level = (short *)us_f0_mstress_level;
				f0_fstress_level = (short *)us_f0_fstress_level;
				f0_mphrase_position = (short *)us_f0_mphrase_position;
				f0_fphrase_position = (short *)us_f0_fphrase_position;
				SCHWA1 = USP_AX;
				SCHWA2 = USP_IX;
 F0_QGesture1 = -91;
 F0_QGesture2 = 401;
				F0_CGesture1 = -171;
				F0_CGesture2 = 250;
				GEST_SHIFT = 1;
				MAX_NRISES   = 7;
   F0_FINAL_FALL  =	120;
				F0_NON_FINAL_FALL  = 150;
				F0_COMMA_FALL	= 120;
				F0_QSYLL_FALL	= 80;
				F0_GLOTTALIZE   = -60; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				Reduce_last = 0;
				
				
				
				
			}
			else if(pKsd_t->lang_curr == LANG_british)
			{ 
				f0_mstress_level = (short *)uk_f0_mstress_level;
				f0_fstress_level = (short *)uk_f0_fstress_level;
				f0_mphrase_position = (short *)uk_f0_mphrase_position;
				f0_fphrase_position = (short *)uk_f0_fphrase_position;
				
				SCHWA1 = UKP_AX;
				SCHWA2 = UKP_IX;
				F0_QGesture1 = -151;
				F0_QGesture2 = 351;
				F0_CGesture1 = -50;
				F0_CGesture2 = 220;
				GEST_SHIFT = 1;
				MAX_NRISES  = 6; 
				F0_FINAL_FALL  = 160;
				F0_NON_FINAL_FALL  = 150;
				F0_QSYLL_FALL	= 80;
				F0_GLOTTALIZE  = -10; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				F0_COMMA_FALL	 = 120;
				Reduce_last = 50;
				
			}
	else if(pKsd_t->lang_curr == LANG_italian)
			{ 
				f0_mstress_level = (short *)it_f0_mstress_level;
				f0_fstress_level = (short *)it_f0_fstress_level;
				f0_mphrase_position = (short *)it_f0_mphrase_position;
				f0_fphrase_position = (short *)it_f0_fphrase_position;
				
				SCHWA1 = ITP_AX;
				SCHWA2 = ITP_AX;
				F0_QGesture1 = -151;
				F0_QGesture2 = 351;
				F0_CGesture1 = -50;
				F0_CGesture2 = 220;
				GEST_SHIFT = 1;
				MAX_NRISES  = 6; 
				F0_FINAL_FALL  = 160;
				F0_NON_FINAL_FALL  = 150;
				F0_QSYLL_FALL	= 80;
				F0_GLOTTALIZE  = -10; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				F0_COMMA_FALL	 = 120;
				Reduce_last = 50;
				
			}

			else if(pKsd_t->lang_curr == LANG_korean)
			{ 
				f0_mstress_level = (short *)it_f0_mstress_level;
				f0_fstress_level = (short *)it_f0_fstress_level;
				f0_mphrase_position = (short *)it_f0_mphrase_position;
				f0_fphrase_position = (short *)it_f0_fphrase_position;
				
				SCHWA1 = ITP_AX;
				SCHWA2 = ITP_AX;
				F0_QGesture1 = -151;
				F0_QGesture2 = 351;
				F0_CGesture1 = -50;
				F0_CGesture2 = 220;
				GEST_SHIFT = 1;
				MAX_NRISES  = 6; 
				F0_FINAL_FALL  = 160;
				F0_NON_FINAL_FALL  = 150;
				F0_QSYLL_FALL	= 80;
				F0_GLOTTALIZE  = -10; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				F0_COMMA_FALL	 = 120;
				Reduce_last = 50;
				
			}
			else if(pKsd_t->lang_curr == LANG_chinese)
			{ 
				f0_mstress_level = (short *)it_f0_mstress_level;
				f0_fstress_level = (short *)it_f0_fstress_level;
				f0_mphrase_position = (short *)it_f0_mphrase_position;
				f0_fphrase_position = (short *)it_f0_fphrase_position;
				
				SCHWA1 = CHP_A;
				SCHWA2 = CHP_A;
				F0_QGesture1 = -151;
				F0_QGesture2 = 351;
				F0_CGesture1 = -50;
				F0_CGesture2 = 220;
				GEST_SHIFT = 1;
				MAX_NRISES  = 6; 
				F0_FINAL_FALL  = 160;
				F0_NON_FINAL_FALL  = 150;
				F0_QSYLL_FALL	= 80;
				F0_GLOTTALIZE  = -10; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				F0_COMMA_FALL	 = 120;
				Reduce_last = 50;
				
			}
			else if(pKsd_t->lang_curr == LANG_hebrew)
			{ 
				
				
				f0_mstress_level = (short *)he_f0_mstress_level;
				f0_fstress_level = (short *)he_f0_fstress_level;
				f0_mphrase_position = (short *)he_f0_mphrase_position;
				f0_fphrase_position = (short *)he_f0_fphrase_position;
				SCHWA1 = HEP_AX;
				SCHWA2 = HEP_AX;
				F0_QGesture1 = -91;
				F0_QGesture2 = 401;
				F0_CGesture1 = -171;
				F0_CGesture2 = 250;
				GEST_SHIFT = 1;
				MAX_NRISES   = 7;
				F0_FINAL_FALL  =	120;
				F0_NON_FINAL_FALL  = 150;
				F0_COMMA_FALL	= 120;
				F0_QSYLL_FALL	= 80;
				F0_GLOTTALIZE   = -60; //New method that drops per cycle rather than impulse BATS 796 EAB 11/4/98
				Reduce_last = 0;
				
				
				
				
			}


	
	/* Should set pDphsettar->nrises_sofar to zero after a ph_init=0 hard reset */
	/* End of initialization */
	
	if (pKsd_t->lang_curr == LANG_british)
		/* Find last syllable nucleus*/
		/* EAB Part of BATS 600 nphon->n_last_syl_nuc*/
		for(n_last_syl_nuc=pDph_t->nallotot;n_last_syl_nuc > 1;n_last_syl_nuc--)
		{
			if ((phone_feature( pDph_t,pDph_t->allophons[n_last_syl_nuc]) & FSYLL) IS_PLUS)
				break;
		}
//fill in vot arrray can't wait til later
		
		
		for (nphon = 1; nphon < pDph_t->nallotot; nphon++)
		{
			vot = 0;
			if((phone_feature( pDph_t,pDph_t->allophons[nphon-1]) & FPLOSV) IS_PLUS)
			{
				if (( ((phone_feature( pDph_t,pDph_t->allophons[nphon-1]) & FVOICD) IS_MINUS)
					&& ((phone_feature( pDph_t,pDph_t->allophons[nphon]) & FSONOR) IS_PLUS ))) 
				{
			
											   
				vot = NF60MS; /* Asp dur for /p,t,k/ before stressed sonor */
				if(pDph_t->number_words == 1)
					//In citing lengthen vot
				{
					vot = NF60MS;
				
				}
				
				if ( nphon > 2 && pDph_t->allophons[nphon -2]== USP_S)          
				{
					/* Plosive is in an [s] cluster */
					if ((pDph_t->allofeats[nphon - 2] & FBOUNDARY) IS_MINUS )
					{
						vot = NF15MS;	   
					}
				}
				else if ((( phone_feature( pDph_t,pDph_t->allophons[nphon])& FSYLL) IS_MINUS))
				{
					vot += NF20MS;			   /* Vot longer in a sonorant consonant */
				}
				
#ifdef FASTTYPING_MODE
				if(phTTS->bInTypingMode == TRUE || pKsd_t->sprate >300)
				{
					
					vot=vot>>2;
					if(vot == 0)
						vot=1;
				}
#endif


		//handle unstressed
		 if ((pDph_t->allofeats[nphon] & FSTRESS) IS_MINUS)
		{
			if ((pDph_t->allofeats[nphon] & FWINITC) IS_PLUS)
				vot = NF40MS;
			else
			{
			PAP.pspesh -= 3;
			vot = 2;
			}
		}


	 if ((pDph_t->allofeats[nphon] & FSTRESS) IS_MINUS)
		{
			if ((pDph_t->allofeats[nphon-1] & FWINITC) IS_PLUS)
				vot = NF40MS;
			else
			{
			PAP.pspesh -= 3;
			vot = 2;
			}
			vot = 1;
		}


				//pDph_t->vot_time[nphon] = vot;
				
				
				}
				
			}
			pDph_t->vot_time[nphon] = vot;
		}

		
		
		/* MAIN LOOP, for each output phoneme */
		
		for (nphon = 0; nphon < pDph_t->nallotot; nphon++)
		{

			if (nphon > 0)
			{
				pholas = pDph_t->allophons[nphon - 1];
				struclas = pDph_t->allofeats[nphon - 1];
				fealas = phone_feature(pDph_t,pholas);
			}
			
			phocur = pDph_t->allophons[nphon];
			struccur = pDph_t->allofeats[nphon];
			
			
			
			stresscur = struccur & FSTRESS;
			feacur = phone_feature( pDph_t,phocur);
			if (nphon < (pDph_t->nallotot - 1))
			{
				phonex = pDph_t->allophons[nphon + 1];
				strucnex = pDph_t->allofeats[nphon + 1];
				feanex = phone_feature( pDph_t,phonex);
			}
if(pKsd_t->lang_curr == LANG_japanese)	
{

/*	  Reset number of accent rises sofar if at very end of sentence */
	    if ((struccur &FBOUNDARY) > FPERNEXT )
		{
		pDphsettar->nrises_sofar = 0;
	    }

/*	  Soft reset number of accent rises sofar if at end of a phrase */
	    if ((struccur &FBOUNDARY) > FPPNEXT )
		{
		if (pDphsettar->nrises_sofar > 1)    pDphsettar->nrises_sofar = 1;
	    }

/* RULE 1: Incremental rise (and fall) greater at beginning of a phrase */

	    if ((struccur & FRISE) IS_PLUS) {
/*	      Set f0 increment for this accent rise and comming accent fall */
		f0_incr = f0_accent_table[pDphsettar->nrises_sofar];

/*	      Accent rise attenuated if not first rise of phrase */
		if (pDph_t->hatpos != 0)  f0_incr -= 100;

/* RULE 2: Strengthen f0 gesture if [/] or [\] in input string */
		if((struccur & FSTRESS) == FSTRONG)
		{
		    f0_incr += 250;
		}

/*	      Increment counter of number of accent rises sofar */
		if (pDphsettar->nrises_sofar < 4)    pDphsettar->nrises_sofar++;
/*	      Remember that next rise is phrase-internal */
		pDph_t->hatpos++;

/* RULE 3: If rise and fall on same syllable, start rise early and delay fall */

/*	      Time of rise depends on whether there is an impending fall */
/*	      Also start early if rise occurs on syllabic nasal */
		if ((struccur & FFALL) IS_PLUS) {
		    step_time = - NF60MS;
		}
		else if ((feacur & FNASAL) IS_PLUS) {
		    step_time = - NF40MS;
		}
		else    
			step_time = 0;

		make_f0_command ( phTTS,GLIDE,921,f0_incr,step_time+4,pDph_t->allodurs[nphon]-4, &cumdur,nphon);
	    }
		
		/*	  F0 fall if accent fall */
		if ((struccur & FFALL) IS_PLUS) {
			/*	      Time  of fall depends on whether there was a rise on syll too */
			if ( ((struccur & FRISE) IS_PLUS)
				&& (phonex != GEN_SIL) ) {
				step_time = NF60MS;
			}
			else {
				step_time = 0;
			}
			
			/*	      Execute if FEA1 seen in input string before FEA2 is seen */
			if (firstfeature(phTTS,(FPERNEXT|FCBNEXT|FPPNEXT|FQUENEXT),
				(FRISE|FFALL),nphon)) {
				if (pDph_t->hatpos != 1)    f0_incr += 100;
				pDph_t->hatpos = 0;
			}
			else {
				if (pDph_t->hatpos == 1)    f0_incr -= 100;
			}
			
			/* RULE 4: Make fall go 10 Hz below baseline on last syll before period */
			
			if ((struccur & FBOUNDARY) == FPERNEXT) {
				f0_incr += 100;
			}
			make_f0_command ( phTTS,GLIDE,944,-f0_incr,step_time,pDph_t->allodurs[nphon]-4, &cumdur,nphon);
		}
		else {
			/*	      Make fall go 10 Hz below baseline if final SYLL before period */
			if (((struccur & FBOUNDARY) == FPERNEXT))
				if ((feacur & FSYLL) IS_PLUS)
				if ((feacur & FNASAL) IS_MINUS)  {
				make_f0_command ( phTTS, GLIDE,945,-100,0,9, &cumdur,nphon);
			}
		}

/* RULE 5: Make F0 go up 80 ms before end of vowel for a question */

		if (((struccur & FBOUNDARY) == FQUENEXT) 
	      && ((feacur & FSYLL) IS_PLUS)
	      && ((feacur & FNASAL) IS_MINUS) ) {
		make_f0_command ( phTTS,GLIDE,966,200,(pDph_t->allodurs[nphon]-NF80MS),pDph_t->allodurs[nphon], &cumdur,nphon);
	    }
	if ((phocur == GEN_SIL) && (nphon != 0)) {
		//make_f0_command(0,NF25MS);		/* 0 Hz is a signal */
		pDph_t->hatpos = 0;
	    }

	/* Update cumdur to time at end of current phone */
				cumdur += pDph_t->allodurs[nphon];


				if( (nphon <= (pDph_t->nallotot-1) &&
					(nphon > 0 && (pDph_t->allophons[nphon] & PVALUE)!= 0))
					|| nphon==0 ) //1st two lines check end of cluase
					//This counts inital silence (we don't vount final silence 
					/* EAB It turns out that there are two possibilites for the way things
					get transmitted one is with and end of clause symbols and the other is without(implied)
					therefore you have to do two checks to know whether or not your at the last real phoneme
					or at a silence phoneme BATS 897 */
					pDph_t->tcumdur += pDph_t->allodurs[nphon];





		pDph_t->hatstatel = pDph_t->hatstate;	/* Remember previous state */
		if (phocur == GEN_SIL) 
			pDph_t->hatstate = BEFORE_HAT_RISE;
}

else
{			
			
			/*	  Step 1, skip over word-initial consonants of this syllable */
			nphonx = nphon;
			while ((pDph_t->allofeats[nphonx] & FWINITC) IS_PLUS) {
				nphonx++;
			}
			/*	  Step 2, Look up boundary immediately after current syll */
			nextsylbou = pDph_t->allofeats[nphonx] & FBOUNDARY;
			/*	  Step 3, Try to make direct assignment of boundary after current word */
			if (nextsylbou >= FWBNEXT) {
				nextwrdbou = nextsylbou;
			}
			/*	  Step 4, Look ahead if this is not a word-final syllable */
			else {
				for (nphonx=nphonx+1;nphonx<pDph_t->nallotot; nphonx++) {
					if ((nextwrdbou=pDph_t->allofeats[nphonx]&FBOUNDARY)>=FWBNEXT) {
						goto wbfound;
					}
				}
			}
wbfound:
			/*	  Step 5, Try to make direct assignment of boundary after curr phrase */
			if (nextsylbou >= FCBNEXT) {				/* RSM To not count spurious FPPNEXT, FVPNEXT, and FRELNEXT */
				nextphrbou = nextwrdbou;
			}
			/*	  Step 6, Look ahead if this is not a phrase-final syllable */
			else {
				//wait this code is broken phonex+1
				
				
				for (nphonx=nphonx+1;nphonx<pDph_t->nallotot; nphonx++) {
					if ((nextphrbou=pDph_t->allofeats[nphonx]&FBOUNDARY)>FRELNEXT) {	
						goto fbfound;
					}
				}
			}
fbfound:
			
			/* Rule 0: User-specified commands for phoneme f0 targets or singing */
			
			if ((pDph_t->f0mode == PHONE_TARGETS_SPECIFIED)
				|| (pDph_t->f0mode == SINGING))
			{
				if (pDph_t->user_f0[nphon] != 0)
				{
					make_f0_command ( phTTS, USER,0, (short)(1000 + pDph_t->user_f0[nphon]), 0, 0, &cumdur,nphon); // NAL warning removal
				}
				goto skiprules;
			}
		

			if((struccur & FHAT_BEGINS) IS_PLUS  )
			{
				pDph_t->had_hatbegin= 1;
				pDph_t->in_phrase_final = 0;
				
			}
			/* eab 4/30/98  another oliver check I think hat falls are good*/
			
			if((struccur & FHAT_ENDS) IS_PLUS )
				pDph_t->had_hatend= 1;
			if((struccur & FHAT_ROOF) IS_PLUS )
				pDph_t->had_rf= 1;
			if((struccur & FHAT_DIP) IS_PLUS )
				if (stresscur != FEMPHASIS)
					do_phrase_dip( phTTS, &cumdur, nphon );
				
				if (pDph_t->f0mode == HAT_F0_SIZES_SPECIFIED)
				{
					//	do_by_rule_command(phTTS,&mf0,&cumdur,&nphon );
					if (pDph_t->had_hatbegin == 1
						|| pDph_t->had_hatend == 1
						|| pDph_t->had_rf == 1)
					{
						//command pending 
						if( (stresscur & FSTRESS_1 ) || pKsd_t->lang_curr == LANG_chinese)
							if (((feacur & FSYLL) IS_PLUS)
								||((feacur & FVOICD) && pDph_t->had_rf ))
								
							{
								while(((pDph_t->had_hatbegin == 1
									|| pDph_t->had_hatend == 1)
									&& ((feacur & FSYLL) IS_PLUS))
									//abort til fsyl if it was an rf command
									|| pDph_t->had_rf == 1)
								{
									if(pDph_t->had_rf)
										pDph_t->had_rf =0;
									else if(pDph_t->had_hatbegin)
										pDph_t->had_hatbegin =0;
									else if(pDph_t->had_hatend)
										pDph_t->had_hatend =0;
							
									if( pDph_t->user_f0[mf0])
									{
										targf0 = (pDph_t->user_f0[mf0] * 10);
										tmp = pDph_t->user_offset[mf0];
										delayf0=0;
										temp = nphon;
										//delayf0 = pDph_t->allodurs[nphon]*pDph_t->user_offset[mf0]/100;
										if(tmp < 0)
											delayf0 = pDph_t->allodurs[nphon-1]*tmp/100;
										else while(tmp > 0) 
										{
											if(tmp > 100)
												delayf0 += pDph_t->allodurs[temp];
											else
												delayf0 += pDph_t->allodurs[temp]*tmp/100;
											tmp -= 100;
											temp ++;
										}
#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows CE
	if (DT_DBG(PH_DBG,0x010))
									FNX_PRINTF("delay %d percent  = %d\n",pDph_t->user_offset[mf0],(int)(delayf0*6.4));
#endif
										tmp=pDph_t->user_length[mf0];
										length=0;
										temp = nphon;
										//delayf0 = pDph_t->allodurs[nphon]*tmp/100;
										if(tmp < 0)
											length = pDph_t->allodurs[nphon-1]*tmp/100;
										else while(tmp > 0) 
										{
											if(tmp > 100)
												length += pDph_t->allodurs[temp];
											else
												length += pDph_t->allodurs[temp]*tmp/100;
											tmp -= 100;
											temp ++;
										}	
										length -= delayf0;
#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows CE
	if (DT_DBG(PH_DBG,0x010))
										FNX_PRINTF("lenght %d percent  = %d \n",pDph_t->user_length[mf0],(int)(length*6.4));
#endif
										
										
										mf0++;
										make_f0_command( phTTS,GLIDE, 99, targf0,
											delayf0, length, &cumdur, nphon); // NAL warning removal
										pDphsettar->hatsize += targf0;
										
									}
									else
									mf0++;

								}
								goto skiprules;
							}
							
							
							else 
							{
								//command pending skip rules
								goto skiprules;
							}
					}
				}
				
				
				if (nphon == 0 )

					do_onset_conditions( phTTS, &cumdur, nphon );
				
				else if((feacur & FSYLL) IS_PLUS)
				{
					
					if(pKsd_t->lang_curr == LANG_british)
						do_british_inton(phTTS, &cumdur,nphon, &n_last_syl_nuc);
				else if(pKsd_t->lang_curr == LANG_korean)
					{
				//	if ((struccur & FBOUNDARY) == FQUENEXT)
				//		printf(" question \n");
					if((pDph_t->had_hatend != 1)
						||  pDph_t->allophons[nphon - 1] == GEN_SIL
						||  pDph_t->allophons[nphon - 2] == GEN_SIL)
					{
					
					
						//printf("wb found at %d \n", nphon);
						//targf0 = 150;
						targf0 = phrase_pos[pDphsettar->nrises_sofar];
						if(pDphsettar->nrises_sofar <5)
						pDphsettar->nrises_sofar++;
					//if(nphon < pDph_t->nallotot-4)
						//don't put it on next to last where we're going to frop.
						//if(pDphsettar->nrises_sofar <)
							do_non_phrase_final( phTTS,targf0, &cumdur, nphon );
					}
					 else if(nphon > pDph_t->nallotot-3)
					{
						delayf0 = 0;
						//BACK UP TO LAST SYL
						for(tmp = nphon; tmp >1; tmp--)
						{
						if((phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSYLL) IS_PLUS)
						{
							delayf0 -= pDph_t->allodurs[tmp]>>1;
							break;	
							
						}
						delayf0 -= pDph_t->allodurs[tmp];
						}
						targf0 = -250;
						if (pDph_t->cbsymbol )
						{
							length=0;
							//another syl??
						for(tmp = nphon;  tmp <= pDph_t->nallotot;tmp++)
						{
							
						if((phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSYLL) IS_PLUS)
						{
							length += pDph_t->allodurs[tmp];
							
							
						}
						}

							targf0 = 102;	/* comma boundary*/
							
							make_f0_command (phTTS,GLIDE, 996, targf0,0,(length >>1) , &cumdur, tmp);
							make_f0_command (phTTS,GLIDE, 996, -(targf0+100),(length>>1), (length>>1), &cumdur, tmp);
						}
						
						}
					}					
					if ( pDph_t->had_hatend )
					{
						pDph_t->had_hatend = 0;
						pDph_t->in_phrase_final = 1;
						if (stresscur != FEMPHASIS)
							do_phrase_final( phTTS, &cumdur, nphon );
					}
					
					
					
					/* Rule 2 stress rules*/ 
					
					
					else if ( (stresscur & FSTRESS) IS_PLUS  )
					{			/* Primary or emph */
						/* Make stress IMPULSE prop. to degree of stress */
						/* and stress position relative to clause onset */
						targf0=0;
						
						if (pKsd_t->lang_curr == LANG_british)
						{
							
							if (((struccur & FHAT_ENDS) IS_PLUS)
								|| ((struccur & FPERNEXT) IS_PLUS) ) /* last stressed syl of phrase*/
							{
								/* BATS 600 other coding bug */
								goto skiprules; /* For UK english code change to hit last syllable this hits
								last stressed syllable EAB 1/2/98 */
							}
							else
							{
								make_f0_command ( phTTS,IMPULSE, 22, targf0, -6,pDph_t->allodurs[nphon], &cumdur, nphon);
								pDph_t->prevnphon= nphon;
								pDph_t->prevtargf0 = targf0;
							}
							
						}
						if (pKsd_t->lang_curr == LANG_chinese)
							goto no_stress;
				//note right shift to  adj for filter to glide chnage
						if (pDph_t->malfem == MALE)
							targf0 = f0_mstress_level[stresscur];
						else
							targf0 =  f0_fstress_level[stresscur];
						
						
						
						if (pDph_t->malfem == MALE)
							targf0 += f0_mphrase_position[pDphsettar->nrises_sofar];
						else
							targf0 += f0_fphrase_position[pDphsettar->nrises_sofar];
						
						if( pDph_t->number_words == 1 )
							targf0 = targf0 - (targf0>>1);
						pDph_t->impulse_width = pDph_t->allodurs[nphon]-4; 
						
						
						if (pDph_t->cbsymbol )
						{
							targf0 >>= 2;	/* All gestures reduced in ? */
						}
						
						/* eab 2/21/97 EAB remove emphasisflag and associated stuff pulled see note in phsort.c */
						
						if (pKsd_t->lang_curr == LANG_italian)
							do_italian_stress(phTTS, &cumdur,nphon,&targf0);
						else 
								if(pKsd_t->lang_curr == LANG_spanish
						|| pKsd_t->lang_curr == LANG_latin_american)
						do_sp_non_phrase_final( phTTS,targf0, &cumdur, nphon );
						else
							if(pDph_t->in_phrase_final ==0)
								do_non_phrase_final( phTTS,targf0, &cumdur, nphon );
							else
							{
								
								delayf0 = pDph_t->allodurs[nphon] -(pDph_t->impulse_width);
								
								
								/* Begin IMPULSE much earlier when last stress of phrase */
								if (((struccur & FHAT_ENDS) IS_PLUS)
									|| ((struccur & FPERNEXT) IS_PLUS))
								{
									///effectively skip the normal stress rule on last word
									if (stresscur != FEMPHASIS)
									{
										targf0 = targf0>>1; //eablookat this after merge
										//	pDph_t->impulse_width = pD/_t->allodurs[nphon]>>2;
										delayf0 = -NF20MS;
									}
								}
								
								
								/* Scale by speaker def paramter SR, unless emphasized */
								temp = pDph_t->scale_str_rise;
								if ((stresscur == FEMPHASIS) )
								{
									temp = 32;
								}
								pDph_t->arg2 = targf0;
								pDph_t->arg3 = 32;
								targf0 = muldv (temp, targf0, 32);
								if (pKsd_t->lang_curr == LANG_british)
								{
									
									if (((struccur & FHAT_ENDS) IS_PLUS)
										|| ((struccur & FPERNEXT) IS_PLUS) ) /* last stressed syl of phrase*/
									{
										/* BATS 600 other coding bug */
										goto skiprules; /* For UK english code change to hit last syllable this hits
										last stressed syllable EAB 1/22/98 */
									}
									else
									{
										make_f0_command ( phTTS,IMPULSE, 22, targf0, -6,pDph_t->allodurs[nphon], &cumdur, nphon);
										pDph_t->prevnphon= nphon;
										pDph_t->prevtargf0 = targf0;
									}	
									
								}
								
								
								
								//	if(pDph_t->number_words > 1)
								//#define OLD_METHOD
								make_f0_command( phTTS,IMPULSE, 2, targf0, delayf0,pDph_t->impulse_width , &cumdur, nphon);
								
	
							}
							/* Increment stressed syllable counter */
							if (pDphsettar->nrises_sofar < MAX_NRISES)
								pDphsettar->nrises_sofar++;
no_stress:					if (pDphsettar->nrises_sofar == MAX_NRISES)
								pDphsettar->nrises_sofar=1;					
			}
			}
			
			
			/* Rule 3: Execute hat fall */
			
			/* If presently at top of hat, return to base shortly after */
/* vowel onset if this is last stressed syllable in phrase */
//ifdef check it first now
#ifdef MOVEDIT
if ( pDph_t->had_hatend )
{
	pDph_t->had_hatend = 0;
	pDph_t->in_phrase_final = 1;
	if (stresscur != FEMPHASIS)
	do_phrase_final( phTTS, &cumdur, nphon );
}
#endif				
#ifdef itsoutaher				
				/* 
				* Rule 4: Add positive pulse to approximate nonterminal fall-rise          
				* in stressed clause-final but non-sentence-final syllable, 
				* or in sentence ending in a question mark 
				*/
				delayf0 = -5;
				if ((struccur & FBOUNDARY) == FQUENEXT)
					NotQuest = 0;	   /* it is a question allow early stress */
				
				if ((struccur & FBOUNDARY) == (FPERNEXT | FEXCLNEXT | FSENTENDS))
				{
					NotQuest = 1;
				}
				// BATS 711 Old code a lie without stresscur it isn't checking for stress
				// old comment correct 
				// do to changing a comma boundary to a period we must check--
				//we really should decide on cluse stuff at the very beginning
				if (((struccur & FBOUNDARY) == FCBNEXT)
					|| ((struccur & FBOUNDARY) == FQUENEXT))
				{
					/* Time rise to begin near end of vowel */
					delayf0 = pDph_t->allodurs[nphon] - NF80MS;
					
					pDph_t->delta_special = 0;
					
					if ((struccur & FBOUNDARY) == FQUENEXT)
					{
						/* Sent.-final stressed vowel followed by q-mark */
						
						/* EAB We want the hardcoded gestures to be defined in a language specific 
						or speaker specific way file they ultimately want to be setable perhaps so for now I'm going
						to move all of these kinds of things to the speaker def files. 2/26/97 */
						
						/* found that it is firing on clause final syllable in German for some
						reason but also in German it presently appears to not be required so not worth
						troubleshooting at the moment */ 
						pDph_t->delta_special = 0; /* This code is garbage put here as a noop space filter
						so that sompiler wouldn't get lost*/
						//	if(pDph_t->number_words == 1)
						//	{
						//		make_f0_command ( phTTS,IMPULSE, 341,F0_QGesture1, -5,15, &cumdur, nphon);
						//		make_f0_command ( phTTS, IMPULSE, 341,F0_QGesture2, pDph_t->allodurs[nphon],24, &cumdur, nphon);
						//	}
						//	else
						//	{
						//		make_f0_command ( phTTS,IMPULSE, 341,F0_QGesture1, -6,15, &cumdur, nphon);
						//		make_f0_command ( phTTS, IMPULSE, 341,F0_QGesture2, (pDph_t->allodurs[nphon]>>1),20, &cumdur, nphon);
						//	}
							
						
						
					}
				}
				else
				{
#ifdef THISSHOULDBEWHQAT
					/* Phrase-final stressed vowel followed by comma */
					/* 2/26/97 See comment above*/
					
					
					pDph_t->delta_special = -50;
					/* EAB 2/15/98 Needs earlier */
					
					delayf0 -= NF20MS;
					
					
					//BATS 709
					if(pDph_t->commacnt == 0)
					{
						//make_f0_command ( phTTS, IMPULSE,42,  F0_CGesture1, 3 ,12, &cumdur, nphon);
						make_f0_command ( phTTS, IMPULSE, 42, 120, (short)delayf0,18, &cumdur, nphon);
					}
					
					else
						
						
					{
						make_f0_command ( phTTS, IMPULSE,420,  F0_CGesture1, delayf0,24, &cumdur, nphon);
						make_f0_command ( phTTS, IMPULSE, 420, F0_CGesture2, (short)(pDph_t->allodurs[nphon]>>1),24, &cumdur, nphon); // NAL warning removal
					}
					pDph_t->commacnt++;
#endif THISSHOULDBEWHQAT							
				}
				
#endif				
				
				
				
				
				/* 
				* Rule 5: Final fall on unstress clause-final syl, or on stressed 
				* clause - final syll that didn't have hat fall due to earlier emphasis 
				*/
				
				
				if ((feacur & FSYLL) IS_PLUS)
				{
					if(((stresscur & FSTRESS_1) IS_MINUS) &&
						((struccur & FHAT_ENDS) IS_MINUS))
						
					{					   /* or 1-str nofall */
						
						
						
						if(pKsd_t->lang_curr == LANG_spanish
							|| pKsd_t->lang_curr == LANG_latin_american)
						{
							if ((struccur & FTYPESYL) >= FBISYL
								&& ((struccur & FBOUNDARY) > FWBNEXT
								|| (phone_feature( pDph_t,phonex) & FCONSON) IS_PLUS))
							{
							}
							
							/* Pitch falls (glottalize) at end of declar. sent. */
							if (((struccur & FBOUNDARY) == FPERNEXT)
								|| ((struccur & FBOUNDARY) == FEXCLNEXT))
							{
								targf0 = F0_GLOTTALIZE;
								/* 
								* Pitch falls are less pronounced for some speakers 
								* to reduce impression of assertive personality 
								*/
								targf0 = frac4mul (targf0, pDph_t->assertiveness);
								
								targf0 |= 01;  /* Must be odd */
								
								pDph_t->test_targf0 = targf0;	/* RSM created this variable to reduce final fall in GERMAN */
								pDph_t->impulse_width = 20;		/* RSM */
								
								/* Sent.-final unstressed vowel followed by a period */
								/* EAB with addition of new code to glotalize in phdrwt0 this
								needs to grt alot weaker*/
								if(pKsd_t->lang_curr == LANG_german)
								{
									pDph_t->test_targf0 = targf0>>2;	/* RSM */
									pDph_t->impulse_width = 10;			/* RSM */
								}
								if(pDph_t->number_words == 1)
								{
									make_f0_command ( phTTS,GLOTTAL, 5, (short)(targf0>>1), (short)(pDph_t->allodurs[nphon] - NF30MS), 30, &cumdur, nphon); // NAL warning removal
								}
								else
									
								/*	EAB 11/4/98 Final glotalization gesture is very sensitive to timing and needs to be adjusted in onset if
								the phoneme following the syllable nucleus is voiced BATS 796. */
								if(phone_feature( pDph_t,phonex) & FVOICD)
								{
									pDph_t->test_targf0 = targf0>>3;
									if(nphon+2 <= pDph_t->nallotot && (phone_feature( pDph_t,pDph_t->allophons[nphon+2]) & FVOICD))
										/* nucleus has two voiced phonemes following it so delay even more*/
									{
										make_f0_command ( phTTS,IMPULSE, 5, pDph_t->test_targf0,  (pDph_t->allodurs[nphon]), pDph_t->impulse_width, &cumdur, nphon);
									}
									else
									{
										delayf0 = (pDph_t->allodurs[nphon]>>1) + (pDph_t->allodurs[nphon]>>2);
										make_f0_command ( phTTS,IMPULSE, 5, pDph_t->test_targf0,  delayf0, pDph_t->impulse_width, &cumdur, nphon);
									}
									
								}
								else
								{
									make_f0_command ( phTTS,IMPULSE, 5, pDph_t->test_targf0, (short)(pDph_t->allodurs[nphon]>>1), pDph_t->impulse_width, &cumdur, nphon); // NAL warning removal
								}
								
								if ((phone_feature( pDph_t,pDph_t->allophons[nphon]) & FSYLL) IS_PLUS)
								{
									
								}
							}
						}
					}
				
#ifdef Lets_junk_this		
			/* Rule 6: Continuation rise on unstress clause-final syll before  comma or ? */
			/* Rise occurs just before end of vowel */ 
			/* This rule also appears to apply a final pitch fall to all declarative sentence-final syllables RSM */
			
			delayf0 = pDph_t->allodurs[nphon] - NF115MS;
			if ((struccur & FBOUNDARY) == FQUENEXT)
			{
if(pKsd_t->lang_curr == LANG_german)
{			/* Unstressed vowel followed by a question mark */
				make_f0_command (phTTS,GLIDE, 6, F0_QGesture1, delayf0, 24, &cumdur, nphon);
				make_f0_command (phTTS,GLIDE, 6, F0_QGesture2,0, pDph_t->allodurs[nphon], &cumdur, nphon);
}
else
{
				/* Unstressed vowel followed by a question mark */
				make_f0_command (phTTS,GLIDE, 6, F0_QGesture1, 0, 24, &cumdur, nphon);
				make_f0_command (phTTS,GLIDE, 6, F0_QGesture2,delayf0, pDph_t->allodurs[nphon], &cumdur, nphon);
				}
				

				
			}
			if ((struccur & FBOUNDARY) == FCBNEXT)
			{
				/* Unstressed vowel followed by a comma */			
				delayf0 += NF20MS;

				make_f0_command ( phTTS,GLIDE, 6, F0_CGesture1, 0, 10, &cumdur, nphon);
				make_f0_command ( phTTS,GLIDE, 6, F0_CGesture2,delayf0, pDph_t->allodurs[nphon], &cumdur, nphon);
				

				
			}
			
			
			
			
			
			else if (((struccur & FBOUNDARY) == FPERNEXT))
			{
				targf0 = F0_GLOTTALIZE-200;
				
				/* 
				* Pitch falls are less pronounced for some speakers 
				* to reduce impression of assertive personality 
				*/
				
				targf0 = frac4mul(targf0, pDph_t->assertiveness);
				
				pDph_t->test_targf0 = targf0; /* RSM variable use to reduce the final fall in GERMAN */
				pDph_t->impulse_width = 20;	/* RSM */
				/* Sent.-final unstressed vowel followed by a period */
				/* eab 4/13/98 comment is wrong this is stressed vowel so review code in detail 
				when time permits*/
#ifdef GERMAN
			
				pDph_t->impulse_width = 10;
				if(pDph_t->number_words == 1)
				{
					make_f0_command ( phTTS,GLIDE, 6, (short)targf0, (short)(pDph_t->allodurs[nphon] - NF130MS), 20, &cumdur, nphon); // NAL warning removal
				}
				else
#endif							
				/*	EAB 11/4/98 Final glotalization gesture is very sensitive to timing and needs to be adjusted in onset if
				the phoneme following the syllable nucleus is voiced.BATS 796  */
				if(phone_feature( pDph_t,phonex) & FVOICD)
				{
					pDph_t->test_targf0 = targf0>>3;
					if(nphon+2 <= pDph_t->nallotot && (phone_feature( pDph_t,pDph_t->allophons[nphon+2]) & FVOICD))
						/* nucleus has two voiced phonemes following it so delay even more*/
					{
						make_f0_command ( phTTS,GLIDE, 6, pDph_t->test_targf0,  (pDph_t->allodurs[nphon]), pDph_t->impulse_width, &cumdur, nphon);
					}
					else	/* RSM */
					{
						delayf0 = (pDph_t->allodurs[nphon]>>1) + (pDph_t->allodurs[nphon]>>2);
						make_f0_command( phTTS,GLIDE, 6, pDph_t->test_targf0,  delayf0, pDph_t->impulse_width, &cumdur, nphon);
					}
				}
				else
				{	// CAB Removed warnings by enclosing typecast in ()
					make_f0_command( phTTS,GLIDE, 6, targf0, (short)((pDph_t->allodurs[nphon]>>1)+3),
						pDph_t->impulse_width, &cumdur, nphon); // NAL warning removal
				}	
			}
#endif							
				
				/* Rule 7: Reset baseline at end of sentence */
				
				if (phocur == GEN_SIL)
				{
					
					stepcount=0;
					/* Reset f0 to hat bottom from sub-hat-bottom */
					if ((pDphsettar->hat_loc_re_baseline != 0) && (pDph_t->nf0tot > 0))
					{
						
						if(pKsd_t->lang_curr == LANG_british)
							make_f0_command ( phTTS,STEP, 7, (short)( -(pDphsettar->hat_loc_re_baseline) ), 0, 20, &cumdur, nphon); // NAL warning removal
						
						pDphsettar->hat_loc_re_baseline = 0;
					}
					
					if (nphon > 0)
						pDphsettar->nrises_sofar = 1;	/* Soft reset */
					else if(pKsd_t->lang_curr == LANG_french)
					{
						pDph_t->hatpos=AT_BOTTOM_OF_HAT;
					}
					else
					{
						if ((pDph_t->allofeats[nphon - 1] & FBOUNDARY) == FCBNEXT
							&& pDph_t->nf0tot > 0
							&& (pDph_t->allophons[nphon - 1] != GEN_SIL))
						{
							make_f0_command ( phTTS,F0_RESET, 7, 0, 0, 0, &cumdur, nphon); /* RESET! */
							issubclause = TRUE;
						}
					}
				}
				/* Rule 8: Reset baseline and hat position to brim if end of a sentence */
				if ((struclas & FSENTENDS) IS_PLUS
					&& nphon == (pDph_t->nallotot))
				{
					pDph_t->commacnt=0; //BATS709
					make_f0_command ( phTTS,F0_RESET, 8, 0, 0, 0, &cumdur, nphon);
					pDphsettar->hat_loc_re_baseline = 0;
					/* Hard reset counter of stressed sylls in clause */
					pDphsettar->nrises_sofar = 0;
				}
				}
				
				
skiprules:					   /* END OF F0 RULES */
				
				/* Update cumdur to time at end of current phone */
				cumdur += pDph_t->allodurs[nphon];
#ifndef TOMBUCHLER				
				if( (nphon <= (pDph_t->nallotot-1) &&
					(nphon > 0 && (pDph_t->allophons[nphon] & PVALUE)!= 0))
					|| nphon==0 ) //1st two lines check end of cluase
					//This counts inital silence (we don't vount final silence 
					/* EAB It turns out that there are two possibilites for the way things
					get transmitted one is with and end of clause symbols and the other is without(implied)
					therefore you have to do two checks to know whether or not your at the last real phoneme
					or at a silence phoneme BATS 897 */
#endif
					pDph_t->tcumdur += pDph_t->allodurs[nphon];
				
					/* Rule 9: Add short schwa vowel to create release of [p,t,k,b,d,g] into sil. 
					* Logically, this kind of rule should appear in PHALLOPH.C, but
					* delaying it to here makes all dur and f0 rules much simpler 
					* EAB 2/28/97 changing to generalize when FPLOS + Fburst then release into a schwa
				*/
				// 3/14/01 eab don't use dummy vowel if cons unvoiced
				// 6/14/01 eab chnage- use it but don't voic it we need the placeholder for the release
				
				if (( phonex == GEN_SIL)&& (phone_feature( pDph_t,phocur) & FPLOSV) 
					&& (phone_feature( pDph_t,phocur) & FBURST) 
					&& pKsd_t->lang_curr != LANG_hebrew)
				{
					/* p t k b d g */
					/* || ((feacur & FNASAL) IS_PLUS) */ 
					/* m n nx en */
					/*&& (pDph_t->nallotot < NPHON_MAX)) I don't believe we should need this chnage earlier 
					NPHO_MAX code to a yellow zone code so we don't have to constantly check if were at the end things don't
					get added that frequently*/
					
					for (n = pDph_t->nallotot+1; n > nphon; n--)
					{
						pDph_t->allophons[n] = pDph_t->allophons[n - 1];
						pDph_t->allofeats[n] = pDph_t->allofeats[n - 1];
						pDph_t->allodurs[n] = pDph_t->allodurs[n - 1];
						pDph_t->user_f0[n] = pDph_t->user_f0[n - 1];
					}
					
					pDph_t->allophons[nphon + 1] = SCHWA1;
					if ( (begtyp(pholas ) == 1)
						|| (phone_feature( pDph_t,phocur) & FALVEL ))
					{
						pDph_t->allophons[nphon + 1] = SCHWA1;
					}
					/*	      B,D,G need longer voicing in release to be heard as voiced */
					//eab -why limit the release to an abitrarily short time
					if (/*((phocur >= USP_P) && (phocur <= USP_G))	// p t k b d g 
						&&*/ ((feacur & FVOICD) IS_PLUS)) {

						pDph_t->allodurs[nphon+1] = NF100MS;
						pDph_t->last_sonorant = nphon+1;
					
					}
					
		
					cumdur += pDph_t->allodurs[nphon+1];
					
					/*eab 8/96 update tcumdur*/
					pDph_t->tcumdur +=pDph_t->allodurs[nphon+1];
					pDph_t->allofeats[nphon + 1] = pDph_t->allofeats[nphon] | FDUMMY_VOWEL;
					pDph_t->nallotot++;
					nphon++;
				}
				
				}
				
}
}
void make_f0_command(LPTTS_HANDLE_T phTTS, short type, short rulenumber, short tar,
				 			 short delay, short length, short *psCumdur, short nphon)
{
	PKSD_T pKsd_t = phTTS->pKernelShareData;
	PDPH_T pDph_t = phTTS->pPHThreadData;

	/* Cudur reflects time (in frames) since last f0 command        */
	/* Cumdur+delay should never be less than zero                  */

	/* static short prpholas, temp; *//* MVP : Never Used,comment it out */
	/* If requested time is earlier than last f0 command, zero offset */
		
	if(length == 0 && type != 3)
	{
#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows 
//		WINprintf("error length ==0 settting to 1\n");
#endif
	length=1;
	}
	//init lastevent
	if(pDph_t->nf0tot == 0)
		pDph_t->lastevent = -30;
	if((delay + pDph_t->tcumdur) < pDph_t->lastevent)
	{
		//can't have an event before previous
		delay = (pDph_t->lastevent-pDph_t->tcumdur)+1;
#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows CE
	if (DT_DBG(PH_DBG,0x010))
		FNX_PRINTF(" requested an event before previous one setting to 1 frame after last event % \n ",delay);
#endif
	}
	 
	//if ((delay + *psCumdur) < 0)
	//{
	//		delay = -(*psCumdur);
	//}
	/* Save commands */	
	//-1 for timing fudge factor
	pDph_t->f0tim[pDph_t->nf0tot] =pDph_t->lastevent = pDph_t->tcumdur + delay-1;
	if(pDph_t->f0tim[pDph_t->nf0tot] == 0)
		//zero is not functional 1 is the smallest 
		pDph_t->f0tim[pDph_t->nf0tot] =1;
	pDph_t->f0tar[pDph_t->nf0tot] = tar;
	pDph_t->f0type[pDph_t->nf0tot] = type;
	/* eab 1/10/98 We need to be able to specifiy the length
	of the event instead of having only
	one choice. Initally some commands will ignore length*/
	pDph_t->f0length[pDph_t->nf0tot] = length;
//	FNX_PRINTF(" command is at time %d \n",(int)(6.4*pDph_t->f0tim[pDph_t->nf0tot]));
	/* "Zero" counter of time since last command */
//	*psCumdur = (-delay);
#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows CE
	if (DT_DBG(PH_DBG,0x010))
//		WINprintf("phon %d nphon %d rule %d type %d , tar %d delay %d length %d  \n",(pDph_t->allophons[nphon] & PVALUE), nphon, rulenumber,type, tar, delay, length);
		FNX_PRINTF("tcum %d time point %d phon %d nphon %d rule %d type %d , tar %d delay %d length %d  \n",(int)(pDph_t->tcumdur*6.4),(int)(pDph_t->lastevent*6.4),(pDph_t->allophons[nphon] & PVALUE), nphon, rulenumber,type, tar, (int)(delay*6.4), (int)(length*6.4));
		
#endif
	/* Increment counter of number of f0 commands issued */
	if (pDph_t->nf0tot < NPHON_MAX - 1)
	{
		pDph_t->nf0tot++;
	}
}

void do_by_rule_command(LPTTS_HANDLE_T phTTS,short *mf0, short *pcumdur,short nphon)
{

	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;
	  PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
short temp,tmp,targf0;							
short delayf0;
short length;
				if( pDph_t->user_f0[*mf0])
				{
					targf0 = (pDph_t->user_f0[*mf0] * 10);
					tmp = pDph_t->user_offset[*mf0];
					delayf0=0;
					temp = nphon;
					//delayf0 = pDph_t->allodurs[*nphon]*pDph_t->user_offset[*mf0]/100;
					if(tmp < 0)
						delayf0 = pDph_t->allodurs[nphon-1]*tmp/100;
					else while(tmp > 0) 
					{
						delayf0 += pDph_t->allodurs[temp]*pDph_t->user_offset[*mf0]/100;
						tmp -= 100;
						temp ++;
					}
#if !defined(UNDER_CE) && defined(PH_DEBUG)	//CAB 03/15/00 WINprintf not supported under Windows CE
					if (DT_DBG(PH_DBG,0x010))
									FNX_PRINTF("delay %d percent  = %d\n",pDph_t->user_offset[*mf0],(int)(delayf0*6.4));
#endif
					tmp=pDph_t->user_length[*mf0];
					length=0;
					temp = nphon;
					//delayf0 = pDph_t->allodurs[*nphon]*tmp/100;
					if(tmp < 0)
						length = pDph_t->allodurs[nphon-1]*tmp/100;
					else while(tmp > 0) 
					{
						length += pDph_t->allodurs[temp]*tmp/100;
						tmp -= 100;
						temp ++;
					}	
					length -= delayf0;
					FNX_PRINTF("lenght %d percent  = %d \n",pDph_t->user_length[*mf0],(int)(length*6.4));
			
					
					(*mf0)++;
					make_f0_command( phTTS,GLIDE, 99, targf0,
						delayf0, length, pcumdur, nphon); // NAL warning removal
					pDphsettar->hatsize +=targf0;
				}
			}
			
		

void do_british_inton(LPTTS_HANDLE_T phTTS,short *pcumdur,short nphon, short *n_last_syl_nuc)
{
	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;
short targf0;
			

							
								
							/* Code now works correcctly for UK placing gesture on last syllable
								not on last stresssed syllable as american english does */
								
								if( pDph_t->number_words > 2  )
								{
									/* n_last_syl_nuc is at the last vowel i.e. last syl nucleus*/
									if((pDph_t->nallotot -nphon) <6)
									{
										if (nphon == *n_last_syl_nuc)
										{
											make_f0_command( phTTS,GLIDE, 23, -150, (short)( -(pDph_t->allodurs[nphon-1]) ), 
												pDph_t->allodurs[nphon-1], pcumdur, nphon); // NAL warning removal
											make_f0_command( phTTS,GLIDE, 23, +60, 0, pDph_t->allodurs[nphon], pcumdur, nphon);
											pDph_t->done =1;
										}
									}
								}
								
								if(pDph_t->prevtargf0 >10  && pDph_t->prevnphon < nphon && !pDph_t->done)
								/* eab 1/19/98 previous target was upglide so now we
								want to do a downglide */
								{
									pDph_t->prevtargf0 = -pDph_t->prevtargf0;
									targf0=(pDph_t->prevtargf0- (pDph_t->prevtargf0>>2));
									make_f0_command( phTTS,GLIDE, 20,targf0 , -6,pDph_t->allodurs[nphon], pcumdur, nphon);
									make_f0_command( phTTS,GLIDE, 21, (short)(pDph_t->prevtargf0>>2), pDph_t->allodurs[nphon],
									pDph_t->allodurs[nphon+1], pcumdur, nphon); // NAL warning removal
								}
}








void do_italian_stress( LPTTS_HANDLE_T phTTS,short *pcumdur, short nphon,short *targf0)
{
	
	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;

short length;
									
							length = pDph_t->allodurs[nphon];
							if(phone_feature( pDph_t,pDph_t->allophons[nphon+1]) & FSON1 && pDph_t->allophons[nphon+1] != GEN_SIL)
							{
								length += pDph_t->allodurs[nphon+1]-NF100MS;
							}
							
							if( nphon < (pDph_t->nallotot-5))
							{
								make_f0_command( phTTS,GLIDE, 24, *targf0, 4,length , pcumdur, nphon);
								make_f0_command( phTTS,GLIDE, 24, -*targf0,length ,12, pcumdur, nphon);
							}
		
							
						}							


  /***********************end of ph_inton2.c**************************************/


void do_onset_conditions(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)

{
	
	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;
	  PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
short targf0;

#ifdef ODED
	make_f0_command( phTTS,STEP, 1, (short)50, -20, 1, cumdur, nphon);
	pDphsettar->hatsize = 50;
#endif
		if(pKsd_t->lang_curr == LANG_japanese)
			{
			make_f0_command( phTTS,STEP, 1, (short)-100, -20, 1, cumdur, nphon);
		}
	
	if (pDph_t->f0mode == NORMAL )
	{
								
		targf0 = pDph_t->size_hat_rise;	/* speaker-def param */
		
								
		if((phone_feature(pDph_t,pDph_t->allophons[1]) & FVOWEL)
			&& (pDph_t->allofeats[nphon+1]& FSTRESS_1))
		{
			targf0 = pDphsettar->tune_param_1;
				if((pDph_t->allofeats[nphon] & FSTRESS)== FEMPHASIS
					&& pDph_t->number_words >= 1)
				{
					targf0 += 20;
				}
			make_f0_command( phTTS,STEP, 1, (short)targf0, -20, 1, cumdur, nphon);
		}
		else if(phone_feature( pDph_t,pDph_t->allophons[1]) & FSTOP )
		{
			if(pDph_t->vot_time[2]>2)
			{
			//note this needs to be improve it's assuming that n+1 = vowel
			targf0 = pDphsettar->tune_param_2;
			make_f0_command( phTTS,STEP, 1, (short)targf0, -20, 10, cumdur, nphon);
			//UPLIFT TEMPry new at should canel old hat
		
		//	make_f0_command( phTTS,GLIDE, 1, (short)-(pDphsettar->tune_param_2-pDphsettar->tune_param_1), (pDph_t->allodurs[1]+ pDph_t->vot_time[2]+9),8, cumdur, nphon);
		//		pDphsettar->hatsize = pDphsettar->tune_param_1;
			}
			else
			{
			targf0 = pDphsettar->init_st_no_aspir;
				if((pDph_t->allofeats[nphon] & FSTRESS)== FEMPHASIS
					&& pDph_t->number_words >= 1)
				{
					targf0 += 20;
				}
			make_f0_command( phTTS,STEP, 1, (short)targf0, -20, 1, cumdur, nphon);
		
			}
		}		
		else
		{
			if(pKsd_t->lang_curr == LANG_korean)
				targf0= 100;
			else
			targf0 = targf0>>1;
			make_f0_command( phTTS,STEP, 1, (short)targf0, -20, 1, cumdur, nphon);
		}
	}
		pDphsettar->hatsize += targf0;
	pDphsettar->hat_loc_re_baseline += pDphsettar->hatsize;
	pDph_t->hatpos = AT_TOP_OF_HAT;
	pDph_t->hatstate = ON_TOP_OF_HAT;
	pDph_t->had_hatbegin = 0;
}
void do_phrase_dip(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)
{
	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;
	  PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;

	if (pDph_t->f0mode == NORMAL )
	{
		//	make_f0_command( phTTS,STEP, 1, (short)pDphsettar->hatsize, -20, 1, cumdur, nphon);
			make_f0_command( phTTS,IMPULSE, 9, (short)pDphsettar->hatsize, -60, 20, cumdur, nphon);
	}
	
	pDph_t->hatpos = AT_TOP_OF_HAT;
	pDph_t->hatstate = ON_TOP_OF_HAT;
	}


void do_phrase_final(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)
{
	short f0fall,delayf0,tmp,length,offset,voffset,vowdelay,last_syll;
	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;
	  PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
	if (pDph_t->f0mode == NORMAL)
	{

		f0fall = pDphsettar->hatsize;
		delayf0 = 0;
		/* Pitch falls are less pronounced for some speakers 
		* to reduce impression of assertive personality */
		f0fall = frac4mul (f0fall, pDph_t->assertiveness);
		
		if (pDph_t->cbsymbol)
		{
			f0fall = f0fall >> 1;	/* Gest reduced in ? */
		}
		
	
	}
			if(pKsd_t->lang_curr == LANG_italian)
			{
				//for the updown below base we must do everything different for italian
				f0fall = pDphsettar->hatsize; 
				if(nphon < pDph_t->nallotot-6)
				{
					//we're at an intermediat down/up ppstart or vpstart so
					// add in a constant offset below baseline
					f0fall+= 120;
					delayf0 = -6;
				}
				else
					f0fall += 150;
			}
			pDph_t->last_sonorant = 1; //default cASEA
			for(tmp= pDph_t->nallotot-2; tmp>=nphon; tmp--)
			{
				//first find last sonornat
				if(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSONOR)
				{
					pDph_t->last_sonorant = tmp;
					break;
				}
			}
			for(tmp= pDph_t->nallotot-2; tmp>=nphon; tmp--)
			{
				//first find last vowel
				if(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSYLL)
				{
					last_syll = tmp;
					break;
				}
			}
		//	if ((pDph_t->allofeats[pDph_t->nallotot-2] & FBOUNDARY) == FQUENEXT) 
		//	{
		//		delayf0=0;
		//		make_f0_command ( phTTS,GLIDE, 900, 350,0, pDph_t->allodurs[nphon], &cumdur, nphon);
		//		return;
		//MORE COMPLEX RULES NEED TABLE FALLS ON LAST BUT HNEST FALL THROUGHOUT NEED TO TRY 
			//IS ALL SONORANTS....
			//first delay til last_sonranat
			//delayf0 = pDph_t->allodurs[pDph_t->last_sonorant]>>2;
			//and set length to last_sonorant_ a "flat time" of say 4 frames
			//length = pDph_t->allodurs[pDph_t->last_sonorant]-delayf0;
			if( length <= 1)
				length = 1;
			for(tmp= pDph_t->last_sonorant-1; tmp>=nphon; tmp--)
			{
					delayf0 += pDph_t->allodurs[tmp];
					
			}

			offset=0;
				//but if last_sonorant preceeded by a sonorant the gesture is combined

		//	for(tmp= pDph_t->last_sonorant-1; tmp>=nphon; tmp--)
		//	{
		//		if(!(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSONOR)
	/*				|| phone_feature( pDph_t,pDph_t->allophons[tmp]) & FNASAL*/
			//)
		//		{
					//then back up start of final sonorant-sequence
			
		//			break;
		//		}
		//		offset += pDph_t->allodurs[tmp];
		//	}
			
						
			//gesture for stressed vowel of final word.
			// vowel is last or next to last
			if(nphon < (pDph_t->last_sonorant-2))
			{
				voffset =0;
				if(offset == 0) //final cv so lets see what preceeds it
				{
					for(tmp= pDph_t->last_sonorant-2; tmp>=nphon; tmp--)
					{
						if(!(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSONOR))
						{
							//then back up to of stressed vowel

							break;
						}
						voffset += pDph_t->allodurs[tmp];
					}
				}
				if(nphon == tmp)
				{
				vowdelay = pDph_t->allodurs[nphon];
				}
				else
				{
					//they weren't all sonorants
					voffset = pDph_t->allodurs[nphon];
					vowdelay =0;
				}
				//vowel longer from end use a more normal stress
				//add a seperate parameter also check to see the number of sonorants
				//til we get to last
			

				make_f0_command( phTTS, GLIDE, 3332, (short)pDphsettar->str_vow_before_final, vowdelay+pDph_t->vot_time[nphon],(short)(voffset -pDph_t->vot_time[nphon]),cumdur, nphon);
				f0fall += pDphsettar->str_vow_before_final;
			}
			else if(nphon < last_syll)
			{
		//	if((pDph_t->allofeats[nphon] & FSTRESS) == 3)
				//empahtic stress
		//		pDphsettar->endpoint_mult_stress += 3500;

				make_f0_command( phTTS, GLIDE, 3342, (short)pDphsettar->endpoint_mult_stress, pDph_t->vot_time[nphon],(short)(pDph_t->allodurs[nphon]+10-(pDph_t->vot_time[nphon])),cumdur, nphon);
				f0fall += pDphsettar->endpoint_mult_stress;
			}
	
			if(pDph_t->vot_time[tmp+1]>3)
			{
			//if final part has a vot then add it 
			offset -=  pDph_t->vot_time[tmp+1];
			//and bump starting f0
		
			}
			
			length = pDph_t->allodurs[last_syll];


			delayf0 -= offset;
			length += offset;
			//make_f0_command( phTTS, STEP, 311, (short)20, delayf0,4 , &cumdur, nphon);
			//				delayf0=0;
			f0fall += pDphsettar->endpoint_below_ap;
			
			if (((pDph_t->allofeats[nphon] & FBOUNDARY) == FCBNEXT ) || (pDph_t->clausetype == COMMACLAUSE && pKsd_t->lang_curr == LANG_english))
			{
				//delay comma gesture//
				delayf0 += (pDph_t->allodurs[tmp]>>1)+(pDph_t->allodurs[tmp]>>2);
				length -= (pDph_t->allodurs[tmp]>>1)+(pDph_t->allodurs[tmp]>>2);
				if (pDph_t->malfem == MALE)
				{
				make_f0_command ( phTTS,GLIDE, 731, -150, 0, 10, cumdur, nphon);
				make_f0_command ( phTTS,GLIDE, 732, 250,delayf0, length, cumdur, nphon);
				pDphsettar->hatsize += 250-150;	
				}


					else
				{
				make_f0_command ( phTTS,GLIDE, 731, -150, 0, 10, cumdur, nphon);
				make_f0_command ( phTTS,GLIDE, 732, 350,delayf0, length, cumdur, nphon);
				pDphsettar->hatsize += 350-150;		
				}
				pDph_t->hatstate = AFTER_NONFINAL_FALL;
				return;
			}
			if ((pDph_t->allofeats[pDph_t->nallotot-2] & FBOUNDARY) == FQUENEXT) 
			{
	if (pDph_t->malfem == MALE)
				{
				
				make_f0_command( phTTS, GLIDE, 1234, (short)350, delayf0,length-4 , cumdur, nphon);
				}
	else
	{
make_f0_command( phTTS, GLIDE, 1234, (short)200, delayf0,length-4 , cumdur, nphon);
	}
				return;
				
			}
			
			make_f0_command( phTTS, GLIDE, 333, (short)-f0fall, delayf0,length-4 , cumdur, nphon);
			
			
			
			
		
		pDphsettar->hat_loc_re_baseline -= f0fall;
	
	
	pDphsettar->hat_loc_re_baseline += pDphsettar->hatsize;
	pDph_t->hatpos = AT_BOTTOM_OF_HAT;
	pDph_t->hatstate = AFTER_FINAL_FALL;
	pDph_t->had_hatbegin = 0;
	}

	
short firstfeature(LPTTS_HANDLE_T phTTS, short FEA1,short FEA2,short nseg) 

{
			PKSD_T	pKsd_t = phTTS->pKernelShareData;
			PDPH_T	pDph_t = phTTS->pPHThreadData;
			PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
	short n;

	for (n=nseg; n < pDph_t->nallotot; n++) {

	    if ((pDph_t->allofeats[n] & FEA1) IS_PLUS)    return(TRUE);
	    if ((pDph_t->allofeats[n+1] & FEA2) IS_PLUS)    return(FALSE);
	}
	return(FALSE);
}


void do_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon)
{
	short  delayf0,tmp,length,last_sonorant,last_syll;
	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;
	  PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
		
	
			last_sonorant = 1; //default cASEA
			length = pDph_t->allodurs[nphon];	
			for(tmp= nphon+1; tmp <=pDph_t->nallotot-2; tmp++)
			{
				//first find candidates following vowel
				if(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSYLL)
				{
					length = +pDph_t->allodurs[tmp];
					last_syll = tmp;
				}
				else
					break;
			}
			delayf0 = 0;
			//and set length to last_sonorant_ a "flat time" of say 7 frames
			
			length -= 4;
			if( length <= 1)
				length = 1;
			//check for fson1 before vowel
			if(phone_feature( pDph_t,pDph_t->allophons[nphon-1 ]) & FSON1
				&& !(phone_feature( pDph_t,pDph_t->allophons[nphon -1]) & FSTOP))
			{
				delayf0 = -pDph_t->allodurs[nphon-1];
				length += pDph_t->allodurs[nphon];
			}
				//check for fson1 after vowel
			if(phone_feature( pDph_t,pDph_t->allophons[nphon+1 ]) & FSON1
				&& !(phone_feature( pDph_t,pDph_t->allophons[nphon + 1]) & FSTOP))
			{
			
				length += pDph_t->allodurs[nphon];
			}


			if(pKsd_t->lang_curr == LANG_korean)
			{

			make_f0_command( phTTS, GLIDE, 221, (short)stress_target, delayf0,length , cumdur, nphon);			
			make_f0_command( phTTS, GLIDE, 222, (short)-stress_target, length+5, 35 , cumdur, nphon);


			}
			else
			{
			make_f0_command( phTTS, GLIDE, 221, (short)stress_target, delayf0,length , cumdur, nphon);			
			make_f0_command( phTTS, GLIDE, 222, (short)-stress_target, length ,8 , cumdur, nphon);
			}
	
	}

void do_sp_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon)
{
	short  delayf0,tmp,length,last_sonorant,last_syll;
	  PKSD_T pKsd_t = phTTS->pKernelShareData;
	  PDPH_T pDph_t = phTTS->pPHThreadData;
	  PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
		
	
			last_sonorant = 1; //default cASEA
			length = pDph_t->allodurs[nphon];	
			
			delayf0 = 0;
			//and set length to last_sonorant_ a "flat time" of say 7 frames
			
			
			if( length <= 1)
				length = 1;
			//check for fson1 before vowel
			if(phone_feature( pDph_t,pDph_t->allophons[nphon-1 ]) & FSON1
				&& !(phone_feature( pDph_t,pDph_t->allophons[nphon -1]) & FSTOP))
			{
				delayf0 = -pDph_t->allodurs[nphon-1];
				length += pDph_t->allodurs[nphon];
			}
				//check for fson1 after vowel
			if(phone_feature( pDph_t,pDph_t->allophons[nphon+1 ]) & FSON1
				&& !(phone_feature( pDph_t,pDph_t->allophons[nphon + 1]) & FSTOP))
			{
			
				length += pDph_t->allodurs[nphon];
			}
			length = length -(length >>2);
			
			make_f0_command( phTTS, GLIDE, 221, (short)stress_target, delayf0,length , cumdur, nphon);			
			make_f0_command( phTTS, GLIDE, 222, (short)-stress_target, length ,12 , cumdur, nphon);
			
	
	}
