/********************************************
	ILE_EDIC.H	updated at 16th June 1988
	外部辞書に関するデータ
 ********************************************/

#define	MaskEdicGrcd		0x7F	/* 文法コード */

/* 外部辞書ブロック番号／ブロック内位置 */
EXT	WORD		edic_page_no;			/* ページ番号 */
EXT	WORD		edic_ofs;				/* ページ内オフセット */

/* CS1000()に於ける直前が数字のときの処理の為のｓａｖｅ領域 */
EXT	WORD		edic_page_no_sav;
EXT	WORD		edic_ofs_sav;

/* 外部辞書注目ページ開始アドレス */
EXT	LPBYTE		edic_page_addr;

/* 探索語と外部辞書の見出し語との一致した長さ（辞書登録で使う）*/
EXT integer_16	ed_nsame;

/* 現在注目語（フラグ１またはフラグ２から発音表記まで）とその長さ */
EXT	integer_16	ed_ndata;
EXT	BYTE		ed_data[50];

/***************************** END OF ILE_EDIC.H *****************************/
