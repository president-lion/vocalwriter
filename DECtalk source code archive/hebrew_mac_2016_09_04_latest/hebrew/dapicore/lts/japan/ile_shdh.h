/*******************************************
	ILE_SHDH.H	updated at 19th May 1989
	出力データ等
 *******************************************/

/* アクセント情報マスクパターン */
#define	BITACC_MUSEI	0x80
#define	BITACC_FIRST	0x40
#define	BITACC_LEVEL	0x0F

/* 出力データ管理情報 */
EXT	integer_16		cpron_ptr;					/* 出力データ注目位置 */
EXT	integer_16		cpron_len;					/* 現在語変換結果長さ */

/* 出力データ文節管理情報 */
EXT	integer_16		sent_ptr;					/* 注目文節位置 */

/* 出力データ中間エリア */
EXT	integer_16		sent_in_ptr[MAX_SENT];		/* 入力データ位置 */
EXT	integer_16		sent_out_ptr[MAX_SENT];		/* 出力データ位置 */
EXT BYTE			omega[MAX_OMEGA];			/* pronunciation data */
EXT BYTE			speak[MAX_SPEAK];			/* 表音 data */

EXT integer_16		kkana_len;					/* 片仮名処理用領域 */
EXT BYTE			kkana_buf[MAX_OMEGA];

EXT BYTE			kansuji[MAX_INERT*2+1];		/* 漢数字領域 */
EXT integer_16		n_kansuji;
EXT integer_16		digit_mode;					/* 数字変換モード */
												/* =0 : integer type */
												/* =1 : fraction type */
EXT integer_16		digit_type;					/* 数字タイプ */
												/* =0 : normal type */
												/* =1 : 棒読み type */

EXT BYTE			accent0[MAX_OMEGA];			/* accent data */


/* 出力データが、ひとつ前の結果と等しいかどうか調べるための保存領域 */
EXT BYTE			omega_sav[MAX_OMEGA];		/* pronunciation data */
EXT BYTE			accent_sav[MAX_OMEGA];		/* accent data */
EXT	integer_16		sent_in_ptr_sav[MAX_SENT];	/* 入力データ位置 */
EXT	integer_16		sent_out_ptr_sav[MAX_SENT];	/* 出力データ位置 */

/***************************** END OF ILE_SHDH.H *****************************/
