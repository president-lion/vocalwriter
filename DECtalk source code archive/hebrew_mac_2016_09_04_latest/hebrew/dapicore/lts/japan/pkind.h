#ifndef PKIND_H
#define PKIND_H

#define pkind_verb_stem		 1		/* 0x01 Æ°»ì¸ì´´ */
#define pkind_verb_inflect	 2		/* 0x02 Æ°»ì³èÍÑ¸ìÈø */
#define pkind_verb_renyou	 3		/* 0x03 Æ°»ìÏ¢ÍÑ·Á³èÍÑ¸ìÈø */
#define pkind_verb_sa		 4		/* 0x04 ¥µÊÑÌ¾»ì·Á */
#define pkind_adj_stem		 5		/* 0x05 ·ÁÍÆ»ì¸ì´´ */
#define pkind_adj_inflect	 6		/* 0x06 ·ÁÍÆ»ì³èÍÑ¸ìÈø */
#define pkind_stem			 7		/* 0x07 Æ°»ì¡¦·ÁÍÆ»ì¸ì´´ */

#define pkind_adv			 8		/* 0x08 Éû»ì¡¦ÀÜÂ³»ì¡¦´¶Æ°»ì */
#define pkind_adjv			 9		/* 0x09 ·ÁÍÆÆ°»ì¸ì´´ */
#define pkind_noun			10		/* 0x0A Ì¾»ì */
									/*		·ÁÍÆÆ°»ì¤ÏÌ¾»ì°·¤¤ */
#define pkind_pref			11		/* 0x0B ÀÜÆ¬¼­¡¦¿ÍÌ¾Àè¹Ô»ì */
/*#define pkind_prefo		12*/	/* 0x0C ÀÜÆ¬¼­¡Ö¤ª¡×*/
#define pkind_suf			13		/* 0x0D ÀÜÈø¼­ */
#define pkind_aux_verb		14		/* 0x0E ½õÆ°»ì */
#define pkind_particle		15		/* 0x0F ½õ»ì */
#define pkind_ren			16		/* 0x10 Ï¢ÂÎ»ì */
#define pkind_digit			17		/* 0x11 ¿ô»ì */
#define pkind_num			18		/* 0x12 ½õ¿ô»ì */
#define pkind_others		19		/* 0x13 ÆÃ¼ìÊ¸»úÅù */

#define pkind_digiti0		30		/* 0x1E À°¿ô¡¦Ã±ÆÈ */
#define pkind_digiti1		31		/* 0x1F À°¿ô¡¦Ã±ÆÈ */
#define pkind_digiti2		32		/* 0x20 À°¿ô¡¦Ã±ÆÈ */
#define pkind_digiti3		33		/* 0x21 À°¿ô¡¦Ã±ÆÈ */
#define pkind_digitic		39		/* 0x27 À°¿ô¡¦Ê£¹ç */
#define pkind_digitf		40		/* 0x28 ¾®¿ô¡¦Ã±ÆÈ */
#define pkind_digitfc		49		/* 0x31 ¾®¿ô¡¦Ê£¹ç */

#define pkind_alpha1		50		/* 0x32 ±Ñ»ú¡¦Ã±ÆÈ */
#define pkind_alphac		51		/* 0x33 ±Ñ»ú¡¦Ê£¹ç */

#define pkind_tankanji		60		/* 0x3C Ã±´Á»ú */
#define pkind_jiritsu		61		/* 0x3D ¼«Î©¸ì */
#define pkind_fuzoku		62		/* 0x3E ÉÕÂ°¸ì */
#define pkind_jiritsuc		70		/* 0x46 ¼«Î©¸ì¡ÊÊ£¹ç´Á¸ì¡Ë*/

/*#define pkind_number		80*/	/* 0x50 ¿ô»ú¡Ü½õ¿ô»ì */
#define pkind_phrase		90		/* 0x5A ¶ç¡¦Àá */

#endif /* PKIND_H */
