/*************************************************
	ILE_KKHC.C	updated at 26th September 1989
	メインプログラム
 *************************************************/

#define		EXT		extern

#include <stddef.h>
#include	"onsei.h"
#include	"ile_bunh.h"
#include	"ile_errh.h"
#include	"ile_stch.h"


extern	void	ENV_INIT();
extern	void	ENV_TERM();
extern	void	PRON_FIRST();
extern	void	PRON_NEXT();
extern	void	PRON_SHORT();
extern	void	PRON_LONG();
extern	void	PRON_RIGHT();
extern	void	PRON_LEFT();
extern	void	PRON_TERM();
extern	void	PRON_SENT_NO();
extern  void	EDIC_DELETE2();
extern  void	EDIC_ADD();
extern  void	EDIC_DELETE();
extern  void	PRON_ACON();
extern  void	PRON_ACOFF();
extern  void	PRON_PAUSE_INS();
extern  void	PRON_PAUSE_DEL();
extern  void	display_user();

STRUCT_ONSEI* onsei_addr = NULL;

/* ジャンプテーブル */
#define		TBLSIZE		18
static void(* const JmpTbl[TBLSIZE])(void) = {
  ENV_INIT,							/*  1 文章音声変換処理初期設定 */
  ENV_TERM,							/*  2 文章音声変換処理終了 */
  PRON_FIRST,							/*  3 文章音声変換初回 */
  PRON_NEXT,							/*  4 注目文節次候補 */
  PRON_SHORT,							/*  5 文節短方向分割 */
  PRON_LONG,							/*  6 文節長方向分割 */
  PRON_RIGHT,							/*  7 注目文節右移動 */
  PRON_LEFT,							/*  8 注目文節左移動 */
  PRON_TERM,							/*  9 変換確定 */
  PRON_SENT_NO,						/* 10 注目文節番号の取り出し */
  EDIC_DELETE2,						/* 11 外部辞書ユーザー語削除 */
  EDIC_ADD,							/* 12 外部辞書ユーザー語追加 */
  EDIC_DELETE,						/* 13 外部辞書ユーザー語削除 */
  PRON_ACON,							/* 14 アクセント結合 */
  PRON_ACOFF,							/* 15 アクセント非結合 */
  PRON_PAUSE_INS,						/* 16 ポーズ挿入 */
  PRON_PAUSE_DEL,						/* 17 ポーズ削除 */
  display_user,						/* 18 user名の取り出し */
};

integer_16
entrypt(STRUCT_ONSEI* addr)
{
  register integer_16 no;
  
  onsei_addr = addr;
  no = addr->onsei_procno;
  
  error_code=0;
  if(setjmp(jmpbuf)==0) {
    if(1<=no && no<=TBLSIZE) {
      (*JmpTbl[no-1])();
    }
  }

  addr -> onsei_status  = error_code;
  addr -> onsei_wsno    = ws_no;
  addr -> onsei_acctype = stc_accent_raw[0][0];
  return error_code;
}

/***************************** END OF ILE_KKHC.C *****************************/
