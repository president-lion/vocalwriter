/*******************************************
	ILE_ERRH.H	updated at 19th May 1989
	エラー処理等
 *******************************************/
#ifndef ILE_ERRH_H
#define ILE_ERRH_H
#include	<setjmp.h>

extern	integer_16 error_code;
extern	jmp_buf	jmpbuf;
#endif /* ILE_ERRH_H */
/***************************** END OF ILE_ERRH.H *****************************/
