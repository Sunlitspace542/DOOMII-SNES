VERSION		EQU	1
REVISION	EQU	608
DATE	MACRO
		dc.b	'25.10.95'
	ENDM
VERS	MACRO
		dc.b	'XB 1.608'
	ENDM
VSTRING	MACRO
		dc.b	'XB 1.608 (25.10.95)',13,10,0
	ENDM
VERSTAG	MACRO
		dc.b	0,'$VER: XB 1.608 (25.10.95)',0
	ENDM
