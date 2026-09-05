/********************************************
	ILE_STCH.H	updated at 16th June 1988
	スタック定義
 ********************************************/

/* 辞書引きスタック関連変数 */

EXT BYTE		stc_kind[MAX_STACK];			/* 要素の種類 (=stz_kind) */
EXT integer_16	stc_in_ptr[MAX_STACK];			/* 入力データ位置 */
EXT BYTE		stc_in_len[MAX_STACK];			/* 入力データ文字数 */
EXT integer_16	stc_out_ptr[MAX_STACK];			/* 出力データ位置 */
EXT BYTE		stc_out_len[MAX_STACK];			/* 出力データ文字数 */
EXT BYTE		stc_f2[MAX_STACK];				/* 拡張制御情報のコピー */
EXT BYTE		stc_ctl[MAX_STACK];				/* 制御情報 */
EXT BYTE		stc_mru[MAX_STACK];				/* 学習情報 */
EXT WORD		stc_ed_pag[MAX_STACK];			/* 外部辞書ページ番号 */
EXT WORD		stc_ed_ofs[MAX_STACK];			/* 外部辞書ページ内位置 */
	/*
	 *	内部辞書語は、常駐部先頭からのオフセットを格納する。
	 *	外部辞書語は、ページ先頭からのオフセットを格納する。
	 */
EXT BYTE		stc_stz[MAX_STACK];				/* 接続状態 */
EXT BYTE		stc_nbno[MAX_STACK];			/* 後端番号数 */
EXT BYTE		stc_bno[MAX_STACK][MAX_GRCD];	/* 後端番号 */
EXT BYTE		stc_ngrcd[MAX_STACK];			/* 外部辞書文法コード数 */
EXT BYTE		stc_grcd[MAX_STACK][MAX_GRCD];	/* 外部辞書文法コード */
EXT BYTE		stc_dman[MAX_STACK];			/* 数字仮数部・符号・小数点・助数詞 */
EXT BYTE		stc_dexp[MAX_STACK];			/* 数字指数部 */

EXT BYTE		stc_accent_raw[MAX_STACK][10];	/* アクセント生データ */
EXT BYTE		stc_pause[MAX_STACK];			/* ポーズ量 */

EXT	integer_16	stc_ptr;						/* スタック注目位置 */

/***************************** END OF ILE_STCH.H *****************************/
