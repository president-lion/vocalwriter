/********************************************
	ILE_SHDC.C	updated at 31th July 1990
	出力データ関連操作
 ********************************************/

#define		EXT		extern

#if defined(OS9)
#include	"stdio.h"
#else
#include	<stdio.h>
#endif

#include	"onsei.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_dbgh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"


/*========== OMEGA_INIT() ===============
	機能：出力データエリアを初期化する。
  ======================ShdfShokikasuru==*/
void OMEGA_INIT() 
{
	register integer_16 i;

	/* 出力発音記号注目位置・現在語変換結果長さ・注目文節位置 */
	cpron_ptr= cpron_len= sent_ptr= 0;

	/* 文節位置情報 */
	for(i=0;i<MAX_SENT;i++){
		sent_in_ptr[i]=sent_out_ptr[i]=sent_in_ptr_sav[i]=sent_out_ptr_sav[i]=0;
	}

	/* 変換結果 */
	/* ひとつ前の変換結果の保存領域 */
	for(i=0;i<MAX_OMEGA;i++){ omega[i]=omega_sav[i]='\0'; }
}

/*========== OMEGA1000(al,di) ===========================================
	機能：al で渡された出力結果文字を di の示す出力データエリアにしまう。
	値  ：次の文字を指すポインタを返す。
  ======================================================ShdfKekMjShimau==*/
integer_16 OMEGA1000(al,di) BYTE al; register integer_16 di; {
	if(di>=MAX_OMEGA){ jp_ERROR(Err_NaibuOVF); }	/* overflow */
	omega[di]=al;
	return (di+1);
}

/*========== OMEGA5000() =======================================================
	機能：現在注目語表記の指示に従って、出力データ中間エリアに変換結果を求める。
			数字の場合はコールされない。
  =============================================================ShdfKekMotomeru==*/
void OMEGA5000(){
	register integer_16 di,bx;

	di=cpron_ptr;						/* 出力結果位置を求める。*/

	/* 現在語の種別に応じて表記を求める。*/
	/* 句読点、特殊文字は、単純語に分類される。*/
	switch(Stz_CE.stz_kind){
	case DICKIND_I:		di=IDIC5000(di); break;		/* 内部辞書 */
	case DICKIND_E:		di=EDIC5000(di); break;		/* 外部辞書 */
	case DICKIND_T:									/* 単漢字語 */
						if(Stz_CE.stz_dman!=0xFF){
							di=TDIC5000(di);		/* 辞書にある語 */
						}else{
							di=TDIC5009(di);		/* 辞書にない語 */
						}
						break;
	case DICKIND_K:		di=KKANA5000(di); break;	/* 片仮名語 */
	}

	cpron_ptr=di;			/* 出力データ中間エリアの位置を保存する。*/
	bx=sent_ptr;		 	/* 出力データ中間エリアの文節番号を得る。*/
							/* 接頭辞を伴わない最初になりえる語ならば。*/
	if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_FIRST)
	&& StzfAttrNashi(Stz_CE,stz_stz,BITSTZ_PREF)){
		/* 出力可能最大文節数を超えているならば */
		if(bx>=MAX_SENT){ jp_ERROR(Err_NaibuOVF); }
		/* 先頭語でなければ文節を進める。*/
		if(ceptr!=0){ bx++; sent_ptr=bx; }
	}
	/* 出力データ中の入力データ位置と出力データ位置を設定する。*/
	sent_out_ptr[bx]=di;
	sent_in_ptr [bx]=ceptr+celen;
}


/*========== OMEGA6000() ======================
	機能：出力データ中間エリアの内容を１語戻す
  ==============================ShdfKekModosu==*/
void OMEGA6000() {
	register integer_16 bx;

	bx=sent_ptr; 				/* 出力データ中間エリアの文節位置を得る。*/
	sent_out_ptr[bx]=cpron_ptr;	/* 出力データ中の出力データ位置を設定する。*/
	sent_in_ptr [bx]=ceptr;		/* 出力データ中の入力データ文字位置を設定する。*/
								/* 接頭辞を伴わない最初になりえる語ならば。*/
	if(StzfAttrAri(Stz_CE,stz_ctl,BITCTL_FIRST)
	&& StzfAttrNashi(Stz_CE,stz_stz,BITSTZ_PREF)){
		if(ceptr!=0){		/* 先頭語でなければ文節を戻す。*/
			sent_in_ptr[bx]=sent_out_ptr[bx]=0;
			bx--;
			sent_ptr=bx;
		}
	}
}

/*========== OMEGA9000() =========================================
	機能：出力データ中間エリアにある情報を出力データエリアに移す。
  =================================================ShdfKekWatasu==*/
void OMEGA9000() {
	register BYTE *si;
	integer_16 col;
	register integer_16 cx;
	LPBYTE shd;
	LPWORD shd16;

	onsei_addr->onsei_sentno=sent_ptr+1;
	shd16=onsei_addr->onsei_sent;
	for(cx=0;cx<=sent_ptr;++cx){
		*shd16++=sent_in_ptr[cx];
		*shd16++=sent_out_ptr[cx];
	}

	shd16=onsei_addr->onsei_sent;
	shd=onsei_addr->onsei_speak;
	si=speak;
	col=0;
	++shd16;
	for(;*si;){
		if(*si==0xFF){
			*shd16=col;
			++si;
			shd16+=2;
		}else{
			*shd++= *si++;
			col++;
		}
	}
	*shd++=0x00;
}

/*========== OMEGA_SAVE() ======================================
	機能：出力データを次の変換結果と比較するために一時保存する
  ===============================================ShdfKekOboeru==*/
void OMEGA_SAVE() {
	memcpy(sent_in_ptr_sav, sent_in_ptr, MAX_SENT*2);
	memcpy(sent_out_ptr_sav,sent_out_ptr,MAX_SENT*2);
	memcpy(omega_sav,       omega,       MAX_OMEGA);
	memcpy(accent_sav,      accent0,     MAX_OMEGA);
}

/*========== OMEGA_CHECK() ===========================================
	機能：現在の出力データが直前の変換結果と同じかどうかを調べる。
		　比較する項目は、文節の切れ目、発音表記、アクセントである。
	値　：０＝異なる，１＝同じ
  ======================================================ShdfKekOnaji==*/
BOOL OMEGA_CHECK() {
	if(memcmp(sent_in_ptr, sent_in_ptr_sav, MAX_SENT*2)){ return 0; }
	if(memcmp(sent_out_ptr,sent_out_ptr_sav,MAX_SENT*2)){ return 0; }
	if(memcmp(omega,       omega_sav,       MAX_OMEGA )){ return 0; }
	if(memcmp(accent0,     accent_sav,      MAX_OMEGA )){ return 0; }
	return 1;
}

/***************************** END OF ILE_SHDC.C *****************************/
