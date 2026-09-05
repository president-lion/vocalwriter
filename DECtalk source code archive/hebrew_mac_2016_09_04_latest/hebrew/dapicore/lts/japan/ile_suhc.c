/*********************************************
	ILE_SUHC.C	updated at 17th April 1990
	算用数字 vs 漢数字変換
 *********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_envh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_func.h"

static const BYTE SuhwKan[10][2];
static const BYTE SuhwKan2[3][2];
static const BYTE SuhwKurai[19][2];


/*========== DIGIT_CONV(sw) ==================================================
	機能：漢数字・算用数字・符号・小数点の混ざった数字列を漢数字列に変換する。
		小数点より前は整数型扱い、後は棒読み扱いとする。
		１０万→十万，１２．３４→十二．三四
		　棒読みモードのときは、全て、棒読み扱いとする。(digit_type =1)
	入力：ｓｗ＝０：読み方指定なし。
		　ｓｗ＝１：桁読み指定。
		　ｓｗ＝２：棒読み指定。
  ============================================================================*/
void DIGIT_CONV(sw) integer_16 sw; {
	register integer_16 ptr,ikind;
	integer_16 cdptr,cdlen,maxlen;
	BYTE al,*kptr;

	digit_type=0;	/* normal type (integer,fraction) */
	digit_mode=0;	/* convert mode (integer) */
	n_kansuji=0;

	if(sw==1){ goto DIGIT_CONV_200; }
	if(sw==2){ digit_type=1; goto DIGIT_CONV_200; }

	if(onsei_addr->onsei_kouseisystem){
		/* 校正モードなら、必ず、棒読みとする。--89.11.29-- */
		digit_type=1; goto DIGIT_CONV_200;
	}

	/* 数字列に小数点やコンマが含まれているときは、棒読み型にはなりえない。*/
	/* 棒読み型になるものは、算用数字と漢数字１（一二三…〇）のみ含みうる。*/
	for(ptr=ceptr;ptr<ceptr+celen;++ptr){
		if((inert_kind[ptr]!=CHRKIND_KDigit1)
		&& (inert_kind[ptr]!=CHRKIND_ADigit)){
			/* 算用数字・漢数字１以外を含むときは、棒読み型とはしない。*/
			goto DIGIT_CONV_200;
		}
	}

	/* 数字列の前後が（or）or−ならば、棒読み型とする。*/
	/* 235-3674, (235)3674, etc.*/
	if((ptr=ceptr-1)!=(-1)){
		ikind=inert[ptr*2]*256+inert[ptr*2+1];
		if(ikind==0x214A || ikind==0x214B || ikind==0x215D){
			digit_type=1;
		}
	}
	if((ptr=ceptr+celen)!=n_inert){
		ikind=inert[ptr*2]*256+inert[ptr*2+1];
		if(ikind==0x214A || ikind==0x214B || ikind==0x215D){
			digit_type=1;
		}
	}

	/* 数字列がゼロで始まっていたら、棒読み型とする。*/
	/* 03, 0990, etc.*/
	ikind=inert[ceptr*2]*256+inert[ceptr*2+1];
	if(ikind==0x213b || ikind==0x2330){	/* 〇 or ０ */
		digit_type=1;
	}

DIGIT_CONV_200:;
	ptr=ceptr;

	while(1){
		if(ptr>=ceptr+celen){ return; }

		ikind=inert_kind[ptr];
		if(ikind==CHRKIND_ADigit || ikind==CHRKIND_KDigit1){
			cdptr=ptr; cdlen=1;
			kptr=(&inert_kind[cdptr+cdlen]);
			while(*kptr==CHRKIND_ADigit || *kptr==CHRKIND_KDigit1
			   || *kptr==CHRKIND_DSep){
				++cdlen; ++kptr;
			}
			maxlen=ceptr+celen-cdptr;
			if(cdlen>maxlen){ cdlen=maxlen; }
			DIGIT_CONV1(cdptr,cdlen);
			ptr+=cdlen;
		}else{
			/* if decimal point is found, convert mode changed */
			if(inert_kind[ptr]==CHRKIND_DDP){ digit_mode=1; }
			kansuji[n_kansuji*2]  =inert[ptr*2];
			kansuji[n_kansuji*2+1]=inert[ptr*2+1];
			++n_kansuji; ++ptr;
		}
	}
}


/*========== DIGIT_CONV1(cdptr,cdlen) ==============================
	入力：　cdptr					変換する文字列のオフセット
 			cdlen					変換する文字数
			digit_mode				=0 : integer type
									=1 : fraction type
			digit_type				=0 : normal mode
									=1 : 棒読み type
	出力：　kansuji[]				変換結果
 			n_kansuji				変換結果の文字数
	機能：　現在語を標準形の数字に変換する。（最大20桁）
			20桁以上の時は棒読み型とする。　例：一二三四五
	注意：　入力文字には数字列のセパレーター（コンマ等）が含まれる。
  ==================================================================*/
void DIGIT_CONV1(cdptr,cdlen) integer_16 cdptr,cdlen; {
	/*
	 *	len		: length concludes comma
	 *	net_len	: length without comma
	 */
	register BYTE *si; BYTE ah,al;
	register integer_16 len; integer_16 net_len,dx;

	/* 棒読み型または小数型 */
	if(digit_mode==1||digit_type==1){ goto DIGIT_CONV1_500; }

	/* １桁及び２１桁以上は、棒読み型とする。*/
	net_len=0;
	for(dx=0;dx<cdlen;++dx){
		if(inert_kind[cdptr+dx]==CHRKIND_ADigit
		|| inert_kind[cdptr+dx]==CHRKIND_KDigit1){
			++net_len;
		}
	}
	if(net_len>20 || net_len==1){ goto DIGIT_CONV1_500; }

	/* １．整数型 */
	len=cdlen;						/* 変換する文字数を取り出す */
	si=cdptr*2+inert;				/* 変換する文字列のアドレスを取り出す */

	dx=0;
	do{
		ah=(*si++); al=(*si++);

		/* 漢数字ならば、算用数字に変換する。*/
		DIGIT_CONV2(&ah,&al);
		/* 数字以外は無視する */
		if(ah!=0x23 || al<0x30 || al>0x39){ goto DIGIT_CONV1_290; }

		/* 変換する文字がゼロの場合 */
		if(al==0x30){
			/* 位が億・万などのとき、位だけをつける */
			if((net_len & 3)==1){ if(dx){ goto DIGIT_CONV1_240; } }
			/* 変換する文字数が１以外の時は変換しない */
			if(n_kansuji ||(net_len!=1)){ goto DIGIT_CONV1_270; }
		}
		dx++;
		/* 変換する文字が１で位が百か十でないなら */
		if( (al!=0x31) || ((net_len & 2)==0) ){
			/* 表を引いて数字を求める */
			kansuji[n_kansuji*2  ]=SuhwKan[al-0x30][0];
			kansuji[n_kansuji*2+1]=SuhwKan[al-0x30][1];
			n_kansuji++;				/* 変換結果を格納する */
		}
		DIGIT_CONV1_240:
		if(net_len!=1){	/* １の位でないなら */
			/* 位変換テーブルのアドレスをセット */
			kansuji[n_kansuji*2  ]=SuhwKurai[net_len-2][0];
			kansuji[n_kansuji*2+1]=SuhwKurai[net_len-2][1];
			n_kansuji++; /* 変換結果を格納する */
		}
		DIGIT_CONV1_270:
		if(((net_len & 02)==0) && (net_len & 1)){ dx=0; }
		--net_len;
		DIGIT_CONV1_290:;
	}while(--len);
	return;

/* ２．棒読み型 */
DIGIT_CONV1_500:;
	len=cdlen;						/* 変換する文字数を取り出す */
	si=cdptr*2+inert;				/* 変換する文字列のアドレスを取り出す */
	do{
		ah=(*si++); al=(*si++);

		/* 漢数字ならば、算用数字に変換する。*/
		DIGIT_CONV2(&ah,&al);
		/* 数字以外は無視する */
		if(ah==0x23 && al>=0x30 && al<=0x39){
			kansuji[n_kansuji*2  ]=SuhwKan[al-0x30][0];
			kansuji[n_kansuji*2+1]=SuhwKan[al-0x30][1];
			n_kansuji++;
		}
	}while(--len);
	return;
}


/*========== DIGIT_CONV2(ah,al) ====================================
	入力：　ah,al					全角数字
	出力：　ah,al					全角数字
	機能：　数字が漢数字ならば、それを算用数字に変換する。
  ==================================================================*/
void DIGIT_CONV2(ah,al) BYTE *ah,*al; {
	integer_16 cx;
	const BYTE* si;
	/* 〇一二三四五六七八九 */
	for(si= &SuhwKan[0][0],cx=0; cx<10; ++cx,si+=2){
		if(*ah==(*si) && *al==(*(si+1))){
			*ah=0x23; *al=0x30+cx; return;
		}
	}
	/* 壱弐参 */
	for(si= &SuhwKan2[0][0],cx=0; cx<3; ++cx,si+=2){
		if(*ah==(*si) && *al==(*(si+1))){
			*ah=0x23; *al=0x31+cx; return;
		}
	}
}

/* 漢数字 （〇一二三四五六七八九）*/
static const BYTE SuhwKan[10][2]={
	0x21,0x3b,0x30,0x6c,0x46,0x73,0x3b,0x30,0x3b,0x4d,
	0x38,0x5e,0x4f,0x3b,0x3c,0x37,0x48,0x2c,0x36,0x65
};

/* 漢数字 （壱弐参）*/
static const BYTE SuhwKan2[3][2]={
	0x30,0x6d,0x46,0x75,0x3b,0x32
};

/* 位 （十百千万十百千億十百千兆十百千京十百千）*/ 
static const BYTE SuhwKurai[19][2]={
	0x3d,0x3d,0x49,0x34,0x40,0x69,0x4b,0x7c,
	0x3d,0x3d,0x49,0x34,0x40,0x69,0x32,0x2f,
	0x3d,0x3d,0x49,0x34,0x40,0x69,0x43,0x7b,
	0x3d,0x3d,0x49,0x34,0x40,0x69,0x35,0x7E,
	0x3d,0x3d,0x49,0x34,0x40,0x69
};


/***************************** END OF ILE_SUHC.C *****************************/
