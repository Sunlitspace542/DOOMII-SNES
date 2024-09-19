***********************************************
*
* Gamepatch.s
*
* Routines all game patches could probably well use.
*
* Copyright (C) 1995, Catapult Entertainment, Inc.
* 20823 Stevens Creek Blvd., Suite 300
* Cupertino, CA, 95014
* (408) 366-1735
*
* By Richard Kiss
* (408) 366-1735 x253
* kiss@catapent.com
*
* Please call or e-mail if you have any questions or problems!
*
***********************************************
*
* Change history:
*
* 9/7/95: don't clear any of the GR_ area, since the OS does this. Call InitGamePatch
*	on first invokation (only!) of the patch.
*
***********************************************

;
; OS Features that must be supported:
;	Ã joystick value swapping
;		¥ latency should be available (call sync-o-tron)
;		Ã auto error counting
;		Ã line noise errors
;		Ã call waiting errors
;	Ã single player mode
;		Ã keep track of registered, timeout, ring detect, call to return to OS when appropriate
;		Ã single player mode LED juggling
;	¥ LED juggling [using gTicks]
;	¥ evil reset detect
;	Ã ending the game : GameOver
;		Ã auto-calculate the time it took
;
;	¥ NEED TO SET THE MODEM BUSY WHEN REGISTERS MOVE!!!!
;		¥ or maybe shadow the control register?
;
; 	Ã SINGLE PLAYER MODE WITH NO TIMEOUT!!!!!!!
;

***********************************************
*
* Includes
*
***********************************************

			include 'Macros.i'
			include	'Flags.i'
			include 'MemCfg.i'

***********************************************
*
* Memory mapping equates
*
***********************************************

rom_med_start		equ	$d00000
rom_med_end			equ	$dfc000

ram_med_start		equ	$e00000
ram_med_end			equ	$e10000

ram_moves_soft_med	equ	(ram_soft_end<>ram_med_end)
rom_moves_soft_med	equ	(rom_soft_end<>rom_med_end)
soft_med_different	equ	(ram_moves_soft_med|rom_moves_soft_med)

mdFred_Hard			equ	$fbc000

mdKill_Hard			equ	$fbfc01
mdControl_Hard		equ	$fbfc03

mdKill_Soft			equ	rom_soft_end

mdControl_Soft		equ	$4f03
mdControl_Med		equ	$4f03

				if	(Use_Fixed_Internals) then
mdFred_Soft			equ	mdFred_Hard
mdFred_Med			equ	mdFred_Hard
				else
mdFred_Soft			equ	rom_soft_end+$200
mdFred_Med			equ	mdFred_Hard
				endif


mdRangeStart0		equ	$2c*2+1
mdMagicAddr			equ	$38*2+1
mdRangeEnd0			equ	$40*2+1
mdRangeDestination0	equ	$50*2+1
mdRangeMask0		equ	$52*2+1
mdRAMBase			equ	$60*2+1
mdRAMBound			equ $64*2+1
mdVectorTableBase	equ $68*2+1
mdHitEnables		equ $6c*2+1
mdROMBound			equ $70*2
mdROMBase			equ $74*2
mdSNESControl		equ	$78*2
mdSRAMProtect		equ	$79*2
mdLED_Data			equ	$b4*2+1
mdRingCheck			equ	$cf*2+1		;$fbc19f

mdTop_LED_Mask		equ	$02
mdMiddle_LED_Mask	equ	$08
mdBottom_LED_Mask	equ	$20

magic_address		equ	$fffffe


***********************************************
*
* XBAND OS and hardware defines
*
***********************************************

kGPInitGamePatch	equ 0
kGPStartGamePatch	equ 1
kGPDisplayMessage	equ 2
kGPStopGame			equ 3
kGPKillGame			equ 4
kGPReportError		equ 5
kGPPlaySinglePlayerGame	equ 6

kGameTimeout			equ	-426
kCallWaitingErr			equ	-427
kRemoteCallWaitingErr	equ	-428
kSinglePlayerGameOver	equ	-709
kNetRegisterTimeout		equ	-710

gTicks				equ	$20
gRegisterBase		equ	$c0		; where Fred is (required if you move Fred)

kOSHandleGameError	equ	0
kOSGameOver			equ	1
kOSCheckLine		equ	24

OSDispatcher		equ	$e000cc


***********************************************
*
* Data uploaded to the server. The latter fields are zeroed out here.
*
***********************************************

					rsset	$2c88

GR_size					rl	1
GR_gameID				rl	1
GR_connectPhase			rb	1
GR_errorWhere			rb	1
GR_gameError			rw	1
GR_localPlayer1Result	rl	1
GR_localPlayer2Result	rl	1
GR_remotePlayer1Result	rl	1
GR_remotePlayer2Result	rl	1
GR_playTime				rl	1
GR_dblDDataPtr			rl	1
GR_dblDDataSize			rl	1

GR_LocalGameError		rw	1
GR_ErrorRecovers		rw	1
GR_ChecksumErrors		rb	1
GR_Timeouts				rb	1
GR_FrameErrors			rb	1
GR_OverrunErrors		rb	1	; this stuff is recorded in server report

;; now stuff reserved for use by the game

GR_GameReserved			rl	8
GR_numLocalPlayers		rw	1
GR_pad					rw	1
GR_End					rb	0

***********************************************
*
* SNES Defines
*
***********************************************

NMITimEn	equ	$4200
RdNMI		equ	$4210
TimeUp		equ	$4211

BRK_Vector	equ	$ffe6

***********************************************
*
* These external references need to be declared
* by the game patch. Control transfers to them
* after the memory map is all set up.
*
***********************************************

				xref	InitNewGame
				xref	InitGamePatch
				xref	StartModemGame
				xref	SaveGameState
				xref	Start1PGame

***********************************************
*
* These external definitions. Used by other modules.
*
***********************************************

; data

				xdef	shadow_mdHitEnables		; required for patching
				xdef	brk_router				; ditto
				xdef	Patch_Substitution

; code

				xdef	RingDetect
				xdef	WEnableSRAM
				xdef	WDisableSRAM
				xdef	End_If_1P_Done
				xdef	Game_Over
				xdef	Timeout_Exit
				xdef	CalcPlayTime

				xdef	Check_Line

***********************************************
*
* Variables for GamePatch.s
*
***********************************************

				section	Variables

shadow_mdHitEnables
				ds	2

brk_router		ds	22
				ds	33	; it just needs to add up to 55 bytes

StackSave		ds	$200

Save1200		ds	8	; save $1200 through to $1207 inclusive

Timeout			ds	4

Registered		ds	2

EverFinish1P	ds	2

startGameTicks	ds	4

Patch_Substitution	equ	$2500

***********************************************
*
* Entry point. See what part of the game patch the OS wants us to call.
*
***********************************************

				section code

PatchEntry
				ai16
				phk
				plb

			if ModemGameTest then
				tsc
				sec
				sbc	#20			; cheesy
				tcs
				lda	#kGPStartGamePatch
				sta	4,s
		 	endif

			if SinglePlayerTest then
				tsc
				sec
				sbc	#20			; cheesy
				tcs
				lda	#kGPPlaySinglePlayerGame
				sta	4,s
				lda	#60*45
				clc
				adc	|gTicks
				sta	12,s
				lda	|gTicks+2
				adc	#0
				sta	14,s
			endif

				lda	4,s

				cmp	#kGPInitGamePatch
				bne	@2

				jsr	InitGamePatch
				jsr	InitNewGame

; initialize new game

;				lda	#$100	;Variables_End-Variables_Start
;				pha
;				ldx	#kOSSetGamePatchDataSize	; 5
;				jsl	OSDispatcher
;				ldx	#kOSGetGamePatchDataPtr		; 7
;				jsl	OSDispatcher	; this returns information that is not really useful anymore

				lda	|gTicks
				sta	|startGameTicks
				lda	|gTicks+2
				sta	|startGameTicks+2
				bra	@done

@2
				cmp	#kGPStartGamePatch
				bne	@3

; It's a modem game!

				jsr	SetUp_Memory
				jmp	StartModemGame

@3
				cmp	#kGPPlaySinglePlayerGame
				bne	@4

; It's a single player game!

				lda	12,s
				sta	|Timeout
				lda	14,s
				sta	|Timeout+2
				ora	12,s
				beq	@zero

				lda	#-1
@zero
				sta	|Registered
				sta	|EverFinish1P	; if this is set, we finish a 1P game when Registered is clear

				jsr	SetUp_Memory
				jmp	Start1PGame
@4
				cmp	#kGPStopGame
				bne	@5

; Stop the game! I want to get off!

				bra	@done
@5
				cmp	#kGPKillGame
				bne	@6

; Kill the game!

				bra	@done
@6
				cmp	#kGPReportError
				bne	@7

; report the error

				bra	@done

@7
				cmp	#kGPDisplayMessage
				bne	@done
				ldx	#-1
				txa			; return a -1 : required for call waiting, etc.
				rtl

@done
				lda	#0
				tax			; no error!
				rtl

***********************************************
*
* BRK_Code
*
* This routine gets pointed to by the BRK vector.
*
* It routes patched breaks to the correct place,
* and passes unknown breaks onto the Psy-Q.
*
***********************************************

check_router	macro	number
				lsr	a
				bcc	@not_it\@
				cpy	|brk_router+number*5+1
				bne	@not_it\@
				cpx	|brk_router+number*5
				bne	@not_it\@
				lda	|brk_router+number*5+3
				bra	got_it
@not_it\@
				endm

BRK_Code
				ai16
				pha
				phx
				phy
				phb
				phk
				plb
				lda	9,s
				tax
				lda	10,s
				tay
				lda	|shadow_mdHitEnables

				check_router	0
				check_router	1
				check_router	2
				check_router	3
				check_router	4
				bra	check_remainder
got_it
				plb
				ply
				plx
; stack = <a_lo> <a_hi> <p> <rti_lo> <rti_hi> <rti_bank>
				sta	5,s
; stack = <a_lo> <a_hi> <p> <rti_lo> <rts_lo> <rts_hi>
				lda	2,s
				sta	3,s
; stack = <a_lo> <a_hi> <a_hi> <p> <rts_lo> <rts_hi>
				pla
				plp
				plp
; <rts_lo> <rts_hi>
				rts			; "return" to the brk router
check_remainder
				check_router	5
				check_router	6
				check_router	7
				check_router	8
				check_router	9
				check_router	10
to_psyq
				plb
				ply
				plx
				pla
			if USING_PSYQ then
				jml $7de00b		; the Psy-Q brk handler
			else
				sec
				xce
				jmp ($fffc)		; do a phoney reset
			endif


***********************************************
*
* WEnableSRAM
*
* Write enable SRAM. You need to call this before you
* start modifying the game's vectors.
*
***********************************************

WEnableSRAM
				php
				a8
				lda	#$BF			; as requested by Josh
				sta >mdFred_Soft+mdSRAMProtect		; allow writing to all of SRAM	
				plp
				rts

***********************************************
*
* WDisableSRAM
*
* Write protect SRAM. Call this after you
* finish modifying the game's vectors.
*
***********************************************

WDisableSRAM
				php
				a8
				lda	#$EE			; as requested by Josh
				sta >mdFred_Soft+mdSRAMProtect		; allow writing to all of SRAM	
				plp
				rts

***********************************************
*
* This code sets up some key Fred registers and modem RAM.
*
* WARNING! IT CLEARS THE SNES RAM, SO YOU CAN'T SAVE _ANYTHING_ ON
*	THE STACK WHEN YOU CALL THIS! IT BARELY KEEPS THE RETURN ADDRESS
*	AROUND!
*
* You should call this early on in your game patch, before setting
*	up any patches.
*
* The following is done:
*	¥ the magic address is set to the magic rtl (near the top of this file)
*	¥ interrupts are turned off
*	¥ RAM is moved to $e00000, ROM is moved to $d00000 (when in soft
*		here mode). ROM bound is also set to $dfc000, so the kill
*		and control registers can be accessed while keeping the ROM
*		OS routines happy in most cases.
*	¥ "extended exception space" is turned on (that is, RAM at $ff00-$ffe0
*		becomes visible when in soft here mode). This area is used to route
*		SNES vectors to the modem RAM.
*	¥ modem registers are set to appear in their normal locations
*		as well as at the top of ROM. They appear at the top of ROM
*		because of a bug in the Fred chip that allows access to the kill
*		register in soft here mode only at the "top of ROM".
*	¥ go into "soft here" mode (in such a way as to allow code to continue).
*	¥ set up the patch vector table base (for use with InstallPatch, and the
*		patching macros).
*	¥ set exception vectors to point to jump table at $ff00
*	¥ set up jump table to jump to exception handlers in modem RAM
*	¥ clear SNES RAM to a fixed and known value
*
***********************************************

SetUp_Memory
				phk
				plb

; set up the magic address

				longa	on
				lda	#(((magic_address>>1)>>8)|$6000)&$ffff
				sta >mdFred_Hard+mdMagicAddr+2
				lda #(magic_address>>1)&$ffff
				sta >mdFred_Hard+mdMagicAddr

;

				lda #0
				tcd
				a8		
				sta >NMITimEn					; turn off interrupts, since the OS leaves them on
				lda	#$BF						; (as requested by Josh)
				sta >mdFred_Hard+mdSRAMProtect	; allow writing parts of SRAM I need

; Move XBand RAM and ROM to desired location when in "soft here" mode.

				a16
				lda #((ram_soft_start)/$40)&$ffff
				sta >mdFred_Hard+mdRAMBase
				lda #((ram_soft_end)/$40)&$ffff
				sta >mdFred_Hard+mdRAMBound
				a8
				lda #<(rom_soft_start)/$4000
				sta >mdFred_Hard+mdROMBase
				lda #<(rom_soft_end)/$4000
				sta >mdFred_Hard+mdROMBound
				lda #((rom_soft_start/$100000)&$0c)+((ram_soft_start/$400000)&3)
				sta >mdFred_Hard+mdSNESControl

; Set up the control register (it sure does a lot!).

			if	(Use_Fixed_Internals) then
				lda	#$33
			else
				lda	#$2b
			endif

; ram at $ff00-$ffff, Fred regs visible at one of top of rom or normal location, rom visible
; second 32k bank of ram visible, enabled internals

				sta >mdControl_Hard	; control register in "hard here" mode
				a16

; Set up Hit_Enables and the shadow register

				lda	#$8000					; both ranges might be on!
				sta	|shadow_mdHitEnables
				sta	>mdFred_Hard+mdHitEnables

; Set up the range if we're using "medium" mode

			if ram_moves_soft_med then

@range_mask		equ	$00
@range_start	equ	(ram_soft_start)/2
@range_end		equ	(ram_soft_start+$2000)/2
@range_dest		equ	($0000)/32
				lda	#@range_start&$ffff
				sta	>mdRangeStart0+mdFred_Hard
				lda	#@range_end&$ffff
				sta	>mdRangeEnd0+mdFred_Hard
				lda	#@range_dest
				sta	>mdRangeDestination0+mdFred_Hard
				a8
				lda	#^@range_start
				sta	>mdFred_Hard+mdRangeStart0+4
				lda	#^@range_end
				sta	>mdFred_Hard+mdRangeEnd0+4
				lda	#@range_mask
				sta	>mdFred_Hard+mdRangeMask0
				a16

			endif

; Set up the vector table base (patch substitution words).

				lda	#(Patch_Substitution>>5)&$ffff
				sta	>mdFred_Hard+mdVectorTableBase

; Force into soft here mode.

@place_at		equ	$7f0000

				ldx	#(@go_into_soft_end-@go_into_soft_start+1)&$fffe	; even number
@copy_soft_here_code
				lda	|@go_into_soft_start,x
				sta	>@place_at,x
				dex
				dex
				bpl	@copy_soft_here_code

				a8
				lda	#%1000
				sta	>mdKill_Hard
				jml	@place_at

; execute this code at $7f0000

@go_into_soft_start
				lda	>magic_address
				lda	>$ffe0
				jml	@reenter
@go_into_soft_end

; and come back here

@reenter
				phk
				plb		; since bank may have changed!!


; Copy game $ff00-$ffff to our RAM space

BRK_JML			equ	$fffc

				ai16

				ldx	#$fe
@copy_loop
				lda	>$80ff00,x	; sneaky, eh?
				sta	|$ff00,x
				dex
				dex
				bpl	@copy_loop

				a8
				lda	#$5c
				sta	>BRK_JML	;$fffc
				lda	#^BRK_Code
				sta	>BRK_JML+3
				a16
				lda	#BRK_Code&$ffff
				sta	>BRK_JML+1

				lda	#BRK_JML
				sta	>BRK_Vector

			if USING_PSYQ then
				lda	#$0e5c
				sta	|$fff4
				lda	#$7de0
				sta	|$fff6
				lda	#$fff4
				sta	|$ffe4		; Psy-Q insists upon owning the COP vector
			endif

				jsr	WDisableSRAM

; Tell the OS where Fred has gone to.

				lda	#mdFred_Soft&$ffff
				sta	|gRegisterBase
				lda	#^mdFred_Soft
				sta	|gRegisterBase+2

; Clear the SNES RAM to aid in making things deterministic.

				ai16
				ply
				ldx	#0
				lda	#'RK'
@Clear
				sta	>$7e0000,x
				sta >$7f0000,x
				inx
				inx
				bne	@Clear

; Clear out the game reserved area.

;				ldx	#GR_End-GR_LocalGameError-2
@clear_words
;				stz	|GR_LocalGameError,x
;				dex
;				dex
;				bpl	@clear_words

				phy			; so we can rts!

				rts

***********************************************
*
* Hard_Here_NMI
*
* NMI routine used in hard here routine.
*
* All it does in increment the tick count.
*
***********************************************

Hard_Here_NMI
				phb
				phk
				plb

				ai16
				pha
				inc	|gTicks
				bne	@no_inc
				inc	|gTicks+2
@no_inc
				a8
				lda	>RdNMI
				ai16
				pla
				plb
				rti

***********************************************
*
* Hard_Here_IRQ
*
* IRQ routine used in hard here routine.
*
* It doesn't do anything.
*
***********************************************

Hard_Here_IRQ
				a8
				pha
				lda	>TimeUp
				pla
				rti
				longa on

***********************************************
*
* SaveStack
*
* This routine saves the stack and stack pointer
* in preparation for calling the OS.
*
***********************************************

SaveStack
				tsc
				sec
				sbc	#$200
				tax
				ldy	#0
@saveStack
				lda	>0,x
				sta	|StackSave,y
				inx
				inx
				iny
				iny
				cpy	#$200
				bne	@saveStack
				rts

***********************************************
*
* RestoreStack
*
* This routine restores the stack and stack pointer
* after having called the OS.
*
***********************************************

RestoreStack
				tsc
				sec
				sbc	#$200
				tax
				ldy	#0
@saveStack
				lda	|StackSave,y
				sta	>0,x
				inx
				inx
				iny
				iny
				cpy	#$200
				bne	@saveStack
				rts

***********************************************
*
* Soft_To_Hard
*
* This routine does all the necessary slop for
* going into "hard here" mode, including cleverly
* rerouting the interrupt vectors.
*
***********************************************

Soft_To_Hard
				ldx	#6
@loop
				lda	>$1200,x
				sta	|Save1200,x
				dex
				dex
				bpl	@loop

				jsr	SaveStack

				lda	#Hard_Here_NMI&$ffff
				sta	>$1201
				lda	#Hard_Here_IRQ&$ffff
				sta	>$1205
				a8
				lda	#$5c			; "jml"
				sta	>$1200
				sta	>$1204
				lda	#^Hard_Here_NMI
				sta	>$1203
				lda	#^Hard_Here_IRQ
				sta	>$1207
			if (Use_Fixed_Internals) then		; WARNING! REGISTERS ARE MAPPED IN NOW!!!
				lda	#$3b
				sta	>mdControl_Soft
			endif
				lda	#%001
				sta	>mdKill_Soft
				lda	>magic_address
			if (Use_Fixed_Internals) then
				lda	#$33
				sta	>mdControl_Hard
			endif
				a16

			if (mdFred_Soft<>mdFred_Hard) then
				lda	#mdFred_Hard&$ffff
				sta	|gRegisterBase
				lda	#^mdFred_Hard
				sta	|gRegisterBase+2
			endif

				pea	$e0e0
				plb
				plb
				rts

***********************************************
*
* Soft_to_Medium
*
* This routine does all the necessary slop for
* going into "medium here" mode, including cleverly
* rerouting the interrupt vectors.
*
* "Medium here" mode simply maps in the ROM and RAM
* at $d0-$df and $e0; enough to call many OS functions,
* except those that may not return to the game.
*
***********************************************

	if Use_Medium then
Soft_to_Medium
				jsr	SaveStack

			if soft_med_different then		; does soft here place RAM and ROM where you'd expect?

			if (ram_moves_soft_med) then
				lda	#$8800<<0				; turn on ONLY range #0, no patches
			else
				lda	#$8000					; leave only vectors on
			endif
				sta	>mdFred_Soft+mdHitEnables

				a8
			if	(Use_Fixed_Internals=0) then
				lda #$33
				sta >mdControl_Soft			; turn on registers in normal position
			endif	; Use_Fixed_Internals
				lda #<(rom_med_start)/$4000
				sta >mdFred_Hard+mdROMBase
				lda #<(rom_med_end)/$4000
				sta >mdFred_Hard+mdROMBound
				lda #(((rom_med_start)/$100000)&$0c)+(((ram_med_start)/$400000)&3)
				sta >mdFred_Hard+mdSNESControl
				a16
				lda #((ram_med_start)/$40)&$ffff
				sta >mdFred_Hard+mdRAMBase
				lda #((ram_med_end)/$40)&$ffff
				sta >mdFred_Hard+mdRAMBound

				pea	$e0e0
				plb
				plb

			if	(mdFred_Med<>mdFred_Soft) then
				lda	#mdFred_Med&$ffff
				sta	|gRegisterBase
				lda	#^mdFred_Med
				sta	|gRegisterBase+2
			endif	; mdFred_Med<>mdFred_Soft
			endif	; soft_med_different
				rts

***********************************************
*
* Medium_to_Soft
*
* This routine does all the necessary slop for
* getting out of "medium here" mode, back into
* soft here mode.
*
***********************************************

Medium_to_Soft
		if soft_med_different then

				lda #((ram_soft_start)/$40)&$ffff
				sta >mdFred_Hard+mdRAMBase
				lda #((ram_soft_end)/$40)&$ffff
				sta >mdFred_Hard+mdRAMBound
				a8
				lda #<((rom_soft_start)/$4000)&$ffff
				sta >mdFred_Hard+mdROMBase
				lda #<((rom_soft_end)/$4000)&$ffff
				sta >mdFred_Hard+mdROMBound
				lda #(((rom_soft_start)/$100000)&$0c)+(((ram_soft_start)/$400000)&3)
				sta >mdFred_Hard+mdSNESControl
			if	(Use_Fixed_Internals=0) then
				lda	#$2b
				sta >mdControl_Med	; turn on registers in normal position
			endif			; Use_Fixed_Internals
				a16

				phk
				plb

				lda	|shadow_mdHitEnables
				sta	>mdFred_Soft+mdHitEnables	; turn off range #0, turn on patches again

			if	(mdFred_Med<>mdFred_Soft) then
				lda	#mdFred_Soft&$ffff
				sta	|gRegisterBase
				lda	#^mdFred_Soft
				sta	|gRegisterBase+2
			endif		; Use_Fixed_Internals
		endif			; soft_med_different
				jsr	RestoreStack

				rts
		endif		; Use_Medium

***********************************************
*
* Hard_to_Soft
*
* This function goes into soft here mode from hard here.
* It restores the values in $1200-$1207.
*
***********************************************

Hard_to_Soft
				jsr	RestoreStack

				a8
				lda	#%000
				sta	>mdKill_Hard
				lda	>magic_address
				a16

				phk
				plb

			if	(mdFred_Med<>mdFred_Soft) then
				lda	#mdFred_Soft&$ffff
				sta	|gRegisterBase
				lda	#^mdFred_Soft
				sta	|gRegisterBase+2
			endif

				ldx	#6
@loop
				lda	|Save1200,x
				sta	>$1200,x
				dex
				dex
				bpl	@loop
				rts

***********************************************
*
* CalcPlayTime
*
* This function should be called frequently. It updates
* the amount of time played.
*
***********************************************

CalcPlayTime
				sec
				lda	|gTicks
				sbc	|startGameTicks
				sta	|GR_playTime
				lda	|gTicks+2
				sbc	|startGameTicks+2
				sta	|GR_playTime+2
				rts

***********************************************
*
* RingDetect
*
* This function should be called frequently when
* in a single player game.
*
***********************************************

RingDetect
				phb
				phk
				plb
				php
				ai16
				lda	|Registered
				beq	@no_ring
				lda	|Timeout+2
				cmp	|gTicks+2
				blt	@timeout
				bne	@check_ring
@check_low_word
				lda	|Timeout
				cmp	|gTicks
				bge	@check_ring
@timeout
				stz	|Registered
			if Use_Medium then
				jsr	Soft_to_Medium
			else
				jsr	Soft_To_Hard
			endif
				pea	kNetRegisterTimeout&$ffff
				ldx	#0		; kOSHandleGameError
				jsl	OSDispatcher
			if Use_Medium then
				jsr	Medium_to_Soft
			else
				jsr	Hard_To_Soft
			endif
				bra	@no_ring
@check_ring
				a8
				lda	>mdFred_Soft+mdRingCheck	;$fbc19f
				and	#8
				a16
				beq	@no_ring
				jsr	Soft_to_Hard
				pea	$fd3c	; kRingDetected
				ldx	#0		; kOSHandleGameError
				jsl	OSDispatcher	; and never return!
@no_ring

; do stuff with the LEDs here!!

				ldx	#mdBottom_LED_Mask
				lda	|Registered
				beq	@not_both
				lda	|gTicks
				and	#%10000
				bne	@not_both
				ldx	#mdBottom_LED_Mask|mdMiddle_LED_Mask
@not_both
				txa
				and	#$ff
				ora	#$bf00
				sta	>mdFred_Soft+mdLED_Data

				plp
				plb
				rts

***********************************************
*
* End_If_1P_Done
*
* This function should be called frequently when
* it's safe to quit a single player game.
*
***********************************************

End_If_1P_Done
				phb
				phk
				plb
				pha
				lda	|EverFinish1P
				beq	@get_out			; never exit
				lda	|Registered
				bne	@get_out
				ai16
				jsr	Soft_to_Hard
				ldx	#$11ff
				txs
				pea	kSinglePlayerGameOver&$ffff
				ldx	#kOSHandleGameError
				jsl	OSDispatcher
@get_out
				pla
				plb
				rts

***********************************************
*
* Game_Over
*
* This function is called to return to the OS
* after the game ends naturally.
*
***********************************************

Game_Over
				phk
				plb
				jsr	InitNewGame			; in case they want to play again
				jsr	Soft_to_Hard
				a8
				lda	#0
				sta	>$4200				; turn off interrupts
				a16
				ldx	#$11ff
				txs
				ldx	#kOSGameOver
				jsl	OSDispatcher		; -- and never return

***********************************************
*
* Timeout_Exit
*
* This function is called to return to the OS after
* the game ends with an error.
*
***********************************************

Timeout_Exit
				phk
				plb
				jsr	SaveGameState		; in case we try to reenter
				jsr	Soft_to_hard
				a8
				lda	#0
				sta	>$4200				; turn off interrupts
				lda	#$80
				sta	>$2100				; turn off screen before we bail to hide screen crap
				a16
				ldx	#0
@delay
				dex
				nop
				nop
				bne	@delay				; wait a while to allow modem bytes to flush
				ldx	#$11ff				; or the stack sniffer might die!
				txs
				pea	kGameTimeout&$ffff
				ldx	#kOSHandleGameError
				jsl	OSDispatcher

***********************************************
*
* Check_Line
*
* This function is called when a line error occurs.
* It checks the line and looks for a call-waiting error.
*
***********************************************

Check_Line
				phb
				phk
				plb
			if Use_Medium then
				jsr	Soft_To_Medium
			else
				jsr	Soft_to_Hard
			endif
				ldx	#kOSCheckLine
				jsl	OSDispatcher
				cmp	#kCallWaitingErr
				beq	@error
				cmp	#kRemoteCallWaitingErr
				bne	@no_error
@error
				jsr	SaveGameState			; since we're outta here!
			if Use_Medium then
				sta	|GR_LocalGameError
				jsr Medium_To_Soft
				jsr	Soft_To_Hard
				lda	|GR_LocalGameError
			endif
				ldx	#$11ff					; or the stack sniffer might die!
				txs
				pha							; push old stack value
				ldx	#kOSHandleGameError
				jsl	OSDispatcher
@no_error
			if Use_Medium then
				jsr	Medium_To_Soft
			else
				jsr	Hard_To_Soft
			endif
@in_soft
				plb
				rts


				end
