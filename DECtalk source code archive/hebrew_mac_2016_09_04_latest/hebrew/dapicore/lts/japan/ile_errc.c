/*******************************************
	ILE_ERRC.C	updated at 19th May 1989
	エラー処理
 *******************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_errh.h"
#include	"ile_kkhh.h"
#include	"ile_func.h"

jmp_buf jmpbuf;
integer_16 error_code = 0;

/*========== ERROR(ax) ==================================================
	機能：エラーコード ax のエラー状態を設定し初期設定時のラベルに行く。
  =======================================================ErrfSetteisuru==*/
void jp_ERROR(ax) integer_16 ax; {
	error_code=ax;			/* エラーコードを設定する。*/
	longjmp(jmpbuf,1);		/* 指定されたラベルに行く。*/
}

/*========== ERROR5000() =========================
	機能：エラー発生にともない出力結果を設定する
  =============================ErrfKekSetteisuru==*/
void ERROR5000() {
	kkhstat=err;
}

/***************************** END OF ILE_ERRC.C *****************************/
