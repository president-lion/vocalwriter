/************************************************
	ILE_STCC.C	updated at 14th November 1989
	スタック関連
 ************************************************/

#define		EXT		extern

#if defined(DEBUG_STC)
#include	<stdio.h>
#endif

#include	"onsei.h"
#include	"ile_bunh.h"
#include	"ile_dbgh.h"
#include	"ile_edic.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_idic.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_tdic.h"
#include	"ile_func.h"

BYTE		stc_accent_raw[MAX_STACK][10] = {{0}};	/* アクセント生データ */

/*========== STACK_INIT() ===============
	機能：辞書引きスタックを初期化する。
  ======================StcfShokikasuru==*/
void STACK_INIT() {
	register integer_16 cx;
	stc_ptr=0;
	/* initialize next accent data region */
	for(cx=10;cx--;){ stc_accent_raw[stc_ptr][cx]=0x00; }
	/*stc_accent_raw[stc_ptr][4]=0xFF;	第２アクセント */
}


/*========== STACK_PUSH() ===============
	機能：辞書引きスタックに情報を積む。
  =======================StcfStackTsumu==*/
void STACK_PUSH() {
	register integer_16 cx,sp;

#if defined(DEBUG_STC)
	if(debug[1]||debug[2]){ DEBUG_PN("<STC_PUSH>"); }
#endif

	if(stc_ptr==MAX_STACK){ jp_ERROR(Err_NaibuOVF); }

	RtkfChudanShiraberu();				/* 処理中断要求があれば変換を中止する*/
	sp=stc_ptr;							/* 使用可能なスタック開始位置を得る */
	stc_out_len[sp]=cpron_len;			/* 出力データ文字数 */
	stc_out_ptr[sp]=(sp==0 ? 0 : stc_out_ptr[sp-1]+stc_out_len[sp-1]);
	stc_in_ptr[sp]=ceptr;				/* 入力データ位置 */
	stc_in_len[sp]=celen;				/* 入力データ文字数 */
	stc_f2[sp] =Stz_CE.stz_f2;		/* フラグ２（単漢字辞書の種別フラグ）*/
	stc_ctl[sp]=Stz_CE.stz_ctl;		/* 制御情報 */
	stc_mru[sp]=Stz_CE.stz_mru;		/* 学習情報 */
	stc_stz[sp]=Stz_CE.stz_stz;		/* 接続状態 */
	stc_dman[sp]=Stz_CE.stz_dman;	/* 数字情報 */
	stc_dexp[sp]=Stz_CE.stz_dexp;	/* 数字情報 */

	cx=stc_nbno[sp]=Stz_CE.stz_nbno;	/* 後端番号数 */
	if(cx){	/* 後端番号があるならば 後端番号を保存する (cxは１オリジン) */
		do{ stc_bno[sp][cx-1]=Stz_CE.stz_bno[cx-1]; }while(--cx);
	}
	cx=stc_ngrcd[sp]=Stz_CE.stz_ngrcd;	/* 文法コード数 */
	if(cx){	/* 文法コードがあるならば保存する (cxは１オリジン) */
		do{ stc_grcd[sp][cx-1]=Stz_CE.stz_grcd[cx-1]; }while(--cx);
	}

	switch(stc_kind[sp]=Stz_CE.stz_kind){
	case DICKIND_5Y:					/* 五段動詞自動発生連用形語尾 */
		stc_in_len[sp]=0;
	case DICKIND_E:						/* 外部自立語辞書語 */
		stc_ed_pag[sp]=edic_page_no;
		stc_ed_ofs[sp]=edic_ofs;
		break;
	case DICKIND_T:						/* 単漢字辞書語 */
		stc_ed_pag[sp]=edic_page_no;
		stc_ed_ofs[sp]=tdic_ofs;
		stc_dexp[sp]=tdic_len;
		break;
	case DICKIND_I:						/* 内部付属語辞書 */
		stc_ed_pag[sp]=0;
		stc_ed_ofs[sp]=(WORD)(idic_addr-FddHDR);
		break;
	/* case DICKIND_D: --- stc_ed_ofs is digit kind. (int. or frac.) */
	}

	stc_ptr++;
	/* initialize next accent data region */
	for(cx=10;cx--;){ stc_accent_raw[stc_ptr][cx]=0x00; }
	/*stc_accent_raw[stc_ptr][4]=0xFF;	第２アクセント */

#if defined(DEBUG_STC)
	if(debug[1]){ DEBUG_STC_ALL(); }
	if(debug[2]){ DEBUG_STC_CE(stc_ptr); }
#endif
}


/*========== STACK_POP() ====================
	機能：辞書引きスタックから情報をおろす。
  ===========================StcfStackOrosu==*/
void STACK_POP() {
	register integer_16 sp,cx;

#if defined(DEBUG_STC)
	if(debug[3]){ DEBUG_PN("<STACK_POP>"); DEBUG_STC_CE(stc_ptr); }
#endif

	RtkfChudanShiraberu();			/* 処理中断要求があれば変換を中止する*/

	if(stc_kind[stc_ptr-1]==DICKIND_5Y){				/* 五段連用語尾 */
		/* 接続情報に関連する情報を２単語戻す*/
		--stc_ptr;
		sp=stc_ptr-1;
		Stz_CE.stz_kind=stc_kind[sp];

		if(cx=Stz_CE.stz_nbno=stc_nbno[sp]){
			do{ Stz_CE.stz_bno[cx-1]=stc_bno[sp][cx-1]; }while(--cx);
		}
		if(cx=Stz_CE.stz_ngrcd=stc_ngrcd[sp]){
			do{ Stz_CE.stz_grcd[cx-1]=stc_grcd[sp][cx-1]; }while(--cx);
		}

		Stz_CE.stz_ctl=stc_ctl[sp];				/* 制御情報 */
		Stz_CE.stz_f2=stc_f2[sp];				/* フラグ２ */
		Stz_CE.stz_stz=stc_stz[sp];				/* 接続状態 */
		Stz_CE.stz_mru=stc_mru[sp];				/* 学習情報 */
		Stz_CE.stz_dman=stc_dman[sp];			/* 数字情報 */
		Stz_CE.stz_dexp=stc_dexp[sp];			/* 数字情報 */
	}else{
		/* 接続情報に関連する情報を１単語戻す*/
		StzfElmBackward();
	}

STACK_POP_100:
	stc_ptr--;
	sp=stc_ptr;
	ceptr=stc_in_ptr[sp];			/* current element pointer */
	celen=stc_in_len[sp];			/* current element length */
	cpron_ptr=stc_out_ptr[sp];		/* current pronunciation pointer */
									/* 現在注目語関連情報を戻す */
	switch(stc_kind[sp]){
	case DICKIND_I:					/* 内部付属語辞書語 */
		idic_addr=FddHDR+stc_ed_ofs[sp]; break;
	case DICKIND_D:					/* 内部合成数字 */
		if(sp==0){ break; }	/* no more stack exist */
		/*
		 * 先行語も数字なら、先行語が数字でなくなるまでｐｏｐする。
		 *	「三十二」の「二」をｐｏｐしたら、「三十」も必ずｐｏｐする。
		 *	「１．|一」の「一」をｐｏｐしたら、「１．」はｐｏｐしてはならない。
		 *	すなわち、先頭になりうる数字までｐｏｐする。
		 */
		if(stc_ctl[sp] & BITCTL_FIRST){ break; }
		if(stc_kind[sp-1]==DICKIND_D){
			Stz_CE.stz_ctl=stc_ctl[sp-1];
			goto STACK_POP_100;
		}
		break;
	case DICKIND_E:					/* 外部自立語辞書語 */
		edic_page_no=stc_ed_pag[sp];	/* ページ番号 */
		edic_ofs=stc_ed_ofs[sp];		/* ページ内オフセット */
		break;
	case DICKIND_T:					/* 単漢字辞書語 */
		edic_page_no=stc_ed_pag[sp];	/* ページ番号 */
		tdic_ofs=stc_ed_ofs[sp];		/* ページ内オフセット */
		tdic_len=stc_dexp[sp];			/* レコードトータルサイズ */
		break;
	}

	OMEGA6000();						/* 出力結果を戻す */
	if(stc_ptr){						/* 先行語が残っていれば先行語関連情報を戻す*/
		sp=stc_ptr-1;
		StzfElmKind(&Stz_PE,stc_kind[sp]);
		if(cx=Stz_PE.stz_nbno=stc_nbno[sp]){
			do{ Stz_PE.stz_bno[cx-1]=stc_bno[sp][cx-1]; }while(--cx);
		}
		if(cx=Stz_PE.stz_ngrcd=stc_ngrcd[sp]){
			do{ Stz_PE.stz_grcd[cx-1]=stc_grcd[sp][cx-1]; }while(--cx);
		}
		Stz_PE.stz_f2=stc_f2[sp];			/* フラグ２ */
		Stz_PE.stz_ctl=stc_ctl[sp];			/* 制御情報 */
		Stz_PE.stz_mru=stc_mru[sp];			/* 学習情報 */
		Stz_PE.stz_stz=stc_stz[sp];			/* 接続状態 */
		Stz_PE.stz_dman=stc_dman[sp];		/* 数字情報 */
		Stz_PE.stz_dexp=stc_dexp[sp];		/* 数字情報 */
	}

	/* initialize next accent data region */
	for(cx=10;cx--;){ stc_accent_raw[stc_ptr][cx]=0x00; }
	/*stc_accent_raw[stc_ptr][4]=0xFF;	第２アクセント */
}


/*========== STACK_POP_FS() =========================================
	機能：辞書引きスタックから利用者注目文節直前までの情報をおろす。
		実行後、ceptr(current element pointer)と
		fsptr(following sentence pointer)が等しくなる。
  ================================================StcfBunOnajiOrosu==*/
void STACK_POP_FS() {
	while(ceptr>fsptr){ STACK_POP(); }
}


/*========== STACK_POP_WS() =====================================
	機能：辞書引きスタックから利用者注目文節までの情報をおろす。
		実行後、ceptr(current element pointer)と
		wsptr(watching sentence pointer)が等しくなる。
  ============================================StcfBunTemaeOrosu==*/
void STACK_POP_WS() {
	while(ceptr>wsptr){ STACK_POP(); }
}


#if defined(DEBUG_STC)
/* debug routine for STACK */
void DEBUG_STC_ALL() {
	register integer_16 p;
	for(p=1;p<=stc_ptr;p++){ DEBUG_STC_CE(p); }
}

void DEBUG_STC_CE(ptr) integer_16 ptr; {
	register integer_16 p,i; integer_16 n,sip,sil;
	p=ptr-1; sip=stc_in_ptr[p]; sil=stc_in_len[p];
	printf(" (%d) iP=%d",ptr,sip+1); printf(":%d",sil);
	/*printf(",oP=%d",stc_out_ptr[p]+1); printf(":%d",stc_out_len[p]);*/
	putchar(','); DEBUG_K1(&inert[sip*2],sil);
	switch(stc_kind[p]){
	case DICKIND_E:  printf(",外部"); break;
	case DICKIND_I:  printf(",内部"); break;
	case DICKIND_D:  printf(",数字"); break;
	case DICKIND_5Y: printf(",連用"); break;
	case DICKIND_K:  printf(",片仮名"); break;
	case DICKIND_T:  printf(",単漢字"); break;
	}
	printf(",F2="); DEBUG_H1(stc_f2[p]);
	printf(",CTL="); DEBUG_H1(stc_ctl[p]);
	printf(",MRU="); DEBUG_H1(stc_mru[p]);
	printf(",STZ="); DEBUG_H1(stc_stz[p]);
	printf(",BNO=");
	for(n=stc_nbno[p],i=0;n--;i++){
		printf("%d",stc_bno[p][i]); if(n>0){ putchar(','); }
	}
	putchar('\n');
}			
#endif

/***************************** END OF ILE_STCC.C *****************************/
