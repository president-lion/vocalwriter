/********************************************
	ILE_V5CC.C	updated at 18th June 1989
	五段動詞連用形
 ********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_fddh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_v5ch.h"
#include	"ile_func.h"

/*========== V5DANYOU_PUSH() ====================================
	機能：五段動詞の省略された連用形活用語尾の情報をスタックする
  ===============================================================*/
void V5DANYOU_PUSH() {
#if defined(O_DEBUG)
	DEBUG_STR("『連用形語尾追加ぢゃ。』\n");
#endif
	/* 連用形語尾の音を出力領域に追加する。*/
	cpron_ptr=V5DAN5000(cpron_ptr); cpron_len=1;

	/* 現在要素は五段動詞連用形活用語尾である。*/
	Stz_CE.stz_kind=DICKIND_5Y;
	Stz_CE.stz_nbno=1;
	Stz_CE.stz_bno[0]=Fdd_Header.Fdd_Bno5danYou;

	/* 自動発生した連用形語尾は、最後になってはならない。*/
	/* 特に、エラーコードが３・４のときに問題になる。
	   もし、max_ceptrが自動発生された連用形語尾の直後にあると再変換できない。*/
	/*Stz_CE.stz_ctl=BITCTL_LAST | BITCTL_PEDIC | BITCTL_FEDIC;*/
	Stz_CE.stz_ctl=BITCTL_PEDIC | BITCTL_FEDIC;

	/* 文節情報の出力データ位置を再設定する */
	/* これは活用語尾であるから、文節先頭にはならない。*/
	sent_out_ptr[sent_ptr]=cpron_ptr;
	STACK_PUSH();
}


/*========== V5DAN5000(ptr) ============================
	機能：「動始める」の省略された「き」の音を追加する
  ======================================================*/
integer_16 V5DAN5000(di) integer_16 di; {
	BYTE al,ah;
	register WORD si;
	register integer_16 cx;

	si=Fdd_Header.Fdd_V5youOfs;
	cx=FddHDR[si++];
	for(;cx--;){
		al=FddHDR[si++];
		ah=al/16; al&=0x0F;
		if(ah<10){ stc_accent_raw[stc_ptr][ah]=al; }
	}

	di=OMEGA1000(FddHDR[si+flg_v5you-Fdd_Header.Fdd_Bno5danLL],di);
	return di;
}

/***************************** END OF ILE_V5CC.C *****************************/
