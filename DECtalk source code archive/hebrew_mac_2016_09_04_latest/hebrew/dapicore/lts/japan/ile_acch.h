/***********************************************
	ILE_ACCH.H	updated at 24th October 1988
	アクセント結合処理用データ定義
 ***********************************************/

#ifndef EXT 
#define EXT extern
#endif

EXT BYTE		phrase_kind[MAX_STACK];		/* phrase grammer kind */
EXT integer_16	phrase_ptr [MAX_STACK];		/* phrase start pointer */
EXT integer_16	phrase_len [MAX_STACK];		/* phrase length */

/* stc_accent_raw[][0..3]はacc2000()で壊されるのでワーク領域にコピーして使う。*/
EXT BYTE		acc_accent_raw[MAX_STACK][4];

/* phrase_kind[]はacc2000()で壊されるのでワーク領域にコピーしておく。*/
/* このワーク領域はとりあえず PAUSE1000() でのみ使われる。*/
EXT BYTE		phrase_kind_sav[MAX_STACK];

#define MAX_PHRASE	30		/* 一息で話し続ける最長のモーラ数 */
#define MAX_TONE	30		/* 同じ高さの音が続く最長のモーラ数 */

/***************************** END OF ILE_ACCH.H *****************************/
