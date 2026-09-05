/********************************************
	ILE_FDDH.H	updated at 16th July 1991
	辞書ヘッダーデータ定義
 ********************************************/

/* ＦＤ上初期ロードデータ関連定数 */
#define	MAX_FDDBLK		0x12	/* 初期ロードデータ最大ブロック数 */
#define	Fdd_BlkSize		1024	/* 初期ロードデータブロック長 */ 
#define	Fdd_PartirSize	193		/* 先頭データのサイズ */

/* ヘッダーレコード定義 */
typedef struct {
	BYTE	Fdd_DVer[4];		/* +01(  1) 辞書バージョン番号 */
	BYTE	Fdd_UserID[22];		/* +05(  5) 顧客名 */
	BYTE	Fdd_DDate[6];		/* +1B( 27) 作成日付 */

	WORD	Fdd_Fil33;			/* +21( 33) （空き） */
	WORD	Fdd_ChrAttrOfs;		/* +23( 35) 文字コード表のoffset */

	WORD	Fdd_Fil37;			/* +25( 37) （空き） */
	WORD	Fdd_GrcdTblOfs;		/* +27( 39) 辞書文法コード表のoffset */

	WORD	Fdd_Fil41;			/* +29( 41) （空き） */
	WORD	Fdd_IdicIndOfs;		/* +2B( 43) 文法書文字数別pointer offset */

	WORD	Fdd_Fil45;			/* +2D( 45) （空き） */
	WORD	Fdd_IdicPronOfs;	/* +2F( 47) 文法書発音情報offset */
	
	WORD	Fdd_Fil49;			/* +31( 49) （空き） */
	WORD	Fdd_VecpOfs;		/* +33( 51) 前方参照vector表offset */

	WORD	Fdd_Fil53;			/* +35( 53) （空き） */
	WORD	Fdd_EdicIndOfs;		/* +37( 55) 辞書インデックスのoffset */

	WORD	Fdd_Fil57;			/* +39( 57) （空き） */
	WORD	Fdd_TdicIndOfs;		/* +3B( 59) 単漢字辞書見出しoffset */

	WORD	Fdd_Fil61;			/* +3D( 61) （空き） */
 	WORD	Fdd_IdicOfs;		/* +3F( 63) 内部辞書offset */

	WORD	Fdd_HdrSector;		/* +41( 65) ＦＤ上ヘッダセクター数 */
	WORD	Fdd_HdrNg;			/* +43( 67) ＦＤ上ヘッダ実際長 */

	WORD	Fdd_MaxNjsLen;		/* +45( 69) 文法書見出しの最大長さ */
	WORD	Fdd_MaxGjsInd;		/* +47( 71) 辞書インデックスの長さ */

	WORD	Fdd_Fil73;			/* +49( 73) （空き） */
	WORD	Fdd_Fil75;			/* +4B( 75) （空き） */

	WORD	Fdd_Fil77;			/* +4D( 77) （空き） */
	WORD	Fdd_V5youOfs;		/* +4F( 79) 五段動詞連用形語尾音韻表オフセット */

	WORD	Fdd_Fil81;			/* +51( 81) （空き） */
	WORD	Fdd_BnoTblOfs;		/* +53( 83) 後端番号表offset */

	WORD	Fdd_Fil85;			/* +55( 85) （空き） */
	WORD	Fdd_MatrixOfs;		/* +57( 87) 文節メイトリクス表offset */

	WORD	Fdd_Fil89;			/* +59( 89) （空き） */
	WORD	Fdd_EdicKeyOfs;		/* +5A( 91) 辞書indexのkeyのoffset */

	WORD	Fdd_Fil93;			/* +5C( 93) （空き） */
	WORD	Fdd_EdicIndSector;	/* +5E( 95) 辞書indexのsector数 */

	BYTE	Fdd_Fil97[8];		/* +60( 97..104) （空き） */

	WORD	Fdd_IdicPageOfs;	/* +69(105) 文法書ページオフセット */
	WORD	Fdd_IdicPageSize;	/* +6B(107) ページサイズ(単位はセクタ1024バイト) */
	WORD	Fdd_IdicNoPage;		/* +6D(109) 総ページ数(見出し数) */
	WORD	Fdd_Fil111;			/* +6F(111) （空き） */

	WORD	Fdd_TdicPageOfs;	/* +71(113) 単漢字辞書ページオフセット */
	WORD	Fdd_TdicPageSize;	/* +73(115) ページサイズ(単位はセクタ1024バイト) */
	WORD	Fdd_TdicNoPage;		/* +75(117) 総ページ数(見出し数) */
	WORD	Fdd_Fil119;			/* +77(119) （空き） */

	WORD	Fdd_EdicPageOfs;	/* +79(121) 外部辞書ページオフセット */
	WORD	Fdd_EdicPageSize;	/* +7B(123) ページサイズ(単位はセクタ1024バイト) */
	WORD	Fdd_EdicNoPage;		/* +7D(125) 総ページ数(見出し数) */
	WORD	Fdd_Fil127;			/* +7F(127) （空き） */

	BYTE	Fdd_BnoPrefo;		/* +81(129) 丁寧「お」の後端番号 */
	BYTE	Fdd_Bno5danLL;		/* +82(130) 五段動詞の後端番号下限 */
	BYTE	Fdd_Bno5danUL;		/* +83(131) 五段動詞の後端番号上限 */
	BYTE	Fdd_Bno5danYou;		/* +84(132) 五段動詞連用形の後端番号 */
	BYTE	Fdd_Bno1danMei;		/* +85(133) 一段名詞形の後端番号 */
	BYTE	Fdd_Bno1danYou;		/* +86(134) 一段非名詞形の後端番号 */
	BYTE	Fdd_BnoSahen;		/* +87(135) サ変名詞形の後端番号 */
	BYTE	Fdd_BnoSahenYou;	/* +88(136) サ変連用形の後端番号 */
	BYTE	Fdd_BnoHifu;		/* +89(137) 「非不無未」の後端番号 */
	BYTE	Fdd_BnoAdv;			/* +8A(138) 副詞の後端番号 */
	BYTE	Fdd_GrcdToku1;		/* +8B(139) 特殊記号１の文法コード */
	BYTE	Fdd_GrcdToku2;		/* +8C(140) 特殊記号２の文法コード */
	BYTE	Fdd_GrcdDigit;		/* +8D(141) 数字の文法コード */
	BYTE	Fdd_GrcdAdjv;		/* +8E(142) 形容動詞２の文法コード */
	BYTE	Fdd_GrcdGai;		/* +8F(143) 外来語の文法コード */
	BYTE	Fdd_GrcdUsr;		/* +90(144) 利用者登録語の文法コード */
	BYTE	Fdd_GrcdDai;		/* +91(145) 数接頭辞「第」文法コード */
	BYTE	Fdd_GrcdPropLL;		/* +92(146) 固有名詞文法コード下限 */
	BYTE	Fdd_GrcdPropUL;		/* +93(147) 　　〃　　　〃    上限 */
	BYTE	Fdd_PropMeaning;	/* +94(148) 固有名詞の意味分類 */

	BYTE	Fdd_GrcdSufLL;		/* +95(149) 接尾辞の前方接続の文法コード下限 */
	BYTE	Fdd_GrcdSufUL;		/* +96(150) 　　〃　　　　　　　　　〃  上限 */

	BYTE	Fdd_Fil149[41];		/* +97(151)..43BYTES 未使用 */
	BYTE	Fdd_GrcdNum1;		/* +C0(192) 助数詞1の文法コード */
	BYTE	Fdd_GrcdNum2;		/* +C1(193) 助数詞2の文法コード */

	BYTE	Fdd_Fil194[Fdd_BlkSize-Fdd_PartirSize];
								/* +C2(194) 辞書ヘッダー（残り）*/
	BYTE	Fdd_Body[Fdd_BlkSize*(MAX_FDDBLK-1)];
								/*+401(1025) 辞書本体 */
} DIC_STRUCT;

EXT	DIC_STRUCT Fdd_Header;
EXT	LPBYTE Fdd_EdicIndex;

#define	FddHDR	((BYTE *)&Fdd_Header)
#define	FddBODY	((BYTE *)Fdd_Header.Fdd_Body)

/*
 *	後端番号表（格決定用テーブルオフセット）の値
 *
 *	２ビット目：前に対する影響がＡのときに用いる。
 *	３ビット目：「わ」「ね」などの助詞は文末ではアクセントレベルが９になる。
 *	４ビット目：尾高型の連用終止形はアクセント核が１拍前に移動する。
 *	５ビット目：平板型の名詞に付いたとき特殊なアクセントになる付属語がある。
 *	６..８ビット目：ポーズ量の決定に用いる。
 */
#define	BITBNO_1			0x80
#define	BITBNO_2			0x40		/* 前に対する影響がＡのときに用いる */
#define	BITBNO_Level9		0x20		/* 高さが９の助詞 */
#define	BITBNO_Renyou		0x10		/* 連用形 */
#define	BITBNO_Taigen		0x08		/* 体言形 */
#define	BITBNO_Shushi		0x04		/* 終止形 */
#define	BITBNO_RentaiKaku	0x02		/* 連体修飾格 */
#define	BITBNO_RenyouKaku	0x01		/* 連用修飾格 */

/***************************** END OF ILE_FDDH.H *****************************/
