/**********************************************
	ILE_NUMC.C	updated at 1st October 1990
	数字変換・助数詞の処理
 **********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_acch.h"
#include	"ile_edic.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_idic.h"
#include	"ile_kkhh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"
#include "pkind.h"

/*
 *	(1)　MAN, EXPの意味
 *		その数字の仮数部と指数部を表わす。
 *		一千と千を区別する為に、一千は、MAN=1、千は、MAN=0とする。
 *		十・百についても、都合で、MAN=0とする。
 *		十一‥十九は、仮数部を11..19、指数部を１として扱う。
 *		「数」・「何」・「幾」の仮数部はそれぞれ、21、22、23で表わす。
 *		また、符号、小数点は、仮数部をそれぞれ65、66とし、指数部を0とする。
 *
 *	(2)　撥音・促音は直後の音に依存するため、0xD7・0xCFなるコードで処理し、
 *	助数詞との結合後、0xD0..0xD4、0xC8..0xCB、に分類する。
 */

static const BYTE DPRON_TBL1[]={
	/* JIS    MAN EXP ACC.	   PRON. */	
	0x21,0x3B,  0, 0, 0x01, 0x02,0x83,0x64,		/* 〇 */
	0x30,0x6C,  1, 0, 0x02, 0x02,0x01,0x29,		/* 一 */
	0x46,0x73,  2, 0, 0x01, 0x01,0x39,			/* 二 */
	0x3B,0x30,  3, 0, 0x00, 0x02,0x10,0xD7,		/* 三 */
	0x3B,0x4D,  4, 0, 0x01, 0x02,0x5C,0xD7,		/* 四 */
	0x38,0x5E,  5, 0, 0x01, 0x01,0x74,			/* 五 */
	0x4F,0x3B,  6, 0, 0x02, 0x02,0x64,0x0A,		/* 六 */
	0x3C,0X37,  7, 0, 0x01, 0x02,0x38,0x38,		/* 七 */
	0x48,0x2C,  8, 0, 0x02, 0x02,0x40,0x29,		/* 八 */
	0x36,0x65,  9, 0, 0x01, 0x02,0x0E,0xC0,		/* 九 */
	0x3F,0x74, 21, 0, 0x01, 0x02,0x12,0xC0,		/* 数 */
	0x32,0x3F, 22, 0, 0x01, 0x02,0x38,0xD7,		/* 何 */
	0x34,0x76, 23, 0, 0x01, 0x02,0x01,0x0A,		/* 幾 */

	0x3D,0x3D,  0, 1, 0x01, 0x02,0x8A,0xC0,		/* 十 */
	0x3D,0x26,  0, 1, 0x01, 0x02,0x8A,0xC0,		/* 拾 */
	0x49,0x34,  0, 2, 0x02, 0x02,0x45,0x0A,		/* 百 */
	0x40,0x69,  0, 3, 0x01, 0x02,0x13,0xD7,		/* 千 */

	0x4B,0x7C,  0, 4, 0x01, 0x02,0x50,0xD7,		/* 万 */
	0x68,0x5F,  0, 4, 0x01, 0x02,0x50,0xD7,		/* 萬 */
	0x32,0x2F,  0, 8, 0x01, 0x02,0x04,0x0A,		/* 億 */
	0x43,0x7B,  0,12, 0x01, 0x02,0x2C,0xC0,		/* 兆 */
	0x35,0x7E,  0,16, 0x01, 0x02,0x0B,0xC0,		/* 京 */
	
	0x21,0x5C, 65, 0, 0x00, 0x03,0xA2,0x60,0x12,		/* ＋ */
	0x21,0x5D, 65, 0, 0x00, 0x04,0x50,0x01,0x38,0x12,	/* − */
	0x21,0x5E, 65, 0, 0x04, 0x07,0xA2,0x60,0x12,0x50,0x01,0x38,0x12,	/* ± */
	0x21,0x25, 66, 0, 0x01, 0x02,0x23,0xD7,				/* ． */

	0xFF										/* STOPPER */
};

static const BYTE DPRON_TBL2[]={
	/*   JIS		    MAN EXP ACC.		  PRON. */
	0x30,0x6C,0x3D,0x26,  2, 1, 0x03, 0x04,0x01,0x29,0x8A,0xC0,	/* 一拾 */
	0x46,0x73,0x3D,0x3D,  2, 1, 0x01, 0x03,0x39,     0x8A,0xC0,	/* 二十 */
	0x46,0x73,0x3D,0x26,  2, 1, 0x01, 0x03,0x39,     0x8A,0xC0,	/* 二拾 */
	0x3B,0x30,0x3D,0x3D,  3, 1, 0x01, 0x04,0x10,0xD7,0x8A,0xC0,	/* 三十 */
	0x3B,0x30,0x3D,0x26,  3, 1, 0x01, 0x04,0x10,0xD7,0x8A,0xC0,	/* 三拾 */
	0x3B,0x4D,0x3D,0x3D,  4, 1, 0x01, 0x04,0x5C,0xD7,0x8A,0xC0,	/* 四十 */
	0x3B,0x4D,0x3D,0x26,  4, 1, 0x01, 0x04,0x5C,0xD7,0x8A,0xC0,	/* 四拾 */
	0x38,0x5E,0x3D,0x3D,  5, 1, 0x02, 0x03,0x74,     0x8A,0xC0,	/* 五十 */
	0x38,0x5E,0x3D,0x26,  5, 1, 0x02, 0x03,0x74,     0x8A,0xC0,	/* 五拾 */
	0x4F,0x3B,0x3D,0x3D,  6, 1, 0x03, 0x04,0x64,0x0A,0x8A,0xC0,	/* 六十 */
	0x4F,0x3B,0x3D,0x26,  6, 1, 0x03, 0x04,0x64,0x0A,0x8A,0xC0,	/* 六拾 */
	0x3C,0x37,0x3D,0x3D,  7, 1, 0x02, 0x04,0x38,0x38,0x8A,0xC0,	/* 七十 */
	0x3C,0x37,0x3D,0x26,  7, 1, 0x02, 0x04,0x38,0x38,0x8A,0xC0,	/* 七拾 */
	0x48,0x2C,0x3D,0x3D,  8, 1, 0x03, 0x04,0x40,0x29,0x8A,0xC0,	/* 八十 */
	0x48,0x2C,0x3D,0x26,  8, 1, 0x03, 0x04,0x40,0x29,0x8A,0xC0,	/* 八拾 */
	0x36,0x65,0x3D,0x3D,  9, 1, 0x01, 0x04,0x0E,0xC0,0x8A,0xC0,	/* 九十 */
	0x36,0x65,0x3D,0x26,  9, 1, 0x01, 0x04,0x0E,0xC0,0x8A,0xC0,	/* 九拾 */
	0x3F,0x74,0x3D,0x3D, 21, 1, 0x01, 0x04,0x12,0xC0,0x8A,0xC0,	/* 数十 */
	0x32,0x3F,0x3D,0x3D, 22, 1, 0x01, 0x04,0x38,0xD7,0x8A,0xC0,	/* 何十 */
	0x34,0x76,0x3D,0x3D, 23, 1, 0x01, 0x04,0x01,0x0A,0x8A,0xC0,	/* 幾十 */

	0x30,0x6C,0x49,0x34,  2, 2, 0x04, 0x04,0x01,0xC8,0xA5,0x0A,	/* 一百 */
	0x46,0x73,0x49,0x34,  2, 2, 0x03, 0x03,0x39,     0x45,0x0A,	/* 二百 */
	0x3B,0x30,0x49,0x34,  3, 2, 0x01, 0x04,0x10,0xD7,0x9D,0x0A,	/* 三百 */
	0x3B,0x4D,0x49,0x34,  4, 2, 0x01, 0x04,0x5C,0xD7,0x45,0x0A,	/* 四百 */
	0x38,0x5E,0x49,0x34,  5, 2, 0x03, 0x03,0x74,     0x45,0x0A,	/* 五百 */
	0x4F,0x3B,0x49,0x34,  6, 2, 0x04, 0x04,0x64,0xC8,0xA5,0x0A,	/* 六百 */
	0x3C,0x37,0x49,0x34,  7, 2, 0x02, 0x04,0x38,0x38,0x45,0x0A,	/* 七百 */
	0x48,0x2C,0x49,0x34,  8, 2, 0x04, 0x04,0x40,0xC8,0xA5,0x0A,	/* 八百 */
	0x36,0x65,0x49,0x34,  9, 2, 0x01, 0x04,0x0E,0xC0,0x45,0x0A,	/* 九百 */
	0x3F,0x74,0x49,0x34, 21, 2, 0x01, 0x04,0x12,0xC0,0x45,0x0A,	/* 数百 */
	0x32,0x3F,0x49,0x34, 22, 2, 0x01, 0x04,0x38,0xD7,0x9D,0x0A,	/* 何百 */
	0x34,0x76,0x49,0x34, 23, 2, 0x01, 0x04,0x01,0x0A,0x45,0x0A,	/* 幾百 */

	0x30,0x6C,0x40,0x69,  1, 3, 0x03, 0x04,0x01,0xC8,0x13,0xD7,	/* 一千 */
	0x46,0x73,0x40,0x69,  2, 3, 0x02, 0x03,0x39,     0x13,0xD7,	/* 二千 */
	0x3B,0x30,0x40,0x69,  3, 3, 0x03, 0x04,0x10,0xD7,0x83,0xD7,	/* 三千 */
	0x3B,0x4D,0x40,0x69,  4, 3, 0x03, 0x04,0x5C,0xD7,0x13,0xD7,	/* 四千 */
	0x38,0x5E,0x40,0x69,  5, 3, 0x02, 0x03,0x74,     0x13,0xD7,	/* 五千 */
	0x4F,0x3B,0x40,0x69,  6, 3, 0x03, 0x04,0x64,0x0A,0x13,0xD7,	/* 六千 */
	0x3C,0x37,0x40,0x69,  7, 3, 0x03, 0x04,0x38,0x38,0x13,0xD7,	/* 七千 */
	0x48,0x2C,0x40,0x69,  8, 3, 0x03, 0x04,0x40,0xC8,0x13,0xD7,	/* 八千 */
	0x36,0x65,0x40,0x69,  9, 3, 0x03, 0x04,0x0E,0xC0,0x13,0xD7,	/* 九千 */
	0x3F,0x74,0x40,0x69, 21, 3, 0x01, 0x04,0x12,0xC0,0x13,0xD7,	/* 数千 */
	0x32,0x3F,0x40,0x69, 22, 3, 0x03, 0x04,0x38,0xD7,0x83,0xD7,	/* 何千 */
	0x34,0x76,0x40,0x69, 23, 3, 0x01, 0x04,0x01,0x0A,0x13,0xD7,	/* 幾千 */

	0x4A,0X23,0X3F,0X74, 64, 0, 0x03, 0x04,0x42,0x0A,0x12,0xC0,	/* 複数 */

	0x3D,0x3D,0x30,0x6C, 11, 1, 0x04, 0x04,0x8A,0xC0,0x01,0x29,	/* 十一 */
	0x3D,0x26,0x30,0x6C, 11, 1, 0x04, 0x04,0x8A,0xC0,0x01,0x29,	/* 拾一 */
	0x3D,0x3D,0x46,0x73, 12, 1, 0x03, 0x03,0x8A,0xC0,0x39,		/* 十二 */
	0x3D,0x26,0x46,0x73, 12, 1, 0x03, 0x03,0x8A,0xC0,0x39,		/* 拾二 */
	0x3D,0x3D,0x3B,0x30, 13, 1, 0x01, 0x04,0x8A,0xC0,0x10,0xD7,	/* 十三 */
	0x3D,0x26,0x3B,0x30, 13, 1, 0x01, 0x04,0x8A,0xC0,0x10,0xD7,	/* 拾三 */
	0x3D,0x3D,0x3B,0x4D, 14, 1, 0x03, 0x04,0x8A,0xC0,0x5C,0xD7,	/* 十四 */
	0x3D,0x26,0x3B,0x4D, 14, 1, 0x03, 0x04,0x8A,0xC0,0x5C,0xD7,	/* 拾四 */
	0x3D,0x3D,0x38,0x5E, 15, 1, 0x01, 0x03,0x8A,0xC0,0x74,		/* 十五 */
	0x3D,0x26,0x38,0x5E, 15, 1, 0x01, 0x03,0x8A,0xC0,0x74,		/* 拾五 */
	0x3D,0x3D,0x4F,0x3B, 16, 1, 0x04, 0x04,0x8A,0xC0,0x64,0x0A,	/* 十六 */
	0x3D,0x26,0x4F,0x3B, 16, 1, 0x04, 0x04,0x8A,0xC0,0x64,0x0A,	/* 拾六 */
	0x3D,0x3D,0x3C,0x37, 17, 1, 0x03, 0x04,0x8A,0xC0,0x38,0x38,	/* 十七 */
	0x3D,0x26,0x3C,0x37, 17, 1, 0x03, 0x04,0x8A,0xC0,0x38,0x38,	/* 拾七 */
	0x3D,0x3D,0x48,0x2C, 18, 1, 0x04, 0x04,0x8A,0xC0,0x40,0x29,	/* 十八 */
	0x3D,0x26,0x48,0x2C, 18, 1, 0x04, 0x04,0x8A,0xC0,0x40,0x29,	/* 拾八 */
	0x3D,0x3D,0x36,0x65, 19, 1, 0x03, 0x04,0x8A,0xC0,0x0E,0xC0,	/* 十九 */
	0x3D,0x26,0x36,0x65, 19, 1, 0x03, 0x04,0x8A,0xC0,0x0E,0xC0,	/* 拾九 */

	0xFF														/* STOPPER */
};

static	STZ_STRUCT Stz_PE_SAV;	/* 接続関係 保存領域 */
static	STZ_STRUCT Stz_CE_SAV;	/* 接続関係 保存領域 */

/*========== NUMB1000() ==================================================
	機能：位置ceptrから文字数celenの文字列が数字変換できるかどうか調べる。
	値　：１＝可能性有り／０＝可能性無し
  ========================================================================*/
BOOL NUMB1000() {
	BYTE al;
	register integer_16 ptr;

	/* 初回放棄語検索指定のときは、処理しない。*/
	/*****if(Stz_CE.stz_mru & BITMRU_ABORT)then return 0; fi*****/

	/* 数字以外の文字を含んでいたら、可能性無し。*/
	if((al=OTHER2000())==0){ return 0; }
	if(al!=CHRKIND_Digit){ return 0; }

	/*
	 *	前の文字が数字なら、処理しない。--- ウソ
	 *		東京１、十三１、定数２、２進数１、等、
	 *		名詞の最後が数字で終わっている場合があるので、
	 *		このチェックはやらない方がよい。
	 *	但し小数点のときは例外的に処理する。このとき小数点は区切り記号である。
	 *	if((ptr=ceptr-1)!=(-1)){
	 *		if((inert_kind[ptr] & 0xF0)==CHRKIND_Digit
	 *		&& (inert_kind[ptr]!=CHRKIND_DDP)){ return 0; }
	 *	}
	 *
	 *
	 * 後の文字が数字でなければ、処理する。
	 *	if((ptr=ceptr+celen)==n_inert){ return 1; }
	 *	if((inert_kind[ptr] & 0xF0)!=CHRKIND_Digit){ return 1; }
	 *
	 *
	 *	後の文字が数字のとき、------ うそ
	 *		十、百、千、万、億、兆ならば処理する。	（１００／億円）
	 *		小数点ならば処理する。					（３／．／５…）
	 *		それ以外は処理しない。
	 *	数字の後に「数」、「何」、「幾」ではじまる語や、「八戸」のような語が
	 *	くるかもしれないのでこの処理はやめる。
	 *	if(inert_kind[ptr]==CHRKIND_KDigit2){ return 1; }
	 *	if(inert_kind[ptr]==CHRKIND_KDigit4){ return 1; }
	 *	if(inert_kind[ptr]==CHRKIND_DDP){ return 1; }
	 *	return 0;
	 */
	return 1;
}


/*========== NUMB5000(sw) ==================================================
	機能：現在要素開始位置から始まる数字の発音を出力データエリアに格納する。
		スタック操作、及び文節位置設定もこのルーチンで行われる。
	注意：撥音・促音は直後の音に依存するため、0xD7・0xCFなるコードで処理し、
		助数詞との結合後、0xD0..0xD4、0xC8..0xCB、に分類する。
	入力：ｓｗ＝０：読み方指定なし。
		　ｓｗ＝１：桁読み指定。
		　ｓｗ＝２：棒読み指定。
  ===========================================================================*/
BOOL NUMB5000(sw) integer_16 sw; {
	const BYTE* tbl;
        BYTE* si;
	integer_16 dman,dexp,frac_len,int_len;
	integer_16 di,bx,cx,dptr,dlen,dptr2,dlen2,dif,dlast_precede;

	if(kkhstat==err){ return 0; }

	int_len=NUM0010();
	frac_len=NUM0020();
	DIGIT_CONV(sw);
	digit_mode=0;		/* reset digit convert mode (to integer) */
	dptr=0;
NUMB5000_100:;
	di=cpron_ptr;
	if((dlen=n_kansuji-dptr)==0){ goto NUMB5000_SUCCESS; }
	if(dlen>2){ dlen=2; }
	dptr2=dptr;	dlen2=dlen;
NUMB5000_200:;
	if(dlen2==1){ tbl=DPRON_TBL1; }else{ tbl=DPRON_TBL2; }
	while(1){
		if(*tbl==0xFF){ goto NUMB5000_NF; }	/* not found */
		si=(&kansuji[dptr2*2]); cx=dlen2*2; dif=0;
		for(;cx--;){
			if(*si-*tbl){ dif++; }
			++si; ++tbl;
		}
		if(dif==0){ goto NUMB5000_F; }			/* found */
		tbl+=3; cx=(*tbl); tbl+=cx+1;
	}
NUMB5000_NF:;										/* NOT FOUND !*/
	if((dlen2=(--dlen))==0){ return 0; }		/* convert disable */
	goto NUMB5000_200;								/* search length 1 */

NUMB5000_F:;										/* FOUND !*/
	stc_ed_ofs[stc_ptr]=pkind_digiti0;
	Stz_CE.stz_dman=(*tbl); ++tbl;
	Stz_CE.stz_dexp=(*tbl);
	stc_ed_ofs[stc_ptr]+=(*tbl); ++tbl;
	if(stc_ed_ofs[stc_ptr]>pkind_digiti3){ stc_ed_ofs[stc_ptr]=pkind_digiti3; }
		/*
		 *	stc_ed_ofs[] =	0..9	 : pkind_digiti0
		 *					10..90	 : pkind_digiti0+1
		 *					100..900 : pkind_digiti0+2
		 *					>=1000	 : pkind_digiti0+3
		 */
	stc_accent_raw[stc_ptr][0]=(*tbl++);			/* basic accent type */
	stc_accent_raw[stc_ptr][1]=0;
	stc_accent_raw[stc_ptr][2]=0;
	/* ３以外は「の」が低く付く。*/
	if(Stz_CE.stz_dman!=3 || Stz_CE.stz_dexp!=0){
		stc_accent_raw[stc_ptr][6]=2;
	}
	dlast_precede=di-1;
													/* move pronunciation data */
	cx=cpron_len=(*tbl++);							/* mora length */

	/* コンマ等、区切り */
	/* if(cx==0){ dptr+=dlen; goto NUMB5000_100; } */

	for(;cx--;){ di=OMEGA1000(*tbl++,di); }

	if(Stz_CE.stz_dman || Stz_CE.stz_dexp<4 || dptr==0){
		goto NUMB5000_500;
	}

	/* 但し、校正モードなら、そのまま出力する。--89.12.12-- */
	if(onsei_addr->onsei_kouseisystem){ goto NUMB5000_500; }

	/***********************************************/
	/* 万・億・兆・京 の場合、直前の数字と結合する */
	/***********************************************/

	/* 兆・京の直前の一、八、十、十一、十八は促音化する。*/
	if(Stz_CE.stz_dexp!=12 && Stz_CE.stz_dexp!=16){ goto NUMB5000_320; }
	dman=stc_dman[stc_ptr-1];
	dexp=stc_dexp[stc_ptr-1];
	/* 一、八、十、十の倍数、十一、十八ならば、*/
	if(dman==11||dman==18){ goto NUMB5000_310; }		/* 11,18 */
	if(dman>10){ goto NUMB5000_320; }					/* 12..17,19 */
	if(dexp==1){ goto NUMB5000_310; }					/* 10,20..90 */
	if(dexp!=0){ goto NUMB5000_320; }					/* >=100 */
	if(dman==1||dman==8){ goto NUMB5000_310; }			/* 1,8 */
	goto NUMB5000_320;										/* else */
NUMB5000_310:
	omega[dlast_precede]=0xCF;
NUMB5000_320:
	/**
	 **	１１..１９のときは、stz_dexpを１にしておく。
	 **	１１..１９は特別扱いであるが、１１万・１１億・１１兆は普通扱いでよい。
	 **	if(Stz_PE.stz_dman<=9 && Stz_PE.stz_dexp==0
	 **	&& stc_out_len[stc_ptr-1]>2){
	 **		stc_dman[stc_ptr-1]=2;
	 **		stc_dexp[stc_ptr-1]=1;
	 **		stc_ed_ofs[stc_ptr-1]=pkind_digiti0+1;
	 **	}
	 **/

	/* 前の数字は平板化し、万は頭高、億・兆・京は低く付く */
	if(Stz_CE.stz_dexp==4){
		stc_accent_raw[stc_ptr-1][0]=stc_out_len[stc_ptr-1]+1;
	}else{
		/* 何・数・幾 ＋ 億・兆・京は、頭高化する */
		stc_accent_raw[stc_ptr-1][0] =
			( (stc_dman[stc_ptr-1]>=21 && stc_dman[stc_ptr-1]<=23)
				? 0x01 : stc_out_len[stc_ptr-1] );
	}
	stc_ed_ofs[stc_ptr-1]+=Stz_CE.stz_dexp;
	if(stc_ed_ofs[stc_ptr-1]>pkind_digiti3){
		stc_ed_ofs[stc_ptr-1]=pkind_digiti3;
	}
	stc_dexp[stc_ptr-1]+=Stz_CE.stz_dexp;
	stc_out_len[stc_ptr-1]+=2;

	dptr+=dlen;				/* 次の入力データ位置 */
	cpron_ptr=di;			/* 出力データ中間エリアの位置を保存する */
	/*
	 * 「七百万」の変換において、「七百」を変換した時点で、sent_in_ptr[]は、
	 * ３になっている。「万」を変換したときに「万」の長さ１を加えてはいけない。
	 *		sent_in_ptr[bx]+=dlen; --- ウソ
	 */
	sent_in_ptr[bx]=ceptr+celen;
	sent_out_ptr[sent_ptr]=di;
	memcpy(&Stz_PE,&Stz_CE,sizeof(STZ_STRUCT));
							/* 単語接続関連情報を１単語進める */
	goto NUMB5000_100;

NUMB5000_500:
	/*
	 *	一〜九の直前の十、五十、六十、八十、は平板化する。
	 *	このとき、一〜九は尾高化する ---- ウソ
	 *
	 *	if(Stz_CE.stz_dexp==0 && Stz_CE.stz_dman<=9){
	 *		if(stc_dexp[stc_ptr-1]==1){
	 *			switch(stc_dman[stc_ptr-1]){
	 *			case 0: case 5: case 6: case 8:
	 *				stc_accent_raw[stc_ptr-1][0]=0;
	 *				stc_accent_raw[stc_ptr][0]=cpron_len;
	 *			}
	 *		}
	 *	}
	 */
	/*
	 *	小数部または、棒読み型の二、五は長音化する。
	 *	但し、小数のときは、小数部が１桁のときは長音化しない。
	 *	また、棒読みの１桁は長音化しない。
	 */
	if((digit_mode==1 && frac_len>1)||(digit_type==1 && int_len>1)){
		stc_ed_ofs[stc_ptr]=pkind_digitf;
		switch(Stz_CE.stz_dman*100+Stz_CE.stz_dexp){
		case 200:
		case 500:
			++cpron_len; di=OMEGA1000(0xC0,di);
		}
	}

	/*
	 *	小数点なら小数モードに切り替える。
	 *	小数点の直前の数字が１０未満のときは、数字を頭高化する。--- ???
	 *	小数点の直前の一、八、十は促音化する。
	 *	小数点の直前の二、五は長音化する。
	 */
	if(Stz_CE.stz_dman==DMANKIND_DP){
		digit_mode=1;
		/* 直前に数字があれば */
		if(dptr!=0){
			/*if(stc_dexp[stc_ptr-1]==0){ stc_accent_raw[stc_ptr-1][0]=1; }*/

			/* 一、八、十、十の倍数ならば、促音化する。*/
			/* 十一〜十九は、一〜九と同じ扱いにする。*/
			if(stc_dman[stc_ptr-1]<11 || 19<stc_dman[stc_ptr-1]){
				if(stc_dexp[stc_ptr-1]==1){ goto NUMB5000_540; }
			}

			switch(stc_dman[stc_ptr-1]*100+stc_dexp[stc_ptr-1]){
			case 100: case 800: case 1101: case 1801:
				NUMB5000_540:
				/* 校正モードでないときだけ促音化する。--89.12.12-- */
				if(onsei_addr->onsei_kouseisystem==0){
					omega[dlast_precede]=0xCF;
				}
				break;
			/* 二、五ならば */
			case 200: case 500: case 1201: case 1501:
				/*
				 * 「６３．５．５」の真中の５は小数点の直後なので長音化するが、
				 *  小数点の直前でもある。二重に長音化しない為に、
				 *  既に長音で終わっているかどうかを調べる。-- 88.6.13 --
				 */
				if(0xC0!=omega[di-3]){
					omega[di]=omega[di-1]; omega[di-1]=omega[di-2];
					omega[di-2]=0xC0;
					++di;
					stc_out_len[stc_ptr-1]=stc_out_len[stc_ptr-1]+1;
				}
			}
		}
	}

	cpron_ptr=di;			/* 出力データ中間エリアの位置を保存する */
	bx=sent_ptr;		 	/* 出力データ中間エリアの文節番号を得る */
	/* 先頭の数字のみ文節最初になり得る */
	if(dptr){
		Stz_CE.stz_ctl &= ~BITCTL_FIRST;
	}else{
		Stz_CE.stz_ctl |= BITCTL_FIRST;
	}
	if(dptr==0){			/* dptr==0 のときだけ文節最初になりうる */
							/* 接頭辞を伴わない最初になりえる語ならば */
		if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_FIRST)
		&& StzfAttrAri(Stz_CE,stz_stz,BITSTZ_PREF)==0){
			/* 出力可能最大文節数を超えているならば */
			if(bx>=MAX_SENT){ jp_ERROR(Err_NaibuOVF); }
			/* 先頭語でなければ文節を進める */
			if(ceptr!=0){ bx++; sent_ptr=bx; }
		}
	}
	/* 文節の最後の文字の位置を１オリジンで設定する */
	sent_out_ptr[bx]=di;
	sent_in_ptr [bx]=ceptr+celen;	/*dptr+dlen;*/

	STACK_PUSH();
	/*
	 *	PUSHの際には、ceptr・celen は数字列の先頭と全体の長さを表わしており、
	 *	それぞれの PUSH 要素とは対応していないので注意すること !!
	 */ 
	dptr+=dlen;				/* 次の入力データ位置 */
	memcpy(&Stz_PE,&Stz_CE,sizeof(STZ_STRUCT));
							/* 単語接続関連情報を１単語進める */
	goto NUMB5000_100;

NUMB5000_SUCCESS:
	FOLLOW_INIT(&Stz_CE);
	ceptr+=celen;
	return 1;
}


/*========== NUMB5XXX(sw) ==================================================
	機能：先行要素が数字のとき現在要素をセーブして先行する数字を変換し直す。
	入力：ｓｗ＝０：読み方指定なし。
		　ｓｗ＝１：桁読み指定。
		　ｓｗ＝２：棒読み指定。
  ===========================================================================*/
void NUMB5XXX(sw) integer_16 sw; {
	/* 先行要素が数字ならそれを変換し直す。*/
	if(stc_kind[stc_ptr-1]==DICKIND_D){
		/*
		 *	数字および、数字の直後の要素の接続情報は保存しておく。
		 *
		 *	数字の情報は数字の発音を求め直したときに変わっていることがある。
		 *	即ち、数字の直前が「第・図・表」のときは、
		 *	数字が漢語接続に変わっているのである。
		 *	NUMB5000()の実行後はこの漢語接続情報が消えてしまっている。
		 *	従って、数字の接続情報は保存する必要がある。
		 *	しかしながら、数字の漢語接続化をFOLLOW()ではなく、
		 *	NUMB5000()でするように書き換えれば問題はないはずである。
		 *
		 *	数字の直後の要素の接続情報はすぐ下で使っている。
		 */
		memcpy(&Stz_PE_SAV,&Stz_PE,sizeof(STZ_STRUCT));
		memcpy(&Stz_CE_SAV,&Stz_CE,sizeof(STZ_STRUCT));
		edic_page_no_sav=edic_page_no;
		edic_ofs_sav=edic_ofs;
		idic_addr_sav=idic_addr;
		ceptr_sav=ceptr; celen_sav=celen;

		/* 数字をpopして、変換し直す。cpron_ptrは変わることがある。*/
		STACK_POP();

		StzfElmKind(&Stz_CE,DICKIND_D);		/* 単語種別を数詞とする。*/
		OTHER_GRCD();						/* 数字の文法情報を設定する。*/

		/*
		 * PEが接頭辞の場合、CEを文節の切れ目にならなくする。
		 * 但し、CTLをこわすとアクセント処理で困るのでSTZに書込む。
		 *		この情報はNUMB5000(sw)で用いる。
		 */
		if(ceptr!=0){
			if(StzfAttrAri(Stz_PE,stz_ctl,BITCTL_PREF)){
				Stz_CE.stz_stz |= BITSTZ_PREF;
			}
		}
		NUMB5000(sw);

		/* 助数詞（または数字の直後の要素）の接続情報を戻す。*/
		edic_page_no=edic_page_no_sav;
		edic_ofs=edic_ofs_sav;
		idic_addr=idic_addr_sav;
		ceptr=ceptr_sav; celen=celen_sav;
		memcpy(&Stz_PE,&Stz_PE_SAV,sizeof(STZ_STRUCT));
		memcpy(&Stz_CE,&Stz_CE_SAV,sizeof(STZ_STRUCT));
		stc_stz[stc_ptr-1]=Stz_PE.stz_stz;
	}
}


/*========== NUM0010() ===============
	機能：数字の整数部の桁数を求める。
  ====================================*/
integer_16 NUM0010() {
	BYTE *si;
	integer_16 len,cx;

	si=(&inert[ceptr*2]);
	len=0;
	for(cx=celen;cx--;){
		/* 終了条件は小数点ではなく、数字以外とする。*/
		/* 「２万」の整数部を１桁にしたいからである。*/
		/* 校正モードでも、「ニーマン」ではなく「ニマン」と読みたい。*/
		/*if(*si==0x21 && *(si+1)==0x25){ goto NUM0010_200; }*/
		if(*si!=0x23 || *(si+1)<0x30 || *(si+1)>0x39){ goto NUM0010_200; }
		++len; si+=2;
	}
NUM0010_200:;
	return len;
}


/*========== NUM0020() ===============
	機能：数字の小数部の桁数を求める。
  ====================================*/
integer_16 NUM0020() {
	BYTE *si;
	integer_16 len,cx;

	si=(&inert[(ceptr+celen-1)*2]);
	len=0;
	for(cx=celen;cx--;){
		if(*si==0x21 && *(si+1)==0x25){ goto NUM0020_200; }
		++len; si-=2;
	}
NUM0020_200:;
	if(cx==-1){ len=0; }
		/* for文でcx--としているので、{を抜けるとcxは-1になっている。*/
	return len;
}


/*========== DIGIT7000() =====================
	機能：数字＋助数詞の相互の影響を考察する。
  ============================================*/
void DIGIT7000() {
	BYTE acc3,acc4,acc5;
	register integer_16 sp_d,sp_n;
	integer_16 dptr,dlen,nptr,nlen,dlast,dman,dexp,ln,di,l;
	integer_16 complex;	/* 複合数字なら１とする。*/

	/* integer_16	sent_in_ptr[MAX_SENT];				 入力文字位置 */
	/* integer_16	sent_out_ptr[MAX_SENT];				 出力文字位置 */
	/* BYTE			stc_accent_raw[MAX_STACK][10];		 raw accent data */
	/* BYTE			inert[MAX_INERT*2];					 inert data */
	/* BYTE			omega[MAX_OMEGA];					 pronunciation data */

	sp_d=stc_ptr-2;		/* stack pointer for digit */
	sp_n=stc_ptr-1;		/* stack pointer for numerator */

	stc_dman[sp_n] = DMANKIND_NUM;			/*助数詞*/

	/* 校正モードでは、何もしない。--89.12.12-- */
	/* 但し、stc_dman[sp_n]は、セットすること。*/
	if(onsei_addr->onsei_kouseisystem){ return; }

	dptr=stc_out_ptr[sp_d]; dlen=stc_out_len[sp_d];
	nptr=stc_out_ptr[sp_n]; nlen=stc_out_len[sp_n];
	/*DKIND=stc_dman[sp_d]*100+stc_dexp[sp_d];*/
	acc3=stc_accent_raw[sp_n][2];
	acc4=stc_accent_raw[sp_n][3];
	acc5=stc_accent_raw[sp_n][4];
	dlast=dptr+dlen-1;
	dman=stc_dman[sp_d];
	dexp=stc_dexp[sp_d];

	/* 数字が１０００以上のときはなにもしない。*/
	if(dexp>=3){ return; }

	/* 数字が１０..９０、数十、数百、１００..９００のときの処理 */
	/* dexp==1 && dman>=11 && dman<=19 のときは除外する。*/
	/* １００..９００の最終拍の促音化は５・６型のときだけ起こる。*/
	if((dexp==1 && (dman<10||dman==21||dman==22||dman==23))
	|| (dexp==2 && (acc3==5||acc3==6))){
		/* 数字の最終拍の促音化 */
		if(acc3>=1 && acc3<=6){ omega[dlast]=0xCF; }
		/* 助数詞の先頭拍の変化（ＰＡ行化）*/
		if(acc5>=2 && acc5<=7){ DIGIT7020(&omega[nptr]); }
		return;
	}
	/* 促音化しない１００..９００は何もしない。*/
	if(dexp==2){ return; }


	/***********************************************************************
		以下のコーディングで「if(stc_out_len[sp_d]==2){」などが、
	　注釈になっている理由は、基本数詞に、１，１１，２，１２，３，１３など、
	　単独の１..９以外に、単独の１１..１９があるからである。
		但し、acc4==1のときの変化は１１..１９には適用されない。
	************************************************************************/

	/*
	 * １〜９以外は複合数字とみなす。１１〜１９、９８０１、等。
	 * 「１箱」は「ひとはこ」、「１１箱」、「１０１箱」は「・・いちはこ」？
	 *			・・・どうしようかな・・・
	 * 但し、「２４時」、「２４時間」は、「・・よ・・」。
	 */
	complex=((stc_ptr>2)&&(stc_kind[stc_ptr-3]==DICKIND_D))||(dman>10);

	/* 数字が１..９，１１..１９のときの処理 */
	switch(dman){
	case 1:
	case 11:
		/* 数字の読みの変化 */
		/*if(1==acc4 && 1==dman){*/
		if(1==acc4 || 2==acc4){
			if(!complex){
				/* 「いち」→「ひと」 */
				omega[dlast-1]=0x41; omega[dlast]=0x24;
			}
		}else{
			/* 数字の最終拍の促音化（＄７）。もちろん「ひと」には適用しない。*/
			switch(acc3){
			case 2: case 4: case 5: case 6:
				omega[dlast]=0xCF;
			}
			/* 助数詞の１拍目のＰＡ行化 */
			switch(acc5){
			case 2: case 3: case 5: case 6:
				DIGIT7020(&omega[nptr]);
			}
		}
		break;
	case 2:
	case 12:
		/* 数字の読みの変化 */
		/* 小数部などで、長音化した２は適用範囲外である。*/
		/*if(acc4==1 && omega[dlast]!=0xC0){*/
		/*if(1==acc4 && 2==dman){*/
		if(1==acc4 || 2==acc4){
			if(!complex){
				/* 「に」→「ふた」 */
				stc_out_len[sp_d]++; stc_out_ptr[sp_n]++;
				di=cpron_ptr;
				ln=stc_out_len[sp_n];
				for(l=0;l<ln;++l){ omega[di-l]=omega[di-l-1]; }
				omega[dlast]=0x42; omega[dlast+1]=0x20;
				sent_out_ptr[sent_ptr]++;
				cpron_ptr++;
			}
		}
		break;
	case 3:
	case 13:
		/* 数字の読みの変化 */
		/*if(1==acc4 && 3==dman){*/
		if(1==acc4){
			if(!complex){
				/* 「さん」→「み」 */
				stc_out_len[sp_d]--; stc_out_ptr[sp_n]--;
				ln=stc_out_len[sp_n]; di=cpron_ptr;
				omega[dlast-1]=0x51;
				for(;ln;--ln){ omega[di-ln-1]=omega[di-ln]; }
				sent_out_ptr[sent_ptr]--;
				cpron_ptr--;
				stc_accent_raw[sp_d][0]=1;
			}
		}else{
			/* 助数詞の１拍目の変化 */
			switch(acc5){
			case 1:								/* 濁音化 */
				DIGIT7010(&omega[nptr]); break;
			case 4: case 5: case 6: case 7:		/* バ行 */
				DIGIT7030(&omega[nptr]); break;
			case 2: case 3:						/* パ行 */
				DIGIT7020(&omega[nptr]);
			}
		}
		break;
	case 4:
	case 14:
		/* 数字の読みの変化 */
		/*if(7==acc3 || 11==acc3 || (1==acc4 && 4==dman)){*/
		if(7==acc3 || 11==acc3 || 1==acc4){
			if(1==acc4 && complex){ break; }
			/*削*if(!complex){*/
				/* 「よん」→「よ」 */
				stc_out_len[sp_d]--; stc_out_ptr[sp_n]--;
				ln=stc_out_len[sp_n]; di=cpron_ptr;
				for(;ln;--ln){ omega[di-ln-1]=omega[di-ln]; }
				sent_out_ptr[sent_ptr]--;
				cpron_ptr--;
				/* 基本アクセントは、４も１４も１（頭高）である。*/
				stc_accent_raw[sp_d][0]=1;
			/*}*/
		}else{
			/* 助数詞の１拍目の変化 */
			switch(acc5){
			case 2: case 3:
				DIGIT7020(&omega[nptr]); break;		/* P */
			case 7:
				DIGIT7030(&omega[nptr]);			/* B */
			}
		}
		break;
	case 6:
	case 16:
		/* 数字の最終拍の促音化（＄７）*/
		switch(acc3){
		case 3: case 5: case 6:
			omega[dlast]=0xCF;
		}
		/* 助数詞の１拍目のＰＡ行化 */
		switch(acc5){
		case 2: case 3: case 4: case 5: case 6: case 7:
			DIGIT7020(&omega[nptr]);
		}
		break;
	case 7:
	case 17:
		/* 数字の読みの変化 */
		switch(acc3){
		case 8: case 10: case 11:
			/*削*if(!complex){*/
				/* 「なな」→「しち」 */
				ln=stc_out_len[sp_n]; di=cpron_ptr;
				omega[dlast-1]=0x19; omega[dlast]=0x29;
				/* 基本アクセントは、７は２（尾高）、１７は４（尾高）である。*/
				stc_accent_raw[sp_d][0]=(7==dman ? 2:4);
			/*削*}*/
		}
		break;
	case 8:
	case 18:
		/* 数字の最終拍の促音化（＄７）*/
		switch(acc3){
		case 4: case 6:
			omega[dlast]=0xCF;
		}
		/* 助数詞の１拍目のＰＡ行化 */
		switch(acc5){
		case 3: case 6:
			DIGIT7020(&omega[nptr]);
		}
		break;
	case 9:
	case 19:
		/* 数字の読みの変化 */
		switch(acc3){
		case 9: case 10: case 11:
			/*削*if(!complex){*/
				/* 「きゅー」→「く」 */
				stc_out_len[sp_d]--; stc_out_ptr[sp_n]--;
				ln=stc_out_len[sp_n]; di=cpron_ptr;
				omega[dlast-1]=0x0A;
				for(;ln;--ln){ omega[di-ln-1]=omega[di-ln]; }
				sent_out_ptr[sent_ptr]--;
				cpron_ptr--;
				/* 基本アクセントは、９も１９も１（頭高）である。*/
				stc_accent_raw[sp_d][0]=1;
			/*削*}*/
		}
		break;
	}
}


/*========== DIGIT7010(ch) ===================================
	機能：chを濁音化する。
  ============================================================*/
void DIGIT7010(ch) register BYTE *ch; {
	switch(*ch & 0xF8){
	case 0x08:	*ch=(*ch & 0x07) | 0x78; break;		/* KA --> QA */
	case 0x10:	*ch=(*ch & 0x07) | 0x80; break;		/* SA --> ZA */
	case 0x18:	*ch=(*ch & 0x07) | 0x88; break;		/* SHA --> JA */
	case 0x28:	*ch=(*ch & 0x07) | 0x88; break;		/* CHA --> JA */
	case 0x20:	*ch=(*ch & 0x07) | 0x90; break;		/* TA --> DA */
	case 0x40:	*ch=(*ch & 0x07) | 0x98; break;		/* HA --> BA */
	case 0x68:	*ch=(*ch & 0x07) | 0x98; break;		/* WA --> BA */
	}
}


/*========== DIGIT7020(ch) ===================================
	機能：chをP化する。
  ============================================================*/
void DIGIT7020(ch) register BYTE *ch; {
	switch(*ch & 0xF8){
	case 0x40: case 0x68:
		*ch=(*ch & 0x07) | 0xA0;	/* HA,WA --> PA */
	}
}


/*========== DIGIT7030(ch) ===================================
	機能：chをB化する。
  ============================================================*/
void DIGIT7030(ch) register BYTE *ch; {
	switch(*ch & 0xF8){
	case 0x40: case 0x68:
		*ch=(*ch & 0x07) | 0x98;	/* HA,WA --> BA */
	}
}


/***************************** END OF ILE_NUMC.C *****************************/
