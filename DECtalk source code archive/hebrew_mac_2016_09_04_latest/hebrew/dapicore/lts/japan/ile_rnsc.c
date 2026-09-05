/********************************************
	ILE_RNSC.C	updated at 29th June 1991
	論理入出力
 ********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_rnsc.h"

#define	incptr(ptr)	((ptr==last)? PageTable : ptr+1)

/* ページテーブルエントリー(ページング用バッファは、PTE によって管理される。)*/
typedef struct 
{
	LPBYTE PBA;		/* ページバッファアドレス */
	WORD PI;		/* ページ情報(Valid bit, Modfy bit, Page No) */
} PTE;

/* PIの各内容を取り出すためのマスク */
#define	V	0x8000	/* Valid bit(エントリーが未使用ならば 1) */
#define	M	0x4000	/* Modfy bit(書きこみ要求があれば 1) */
#define	PN	0x3fff	/* Page No(ページ番号 1 - 16,384) */

/* 用意する PTE の数 */
#define	MaxPTE	25

static PTE PageTable[MaxPTE] = {{0,0}};
static PTE* head = NULL;
static PTE* tail = NULL;
static PTE* last = NULL;

static PTE *GetBuffer(void);
static PTE *SearchBuffer(WORD page);

/*========== RNS_INIT() ======================================
	機能：ページバッファを確保し、リングバッファを初期化する。
  ===========================================RnsfShokikasuru==*/
void RNS_INIT(void) 
{
	WORD count;
	PTE* bp;
	LPBYTE addr;
	int allocFailed = 0;
	head=bp=PageTable;
	
	for (count=MaxPTE; count--; ) 
	{
		if(RtkfAlloc(&addr,Fdd_Header.Fdd_EdicPageSize)==0) 
		{
			allocFailed = 1;
			break;
		}
		bp->PI = V;
		bp->PBA = addr;
		++bp;
	}
	
	/* ページが確保できなければエラー。*/
	if(allocFailed)
	{
		PTE* bpp;
		for (bpp=PageTable; bpp->PBA; ++bpp) 
		{
			RtkfDealloc(&(bpp->PBA));
		}
		jp_ERROR(Err_Fatal_Alloc);
	}
	tail=last=(--bp);
}

void RNS_DEALLOC()
{
	int count;
	PTE* bpp;
	for (bpp=PageTable, count = 0; bpp->PBA && count < MaxPTE; ++bpp, ++count) 
	{
		RtkfDealloc(&(bpp->PBA));
	}
}

/*========== RnsfReadEdic(Page,addr) ======================
	機能：ページングを用いて指定されたページへを読み込む。
	入力：Page	ページ番号
	出力：*addr	ページバッファアドレス
  =========================================================*/
void RnsfReadEdic(WORD Page,LPBYTE* addr)
{
	PTE *bp;
	
	if((bp=SearchBuffer(Page))==NULL) 
	{
		bp=GetBuffer();
		RtkfRead(Page,Fdd_Header.Fdd_EdicPageSize,bp->PBA);
		bp->PI=Page;
	}
	*addr = bp->PBA;
}

/*========== RnsfWriteEdic(Page) ============================
	機能：指定されたページのPTEのModfy bitをonにする。
	入力：Page	ページ番号
  ===========================================================*/
void RnsfWriteEdic(WORD Page)
{
	PTE *bp;
	
	/* バッファに書きたいページが無ければエラー。*/
	if((bp=SearchBuffer(Page))==NULL) 
	{
		jp_ERROR(Err_Fatal_Alloc); 
	}
	bp->PI |= M;
}

/*========== RnsfFlushBuffer() ====================
	機能：Modfy bitがonのページを1ページ書き出す。
	出力：＝０：　書き出すべきページがなかった。
		　＝１：　正常終了。
  =================================================*/
BOOL RnsfFlushBuffer(void)
{
	PTE *pp;
	WORD pi;
	
	for(pp=head; ;pp=incptr(pp)) 
	{
		pi=pp->PI;
		if(((pi & V)==0) && (pi & M)) 
		{
			RtkfWrite(pi & PN,Fdd_Header.Fdd_EdicPageSize,pp->PBA);
			pp->PI &= ~M;
			return 1;
		}
		if(pp==tail)
		{ 
			goto RnsfFlushBuffer_end; 
		}
	}
	
RnsfFlushBuffer_end:
	return 0;
}

/*========== SearchBuffer(page) ========================================
	機能：バッファ内に必要なページがあるか PageTable の各PTE を調べる。
	入力：page	ページ番号
	出力：＝０：　該当ページなし
		　≠０：　PTEへのポインタ（０でないようにすること）
  ======================================================================*/
static PTE *SearchBuffer(WORD page)
{
	register PTE *pp;
	register WORD pi;
	
	for(pp=head; ;pp=incptr(pp)) 
	{
		pi=pp->PI;
		if((pi & V)==0) 
		{
			pi &= PN;
			if(pi==page) 
			{
				return pp;
			}
		}
		if(pp==tail)
		{ 
			goto SearchBuffer_end; 
		}
	}
	
SearchBuffer_end:
	return NULL;
}

/*========== GetBuffer() ============================
	機能：ページングのためのバッファ領域を確保する。
	出力：PTEへのポインタ
  ===================================================*/
static PTE *GetBuffer(void)
{
	register PTE *pp;
	register WORD pi;
	
	pp=incptr(tail);
	pi=pp->PI;
	if((pi & V)==0) 
	{
		if(pi & M) 
		{
			RtkfWrite(pi & PN,Fdd_Header.Fdd_EdicPageSize,pp->PBA);
			pp->PI &= ~M;
		}
		head=incptr(head);
	}
	tail=incptr(tail);
	return pp;
}

/***************************** END OF ILE_RNSC.C *****************************/
