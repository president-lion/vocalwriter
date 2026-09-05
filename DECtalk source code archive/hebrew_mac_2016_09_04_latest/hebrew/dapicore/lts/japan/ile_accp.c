/*********************************************
	ILE_ACCP.C	updated at 17th April 1990
	文節の格決定とポーズ位置の推定
 *********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_acch.h"
#include	"ile_envh.h"
#include	"ile_fddh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"
#include "pkind.h"

#if defined(O_JAL)
extern BYTE buf[];
#endif

#define		CONTINUE		 0		/* ポーズは入らない。*/
#define		RENTAI_NOPAUSE	 1		/* 連体修飾格：ポーズは入らない。*/
#define		RENTAI			 2		/* 連体修飾格：ポーズは未定。*/
#define		RENYOU			 4		/* 連用修飾格：ポーズは未定。*/
#define		PAUSE_SHORT		 8		/* 短いポーズを入れる。*/
#define		PAUSE_LONG		16		/* 長いポーズを入れる。*/

/*#define	PAUSE_MEDIUM	12		   副詞の後のポーズ。*/

#define		NAKATEN			0x2126	/* 「・」 */
#define		TOUTEN			0x2123	/* 「。」 */

/*========== PAUSE1000() =========================
	機能：格の種類を調べてポーズの有無を決定する。
  ================================================*/
void PAUSE1000() 
{
	BYTE al;
	integer_16 bno,even,mem;
	WORD code;
	register integer_16 i,c;

	/* 後端番号から、格の種類を初期設定する */
	stc_pause[0]=0;
	for(i=0;i<stc_ptr-1;++i){
		stc_pause[i+1]=0;
		for(c=0;c<stc_nbno[i];++c){
			if(bno=stc_bno[i][c]){
				/* 格に関係するのは、下位３ビットだけである。*/
				switch(FddHDR[Fdd_Header.Fdd_BnoTblOfs+bno-1] & 0x07){
				case BITBNO_RentaiKaku:
					al=RENTAI; break;
				case BITBNO_RenyouKaku:
					al=RENYOU;
					/* 連用修飾格で、副詞ならば、必ずポーズを入れる。
					 * if(bno==Fdd_Header.Fdd_BnoAdv){ al=PAUSE_MEDIAM; fi
					 */
					break;
				case BITBNO_Shushi:
					al=PAUSE_LONG; break;
				default:
					al=CONTINUE; break;
				}
				stc_pause[i+1]=al;
				goto PAUSE1000_170;
			}
		}
		PAUSE1000_170:;
	}

	/*
	 * ５９の前は終止形とする。
	 *		==> 複合名詞の間にポーズが入ってしまうので良くない。
	 *	for(i=1;i<stc_ptr;++i){
	 *		if(acc_accent_raw[i][1]==5 && acc_accent_raw[i][2]==9){
	 *			stc_pause[i-1]=BITBNO_Shushi;
	 *		}
	 *	}
	 */
	/*
	 * 付属語の直前は必ず結合する。
	 * 複合動詞・複合形容詞はひとつの語と見做されるべきであり、
	 * 間にポーズを入れてはならない。(87.1.13)
	 * 文末は、必ずポーズする。
	 * 文頭の句読点（特殊文字）は考慮しない。
	 */
	for(i=1;i<stc_ptr;++i){
		/* 付属語の直前は必ず結合する。*/
		if((stc_ctl[i] & BITCTL_FIRST)==0){ stc_pause[i]=0; }
		/*
		 *	複合動詞・複合形容詞はひとつの語と見做されるべきであり、
		 *	間にポーズを入れてはならない。
		 *	「（動詞語幹 or 動詞連用形語尾）＋（動詞語幹 or 形容詞語幹）」
		 *	のときはポーズしないようにする。
		 */
		if((phrase_kind_sav[i-1]==pkind_verb_stem
		  ||phrase_kind_sav[i-1]==pkind_verb_renyou)
		 &&(phrase_kind_sav[i]==pkind_verb_stem
		  ||phrase_kind_sav[i]==pkind_adj_stem)){
			stc_pause[i]=CONTINUE;
		}
	}

	/*
	 * 句読点は句の境界とする。（小数点が区切りに変化したものも含める。）
	 * 従って、句読点の直後は必ずポーズを入れる。
	 * 但し、文頭に特殊文字（空白など）があっても、次に続く文節との間には
	 * ポーズは入らない。
	 */
	for(i=1;i<stc_ptr-1;++i){
		if(code=chk_separator(i)){
			if(code==NAKATEN
			&&(inert_kind[stc_in_ptr[i-1]]==CHRKIND_KKana
			 ||inert_kind[stc_in_ptr[i+1]]==CHRKIND_KKana)){
				/*
				 *	句読点が中点の時は、
				 *	前後のどちらかが片仮名ならばポーズしない。
				 *	　（例）ニュー・ヨーク、ポーズ・時間
				 */
				stc_pause[i]=stc_pause[i+1]=CONTINUE;
			}else{
				/* 句読点の直前の格はクリアしておく。*/
				stc_pause[i]=CONTINUE;
				/* 句点は２拍、それ以外は１拍ポーズする。*/
				stc_pause[i+1]=(code==TOUTEN? PAUSE_LONG:PAUSE_SHORT);
			}
		}
	}

#if defined(O_JAL)
	/*
	 * 半角スペースの直後はポーズをはずす。
	 */
	for(i=1;i<stc_ptr-1;++i){
		if(chk_han_space(i)){
			/* 半角スペースの直前の格はクリアしておく。*/
			stc_pause[i]=CONTINUE;
			stc_pause[i+1]=0;
		}
	}
#endif

	/********************************************************************
	 *																	*
	 *	この時点でのstc_pause[]の意味。									*
	 *																	*
	 *		直前が句読点・特殊文字・終止形	：	PAUSE_LONG				*
	 *		直前が連用修飾格				：	RENYOU					*
	 *		直前が連体修飾格				：	RENTAI					*
	 *		直前にポーズを入れない			：	0						*
	 *																	*
	 ********************************************************************/

	/*
	 * 係助詞「は」の後は必ずポーズした方がよいかもしれない。
	 *
	 *		これは、あくまでも、仮説である。
	 *
	 *	for(i=0;i<stc_ptr;++i){
	 *		if(stc_bno[i][0]==99){ stc_pause[i]=PAUSE_SHORT; }
	 *	}
	 */

	/*
	 * 連体修飾格の場合、直前・直後のポーズまでのモーラ数がある長さ以上に長ければ、
	 * 強制的にポーズさせるが、短ければ結合する。
	 *	for(c=0,i=0;i<stc_ptr;++i){
	 *		if(stc_pause[i]==0){ c+=stc_out_len[i]; }
	 *		if(stc_pause[i]>=4 || stc_pause[i]==BITBNO_RenyouKaku){ c=0; }
	 *		if(stc_pause[i]==BITBNO_RentaiKaku){ mem=i; }
	 *		if(c>MAX_PHRASE){ stc_pause[mem]=4; c-=mem; }
	 *	}
	 *	for(c=0,i=stc_ptr;i--;){
	 *		if(stc_pause[i]==0){ c+=stc_out_len[i]; }
	 *		if(stc_pause[i]>=4 || stc_pause[i]==BITBNO_RenyouKaku){ c=0; }
	 *		if(stc_pause[i]==BITBNO_RentaiKaku){ mem=i; }
	 *		if(c>MAX_PHRASE){ stc_pause[mem]=4; c-=mem; }
	 *	}
	 *	for(i=0;i<stc_ptr;++i){
	 *		if(stc_pause[i]==BITBNO_RentaiKaku){ stc_pause[i]=0; }
	 *	}
	 */

	/*
	 *	連体修飾格が連続する場合は、後から見て、
	 *	奇数番目は結合、偶数番目はポーズを入れる。
	 */
	even=1;
	for(i=stc_ptr;i--;){
		switch(stc_pause[i]){
		case RENTAI:	even=1-even;
						stc_pause[i]=(even? PAUSE_SHORT:RENTAI_NOPAUSE);
						break;
		case 0:			break;
		default:		even=1;
		}
	}

	/*
	 * 連用修飾格の場合、直前・直後のポーズまでのモーラ数がある長さ以上に長ければ、
	 * 強制的にポーズさせる。
	 * c=0;
	 *	for(i=0;i<stc_ptr;++i){
	 *		if(stc_pause[i]==0){ c+=stc_out_len[i]; }
	 *		if(stc_pause[i]>=4){ c=0; }
	 *		if(stc_pause[i]==BITBNO_RenyouKaku){ mem=i; }
	 *		if(c>MAX_PHRASE){ stc_pause[mem]=4; c-=mem; }
	 *	}
	 *	c=0;
	 *	for(i=stc_ptr;i--;){
	 *		if(stc_pause[i]==0){ c+=stc_out_len[i]; }
	 *		if(stc_pause[i]>=4){ c=0; }
	 *		if(stc_pause[i]==BITBNO_RenyouKaku){ mem=i; }
	 *		if(c>MAX_PHRASE){ stc_pause[mem]=4; c-=mem; }
	 *	}
	 */

	/*
	 *	連用修飾格が連続する場合は、前から見て、
	 *	奇数番目は結合、偶数番目はポーズを入れる。
	 *	但し、体＋用＋用＋…のときは、体＋用／用＋…とする。
	 *	また、用＋体＋用＋…のときは、用／体＋用／…とする。
	 *
	 *	（例）	美しい花が／咲く
	 *			春が来て／花が咲く
	 *			私は／妹の学校へ／行った
	 *			言語工学は優秀だ？
	 *			言語工学の技術は／優秀だ？
	 *			言語工学の／一戸の技術は／優秀だ！？
	 *			美しい花が／咲いて春が／来る	（これは仕方ないであろう）
	 */
	even=1;
	for(i=0;i<stc_ptr;++i){
		switch(stc_pause[i]){
		case RENYOU:
			mem=i; even=1-even;
			stc_pause[i]=(even? PAUSE_SHORT:CONTINUE);
			break;
		case CONTINUE:
			break;
		case RENTAI_NOPAUSE:
			stc_pause[i]=CONTINUE;
			if(even==0){ stc_pause[mem]=PAUSE_SHORT; }
			even=0;
			break;
		default:
			even=1;
		}
	}

	/*
	 * 「美しい花が／咲いて／春が来る」としたい為に、
	 * 体＋用の直後の用の前だけでなく後でもポーズするようにしたもの。
	 * even=1; c=0;
	 * for(i=0;i<stc_ptr;++i){
	 *	 switch(stc_pause[i]){
	 *	 case RENYOU:
	 *		if(c==4){ even=0; c=0; }
	 *		mem=i; even=1-even; stc_pause[i]=(even? PAUSE_SHORT:CONTINUE);
	 *		if(c==2){ c=4; }
	 *		break;
	 *	 case CONTINUE:
	 *		break;
	 *	 case RENTAI_NOPAUSE:
	 *		stc_pause[i]=CONTINUE;
	 *		if(even==0){ stc_pause[mem]=PAUSE_SHORT; }
	 *		even=0; c=2; break;
	 *	 default:
	 *		even=1; c=0;
	 *	 }
	 * }
	 */

	/********************************************************************
	 *																	*
	 *	この時点でのstc_pause[]の意味。									*
	 *																	*
	 *		直前にポーズを２拍程度入れる。	：	PAUSE_LONG				*
	 *		直前にポーズを１拍程度入れる。	：	PAUSE_SHORT				*
	 *		直前にポーズを入れない			：	0						*
	 *																	*
	 ********************************************************************/

	/*
	 *	ポーズに関して操作があれば、考慮する。(63/10/18)
	 *	但し、文節最後の要素だけ考慮する。
	 */
	for(i=0;i<stc_ptr;++i){
		if(stc_mru[i] & BITMRU_PINS){
			stc_pause[i]=PAUSE_SHORT;
		}else{
			if(stc_mru[i] & BITMRU_PDEL){
				stc_pause[i]=0;
			}
		}
	}

	/*
	 *	数字の直前にはすこしポーズをいれてみたいが、ｐｅｎｄｉｎｇ。
	 *	なぜならば、「十月」も「一致」も漢語であり、「十月」の前にはポーズを
	 *	入れたいが、「一致」の前にはポーズを入れたくない。
	 *
	 *for(i=1;i<stc_ptr;++i){
	 *	数字以外の要素と数字の間には、ポーズが入りうる。
	 *	外部辞書の数詞（１０月、一日、etc.）も対象となるので注意する。
	 *	c=stc_in_ptr[i];
	 *	if((inert_kind[c-1]&0xF0)!=CHRKIND_Digit&&(inert_kind[c]&0xF0)==CHRKIND_Digit){
	 *		ポーズがすでにあるときはそのままとする。
	 *		if(stc_pause[i]==0){ stc_pause[i]=PAUSE_SHORT; }
	 *	}
	 *}
	 */

	/*
	 *	兆、億、万の直後に数字があれば、すこしポーズをいれてみる。
	 */
	for(i=1;i<stc_ptr;++i){
		if(stc_kind[i-1]==DICKIND_D && stc_kind[i]==DICKIND_D){
			if(stc_dexp[i-1]>=4){
				/* ポーズがすでにあるときはそのままとする。*/
				if(stc_pause[i]==0){ stc_pause[i]=PAUSE_SHORT; }
			}
		}
	}

	/*
	 * この時点でのstc_pause[]は、その要素の直前に入れるポーズの拍数を
	 * １／８拍単位で表わしたものになっている。
	 */

	/*
	 *	校正モードの特殊文字は前後に２拍程度、休憩を入れる。
	 */
	for(i=0;i<stc_ptr;++i){
		if(onsei_addr->onsei_kouseisystem && stc_kind[i]==DICKIND_T){
			al=inert_kind[stc_in_ptr[i]] & 0xF0;
			if(al==CHRKIND_Other1 || al==CHRKIND_Other2){
				stc_pause[i]=PAUSE_LONG;
				if(i<stc_ptr-1){ stc_pause[i+1]=PAUSE_LONG; }
			}
		}
	}

}


#if defined(O_JAL)
chk_han_space(i) integer_16 i; {
	BYTE ch;
	integer_16 ptr;
	register WORD before,after;

	ptr=stc_in_ptr[i];
	/* 長さが２文字以上なら、特殊文字とは見做さない。*/
	if(stc_out_len[i]!=1){ return 0; }
	ch=inert_kind[ptr];
	ch &= 0xF0;
	if(ch!=CHRKIND_Other1 && ch!=CHRKIND_Other2){ return 0; }
	if(((buf[ptr*2]<<8) | buf[ptr*2+1])!=0x2020){ return 0; }
	/* 半角スペースのときは、前後が英字の時、ポーズしない。*/
	before = (buf[ptr*2-2]<<8) | buf[ptr*2-1];
	after  = (buf[ptr*2+2]<<8) | buf[ptr*2+3];
	if( (before>0x2040 && before<0x205B) ||			/* A..Z */
		(before>0x2060 && before<0x207B) ||			/* a..z */
		(before>0x202F && before<0x203A) ||			/* 0..9 */
		(before>0x232F && before<0x233A) ||			/* ０..１ */
		(before>0x2340 && before<0x235B) ||			/* Ａ..Ｚ */
		(before>0x2360 && before<0x237B) ||			/* ａ..ｚ */
		(before>0x2620 && before<0x277F) ){		/* Α..я */
		if( (after>0x2040 && after<0x205B) ||			/* A..Z */
			(after>0x2060 && after<0x207B) ||			/* a..z */
			(after>0x202F && after<0x203A) ||			/* 0..9 */
			(after>0x232F && after<0x233A) ||			/* ０..１ */
			(after>0x2340 && after<0x235B) ||			/* Ａ..Ｚ */
			(after>0x2360 && after<0x237B) ||			/* ａ..ｚ */
			(after>0x2620 && after<0x277F) ){		/* Α..я */
				return 1;
		}
	}
	return 0;
}
#endif

/***************************** END OF ILE_ACCP.C *****************************/
