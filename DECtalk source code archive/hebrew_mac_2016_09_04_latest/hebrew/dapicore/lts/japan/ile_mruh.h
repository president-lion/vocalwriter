/***********************************************
	ILE_MRUH.H	updated at 8th November 1988
	学習に関するデータ
 ***********************************************/

/* 学習情報レコード定義 */
#define	BITMRU_DEL		0x80	/* 外部辞書削除フラグ（外部辞書）*/
#define	BITMRU_ABORT	0x40	/* 初回放棄フラグ（外部辞書・内部辞書）*/
#define	BITMRU_ACOFF	0x20	/* アクセント結合放棄フラグ（外部辞書・内部辞書）*/
#define	BITMRU_ACON		0x10	/* アクセント強制結合フラグ（外部辞書・内部辞書）*/
#define	BITMRU_PINS		0x08	/* ポーズ強制挿入フラグ（外部辞書・内部辞書）*/
#define	BITMRU_PDEL		0x04	/* ポーズ強制削除フラグ（外部辞書・内部辞書）*/

/*
 *	MRU_OR_NOT : ＭＲＵ実行フラグ
 *		次候補・長方向・短方向処理がなされずに初回の結果のまま
 *		確定されたときには学習の必要はない
 */
EXT integer_16	MRU_OR_NOT;		/* １のときＭＲＵ実行する。０のときしない。*/

#if !defined(O_TALKONLY)
/* 並べ替えによる学習が行われた語を覚えるテーブル */
EXT	WORD		EdicMruDone_LBN[MAX_STACK+1];	/* 論理ブロック番号 */
EXT	WORD		EdicMruDone_OFS[MAX_STACK+1];	/* ブロック内位置 */
#endif

/***************************** END OF ILE_MRUH.H *****************************/
