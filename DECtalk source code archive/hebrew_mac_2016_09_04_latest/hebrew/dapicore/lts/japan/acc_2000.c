/*********************************************
	ACC_2000.C	updated at 17th April 1990
	¥¢¥¯¥»¥ó¥È·ë¹ç½èÍý¤Î¥á¥¤¥óÉôÊ¬
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
#include "ile_accd.h"

/*============ ACC2000() =================================================
	µ¡Ç½¡§ÎÙÀÜ¤·¤¿£²Í×ÁÇ¤Î¥¢¥¯¥»¥ó¥È¤ò·ë¹ç¤¹¤ë¡£

	¡¡Ê¸¤Î¹½À®Í×ÁÇ¤Ï°Ê²¼¤Î¼ïÎà¤ËÊ¬Îà¤¹¤ë¡£

	£±¡¥Ï¢ÂÎ»ì				¡§¤³¤Î¡¢°ì¿È¾å¤Î¡¢¤É¤ó¤Ê
	£²¡¥ÀÜÆ¬¼­				¡§Âç¡¢Åì¡¢Ä¶¡¢¹ñºÝ¡¢Ëö¾¿
	£³¡¥ÀÜÈø¼­				¡§¼Ô¡¢ÍÑ¡¢Ä¹¡¢Ä®
	£´¡¥Æ°»ì¸ì´´			¡§Æ°¡Ê¤¯¡Ë¡¢µï¡Ê¤ë¡Ë
	£µ¡¥¸ÞÃÊÆ°»ìÏ¢ÍÑ·Á¸ìÈø	¡§¡ÊÆ°¡Ë¤­
	£¶¡¥·ÁÍÆ»ì¸ì´´			¡§Èþ¤·¡Ê¤¤¡Ë
	£·¡¥£´¡¦£¶°Ê³°¤Î¼«Î©¸ì	¡§¸À¸ì¡¢¹ñÊ¬¡¢Åìµþ¡¢Âç¤­¤Ê¡¢¤É¤ó¤Ê¡¢åºÎï
	£¸¡¥£µ°Ê³°¤ÎÉÕÂ°¸ì		¡§¡ÊÈþ¤·¡Ë¤¤¡¢¡ÊÆ°¤­¡Ë¤Þ¤¹¡¢½õÆ°»ì¡¢½õ»ì
	£¹¡¥±Ñ»ú	¥¢¥¯¥»¥ó¥È³Ë¤¬¤Ò¤È¤Ä¤À¤±¤Î¤â¤Î
¡¡£±£°¡¥¿ô»ú	¥¢¥¯¥»¥ó¥È³Ë¤¬¤Ò¤È¤Ä¤À¤±¤Î¤â¤Î
¡¡£±£±¡¥½õ¿ô»ì
¡¡£±£²¡¥Ã±´Á»ú¼­½ñµ¯¸»¤Î¸ì

	¡¡·ë¹ç¤Î½ç½ø¤Ï°Ê²¼¤ÎÄÌ¤ê¤È¤¹¤ë¡£

	£°¡¥Ã±´Á»ú¡ÜÃ±´Á»ú		¢ª¼«Î©¸ì
	£±¡¥±Ñ»ú¡Ü±Ñ»ú			¢ª¼«Î©¸ì
	£²¡¥¿ô»ú¡Ü½õ¿ô»ì		¢ª¿ô»ú
	£³¡¥¿ô»ú¡Ü¾®¿ôÅÀ		¢ª¿ô»ú
	£´¡¥¿ô»ú¡Ü¿ô»ú			¢ªÉû»ì
	£µ¡¥Æ°»ì¡ÜÏ¢ÍÑ·Á¸ìÈø	¢ªÆ°»ì
	£¶¡¥Ê£¹çÆ°»ì¡¦Ê£¹ç·ÁÍÆ»ì
	£·¡¥ÀÜÆ¬¼­¡Ü¼«Î©¸ì		¢ª¼«Î©¸ì
	£¸¡¥¼«Î©¸ì¡Ü¼«Î©¸ì		¢ª¼«Î©¸ì
	£¹¡¥¼«Î©¸ì¡ÜÀÜÈø¼­		¢ª¼«Î©¸ì
¡¡£±£°¡¥Ï¢ÂÎ»ì¡Ü¼«Î©¸ì		¢ª¼«Î©¸ì
¡¡£±£±¡¥¼«Î©¸ì¡ÜÉÕÂ°¸ì		¢ª¶ç
¡¡£±£²¡¥Éû»ì¡ÜÉÕÂ°¸ì		¢ª¶ç

	¡¡¿ô»ú¡¦±Ñ»ú¤Î°·¤¤

	¡¡±Ñ»ú¤ÏÆó»ú¤º¤Ä¤Þ¤È¤á¤Æ¥¢¥¯¥»¥ó¥È¤ò¤Õ¤Ã¤Æ¤¤¤¯¡£
	¤â¤·ºÇ¸å¤¬°ìÊ¸»ú¤Ë¤Ê¤Ã¤¿¤È¤­¤ÏºÇ¸å¤Ï»°Ê¸»ú¤Ò¤È¤Þ¤È¤á¤Ë¥¢¥¯¥»¥ó¥È¤ò¤Õ¤ë¡£

	¡ÊÎã¡Ë	£É£Ã£Â£Í¡§aISHIi biIEmu
			£É£Â£Í	¡§aIBIIEmu
	¡¡¤â¤·£É£Ã£Â£Í¤ò¡ÖaISHIIBIIEmu¡×¤Î¤è¤¦¤ËÂ³¤±¤ÆÆÉ¤ß¤¿¤¤¤È¤­¤Ë¤Ï¡¢
	¼­½ñÅÐÏ¿¤·¤Ê¤±¤ì¤Ð¤Ê¤é¤Ê¤¤¡£
	¡¡¿ô»ú¤ÏÀ°¿ô¤Î¾ì¹ç¤ÏÉ´¤Î°Ì¡¢½½¤Î°Ì¡¢°ì¤Î°Ì¤Ï¤Þ¤È¤á¤ë¤¬¡¢Àé¤Î°Ì¤è¤ê¾å¤Ï
	ÊÌ¡¹¤Î¥¢¥¯¥»¥ó¥È¤È¤¹¤ë¡£
	¡¡¾®¿ô¤Î¾ì¹ç¤Ï±Ñ»ú¤ÈÆ±ÍÍ¤ËÆó·å¤º¤Ä¤Þ¤È¤á¤Æ¥¢¥¯¥»¥ó¥È¤ò¤Õ¤ë¡£

	¡ÊÃí°Õ¡Ë¤³¤Î¥ë¡¼¥Á¥ó¤Ï¥¢¥¯¥»¥ó¥ÈÀ¸¾ðÊó¤ò²õ¤¹¤Î¤Ç¡¢
		stc_accent_raw[][]¤ÎÂå¤ï¤ê¤Ëacc_accent_raw[][4]¤òÍÑ¤¤¤ë¡£
  ========================================================================*/
void ACC2000() {
	/* Ã±´Á»ú */
	if(onsei_addr->onsei_kouseisystem==0){
		ACC2700(pkind_tankanji,	pkind_tankanji);
		ACC2700(pkind_tankanji,	pkind_suf);
		/*¡ÖÄ«¡¢³Ø¹»¤Ø¡×¤Ç¡Ö¡¢¡×¤È¡Ö³Ø¹»¡×¤¬·ë¹ç¤¹¤ë¤Èº¤¤ë¤Î¤Ç¡¢¤³¤ì¤Ï¤ä¤á¤ë¡£*/
		/*ACC2900(pkind_tankanji,pkind_jiritsu);*/
	}

	/* alphabets */
	ACC2500(pkind_alpha1,	pkind_alpha1);
	ACC2500(pkind_alphac,	pkind_alpha1);

	ACC2900(pkind_alpha1,	pkind_jiritsu);
	ACC2900(pkind_alphac,	pkind_jiritsu);

	/*
	 *	¡ÖÂè¡×¡¢¡Ö¿Þ¡×¡¢¡ÖÉ½¡×¤Ë¿ô»ú¤¬Â³¤­¡¢¤µ¤é¤Ë½õ¿ô»ì¤¬Â³¤¯¾ì¹ç¡¢
	 *	¿ô»ú¤Î·å¿ô¤Ë¤è¤Ã¤Æ·ë¹ç¤Î½ç½ø¤¬ÊÑ¤ï¤ë¤Î¤ÇÃí°Õ¤òÍ×¤¹¤ë¡£
	 *
	 *	(1)£±·å¤Î¿ô»ú¤Î¾ì¹ç
	 *		¡ÊÎã¡Ë¡ÖÂèÆóÀ¤Âå¡×
	 *		ÀÜÆ¬¼­¡ÖÂè¡×¤È¿ô»ú¡ÖÆó¡×¤¬·ë¹ç¤·¤Æ´Á¸ì²½¤¹¤ë¡£
	 *		´Á¸ì¡ÖÂèÆó¡×¤È´Á¸ì¤Î¡ÖÀ¤Âå¡×¤¬·ë¹ç¤¹¤ë¡£
	 *			·ë¹çÍÍ¼°¡§¡Î¡Î¡ÎÂè¡Ï¡ÎÆó¡Ï¡ÏÀ¤Âå¡Ï
	 *
	 *	(2)£²·å°Ê¾å¤Î¿ô»ú¤Þ¤¿¤Ï¾®¿ôÉô¤ò¤â¤Ä¿ô»ú¤Î¾ì¹ç
	 *		¡ÊÎã¡Ë¡ÖÂè½½»°À¤Âå¡×
	 *		ÀÜÆ¬¼­¡ÖÂè¡×¤È¿ô»ú¡Ö½½»°¡×¤Ï·ë¹ç¤·¤Ê¤¤¡£
	 *		¿ô»ú¡Ö½½»°¡×¤È½õ¿ô»ì¤Î¡ÖÀ¤Âå¡×¤¬·ë¹ç¤¹¤ë¡£
	 *			·ë¹çÍÍ¼°¡§¡ÎÂè¡Î¡Î½½»°¡Ï¡ÎÀ¤Âå¡Ï¡Ï¡Ï
	 *					¡¡¡ÎÂè¡Î¡Î¸Þ½½¡Ï¡Î¡Î¸Þ¡Ï¡ÎÀ¤Âå¡Ï¡Ï¡Ï¡Ï
	 */
	ACC2800();		/*¡ÖÂè¡×¤È£±·å¤Î¿ô»ú¤È¤ò·ë¹ç¤·¡¢´Á¸ì²½¤¹¤ë¡£*/

	/*
	 *	¿ô»ú¡Ü½õ¿ô»ì¡Ê¥æ¡¼¥¶¡¼ÅÐÏ¿¸ì¤ò´Þ¤à¡££±¡Ü¡ÖÊ¸Àá¡×Åù¡Ë
	 */
	ACC2400();
	ACC2200(pkind_digiti0,	pkind_jiritsu,		pkind_digitic);
	ACC2200(pkind_digiti1,	pkind_jiritsu,		pkind_digitic);
	ACC2200(pkind_digiti2,	pkind_jiritsu,		pkind_digitic);
	ACC2200(pkind_digiti3,	pkind_jiritsu,		pkind_digitic);
	ACC2200(pkind_digitf,	pkind_jiritsu,		pkind_digitfc);

	ACC2600();		/* decimal point */

	/* digits */
	/*
	 * ¡Ö£·£´ÈÖ¡×¤ò¡Ö}Å|¥Ê}¼Þ­°ÖÝÊÞÝ|¡×¤Ç¤Ï¤Ê¤¯¡Ö}Å|¥Ê}¼Þ­°|¥è}ÝÊÞÝ|¡×¤ÈÆÉ¤à¤¿¤á¤Ë¤Ï¡¢
	 * °Ê²¼¤Î½èÍý¤ò¤ä¤á¤ëÉ¬Í×¤¬¤¢¤ë¡£¡Ö£·£´£°ÈÖ¡×¡¢¡Ö£·£°£´ÈÖ¡×¤âÆ±ÍÍ¡£
	 *	¡Ê¤È¤ê¤¢¤¨¤º¡¢¤³¤ì¤ÇÍÍ»Ò¤ò¤ß¤ë¤³¤È¤È¤¹¤ë¡£¡Ë
	 *
	 * Îã¤¨¤Ð¡ÖÇ¯¡×¤Î½èÍý¤ÇÌäÂê¤¬Í­¤ê²á¤®¤ë¡£Ê£¹ç¿ô»ì¤ÎÂª¤é¤¨Êý¤¬ÌäÂê¡£
	 * £±£´¡¦£±£µ¤¬Ê£¹ç¿ô»ì¤Ç¤¢¤ë¤Î¤ËÂÐ¤·£±£³¡¦£±£¶¤ÏÊ£¹ç¿ô»ì¤Ç¤Ï¤Ê¤¤¡£
	 * ¿ô»ì¤ò¤è¤¯Ê¬Îà¤·¤Ê¤¤¤ÈÌäÂê¤Ï²ò·è¤·¤Ê¤¤¡£(88-8-24)
	 *		(1) £±£±¡Á£±£¹¤ÏÆÃÊÌ¡£
	 *		(2) £²£±°Ê¾å¤Î¾ì¹ç¡¢Æ¬¹â¤«Èø¹â¤«Ê¿ÈÄ¤«Ãæ¹â¤«¤Ë¤è¤Ã¤ÆÊÑ²½¤¹¤ë¡£
	 */
	ACC2300(pkind_digiti1,	pkind_digiti0);
	ACC2300(pkind_digiti2,	pkind_digiti0);
	ACC2300(pkind_digiti2,	pkind_digiti1);

	ACC2300(pkind_digitf,	pkind_digitf);
	ACC2300(pkind_digitfc,	pkind_digitf);

	/*
	 * ½¾Íè¤Ï¿ô»ú¡Ü½õ¿ô»ì¡¢¤ª¤è¤ÓÃ±ÆÈ¤Î¿ô»ú¤Ï¡¢Éû»ì¤È¤·¤Æ°·¤Ã¤Æ¤¤¤¿¤¬¡¢
	 * ¡Ö£±¥È¥ó¥È¥é¥Ã¥¯¡×Åù¤ò°·¤¦¤Î¤Ë¤Ï¡¢¼«Î©¸ì¤¬ÎÉ¤¤¡£
	 * ¤È¤ê¤¢¤¨¤º¡¢¤¹¤Ù¤Æ¤Î¿ô»ú¤ò¼«Î©¸ì¤È¤·¤Æ°·¤¤¡¢ÌäÂê¤¬À¸¤¸¤¿¤éÊÌÅÓ¹Í¤¨¤ë¡£
	 */
	ACC2900(pkind_digiti0,	pkind_jiritsu);
	ACC2900(pkind_digiti1,	pkind_jiritsu);
	ACC2900(pkind_digiti2,	pkind_jiritsu);
	ACC2900(pkind_digiti3,	pkind_jiritsu);
	ACC2900(pkind_digitf,	pkind_jiritsu);
	ACC2900(pkind_digitic,	pkind_jiritsu);
	ACC2900(pkind_digitfc,	pkind_jiritsu);

	/* ¸ÞÃÊÆ°»ìÏ¢ÍÑ·Á¤Ï¡¢Ì¾»ì¤ÈÅù²Á¤Ê¤Î¤Ç¡¢Àè¤Ë½èÍý¤¹¤ë¡£*/
	ACC2200(pkind_verb_stem,	pkind_verb_renyou,	pkind_verb_stem);

	/*
	 * Ì¾»ì¡¦¥µÊÑÌ¾»ì·Á¤Ë¡¢°ìÈÌÅª¤ÊÀÜÆ¬¼­¤¬ÉÕ¤¤¤¿¤È¤­¡£
	 * Ì¾»ì¡¦¥µÊÑÌ¾»ì·Á¡¦¸ÞÃÊÆ°»ìÏ¢ÍÑ·Á¤Ë¡¢ÀÜÆ¬¼­¡Ö¤ª¡×¤¬ÉÕ¤¤¤¿¤È¤­¡£
	 */
	ACC2200(pkind_pref,			pkind_verb_stem,	pkind_verb_stem);
	ACC2200(pkind_pref,			pkind_adj_stem,		pkind_adj_stem);
	ACC2200(pkind_pref,			pkind_jiritsu,		pkind_jiritsu);
	ACC2200(pkind_pref,			pkind_ren,			pkind_jiritsu);

	/* Ê£¹çÆ°»ì¡¦Ê£¹ç·ÁÍÆ»ì */
	ACC2200(pkind_verb_stem,	pkind_verb_stem,	pkind_verb_stem);
	ACC2200(pkind_adj_stem,		pkind_verb_stem,	pkind_verb_stem);
	ACC2200(pkind_verb_stem,	pkind_adj_stem,		pkind_adj_stem);
	ACC2200(pkind_adj_stem,		pkind_adj_stem,		pkind_adj_stem);
	ACC2900(pkind_verb_stem,	pkind_jiritsu);
	ACC2900(pkind_adj_stem,		pkind_jiritsu);

	/*
	 * ÀÜÆ¬¼­¡ÊÆÃ¤Ë£²Ê¸»ú¤Î¤â¤Î¡¢¡Ö¶ñÂÎ¡×¤Ê¤É¡Ë¤Ë¡¢
	 * ÉÕÂ°¸ì¡Ê¡ÖÅª¡×¡Ë¤äÀÜÈø¼­¡Ê¡Ö²½¡×¡Ë¤¬ÉÕ¤¤¤¿¤È¤­¡£
	 */
	ACC2200(pkind_pref,		pkind_fuzoku,		pkind_jiritsu);
	ACC2200(pkind_pref,		pkind_suf,			pkind_jiritsu);

	/* ´Á¸ì */
	ACC2K00();
	/* ¡Ê¡Ê¿ô»ú¡Ü½õ¿ô»ì¡Ë¡ÜÌ¾»ì¡Ë¤Ï½èÍý¤·¤Ê¤¤ÍÍ¤Ë¤¹¤ë¡£*/
	/* Ê£¹çÆ°»ì¡¦Ê£¹ç·ÁÍÆ»ì */
	/* Á°¤ÎÆ°»ì¤ÏÏ¢ÍÑ·Á¤Î¤ß¡£Á°¤Î·ÁÍÆ»ì¤Ï¸ì´´¤Î¤ß¡£*/
	ACC2200(pkind_jiritsu,	pkind_jiritsu,		pkind_jiritsu);

	/*
	 *	Ì¾»ì¡¦¥µÊÑÌ¾»ì·Á¡¦¸ÞÃÊÆ°»ìÏ¢ÍÑ·Á¤Ë¡¢ÀÜÈø¼­¤¬ÉÕ¤¤¤¿¤È¤­¡¥
	 *
	 * ¡ÖÊÑ´¹ÁàºîË¡¡×¤Ê¤É¤òÀµ¤·¤¯ÊÑ´¹¤¹¤ë¤Ë¤Ï¡¢¤³¤Î½èÍý¤ÏÊ£¹çÌ¾»ì·ë¹ç¤Î¤¢¤È¤Ç
	 *	¹Ô¤Ê¤Ã¤¿Êý¤¬ÎÉ¤¤¤«¤â¤·¤ì¤Ê¤¤¡£
	 * ¤·¤«¤·¤Ê¤¬¤é¡¢¡Ö»ë³Ð¾ã³²¼Ô¡×¤Ç¤ÏÁ°¤Ç½èÍý¤·¤¿¤Û¤¦¤¬ÎÉ¤¤¡£
	 * Ê£¹çÌ¾»ì¡Ê¡Ê¿ô»ú¡Ü½õ¿ô»ì¡Ë¡ÜÌ¾»ì¡Ë¤â¤¢¤ê¤¦¤ë¤Î¤Ç¡¢adv¤â½èÍý¤¹¤ë¡£
	 */
	ACC2200(pkind_jiritsu,	pkind_suf,			pkind_jiritsu);
	ACC2200(pkind_jiritsuc,	pkind_suf,			pkind_jiritsuc);
	ACC2200(pkind_adv,		pkind_suf,			pkind_jiritsu);

	/* ´Á¸ì */
	ACC2K00();

	/* Ì¾»ì¡ÜÀÜÈø¼­¡ÜÌ¾»ì¤Î¾ì¹ç¤Î½èÍý */
	ACC2200(pkind_jiritsu,	pkind_jiritsu,		pkind_jiritsu);

	ACC2900(pkind_jiritsuc,	pkind_jiritsu);

	/*¡ÊÌ¾»ì¡ÜÀÜÈø¼­¡Ë¡ÜÀÜÈø¼­ */
	ACC2200(pkind_jiritsu,	pkind_suf,			pkind_jiritsu);

	/* Ï¢ÂÎ»ì */
	ACC2200(pkind_ren,		pkind_jiritsu,		pkind_jiritsu);
	ACC2200(pkind_ren,		pkind_fuzoku,		pkind_jiritsu);
	ACC2200(pkind_ren,		pkind_adv,			pkind_adv);

	/* ÉÕÂ°¸ì */
	ACC2200(pkind_jiritsu,	pkind_fuzoku,		pkind_jiritsu);
	ACC2200(pkind_adv,		pkind_fuzoku,		pkind_adv);
	ACC2200(pkind_tankanji,	pkind_fuzoku,		pkind_jiritsu);
}


/*========== ACC2K00() ===============================
	µ¡Ç½¡§ÎÙÀÜ¤·¤¿´Á¸ì¤Î¥¢¥¯¥»¥ó¥È¤ò·ë¹ç¤¹¤ë¡£
  ====================================================*/
    void ACC2K00() {
	ACC1K00();	/* ´Á¸ì¤òÃ±ÆÈ´Á¸ì¤ÈÊ£¹ç´Á¸ì¤È¤ËÊ¬Îà¤¹¤ë¡£*/
	ACC2200(pkind_jiritsu,	pkind_jiritsu,	pkind_jiritsuc);
	ACC2200(pkind_jiritsuc,	pkind_jiritsu,	pkind_jiritsuc);
	ACC2200(pkind_jiritsu,	pkind_jiritsuc,	pkind_jiritsuc);
}


/*========== ACC2200(pkind1,pkind2,pkind) ===============================
	µ¡Ç½¡§ÎÙÀÜ¤·¤¿£²Í×ÁÇ¤Î¼ïÎà¤¬pkind1,pkind2¤Î¤È¤­¡¢¥¢¥¯¥»¥ó¥È¤ò·ë¹ç¤·¡¢
		·ë¹ç¸å¤Î¼ïÎà¤òpkind¤È¤¹¤ë¡£
  =======================================================================*/
    void ACC2200(pkind1,pkind2,pkind) WORD pkind1,pkind2,pkind; {
	register integer_16 i,j;
	integer_16 pp,pl,c,bno;
	BYTE a13,a21,a22;
	WORD case_tbl;
ACC2200_10:
	i=0;
ACC2200_20:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]!=0 && phrase_kind[i]==pkind1){ goto ACC2200_100; }
	}
	return;
ACC2200_100:
	j=i+1; while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ return; }
	if(phrase_kind[j]!=pkind2){ ++i; goto ACC2200_20; }

	if(stc_pause[j]){ ++i; goto ACC2200_20; }

	/*
	 *	¸å¤Î´Á¸ì¡ÊÀèÆ¬¤Ë¤Ê¤ê¤¦¤ë¸ì¡Ë¤¬¥¢¥¯¥»¥ó¥È·ë¹ç¶¯À©¸ì¤Î¤È¤­¤Ï·ë¹ç¤µ¤»¤ë¡£
	 */
	if((stc_ctl[j] & BITCTL_FIRST)&&(stc_stz[j] & BITSTZ_PREF)==0){
		if(stc_mru[j] & BITMRU_ACON){
			a13=acc_accent_raw[i][3];
			a21=acc_accent_raw[j][1];
			a22=acc_accent_raw[j][2];
			if(a13==0 && a21==0 && a22==0){
				++i; goto ACC2200_20;
			}else{
				goto ACC2200_200;
			}
		}
	}

	/*
	 *	¸å¤Î´Á¸ì¡ÊÀèÆ¬¤Ë¤Ê¤ê¤¦¤ë¸ì¡Ë¤¬¥¢¥¯¥»¥ó¥È·ë¹çÊü´þ¸ì¤Î¤È¤­¤Ï·ë¹ç¤µ¤»¤Ê¤¤¡£
	 */
	if((stc_ctl[j] & BITCTL_FIRST)&&(stc_stz[j] & BITSTZ_PREF)==0){
		if(stc_mru[j] & BITMRU_ACOFF){
			goto ACC2200_140;
		}
	}

	if(pkind1!=pkind_jiritsu && pkind1!=pkind_jiritsuc){ goto ACC2200_150; }
	if(pkind2!=pkind_jiritsu && pkind2!=pkind_jiritsuc){ goto ACC2200_150; }

	if((stc_f2[i] & BITFLG2_KANGO)==0 &&(stc_stz[i] & BITSTZ_KANGO)==0){
		goto ACC2200_150;
	}
	if((stc_f2[j] & BITFLG2_KANGO)==0 &&(stc_stz[j] & BITSTZ_KANGO)==0){
		goto ACC2200_150;
	}
	/*
	 * £éÈÖÌÜ¤â£êÈÖÌÜ¤â´Á¸ì¤Ç¤¢¤ë¡£
	 *
	 *	·ë¹ç¤·¤¿·ë²Ì¤¬£¶Ê¸»ú¤è¤êÄ¹¤±¤ì¤Ð·ë¹ç¤µ¤»¤Ê¤¤¡£
	 *	¤¿¤À¤·¡¢£êÈÖÌÜ¤¬£±Ê¸»ú¤Ê¤é¤½¤Î¸Â¤ê¤Ç¤Ï¤Ê¤¤¡£¡ÊÀÜÈø¼­¡Ë(63/10/18)
	 */
	if(knj_number(j)==1 && stc_in_len[j]==1){ goto ACC2200_150; }
	if(knj_number(i)+knj_number(j)>6){ goto ACC2200_140; }
	goto ACC2200_150;

ACC2200_140:
	acc_accent_raw[j][1]=acc_accent_raw[j][2]=0;
	++i; goto ACC2200_20;
ACC2200_150:
	a13=acc_accent_raw[i][3];
	a21=acc_accent_raw[j][1];
	a22=acc_accent_raw[j][2];

	/*
	 *	Ê£¹çÆ°»ì¡¦Ê£¹ç·ÁÍÆ»ì
	 *
	 *	¡ÖÆ°¤­»Ï¤á¤ë¡×¤Î¡Ö»Ï¤á¡×¤ÏÆ°»ì¤À¤¬¡¢¡ÖÆ°¤­»Ï¤á¤¬¡×¤Î¡Ö»Ï¤á¡×¤ÏÌ¾»ì¤Ç¤¢¤ë¡£
	 *	¤½¤ì¤æ¤¨¤Ë¡¢£êÈÖÌÜ¤ÏÁ´¤Æ¤Î¸åÃ¼ÈÖ¹æ¤ò¥Á¥§¥Ã¥¯¤·¤ÆÆ°»ì¤«¤É¤¦¤«Ä´¤Ù¤ë¡£
	 *	¤³¤Î¥Á¥§¥Ã¥¯¤ÏACC1000()¤Ç¹Ô¤¦¡£
	 */
	if((pkind1==pkind_verb_stem||pkind1==pkind_adj_stem)
	 &&(pkind2==pkind_verb_stem||pkind2==pkind_adj_stem)){
		ACC22CV(i,j); goto ACC2200_360;
	}
#if defined(Biceps_Femoris)
	if(pkind1==pkind_verb_stem||pkind1==pkind_adj_stem){
		for(bno=0;bno<stc_nbno[j];++bno){
			if(pk=stc_bno[j][bno]){
				if(bno_tbl[pk-1]==pkind_verb_stem
				|| bno_tbl[pk-1]==pkind_adj_stem){
					ACC22CV(i,j); goto ACC2200_360;
				}
			}
		}
	}
#endif

	/*
	 * ¸å¤Ø¤Î±Æ¶Á¤âÁ°¤Ø¤Î±Æ¶Á¤â¤Ê¤¤¤È¤­¤Ï¡¢·ë¹ç¤·¤Ê¤¤¡£
	 * ¼«Ê¬¼«¿È¤ÎÊÑ²½¤¬µ¬Äê¤µ¤ì¤Æ¤¤¤ë¤È¤­¤ÏÊ£¹ç¤·¤¿¤È¤ß¤Ê¤¹¤³¤È¤È¤¹¤ë¡£
	 *
	 * Îã¤¨¤Ð¡ÖÀ¤³¦ºÇ½é¤Î¡×¤Ç¡¢¡ÖÀ¤³¦¡×¤È¡ÖºÇ½é¡×¤ò·ë¹ç¤·¤¿¸å¤Ë¡¢
	 * ¡Ö¤Î¡×¤ò·ë¹ç¤¹¤ë¤È¡¢¡ÖÀ¤³¦ºÇ½é¡×¤¬¡Ö¤Î¡×¤Î±Æ¶Á¤ÇÊ¿ÈÄ²½¤·¤Æ¤·¤Þ¤¦¡£
	 * ¡ÖÀ¤³¦¡×¡ÖºÇ½é¡×¤Ë¡Ö¤Î¡×¤¬·ë¹ç¤¹¤ë¤È¡¢¡Ö¤Î¡×¤Î±Æ¶Á¤Ï¡ÖºÇ½é¡×¤À¤±¤Ë¤Ê¤ë¡£
	 */
	if(a13==0 && a21==0 && a22==0){
		++i; goto ACC2200_20;
	}

	/* Ï¢ÍÑ½¤¾þ³Ê¤ÎÄ¾¸å¤Ï·ë¹ç¤·¤Ê¤¤ */
	for(c=0;c<stc_nbno[i];++c){
		if(bno=stc_bno[i][c]){
			case_tbl=Fdd_Header.Fdd_BnoTblOfs;
			/* ³Ê¤Ë´Ø·¸¤¹¤ë¤Î¤Ï¡¢²¼°Ì£³¥Ó¥Ã¥È¤À¤±¤Ç¤¢¤ë¡£*/
			/* ½ª»ß·Á¡¦Ï¢ÂÎ½¤¾þ³Ê¡¦Ï¢ÍÑ½¤¾þ³Ê°Ê³°¤Ï·ë¹ç¤¹¤ë¡£*/
			if((FddHDR[case_tbl+bno-1] & 0x07)==0x00){ goto ACC2200_155; }
			/* ½ª»ß·Á¡¦Ï¢ÂÎ½¤¾þ³Ê¡¦Ï¢ÍÑ½¤¾þ³Ê¤ÎÄ¾¸å¤¬¡¢*/
			if(stc_ctl[j] & BITCTL_FIRST){
				/* ¼«Î©¸ì¤Ê¤é·ë¹ç¤·¤Ê¤¤¡£*/
				++i; goto ACC2200_20;
			}else{
				/* ÉÕÂ°¸ì¤Ê¤éÉ¬¤º·ë¹ç¤¹¤ë¡£*/
				goto ACC2200_155;
			}
		}
	}

ACC2200_155:

#if defined(LIE)/* ¤¦¤½ */
	/* ¸å¤ÎÊ¸Àá¤¬ £±£Â¡¦£µ£Â¤Î¹àÌÜ¤Ç¡¢Á°¤ÎÊ¸Àá¤¬Æ¬¹â¤Î¥µÊÑÌ¾»ì·Á¡Ê´Á¸ì·¿¡Ë¤Î¤È¤­¤Ï¡¢
	 * ¸å¤ÎÊ¸Àá¤Ï£µ£¸·¿¤È¤¹¤ë¡£(87.6.20) ¡Ö¤é¤ì¡×¤Ê¤É
	 * ¤Ê¤¼¤Ê¤é¤Ð¡¢¥µÊÑÌ¾»ì·Á¡ÜÉÕÂ°¸ì¤Ç¤Ï¡¢¸ì´´¤Î¥¢¥¯¥»¥ó¥È¤ÏÊÑ²½¤·¤Ë¤¯¤¤¡£
	 *if((stc_f2[i]&BITFLG2_KANGO)||(stc_stz[i]&BITSTZ_KANGO)){
	 *	if((stc_f2[j]&BITFLG2_KANGO)||(stc_stz[j]&BITSTZ_KANGO)){ goto ACC2200_195; }
	 *	if(stc_bno[i][0]==Fdd_Header.Fdd_BnoSahen){
	 *		a21=0; if(a22==11){ if(accent0[phrase_ptr[i]]==0x82){ a22=8; } }
	 *	}
	 *}
	 */
#endif

#if !defined(LIE)/* ËÜÅö */
	/*¡¡a21=9 ¤Î¤È¤­¡¢¤Þ¤¿¤Ï¡¢Á°¤ËÄ¾Á°¤òÊ¿ÈÄ²½¤¹¤ë²ÄÇ½À­¤Î¤¢¤ëÉÕÂ°¸ì¤¬¤¢¤ë¤È¤­¤Ï¡¢
	 * ¤¹¤Ç¤Ë¥¢¥¯¥»¥ó¥È¤Î»³¤¬²á¤®¤Æ¤·¤Þ¤Ã¤¿¤È¤­¤Ë¤Ï¸å¤ÎÍ×ÁÇ¤ÏÉ¬¤ºÄã¤¯¤Ä¤¯¡£
	 * ¤¹¤Ê¤ï¤Á¡¢¸å¤ÎÊ¸Àá¤Ï£µ£¸·¿¤È¤¹¤ë¡£(87.6.23) ¡Ê¡Ö¤é¤ì¡×¤Ê¤É¡Ë
	 */
#endif

	/* Îã¤¨¤Ð¡¢¥µÊÑÆ°»ì¤Î¸å¤Ë£¹£Â¤Î¹àÌÜ¤¬Â³¤¤¤Æ¤¤¤Æ¡¢¤«¤Ä¡¢Ê¿ÈÄ²½¤·¤Ê¤«¤Ã¤¿¤È¤­¤Ë¤Ï¡¢
	 * ¸å¤Ë¡¢¹â¤¯¤Ä¤¯²ÄÇ½À­¤Î¤¢¤ëÍ×ÁÇ¤¬Â³¤¤¤Æ¤â¡¢¤½¤ì¤Ï¹â¤¯¤Ê¤é¤º¤ËÄã¤¯¤Ä¤¯¡£
	 * ¤Þ¤¿¡¢Æ¬¹â¡¦Ãæ¹â¤Î¥µÊÑÆ°»ì¤Ë£¹£Â¤ÎÍ×ÁÇ¤¬Â³¤¯¤È¤­¤Ë¤âÉ¬¤ºÄã¤¯¤Ä¤¯¡£
	 * ÉÕÂ°¸ì°Ê³°¤ÏÌµ»ë¤¹¤ë¡£
	 */
	pp=phrase_ptr[i]; pl=phrase_len[i];
	/*if((stc_ctl[j] & BITCTL_FIRST)==0 && (a21==9 || j-i>1)){*/
	/*
	 * £Â¤Î¹àÌÜ¤Ï¡¢£°£Â¡¦£±£Â¡¦£¹£Â¤¬¤¢¤ë¡£
	 *		£±£Â¤ÎÎã¡§µ¢¡Ö¤ê¡×
	 *		£µ£Â¤ÎÎã¡§µ¢¡Ö¤Ã¡×¤¿¡¢ÊÖ¡Ö¤Ã¡×¤¿¡¢ÄÌ¡Ö¤Ã¡×¤¿
	 *		£°£Â¤ÎÎã¡§Í­Æñ¤¦¡Ö¤´¤¶¤¤¡×¤Þ¤·¤¿
	 * £°£Â¡¦£¹£Â¤Ç¤Ï¡¢¡Ö¸ì´´¡×¤ÎÂì¤¬²á¤®¤¿¤«¤É¤¦¤«¹ÍÎ¸¤¹¤ë¤¬¡¢£±£Â¤Ï¹ÍÎ¸¤·¤Ê¤¤¡£
	 * Ã¢¤·¡¢£±£Â¤Ç¤â¡¢¡Ö¸ì´´¡Ü¸ìÈø¡×¤ÎÂì¤¬²á¤®¤¿¤«¤É¤¦¤«¤Ï¹ÍÎ¸¤¹¤ë¡£
	 */
	/*if((phrase_kind[j]==pkind_fuzoku) && (a21==9 || a21==0 || j-i>1)){*/
	/*
	 *	(j-i>1)¤Ç¤Ï¡¢(ÀÜÆ¬¼­¡Ü¼«Î©¸ì)¤âÂÐ¾Ý¤Ë¤Ê¤Ã¤Æ¤·¤Þ¤¦¡£
	 *	(phrase_kind[j-1]==pkind_fuzoku)¤Ê¤éÄ¾Á°¤¬ÉÕÂ°¸ì¤Î¤È¤­¤À¤±¤È¤Ê¤ë¡£*/
	if(phrase_kind[j]==pkind_fuzoku){
		if(a21==9 || a21==0 || phrase_kind[j-1]==pkind_fuzoku){
			if(pl>1 && accent0[pp+pl-1]==1){
				/* ¹â¤¯¤Ä¤¯²ÄÇ½À­¤Î¤¢¤ë¤â¤Î¤Ï¤¹¤Ù¤ÆÂÐ¾Ý¤Ë¤Ê¤ë */
				/*if(a22==1 || a22==2 || a22==5 || a22==11){*/
				a21=0; a22=8;
				/*}*/
			}
		}
	}

	/* ¸å¤ÎÊ¸Àá¤¬ £°£°¤Î¹àÌÜ¤Ï·ë¹ç¤·¤Ê¤¤ */
	if((a21==0||a21==0xFF)&&(a22==0||a22==0xFF)){
		/* ÉÕÂ°¸ì¤Î¤È¤­¤Ï¡¢¥¢¥¯¥»¥ó¥È·ë¹ç¤·¤Ê¤¯¤Æ¤â¡¢¤Þ¤È¤á¤Æ¹Í¤¨¤Ê¤¤¤È¤ª¤«¤·¤¯¤Ê¤ë */
		if(pkind1==pkind_pref||pkind1==pkind_ren){ goto ACC2200_200; }
		if(pkind2==pkind_suf||pkind2==pkind_fuzoku){	goto ACC2200_360; }
		++i; goto ACC2200_20;
	}

ACC2200_200:;
	/*
	 *	ÀÜÆ¬¼­¡Ö¤ª¡×¤Î·ë¹ç¡¥
	 * ¡Ö¤ª¡×¤Ï¡¢a13=1 ¤Î¾ì¹ç¤Ç¤¢¤ë¡£pkind_pref ¤È pkind_prefo ¤Ï¡¢¶èÊÌ¤·¤Ê¤¤¡£
	 * if(pkind1==pkind_prefo){ ACC22PO(i,j); goto ACC2200_360; }
	 */

	/*
	 *	¸å¤ÎÍ×ÁÇ¤¬ÀèÆ¬¤Ë¤Ê¤ê¤¨¤Ê¤¤¤â¤Î¤Ê¤é¤Ð¡¢a13¤ÏÌµ»ë¤¹¤ë¡£
	 *	¡ÊÎã¡Ë¡Ö¤ª¼ê»æÇÒ¸«¤·¤Þ¤·¤¿¡£¡×¡¢¡Ö»ä±öÌî¤Ç¤¹¡£¡×¤Ç¤Ï¡¢a13¤òÍÑ¤¤¤ë¡£
	 *	¡ÊÎã¡Ë¡Ö¤ª¼ê»æ¤òÇÒ¸«¤·¤Þ¤·¤¿¡£¡×¡¢¡Ö»ä¤Ï±öÌî¤Ç¤¹¡£¡×¤Ç¤Ï¡¢a13¤òÌµ»ë¤¹¤ë¡£
	 */
	if((stc_ctl[j] & BITCTL_FIRST)==0 || a13==0){
		/*
		 *	 ¸å¤ÎÍ×ÁÇ¤¬ÀèÆ¬¤Ë¤Ê¤ê¤¨¤Ê¤¤¤«¡¢Á°¤ÎÍ×ÁÇ¤Î¸å¤ËÂÐ¤¹¤ë±Æ¶Á¤¬¤Ê¤¤¤È¤­¤Ï¡¢
		 * ¸å¤ÎÍ×ÁÇ¤ÎÁ°¤ËÂÐ¤¹¤ë±Æ¶Á¤ò¹Í»¡¤¹¤ë¡£
		 */
		ACC22XX(i,j,a21,a22);
	}else{
		/*
		 *	 ¸å¤ÎÍ×ÁÇ¤¬ÀèÆ¬¤Ë¤Ê¤ê¤¦¤ë¤È¤­¤Ï¡¢
		 * Á°¤ÎÍ×ÁÇ¤Î¸å¤ËÂÐ¤¹¤ë±Æ¶Á¤¬¤¢¤ì¤Ð¤½¤ì¤òÍ¥Àè¤·¡¢
		 * ¸å¤ÎÍ×ÁÇ¤ÎÁ°¤ËÂÐ¤¹¤ë±Æ¶Á¤òÌµ»ë¤¹¤ë¡£
		 */
            switch(a13){
		case  1:	ACC22PO(i,j); break;
		case  2:	ACC2232(i,j); break;
		case  3:	ACC2233(i,j); break;
		case  4:	break;
		case  5:	break;
		case  6:	acc_accent_raw[j][1]=0; acc_accent_raw[j][2]=0;
					++i; goto ACC2200_20;	/* ·ë¹ç¤·¤Ê¤¤¡£*/
		case  7:	ACC0004(j); break;
		case  8:	break;
		case  9:	ACC2239(i,j); break;
		default:	break;
		}
	}

ACC2200_360:
	phrase_len[i]+=phrase_len[j];
	phrase_kind[i]=pkind;
	phrase_ptr[j]=phrase_len[j]=0;
	/*
	 *	¡Ö¼«Î©¸ì¡ÜÉÕÂ°¸ì¡×¤ÎÁ°¤Ø¤Î±Æ¶Á¤Ï¡¢Á°¤Î¡Ö¼«Î©¸ì¡×¤¬¤â¤Ã¤Æ¤¤¤ë¤â¤Î¤ÈÆ±¤¸¤Ë¤¹¤ë¡£
	 *	¡Ö¼«Î©¸ì¡Ü¼«Î©¸ì¡×¤ÎÁ°¤Ø¤Î±Æ¶Á¤Ï¡¢
	 *	Á°¤Î¡Ö¼«Î©¸ì¡×¤¬Á°¤Ø¤Î±Æ¶Á¤ò»ý¤Ã¤Æ¤¤¤ì¤Ð¤½¤ì¤òÍÑ¤¤¡¢
	 *	Á°¤Î¡Ö¼«Î©¸ì¡×¤¬Á°¤Ø¤Î±Æ¶Á¤ò»ý¤Ã¤Æ¤¤¤Ê¤±¤ì¤Ð¸å¤Î¡Ö¼«Î©¸ì¡×¤Î¤â¤Î¤òÍÑ¤¤¤ë¡£
	 *
	 *	Îã¤¨¤Ð¡¢¡Ö»¥ËÚ»Ô¡×¤ÎÁ°¤Ø¤Î±Æ¶Á¤Ï¡Ö»¥ËÚ¡×¤Î¤½¤ì¤ÈÆ±¤¸¤Ç¤¢¤ê¡¢
	 *	¡Ö»Ô¡×¤Î¤½¤ì¤È¤Ï°Û¤Ê¤ë¡£
	 */
	if(stc_ctl[j] & BITCTL_FIRST){
		/*
		 *	¡ÖÆüËÜ¸ì²òÀÏ¡×¡Ü¡Ö¥·¥¹¥Æ¥à¡×¤Ç¤Ï¡¢ÌäÂê¤Ê¤¤¤¬¡¢
		 *	¡ÖÆüËÜ¸ì¡×¡Ü¡Ö²òÀÏ¥·¥¹¥Æ¥à¡×¤Î»þ¡¢¡Ö²òÀÏ¥·¥¹¥Æ¥à¡×¤Î·ë¹çµ¬Â§¤¬
		 *	ÌäÂê¤Ë¤Ê¤ë¡£¡Ö²òÀÏ¥·¥¹¥Æ¥à¡×¤Î·ë¹çµ¬Â§¤Ï¡Ö²òÀÏ¡×¡¢¡Ö¥·¥¹¥Æ¥à¡×¤Î
		 *	·ë¹çµ¬Â§¤È¤Ï°Û¤Ê¤ê¡¢Â¿Ê¬¡¢Á°¤òÊ¿ÈÄ²½¤·¹â¤¯ÉÕ¤¤¤Æ¼«Ê¬¤ÎÂì¤ÇÍî¤Á¤ë¤È
		 *	»×¤ï¤ì¤ë¡£
		 */
		acc_accent_raw[i][1]=1;		/* Á°¤òÊ¿ÈÄ²½ */
		acc_accent_raw[i][2]=101;	/* ¹â¤¯ÉÕ¤¯ */
#if defined(Perhaps_LIE)
		/* ¤³¤ì°Ê²¼¤Ï¤¦¤½¤À¤è¡¼¤ó¡£¤¦¤½¤Ç¤Ï¤Ê¤¤¤¬¡¢Îã³°¤¬Â¿²á¤®¤ë¡£*/
		if(acc_accent_raw[i][1]==0xFF){
			acc_accent_raw[i][1]=acc_accent_raw[j][1];
		}
		if(acc_accent_raw[i][2]==0xFF){
			acc_accent_raw[i][2]=acc_accent_raw[j][2];
		}
#endif
	}
	/*
	 *	¡ÖÀÜÆ¬¼­¡ÜÌ¾»ì¡×¤Î¸å¤Ø¤Î±Æ¶Á¤Ï¡ÖÌ¾»ì¡×¤¬¤â¤Ã¤Æ¤¤¤ë¤â¤Î¤ÈÆ±¤¸¤Ë¤¹¤ë¡£
	 */
	if(stc_ctl[i] & BITCTL_PREF){
		acc_accent_raw[i][3]=acc_accent_raw[j][3];
	}
	goto ACC2200_10;
}


/*========== ACC2300(dk1,dk2) ================================================
	µ¡Ç½¡§¿ô»ú¤È¿ô»ú¤Î¥¢¥¯¥»¥ó¥È¤ò·ë¹ç¤·¡¢·ë²Ì¤ò¿ô»ú¤È¤¹¤ë¡£

	¡¡Á°¤Î¿ô»ú¤¬Æ¬¹â¤Î¤È¤­¤Ï¡¢¸å¤Î¿ô»ú¤¬°ì·å¤Î¤È¤­¤Ï¸å¤Î¿ô»ú¤¬Äã¤¯¤Ê¤ë¡£
	Æó·å°Ê¾å¤Î¤È¤­¤ÏÆ¬¹â°Ê³°¤ÏÄã¤¯¤Ê¤ë¡£
	¡¡Á°¤Î¿ô»ú¤¬Ãæ¹â¡¦Èø¹â¤Î¤È¤­¤Ï¡¢¸å¤Î¿ô»ú¤¬Ê¿ÈÄ·¿¤Ê¤éÁ´ÂÎ¤¬Ê¿ÈÄ²½¤¹¤ë¡£
	¸å¤Î¿ô»ú¤¬µ¯Éú·¿¤Î¤È¤­¤ÏÁ°¤Î¿ô»ú¤ÏÊ¿ÈÄ²½¡¢¸å¤Î¿ô»ú¤Ï¹â¤¯¤Ä¤¤¤Æ¼«Ê¬¤ÎÂì¤Ç
	Íî¤Á¤ë¡£
	¡¡Á°¤Î¿ô»ú¤¬Ê¿ÈÄ¤Î¾ì¹ç¤Ï¤¢¤ê¤¨¤Ê¤¤¡£Ê¿ÈÄ¤ÏÂ¿Ê¬£³¤Î¤ß¤Ç¤¢¤ë¡£
	¡¡Ã¢¤·¡¢¸å¤Î¿ô»ú¤¬Äã¤¯¤Ä¤¯¤È¤­¤Ï¡¢Äã¤¤¥â¡¼¥é¿ô¤¬Â¿¤¹¤®¤ë¤È¡¢
	¤â¤È¤Î¥¢¥¯¥»¥ó¥È¤ÇÈ¯²»¤µ¤ì¤ë¤³¤È¤¬Â¿¤¤¡£


	¡ÊÎã¡Ë¡¡£³£°£±¡§Æ¬¹â ¡Ü °ì·å				¤µ'¤ó¤Ó¤ã¤¯¤¤¤Á
			£³£²£°¡§Æ¬¹â ¡Ü Æó·å°Ê¾å¤ÎÆ¬¹â		¤µ'¤ó¤Ó¤ã¤¯¡¡¤Ë'¤¸¤å¡¼
			£³£±£±¡§Æ¬¹â ¡Ü Æó·å°Ê¾å¤ÎÆ¬¹â°Ê³°	¤µ'¤ó¤Ó¤ã¤¯¤¸¤å¡¼¤¤¤Á
										¤Þ¤¿¤Ï¡¢¤µ'¤ó¤Ó¤ã¤¯¡¡¤¸¤å¡¼¤¤¤Á
			£¶£³¡¡¡§Ãæ¹â ¡Ü Ê¿ÈÄ				¤í¤¯¤¸¤å¡¼¤µ¤ó
			£¶£·¡¡¡§Ãæ¹â ¡Ü Æ¬¹â				¤í¤¯¤¸¤å¡¼¤Ê'¤Ê

	¡ÊÎã³°¡Ë¡¡£·£·¤È£¶£·
			¤É¤Á¤é¤â¡¢Ãæ¹â¡ÜÆ¬¹â¤Ç¤¢¤ë¤¬¡¢£¶£·¤Ï£·¤¬¹â¤¯¤Ä¤¯¤Î¤Ë¡¢
		¡¡£·£·¤Ç¤Ï£·¤¬Äã¤¯¤Ä¤¯¡£¤³¤ì¤ÈÆ±¤¸¸½¾Ý¤Ï£·£°£°¤Ç¤â¤ª¤³¤ë¡£
			¤ª¤½¤é¤¯¡¢Ãæ¹â¤Ç¤â¡¢¸å¤ÎÆóÇï°Ê¾å¤¬Äã¤¤¤È¤­¤ÏÆ¬¹â¤ÈÆ±¤¸¤è¤¦¤Ë
		¡¡¿¶¤ëÉñ¤¦¤â¤Î¤È»×¤ï¤ì¤ë¡£
  ============================================================================*/
    void ACC2300(dk1,dk2) WORD dk1,dk2; {
	register integer_16 i,j;
	integer_16 ip,il,jp,jl;

ACC2300_10:
	i=0;
ACC2300_20:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]!=0 && phrase_kind[i]==dk1){ goto ACC2300_100; }
	}
	return;
ACC2300_100:
	j=i+1; while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ return; }
	if(phrase_kind[j]!=dk2){ ++i; goto ACC2300_20; }
	/*a1=stc_dman[i]*100+stc_dexp[i];*/
	/*a2=stc_dman[j]*100+stc_dexp[j];*/

	ip=phrase_ptr[i]; jp=phrase_ptr[j];
	il=phrase_len[i]; jl=phrase_len[j];

	/*
	 * ¡Ö¿ô»ú¡Ü¡Ê¿ô»ú¡Ü½õ¿ô»ì¡Ë¡×¡¢¡Ö¿ô»ú¡Ü¿ô»ú¡×¤Ï·ë¹ç¤¹¤ë¤¬¡¢
	 * ¡Ö¡Ê¿ô»ú¡Ü½õ¿ô»ì¡Ë¡Ü¡Ê¿ô»ú¡Ü½õ¿ô»ì¡Ë¡×¤Ï·ë¹ç¤·¤Ê¤¤¡£
	 */
	if(stc_dman[j-1]==DMANKIND_NUM){ goto ACC2300_360; }

	/*
	 *	¾®¿ôÅÀ¤ÏÄã¤¯ÉÕ¤¯¡£
	 *	¾®¿ôÅÀ¤ÎÁ°¤Î£±·å¤Î¿ô»ú¤ÏÊ¿ÈÄ²½¤¹¤ë¡£
	 *	£±·å¤Î¿ô»ú¤Ë´Ø¤·¤Æ¤Ï¡¢¸å¤ÇÂ¥²»¡¦Ùû²»¤Î½èÍý¤ò¹Ô¤¨¤ÐÆ¬¹â¤ËÊÑ¤ï¤ë¡£
	 *	if(stc_dman[j]==DMANKIND_DP){
	 *		if(stc_dexp[i]==0){ ACC0001(i); }
	 *		ACC0004(j); goto ACC2300_360;
	 *	}
	 */

	if(dk2==pkind_digitf){		/* dk1=digitf or digitfc */
		ACC000H(i); ACC0001(j); goto ACC2300_360;
	}
	/*
	 *	Á°¤¬Æ¬¹â¤Î¤È¤­¡¢¤Þ¤¿¤Ï¡¢
	 *	Á°¤¬Ãæ¹â¤Ç¤«¤Ä¼·½½¡¢¼·É´¤Î¤è¤¦¤ËºÇ¸å¤Î£²Çï°Ê¾å¤¬Äã¤¯¤Ê¤ë¤È¤­¡£
	 */
	if(accent0[ip]==0x82 || (il>2 && accent0[ip+il-1]==1 && accent0[ip+il-2]==1)){
		/*
		 *	£±·å¤Î¿ô»ú¤ÏÉ¬¤ºÄã¤¯¤Ä¤¯¡£
		 *	£²·å°Ê¾å¤Î¿ô»ú¤ÏÆ¬¹â°Ê³°¤ÏÄã¤¯¤Ä¤¯¡£
		 */
		if(stc_dexp[j]==0 || (accent0[jp]!=0x82 && stc_dexp[j]>1)){
			/* Äã¤¤¥â¡¼¥é¤Î¿ô¤¬Â¿¤¹¤®¤Ê¤±¤ì¤ÐÄã¤¯¤Ä¤¯ */
			if(count_low(j)<=MAX_TONE){ ACC0004(j); }
		}
		goto ACC2300_360;
	}

	if(accent0[jp+jl-1]==1 || accent0[jp+jl-1]==0x82){
		ACC000H(i);					/* Ãæ¹â¡¦Èø¹â¡Üµ¯Éú */
		if(accent0[jp]==1){ accent0[jp]=2; }
	}else{
		ACC000H(i); ACC0005(j);		/* Ãæ¹â¡¦Èø¹â¡ÜÊ¿ÈÄ */
	}

ACC2300_360:
	phrase_len[i]+=phrase_len[j];
	phrase_kind[i] = (dk2==pkind_digitf ? pkind_digitfc : pkind_digitic);

	phrase_ptr[j]=phrase_len[j]=0;
	acc_accent_raw[i][1]=0;
	acc_accent_raw[i][2]=0;
	goto ACC2300_10;
}


/*========== ACC2400() =======================================
	µ¡Ç½¡§¿ô»ú¤È½õ¿ô»ì¤Î¥¢¥¯¥»¥ó¥È¤ò·ë¹ç¤·¡¢·ë²Ì¤ò¿ô»ú¤È¤¹¤ë¡£
  ============================================================*/
    void ACC2400() {
	register integer_16 i,j;
	BYTE a20,a21;

ACC2400_10:
	i=0;
ACC2400_20:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]){
			if(phrase_kind[i]==pkind_digitf){ goto ACC2400_100; }
			if(phrase_kind[i]/10==pkind_digiti0/10){ goto ACC2400_100; }
		}
	}
	return;
ACC2400_100:
	j=i+1; while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ return; }
	if(stc_dman[j]!=DMANKIND_NUM){ ++i; goto ACC2400_20; }
	/*a1 =stc_dman[i]*100+stc_dexp[i];*/
	a20=acc_accent_raw[j][0];
	a21=acc_accent_raw[j][1];

	/*
	 * ¡ÖÊ£¿ôËÜ¡×,etc. : ¡ÖÊ£¿ô¡×¤ÏÊ¿ÈÄ²½¡¢
	 *	½õ¿ô»ì¤Ïa21=3¤Î¤È¤­¤Ï¹â¤¯¤Ä¤¤¤Æ¼«Ê¬¼«¿È¤ÎÂì¤ÇÍî¤Á¤ë¡£
	 *	¤½¤ì°Ê³°¤Ï¹â¤¯ÉÕ¤¤¤ÆÊ¿ÈÄ²½¤¹¤ë¡£
	 */
	if(stc_dman[i]==DMANKIND_PL){
		ACC000H(i);
		if(a21==3){
			if(accent0[phrase_ptr[j]]==1){ accent0[phrase_ptr[j]]=2; }
		}else{
			ACC0005(j);
		}
		goto ACC2400_360;
	}

	if(a20==1 && a21==1){ ACC2411(i,j); goto ACC2400_360; }
	if(a20==1          ){ ACC2410(i,j); goto ACC2400_360; }

	if(a20==2 && a21==1){ ACC2421(i,j); goto ACC2400_360; }
	if(a20==2 && a21==2){ ACC2422(i,j); goto ACC2400_360; }
	if(a20==2 && a21==3){ ACC2423(i,j); goto ACC2400_360; }
	if(a20==2 && a21==4){ ACC2424(i,j); goto ACC2400_360; }
	if(a20==2          ){ ACC2420(i,j); goto ACC2400_360; }

	if(a20==3          ){ ACC2430(i,j,a21); goto ACC2400_360; }

	if(a20==4 && a21==1){ ACC2441(i,j); goto ACC2400_360; }
	if(a20==4 && a21==2){ ACC2442(i,j); goto ACC2400_360; }
	if(a20==4 && a21==3){ ACC2443(i,j); goto ACC2400_360; }
	if(a20==4          ){ ACC2440(i,j); goto ACC2400_360; }

	if(a20==5          ){ ACC2450(i,j); goto ACC2400_360; }

	if(a20==6 && a21==1){ ACC2461(i,j); goto ACC2400_360; }
	if(a20==6 && a21==2){ ACC2462(i,j); goto ACC2400_360; }
	if(a20==6 && a21==3){ ACC2463(i,j); goto ACC2400_360; }
	if(a20==6          ){ ACC2460(i,j); goto ACC2400_360; }

ACC2400_360:
	phrase_len[i]+=phrase_len[j]; phrase_ptr[j]=phrase_len[j]=0;
	acc_accent_raw[i][1]=acc_accent_raw[i][2]=0;
	goto ACC2400_10;
}


/*========== ACC2500(ak1,ak2) ==============================
	µ¡Ç½¡§±Ñ»ú¤È±Ñ»ú¤Î¥¢¥¯¥»¥ó¥È¤ò·ë¹ç¤·¡¢·ë²Ì¤ò±Ñ»ú¤È¤¹¤ë¡£
  ==========================================================*/
    void ACC2500(ak1,ak2) WORD ak1,ak2; {
	register integer_16 i,j;
ACC2500_10:
	i=0;
ACC2500_20:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]!=0 && phrase_kind[i]==ak1){ goto ACC2500_100; }
	}
	return;
ACC2500_100:
	j=i+1; while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ return; }
	if(phrase_kind[j]!=ak2){ ++i; goto ACC2500_20; }

	ACC000H(i); ACC0001(j);

	phrase_len[i]+=phrase_len[j]; phrase_kind[i]=pkind_alphac;
	phrase_ptr[j]=phrase_len[j]=0;
	acc_accent_raw[i][1]=acc_accent_raw[i][2]=0;
	goto ACC2500_10;
}


/*========== ACC2600() =====================================
	µ¡Ç½¡§¾®¿ôÅÀ¤Î½èÍý
  ==========================================================*/
    void ACC2600() {
	register integer_16 i,j;
ACC2600_10:
	i=0;
ACC2600_20:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]!=0){
			if(phrase_kind[i]>=pkind_digiti0 && phrase_kind[i]<=pkind_digitfc){
				goto ACC2600_100;
			}
		}
	}
	return;
ACC2600_100:
	j=i+1; while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ return; }
	/*if(phrase_kind[j]!=pkind_digiti0){ ++i; goto ACC2600_20; }*/
	if(stc_dman[j]!=DMANKIND_DP){ ++i; goto ACC2600_20; }

	/*
	 *	¾®¿ôÅÀ¤ÏÄã¤¯ÉÕ¤¯¡£
	 *	¾®¿ôÅÀ¤ÎÁ°¤Î¿ô»ú¤ÎÊÑ²½¤Îµ¬Â§¤Ï²¼µ­¤ÎÄÌ¤ê¤Ç¤¢¤ë¡£
	 *¡¡£²¥â¡¼¥é¤Î¤È¤­Æ¬¹â¡¢£³¥â¡¼¥é°Ê¾å¤Î¤È¤­ºÇ½ªÇï¤ÇÍî¤Á¤ëÃæ¹â¤Ç¤¢¤ë¡£
	 *
	 *		¡±¡²¡¢¡²¡±¡²¡¢¡²¡±¡±¡²¡¢¡²¡±¡±¡±¡²
	 */
	/*if(stc_dman[j]==DMANKIND_DP){*/
		if(phrase_len[i]==2){ ACC0001(i); }
		if(phrase_len[i]>2){ ACC0003(i,phrase_len[i]-1); }
		ACC0004(j);
	/*}else{*/
		/*++i; goto ACC2600_20;*/
	/*}*/

	phrase_len[i]+=phrase_len[j];

	/*phrase_kind[i]=(phrase_kind[i]==pkind_digitfc? pkind_digitfc:pkind_digitic);*/
	phrase_kind[i]=pkind_digitic;

	phrase_ptr[j]=phrase_len[j]=0;
	acc_accent_raw[i][1]=acc_accent_raw[i][2]=0;
	goto ACC2600_10;
}


/*========== ACC2700(ak1,ak2) ==============================================
	µ¡Ç½¡§Ã±´Á»ú¤ÈÃ±´Á»ú¤Î¥¢¥¯¥»¥ó¥È¤ò·ë¹ç¤·¡¢·ë²Ì¤òÃ±´Á»ú¤È¤¹¤ë¡£
			·ë²Ì¤Î¥¢¥¯¥»¥ó¥È·¿¤Ï¡¢¤â¤È¤Î¥¢¥¯¥»¥ó¥È·¿¤Ë´Ø·¸¤Ê¤¯Ê¿ÈÄ·¿¤È¤¹¤ë¡£

		Ê¿²¾Ì¾¡¦ÊÒ²¾Ì¾¡¦ÆÃ¼ìµ­¹æ¤Ê¤É¤â¡¢¤È¤ê¤¢¤¨¤º¤¹¤Ù¤ÆÊ¿ÈÄ²½¤·¤Æ¤·¤Þ¤¦¡£
		Ã±ÆÈ¤Î¾ì¹ç¤ÏÊ¿ÈÄ²½¤»¤º¤Ë¤â¤È¤Î¥¢¥¯¥»¥ó¥È¤òÊÝÂ¸¤¹¤ë¡£
  ==========================================================================*/
    void ACC2700(ak1,ak2) WORD ak1,ak2; {
	register integer_16 i,j;
ACC2700_10:
	i=0;
ACC2700_20:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]!=0 && phrase_kind[i]==ak1){ goto ACC2700_100; }
	}
	return;
ACC2700_100:
	j=i+1; while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ return; }
	if(phrase_kind[j]!=ak2){ ++i; goto ACC2700_20; }

	if(j<stc_ptr-1 && phrase_kind[j+1]==pkind_tankanji){
		/* £êÈÖÌÜ¤ÎÃ±´Á»ú¤Î¸å¤Ë¤Þ¤ÀÃ±´Á»ú¤¬¤¢¤ì¤Ð¡¢Á´ÂÎ¤òÊ¿ÈÄ²½¤¹¤ë¡£*/
		ACC000H(i); ACC0005(j);
	}else{
		/*
		 *	 £êÈÖÌÜ¤ÎÃ±´Á»ú¤Î¸å¤Ë¤â¤¦Ã±´Á»ú¤¬¤Ê¤¤¤È¤­¤Ï¡¢
		 * £êÈÖÌÜ¤ÎÃ±´Á»ú¤ÎÁ°¤ËÂÐ¤¹¤ë±Æ¶Á¤È¼«Ê¬¼«¿È¤ÎÊÑ²½¤ò¹ÍÎ¸¤¹¤ë¡£
		 */
		ACC22XX(i,j,acc_accent_raw[j][1],acc_accent_raw[j][2]);
	}

	phrase_len[i]+=phrase_len[j]; phrase_kind[i]=pkind_tankanji;
	phrase_ptr[j]=phrase_len[j]=0;
	acc_accent_raw[i][1]=acc_accent_raw[i][2]=0;
	goto ACC2700_10;
}


/*========== ACC2800() =====================================================
	µ¡Ç½¡§¥Þ¥¹¥¿¡¼¤Î£µ£´¥«¥é¥à¤¬£´¤ÎÍ×ÁÇ¤È¿ô»ú¤È¤Î·ë¹ç¤ò¹Ô¤¦¡£
		¡¡¥Þ¥¹¥¿¡¼¤Î£µ£´¥«¥é¥à¤¬£´¤ÎÍ×ÁÇ¤Ïº£¤Î¤È¤³¤í°Ê²¼¤Î£³¼ï¤Ç¤¢¤ë¡£
			¡ÖÂè¡×¡¢¡Ö¿Þ¡×¡¢¡ÖÉ½¡×

		¡¡¿ô»ú¤¬£±·å¤Î¤È¤­¤À¤±·ë¹ç¤·¡¢£²·å°Ê¾å¤Þ¤¿¤Ï¾®¿ô¤Î¤È¤­¤Ï·ë¹ç¤·¤Ê¤¤¡£
		¡¡¡ÖÂè¡×¤Ê¤É¤ÎÆ¬¹â¤ÎÍ×ÁÇ¤Î¤È¤­¤Ï¡¢¿ô»ú¤ÏÄã¤¯ÉÕ¤¯¡£
		¡¡¡ÖÉ½¡×¤Ê¤É¤ÎÆ¬¹â°Ê³°¤ÎÍ×ÁÇ¤Î¤È¤­¤Ï¡¢¿ô»ú¤Ï¹â¤¯ÉÕ¤¯¡£

	¡ÊÎã¡Ë¡ÖÂè£±¡×¤Ï·ë¹ç¤·¡Ö£±¡×¤ÏÄã¤¯ÉÕ¤¯¡£¡ÖÂè£²£µ¡×¤ÏÈó·ë¹ç¡£
		¡¡¡Ö¿Þ£±¡×¤Ï·ë¹ç¤·¡Ö£±¡×¤Ï¹â¤¯ÉÕ¤¯¡£¡Ö¿Þ£²£µ¡×¤ÏÈó·ë¹ç¡£
		¡¡¡ÖÉ½£±¡×¤Ï·ë¹ç¤·¡Ö£±¡×¤Ï¹â¤¯ÉÕ¤¯¡£¡ÖÉ½£²£µ¡×¤ÏÈó·ë¹ç¡£

	¡ÊÈ÷¹Í¡Ë¡Ö¿Þ¡×¤Ê¤É¤Î£±¥â¡¼¥é¤ÎÊ¿ÈÄ¤ÎÍ×ÁÇ¤È£²·å°Ê¾å¤Î¿ô»ú¤Î¤È¤­¤Ï¡¢
			¿ô»ú¤¬Æ¬¹â°Ê³°¤Î¤È¤­¤Ï¡Ö¿Þ¡×¤òÆ¬¹â¤Ë¤¹¤ë¤«¡¢
			¤Þ¤¿¤Ï¥Ý¡¼¥º¤òÆþ¤ì¤ë¤«¤·¤¿Êý¤¬ÎÉ¤¤¤«¤â¤·¤ì¤Ê¤¤¡£
				¡Ê¡Ö¿Þ£±£²¡×¤Ê¤É¡£¡Ë
  ==========================================================================*/
    void ACC2800() {
	register integer_16 i,j;
	BYTE al;
	/* ceptr,celen¤ò²õ¤¹¤È¼¡¸õÊäÌµ¤·¤Î¤È¤­¤Ëº¤¤ë¤Î¤ÇÃí°Õ¤¹¤ë¤³¤È¡£*/
	ceptr_sav=ceptr; celen_sav=celen;
ACC2800_10:
	i=0;
ACC2800_20:
	for(;i<stc_ptr-1;++i){
		if(phrase_len[i]!=0 && acc_accent_raw[i][3]==4){ goto ACC2800_100; }
	}
	goto ACC2800_999;
ACC2800_100:
	j=i+1; while(phrase_len[j]==0 && j<stc_ptr){ ++j; }
	if(j==stc_ptr){ goto ACC2800_999; }

	al=phrase_kind[j];
	if((al==pkind_digitf)||(al/10==pkind_digiti0/10)){
		/* ¸åÂ³¤¹¤ë¿ô»ú¤ÎËÜÅö¤Î°ÌÃÖ¤ÈÄ¹¤µ¤òµá¤á¤ë¡£*/
		ceptr=stc_in_ptr[j]; celen=n_inert-ceptr;
		while(OTHER2000()==0){ celen=INERT2000(ceptr,celen); }
		/* £±·å¤ÎÀ°¿ô¤À¤±½èÍý¤¹¤ë¡£*/
		/* ¿ô»ú¤ÎÄ¾¸å¤Ë½õ¿ô»ì¤¬Â³¤«¤Ê¤¤¤È¤­¤À¤±½èÍý¤¹¤ë¡£*/
		if(NUM0010()==1 && NUM0020()==0){
			if(stc_ctl[j+1] & BITCTL_FIRST){
				goto ACC2800_200;
			}
		}
	}
	++i; goto ACC2800_20;
ACC2800_200:
	if(accent0[phrase_ptr[i]]==1){
		if(accent0[phrase_ptr[j]]==1){ accent0[phrase_ptr[j]]=2; }
	}else{
		ACC0004(j);
	}
	phrase_len[i]+=phrase_len[j]; phrase_kind[i]=pkind_jiritsu;
	phrase_ptr[j]=phrase_len[j]=0;
	acc_accent_raw[i][1]=acc_accent_raw[i][2]=acc_accent_raw[i][3]=0;
	goto ACC2800_10;
ACC2800_999:
	ceptr=ceptr_sav; celen=celen_sav;
	return;
}


/*========== ACC2900(pkind1,pkind2) ==========================
	µ¡Ç½¡§phrase_kind[]¤¬¡¢pkind1¤Î¤â¤Î¤òpkind2¤Ë½ñ¤­´¹¤¨¤ë¡£
  ============================================================*/
    void ACC2900(pkind1,pkind2) register WORD pkind1; WORD pkind2; {
	register integer_16 i;
	for(i=stc_ptr;i--;){
		if(phrase_kind[i]==pkind1){ phrase_kind[i]=pkind2; }
	}
}

/***************************** END OF ACC_2000.C *****************************/
