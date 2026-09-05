/*********************************************
	ILE_TDIC.C	updated at 30th April 1990
	単漢字辞書
 *********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_edic.h"
#include	"ile_envh.h"
#include	"ile_fddh.h"
#include	"ile_kkhh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_tdic.h"
#include	"ile_func.h"
#include "ile_rnsc.h"


/*========== TDIC1000() ============================================
	機能：単漢字辞書に現在注目語がある可能性があるかどうかを調べる。
	値　：	＝０	無視
			＝１	読まない記号
			＝２	平仮名
			＝３	単漢字辞書使用、辞書にないときは１と同じ。
  ==================================================================*/
integer_16 TDIC1000(){
	register BYTE *si;
	BYTE al,ah;

	al=inert_kind[ceptr];
	if(al==CHRKIND_HKana){
		if(celen>2){ return 0; }
	}else{
		if(celen>1){ return 0; }
	}

	ah=al & 0xF0;
	if(ah==CHRKIND_Other1||ah==CHRKIND_Other2){
		if(inert[ceptr*2]==0x21 && inert[ceptr*2+1]==0x3C){	/* 単独の長音 */
			return 3;
		}
		return(onsei_addr->onsei_kouseisystem? 3:1);
	}

	switch(al){
	case CHRKIND_ADigit:
	case CHRKIND_KKana:
		return 0;
	case CHRKIND_Alpha:
		return(celen==1? 3:0);
	}

	/* 平仮名・漢字 */
	if((onsei_addr->onsei_usetankan==0)
	|| (conv_mode==CONV_FIRST)
	|| (ceptr+celen<=max_ceptr)){
		return 0;
	}

	if(al==CHRKIND_HKana){
		/* 平仮名は１文字はＯＫ、２文字は２文字目が拗音ならＯＫ。*/
		if(celen==1){ return 2; }
		si=inert+ceptr*2;
		if(*(si+2)==0x24){
			if(*(si+3)==0x63||*(si+3)==0x65||*(si+3)==0x67){
				return 2;
			}
		}
		return 0;
	}else{
		return 3;
	}
	/* 直前の要素が最後になりえないものなら駄目にする。
	 *	if(stc_ptr>0){
	 *		if(0==(Stz_PE.stz_ctl & BITCTL_LAST)){ return 0; }
	 *	}
	 */
}


/*========== TDIC2XXX() ================================================
	機能：inert[ceptr*2]から始まる１文字の漢字を単漢字辞書から検索する。
  ======================================================================*/
BOOL TDIC2XXX(){
	register WORD ax;
	register BYTE *si;

	if(TDIC2000()==0){ return 0; }		/* 初回変換 */
	si=inert+ceptr*2;
	ax=(*si<<8)|*(si+1);					/* 検索文字 */
	/*
	 * 辞書引き文字が漢字の時は、
	 *	 前後の文字が共に漢字以外なら訓読みとする。
	 *	 前後の文字のどちらかまたは両方が漢字なら音読みとする。
	 * 辞書引き文字が特殊文字の時は、
	 *	 校正モードの時は、校正読みだけを用いる。
	 */
	/*if(Stz_CE.stz_mru & BITMRU_ABORT){ return 1; }*/
L100:;
	if(ax>0x3000){						/* 漢字の時の処理 */
		if(is_zengo_kanji(ceptr)){
			/* 前後に漢字がある時、音読みならＯＫ。*/
			if(Stz_CE.stz_dman & BITTAN_ON){ return 1; }
		}else{
			/* 前後とも漢字以外の時、訓読みならＯＫ。*/
			if(Stz_CE.stz_dman & BITTAN_KUN){ return 1; }
		}
	}else{
		if(onsei_addr->onsei_kouseisystem){
			/* 校正モードのときは、校正読みならＯＫ。*/
			if(Stz_CE.stz_dman & BITTAN_KOUSEI){ return 1; }
		}
	}
	if(TDIC3000()==1){ goto L100; }		/* 次候補 */
	/* 条件に当てはまる候補がないときは、先頭の候補を使う。*/
	return(TDIC2000());
}


/*========== TDIC2000() ================================================
	機能：inert[ceptr*2]から始まる１文字の漢字を単漢字辞書から検索する。
  ======================================================================*/
BOOL TDIC2000(){
	register WORD ax,mx;
	BYTE *si,mh;
	LPBYTE bp;

	TDIC2100();				/* 指定された表記のある単漢字辞書ブロックを得る */
	RnsfReadEdic(edic_page_no,&edic_page_addr);
							/* 単漢字辞書を読み込み、edic_page_addrにそのアドレスを得る */
	bp=edic_page_addr;		/* 外部辞書ページ内アドレスを初期化する */
	si=inert+ceptr*2;		/* 検索文字のオフセット */
	ax=(*si<<8)|*(si+1);	/* 検索文字（ＪＩＳ）*/
TDIC2000_COMPARE:;
	/* ページの終わりなら探索すべき文字は辞書上にない。*/
	if(0x00==(mh=(*bp))){ goto TDIC2000_NOTFOUND; }
	/* 見出し文字（ＪＩＳ）を求める。*/
	/* (*(WORD *)bp)でアクセスするとハイ・ローが逆になるので注意！*/
	mx=(mh<<8)|*(bp+1);
	/* 見出しが小さければ次の見出しまでスキップする。*/
	if(ax>mx){
		bp+=(*(bp+2)+3); goto TDIC2000_COMPARE;
	}
	/* 見出しが大きければ探索すべき文字は辞書上にない。*/
	if(ax<mx){ goto TDIC2000_NOTFOUND; }

	/* 探索すべき文字は辞書上にある。*/
	/* 単漢字レコードのトータルサイズと属性フラグの位置を保存する。*/
	tdic_len=(*(bp+2));
	tdic_ofs=(WORD)(bp-edic_page_addr+3);
	TDIC4000();
	return 1;
TDIC2000_NOTFOUND:;
	return 0;
}


/*========== TDIC2100() ========================================
	機能：探索すべき文字のある単漢字辞書のブロック番号を求める。
  ==============================================================*/
void TDIC2100(){
	register BYTE *table;
	register integer_16 i;
	BYTE *si;
	WORD ax;

	si=inert+ceptr*2;							/* 検索文字のオフセット */
	ax=(*si<<8)|*(si+1);							/* 検索文字（ＪＩＳ）*/
	table=FddHDR+Fdd_Header.Fdd_TdicIndOfs;		/* インデックスのオフセット */

	/* インデックスサイズはたかだか１０なので二分探索の必要はない。*/
	for(i=0;i<Fdd_Header.Fdd_TdicNoPage;++i,table+=2){
		/* (*(WORD *)table)でアクセスするとハイ・ローが逆になるので注意！*/
		if(ax < ((*table<<8)|*(table+1))){ goto TDIC2100_FOUND; }
	}
TDIC2100_FOUND:;
	i--;
	/* ブロック番号からＦＤ上ページ番号を求める */
	edic_page_no=Fdd_Header.Fdd_TdicPageSize*i+Fdd_Header.Fdd_TdicPageOfs+1;
}


/*========== TDIC3000() ============
	機能：単漢字変換次候補
  ==================================*/
BOOL TDIC3000() 
{
	register WORD ax;
	integer_16 tdic_ofs_save;
	LPBYTE bp;
	
	/* 現在注目位置を取り出す。*/
	bp=edic_page_addr+tdic_ofs;
	tdic_ofs_save=tdic_ofs;
	/* 種別フラグとアクセント情報と発音情報とをとばす。*/
	ax=(*(++bp));
	bp+=(ax>>4)+(ax & 0x0F)+1;
	/* 現在注目位置を保存する。*/
	tdic_ofs=(integer_16)(bp-edic_page_addr);
	/* 次候補レコードのトータルサイズを更新する。*/
	tdic_len-=(tdic_ofs-tdic_ofs_save);
	if(tdic_len==0){ return 0; }	/* 見出し位置ならば次候補なし。*/
	TDIC4000();
	return 1;
}


/*========== TDIC4000() ==============
	機能：単漢字の接続情報を設定する。
  ====================================*/
void TDIC4000() {
	LPBYTE bp;

	/* 辞書を参照しない接続情報を設定する。*/
	TDIC4100();

	/* TDIC4100()はStz_CEを壊すのでコールする順序に注意！*/
	/* 種別フラグを設定する。*/
	bp=tdic_ofs+edic_page_addr;
	Stz_CE.stz_dman=(*bp);
}


/*========== TDIC4100() ==============================
	機能：単漢字の種別フラグ以外の接続情報を設定する。
  ====================================================*/
void TDIC4100() {
	register BYTE *si;
	register WORD ax;

	StzfElmKind(&Stz_CE,DICKIND_T);

	/* 記号かどうかによって後端番号を変える。（９０．４．３０）*/
	switch(inert_kind[ceptr] & 0xF0){
	case CHRKIND_Other1:	/* 先頭可記号 */
		StzfGrcdAdd(Fdd_Header.Fdd_GrcdToku1);
		StzfGrcdStore(Fdd_Header.Fdd_GrcdUsr);
		break;
	case CHRKIND_Other2:	/* 先頭不可記号 */
		/* 先頭不可記号でも、文字列の先頭なら先頭可記号とする。*/
		StzfGrcdAdd(ceptr? Fdd_Header.Fdd_GrcdToku2:Fdd_Header.Fdd_GrcdToku1);
		if(ceptr==0){
			StzfGrcdStore(Fdd_Header.Fdd_GrcdUsr);
		}
		break;
	default:		/* 仮名・英字・漢字 */
		StzfGrcdStore(Fdd_Header.Fdd_GrcdUsr);
		/*StzfGrcdStore(Fdd_Header.Fdd_GrcdGai);*/
		break;
	}

	/* 文法コード→後端番号・前方ベクトル番号・制御情報。*/
	StzfGrcdOR();
	/*
	 * 長音・撥音・促音・「を」は最初にはなりえない。
	 * 但し、「ん」、「ー」、「っ」などで始まる文が入力されたことを考慮して、
	 * リトライモードのときは、文の先頭にある先頭になりえない文字は、
	 * 先頭になりうるものとする。
	 */
	si=inert+ceptr*2;					/* 検索文字のオフセット */
	ax=(*si<<8)|*(si+1);				/* 検索文字（ＪＩＳ）*/
	if(conv_mode==CONV_FIRST || ceptr>0){
		if(ax==0x213C||ax==0x2443||ax==0x2472||ax==0x2473
					 ||ax==0x2543||ax==0x2572||ax==0x2573){
			Stz_CE.stz_ctl &= ~BITCTL_FIRST;
		}
	}

	/* フラグ２：＝漢語 */
	Stz_CE.stz_f2= 0x80|BITFLG2_KANGO;

	/* 平仮名に続く平仮名は先頭にならないようにする。*/
	if(ceptr>0 && *si==0x24 && omega[stc_out_ptr[stc_ptr-1]]!=0xFF){
		Stz_CE.stz_ctl &= ~BITCTL_FIRST;
	}
}


/*========== TDIC5000(di) ===============================================
	機能：現在要素の発音表記を出力データエリアのｄｉ文字目から格納する。
  =======================================================================*/
integer_16 TDIC5000(di) integer_16 di; {
	register integer_16 a_len,p_len;
	BYTE ah,al;
	BYTE *si,*ti;
	LPBYTE bp;
	integer_16 cx;

	/* 現在注目位置を取り出す。*/
	bp=tdic_ofs+edic_page_addr;
	/* 種別フラグをとばす。*/
	bp++;

	p_len=(*bp++);
	a_len=p_len>>4;				/* length of accent information */
	p_len &= 0x0F;				/* length of pronunciation information */

	/* アクセント生情報を取り出す。*/
	for(;a_len--;){
		al=(*bp++);
		ah=al/16; al=al%16;
		if(ah<10){ stc_accent_raw[stc_ptr][ah]=al; }
	}

	ACC0010();

	/* 発音情報を取り出す。*/
	if(p_len==0){
		cpron_len=0;

		si= &inert[ceptr*2]; ti=inert_kana;
		for(cx=celen*2;cx--;){ *ti++=(*si++); }
		*ti=0x00;

		cx=mkhatu(inert_kana,inert_pron);

		for(si=inert_pron;cx--;++si){
			di=OMEGA1000(*si,di); ++cpron_len;
		}
	}else{
		cpron_len=p_len;			/* 発音表記文字数を設定する */
		for(;p_len--;){ al=(*bp++); di=OMEGA1000(al,di); }
	}
	return di;
}


/*========== TDIC5009(di) ==========================
	機能：辞書に無い為に読めない単漢字の発音表記を
		出力データエリアのｄｉ文字目から格納する。
  ==================================================*/
integer_16 TDIC5009(di) register integer_16 di; {
	register BYTE *si;
	BYTE ah,al;

	/*
	 * アクセント生情報をセットする。
	 *	基本型は平板型。
	 *	複合したときは、前の語を平板化し、自分も高く付いて全体が尾高化する。
	 */
	stc_accent_raw[stc_ptr][0]=0;
	stc_accent_raw[stc_ptr][1]=1;
	stc_accent_raw[stc_ptr][2]=5;

	/* 発音記号をセットする。*/
	si=(&inert[ceptr*2]);
	ah=(*si++); al=(*si);
	/* 長音は特別扱い。*/
	if(ah==0x21 && al==0x3C){
		cpron_len=1; al=0xC0; goto TDIC5009_KANA;
	}
	/* 特殊文字は読まない。*/
	if(ah!=0x24){
		cpron_len=1; al=0xFF; di=OMEGA1000(al,di); goto TDIC5009_END;
	}
	/* 平仮名 */
	cpron_len=1;
	if(kana_mora()==0){ goto TDIC5009_END; }
	al=kkana_buf[0];
TDIC5009_KANA:;
	di=OMEGA1000(al,di);
	goto TDIC5009_END;
TDIC5009_END:;
	return di;
}

/***************************** END OF ILE_TDIC.C *****************************/
