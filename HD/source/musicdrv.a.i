VERSION		EQU	1
REVISION	EQU	205
DATE	MACRO
		dc.b	'24.8.95'
	ENDM
VERS	MACRO
		dc.b	'musicdrv.a 1.205'
	ENDM
VSTRING	MACRO
		dc.b	'musicdrv.a 1.205 (24.8.95)',13,10,0
	ENDM
VERSTAG	MACRO
		dc.b	0,'$VER: musicdrv.a 1.205 (24.8.95)',0
	ENDM
