/************************************************
	ILE_IDIC.C	updated at 5th September 1989
	内部辞書
 ************************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_fddh.h"
#include	"ile_idic.h"
#include	"ile_jknh.h"
#include	"ile_kkhh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"


/*========== IDIC1000() ===========================================
	機能：内部辞書に現在注目語がある可能性があるかどうかを調べる。
  =================================================================*/
BOOL IDIC1000() {
	/*
	 *	注目要素の状況を調べ、判定する。
	 *	内部辞書引き可能最大文字数を超えている場合、可能性なし。
	 *	辞書検索表に読みがないことが記録されている場合、可能性なし。
	 *	上記以外の場合、可能性あり。
	 */
	if(celen>MAX_IDIC){ return 0; }
	if(JknfAttrAri(BITDUT_ID_NONE)){ return 0; }
	return 1;

	/*
	 *	初回放棄語検索指示で、初回放棄語でなければ無し。
	 *	if(StzfAttrAri(Stz_CE,stz_mru,BITMRU_ABORT)
	 *	&&(JknfAttrAri(BITDUT_ID_ABORT)==0)){ return 0; }
	 */
}


/*========== IDIC2000() ======================================
	機能：現在要素開始位置から始まる、現在要素長さの語がある、
 		内部辞書のメモリー上のアドレスを求める。
 		エラーがあった場合は、エラーコードを設定して戻る。
  ============================================================*/
BOOL IDIC2000() {
	register BYTE *si,*di;
	IDIC_INDICES *mjsptr;
	BYTE *endptr,*source,*njsptr;
	integer_16 len,cebyte;
	WORD offset;

	cebyte=celen*2;

	/* 辞書検索表にあれば採用する。*/
	if(JknfAttrAri(BITDUT_ID_EXIST)){
		njsptr=JknfIdicToridasu;
		goto IDIC2000_200;
	}

	source=(&inert[ceptr*2]);

	mjsptr = (IDIC_INDICES *)(&FddHDR[Fdd_Header.Fdd_IdicIndOfs]
			+ sizeof(IDIC_INDICES)*(celen-1));
	/* offset = mjsptr->idic_len_ptr; */
	offset = (mjsptr->idic_len_ptr1 <<8) + mjsptr->idic_len_ptr2;
	njsptr = FddHDR + Fdd_Header.Fdd_IdicOfs + offset;
	/* offset = (mjsptr+1)->idic_len_ptr; */
	offset = ((mjsptr+1)->idic_len_ptr1 <<8) + (mjsptr+1)->idic_len_ptr2;
	endptr = FddHDR + Fdd_Header.Fdd_IdicOfs + offset;

	for(;;){
		if(njsptr>=endptr){ JknfAttrAdd(BITDUT_ID_NONE); return 0; }
	
		/* 表記を比較する。*/
		si=source; di=njsptr;
		for(len=cebyte; len; len--,si++,di++){
			if(*si!=(*di)){ break; }
		}

		/* すべて一致すれば良い。*/		
		if(len==0){ break; }
			
		/* 内部辞書が大きければ 内部辞書に無かったことを記録する。*/
		if(*si<*di){ JknfAttrAdd(BITDUT_ID_NONE); return 0; }

		/* 内部辞書現在要素表記がまだ小さいならば次の表記まで進む。*/
		njsptr += cebyte+SIZE_IDIC;
	}

	/* 現在注目要素の制御情報のオフセットを保存する。*/
	njsptr+=cebyte;
	JknfAttrAdd(BITDUT_ID_EXIST);	/* 内部辞書にあることを記録。*/
	JknfIdicShimau(njsptr);			/* 辞書検索表にoffsetをしまう。*/
	
IDIC2000_200:
	idic_addr=njsptr;
	/* 学習状態ならば現在語を採用する。*/
	if(kkhstat!=mru){
		if(((IDIC_DATA *)njsptr)->idic_mru & BITMRU_ABORT){
			/* 見つかった語が放棄語で、
			 * 放棄語不採択指示ならば次の候補を捜す。
			 */
			JknfAttrAdd(BITDUT_ID_ABORT);
			if(StzfAttrNashi(Stz_CE,stz_mru,BITMRU_ABORT)){
				return IDIC3000();
			}
		}else{
			/* 見つかった語が放棄語ではなくて、
			 * 放棄語採択指示ならば次の候補を捜す。
			 */
			if(StzfAttrAri(Stz_CE,stz_mru,BITMRU_ABORT)){
				return IDIC3000();
			}
		}
	}

	/* 注目要素の接続情報・発音情報等を設定する。*/
	IDIC4000();
	return 1;
}


/*========== IDIC3000() ===============================
	機能：内部辞書の語の次の同音意義語の位置を求める。
  =====================================================*/
BOOL IDIC3000() {
	integer_16 len;
	IDIC_INDICES *mjsptr;
	BYTE *endptr,*source;
	register BYTE *si,*njsptr;
	WORD offset;

	source = ceptr*2+inert;
	njsptr = idic_addr+SIZE_IDIC;

	mjsptr = (IDIC_INDICES *)(&FddHDR[Fdd_Header.Fdd_IdicIndOfs]
			+ sizeof(IDIC_INDICES)*celen);
	/* offset = mjsptr->idic_len_ptr; */
	offset = (mjsptr->idic_len_ptr1 <<8) + mjsptr->idic_len_ptr2;
	endptr = FddHDR + Fdd_Header.Fdd_IdicOfs + offset;

	for(;;){
		if(njsptr>=endptr){ return 0; }

		/* 表記を比較する。*/
		si=source;
		for(len=celen*2; len--;){
			if(*si++!=(*njsptr++)){ return 0; }
		}
	
		/* 学習状態ならば現在語を採用する。*/
		if(kkhstat==mru){ break; }
		/*
		 *	前回放棄語の時は辞書検索表に記録し
		 *	放棄語の採用が指示されていれば採用する。
		 */
		if(((IDIC_DATA *)njsptr)->idic_mru & BITMRU_ABORT){
			JknfAttrAdd(BITDUT_ID_ABORT);
			if(StzfAttrAri(Stz_CE,stz_mru,BITMRU_ABORT)){ break; }
		}else{
			if(StzfAttrNashi(Stz_CE,stz_mru,BITMRU_ABORT)){ break; }
		}
		njsptr += SIZE_IDIC;
	}

	idic_addr=njsptr;
	IDIC4000();
	return 1;
}


/*========== IDIC4000()===============
	機能：現在語の接続情報を設定する。
  ====================================*/
void IDIC4000() {
	register IDIC_DATA *njsptr;

	StzfElmKind(&Stz_CE,DICKIND_I);			/* 単語種別を内部辞書とする。*/
	njsptr=(IDIC_DATA *)idic_addr;			/* 内部辞書の注目位置を取り出す。*/
	Stz_CE.stz_f2=njsptr->idic_f2;			/* フラグ２を設定する。*/
	Stz_CE.stz_ctl=njsptr->idic_ctl;		/* 制御情報を設定する。*/
	Stz_CE.stz_ngrcd=1;						/* 文法コード数を１とする。*/
	StzfBnoAdd(&Stz_CE,njsptr->idic_bno);	/* 後端番号を設定する。*/
	Stz_CE.stz_pno[0]=njsptr->idic_pno;		/* 前方ベクトル番号を設定する。*/
	/* 学習情報を設定する（前回放棄語フラグを持ち越す）*/
	Stz_CE.stz_mru=(njsptr->idic_mru & ~BITMRU_ABORT)
				  |(Stz_CE.stz_mru & BITMRU_ABORT);

	/*
	 * 現在語が先行語漢語接続ならば、接続状態指示に漢語を加える。
	 * ここで設定した漢語フラグは先行語との漢語接続の有無を調べるために用いる。
	 * 先行語との間で接続関係が確認された時点で、フラグの意味は後続語との
	 * 漢語接続可能を示す意味に代える。
	 */
	if(njsptr->idic_ctl & BITCTL_PKANGO){ Stz_CE.stz_stz |= BITSTZ_KANGO; }
}


/*========== IDIC5000(di) =================================================
	機能：現在注目語の発音表記を出力データエリアのｄｉ文字目から格納する。
  =========================================================================*/
integer_16 IDIC5000(di) integer_16 di; {
	register BYTE *si;
	register integer_16 cx;
	IDIC_DATA *njsptr;
	integer_16 a_len,p_len;
	BYTE *accptr,al,ah,*ti;
	WORD offset;

	/* 内部辞書現在注目位置を取り出す。*/
	njsptr=(IDIC_DATA *)idic_addr;
	/* 内部辞書中のaccent & pronunciation位置を取り出す。*/
	/* offset = njsptr->idic_acc_ofs; */
	offset = (njsptr->idic_acc_ofs1 <<8) + njsptr->idic_acc_ofs2;
	accptr = (&FddHDR[Fdd_Header.Fdd_IdicPronOfs]) + offset;

	p_len=(*accptr++);
	a_len=p_len>>4;				/* length of accent information */
	p_len &= 0x0F;				/* length of pronunciation information */

	for(cx=a_len;cx--;){
		al=(*accptr++);
		ah=al/16; al=al%16;
		if(ah<10){ stc_accent_raw[stc_ptr][ah]=al; }
	}

	ACC0010();

	/* 発音情報を取り出す。*/
	if(p_len==0){
		cpron_len=0; si= &inert[ceptr*2]; ti=inert_kana;
		for(cx=celen*2;cx--;){
			*ti++=(*si++);
		}
		*ti=0x00;
		cx=mkhatu(inert_kana,inert_pron);
		for(si=inert_pron;cx--;++si){
			di=OMEGA1000(*si,di); ++cpron_len;
		}
	}else{
		for(cx=cpron_len=p_len;cx--;){
			al=(*accptr++); di=OMEGA1000(al,di);
		}
	}
	return di;
}

/***************************** END OF ILE_IDIC.C *****************************/
