/************************************************
	ILE_STZC.C	updated at 19th February 1991
	要素間接続関係
 ************************************************/

#define		EXT		extern

#if defined(O_DEBUG)||defined(DEBUG_STZ)
#include	<stdio.h>
#endif

#include	"onsei.h"
#include	"ile_bunh.h"
#include	"ile_dbgh.h"
#include	"ile_fddh.h"
#include	"ile_kkhh.h"
#include	"ile_nydh.h"
#include	"ile_stzh.h"
#include	"ile_func.h"

/*========== StzfElmKind(s,kind) ===========================================
	機能：先行要素(s=Stz_PE)，現在要素(s=Stz_CE)の単語種別をkindとし、
		学習情報以外を初期化する。
  ==========================================================================*/
void StzfElmKind(s,kind) register STZ_STRUCT *s; BYTE kind; {
	BYTE work;

	work=s->stz_mru;		/* 学習情報を保存する */
	FOLLOW_INIT(s);			/* 現在要素の接続情報をクリアする */
	s->stz_kind=kind; 		/* 単語種別を設定する */
	s->stz_mru=work;		/* 学習情報を戻す */
}


/*========== FOLLOW_INIT(di) ==================================
	機能：先行要素／現在要素の接続関係関連情報を初期化する。
  =========================================StzfXxgShokikasuru==*/
void FOLLOW_INIT(di) register STZ_STRUCT *di; {
	register integer_16 i;

	/* 単語種別、制御情報の論理和、フラグ２、接続状態、学習情報、数字情報 */
	di->stz_kind=
	di->stz_ctl=
	di->stz_f2=
	di->stz_stz=
	di->stz_mru=
	di->stz_dman=
	di->stz_dexp='\0';
	/* 文法コード数・後端番号数 */
	di->stz_ngrcd=di->stz_nbno=0;
	/* 文法コード・前方ベクトル番号・後端番号 */
	for(i=MAX_GRCD;i--;){
		di->stz_grcd[i]=di->stz_pno[i]=di->stz_bno[i]='\0';
	}
}


/*========== StzfElmBackward() =========================
	機能：先行要素の接続関係関連情報を、現在要素に移す。
  =====================================StzfChmGoModosu==*/
void StzfElmBackward() {
	/* 先行要素の接続関連情報を現在要素のものとし、先行要素の接続関連情報を初期化する */
	memcpy(&Stz_CE,&Stz_PE,sizeof(STZ_STRUCT));
	FOLLOW_INIT(&Stz_PE);
}


/*========== StzfElmForward() ========================
	機能：現在要素の接続関係関連情報を先行要素に移す。
  =================================StzfChmGoSusumeru==*/
void StzfElmForward() {
	/* 現在要素の接続関連情報を先行要素のものとし、現在語の接続関連情報を初期化する */
	memcpy(&Stz_PE,&Stz_CE,sizeof(STZ_STRUCT));
	FOLLOW_INIT(&Stz_CE);
}


/*========== StzfGrcdStore(code) ==========
	機能：現在要素の文法コードを格納する。
  =========================================*/
void StzfGrcdStore(code) BYTE code; {
	if(Stz_CE.stz_ngrcd<MAX_GRCD){
		Stz_CE.stz_grcd[Stz_CE.stz_ngrcd]=code;
		Stz_CE.stz_ngrcd++;
	}
}


/*========== StzfGrcdOR(code) ==========================================
	機能：全ての文法コードから、後端番号・前方ベクトル番号・制御情報を
		取り出してしかるべき場所に格納する。
		　制御情報はＯＲをとって格納する。
  ======================================================================*/
void StzfGrcdOR() {
	register BYTE *si;
	register integer_16 cx;
	integer_16 cond;
	BYTE ctl,grcd;
/********************************************************************************
 *																				*
 *	　『現在要素が漢語接尾辞で、先行要素が漢語でないときは、接尾辞の前方接続の	*
 *	文法コードを無視する。接続チェックは、マトリックスでのみ行ない、漢語接続の	*
 *	チェックは無視する。』														*
 *																				*
 *	（例）「辞書の先生」は正しい。「辞書先生」は誤り。							*
 *		　「東京の先生」は正しい。「東京先生」は誤り。							*
 *																				*
 *	　接尾辞の前方接続の文法コードの上下限は通信領域の１４９／１５０バイト目に	*
 *	書かれている。																*
 *																				*
 *	（解説）																	*
 *	　漢語接尾辞には自立し得るものとし得ないものとがあり、先行要素が			*
 *	漢語のときは、どちらも意味的に接続し得る品詞だけに接続する。				*
 *	　しかしながら、先行要素が漢語以外の時は、自立し得る漢語接尾辞のみ接続する	*
 *	可能性が有り、可能性はマトリックスのみで検査される。「辞書先生」のような	*
 *	誤りを起こさないためには、漢語接続のチェックをはずさなければならない。		*
 *																				*
 *	（例）自立するもの　　……　先生、教授										*
 *		　自立しないもの　……　止り											*
 *																				*
 ********************************************************************************/
	/*
	 * 漢語接尾辞でかつ先行要素が漢語でない時は、ある範囲の文法コードを無視する。
	 * 文節先頭なら無視しない。
	 */
	cond=(Stz_CE.stz_f2 & BITFLG2_SUF) && pe_is_kango==0;
	if(ceptr==0){ cond=0; }
	/*
	 * 全ての文法コードから、後端番号・前方ベクトル番号・制御情報を取り出して、
	 * しかるべき場所に格納する。
	 * 制御情報はＯＲをとって格納する。
	 */
	for(cx=Stz_CE.stz_ngrcd;cx--;){
		/* 文法コードを取り出す。*/
		grcd=Stz_CE.stz_grcd[cx];
		if(cond){
			if(grcd>=Fdd_Header.Fdd_GrcdSufLL && grcd<=Fdd_Header.Fdd_GrcdSufUL){
				goto Next;
			}
		}

		/* 文法コード表を引く */
		si=FddHDR+Fdd_Header.Fdd_GrcdTblOfs;

		/* 制御情報を移す */
		ctl=((char(*)[4])si)[grcd-1][GRCDTBL_CTL];
		Stz_CE.stz_ctl|=ctl;

		/* 現在要素が先行要素漢語接続ならば 接続状態指示に漢語を加える */
		if(ctl & BITCTL_PKANGO){ Stz_CE.stz_stz|=BITSTZ_KANGO; }
		/*
		 *	ここで設定した漢語フラグは、
		 *	先行要素との漢語接続の有無を調べるために用いる。
		 *	先行要素との間で接続関係が確認された時点で、
		 *	フラグの意味は後続語との漢語接続可能を示す意味に代える。
		 */

		/* 前方ベクトル番号を移す */
		Stz_CE.stz_pno[cx]=((char(*)[4])si)[grcd-1][GRCDTBL_PNO];
		/* 後端番号を移す */
		StzfBnoAdd(&Stz_CE,((char(*)[4])si)[grcd-1][GRCDTBL_BNO]);

		Next:;
	}
}


/*========== StzfGrcdAdd(grcd) ===============
	機能：現在要素の接続関係コードを追加する。
  ============================================*/
void StzfGrcdAdd(grcd) BYTE grcd; 
{
	register BYTE *si;
	register integer_16 ax;
	BYTE ctl;
	
	/* 文法コードを設定する */
	ax=Stz_CE.stz_grcd[Stz_CE.stz_ngrcd]=grcd;
	si=FddHDR+Fdd_Header.Fdd_GrcdTblOfs;
	/* 制御情報のＯＲをとる */
	ctl=((char(*)[4])si)[ax-1][GRCDTBL_CTL];
	Stz_CE.stz_ctl|=ctl;
	/* 現在要素が先行要素漢語接続ならば 接続状態指示に漢語を加える */
	if(ctl & BITCTL_PKANGO){ Stz_CE.stz_stz|=BITSTZ_KANGO; }
	
	/*
	* ここで設定した漢語フラグは先行要素との漢語接続の有無を調べるために用いる。
	* 先行要素との間で接続関係が確認された時点で、フラグの意味は後続語との
	* 漢語接続可能を示す意味に代える。
	*/
	Stz_CE.stz_pno[Stz_CE.stz_ngrcd]=((char(*)[4])si)[ax-1][GRCDTBL_PNO];	/* 前方ベクトル番号を移す */
	StzfBnoAdd(&Stz_CE,((char(*)[4])si)[ax-1][GRCDTBL_BNO]);
	/* 後端番号を移す */
	Stz_CE.stz_ngrcd++;						/* 文法コード数を増やす */
}


/*========== StzfBnoAdd(elm,bno) =============
	機能：現在要素の後端番号にbnoを追加する。
  ============================================*/
void StzfBnoAdd(elm,bno) BYTE bno; register STZ_STRUCT *elm; {
	register integer_16 n;
	integer_16 next;
 
	n=next=elm->stz_nbno;			/* 後端番号数を取り出す */
	/* 同じ後端番号があるかどうか調べる */
	do{
		if(--n<0){				/* 注目後端番号数を１つ戻す */
									/* 後端番号がないならばやめる */
									/* 全部の後端番号と違っていたならば */
			elm->stz_bno[next]=bno;	/* 後端番号を移す */
			elm->stz_nbno++;		/* 後端番号数を増す */
			return;
		}
	}while(bno!=(BYTE)elm->stz_bno[n]);	/* 後端番号が違っていたら繰り返す */
}


/*========== FOLLOW() =================================================
	機能：現在要素と先行要素の間に接続関係があるかどうかを調べる。
	値：　1 = 接続関係あり  0 = 接続関係なし
  =========================================================StzfStzAri==*/
BOOL FOLLOW() {
	register integer_16 cx;
	BYTE bno;
#if defined(DEBUG_STZ)
	if(debug[4]){ DEBUG_PN("<FOLLOW>"); DEBUG_STZ(); }
#endif
	/*
	 *	最後の要素の場合は、
	 *	最後になりうる要素の時は接続し、なりえない要素の時は接続しない。
	 *	先頭の要素でかつ最後の要素である場合があるのでチェックの仕方に注意する。
	 */
	if((ceptr+celen)==n_inert){
		if((Stz_CE.stz_ctl & BITCTL_LAST)==0){ goto FOLLOW_997; }
	}
	/*
	 * 先頭の要素の場合は、
	 * 最初になりうる要素の時は接続し、なりえない要素の時は接続しない。
	 */
	if(ceptr==0){
		if(Stz_CE.stz_ctl & BITCTL_FIRST){
			goto FOLLOW_510;
		}else{
			goto FOLLOW_997;
		}
	}
	/*
	 * 　先行要素と後続要素の境界が、単漢字辞書検索を始めた位置の時は、
	 * 接続関係は調べない。
	 *   もちろん、初回変換では必ず接続関係を調べなければならない。
	 * 但し、先頭要素は、先頭になりうるかだけは、調べることとする。
	 *
	 *	（例）「２：多から１：多」の「から」と「１」の間
	 *		　「１」は単漢字ではないから特別な処理が必要である。
	 */
	if(conv_mode!=CONV_FIRST){
		if(ceptr==max_ceptr){ goto FOLLOW_500; }
	}

	/*=============== 先頭の要素はこれより下には来ない。===============*/

	/*
	 *	現在要素が単漢字ならば、先行要素が単漢字なら必ず接続し、
	 *	さもなければ、撥音・長音・促音・「を」以外は必ず接続する。-- ウソ --
	 *
	 * 「ふさがって」は「ふさが」迄が普通に変換でき、「っ」以降が変換できない。
	 *	ゆえに先頭になりえない単漢字も接続する必要がある。
	 */
	/*
	 * 現在要素が単漢字でも記号の時は、接続可能性を調べる。
	 * 「書、」で「か、」がでると困る。（９０．４．３０）
	 */
	if(Stz_CE.stz_kind==DICKIND_T){
		switch(inert_kind[ceptr] & 0xF0){
		case CHRKIND_Other1:
		case CHRKIND_Other2:
			break;				/* 記号はmatrixを調べる。*/
		default:
			goto FOLLOW_500;	/* 記号以外はＯＫ。*/
		}
	}

	/* 先行要素・現在要素の両方が数字のときの処理。*/
	if(Stz_PE.stz_kind==DICKIND_D && Stz_CE.stz_kind==DICKIND_D){
		goto FOLLOW_500;
	}

	/* 「１０１ＣＢ」、「Ｐ３Ｃ」などで、数字と英字の接続を許す。*/
	if(Stz_PE.stz_kind==DICKIND_D && Stz_CE.stz_kind==DICKIND_E){
		/* 現在要素が１文字の英字なら接続する。*/
		if(inert_kind[ceptr]==CHRKIND_Alpha){ goto FOLLOW_500; }
	}

	/*
	 * 「ＦＭ」、「ＲＯＭ」等。
	 *	大文字の英字＋数字属性をもった英字の接続を許さない。
	 *	小文字に関しては許すことにする。（ａＭバイト）
	 */
	if(Stz_PE.stz_kind==DICKIND_E && Stz_CE.stz_kind==DICKIND_E){
		/* 先行要素が大文字の英字で終わっていて、
		   現在要素が英字で始まっているとき、*/
		if(inert_kind[ceptr-1]==CHRKIND_Alpha
		&& inert_kind[ceptr]==CHRKIND_Alpha
		&& inert[ceptr*2-1]<0x60){
			/* 現在要素が数字の後端番号をもっていれば接続しない。*/
			if(chk_value(Fdd_Header.Fdd_GrcdDigit,
						 Stz_CE.stz_grcd,Stz_CE.stz_ngrcd)){
				goto FOLLOW_997;
			}
		}
	}

	/* 先行要素が接頭辞の場合は、接頭辞、接尾辞には接続しない。*/
	if(StzfAttrAri(Stz_PE,stz_ctl,BITCTL_PREF)){
		if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_PREF)
		|| StzfAttrAri(Stz_CE,stz_ctl,BITCTL_SUF)){ goto FOLLOW_997; }
	}else{ 
		if(StzfAttrAri(Stz_PE,stz_ctl,BITCTL_SUF)){
			/* 先行要素が接尾辞の場合は、接尾辞には接続しない。*/
			if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_SUF)){ goto FOLLOW_997; }
		}
	}

	/* 漢語接尾辞のときは、漢語接続のチェックは行なわない。*/
	if(StzfAttrAri(Stz_CE,stz_f2,BITFLG2_SUF)){ goto FOLLOW_100; }
	/*
	 * 漢語接尾辞以外の接尾辞も、漢語接続のチェックは行なわない。
	 *	（例）「東京一」は正しい。「田中一」は誤り。
	 */
	if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_SUF)){ goto FOLLOW_100; }

	if((pe_is_kango==0)||(ce_is_kango==0)){ goto FOLLOW_100; }

	/**************************************
	 * 現在要素、先行要素ともに漢語の場合 *
	 **************************************/

	/* 先行要素が非不無未(漢語)で、現在要素が漢語ならば、
	   後端番号に形容動詞を足す。*/
	if(chk_value(Fdd_Header.Fdd_BnoHifu, Stz_PE.stz_bno, Stz_PE.stz_nbno)){
		StzfGrcdAdd(Fdd_Header.Fdd_GrcdAdjv);
	}
	goto FOLLOW_500;

	/**********************************************************
	 * 現在要素、先行要素どちらか、または両方が漢語でない場合 *
	 **********************************************************/
FOLLOW_100:
	 /* 先行要素が接頭辞「お」の場合、*/
	if(chk_value(Fdd_Header.Fdd_BnoPrefo,
				 Stz_PE.stz_bno,Stz_PE.stz_nbno)){
		/* 現在要素が五段動詞語幹の時、「お」に続く五段動詞の指示をする。*/
		if(chk_limits(Fdd_Header.Fdd_Bno5danLL,Fdd_Header.Fdd_Bno5danUL,
					  Stz_CE.stz_bno,Stz_CE.stz_nbno)){
			Stz_CE.stz_stz |= BITSTZ_PREFO5V;
		}else{
			/* 現在要素が一段動詞名詞形の語幹の場合、現在要素にサ変を加える。*/
			if(chk_value(Fdd_Header.Fdd_Bno1danMei,
						 Stz_CE.stz_bno,Stz_CE.stz_nbno)){
				StzfBnoAdd(&Stz_CE,Fdd_Header.Fdd_BnoSahen);
			}
		}
	}else{
		/*
		 * 先行要素が「お」に続く五段動詞語幹で、現在要素が連用形語尾の場合、
		 * 現在要素にサ変動詞語幹を加える。
		 */
		if(StzfAttrAri(Stz_PE,stz_stz,BITSTZ_PREFO5V) &&
		   chk_value(Fdd_Header.Fdd_Bno5danYou,
					 Stz_CE.stz_bno,Stz_CE.stz_nbno)){
			StzfBnoAdd(&Stz_CE,Fdd_Header.Fdd_BnoSahen);
		}
	}

	/* 先行要素が内部辞書語で現在要素副詞付加の場合、*/
	/* 「〜につき」：「つ」に続く「き」は副詞として作用しうる。(連用終止) */
	if(Stz_PE.stz_kind==DICKIND_I){
		if(StzfAttrAri(Stz_PE,stz_f2,BITFLG2_NADV)){
			/*
			 * 現在語が五段動詞連用形の活用語尾の場合は、副詞を付加する。
			 * 現在語が一段動詞連用形の活用語尾の場合は、副詞を付加する。
			 * 現在語がサ変動詞連用形の活用語尾の場合は、副詞を付加する。
			 */
			if(chk_value(Fdd_Header.Fdd_Bno5danYou,
						 Stz_CE.stz_bno,Stz_CE.stz_nbno)
			|| chk_value(Fdd_Header.Fdd_Bno1danMei,
						 Stz_CE.stz_bno,Stz_CE.stz_nbno)
			|| chk_value(Fdd_Header.Fdd_Bno1danYou,
						 Stz_CE.stz_bno,Stz_CE.stz_nbno)
			|| chk_value(Fdd_Header.Fdd_BnoSahenYou,
						 Stz_CE.stz_bno,Stz_CE.stz_nbno)){
				StzfBnoAdd(&Stz_CE,Fdd_Header.Fdd_BnoAdv);
			}
		}
	}

	/* matrixによる接続check。*/
	if(BNO_VECP()==0){ goto FOLLOW_997; }

FOLLOW_500:
	/*
	 * 先行要素が接頭辞の場合、現在要素を文節の切れ目にならなくする。
	 * 但し、制御情報をこわすと、アクセント処理で困るので、接続情報に書込む。
	 */
	if(StzfAttrAri(Stz_PE,stz_ctl,BITCTL_PREF)){
		Stz_CE.stz_stz |= BITSTZ_PREF;
	}
FOLLOW_510:

#if !defined(O_TALKONLY)
	/*
	 *	＜＜注目文節長さ固定処理をする場合＞＞
	 *   次候補・短方向・長方向のとき、
	 * 注目文節の開始位置と次の文節の開始位置とをあらかじめ設定しておき、
	 * その位置と矛盾する文節は破棄する。
	 *   次候補のときは開始位置・次開始位置ともに前と同じである。
	 * 短方向・長方向のときは、次開始位置をあらかじめ求めておく。
	 */
	if(conv_mode==CONV_NEXT||conv_mode==CONV_LONG||conv_mode==CONV_SHORT){
		/*
		 * 数字の場合、ceptr,celen,fsptrに矛盾があれば接続なし。
		 * 短方向処理の場合に「１００１人」は１文字短くできるが２文字はできない。
		 * そこで、celenがfsptrを越えていないかどうかをここで調べる。
		 */
		if(WS_CE_CHK()==0){ goto FOLLOW_997; }
		/*
		 * 現在要素が固定位置指示内で始まり、
		 * 最初になり得る要素ならば、接続なしとする。
		 */
		if(wsptr<ceptr && ceptr<fsptr){
			if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_FIRST)){
				if(StzfAttrNashi(Stz_CE,stz_stz,BITSTZ_PREF)){
					goto FOLLOW_997;
				}
			}
		}
		/*
		 * 現在要素が固定位置指示の始めまたは終わりから始まり、
		 * 最初になり得ない要素ならば、接続なしとする。
		 */
		if(wsptr==ceptr||ceptr==fsptr){
			if(StzfAttrNashi(Stz_CE,stz_ctl,BITCTL_FIRST)){ goto FOLLOW_997; }
			if(StzfAttrAri(Stz_CE,stz_stz,BITSTZ_PREF)){ goto FOLLOW_997; }
		}
	}
#endif

	/* 現在要素が後続語漢語接続なら漢語接続とする。*/
	if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_FKANGO)){ goto FOLLOW_720; }
	/* 先行要素が接頭辞「第」でないなら漢語接続をはずす。*/
	if(chk_value(Fdd_Header.Fdd_GrcdDai,
				 Stz_PE.stz_bno,Stz_PE.stz_nbno)==0){
		goto FOLLOW_740;
	}
	/* 現在要素が数字でないなら漢語接続をはずす。*/
	if(chk_value(Fdd_Header.Fdd_GrcdDigit,
				 Stz_CE.stz_grcd,Stz_CE.stz_ngrcd)==0){
		goto FOLLOW_740;
	}

FOLLOW_720:
	/* 接続に漢語を追加する。*/
	Stz_CE.stz_stz |= BITSTZ_KANGO; goto FOLLOW_900;
	/* それ以外は漢語接続をはずす。*/
FOLLOW_740:
	/* 接続から漢語を削除する。*/
	Stz_CE.stz_stz &= ~BITSTZ_KANGO;
FOLLOW_900:
	/*
	 * 「コンピューター」の最後の「ー」、「それ自身」の「自身」
	 * 「〜すべて」の「すべて」
	 * 「〜がすべて」の「すべて」は、除外されるので注意する。(89/1/20)
	 * この要素の制御情報と後端番号は、ひとつ前の語のものを使う。
	 */
	if(StzfAttrAri(Stz_CE,stz_f2,BITFLG2_SELF)){
		/*
		 * 直前の要素がないときは除外する。
		 * 先頭になりえない接尾辞ならコピーする。
		 */
		if(ceptr==0){ goto FOLLOW_990; }
		if(Stz_PE.stz_kind==DICKIND_T){
			switch(inert_kind[ceptr-1] & 0xF0){
			case CHRKIND_Other1:
			case CHRKIND_Other2:
				goto FOLLOW_990;
			}
		}
		if((Stz_CE.stz_ctl & BITCTL_FIRST)==0){ goto FOLLOW_910; }
		/*
		 * 直前の要素が体言なら、ひとつ前のものにする。
		 */
		for(cx=0;cx<Stz_PE.stz_nbno;++cx){
			if(bno=Stz_PE.stz_bno[cx]){
				if(FddHDR[Fdd_Header.Fdd_BnoTblOfs+bno-1] & BITBNO_Taigen){
					goto FOLLOW_910;
				}
			}
		}
	}
	goto FOLLOW_990;
FOLLOW_910:
	Stz_CE.stz_ctl|=Stz_PE.stz_ctl & ~BITCTL_FIRST;	/* without 1st */
	Stz_CE.stz_stz=Stz_PE.stz_stz;
	Stz_CE.stz_f2|=Stz_PE.stz_f2 & 0x08;	/* 漢語bit */
	cx=(Stz_CE.stz_nbno=Stz_PE.stz_nbno);
	for(;cx--;){
		Stz_CE.stz_bno[cx]=Stz_PE.stz_bno[cx];
	}
FOLLOW_990:
#if defined(O_DEBUG)
	if(debug[6]){ DEBUG_ELM(1); }
#endif
	return 1;
FOLLOW_997:
#if defined(O_DEBUG)
	if(debug[6]){ DEBUG_ELM(0); }
#endif
	return 0;
}

#if defined(DEBUG_STZ)
/*========== DEBUG_STZ() =====================================
	機能：debug routine for Stz_CE,Stz_PE,ceptr,celen.
  ============================================================*/
void DEBUG_STZ() {
	register integer_16 i,n;

	if(ceptr==0){ goto DEBUG_STZ99; }

	switch(Stz_PE.stz_kind){
	case DICKIND_D:  printf(",数字"); break;
	case DICKIND_5Y: printf(",連用"); break;
	case DICKIND_I:  printf(",内部"); break;
	default:
		printf(pe_is_kango ? ",漢語" : ",外部");
	}
	printf(",F2="); DEBUG_H1(Stz_PE.stz_f2);
	printf(",CTL="); DEBUG_H1(Stz_PE.stz_ctl);
	printf(",MRU="); DEBUG_H1(Stz_PE.stz_mru);
	printf(",STZ="); DEBUG_H1(Stz_PE.stz_stz);

	n=Stz_PE.stz_nbno; printf(",NBNO=%d",n); printf(",BNO=");
	for(i=0;n--;i++){
		printf("%d",Stz_PE.stz_bno[i]);
		if(n>0){ putchar(','); }
	}
	putchar('\n');

DEBUG_STZ99:
	/* print current word */
	printf(" iP=%d",ceptr+1); printf(",iL=%d",celen);
	putchar(','); DEBUG_K1(&inert[ceptr*2],celen);
	putchar('\n');

	switch(Stz_CE.stz_kind){
	case DICKIND_D:  printf(",数字"); break;
	case DICKIND_5Y: printf(",連用"); break;
	case DICKIND_I:  printf(",内部"); break;
	default:
		printf(ce_is_kango ? ",漢語" : ",外部");
	}
	printf(",F2="); DEBUG_H1(Stz_CE.stz_f2);
	printf(",CTL="); DEBUG_H1(Stz_CE.stz_ctl);
	printf(",MRU="); DEBUG_H1(Stz_CE.stz_mru);
	printf(",STZ="); DEBUG_H1(Stz_CE.stz_stz);

	n=Stz_CE.stz_nbno; printf(",NBNO=%d",n); printf(",BNO=");
	for(i=0;n--;i++){
		printf("%d",Stz_CE.stz_bno[i]);
		if(n>0){ putchar(','); }
	}
	putchar('\n');
}
#endif

/***************************** END OF ILE_STZC.C *****************************/
