/*
	File:		GTNetwork.h

	Contains:	xxx put contents here xxx

	Written by:	Jevans

	Copyright:	© 1994 by Catapult Entertainment, Inc., all rights reserved.

	Change History (most recent first):

		 <3>	 7/24/94	SAH		Removed some unused stuff.
		 <2>	 7/22/94	SAH		Next checked in.
		 <1>	 6/20/94	DJ		first checked in

	To Do:
*/



#ifndef __GTNETWORK__
#define __GTNETWORK__

#include "GT.h"

//
// Data moves from the right to the left
//	(eg. fromMaster[kLatencyBytes-1] is entrance into the wire and fromMaster[0] is exit)
//

typedef struct GTNetwork {
	long				time;
	struct GTModem	*	masterModem;
} GTNetwork;


#endif __GTNETWORK__
