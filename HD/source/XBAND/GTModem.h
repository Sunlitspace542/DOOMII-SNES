/*
	File:		GTModem.h

	Contains:	xxx put contents here xxx

	Written by:	Jevans

	Copyright:	© 1994 by Catapult Entertainment, Inc., all rights reserved.

	Change History (most recent first):

		 <2>	 1/30/95	SAH		New stuff in GTModem struct for new reentrancy scheme.
		 <4>	 8/24/94	SAH		Turned the Pad into a currentFrame counter (Perlman). Added some
									fifo masks.
		 <3>	 7/24/94	SAH		Cleaned up, added new protos.
		 <2>	 7/22/94	SAH		New modem stuff.
		 <1>	 6/20/94	DJ		first checked in

	To Do:
*/



#ifndef __GTMODEM__
#define __GTMODEM__

#include "GTNetwork.h"


//
// The modem has a 32 byte buffer (eg. 8 frames).
//
#define kModemInBufferLength	32
#define kModemOutBufferLength	16

//
// These masks are multiplied by four to preshift the array lookups
//
#define	kModemInBufferMask		((kModemInBufferLength<<2) - 1)	
#define	kModemOutBufferMask		((kModemOutBufferLength<<2) - 1)	
#define	kModemInBufferMask		((kModemInBufferLength<<2) - 1)	

//
// Because you can only read fred's current frame number when the 
//	rx fifo is empty, most data arrives without us being able to
//	determine the current frame number
//

#define	kUnknownCurrentFrame	0x0f

typedef
struct GTModemByte {
	unsigned char	byte;
	unsigned char	roskoFrame;
	unsigned char	currentFrame;
	unsigned char	vCount;
} GTModemByte;

typedef
struct GTModem {
	GTModemByte		inBuffer[kModemInBufferLength];
	short			readIndex;
	short			writeIndex;
	short			byteCount;
	
	Err				modemErr;

	struct GTNetwork	network;
	Boolean			master;
	
	/* on nintendo interrupts aren't latched, so we need to add our own */
	/* deferral mechanism. the only thing we care about is reading the modem */
	short			busyCount;
	Boolean			pendingReadModem;	/* someone tried to read the modem, do it later for them */

} GTModem;


#endif __GTMODEM__
