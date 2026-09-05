/*************************************************
	ILE_RTGC.C	updated at 12th September 1991
	辞書登録削除
 *************************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_edic.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_kkhh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"
#include "ile_rnsc.h"

/*========== EDIC_ADD() ====================
	機能：利用者定義語を外部辞書に登録する。
  =============================RtgfTouroku==*/
void EDIC_ADD() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	register integer_16 cptr,nptr;
	integer_16 cx;
	BYTE al,ah,l;
	LPBYTE f1,f2,bp;

	kkhstat=shoki;				/* 初期状態を設定する */
	INERT_INIT();				/* 入力データを入力作業エリアに移す */
	PRON_INIT();				/* 変換処理関連エリアを初期化する */
	ceptr=0; celen=n_inert;
	if((RtgfNydShiraberu()==0) || (EDIC1000()==0)){
		/* 登録語として、表記・読みのどちらかが適当でない */
		ERROR5000(); jp_ERROR(Err_EdicTooLong);
	}
	ed_ndata=0;					/* 追加語の長さを初期化する */

	/* 外部辞書を引き対応する読みがなければフラグ１から追加する。(case 3) */
	if(EDIC2000()==0){ goto EDIC_CASE_3; }

	/* 外部辞書を引き対応する読みがあれば */
	do{
		bp=edic_ofs+edic_page_addr;
		/* 外部辞書の表記と登録語の表記が同じで、かつ、登録語の文法コードが
		　利用者定義語の文法コードならば、削除フラグを復活する。(case 1) */
		if(RtgfKnjHykOnaji(bp) && RtgfGrcdAri(Fdd_Header.Fdd_GrcdUsr,bp)){
			goto EDIC_CASE_1;
		}
	}while(EDIC3000());

	/* 次候補がなければ、フラグ２から追加する。(case 2) */
	/* case 2 */
	bp=edic_ofs+edic_page_addr;
	/* 同字語グループのフラグ１と先頭語のフラグ２をさがす */
	EdicfSentouSagasu(bp,&f1,&f2);
	edic_ofs=(WORD)(f2-edic_page_addr);
	/* 追加語のフラグ２以降を作る */
	RtgfKchTsukuru(0);
	/* できた追加語がページに収まるか調べる */
	if(RtgfTsuikaKanou()==0){ jp_ERROR(Err_EdicOVF); }
	/*
	 * 次のフラグ１迄のバイト数を更新する。
	 * ここではページの中身を書き換えてしまうので、
	 * その前にOVERFLOWしないことを確認すること！
	 */
	al=(*(f1+1)); al+=ed_ndata; *(f1+1)=al;
	goto EDIC_INSERT;	/* フラグ２から追加する */

EDIC_CASE_1:
	/* case 1 */
	if((al=(*(bp+1))) & BITMRU_DEL){
		/* 削除フラグが立っているならば、それをおとして書き出す。*/
		*(bp+1) = (al & ~BITMRU_DEL);
		goto EDIC_WRITE;
	}else{
		/* 削除フラグが立っていなければ既に登録済みなので、エラーとする。*/
		jp_ERROR(Err_TourokuSumi);
	}

EDIC_CASE_3:
	/* case 3 */
	RtgfFlag1Tsukuru();		/* フラグ１から追加する */
	/* できた追加語がページに収まるか調べる */
	if(RtgfTsuikaKanou()==0){ jp_ERROR(Err_EdicOVF); }
	bp=edic_ofs+edic_page_addr;
	/* 登録語と次の語の見出しが部分的に一致する可能性があるならば */
	if((*bp & 0x1F)==ed_nsame){
		l=0; cptr=2; nptr=2;
		/* 一致する長さを求める */
		while(1){
			al=(*(bp+nptr));
			ah=ed_data[cptr];
			if(sign(al) || sign(ah)){ goto EDIC_CASE_3_100; }
			if(al!=ah){ goto EDIC_CASE_3_100; }
			++l; ++nptr; ++cptr;
		}
		EDIC_CASE_3_100:;
		if(l==0){ goto EDIC_INSERT; }
		/*
		 * 次の語のフラグ１と同字語グループの長さを書き換える。
		 * ここではページの中身を書き換えてしまうので、
		 * その前にOVERFLOWしないことを確認すること！
		 */
		al=(*bp); al=((al & 0x1F)+l)|0xE0; *bp=al;
		al=(*(bp+1)); *(bp+1)=al-l;
		EdicfTsumeru(l,bp+2);
	}

EDIC_INSERT:
	bp=edic_ofs+edic_page_addr;
	cx=ed_ndata;
	EdicfAkeru(cx,bp);			/* 追加語を入れるための空きをつくる。*/
	EdicfIreru(bp);				/* 追加語を入れる。*/

EDIC_WRITE:
	RnsfWriteEdic(edic_page_no);	/* 変更した外部辞書を書き出す */
	while(RnsfFlushBuffer());	/* 学習結果をすべて外部辞書に書き出す */
#endif
}


#if !defined(O_TALKONLY)
/*========== RtgfFlag1Tsukuru() ==================
	出力：ed_data		追加する語を格納する場所
 		  ed_ndata		追加する語の長さ
	機能：フラグ１以降をつくる。
  ================================================*/
void RtgfFlag1Tsukuru() {
	register BYTE *si;
	register integer_16 cx; 
	integer_16 di;
	BYTE al;

	/* フラグ１をつくる（直前語と読みの一致する長さは０としておく） */
	al=ed_nsame;
	ed_data[0]=0xE0 | al;
	di=2;
	/* 表記をコピーする */
	cx=n_inert*2-ed_nsame;
	for(si=(&inert[ed_nsame]);cx--;){ ed_data[di++]=(*si++); }
	ed_ndata=di;
	/* フラグ２以降をつくる */
	RtgfKchTsukuru(di);
	/* 次のフラグ１までの距離をセットする。*/
	ed_data[1]=ed_ndata-2;
}
#endif


#if !defined(O_TALKONLY)
/*========== RtgfKchTsukuru(di) ====================
	入力：di			フラグ２以降を格納する位置
	出力：ed_data		追加する語を格納する場所
 		　ed_ndata		追加する語の長さ
	機能：フラグ２以降をつくる。
  ==================================================*/
void RtgfKchTsukuru(di) integer_16 di; {
	register BYTE *dptr;
	register integer_16 cx;
	LPBYTE si;

	dptr = &ed_data[di];
	*dptr++ =0x80 | BITFLG2_KANGO;			/* フラグ２をつくる */
#if 1
	*dptr++ =0;								/* 学習情報をつくる */
#else
	*dptr++ =0 | BITMRU_ACOFF;				/* 学習情報をつくる（人名）*/
#endif
	*dptr++ =Fdd_Header.Fdd_GrcdUsr | 0x80;	/* 文法コードをつくる */
	ed_ndata+=3;
	/* 漢字表記をつくる */
	si=onsei_addr->onsei_omega;	/*ShdaChmHaj;*/	/* 出力データエリアのアドレスを取り出す */

	/*
	 *	byte position
	 *   0		 1	   2...			  L1+2...
	 *	+-------+-----+--------------+--------------------+
	 *	|L1+L2+1|L1/L2| accent codes | pronunciation data |
	 *	+-------+-----+--------------+--------------------+
	 *	              |<-----L1----->|<--------L2-------->|
	 *
	 *	outbuf[0]				: number of total length (L1+L2+1)
	 *	outbuf[1] upper 4 bits	: number of accent code
	 *	outbuf[1] lower 4 bits	: mora number of pronunciation data
	 *	outbuf[2]..				: accent codes
	 *	outbuf[following]..		: pronunciation data
	 */
	ed_ndata += (cx=(*si++));
	for(;cx--;){ *dptr++=(*si++); }
}
#endif


/*========== EDIC_DELETE2() ==============================
	機能：利用者定義語（単独実行）を外部辞書から削除する。
  ===========================================RtgfSakujyo==*/
void EDIC_DELETE2() {
#if !defined(O_DEBUG)
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	register integer_16 cnd;
	BYTE al;
	LPBYTE bp;

	kkhstat=shoki;				/* 初期状態を設定する */
	INERT_INIT();				/* 入力データを入力作業エリアに移す */
	PRON_INIT();				/* 変換処理関連エリアを初期化する */
	ceptr=0; celen=n_inert;
	if((RtgfNydShiraberu()==0) || (EDIC1000()==0)){
		/* 登録語として、表記・読みのどちらかが適当でない */
		ERROR5000(); jp_ERROR(Err_EdicTooLong);
	}
	cnd=EDIC2000();				/* 外部辞書を引く */
	do{						/* 文法コードが一致するものが見つかるまで */
		while(1){			/* 読みと表記が一致するものが見つかるまで */
			bp=edic_ofs+edic_page_addr;
			/* 外部辞書に対応する読みがなければ エラーコードを設定して終了 */
			if(cnd==0){ jp_ERROR(Err_MiTouroku); }
			/* 外部辞書の表記と出力データが同じならばよい */
			if(RtgfKnjHykOnaji(bp)){ break; }
			/* 次の同音意義語をさがす */
			cnd=EDIC3000();
		}
		/* 文法コードが利用者定義語と一致すればよい */
	}while(RtgfGrcdAri(Fdd_Header.Fdd_GrcdUsr,bp)==0);

	/* 学習情報の削除フラグが立っていればエラーとする */
	if((al=(*(bp+1))) & BITMRU_DEL){ jp_ERROR(Err_MiTouroku); }
	/* 削除フラグを立てる */
	*(bp+1) = (al | BITMRU_DEL);

	RnsfWriteEdic(edic_page_no);	/* 更新した外部辞書を書き出す */
	while(RnsfFlushBuffer());		/* 学習結果をすべて外部辞書に書き出す */
#endif
#endif
}


/*========== EDIC_DELETE() =============================
	機能：利用者定義語（変換語）を外部辞書から削除する。
  ======================================================*/
void EDIC_DELETE() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	LPBYTE bp;
	/*
	 *	変換したものが１要素からなり、文法コードが１つで、それがユーザー登録語の
	 *	文法コードのときに限り、それを削除する。
	 */
	if(stc_kind[0]==DICKIND_E && stc_ptr==1){
		if(stc_ngrcd[0]==1 && stc_grcd[0][0]==Fdd_Header.Fdd_GrcdUsr){
			bp=edic_ofs+edic_page_addr;		/* フラグ２のアドレスを求める。*/
			*(bp+1) |= BITMRU_DEL;			/* 削除フラグを立てる。*/
			RnsfWriteEdic(edic_page_no);	/* 更新した外部辞書を書き出す。*/
			while(RnsfFlushBuffer());		/* 学習結果をすべて外部辞書に書き出す。*/
			MRU_OR_NOT=0;					/* この結果は学習しないようにする。*/
			return;
		}
	}
	jp_ERROR(Err_MiTouroku);
#endif
}


#if !defined(O_TALKONLY)
/*========== RtgfKnjHykOnaji(bp) =================
	機能：外部辞書の表記と出力データが同じか調べる。
  ===================================================*/
BOOL RtgfKnjHykOnaji(bp) LPBYTE bp; {
	register integer_16 cx;
	LPBYTE si;

	bp++;						/* フラグ２をとばす */
	/* 学習情報と、あれば文法コードをとばす */
	do{ bp++; }while(sign(*bp));

	si=onsei_addr->onsei_omega;	/*ShdaChmHaj;*/ /* 出力データエリアのアドレスを取り出す */
	cx=(*si++);
	for(;cx--;){
		if(*si++!=(*bp++)){ return 0; }
	}
	return 1;
}
#endif


#if !defined(O_TALKONLY)
/*========== RtgfGrcdAri(grcd,bp) ====================
	入力：grcd	文法コード
	機能：現在注目語が指定した文法コードを持つか調べる。
  =======================================================*/
BOOL RtgfGrcdAri(grcd,bp) BYTE grcd; LPBYTE bp; {
	bp+=2;								/* フラグ２と学習情報をとばす */
	while(sign(*bp)){				/* 文法コードがある間 */
		/* 文法コ―ドが指定された文法コードと同じなら */
		if((*bp & 0x7F)==grcd){ return 1; }
		bp++;
	}
	return 0;
}
#endif


#if !defined(O_TALKONLY)
/*========== RtgfTsuikaKanou() ====================
	機能：辞書登録語がページにおさまるかしらべる。
	値　：＝０	辞書空き領域不足
		　＝１	辞書空き領域十分
  =================================================*/
BOOL RtgfTsuikaKanou() {
	register integer_16 cx;
	LPBYTE bp;

	bp=edic_page_addr+(Fdd_BlkSize*Fdd_Header.Fdd_EdicPageSize-1);
	cx=0;
	while(1){
		if(*bp==0xFF){ ++cx; }else{ goto RtgfTK_100; }
		if(cx>ed_ndata){ return 1; }
		--bp;
	}

RtgfTK_100:
	if(cx>ed_ndata){ return 1; }else{ return 0; }
}
#endif


#if !defined(O_TALKONLY)
/*========== RtgfNydShiraberu() ==========
	機能：登録語として読みが適当か調べる
  ========================================*/
BOOL RtgfNydShiraberu() {
	if(n_inert>MAX_EDIC){ return 0; }else{ return 1; }
}
#endif

/***************************** END OF ILE_RTGC.C *****************************/
