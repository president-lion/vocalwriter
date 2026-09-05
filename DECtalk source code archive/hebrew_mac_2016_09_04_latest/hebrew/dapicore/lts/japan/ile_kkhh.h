/***********************************************
	ILE_KKHH.H	updated	at 3rd February 1988
	解析処理に関するデータ
 ***********************************************/

EXT enum {
		shoki,			/* 初期状態 */
		bunsetsu,       /* 文節設定状態 */
		funou,          /* 変換不能状態 */
		err,            /* エラー発生状態 */
		mru,            /* 学習状態 */
		riyousha        /* 利用者定義語状態 */
} kkhstat;

/* == KkhfMruKanou ==
  構文：　KkhfMruKanou 
  機能：　現在の変換状態が学習可能かどうかをしらべる
*/
#define	KkhfMruKanou	 (kkhstat==bunsetsu || kkhstat==funou)

#define	CONV_FIRST		1
#define	CONV_NEXT		2
#define	CONV_RIGHT		3
#define	CONV_LEFT		4
#define	CONV_SHORT		5
#define	CONV_LONG		6
#define	CONV_RETRY		7	/* 初回変換でエラー３・４のときの途中までの変換 */

EXT	BYTE	conv_mode;		/* 変換種別 */

/***************************** END OF ILE_KKHH.H *****************************/
