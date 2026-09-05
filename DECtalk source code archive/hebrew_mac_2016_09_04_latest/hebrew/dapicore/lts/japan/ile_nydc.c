/*************************************************
	ILE_NYDC.C	updated at 18th September 1990
	入力文関連
 *************************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_kkhh.h"
#include	"ile_mjzh.h"
#include	"ile_nydh.h"
#include	"ile_func.h"


/*========== INERT_INIT() ===================================================
	機能：入力文の各文字の標準化をする

	(1) 文字の種類を設定する
			漢数字１　::=　一二三四五六七八九〇壱弐参
			漢数字２　::=　十百千拾
			漢数字３　::=　数幾何
			漢数字４　::=　万億兆京萬
			漢字　　　::=　漢数字を除く漢字
			平仮名　　::=　あ..をん　及び　平仮名に続く長音
			片仮名　　::=　ア..ヲン　及び　片仮名に続く長音
			英字　　　::=　Ａ..Ｚ　及び　Α..Ω　及び　А..Я
			算用数字　::=　１２３４５６７８９０
			開括弧　　::=　‘“（〔［｛〈《「『【
			閉括弧　　::=　’”）〕］｝〉》」』】
			符号　　　::=　＋−±
			その他
	(2) コンマ，ピリオド：
			前後が共に数字の時はそれぞれ三桁毎の区切り、小数点とする
			さもなければ特殊文字と見做す
	(3) ハイフン or マイナス：
			前後が共に英数字ならばハイフンとする
			さもなければ負記号と見做す
		例：０３−１２３−４５６７		：ハイフン
			今日の気温は−１０℃です	：マイナス
  ===========================================================================*/

void INERT_INIT() {
	register BYTE *kptr,*aptr;
	LPBYTE nyd;
	BYTE ah,al,dl,kind;
	WORD ax;
	integer_16 id1,id2;

	n_inert=0;
	for(kptr=inert;	     kptr<(inert+MAX_INERT*2)   ;){ *kptr++='\0'; }
	for(kptr=inert_kind; kptr<(inert_kind+MAX_INERT);){ *kptr++='\0'; }
	for(kptr=inert_attr; kptr<(inert_attr+MAX_INERT);){ *kptr++='\0'; }
	ceptr=0; celen=0; ceptr_sav=0; celen_sav=0;

	nyd= onsei_addr->onsei_inert;	/* 入力文字列のアドレス */
	id1=0; id2=0;
	kptr=inert_kind; aptr=inert_attr;
	while(1){
		ah=(*nyd++);				/* 入力文字の第１バイトを取り出す。*/
		if(ah==0){ break; }		/* end of string なら終わり。*/

		if(id1>=MAX_INERT){ jp_ERROR(Err_InertOVF); }	/* 文字数超過 */

		al=(*nyd++);				/* 入力文字の第２バイトを取り出す。*/
		ax=al|(ah<<8);
		inert[id2]=ah; inert[id2+1]=al;
		*aptr=0;					/* 入力文字の属性をリセットする。*/

		/* 半角文字はエラー。*/
		if(ah==0x00||ah==0x20||ah==0x29){
			jp_ERROR(Err_Hankaku);
		}

		/* 漢数字・符号（＋±）を分類する。*/
		switch(ax){
		case 0x306C: case 0x4673: case 0x3B30: case 0x3B4D:	/* 一二三四 */
		case 0x385E: case 0x4F3B: case 0x3C37: case 0x482C:	/* 五六七八 */
		case 0x3665: case 0x213B:							/* 九〇 */
		case 0x306D: case 0x4675: case 0x3B32:				/* 壱弐参 */
			*kptr=CHRKIND_KDigit1; goto NextChr;
		case 0x3D3D: case 0x4934: case 0x4069:				/* 十百千 */
		case 0x3D26:										/* 拾 */
			*kptr=CHRKIND_KDigit2; goto NextChr;
		case 0x323F: case 0x3476: case 0x3F74:				/* 何数幾 */
		/* 複＝0x4A23 */									/* 複数 */
			*kptr=CHRKIND_KDigit3; goto NextChr;
		case 0x4B7C: case 0x322F: case 0x437B: case 0x357E: /* 万億兆京 */
		case 0x685F:										/* 萬 */
			if(id1==0){ break; }
			if(*(kptr-1)==CHRKIND_KDigit1 || *(kptr-1)==CHRKIND_KDigit2
			|| *(kptr-1)==CHRKIND_KDigit3 || *(kptr-1)==CHRKIND_ADigit){
				*kptr=CHRKIND_KDigit4; goto NextChr;
			}else{
				break;
			}
		/* case 0x215C: case 0x215E: */							/* ＋± */
			/* *kptr=CHRKIND_DSign; goto NextChr; */
		}

		/* 漢字・平仮名・片仮名の分類 */
		if(ah>=0x30){ *kptr=CHRKIND_Kanji; goto NextChr; }
		if(ah==0x24){				/* 平仮名 */
			*kptr=CHRKIND_HKana;
			dl=MjzfGetChrAttr(al);		/* 文字属性表を引く */
			if(MjzfHido(dl)){		/* 非独立文字(ぁぃぅぇぉゃゅょ) */
				*aptr=CHRATTR_Hido; goto NextChr;
			}
			if(MjzfSoku(dl)){		/* 促音または撥音 */
				*aptr=CHRATTR_Soku; goto NextChr;
			}
			goto NextChr;
		}

		if(ah==0x25){				/* 片仮名 */
			*kptr=CHRKIND_KKana;
			dl=MjzfGetChrAttr(al);		/* 文字属性表を引く */
			if(MjzfHido(dl)){		/* 非独立文字(ァィゥェォャュョ) */
				*aptr=CHRATTR_Hido; goto NextChr;
			}
			if(MjzfSoku(dl)){		/* 促音または撥音 */
				*aptr=CHRATTR_Soku; goto NextChr;
			}
			goto NextChr;
		}

		/* 英字・希字・露字・算用数字の分類 */
		*kptr=CHRKIND_Alpha;
		if(ah==0x23 && al>=0x41){ goto NextChr; }	/* English */
		if(ah==0x26){ goto NextChr; }				/* Greek */
		if(ah==0x27){ goto NextChr; }				/* Russian */

		if(ah==0x23 && al<=0x39){					/* digit */
			*kptr=CHRKIND_ADigit; goto NextChr;
		}

		if(ah==0x21){
			switch(al){
			case 0x39: case 0x3A:	/* 々〆 （９０年５月１日）*/
				*kptr=CHRKIND_Kanji;
				goto NextChr;
			case 0x3C:
				/*
				 * 先頭の長音は誤文、仮名以外の文字に続くなら誤文。
				 * 平仮名の直後なら平仮名、片仮名の直後なら片仮名。
				 * さらに、長音は促音・撥音同様の扱いとする。
				 */
				kind=(*(kptr-1));	/*inert_kind[id1-1];*/
				if(id1==0										/* 先頭の長音 */
				||(inert[id2-2]==0x21 && inert[id2-1]==0x3C)	/* 前が長音 */
				||(kind!=CHRKIND_HKana && kind!=CHRKIND_KKana) ){	/* 前が仮名以外 */
					goto yomenai;
				}
				*kptr=kind;
				*aptr=CHRATTR_Soku;
				goto NextChr;
			default:
				yomenai:
				dl=MjzfGetChrAttr(al);		/* 文字属性表を引く */
				*kptr=(MjzfToku1(dl)? CHRKIND_Other1:CHRKIND_Other2);

				switch(al){
				case 0x46: case 0x48: case 0x4A: case 0x4C: case 0x4E:
				case 0x50: case 0x52: case 0x54: case 0x56: case 0x58:
				case 0x5A:
					*kptr|=CHRKIND_BOpen; goto NextChr;
				case 0x47: case 0x49: case 0x4B: case 0x4D: case 0x4F:
				case 0x51: case 0x53: case 0x55: case 0x57: case 0x59:
				case 0x5B:
					*kptr|=CHRKIND_BClose; goto NextChr;
				default:
					*kptr|=CHRKIND_Other; goto NextChr;
				}
			}
		}else{
			/* 22xx,2cxx,2dxx */
			*kptr=CHRKIND_Other1 | CHRKIND_Other;
		}

		NextChr:
		id1++; id2+=2;
		++kptr; ++aptr;
	}

	if(id1==0){ jp_ERROR(Err_InertNone); }		/* 入力文字列の長さがゼロ */

	*kptr=0;			/* 最後の文字をnullにする */
	n_inert=id1;		/* 入力文字数 */

	INERT_DIGIT();		/* ハイフン・ピリオドの分類 */
	INERT_COMMA();		/* コンマの分類 */

	if(n_inert==0){ jp_ERROR(Err_InertNone); } /* 入力文字列の長さがゼロ */

	ceptr=0;
	onsei_addr->onsei_ninert= n_inert;

	return;
}


/*========== INERT_DIGIT() ==================================================
	機能：ハイフンとマイナスの分類。
		　ピリオドが、区切り記号か小数点かを分類。
  ===========================================================================*/
void INERT_DIGIT() {
	register integer_16 id1,id2;
	integer_16 bptr,nptr;
	WORD ax;
	BYTE ah,al;

	id1=0; id2=0;
	while(1){
		if(inert_kind[id1]==0){ break; }
 
		bptr=id1-1;
		if(bptr!=-1){ ah=inert_kind[bptr]; }	/* 前の文字の種類 */
		nptr=id1+1;
		if(nptr!=n_inert){ al=inert_kind[nptr]; }	/* 次の文字の種類 */

		ax=(inert[id2]<<8) | inert[id2+1];		/* １文字取り出す */

		if(ax==0x215D || ax==0x215C || ax==0x215E){
			/* 「＋−±」：前後が共に英数字ならば特殊文字とする
					   さもなければ、符号 */
			inert_kind[id1]=CHRKIND_Other1 | CHRKIND_Other;
			if(bptr!= -1 && nptr!=n_inert){
				if((ah & 0xF0)==CHRKIND_Digit || ah==CHRKIND_Alpha){
					if((al & 0xF0)==CHRKIND_Digit || al==CHRKIND_Alpha){
						goto NextChr2;
					}
				}
			}
			if(nptr!=n_inert){
				if((al & 0xF0)==CHRKIND_Digit){
					/* 次に数字があれば数字、さもなければ、特殊文字とする。*/
					inert_kind[id1]=CHRKIND_DSign;
				}
			}
			goto NextChr2;
		}

		if(ax==0x2125){
			/* 前後が数字ならピリオドは小数点、さもなければ句読点。*/
			if(bptr== -1){ goto NextChr2; }		/* 1st char */
			if(nptr==n_inert){ goto NextChr2; }		/* last char */
			if((ah & 0xF0)==CHRKIND_Digit &&
			   (al & 0xF0)==CHRKIND_Digit){	/* 前後が数字 */
				inert_kind[id1]= (ax==0x2124? CHRKIND_DSep:CHRKIND_DDP);
				goto NextChr2;
			}
		}

		NextChr2: ;
		id1++; id2+=2;
	}
}


/*========== INERT_COMMA() ==================================================
	機能：コンマが、区切り記号か、桁区切りかを分類。

		��	前が１〜３桁の数字１で、かつ後が３桁の数字１なら、桁区切り、
			さもなければ、句読点とする。

			１，２３４，５６７			：	１２３４５６７
			１，２３，４５，６７		：	１・２３・４５・６７
			１２３４，５６７			：	１２３４・５６７
			一，二三四，五六七			：	１２３４５６７
			一，二百三十四，五百六十七	：	１・２３４・５６７

		��	数字列の途中に句読点が１つでもあれば、やはり句読点とする。

			１２，３４，５６７，８９０	：	１２・３４・５６７・８９０
			１２，３４５，６７８，９０	：	１２・３４５・６７８・９０
			１２，３４５，６７，８９０	：	１２・３４５・６７・８９０
  ===========================================================================*/
void INERT_COMMA() {
	register integer_16 id1,count;
	integer_16 comma;

	if(onsei_addr->onsei_kouseisystem){
		/* 校正モードでは、数字は必ず棒読みとするので、コンマは分類せず。*/
		/* --89.11.29-- */
		return;
	}

	count=0; comma= -1;
	for(id1=n_inert; id1--;){
		switch(inert_kind[id1]){
		case CHRKIND_KDigit1:	/* 漢数字1 : 一二三四五六七八九〇 */
		case CHRKIND_ADigit:	/* 算用数字 : １２３４５６７８９０ */
			++count;
			break;
		default:
			if(inert[id1*2]==0x21 && inert[id1*2+1]==0x24){
				if(count==3){
					/* コンマの後が３桁の数字ならそのコンマを桁区切りとする。*/
					inert_kind[id1]=CHRKIND_DSep;
					comma=id1;
				}
				count=0;
			}else{
				/* neither comma nor digit */
				if(count==0 || count>3){
					if(comma!= -1){
						/*　「１２３４，５６７」、「，２３４」のコンマは句読点。*/
						inert_kind[comma]=CHRKIND_Other2 | CHRKIND_Other;
					}
				}
				count=0; comma= -1;
			}
		}
	}
	if(count==0 || count>3){
		if(comma!= -1){
			/*　「１２３４，５６７」、「，２３４」のコンマは句読点。*/
			inert_kind[comma]=CHRKIND_Other2 | CHRKIND_Other;
		}
	}
	/*
	 * ここまでの処理で以下の様になっている。
	 *	１２３４，５６７			：	１２３４・５６７			：　ＯＫ
	 *	一，二三四，五六七			：	１，２３４，５６７			：　ＯＫ
	 *	一，二百三十四，五百六十七	：	１・２３４・５６７			：　ＯＫ
	 *	１，２３４，５６７			：	１，２３４，５６７			：　ＯＫ
	 *	１，２３，４５，６７		：	１・２３・４５・６７		：　ＯＫ
	 *	１２，３４，５６７，８９０	：	１２・３４，５６７，８９０	：
	 *	１２，３４５，６７８，９０	：	１２，３４５，６７８・９０	：
	 *	１２，３４５，６７，８９０	：	１２，３４５・６７，８９０	：
	 */
	comma= -1;
	for(id1=n_inert; id1--;){
		switch(inert_kind[id1]){
		case CHRKIND_KDigit1:	/* 漢数字1 : 一二三四五六七八九〇 */
		case CHRKIND_ADigit:	/* 算用数字 : １２３４５６７８９０ */
			break;
		default:
			if(inert[id1*2]==0x21 && inert[id1*2+1]==0x24){
				if(comma==-1){
					if(inert_kind[id1]!=CHRKIND_DSep){
						comma=id1;
					}
				}else{
					inert_kind[id1]=CHRKIND_Other2 | CHRKIND_Other;
				}
			}else{
				comma= -1;
			}
		}
	}
	comma= -1;
	for(id1=0; id1<n_inert; ++id1){
		switch(inert_kind[id1]){
		case CHRKIND_KDigit1:	/* 漢数字1 : 一二三四五六七八九〇 */
		case CHRKIND_ADigit:	/* 算用数字 : １２３４５６７８９０ */
			break;
		default:
			if(inert[id1*2]==0x21 && inert[id1*2+1]==0x24){
				if(comma== -1){
					if(inert_kind[id1]!=CHRKIND_DSep){
						comma=id1;
					}
				}else{
					inert_kind[id1]=CHRKIND_Other2 | CHRKIND_Other;
				}
			}else{
				comma= -1;
			}
		}
	}
}


/*========== INERT1000() ===============================================
	機能：「現在要素開始位置」から始まる、辞書引きができる、
		引き続く同じ字種の長さを求める。
	値　：読みの長さ
  =============================================NydfSaichouGjsMotomeru==*/
integer_16 INERT1000() {
	/* 片仮名語や数字、特殊文字は、文字数の制限がない。*/

#if !defined(O_TALKONLY)
	/* 次候補・長方向・短方向処理のとき、*/
	if(conv_mode==CONV_NEXT||conv_mode==CONV_SHORT||conv_mode==CONV_LONG){
		/* 注目要素の位置と長さが文節範囲と矛盾があれば長さを縮める。*/
		while(celen && (WS_CE_CHK()==0)){
			celen=INERT2000(ceptr,celen);
		}
	}
#endif

	return celen;
}


/*========== INERT2000(開始位置,読み長さ) =================================
	機能：「現在要素開始位置」から始まる「現在要素長さ」を非独立文字を
 		考慮して縮める。
	値　：縮めたあとの「現在要素長さ」
  =======================================================NydfYomChijimeru==*/
integer_16 INERT2000(e_ptr,e_len) integer_16 e_ptr,e_len; {
	/*
	 *	「１ヵ月」の「ヵ月」のような例もあるので、非独立文字の場合も許す。
	 */
	--e_len;
	return e_len;
}


/*========== INERT200B(開始位置,読み長さ) =================================
	機能：「現在文節開始位置」から始まる「現在文節長さ」を非独立文字を
		　考慮して縮める。
	値　：縮めたあとの「現在文節長さ」
  =======================================================NydfYomChijimeru==*/
integer_16 INERT200B(b_ptr,b_len) register integer_16 b_ptr,b_len; {

INERT200B_10:
	if(--b_len==0){ return 0; }

	/* 次の文節の先頭が非独立文字ならさらに縮める。*/
	if(inert_attr[b_ptr+b_len] & CHRATTR_Hido){ goto INERT200B_10; }
	return b_len;
}


/*========== INERT3000() ==================================================
	機能：「現在要素開始位置」から始まる「現在要素長さ」を非独立文字を
 		考慮してのばす。
	値　：のばせたときは、「現在要素長さ」をかえす。のばせないときは、０。

	integer_16 INERT3000(e_ptr,e_len) register integer_16 e_ptr,e_len; {
		if((e_ptr+e_len)>=n_inert){
			return 0;
		}else{
			e_len++; return e_len;
		}
	}
  ==========================================================NydfYomNobasu==*/


/*========== INERT300B() ==================================================
	機能：「現在文節開始位置」から始まる「現在文節長さ」を非独立文字を
 		　考慮してのばす。
	値　：のばせたときは、「現在文節長さ」をかえす。延ばせないときは、０。
  ==========================================================NydfYomNobasu==*/
integer_16 INERT300B(b_ptr,b_len) register integer_16 b_ptr,b_len; {

INERT300B_10:
	if((b_ptr+b_len)>=n_inert){ return 0; }
	++b_len;

	/* 次の文節の先頭が非独立文字ならさらに延ばす。*/
	if(inert_attr[b_ptr+b_len] & CHRATTR_Hido){ goto INERT300B_10; }

	return b_len;
}


/*========== INERT4000() ====================================================
	機能：「現在要素開始位置」から始まる引き続く同じ字種の長さを、
		「現在要素長さ」に求める。
	値　：入力文字があるなら、１、なければ、０。

	BOOL INERT4000() {
		register BYTE *bx,al;

		if(ceptr>=n_inert){ return 0; }	** 入力文字がない **
		bx=(&inert_kind[ceptr]);

		celen=0;

		** 文字の種類は上位４ビットで比較する **
		al=(*bx) & 0xF0; ++bx; ++celen;
		for(;*bx;celen++,bx++){
			if(al!=(*bx & 0xF0)){ break; }
		}

		return 1;
	}
  ==================================================NydfSaichouJsuMotomeru==*/

/***************************** END OF ILE_NYDC.C *****************************/
