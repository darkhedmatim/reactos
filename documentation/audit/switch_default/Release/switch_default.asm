; Listing generated by Microsoft (R) Optimizing Compiler Version 14.00.50727.42 

	TITLE	f:\reactos\trunk\documentation\audit\switch_default\switch_default.cpp
	.686P
	.XMM
	include listing.inc
	.model	flat

INCLUDELIB OLDNAMES

PUBLIC	??_C@_04DEEPCGMJ@Foo?6?$AA@			; `string'
PUBLIC	??_C@_04DFOOMHBJ@Bar?6?$AA@			; `string'
EXTRN	@__security_check_cookie@4:PROC
EXTRN	__imp__printf:PROC
;	COMDAT ??_C@_04DFOOMHBJ@Bar?6?$AA@
CONST	SEGMENT
??_C@_04DFOOMHBJ@Bar?6?$AA@ DB 'Bar', 0aH, 00H		; `string'
CONST	ENDS
;	COMDAT ??_C@_04DEEPCGMJ@Foo?6?$AA@
CONST	SEGMENT
??_C@_04DEEPCGMJ@Foo?6?$AA@ DB 'Foo', 0aH, 00H		; `string'
CONST	ENDS
PUBLIC	_wmain
; Function compile flags: /Ogtpy
; File f:\reactos\trunk\documentation\audit\switch_default\switch_default.cpp
;	COMDAT _wmain
_TEXT	SEGMENT
_argc$ = 8						; size = 4
_argv$ = 12						; size = 4
_wmain	PROC						; COMDAT

; 8    : {

	push	esi

; 9    : 	int a = 1;
; 10   : 
; 11   : 	switch(a) {
; 12   : 	default:
; 13   : 		printf("Foo\n");

	mov	esi, DWORD PTR __imp__printf
	push	OFFSET ??_C@_04DEEPCGMJ@Foo?6?$AA@
	call	esi

; 14   : 		break;
; 15   : 	}
; 16   : 
; 17   : 	printf("Bar\n");

	push	OFFSET ??_C@_04DFOOMHBJ@Bar?6?$AA@
	call	esi
	add	esp, 8

; 18   : 
; 19   : 	return 0;

	xor	eax, eax
	pop	esi

; 20   : }

	ret	0
_wmain	ENDP
_TEXT	ENDS
END
