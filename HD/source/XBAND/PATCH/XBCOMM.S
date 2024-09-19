***********************************************
*
* Pocky Board Communications Software
* to test for XBAND compliance
*
* This file assembles using the Psy-Q assembler.
* We have attempted to comment all lines with non-standard
* opcodes and pseudo-ops.
*
* Version 0.96[prerelease]
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
* Version history:
*
* Version 0.9B: initial release.
*
* Version 0.91: This section added. XBPollModem dead code stripped,
*   modified noise generation mechanism. _XBSwapData2 removed. Flag
*   "Final_Compile" changed to "Final_Build". "public on" removed.
*
* Version 0.92: Bug in _XBEstablishSync (called by XBSynchronize) fixed:
*   it would trash low-memory.
*
* Version 0.95: Modem_Comm flag added. XBModemErr set on init, so an
*   error will automatically occur on the first call to XBSwapData (and no
*   special code is required for the initial XBSynchronize). Game_Patch flag added.
*   Flags moved to "XBComm.i". Dispatcher code moved to after code it calls.
*   _XBPockyByte changed to work significantly faster, which should result in
*   an overall increase in speed. Error counting added for game patches.
*
* Version 0.96: Timeout is now 15 seconds instead of 30 seconds. Experimenting with
*   32 bit packets. Fixed bug in counting frame errors.
*
***********************************************

		include "XBComm.i"  ; for flags

Init_Port_A	equ (Debug_Port_A|Comm_Debug)
Init_Port_B	equ (Modem_Comm=0)

endl		equ $d		; end of line, terminal

Packet_Size	equ 3		; 3 or 4 only please. 4 doesn't work very well yet.

***********************************************
*
* UART 16550 Registers for ports A and B
*
***********************************************

rxd_A	    equ	    $21C0
txd_A	    equ	    $21C0
ier_A	    equ	    $21C1	; int enable reg
dll_A	    equ	    $21C0	; divisor latch lo
dlh_A	    equ	    $21C1	; divisor latch hi
iir_A	    equ	    $21C2	; int id reg
fcr_A	    equ	    $21C2	; fifo ctl reg
lcr_A	    equ	    $21C3	; line ctl reg
mcr_A	    equ	    $21C4	; modem ctl reg
lsr_A	    equ	    $21C5	; line status reg
msr_A	    equ	    $21C6	; modem status reg
scr_A	    equ	    $21C7	; scratch pad reg

rxd_B	    equ	    $21D0
txd_B	    equ	    $21D0
ier_B	    equ	    $21D1	; int enable reg
dll_B	    equ	    $21D0	; divisor latch lo
dlh_B	    equ	    $21D1	; divisor latch hi
iir_B	    equ	    $21D2	; int id reg
fcr_B	    equ	    $21D2	; fifo ctl reg
lcr_B	    equ	    $21D3	; line ctl reg
mcr_B	    equ	    $21D4	; modem ctl reg
lsr_B	    equ	    $21D5	; line status reg
msr_B	    equ	    $21D6	; modem status reg
scr_B	    equ	    $21D7	; scratch pad reg

port_A_baud equ 57600
port_B_baud equ 4800

port_A_baud_val equ 115200/port_A_baud
port_B_baud_val equ 115200/port_B_baud

**************************************************
*
* Error code equates.
*
* These values are carefully chosen to be consistent
* with XBAND OS error codes.
*
* That explains why they seem so oddly chosen.
*
**************************************************
; public on
kOverrunError	    equ	    -425	; $fe57
kTimeout	    equ	    -426	; $fe56
kGibbledPacket	    equ	    -601	; $fda7
kNoData		    equ	    -602	; $fda6
kOutOfSync	    equ	    -901	; $fc76

**************************************************
*
* Other equates.
*
**************************************************

XBBufferSize	    equ	    32			    ; must be a power of two!
XBBufferSizeMask    equ	    (XBBufferSize-1)*4	    ; in order for this to work as an AND mask
XBModemBufferSize   equ	    XBBufferSize*4	    ; ditto
XBModemBufferMask   equ	    XBModemBufferSize-1

**************************************************
*
* Hardware modem register offsets (from the start of the register area).
*
**************************************************

		if (Modem_Comm) then
		    include 'MemCfg.i'		; for ram_soft_start & Use_Fixed_Internals
		if  (Use_Fixed_Internals) then
mdFred_Soft	    equ $fbc000
		else
mdFred_Soft	    equ rom_soft_end+$200
		endif
		endif

mdTransmit_Buffer   equ	    $90*2
mdReceive_Buffer    equ	    $94*2
mdRead_Status_2	    equ	    $98*2
mdRead_Status_1	    equ	    $a0*2
mdRetrainReg	    equ	    $c8*2+1

**************************************************
*
* Private and I/O variables for the XBAND
* communications routines.
*
**************************************************

		    section XBStatics	; "section" is used to place this chunk in a different ORG place

XBLocalQueue	    ds	4*XBBufferSize	; local data is kept here for later use, and for error recovery
XBLocalHead	    ds	2		; data goes in the head (postdecrement) : always a multiple of 4
XBLocalTail	    ds	2		; and comes out the tail (postdecrement) : always a multiple of 4

XBModemLatency	    ds	2		; DO NOT ASSUME THIS WILL BE ANY PARTICULAR VALUE.

XBGameLoopCounter   ds	2		; counts how many times XBSwapData is called

XBPacketOut	    ds	4		; used privately by XBGetPacket (to protect the value of XBRemotePacketOut)

*
* Serial hardware
*

XBNoiseLevel	    ds	2	; compared to random number on modem read to generate bogus data
XBRndSeed	    ds	2	; random number seed used for noise generation

XBModemErr	    ds	2	; set when there's an overrun or other modem error
XBModemBuffer	    ds	XBModemBufferSize	; modem buffer
XBModemHead	    ds	2	; data goes in here. Nothing can touch this besides XBReadModem
XBModemTail	    ds	2	; data comes out here.
XBModemBusy	    ds	2	; this indicates modem hardware is busy
XBSendPacket	    ds	4	; the packet to send

		    if Final_Build then ; compiled iff Final_Build is nonzero
		    else

XBFinalPosted	    ds	48	; copy of the final data posted. This will normally be copied to XBAND RAM.

		    endif

*
* I/O Parameters
*
* These are the only variables that need to be seen outside of this module.
*


		    xdef    XBTicks,XBPostedData,XBIsNetworkGame	    ; "xdef" means export to the linker
		    xdef    XBLocalPacketIn,XBLocalPacketOut,XBRemotePacketOut

		    xdef    XBLocalScore1,XBLocalScore2,XBRemoteScore1,XBRemoteScore2,XBMiscPostedData

XBTicks		    ds	4   ; READ ONLY

XBPostedData	    ds	48  ; read and write

XBLocalScore1	    equ XBPostedData
XBLocalScore2	    equ XBPostedData+4
XBRemoteScore1	    equ XBPostedData+8
XBRemoteScore2	    equ XBPostedData+12

XBMiscPostedData    equ XBPostedData+16

XBIsNetworkGame	    ds	2	; 1 if master, -1 if slave, 0 if XBAND not installed

XBLocalPacketIn	    ds	4	; the externally visible packet to send

XBLocalPacketOut    ds	4	; the current local packet out
XBRemotePacketOut   ds	4	; the current remote packet out


		if (Modem_Comm&Game_Patch) then
		    xref Check_Line
		endif


		if (Game_Patch) then
**************************************************
*
* Game patch
*
**************************************************
; public off
		    rsset   $2c88

GR_size			rl  1
GR_gameID		rl  1
GR_connectPhase		rb  1
GR_errorWhere		rb  1
GR_gameError		rw  1
GR_localPlayer1Result	rl  1
GR_localPlayer2Result	rl  1
GR_remotePlayer1Result	rl  1
GR_remotePlayer2Result	rl  1
GR_playTime		rl  1
GR_dblDDataPtr		rl  1
GR_dblDDataSize		rl  1

GR_LocalGameError	rw  1
GR_ErrorRecovers	rw  1
GR_ChecksumErrors	rb  1
GR_Timeouts		rb  1
GR_FrameErrors		rb  1
GR_OverrunErrors	rb  1	; this stuff is recorded in the server report

		endif	; Game_Patch

; public on

**************************************************
*
* Beginning of code.
*
**************************************************

		section XBComm	; this code is not necessarily physically adjacent to the above code

	    if Final_Build then		    ; for the final cartridge image, you don't want any Pocky code

_XBInit:
		stz |XBIsNetworkGame		; no XBAND installed
_XBSetNoise:
_XBSynchronize:
_XBExchangeData:
_XBSwapData:
_XBPostData:
_XBSafeQuit:
_XBReturn:
		lda #0	; no error
		rts
_XBPollModem:
		inc |XBTicks
		bne @noinc
		inc |XBTicks+2
@noinc:
		rts

	    else

**************************************************
*
* _XBInit
*
* This routine should be the first XBAND routine called.
* It initializes all important variables, and checks to
* make sure the Pocky is installed. If the Pocky is installed,
* that is considered equivalent to an XBAND game request, and
* it checks the jumpers to see if this machine is the slave
* or the master.
*
**************************************************

_XBInit:
	    if (Init_Port_A|Init_Port_B) then
		sep #$20
		longa off
		phb
		pea $0000	; now we're setting Pocky-board hardware registers
		plb
		plb
	    endif

	    if (Init_Port_B) then
		lda #$aa
		sta |scr_B	; if the Pocky is installed, this value will stick
		cmp |scr_B
		bne @no_pocky
		lda #$ff
		sta |scr_B	; negative value, indicating "first byte"
		cmp |scr_B
		bne @no_pocky

		lda #$00
		sta |ier_B	    ; interrupts off
		lda #%11000111
		sta |fcr_B	    ; turn on 16 byte FIFO
		lda #%10000000
		sta |lcr_B	    ; go into "set up baud rate" mode
		lda #<port_B_baud_val
		sta |dll_B	    ; set baud rate
		lda #>port_B_baud_val
		sta |dlh_B
		lda #%00000011
		sta |lcr_B	    ; set to N81 (turn off baud set mode)

		lda |msr_B	    ; check jumper S2
		bpl @positive
		ldx #-1
		bra @is_slave
@positive:
		ldx #1
@is_slave:
	    endif

	    if	(Init_Port_A) then  ; conditional compilation: port A is not initialized unless it's used
		lda #$aa
		sta |scr_B
		cmp |scr_B
		bne @no_pocky
		lda #-1
		sta |scr_B
		cmp |scr_B
		bne @no_pocky

		lda #$00
		sta |ier_A	    ; interrupts off
		lda #%11000111
		sta |fcr_A	    ; turn on 16 byte FIFO
		lda #%10000000
		sta |lcr_A	    ; go into "set up baud rate" mode
		lda #<port_A_baud_val	
		sta |dll_A	    ; set baud rate
		lda #>port_A_baud_val
		sta |dlh_A
		lda #%00000011
		sta |lcr_A	    ; set to N81 (turn off baud set mode)

		lda |$2137	    ; latch horizontal and vertical position for "random number"
	    endif

	    if (Init_Port_A|Init_Port_B) then
		plb
		rep #$30
		longa on
		bra @got_pocky
@no_pocky:			    ; NO POCKY INSTALLED!!!
		rep #$30
		longa on
		plb
		lda #1
		sta |XBModemBusy    ; so XBPollModem is skipped
		lda #0
		sta |XBIsNetworkGame
		rts
@got_pocky
	    endif

	    if	(Init_Port_A) then  ; conditional compilation: port A is not initialized unless it's used
		longa on
		rep #$20

;; Pull out a pseudo-random number. Never use a hardware random
;; number yourself. We can do it because it won't affect game state.

		lda >$213c	    ; HV trace position
		ora #1		    ; ensure it's non-zero
		sta |XBRndSeed

		stx |XBIsNetworkGame
	    endif

	    if (Debug_Port_A) then
		phb
		phk
		plb
		txy
		bmi @slave
		ldx #@mText&$ffff
		bra @doPrint
@mText:		db  endl,endl,'Master',endl,0
@sText:		db  endl,endl,'Slave',endl,0
@slave:
		ldx #@sText&$ffff
@doPrint:
		jsr _XBPrintText
		plb
	    endif

;; init packet stuff

		stz |XBLocalHead		
		stz |XBLocalTail
		lda #6			; hard coded latency value of six. The XBAND modem
		sta |XBModemLatency	; can calculate the true latency, when necessary.
		stz |XBGameLoopCounter

;; init modem hardware stuff

		stz |XBModemHead
		stz |XBModemTail
		stz |XBModemBusy

		lda #-1
		sta |XBModemErr	    ; so we get an error the first time

		stz |XBTicks
		stz |XBTicks+2

		stz |XBNoiseLevel   ; no noise is good noise (pun)

	    if (Game_Patch=0) then
		lda #0
		ldx #48-2
@clear_post:
		sta |XBPostedData,x
		sta |XBFinalPosted,x
		dex
		dex
		bpl @clear_post
	    endif

;; prefill fifo

		ldy |XBModemLatency
@fill_loop:

;; queue the packet for retries

		ldx |XBLocalHead
		lda |XBLocalPacketIn
		sta |XBLocalQueue,x
		txa
		clc
		adc #4
		and #XBBufferSizeMask
		sta |XBLocalHead
		dey
		bne @fill_loop
@exit:
		rts

	if (Init_Port_B) then
**************************************************
*
* _XBSetNoise
*
* This routine sets the noise level. Noise is generated
* during the read, in _XBPollModem.
*
**************************************************

_XBSetNoise:
		sta |XBNoiseLevel
		rts

**************************************************
*
* _XBRnd
*
* A simple, yet effective random number routine.
* With a non-zero seed, this routine hits every value
* between 1 and 65535 before repeating.
*
* WARNING! DO NOT USE THIS RANDOM NUMBER ROUTINE FOR YOUR
* GAME, AS IT IS CALLED IN INTERRUPT! Of course, you can use the algorithm,
* but use a different memory location for the seed.
*
**************************************************

_XBRnd:
		lda |XBRndSeed
		lsr a
		bcc @no_eor
		eor #$b400
@no_eor:
		sta |XBRndSeed
		rts
	endif	; Game_Patch

**************************************************
*
* _XBPollModem
*
* Pull the bytes out of the serial buffer and put them
* into a software buffer.
*
* Also, increment the tick count.
*
**************************************************

_XBPollModem:
		lda |XBModemBusy
		bne @modem_busy
		inc |XBModemBusy
		sep #$30
		longa	off
		longi	off
		ldx |XBModemHead
@do_read:
	    if (Modem_Comm) then
		lda >mdFred_Soft+mdRead_Status_2
		lsr a
		bcc @no_data
		and #%11
		beq @packet_ok
	    if (Game_Patch) then
		bit |XBModemErr
	    endif
		sta |XBModemErr
	    if (Game_Patch) then
		bne @already_error	; if XBModemErr err is set, the error's been counted
		inc |GR_FrameErrors
@already_error
	    endif

@packet_ok:
		lda >mdFred_Soft+mdReceive_Buffer
	    else
		lda >lsr_B	;get comm status byte
		lsr a		;check low bit
		bcc @no_data	;if set, got byte
		and #%111
		beq @packet_ok
		sta |XBModemErr ; UART detected an overrun, parity or frame error
@packet_ok:
		lda >scr_B
		php
		lda >rxd_B	;data ready, get it!
		plp
		bpl @mask_it
		pha
		and #$55
		cmp 1,s
		beq @same
		lda #-1
@same:
		sta >scr_B
		pla
		bra @do_read
@mask_it:
		pha
		and #$aa
		cmp 1,s
		beq @same2
		lda #0
		bra @same
@same2:
		pla
		ora >scr_B
		pha
		lda #-1
		sta >scr_B

;		rep #$30
;		jsr _XBRnd	; generate fake noise?
;		cmp >XBNoiseLevel
;		bge @good_data
;		jsr _XBRnd
;		sep #$30
;		eor 1,s
;		sta 1,s
@good_data:
;		sep #$30
		pla
	    endif
		sta |XBModemBuffer,x
		txa
		inc a
		and #XBModemBufferMask
		tax
		bra @do_read
@no_data:
		stx |XBModemHead
		rep #$30
		longa	on
		stz |XBModemBusy
@modem_busy:
		inc |XBTicks
		bne @noinc
		inc |XBTicks+2
@noinc:
		rts


	    if (Init_Port_B) then
**************************************************
*
* _XBPockyByte
*
* Write a 4 and 4 encoded byte to the Pocky board.
* This is required to get around hardware flaw of the Pocky boards.
* Pocky boards cannot reliably read bytes with many consecutive
* set bits. Thus we use a 4800 baud serial port, and send
* two bytes instead of one: the first contains the even bits,
* the second, the odd bits. Our read routine in _XBPollModem
* reconstructs the original byte.
*
**************************************************

_XBPockyByte:
		longa off	    ; you must call in 8-bit memory mode
		pha
@wait1:
;		lda >lsr_B
;		and #$20	    ;Xmit Holding Reg Empty when set
;		beq @wait1	    ;wait for bit to be set
;		lda 1,s		    ; instead of pla pha
		and #$55
		sta >txd_B	    ;write it
@wait2:
		lda 1,s
		and #$aa
		sta >txd_B	    ;write it
		pla
		longa on
		rts
	    endif    ; Modem_Comm

**************************************************
*
* _XBGetPacket16
*
* This routine pulls a packet out of the modem buffer and
* returns an error code.
*
* Inputs: X8 = # of ticks to allow kNoData errors
*
* Outputs: error code in A16, one of kNoData, kOverrunError, kGibbledPacket
*
* If there's no error, the 16-bit packet is returned in XBPacketOut.
*
**************************************************

_XBGetPacket16:
		sep #$30
		longa	off
		longi	off

		lda |XBModemErr
		bne @modemErr
@check_for_data:
		lda |XBModemHead
		sec
		sbc |XBModemTail
		cmp #Packet_Size	    ; # of bytes required
		bge @data_ready
		lda |XBTicks
@wait_a_tick:
		cmp |XBTicks
		beq @wait_a_tick
		dex
		bne @check_for_data
		bra @noData
@data_ready:
		ldx |XBModemTail
		lda |XBModemBuffer,x
		sta |XBPacketOut
		txa
		inc a
		and #XBModemBufferMask
		tax

		lda |XBModemBuffer,x
		sta |XBPacketOut+1
		txa
		inc a
		and #XBModemBufferMask
		tax

	    if (Packet_Size=4) then

		lda |XBModemBuffer,x
		sta |XBPacketOut+2
		txa
		inc a
		and #XBModemBufferMask
		tax

		lda |XBModemBuffer,x
		sta |XBPacketOut+3
		tay
		txa
		inc a
		and #XBModemBufferMask
		sta |XBModemTail

	    else

		lda |XBModemBuffer,x
		sta |XBPacketOut+2
		tay
		txa
		inc a
		and #XBModemBufferMask
		sta |XBModemTail

	    endif

;; check checksum

		tya
		eor |XBPacketOut+1
		eor |XBPacketOut
		cmp #$5a
		bne @bad_checksum

;; mask data (for 16-bit, not needed)

		rep #$30
		longa	on
		longi	on

		lda #0		; no error!
		rts
@modemErr:
		rep #$30
		longa	on
		longi	on
		lda #kOverrunError
		rts
@noData:
		rep #$30
		longa	on
		longi	on
		lda #kNoData
		rts
@bad_checksum:
	    if (Game_Patch) then
		inc |GR_ChecksumErrors
	    endif
		rep #$30
		longa	on
		longi	on
		lda #kGibbledPacket
		rts

**************************************************
*
* _XBSendSavePacket16
*
* Save the joystick value in the local queue for later
* use and for error correction that may be required later.
* Fall through to...
*
* _XBSendPacket16
*
* Calculate the checksum and send the packet.
*
* Inputs: XBSendPacket
*
* Outputs: none. This routine can't fail.
*
**************************************************

_XBSendSavePacket16:

;; queue the packet for retries

		ldx |XBLocalHead
		lda |XBSendPacket
		sta |XBLocalQueue,x
		txa
		clc
		adc #4
		and #XBBufferSizeMask
		sta |XBLocalHead

_XBSendPacket16:

;; calculate the checksum

		sep #$20
		longa	off
		lda |XBSendPacket
		eor |XBSendPacket+1
		eor #$5a
		sta |XBSendPacket+2
		sta |XBSendPacket+3

;; Send the bytes

		ldx #0
@wait1:
	    if (Modem_Comm) then
		lda >mdFred_Soft+mdRead_Status_1
		bit #%1000
		bne @overrun
		lsr a
		bcs @wait1
		lda |XBSendPacket,x
		sta >mdFred_Soft+mdTransmit_Buffer
	    else
		lda |XBSendPacket,x
		jsr _XBPockyByte
	    endif
		inx
		cpx #Packet_Size
		bne @wait1
		rep #$30
		longa	on
		rts
	    if (Modem_Comm) then
@overrun:
		sta |XBModemErr
	    if (Game_Patch) then
		inc |GR_OverrunErrors
	    endif
		rep #$30
		rts
	    endif

**************************************************
*
* _XBEstablishSync
*
* This routine attempts to gain a clear line by
* sending consecutive bytes in a table.
*
* The first byte is sent repeatedly until it is received in return.
* Then the second byte is sent once, and we wait until we receive one in
* return. Then the third is sent once, we wait, then the fourth. If
* we ever get something unexpected, we return to the first state (writing
* the first byte repeatedly).
* Note: when waiting for the second byte, we accept and ignore the
* first byte, since it's been sent many times.
*
* Do not depend on the exact timing of this routine, since you never no how
* long it will take; especially when it's patched out to the modem version,
* where it will tweak modem registers upon failure.
*
* Inputs: none.
*
* Outputs: A16 = error code. Nonzero means "error".
*
**************************************************

_XBEstablishSync:

; Set up local variables

XBEntryTicks	equ 1	    ; word
XBLocalTimeout	equ 3	    ; word

localsSize	equ 4
@localTimeout	equ 60	    ; time to wait for next table entry (one second)
@globalTimeout	equ 60*4    ; time to allow this routine to run (four seconds)
@tableSize	equ 4	    ; number of elements to send

		phd	    ; set up a local stack frame
		pha
		pha
		tsc
		tcd

		lda |XBTicks
		sta <XBEntryTicks

		ldy |XBModemHead
		sty |XBModemTail    ; clear buffer

		sep #$20
		longa	off	    ; 8 bit accumulator throughout the loop

@establish0:
		ldx #0		    ; X = index into EstablishTable

		rep #$20
		longa on

		lda |XBTicks
		sec
		sbc <XBEntryTicks
		cmp #@globalTimeout
		bge @errorExit

		sep #$20
		longa off
@establish1:
		ldy #@localTimeout
		sty <XBLocalTimeout
@wait2write:
	    if (Modem_Comm) then
		lda >mdFred_Soft+mdRead_Status_1
; ignore overrun errors
		lsr a
		bcs @wait2write
		lda |@EstablishTable,x
		sta >mdFred_Soft+mdTransmit_Buffer
	    else
		lda |@EstablishTable,x
		jsr _XBPockyByte
	    endif
	    if Comm_Debug then
		ora #$20
		jsr _XBCout
	    endif
@read_byte:
		ldy |XBModemTail
		cpy |XBModemHead
		bne @getbyte
		ldy |XBTicks
@waitTick:
		cpy |XBTicks
		beq @waitTick
		dec <XBLocalTimeout
		beq @establish0		; if we've timed out, go back to phase 0
		txy
		bne @read_byte		; if we're not in phase zero, keep waiting until our local timeout ends
		bra @wait2write		; if we're in phase zero, write the first byte again
@getbyte:
		lda |XBModemBuffer,y
		pha
		tya
		inc a
		and #XBModemBufferMask
		sta |XBModemTail
		pla
	    if Comm_Debug then
		pha
		and #$7f
		cmp #$20
		bge @no_or
		ora #$20
@no_or
		jsr _XBCout		; this prints out debug stuff so you can see _XBEstablishSync in action
		pla
	    endif
		cmp >@EstablishTable,x
		beq @correct_byte
		cpx #0
		beq @read_byte		; if we're waiting for #0, keep looking!
		cpx #1
		bne @establish0
		cmp >@EstablishTable
		beq @read_byte		; if we're waiting for #1 and we get #0, that's okay
		bra @establish0		; start again
@correct_byte:
		inx
		cpx #@tableSize
		bne @establish1
@errorExit:
		rep #$30
		longa on
		cpx #@tableSize	    ; in other words, was XBEstablishIndex at the end of the table?
		bne @error
	    if Comm_Debug then
		lda #endl
		jsr _XBCout	    ; print out a <CR>
	    endif
		lda #0
		sta |XBModemErr	    ; clear out old modem errors
		bra @exit
@error:
		lda #kTimeout
@exit:
		pld
		pld	; clean up the stack
		pld
		rts

@EstablishTable db  'DARK'


**************************************************
*
* _XBSwapData
*
* This routine is called every time you want to exchange
* controller values.
*
* Inputs: XBLocalPacketIn = the local 16-bit controller (or other) value
*   to send to the remote and queue up locally for later use.
*
* Outputs: A16 = error code, hopefully 0.
*   If A16 is 0, XBRemotePacketOut and XBLocalPacketOut are
*   the remote and local values passed to XBLocalPacketIn by the
*   two machines XBLatency values ago.
*
* You will probably need to conditionally reverse these values
* on one of the machines (generally the slave).
*
*
**************************************************

_XBSwapData:
		ldx #1
		jsr _XBGetPacket16
		tax
		bne @exit

		lda |XBPacketOut
		sta |XBRemotePacketOut

		inc |XBGameLoopCounter

;; get local data

		ldx |XBLocalTail
		lda |XBLocalQueue,x
		sta |XBLocalPacketOut
		txa
		clc
		adc #4
		and #XBBufferSizeMask

		sta |XBLocalTail
		lda |XBLocalPacketIn
		sta |XBSendPacket
		jsr _XBSendSavePacket16

		lda #0		; no error
@exit:
		rts

**************************************************
*
* _XBExchangeData
*
* This routine is the high level version of _XBSwapData.
* It may take a long time (up to about 15 seconds if there's an error)
* before returning.
*
* Inputs: XBLocalPacketIn = the local 16-bit controller (or other) value
*   to send to the remote and queue up locally for later use.
*
* Outputs: A16 = error code, hopefully 0.
*   If A16 is 0, XBRemotePacketOut and XBLocalPacketOut are
*   the remote and local values passed to XBLocalPacketIn by the
*   two machines XBLatency values ago.
*
* If this guy fails, pass the error code to XBReturn.
*
**************************************************

_XBExchangeData:
@timeout	equ 60*15	    ; 15 second timeout

		lda |XBTicks
		pha		    ; entry time is at 1,s
@retry_loop:
		ldy #30		    ; number of ticks to allow kNoData errors
@retry_noData:
		phy
	    if (Game_Patch) then
		jsr XBSwapData
	    else
		jsl XBSwapData
	    endif
		ply
		tax
		beq @exit	    ; no problem!

;; Put your message "Line noise; hang on!" here. (No hurry; you must wait a tick to retry anyway.)

		cpx #kNoData
		bne @big_error
		lda |XBTicks
@wait_tick:
		cmp |XBTicks
		beq @wait_tick	    ; wait a tick before trying again
		dey
		bne @retry_noData
	    if (Game_Patch) then
		sep #$20
		inc |GR_Timeouts
		rep #$20
	    endif
@big_error:			    ; "big" means "not just kNoData", or kNoData for a long time
	    if Comm_Debug then	    ; print out error code returned by XBSwapData
		phx
		lda #endl
		jsr _XBCout
		lda #'#'
		jsr _XBCout
		lda 1,s
		jsr _XBWordOut
		lda #' '
		jsr _XBCout
		plx
	    endif
		lda |XBTicks
		sec
		sbc 1,s	    ; old XBTicks value
		cmp #@timeout
		bge @err_exit

		phy
	    if (Game_Patch) then
		inc |GR_ErrorRecovers	    ; synchronizes called
		jsr XBSynchronize
	    else
		jsl XBSynchronize
	    endif
		ply
		tax
		beq @retry_loop
		bra @big_error
@err_exit:
		txa		; get error code back
	    if (Game_Patch) then
		sta |GR_LocalGameError
	    endif
@exit:
		plx		; clean up stack

;; turn off message here

		rts

**************************************************
*
* _XBSynchronize
*
* This routine is used to establish the initial connection
* and to recover from errors from XBSwapData (XBExchangeData
* calls this automatically). It calls _XBEstablishSync
* up to 8 times, so it can take about 15 seconds before
* giving up. If it fails, pass the error code to XBReturn.
*
* Inputs: none.
*
* Outputs: A16 = error code, hopefully 0.
*
**************************************************

_XBSynchronize:
@timeout	equ 15*60		; 15 seconds

		ldx |XBTicks
		phx			; save it on the stack
@sync_loop:
	    if (Modem_Comm&Game_Patch) then
		jsr Check_Line
	    endif
		jsr _XBEstablishSync
		tax
		bne @error
		jsr _XBResendPackets16
		tax
		beq @exit
@error:
	    if (Modem_Comm) then
		jsr _XBRetrainModem
	    endif
		lda |XBTicks
		sec
		sbc 1,s		    ; old tick value
		cmp #@timeout
		blt @sync_loop
		txa
@exit:
		plx	; fix up stack
		rts

	    if (Modem_Comm) then
**************************************************
*
* _XBRetrainModem
*
* This routine is used after an error in XBEstablishSync
* to "retrain" the modem.
*
* Inputs: none.
*
* Outputs: none. Hopefully, the modems are behaving now.
*
**************************************************

_XBRetrainModem:
		ldx #20		; retrain tick count
		sep #$20
		longa	off
		lda >mdFred_Soft+mdRetrainReg
		ora #2
		sta >mdFred_Soft+mdRetrainReg
		nop
		nop
@retrain_wait:
		ldy |XBTicks
@wait_a_tick:
		cpy |XBTicks
		beq @wait_a_tick
		lda >mdFred_Soft+mdRetrainReg
		and #1
		beq @retrain_done
		dex
		bne @retrain_wait
		lda >mdFred_Soft+mdRetrainReg
		and #$ff-2
		sta >mdFred_Soft+mdRetrainReg
@retrain_done:
		rep #$30
		longa	on
		and #$ff
		rts
	    endif


**************************************************
*
* _XBResendPackets16
*
* This routine is used after an error to determine where
* the two machines are with respect to each other (since an
* error may cause a drift by a frame or two, as the machine
* without the error may empty its packet queue before entering
* error recovery mode), and resend the appropriate packets.
*
* Inputs: none.
*
* Outputs: A16 = error code, hopefully 0.
*
**************************************************

_XBResendPackets16:
		lda #$dead
		sta |XBSendPacket
		jsr _XBSendPacket16

		lda |XBGameLoopCounter
		sta |XBSendPacket
		jsr _XBSendPacket16

		ldx #30	    ; number of ticks to wait for data
		jsr _XBGetPacket16
		tax
		bne @data_error

		lda |XBPacketOut
		cmp #$dead
		bne @data_error

		ldx #30	    ; number of ticks to wait for data
		jsr _XBGetPacket16
		tax
		bne @data_error

		lda |XBPacketOut	;; this is the remote game frame number
		sec
		sbc |XBGameLoopCounter	;; local game frame number

;; A = remote game frame - local game frame

		bmi @negative
		cmp #15
		blt @ok
@out_of_sync:
		lda #kOutOfSync
		bra @data_error
@negative:
		cmp #-15
		blt @out_of_sync
@ok:
		asl a
		asl a	; since XBLocalTail is multiplied by 4
		clc
		adc |XBLocalTail	    ; so A = ptr to head of data to resend
@resend_loop:
		and #XBBufferSizeMask
		tax
		cmp |XBLocalHead
		beq @done_resend
		lda |XBLocalQueue,x
		sta |XBSendPacket
		phx
		jsr _XBSendPacket16
		pla
		clc
		adc #4
		ldx |XBTicks
@wait_tick:
		cpx |XBTicks
		beq @wait_tick
		bra @resend_loop
@done_resend:
		lda #0
		rts
@data_error:
		lda #kTimeout
		rts

	    if (Game_Patch=0) then
**************************************************
*
* _XBPostData
*
* This routine copies the game data
*
* Inputs:
*
* Outputs:
*
**************************************************

_XBPostData:
		ldx #48
		lda |XBIsNetworkGame
		beq @silly_copy
@copy:
		lda |XBPostedData,x
		sta |XBFinalPosted,x
		dex
		dex
		bpl @copy
		rts
@silly_copy:
		lda |XBPostedData,x
		sta |XBPostedData,x
		dex
		dex
		bpl @silly_copy
		rts
	    endif   ;Game_Patch

	    if (Game_Patch=0) then
**************************************************
*
* _XBSafeQuit
*
* This routine returns to the XBAND OS iff the
* following conditions are true:
*
* (1) XBAND is installed and active
* (2) the player is playing a single player practice game
* (3) the player has timed out, waiting for an opponent.
*
* Otherwise the routine returns to its caller.
*
* Inputs: none.
*
* Outputs: none. It might not return tho.
*
**************************************************

_XBSafeQuit:
		if Debug_Port_A then
		phb
		phk
		plb
		ldx #@text&$ffff
		jsr _XBPrintText
		bra @done
@text:
		db  'Safe to quit'
		db  endl,0
@done:
		plb
		endif
		rts
	    endif   ;Game_Patch


	    if (Game_Patch)

		xdef	XBInit,XBPollModem
		xdef	XBSynchronize,XBExchangeData,XBSwapData	    ; these labels are exported to the linker

XBInit		equ _XBInit
XBPollModem	equ _XBPollModem
XBExchangeData	equ _XBExchangeData
XBSwapData	equ _XBSwapData
XBSynchronize	equ _XBSynchronize
	    else

XBLibrary:
		xdef	XBInit,XBSetNoise,XBPollModem
		xdef	XBSynchronize,XBExchangeData,XBSwapData	    ; these labels are exported to the linker
		xdef	XBPostData,XBSafeQuit,XBReturn

XBInit:
		ldx #kXBInit-XBDispatchTab
		bra _XBDispatcher
XBSetNoise:
		ldx #kXBSetNoise-XBDispatchTab
		bra _XBDispatcher
XBPollModem:
		ldx #kXBPollModem-XBDispatchTab
		bra _XBDispatcher
XBSynchronize:
		ldx #kXBSynchronize-XBDispatchTab
		bra _XBDispatcher
XBExchangeData:
		ldx #kXBExchangeData-XBDispatchTab
		bra _XBDispatcher
XBSwapData:
		ldx #kXBSwapData-XBDispatchTab
		bra _XBDispatcher
XBPostData:
		ldx #kXBPostData-XBDispatchTab
		bra _XBDispatcher
XBSafeQuit:
		ldx #kXBSafeQuit-XBDispatchTab
		bra _XBDispatcher
XBReturn:
		ldx #kXBReturn-XBDispatchTab	    ; fall through to _XBDispatcher

**********************************
*
* Use the dispatcher for the following reasons:
*
*  - so we need only patch one place to make the game work with the modem
*  - so it's easy to compile the "no Pocky" final version of the cart
*
* Warning! This makes it a bit difficult to debug;
* since XBPollModem is called in interrupt, putting a
* breakpoint in the dispatcher can cause the debugger to
* get confused, since it may hit the breakpoint in interrupt.
*
**********************************


_XBDispatcher:
		php
		rep #$30
		phb
		pea XBLocalQueue>>8 ; set up the correct bank
		plb
		plb
		jsr (XBDispatchTab,x)
		plb
		plp
		rtl

XBDispatchTab:
kXBInit:
		dw  _XBInit&$ffff
kXBSetNoise:
		dw  _XBSetNoise&$ffff
kXBPollModem:
		dw  _XBPollModem&$ffff
kXBSynchronize:
		dw  _XBSynchronize&$ffff
kXBExchangeData:
		dw  _XBExchangeData&$ffff
kXBSwapData:
		dw  _XBSwapData&$ffff
kXBPostData:
		dw  _XBPostData&$ffff
kXBSafeQuit:
		dw  _XBSafeQuit&$ffff
kXBReturn:
		dw  _XBReturn&$ffff

	    endif   ; Game_Patch


	    if (Game_Patch=0) then
**************************************************
*
* _XBReturn
*
* This routine returns to the XBAND OS and terminates
* the game.
*
* Inputs: A16 = error code. A successful game
*  should return error code 0.
*
* Outputs: does not return.
*
**************************************************

_XBReturn:
		if Debug_Port_A then
		phb
		phk
		plb
		pha
		ldx #@text0&$ffff
		jsr _XBPrintText
		pla
		jsr _XBWordOut
		ldx #@text1&$ffff
		jsr _XBPrintText
		lda >XBFinalPosted+2
		jsr _XBWordOut
		lda >XBFinalPosted
		jsr _XBWordOut
		ldx #@text2&$ffff
		jsr _XBPrintText
		lda >XBFinalPosted+6
		jsr _XBWordOut
		lda >XBFinalPosted+4
		jsr _XBWordOut
		ldx #@text3&$ffff
		jsr _XBPrintText
		lda >XBFinalPosted+10
		jsr _XBWordOut
		lda >XBFinalPosted+8
		jsr _XBWordOut
		ldx #@text4&$ffff
		jsr _XBPrintText
		lda >XBFinalPosted+14
		jsr _XBWordOut
		lda >XBFinalPosted+12
		jsr _XBWordOut
		ldx #@text5&$ffff
		jsr _XBPrintText
		ldx #16
@pr_bytes:
		lda >XBFinalPosted,x
		jsr _XBWordOut
		lda #' '
		jsr _XBCout
		inx
		inx
		cpx #48
		blt @pr_bytes

		lda #$8f
		sta >$2100  ; turn off the screen

		brk 0
@text0:
		db  endl,endl
		db  'Error code:'
		db  0
@text1:
		db  endl
		db  'Score local 1:'
		db  0
@text2:
		db  endl
		db  'Score local 2:'
		db  0
@text3:
		db  endl
		db  'Score remote 1:'
		db  0
@text4:
		db  endl
		db  'Score remote 2:'
		db  0
@text5:
		db  endl,endl
		db  'Other data:'
		db  0

@done:
		plb
		endif

		sep #$20
		longa off
@hang:
		lda #$80
		sta >$2100	    ; turn off screen
		bra @hang	    ; since we're supposed to return to the OS

		longa on

		rts
	    endif   ;Game_Patch

	    if (Debug_Port_A|Comm_Debug) then

;
; The following code is only included if you want to use
; XBCout to print data out port A.
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; XBCout
;;
;;  Print ascii low byte of accumulator to port A (debug port)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_XBCout:
		php
		sep #$20
		longa off	; put the assembler into "short memory" mode for immediate instructions
		pha
		lda >XBIsNetworkGame
		bne @wait
		pla
		bra @wrote_it
@wait:
		lda >lsr_A	;Line Status Reg
		and #$20	;Xmit Holding Reg Empty when set
		beq @wait	;wait for bit to set
		pla
		sta >txd_A	;write it
@wrote_it:
		plp
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; XBPrintText
;;
;;  Print ascii text to port A (debug port)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_XBPrintText:
		php
		sep #$20
@loop:
		lda |0,x
		beq @done
		jsr _XBCout
		inx
		bra @loop
@done:
		plp
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; XBNibbleOut
;;
;;  Print low nibble of accumulator as hex value to port A (debug port)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_XBNibbleOut:
		php
		sep #$20
		longa	off
		and #$f
		cmp #$a
		blt @noadd
		adc #'A'-'9'-2
@noadd
		adc #'0'
		jsr _XBCout
		plp
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; XBByteOut
;;
;;  Print 8-bit accumulator as hex value to port A (debug port)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_XBByteOut:
		php
		sep #$20
		longa	off	    ; turn off 16-bit accumulator (assembler pseudo-op only)
		pha
		lsr a
		lsr a
		lsr a
		lsr a
		jsr _XBNibbleOut
		lda 1,s
		jsr _XBNibbleOut
		pla
		plp
		longa on
		rts


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; XBWordOut
;;
;;  Print 16-bit accumulator as hex value to port A (debug port)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_XBWordOut:
		pha
		xba
		jsr _XBByteOut
		lda 1,s
		jsr _XBByteOut
		pla
		rts

	    endif		    ; Debug_Port_A

	    endif		    ; Final_Build


	end	    ; end of file (my assembler gives me a warning if this is not here)
