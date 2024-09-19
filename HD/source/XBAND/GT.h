/*
	File:		GT.h

	Contains:	xxx put contents here xxx

	Written by:	Jevans,Shannon Holland

	Copyright:	© 1994 by Catapult Entertainment, Inc., all rights reserved.

	Change History (most recent first):

		<11>	 1/30/95	SAH		Added a checksum table ptr to the lametalk session record.
		<10>	 1/17/95	SAH		Support for SNES and fred II.
		 <9>	12/15/94	SAH		Included rest for snes.
		 <8>	12/14/94	HEC		Temp turn off reins.
		 <7>	 12/6/94	CMY		Make this compile on the SNES box. No 68k asm!
		 <6>	 12/6/94	KON		Move callthroughvector macro outside SNES ifdef so other stuff
									that depends on it can compile for SNES.
		 <5>	 12/4/94	CMY		Pretty much #ifdef'ed out the whole thing for SNES.
		 <2>	 9/22/94	CMY		Changed to use new CallDispatchedFunction macro.
		<22>	 8/29/94	ATM		Added #ifndef __SERVER__ stuff to make it build under UNIX.
		<21>	 8/27/94	SAH		Bumped size of gametalk fifos.
		<20>	 8/24/94	SAH		New interfaces. Moved Stinko to GTStinkOTron.h. New packet
									formats.
		<19>	 8/22/94	SAH		Added all the packet formats.
		<18>	 8/22/94	SAH		More stink.
		<17>	 8/22/94	SAH		Added GTSyncOTron_GraphicSynch.
		<16>	 8/21/94	ADS		New lomem layout
		<15>	 8/18/94	SAH		Took out k16bitData for a while.
		<14>	 8/15/94	SGP		Added constant for 16 bit data
		<13>	 8/10/94	HEC		Added eqmThreshold session field.
		<12>	  8/8/94	SAH		Keep track of bad packets.
		<11>	 7/26/94	SAH		Added GTSession_CloseSessionSynch.
		<10>	 7/24/94	SAH		New functions. Cleaned up.
		 <9>	 7/22/94	SAH		New interfaces.
		 <8>	  7/8/94	DJ		Added ReadBytes and FlushInput
		 <7>	  7/3/94	SAH		Did the <> thing around the Types.h include to speed up
									compiles.
		 <6>	 6/28/94	DJ		GetCurrentTime shit
		 <5>	 6/21/94	BET		We think we integrate, but then we learn.
		 <4>	 6/21/94	HEC		Compiling is good
		 <3>	 6/21/94	BET		Need these too...
		 <2>	 6/21/94	BET		Managerize
		 <1>	 6/20/94	DJ		first checked in

	To Do:
*/




#ifndef __GT__
#define __GT__


#include "GTNetwork.h"
#include "GTModem.h"


//
// Some packet formats
//
enum
{
	k8BitData,		// 16 bit packets
	k12BitData,		// 16 bit packets
	k16BitData,		// 24 bit packets
	k19BitData,		// 24 bit packets
	k24BitData,		// 32 bit packets
	k27BitData		// 32 bit packets
};


	// BRAIN DAMAGE: because of our fifo masking, make sure this is always a power of two
#define kMaxFrameLatency	16		// This is the maximum one way latency (in frames (vbls))
									// that our software can handle.

#define	kMaxFifoSize		(kMaxFrameLatency*2)		// size of fifo to accomodate max frame latency (size in longs)
#define	k1ByteFifoIndexMask	(kMaxFifoSize - 1)			// mask for 1 byte fifo's - assumes power of two
#define	k2ByteFifoIndexMask	(k1ByteFifoIndexMask << 1)	// mask for 2 byte fifo's - assumes power of two
#define	k4ByteFifoIndexMask	(k1ByteFifoIndexMask << 2)	// mask for 4 byte fifo's - assumes power of two

#define	kTimeStampMask		0x7f	// we send 7bits of frame data in our resend command
									// therefore we keep 7 bits of frame data in our local
									// time stamps

typedef struct GTSession
{
	struct GTModem modem;

	short 			frameDelay;			// frame delay we're working with
	short			byteLatency;		// one way latency for single byte
	short			roundtripLatency;	// how long for modem round trip

	short			packetFormat;		// packet format for this gt session
	short			bytesInPacket;		// how many bytes in a packet for this format
	short			bitSize;			// size of the packet data in bits
	long			packetMask;			// mask for packet data only
	
	unsigned long	sendFifo[ kMaxFifoSize ];	// send/local fifo
	short			localIndex;			// index where we're reading local controllers from
	short			remoteIndex;		// index where we're sending remote controllers from
	short			writeIndex;			// index where we're putting new local controller reads

	short			sendTimeStamp;		// timestamp for last packet sent
	short			recvTimeStamp;		// timestamp of last packet received.
	
	long			eqmThreshold;		// take action above this noise threshold 

	unsigned char *	checksumTable;		// checksum table
	
	// some default timeouts
	long			defaultEstSynchTimeout;
	long			defaultSynchStateTimeout;
	long			defaultExgCommandsTimeout;
	long			defaultErrorRecoverTimeout;
	
	// some debugging stuff
	long			badPackets;
	
	// no one needs this yet as far as i know, but i'll leave it here as it's small
	// and convenient
	Boolean			master;
		
} GTSession;



#endif __GT__

