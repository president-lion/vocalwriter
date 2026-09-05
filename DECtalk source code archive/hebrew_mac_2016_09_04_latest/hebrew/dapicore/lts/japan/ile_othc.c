/**********************************************
	ILE_OTHC.C	updated at 8th January 1991
	ÆÃ¼ìÊ¸»ú¡¦ÊÒ²¾Ì¾¸ìÊÑ´¹
 **********************************************/

#define		EXT		extern

#include	"onsei.h"
#include	"ile_envh.h"
#include	"ile_errh.h"
#include	"ile_fddh.h"
#include	"ile_kkhh.h"
#include	"ile_nydh.h"
#include	"ile_shdh.h"
#include	"ile_stch.h"
#include	"ile_stzh.h"
#include	"ile_func.h"

#include	"ile_mruh.h"

/*========== KKANA1000() ===================================================
	µ¡Ç½¡§°ÌÃÖceptr¤«¤éÊ¸»ú¿ôcelen¤ÎÊ¸»úÎó¤¬ÊÒ²¾Ì¾ÊÑ´¹¤Ç¤­¤ë¤«¤É¤¦¤«Ä´¤Ù¤ë¡£
	ÃÍ¡¡¡§£±¡á²ÄÇ½À­Í­¤ê¡¿£°¡á²ÄÇ½À­Ìµ¤·
  ==========================================================================*/
BOOL KKANA1000() 
{
	BYTE al;

	/* ½é²óÊü´þ¸ì¸¡º÷»ØÄê¤Ç¤Ê¤¤¤È¤­¤Ï¡¢½èÍý¤·¤Ê¤¤¡£*/
	if((Stz_CE.stz_mru & BITMRU_ABORT)==0){ return 0; }

	/* ÊÒ²¾Ì¾°Ê³°¤ÎÊ¸»ú¤ò´Þ¤ó¤Ç¤¤¤¿¤é¡¢²ÄÇ½À­Ìµ¤·¡£*/
	if((al=OTHER2000())==0){ return 0; }
	if(al!=CHRKIND_KKana){ return 0; }

	/* Á°¤ÎÊ¸»ú¤¬ÊÒ²¾Ì¾¤Ê¤é¡¢½èÍý¤·¤Ê¤¤¡£(91.1.8}Ã½Ä|)*/
	if(ceptr!=0){
		if(*(WORD *)&inert[ceptr*2-2]==0x3C21){
			if(inert[ceptr*2-4]==0x25){ return 0; }
		}else{
			if(inert[ceptr*2-2]==0x25){ return 0; }
		}
	}

	/* Á°¤ÎÊ¸»ú¤¬¼«Æ°È¯À¸ÊÒ²¾Ì¾¤Ê¤é¡¢½èÍý¤·¤Ê¤¤¡£*/
	if(ceptr!=0){
		if(stc_kind[stc_ptr-1]==DICKIND_K){ return 0; }
	}

	/* Ùû²»¡¦Ä¹²»¡¦Â¥²»¤Ç»Ï¤Þ¤Ã¤Æ¤¤¤¿¤é¡¢Ä¾Á°¤¬ÊÒ²¾Ì¾°Ê³°¤Ê¤é½èÍý¤¹¤ë¡£*/
	if(inert_attr[ceptr]==CHRATTR_Soku){
		/* ÀèÆ¬¤Ê¤é¡¢½èÍý¤¹¤ë¡£*/
		if(ceptr==0){ return 1; }
		/* Ä¾Á°¤¬ÊÒ²¾Ì¾°Ê³°¤Ê¤é½èÍý¤¹¤ë¡£*/
		if(inert_kind[ceptr-1]!=CHRKIND_KKana){ return 1; }
		return 0;
	}
	return 1;
}


/*========== KKANA5000(ptr) ============================================
	µ¡Ç½¡§¸½ºßÃíÌÜ¸ì³«»Ï°ÌÃÖ¤«¤é»Ï¤Þ¤ëÊÒ²¾Ì¾¸ì¤ÎÈ¯²»¤ò½ÐÎÏ¥Ç¡¼¥¿ÎÎ°è¤Î
 		¥ª¥Õ¥»¥Ã¥Èdi¤«¤é»Ï¤Þ¤ë°ÌÃÖ¤Ë³ÊÇ¼¤¹¤ë¡£
  ======================================================================*/
integer_16 KKANA5000(di) integer_16 di; {
	register BYTE *si;
	register integer_16 cx;
	integer_16 last_chouon;
	BYTE ch;

	si=(&stc_accent_raw[stc_ptr][0]);
	/*
	 *	ÊÒ²¾Ì¾¸ì¤ò¥â¡¼¥é¥Ç¡¼¥¿¤ËÊÑ´¹¤¹¤ë¡£
	 *	ÊÑ´¹¤Ç¤­¤Ê¤±¤ì¤Ð½èÍý¤Ç¤­¤Ê¤¤¡£
	 *	kkana_len,kkana_buf¤Ë¥â¡¼¥é¥Ç¡¼¥¿¤¬³ÊÇ¼¤µ¤ì¤ë¡£
	 */
	if(kana_mora()==0){ jp_ERROR(Err_HenkanFunou); }

	/* Ùû²»¡¦Ä¹²»¡¦Â¥²»¤Ç»Ï¤Þ¤Ã¤Æ¤¤¤¿¤éÃí°Õ¤¹¤ë¡£*/
	/* Ä¹²»¤Ïkana_mora()¤Ç½èÍý¤¹¤ë¡£¼ÂºÝ¤ÏÆÉ¤Þ¤Ê¤¤¡£*/
	if(kkana_buf[0]==0xCF){ kkana_buf[0]=0x32; }

	/* ºÇ¸å¤ÎÊ¸»ú¤¬Ä¹²»¤«¤É¤¦¤«Ä´¤Ù¤ë¡£*/
	last_chouon=(kkana_buf[kkana_len-1]==0xC0 ? 1:0);
	cx=kkana_len-last_chouon;
	/*
	 *	£µÇï°Ê¾å¤Ê¤é¥¢¥¯¥»¥ó¥È¤ÎÂì¤Ï¸å¤«¤é£³ÇïÌÜ¡£Ã¢¤·ºÇ¸å¤ÎÄ¹²»¤ÏÌµ»ë¤¹¤ë¡£
	 *	£³Çï¤ÇÄ¹²»¤Ç½ª¤ï¤ë¸ì¤Ï¥¢¥¯¥»¥ó¥È¤ÎÂì¤Ï£²ÇïÌÜ¡£¡Ê¥°¥ì¡¼¡¢¥Õ¥ê¡¼¡¢¥¿¥Ö¡¼¡Ë
	 *	£³Çï°Ê²¼¤Ê¤éÆ¬¹â¡£
	 */
	if(cx>=5){ goto KKANA5000_NAKA3; }
	if(kkana_len==3 && last_chouon){ goto KKANA5000_NAKA2; }
	if(cx<=3){ goto KKANA5000_ATAMA; }
	/*
	 *	£´Çï¸ì¤Ç£³ÇïÌÜ¤¬¥â¡¼¥é²»ÁÇ¤Ê¤é¤Ð¥¢¥¯¥»¥ó¥È¤ÎÂì¤Ï£²ÇïÌÜ¡¢
	 *	¤µ¤â¤Ê¤±¤ì¤Ð¡¢Æ¬¹â¡£
	 */
	ch=kkana_buf[2];
	if(0xC0==ch||0xCF==ch||0xD7==ch){ goto KKANA5000_NAKA2; }

KKANA5000_ATAMA:	*si=1; goto KKANA5000_500;
KKANA5000_NAKA2:	*si=2; goto KKANA5000_500;
KKANA5000_NAKA3:	*si=(last_chouon? kkana_len-3:kkana_len-2);
KKANA5000_500:
	*(++si)=0;		/* stc_accent_raw[stc_ptr][1]=0 */
	*(++si)=0;		/* stc_accent_raw[stc_ptr][2]=0 */

	/* È¯²»¥Ç¡¼¥¿¤ò½ÐÎÏÎÎ°è¤Ø¥³¥Ô¡¼¤¹¤ë¡£*/
	for(cx=cpron_len=kkana_len,si=kkana_buf; cx--; ){
		di=OMEGA1000(*si++,di);
	}
	return di;
}


/*========== OTHER2000() =============================================================
	µ¡Ç½¡§°ÌÃÖceptr¤«¤éÊ¸»ú¿ôcelen¤ÎÊ¸»úÎó¤¬£²¤Ä°Ê¾å¤Î¼ïÎà¤ÎÊ¸»ú¤ò´Þ¤à¤«¤É¤¦¤«Ä´¤Ù¤ë¡£
	ÃÍ¡¡¡§¡á£°¡§£²¤Ä°Ê¾å¤Î¼ïÎà¤ò´Þ¤à¡£¡¿¡â£°¡§Ê¸»ú¤Î¼ïÎà¤òÊÖ¤¹¡£¡Ê¾å°Ì£´¥Ó¥Ã¥È¡Ë
  ====================================================================================*/
WORD OTHER2000() {
	/* Ê¸»ú¤Î¼ïÎà¤È¤Ï¡¢¾å°Ì£´¥Ó¥Ã¥È¤Î¤³¤È¤Ç¤¢¤ë¡£*/
	register BYTE *bx; BYTE al; register integer_16 cx;

	cx=celen; bx=(&inert_kind[ceptr]);
	
	/* Ê¸»ú¤Î¼ïÎà¤Ï¾å°Ì£´¥Ó¥Ã¥È¤ÇÈæ³Ó¤¹¤ë¡£*/
	al=(*bx) & 0xF0; ++bx; --cx;
	for(;cx--;bx++){
		if(al!=(*bx & 0xF0)){ return 0; }
	}
	return al;
}


/*========== OTHER5000(ptr) ========================================
	µ¡Ç½¡§¸½ºßÍ×ÁÇ³«»Ï°ÌÃÖ¤«¤é»Ï¤Þ¤ëÃ±½ã¸ì¤ÎÈ¯²»¤ò½ÐÎÏ¥Ç¡¼¥¿ÎÎ°è¤Î
 		¥ª¥Õ¥»¥Ã¥Èptr¤«¤é»Ï¤Þ¤ë°ÌÃÖ¤Ë³ÊÇ¼¤¹¤ë¡£
  =================================================TjgfKekMotomeru==*/
integer_16 OTHER5000(di) register integer_16 di; {
	di=OMEGA1000(0xFF,di);
	cpron_len=1;			/* ¤È¤ê¤¢¤¨¤º¡¢ÆÉ¤ß¤ÏÂ¸ºß¤·¤Ê¤¤¤³¤È¤Ë¤·¤Æ¤ª¤¯¡£*/
	return di;
}


/*========== OTHER_GRCD() =========================================
	µ¡Ç½¡§¸½ºßÃíÌÜ¸ì³«»Ï°ÌÃÖ¤«¤é»Ï¤Þ¤ëÃ±½ã¸ì¤ÎÊ¸Ë¡¾ðÊó¤òÀßÄê¤¹¤ë¡£
	Ãí°Õ¡§Ê¿²¾Ì¾¡¿ÊÒ²¾Ì¾¸ì¤Ï¡¢³°Íè¸ì°·¤¤¤ò¤¹¤ë¡£
  =================================================================*/
void OTHER_GRCD() 
{
	/*
	 *	»ú¼ï¤Ë¤è¤êÊ¸Ë¡¥³¡¼¥É¡¢µÚ¤Ó¸åÃ¼ÈÖ¹æ¤ò·è¤á¤ë¡£
	 *	¡ÊÎã³°¡ËÄ¹¤µ¤¬£±¤Ç¾®¿ôÅÀ¤Î¤È¤­¤Ï¡¢Other2¤È¤ß¤Ê¤¹¡£
	 *			¡Ê£³¡¿¡¥¡¿£µ¿Í¤Ï¡Ä¡Ë
	 */
	if(celen==1 && inert_kind[ceptr]==CHRKIND_DDP){ goto OTHER_GRCD_120; }

	switch(inert_kind[ceptr] & 0xF0){
	case CHRKIND_Other1:	/* ÀèÆ¬²ÄÇ½Ê¸»ú */
		StzfGrcdAdd(Fdd_Header.Fdd_GrcdToku1);
		StzfGrcdStore(Fdd_Header.Fdd_GrcdUsr);
		break;
	case CHRKIND_Other2:	/* ÀèÆ¬ÉÔ²ÄÊ¸»ú */
		OTHER_GRCD_120:
		/* ÀèÆ¬ÉÔ²ÄÊ¸»ú¤Ç¤â¡¢ÆþÎÏÊ¸»úÎó¤ÎÀèÆ¬¤Ê¤é¡¢ÀèÆ¬²ÄÇ½Ê¸»ú¤È¤·¤Æ°·¤¦¡£*/
		if(ceptr==0){
			StzfGrcdAdd(Fdd_Header.Fdd_GrcdToku1);
			StzfGrcdStore(Fdd_Header.Fdd_GrcdUsr);
		}else{
			StzfGrcdAdd(Fdd_Header.Fdd_GrcdToku2);
		}
		break;
	case CHRKIND_Digit:
		StzfGrcdAdd(Fdd_Header.Fdd_GrcdDigit);
		break;
	case CHRKIND_KKana:
		StzfGrcdAdd(Fdd_Header.Fdd_GrcdGai);
		StzfGrcdAdd(Fdd_Header.Fdd_GrcdUsr);
		StzfGrcdAdd(36);	/* }µÝ|,}Îß°½Þ|,}Ì§²Ù |*/
		StzfGrcdAdd(88);	/* }Æ­°Ö°¸|,}Û°Ï |*/
		StzfGrcdAdd(123);	/* }µÌ |*/
		StzfGrcdAdd(126);	/* }µÌ |*/
		break;
	}
}

/***************************** END OF ILE_OTHC.C *****************************/
