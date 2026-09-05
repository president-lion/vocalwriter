/**********************************************
	ILE_HNKC.C	updated at 8th January 1991
	変換メイン
 **********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_bunh.h"
#include	"ile_edic.h"
#include	"ile_envh.h"
#include	"ile_fddh.h"
#include	"ile_idic.h"
#include	"ile_kkhh.h"
#include	"ile_mruh.h"
#include	"ile_nydh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_v5ch.h"
#include	"ile_func.h"
#include "ile_rnsc.h"

static integer_16	pop_counter = 0;


/*========== CS1000(ind) =====================================
	入力：ind=0なら現在の位置から変換する。
		　ind=1なら現在の状態からバックトラックを続ける。
	機能：設定されている現在の環境から文章音声変換を続行する。
	値　：＝０	変換不能
		　＝１	変換成功
		　＝２	バックトラック回数制限で変換不能
  ============================================HnkfHenkansuru==*/
integer_16 CS1000(ind) BOOL ind; 
{
	register integer_16 si,stc_before;
	integer_16 res;
	BYTE ch;

	pop_counter=0;				/* バックトラック回数カウンタリセット。*/
	if(ind==1){ goto CS1000_BACKTRACK; }

CS1000_START:
	Stz_CE.stz_mru=0;		/* 変換開始単語の学習情報を初期化する。*/

CS1000_100:
	/* 引き続く文字列の最大長を求める。*/
	/****************************************************************
	 *	外部辞書の見出しの文字の組合せは下記の通りいろいろある。	*
	 *																*
	 *		(1)	漢字＋平仮名										*
	 *		(2)	漢字＋片仮名		(ex)京セラ						*
	 *		(3)	漢字＋英字			(ex)ＣＢＳソニー				*
	 *		(4)	漢字＋算用数字		(ex)１１０番					*
	 *		(5)	漢字＋漢数字		(ex)八戸						*
	 *																*
	 *	その他、ユーザー登録等でいろいろな組合せの言葉が外部辞書に	*
	 *	入りうるので注意する。										*
	 *																*
	 *	一方、内部辞書では、下記の３種類のみである。				*
	 *																*
	 *		(1) 漢字、 (2) 平仮名、 (3) 漢字＋平仮名				*
	 ****************************************************************/
	/* 変換する文字列がある間くり返す。*/
	
	if((celen=n_inert-ceptr)==0){ goto CS1000_SUCCESS; }

	/* このループは、初回放棄語のためのものである。*/
	while(1){
		/* 辞書引き可能な最長文字数を求める。*/
		if(INERT1000()==0){ goto CS1000_BACKTRACK; }
		do{						/* 最長一致法のループ。*/
			/****************
			 *	内部辞書	*
			 ****************/
			if(IDIC1000()){
				if(IDIC2000()){
					do{
						if(check_idic_null()==0){
							if(FOLLOW()){ res=1; goto CS1000_NEXT; }
						}
						CS1000_NXT_I: ;
					}while(IDIC3000());
				}
			}
			/****************
			 *	外部辞書	*
			 ****************/
			if(EDIC1000()){
				if(EDIC2XXX()){
					do{
						if(check_edic_null()==0){
							if(FOLLOW()){ goto CS1000_NEXT; }
						}
						res=EDIC3XXX();
						CS1000_NXT_E: ;
					}while(res);
				}
			}
			/****************
			 *	数字処理	*
			 ****************/
			if(NUMB1000()){
				StzfElmKind(&Stz_CE,DICKIND_D);
				OTHER_GRCD();
				if(FOLLOW()){
					if(NUMB5000(0)){ goto CS1000_100; }
				}
			}
			/*************++++***
			 *	片仮名語の処理	*
			 ********************/
			if(KKANA1000()){
				StzfElmKind(&Stz_CE,DICKIND_K);
				OTHER_GRCD();
				if(FOLLOW()){ goto CS1000_NEXT; }
			}
			/************************************************************
			 *	読めない熟語を単漢字辞書を引いて無理やり読ませる処理。	*
			 ************************************************************/
			/*
			 * 漢字・平仮名
			 *	初回変換モードのときは駄目。
			 *	リトライモードでは処理が必要。
			 *	初回変換モードのときに変換できた最長文字列より前ならば駄目。
			 * 片仮名・数字
			 *	無視
			 * 記号
			 *	校正モード時は、必ず読む。
			 * 英字
			 *	常に読む。
			 */
			switch(res=TDIC1000()){
			case 0:	goto CS1000_SHORTEN;
			case 1:	goto CS1000_TDIC_NONE;
			case 2:	goto CS1000_TDIC_NONE;
			}

			if(TDIC2XXX()){
				do{
					if(FOLLOW()){ goto CS1000_NEXT; }
					res=TDIC3000();
					CS1000_NXT_T: ;
				}while(res);
				goto CS1000_SHORTEN;
			}

			CS1000_TDIC_NONE:;
				/* どの辞書にもない文字。適当に処理する。*/
				TDIC4100();
				/* 種別フラグ(0xFF)は単漢字辞書にはないことを示す。*/
				/* TDIC4100()はStz_CEを壊すのでコールする順序に注意！*/
				Stz_CE.stz_dman=0xFF;
				if(FOLLOW()){ goto CS1000_NEXT; }

			CS1000_SHORTEN:;
		}while(celen=INERT2000(ceptr,celen));

		/* 初回放棄語の検索が済んでいるならば辞書語の採用を諦める。*/
		if(StzfAttrAri(Stz_CE,stz_mru,BITMRU_ABORT)){ goto CS1000_ND; }
		/* 初回放棄語の検索指示をする。*/
		StzfAttrAdd(Stz_CE,stz_mru,BITMRU_ABORT);
		/* 辞書引きを繰り返すために、引き続く文字列の最大長を改めて求める*/
		celen=n_inert-ceptr;
	}/* このループは、初回放棄語のためのものである。*/

CS1000_ND:	/* non existent in dictionary */
	/********************
	 *	片仮名語の処理	*
	 ********************
	 *celen=n_inert-ceptr;
	 *INERT1000();
	 * 片仮名以外の文字を含んでいたら短くする。*
	 *if(inert_kind[ceptr]!=CHRKIND_KKana){ goto CS1000_ND_V5; }
	 *while(1){
	 *	if(OTHER2000()){ break; }
	 *	celen=INERT2000(ceptr,celen);
	 *}
	 * 前の文字が自動発生された片仮名なら、処理しない。89.11.15 *
	 *if(ceptr!=0){
	 *	if(stc_kind[stc_ptr-1]==DICKIND_K){ goto CS1000_ND_V5; }
	 *}
	 * 片仮名語に続く片仮名語で、
	 * 撥音・長音・促音で始まっていたらエラーとする。91.1.8
	 *
	 *if(ceptr!=0){
	 *	if(inert[ceptr*2-2]==0x25){
	 *		if(inert_attr[ceptr]==CHRATTR_Soku){ goto CS1000_ND_V5; }
	 *	}
	 *}
	 *StzfElmKind(&Stz_CE,DICKIND_K);	* 単語種別を片仮名語とする。*
	 *OTHER_GRCD();						* 片仮名の文法情報を設定する。*
	 *if(FOLLOW()){ goto CS1000_NEXT; }
	 */
CS1000_ND_V5:;
	/* 直前要素がなければ五段動詞連用形語尾自動発生処理は出来ない。*/
	if(stc_ptr==0){ goto CS1000_BACKTRACK; }
	/*
	 * 直前の要素が五段動詞語幹を含むときは、連用形活用語尾が省略されたものと
	 * 仮定して、語尾を自動発生して、続きを変換してみる。
	 *
	 *	 後に語幹に接続する要素、例えば連用形語尾があっても、
	 * 連用形語尾を発生しているので、冗長である。
	 *	 しかしながら、「お話して」を「おはなしして」とも読みたいので、
	 * ここの処理はこのままにしておく必要がある。
	 *	 「行って」を「おこないって」、「いきって」とも読んでしまうが、
	 * 仕方がないと思われる。
	 */
	stc_before=stc_ptr-1;
	/*
	 *	直前要素が五段動詞語幹でなければ何もしない。
	 *	直前要素が五段動詞語幹のときは、その後端番号を保存する。
	 *	この後端番号はあとで連用形語尾を自動発生する際に必要なものである。
	 */
	if((si=chk_limits(Fdd_Header.Fdd_Bno5danLL,Fdd_Header.Fdd_Bno5danUL,
					  &stc_bno[stc_before][0],stc_nbno[stc_before]))==0){ goto CS1000_BACKTRACK; }
	flg_v5you=stc_bno[stc_before][si-1];	/* 五段動詞の後端番号 */
	/*
	 * 語幹の最後の文字が漢字でない五段動詞連用形省略形はありえない。
	 *		「うごき」・「鳴かし」、はダメ。「ぶち壊し」、はＯＫ。
	 */
	if(inert_kind[stc_in_ptr[stc_before]+stc_in_len[stc_before]-1]!=CHRKIND_Kanji){
		goto CS1000_BACKTRACK;
	}
	/* 語幹の直後が句読点のときは省略形とはみなさないことにする。*/
	ch=inert_kind[ceptr] & 0xF0;
	if(ch==CHRKIND_Other1||ch==CHRKIND_Other2){ goto CS1000_BACKTRACK; }
	/* 語幹の直後が漢字以外のときは省略形とはみなさないことにする。*/
	ch=inert_kind[ceptr] & 0xF0;
	if(ch!=CHRKIND_Kanji){ goto CS1000_BACKTRACK; }
	/* 連用形活用語尾を自動発生して、続きを変換してみる。*/
	V5DANYOU_PUSH();
	StzfElmForward();		/* 要素接続関連情報を１要素分進める。*/
	goto CS1000_START;

CS1000_NEXT:;
	/*
	 * 現在注目要素が助数詞でかつ先行要素が数字ならば、
	 * 数字を強制的に桁読みとする。
	 * 但し、校正モードでは、この処理は行わない。
	 */
	if(onsei_addr->onsei_kouseisystem==0 &&
	   Stz_CE.stz_kind==DICKIND_E && stc_ptr>1){
		if(chk_limits(Fdd_Header.Fdd_GrcdNum1,Fdd_Header.Fdd_GrcdNum2,
					  Stz_CE.stz_grcd,Stz_CE.stz_ngrcd)){
			if(chk_value(Fdd_Header.Fdd_GrcdDigit,
						 Stz_PE.stz_grcd,Stz_PE.stz_ngrcd)){
				NUMB5XXX(1);
			}
		}
	}
	OMEGA5000();			/* 接続可能なら現在要素の出力結果を求める。*/
	STACK_PUSH();			/* 現在要素の情報をstackに積む。*/
	ceptr+=celen;			/* 次の入力データ位置 */
	/*
	 *	　現在注目要素が助数詞でかつ先行要素が数字ならば、
	 *	数字の読み方と助数詞の読み方が変化することがある。
	 *	　この場合、スタックの中身までも変化する可能性が多いので、
	 *	特に注意を要する。
	 *	　助数詞は外部辞書以外には存在しない。
	 */
	if(Stz_CE.stz_kind==DICKIND_E && stc_ptr>1){
		if(chk_limits(Fdd_Header.Fdd_GrcdNum1,Fdd_Header.Fdd_GrcdNum2,
					  Stz_CE.stz_grcd,Stz_CE.stz_ngrcd)){
			if(chk_value(Fdd_Header.Fdd_GrcdDigit,
						 Stz_PE.stz_grcd,Stz_PE.stz_ngrcd)){
				DIGIT7000();
			}
		}
	}

	StzfElmForward();		/* 単語接続関連情報を１単語進める。*/
	goto CS1000_100;		/* 接続可能な間繰り返す。*/

CS1000_SUCCESS:;
	ACCXXXX();
	/*
	 *	変換成功のときはmax_ceptrをゼロ以外にしておく。
	 *	初回モード以外の時は、max_ceptrは変えてはならない。
	 *	これは、pron.cで用いられる。-- 但し、88/2/23現在では未使用。
	 */
	if(conv_mode==CONV_FIRST){ max_ceptr=n_inert; }
	return 1;

CS1000_BACKTRACK: ;
	/************************************************************************
	 *	　数字列の中で切り方に誤りがあっても次候補の処理は行わない。		*
	 *	多分、そのようなことは普通の数字列においてはありえないと思われる。	*
	 *	将来、問題が生じた場合には、改めて考え直すこととする。				*
	 ************************************************************************/

	/*
	 *	変換できた最長のものをｓａｖｅする。
	 *	ただし、現在要素が最後になりえないときはｓａｖｅしない。
	 */
	if(conv_mode==CONV_FIRST && ceptr>max_ceptr){
		/*
		 *	直前が自動発生した連用形語尾ならば、それは切れ目にはなりえない。
		 *	この修正はV5DAN_PUSH()で行う。即ち、BITCTL_LASTをクリアする。
		 */
		/*$if(stc_kind[stc_ptr-1]!=DICKIND_5Y){*/
			/* 直前が最後になりうる項目のときだけsaveする。*/
			if(stc_ctl[stc_ptr-1] & BITCTL_LAST){
				max_ceptr=ceptr; /*max_cpron_ptr=cpron_ptr;*/
			}
		/*$fi*/
	}

 	/* 先頭の語でなければ、*/
	while(ceptr!=0){
		/*
		 *	注目文節長固定処理をするときに、
		 *	もう既に注目文節よりも前に戻っているならば、変換不能とする。
		 */
		if(conv_mode==CONV_NEXT||conv_mode==CONV_SHORT||conv_mode==CONV_LONG){
			if(ceptr<=wsptr){ return 0; };
		}

		/* バックトラック回数を制限して、変換できないものの変換速度を向上する。*/
		/* 初回変換以外はｐｏｐ回数でアボートしない。*/
		if(conv_mode==CONV_FIRST){ ++pop_counter; }
		if(pop_counter>MAX_POP_TIMES){ goto CS1000_TIME; }

		/* stackの内容を１つ戻す。*/
		STACK_POP();

		/************************************************************************
		 *	　数字は後続する助数詞によって、読み方が変わったり、				*
		 *	最終拍の促音化を引き起こしたりするので、バックトラックが発生したら、*
		 *	数字の読み方やアクセントをもとに戻す必要がある。					*
		 *	　例えば、「２０度」の「度」には、「たび」と「ど」の２通りの		*
		 *	助数詞があるが、初回に「たび」が出て「２０」が促音化したあとに、	*
		 *	次候補で「ど」が出ると、「２０」は促音化したままであるから、		*
		 *	「にじゅっど」となってしまう。										*
		 *	　従って、スタックをポップしたのち、スタックのトップが数字のときは、*
		 *	さらに数字をポップして、しかるのちに数字を再変換する。				*
		 *	ここで、助数詞の接続情報は保存しておかねばならない。				*
		 ************************************************************************/

		/* 先行要素が数字ならそれを変換し直す。*/
		if(stc_ptr){ NUMB5XXX(0); }

		/* 戻した要素が内部辞書起源ならば、内部辞書探索の継続処理に移る。*/
		if(Stz_CE.stz_kind==DICKIND_I){
			push_null(ceptr,idic_addr,0);
			goto CS1000_NXT_I;
		}

		/* 戻した要素が外部辞書起源ならば、*/
		if(Stz_CE.stz_kind==DICKIND_E){
			push_null(ceptr,edic_ofs,edic_page_no);
			/* 外部辞書を引きなおし、次の同字異音語を探す。*/
			RnsfReadEdic(edic_page_no,&edic_page_addr);
			res=EDIC3XXX();
			goto CS1000_NXT_E;
		}

		/* 戻した要素が単漢字辞書起源ならば、*/
		if(Stz_CE.stz_kind==DICKIND_T){
			if(Stz_CE.stz_dman==0xFF){
				/* 本当に読めない文字は次候補なんかある筈がない。*/
				res=0;
			}else{
				/* 単漢字辞書を引きなおし、次の読み方を探す。*/
				RnsfReadEdic(edic_page_no,&edic_page_addr);
				res=TDIC3000();
			}
			goto CS1000_NXT_T;
		}
		/*
		 *	　戻した要素が辞書以外の自動発生したものであるときは、
		 *	長さを変えてみる。
		 *	　片仮名・数字・特殊文字は、相反する種類であるので、
		 *	どれかで駄目なら他でも駄目である。
		 */
		/* 片仮名語は次候補が存在しないので、もうひとつｐｏｐする。*/
		if(Stz_CE.stz_kind!=DICKIND_K){
			goto CS1000_SHORTEN;
		}
	}

	CS1000_FAIL:;	return 0;	/* 変換不能状態をしらせる。*/
	CS1000_TIME:;	return 2;	/* ｐｏｐ回数制限 */
}


void ACCXXXX() {
	PAUSE1000();	/* 連用・連体修飾格、終止形、等の分類と休止時間の決定。*/
	/*
	 * ACC1000..ACC7000,OMEGA9000,PAUSE1000のコールする順序に注意すること。
	 */
	ACC4000();		/* 撥音・促音の分類。*/
	ACC1000();		/* 基本アクセントの設定。*/
	ACC2000();		/* アクセント結合。*/

	/*PAUSE1000();	連用・連体修飾格、終止形、等の分類と休止時間の決定。*/
	ACC_JOSI_NO();	/* ポーズがないとき格助詞「の」が高ければ次の拍が高くなる。*/
	ACC5000();		/* 文節のアクセント結合。*/
	ACC_PHRASE();	/* ひとつの「うねり」の上に乗る句の区切りを決定する。*/

	ACC7000();		/*
					 * 連用終止の例外的処理。滝が１拍前に移動する。
					 * 「察し」、「信じ」のときは、モーラ音素を含むので、
					 * さらに一拍移動するが、この処理はACC3000()で行う。
					 */
	ACC3000();		/* アクセントの滝が撥音・促音・長音の直後に有る場合の処理。*/
	ACC6000();		/* 文節先頭の鼻濁音を口濁音に変換、および、母音の無声化。*/
	ACC8000();		/* レベル９（疑問文）の拍を決定する。*/

	ACC_OUTPUT();
	OMEGA9000();	/* 変換結果を呼び出しプログラムに渡す。*/
}

/***************************** END OF ILE_HNKC.C *****************************/
