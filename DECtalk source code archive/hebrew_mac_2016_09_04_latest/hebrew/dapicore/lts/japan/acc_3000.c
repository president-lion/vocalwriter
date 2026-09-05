/***********************************************
	ACC_3000.C	updated at 30th October 1989
	促音・撥音・長音に関する一般的規則
 ***********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_acch.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_func.h"


/*========== ACC3000() =========================================
	機能：アクセントの滝が、促音、撥音、長音、の直後にあるとき、
		アクセントの滝を一拍前に移動する。
  ==============================================================*/
void ACC3000() {
	register integer_16 i,j;
	integer_16 cx;
	BYTE ch;
	for(i=0;i<stc_ptr;++i){
		if((cx=phrase_len[i])>2){
			for(j=phrase_ptr[i]+1,cx-=2;cx--;++j){
				ch=omega[j];
				if(ch>=0xC0 && ch<=0xD7){
					if(accent0[j]!=1 && accent0[j+1]==1){
						accent0[j-1]=0x82; accent0[j]=1;
					}
				}
			}
		}
	}
}

/*========== ACC4000() ======================================================
	機能：撥音・促音の区別。

	　数字音声変換では、撥音・促音を区別していないので、
	撥音・促音を分類する必要がある。
	ile_dtbl.hでは、撥音を0xD7、促音を0xCF、で定義しておく。
	　内部辞書・外部辞書の項目については、最後のモーラが撥音・促音のときのみ、
	次のモーラに依存する。
	最後以外のモーラは、次のモーラに依存するとは限らないので注意する。
	　撥音の後にポーズが入る場合は、Ｘ２とする。

	88.11.22
	例外はないようなので、すべてチェックする。
  ===========================================================================*/
void ACC4000() 
{ 
	check_X(omega,cpron_ptr);
}


/*========== ACC4100(addr) ===================================
	機能：撥音は、次のモーラに依存して、いくつかに分類される。

		ＮＩ・ＪＩの前は、Ｘ３
		ＰＡ行・ＭＡ行・ＢＡ行の前は、Ｘ１
		ＮＡ行・ＴＡ行・ＤＡ・ＳＡ行の前は、Ｘ２
		ＧＡ行・ＱＡ行・ＫＡ行の前は、Ｘ４
		Ａ行・ＨＡ行の前は、Ｘ５
  ============================================================*/
void ACC4100(addr) register BYTE *addr; {
	BYTE next_mora;
	next_mora=(*(addr+1));

	switch(next_mora){
	case 0x39: case 0x89:
		*addr=0xD2;	return;					/* X3 before (NI),(JI) */
	}

	switch(next_mora & 0xF8){
	case 0xA0: case 0x50: case 0x98:
		*addr=0xD0; break;					/* X1 before PA,MA,BA */
	case 0x38: case 0x20: case 0x90: case 0x10:
		*addr=0xD1; break;					/* X2 before NA,TA,DA,SA */
	case 0x08: case 0x70: case 0x78:
		*addr=0xD3; break;					/* X4 before GA,QA,KA */
	case 0x00: case 0x40:
		*addr=0xD4; break;					/* X5 before A,HA */
	default:
		*addr=0xD1; break;					/* X2 before default */
	}
}

/*========== ACC4200(addr) ===================================
	機能：促音は、次のモーラに依存して、いくつかに分類される。

		ＳＡ行の前は、＄１
		ＳＨＡ行の前は、＄２
		ＨＡ行の前は、＄３
		それ以外は、＄
  ============================================================*/
void ACC4200(addr) register BYTE *addr; {
	switch(*(addr+1) & 0xF8){
	case 0x10:	*addr=0xC9; break;			/* $1 before SA */
	case 0x18:	*addr=0xCA; break;			/* $2 before SHA */
	case 0x40:	*addr=0xCB; break;			/* $3 before HA */
	default:	*addr=0xC8; break;			/* $ before default */
	}
}

/*========== check_X(p,l) =========================
	機能：撥音と促音を前後の文字によって分類する。

	88.11.22
	例外はないようなので、すべてチェックする。
  =================================================*/
void check_X(p,l) BYTE *p; integer_16 l; {
	register integer_16 len; register BYTE *ptr;
	/* 撥音 */
	for(len=l,ptr=p;len--;++ptr){
		/*if(*ptr==0xD7){*/
		if((*ptr & 0xF0)==0xD0){
			if(len==0){
				*ptr=0xD1;
			}else{
				ACC4100(ptr);
			}
		}
	}
	/* 促音 */
	for(len=l,ptr=p;len--;++ptr){
		/*if(*ptr==0xCF){*/
		if((*ptr & 0xF8)==0xC8){
			if(len==0){
				*ptr=0xC8;
			}else{
				ACC4200(ptr);
			}
		}
	}
}

/***************************** END OF ACC_3000.C *****************************/
