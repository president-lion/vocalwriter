/* 
 ***********************************************************************
 *
 *                           Copyright ©
 *	  Copyright © 2002 Fonix Corporation. All rights reserved.
 *	  Copyright © 2000-2001 Force Computers, Inc., a Solectron Company. All rights reserved.
 *    © Digital Equipment Corporation 1996, 1997. All rights reserved.
 */


#include "dectalkf.h"
#include "ph_def.h"

//#define DEBUGPHT
//#define FASTTALK
//extern __inline short phone_feature(PDPH_T pDph_t, int phone);	   /* Phonetic features            */
extern int inh_timing(LPTTS_HANDLE_T phTTS, int phone);
extern int min_timing(LPTTS_HANDLE_T phTTS, int phone);


/* ******************************************************************
 *      Function Name: phtiming()      
 *
 *  	Description: 
 *
 *      Arguments: LPTTS_HANDLE_T phTTS Text-to-speech handle
 *
 *      Return Value: void
 *
 *      Comments:
 *
 * *****************************************************************/
void krnew_phtiming(LPTTS_HANDLE_T phTTS)
{
	short                   psonsw = 0, posvoc = 0;
	PDPH_T                  pDph_t = phTTS->pPHThreadData;
	PKSD_T                  pKsd_t = phTTS->pKernelShareData;
	PDPHSETTAR_ST           pDphsettar = pDph_t->pSTphsettar;
	short                   stcnt = 0;

	short					cons_dur =0;

	short                   syldur = 0;
	short                   ncnt = 0;
	short                   endcnt = 0;
	short					vot =0;

	short					sonocnt = 0;
	short                   adjust = 0;
	short                   emphasissw = FALSE;		/* Made local and initialized */
	short                   pholas = GEN_SIL;		/* Made local and initialized */
	short                   struclas = 0;			/* Made local and initialized */
	short                   fealas = GEN_SIL;		/* Made local and initialized */
	short                   prcnt = 0, durinh = 0, durmin = 0, deldur = 0, nphon = 0;
	short                   phocur = 0;
	int						feacur = 0, feasyllabiccur = 0;
	short                   struccur = 0, strucboucur = 0, strucstresscur = 0;
	short                   dpause = 0;
	short					arg1,arg2;	/* arguments for the phmath routines. */


	U32  wordfeat;
	short tmp;
	/* Initialization  (Set sprat1, sprat2, zero counters, print debug) */

	init_timing (phTTS);

	pDphsettar->numstresses =0;
	pDph_t->tcumdur =0;/* 9/97EAB Since durations are done differently 
					   in typing mode calculation
					   of total dur must be moved to here also
						NOTE TO CARL THIS MAY HAVE ALREADY BEEN SUBMITTED BY DOUG*/
//#define MSDBG5



	/* MAIN LOOP, for each output phoneme */

	for (nphon = 0; nphon < pDph_t->nallotot; nphon++)
	{
		if (nphon > 0)
		{
			pholas = pDph_t->allophons[nphon - 1];
			struclas = pDph_t->allofeats[nphon - 1];
			fealas = phone_feature(pDph_t ,pholas);
		}

		phocur = pDph_t->allophons[nphon];
		tmp = phocur & PVALUE;
		struccur = pDph_t->allofeats[nphon];
		strucboucur = struccur & FBOUNDARY;
		feacur = phone_feature( pDph_t ,phocur);
		feasyllabiccur = feacur & FSYLL;
		strucstresscur = struccur & FSTRESS;
		prcnt = 128;
		if(((struccur & FSTRESS) IS_PLUS)
			&& (pDph_t->allophons[nphon] == HEP_NG
			|| pDph_t->allophons[nphon ] == HEP_H
			|| pDph_t->allophons[nphon ] == HEP_HH))
		{
			pDphsettar->numstresses++;
			//legnthen stressed things
			prcnt = 178;
		}	
		if (nphon < (pDph_t->nallotot - 1))
		{
			pDphsettar->phonex_timing = pDph_t->allophons[nphon + 1];
			pDphsettar->strucnex = pDph_t->allofeats[nphon + 1];
			pDphsettar->feanex = phone_feature( pDph_t ,pDphsettar->phonex_timing);
		}		

		wordfeat = pDph_t->allofeats[nphon] & WORDFEAT;

		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		/* Duration Rules                         						   */
		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		/* Use user-specified duration if one exists.    */
		/* User durations can be huge; call the conversion routine.   */
		if (pDph_t->user_durs[nphon] != 0)
		{
			pDphsettar->durxx = mstofr(pDph_t->user_durs[nphon] + 4);
			deldur = 0;
#ifdef MSDBG5
			FNX_PRINTF ("durxx = mstofr(user_durs[nphon]+4) durxx=%d\n", pDphsettar->durxx);
#endif

#ifdef DEBUG_USER_PROSODICS
			FNX_     ("\tFound user_dur[%s] = %3d frames in PHTIMING\n",
					phprint (phocur), pDphsettar->durxx);
#endif

			goto break3;
		}

		/* Convert inherent and minimum duration in msec to frames.*/
		/* Fixed overflow in 16 bits is not possible on these ones.*/
		
		durinh = ((inh_timing(phTTS,phocur) * 10) + 50) >> 6;
		durmin = ((min_timing(phTTS,phocur) * 10) + 50) >> 6;
		if ((feacur & FVOWEL) IS_PLUS )
		{
			durinh = durinh * 7/10;
			durmin = durmin * 7/10;
		}

		/* Additive increment */
		deldur = 0;
		/* Multiplicative constant (let 128 be 100%) */



		/* 
		 * Rule 1: Pause durations depend on syntax
		 *          Clause-initial pause should be minimal, i.e. NF64MS (enough
		 *          time for initial fricative to build up amp gradually)
		 *          Shortened to 0, except for fricitive and plosive in next phone
		 *          we go do 7 ...
		 */
		if (phocur == GEN_SIL)
		{
			if ((pDphsettar->feanex & (FVOICD | FOBST)) || (pDphsettar->feanex & FPLOSV))
				
				dpause = 4;//NF7MS; 
			else
				dpause = 5;

			pDph_t->asperation = (pDph_t->asperation - BASE_ASP) / 10;

			/* Treatment of other than clause-initial pauses: */
			if (nphon > 1)
			{
				/* If this clause ends in a comma, use short pause */
				/* Note extra compause added if user command [:dv cp __] */
				if ((struclas & FBOUNDARY) == FCBNEXT)
				{
#ifdef MSDEBUG
					FNX_PRINTF ("asperation 1111 asperation=%d\n", pDph_t->asperation);
#endif

					if (pDph_t->asperation > MAX_ASP_COMMA)
						pDph_t->asperation = MAX_ASP_COMMA;
					else if (pDph_t->asperation < MIN_ASP_COMMA);

					pDph_t->asperation = MIN_ASP_COMMA;
#ifdef MSDEBUG
					FNX_PRINTF ("asperation is now screwed up 1111\n");
#endif
					dpause = pDph_t->nfcomma + pDph_t->compause + pDph_t->asperation;
				}
				/* End of clause has long pause if ends with "." "!" or "?" */
				/* Note extra perpause added if user command [:dv pp __] */
				if (((struclas & FBOUNDARY) & FSENTENDS) IS_PLUS)
				{
#ifdef MSDEBUG
					FNX_PRINTF ("asperation 2222 asperation=%d\n", pDph_t->asperation);
#endif
					if (pDph_t->asperation > MAX_ASP_PERIOD)
						pDph_t->asperation = MAX_ASP_PERIOD;
					else if (pDph_t->asperation < MIN_ASP_PERIOD);
					pDph_t->asperation = MIN_ASP_PERIOD;
#ifdef MSDEBUG
					FNX_PRINTF ("asperation is now screwed up 2222\n");
#endif
					dpause = pDph_t->nfperiod + pDph_t->perpause + pDph_t->asperation;	
				}
			}
			/* Make sentence-initial pause long if this is a new paragraph */
			else if (pDph_t->newparagsw != FALSE)
			{
				dpause = pDph_t->nfperiod;
			}

			pDph_t->asperation = 0;

			/* Effect of speaking rate greatest on pauses */

			dpause = mlsh1 (dpause, pDphsettar->sprat1);
			/* Minimum pause is 13 ms*/
			if (dpause < 2)
				dpause = 2;
#ifdef ODED
                      dpause = 30;
#endif
			/* Skip over remaining duration rules if input is SIL */

			pDphsettar->durxx = dpause;
#ifdef MSDBG5
			FNX_PRINTF ("durxx = dpause durxx=%d\n", pDphsettar->durxx);
#endif

			durinh = pDphsettar->durxx;		/* for debugging print only */
			durmin = pDphsettar->durxx;
			goto break3;
		}
		/* Rule 2: Lengthening of segments in clause-final rime */
		if (strucboucur >= FCBNEXT)
		{
			deldur = 0;
			/* Except for plosives and voiced fricatives */
			if (((feacur & FVOICD) IS_PLUS))
				deldur=NF40MS;
				if ((feacur & FOBST) IS_PLUS)
			{
				deldur = NF20MS;
			}
			if ((feacur & FPLOSV) IS_PLUS)
			{
				deldur = 0;
			}
			/* Except for sonor conson [rx, lx] followed by voiceless obst */
			if (((phocur == USP_RX) || (phocur == USP_LX))
				&& ((pDphsettar->feanex & FOBST) IS_PLUS)
				&& ((pDphsettar->feanex & FVOICD) IS_MINUS))
			{
				deldur = NF15MS;
			}
			/* More lengthening of a vowel if in a short phrase */
			if ((pDph_t->nallotot < 10)
				&& (feasyllabiccur IS_PLUS)
				&& (strucstresscur IS_PLUS))
			{
				deldur += (NF40MS - (pDph_t->nallotot >> 1));
			}
			/* Less lengthening if next seg is sonorant in same rime */
			if ((pDphsettar->feanex & FSON1) IS_PLUS)
			{
				deldur -= NF20MS;
			}
		}
	
	
		/* Rule 8: Lengthen each seg of an emphasized syllable, including rime */
		if (((struccur & FWINITC) IS_PLUS)
			|| ((feasyllabiccur IS_PLUS) && (strucstresscur != FEMPHASIS)))
		{
			emphasissw = FALSE;
		}
		if (strucstresscur == FEMPHASIS)
		{
			emphasissw = TRUE;
		}
		if (emphasissw == TRUE)
		{
			deldur = deldur + NF20MS;
			if (feasyllabiccur IS_PLUS)
				deldur = deldur + NF40MS;
			
		}
		/* Rule 9: Influence of final conson on vowels and postvoc sonor */
		/* Switch to indicate presence of a postvocalic sonorant */
		psonsw = 0;					   /* Set to 1 if +syl is followed by sonor */
		arg1 = FRAC_ONE;	   /* Default if posvoc not obst */
		posvoc = GEN_SIL;				   /* Default postvocalic consonant */
		/* Does rule apply ? */
#ifdef outfornow
		if ((feasyllabiccur IS_PLUS)
			|| ((phocur >= USP_RX) && (phocur <= USP_NX)
				&& ((struccur & (FSTRESS | FWINITC)) IS_MINUS)
				&& ((pDphsettar->feanex & FOBST) IS_PLUS)))
		{

			if (  phone_feature( pDph_t , pDph_t->allophons[nphon+1]) & FSYLL)
				//two vowels toegther shorten then combo
			{
				prcnt = mlsh1(prcnt,N70PRCNT);
			}
			
			
			
			/* Determine whether next segment is postvocalic consonant */
			if (((pDphsettar->feanex & FSYLL) IS_MINUS)
				&& ((pDphsettar->strucnex & (FSTRESS | FWINITC)) IS_MINUS))
			{
				posvoc = pDphsettar->phonex_timing;
				/* See if postvocalic consonant is a sonorant */
				/* or if postvoc sonor is followed by an obst cons */
				if (((posvoc >= USP_RX) && (posvoc <= USP_NX))
					&& ((phone_feature( pDph_t ,pDph_t->allophons[nphon + 2]) & FOBST) IS_PLUS)
					&& ((pDph_t->allofeats[nphon + 2] & (FSTRESS | FWINITC)) IS_MINUS))
				{
					psonsw = 1;
					posvoc = pDph_t->allophons[nphon + 2];
				}
				/* If posvoc is now voicless or obst or nasal, do something */
				if (posvoc != GEN_SIL)
				{
					if ((phone_feature( pDph_t,posvoc) & FVOICD) IS_MINUS)
					{
						deldur = deldur - (deldur >> 1);
						/* Multiply by 0.8 if a voiceless fric */

						arg1 = N80PRCNT;
						if (((phone_feature( pDph_t ,posvoc) & FPLOSV) IS_PLUS)
							|| (posvoc == USP_CH))
						{
							/* Multiply by 0.7 if a voiceless plosive */
							arg1 = N70PRCNT;
						}
					}
					/* Postvocalic segment is voiced */
					else
					{
						/* Assume voiced plosive, multiply by 1.2 */
						/*EAB found that this rule lenghthened syallbic n by
						too much in final position 11/13/97 This I left in
						because it is very specific and safe */
						if ((phone_feature( pDph_t ,posvoc) & FOBST) IS_PLUS && phocur != USP_EN)
						{
							arg1 = N120PRCNT;
							/* Voiced fricative, add 25 ms to +syl */
							if (((phone_feature( pDph_t ,posvoc) & FPLOSV) IS_MINUS)
							/* OUT                && (strucboucur >= FVPNEXT)   END OUT */
								&& (posvoc != USP_DX)
								&& ((feacur & FSYLL) IS_PLUS))
							{
								deldur = deldur + NF25MS;
							}
							
						}
						/* Nasal, multiply by 0.85 */
						else if ((phone_feature( pDph_t ,posvoc) & FNASAL) IS_PLUS)
						{	
							arg1 = N85PRCNT;	
						}
					}
				}
			}
			if(phocur == USP_Q)
			{
				durmin = durmin>>1;
				
					arg2 = prcnt;
			prcnt = mlsh1 (prcnt, N20PRCNT);
			}
			/* Attenuate effect if not phrase-final or +syl followed by sonor */
			/* or if postvoc sonor next */
			if ((strucboucur < FVPNEXT) || (psonsw == 1))
			{
				arg1 = FRAC_HALF + (arg1 >> 1);
			}
			
			/* Multiply by 0.1 if [nt] is  postvocalic cluster (T nonwordinit) */
			if (((phocur == USP_N) && (pDphsettar->phonex_timing == USP_T))
				&& ((pDphsettar->strucnex & (FWINITC | FSTRESS)) IS_MINUS))
			{
				arg1 = N10PRCNT;
				if (((phone_feature( pDph_t ,pDph_t->allophons[nphon + 2]) & FSYLL) IS_PLUS)
					&& ((pDph_t->allofeats[nphon + 2] & FMEDIALSYL) IS_MINUS))
				{
					pDph_t->allophons[nphon + 1] = USP_D;	/* Change to [d] after durs */

					arg1 = N70PRCNT;
				}
			}
			arg2 = prcnt;
			prcnt = mlsh1 (arg1, arg2);
			}
			
#endif
		
		
		/* Rule 10: Lengthen first vowel of a two vowel sequence */
		if (feasyllabiccur IS_PLUS)
		{
			if ((pDphsettar->feanex & FSYLL) IS_PLUS)
			{
				//helpme this will lengthen all okay???
				deldur = deldur + NF30MS;
				
			}
			
			/* Rule 11: Lengthen word-initial stressed vowel of polysyllabic word */
			if (((struccur & FTYPESYL) == FFIRSTSYL)
				&& ((struccur & FSTRESS_1) IS_PLUS)
				&& ((struclas & FWINITC) IS_MINUS))
			{
				deldur += NF25MS;
				
			}
			/* Rule 12: Shorten vowels before postvocalic L */
			if (pDphsettar->phonex_timing == USP_LX
				|| pDphsettar->phonex_timing == USP_LY)
			{
					/* Reduce percent by factor of 0.7 */
				arg1 = N40PRCNT;
				arg2 = prcnt;
				prcnt = mlsh1 (arg1, arg2);	
			}
		}
		/* Rule 13: Shorten consonant clusters */
		else
		{
			if ((feacur & FCONSON) IS_PLUS)
			{
				if (((pDphsettar->feanex & FCONSON) IS_PLUS)
					&& (strucboucur < FWBNEXT))
				{
					/* First consonant of a two - consonant sequence */
					/* Default shortening is 70 percent */
					//unless plosive plosive sequence 
					if ((pDphsettar->feanex & FPLOSV) IS_MINUS
						|| (feacur & FPLOSV) IS_MINUS)
							arg1 = N70PRCNT;
					/* Length nasal by 1.5 if next cons is word-init */
					if (((feacur & FNASAL) IS_PLUS)
						&& ((pDphsettar->strucnex & FWINITC) IS_PLUS))
					{
						arg1 = N150PRCNT;
					}
					/* Also make min duration shorter for C's in a cluster */
					else
						durmin -= (durmin >> 2);

					/* Shorten [S,TH] followed by a plosive or [SH] */
					if ((phocur == USP_S) || (phocur == USP_TH))
					{
						if ((pDphsettar->feanex & FPLOSV) IS_PLUS)
						{
							/* Multiply by 0.5 */
							arg1 = FRAC_HALF;
						}
						if (pDphsettar->phonex_timing == USP_SH)
						{
							pDphsettar->durxx = NF15MS;
#ifdef MSDBG5
							FNX_PRINTF ("durxx = NF15MS durxx=%d\n", pDphsettar->durxx);
#endif
							goto break3;
						}
					}
					arg2 = prcnt;
					prcnt = mlsh1 (arg1, arg2);
					
				}
				if (((fealas & FCONSON) IS_PLUS)
					&& ((struclas & FBOUNDARY) < FVPNEXT))
				{
					/* Second consonant of a two-consonant sequence */
					/* Multiply by 0.7 */
					arg1 = N70PRCNT;
					/* Also make min duration shorter for C's in a cluster */
					durmin -= (durmin >> 2);
					if ((feacur & FPLOSV) IS_PLUS)
					{
						/* Shorten plosive if preceded by [s] */
						/* Multiply by 0.6 */
						if (pholas == USP_S)
						{
							arg1 = N60PRCNT;
						}
						/* Shorten unstr plos if preceded by nasal */
						if ((fealas & FNASAL) IS_PLUS)
						{
							/* Multiply by 0.1 */
							if (strucstresscur IS_MINUS)
								arg1 = 1638;
						}
						/* OUT                      Do not shorten a plos preceded by a plos if ((fealas & FPLOSV) IS_PLUS) arg1 = FRAC_ONE;
						 * END OUT */
					}
					arg2 = prcnt;
					prcnt = mlsh1 (arg1, arg2);
					
				}
			}
		}
		/* Rule 14: Increase sonor dur if preceding plosive is aspirated */
		if ((feacur & FSON1) IS_PLUS)
		{
			if (((fealas & FVOICD) IS_MINUS)
				&& ((fealas & FPLOSV) IS_PLUS))
			{
				deldur = deldur + NF20MS;
				
			}
		}
		/* Rule 15: Increase duration of phrase-initial vowels (following silence) 
		eab 3/2004 unless number_wrods == 1 ie. intial is final */
		if ((feacur & FSYLL) IS_PLUS 
			&& pDph_t->number_words >1 )
		{
			if (pholas == GEN_SIL)
			{
				deldur = deldur + NF30MS;
				
			}
		}
		/* Rule 16: Increase vowel dur if preceeded by non-nasal sonor conson */
		/* (May not apply to function words such as "was,were") */
		if ((feacur & FSYLL) IS_PLUS)
		{
			if (((fealas & FSON2) IS_PLUS)
				&& ((fealas & FNASAL) IS_MINUS))
			{
				if (deldur == 0)
					deldur = NF20MS;
				
			}
		}
				
	
		/* Rule turned on again with slightly less reduction I think this will be OK*/

		/* 
		 * Rule 18:Shortening of prevocalic clustered semivowels clustered 
		 * on left by stop or frcative. Before this rule, many sounded like 
		 * syllabic reduced segmanents, so fruit sounded like feruit. */	 

		if ((feacur & FSONCON) IS_PLUS && ((fealas & FOBST) IS_PLUS))
		{
			arg1 = prcnt;
			arg2 = N70PRCNT;
			prcnt = mlsh1 (arg1, arg2);
		}


		/* RULE 19: Shorten function word final TH as in "with: */
		if( (phocur == USP_TH) && ( (struccur & FTYPESYL) == FMONOSYL ) &&
			(strucboucur >= FWBNEXT) && (strucstresscur == FNOSTRESS) )
		{
			arg1 = prcnt;
			arg2 = N60PRCNT;
			prcnt = mlsh1 (arg1, arg2);
		}
		
		/* RULE 20: lengthen i in "the"("me,he" also OK followed by vowel */
		if ((phocur == USP_IY) && ((struccur & FBOUNDARY) > FMBNEXT) &&
			(strucstresscur == FNOSTRESS) &&
			((struccur & FTYPESYL) == FMONOSYL))
		{
			arg1 = prcnt;
			arg2 = N150PRCNT;
			prcnt = mlsh1 (arg1, arg2);
		}

		/* 
		 * RULE 21 SHorten stop following a stop and preceding a fricative 
		 * within the same sylable. Before this words like products have just as long 
		 * a /k/ and /t/ as a word missing the other stop would have, whereas in
		 * reality the /t/ should be reduced greatly. Cut minimum duration in
		 * half, and reduce the multiplier a lot. Probably actually want to reduce 
		 * this eve more. 
		 */

		if (((feacur & FPLOSV) IS_PLUS) &&
			((fealas & FPLOSV) IS_PLUS) &&
			((pDphsettar->feanex & FOBST) IS_PLUS) &&
			(strucboucur > FMBNEXT))
		{
			durmin = durmin >> 1;
			arg1 = prcnt;
			arg2 = N25PRCNT;
			prcnt = mlsh1 (arg1, arg2);

		}

		/* rule 23  shorten vowel if phonex == df writing versus riding */
		/* Improve rule by preventing it from firing if already  reduced*/

		if (pDphsettar->phonex_timing == USP_DF)
		{
			if(prcnt > 50)
			{
				arg1 = prcnt;
				arg2 = N40PRCNT;
				prcnt = mlsh1 (arg1, arg2);
			}
		}


		//10/14/00 EAB baaggerly and eagerrly showed that the rr can get shorten too much
		//being retroflexed it needs a minimum minudr won't do it because that minimum in a stressed situation
		if(phocur == USP_RR)
		{
			if(durmin <=13)
				durmin=13;
		}

		//Rule 25:Lengthen unvoiced cons after vowel previous rule shortened vowel
		if (((fealas & FVOICD) IS_PLUS
					&& (feacur & FVOICD) IS_MINUS))

		{
			deldur = deldur ;
			arg1 = N110PRCNT;
			arg2 = prcnt;
			prcnt = mlsh1 (arg1, arg2);
		}
	
		//RUle 26: Shorten word inital HX
		if ((phocur == USP_HX) && ((struccur & FWINITC) IS_PLUS))
		{
			arg1 = N60PRCNT;
			arg2 = prcnt;
			prcnt = mlsh1 (arg1, arg2);
		}

		pDphsettar->strucstressprev = strucstresscur;
		
		/* Finish up */
	
		/* Set duration from durinh, durmin, and percent */

		pDphsettar->durxx = (prcnt * (durinh - durmin)) DIV_BY128;
#ifdef MSDBG5
		FNX_PRINTF ("durxx = (prcnt * (durinh - durmin)) DIV_BY128 durxx=%d\n", pDphsettar->durxx);
#endif
		pDphsettar->durxx += durmin;   /* SHOULD BE DONE AFTER SPRATE ADJUSTMENT */
#ifdef MSDBG5
		FNX_PRINTF ("durxx += durmin durxx=%d\n", pDphsettar->durxx);
#endif
	//FNX_PRINTF ("durxx += durmin durxx=%d\n", pDphsettar->durxx);
	/* Rule for slow speaking lengthen inserted glotal stop*/
	
break3:

		/* Effect of speaking rate */
		if ((pDphsettar->sprat0 != 180) && (pDphsettar->durxx != 0))
		{
			arg1 = pDphsettar->durxx;
			arg2 = pDphsettar->sprat2;
			if(pDph_t->number_words == 1)
				arg2 += (pDphsettar->sprat2>>3);
			pDphsettar->durxx = mlsh1 (arg1, arg2) + 1;		/* Round upwards */
#ifdef MSDBG5
			FNX_PRINTF ("durxx = mlsh1(arg1,arg2)+1 durxx=%d\n", pDphsettar->durxx);
#endif

			/* Effect of speaking rate on additive increment to dur */
			arg1 = deldur;
			arg2 = pDphsettar->sprat1;
			deldur = mlsh1 (arg1, arg2);
		}
		/* Add in rule-governed additive increment to dur  */
		pDphsettar->durxx = pDphsettar->durxx + deldur;
#ifdef MSDBG5
		FNX_PRINTF ("durxx = durxx + deldur durxx=%d\n", pDphsettar->durxx);
#endif

		if (pDphsettar->durxx < 0 ) /*eab oct 93 found dur could get set =0 compromise*/
			pDphsettar->durxx=1;    /*over putting command later(safer) see comment above*/
	

			if(feasyllabiccur IS_MINUS
				&& phocur != GEN_SIL
				&& pDph_t->allophons[nphon] != HEP_JH)
				cons_dur +=pDphsettar->durxx ;


		pDph_t->allodurs[nphon] = pDphsettar->durxx;	   /* Save in array for phonetic comp */

		if (pDph_t->allophons[nphon] != GEN_SIL)	   			   /* don't count silence 			  */
		{
			/* FNX_PRINTF("add %d for phon %d\n",pDphsettar->durxx,pDph_t->allophons[nphon]); 	  */
			//EAB 11/20/98 In English this is really not syldur but duration between stess as English
			//is a stressed timed language. The code was origianlly down for a syllable timed language hence hte syldur nomiclature
			syldur += pDphsettar->durxx;
		}

		//for hebrew count everything
		sonocnt++;

//		if ((((struccur & FISBOUND) == FISBOUND) && nphon != 0 || nphon == pDph_t->nallotot - 2))
		//EAB 0/98 Found that consonants before vowel are also marked with stress and these we want to ignore here
#ifdef Outfornow
		if( ((struccur & FSTRESS) IS_PLUS))
			if((feasyllabiccur IS_PLUS) )
		{

#ifdef DEBUGPHT
			FNX_PRINTF (" 2fbound struccur%o, p= %d\n", struccur, (pDph_t->allophons[nphon] & PVALUE));
			FNX_PRINTF ("syldur = %d \n ", (syldur * 64) / 10);
			FNX_PRINTF ("sonocnt=%d\n", sonocnt);
#endif
			switch (sonocnt)
			{
				case 1:
					adjust = (pDph_t->timeref - (syldur ));
					break;
				case 2:
					adjust = ((pDph_t->timeref - (syldur )) >> 1);
					break;
				case 3:
					/* do 3/8 instead of divide by 3 */
					adjust = ((pDph_t->timeref - (syldur )) >> 3) * 3;
					break;
				case 4:
					adjust = ((pDph_t->timeref - (syldur )) >> 2);
					break;
				case 5:
					adjust = ((pDph_t->timeref - (syldur )) >> 3);
					break;
				case 6:
					adjust = ((pDph_t->timeref - (syldur )) >> 4 );
					break;
				case 7:
					adjust = ((pDph_t->timeref - (syldur )) >> 5);
					break;
				case 8:
					adjust = ((pDph_t->timeref - (syldur )) >> 6);
					break;
				case 9:
					adjust = ((pDph_t->timeref - (syldur )) >> 7);
					break;
				case 10:
					adjust = ((pDph_t->timeref - (syldur )) >> 8);
					break;
				default:
					adjust = ((pDph_t->timeref - (syldur )) >> 7);
#ifdef DEBUGPHT
					FNX_PRINTF ("WHY HERE??");
					FNX_PRINTF ("sonocnt=%d phon= %d nphon= %d\n", sonocnt, pDph_t->allophons[nphon], nphon);
#endif

					//	WINprintf("%d vcnt %d\n ",adjust,sonocnt);
					break;
			}

	
				adjust = adjust>>1;
				adjust = 0; //until fixed
		if (pDphsettar->sprat0 <= 150) /* note above 250 sprat0=250+(sprat-250)>>1 */
		{
			adjust =0;
		}
		if(nphon < 3 ) //first stress at begining vowel of a stessed word reduce effect
		{
			adjust=adjust>>3;
		}
		


		//WINprintf("stcnt %d endcnt %d \n",stcnt,nphon-1);

			//adjust = 0;
			
			for (endcnt = nphon; stcnt - (endcnt); endcnt--)
		{
			
			if ((phone_feature( pDph_t ,pDph_t->allophons[endcnt]) & FSON1)
				&& !(phone_feature( pDph_t ,pDph_t->allophons[endcnt]) & FNASAL) )
				
			{
				//FNX_PRINTF ("set  %d dur %d syldur=%d adj=%d", (pDph_t->allophons[endcnt] & 0xff), pDph_t->allodurs[endcnt], (syldur * 64) / 10, adjust);
				//Not first one much
				if (endcnt >= 3)
				{
				if (pDph_t->allodurs[endcnt] <= 8)
					adjust = adjust>>1; //phone already very short so minimize effect	
				pDph_t->allodurs[endcnt] += adjust;
				if (pDph_t->allodurs[endcnt] <=6)
					pDph_t->allodurs[endcnt]=6;
				
				//FNX_PRINTF ("to  %d \n", ((pDph_t->allodurs[endcnt] * NSAMP_FRAME) / 10));
				ncnt++;
				}
			}
		}
			
		//	  FNX_PRINTF("set  syldur from %d to %d in %d adjusts.\n",
		//	  ((syldur)*64)/10,((syldur+(ncnt*adjust))*64)/10,ncnt ); 
			 
		ncnt = 0;
		/* FNX_PRINTF("-- \n"); */
		stcnt = nphon;
		/* reset slydur */
		syldur = 0;
		sonocnt = 0;

	}
#endif

	if((pKsd_t->modeflag & MODE_READING)==0)
	{
				if(pDph_t->allophons[nphon] == HEP_JH)
			goto skipall;
			if(pDph_t->allofeats[nphon]& FSBOUND)
		//		FNX_PRINTF("ph=%d \n",pDph_t->allophons[nphon]); 
			if( nphon) 
				//nphon was >1 for hebrew so check it
			{
			

			if ((phone_feature( pDph_t ,pDph_t->allophons[nphon+1]) & FSYLL)
				&& ( (pDph_t->allofeats[nphon+1] & FSBOUND) != FSBOUND))
				//first try forward assuming consonants before nucleus
				
			{
			
				pDph_t->allodurs[nphon] -= 	cons_dur;

				if (pDph_t->allodurs[nphon] <= 5)
					pDph_t->allodurs[nphon] = 5;
						cons_dur =0; //adjusmnet done zero cons_duration
			}
			else
				if((pDph_t->allofeats[nphon] & FSBOUND) == FSBOUND)
			for (endcnt = nphon; endcnt >= 1; endcnt--)
			{ 
				if(endcnt < 1) //protection code
					break;
				//back up to fins first syl
				if(phone_feature( pDph_t ,pDph_t->allophons[endcnt]) & FSYLL)
				{
					if(cons_dur == 0)
					//	must have a cv with a place holder cons
					pDph_t->allodurs[endcnt] = pDph_t->allodurs[endcnt]>>1;
					else
					pDph_t->allodurs[endcnt] -= 	cons_dur;

				if (pDph_t->allodurs[endcnt] <= 5)
					pDph_t->allodurs[endcnt] = 5;
				cons_dur =0; //adjusmnet done zero cons_duration
				break;
				}
			}
			}
	}

skipall:



//		FNX_PRINTF ("final duration = %d  durxx  \n", ((pDphsettar->durxx * NSAMP_FRAME) + 5) / 10);

	//break3:
	/* ccc change for 1/2 sample */

	if (NSAMP_FRAME == 128)
		pDphsettar->durxx = (pDphsettar->durxx >> 1);	/* Save in array for phonetic comp */



#ifdef EPSON_ARM7
		if (phone_feature(pDph_t,pDph_t->allophons[nphon]) & FPLOSV)
			{
				pDph_t->allodurs[nphon] += pDph_t->curspdef[SPD_NF];
			//	pDph_t->tcumdur += 60;
				//pDph_t->tcum +=60;
			}
#endif

#ifdef MSDBG4
		FNX_PRINTF ("allodurs[nphon] = durxx allodurs[%d]=%d\n", nphon, pDph_t->allodurs[nphon]);
#endif
		pDph_t->longcumdur += (pDphsettar->durxx * NSAMP_FRAME);	/* Cum dur in waveform samples     */
	}	
}