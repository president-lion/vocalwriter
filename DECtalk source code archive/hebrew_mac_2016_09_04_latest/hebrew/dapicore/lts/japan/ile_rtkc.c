/*******************************************
	ILE_RTKC.C	updated at 19th May 1989
	環境函数
 *******************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_kkhh.h"
#include	"ile_func.h"

/* == RtkfChudanShiraberu ==
  構文：　RtkfChudanShiraberu
  機能：　利用者定義中断要求判定ルーチンをコールする。
*/
int
RtkfChudanShiraberu()
{
    if((*user_stop)(0)) {
        ERROR5000();
        kkhstat=shoki;
        jp_ERROR(Err_Chuudan);
    }
}

/* == RtkfAlloc ==
  機能：　利用者定義領域確保ルーチンをコールする。
*/
int RtkfAlloc(void** addr, size_t size)
{
    integer_16 status;
    status = (*user_alloc)(addr,size);
    return status == 0? 1: 0;
}

/* == RtkfDealloc ==
   free memory allocated by RtkfAlloc
*/
void RtkfDealloc(void** addr)
{
    (*user_dealloc)(addr);
}

/* == RtkfRead ==
  機能：　利用者定義読み込みルーチンをコールする。
*/
int RtkfRead(integer_16 pno, size_t size, void* addr)
{
    integer_16 status;
    if(status = (*user_read)(pno,size,addr)) {
        jp_ERROR(status);
    }
}

/* == RtkfWrite ==
  機能：　利用者定義書き込みルーチンをコールする。
*/
int RtkfWrite(integer_16 pno, size_t size, void* addr)
{
    integer_16 status;
    if(status = (*user_write)(pno,size,addr)) 
	{
        jp_ERROR(status);
    }
}

/***************************** END OF ILE_RTKC.C *****************************/
