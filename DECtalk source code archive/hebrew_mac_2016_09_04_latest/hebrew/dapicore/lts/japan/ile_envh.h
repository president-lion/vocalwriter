/********************************************
	ILE_ENVH.H	updated at	19th May 1989
	環境等
 ********************************************/

EXT STRUCT_ONSEI *onsei_addr;	/* パラメタ構造体のアドレス */

extern int (*user_stop)(void*);			/* 中断判定関数 */
extern int (*user_alloc)(void** addr, size_t size);				/* 領域確保関数 */
extern int (*user_read)(int page, size_t size, void* addr);				/* 物理入力関数 */
extern int (*user_write)(int page, size_t size, void* addr);                         /* 物理出力関数 */
extern void (*user_dealloc)(void** addr);				

//#if defined(unix)
#define MAX_POP_TIMES 499			/* POP制限回数 */
//#endif

/***************************** END OF ILE_ENVH.H *****************************/
