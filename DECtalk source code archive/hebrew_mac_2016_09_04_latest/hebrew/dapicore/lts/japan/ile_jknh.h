/************************************************
	ILE_JKNH.H	updated at 20th February 1991
	辞書検索表
 ************************************************/

/* 辞書検索表 dictionary using table */
EXT	BYTE		dut_value[MAX_INERT*MAX_EDIC];	/* 検索情報 */
EXT	WORD		dut_edofs[MAX_INERT*MAX_EDIC];	/* 外部辞書ページ内オフセット */
EXT	BYTE		*dut_idaddr[MAX_INERT*MAX_EDIC];/* 内部辞書内アドレス */

/* 辞書検索表レコード定義 */
#define		BITDUT_ED_NONE	0x80	/* 外部辞書該当項目なし */
#define		BITDUT_ED_ABORT	0x40	/* 外部辞書放棄フラグ検出 */
#define		BITDUT_ED_USE	0x20	/* 外部辞書変換時使用 */
#define		BITDUT_ED_EXIST	0x10	/* 外部辞書該当項目あり */
#define		BITDUT_ID_NONE	0x08	/* 内部辞書該当項目なし */
#define		BITDUT_ID_ABORT	0x04	/* 内部辞書放棄フラグ検出 */
#define		BITDUT_ID_USE	0x02	/* 内部辞書変換時使用 */
#define		BITDUT_ID_EXIST	0x01	/* 内部辞書該当項目あり */

/*========== dut_ptr ==========
  機能：　辞書検索表のオフセットを求める。
*/
#define		dut_ptr					(ceptr*MAX_EDIC+celen)

/*========== JknfAttrAri ==========
  構文：　JknfAttrAri(shu)
  機能：　辞書検索表にshuで指定した属性があるかどうかを調べる
  戻り値：　属性がある場合 非ゼロ
*/
#define	JknfAttrAri(shu)			(dut_value[dut_ptr] & shu)

/*========== JknfAttrAdd ==========
  構文：  JknfAttrAdd(shu)
  機能：　辞書検索表にshuで指定した属性を加える
*/
#define	JknfAttrAdd(shu)			(dut_value[dut_ptr] |= shu)
#define	JknfAttrAdd3(shu,ptr,len)	(dut_value[ptr*MAX_EDIC+len] |= shu)

/*========== JknfAttrToru ==========
  構文：  JknfAttrToru(shu)
  機能：　辞書検索表からshuで指定した属性を取る。
*/
#define	JknfAttrToru(shu)			(dut_value[dut_ptr] &= ~shu)

/*========== JknfEdicShimau ==========
  構文：　JknfEdicShimau(bp)
  入力：　bp	外部辞書ページ内オフセット
  機能：　外部辞書のページ内オフセットを辞書検索表にしまう。
*/
#define	JknfEdicShimau(bp)			(dut_edofs[dut_ptr]=bp)

/*========== JknfEdicToridasu ==========
  構文：　JknfEdicToridasu
  機能：　外部辞書のページ内オフセットを辞書検索表から取り出す。
*/
#define	JknfEdicToridasu			(dut_edofs[dut_ptr])

/*========== JknfIdicShimau ==========
  構文：　JknfIdicShimau(bp)
  入力：　bp	内部辞書内アドレス
  機能：　内部辞書内アドレスを辞書検索表にしまう。
*/
#define	JknfIdicShimau(bp)			(dut_idaddr[dut_ptr]=bp)

/*========== JknfIdicToridasu ==========
  構文：　JknfIdicToridasu
  機能：　内部辞書のページ内オフセットを辞書検索表から取り出す。
*/
#define	JknfIdicToridasu			(dut_idaddr[dut_ptr])

/*
 *	ここから後は、現在ダミーになっている。
 *	虫があるらしいが、よくわからない。
 *	方針が決まれば復活する予定である。
 *	define O_NULL
 */

#if defined(O_NULL)
/* 入力文字検索表 */
#define MAX_NULL 100
EXT WORD		null_no;
EXT integer_16	null_ptr[MAX_NULL];
EXT WORD		null_ofs[MAX_NULL];
EXT WORD		null_page[MAX_NULL];
#endif

/***************************** END OF ILE_JKNH.H *****************************/
