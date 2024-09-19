rom_soft_start	equ	$d00000			; beginning of ROM in soft here mode
rom_soft_end	equ	rom_soft_start+$100000-$4000

ram_soft_start	equ	$e00000
ram_soft_end	equ	ram_soft_start+$10000

Use_Fixed_Internals	equ	1		; as opposed to the Fred at the end of ROM
