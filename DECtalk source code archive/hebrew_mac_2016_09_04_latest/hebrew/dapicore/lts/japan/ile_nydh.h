/*********************************************
	ILE_NYDH.H	updated at 30th April 1990
	入力データ等
 *********************************************/

/* definitions for ( inert_kind[] ) */
/* 0x00は使ってはならない */

#define	CHRKIND_Kanji	0x10		/* 漢字 */
#define	CHRKIND_HKana	0x20		/* 平仮名 */
#define	CHRKIND_KKana	0x30		/* 片仮名 */

#define CHRKIND_KDigit1	0x41		/* 漢数字1 : 一二三四五六七八九〇壱弐参 */
#define CHRKIND_KDigit2	0x42		/* 漢数字2 : 十百千拾 */
#define CHRKIND_KDigit3	0x43		/* 漢数字3 : 幾数何 */
#define CHRKIND_KDigit4	0x44		/* 漢数字4 : 万億兆京萬 */
#define	CHRKIND_ADigit	0x45		/* 算用数字 : １２３４５６７８９０ */
#define	CHRKIND_DSign	0x46		/* 符号 : −＋± */
#define	CHRKIND_DSep	0x47		/* 数字の区切り記号 : ，*/
#define	CHRKIND_DDP		0x48		/* DECIMAL POINT : ．*/
#define CHRKIND_Digit	(CHRKIND_ADigit & 0xF0)

#define	CHRKIND_Alpha	0x50		/* 英字・露字・希字 */

#define CHRKIND_Other1	0x60		/* 先頭可能記号 */
#define CHRKIND_Other2	0x70		/* 先頭不可記号 */

#define		CHRKIND_BOpen	0x01	/* 開き括弧類 : （｛“ */
#define		CHRKIND_BClose	0x02	/* 閉じ括弧類 :  ）｝”*/
#define		CHRKIND_Other	0x03	/* その他の特殊文字 : −(hyphen)・ */

/* definitions for ( inert_attr[] ) */

#define	CHRATTR_Soku	0x01 		/* 促音・撥音 */
#define	CHRATTR_Hido	0x02		/* 拗音・非独立文字(ぁぃぅぇぉー) */


/* 現在注目文字の字種(k=jsu)が属性zを持つかどうかを調べる */
#define	NydfMjJsu(z,d)	(inert_kind[ceptr+d]==z)

EXT	integer_16	n_inert;				/* 入力データ長 */
EXT	BYTE		inert   [2*MAX_INERT];	/* 入力文       　（原文） */
EXT	BYTE		inert_kind[MAX_INERT];	/* 入力文文字種　 （原文） */
EXT BYTE		inert_attr[MAX_INERT];	/* 入力文文字属性 （標準形） */

EXT	integer_16	ceptr,ceptr_sav;		/* current element pointer */
EXT	integer_16	celen,celen_sav;		/* current element length */

EXT integer_16	max_ceptr;				/* 無理せず変換できた最長文字列位置 */

EXT BYTE		inert_kana[MAX_EDIC*2+1];
EXT BYTE		inert_pron[MAX_EDIC+1];

/***************************** END OF ILE_NYDH.H *****************************/
