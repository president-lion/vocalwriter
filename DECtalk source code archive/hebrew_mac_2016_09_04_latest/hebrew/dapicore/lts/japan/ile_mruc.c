#if !defined(O_TALKONLY)
/************************************************
	ILE_MRUC.C	updated at 18th December 1990
	学習関連
 ************************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_edic.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_idic.h"
#include	"ile_jknh.h"
#include	"ile_kkhh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"
#include "ile_rnsc.h"

/*========== MRU_CSENT() =================================================
	機能：現在注目文節までの変換結果を学習する。
			（現在注目文節が変換不能なら、その文節の手前までを学習する。）
  ===================================================GksfChbGakushuusuru==*/
void MRU_CSENT() {
	/* 変換不能状態でなければ、注目文節になるまでスタックを戻す。*/
	/***if(kkhstat!=funou)then STACK_POP_FS(); fi***/
	/* スタック上の要素を学習する。*/
	MRU_ALL();
}


/*========== MRU_ALL() =================================================
	機能：現在の変換結果を学習する。
			（変換不能文節がある場合は、その文節の手前までを学習する。）
	注意：放棄処理と入れ違い処理の順序に注意する。
  ======================================================================*/
void MRU_ALL() {
	register integer_16 p;
	/*
	 * アクセント結合の学習を行う。
	 * この学習は注目文節位置とは何の関係も無いので注意する。
	 */
	for(p=stc_ptr;p--;){
		if(stc_mru[p] & (BITMRU_ACON|BITMRU_ACOFF)){
			ceptr=stc_in_ptr[p];
			celen=stc_in_len[p];
			switch(stc_kind[p]){
			case DICKIND_I:
				IDIC_ACC_STUDY(p);
				break;
			case DICKIND_E:
				EDIC_ACC_STUDY(p);
			}
		}
	}

	/* 次候補・長方向・短方向を実行していないときは学習の必要なし。*/
	if(MRU_OR_NOT==0){ return; }

	/***if(kkhstat==funou){***/	/* 変換不能状態ならば */
		/***STACK_POP_WS();***/		/* 現在注目文節になるまでスタックを戻す*/
		/* watching sentenceが先頭文節ならばこの時点でstc_ptr=0である。*/
		/*if(ws_no==1){ return; }*/
	/***}***/
	kkhstat=mru;					/* 学習状態とする */

	for(ceptr=n_inert;ceptr--;){
		for(celen=n_inert-ceptr;celen;--celen){
			/* 辞書を採用したことがあれば辞書の使用を放棄する。*/
			if(celen<=MAX_IDIC && JknfAttrAri(BITDUT_ID_USE)){
				IDIC_ABORT();
			}
			if(celen<=MAX_EDIC && JknfAttrAri(BITDUT_ED_USE)){
				EDIC_ABORT();
			}
		}
	}

	for(p=stc_ptr;p--;){			/* それぞれの要素について */
		ceptr=stc_in_ptr[p]; celen=stc_in_len[p];
		/* 外部辞書または内部辞書ならば、検索表のフラグを消して学習する。*/
		switch(stc_kind[p]){
		case DICKIND_I:
			JknfAttrToru(BITDUT_ID_USE);
			IDIC_MRU(p);
			break;
		case DICKIND_E:
			JknfAttrToru(BITDUT_ED_USE);
			EDIC_MRU(p);
		}
	}
}


/*========== MRU_REMEMBER() ========================
	機能：現在の変換結果で使用している語を記録する。
  ==================================================*/
void MRU_REMEMBER() {
	register integer_16 p;
	register integer_16 len;
	integer_16 ptr;

	for(p=stc_ptr;p--;){
		ptr=stc_in_ptr[p];
		len=stc_in_len[p];
		switch(stc_kind[p]){
		case DICKIND_I:
			JknfAttrAdd3(BITDUT_ID_USE,ptr,len);
			break;
		case DICKIND_E:
			JknfAttrAdd3(BITDUT_ED_USE,ptr,len);
		}
	}
}


/*========== IDIC_MRU(p) ==================
	機能：注目要素の接続情報を学習する。
	入力：ｐ＝要素番号
  ====================NjsfStzGakushuusuru==*/
void IDIC_MRU(p) integer_16 p; {
	register BYTE *si,*di;
	BYTE tmp;
	integer_16 len;

	/* 内部辞書オフセットを取り出す。*/
	di=FddHDR+stc_ed_ofs[p];
	/* 放棄フラグを消す。*/
	((IDIC_DATA *)di)->idic_mru &= ~BITMRU_ABORT;
	/*
	 * 注目要素と同じ表記の最初の要素を探す。
	 * 要素が見つからないならば辞書が壊れている。
	 */
	if(IDIC2000()==0){ jp_ERROR(Err_Fatal_Idic); }
	/* 同表記の最初の要素の位置を取り出す。*/
	si=idic_addr;
	/* 同表記の最初の要素と注目要素が異なるときは、固定情報部分を入れ替える。*/
	if(di!=si){
		for(len=SIZE_IDIC; len--; ++si,++di){
			tmp=(*si); *si=(*di); *di=tmp;
		}
	}
}


/*========== IDIC_ABORT() ==============================================
	機能：注目要素の表記と同じ表記を持つ、内部辞書要素の使用を放棄する。
  ====================================================NjsfYomHoukisuru==*/
void IDIC_ABORT() {
	/* 内部辞書を引く。見つからなければ辞書が壊れている。*/
	if(IDIC2000()==0){ jp_ERROR(Err_Fatal_Idic); }
	do{
		((IDIC_DATA *)idic_addr)->idic_mru |= BITMRU_ABORT;
	}while(IDIC3000());
}


/*========== IDIC_ACC_STUDY(p) ===================
	機能：内部辞書語のアクセント結合の有無の学習
	入力：ｐ＝要素番号
  ================================================*/
void IDIC_ACC_STUDY(p) integer_16 p; {
	idic_addr=FddHDR+stc_ed_ofs[p];
	if(stc_mru[p] & BITMRU_ACOFF){
		((IDIC_DATA *)idic_addr)->idic_mru |= BITMRU_ACOFF;
		((IDIC_DATA *)idic_addr)->idic_mru &= ~BITMRU_ACON;
	}else{
		((IDIC_DATA *)idic_addr)->idic_mru |= BITMRU_ACON;
		((IDIC_DATA *)idic_addr)->idic_mru &= ~BITMRU_ACOFF;
	}
}


/*========== EDIC_MRU_INIT() ===========
	機能：学習状態を初期化する。
  ======================================*/
void EDIC_MRU_INIT() {
	EdicMruDone_LBN[0]= EdicMruDone_OFS[0]= 0;
}


/*========== EDIC_MRU_NEED(Page,bp,f2) =====================
	入力：Page	現在注目語のある外部辞書ページ番号
 		　bp	現在注目語のアドレス
 		　f2	同音語の先頭の語のアドレス
	出力：0		学習の必要なし。
 		  1		学習の必要あり。
	機能：現在語を学習すべきかどうかを調べる。
  ==========================================================*/
BOOL EDIC_MRU_NEED(Page,bp,f2) WORD Page; LPBYTE bp,f2; {
	register integer_16 si;
	register WORD ax;
	WORD bx;

	si=0; ax=Page; bx=(WORD)(f2-edic_page_addr);
	/* 今まで学習した語の情報とくらべて学習するか決める。*/
	while(1){
		/* すべて調べたら終わり。*/
		if((EdicMruDone_LBN[si]==0)&&(EdicMruDone_OFS[si]==0)){ break; }
		/* ページ番号とページ内オフセットが一致したら、学習しない。*/
		if((EdicMruDone_LBN[si]==ax)&&(EdicMruDone_OFS[si]==bx)){ return 0; }
		si++;
	}
	/* 学習する語の情報を格納する。*/
	EdicMruDone_LBN[si]=ax; EdicMruDone_OFS[si++]=bx;
	/* テーブルのストッパーを入れる。*/
	EdicMruDone_LBN[si]=0; EdicMruDone_OFS[si]=0;
	return 1;
}


/*========== EDIC_MRU(p) =====================================================
	機能：現在注目語の接続情報を学習する。現在注目語の情報を、同音語の先頭に
 		　移し、外部辞書に書きもどす。
	入力：ｐ＝要素番号
  ============================================================================*/
void EDIC_MRU(p) register integer_16 p; {
	LPBYTE bp,f1,f2;
	integer_16 mru=0;

	/* 外部辞書を読み込み、edic_page_addrにそのアドレスを得る。*/
	RnsfReadEdic(stc_ed_pag[p],&edic_page_addr);
	/* フラグ２のアドレスを計算する。*/
	bp=stc_ed_ofs[p]+edic_page_addr;

	/* 同字語グループのフラグ１と先頭語のフラグ２を探す。*/
	EdicfSentouSagasu(bp,&f1,&f2);

	/*
	 *	すでに学習済の同字語グループならば、学習しない。
	 *	もし、へたに学習すると、順序を入れ替えた後で、
	 *	入れ替える前の edic_ofs を用いて放棄フラグをクリアしてしまう。
	 *	当然、入れ替わった後の辞書を壊すことになるので、
	 *　ページ番号と先頭語のフラグ２の位置が一致したら学習しないことにする。
	 *	上記の現象は候補が１つだけのときは問題ない。
	 */
	if(EDIC_MRU_NEED(stc_ed_pag[p],bp,f2)==0){ return; }

	if(*(bp+1) & BITMRU_ABORT){		/* 放棄フラグが付いている時、*/
		*(bp+1) &= ~BITMRU_ABORT;		/* 放棄フラグを消す。*/
		mru=1;
	}

	/* 現在注目語が同音語の先頭でなければ、順序を入れ替える必要がある。*/
	if(f2!=bp){
		EdicfToridasu(bp);		/* 注目している語を取り出し、長さを求める。*/
		EdicfShift(bp-1,bp+ed_ndata-1,bp-f2); /* 取り出したあとをつめる。*/
		EdicfIreru(f2);			/* 注目している語を同字語の先頭に入れる。*/
		mru=1;
	}

	if(mru){
		RnsfWriteEdic(stc_ed_pag[p]);		/* 学習結果を書き出す。*/
	}
}


/*========== EDIC_ABORT() =========================================
	機能：現在語の読みと同じ読みを持つ、外部辞書の使用を放棄する。
  =================================================================*/
void EDIC_ABORT() {
	LPBYTE bp;
	/* 外部辞書を引いて、読みが見つからなければ辞書が壊れている。*/
	if(EDIC2000()==0){ jp_ERROR(Err_Fatal_Edic); }
	bp=edic_ofs+edic_page_addr;
	if((*(bp+1) & BITMRU_ABORT)==0){		/* 放棄フラグが付いていない時、*/
		*(bp+1)|=BITMRU_ABORT;				/* 放棄フラグを付ける。*/
		RnsfWriteEdic(edic_page_no);		/* 学習結果を書き出す。*/
	}
}


/*========== EDIC_ACC_STUDY(p) ===================
	機能：外部辞書語のアクセント結合の有無の学習
	入力：ｐ＝要素番号
  ================================================*/
void EDIC_ACC_STUDY(p) integer_16 p; {
	LPBYTE bp;
	/* 外部辞書を読み込み、edic_page_addrにそのページ先頭アドレスを得る。*/
	RnsfReadEdic(stc_ed_pag[p],&edic_page_addr);
	bp=stc_ed_ofs[p]+edic_page_addr;
	if(stc_mru[p] & BITMRU_ACOFF){
		*(bp+1)|= BITMRU_ACOFF;
		*(bp+1)&= ~BITMRU_ACON;
	}else{
		*(bp+1)|= BITMRU_ACON;
		*(bp+1)&= ~BITMRU_ACOFF;
	}
	RnsfWriteEdic(stc_ed_pag[p]);		/* 学習結果を書き出す。*/
}


/*========== EdicfToridasu(bp) ======================================
	入力：bp			取り出す語のアドレス
	出力：ed_data		取り出した語
 		　ed_ndata		取り出した語の長さ
	機能：注目している語を取り出し（フラグ２から発音表記まで）、
 		　長さを求める
  ==================================================================*/
void EdicfToridasu(bp) LPBYTE bp; {
	register integer_16 a_len,p_len;
	integer_16 bx;
	BYTE al;

	bx=0;						/* レコード長のリセット */
	al=ed_data[bx++]=(*bp++);	/* フラグ２を取り出す */

	/* 学習の虫を捜す。*/
	if(sign(al)==0){ jp_ERROR(Err_Fatal_Edic); }

	/* 学習情報と文法コードを取り出す */
	do{ ed_data[bx++]=(*bp++); }while(sign(*bp));

	ed_data[bx++]=p_len=(*bp++);
	a_len=p_len>>4;				/* length of accent information */
	p_len &= 0x0F;				/* length of pronunciation information */

	/* accent information */
	for(;a_len--;){ ed_data[bx++]=(*bp++); }

	/* pronunciation information */
	for(;p_len--;){ ed_data[bx++]=(*bp++); }

	ed_ndata=bx;	/* 取り出した語の長さ */
}


/*========== EdicfIreru(bp) ========================
	入力：bp			入れるアドレス
 		　ed_data[]		いれる語
 		　ed_ndata		いれる語の長さ
	機能：注目している語を指定された場所に入れる。
  =================================================*/
void EdicfIreru(bp) LPBYTE bp; {
	register BYTE *si;
	register integer_16 cx;
	for(cx=ed_ndata,si=ed_data;cx--;){
		*bp++=(*si++);
	}
}


/*========== EdicfShift(si,di,cx) =============
	機能：学習の入れ替え作業時のデータ移動
  ============================================*/
void EdicfShift(si,di,cx) LPBYTE si,di; register integer_16 cx; {
	for(;cx--;){
		*di--=(*si--);
	}
}


/*========== EdicfAkeru(cx,bp) =============================================
	入力：bp			あけるアドレス
 		  cx			あけるバイト数
	機能：指定された場所から指定されたバイト数だけ外部辞書にあきをつくる。
  =========================================================================*/
void EdicfAkeru(cx,bp) register integer_16 cx; LPBYTE bp; {
	register integer_16 ax;
	LPBYTE si,di;

	ax=Fdd_BlkSize*Fdd_Header.Fdd_EdicPageSize;
	si=edic_page_addr+(ax-cx); di=edic_page_addr+ax;
	cx=ax-((integer_16)(bp-edic_page_addr)+cx);
	for(;cx;--cx){
		*(--di)=(*(--si));
	}
}


/*========== EdicfTsumeru(cx,bp) ===========================================
	入力：bp			つめるアドレス
 		  cx			つめるバイト数
	機能：指定された場所から指定されたバイト数だけ外部辞書の内容をつめる。
  =========================================================================*/
void EdicfTsumeru(cx,bp) register integer_16 cx; LPBYTE bp; {
	LPBYTE si;

	si=bp+cx;
	cx=Fdd_BlkSize*Fdd_Header.Fdd_EdicPageSize-(WORD)(bp-edic_page_addr)-cx;
	for(;cx;--cx){
		*bp++=(*si++);
	}
}


/*========== EdicfSentouSagasu(bp,&f1,&f2) ===========================================
	入力：es:[di+bp]	現在注目語のフラグ２のアドレス
	出力：f1			最初の同字語のフラグ１のアドレス
		　f2			最初の同字語のフラグ２のアドレス
	機能：現在注目語を含む同字語グループの先頭の語のアドレスを求める。
  ==============================================================================*/
void EdicfSentouSagasu(bp,f1,f2) LPBYTE bp,*f1,*f2; {
	LPBYTE save,bx;

	bx=edic_page_addr;				/* ページの先頭から調べて行く。*/
	while(1){
		if(bx>=bp){ break; }	/* 注目語の後のフラグ１なら終わり。*/
		save=bx;					/* フラグ１のページ内オフセットを覚えておく。*/
		bx+=(*(bx+1)+2);
	}
	*f1=save;				/* 覚えておいたフラグ１のページ内オフセット */
	bx=save+2;				/* 見出し語のページ内オフセット */
	while(sign(*bx)==0){ ++bx; }	/* 見出し語をとばす。*/
	*f2=bx;
}

/***************************** END OF ILE_MRUC.C *****************************/
#endif
