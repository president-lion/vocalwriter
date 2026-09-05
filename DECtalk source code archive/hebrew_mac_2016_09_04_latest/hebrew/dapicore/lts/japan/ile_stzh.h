/*******************************************
	ILE_STZH.H	updated at 22th May 1989
	要素間の接続に関するデータ
 *******************************************/

/* 要素間の接続関係関連情報の構造体の定義 */

typedef	struct {
	BYTE stz_kind;			/* 単語種別 */
		/*（外部辞書／内部辞書／五段動詞連用形語尾／片仮名／数詞／特殊文字）*/
	BYTE stz_ctl;			/* 制御情報の論理和 */
	BYTE stz_f2;			/* フラグ２ */
	BYTE stz_stz;			/* 接続状態 */
	BYTE stz_ngrcd;			/* 文法コード数＝前方ベクトル数 */
	BYTE stz_nbno;			/* 後端番号数 */
	BYTE stz_grcd[MAX_GRCD];/* 文法コード(外部辞書語以外は意味を持たない) */
	BYTE stz_bno[MAX_GRCD];	/* 後端番号 */
	BYTE stz_pno[MAX_GRCD];	/* 前方ベクトル番号 */
	BYTE stz_mru;			/* 学習情報 */
	BYTE stz_dman;			/* mantissa */
	BYTE stz_dexp;			/* exponent */
		/*
		 *	MAN, EXPの意味
		 *	その数字の仮数部と指数部を表わす。
		 *	一千と千を区別する為に、一千は、MAN=1、千は、MAN=0とする。
		 *	十・百についても、都合で、MAN=0とする。
		 *	「数」・「何」・「幾」の仮数部はそれぞれ、10、11、12で表わす。
		 *	また、符号、小数点は、仮数部をそれぞれ65、66とし、指数部を0とする。
		 */
		/*
		 *	単漢字の場合は、
		 *	dmanには種別フラグを、dexpにはトータルレコードサイズをセットする。
		 *	トータルレコードサイズは残っている候補のレコード長を合計したものである。
		 */
} STZ_STRUCT;

/* 数字情報関連情報 */
#define DMANKIND_PL			 64		/* 複数 */
#define DMANKIND_SIGN		 65		/* sign */
#define DMANKIND_DP			 66		/* decimal point */
#define DMANKIND_NUM		129		/* numerator */

/* 単語種別関連情報 stc_kind[] */
#define	DICKIND_E		0x80		/* EXTERNAL DICTIONARY */
#define	DICKIND_I		0x40		/* INTERNAL DICTIONARY */
#define	DICKIND_D		0x20		/* NUMERALS 数字 */
#define	DICKIND_5Y		0x10		/* 五段動詞連用形活用語尾 */
#define	DICKIND_K		0x08		/* 片仮名語 */
#define	DICKIND_T		0x04		/* 単漢字辞書 */

/* 単語種別関連情報 stc_ed_ofs[] */
#define DKIND_INT		0x01		/* integer type */
#define DKIND_FRAC		0x02		/* fraction type */

/* 接続状態レコード定義 */
#define	BITSTZ_PREFO5V	0x80		/* おの次の語で５段動詞 */
									/* 連用形がサ変動詞名詞型となる */
#define	BITSTZ_KANGO	0x40		/* 漢語 */
#define BITSTZ_PREF		0x20		/* 接頭辞に続く語である：文節の切れ目になりえない */

/* 制御情報レコード定義 */
#define	BITCTL_PREF		0x80	/* prefix */
#define	BITCTL_SUF		0x40	/* suffix */
#define	BITCTL_PKANGO	0x20	/* 先行語漢語接続あり preceding kango */
#define	BITCTL_FKANGO	0x10	/* 後続語漢語接続あり following kango */
#define	BITCTL_FIRST	0x08	/* 最初になり得る */
#define	BITCTL_LAST		0x04	/* 最後になり得る */
#define	BITCTL_PEDIC	0x02	/* 先行語が外部辞書にありうる preceding EDIC */
#define	BITCTL_FEDIC	0x01	/* 後続語が外部辞書にありうる following EDIC */

/* フラグ２レコード定義 */
#define BITFLG2_AUX		0x10
				/* 助動詞の「ない」、「たい」*/
				/* アクセントの滝を１拍前に移動する性質の要素が続いても、
				　これらの要素の場合は滝が移動しない。
				　本来、文節の区切りになるような要素であったが、
				　助動詞的に使われるようになってしまったもの。*/
#define	BITFLG2_KANGO	0x08
				/* 漢語 */
#define	BITFLG2_SUF		0x04
				/* 漢語接尾辞 */
				/* 「東京どまり」、「課長どまり」の「どまり」 */
#define	BITFLG2_NADV	0x02
				/* 後続語副詞付加 next adverb */
				/* 「〜につき」の「つ」：「き」に副詞の後端番号を付加する */
#define	BITFLG2_SELF	0x01
				/* 「あなた自身」、「それ自身」の「自身」 */
				/* この語の制御情報、後端番号はひとつ前の語と同じとする */

/* 文法コードｖｓ後端番号表 */
#define GRCDTBL_CTL		0		/* 制御情報 */
#define GRCDTBL_BNO		1		/* 後端番号 */
#define GRCDTBL_PNO		2		/* 前方ベクトル番号 */
#define GRCDTBL_NULL	3		/* 未使用 */


/*========== ce_is_kango ==========================================
	構文：ce_is_kango, pe_is_kango
	機能：先行語(PE)／現在注目語(CE)が漢語かどうかを調べる
  =================================================StzfXxgKangGzg==*/
#define	ce_is_kango \
	((Stz_CE.stz_f2 & BITFLG2_KANGO)||(Stz_CE.stz_stz & BITSTZ_KANGO))

#define	pe_is_kango \
	((Stz_PE.stz_f2 & BITFLG2_KANGO)||(Stz_PE.stz_stz & BITSTZ_KANGO))


/*========== StzfAttrAri =====================================================
	構文：StzfAttrAri(go,target,Attr)
	機能：先行要素(go=PE)／注目要素(go=CE)の制御情報／フラグ２／学習情報が
		　Attrで指定した属性を含むかどうかをしらべる。
  ============================================================================*/
#define	StzfAttrAri(go,target,Attr)		(go.target & Attr)
#define	StzfAttrNashi(go,target,Attr)	((go.target & Attr)==0)


/*========== StzfAttrAdd =====================================================
	構文：StzfAttrAdd(go,target,Attr)
	機能：先行要素(go=PE)／現在注目要素(go=CE)の制御情報／フラグ２／学習情報に
		　Attrで指定した属性を加える。
  ============================================================================*/
#define	StzfAttrAdd(go,target,Attr)		(go.target|=Attr)


/* 要素間の接続関係処理の構造体定義 */
EXT	STZ_STRUCT		Stz_PE;		/* 先行要素 接続関係 付随情報 */
EXT	STZ_STRUCT		Stz_CE;		/* 現在要素 接続関係 付随情報 */

/***************************** END OF ILE_STZH.H *****************************/
