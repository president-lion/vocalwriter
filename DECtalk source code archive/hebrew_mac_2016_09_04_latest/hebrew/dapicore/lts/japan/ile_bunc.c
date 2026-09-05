/***********************************************
	ILE_BUNC.C	updated at 25th October 1990
	文節処理関連操作
	注目文節右移動・左移動
	注目文節短方向変換・長方向変換
 ***********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_bunh.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_kkhh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_func.h"

integer_16 ws_no = 0;
integer_16 wsptr = 0;
integer_16 fsptr = 0;
integer_16 fsptr_sav = 0;

/*========== PRON_FIRST() =========================
	機能：文章音声変換初回（詳細は外部仕様書参照）
  ===========================ShhfShokaiHenkansuru==*/
void PRON_FIRST() {
	integer_16 status;
	conv_mode=CONV_FIRST;

#if !defined(O_TALKONLY)
	/* 学習可能な状態ならば利用者注目文節までを学習する。*/
	/*if(KkhfMruKanou)then MRU_CSENT(); fi*/
#endif

	/* 初期状態に設定する。*/
	kkhstat=shoki;
	MRU_OR_NOT=0;
	PRON_INIT(); INERT_INIT();

	clear_null();

	ceptr=0;					/* ceptrを文の先頭にする。*/
	status=CS1000(0);			/* 現在要素位置(ceptr)から文章音声変換をする。*/
	if(status==1){			/* 変換できたならば、*/
		WS_BECOMES_TOP();		/* 注目文節番号を先頭に設定する。*/
		kkhstat=bunsetsu;		/* 文節設定状態に設定する。*/
	}else{					/* 変換できなければ、*/
		/* 単漢字辞書使用モードの時は、単漢字辞書を使用して変換し直す。*/
		/* 単漢字辞書未使用モードの時は、変換できた所までを変換し直す。*/
		if(onsei_addr->onsei_usetankan==0){
			if(max_ceptr==0){
				/* １文字も変換できなかったときは、エラーとする。*/
				ERROR5000();
				jp_ERROR(Err_HenkanFunou);
			}else{
				/* １文字でも変換できた時は、できた所まで変換し直す。*/
				/* 文の長さは変換できた最長の文字数とする。*/
				n_inert=max_ceptr;
			}
		}
#if defined(O_DEBUG)
		DEBUG_STR("『やり直しぢゃ。』\n");
#endif
		PRON_INIT2();			/* 出力領域・スタック・文節位置情報を初期化。*/
		ceptr=0;				/* ceptrを文の先頭にする。*/
		conv_mode=CONV_RETRY;
		/* 現在要素位置(ceptr)から文章音声変換をする */
		if(CS1000(0)==0){
			/* 「っ」や「ん」を単独で変換するとこうなる。*/
			jp_ERROR(Err_HenkanFunou);
		}
		WS_BECOMES_TOP();
		kkhstat=bunsetsu;
		if(status==0){
			jp_ERROR(Err_HenkanTochuu);	/* status==0 */
		}else{
			jp_ERROR(Err_RestrictCNT);		/* status==2 */
		}
	}
}


/*========== PRON_NEXT() ============================
	機能：文章音声変換次候補（詳細は外部仕様書参照）
  ===================BtgfBunsetsuTsugikouhoMotomeru==*/
void PRON_NEXT() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	conv_mode=CONV_NEXT;
	MRU_OR_NOT=1;
	if(kkhstat==bunsetsu){ goto PRON_NEXT_100; }	/* 文節状態 */
	if(kkhstat==funou   ){ goto PRON_NEXT_200; }	/* 変換不能状態 */
	jp_ERROR(Err_Abnormal); 

PRON_NEXT_100:					/* 文節状態ならば */
	MRU_REMEMBER();				/* 前回の変換結果に使用した語を記録する */
	OMEGA_SAVE();				/* 前回の変換結果を比較のために保存する */
	/* 注目文節の次の文節(following sentence)の位置を保存する。*/
	/*fsptr_sav=fsptr;*/
	PRON_NEXT_120:
		STACK_POP_FS();			/* 現在注目文節になるまでスタックを戻す */
		if(CS1000(1)==0){ goto PRON_NEXT_180; }	/* 変換不可能ならば */
	if(OMEGA_CHECK()){ goto PRON_NEXT_120; }	/* 前回と同じならやり直す */
	kkhstat=bunsetsu;			/* 文節設定状態とする */
	goto PRON_NEXT_990;

PRON_NEXT_180:
	CS1000(0); 					/* 現在の状態から文章音声変換をする */
	kkhstat=funou;				/* 変換不可能状態とする */
	jp_ERROR(Err_NextNone);	 	/* 変換不能状態を知らせる */

PRON_NEXT_200:					/* 変換不能状態ならば */
	STACK_POP_WS();				/* 現在注目文節の次までスタックを戻す */
	kkhstat=bunsetsu;			/* 文節設定状態を記録する */
	CS1000(0);					/* 続きを再変換する */

PRON_NEXT_990:
	return;
#endif
}


/*========== PRON_TERM() ============================
	機能：文章音声変換確定（詳細は外部仕様書参照）
  ============================KakfHenkanKakuteisuru==*/
void PRON_TERM() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	if(KkhfMruKanou){
		MRU_ALL();
		kkhstat=shoki;
	}else{
		jp_ERROR(Err_Abnormal);
	}
#endif
}


/*========== PRON_ACON() =================
	機能：文章音声変換アクセント結合指定
  ========================================*/
void PRON_ACON() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	integer_16 p;
	if(ws_no==sent_ptr+1){ return; }	/* 最終文節のときは無視する。*/

	conv_mode=CONV_NEXT;
	/* 変換不能状態 or 文節状態ならば処理する。*/
	if(kkhstat==bunsetsu||kkhstat==funou){ goto PRON_ACON_100; }
	jp_ERROR(Err_Abnormal);

PRON_ACON_100:
	for(p=0;p<stc_ptr;++p){
		if(stc_in_ptr[p]==sent_in_ptr[ws_no-1]){ goto PRON_ACON_200; }
	}
PRON_ACON_200:					/* 文節状態ならば */
	stc_mru[p]|=BITMRU_ACON;
	stc_mru[p]&=~BITMRU_ACOFF;
	ACCXXXX();
	kkhstat=bunsetsu;			/* 文節設定状態とする */
#endif
}


/*========== PRON_ACOFF() ================
	機能：文章音声変換アクセント結合放棄
  ========================================*/
void PRON_ACOFF() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	integer_16 p;
	if(ws_no==sent_ptr+1){ return; }	/* 最終文節のときは無視する。*/

	conv_mode=CONV_NEXT;
	/* 変換不能状態 or 文節状態ならば処理する。*/
	if(kkhstat==bunsetsu||kkhstat==funou){ goto PRON_ACOFF_100; }
	jp_ERROR(Err_Abnormal);

PRON_ACOFF_100:
	for(p=0;p<stc_ptr;++p){
		if(stc_in_ptr[p]==sent_in_ptr[ws_no-1]){ goto PRON_ACOFF_200; }
	}
PRON_ACOFF_200:					/* 文節状態ならば */
	stc_mru[p]|=BITMRU_ACOFF;
	stc_mru[p]&=~BITMRU_ACON;
	ACCXXXX();
	kkhstat=bunsetsu;			/* 文節設定状態とする */
#endif
}


/*========== PRON_PAUSE_INS() =========
	機能：文章音声変換ポーズ強制挿入
  =====================================*/
void PRON_PAUSE_INS() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	integer_16 p;
	if(ws_no==sent_ptr+1){ return; }	/* 最終文節のときは無視する。*/

	conv_mode=CONV_NEXT;
	/* 変換不能状態 or 文節状態ならば処理する。*/
	if(kkhstat==bunsetsu||kkhstat==funou){ goto PRON_PINS_100; }
	jp_ERROR(Err_Abnormal);

PRON_PINS_100:
	for(p=0;p<stc_ptr;++p){
		if(stc_in_ptr[p]==sent_in_ptr[ws_no-1]){ goto PRON_PINS_200; }
	}
PRON_PINS_200:					/* 文節状態ならば、*/
	stc_mru[p]|=BITMRU_PINS;
	stc_mru[p]&=~BITMRU_PDEL;
	ACCXXXX();
	kkhstat=bunsetsu;			/* 文節設定状態とする。*/
#endif
}


/*========== PRON_PAUSE_DEL() =========
	機能：文章音声変換ポーズ強制削除
  =====================================*/
void PRON_PAUSE_DEL() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	integer_16 p;
	if(ws_no==sent_ptr+1){ return; }	/* 最終文節のときは無視する。*/

	conv_mode=CONV_NEXT;
	/* 変換不能状態 or 文節状態ならば処理する。*/
	if(kkhstat==bunsetsu||kkhstat==funou){ goto PRON_PDEL_100; }
	jp_ERROR(Err_Abnormal);

PRON_PDEL_100:
	for(p=0;p<stc_ptr;++p){
		if(stc_in_ptr[p]==sent_in_ptr[ws_no-1]){ goto PRON_PDEL_200; }
	}
PRON_PDEL_200:					/* 文節状態ならば、*/
	stc_mru[p]|=BITMRU_PDEL;
	stc_mru[p]&=~BITMRU_PINS;
	ACCXXXX();
	kkhstat=bunsetsu;			/* 文節設定状態とする。*/
#endif
}


/*========== PRON_SENT_NO() ==================
	機能：現在注目している文節番号を取り出す。
  ==============CbtfChuumokubunsetsuToridasu==*/
void PRON_SENT_NO() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	/* 文節状態または変換不可能状態ならば、*/
	if(kkhstat==bunsetsu || kkhstat==funou){
		/* 注目文節(watching sentence)の番号を設定する。*/
		onsei_addr -> onsei_wsno = ws_no;
	}else{
		jp_ERROR(Err_Abnormal);
	}
#endif
}


/*========== PRON_RIGHT() =======================
	機能：注目文節右移動（詳細は外部仕様書参照）
		　移動できないとき、エラー
  =====================BmgfBunsetsuMigiIdousuru==*/
void PRON_RIGHT() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	conv_mode=CONV_RIGHT;
	if(kkhstat==bunsetsu || kkhstat==funou){
		/* 変換初期状態でなければ、*/
		OMEGA9000();				/* 前回の変換結果をそのまま渡す。*/
		/* 注目文節(watching sentence)が最後の文節でないならば、*/
		if(ws_no!=sent_ptr+1){
			WS_MOVE_RIGHT();		/* 注目文節を進める。*/
			kkhstat=bunsetsu;		/* 文節設定状態に設定する。*/
			return;
		}else{					/* 最後の文節ならば、*/
			jp_ERROR(Err_IdouFunou);	/* 移動不能を知らせる。*/
		}
	}else{
		jp_ERROR(Err_Abnormal);
	}
#endif
}


/*========== PRON_LEFT() ========================
	機能：注目文節左移動（詳細は外部仕様書参照）
		　移動できないとき、エラー
  ===================BhifBunsetsuHidariIdousuru==*/
void PRON_LEFT() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	conv_mode=CONV_LEFT;
	if(kkhstat==bunsetsu || kkhstat==funou){
		/* 変換初期状態でなければ */
		OMEGA9000();				/* 前回の変換結果をそのまま渡す */
		/* 注目文節(watching sentence)が先頭の文節でないならば */
		if(ws_no!=1){
			WS_MOVE_LEFT();			/* 注目文節を戻す */
			kkhstat=bunsetsu;		/* 文節設定状態に設定する */
			return;
		}else{					/* 先頭の文節ならば */
			jp_ERROR(Err_IdouFunou);	/* 移動不能を知らせる */
		}
	}else{
		jp_ERROR(Err_Abnormal);
	}
#endif
}


/*========== PRON_SHORT() ===========================
	機能：注目文節短方向変換（詳細は外部仕様書参照）
  ==========================BtnfTanhoukouHenkansuru==*/
void PRON_SHORT() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	conv_mode=CONV_SHORT;
	/* 文節状態または変換不能状態ならば */
	if(kkhstat==bunsetsu || kkhstat==funou){
		MRU_OR_NOT=1;
		/* 注目文節の次の文節(following sentence)の位置を保存する。*/
		fsptr_sav=fsptr;
		MRU_REMEMBER();				/* 前回の変換結果に使用した語を記録する */
		do{						/* 短い文節の切れ目が見つかるまで繰り返す*/
			/* 利用者注目文節長さが縮まらなければ、状態を回復してエラーとする */
			if(WS_CHK_SHORT()==0){ PRON_RECOVER(); }
			/* 注目文節(ceptr==wsptr)になるまでスタックを戻す。*/
			STACK_POP_WS();
			/* 注目文節の先頭から新しく変換する。*/
		}while(CS1000(0)==0);
		return;
	}else{						/* その他の状態ならば */
		jp_ERROR(Err_Abnormal);	/* 呼び出し矛盾を知らせる */
	}
#endif
}


/*========== PRON_LONG() ============================
	機能：注目文節長方向変換（詳細は外部仕様書参照）
  =========================BchfChouhoukouHenkansuru==*/
void PRON_LONG() {
#if defined(O_TALKONLY)
	ERROR(Err_Abnormal);
#else
	conv_mode=CONV_LONG;
	/* 文節状態または変換不能状態ならば */
	if(kkhstat==bunsetsu || kkhstat==funou){
		MRU_OR_NOT=1;
		/* 注目文節の次の文節(following sentence)の位置を保存する。*/
		fsptr_sav=fsptr;
		/* 注目文節(watching sentence)が最後の文節でないならば */
		if(ws_no!=sent_ptr+1){
			MRU_REMEMBER(); 		 	/* 前回の変換結果に使用した語を記録する*/
			do{						/* 長い文節の切れ目が見つかるまで繰り返す*/
				/* 利用者注目文節長さが延せないなら回復してエラーとする */
				if(WS_CHK_LONG()==0){ PRON_RECOVER(); }
				/* 注目文節(ceptr==wsptr)になるまでスタックを戻す。*/
				STACK_POP_WS();
				/* 注目文節の先頭から新しく変換する。*/
			}while(CS1000(0)==0);
			return;
		}
		PRON_RECOVER();				/* 最後の文節ならばエラー */
	}
	/* その他の状態ならば、呼び出し矛盾を知らせる */
	jp_ERROR(Err_Abnormal);
#endif
}


#if !defined(O_TALKONLY)
/*========== PRON_RECOVER() ==============================
	機能：長方向変換・短方向変換で、変換不能のときの処理
  =============================================BchfRecvr==*/
void PRON_RECOVER() {
	kkhstat=bunsetsu;				/* 文節状態を設定する */
	fsptr=fsptr_sav;				/* 注目文節の次の文節の位置を戻す */
	STACK_POP_WS();					/* 注目文節までスタックを戻す */
	CS1000(0);						/* 注目文節の先頭から変換しなおす */
	jp_ERROR(Err_IdouFunou);			/* エラーコードをセットして戻る*/
}
#endif


/*========== WS_BECOMES_TOP() ===================
	機能：利用者注目文節位置を文頭に設定する。
		Make watching sentence to Top position.
  ============================BunfChbHajimesuru==*/
void WS_BECOMES_TOP() {
	onsei_addr->onsei_wsno = ws_no = 1;
							/* 注目文節番号(watching sentence no.)を１にする */
	wsptr=0;				/* 注目文節(watching sentence)の位置を設定する */
	fsptr=sent_in_ptr[0];	/* 注目文節の次の文節(following sentence)の位置を設定する */
}


#if !defined(O_TALKONLY)
/*========== WS_MOVE_LEFT() ==================
	機能：利用者注目文節管理情報を１文節戻す。
  =============================BunfChbModosu==*/
void WS_MOVE_LEFT() {
	/* 利用者注目文節番号を減らす */
	--ws_no;
	onsei_addr -> onsei_wsno = ws_no;
	/* 直前の利用者注目文節開始位置を現在の文節終了位置とする */
	fsptr=wsptr;
	/* 利用者注目文節開始位置を設定する*/
	/* 直前の利用者注目文節終了位置を現在文節開始位置とする 
	   先頭の文節ならば文節開始位置を文頭とする */
	wsptr=(ws_no==1 ? 0 : sent_in_ptr[ws_no-2]);
}
#endif


#if !defined(O_TALKONLY)
/*========== WS_MOVE_RIGHT() ===================
	機能：利用者注目文節管理情報を１文節進める。
  =============================BunfChbSusumeru==*/
void WS_MOVE_RIGHT() {
	/* 利用者注目文節番号を増やす */
	++ws_no;
	onsei_addr -> onsei_wsno = ws_no;
	/* 直前の利用者注目文節終了位置を現在の文節開始位置とする */
	wsptr=fsptr;
	/* 利用者注目文節終了位置を設定する */
	fsptr=sent_in_ptr[ws_no-1];
}
#endif


#if !defined(O_TALKONLY)
/*========== WS_CHK_SHORT() ==========================================================
	構文：if(WS_CHK_SHORT()) 縮まったとき else 縮まないとき;
	機能：利用者現在注目文節長さが縮まるかどうかを判定し、縮まればその長さを設定する。
		　さらに、新しい文節終了位置を設定する。
		　文節終了位置とは次の文節の開始位置（ゼロオリジン）のことである。
	出力：縮まるとき、縮まった後の文節の長さ（非ゼロ）
		　縮まらないとき、ゼロ
  ===============================================================BunfChbYomChijimaru==*/
integer_16 WS_CHK_SHORT() {
	register integer_16 new_len,ptr; integer_16 i;

	/* （昔）数字の場合は縮まらない。（今）小数点を区切りにするときだけ縮まる。*/
	for(i=0;i<stc_ptr;++i){
		if(stc_in_ptr[i]==fsptr){ goto WS_CHK_SHORT100; }
	}
	i=stc_ptr;
WS_CHK_SHORT100:
	/*if(stc_kind[i-1]==DICKIND_D){ return 0; }*/

	ptr=wsptr;
	/* 注目文節長さを短くする */
	new_len=INERT200B(ptr,fsptr-ptr);
	/* 新しい文節終了位置をセットする */
	fsptr=ptr+new_len;
	return new_len;		/* 新しい文節長さ（縮まらないときゼロ）*/
}
#endif


#if !defined(O_TALKONLY)
/*========== WS_CHK_LONG() ===========================================================
	構文：if(WS_CHK_LONG()) 延びたとき else 伸びないとき;
	機能：利用者現在注目文節長さが延びるかどうかを判定し、伸びればその長さを設定する
	出力：延びる、延びた後の文節の長さ（非ゼロ）
		　延びないとき、ゼロ
  ==================================================================BunfChbYomNobiru==*/
integer_16 WS_CHK_LONG() {
	register integer_16 new_len,ptr;
	ptr=wsptr;
	/* 注目文節長さを長くする */
	new_len=INERT300B(ptr,fsptr-ptr);
	/* 新しい文節終了位置をセットする */
	fsptr=ptr+new_len;
	return new_len;		/* 新しい文節長さ（延びないときゼロ）*/
}
#endif


/*========== SENT_INIT() ================
	機能：注目文節管理情報を初期化する。
  ======================BunfShokikasuru==*/
void SENT_INIT() {
	register integer_16 AX=0;
	/* watching sentence no. */
	/* watching sentence pointer */
	/* following sentence pointer */
	ws_no=wsptr=fsptr=fsptr_sav=AX;
}


#if !defined(O_TALKONLY)
/*========== WS_CE_CHK() =========================================================
	構文：if(WS_CE_CHK()){ no error; }else{ error; }
	機能：現在要素の開始位置／長さが文節固定指示位置と矛盾がないかどうかを調べる。
	出力：１：正常、０：エラー有り
  ================================================================BunfYomKoteiAu==*/
BOOL WS_CE_CHK() {
	register integer_16 cetail,ptr;
	cetail=(ptr=ceptr)+celen-1;
	/* 現在要素が指示位置開始をまたがっている場合は矛盾あり */
	/* 現在要素が指示位置終了をまたがっている場合は矛盾あり */
	if((ptr<wsptr) && (cetail>=wsptr)){ return 0; }
	if((ptr<fsptr) && (cetail>=fsptr)){ return 0; }
	return 1;
}
#endif

/***************************** END OF ILE_BUNC.C *****************************/
