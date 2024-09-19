Debug_Port_A	equ	1			; do we want any debug information sent out port A? (57600, 8N1)

Comm_Debug		equ	1			; do we want Communications debug information sent to port A?

Final_Build		equ	0			; set this to "1" when doing the final ROM image compile
								; note: alternately, you could just make _XBInit return a 0
								; if you're scare about the code moving and causing other bugs

Modem_Comm		equ	1			; are we using the modem to communicate? (1 = yes)

Game_Patch		equ	1			; is this a game patch? (If so, use jsr's, only include comm code)

