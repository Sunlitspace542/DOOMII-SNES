/*
	File:		GamePatch.h

	Contains:	xxx put contents here xxx

	Written by:	Shannon Holland

	Copyright:	© 1994 by Catapult Entertainment, Inc., all rights reserved.

	Change History (most recent first):

		<12>	 1/30/95	SAH		Latest interfaces for snes.
		<11>	 1/21/95	SAH		Rearranged the game heap world so that it's no longer a heap!
		<10>	 1/19/95	SAH		Merge with Sega 1.0 Project.
		 <9>	 1/17/95	SAH		Make sure to go through sram on the game dispatcher dispatch.
		 <8>	  1/5/95	SAH		Don't preshift selector numbers.
		 <7>	12/19/94	SAH		More snes stuff.
		 <6>	12/15/94	SAH		Made more real for snes.
		 <5>	 12/6/94	CMY		Make it compile on the real SNES box. No 68k assembly on a SNES!
		 <4>	 12/6/94	KON		Make it work better when included for SNES.
		 <3>	 12/4/94	CMY		#ifdefed out a ton of stuff for SNES!
		 <2>	 9/22/94	CMY		Changed to use new CallDispatchedFunction macro.
		<35>	 8/29/94	ATM		Added #ifndef __SERVER__ stuff to make it build under UNIX.
		<34>	 8/27/94	SAH		Real sinkotron interface/selectors. Bumped size of game dispatch
									table.
		<33>	 8/26/94	HEC		BIG ONE: Added lots of PModem routines to the game dispatcher to
									make OSCheckLine callable during game play.
		<32>	 8/24/94	HEC		Added game dialog idle and countdown timer.
		<31>	 8/23/94	HEC		Changed GPDialogRec
		<30>	 8/22/94	SAH		More general low mems.
		<29>	 8/22/94	SAH		Added Stinkotron.
		<28>	 8/22/94	HEC		Fucker.
		<27>	 8/22/94	HEC		Update game dispatcher to new vectors.
		<26>	 8/21/94	ADS		New lomem layout
		<25>	 8/21/94	JOE		Christen the Game Patch Flags section with kMasterOnLeftFlag
		<24>	  8/7/94	HEC		More to GPDialogRec.  Added dialog commands.
		<23>	  8/6/94	ADS		New interface for controller types & reading them
		<22>	  8/6/94	HEC		Filled out GPDialogRec.
		<21>	  8/6/94	ADS		Added controller init call
		<20>	  8/4/94	SAH		Changed minFreeGames to freeTimeOver. Made OSNewMemory and
									OSDisposeMemory game dispatcher calls to stop assembly selector
									hell. Killed ResumeGame. Added GPReportErrors.
		<19>	  8/1/94	HEC		Removed gameOwnsDram.
		<18>	 7/31/94	HEC		Added gameOwnsDram game param.  Added kOSCheckLine.
		<17>	 7/29/94	SAH		Made the game dispatcher macro smaller.
		<16>	 7/26/94	HEC		Added HandleCallWaiting support.
		<15>	 7/26/94	SAH		Added OSGTCloseSessionSynch.
		<14>	 7/25/94	SAH		Added minChargedGames to the GamePatchParamBlock.
		<13>	 7/25/94	ADS		Added ReadControllers
		<12>	 7/24/94	SAH		Happy happy gametalk fun.
		<11>	 7/15/94	SAH		New GamePatchParamBlock.
		<10>	 7/12/94	SAH		Added CreateGameDispatcher.
		 <9>	 7/12/94	HEC		Defined PModem selectors/protos for GameTalk via the game os.
		 <8>	  7/5/94	SAH		Updated game dispatch macro to use latest low mems.
		 <7>	  7/2/94	SAH		Added minMappedRom.
		 <6>	 6/28/94	SAH		Moved the game patch globals to globals.h.
		 <5>	 6/21/94	SAH		GameTalk is here.
		 <4>	 6/20/94	SAH		Added affectsRanking to the game param block.
		 <3>	 6/18/94	GOD		Initial working version.
		 <2>	 6/17/94	SAH		Latest game patch stuff.
		 <1>	 6/16/94	SAH		first checked in

	To Do:
*/


#ifndef __GamePatch__
#define __GamePatch__


#ifndef __GT__
#include "GT.h"
#endif

#ifndef __SERVER__
#ifndef __GTSYNCOTRON__
#include "GTSyncOTron.h"
#endif

#include "Controls.h"
#endif	/*__SERVER__*/

/*
 *  These are the flags defined for the PatchDescriptor's flags field
 */

#define	kMasterOnLeftFlag	0x00000001
#define	kSupportSingleGame	0x00000002
#define kCompressedGame		0x00000004


/*
*	The first structure is the parameter block through which the game patch returns results
*	to the server. the second structure is the overall os/patch parameter block. This one has
*	space for the os to send information to the game patch.
*/

typedef
struct GameResult
{
	long			size;					// the size of the game results structure
	long			gameID;					// id of the cart - don't worry about this
	long			gameError;				// any errors that caused the game to terminate
	unsigned long	localPlayer1Result;		// scores
	unsigned long	localPlayer2Result;
	unsigned long	remotePlayer1Result;
	unsigned long	remotePlayer2Result;
	long			playTime;				// how long game played in frames
	long			dbIDDataPtr;			// xtra data to send to server (nil if none)
	long			dbIDDataSize;			// size of that extra data
	long			gameReserved[ 10 ];		// xtra room
	short			numLocalPlayers;		// 1 or 2 local players
	char			pad[2];
} GameResult;


typedef
struct GamePatchParamBlock
{
	// this stuff is passed into the game patch by the os
	Boolean			isMaster;			// true if master, false if slave
	Boolean			affectsRanking;		// does this game affect you ranking or is it just fun
	short			freeTimeOver;		// in flux
	short			frameDelay;			// frames of latency (currently not set)
	long			minMappedRam;		// size of sram from beginning that must be mapped in
	long			minMappedRom;		// size of rom from beginning that must be mapped in
	long			maxTimePlay;		// not used yet - max no. of frames for game
	short			maxGamesPlay;		// not used yet - max no. of games to allow
	long			gameFlags;			// flags passed in to game
	GTSession *		commSession;		// ptr to the gametalk session record to use
	long			osReserved[ 10 ];
	
	// this stuff is set by the game patch for the os to look at
	GameResult		results;			// above structure
} GamePatchParamBlock;



/* game play flags */
#define	kRemoteGame			0x01		/* set if a local/remote game, clear if lcoal only */
#define	kColdStart			0x02		/* set if cold boot (show copyright screens) */
#define kCallWaitingAllowed	0x04		/* set if box allows call waiting */


/*
*	Game Patch interface - don't let c fool you, the compiler uses pascal calling conventions
*	with results returned in a and x (low word in a, high word in x). the numbers you care
*	about returning are 0 (no error) and -1 (error). pascal conventions means parameters pushed
*	left to right and the callee has to clean up the stack: ie, save return address, then pop
*	parameters and then return.
*/

typedef long ( *GameProc ) ( long command, long data1, long data2 );

/* these are the commands that your entry point is called with */
enum
{
	kGPInitGamePatch,			/* init anything, but leave os state (dram) intact */
	kGPStartGamePatch,			/* go off and play the game (the world is yours) */
	kGPDisplayMessage,			/* put up a message */
	kGPStopGame,				/* halt the game - not really used */
	kGPKillGame,				/* clean up after a game - not really used */
	kGPReportError,				/* report any errors through game results */
	kGPPlaySinglePlayerGame,	/* play a single player game while waiting */
	
	/* last game patch command */
	kLastGameCommand
};


/*
*	Dialog stuff for the kGPDisplayMessage message
*/

// Game Patch Errors to OS
enum
{
	kGPNoErr = 0,
	kGPCannotDrawDialogErr = -1
};

// Game Patch Dialogs

#define kGPDShowTimer	0x00000001L

enum {
	kGPDDisplayCmd,			// put up dialog
	kGPDIdleCmd,			// dialog idle (does not have to be supported)
	kGPDRemoveCmd			// tear down dialog
};

typedef struct {
	unsigned short command;	// What should dialog do?
	unsigned short flags;	// flags
	char 		*message;	// text to display
	char		*respA;		// Button A -- 1st response text (nil if not used)
	char		*respB;		// Button B -- 2nd response text (nil if not used)
	char		*respC;		// Button C -- 3nd response text (nil if not used)
	long		countdown;	// Number of remaining ticks until timeout (0 if no timer)
	long		refCon;		// for game patch use
} GPDialogRec;


/*
*	Selectors for calls back to the os from the game
*/

/* total number of game selectors available */
#define	kNumGameSelectors	64

enum
{
	/* general game stuff */
	kOSHandleGameError,
	kOSGameOver,
	
	/* gametalk */
	kOSGTSSetPacketFormat = kOSGameOver + 24,
	kOSGTSSetRamRomOffset,
	kOSGTSessionSetLatency,
	kOSGTSessionPrefillFifo,
	kOSGTSessionEstablishSynch,
	kOSGTSErrorRecover,
	kOSGTSCloseSessionSynch,
	kOSGTSFlushInput,
	kOSGTSessionValidateControl,
	kOSGTSessionExchangeCommands,
	kOSGTSDoCommand,
	kOSGTSDoResend,
	kOSGTSResendFromFrame,
	kOSGTModemInit,
	kOSGTModemGetModemError,
	kOSGTModemClearFifo,
	kOSGTModemClockInByte,
	kOSGTModemClockOutByte,
	kOSGTModemAbleToSend,
	kOSGTModemSendBytes,
	kOSGTModemCheckLine,
	
	/* controller - not really used yet */
	kOSInitControllers,
	kOSReadControllers,
	
	/* stinkotron - not implemented yet */
	kOSGTSyncotron,
	kOSGTMasterCalculateLatency,
	kOSGTSlaveCalculateLatency,
	kOSGTSyncoReadModemVBL,
	kOSGTSyncronizeVBLs,
	kOSGTSyncronizeMasterLeave,
	kOSGTSyncronizeSlaveLeave,
	kOSGTSyncoTronVBLHandler,
	
	/* keep this one */
	kOSLastFunction
};



/* macro to dispatch through the game dispatcher */
/* sel is the selector from the above enum. in turms of assembly, you want to do this: */
/*	ldx	#sel */
/*	jsl	$E000CC */
#define CallGameOSFunction(sel)	inline ( sel, 0xE000CC )


/*
*	These macros are used to call gametalk during active game time. they can be called with
*	any memory mapping.
*/

#define	kGTReadControllerVector		0xE000E0
#define	kGTSendControllerVector		0xE000E4
#define	kGTReadHardwareModemVector	0xE000E8


#define	CallGTReadController()		inline( 0, kGTReadControllerVector )
#define	CallGTSendController()		inline( 0, kGTSendControllerVector )
#define	CallGTReadModem()			inline( 0, kGTReadHardwareModemVector )


/* 
*	os calls for the game patch. these must be called with normal memory mapping (ie
*	our rom is at $d00000 and our sram is at $e00000)
*/

	/* general game calls */
	
	/* an error has occured (from GTSession_ErrorRecover), pass it in here and the os will deal */
	/* if it can recover directly, it will return to you. if it needs to reboot the os (to put */
	/* up dialogs), you will be called with another StartGame message */
short	OSHandleGameError ( short err )  
	CallGameOSFunction ( kOSHandleGameError );

	/* a successful game has completed. if the user wants to play another game, you will be called */
	/* with a startgame message - ie this call never returns */
short	OSGameOver ( void )  
	CallGameOSFunction ( kOSGameOver );


	/* gametalk */
	
	/* set the packet format - use the format #defines from above. */
long	OSGTSession_SetGTPacketFormat ( GTSession * session, short packetFormat )  
	CallGameOSFunction( kOSGTSSetPacketFormat );

	/* if you remap our sram and rom, call this to give us the offsets to the new location before */
	/* you do it */
long	OSGTSession_SetRamRomOffsets ( GTSession * session, long ramOffset, long romOffset )  
	CallGameOSFunction( kOSGTSSetRamRomOffset );

	/* set the latency in frames for the connection */
long 	OSGTSession_SetLatency ( GTSession * session, short frameLatency )  
	CallGameOSFunction( kOSGTSessionSetLatency );

	/* presend some number of null controller reads to fill all the fifo for our latency */
long	OSGTSession_PrefillFifo ( GTSession * session, unsigned long fillData )  
	CallGameOSFunction( kOSGTSessionPrefillFifo );

	/* establish a connection with the remote unit (a gametalk connection, not a modem connection) */
	/* this is used to syncronize and clear any shit out of the line */
long	OSGTSession_EstablishSynch ( GTSession * session, long timeout )  
	CallGameOSFunction( kOSGTSessionEstablishSynch );

	/* recover from an error from ReadController. pass the error you got in here. */
	/* if this guy returns an error, call HandleGameError with it */
	/* this function is used to recover from simple line noise. if it can't, it will return an erro */
	/* to go to HandleGameError which will deal with more drastic things such as dropped lines and */
	/* call waiting */
long	OSGTSession_ErrorRecover ( GTSession * session, long error, long timeout )  
	CallGameOSFunction( kOSGTSErrorRecover );

	/* synch with the other unit and send them a close command used to force a termination of the game */
long	OSGTSession_CloseSessionSynch ( GTSession * session, long timeout )  
	CallGameOSFunction( kOSGTSCloseSessionSynch );

	/* flush any imput from the software fifos */
long 	OSGTSession_FlushInput(GTSession *session)  
	CallGameOSFunction( kOSGTSFlushInput );

	/* read the controllers for the next game frame. returns a matched local and remote controller */
	/* pair. will return an error if there is noise or we have no data */
long 	OSGTSession_ReadController ( GTSession * session, long * localData, long * remoteData )  
	CallGTReadController();

	/* send the controller to the remote guy and put it in our local fifo. ignore any errors it returns */
long 	OSGTSession_SendController ( GTSession * session, long data )  
	CallGTSendController();


	/* call this within the nmi (every 60th of a second) to pull bytes out of the modem */
	/* fifo to prevent overruns */
Err 	OSGTSession_ReadHardwareModem ( GTSession * session )  
	CallGTReadModem();

	
#endif
