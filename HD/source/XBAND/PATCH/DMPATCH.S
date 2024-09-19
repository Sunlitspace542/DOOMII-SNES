
***********************************************
*
* SNES DOOM game patch
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
***********************************************

			section code

			include 'Macros.i'
			include 'Flags.i'
			include 'Patcher.i'
			include 'MemCfg.i'

***********************************************
*
* SNES defines
*
***********************************************

RdNMI           equ     $4210
HVBJoy          equ     $4212
JOY1L           equ     $4218

***********************************************
*
* XBAND OS and hardware defines
*
***********************************************

gTicks                          equ     ram_soft_start+$20
gSessionIsMaster        equ     ram_soft_start+$2c44

				if      (Use_Fixed_Internals) then
mdFred_Soft                     equ     $fbc000
				else
mdFred_Soft                     equ     rom_soft_end+$200
				endif

mdControl_Soft                  equ     $4f03

mdLED_Data                      equ     $b4*2+1

mdTop_LED_Mask          equ     $02
mdMiddle_LED_Mask       equ     $08
mdBottom_LED_Mask       equ     $20

***********************************************
*
* Modules that need to be referenced outside this file.
*
***********************************************

			xdef    InitNewGame
			xdef    InitGamePatch
			xdef    StartModemGame
			xdef    SaveGameState
			xdef    Start1PGame

***********************************************
*
* Data for Patch.s
*
***********************************************

			section Variables

LED_Counter             ds      2

***********************************************
*
* Externals.
*
***********************************************

; Code from GamePatch.s

			xref    WEnableSRAM
			xref    WDisableSRAM
			xref    RingDetect
			xref    End_If_1P_Done
			xref    Game_Over
			xref    Timeout_Exit
			xref    CalcPlayTime

; Data from GamePatch.s

			xref    brk_router
			xref    shadow_mdHitEnables
			xref    Patch_Substitution

; Code from XBComm.s

			xref    XBInit
			xref    XBSynchronize
			xref    XBExchangeData
			xref    XBPollModem

; Data from XBComm.s

			xref    XBLocalPacketIn
			xref    XBLocalPacketOut
			xref    XBRemotePacketOut                               

***********************************************
*
* Equates.
*
***********************************************

MasterIsPlayer1 equ     1               ; if 1, master is on the left

***********************************************
*
* Data uploaded to the server. This is zeroed out automagically.
*
***********************************************

					rsset   $2c88

GR_size                                 rl      1
GR_gameID                               rl      1
GR_connectPhase                 rb      1
GR_errorWhere                   rb      1
GR_gameError                    rw      1
GR_localPlayer1Result   rl      1
GR_localPlayer2Result   rl      1
GR_remotePlayer1Result  rl      1
GR_remotePlayer2Result  rl      1
GR_playTime                             rl      1
GR_dblDDataPtr                  rl      1
GR_dblDDataSize                 rl      1

GR_LocalGameError               rw      1
GR_ErrorRecovers                rw      1
GR_ChecksumErrors               rb      1
GR_Timeouts                             rb      1
GR_FrameErrors                  rb      1
GR_OverrunErrors                rb      1       ; this stuff is recorded in the server report

;; now my stuff

GR_Unused1                              rw      1
GR_Unused2                              rw      1
GR_Unused3                              rw      1       ; number of forced delays due to kNoData error (in ticks)
GR_Unused4                              rw      1
GR_Unused5                              rw      1
GR_Unused6                              rw      1
GR_Unused7                              rw      1
GR_Unused8                              rw      1
GR_Unused9                              rw      1
GR_Unused10                             rw      1
GR_Unused11                             rw      1
GR_Unused12                             rw      1
GR_Unused13                             rw      1
GR_Unused14                             rw      1
GR_Unused15                             rw      1
GR_Unused16                             rw      1       ; that's just right!

; more predefined stuff

GR_numLocalPlayers              rw      1
GR_pad                                  rw      1
GR_End                                  rb      0

	section code

***********************************************
*
* InitGamePatch
*
* Initialize game patch.
*
* Set up whatever variables you want here.
*
* Note: You now must explicitly zero out any GR_ variables
* yourself (if you want them zeroed).
*
***********************************************

InitGamePatch
				rts


***********************************************
*
* InitNewGame
*
* Initialize new game. Do whatever needs to be done to indicate
* that next time through the patch we should start from the beginning,
* afresh, and not resume a partial game.
*
***********************************************

InitNewGame
				rts


***********************************************
*
* SaveGameState
*
* Save the current game state, as we're going to exit
* and return to the OS, and may want to reenter the game more
* or less where we were.
*
***********************************************

SaveGameState
				rts


***********************************************
*
* Start1PGame
*
* Play a single-player game.
*
***********************************************

Start1PGame

				jsr     WEnableSRAM

; Take over the NMI vector.

NMI_JML_Vector  equ     $ffe0

				a8
				lda     #$5c
				sta     >NMI_JML_Vector
				lda     #^NMI_Handler_1P
				sta     >NMI_JML_Vector+3
				a16
				lda     #NMI_Handler_1P&$ffff
				sta     >NMI_JML_Vector+1

;                               lda     #NMI_JML_Vector
;                               sta     >$ffea                          ; NMI vector

				stz     |LED_Counter

				jsr     WDisableSRAM

; jump to the reset vector



***********************************************
*
* StartModemGame
*
* Play a modem linked game.
*
***********************************************

StartModemGame
				jsr     WEnableSRAM

; Give Randy his vectors back

				ldx     #$fe
@copy
				lda     >$407f00,x
				sta     >$ff00,x
				dex
				dex
				bpl     @copy

				jsr     WDisableSRAM
				
				stz     |LED_Counter

				jsr     XBInit
				jml     >$8100

; jump to reset!


***********************************************
*
* NMI routine when a modem game is going.
*
***********************************************

NMI_Handler_Modem
				ai16
				pha
				phx
				phy
				phb

; do some fun LED stuff

				lda     |gTicks
				and     #%1111
				bne     no_LED_Stuff
				dec     |LED_Counter
				bpl     @not_minus
				lda     #LED_Table_End-LED_Table_Start-1
				sta     |LED_Counter
@not_minus
				ldx     |LED_Counter
				lda     |LED_Table_Start,x
				and     #$ff
				ora     #$bf00
				sta     >mdFred_Soft+mdLED_Data
no_LED_Stuff

@skip_game_NMI
				inc     |gTicks
				bne     @no_inc
				inc     |gTicks+2
@no_inc

; calculate play time please!

				jsr     CalcPlayTime

				jsr     XBPollModem
				lda     >RdNMI  ; just in case
finish
				plb
				ply
				plx
				pla
				rti

LED_Table_Start db      mdBottom_LED_Mask,mdMiddle_LED_Mask
				db      mdTop_LED_Mask,mdMiddle_LED_Mask
				db      mdBottom_LED_Mask,mdMiddle_LED_Mask
				db      mdTop_LED_Mask,mdMiddle_LED_Mask
				db      mdBottom_LED_Mask,mdMiddle_LED_Mask
				db      mdTop_LED_Mask,mdMiddle_LED_Mask
				db      mdBottom_LED_Mask,mdTop_LED_Mask
				db      mdBottom_LED_Mask,mdTop_LED_Mask
LED_Table_End

***********************************************
*
* NMI routine when a single player game is going.
*
***********************************************

NMI_Handler_1P
				ai16
				pha
				phx
				phy
				phb
				cli                     ; to allow horizontal interrupts to occur
				php
				lda     #>@do_my_NMI_stuff
				pha
				lda     #<@do_my_NMI_stuff<<8
				pha
				;jml     NMI_Routine

@do_my_NMI_stuff
				plp                     ; since P was screwed up in the RTI
				phk
				plb

@skip_game_NMI
				inc     |gTicks
				bne     @no_inc
				inc     |gTicks+2
@no_inc

				jsr     RingDetect
				lda     >RdNMI  ; just in case
@finish
				plb
				ply
				plx
				pla
				rti


				end
