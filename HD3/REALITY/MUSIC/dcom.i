;
; dcom.i
; SNES SPC700 music driver commands
; Sculptured Software
;
; last edited: 1/20/94
;
; These are the commands that you can pass to the Sculptured Software
; music driver using the routine Pass_Driver_Com. The command numbers
; are not sequential because obsolete commands have been removed.
;
; "Port 0" refers to low byte of y register. "Port 1" is the high byte
; of the y register. Unless specified in the description of a command,
; ports 0 and 1 are ignored.
;
; The music driver is designed to work with output from the PC-based
; development program, "Wolfgang."
;



; This command tells the driver to run. It used after uploading a module
; to the sound ram using the APU_Upload_Ind routine.
;
; Coding example:
;
;	lda	#DCOM_RUN
;	jsr	Pass_Driver_Com

DCOM_RUN = 2



; These commands start a sound effect or song. The sound or song must be
; resident in an already-loaded common module or unique module. When
; Wolfgang outputs sound data to be included in a game it also produces
; a .asm file that contains sound effect and song equates.
;
; Coding example:
;
;	ldy	#crash_eff		; from Wolfgang .asm file
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com

DCOM_START_EFFECT = 4			; port 0 is effect number
DCOM_START_SONG = 6			; port 0 is song number

;****** NEW COMMANDS - NEED GOOD DOCS


DCOM_START_CONT_EFFECT = 8

; port0 contains effect number    1-127
; port1 contains effect ID as in SET_EFFECT_ID
; starts an effect with no duration.
; effect will play until new module is loaded, or DCOM_STOP_CONT_EFFECT
; is called.
; There is nothing wrong with calling several effects with same ID
; if you want to modulate or stop them as a group.
; Also, since the ID is already set with this command, you may use any DCOM
; command with this effect which normally would require SET_EFFECT_ID to
; be called before starting this effect (parameterized effects).

DCOM_STOP_CONT_EFFECT  = 10

; port0 contains ID number of effect to stop.
; This command will stop ALL effects with ID number, including those
; set with SET_EFFECT_ID and START_EFFECT.
; This command works differently than normal STOP_EFFECT calls in that
; the effect is allowed to decay normally, rather than an abrupt stop
; in sound. This allows more natural effects to be played and then
; stopped.
;***********************************************************************



;***********************************************************************
; Debugging command. This section should be deleted for client release.
;***********************************************************************
;
; give a memory dump of APU
; DIAGNOSTIC - reads 3 bytes to ports starting
; at address in PORT0/PORT1
; bytes returned are:
;   PORT0   [ADDRESS]
;   PORT1   [ADDRESS+1]
;   PORT3   [ADDRESS+2]

DCOM_DEBUG = 12

;***********************************************************************



; This command causes the sound driver to mute and re-start the APU upload
; procedure. You can load in new music or a whole new driver. This command
; is used before calling the APU_Upload_Ind routine to load a new module,
; and is used along with DCOM_RUN, described above. A typical sequence 
; might be:
;
;	lda	#DCOM_RELOAD		; tell driver a new module is coming
;	jsr	Pass_Driver_Com
;	lda	#^Common_Address	; upload common module
;	ldx	#Common_Address
;	jsr	APU_Upload_Ind
;	lda	#DCOM_RUN		; tell driver to run again
;	jsr	Pass_Driver_Com

DCOM_RELOAD = 14



; These commands control overall volume fading. DCOM_FADEOUT and DCOM_FADEIN
; use ports 0 and 1 to control the speed of the fade. Port 0 specifies how 
; many ticks between each volume turndown (1 to 255). Port 1 is the size of
; each volume turndown (1 to 127). Both values must be non-zero. Each tick
; is roughly 1/50 of a second. You can calculate the duration of a fadedown
; given the values for Port 0 and Port 1 (assuming you start at full volume):
;
; duration = ((Port 0 * (127 / Port 1)) / 50) seconds
;
; Some examples:
;
;	Port 0 = 1	2.5 seconds to go quiet
;	Port 1 = 1
;
;	Port 0 = 1	quiet in 1 tick
;	Port 1 = 127
;
;	Port 0 = 255	10 minutes to go quiet
;	Port 1 = 1
;
; DCOM_FADEIN will stop once the volumes reaches maximum volume. DCOM_FADEOUT
; will stop when volume reaches 0.
;
; Overall volume will remain at its current level until changed again. That
; is, when you fade a song out, the volume will stay at 0 until you fade
; it back in.
;
; Coding example:
;
;	lda	#DCOM_FADEOUT
;	ldy	#$0201			; port 0 = 1, port 1 = 2
;	jsr	Pass_Driver_Com
;
; Also, see the related command DCOM_FADE_AND_STOP below.

DCOM_FADEOUT = 16
DCOM_FADEIN = 18



; This command is also related to overall volume control. It will stop a
; fade-in or fade-out at the current volume. So if you wanted to fade a 
; song to half volume, you could start a fade-out and then wait half of
; the fade duration before issuing the DCOM_STOPFADE command. However,
; this tends to be inaccurate.
;
; Coding example:
;
;	lda	#DCOM_STOPFADE
;	jsr	Pass_Driver_Com

DCOM_STOPFADE = 20



; This command controls whether the sound driver runs in stereo mode or
; not. Call with port 0 = 50 for mono, port 0 = 0 for stereo. Default is
; stereo.
;
; Coding example:
;
;	lda	#DCOM_STEREOMODE
;	ldy	#0			; stereo
;	jsr	Pass_Driver_Com

DCOM_STEREOMODE = 22


; inform driver that new effect is being blasted. Force shutdown of old
; blast effect. Effect number is in PORT 0.

DCOM_BLASTEFFECT = 24



; This command will stop a currently-playing song without touching the
; current volume (so sound effects will still play). Notes are allowed to
; decay normally.
;
; Coding example:
;
;	lda	#DCOM_STOP_SONG
;	ldy	#0			; stereo
;	jsr	Pass_Driver_Com

DCOM_STOP_SONG = 26



; This command provides a number of different ways to shut off sound.
;
; If port 1 = KILL_EFFECT, Port0 specifies the effect number to kill. ALL
; channels playing this effect number will be killed.
;
; If port 1 = KILL_ALL_EFFECTS, all channels playing effects will be stopped,
; but all music will continue. Port 0 parameter is ignored.
;
; If port 1 = KILL_ALL_MUSIC, all channels playing a music sound will be
; stopped. Port 0 parameter is ignored.
;
; If port 1 = KILL_ALL_SOUNDS, every active channel is stopped (but music
; can trigger new notes, as in KILL_ALL_MUSIC). Port 0 parameter is ignored.
;
; Coding example:
;
;	ldy	#(KILL_ALL_SOUNDS << 8)	; set port 1 to KILL_ALL_SOUNDS
;	lda	#DCOM_STOP_SOUND
;	jsr	Pass_Driver_Com

DCOM_STOP_SOUND = 28

; Port 1 parameters to DCOM_STOP_SOUND:

KILL_EFFECT = 0
KILL_ALL_EFFECTS = $1
KILL_ALL_MUSIC = $FF
KILL_ALL_SOUNDS = $80



;***********************************************************************
; Blast commands. This section should be deleted for client release,
; unless client has licensed blasting.
;***********************************************************************

; Get ready to anytime go into BlastDown mode.

DCOM_BLAST_ADDR = 30

; Go immediately into Blast receive mode ASAP.

DCOM_DO_BLAST = 32

; Go immediately into Blast using checksums.

DCOM_SLOW_BLAST = 34

;***********************************************************************



; This command is essentially the same as DCOM_FADEOUT, with the same
; parameters, but once the volume reaches 0 all music and sounds are
; killed
;
; Coding example:
;
;	lda	#DCOM_FADE_AND_STOP
;	ldy	#$0201			; port 0 = 1, port 1 = 2
;	jsr	Pass_Driver_Com

DCOM_FADE_AND_STOP = 36



; This command is similar to DCOM_START_EFFECT, except that it also
; specifies a stereo position for the sound effect. port 0 is the sound
; effect number, port 1 is the relative stereo position
; (-100 to 100, -100 is right, 0
; is center, 100 is left). The stereo position is added to the original
; position as set by the sound designer.
;
; Coding example:
;
;	ldy	#crash_eff + (50 << 8)	; port 1 is stereo position
;	lda	#DCOM_START_STEREO_EFFECT
;	jsr	Pass_Driver_Com

DCOM_START_STEREO_EFFECT = 38





; This command is used to scale the master volume of songs playing. It is
; only available through DCOM (Wolfgang cannot change this). You may use
; this previous to playing a song to scale down the volume or during a
; song if you want to turn down or up the volume of the song playing for
; some reason. When the driver is initialized, the master volume is set
; to 255 as a default (full volume).
;
; Note: This volume will stick for the current song and for future songs
; played until the driver is reinitialized or you use this command again.
;
; Port 0 is the volume scale (0 to 255). 255 is full volume, 128 is half
; volume.
;
; Coding example:
;
;	ldy	#128			; set to half volume
;	lda	#DCOM_SET_MASTER_SONG_VOLUME
;	jsr	Pass_Driver_Com

DCOM_SET_MASTER_SONG_VOLUME = 40
DCOM_SONG_VOLUME = DCOM_SET_MASTER_SONG_VOLUME


; This command is used to change the hardware TEMPO clock used by the driver
; to time music tempo. It is only available through DCOM (Wolfgang cannot
; change this). You may use this previous to playing a song to the overall
; tempo or during a song if you want to speed up or slow down the tempo of
; the song playing. When the driver is initialized, the tempo clock is set
; to 160 as a default.
;
; Note: This tempo will stick for the current song and for future songs
; played until the driver is reinitialized or you use this command again.
;
; Port 0 is the tempo (0 to 255). 160 is normal tempo, 80 is half tempo.
;
; Coding example:
;
;	ldy	#160			; set to normal tempo
;	lda	#DCOM_SET_MASTER_SONG_TEMPO
;	jsr	Pass_Driver_Com

DCOM_SET_MASTER_SONG_TEMPO = 42
DCOM_TEMPO = DCOM_SET_MASTER_SONG_TEMPO


; This command will cause a song to be played at a higher or lower key than
; it was originally written. It may be set before a song is played or while
; a song is being played. For instance, to have a song play 1 semitone above
; the original key, set this to 20. To have it played 1 semitone below, set
; this at -20. This command can only be set through DCOM.
;
; Note: This offset will stick for the current song and for future songs
; played until the driver is reinitilized or you use this command again.
;
; Warning: Overflows are NOT checked here, so don't make wild requests like
; playing 18 octaves above the original key!
;
; Ports 0 and 1 are signed offset in microtones, with 0 being normal 
; transpose. Port 0 is low byte, port 1 is high byte.
;
; Coding example:
;
;	ldy	#20			; one semitone above original key
;	lda	#DCOM_SET_MASTER_SONG_TRANSPOSE
;	jsr	Pass_Driver_Com

DCOM_SET_MASTER_SONG_TRANSPOSE = 44
DCOM_TRANSPOSE = DCOM_SET_MASTER_SONG_TRANSPOSE



; This command is used to scale the master volume of effects playing. It is
; only available through DCOM (Wolfgang cannot change this). You should set
; this command before playing any effects as this affects all effects
; volumes. When the driver is initialized, the master volume is set to 255
; as a default (full volume).
;
; Note: This volume will stick for all effects until the driver is
; reinitialized or you use this command again.
;
; Port 0 is the volume scale (0 to 255). 255 is full volume, 128 is half
; volume.
;
; Coding example:
;
;	ldy	#128			; set to half volume
;	lda	#DCOM_SET_MASTER_EFFECTS_VOLUME
;	jsr	Pass_Driver_Com

DCOM_SET_MASTER_EFFECTS_VOLUME = 46
DCOM_EFFECT_VOLUME = DCOM_SET_MASTER_EFFECTS_VOLUME



; Set pitch of next effect started with DCOM_START_EFFECT. Pitch is specified
; as a relative value from the sound effect's default pitch. Ports 0 and 1
; hold the signed offset value (port 0 is low byte, port 1 is high byte).
; This value is added to the sound effect's default pitch. Use this to start
; an effect at a higher or lower pitch than normal. Offset value is in
; in microtones (20 microtones per semitone, 240 microtones per octave)
;
; Note: Overflow is not checked.
;
; Coding example:
;
;	ldy	#-20			; one semitone below default pitch
;	lda	#DCOM_SET_NEXT_EFFECT_PITCH
;	jsr	Pass_Driver_Com
;	ldy	#crash_eff		; this is the affected sound effect
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com

DCOM_SET_NEXT_EFFECT_PITCH = 48
DCOM_NEXT_EPITCH = DCOM_SET_NEXT_EFFECT_PITCH



; This command will preset the volume for the next effect started with
; DCOM_START_EFFECT. Port 0 contains the volume value (0 to 255). 255 is
; full volume, 128 is half volume.
;
; Coding example:
;
;	ldy	#128			; play at half volume
;	lda	#DCOM_SET_NEXT_EFFECT_VOL
;	jsr	Pass_Driver_Com
;	ldy	#crash_eff		; this is the affected sound effect
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com

DCOM_SET_NEXT_EFFECT_VOL = 52
DCOM_NEXTE_VOL = DCOM_SET_NEXT_EFFECT_VOL



; This command can be used to preset the offset stereo pan for the next
; effect to be started with DCOM_START_EFFECT. Pan value is in port 0
; (-100 to 100). This value is added directly to the sound effect's
; default stereo pan position. If the pan position is overflowed, the
; actual effect pan is set to minumin or maximum depending on the
; direction of overflow.
;
; Coding example:
;
;	ldy	#-100			; pan full right
;	lda	#DCOM_SET_NEXT_EFFECT_PAN
;	jsr	Pass_Driver_Com
;	ldy	#crash_eff		; this is the affected sound effect
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com

DCOM_SET_NEXT_EFFECT_PAN = 60
DCOM_PAN = DCOM_SET_NEXT_EFFECT_PAN



;***********************************************************************
; Parameterized effect commands. This section should be deleted for client
; release until parameterized effects are stable.
;***********************************************************************

; This command sets an ID number for the next effect started with
; DCOM_START_EFFECT. This ID number is recorded and can be used later to
; change the effect's parameters (e.g. pitch, volume, etc.) at a later time.
;
; NOTE: There is nothing wrong with setting the same ID for several different
; effects.
; This will allow you to modulate or stop (using STOP_CONT_EFFECT) several
; sounds at the same time.
; Port 0 is ID number (1 to 127).
;
; Coding example:
;
;	ldy	#1			; ID number
;	lda	#DCOM_SET_EFFECT_ID
;	jsr	Pass_Driver_Com
;	ldy	#crash_eff		; this sound effect gets ID = 1
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com

DCOM_SET_EFFECT_ID = 50
DCOM_ID = DCOM_SET_EFFECT_ID



; This command sets the internal registers RP0 and RP1, which can be used
; for parameterized effect commands that require more than 16 bits of
; data. This command is used BEFORE the over-16-bits command. Port 0 is
; stored to RP0, port 1 is stored to RP1.
;
; Coding example:
;
;	ldy	#16			; RP0 to 16, RP1 to 0
;	lda	#DCOM_SET_PARM_RP01
;	jsr	Pass_Driver_Com

DCOM_SET_PARM_RP01 = 56
DCOM_PARM = DCOM_SET_PARM_RP01



; This command controls on-the-fly changes of a sound effect's volume. The
; sound effect must already have been assigned an ID number (with
; DCOM_SET_EFFECT_ID) and started with DCOM_START_EFFECT. This command
; allows the programmer to change the sound effect's volume as it plays.
;
; Port 0 holds the effect ID number, port 1 holds the new volume (0 to 255).
; 255 is full volume, 128 is half volume.
;
; Coding example: start a sound effect at volume 200, then turn it down
; to 150.
;
;	ldy	#128			; play next effect at half volume
;	lda	#DCOM_SET_NEXT_EFFECT_VOL
;	jsr	Pass_Driver_Com
;	ldy	#1			; set next effect as ID number 1
;	lda	#DCOM_SET_EFFECT_ID
;	jsr	Pass_Driver_Com
;	ldy	#crash_eff		; start the effect
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com
;	ldy	#1 + (150 << 8)		; port 0 is ID, port 1 is volume
;	lda	#DCOM_SET_EFFECT_VOL
;	jsr	Pass_Driver_Com

DCOM_SET_EFFECT_VOL = 54
DCOM_ID_VOL = DCOM_SET_EFFECT_VOL



; This command controls on-the-fly changes of a sound effect's pitch. The
; sound effect must already have been assigned an ID number (with
; DCOM_SET_EFFECT_ID) and started with DCOM_START_EFFECT. This command
; allows the programmer to change the sound effect's pitch as it plays.
; Since the pitch offset value is 16 bits, you will have to use the
; DCOM_SET_PARM_RP01 command in conjunction with this command.
;
; Ports 0 and 1 contain pitch offset value for DCOM_SET_PARM_RP01 command.
; Port 0 is low byte, port 1 is high byte. Pitch offset value is signed
; offset in microtones from the sound effect's default pitch (20 microtones
; to a semitone).
;
; Port 0 contains the sound effect ID number for DCOM_SET_EFFECT_PITCH
; command.
;
; Coding example: start a sound effect at tuning -240, then retune it to
; to 160.
;
;	ldy	#-240			; play next effect at different pitch
;	lda	#DCOM_SET_NEXT_EFFECT_PITCH
;	jsr	Pass_Driver_Com
;	ldy	#1			; set next effect as ID number 1
;	lda	#DCOM_SET_EFFECT_ID
;	jsr	Pass_Driver_Com
;	ldy	#crash_eff		; start the effect
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com
;	ldy	#160			; new pitch offset value
;	lda	#DCOM_SET_PARM_RP01
;	jsr	Pass_Driver_Com
;	ldy	#1			; port 0 is ID
;	lda	#DCOM_SET_EFFECT_VOL
;	jsr	Pass_Driver_Com

DCOM_SET_EFFECT_PITCH = 58
DCOM_ID_PITCH = DCOM_SET_EFFECT_PITCH



; This command controls on-the-fly changes of a sound effect's stereo
; position. The sound effect must already have been assigned an ID number
; (with DCOM_SET_EFFECT_ID) and started with DCOM_START_EFFECT. This command
; allows the programmer to change the sound effect's stereo position as it
; plays.
;
; Port 0 holds the effect ID number, port 1 holds the new stereo position
; (-100 to 100). The passed value is added to the sound effect's default
; stereo position. Overflow is checked and corrected.
;
; Coding example: assuming a sound effect's default position is 50 (center),
; start it at stereo position -10 (right of center), then pan it to stereo
; position 25 (left of center)
;
;	ldy	#-10			; adjust next effect's stereo position
;	lda	#DCOM_SET_NEXT_EFFECT_PAN
;	jsr	Pass_Driver_Com
;	ldy	#1			; set next effect as ID number 1
;	lda	#DCOM_SET_EFFECT_ID
;	jsr	Pass_Driver_Com
;	ldy	#crash_eff		; start the effect
;	lda	#DCOM_START_EFFECT
;	jsr	Pass_Driver_Com
;	ldy	#1 + (25 << 8)		; port 0 is ID, port 1 is pan
;	lda	#DCOM_SET_EFFECT_PAN
;	jsr	Pass_Driver_Com

DCOM_SET_EFFECT_PAN = 62
DCOM_ID_PAN = DCOM_SET_EFFECT_PAN

;***********************************************************************



; This command sets the echo spec for the next effect to be played. Port 0
; contains the number of the new echo spec (0 to 254). Echo spec number
; 255 is the default echo spec (set by the sound engineer using Wolfgang).
; The echo spec is used for both sound effects and songs. It is only
; cleared on reset.

DCOM_SET_NEXT_EFFECT_ECHO = 64
DCOM_ID_ECHO = DCOM_SET_NEXT_EFFECT_ECHO

; this command mutes a track in the current playing song. Port 0 contains the
; number of the track to be muted. Track numbers should be derived from
; the WOLFGANG .asm file. This track remains muted until an UNMUTE command is
; sent.

DCOM_MUTE_SONG_TRACK       = 66


; this command unmutes a track in the current playing song. Port 0 contains the
; number of the track to be muted. Track numbers should be derived from
; the WOLFGANG .asm file.

DCOM_UNMUTE_SONG_TRACK     = 68

; this command forces song to either reset forced muting on song loop or freezes
; it even when song starts. This flag sticks so it must be set or reset before a
; song plays if it is used at all.  It is only RESET on module loads.
; NED - MAYBE you can come up with a better
; explanation????  Port zero contains 0 for allowing songs to always come
; up with tracks unmuted when the track starts over. Port zero contains
; non-zero to freeze track muting status on start of song playing. This
; is true even if it is the first time the song starts playing.

DCOM_FREEZE_TRACK_MUTING     = 70


; cold reset the driver - like a DORELOAD but without loading module

DCOM_RESET      =   72

; change effect priority ordering between steal first stealable channel
; and best channel allocation. ; ****** ELABORATE

; PORT 0 = 0 STEAL MODE  (default)
; PORT 0 = 1 BEST  MODE


DCOM_ALLOCATION_MODE  = 74



;


;

DCOM_SETWAVEASBLAST =   76
;   sets wavenumber in PORT 0 to address set in last DCOM_BLAST_ADDR.
;   The next time the waveform is played as part of an effect etc. it
;   will assume all info will start at DCOM_BLAST_ADDR. Actual waveform
;   start and looppoint are calculated as offsets from the last DCOM_BLAST_ADDR
;   and placed into wave table. RANDY - THIS IS YOUR COMMAND
;
;
;
;   RANDY - TO USE THIS COMMAND,
; after each module load or each time blast buffer for a wave is changed
;
;
;   1. Use DCOM_BLAST_ADDR  to set blast address
;
;   2. Use DCOM_SETWAVEASBLAST with waveform numbers (EFFECTNAME_eff_BWV in
;       wolfgang .asm files) to set each waveform to the blast address.
;   It is not necessary to blast after this if not needed. You may use
;   repeat this sequence if necessary to pre-initialize the wavetable if
;   desired. SETWAVEASBLAST is a high-priority command and will be processed
;  immediately by the sound driver.
;
; This sequence will also cause internal start and loop offsets to be calculated
; internally. (Hairy on your end, Randy, but easy on mine).
    



DCOM_MAX_COMMAND = 76

; The highest command number that music driver will accept:


