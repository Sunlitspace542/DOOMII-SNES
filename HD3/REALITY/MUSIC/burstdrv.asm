;****************************************************************************
;*                                                                          *
;*               SCULPTURED SOFTWARE BURST DRIVER    CODE MODULE            *
;*                                                                          *
;*               VERSION 1.00      APRIL 5, 1995       RON STAGG            *
;*                                                                          *
;****************************************************************************



;	*************	     EQUATES FOR BURST DRIVER        *************

;_MDNumModules      equ     MusicTableAddress+0     ; Number of Modules
;_MDNumSongs        equ     MusicTableAddress+1     ; Number of Songs
;_MDNumEffects      equ     MusicTableAddress+2     ; Number of Effects
;_MDModuleTable     equ     MusicTableAddress+3     ; Module Address Table
;B_CHANNEL_COUNT    equ	    3
;B_LAUNCH_DELAY     equ	    5	;Max number of V-Blanks between request and launch of effect
;B_RELEASE_DELAY    equ     -10 
;B_BUFFER0	    equ	    BlastBufferAddress 
;B_BUFFER1	    equ	    B_BUFFER0 + (BlastBufferSize/B_CHANNEL_COUNT)
;B_BUFFER2	    equ	    B_BUFFER1 + (BlastBufferSize/B_CHANNEL_COUNT)
;B_UPLOAD_BLOCKSIZE equ     90	;** WARNING **  MUST BE MULTIPLE OF THREE
;B_STRUCTURE_SIZE   equ	    11		;number of bytes in burst structure
;APU_Port0     	    equ	    $2140
;APU_Port1     	    equ	    $2141
;APU_Port2          equ	    $2142
;APU_Port3     	    equ     $2143
;DCOM_BURST	    equ	    DCOM_SLOW_BLAST
;BURST_CLEAR	    equ	    $BC
;BURST_BUSY	    equ	    $BB

;       *************       VARIABLES FOR BURST DRIVER       **************
;
;       >>>   SCULPTURED SOFTWARE BURST DRIVER RAM0 (DP) VARIABLES   <<<

;BBF		  ds	1	;Burst Semaphore
;b_effect	  ds	1	;holds number of requested effect
;b_priority	  ds	1	;holds priority of requested burst effect
;b_bwave	  ds	1	;holds bwave number of requested effect
;burst_mode	  ds	1	;boolean: in burst mode or not.
;block_size	  ds    2	;
;temp		  ds	2 	;
;apu_addr	  ds	2	;holds destination apu address of requested effect
;effect_table	  ds	3 	;address of effect table 
;b_history        ds	3	;history list of burst channel usage
;burst_structure  ds 	(B_CHANNEL_COUNT * B_STRUCTURE_SIZE)
;********************************************
;****  Format of Burst Structure 	*****
;****					*****
;****  byte  0       = Priority  	*****
;****  byte  1       = Effect Number    *****
;****  byte  2       = Bwave Number     *****
;****  byte  3       = Launch/Release   *****
;****  bytes 4,5     = Byte Counter     *****
;****  bytes 6,7,8   = Source ROM Addr  *****
;****  bytes 9,10    = APU Address	*****
;****					*****
;**** This is repeated for each channel *****
;********************************************





;****************************************************************************
;****************************************************************************
Burst_Init_Long
;************************************************************
;**** This procedure initializes all burst pointers and *****
;**** variables.  Interupts must NOT be enabled before  *****
;**** this routine executes.				*****
;************************************************************
	LAI
	lda	>_MDNumModules	;total number of modules
	and	#$ff
	inc			;add 1 for common module
	sta	temp
	asl			;*3 bytes for each module entry
	clc
	adc	temp		;index past module address table
	clc
	adc	#_MDModuleTable
	sta	temp
	lda	>_MDNumSongs	;index past the song_array (1 byte for each song)
	and	#$ff
	clc
	adc	temp
	sta	effect_table
	SA
	lda	#^_MDModuleTable
	sta	effect_table+2

	stz	burst_structure
	stz	burst_structure+(B_STRUCTURE_SIZE)	;clear all three
	stz	burst_structure+(2*B_STRUCTURE_SIZE)	;priority levels
	stz	burst_structure+2
	stz	burst_structure+(B_STRUCTURE_SIZE)+2	;init all three
	stz	burst_structure+(2*B_STRUCTURE_SIZE)+2	;bwave numbers
        
        stz     b_history
        lda     #1
        sta     b_history+1
        lda     #2
        sta     b_history+2

	lda	#BURST_CLEAR
	sta	BBF		;clear Burst Busy Flag

	rtl

;****************************************************************************
;****************************************************************************
Burst_Effect_Long
;****************************************************************************
;**** Call this procedure to start   ******
;**** a burst-effect.		     ******
;**** a = effect number  (00-FF)     ******
;**** x = priority level (01-FF)     ******
;**** y = bwave number		     ******
;******************************************
	SAI
	stx b_priority
	sta b_effect
	sty b_bwave
        
	lda #BURST_BUSY
	sta BBF			;set Burst Busy Flag

        jsr check_for_repeat
        bcs .return             ;carry bit will be set if repeat was found
	jsr select_burst_channel
	cpx #$FF		;was burst request denied?
	bne .request_granted
.return
	SA
	lda #BURST_CLEAR
	sta BBF
	LAI
	rtl			;return if request was denied, or if
                                ;repeat was found.
.request_granted
	SAI

;****  FIRST: Kill effect which is currently in this channel  ******
	
	lda #KILL_EFFECT
	xba
	lda burst_structure+1,x	;get effect number
	LAI
	tay
	phx			;save x
	lda #DCOM_STOP_SOUND
	jsl _SendDriverCommand
	LI
	plx			;restore x

;****  Now all burst parameters must be loaded into the   *****
;****  structure that pertains to the selected channel.   *****

	SAI

;****  load priority   ***********
	lda b_priority
	sta burst_structure,x

;****  load effect number   ******

	lda b_effect
	sta burst_structure+1,x

;****  load bwave number   ********

	lda b_bwave
	sta burst_structure+2,x

;****  Now setup Launch variable  ****

	lda #B_LAUNCH_DELAY
	sta burst_structure+3,x

;****  load Byte counter  ************
	
	ldy b_effect
	LAI
	lda [effect_table],y	;get module number of effect
	and #$ff	;\
	sta temp	; \
	asl		;  multiply by 3
	clc		; /
	adc temp	;/

	phx
	tax
	lda >_MDModuleTable,x
	sta src_ptr		;get address of burst header
	SA
	lda >_MDModuleTable+2,x
	sta src_ptr+2		;get bank of burst header
	LA
	plx

	ldy #9			;offset to burst length in burst header
	lda [src_ptr],y		;get burst length
	sta burst_structure+4,x	;save burst length in byte counter


;****  Now load Source ROM Address  ****
	
	iny			;inc index into burst header
	iny			;inc index into burst header
	lda [src_ptr],y		;get ROM address of burst data
	sta burst_structure+6,x	;load address into structure
	iny			;inc index into burst header
	iny			;inc index into burst header
	SA
	lda [src_ptr],y		;get ROM bank of burst data
	sta burst_structure+8,x	;load bank into burst structure

;*****  Now get destination APU address ****

	LA
	lda apu_addr		;this value was determined in select_burst_channel routine
	sta burst_structure+9,x

	LAI
	txa			;get structure offset to determine channel number
	beq .chan0		;if structure offset=0 then channel=0
	cmp #B_STRUCTURE_SIZE	;check for channel 1
	beq .chan1
	bra .chan2		;


;*****  Now realign APU bwave pointer for this effect  **************

.chan0 				;
	LAI			; 
	ldy #B_BUFFER0		;get address of burst buffer0  
	lda #DCOM_BLAST_ADDR	;   
	jsl _SendDriverCommand
	bra .setwave     	;     
.chan1				;      
	LAI			;	
	ldy #B_BUFFER1		;get address of burst buffer1
	lda #DCOM_BLAST_ADDR	;	  
	jsl _SendDriverCommand
	bra .setwave            ;	    
.chan2				;	     
	LAI			;
	ldy #B_BUFFER2		;get address of burst buffer2
	lda #DCOM_BLAST_ADDR	;
	jsl _SendDriverCommand

.setwave			;
        LAI
	lda b_bwave     	;get Bwave number of effect
	and #$00FF
	tay
	lda #DCOM_SETWAVEASBLAST;
	jsl _SendDriverCommand

	SA
	lda #BURST_CLEAR
	sta BBF			;clear Burst Busy Flag
	LAI
	rtl
;****************************************************************************
;****************************************************************************
check_for_repeat
;******************************************************************
;***  This routine checks each channel to see if it is already ****
;***  playing an effect which shares the same bwave number     ****
;***  as the requested effect. If found, the priority of that  ****
;***  channel is set to the highest priority level of the two  ****
;***  effects, and if the first sound is in play mode then the ****
;***  the second sound is started immediately.                 ****
;******************************************************************
        SAI
        ldx #0
        lda b_bwave             
        cmp burst_structure+2,x           ;check channel 0 for repeat bwave
        beq .repeat_found

        ldx #B_STRUCTURE_SIZE
        lda b_bwave
        cmp burst_structure+2,x           ;check channel 1 for repeat bwave
        beq .repeat_found

        ldx #(2*B_STRUCTURE_SIZE)
        lda b_bwave
        cmp burst_structure+2,x           ;check channel 2 for repeat bwave
        beq .repeat_found

.no_repeat_found
        clc                     ;no repeat was found, clear carry bit
        rts

.repeat_found
        lda b_priority
        cmp burst_structure,x           ;if requested priority is higher
        bcc .check_mode                 ;than current priority then
        sta burst_structure,x           ;copy requested priority into
        bra .check_mode                 ;burst_structure

.check_mode
        lda burst_structure+3,x         ;get Launch/Release counter value
	bmi .in_play_mode		;if in release mode, play effect
	beq .in_play_mode		;if in play mode, play effect
        bra .return                     ;if in launch mode, dont play effect

.in_play_mode
        lda #B_RELEASE_DELAY
        sta burst_structure+3,x
.start_effect
        LAI
	lda b_effect    	;get effect number
        and #$00ff
	tay
	lda #DCOM_START_EFFECT	;tell APU to start playing effect
	jsl _SendDriverCommand
.return
        sec                     ;repeat was found, set carry bit
        rts
;****************************************************************************
;****************************************************************************
select_burst_channel
;******************************************************************
;***  This procedure selects a channel and returns an offset   ****
;***  to the selected channel data in X.  If no channel was    ****
;***  selected, $FF will be returned in X.		       ****
;******************************************************************
	SAI
        ldx #$FF

.hist0  lda b_history           ;get the channel number from the first
        jsr .check_chan         ;history slot and check its priority
        cpx #$FF
        beq .hist1
          SA
          ldy b_history         ;\
          lda b_history+1       ; reorder history list:
          sta b_history         ; first slot holds oldest effect
          lda b_history+2       ; second slot holds next oldest effect
          sta b_history+1       ; third slot holds newest effect
          sty b_history+2       ;/
          rts

.hist1  lda b_history+1         ;get the channel number from the second
        jsr .check_chan         ;history slot and check its priority
        cpx #$FF
        beq .hist2
          SA
          ldy b_history+1       ;
          lda b_history+2       ;   reorder history list  (see above)
          sta b_history+1       ;
          sty b_history+2       ;
          rts

.hist2  lda b_history+2         ;get the channel number from the third
        jsr .check_chan         ;history slot and check its priority
        rts  
        


.check_chan
        beq .chan0
        cmp #1
        beq .chan1
        bra .chan2
.chan0
	lda burst_structure
	cmp b_priority          ;check priority of effect in channel 0
	bcs .done0              ;leave if requested priority is lower
          LA
	  lda #B_BUFFER0
	  sta apu_addr          ;channel 0 has been selected
          ldx #0
.done0    rts


.chan1	lda burst_structure+B_STRUCTURE_SIZE
	cmp b_priority          ;check priority of effect in channel 1
	bcs .done1              ;leave if requested priority is lower
          LA
	  lda #B_BUFFER1
	  sta apu_addr          ;channel 1 has been selected
          ldx #B_STRUCTURE_SIZE
.done1    rts


.chan2	lda burst_structure+(2*B_STRUCTURE_SIZE)
	cmp b_priority          ;check priority of effect in channel 2
	bcs .done2              ;leave if requested priority is lower
          LA
	  lda #B_BUFFER2
	  sta apu_addr          ;channel 2 has been selected
          ldx #(2*B_STRUCTURE_SIZE)
.done2    rts

;****************************************************************************
;****************************************************************************
Upload_Burst_Long
;*******************************************
;****   Call this procedure from NMI   *****
;****   routine.  This must execute    *****
;****   every V-blank.		       *****
;*******************************************
	SA
	lda BBF		
	cmp #BURST_CLEAR	;check Burst Busy Flag
	beq .continue		;if clear then continue
	rtl			;else return

.continue
        stz burst_mode                  ;initially we are not in burst mode. Clear flag

        LI              
	ldx #$00			;set structure index to channel 0 
        jsr burst_block			;burst channel 0
        bcs .return                     ;return if we have left Burst Mode

	LI
	ldx #B_STRUCTURE_SIZE		;set structure index to channel 1
	jsr burst_block			;burst channel 1
        bcs .return                     ;return if we have left Burst Mode

	LI
	ldx #(2*B_STRUCTURE_SIZE)	;set structure index to channel 2
        jsr burst_block			;burst channel 2
        bcs .return                     ;return if we have left Burst Mode

        SA                      ;if Burst Mode was never entered
        lda burst_mode          ;then bypass code which tells APU
        beq .return             ;to exit Burst Mode

	LA			;\
	stz APU_Port0		; \
	SA			;  This tells APU to exit Burst Mode
	lda _MDSerialNum	; /
	inc			;/
	bmi .neg
	lda #$80		;\
.neg				; \
	and #$9f		;  Get a compatible serial number	
        sta _MDSerialNum        ; /   
	sta APU_Port2		;/
.wait
	cmp APU_Port2
	bne .wait

.return
	rtl
;****************************************************************************
;****************************************************************************
burst_block
;*************************************************
;****   X must contain an offset to the start  ***
;****   byte of the desired burst structure.   ***
;****   This procedure will then check if the  ***
;****   desired channel is active.  If so, a   ***
;****   block of data will be uploaded to the  ***
;****   APU and all pertinent channel data     ***
;****   will be updated.                       ***
;*************************************************
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	check priority to see if channel is inactive (priority=0)
;           return
;	check if in Release Mode   (Launch/Release byte < 0)
;	    inc release counter
;	    check if release counter==0
;	        deactivate channel (set priority to 0)
;	    return
;	upload block to apu
;	check if upload is complete  (byte counter <= 0)
;	    set release counter   (will be negative value)
;	    return
;	check if in Launch Mode	  (Launch/Release byte > 0)
;	    dec launch counter
;	    check if launch counter==0
;	    	launch effect
;	return
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	SAI
	lda burst_structure,x		;check priority byte
	bne .active
        clc                             ;ok to burst next channel
	rts				;return if priority==0
.active
	lda burst_structure+3,x		;check Launch/Release byte
	bpl .start_upload		;if Release >= 0 then bypass release code


.release_mode
	    inc
	    sta burst_structure+3,x	;inc release counter
	    bne .release_return		;exit release mode if release counter==0
	    stz burst_structure,x	;set priority to zero
.release_return
            clc                         ;ok to burst next channel
	    rts				;return if in release mode


.start_upload
        SA                              ;only get a new serial number once,
        lda burst_mode                  ;just before entering Burst Mode
        bne .send_start
.again                                  ;This gaurantees that APU
        lda _MDSerialNum                ;is ready because randy's code
        cmp APU_Port2                   ;has pre-acknowledge instead
        bne .again                      ;of post_aknowledge
        inc                             ;\
        bmi .neg_num                    ; \
        lda #$80                        ;  gaurantees a serial number
.neg_num                                ;  which is burst compatible
	and #$9f			; /
        sta _MDSerialNum               	;/

.send_start
	LA
	lda burst_structure+9,x		;get APU destination address
	sta APU_Port0
	SA
	lda #DCOM_BURST
	sta APU_Port3	   		;Send request for Burst upload
	lda _MDSerialNum		;send serial number (negative)
	sta APU_Port2
.wait
	cmp APU_Port2
	bne .wait

;  ************  APU is now in Burst Mode  ***********

        lda #$01
        sta burst_mode                  ;set burst mode flag

	ldy #B_UPLOAD_BLOCKSIZE		;load y with internal block counter
	LA
	lda burst_structure+6,x		;get ROM address
	sta src_ptr
	SA
	lda burst_structure+8,x		;get ROM bank
	sta src_ptr+2

	LA
	lda #B_UPLOAD_BLOCKSIZE		;if(B_UPLOAD_BLOCKSIZE > byte counter)
	sta block_size			;    block_size = byte counter
	lda burst_structure+4,x		;else
	cmp block_size			;    block_size =  B_UPLOAD_BLOCKSIZE
	bcs .check.bank			;
	sta block_size			;
.check.bank
	phx   				;save x on stack for later use
	ldx #0				;clear bank overshoot flag
	lda src_ptr			;get current ROM source pointer
	bpl .clear.y			;if not negative then no change (branch)
	  clc
	  adc block_size		;add block_size to ROM pointer
	  bmi .clear.y			;if still negative, then no change (branch)
	    pha	  		;
	    lda block_size	;
	    sec			;
	    sbc 1,s		;subtract overshoot from blocksize
	    sta $4204		;load dividend register
	    pha			;
	    lda #3		;
	    sta $4206		;load divisor (divide blocksize by 3)
	    pla			;
	    sta block_size	;
	    pla			;
	    ldx #1		;set bank overshoot flag
	    nop
	    nop
	    lda $4216		;get remainder of division
	    pha			;store remainder on stack for later use
	    lda block_size
	    sec
	    sbc 1,s		;subtract remainder from block (block is now an even multiple of 3)
	    sta block_size
.clear.y
	ldy #$00

.upload_loop;;;;;;;;;;;;;;;;;;;;;
        LA			;
	lda [src_ptr],y		;
	sta APU_Port0		;send first two bytes
	iny			;
	iny			;
        SA			;
	lda [src_ptr],y		;
	sta APU_Port3		;send third byte
        iny			;
	tya			;
	sta APU_Port2		;use byte counter for serial number
	cmp block_size		;
	beq .upload_done	;if byte counter==blocksize exit loop
.ack				;
	cmp APU_Port2		;wait for handshake
	bne .ack		;
	bra .upload_loop;;;;;;;;;

.upload_done
	cmp APU_Port2
	bne .upload_done		;wait for handshake

	txa				;get overshoot flag
	beq .update1			;branch if clear
	  jsr three_bytes
	  pla				;restore stack pointer
	  plx
	  bra .update2
.update1
	LA
	plx			;restore channel index
	lda burst_structure+6,x	;get old ROM address
	clc
	adc block_size		;update ROM address
        sta burst_structure+6,x ;save new ROM address 
.update2
	lda burst_structure+9,x	;get APU destination address
	clc			;
	adc block_size		;update APU destination address
	sta burst_structure+9,x	;save new APU destination address

	lda burst_structure+4,x	;get old byte counter
	sec
	sbc block_size		;update byte counter
	sta burst_structure+4,x	;save new byte counter
	beq .effect_done	;if byte counter = 0 then effect done

	SA
	lda burst_structure+3,x	;get launch variable
	bne .update_launch	;check if in launch mode
        clc                     ;ok to burst next channel
	rts			;return if not in launch mode


.effect_done			;effect is completely uploaded 
	SA
	lda #B_RELEASE_DELAY	;get release delay count
	sta burst_structure+3,x	;put channel in release mode
        clc                     ;ok to burst next channel
	rts


.update_launch			;launch mode
	dec			;dec launch counter
	sta burst_structure+3,x ;save launch counter
	beq .launch_effect	;if launch counter==0 then launch effect
        clc                     ;ok to burst next channel
	rts			;else return

.launch_effect
	LA			;\
	stz APU_Port0		; \
	SA			;  This tells APU to exit Burst Mode
	lda _MDSerialNum	; /
	inc			;/
	bmi .neg
	lda #$80		;\
.neg				; \
	and #$9f		;  Get a compatible serial number	
        sta _MDSerialNum        ; /   
	sta APU_Port2		;/
.w
	cmp APU_Port2
	bne .w

; **********   APU is no longer in Burst Mode   ***********

        LAI
	lda burst_structure+1,x	;get effect number
        and #$00FF
	tay
	lda #DCOM_START_EFFECT	;tell APU to start playing effect
	jsl _SendDriverCommand

;*******************************************
;NOTE:
;After an effect has been launched. No more
;channels should be immediately uploaded.
;An unacceptable delay would occur while SNES waits
;for APU to re-enter Burst Mode.  The remaining
;channels must wait for the next V-blank.
;************************************************
        SI
        sec                     ;signals that no more channels can be 
                                ;bursted during this interrupt
        rts

;****************************************************************************
;****************************************************************************
three_bytes
;**************************************************************
;***  This routine will upload the three bytes that	*******
;***  reside on the bank boundaries.  After the		*******
;***  upload, the ROM address and ROM bank will be 	*******
;***  updated to point to the new bank.			*******
;**************************************************************
	  SA
	  lda 3,s			;get overshoot remainder off stack
	  tax
	  beq .no_remainder		;
	    lda [src_ptr],y		;
	    sta APU_Port0		;send first byte
	    dex 
	    beq .inc_bank1
	    iny
	    lda [src_ptr],y
	    bra .store_port1
.inc_bank1
	    stz src_ptr
	    lda #$80
	    sta src_ptr+1
	    inc src_ptr+2		;inc bank
	    ldy #0
	    lda [src_ptr],y
.store_port1
	    sta APU_Port1
	    cpy	#0
	    bne .inc_bank2
	    iny
	    lda [src_ptr],y
	    bra .store_port3
.inc_bank2
	    stz src_ptr
	    lda #$80
	    sta src_ptr+1
	    inc src_ptr+2
	    ldy #0
	    lda [src_ptr],y
.store_port3
	    sta APU_Port3
	    lda #B_UPLOAD_BLOCKSIZE
	    sta APU_Port2
	    iny
.ack2				
	    cmp APU_Port2		;wait for handshake
	    bne .ack2		
	    bra .overshoot_update

.no_remainder
	  stz src_ptr
	  lda #$80
	  sta src_ptr+1
	  inc src_ptr+2
	  ldy #0
	  bra .r0
.overshoot_update
	  LA
	  lda #3		;if remainder > 0 then
	  clc			;
	  adc block_size	;add 3 to block_size
	  sta block_size
.r0
	  SA
	  lda 5,s		;restore channel index
	  tax
	  tya
	  clc
	  adc src_ptr
	  sta src_ptr
	  LA
	  lda src_ptr
	  sta burst_structure+6,x	;update ROM address
	  SA
	  lda src_ptr+2			;get new ROM bank
	  sta burst_structure+8,x	;save new ROM bank
	  LA
       	  rts
;****************************************************************************
;****************************************************************************
