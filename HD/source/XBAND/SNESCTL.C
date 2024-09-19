/*
	File:		SNESControls.c

	Contains:	Controller read code for SNES

	Written by:	Chris Yerga

	Copyright:	c 1994 by Catapult Entertainment, Inc., all rights reserved.

	Change History (most recent first):

		<20>	  4/9/95	JOE		Support for new hardware keyboard scancodes that emulate SNES
									joypad buttons.
		<19>	  4/7/95	JOE		Don't try to build 65816 code on the stim.
		<18>	  4/7/95	JOE		Bit timing for hardware keyboard now comes from a DBConstant
		<17>	  4/7/95	JOE		Still more Hardware keyboard junk.
		<16>	  4/6/95	JOE		More Hardware keyboard junk
		<15>	  4/4/95	JOE		Get the HW keyboard mapping table via the symbolic ID now in
									Tables.h.
		<14>	  4/4/95	JOE		Added more hardware keyboard support.
		<13>	  4/3/95	JOE		More hardware keyboard support
		<12>	 2/25/95	SAH		Implemented dummy handlers for the hardware keyboard.
		<11>	 2/21/95	JOE		tripovbl needs hack.h
		<10>	 2/21/95	JOE		give tripotron a chance at VBL
		 <9>	  2/3/95	JOE		Changed SegaSound.h to Sound.h
		 <8>	  2/1/95	KON		Add controlMgr, rename old controlMgr to controllerManager.
		 <7>	 1/24/95	JBH		Use real globals on SNES.
		 <6>	  1/1/95	JBH		short -> unsigned short. Added simulator support for all SNES
									buttons.
		 <5>	12/22/94	CMY		Add stubs for some hardware keyboard functions.
		 <4>	12/11/94	CMY		Fixed an infinite-looping bug in the SNES box version of the
									controller read routine.
		 <3>	 12/6/94	CMY		Quote the segment name.
		 <2>	 12/4/94	CMY		Don't read the controls while the SNES hardware is accessing
									them.
		 <1>	11/21/94	CMY		first checked in

	To Do:
*/


#include "CompilerPrefix.h"

SEGMENT("Controls")

#include "Dispatcher.h"
#include "BoxGlobals.h"
#include "SNESControls.h"
#include "SNESControlsPriv.h"
#include "SegaIn.h"
#include "Sound.h"
#include "Time.h"
#include "Errors.h"
#include "DBTypes.h"
#include "DBConstants.h"
#include "BoxSer.h"
#include "Hack.h"
#include "Tables.h"

#ifdef SIMULATOR
#include <Events.h>
#endif

#if !defined(DONT_ALLOCATE_GLOBAL_SPACE)
ControlGlobals controls;
#endif


ControllerType	_InitControllers( void );
unsigned short	_ReadHardwareController ( short whichOne );
void			_ReadAllControllers ( ControllerType type, short* results, short flags );
void			_ControllerVBL( void );

Boolean			_FlushHardwareKeyboardBuffer( void );
unsigned char	_GetNextHardwareKeyboardChar( void );
long			_GetHardwareKeyboardFlags( void );
void			_SetHardwareKeyboardFlags( long flags );
unsigned char	_GetNextESKeyboardRawcode( void );
unsigned char	_GetNextESKeyboardStatus( void );
unsigned char	_GetNextESKeyboardChar( void );
void			_SendCmdToESKeyboard( unsigned char *cmdBuf, unsigned char cmdLen );

void			_ReadESKeyboard ( void );
void			_WriteESKeyboard ( void );
Boolean			_FindESKeyboard ( void );
void 			_ResetHardwareKeyboard( void );
void			_EmulateJoypadWithKeyboard( short *results );

void 			BackUpKeycodeTail( void );		/* support routine, only used by GetNextESKeyboardChar() */

long
_ControllerControl ( short command, long data )
{
ControlGlobals *globals;
long offset;
short error;
long theID;
long bitdelay;

	error = commandSelectorUnknown;

	switch ( command )
		{
		case kHardInitialize:
		
#ifdef USE_GLOBAL_PTR
			GETMGRGLOBALSOFFSET(controls,offset);

			error = AllocateGlobalSpace ( kControllerManager, offset,
				sizeof(ControlGlobals), (Ptr *) &globals );
			
			ASSERT_MESG ( !error, "Can't create controls globals" );
			if ( error != noErr )
				{
				return error;
				}
#else
			error = noErr;
#endif

			/* install our selectors */
			SetDispatchedFunction ( kInitControllers,			kControllerManager,	_InitControllers );
			SetDispatchedFunction ( kReadHardwareController,	kControllerManager,	_ReadHardwareController );
			SetDispatchedFunction ( kControllerVBL,				kControllerManager,	_ControllerVBL );
			SetDispatchedFunction ( kReadAllControllers,		kControllerManager,	_ReadAllControllers );
			SetDispatchedFunction ( kFlushHardwareKeyboardBuffer,	kControllerManager,	_FlushHardwareKeyboardBuffer );
			SetDispatchedFunction ( kGetHardwareKeyboardFlags,	kControllerManager,	_GetHardwareKeyboardFlags );
			SetDispatchedFunction ( kSetHardwareKeyboardFlags,	kControllerManager,	_SetHardwareKeyboardFlags );
			SetDispatchedFunction ( kGetNextHardwareKeyboardChar,	kControllerManager,	_GetNextHardwareKeyboardChar );
			SetDispatchedFunction ( kGetNextESKeyboardRawcode,	kControllerManager,	_GetNextESKeyboardRawcode );
			SetDispatchedFunction ( kGetNextESKeyboardStatus,	kControllerManager,	_GetNextESKeyboardStatus );
			SetDispatchedFunction ( kGetNextESKeyboardChar,		kControllerManager,	_GetNextESKeyboardChar );
			SetDispatchedFunction ( kSendCmdToESKeyboard,		kControllerManager,	_SendCmdToESKeyboard );
			SetDispatchedFunction ( kReadESKeyboard,			kControllerManager,	_ReadESKeyboard );
			SetDispatchedFunction ( kWriteESKeyboard,			kControllerManager,	_WriteESKeyboard );
			SetDispatchedFunction ( kFindESKeyboard,			kControllerManager,	_FindESKeyboard );
			SetDispatchedFunction ( kResetHardwareKeyboard,		kControllerManager,	_ResetHardwareKeyboard );
			SetDispatchedFunction ( kEmulateJoypadWithKeyboard,	kControllerManager,	_EmulateJoypadWithKeyboard );
			break;
		
		case kSoftInialize:
			REFGLOBAL( controls, controlValues[0] ) = 0;
			REFGLOBAL( controls, controlValues[1] ) = 0;
			REFGLOBAL( controls, samplePhase ) = 0;

			REFGLOBAL( controls, keyboardPresent ) = false;
			REFGLOBAL( controls, sysKeysHead ) = 0;
			REFGLOBAL( controls, sysKeysTail ) = 0;
			REFGLOBAL( controls, keycodeHead ) = 0;
			REFGLOBAL( controls, keycodeTail ) = 0;
			REFGLOBAL( controls, statusHead ) = 0;
			REFGLOBAL( controls, statusTail ) = 0;
			REFGLOBAL( controls, cmdHead ) = 0;
			REFGLOBAL( controls, cmdTail ) = 0;

			REFGLOBAL( controls, keyMapTable ) = (KeyMappingTable *)DBGetItem( kTable1ByteType, kHWKeyboardMapID );
			REFGLOBAL( controls, keyboardFlags ) = 0L;

			DBGetConstant( kHardwareKeyboardIDConst, &theID );
			REFGLOBAL( controls, keyboardID ) = theID;				/* byte-sized ID */

			DBGetConstant( kHWKeyboardBitTimeConst, &bitdelay );	
			REFGLOBAL( controls, HWKeyboardBitDelay ) = bitdelay;	/* byte-sized bit delay */

			ResetHardwareKeyboard();						/* in case we have one, be sure it's happy */
			
			error = noErr;
			break;
		
		case kHardClose:
			error = noErr;
			break;
		
		case kSoftClose:
			error = noErr;
			break;
					
		case kCodeBlockMoved:
			WARNING_MESG ( "controller code block moved" );
			error = noErr;
			break;
					
		case kGlobalsMoved:
			WARNING_MESG ( "controller global block moved");
			error = noErr;
			break;
		}
		
	return error;
}


#ifdef SIMULATOR

/* isPressed( scanCode )
 *
 * Checks to see if a given key is pressed on the Mac. Used by the SIMULATOR
 * to emulate the Sega joypad
 */
static short isPressed( unsigned short k )
{
	unsigned char km[16];

	GetKeys( (long *) km);
	if ( ( km[k>>3] >> (k & 7) ) & 1) {
		return 1;
	} else {
		return 0;
	}
}

#endif


void	_ControllerVBL( void )
{
register long	newPhase;
unsigned short	controller1;
unsigned short	controller2;

	// to avoid sucking down too much CPU, we do a few things based on
	// a phase counter.  We read the controls 15 times per second (instead
	// of 60)
	
	newPhase = REFGLOBAL( controls, samplePhase ) + 1;
	
	if ((newPhase & 3) == 0)							// 4 VBLs passed? 
	{
#ifdef SIMULATOR
		Boolean isOption = false;
		
		if( isPressed( optionKey) )
			isOption = true;
				
		controller1 = 0;
		controller2 = 0;
		
		if( isPressed( leftArrow ) )
			controller1 = kLEFT;
		else
		if( isPressed( rightArrow ) )
			controller1 = kRIGHT;
		else
		if( isPressed( upArrow ) )
			controller1 = kUP;
		else
		if( isPressed( downArrow ) )
			controller1 = kDOWN;
		else
		if( isPressed( returnKey ) )
			controller1 = kButtonPressed;				// enter presses are all-button presses
		else											// need "option" to not screw up email kbd
		if( isOption && isPressed( letterBKey ) )
			controller1 = kButtonB;
		else
		if( isOption && isPressed( letterYKey ) )
			controller1 = kButtonY;
		else
		if ( isOption && isPressed( letterSKey ) && isPressed( macControlKey ) )
			controller1 = kButtonSelect;
		else
		if( isOption && isPressed( letterSKey ) )
			controller1 = kButtonStart;
		else
		if( isOption && isPressed( letterAKey ) )
			controller1 = kButtonA;
		else
		if( isOption && isPressed( letterXKey ) )
			controller1 = kButtonX;
		else
		if( isOption && isPressed( letterLKey ) )
			controller1 = kButtonL;
		else
		if( isOption && isPressed( letterRKey ) )
			controller1 = kButtonR;
#else

		TripOVBL();										// only does anything if tripotron is enabled
		
		asm
		{
wait:		LDA >0x004212		;See if the SNES is reading the controllers
			BIT	#1
			BNE	wait			;Wait for it to finish
			
			LDA	>0x004218
			STA	controller1
			
			LDA >0x00421A
			STA	controller2
		};
		
		
#endif

		REFGLOBAL( controls, controlValues[0] ) = controller1;
		REFGLOBAL( controls, controlValues[1] ) = controller2;


		if ( REFGLOBAL( controls, keyboardPresent ) )
		{
			ReadESKeyboard();
			EmulateJoypadWithKeyboard( (short *)(REFGLOBAL( controls, controlValues )) );
		}
	}

	
	if ( (newPhase & 0x1f) == 0 )								// check for keyboard every 128 VBLs (2.13 secs)
	{
		if ( FindESKeyboard() )									// do we have a hardware keyboard?
		{
			if (!REFGLOBAL( controls, keyboardPresent ))		// do we already know it?
			{
				REFGLOBAL( controls, keyboardPresent ) = true;
				REFGLOBAL( controls, keyboardFlags ) = 0L;
			}	
		}
		else
			REFGLOBAL( controls, keyboardPresent ) = false;
	}

	
	REFGLOBAL( controls, samplePhase ) = newPhase;
}


unsigned short
_ReadHardwareController ( short whichOne )
{
	return REFGLOBAL( controls, controlValues[whichOne-1] );

}


//
//----- First, the external interfaces
//

ControllerType
_InitControllers( void )
{
	return (ControllerType) nil;
}

void
_ReadAllControllers( ControllerType type, short* results, short flags )
{
}




/* ----------------------------------------------
 *	Hardware Keyboard Support
 *
 */

/*
	See if there is anything waiting for us from the hardware keyboard.
	If there is, read it into the keyCodeBuf.
	
	Data is read from the keyboard 2 bits at a time using the manual controller
	read registers.  The automatic read is too fast for the keyboard micro to
	reliably keep up.
	
	The keyboard only responds when PP7 is low.  Dropping PP7 activates the 
	keyboard's host communication interrupt routine.
	
	Once PP7 is dropped:
		The first 8 bits (4 clocks) contain the keyboard's signature.
		The next 4 bits (2 clocks) tell us how many bytes are waiting (0-15).
		If any data is waiting, the next bits are that data.
		
	A keyboard ID-only transaction can be accomplished by raising PP7 after reading
	the first dibit.
	
	The Caps Lock LED is controlled via the state of PP7 between the 1st and 2nd
	dibit of the "chars waiting" value (the 2 clocks after the 4 clock ID read):
	
							PP7 High --> Caps Lock OFF
							PP7 LOW  --> Caps Lock ON
	
	Terminating the transaction at any other time (an ABORT) may cause waiting
	data to be lost and/or the keyboard to get really confused.
 */
 
void _ReadESKeyboard ( void )
{
#if defined(SNES) && !defined(SIMULATOR)
unsigned char *bufPtr;
unsigned char offset;
unsigned char dataByte;
unsigned char readID;
unsigned char capsLock;
unsigned char	bitdelay;

	bitdelay = REFGLOBAL( controls, HWKeyboardBitDelay );

	capsLock = (REFGLOBAL( controls, keyboardFlags )) & kCapsLocked;

	bufPtr = REFGLOBAL( controls, keycodeBuf );
	offset = REFGLOBAL( controls, keycodeHead );
	
	asm
	{
		sep     #0x30			; A,X,Y is 8-bit

		lda		#0x7f
		sta		>0x004201		; set PP7 LOW

		
		;
		; First read the ID
		;

		ldx		#4
readIDLoop:	
		lda		bitdelay
t1:		dec		a
		bne		t1
		
		lda     >0x004017		; read the pad data lines, d0 & d1 hold the goods		5 -> actual read @ 1.79MHz
		lsr     a				; d0 into carry											2
		ror     readID			;  store it												5
		lsr     a				; d1 into carry											2
		ror     readID			;  store it 											5

		dex
		bne		readIDLoop

								; should we verify the ID is still valid?
		
		lda		capsLock		; we can turn the caps lock LED off by raising PP7 here
		bne		turnCapsOn
		lda		#0xff
		sta		>0x004201		; set PP7 HIGH
turnCapsOn:
		
		;
		; Now read the number of bytes waiting
		;
		
		ldx		#2
readWaitingLoop:	
		lda		bitdelay
t2:		dec		a
		bne		t2
		
		lda     >0x004017		; read the pad data lines, d0 & d1 hold the goods		5 -> actual read @ 1.79MHz
		lsr     a				; d0 into carry											2
		ror     dataByte		;  store it												5
		lsr     a				; d1 into carry											2
		ror     dataByte		;  store it 											5

		dex
		bne		readWaitingLoop

		lda		dataByte
		ror		a
		ror		a
		ror		a
		ror		a
		eor		#0xff			; invert it
		and		#0x0f			; goes 0->15

		tax						; save byte count		
		beq		readExit
		
		;
		; And finally read the actual data
		;
		
		ldy		offset
		
readLoop:
		phx
		
		ldx		#4				; 4 dibits
readByteLoop:
		lda		bitdelay
t3:		dec		a
		bne		t3
		
		lda     >0x004017		; read the pad data lines, d0 & d1 hold the goods		5 -> actual read @ 1.79MHz
		lsr     a				; d0 into carry											2
		ror     dataByte		;  store it												5
		lsr     a				; d1 into carry											2
		ror     dataByte		;  store it 											5

		dex
		bne		readByteLoop
		
		lda		dataByte
		eor		#0xff			; invert it

		sta		[bufPtr],Y		; and store it
		iny
		tya
		and		#0x0F			; circular buf
		tay

		plx						; recover count
		dex
		bne		readLoop
		
		sty		offset
		
readExit:	

		;
		; Now raise PP7 to end the transaction
		;

		lda		#0xff
		sta		>0x004201		; set PP7 HIGH

		rep		#0x30			; 16-bit A,X,Y
	}
	
	REFGLOBAL( controls, keycodeHead ) = offset;
#endif
}



/*
	Drop PP7, do 4 clocks.  Raise PP7 after the 1st clock to let the keybd know this is just a Find, not a read read.
 */
 
Boolean _FindESKeyboard ( void )
{
#if defined(SNES) && !defined(SIMULATOR)
unsigned char theID;
unsigned char dataByte;
unsigned char clk;
Boolean retVal;
unsigned char	bitdelay;

	bitdelay = REFGLOBAL( controls, HWKeyboardBitDelay );
	
	theID = REFGLOBAL( controls, keyboardID);	

	asm
	{
		sep     #0x20			; A is 8-bit

		lda		#0x7f
		sta		>0x004201		; set PP7 LOW

		lda		bitdelay
t1:		dec		a
		bne		t1
		
		lda     >0x004017		; read the pad data lines, d0 & d1 hold the goods		5 -> actual read @ 1.79MHz
		lsr     a				; d0 into carry											2
		ror     dataByte		;  store it												5
		lsr     a				; d1 into carry											2
		ror     dataByte		;  store it 											5


		lda		#0xff
		sta		>0x004201		; this is just a Find, so set PP7 HIGH


		ldx		#3
readIDLoop:	
		lda		bitdelay
t2:		dec		a
		bne		t2
		
		lda     >0x004017		; read the pad data lines, d0 & d1 hold the goods		5 -> actual read @ 1.79MHz
		lsr     a				; d0 into carry											2
		ror     dataByte		;  store it												5
		lsr     a				; d1 into carry											2
		ror     dataByte		;  store it 											5

		dex
		bne		readIDLoop


		stz		retVal			; assume we didn't find one

		lda		dataByte
		eor		#0xFF			; invert it
		cmp		theID
		bne		done
		
		lda		#0x01			; yay, match, we have one!
		sta		retVal
done:		
		rep		#0x20			; 16-bit A
	}
	
	return ( retVal );
#else
	return ( false );
#endif
}


/*
	Clock the keyboard a bunch of times at startup to be sure that it is in a happy state.
	If someone (other than the OS, while we were away) started something that looked to the
	keyboard like a transaction, the keyboard could be waiting for the transaction to complete.
	By clocking the keyboard a bunch of times, we can ensure that its transaction state machine
	is in the "waiting for transaction start" state.
	
	The most number of clocks a single transaction should take is:
	
			4 clks/ID + 2 clks/waitCnt + (4 clks/byte * 15 bytes) = 66 clks
			
	Just to be paranoid, we'll clock it 75 times.  At 18us/clk, this will take 1.35ms.  Sorry
	about lengthening the boot time...:-)
 */
 
void _ResetHardwareKeyboard( void )
{
#if defined(SNES) && !defined(SIMULATOR)
unsigned char	bitdelay;

	bitdelay = REFGLOBAL( controls, HWKeyboardBitDelay );
	
	asm
	{
		sep		#0x20			; 8-bit A
		
		ldx		#75				; do it 75 times
		
resetLoop:
		lda     >0x004017		; read the pad data lines (clock it), d0 & d1 hold the goods
	
		lda		bitdelay
t1:		dec		a
		bne		t1
		
		dex
		bne		resetLoop
		
		rep		#0x20			; 16-bit A
	}
#endif
}



/*
	If there are bytes in the scancode buffer, turn them into bytes in the sysKeys buffer.
	
	THIS SUCKS.  Rework for SNES.
 */
 
void _EmulateJoypadWithKeyboard( short *results )
{
#if defined(SNES) && !defined(SIMULATOR)
unsigned char key;
unsigned char head;
register short pad1 = results[0];

	do	
	{
		key = GetNextESKeyboardChar();		/* try to take scancode(s) from scancodes buf & turn	*/
											/*  them into real key values (ASCII + a few specials)	*/
		if ( key != kNoKey )
		{
			switch( key )
			{
			case kUpArrowKey:
						if ( !(pad1 & kDOWN ) )
							pad1 |= kUP;
						break;
						
			case kDownArrowKey:
						if ( !(pad1 & kUP ) )
							pad1 |= kDOWN;
						break;
						
			case kLeftArrowKey:
						if ( !(pad1 & kRIGHT ) )
							pad1 |= kLEFT;
						break;
						
			case kRightArrowKey:
						if ( !(pad1 & kLEFT ) )
							pad1 |= kRIGHT;
						break;
						
			case kEnterKey:
						pad1 |= kButtonA;
						break;
						
			case kAButtonKey:
						pad1 |= kButtonA;
						break;
																
			case kBButtonKey:
						pad1 |= kButtonB;
						break;
																
			case kXButtonKey:
						pad1 |= kButtonX;
						break;
																
			case kYButtonKey:
						pad1 |= kButtonY;
						break;
																
			case kLButtonKey:
						pad1 |= kButtonL;
						break;
																
			case kRButtonKey:
						pad1 |= kButtonR;
						break;
																
			case kSelButtonKey:
						pad1 |= kButtonSelect;
						break;
																
			case kStartButtonKey:
						pad1 |= kButtonStart;
						break;
																
			default:												
						if ( REFGLOBAL( controls, keyboardFlags ) & kAltDown )
						{
							switch( key )
							{
								case 'A':
								case 'a':
											pad1 |= kButtonA;
											break;				
								case 'B':	
								case 'b':	
											pad1 |= kButtonB;
											break;
								case 'X':	
								case 'x':	
											pad1 |= kButtonX;
											break;
								case 'S':	
								case 's':	
											pad1 |= kStart;
											break;
							}
						}
						else
						{
							REFGLOBAL( controls, sysKeysHead )++;		/* not pad-like, stick in key buf */
							REFGLOBAL( controls, sysKeysHead ) &= kSysKeysFifoMask;
							head = REFGLOBAL( controls, sysKeysHead );
							REFGLOBAL( controls, sysKeysBuf )[head] = key;
						}
						break;
			}
		}
		
	} 
	while ( key != kNoKey );
	
	results[0] = pad1;
#endif
}



Boolean _FlushHardwareKeyboardBuffer( void )
{
	REFGLOBAL( controls, sysKeysTail ) = REFGLOBAL( controls, sysKeysHead );
	
	if ( REFGLOBAL( controls, keyboardPresent ) )
		return( true );
	else
		return( false );
}



long _GetHardwareKeyboardFlags( void )
{
	return( REFGLOBAL( controls, keyboardFlags ) );
}



void _SetHardwareKeyboardFlags( long flags )
{
	REFGLOBAL( controls, keyboardFlags ) = flags;
}



unsigned char _GetNextHardwareKeyboardChar( void )
{
unsigned char tail;

	if ( REFGLOBAL( controls, keyboardPresent ) &&
			(REFGLOBAL( controls, sysKeysTail ) != REFGLOBAL( controls, sysKeysHead )) )
	{
		REFGLOBAL( controls, sysKeysTail )++;
		REFGLOBAL( controls, sysKeysTail ) &= kSysKeysFifoMask;
		tail = REFGLOBAL( controls, sysKeysTail );
		return( REFGLOBAL( controls, sysKeysBuf )[tail] );
	}
	else
		return( kNoKey );
}



unsigned char _GetNextESKeyboardRawcode( void )
{
unsigned char tail;
unsigned char c;

	if ( REFGLOBAL( controls, keyboardPresent ) &&
			(REFGLOBAL( controls, keycodeTail ) != REFGLOBAL( controls, keycodeHead )) )
	{
		tail = REFGLOBAL( controls, keycodeTail );
		c = REFGLOBAL( controls, keycodeBuf )[tail];
		
		REFGLOBAL( controls, keycodeTail )++;
		REFGLOBAL( controls, keycodeTail ) &= kKeybdDataFifoMask;
		
		return( c );		/* How did this ever work before??? */
	}
	else
		return( 0xFF );
}



void BackUpKeycodeTail( void )
{
	if ( REFGLOBAL( controls, keycodeTail ) == 0 )				// back up; we'll try again later
		REFGLOBAL( controls, keycodeTail ) = kKeybdDataFifoMask;
	else
		REFGLOBAL( controls, keycodeTail )--;
}



/* 
 	THIS SUCKS.  Rework for SNES.
 */
 
unsigned char _GetNextESKeyboardChar( void )
{
unsigned char raw;
unsigned char map;
unsigned long offset;
unsigned char fuck[2];
Boolean 	  specialPending;
	
	specialPending = false;
	map = kNoKey;
	
	if ( REFGLOBAL( controls, keyMapTable ) )
	{
		raw = GetNextESKeyboardRawcode();			// get next code out of buffer
		
		if ( raw != 0xFF )							// anything in buffer?
		{
		
		//
		// BREAK (KEYUP) CODES
		//
		
		if ( raw == 0xF0 )								
		{
			raw = GetNextESKeyboardRawcode();								// try to get the one that's breaking
			
			if ( raw == 0xFF )												// not in buf yet?
			{
				BackUpKeycodeTail();
				return( kNoKey );
			}
				
			if ( raw < (REFGLOBAL( controls, keyMapTable )->tblSectionSize) )		// valid code?
			{
				map = (REFGLOBAL( controls, keyMapTable )->mapTbl)[raw];
				
				if ( map == kShiftKey )				
					REFGLOBAL( controls, keyboardFlags ) &= ~kShiftDown;

				if ( map == kAltKey )				
					REFGLOBAL( controls, keyboardFlags ) &= ~kAltDown;

				if ( map == kControlKey )				
					REFGLOBAL( controls, keyboardFlags ) &= ~kControlDown;

				if ( map == kCapsLockKey )			
					REFGLOBAL( controls, keyboardFlags ) &= ~kCapsLockDown;		// caps lock up
			}
			
			return( kNoKey );
		}
		
		//
		// 101-STYLE CODES
		//

		if ( raw == 0xE0 )													
		{
			raw = GetNextESKeyboardRawcode();						// get key code
			
			if ( raw == 0xFF )										// special key there yet?
			{
				BackUpKeycodeTail();
				return( kNoKey );
			}
			else													// special key *is* there
			{
				if ( raw == 0xF0 )									// just trash breaks
				{
					raw = GetNextESKeyboardRawcode();				// try to get one to break
					if ( raw == 0xFF )								// key to break here?
					{
						BackUpKeycodeTail();
						BackUpKeycodeTail();
					}
					return( kNoKey );
				}
				specialPending = true;				// flag it special
			}
		}
		
		//
		// NORMAL CODES
		//
		
		if ( raw < (REFGLOBAL( controls, keyMapTable )->tblSectionSize) )		// valid code?
		{
			map = (REFGLOBAL( controls, keyMapTable )->mapTbl)[raw];			// map to lower case

			if ( REFGLOBAL( controls, keyboardFlags ) & kShiftDown )			// shift key -> ALL shifted
			{
				if ( (map >= 0x20) && (map <= 0x7E) )
				{
					offset = raw + REFGLOBAL( controls, keyMapTable )->tblSectionSize;	// use upper tbl
					map = (REFGLOBAL( controls, keyMapTable )->mapTbl)[offset];			// map to shifted
				}	
			}
			else
				if ( REFGLOBAL( controls, keyboardFlags ) & kCapsLocked )			// capslock key -> CHARS shifted
				{
					if ( (map >= 'a') && (map <= 'z') )
					{
						offset = raw + REFGLOBAL( controls, keyMapTable )->tblSectionSize;	// use upper tbl
						map = (REFGLOBAL( controls, keyMapTable )->mapTbl)[offset];			// map to shifted
					}
				}
		}
		
		//
		// META KEYS
		//
			
		switch(map)
		{	
			case kShiftKey:
					REFGLOBAL( controls, keyboardFlags ) |= kShiftDown;
					return( kNoKey );
					break;
		
			case kAltKey:
					REFGLOBAL( controls, keyboardFlags ) |= kAltDown;
					return( kNoKey );
					break;
		
			case kControlKey:
					REFGLOBAL( controls, keyboardFlags ) |= kControlDown;
					return( kNoKey );
					break;
		}
		
		if ( (map == kCapsLockKey) && !(REFGLOBAL( controls, keyboardFlags ) & kCapsLockDown) )
		{
			REFGLOBAL( controls, keyboardFlags ) |= kCapsLockDown;
			REFGLOBAL( controls, keyboardFlags ) ^= kCapsLocked;			// flip caps lock state
			return( kNoKey );
		}

		//
		// Special Keys
		//

		if ( !(REFGLOBAL( controls, keyboardFlags ) & kReturnIsLF) && (map == 0x0A) )
			map = kEnterKey;

		if ( specialPending )
		{
			switch( raw )
			{
				case 0x75:							// up arrow
							map = kUpArrowKey;
							break;
				case 0x72:							// down arrow
							map = kDownArrowKey;
							break;
				case 0x6B:							// left arrow
							map = kLeftArrowKey;
							break;
				case 0x74:							// right arrow
							map = kRightArrowKey;
							break;
				case 0x5A:
							map = kEnterKey;
							break;
				default:
							map = kNoKey;
							break;
			}
		}	
		
		}
		else		/* raw == 0xff, nothing in buffer */
		{
			map = kNoKey;
		}
		
	}
		
	return( map );
}




/*
	UNUSED on SNES
 */
 
void _WriteESKeyboard ( void )
{
}

/*
	UNUSED on SNES
 */
 
void _SendCmdToESKeyboard( unsigned char *cmdBuf, unsigned char cmdLen )
{

}

/*
	UNUSED on SNES
 */
unsigned char _GetNextESKeyboardStatus( void )
{
	return( 0xFF );
}



