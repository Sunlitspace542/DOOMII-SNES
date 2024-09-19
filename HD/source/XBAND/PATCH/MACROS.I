;
; Macros that affect 8/16 bit memory, index registers.
;


a16			macro
			longa	on
			rep		#$20
			endm

a8			macro
			longa	off
			sep		#$20
			endm

i16			macro
			longi	on
			rep		#$10
			endm

i8			macro
			longi	off
			sep		#$10
			endm

ai16		macro
			longa	on
			longi	on
			rep		#$30
			endm

ai8			macro
			longa	off
			longi	off
			sep		#$30
			endm

;
; Macro for Psy-Q "halt" instruction.
;

halt		macro
			cop		$3
			endm

;
; Macro for Psy-Q "pollhost" instruction.
;

pollhost	macro	
			cop		$0
			endm

