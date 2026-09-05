/*************************************************
	ILE_EDIC.C	updated at 13th September 1990
	外部辞書
 *************************************************/

#define		EXT		extern

#include <stddef.h>
#include	"onsei.h"
#include	"ile_edic.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_jknh.h"
#include	"ile_kkhh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"
#include "ile_rnsc.h"

void ACC0010(void);

LPBYTE Fdd_EdicIndex = NULL;

/*========== EDIC1000() ==========================================
	機能：外部辞書に現在注目語がある可能性があるかどうかを調べる
  ===============================================GjsfKanouseiAri==*/
BOOL EDIC1000() {
	BYTE al;
 	/*
	 *	外部辞書見出しの最大文字数を超えている場合はなし。
	 *	辞書検索表に表記が無いことが記録されている場合はなし。
	 *
	 *	促音・撥音で始まる語は外部辞書にない。--- 嘘：「ヵ月」など。
	 *	if(inert_attr[ceptr]==CHRATTR_Soku)then goto nashi; fi
	 */
	if(celen>MAX_EDIC){ goto nashi; }
	if(JknfAttrAri(BITDUT_ED_NONE)){ goto nashi; }
	/*
	 * 初回放棄語検索指示で、初回放棄語でなければなし。
	 * if(StzfAttrAri(Stz_CE,stz_mru,BITMRU_ABORT) &&
	 * (JknfAttrAri(BITDUT_ED_ABORT)==0)){ goto nashi; }
	 */
	/* 後方辞書引きの指示がない場合は記号以外はなし。*/
	if(ceptr!=0){
		if(pe_is_kango){ goto ari; }
		if(StzfAttrAri(Stz_PE,stz_ctl,BITCTL_FEDIC)==0){
			al=inert[ceptr*2];
			if(al==0x24){ goto nashi; }
			if(al==0x25){ goto nashi; }
			if(al>0x2F){ goto nashi; }
		}
	}
	ari:	return 1;
	nashi:	return 0;
}


/*========== EDIC2XXX() ====================================================
	機能：現在語の表記と同じ表記を持ち現在の要求状態に合致する、外部辞書の
 		ページ内オフセットを求め、現在語の接続情報を設定する。
  =====================================================GjsfYomChantoSagasu==*/
BOOL EDIC2XXX() {
	LPBYTE bp;

	/* 辞書検索表に求める語があるならば、*/
	if(JknfAttrAri(BITDUT_ED_EXIST)){
		/* 指定された表記のある外部辞書ブロックを得る。*/
		EDIC2100();
		/* 外部辞書を読み込み、edic_page_addrにそのページ先頭アドレスを得る。*/
		RnsfReadEdic(edic_page_no,&edic_page_addr);
		/* 辞書検索表からページ内オフセットを取り出しフラグ２の位置を保存する。*/
		edic_ofs=JknfEdicToridasu;
		bp=edic_ofs+edic_page_addr;
	}else{
		/* なければちゃんとさがす。*/
		if(EDIC2000()){
			bp=edic_ofs+edic_page_addr;
			/* 探している語があったなら、外部辞書にあったことを記録する。*/
			JknfAttrAdd(BITDUT_ED_EXIST);
			/* 辞書検索表にページ内オフセットをしまう。*/
			JknfEdicShimau(edic_ofs);
		}else{
			/* なかったら、外部辞書になかったことを記録する。*/
			JknfAttrAdd(BITDUT_ED_NONE);
			return 0;
		}
	}

	/* 学習状態ならば現在要素を採用する。*/
	if(kkhstat!=mru){
		/* 現在注目要素が初回放棄語の場合。*/
		/* 注意：この処理は文節切れ目の学習の為にあるので、
				 放棄フラグのない同字語を探してはならない。*/
		if(*(bp+1) & BITMRU_ABORT){
			/* 初回放棄語を検索対象としない場合は、*/
			if(StzfAttrNashi(Stz_CE,stz_mru,BITMRU_ABORT)){
				/* 辞書検索表に初回放棄語を記録して、注目要素は破棄する。*/
				JknfAttrAdd(BITDUT_ED_ABORT);
				return 0;
				/*return EDIC3XXX();*/
			}
		}else{
			/* 注目要素が初回採択要素のときは、
			   初回放棄語を検索対象とする場合は、注目要素は破棄する。*/
			if(StzfAttrAri(Stz_CE,stz_mru,BITMRU_ABORT)){
				return 0;
				/*return EDIC3XXX();*/
			}
		}
	}
	
	/* 現在注目語が利用者削除語の場合、*/
	if(*(bp+1) & BITMRU_DEL){
		/* 次の同字語を捜す。*/
		if(EDIC3XXX()==0){				/* 次の同字語がないならば終り。*/
			JknfAttrAdd(BITDUT_ED_NONE);	/* 外部辞書にはないことを記録する。*/
			return 0;
		}
	}

	/* 現在語の接続情報を設定する。*/
	EDIC4000();
	return 1;
}


/*========== EDIC2000() ===================================================
	機能：現在要素開始位置から始まる現在要素長さの語がある外部辞書の
		ページ内オフセットとページ先頭アドレスを求める。
  ==========================================================GjsfYomSagasu==*/
BOOL EDIC2000() {
	register BYTE *si;
	register integer_16 bx;
	BYTE cechr,al;
	integer_16 ax,dist,ed_nsame_old;
	LPBYTE bp,f1;

	EDIC2100();				/* 指定された表記のある外部辞書ページ番号を得る。*/
	RnsfReadEdic(edic_page_no,&edic_page_addr);
							/* 外部辞書を読み込み、edic_page_addrにその先頭アドレスを得る。*/
	bp=edic_page_addr;		/* 外部辞書ページ内アドレスを初期化する。*/
	bx=0;					/* 比較済みバイト数を初期化する。*/
	ed_nsame=0;

	si=ceptr*2+inert;

	while(1){
		f1=bp;		/* フラグ１の位置を保存する。*/
		/* フラグ1を取り出し直前語と表記の一致するバイト数を求める。*/
		ax=(*bp++) & 0x1F;	/* tail 5 bit */
		/* 外部辞書の終わりなら ブロックの終了位置を保持する。*/
		if(ax==0x1F){ goto EDIC2000_997; }
		/* get distance to next flag 1 */
		dist=(*bp++);
		/* 一致するバイト数が比較済みバイト数より大きいなら
		   以降の比較は無駄なので次のフラグ１まで読みとばす。*/
		if(ax>bx){ goto EDIC2000_500; }
		/* 同じか小さいなら、比較済みバイト数を再設定する。*/
		if(ax<bx){ goto EDIC2000_997; }
		/*bx=ax;*/
		/* 比較済みバイト数が現在語の表記の長さと等しくなるまで */
		while(bx!=celen*2){
			cechr=si[bx];
			al=(*bp);					/* １バイト取り出す */
			/* フラグ２ならば次のフラグ１まで読みとばす。*/
			if(sign(al)){ goto EDIC2000_500; }
			bp++;
			/* 現在語よりも小さければ、次のフラグ１まで読みとばす。*/
			if(al<cechr){ goto EDIC2000_500; }
			/* 現在語と比較して大きいなら */
			if(al>cechr){ goto EDIC2000_997; }	/* 現在語は辞書にない */
			/* 一致した長さを保存する。（これは辞書登録で使われる）*/
			++bx; ed_nsame=bx;
			/*ed_same[bx++]=al;*/
		}
		/* 現在語と現在注目語が完全に一致したら */
		if((*bp & 0xC0)==0x80){	/* 10?????? */
			/*ed_same[bx]=0;*/		/* 注目読みの後にゼロを付け加える。*/
			edic_ofs=(WORD)(bp-edic_page_addr);	/* フラグ２の位置を保存する。*/
			/* 表記があったとして戻る。*/
			/* ed_nsame==celen*2 である。*/
			/*return edic_ofs;*/
			return 1;
		}else{
			goto EDIC2000_997;
		}

		EDIC2000_500:
		ed_nsame_old=ed_nsame;
		bp=f1+dist+2;	/* 次のフラグ１まで読み飛ばす。*/
	}
EDIC2000_997:;
	/*
	 *	見つからなかった時は、ひとつ前の語と一致したバイト数をセットする。
	 *	edic_ofsは、フラグ１のアドレスを指すようにしておく。
	 *	これは、辞書登録のときに使用される。
	 */
	ed_nsame=ed_nsame_old;
	edic_ofs=f1-edic_page_addr;
	return 0;
}


/*========== EDIC2100() ======================================================
	機能：現在語開始位置から始まる現在語長さの語がある外部辞書ブロック番号と
		そのブロックのページ番号を求める。
  =============================================================GjmfYomSagasu==*/
void EDIC2100() {
	register BYTE *si;
	register integer_16 dif;
	integer_16 cx,len,cebyte;
	integer_16 LL,UL,IP;
	LPBYTE di;
	WORD offset;

	cebyte=celen*2;

	LL=0;								/* ページ番号下限 */
	UL=Fdd_Header.Fdd_EdicNoPage;		/* ページ番号上限 */
	IP=UL/2;							/* 初回比較位置 */

	do{
		si=inert+ceptr*2;				/* 検索文字のオフセット */
		di=3*IP+Fdd_EdicIndex;
		cx=(WORD)*(di+2);

		/* offset=(*(WORD far *)di); */
		offset= *di + (*(di+1)<<8);
		di=Fdd_EdicIndex+Fdd_Header.Fdd_EdicKeyOfs+offset;

		/* 表記を比較する。*/
		for(len=cebyte;len--;--cx){
			if(cx){
				if(dif=(*di++)-(*si++)){ goto EDIC2100_170; }
			}else{
				dif=(-1); goto EDIC2100_170;
			}
		}
		if(cx){
			dif=1;
		}else{
			goto EDIC2100_900;
		}

		EDIC2100_170:;
		/*
		 * 注目外部辞書見出しが大きければ 表引き上限を現在注目番号にする。
		 * 注目外部辞書見出しが小さければ 表引き下限を現在注目番号にする。
		 */
		if(dif>0){
			UL=IP; IP=(LL+IP)/2; dif=IP-UL;
		}else{
			LL=IP; IP=(IP+UL)/2; dif=IP-LL;
		}
	}while(dif);						/* 前回比較位置と違っていれば繰り返す。*/

EDIC2100_900:
	/* ブロック番号からＦＤ上ページ番号を求める */
	edic_page_no=Fdd_Header.Fdd_EdicPageSize*IP+Fdd_Header.Fdd_EdicPageOfs+1;
}


/*========== EDIC3XXX() ===============================
  機能：  外部辞書の語の次の同音意義語の位置を求める。
  ===============================GjsfTsugiTangoSagasu==*/
BOOL EDIC3XXX() {
	LPBYTE bp;

	while(1){
		/* 次候補が無いときは終り。*/
		if(EDIC3000()==0){ return 0; }
		bp=edic_ofs+edic_page_addr;
		if((*(bp+1) & BITMRU_DEL)==0){ return 1; }
		/* 削除語ならば次を捜す。*/
	}
}


/*========== EDIC3000() ===============================
  機能：  外部辞書の語の次の同音意義語の位置を求める。
  ===============================GjsfTsugiTangoSagasu==*/
BOOL EDIC3000() {
	LPBYTE bp;
	BYTE al;

	bp=edic_ofs+edic_page_addr;		/* 現在注目位置を取り出す。*/

	/* フラグ２があればそれをとばす。なければ辞書が壊れている。*/
	if((*bp & 0xC0)!=0x80){ jp_ERROR(Err_Fatal_Edic); }
	bp++;

	/* 学習情報とあれば文法コードをとばす。*/
	do{ bp++; }while(sign(*bp));
	/* アクセント情報と発音情報とをとばす。*/
	al=(*bp);
	bp+=(al>>4)+(al & 0x0F)+1;

	edic_ofs=(WORD)(bp-edic_page_addr);	/* 現在注目位置を保存する。*/
	if((*bp & 0xE0)==0xE0){ return 0; }		/* フラグ１ならば次候補なし。*/

	/* 現在語の接続情報を設定する。*/
	EDIC4000();
	return 1;
}


/*========== EDIC4000() ====================
	機能：現在語の接続情報を設定する
  ====================GjsfGzgStzSetteisuru==*/
void EDIC4000() {
	LPBYTE bp;
	BYTE al,ah;

	StzfElmKind(&Stz_CE,DICKIND_E);	/* 単語種別を外部辞書とする。*/
	bp=edic_ofs+edic_page_addr;		/* 現在語のアドレス。*/
	Stz_CE.stz_f2=(*bp++);			/* フラグ２を取り出す。*/
	al=(*bp++);
	ah=Stz_CE.stz_mru;				/*（前回放棄語フラグを持ち越す）*/
	ah &= BITMRU_ABORT;
	al &= ~BITMRU_ABORT;
	Stz_CE.stz_mru=al|ah;

	/* 全ての文法コードを設定する。*/
	while(1){
		al=(*bp++);						/* 外部辞書の情報を１バイト取り出す。*/
		if(sign(al)==0){ break; }	/* 文法コード以外ならやめる。*/
		al &=MaskEdicGrcd;				/* 文法コード部分を取り出す。*/
		StzfGrcdStore(al);				/* 文法コードを追加する。*/
	}

	/* 文法コード→後端番号・前方ベクトル番号・制御情報 */
	StzfGrcdOR();
}


/*========== EDIC5000(di) ===============================================
	機能：現在要素の発音表記を出力データエリアのｄｉ文字目から格納する。
  ======================================================GjsfKekMotomeru==*/
integer_16 EDIC5000(di) integer_16 di; {
	register BYTE *si;
	register integer_16 cx;
	integer_16 a_len,p_len;
	BYTE ah,al,*ti;
	LPBYTE bp;

	/* 現在注目位置(フラグ２の位置)を取り出し、フラグ２をとばす。*/
	bp=edic_ofs+edic_page_addr; bp++;
	/* 学習情報とあれば文法コードをとばす。(文法コードはMSBがONになっている) */
	do{ bp++; }while(sign(*bp));

	p_len=(*bp++);
	a_len=p_len>>4;				/* length of accent information */
	p_len &= 0x0F;				/* length of pronunciation information */

	for(cx=a_len;cx--;){
		al=(*bp++);
		ah=al/16; al=al%16;
		if(ah<10){ stc_accent_raw[stc_ptr][ah]=al; } /* accent raw data */
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
		for(cx=cpron_len=p_len;cx--;){
			al=(*bp++); di=OMEGA1000(al,di);
		}
	}
	/*
	 *	長音接続可能で次の文字が長音以外なら長音を付加する。
	 *	但し、校正モードではなにもしない。
	 */
	if(onsei_addr->onsei_kouseisystem){ goto EDIC5000_990; }
	for(cx=Stz_CE.stz_nbno;cx--;){
		if(Stz_CE.stz_bno[cx]==60){ goto EDIC5000_700; }
	}
	goto EDIC5000_990;
EDIC5000_700:
	if((cx=ceptr+celen)==n_inert){ goto EDIC5000_770; }
	if(inert[cx*2]!=0x21||inert[cx*2+1]!=0x3C){
		EDIC5000_770:
		di=OMEGA1000(0xC0,di); cpron_len++;
	}
EDIC5000_990:
	return di;
}


void ACC0010(void) 
{
	register BYTE *di;
	if(chk_limits(Fdd_Header.Fdd_GrcdNum1,Fdd_Header.Fdd_GrcdNum2,
		Stz_CE.stz_grcd,Stz_CE.stz_ngrcd)==0)
	{
		/* 助数詞以外ならば以下の処理を行う。*/
		di= &stc_accent_raw[stc_ptr][0];
		if(*(di+4)==0x00 && *(di+1))
		{
			if(*di)
			{
				*(di+4)=(*di);	/* (*di) is not 平板型 */
			}
			else
			{
				*(di+4)=1;		/* (*di) is 平板型 */
			}
		}
		if(*di==0x0F)
		{
			*di= 0;
			*(di+1)= *(di+2)= *(di+4)= 1;
		}
		if(*(di+4)==0x0F)
		{
			*(di+4)=0;
		}
	}
}

/***************************** END OF ILE_EDIC.C *******++++******************/
