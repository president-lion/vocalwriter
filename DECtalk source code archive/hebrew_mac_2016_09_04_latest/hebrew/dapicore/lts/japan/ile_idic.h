/************************************************
	ILE_IDIC.H	updated at 20th February 1991
	内部辞書に関するデータ
 ************************************************/

/* 内部辞書文字数別ポインタ */
typedef	struct {
	/*WORD		idic_len_ptr;*/
	BYTE		idic_len_ptr2;
	BYTE		idic_len_ptr1;
	WORD		idic_index_dummy;
} IDIC_INDICES;

/* 内部辞書固定情報構造定義 */
typedef	struct {
	BYTE		idic_ctl;		/* 制御情報 */
	BYTE		idic_f2;		/* flag 2 */
	BYTE		idic_bno;		/* 後端番号 */
	BYTE		idic_pno;		/* 前方参照ベクトル番号：offsetは20倍する */
	/*WORD		idic_acc_ofs;*/	/* accent & pronunciation data offset */
	BYTE		idic_acc_ofs2;	/* accent & pronunciation data offset */
	BYTE		idic_acc_ofs1;	/* accent & pronunciation data offset */
	BYTE		idic_mru;		/* 学習情報 */
} IDIC_DATA;

#define	SIZE_IDIC	7		/* sizeof(IDIC_DATA)=8 !!! */

/* 内部辞書項目の制御情報のアドレス */
EXT	BYTE		*idic_addr;

/* CS1000()に於ける直前が数字のときの処理の為のｓａｖｅ領域 */
EXT	BYTE		*idic_addr_sav;

/***************************** END OF ILE_IDIC.H *****************************/
