#if defined(O_DEBUG)
/*******************************************
	ILE_DBGC.C	updated at 22th May 1989
	£Ä£Å£Â£Õ£Ç´ØÏ¢
 *******************************************/

#define		EXT		extern

#include	<stdio.h>
#include	"onsei.h"
#include	"ile_dbgh.h"
#include	"ile_func.h"

static FILE		*dbgfp = NULL;

static const BYTE hex_tbl[16]={
	'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

void DEBUG_INIT() {
	debug[ 1]=DISABLE;	/* STACK_PUSH	all elements */
	debug[ 2]=DISABLE;	/* STACK_PUSH	current element */
	debug[ 3]=DISABLE;	/* STC_POP		current element */
	debug[ 4]=DISABLE;	/* FOLLOW		Stz_CE and Stz_PE */
	debug[ 5]=DISABLE;	/* OMEGA9000	stack contents after xfer */
	debug[ 6]= ENABLE;	/* FOLLOW		Stz_CE */
}

void DEBUG_STR(name) BYTE *name; {
	printf("%s",name);
	fprintf(dbgfp,"%s",name);
}

void DEBUG_PN(name) BYTE *name; {
	printf("%s\n",name);
	fprintf(dbgfp,"%s\n",name);
}

void DEBUG_I(name,id,l) BYTE *name; register integer_16 *id,l; {
	integer_16 ll;
	integer_16 *id2;
	ll=l; id2=id;
	printf("%s : ",name);
	for(;l--;){
		printf("%d",*id++); if(l>0){ putchar(','); }
	}
	putchar('\n');

	l=ll; id=id2;
	fprintf(dbgfp,"%s : ",name);
	for(;l--;){
		fprintf(dbgfp,"%d",*id++); if(l>0){ fputc(',',dbgfp); }
	}
	fputc('\n',dbgfp);
}

void DEBUG_IB(name,id,l) BYTE *name; register BYTE *id; register integer_16 l; {
	integer_16 ll;
	BYTE *id2;
	ll=l; id2=id;
	printf("%s : ",name);
	for(;l--;){
		printf("%d",*id++); if(l>0){ putchar(','); }
	}
	putchar('\n');

	l=ll; id=id2;
	fprintf(dbgfp,"%s : ",name);
	for(;l--;){
		fprintf(dbgfp,"%d",*id++); if(l>0){ fputc(',',dbgfp); }
	}
	fputc('\n',dbgfp);
}

void DEBUG_A(name,id,l) BYTE *name; register BYTE *id; register integer_16 l; {
	int ll;
	BYTE *id2;
	ll=l; id2=id;
	printf("%s : ",name);
	for(;l--;){ putchar(*id++); }
	putchar('\n');

	l=ll; id=id2;
	fprintf(dbgfp,"%s : ",name);
	for(;l--;){ fputc(*id++,dbgfp); }
	fputc('\n',dbgfp);
}

void DEBUG_H(name,id,l) BYTE *name; register BYTE *id; register integer_16 l; {
	int ll;
	BYTE *id2;
	integer_16 n;
	ll=l; id2=id;
	printf("%s : ",name);
	for(;l--;){
		n=(*id++); putchar(hex_tbl[n/16]); putchar(hex_tbl[n%16]);
		putchar(' ');
	}
	putchar('\n');

	l=ll; id=id2;
	fprintf(dbgfp,"%s : ",name);
	for(;l--;){
		n=(*id++); fputc(hex_tbl[n/16],dbgfp); fputc(hex_tbl[n%16],dbgfp);
		fputc(' ',dbgfp);
	}
	fputc('\n',dbgfp);
}

void DEBUG_H2(var) register WORD var; {
	putchar(hex_tbl[(var>>12)& 0x000f]);
	putchar(hex_tbl[(var>> 8)& 0x000f]);
	putchar(hex_tbl[(var>> 4)& 0x000f]);
	putchar(hex_tbl[(var    )& 0x000f]);

	fputc(hex_tbl[(var>>12)& 0x000f],dbgfp);
	fputc(hex_tbl[(var>> 8)& 0x000f],dbgfp);
	fputc(hex_tbl[(var>> 4)& 0x000f],dbgfp);
	fputc(hex_tbl[(var    )& 0x000f],dbgfp);
}

void DEBUG_H1(var) BYTE var; {
	putchar(hex_tbl[var/16]); putchar(hex_tbl[var%16]);
	fputc(hex_tbl[var/16],dbgfp); fputc(hex_tbl[var%16],dbgfp);
}

void DEBUG_K1(id,l) register BYTE *id; register integer_16 l; {
	BYTE s[2]; int ll; BYTE *id2;
	ll=l; id2=id;
	for(;l--;){
		if(*id==0x20){
			putchar(*(id+1));
		}else{
			*s=(*id); *(s+1)=(*(id+1)); jis2sft(s);
			putchar(*s); putchar(*(s+1));
		}
		id+=2;
	}

	l=ll; id=id2;
	for(;l--;){
		if(*id==0x20){
			fputc(*(id+1),dbgfp);
		}else{
			*s=(*id); *(s+1)=(*(id+1)); jis2sft(s);
			fputc(*s,dbgfp); fputc(*(s+1),dbgfp);
		}
		id+=2;
	}
}

/*========== DEBUG_ELM() ===============================
	µ¡Ç½¡§debug routine for Stz_CE,Stz_PE,ceptr,celen.
  ======================================================*/
void DEBUG_ELM(sw) integer_16 sw; {
	register integer_16 i,n;

	if(sw){
		DEBUG_STR("¡û ");
	}else{
		DEBUG_STR("¡ß ");
	}
	DEBUG_K1(&inert[ceptr*2],celen);

	switch(Stz_CE.stz_kind){
	case DICKIND_D:  DEBUG_STR(" ©¨¿ô»ú©© "); break;
	case DICKIND_5Y: DEBUG_STR(" ©¨Ï¢ÍÑ©© "); break;
	case DICKIND_I:  DEBUG_STR(" ©¨ÆâÉô©© "); break;
	case DICKIND_E:  DEBUG_STR(" ©¨³°Éô©© "); break;
	case DICKIND_T:  DEBUG_STR(" ©¨´Á»ú©© "); break;
	case DICKIND_K:  DEBUG_STR(" ©¨²¾Ì¾©© "); break;
	}

	if(Stz_CE.stz_mru & 0x40){ DEBUG_STR("¡ÌÊü´þ¡Í"); }

	n=Stz_CE.stz_nbno;
	for(i=0;n--;i++){
		printf("%3d",Stz_CE.stz_bno[i]);
		if(n>0){ putchar(','); }

		fprintf(dbgfp,"%3d",Stz_CE.stz_bno[i]);
		if(n>0){ fputc(',',dbgfp); }
	}
	putchar('\n');
	fputc('\n',dbgfp);
}

/***************************** END OF ILE_DBGC.C *****************************/
#endif
