;***************************************************************************
;*                                                                         *
;*                               C U T T E R                               *
;*                                                                         *
;*                              PICTURE MODULE                             *
;*                                                                         *
;***************************************************************************


	xref	_LVOOpenLibrary,_LVOCloseLibrary
	xref	_LVOOpenScreen,_LVOCloseScreen
	xref	_LVOOpenWindow,_LVOCloseWindow
	xref	_LVOSetDrMd,_LVOSetAPen,_LVOMove,_LVODraw,_LVORectFill
	xref	_LVOModifyIDCMP
	xref	_LVOOpen,_LVOClose,_LVORead,_LVOSeek
	xref	_LVOAllocMem,_LVOFreeMem
	xref	_LVOOpenScreenTagList,_LVOLoadRGB4

	xref	_IntuitionBase,_GfxBase,_DOSBase


	section	IFF,CODE

	xdef	OpenPictureSNES,PictureLoadSNES,ClosePicture,SetPicPlanes
	xdef	FindIFFChunk,DrawIFFPic
	xdef	@ReadIFF,__STD_CloseIFF
	xdef	@MarkIFF1x1,@MarkIFF2x2


;
;	* * * * * * *       FIND AN IFF CHUNK       * * * * * * *
;
;	A2 = IFF Data Block
;	D0 = Chunk ID to Find
;
FindIFFChunk
	move.l	4(a2),d2					; D2 = Length of IFF FORM
	add.w	#12,a2						; Skip FORM+Length+ILBM/PBM
	sub.l	#12,d2
FIC200
	subq.l	#4,d2
	beq	FIC900						; At end of file
	bmi	FIC900
	cmp.l	(a2)+,d0					; Same chunk?
	beq	FIC800						; Yes!
	move.l	(a2)+,d1					; D1 = Length
	addq.l	#1,d1
	and.l	#$fffffffe,d1
	sub.l	d1,d2
	add.l	d1,a2
	bra	FIC200
FIC800
	moveq.l	#0,d0
	rts
FIC900
	moveq.l	#-1,d0
	rts


;
;	* * * * * * *       GET IFF BYTE       * * * * * * *
;
;	A2 = IFF Data Pointer
;
;	D0 = IFF Byte returned
;
GetIFFByte
	tst.b	IFFComp						; IFF Compression ON?
	beq	GIB900						; No
	move.b	IFFCompLen,d0				; Any pending length?
	bmi	GIB100
	beq	GIB200
	subq.b	#1,d0						; LEN > 0 = LITERAL RUN
	move.b	d0,IFFCompLen
	move.b	(a2)+,d0
	rts
GIB100
	addq.b	#1,d0						; LEN < 0 = REPEAT RUN
	move.b	d0,IFFCompLen
	move.b	IFFCompData,d0
	rts
GIB200
	move.b	(a2)+,d0					; LEN = 0 = NEXT RUN
	cmp.b	#$80,d0						; NOP?
	beq	GIB200
	move.b	d0,IFFCompLen
	tst.b	d0
	bmi	GIB300
	move.b	(a2)+,d0					; Get LITERAL Byte
	rts
GIB300
	move.b	(a2)+,d0					; Get REPEATED Byte
	move.b	d0,IFFCompData
	rts
GIB900
	move.b	(a2)+,d0					; Return RAW value
	rts


;
;	* * * * * * *       DRAW IFF PICTURE       * * * * * * *
;
;	A2 = IFF Data
;	A5 = PlanePtrs
;	D5 = #Planes-1
;	D6 = #Bytes-1
;	D7 = #ScanLines-1
;
DrawIFFPic
	move.l	PicpChar,a3					; A3 = PixelArray
	move.l	#'BODY',d0					; >>>BODY<<<
	bsr	FindIFFChunk
	bne	DIP990						; Error!
	addq.w	#4,a2						; Skip LENGTH of BODY Chunk
	clr.b	IFFCompLen					; No Compressed Data Pending
	move.w	d7,-(sp)
;
;	>>>   NEXT LINE   <<<
;
DIP200
	tst.b	IFFMode						; ILBM/PBM?
	bne	DIP400						; PBM
;
;	>>>   ILBM   <<<
;
	move.w	d5,d2						; D2 = PlaneCounter
	move.l	a5,a1						; A1 = PlanePtr
DIP300
	move.w	d5,-(sp)
	moveq.l	#0,d5						; D5 = PixelCounter
	move.l	(a1),a0						; A0 = Destination Plane
DIP310
	move.w	d6,d1						; D1 = ByteCounter
	tst.w	d2						; Doing MASK Plane?
	bmi	DIP340						; Yes
	move.l	a0,d0						; Invalid Plane?
	beq	DIP340						; Yes
;
;	>>>   REGULAR PLANE   <<<
;
DIP320
	bsr	GetIFFByte					; Get Next IFF Byte
	cmp.w	#256,d5						; Past 256 pixels?
	bge.s	DIP330						; Yes
	move.b	d0,(a0)+
DIP330
	addq.w	#8,d5
	dbf	d1,DIP320					; Next Byte
	bra	DIP380						; Finished this line
;
;	>>>   MASK PLANE / INVALID PLANE   <<<
;
DIP340
	bsr	GetIFFByte					; Get Next IFF Byte
;	addq.w	#8,d5
	dbf	d1,DIP340					; Next Byte
	bra	DIP390
;
;	>>>   MOVE TO NEXT SCANLINE OF REGULAR PLANE   <<<
;
DIP380
	move.l	#(256/8),d0					; Move to next PlaneLine
	add.l	d0,(a1)+
;
;	>>>   DO NEXT PLANE   <<<
;
DIP390
	move.w	(sp)+,d5					; Restore (#Planes-1)
	tst.w	d2						; Doing MASK Plane?
	bmi.s	DIP3000						; Yes!
	dbf	d2,DIP300					; Next Plane
	tst.b	IFFMask						; MASK?
	bne	DIP310						; Yes, skip data for a single plane
DIP3000
	move.w	d5,d0						; D0 = #Planes Done
DIP3100
	addq.w	#1,d0
	cmp.w	#8,d0						; Done all 8 planes?
	beq	DIP800						; Yes
	move.l	(a1)+,a0					; A0 = Address of Plane
	clr.l	(a0)+						; Clear out the plane
	clr.l	(a0)+
	clr.l	(a0)+
	clr.l	(a0)+
	clr.l	(a0)+
	clr.l	(a0)+
	clr.l	(a0)+
	clr.l	(a0)+
	bra	DIP3100
;
;	>>>   PBM   <<<
;
DIP400
	move.w	d5,-(sp)
	move.w	d6,d1						; D1 = PixelCounter
	moveq.l	#8-1,d3						; D3 = Mod8 PixelCount
	moveq.l	#0,d5
DIP420
	bsr	GetIFFByte					; Get Next IFF Byte
	cmp.w	#256,d5						; Past 256 pixels?
	bge	DIP460						; Yes
	moveq.l	#0,d2						; D2 = PlaneCounter
	move.l	a5,a1						; A1 = PlanePtr
DIP430
	tst.l	(a1)						; Any plane here?
	beq	DIP450						; No
	move.l	(a1),a0						; A0 = PicPlane
	bclr	d3,(a0)
	btst	d2,d0						; Bit Set?
	beq	DIP440						; No
	bset	d3,(a0)
DIP440
	tst.b	d3						; At end of byte?
	bne	DIP450
	addq.l	#1,(a1)						; Yes, move over one
DIP450
	addq.w	#4,a1						; Move to next plane
	addq.w	#1,d2
	cmp.w	#8,d2
	bne	DIP430						; Next Plane Bit
DIP460
;
;	>>>   NEXT PIXEL   <<<
;
	addq.w	#1,d5						; Next Pixel Over
	subq.b	#1,d3						; Next Pixel Over
	bpl     DIP420
	and.b	#$7,d3
	dbf	d1,DIP420					; Next Byte
	move.w	(sp)+,d5
;
;	>>>   NEXT SCANLINE   <<<
;
DIP800
	tst.l	IFFFlipX			; Flip IFF Around "X"?
	beq	DIP810
	jsr	FlipIFFX			; Yes, Flip It
DIP810
	move.l	d5,-(sp)
;
	move.l	#(256/8),d5			; D5 = Offset to last PlaneLine
	moveq.l	#0,d4				; D4 = PixelCounter
	moveq.l	#8-1,d3				; D3 = Mod8 PixelCount
DIP820
	moveq.l	#0,d2				; D2 = PlaneCounter
	move.l	a5,a1				; A1 = PlanePtr
	moveq.l	#0,d0				; D0 = ColourByte
DIP830
	tst.l	(a1)				; Any plane here?
	beq	DIP840				; No
	move.l	(a1),a0				; A0 = PicPlane
	sub.l	d5,a0
	btst	d3,(a0)				; Is this bit set?
	beq	DIP840
	bset	d2,d0				; Yes
DIP840
	addq.w	#4,a1				; Move to next plane
	addq.w	#1,d2
	cmp.w	#8,d2
	bne	DIP830				; Next Plane Bit
;
;	>>>   NEXT PIXEL   <<<
;
	subq.b	#1,d3
	bpl	DIP860
	subq.l	#1,d5				; Next Pixel Over
DIP860
	and.b	#$7,d3
;
	and.b	#$0f,d0				; Only Save Lowest 16 Colours!
	move.b	d0,(a3)+			; Save generated byte
	addq.w	#1,d4				; Next Pixel
	cmp.w	#256,d4				; Only want first 256
	bne	DIP820
;
	move.l	(sp)+,d5
	dbf	d7,DIP200			; Next Line
;
;	>>>   FILL TO END OF PICSCREEN   <<<
;
	move.w	(sp)+,d0			; D0 = (#ScanLines-1)
	addq.w	#1,d0				; D0 = #ScanLines
	cmp.w	#256,d0				; At least 256?
	bge	DIP890				; Yes
	move.w	#256,d7				; No, clear CharPicByte Buffer
	sub.w	d0,d7				; D7 = #ScanLines to Clear
	move.w	d7,-(sp)
	moveq.l	#0,d0
	move.l	d0,d1
	move.l	d0,d2
	move.l	d0,d3
	move.l	d0,d4
	move.l	d0,d5
	move.l	d0,d6
	bra	DIP880
DIP870
	adda.l	#256,a3
	movem.l	d0-d6,-(a3)			; Clear 1 scanline of data
	movem.l	d0-d6,-(a3)
	movem.l	d0-d6,-(a3)
	movem.l	d0-d6,-(a3)
	movem.l	d0-d6,-(a3)
	movem.l	d0-d6,-(a3)
	movem.l	d0-d6,-(a3)
	movem.l	d0-d6,-(a3)
	movem.l	d0-d6,-(a3)			; 28
	move.l	d0,-(a3)			; 4
	adda.l	#256,a3
DIP880
	dbf	d7,DIP870
;
;	>>>   CLEAR PICTURE IMAGE DATA PAST END   <<<
;
	move.w	(sp)+,d7			; D7 = (#ScanLines-1)
	moveq.l	#0,d2
	move.l	d2,d3
	move.l	d3,d4
	move.l	d4,d5
	bra	DIP1800
DIP1000
	moveq.l	#8-1,d1				; D1 = PlaneCounter
	move.l	a5,a1				; A1 = PlanePtr
DIP1100
	tst.l	(a1)				; Any plane here?
	beq	DIP1300				; No
	move.l	(a1),a0				; A0 = PicPlane
	add.l	#(256/8),a0
	movem.l	d2-d5,-(a0)
	movem.l	d2-d5,-(a0)
	add.l	#(256/8),a0
	move.l	a0,(a1)
DIP1300
	addq.w	#4,a1				; Move to next plane
	dbf	d1,DIP1100
DIP1800
	dbf	d7,DIP1000			; Next Line
;
DIP890
	moveq.l	#-1,d0				; No Problems!
	rts
DIP990
	moveq.l	#0,d0				; Error!
	rts


;
;	* * * * * * *       FLIP IFF IMAGE AROUND "X"       * * * * * * *
;
FlipIFFX
	moveq.l	#8-1,d3				; D3 = PlaneCounter
	move.l	a5,a1				; A1 = PlanePtr
FIX200
	tst.l	(a1)				; Any plane here?
	beq	FIX800				; No
	move.l	(a1),a0				; A0 = PicPlane
	sub.l	#(256/8),a0			; Move to last PlaneLine
;
	movem.l	a1-a2,-(sp)
	lea	(256/8)(a0),a1			; A1 = End of Line
	lea	RLCFlippedBits(pc),a2
	move.w	#(128/8)-1,d2			; D2 = #Bytes to Flip
	moveq.l	#0,d0
	moveq.l	#0,d1
FIX300
	move.b	(a0),d0				; D0 = LeftByte
	move.b	-(a1),d1
	move.b	(a2,d0.w),(a1)
	move.b	(a2,d1.w),(a0)+
	dbf	d2,FIX300
	movem.l	(sp)+,a1-a2
;
	move.l	(a1),a0				; Shift image 1 pixel to right
	sub.l	#(256/8),a0			; Move to last PlaneLine
	move.w	#(256/8/4)-1,d1			; #Longs to Shift
	andi	#$0f,ccr			; Clear .X
FIX720
	move.l	(a0),d0
	roxr.l	#1,d0
	move.l	d0,(a0)+
	dbf	d1,FIX720
;
FIX800
	addq.w	#4,a1				; Move to next plane
	dbf	d3,FIX200			; Next Plane
	rts

;
;	* * * * * * *       RLC FLIPPED BITS TABLE       * * * * * * *
;
RLCFlippedBits
	dc.b	$00,$80,$40,$c0,$20,$a0,$60,$e0,$10,$90,$50,$d0,$30,$b0,$70,$f0
	dc.b	$08,$88,$48,$c8,$28,$a8,$68,$e8,$18,$98,$58,$d8,$38,$b8,$78,$f8
	dc.b	$04,$84,$44,$c4,$24,$a4,$64,$e4,$14,$94,$54,$d4,$34,$b4,$74,$f4
	dc.b	$0c,$8c,$4c,$cc,$2c,$ac,$6c,$ec,$1c,$9c,$5c,$dc,$3c,$bc,$7c,$fc
	dc.b	$02,$82,$42,$c2,$22,$a2,$62,$e2,$12,$92,$52,$d2,$32,$b2,$72,$f2
	dc.b	$0a,$8a,$4a,$ca,$2a,$aa,$6a,$ea,$1a,$9a,$5a,$da,$3a,$ba,$7a,$fa
	dc.b	$06,$86,$46,$c6,$26,$a6,$66,$e6,$16,$96,$56,$d6,$36,$b6,$76,$f6
	dc.b	$0e,$8e,$4e,$ce,$2e,$ae,$6e,$ee,$1e,$9e,$5e,$de,$3e,$be,$7e,$fe
	dc.b	$01,$81,$41,$c1,$21,$a1,$61,$e1,$11,$91,$51,$d1,$31,$b1,$71,$f1
	dc.b	$09,$89,$49,$c9,$29,$a9,$69,$e9,$19,$99,$59,$d9,$39,$b9,$79,$f9
	dc.b	$05,$85,$45,$c5,$25,$a5,$65,$e5,$15,$95,$55,$d5,$35,$b5,$75,$f5
	dc.b	$0d,$8d,$4d,$cd,$2d,$ad,$6d,$ed,$1d,$9d,$5d,$dd,$3d,$bd,$7d,$fd
	dc.b	$03,$83,$43,$c3,$23,$a3,$63,$e3,$13,$93,$53,$d3,$33,$b3,$73,$f3
	dc.b	$0b,$8b,$4b,$cb,$2b,$ab,$6b,$eb,$1b,$9b,$5b,$db,$3b,$bb,$7b,$fb
	dc.b	$07,$87,$47,$c7,$27,$a7,$67,$e7,$17,$97,$57,$d7,$37,$b7,$77,$f7
	dc.b	$0f,$8f,$4f,$cf,$2f,$af,$6f,$ef,$1f,$9f,$5f,$df,$3f,$bf,$7f,$ff


;
;	* * * * * * *       OPEN SNES PICTURE       * * * * * * *
;
OpenPictureSNES
	tst.l	PicScreen			; Screen already open?
	bne	OPSNS500			; Yes, just set new Palette!
	move.l	_IntuitionBase,a6		; intuition.library
;	move.w	PicY,d1
;	cmp.w	#256,d1				; At least 256 tall?
;	bge	OPSNS200
;	move.w	#256,d1				; No, make it 256 pixels
;OPSNS200
;	add.w	#11,d1
;	move.w	PicX,d2
;	move.w	PicNumPlanes,d3
	lea	NewSNESPicScreen,a0
;	move.w	d1,6(a0)
;	move.w	d2,4(a0)
;	move.w	d3,8(a0)
	lea	NewSNESPicScreenTags,a1
	jsr	_LVOOpenScreenTagList(a6)
	tst.l	d0				; Did the screen open?
	beq	OPSNS800			; No!
	move.l	d0,PicScreen
	move.l	d0,NewSNESPicWindowScreen
	move.l	d0,a1
	add.l	#$2c,d0
	move.l	d0,PicVPort
;
	lea	NewSNESPicWindow,a0
	move.w	$c(a1),d0
	move.w	$e(a1),d1
	move.w	d0,4(a0)
	sub.w	#11,d1
	move.w	d1,6(a0)
	jsr	_LVOOpenWindow(a6)
	tst.l	d0
	beq	OPSNS800
	move.l	d0,PicWindow
;
OPSNS500
	move.l	_GfxBase,a6			; Load 256 Colours
	move.l	PicVPort,a0
	lea	PicPalAmiga,a1
	move.l	#256,d0
	jsr	_LVOLoadRGB4(a6)
;
	moveq.l	#0,d0
	rts
OPSNS800
	bsr	ClosePicture			; Close Anything Opened!
	moveq.l	#-1,d0
	rts


;
;	* * * * * * *       CLOSE PICTURE       * * * * * * *
;
ClosePicture
__STD_CloseIFF
	move.l	_IntuitionBase,a6		; intuition.library
	lea	PicWindow,a1			; Window Open?
	tst.l	(a1)
	beq	CPE700
	move.l	(a1),a0
	clr.l	(a1)
	clr.l	$56(a0)
	jsr	_LVOCloseWindow(a6)
CPE700
	lea	PicScreen,a1			; Screen Open?
	tst.l	(a1)
	beq	CPE800
	move.l	(a1),a0
	clr.l	(a1)
	jsr	_LVOCloseScreen(a6)
CPE800
	rts


;
;	* * * * * * *       LOAD A SNES PICTURE       * * * * * * *
;
@ReadIFF
PictureLoadSNES
;
;	>>>   READ THE IFF FILE   <<<
;
	move.l	d0,IFFFlipX			; Flip Image around "X"?
	move.l	a0,d1				; Name
	move.l	a1,PicpChar
	move.l	_DOSBase,a6			; dos.library
	move.l	#1005,d2			; MODE_OLDFILE
	jsr	_LVOOpen(a6)
	tst.l	d0
	beq	PLSNS900					; File doesn't exist!
	lea		PicFIB,a0				; Save FIB
	move.l	d0,(a0)
	move.l	d0,d1						; Move to end
	moveq.l	#0,d2
	move.l	#1,d3
	jsr		_LVOSeek(a6)
	move.l	PicFIB,d1				; Move back to beginning
	moveq.l	#0,d2
	moveq.l	#-1,d3
	jsr		_LVOSeek(a6)				; D0 = SIZE OF FILE
	lea		PicIFFSize,a0
	move.l	d0,(a0)
	move.l	4,a6
	moveq.l	#0,d1
	jsr		_LVOAllocMem(a6)
	tst.l	d0
	beq	PLSNS900					; Error!
	lea		PicIFF,a0
	move.l	d0,(a0)
	move.l	_DOSBase,a6				; Read the entire file in
	move.l	PicFIB,d1
	move.l	d0,d2
	move.l	PicIFFSize,d3
	jsr		_LVORead(a6)
	cmp.l	PicIFFSize,d0
	bne	PLSNS900					; Error with file read
	lea		PicFIB,a0
	move.l	(a0),d1
	clr.l	(a0)
	jsr		_LVOClose(a6)
;
;	>>>   DETERMINE PICTURE FORMAT   <<<
;
	move.l	PicIFF,a2				; >>>ILBM/PBM <<<
	move.l	8(a2),d0
	moveq.l	#0,d1
	cmp.l	#'ILBM',d0
	beq	PLSNS200
	moveq.l	#1,d1
	cmp.l	#'PBM ',d0
	bne	PLSNS900
PLSNS200
	move.b	d1,IFFMode					; 0=ILBM,1=PBM
;
;	>>>   DETERMINE PICTURE CONFIGURATION   <<<
;
	move.l	PicIFF,a2				; >>>BMHD<<<
	move.l	#'BMHD',d0
	jsr		FindIFFChunk
	bne	PLSNS900					; Error!
;
;	>>>   GET # BITPLANES   <<<
;
	moveq.l	#0,d0
	move.b	$c(a2),d0					; Get #BitPlanes
	cmp.b	#8,d0						; 8 BitPlanes?
;	bne	PLSNS900					; Too many
	lea		PicNumPlanes,a0
	move.w	d0,(a0)
;
;	>>>   GET STENCIL(MASK) / COMPRESSION STATUS   <<<
;
	move.b	$d(a2),IFFMask			; STENCIL Mask?
	move.b	$e(a2),IFFComp			; Compressed data?
;
;	>>>   GET DIMENSIONS   <<<
;
	moveq.l	#0,d0				; Save RASTER X and Y
	lea	PicX,a0
	move.w	4(a2),d0
	cmp.w	#256,d0				; Must be at least 256 pixels wide!
	blt	PLSNS900
	move.w	d0,(a0)
	lea	PicXBytes,a0
	add.w	#15,d0
	lsr.w	#4,d0				; #Words
	add.w	d0,d0				; #Bytes
	move.w	d0,(a0)
	lea	PicXNBytes,a0			; #BYTES FOR SNES SCREENS
	move.w	d0,(a0)
	lea	PicY,a0
	move.w	6(a2),d0
;	cmp.w	#256,d0				; Must be at least 256 pixels tall!
;	blt	PLSNS900
	move.w	d0,(a0)
	lea	PicYBytes,a0
	lsr.w	#3,d0				; #Lines
	move.w	d0,(a0)
;
;	>>>   EXTRACT PALETTE INFORMATION   <<<
;
	move.l	PicIFF,a2				; >>>CMAP<<<
	move.l	#'CMAP',d0
	jsr	FindIFFChunk
	bne	PLSNS900					; Error!
;
	move.l	(a2)+,d6					; D6 = Size of CMAP Chunk
	addq.l	#1,d6
	and.l	#$fffffffe,d6
	divu	#3,d6						; Get #Colours in palette
	cmp.l	#256,d6						; Too many colours?
	bgt	PLSNS900					; Yes
	subq.w	#1,d6						; Adjust for DBF Loop
;
;	>>>   CONVERT IFF COLOURS TO AMIGA COLOURS   <<<
;
	lea		PicPalAmiga,a0			; A0 = AMIGA Colours
PLSNS350
	moveq.l	#0,d1						; Convert IFF Colour
	move.b	(a2),d1
	and.b	#$f0,d1
	lsl.w	#4,d1
	move.b	1(a2),d1
	and.b	#$f0,d1
	move.b	2(a2),d0
	and.b	#$f0,d0
	lsr.b	#4,d0
	or.b	d0,d1
	move.w	d1,(a0)+				; Save AMIGA Colour
	addq.w	#3,a2
	dbf	d6,PLSNS350
	
;jpm color additions

;              RGB
;-------------------
	move.w	#$0a2a,PicPalAmiga+(16*2)
	move.w	#$02a2,PicPalAmiga+(17*2)
	move.w	#$022a,PicPalAmiga+(18*2)
	move.w	#$0a22,PicPalAmiga+(19*2)
	
	move.w	#$0f4f,PicPalAmiga+(20*2)
	move.w	#$04f4,PicPalAmiga+(21*2)
	move.w	#$044f,PicPalAmiga+(22*2)
	move.w	#$0f44,PicPalAmiga+(23*2)

	move.w	#$0fcf,PicPalAmiga+(24*2)
	move.w	#$08f8,PicPalAmiga+(25*2)
	move.w	#$088f,PicPalAmiga+(26*2)
	move.w	#$0f88,PicPalAmiga+(27*2)
;
;	>>>   OPEN PICTURE SCREEN   <<<
;
;	bsr	ClosePicture
	bsr	OpenPictureSNES				; Can we open the picture?
	bne	PLSNS900				; No, Error!
;
;	>>>   SET UP PICTURE PLANES   <<<
;
	bsr	SetPicPlanes				; Set PicPlanes
;
;	>>>   UNPACK THE IFF IMAGERY   <<<
;
	move.l	PicIFF,a2				; A2 = IFF Data Block
	moveq.l	#0,d7					; D7 = #Lines-1
	move.w	PicY,d7
	cmp.w	#256,d7					; Only want first 256 lines
	ble	PLSNS800
	move.w	#256,d7
PLSNS800
	subq.w	#1,d7
	moveq.l	#0,d6					; D6 = #Bytes-1
	move.w	PicXBytes,d6
	subq.w	#1,d6
	moveq.l	#0,d5					; D5 = #Planes-1
	move.w	PicNumPlanes,d5
	subq.w	#1,d5
	jsr	DrawIFFPic				; Draw the IFF Imagery
	bra	PLSNS910				; COMPLETED!
;
;	>>>   FINISHED   <<<
;
PLSNS900
	moveq.l	#0,d0					; D0 = 0 = ERROR!
PLSNS910
	move.l	d0,-(a7)				; Save Return Code
	move.l	4,a6					; Release the Memory
	move.l	PicIFFSize,d0
	lea		PicIFF,a0
	tst.l	(a0)
	beq	PLSNS920
	move.l	(a0),a1
	clr.l	(a0)
	jsr	_LVOFreeMem(a6)
PLSNS920
	move.l	_DOSBase,a6
	lea	PicFIB,a0
	move.l	(a0),d1					; Any file opened?
	clr.l	(a0)
	beq	PLSNS980
	jsr	_LVOClose(a6)
PLSNS980
	move.l	(a7)+,d0				; D0 = Error Code
	rts


;
;	* * * * * * *       SET PIC PLANES       * * * * * * *
;
SetPicPlanes
	lea		PicPlanes,a5			; A5 = PlanePointer
	move.l	a5,a2
	move.l	PicScreen,a0			; A0 = Screen
	move.l	$58(a0),a0					; A0 = RPort->BitMap
	addq.w	#4,a0
	move.w	(a0)+,d2					; D2 = #Valid Planes
	and.w	#$00ff,d2
	addq.w	#2,a0						; A0 = PLANES
	move.l	#0,d1
;	move.w	PicXBytes,d1			; ScanLine Width
;	mulu	#11,d1						; 11 lines down
	move.l	#(256/8*11),d1
	moveq.l	#8-1,d6						; D6 = #Planes
SPP200
	moveq.l	#0,d0
	tst.w	d2							; Valid plane here?
	beq	SPP300						; No!
	move.l	(a0)+,d0					; D0 = PlanePtr
	add.l	d1,d0						; Add Offset to Window
	subq.w	#1,d2						; One less plane
SPP300
	move.l	d0,(a2)+					; Save PLANEPTR
	dbf		d6,SPP200
	rts


;
;	* * * * * * *       MARK A 1X1       * * * * * * *
;
@MarkIFF1x1
	moveq.l	#8-1,d2				; D2 = Size of each side
	bra	@MarkIFF
@MarkIFF2x2
	moveq.l	#16-1,d2			; D2 = Size of each side
@MarkIFF
	move.l	d0,-(sp)			; Save X coordinate
	add.l	#11,d1
	move.l	d1,-(sp)			; Save Y coordinate
	move.l	_GfxBase,a6			; graphics.library
	move.l	PicScreen,a1			; Set APen
	lea	$54(a1),a1
	move.l	12(sp),d0
	jsr	_LVOSetAPen(a6)
	move.l	PicScreen,a1			; Move to top-left
	lea	$54(a1),a1
	move.l	(sp),d1
	move.l	4(sp),d0
	jsr	_LVOMove(a6)
	move.l	PicScreen,a1			; First Line
	lea	$54(a1),a1
	move.l	(sp),d1
	move.l	4(sp),d0
	add.l	d2,d0
	jsr	_LVODraw(a6)
	move.l	PicScreen,a1			; Second Line
	lea	$54(a1),a1
	move.l	(sp),d1
	add.l	d2,d1
	move.l	4(sp),d0
	add.l	d2,d0
	jsr	_LVODraw(a6)
	move.l	PicScreen,a1			; Third Line
	lea	$54(a1),a1
	move.l	(sp),d1
	add.l	d2,d1
	move.l	4(sp),d0
	jsr	_LVODraw(a6)
	move.l	PicScreen,a1			; Fourth Line
	lea	$54(a1),a1
	move.l	(sp)+,d1
	move.l	(sp)+,d0
	jmp	_LVODraw(a6)


	section	__MERGED,DATA

	xdef	PicY,PicYBytes
	xdef	PicX,PicXBytes,PicXNBytes
	xdef	PicNumPlanes,PicPlanes
	xdef	PicIFF,PicIFFSize
	xdef	PicPalAmiga
	xdef	PicFIB

	xdef	IFFMode,IFFMask,IFFComp,IFFCompLen,IFFCompData

	xdef	PicScreen,PicWindow,PicVPort
	xdef	NewSNESPicScreen
	xdef	NewSNESPicScreenTags
	xdef	NewSNESPicWindow,NewSNESPicWindowScreen


;
;	* * * * * * *       VARIABLES       * * * * * * *
;
PicpChar		dc.l	0		; Pointer to PictureArray

PicY			dc.w	0		; Y Dimension
PicYBytes		dc.w	0		; Y Dimension #Lines
PicX			dc.w	0		; X Dimension
PicXBytes		dc.w	0		; X Dimension #Bytes
PicXNBytes		dc.w	0		; NES/SNES X Dimension #Bytes
PicNumPlanes		dc.w	0		; Number of Planes in IFF Imagery
PicIFF			dc.l	0		; IFF File
PicIFFSize		dc.l	0		; Size of IFF File
PicPalAmiga		ds.w	256		; 256 Amiga Colour Words
PicPlanes		ds.l	8		; 8 PlanePointers
PicFIB			dc.l	0		; FIB for Picture

IFFMode			dc.b	0		; 0=ILBM,1=PBM
IFFMask			dc.b	0		; 0=No Mask/Stencil,1=Stencil/Mask ON
IFFComp			dc.b	0		; Compression Format
IFFCompLen		dc.b	0		; Compression Length
IFFCompData		dc.b	0		; Compression DataByte

IFFFlipX		dc.l	0		; Flip Image around "X"?

;
;	* * * * * * *       TEXT TITLES       * * * * * * *
;
NewSNESPicScreenTitle
	dc.b	'CUTTER SNES Picture Screen',0

	dc.w	0


;
;	* * * * * * *       SNES PICTURE SCREEN/WINDOWS       * * * * * * *
;
NewSNESPicScreen
	dc.w	0,50,256,256+11,8		; -1,-1,5
	dc.b	0,1
	dc.w	$0000
	dc.w	$000f
	dc.l	0
	dc.l	NewSNESPicScreenTitle
	dc.l	0
	dc.l	0

NewSNESPicScreenTags
	dc.l	$8000003a,NewSNESPicScreenCols
	dc.l	$80000034,1
	dc.l	$80000039,-1
	dc.l	0,0
NewSNESPicScreenCols
	dc.w	-1

NewSNESPicWindow
	dc.w	0,11,-1,-1
	dc.b	0,1
	dc.l	$00000000
	dc.l	$00000900
	dc.l	0	;SNESPicWindowGadgets
	dc.l	0
	dc.l	0
NewSNESPicWindowScreen
	dc.l	0
	dc.l	0
	dc.w	-1,-1,-1,-1
	dc.w	$000f

;
;	* * * * * * *       SCREENS/WINDOWS POINTERS       * * * * * * *
;
PicScreen		dc.l	0				; Picture Screen Pointer
PicWindow		dc.l	0				; Picture Window Pointer
PicVPort		dc.l	0				; Picture ViewPort


	end
