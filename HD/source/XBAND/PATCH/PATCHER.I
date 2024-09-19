***********************************************
*
* Patching macros, for use with Fred and GamePatch.s
*
***********************************************

; patch a word in memory with the specified word

FastPatch		macro	;&number,&address,&word,&router ; REQUIRES bank to be set		
				lda	#\3
				sta	|Patch_Substitution+\1+\1
				lda	#(\2>>1)&$ffff
				sta	>mdFred_Soft+$00*2+1+8*\1
				lda	#\2>>9
				sta	>mdFred_Soft+$00*2+1+2+8*\1
				lda	#(\2+2)&$ffff
				sta	|brk_router+\1*5
				lda	#^\2
				sta	|brk_router+2+\1*5
				lda	#\4-1
				sta	|brk_router+3+\1*5
				endm

; put a breakpoint in memory with the specified word

Patch			macro	;&number,&address,&router
@parity\@		equ	(\2)&1
@word_address\@	equ	((\2)-@parity\@)
				lda	>@word_address\@
				if	@parity\@ then	; odd address
				and	#$ff
				else
				and	#$ff00
				endif
				sta	|Patch_Substitution+\1+\1
				lda	#(\2>>1)&$ffff
				sta	>mdFred_Soft+$00*2+1+8*\1
				lda	#\2>>9
				sta	>mdFred_Soft+$00*2+1+2+8*\1
				lda	#(\2+2)&$ffff
				sta	|brk_router+\1*5
				lda	#^\2
				sta	|brk_router+2+\1*5
				lda	#(\3-1)&$ffff
				sta	|brk_router+3+\1*5
				endm

; put a word in memory to anything, with no break router update

WordPatch		macro	;&number,&address,&word ; REQUIRES bank to be set
				lda	#\3
				sta	|Patch_Substitution+\1+\1
				lda	#(\2>>1)&$ffff
				sta	>mdFred_Soft+$00*2+1+8*\1
				lda	#\2>>9
				sta	>mdFred_Soft+$00*2+1+2+8*\1
				endm
			

Turn_Off_Patch	macro
@mask			set	0
				rept	narg
@mask			set	@mask+(1<<\1)
				shift
				endr
				lda	|shadow_mdHitEnables
				and	#(-1^@mask)
				sta	|shadow_mdHitEnables
				sta	>mdFred_Soft+$6c*2+1
				endm

Turn_On_Patch	macro
@mask			set	0
				rept	narg
@mask			set	@mask+(1<<\1)
				shift
				endr
				lda	|shadow_mdHitEnables
				ora	#@mask
				sta	|shadow_mdHitEnables
				sta	>mdFred_Soft+$6c*2+1
				endm
