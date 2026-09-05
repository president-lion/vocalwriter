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
void new_onset_conditions(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon);
void new_phrase_dip(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon);
void new_phrase_final(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon);
void new_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon);
void newsh_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon);

void newsh_phrase_final(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon);
short firstfeature(LPTTS_HANDLE_T phTTS, short fea1,short fea2,short nseg);



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
void us_inton_sh (LPTTS_HANDLE_T phTTS)
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
	short n=0;
	short oddeven = -1;
	short f0_accent_table[] = { 250, 150, 100, 80, 70 };
	short phrase_pos[6]=
	{250,250,250,250,250,250};
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
			new_phrase_dip( phTTS, &cumdur, nphon );
		
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
			goto skiprules;
		}
		
		
		
		if((feacur & FSYLL) IS_PLUS)
		{
			
			
			if ( pDph_t->had_hatend )
			{
				pDph_t->had_hatend = 0;
				pDph_t->in_phrase_final = 1;
				if (stresscur != FEMPHASIS)
					new_phrase_final( phTTS, &cumdur, nphon );
			}
			
			
			
			/* Rule 2 stress rules*/ 
			
			
			else if ( (stresscur & FSTRESS) IS_PLUS  )
			{			/* Primary or emph */
				
				if(nphon < pDph_t->nallotot-3)
				{
					targf0 = phrase_pos[pDphsettar->nrises_sofar];
				new_non_phrase_final( phTTS,targf0, &cumdur, nphon );
				}
				
				/* Increment stressed syllable counter */
				if (pDphsettar->nrises_sofar < 5)
					pDphsettar->nrises_sofar++;
				
			}
			
			
			/* Rule 3: Execute hat fall */
			
		}
		
		// DO phrase end processing.
		delayf0 = -5;
		
		if ((struccur & FBOUNDARY) >= FCBNEXT)
		{
			new_phrase_final;
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
					/*&& phone_feature( pDph_t,phocur) & FVOICD*/)
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
					
					pDph_t->allophons[nphon + 1] = USP_AX;
					if ( (begtyp(pholas ) == 1)
						|| (phone_feature( pDph_t,phocur) & FALVEL ))
					{
						pDph_t->allophons[nphon + 1] = USP_AX;
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



void us_phinton (LPTTS_HANDLE_T phTTS)
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
	short n=0;
	short oddeven = -1;
	short f0_accent_table[] = { 250, 150, 100, 80, 70 };
	short phrase_pos[6]= {250,180,150,100,70,50};
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

	if(pKsd_t->lang_curr == LANG_hebrew
				&& nphon == 0 )
				{
				targf0 = pDph_t->size_hat_rise;
				make_f0_command( phTTS,STEP, 4325, (short)targf0, -20, 1, cumdur, nphon);
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
			new_phrase_dip( phTTS, &cumdur, nphon );
		
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
			goto skiprules;
		}
		
		
		
		if((feacur & FSYLL) IS_PLUS)
		{
			//Phrase final rule		
			
			if ( pDph_t->had_hatend )
			{
				pDph_t->had_hatend = 0;
				pDph_t->in_phrase_final = 1;

				new_phrase_final( phTTS, &cumdur, nphon );
			}
			
			
			
			/* Rule 2 stress rules*/ 
			
			
			else if ( (stresscur & FSTRESS) IS_PLUS  )
			{			/* Primary or emph */
				
				if(nphon < pDph_t->nallotot-3)
				{
				targf0 = phrase_pos[pDphsettar->nrises_sofar];
				newsh_non_phrase_final( phTTS,targf0, &cumdur, nphon );
				}
				
				/* Increment stressed syllable counter */
				if (pDphsettar->nrises_sofar < 5)
					pDphsettar->nrises_sofar++;
				
			}
			
			
			/* Rule 3: Execute hat fall */
			
		}
		
		/* 
		* Rule 4: Add positive pulse to approximate nonterminal fall-rise          
		* in stressed clause-final but non-sentence-final syllable, 
		* or in sentence ending in a question mark 
		*/
		delayf0 = -5;
		
	
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
				
					
				if ( phonex == GEN_SIL && (phone_feature( pDph_t,phocur) & FPLOSV) 
					&& (phone_feature( pDph_t,phocur) & FBURST)) 
				{
					/* p t k b d g */
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
					
					pDph_t->allophons[nphon + 1] = USP_AX;
					if ( (begtyp(pholas ) == 1)
						|| (phone_feature( pDph_t,phocur) & FALVEL ))
					{
						pDph_t->allophons[nphon + 1] = USP_AX;
					}
					/*	      B,D,G need longer voicing in release to be heard as voiced */
					//eab -why limit the release to an abitrarily short time
					if (((feacur & FVOICD) IS_PLUS)) 
					{
						
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

void new_onset_conditions(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)

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
void new_phrase_dip(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)
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


void new_phrase_final(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)
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
//	if( length <= 1)
		length = 1;
	for(tmp= pDph_t->last_sonorant-1; tmp>=nphon; tmp--)
	{
		delayf0 += pDph_t->allodurs[tmp];
		
	}
	
	offset=0;
				//but if last_sonorant preceeded by a sonorant the gesture is combined
	for(tmp= pDph_t->last_sonorant-1; tmp>=nphon; tmp--)
	{
		if(!(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSONOR)
			/*				|| phone_feature( pDph_t,pDph_t->allophons[tmp]) & FNASAL*/)
		{
			//then back up start of final sonorant-sequence
			
			break;
		}
		offset += pDph_t->allodurs[tmp];
	}
	
	
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
			delayf0 = pDph_t->allodurs[nphon];
	}
	
//	if(pDph_t->vot_time[tmp+1]>3)
//	{
		//if final part has a vot then add it 
//		offset -=  pDph_t->vot_time[tmp+1];
		//and bump starting f0
		
//	}
	for( tmp = nphon; tmp <= last_syll; tmp++)
	{
	length += pDph_t->allodurs[tmp];
	}
	
	
//	delayf0 -= offset;
	f0fall = 60;	
	//make_f0_command( phTTS, STEP, 311, (short)20, delayf0,4 , &cumdur, nphon);
	//				delayf0=0;
	f0fall += pDphsettar->endpoint_below_ap;
	
	if (((pDph_t->allofeats[nphon] & FBOUNDARY) == FCBNEXT ) || (pDph_t->clausetype == COMMACLAUSE && pKsd_t->lang_curr == LANG_english))
	{
		
		if (pDph_t->malfem == MALE)
		{
			//falling better than rising if you don't know
			//make_f0_command ( phTTS,GLIDE, 731, 500, -15, 10, cumdur, nphon);
			make_f0_command ( phTTS,GLIDE, 732,-pDphsettar->hatsize ,delayf0, length, cumdur, nphon);
			pDphsettar->hatsize = 0;	
		}
		
		
		else
		{
//			make_f0_command ( phTTS,GLIDE, 731, -150, 0, 10, cumdur, nphon);
//			make_f0_command ( phTTS,GLIDE, 732, 350,delayf0, length, cumdur, nphon);
			make_f0_command ( phTTS,GLIDE, 732,-pDphsettar->hatsize ,delayf0, length, cumdur, nphon);

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
	
	
	
	
	void newsh_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon)
	{
		short  delayf0,tmp,length,last_sonorant,last_syll;
		PKSD_T pKsd_t = phTTS->pKernelShareData;
		PDPH_T pDph_t = phTTS->pPHThreadData;
		PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
		
		
		last_sonorant = 1; //default cASEA
		length = pDph_t->allodurs[nphon];	
		//			for(tmp= nphon+1; tmp <=pDph_t->nallotot-2; tmp++)
		//			{
		//first find candidates following vowel
		//				if(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSYLL)
		//				{
		//					length = +pDph_t->allodurs[tmp];
		//					last_syll = tmp;
		//				}
		//				else
		//					break;
		//			}
		delayf0 = 0;
		//and set length to last_sonorant_ a "flat time" of say 7 frames
		
		
		if( length <= 1)
			length = 1;
		if(nphon > 1)
			delayf0 = -(pDph_t->allodurs[nphon-1]>>1);
		if(pDph_t->allofeats[nphon - 1] && FPLOSV
			&& !(pDph_t->allofeats[nphon - 1]&& FVOICD))
			delayf0 += 6;
		
		
		
		if(pDphsettar->nrises_sofar == 0)
		{
		if(pKsd_t->lang_curr == LANG_hebrew)
		{

		make_f0_command( phTTS, GLIDE, 1221, (short)stress_target, delayf0,length , cumdur, nphon);			
		make_f0_command( phTTS, GLIDE, 1222, (short)-stress_target,pDph_t->allodurs[nphon]+4  ,8 , cumdur, nphon);	
		}

			else
		{
		pDphsettar->hatsize =300;
		make_f0_command( phTTS, GLIDE, 1221, (short)stress_target+pDphsettar->hatsize, delayf0,length , cumdur, nphon);			
		make_f0_command( phTTS, GLIDE, 1222, (short)-stress_target,pDph_t->allodurs[nphon]+4  ,8 , cumdur, nphon);	
		}
		}
		else
		{
		make_f0_command( phTTS, GLIDE, 2221, (short)stress_target, delayf0,length , cumdur, nphon);			
		make_f0_command( phTTS, GLIDE, 2222, (short)-stress_target,pDph_t->allodurs[nphon]+4  ,8 , cumdur, nphon);
		}	
	
	}

void new_sp_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon)
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
void newsh_inton (LPTTS_HANDLE_T phTTS)
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
	short phrase_pos[6]=
	{250,250,150,100,70,50};
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
	
	
	
	
	
	/* Should set pDphsettar->nrises_sofar to zero after a ph_init=0 hard reset */
	/* End of initialization */
	
	
		
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
			new_phrase_dip( phTTS, &cumdur, nphon );
		
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
			goto skiprules;
		}
		
		
		
		if((feacur & FSYLL) IS_PLUS)
		{
			
			
			if ( pDph_t->had_hatend )
			{
				pDph_t->had_hatend = 0;
				pDph_t->in_phrase_final = 1;
				if (stresscur != FEMPHASIS)
					newsh_phrase_final( phTTS, &cumdur, nphon );
			}
			
			
			
			/* Rule 2 stress rules*/ 
			
			
			else if ( (stresscur & FSTRESS_1) IS_PLUS  )
			{			/* Primary or emph */
				
				if(nphon < pDph_t->nallotot-3)
				{
					targf0 = phrase_pos[pDphsettar->nrises_sofar];
				newsh_non_phrase_final( phTTS,targf0, &cumdur, nphon );
				}
				
				/* Increment stressed syllable counter */
				if (pDphsettar->nrises_sofar < 5)
					pDphsettar->nrises_sofar++;
				
			}
			
			
			/* Rule 3: Execute hat fall */
			
		}
		
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
					/*&& phone_feature( pDph_t,phocur) & FVOICD*/)
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
					
					pDph_t->allophons[nphon + 1] = USP_AX;
					if ( (begtyp(pholas ) == 1)
						|| (phone_feature( pDph_t,phocur) & FALVEL ))
					{
						pDph_t->allophons[nphon + 1] = USP_AX;
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

void new_shonset_conditions(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)

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
void new_shphrase_dip(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)
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


void newsh_phrase_final(LPTTS_HANDLE_T phTTS,short *cumdur, short nphon)
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
	delayf0 = 10;
	//delayf0 = pDph_t->allodurs[pDph_t->last_sonorant-1];
	//and set length to last_sonorant_ a "flat time" of say 4 frames
	length = pDph_t->allodurs[pDph_t->last_sonorant];
	if( length <= 1)
		length = 1;
	for(tmp= pDph_t->last_sonorant-1; tmp>=nphon; tmp--)
	{
		delayf0 += pDph_t->allodurs[tmp];
		
	}
	
	offset=0;
				//but if last_sonorant preceeded by a sonorant the gesture is combined
	for(tmp= pDph_t->last_sonorant-1; tmp>=nphon; tmp--)
	{
		if(!(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSONOR)
			/*				|| phone_feature( pDph_t,pDph_t->allophons[tmp]) & FNASAL*/)
		{
			//then back up start of final sonorant-sequence
			
			break;
		}
		offset += pDph_t->allodurs[tmp];
	}
	
	
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
	//	make_f0_command( phTTS, GLIDE, 3342, (short)-200, pDph_t->vot_time[nphon],-20,cumdur, nphon);
	//	make_f0_command( phTTS, GLIDE, 3343,-30,pDph_t->allodurs[nphon] ,length,cumdur, nphon);
		f0fall += pDphsettar->endpoint_mult_stress;
	}
	
	if(pDph_t->vot_time[tmp+1]>3)
	{
		//if final part has a vot then add it 
		offset -=  pDph_t->vot_time[tmp+1];
		//and bump starting f0
		
	}
	
	length = pDph_t->allodurs[last_syll]-10;
	
	
	delayf0 -= offset;
	f0fall = 60;	
	//make_f0_command( phTTS, STEP, 311, (short)20, delayf0,4 , &cumdur, nphon);
	//				delayf0=0;
	f0fall += pDphsettar->endpoint_below_ap;
	
	if (((pDph_t->allofeats[nphon] & FBOUNDARY) == FCBNEXT ) || (pDph_t->clausetype == COMMACLAUSE && pKsd_t->lang_curr == LANG_english))
	{
		
		if (pDph_t->malfem == MALE)
		{
			//for steve have early rise followed by fall
	//		make_f0_command ( phTTS,GLIDE, 731, , -15, 10, cumdur, nphon);
			make_f0_command ( phTTS,GLIDE, 732,-(pDphsettar->hatsize) ,0, length, cumdur, nphon);
			pDphsettar->hatsize = 0;	
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
	make_f0_command( phTTS, GLIDE, 333, (short)-150, -5,20 , cumdur, nphon);
	
	make_f0_command( phTTS, GLIDE, 333, (short)-(f0fall), delayf0,length , cumdur, nphon);
	
	
	
	
	
	pDphsettar->hat_loc_re_baseline -= f0fall;
	
	
	pDphsettar->hat_loc_re_baseline += pDphsettar->hatsize;
	pDph_t->hatpos = AT_BOTTOM_OF_HAT;
	pDph_t->hatstate = AFTER_FINAL_FALL;
	pDph_t->had_hatbegin = 0;
	}
	
	
	
	
	void new_non_phrase_final(LPTTS_HANDLE_T phTTS,short stress_target, short *cumdur, short nphon)
	{
		short  delayf0,tmp,length,last_sonorant,last_syll;
		PKSD_T pKsd_t = phTTS->pKernelShareData;
		PDPH_T pDph_t = phTTS->pPHThreadData;
		PDPHSETTAR_ST pDphsettar = pDph_t->pSTphsettar;
		
		
		last_sonorant = 1; //default cASEA
		length = pDph_t->allodurs[nphon];	
		//			for(tmp= nphon+1; tmp <=pDph_t->nallotot-2; tmp++)
		//			{
		//first find candidates following vowel
		//				if(phone_feature( pDph_t,pDph_t->allophons[tmp]) & FSYLL)
		//				{
		//					length = +pDph_t->allodurs[tmp];
		//					last_syll = tmp;
		//				}
		//				else
		//					break;
		//			}
		delayf0 = 0;
		//and set length to last_sonorant_ a "flat time" of say 7 frames
		
		
		if( length <= 1)
			length = 1;
		if(nphon > 1)
			delayf0 = -pDph_t->allodurs[nphon-1];
		
		//check for fson1 after vowel
		if(length > 30)
			length = 30;
		
		
		
		make_f0_command( phTTS, GLIDE, 221, (short)stress_target, delayf0,length , cumdur, nphon);			
		make_f0_command( phTTS, GLIDE, 222, (short)-stress_target,pDph_t->allodurs[nphon]+4  ,22 , cumdur, nphon);
			
	
	}


