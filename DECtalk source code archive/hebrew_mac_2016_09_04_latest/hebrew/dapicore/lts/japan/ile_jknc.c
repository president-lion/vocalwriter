/**********************************************
	ILE_JKNC.C	updated at 8th January 1991
	辞書検索表
 **********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_bunh.h"
#include	"ile_edic.h"
#include	"ile_idic.h"
#include	"ile_jknh.h"
#include	"ile_kkhh.h"
#include	"ile_nydh.h"
#include	"ile_func.h"

/*========== JKN_INIT() ==============
	機能：辞書検索表を初期状態にする
  ===================JknfShokikasuru==*/
void JKN_INIT() { register integer_16 cx;
 	for(cx=MAX_INERT*MAX_EDIC;cx--;){
		dut_value[cx]=dut_edofs[cx]=0;
		dut_idaddr[cx]=(BYTE *)0;
	}
}

/*
 *	ここから後は、現在ダミーになっている。
 *	虫があるらしいが、よくわからない。
 *	方針が決まれば復活する予定である。
 *	define O_NULL
 */

clear_null() {
#if defined(O_NULL)
	null_no=0;
#endif
}

push_null(ptr,ofs,page) WORD ptr,ofs,page; {
#if defined(O_NULL)
	register WORD *pp,*op;
	WORD *bp,cx;

	if(null_no<MAX_NULL){
		/*
		 * 次候補・長方向・短方向のときは、
		 * 注目文節内の要素は登録しないこととする。
		 */
		if(conv_mode==CONV_NEXT||conv_mode==CONV_LONG||conv_mode==CONV_SHORT){
			if(ceptr<=fsptr){ return; }
		}

		pp=null_ptr; op=null_ofs; bp=null_page;
		for(cx=null_no ;cx--; ++pp,++op,++bp){
			if(ptr==(*pp) && ofs==(*op) && page==(*bp)){
				return;
			}
		}

		null_ptr[null_no]=ptr;
		null_ofs[null_no]=ofs;
		null_page[null_no]=page;
		++null_no;
		/*printf("%d : %d, %x\n",null_no,ptr,ofs);*/
	}
#endif
}

check_edic_null() {
#if defined(O_NULL)
	register WORD *pp,*op;
	WORD *bp,cx;

	pp=null_ptr; op=null_ofs; bp=null_page;
	for(cx=null_no ;cx--; ++pp,++op,++bp){
		if(ceptr==(*pp) && edic_ofs==(*op) && edic_page_no==(*bp)){
			/*printf(" ignore.");*/
			return 1;
		}
	}
#endif
	return 0;
}

check_idic_null() {
#if defined(O_NULL)
	register integer_16 *pp;
	register WORD *op;
	integer_16 cx;

	pp=null_ptr; op=null_ofs;
	for(cx=null_no ;cx--; ++pp,++op){
		if(ceptr==(*pp) && idic_addr==(BYTE *)(*op)){
			/*printf("\nignore.");*/
			return 1;
		}
	}
#endif
	return 0;
}

/***************************** END OF ILE_JKNC.C *****************************/
