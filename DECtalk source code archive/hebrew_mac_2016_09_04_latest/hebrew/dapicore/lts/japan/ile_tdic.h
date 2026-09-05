/********************************************
	ILE_TDIC.H	updated at 16th June 1988
	単漢字辞書に関するデータ
 ********************************************/

#define	BITTAN_KOUSEI	0x10	/* 校正システム用項目 */
#define	BITTAN_ON		0x02	/* 音読み用項目 */
#define	BITTAN_KUN		0x01	/* 訓読み用項目 */

EXT WORD		tdic_ofs;			/* ページ内オフセット */
EXT integer_16	tdic_len;			/* 単漢字レコードトータルサイズ */

/***************************** END OF ILE_TDIC.H *****************************/
