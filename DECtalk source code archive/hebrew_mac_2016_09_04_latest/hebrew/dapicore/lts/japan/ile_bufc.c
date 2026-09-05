/*********************************************
	ILE_BUFC.C	updated at 24th April 1990
	各種サブルーチン集
 *********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_acch.h"
#include	"ile_fddh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_func.h"


/*========== get_case(p) ====================================
	機能：ｐ番目の要素の後端番号から格決定用テーブルを引く。
	値　：格決定用テーブルの値（８ビット）。
  ===========================================================*/
WORD get_case(p) register integer_16 p; {
	register integer_16 i;
	WORD bno;

	for(i=0;i<stc_nbno[p];++i){
		if(bno=stc_bno[p][i]){ goto get_case_200; }
	}
get_case_200:;
	return FddHDR[Fdd_Header.Fdd_BnoTblOfs+bno-1];
}


/*========== chk_heiban(p) =====================
	機能：ｐ番目の要素が平板型かどうかを調べる。
	値　：平板型なら１、そうでなければ０。
  ==============================================*/
BOOL chk_heiban(p) integer_16 p; {
	register integer_16 pp,pl;

	pp=phrase_ptr[p]; pl=phrase_len[p];
	if((pl>1 && accent0[pp+pl-1]==2)||(pl==1 && accent0[pp]==1)){
		return 1;
	}else{
		return 0;
	}
}


/*========== chk_odaka(p) ======================
	機能：ｐ番目の要素が尾高型かどうかを調べる。
	値　：尾高型なら１、そうでなければ０。
  ==============================================*/
BOOL chk_odaka(p) register integer_16 p; {
	register integer_16 pl;

	pl=phrase_len[p];
	if(pl>1 && accent0[phrase_ptr[p]+pl-1]==0x82){
		return 1;
	}else{
		return 0;
	}
}


/*========== chk_separator(i) =====================
	機能：ｉ番目の文字が特殊文字かどうかを調べる。
	値　：特殊文字なら文字コード、そうでなければ０。
  =================================================*/
WORD chk_separator(i) register integer_16 i; {
	BYTE ch;
	register integer_16 ptr;

	ptr=stc_in_ptr[i];
	/* 長さが２文字以上なら、特殊文字とは見做さない。*/
	if(stc_out_len[i]!=1){ return 0; }
	ch=inert_kind[ptr];
	/* 小数点のピリオド化したものは特殊文字と見做す。（３／．／６人が…）*/
	if(ch==CHRKIND_DDP){ return(0x2125); }
	ch &= 0xF0;
	if(ch==CHRKIND_Other1||ch==CHRKIND_Other2){
		return((inert[ptr*2]<<8) | inert[ptr*2+1]);
	}
	return 0;
}


/*========== next_phrase(i) =================
	機能：ｉ番目の句の次の句の番号を求める。
	値　：次の句があれば句の番号、なければ０。
  ===========================================*/
integer_16 next_phrase(i) register integer_16 i; {
	register integer_16 j;

	for(j=i+1;j<stc_ptr;++j){
		if(phrase_len[j]>0){ return j; }
	}
	return 0;
}


/*========== net_phrase_in_len(i) =================
	機能：ｉ番目の句の入力文字の文字数を求める。
	値　：入力文字の数。
  =================================================*/
integer_16 net_phrase_in_len(i) integer_16 i; {
	integer_16 j;
	register integer_16 len;

	len=((j=next_phrase(i))? stc_in_ptr[j]:n_inert) - stc_in_ptr[i];
	return len;
}


/*========== knj_number(i) =================
	機能：ｉ番目の句の漢字の文字数を求める。
	値　：漢字数。
  ==========================================*/
integer_16 knj_number(i) integer_16 i; {
	integer_16 j,knj;
	register integer_16 ptr,len;

	len=((j=next_phrase(i))? stc_in_ptr[j]:n_inert)-(ptr=stc_in_ptr[i]);
	for(knj=0;len--;ptr++){
		if(inert_kind[ptr]==CHRKIND_Kanji){ ++knj; }
	}
	return knj;
}


/*========== chk_value(AL,SI,CX) ====================================================
	構文：PTR = chk_value(AL,SI,CX);
		このとき、SI[PTR-1] = ALである。
	機能：大きさCXのバイト配列SI[]に要素ALが含まれるかどうかを調べる。
	戻値：０なら属性なし。
		さもなければ、ALに等しかった要素の位置を１オリジンで示す。
  ===================================================================================*/
integer_16 chk_value(AL,SI,CX) register BYTE *SI; BYTE AL; register integer_16 CX; {
	for(;CX--;){ if(SI[CX]==AL){ return (CX+1); } }
	return 0;
}


/*========== chk_limits(LL,UL,SI,CX) ===================================================
	構文：PTR = chk_limits(LL,UL,SI,CX);
		このとき、LL ≦ SI[PTR-1] ≦ ULである。
	機能：大きさCXのバイト配列SI[]にLL〜ULの範囲の要素が含まれるかどうかを調べる。
	戻値：０なら属性なし。さもなければ、LL〜ULの範囲の要素がある位置を１オリジンで示す。
  ======================================================================================*/
integer_16 chk_limits(LL,UL,SI,CX) register BYTE *SI; BYTE LL,UL; register integer_16 CX; {
	for(;CX--;){
		if(SI[CX]>=LL && SI[CX]<=UL){ return (CX+1); }
	}
	return 0;
}


/*========== is_zengo_kanji(i) =================================================
	機能：ｉ番目の文字の前後の文字のどちらか片方または両方が漢字かどうか調べる。
	値　：＝０：前後共に漢字ではない。
		　＝１：前後のどちらかが漢字である。
  ==============================================================================*/
BOOL is_zengo_kanji(ptr) integer_16 ptr; 
{
	register BYTE *si;
	register integer_16 ptr2;
	
	if((ptr2=ptr-1)!=(-1))
	{
		si=inert+ptr2*2;
		/*ax=(*si<<8)|*(si+1); if(ax>0x3000){ return 1; }*/
		if(*si>=0x30){ return 1; }
	}
	if((ptr2=ptr+1)!=n_inert)
	{
		si=inert+ptr2*2;
		/*ax=(*si<<8)|*(si+1); if(ax>0x3000){ return 1; }*/
		if(*si>=0x30){ return 1; }
	}
	return 0;
}


/*========== before_ch_ptr(i) ================================
	機能：ｉ番目の文字の前の文字の位置を求める。
	値　：前の文字があれば文字位置、なければ−１。
		文字位置はゼロオリジンであるから、ゼロの値もとりうる。
	integer_16 before_ch_ptr(i) register integer_16 i; {
		--i;
		return i;
	}
  ============================================================*/

/*========== next_ch_ptr(i) ======================
	機能：ｉ番目の文字の次の文字の位置を求める。
	値　：次の文字があれば文字位置、なければ−１。
	integer_16 next_ch_ptr(i) register integer_16 i; {
		++i;
		if(i==n_inert){ return -1; }
		return i;
	}
  ================================================*/

/*========== first_char_ptr(i) =========================
	機能：ｉ番目の句の先頭の文字の位置を求める。
	値　：先頭の文字の位置。
	integer_16 first_char_ptr(i) integer_16 i; {
		return stc_in_ptr[i];
	}
  ======================================================*/

/*========== first_or_not(i) =====================
	機能：ｉ番目の文字が先頭文字かどうかを調べる。
	値　：先頭文字なら１、そうでなければ０。
	BOOL first_or_not(i) integer_16 i; {
		return (i==0);
	}
  ================================================*/

/*========== last_or_not(i) ======================
	機能：ｉ番目の文字が最終文字かどうかを調べる。
	値　：最終文字なら１、そうでなければ０。
	BOOL last_or_not(i) integer_16 i; {
		return (i==n_inert-1);
	}
  ================================================*/

/***************************** END OF ILE_BUFC.C *****************************/
