/********************************************
	ACC_5000.C	updated at 16th July 1991
	╩╕└сд╬евепе╗еєе╚╖ы╣ч
	╩ь▓╗д╬╠╡└╝▓╜
	╔б┬∙▓╗бж╕¤┬∙▓╗╩╤┤╣
	╧в═╤╜к╗▀д╬евепе╗еєе╚░▄╞░
	╣тд╡г╣д╬╟ядЄ╖ш─ъд╣ды
	╔╒┬░╕ьб╓д╬б╫д╬╜ш═¤
	╔√╝бевепе╗еєе╚д╬═▐└й
	╔╜▓╗е╟б╝е┐╜╨╬╧д╬╡б│г╚є░═┬╕╔Ї
 ********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_acch.h"
#include	"ile_fddh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"
#include "pkind.h"
#include "ile_accd.h"


/*========== ACC5000() ===================================================
	╡б╟╜бз╩╕└сд╬╖ы╣чбг
		┴░д╬╩╕└сдм╩┐╚─╖┐д╩дщд╨бв╕хд╬╩╕└сд╧╣тдпд─ддд╞╝л╩м╝л┐╚д╬┬ьд╟═юд┴дыбг
		┴░д╬╩╕└сдм╡п╔·╖┐д╩дщд╨бв╕хд╬╩╕└сд╧─удпд─дпбг
		├вд╖бв╕хд╬╩╕└сдм╞м╣т╖┐д╬д╚днд╧╖ы╣чд╖д╦дпддбг
		д▐д┐бв╕хд╬╩╕└сдм─╣ддд╚днд╧бв╖ы╣чд╖д╩ддбг
		├э░╒бз┴░╕хд╬╩╕└сдм├▒╜у╕ьд╬д╚днд╧бв╖ы╣чд╖д╩ддбг
			бб┴░д╬╩╕└сд╬║╟╕хдм╔╒┬░╕ь░╩│░д╬д╚днд╧╖ы╣чд╖д╩ддбг
  ========================================================================*/
void ACC5000() {
	register integer_16 i,j;
	for(i=0;i<stc_ptr-1;++i)
	{
		if(phrase_len[i]!=0)
		{
			/* ║╟╕хд╬╢чд╩дщдкдядъ */
			if((j=next_phrase(i))==0){ return; }
			/* д╔д┴дщдлдм├▒╜у╕ьд╩дщ╖ы╣чд╖д╩дд */
			if((phrase_kind[i]==pkind_others)||(phrase_kind[j]==pkind_others)){ goto ACC5000_190; }
			/* ─╣д╣додыд╚днд╧╖ы╣чд╖д╩дд */
			if(phrase_len[i]+phrase_len[j]>MAX_PHRASE){ goto ACC5000_190; }
			/* .00д╧╖ы╣чд╖д╩дд */
			if(acc_accent_raw[j][1]==0 && acc_accent_raw[j][2]==0){ goto ACC5000_190; }
			/* ╩г╣ч╠╛╗ьд╬д╚днд╩д╔бв┴░д╬╩╕└сдм╔╒┬░╕ь░╩│░д╟╜кд├д╞дддыд╚днд╧╖ы╣чд╖д╩дд */
			if(stc_ctl[j-1] & BITCTL_FIRST){ goto ACC5000_190; }
			/* е▌б╝е║д╣ды╔м═╫д╬двдыд╚днд╧╖ы╣чд╖д╩дд */
			/*if(stc_pause[j-1]>0){ goto ACC5000_190; }*/
			goto ACC5000_190;	/* for DEBUG ╛яд╦╩╕└с╖ы╣чд╩д╖ */
			/************/
			/*  ACC5100(j); */
			/************/
		}
ACC5000_190:;
	}
}


/*========== ACC5100() =========================================================
	╡б╟╜бз╩╕└сд╬╖ы╣чбг
		┴░д╬╩╕└сдм╩┐╚─бж╚°╣т╖┐д╩дщд╨бв╕хд╬╩╕└сд╧╣тдпд─ддд╞╝л╩м╝л┐╚д╬┬ьд╟═юд┴дыбг
		┴░д╬╩╕└сдм╡п╔·╖┐д╩дщд╨бв╕хд╬╩╕└сд╧─удпд─дпбг
  ==============================================================================*/
void ACC5100(j) register integer_16 j; {
	register integer_16 ptr;
	ptr=phrase_ptr[j];
	/* ╕хд╬╩╕└сдм╞м╣т░╩│░д╬д╚днд╧╖ы╣чд╣ды */
	if(accent0[ptr]!=0x82){
		if(accent0[ptr-1]==1){
			/* ┴░д╬╩╕└сд╬║╟╕хд╬╟ядм─уддд╚днбв╕хд╬╩╕└сд╧─удпд─дп */
			if(count_low(j)<=MAX_TONE){ ACC0004(j); }
		}else{
			/* ┴░д╬╩╕└сд╬║╟╕хд╬╟ядм╣тддд╚днбв╕хд╬╩╕└сд╧╣тдпд─дп */
			if(count_high(j)<=MAX_TONE){ accent0[ptr]=2; }
		}
	}
}

/*========== count_high(i) ==========================================
	╡б╟╜бз╩╕└сд╬╖ы╣чдм╡пд│д├д┐д╚днбв╧в┬│д╣ды╣тддетб╝ещд╬┐ЇдЄ╡сдсдыбг
  ===================================================================*/
integer_16 count_high(i) integer_16 i; {
	register integer_16 c,p;
	c=0;
	for(p=phrase_ptr[i]-1;;--p){
		if(accent0[p]==2){ ++c; }else{ goto count_high_100; }
	}
count_high_100:
	/* ╕хд╬╩╕└сд╬┬ш░ьетб╝ещд╧бв╣тдпд╩дыбг*/
	for(p=phrase_ptr[i]+1;p<phrase_ptr[i]+phrase_len[i];++p){
		if(accent0[p]!=1){ ++c; }else{ goto count_high_200; }
	}
count_high_200:
	return c+1;
}

/*========== count_low(i) ===========================================
	╡б╟╜бз╩╕└сд╬╖ы╣чдм╡пд│д├д┐д╚днбв╧в┬│д╣ды─уддетб╝ещд╬┐ЇдЄ╡сдсдыбг
  ===================================================================*/
integer_16 count_low(i) integer_16 i; {
	register integer_16 c,p;
	c=0;
	for(p=phrase_ptr[i]-1;;--p){
		if(accent0[p]==1){ ++c; }else{ goto count_low_100; }
	}
count_low_100:
	/* ╕хд╬╩╕└сд╧┴┤д╞─удпд─дп */
	return c+phrase_len[i];
}


/*========== ACC6000() ======================
	╡б╟╜бз╩╕└с└ш╞мд╬╔б┬∙▓╗д╧╕¤┬∙▓╗д╦┤╣дидыбг
		  ╩ь▓╗д╬╠╡└╝▓╜бг
  ===========================================*/
void ACC6000() 
{
	register integer_16 ptr,len; integer_16 i;
	BYTE mora1,mora2,a1,a2,a3;
	
	/* ╩╕└с└ш╞мд╬╔б┬∙▓╗дЄ╕¤┬∙▓╗д╦┤╣дидыбг*/
	for(i=0;i<stc_ptr;++i){
		if(phrase_len[i]!=0){
			mora1=omega[ptr=phrase_ptr[i]];
			if((mora1 & 0xF8)==0x78){ omega[ptr]=(mora1 & 0x07)|0x70; }
		}
	}
	
	/* г░г°г╕г▓б╩евепе╗еєе╚│╦б╦д╬еье┘еыд╧бвг▓д╦д╖д╞дкдп */
	for(i=cpron_ptr;i--;){
		if(accent0[i]==0x82){ accent0[i]=2; }
	}
	
	/*
	* ен,еп,е╖,е╣,е┴,е─,е╥,е╒,е╖ехдмбв
	* K,S,T,H,P╣╘д╬╟яд╬─╛┴░д╦днд┐д╚дн╠╡└╝▓╜д╣дыбг
	*
	* д╡дщд╦бв
	* е╘,е╫ дмбв
	* K,S,T,H,P╣╘д╬╟яд╬─╛┴░д╦днд┐д╚дн╠╡└╝▓╜д╣дыбг-- without RICOH --
	*
	* е┴ех,е╒ег,е─ег дмбв
	* K,S,T,H,P╣╘д╬╟яд╬─╛┴░д╦днд┐д╚дн╠╡└╝▓╜д╣дыбг-- RICOH --
	*/
	for(i=0;i<stc_ptr;++i){
		if((len=phrase_len[i])!=0){
			ptr=phrase_ptr[i];
			for(;len--;++ptr){
				mora1=omega[ptr];
				if(len){			/* ╩╕└с║╟╕хд╬╟яд╟д╧д╩ддд╚дн */
					switch(mora1){
					case 0x09: case 0x0A:		/* ен еп */
					case 0x29: case 0x32:		/* е┴ е─ */
#if defined(V_RICOH)
					case 0x2A: case 0x31:		/* е┴ех е─ег */
#else
					case 0xA2:					/* е╫ */
#endif
						mora2=omega[ptr+1];
						switch(mora2 & 0xF8){
						case 0x08: case 0x10: case 0x18:	/* еле╡е┐е╧е╤╣╘ */
						case 0x20: case 0x28: case 0x30:
						case 0x40: case 0x48: case 0xA0:
							/* евепе╗еєе╚д╬┬ь░╩│░д╧╠╡└╝▓╜д╣ды (еже╜ 87.10.16)*/
							/*if(accent0[ptr]==2 && accent0[ptr+1]==1){*/
							/*	;*/
							/*}else{*/
							accent0[ptr]|=0x80;
							/*}*/
						}
					}
					
					switch(mora1){
					case 0x19: case 0x12:				/* е╖ е╣ */
					case 0x41: case 0x42: case 0x1A:	/* е╥ е╒ е╖ех */
#if defined(V_RICOH)
					case 0x49:							/* е╒ег */
#else
					case 0xA1:							/* е╘ */
#endif
						mora2=omega[ptr+1];
						switch(mora2 & 0xF8){
						case 0x08: case 0x20: case 0x28:	/* еле┐е╤╣╘ */
						case 0x30: case 0xA0:
							accent0[ptr]|=0x80;
						}
					}
				}
			}
		}
	}
	
	/*
	* ╩╕└с╦Ўд╬─удд ен,еп,е╖,е╣,е┴,е─д╧╠╡└╝▓╜д╣дыбг
	* д╡дщд╦бв
	* ╩╕└с╦Ўд╬─удд е╘,е╫д╧╠╡└╝▓╜д╣дыбг-- without RICOH --
	* ╩╕└с╦Ўд╬─удд е┴ех,е╒ег,е─егд╧╠╡└╝▓╜д╣дыбг-- RICOH --
	*/
	for(i=0;i<stc_ptr;++i){
		if((len=phrase_len[i])!=0){
			ptr=phrase_ptr[i]+len-1;
			if(accent0[ptr]==1){
				if(next_phrase(i)){
					/* ─╛╕хдм═н└╝╗╥▓╗д╩дщ╠╡└╝▓╜д╖д╩ддбг*/
					switch(omega[ptr+1] & 0xF8){
					case 0x00: case 0x38: case 0x50:	/* ев,е╩,е▐╣╘ */
					case 0x58: case 0x60: case 0x68:	/* еф,ещ,ея╣╘ */
					case 0x70: case 0x78: case 0x80:	/* ем,елбм,е╢╣╘ */
					case 0x88: case 0x90: case 0x98:	/* е╕еу,е└,е╨╣╘ */
					case 0xA8: case 0xB8:				/* еЇеб,е░еб╣╘ */
						goto ACC6000_200;
					}
				}
				switch(omega[ptr]){
				case 0x09: case 0x0A: case 0x19:		/* ен еп е╖ */
				case 0x12: case 0x29: case 0x32:		/* е╣ е┴ е─ */
				case 0x1A:								/* е╖ех */
#if defined(V_RICOH)
				case 0x2A: case 0x49: case 0x31:		/* е┴ех е╒ег е─ег */
#else
				case 0xA1: case 0xA2:					/* е╘ е╫ */
#endif
					accent0[ptr]|=0x80;
				}
				
ACC6000_200:;
			}
		}
	}
	
#if defined(V_RICOH)
#else
	/*
	* евепе╗еєе╚дм─удд╕ь╞мд╬}╢|,}║|д╬╟яд╬╝бд╦бв╞▒▓╗д╬╣тдд╟ядмдпдыд╚днбг
	 *		б╩╬уб╦б╓длдлд╖б╫б╓д│д│дэб╫д╩д╔д╬╕ь╞мд╬б╓длб╫б╓д│б╫
	 * ├вд╖бвг╥г╔г├г╧г╚╚╟д╧╠╡└╝▓╜д╖д╩ддбг
	 */
	for(i=0;i<stc_ptr;++i){
		if((len=phrase_len[i])!=0){
			ptr=phrase_ptr[i];
			if(accent0[ptr]==1 && omega[ptr]==omega[ptr+1]){
				switch(omega[ptr]){
				case 0x08: case 0x0C:	/* ел,е│ */
					accent0[ptr]|=BITACC_MUSEI;
				}
			}
		}
	}

	/*
	 * евепе╗еєе╚дм─удд╕ь╞мд╬}╩|,}╬|д╬╝бд╦бвдв╣╘бждк╣╘д╬╟ядмдпдыд╚днбг
	 *		б╩╬уб╦б╓д█д│дъб╫б╓д╧длб╫д╩д╔д╬╕ь╞мд╬б╓д█б╫б╓д╧б╫
	 * ├вд╖бвг╥г╔г├г╧г╚╚╟д╧╠╡└╝▓╜д╖д╩ддбг
	 */
	for(i=0;i<stc_ptr;++i){
		if((len=phrase_len[i])!=0){
			ptr=phrase_ptr[i];
			if(accent0[ptr]==1){
				switch(omega[ptr]){
				case 0x40: case 0x44:	/* е╧,е█ */
					switch(omega[ptr+1] & 0x07){
					case 0x00: case 0x04:	/* ев,ек╣╘ */
						accent0[ptr]|=BITACC_MUSEI;
					}
				}
			}
		}
	}
#endif

	/* ╠╡└╝▓╜д╖д┐▓╗дм╗░д─┬│дп╛ь╣чбг*/
	for(i=0;i<cpron_ptr-2;++i){
		a1=accent0[i]; a2=accent0[i+1]; a3=accent0[i+2];
		if((a1 & 0x80)&&(a2 & 0x80)&&(a3 & 0x80)){
			/* ┐┐├цд╬▓╗д╧╠╡└╝▓╜д╖д╩ддбг*/
			accent0[i+1]&=0x7F;
		}
	}

	/* ╠╡└╝▓╜д╖д┐▓╗дм╞єд─┬│дп╛ь╣чбг*/
	for(i=0;i<cpron_ptr-1;++i){
		a1=accent0[i]; a2=accent0[i+1];
		if((a1 & 0x80)&&(a2 & 0x80)){
			/*
			 * ┴░дм╠╡└╝▓╜д╣дыбг
			 * ├вд╖бв╩╥╩¤дмел╣╘д╬╗■д╧бвел╣╘д╬╩¤дм╠╡└╝▓╜д╖д╩ддбг
			 * ╬╛╩¤дмел╣╘д╬╗■д╧бв┴░дм╠╡└╝▓╜д╣дыд│д╚д╦д╖д╞дкдпбг
			 * б╓╩▓дн╔╒д▒ды(taKITSUKERU)б╫д╬KIд╧╠╡└╝▓╜д╖д╩ддбг
			 * б╓╬Є╗╦┼к(reKISHITEKI)б╫д╬KIд╧╠╡└╝▓╜д╖д╩ддбг
			 * б╓╖·├█д╚(keXCHIKUTO)б╫д╬KUд╧╠╡└╝▓╜д╖д╩ддбг
			 * б╓╩╣дн╩¤(kiKIKATA)б╫д╬г▓╚╓╠▄д╬KIд╧╠╡└╝▓╜д╖д╩ддбг
			 * б╓├╧╢ш(CHIku)б╫д╬KUд╧╠╡└╝▓╜д╖д╩ддбг
			 *			updated 91. 7.15
			 */
			/*if((a1 & 0x7F)==(a2 & 0x7F)){*/
			/*mora1=omega[i] & 0xF8;*/
			mora2=omega[i+1] & 0xF8;
			if(mora2==0x08){
				accent0[i+1] &=0x7F;
			}else{
				accent0[i] &=0x7F;
			}
			/*}*/
			/*
			 * ░удж╣тд╡д╩дщбв╣тдд╩¤д╧╠╡└╝▓╜д╖д╩ддд╚╗╫д├д┐дмбв╬у│░дм┬┐ддд╬д╟бв
			 * ╞▒д╕╣тд╡д╬╗■д╚╞▒д╕╜ш═¤д╦д╖д╞дкдпбг
			 *			updated 91. 7.16
			 *if((a1 & 0x7F)!=(a2 & 0x7F)){
			 *	if((a1 & 0x7F)==2){
			 *		accent0[i]&=0x7F;
			 *	}else{
			 *		accent0[i+1]&=0x7F;
			 *	}
			 *}
			 */
		}
	}
}


/*========== ACC7000() =================================================
	╡б╟╜бз╢ч╞╔┼└д╬─╛┴░бвд▐д┐д╧бв╩╕╦Ўд╬╞░╗ь╧в═╤╖┴д╧бвд╜дьдм╚°╣т╖┐д╬д╚днбв
		евепе╗еєе╚д╬┬ьдм░ь╟я─╛┴░д╦░▄╞░д╣дыбг

	(╬у)бз░√д▀бв┐йд┘бв╝щдъбв└╕днбв
		бб╗бд╖бв┐од╕бв┼∙етб╝ещ▓╗┴╟дЄ┤▐др╛ь╣чд╧д╡дщд╦░ь╟я░▄╞░д╣дыбг
  ======================================================================*/
void ACC7000() {
	register integer_16 i;
	for(i=1;i<stc_ptr;++i){
		/* гщ╚╓╠▄дм╢ч╞╔┼└д╩дщд╨бвгщб▌г▒╚╓╠▄дЄ─┤д┘дыбг*/
		if(chk_separator(i)){ ACC7100(i-1); }
	}
	/* ╩╕╦Ўдм╢ч╞╔┼└д╟д╩д▒дьд╨бв╩╕╦ЎдЄ─┤д┘дыбг*/
	if(chk_separator(stc_ptr-1)==0){ ACC7100(stc_ptr-1); }
}

void ACC7100(i) register integer_16 i; {
	integer_16 c,bno,case_tbl; register integer_16 ptr;
	for(c=0;c<stc_nbno[i];++c){
		if(bno=stc_bno[i][c]){
			case_tbl=Fdd_Header.Fdd_BnoTblOfs;
			/* 4bit╠▄дмекеєд╩дщд╨бв╧в═╤╖┴д╟двдыбг*/
			if(FddHDR[case_tbl+bno-1] & BITBNO_Renyou){
				goto ACC7000_100;
			}
		}
	}
	return;

	ACC7000_100:;
	/* ░ь╟яд╬╧в═╤╖┴б╩╕▐├╩╞░╗ь░╩│░б╦дЄ╜№дпбг*/
	/*
	 * ╧в═╤╖┴╕ь╚°д▐д┐д╧ б╩╕▐├╩╞░╗ь╧в═╤╖┴б╦
	 * г▓╩╕╗·░╩╛хд╬╕ь┤┤б▄│ш═╤╕ь╚° б╩б╓▓╝д╡ддб╫д╩д╔бв╕▐├╩╞░╗ьд╬╬у│░б╦д▐д┐д╧
	 * г▓╩╕╗·░╩╛хд╬╕ь┤┤д╬д╚днбвб╩г▓╩╕╗·░╩╛хд╬░ь├╩╞░╗ь╧в═╤╖┴б╦
	if((bno_tbl[bno-1]==pkind_verb_renyou)
	|| (bno_tbl[bno-1]==pkind_verb_inflect && stc_out_len[i-1]>1)
	|| (bno_tbl[bno-1]!=pkind_verb_inflect && stc_out_len[i]>1)){
	 */
	/* г╡г╖елещердмг▒д╩дщ░▄╞░д╖д╩ддбг*/
	if(stc_accent_raw[i][6]!=1){
		ptr=stc_out_ptr[i]+stc_out_len[i]-1;
		/*
		 * ptr==0 д╣д╩дяд┴бвг▒╟я╕ьд╬д╚днд╧бв░▄╞░д╟днд╩ддд╬д╟├э░╒д╣дыбг
		 */
		if(ptr>0 && accent0[ptr]==0x82){
			accent0[ptr-1]=0x82; accent0[ptr]=1;
		}
	}
	return;
}

/*========== ACC8000() =========================================================
	╡б╟╜бзб╩г▒б╦╡┐╠ф╔фб╓бйб╫д╬─╛┴░д╬╟яд╬╣тд╡дЄг╣д╚д╣дыбг
		ббб╩г▓б╦╜ї╗ьб╓дяб╫б╓дшб╫б╓д═б╫д╩д╔дмбв╢ч╞╔┼└д╬─╛┴░д▐д┐д╧╩╕╦Ўд╦двдыд╚дн
				д╜д╬╣тд╡дЄг╣д╚д╣дыбг
					б╩╬уб╦└╓дддяб╓д═б╫бв╞░ддд┐б╓дяб╫
				д╜дьдщд╬╜ї╗ьд╧╕х├╝╚╓╣ц╔╜д╬г│е╙е├е╚╠▄дмекеєд╦д╩д├д╞дддыбг
  ==============================================================================*/
void ACC8000() {
	integer_16 i,j,case_tbl,ptr,len;
	for(i=0;i<stc_ptr;++i){
		/* ╢чд╬─╣д╡дмг░д╬д╚дн╝бд╬╢чдЄ─┤д┘дыбг*/
		if((len=phrase_len[i])==0){ goto ACC8000_190; }
		/* ╢чдм╞├╝ь╩╕╗·д└д▒длдщд╩дыд╚днд╧╝бд╬╢чдЄ─┤д┘дыбг*/
		if(len==1 && accent0[phrase_ptr[i]]==0xFF){ goto ACC8000_190; }

		/* ╝бд╬─╣д╡дме╝еэд╟д╩дд╢чд╬╚╓╣цдЄ─┤д┘дыбгдтд╖║╟╕хд╬╢чд╩дще╝еэдЄ╩╓д╣бг*/
		j=next_phrase(i);
		if(j==0){ j=stc_ptr; }
		/* ├э╠▄д╖д╞ддды╢чд╬║╟╕хд╬╟яд╬░╠├╓дЄ╡сдсдыбг*/
		ptr=phrase_ptr[i]+len-1;

		/***************************************************/
		/* б╩г▒б╦╡┐╠ф╔фб╓бйб╫д╬┴░д╬етб╝ещд╬╣тд╡дЄг╣д╚д╣дыбг*/
		/***************************************************/

		/* ptr╚╓╠▄д╬╩╕╗·дм╩╕╦Ўд╟д╧д╩дпд╞бв*/
		if(j){
			/* (ptr+1)╚╓╠▄д╬╩╕╗·дм╞├╝ь╡н╣цд╟бв*/
			if(chk_separator(j)){
				/* (ptr+1)╚╓╠▄д╬╩╕╗·дм╡┐╠ф╔фд╬д╚днбвptr╚╓╠▄д╬╟яд╬╣тд╡дЄг╣д╚д╣дыбг*/
				if(inert[stc_in_ptr[j]*2]==0x21 && inert[stc_in_ptr[j]*2+1]==0x29){
					accent0[ptr]=9;
				}
			}
		}

		/*************************************************************************/
		/* б╩г▓б╦╢ч╞╔┼└д╬─╛┴░д▐д┐д╧╩╕╦Ўд╬б╓дяб╫б╓дшб╫б╓д═б╫д╩д╔д╬╣тд╡дЄг╣д╚д╣дыбг*/
		/*************************************************************************/

		/* ptr╚╓╠▄д╬╩╕╗·дм╩╕╦Ўд╬д╚днбвд▐д┐д╧бв(ptr+1)╚╓╠▄д╬╩╕╗·дм╞├╝ь╩╕╗·д╬д╚днбв*/
		if(j==stc_ptr || chk_separator(j)){
			/* ╕х├╝╚╓╣ц╔╜дЄ─┤д┘д╞бв╣тд╡дмг╣д╦д╩дъджды╜ї╗ьд╩дщд╨бв*/
			case_tbl=Fdd_Header.Fdd_BnoTblOfs;
			if(FddHDR[case_tbl+stc_bno[j-1][0]-1] & BITBNO_Level9){
				accent0[ptr]=9;
			}
		}

		ACC8000_190:;
	}
}


/*========== ACC_JOSI_NO() ========================
	╡б╟╜бз╕хд╦┬╨д╣ды▒╞╢┴дмг╣д╬╔╒┬░╕ьб╓д╬б╫д╬╜ш═¤бг
  =================================================*/
void ACC_JOSI_NO() {
	register integer_16 i,p;
	/* б╓┐х╞╗д╬╝╪╕¤б╫д╬б╓д╬б╫д╬▒╞╢┴д╟б╓╝╪╕¤б╫д╬└ш╞м╟ядм╣тдпд╩дыбг*/
	/* д│д╬╜ш═¤д╧д╣д┘д╞д╬╔╒┬░╕ьд╬╖ы╣ч╜ш═¤дЄ╜кдид┐├╩│мд╟╣╘дяд╩д▒дьд╨д╩дщд╩ддбг*/
	for(i=0;i<stc_ptr-1;++i){
		if(acc_accent_raw[i][3]==9){
			/* ╝бд╬╟ядЄ╣тдпд╣дыд┘дн═╫┴╟д╩дщд╨бв*/
			if(stc_pause[i+1]==0){
				/* е▌б╝е║дЄдвд▒дыд╚днд╧бв╝бд╬╟яд╪д╬▒╞╢┴д╧д╩ддбг*/
				p=stc_out_ptr[i+1];
				/* д╜д╬╟ядм╣тддд╚днд└д▒╝бд╬╟ядЄ╣тдпд╣дыбг*/
				if(accent0[p-1]!=1 && omega[p]!=0xFF){ accent0[p]=2; }
			}
		}
	}
}


/*========== ACC_PHRASE() ==================================
	╡б╟╜бзд╥д╚д─д╬б╓джд═дъб╫д╬╛хд╦╛шды╢чд╬╢ш└┌дъдЄ╖ш─ъд╣дыбг
  ==========================================================*/
void ACC_PHRASE() {
	register integer_16 i,j;
	integer_16 precede,fall;
	integer_16 cx;
	BYTE *si;
/*
 *	б╓╕└╕ь╣й│╪╕ж╡ц╜ъб╫д╧б╓╕└╕ь╣й│╪б╫бвб╓╕ж╡ц╜ъб╫д╬г▓д─д╬╢чдшдъ└одыбг
 *	б╓╗фд╬▓ё╝╥б╫д╧д╥д╚д─д╬╢чд╟двдыбг
 *	д│д│д╟д╧бвд╥д╚д─д╬╢чдмд╥д╚д─д╬┬ьдЄдтд─дшджд╦╢чд╬╢н│ждЄ─┤└░д╣дыбг
 */
	for(i=0;i<stc_ptr;++i){
		if(phrase_len[i]==0){ goto ACC_PHRASE_90; }
		precede=1;
		fall=0;
		for(si=(&accent0[phrase_ptr[i]]),cx=phrase_len[i];cx--;++si){
			if(precede==2 && *si==1){ fall=1; }
			if(fall==1){ *si=1; }
			precede=(*si);
		}
		ACC_PHRASE_90:;
	}
ACC_PHRASE_100:
	i=0;
ACC_PHRASE_110:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]!=0){ goto ACC_PHRASE_120; }
	}
	return;
ACC_PHRASE_120:
	j=i+1;
	while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ return; }

	if(stc_pause[j]!=0){ ++i; goto ACC_PHRASE_110; }

	if(phrase_len[i]==1){
		/*
		 *	┴░дмг▒╟яд╬д╚днд╧бв
		 *	д╜дьдм─удд▓╗д╩дщбв╕хд╬└ш╞мдм╣тддд╚днд└д▒г╧г╦бг
		 *	д╜дьдм╣тдд▓╗д╩дщбв╕хдмд╣д┘д╞─уддд╚днд└д▒г╧г╦бг
		 */
		if(accent0[phrase_ptr[i]]==1){
			if(accent0[phrase_ptr[j]]==1){
				goto ACC_PHRASE_SEPARATE;
			}else{
				goto ACC_PHRASE_INTEGRATE;
			}
		}else{
			goto ACC_PHRASE_180;
		}
	}else{
		/*
		 *	┴░дмг▓╟я░╩╛хд╬д╚днд╧бв
		 *	┴░д╬║╟╕хдм─удд▓╗д╩дщбв╕хдмд╣д┘д╞─уддд╚днд└д▒г╧г╦бг
		 *	┴░д╬║╟╕хдм╣тдд▓╗д╩дщбв╕хд╬└ш╞мдм╣тддд╚днд╚╕хдмд╣д┘д╞─уддд╚днд└д▒г╧г╦бг
		 */
		if(accent0[phrase_ptr[j]-1]!=1){
			if(accent0[phrase_ptr[j]]!=1){ goto ACC_PHRASE_INTEGRATE; }
		}
	}
ACC_PHRASE_180:;
	/* ╕хдмд╣д┘д╞─уд▒дьд╨г╧г╦бг*/
	for(si=(&accent0[phrase_ptr[j]]),cx=phrase_len[j]; cx--; ++si){
		if(*si!=1){ goto ACC_PHRASE_SEPARATE; }
	}
ACC_PHRASE_INTEGRATE:;
	phrase_len[i]+=phrase_len[j];
	phrase_ptr[j]=phrase_len[j]=0;
	goto ACC_PHRASE_100;
ACC_PHRASE_SEPARATE:;
	++i;
	goto ACC_PHRASE_110;
}


integer_16 ACC_OUTPUT_C(al,di)
	BYTE al; register integer_16 di;
{
	if(di>=MAX_SPEAK){ jp_ERROR(Err_NaibuOVF); }
	speak[di]=al;
	return (di+1);
}


integer_16 ACC_OUTPUT_N(num,len,di) integer_16 num,len,di; {
	register integer_16 b,l;
	/* ╜╨╬╧д╣ды┐Ї╗·д╬╖х┐ЇдЄ─┤д┘ды */
	for(b=10,l=1;b<=num;b*=10,l++)
		;
	b/=10;
	/* ╖х┐Їдм┬ндъд╩д▒дьд╨0дЄ╩фдж */
	for(;len>l;len--){
		di=ACC_OUTPUT_C('0',di);
	}
	/* ┐Ї╗·дЄ╜╨╬╧д╣ды */	
	/*while(len--){*/
	while(l--){
		di=ACC_OUTPUT_C((num/b)+'0',di); num%=b; b/=10;
	}
	return di;
}


integer_16 ACC_OUTPUT_S(str,di)
	register BYTE *str; register integer_16 di;
{
	while(*str){ di=ACC_OUTPUT_C(*str++,di); }
	return di;
}

/***************************** END OF ACC_5000.C *****************************/
