/************************************************
	ILE_ENVC.C	updated at 26th September 1991
	変換初期設定・変換初回・変換終了
	変換次候補・変換確定
 ************************************************/

#define		EXT		extern

#include	<stddef.h>

#include	"onsei.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_kkhh.h"
#include	"ile_nydh.h"
#include	"ile_stzh.h"
#include	"ile_func.h"
#include "ile_rnsc.h"

#define	VeraDateL		6
#define	VeraVerL		4
#define	VerwVerL		(VeraVerL / 2)

int (*user_stop)(void*) = NULL;
int (*user_alloc)(void** addr, size_t size) = NULL;
int (*user_read)(int page, size_t size, void* addr) = NULL;
int (*user_write)(int page, size_t size, void* addr) = NULL;
void (*user_dealloc)(void** addr) = NULL;				

/* 文章音声変換プログラム作成日付等 */
static const integer_16	VerwVer[2]={2,10};
static const BYTE VerbDate[]="910821";


/*========== ENV_INIT() =================================
	機能：文章音声変換初期設定（詳細は、外部仕様書参照）
         	ジャンプテーブルで呼ばれる
  ======================================ShkfShokikasuru==*/
void ENV_INIT()
{
	BYTE *si;
	integer_16 cx;
	integer_16 ax;
	LPBYTE shd;
	
	user_stop    = onsei_addr -> onsei_stop;
	user_alloc   = onsei_addr -> onsei_alloc;
	user_dealloc   = onsei_addr -> onsei_dealloc;
	user_read    = onsei_addr -> onsei_read;
	user_write   = onsei_addr -> onsei_write;
	
	RtkfRead(1,1,(LPBYTE)FddHDR);	/* 先頭トラックを読む (バッファはＤＳ) */
	/* 初期ロードデータ最終トラック番号を得る*/
	
#if defined(C_SUN4)||defined(OS9)
	/*printf("size:%d\n",sizeof(DIC_STRUCT));*/
	SwapHeader((LPBYTE)FddHDR);
#endif
	
	cx=Fdd_Header.Fdd_HdrSector-1;
	/* 残りロードデータトラック数を計算する */
	RtkfRead(2,cx,(LPBYTE)FddBODY);	/* 残りの情報を読む (バッファはＤＳ) */
	
	cx=Fdd_Header.Fdd_HdrSector;
	ax=Fdd_Header.Fdd_EdicIndSector;
	
	if (Fdd_EdicIndex != NULL) {
		RtkfDealloc(&Fdd_EdicIndex);
	}
	if(RtkfAlloc(&Fdd_EdicIndex,ax)==0) {
		jp_ERROR(Err_Fatal_Alloc); 
	}
	RtkfRead(cx+1,ax,Fdd_EdicIndex);/* 外部辞書インデックスを読む (バッファはＥＳ) */
	
	Fdd_Header.Fdd_EdicKeyOfs-=Fdd_Header.Fdd_EdicIndOfs;	/* 辞書indexのkeyのＥＳ上のoffset */
	
	if(user_checksum()==0) {		/* patch check */
		jp_ERROR(FAT_Patch);			/* チェックサムが合わないときはエラー */
	}
	
	RNS_INIT();						/* 論理入出力ルーチンを初期化する */
	
	/* 変換プログラムのバージョンと作成日付をセットする */
	shd=onsei_addr->onsei_omega;	/*ShdaChmHaj;*/
	for(cx=VerwVerL,si=(BYTE *)VerwVer;cx--;) {
		*(integer_16 far *)shd=(*(integer_16 *)si); shd+=2; si+=2;
	}
	for(cx=VeraDateL,si=VerbDate;cx--;){
		*shd++=(*si++);
	}
	
	/* 辞書のバージョンと作成日付をセットする */
	for(cx=VeraVerL,si=Fdd_Header.Fdd_DVer;cx--;) {
		*shd++=(*si++); 
	}
	for(cx=VeraDateL,si=Fdd_Header.Fdd_DDate;cx--;) {
		*shd++=(*si++);
	}
	
	kkhstat=shoki;			/* 変換状態を初期状態に設定する */
	PRON_INIT();
	
#if defined(O_DEBUG)
	DEBUG_INIT();			/* debug option setting */
#endif
}


/*========== ENV_TERM() ===========================
	機能：文章音声変換終了（詳細は外部仕様書参照）
  ===============================ShufShuuryousuru==*/
void
ENV_TERM()
{
#if !defined(O_TALKONLY)
  integer_16 si,cx;
  LPBYTE addr;

  /* 学習可能な状態ならば 利用者注目文節までを学習する */
  if(KkhfMruKanou) {
    MRU_CSENT();
  }

  kkhstat=shoki;						/* 初期状態に設定する */
  while(RnsfFlushBuffer());			/* 学習結果をすべて外部辞書に書き出す */

  /********************************************************
   * 文法書の学習結果を辞書に書き出す						*
   *														*
   * 書き出す部分は										*
   *														*
   * 文法書オフセット　　　Fdd_Header.Fdd_IdicOfs			*
   * 文法書開始ページ番号　Fdd_Header.Fdd_IdicPageOfs+1	*
   * ページ数　　　　　　　Fdd_Header.Fdd_IdicNoPage		*
   ********************************************************/
  si=Fdd_Header.Fdd_IdicPageOfs+1;
  cx=Fdd_Header.Fdd_IdicNoPage;
  addr=(LPBYTE)(FddHDR+Fdd_Header.Fdd_IdicOfs);

#if defined(C_SUN4)||defined(OS9)
  SwapHeader((LPBYTE)FddHDR);
#endif

  RtkfWrite(si,cx,addr);
#endif
  if (Fdd_EdicIndex != NULL) {
    RtkfDealloc(&Fdd_EdicIndex);
  }
  RNS_DEALLOC();
}


/*========== PRON_INIT() =================================
	機能：文章音声変換をするための各種エリアを初期化する
  ====================================ShkfHnkShokikasuru==*/
void
PRON_INIT()
{
  PRON_INIT2();
  JKN_INIT();					/* 辞書検索表をクリアーする */
  max_ceptr=0;				/* 最長変換入力文字列長の初期化 */
}

/*========== PRON_INIT2() ==========================================================
	機能：変換失敗のとき、最長変換文字列を変換しなおすための各種エリアを初期化する。
  ==============================================================ShkfHnkShokikasuru==*/
void
PRON_INIT2()
{
  OMEGA_INIT();				/* 出力発音記号領域を初期化する */
  STACK_INIT();				/* 辞書引きスタックを初期化する */
  FOLLOW_INIT(&Stz_CE);		/* 現在注目語の情報を初期化する */
  FOLLOW_INIT(&Stz_PE);		/* 先行語の情報を初期化する */
  SENT_INIT();				/* 文節位置情報を初期化する */
#if !defined(O_TALKONLY)
  EDIC_MRU_INIT(); 			/* 学習状態を初期化する */
#endif
}


/*========== display_user() ====================================
	機能：辞書５バイト目からのユーザー名を出力領域にセットする。
  ==============================================================*/
void
display_user()
{
  register BYTE *si;
  register integer_16 cx;
  BYTE al;
  LPBYTE di;

  si=(&Fdd_Header.Fdd_UserID[0]);
  di=onsei_addr->onsei_omega;
  for(cx=20;cx--;) {
    al=(*si++);
    al=((al & 0x01)<<7) + (al>>1);
    *di++=(al? al:0x20);
  }
  *di=0x00;
}


/*========== user_checksum() =====================
	機能：辞書５バイト目からのユーザー名を調べる。
	値　：＝０：異常
		　＝１：正常
  ================================================*/
BOOL
user_checksum()
{
  register BYTE *si;
  register integer_16 cx;
  BYTE al;
  WORD sum;

  si=(&Fdd_Header.Fdd_UserID[0]);
  for(sum=0,cx=20;cx--;) {
    al=(*si++);
    al=((al & 0x01)<<7) + (al>>1);
    sum+=(WORD)al;
  }
  /* return( sum==(*(WORD *)si) ? 1:0); */
  return( sum== ((*(si+1)<<8) + *si) ? 1:0);
}


#if defined(C_SUN4)||defined(OS9)
/*========== SwapHeader() =============================
	機能：辞書ヘッダの１６ビットデータのバイトスワップ
			SUN4,68000用 8086,DECは不要
  =====================================================*/
void
SwapHeader(DIC_STRUCT* addr)
{
  swapval(&(addr->Fdd_ChrAttrOfs));		/* +23( 35) 文字コード表のoffset */
  swapval(&(addr->Fdd_GrcdTblOfs));		/* +27( 39) 辞書文法コード表のoffset */
  swapval(&(addr->Fdd_IdicIndOfs));		/* +2B( 43) 文法書文字数別pointer offset */
  swapval(&(addr->Fdd_IdicPronOfs));		/* +2F( 47) 文法書発音情報offset */
  swapval(&(addr->Fdd_VecpOfs));			/* +33( 51) 前方参照vector表offset */
  swapval(&(addr->Fdd_EdicIndOfs));		/* +37( 55) 辞書インデックスのoffset */
  swapval(&(addr->Fdd_TdicIndOfs));		/* +3B( 59) 単漢字辞書見出しoffset */
  swapval(&(addr->Fdd_IdicOfs));			/* +3F( 63) 内部辞書offset */
  swapval(&(addr->Fdd_HdrSector));		/* +41( 65) ＦＤ上ヘッダセクター数 */
  swapval(&(addr->Fdd_HdrNg));			/* +43( 67) ＦＤ上ヘッダ実際長 */
  swapval(&(addr->Fdd_MaxNjsLen));		/* +45( 69) 文法書見出しの最大長さ */
  swapval(&(addr->Fdd_MaxGjsInd));		/* +47( 71) 辞書インデックスの長さ */
  swapval(&(addr->Fdd_V5youOfs));			/* +4F( 79) 五段動詞連用形語尾音韻表オフセット */
  swapval(&(addr->Fdd_BnoTblOfs));		/* +53( 83) 格決定用テーブルオフセット */
  swapval(&(addr->Fdd_EdicKeyOfs));		/* +5A(91) 辞書indexのkeyのoffset */
  swapval(&(addr->Fdd_EdicIndSector));	/* +5E(95) 辞書indexのsector数 */
  swapval(&(addr->Fdd_IdicPageOfs));		/* +69(105) 文法書ページオフセット */
  swapval(&(addr->Fdd_IdicPageSize));		/* +6B(107) ページサイズ(単位はセクタ1024バイト) */
  swapval(&(addr->Fdd_IdicNoPage));		/* +6D(109) 総ページ数(見出し数) */
  swapval(&(addr->Fdd_TdicPageOfs));		/* +71(113) 単漢字辞書ページオフセット */
  swapval(&(addr->Fdd_TdicPageSize));		/* +73(115) ページサイズ(単位はセクタ1024バイト) */
  swapval(&(addr->Fdd_TdicNoPage));		/* +75(117) 総ページ数(見出し数) */
  swapval(&(addr->Fdd_EdicPageOfs));	/* +79(121) 外部辞書ページオフセット */
  swapval(&(addr->Fdd_EdicPageSize));	/* +7B(123) ページサイズ(単位はセクタ1024バイト) */
  swapval(&(addr->Fdd_EdicNoPage));		/* +7D(125) 総ページ数(見出し数) */
}

void
swapval(BYTE* val)
{
  BYTE	c;
  c = val[0]; val[0] = val[1]; val[1] = c;
}

void
swapval2(BYTE* src, BYTE* dst)
{
  BYTE	c;
  dst[0] = src[1]; dst[1] = src[0];
}
#endif

/***************************** END OF ILE_ENVC.C *****************************/
