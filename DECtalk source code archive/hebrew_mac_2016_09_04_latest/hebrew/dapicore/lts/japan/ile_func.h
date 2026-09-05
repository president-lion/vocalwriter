/**********************************************
	ILE_FUNC.H	updated at 8th January 1991
	関数プロトタイプ宣言
 **********************************************/

void		ACCXXXX( );
void		ACC000H( integer_16 );
void		ACC0001( integer_16 );
void		ACC0002( integer_16 );
void		ACC0003( integer_16, integer_16 );
void		ACC0004( integer_16 );
void		ACC0005( integer_16 );
void		ACC1000( );
void		ACC2000( );
void		ACC1K00( );
void		ACC2K00( );
void		ACC2200( WORD, WORD, WORD);
void		ACC22XX( integer_16, integer_16, WORD, WORD );
void		ACC22CV( integer_16, integer_16 );
void		ACC22PO( integer_16, integer_16 );
void		ACC2213( integer_16 );
void		ACC2214( integer_16 );
void		ACC2216( integer_16, integer_16 );
void		ACC2217( integer_16, integer_16 );
void		ACC2219( integer_16 );
void		ACC221A( integer_16 );
void		ACC2221( integer_16 );
void		ACC2222( integer_16 );
void		ACC2223( integer_16 );
void		ACC2225( integer_16 );
void		ACC2226( integer_16, integer_16 );
void		ACC2227( integer_16, integer_16 );
void		ACC222A( integer_16, integer_16 );
void		ACC222B( integer_16, integer_16 );
void		ACC222C( integer_16, integer_16 );
void		ACC222D( integer_16 );
void		ACC222E( integer_16, integer_16 );
void		ACC222Z( integer_16 );
void		ACC2232( integer_16, integer_16 );
void		ACC2233( integer_16, integer_16 );
void		ACC2239( integer_16, integer_16 );
void		ACC2300( WORD, WORD );
void		ACC2400( );
void		ACC2410( integer_16, integer_16 );
void		ACC2411( integer_16, integer_16 );
void		ACC2412( integer_16, integer_16 );
void		ACC2413( integer_16, integer_16 );
void		ACC2420( integer_16, integer_16 );
void		ACC2421( integer_16, integer_16 );
void		ACC2422( integer_16, integer_16 );
void		ACC2423( integer_16, integer_16 );
void		ACC2424( integer_16, integer_16 );
void		ACC2430( integer_16, integer_16, integer_16 );
void		ACC2440( integer_16, integer_16 );
void		ACC2441( integer_16, integer_16 );
void		ACC2442( integer_16, integer_16 );
void		ACC2443( integer_16, integer_16 );
void		ACC2450( integer_16, integer_16 );
void		ACC2460( integer_16, integer_16 );
void		ACC2461( integer_16, integer_16 );
void		ACC2462( integer_16, integer_16 );
void		ACC2463( integer_16, integer_16 );
BOOL		ACC2499( integer_16, integer_16 );
void		ACC2500( WORD, WORD );
void		ACC2600( );
void		ACC2700( );
void		ACC2800( );
void		ACC2900( WORD, WORD );
void		ACC2A00( );
void		ACC3000( );
void		ACC4000( );
void		ACC4100( BYTE * );
void		ACC4200( BYTE * );
void		ACC5000( );
void		ACC5100( integer_16 );
void		ACC6000( );
void		ACC7000( );
void		ACC7100( integer_16 ); 
void		ACC8000( );
void		ACC_OUTPUT( );
integer_16	ACC_OUTPUT_C( BYTE, integer_16 );
integer_16	ACC_OUTPUT_N( integer_16, integer_16, integer_16 );
integer_16	ACC_OUTPUT_S( BYTE *, integer_16 );

integer_16	CS1000( BOOL );

void		DEBUG_A( BYTE *, BYTE *, integer_16 );
void		DEBUG_ELM( integer_16 );
void		DEBUG_H( BYTE *, BYTE *, integer_16 );
void		DEBUG_H1( BYTE );
void		DEBUG_H2( WORD );
void		DEBUG_I( BYTE *, integer_16 *, integer_16 );
void		DEBUG_IB( BYTE *, BYTE *, integer_16 );
void		DEBUG_INIT( );
void		DEBUG_K1( BYTE *, integer_16 );
void		DEBUG_PN( BYTE * );
void		DEBUG_STC_ALL( );
void		DEBUG_STC_CE( integer_16 );
void		DEBUG_STR( BYTE * );
void		DEBUG_STZ( );

void		DIGIT7000( );
void		DIGIT7010( BYTE * );
void		DIGIT7020( BYTE * );
void		DIGIT7030( BYTE * );
void		DIGIT_CONV( integer_16 );
void		DIGIT_CONV1( integer_16, integer_16 );
void		DIGIT_CONV2( BYTE *, BYTE * );

BOOL		EDIC1000( );
BOOL		EDIC2XXX( );
BOOL		EDIC2000( );
void		EDIC2100( );
BOOL		EDIC3XXX( );
BOOL		EDIC3000( );
void		EDIC4000( );
integer_16	EDIC5000( integer_16 );
void		EDIC_ABORT( );
void		EDIC_ACC_STUDY( integer_16 );
void		EDIC_PAUSE_STUDY( integer_16 );
void		EDIC_ADD( );
void		EDIC_DELETE( );
void		EDIC_MRU( integer_16 );
void		EDIC_MRU_INIT( );
BOOL		EDIC_MRU_NEED( WORD, LPBYTE, LPBYTE );
void		EdicfAkeru( integer_16, LPBYTE );
void		EdicfIreru( LPBYTE );
void		EdicfSentouSagasu( LPBYTE, LPBYTE *, LPBYTE *);
void		EdicfShift( LPBYTE, LPBYTE, integer_16 );
void		EdicfToridasu( LPBYTE );
void		EdicfTsumeru( integer_16, LPBYTE );
BOOL		RtgfGrcdAri( BYTE, LPBYTE );
void		RtgfFlag1Tsukuru( );
void		RtgfKchTsukuru( integer_16 );
BOOL		RtgfKnjHykOnaji( LPBYTE );
BOOL		RtgfNydShiraberu( );
BOOL		RtgfTsuikaKanou( );

integer_16	entrypt( STRUCT_ONSEI far * );
void		ENV_INIT( );
void		ENV_TERM( );

void		jp_ERROR( integer_16 );
void		ERROR5000( );

BOOL		FOLLOW( );
void		FOLLOW_INIT();
void		StzfElmBackward( );
void		StzfElmForward( );
void		StzfGrcdAdd( BYTE );
void		StzfGrcdOR( );
void		StzfGrcdStore( BYTE );
void		StzfBnoAdd();
void		StzfElmKind();

BOOL		IDIC1000( );
BOOL		IDIC2000( );
BOOL		IDIC3000( );
void		IDIC4000( );
integer_16	IDIC5000( integer_16 );
void		IDIC_ABORT( );
void		IDIC_ACC_STUDY( integer_16 );
void		IDIC_PAUSE_STUDY( integer_16 );
void		IDIC_MRU( integer_16 );

integer_16	INERT1000( );
integer_16	INERT2000( integer_16, integer_16 );
integer_16	INERT200B( integer_16, integer_16 );
integer_16	INERT300B( integer_16, integer_16 );
void		INERT_COMMA( );
void		INERT_DIGIT( );
void		INERT_INIT( );

void		JKN_INIT( );

BOOL		KKANA1000( );
integer_16	KKANA5000( integer_16 );

void		MRU_ALL( );
void		MRU_CSENT( );
void		MRU_REMEMBER( );

BOOL		NUMB1000( );
BOOL		NUMB5000( integer_16 );
void		NUMB5XXX( integer_16 );
integer_16	NUM0010( );
integer_16	NUM0020( );

integer_16	OMEGA1000( BYTE, integer_16 );
void		OMEGA5000( );
void		OMEGA6000( );
void		OMEGA9000( );
BOOL		OMEGA_CHECK( );
void		OMEGA_INIT( );
void		OMEGA_SAVE( );

WORD		OTHER2000( );
integer_16	OTHER5000( integer_16 );
void		OTHER_GRCD( );

void		PAUSE1000( );

void		PRON_ACON( );
void		PRON_ACOFF( );
void		PRON_FIRST( );
void		PRON_INIT( );
void		PRON_INIT2( );
void		PRON_LEFT( );
void		PRON_LONG( );
void		PRON_NEXT( );
void		PRON_RECOVER( );
void		PRON_RIGHT( );
void		PRON_SENT_NO( );
void		PRON_SHORT( );
void		PRON_TERM( );

void		RnsfRecordMru( );

void		SENT_INIT( );

BOOL		BNO_VECP( );
BOOL		bno_pno( integer_16, integer_16 );
BOOL		bno_prop( integer_16 );

void		STACK_INIT( );
void		STACK_POP( );
void		STACK_POP_FS( );
void		STACK_POP_WS( );
void		STACK_PUSH( );

integer_16	TDIC1000( );
BOOL		TDIC2000( );
void		TDIC2100( );
BOOL		TDIC3000( );
void		TDIC4000( );
void		TDIC4100( );
integer_16	TDIC5000( integer_16 );
integer_16	TDIC5009( integer_16 );

void		WS_BECOMES_TOP( );
BOOL		WS_CE_CHK( );
integer_16	WS_CHK_LONG( );
integer_16	WS_CHK_SHORT( );
void		WS_MOVE_LEFT( );
void		WS_MOVE_RIGHT( );

integer_16	V5DAN5000( integer_16 );
void		V5DANYOU_PUSH( );

integer_16	before_ch_ptr( integer_16 ); 
void		check_X( BYTE *, integer_16 );
BOOL		chk_heiban( integer_16 );
integer_16	chk_limits( BYTE, BYTE, BYTE *, integer_16 );
BOOL		chk_odaka( integer_16 );
WORD		chk_separator( integer_16 ); 
integer_16	chk_value( BYTE, BYTE *, integer_16 );
void		code_to_hardchar( BYTE, BYTE, BYTE * );
integer_16	count_high( integer_16 );
integer_16	count_low( integer_16 );
integer_16	first_char_ptr( integer_16 ); 
BOOL		first_or_not( integer_16 ); 
WORD		get_case( integer_16 );
BOOL		is_zengo_kanji( integer_16 );
void		jis2sft( BYTE * );
BOOL		kana_conv( BYTE, BYTE, BYTE * );
BOOL		kana_mora( );
integer_16	knj_number( integer_16 ); 
BOOL		last_or_not( integer_16 ); 
integer_16	net_phrase_in_len( integer_16 ); 
integer_16	next_ch_ptr( integer_16 ); 
integer_16	next_phrase( integer_16 ); 
BOOL		user_checksum( );

/***************************** END OF ILE_FUNC.H *****************************/
