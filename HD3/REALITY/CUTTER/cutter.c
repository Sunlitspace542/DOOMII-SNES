#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <exec/memory.h>

#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/exec_pragmas.h>


extern	struct IntuitionBase *IntuitionBase;
extern	struct GfxBase *GfxBase;
extern	struct DOSBase *DOSBase;




//void    debugchk (void);

long	ReadIFF		    (char *PicName, char *pChar, long flipInputX);
void	MarkIFF1x1	    (long X, long Y, long pen);
void	MarkIFF2x2	    (long X, long Y, long pen);

void    _STI_GetChe         (void);
long    GetChe              (void);
void    _STD_GetChe         (void);
void	_STD_FreeMem	    (void);


char*   allocMem            (long align, long size);
void    allocMems           (void);
void    cdNext              (void);
void    chkCopyOfReassign   (long *p1x1_);
void    chkMir              (void);
long    chkRowStatus        (void);
void    chkWeight           (void);
void    copyImageAllFlips   (void);
void    cut                 (void);
long    cut1x1AllFlips      (void);
long    cut1x1NoFlip        (void);
long    cut1x1XFlip         (void);
long    cut1x1YFlip         (void);
long    cut1x1XYFlip        (void);
long    cut1x1              (void);
void    cut1x1Mir           (long *p1x2_, long copy_, long ul_y_);
long    cut2x2AllFlips      (void);
long    cut2x2NoFlip        (void);
long    cut2x2XFlip         (void);
long    cut2x2YFlip         (void);
long    cut2x2XYFlip        (void);
long    cut2x2              (void);
long    cut2x2Sub           (long *pCharIndex_, long _2x2Flag_);
void    cut2x2Mir           (long *p2x2Mir_);
void    cutImageSlidingRows (void);
void    cutMir              (void);
long    cutRow              (void);
void    cutRowColumn        (void);
void    cutRowColumn1x1     (void);
void    cutRowColumn2x2     (void);
void    cutRows             (void);
void    delay               (long secs_);
void    dumpImage           (char *str_);
long    findPacked          (char *pChar_);
void    flipAlongDiags      (void);
long	getImage            (void);
void    initControls        (void);
void    initImageList       (void);
void    initMImgTblStructure (void);
void    initPack            (void);
void    inputFiles          (void);
void    inputPack           (void);
void    inputMImgTbl        (void);
void    inputMImgTblI       (void);
void    inputCharData       (void);
void    inputMiscData       (void);
void    inputImageTbl       (void);
void    main                (long numArgs_, char *args_[]);
void    move                (char *dest_, char *src_, long numBytes_);
void    outputFiles         (void);
void    outputPack          (void);
void    outputMImgTbl       (void);
void    outputMImgTblI      (void);
void    outputCharData      (void);
void    outputMiscData      (void);
void    outputCharDef       (void);
void    outputCharDefSub    (long x_, long y_, char *pPack_, char *pChar_);
void    outputImageTbl      (void);
long    packChar            (char *pChar_);
unsigned short packCharDataOffset (long charNum);
void    packCharData        (void);
void    packMiscData        (void);
void    packObjs            (void);
long    pixelMatch          (char *pChar_, char *pChar2_);
void    restore             (void);
void    restoreSub          (long *p_);
long    reverseWord         (long input);
void    setCopyArray        (char *pCharBeg_, long *pCopyBeg_,
                                long leftX_, long rightX_,
                                long topY_, long bottomY_);
long    setImageBounds      (void);
void    showCut             (void);
void    showCutSub          (char *s_, long *p_);
void    undoCut1x1          (void);
void    undoCut1x1Mir       (void);
void    undoCut2x2          (void);
void    undoReassign        (long *p1x1_);
void    useBestCut          (void);
void    shrinkImageTbl      (void);
void    expandImageTbl      (void);





// static arrays:

// #words
#define MIMGTBLI_SIZE 0x1000

// #bytes
#define MIMGTBL_SIZE 0x8000

// #bytes
#define MEM_SIZE 0x380000

// #bytes only for pPackBeg array (put at 0x1b00000 to do 1.5 Meg cart)
#define PACK_MEM_SIZE 0x0800000




// buffers which use above MEM_SIZE:

// #bytes
#define SCALE_SIZE 0x80

// #bytes
#define ROTATE_SIZE 0x1000

// #bytes (no limit)
#define MISC_DATA_SIZE 0x30000

// #bytes
#define CHAR_DATA_BUF_SIZE 0x200

// #bytes
#define CHAR_DATA_SIZE 0xfffc

// #bytes in table, max = (0x10000 / 5 * 5)
#define IMAGE_TBL_SIZE 0x8000

// #bytes
#define SORT_SIZE 0x800

// #bytes
#define SP_SIZE 0x1000

// #bytes
#define SPW_SIZE 0x1000





// note some code assumes below ordering of 2x2 and 1x1 data, so don't
//   go changing it by define values alone
#define UL_X 0
#define UL_Y 1
#define _2X2_COPY 2
#define UL_CHAR 3
#define UL_COPY 4
#define UL_REASSIGN 5
#define LL_CHAR 6
#define LL_COPY 7
#define LL_REASSIGN 8
#define UR_CHAR 9
#define UR_COPY 10
#define UR_REASSIGN 11
#define LR_CHAR 12
#define LR_COPY 13
#define LR_REASSIGN 14

#define _2X2_SIZE 15

#define CHAR 2
#define COPY 3
#define CHAR_SAVE 4
#define COPY_SAVE 5
#define DUPLICATE 6

#define _1X1_SIZE 7

#define OUTPUT 5





// main memory:

// static char     packMem[PACK_MEM_SIZE];
static char     mem[MEM_SIZE];
static short    mImgTblI[MIMGTBLI_SIZE];
static char     mImgTbl[MIMGTBL_SIZE];





// 4 byte vars:

static long     memUsed=0;
static long     weight, bestWeight, bestEverWeight;
static unsigned long sumOfWeightsHi = 0, sumOfWeightsLo = 0;
static long     topY, bottomY, leftX, rightX;
static long     packNumEnd;
static long     topYRow, deltaTopYRow;
static long     leftXRow, rightXRow;
static long     leftXMirCenter, rightXMirCenter;
static long     mirror;
static long     maxWidth1Piece, maxWidthHalf2Simul;
static long     minWidthToMirror;
static long     firstSimulPart;
static long     allow2Char2x2Cut, allow2x2WithLeftBlank, allow2x2WithRightBlank;
static long     rightColumnBlank;
static long     ul_x_g, ul_y_g;
static long     xLimitMir;
static long     downUpForAllYOffsets;
static long     weightVramChar;
static long     weightObj;
static long     weightFlicker;
static long     weightRomChar;
static long     columns;
static long     allowSlidingRows, allowSlidingColumns;
static long     bestEverByColumns;
static long     numYOffsets, numXOffsets;
static long     allowSliding1x1;
static long     proceedAfterSliding1x1;
static long     allowSliding1x1OutOfRow;
static long     allow1x1ToCopy2x2;
static long     imageDepth; 
static long     mImgTblOfstEnd;
static long     numMasterImages;
static long     numImages, imageNum;
static long     palette;
static long     priority;
static long     charNumOfst;
static long     only1x1;
static long     smallImageTbl;
static long     allowDownUp;
static long     packSize;
static long     flipInputX;
static long     allowShowCut, allowSimulCutInMiddle;
static long     allowImageDump;
static long     allowOutputFiles;

//static long     debugCnt=0;





// mImgTbl structure vars:
static unsigned char *pRotateBeg, *pRotateMax;
static unsigned char *pRotateCnt, *pRotateEnd;
static unsigned char *pScaleBeg, *pScaleMax;
static unsigned char *pScaleCnt;
static long     numTilts;





// stuff for char data
static unsigned char    *pcd;
static unsigned short   cdOfst, cdOfstEnd;
static unsigned short   cdMode;
static short            cdNumDups;
static unsigned short   cdNumReps;
static unsigned short   cdCharNum;
static unsigned short   cdCharNumNT;





// ptrs:

static char     *pMem = & mem[0];

static unsigned char    *pmd;
static unsigned long    mdOfstEnd;
static unsigned short   *pCharDataBufBeg, *pCharDataBufEnd, *pCharDataBufMax, *pCharDataBuf;
static unsigned char    *pImageTblBeg;
static unsigned short	imageTblOfstEnd;
static long             *pSortBeg, *pSortEnd, *pSortMax, *pSort;

static long     *p1x1Beg;
static long     *p1x1End;
static long     *p1x1BestBeg;
static long     *p1x1BestEnd;
static long     *p1x1BestEverBeg;
static long     *p1x1BestEverEnd;

static long     *p2x2Beg;
static long     *p2x2End;
static long     *p2x2BestBeg;
static long     *p2x2BestEnd;
static long     *p2x2BestEverBeg;
static long     *p2x2BestEverEnd;

static long     *p1x1RowBeg;
static long     *p2x2RowBeg;
static long     *p1x1CloseMirBeg;
static long     *p2x2CloseMirBeg;
static long     *p1x1CloseMirEnd;
static long     *p2x2CloseMirEnd;

static long     *p1x1Mir;
static long     *p2x2Mir;

static char     *pChar;
static char     *pXChar;
static char     *pYChar;
static char     *pXYChar;

static long     *pCopy;
static long     *pXCopy;
static long     *pYCopy;
static long     *pXYCopy;

static char     *pPackBeg;
static char     *pPackEnd;

static char     *pULchar;
static char     *pULcharLimit;
static long     *pULcopy;

static char     *pULchar2;
static char     *pULcharLimit2;

static char     *pULcloseMir;

static long     *sp;
static unsigned short *spw, *spwBeg, *spwMax;

static char     *pULchar_g;

static char     *pULcharBest;

static char     *VERSTAG="\0$VER:CUTTER 2.6 (C) 1993/1994 Sculptured Software";

static char     outputName1Time[256] = "";

static BPTR     ConFH = 0;
static char     ConData[4];




// files:

static char     imageListFileName[256];
static char     imageFileName[256];
static char     masterFileName[256];
static char     packFileName[256];
static char     inputMasterFileName[256];
static char     charDefFileName[256];
static char     mImgTblIFileName[256];
static char     mImgTblFileName[256];
static char     miscDataFileName[256];
static char     imageTblFileName[256];
static char     charDataFileName[256];
static char     inDir[256]="";
static char     outDir[256]="";


FILE    *imageListFile;

long    packFile;
long    charDefFile;
long    mImgTblIFile;
long    mImgTblFile;
long    miscDataFile;
long    imageTblFile;
long    charDataFile;





// cutter controls:

#define NUM_CONTROLS 30

static long controlValueInit[NUM_CONTROLS] =
    {
    24,         // maxWidthHalf2Simul =      0+    0 will disable feature
     1,         // allowSimulCutInMiddle =   0:1   1 will catch more near mirror chars & takes longer
    48,         // maxWidth1Piece =          1+    
    49,         // minWidthToMirror =        0+    strongly suggest = maxWidth1Piece+1 always
     1,         // allowSlidingColumns =     0:1   strongly suggest 1 for both (must
     1,         // allowSlidingRows =        0:1     have at least 1 on)
    16,         // numYOffsets =             1:16  16 = full test (16x longer than 1)
     1,         // allowDownUp =             0:1   1 = slightly better
     1,         // downUpForAllYOffsets =    0:1   1 = slightly better (ignore if allowDownUp == 0)
     8,         // numXOffsets =             1:8   8 = full test
     1,         // allowSliding1x1 =         0:1   1 suggested (about same speed)
     1,         // allowSliding1x1OutOfRow = 0:1   1 cuts better but flicker has peaks (always 1 if sliding columns)
     0,         // proceedAfterSliding1x1 =  0;1   0 suggested (much faster, about as good)
     5,         // weightObj =               0+    Weights control 1x1 and 2x2 choice.
     1,         // weightFlicker =           0+      A Bigger number gives that feature
     8,         // weightVramChar =          0+      more importance (therefore the cutter
     2,         // weightRomChar =           0+      will try to minimize their occurance)
     0,         // allow1x1ToCopy2x2 =       0:1   1 allows a 1x1 to copy any vram char in a 2x2
     0,         // allow2Char2x2Cut =        0:1   use iff 2 chars in 2x2 is worth trying
     0,         // allow2x2WithLeftBlank =   0:1   ditto, ignored if allow2Char2x2Cut=0
     0,         // allow2x2WithRightBlank =  0:1   ditto, ignored if allow2Char2x2Cut=0                                

     0,         // flipInputX =              0:1   1 = x flip input image about x=128
     1,         // allowOutputFiles =        0:1   1 allows 7 files to be output per master image name
     0,         // allowShowCut =            0:1   1 = output detail of 1x1 and 2x2 cuts
     0,         // allowImageDump =          0:1   1 = output dump of image
     0,         // palette =                 0:7   (for output only)
     0,         // charNumOfst =             0:65535 offset to add to all char #'s

     0,         // only1x1 =                 0:1   1 = enable only 1x1s
     0,         // smallImageTbl =           0:1   1 = output small MiscData ImageTbl Indexes
     0          // priority =                0:3   (for output only)
    };

static char *controlWord[NUM_CONTROLS] =
    {
    "maxWidthHalf2Simul",       // maxWidthHalf2Simul =      0+    0 will disable feature
    "allowSimulCutInMiddle",    // allowSimulCutInMiddle =   0:1   1 will catch more near mirror chars & takes longer
    "maxWidth1Piece",           // maxWidth1Piece =          1+    
    "minWidthToMirror",         // minWidthToMirror =        0+    strongly suggest = maxWidth1Piece+1 always
    "allowSlidingColumns",      // allowSlidingColumns =     0:1   strongly suggest 1 for both (must have
    "allowSlidingRows",         // allowSlidingRows =        0:1     at least 1 on)
    "numYOffsets",              // numYOffsets =             1:16  16 = full test (16x longer than 1)
    "allowDownUp",              // allowDownUp =             0:1   1 = slightly better
    "downUpForAllYOffsets",     // downUpForAllYOffsets =    0:1   1 = slightly better (ignore if allowDownUp == 0)
    "numXOffsets",              // numXOffsets =             1:8   8 = full test (8x longer than 1)
    "allowSliding1x1",          // allowSliding1x1 =         0:1   1 suggested (about same speed)
    "allowSliding1x1OutOfRow",  // allowSliding1x1OutOfRow = 0:1   1 cuts better but flicker has peaks (always 1 if sliding columns)
    "proceedAfterSliding1x1",   // proceedAfterSliding1x1 =  0;1   0 suggested (much faster, about as good)
    "weightObj",                // weightObj =               0+    Weights control 1x1 and 2x2 choice.
    "weightFlicker",            // weightFlicker =           0+      A Bigger number gives that feature
    "weightVramChar",           // weightVramChar =          0+      more importance (therefore the cutter
    "weightRomChar",            // weightRomChar =           0+      will try to minimize their occurance)
    "allow1x1ToCopy2x2",        // allow1x1ToCopy2x2 =       0:1   1 allows a 1x1 to copy any vram char in a 2x2
    "allow2Char2x2Cut",         // allow2Char2x2Cut =        0:1   use iff 2 chars in 2x2 is worth trying
    "allow2x2WithLeftBlank",    // allow2x2WithLeftBlank =   0:1   ditto, ignored if allow2Char2x2Cut=0
    "allow2x2WithRightBlank",   // allow2x2WithRightBlank =  0:1   ditto, ignored if allow2Char2x2Cut=0                                

    "flipInputX",               // flipInputX =              0:1   1 = x flip input image about x=128
    "allowOutputFiles",         // allowOutputFiles =        0:1   1 allows 7 files to be output per master image name
    "allowDetailedListing",     // allowShowCut =            0:1   1 = output detail of 1x1 and 2x2 cuts
    "allowImageDump",           // allowImageDump =          0:1   1 = output dump of image
    "palette",                  // palette =                 0:7   (for output only)
    "charNumOfst",              // charNumOfst =             0:65535 offset to add to all char #'s

    "only1x1",                  // only1x1 =                 0:1   1 = enable only 1x1s
    "smallImageTbl",            // smallImageTbl =           0:1   1 = output small MiscData ImageTbl Indexes
    "priority"                  // priority =                0:3   (for output only)
    };

static long *controlValueAdrs[NUM_CONTROLS] =
    {
    &maxWidthHalf2Simul,        // maxWidthHalf2Simul =      0+    0 will disable feature
    &allowSimulCutInMiddle,     // allowSimulCutInMiddle =   0:1   1 will catch more near mirror chars & takes longer
    &maxWidth1Piece,            // maxWidth1Piece =          1+    
    &minWidthToMirror,          // minWidthToMirror =        0+    strongly suggest = maxWidth1Piece+1 always
    &allowSlidingColumns,       // allowSlidingColumns =     0:1   strongly suggest 1 for both (must have
    &allowSlidingRows,          // allowSlidingRows =        0:1     at least 1 on)
    &numYOffsets,               // numYOffsets =             1:16  16 = full test (16x longer than 1)
    &allowDownUp,               // allowDownUp =             0:1   1 = slightly better
    &downUpForAllYOffsets,      // downUpForAllYOffsets =    0:1   1 = slightly better (ignore if allowDownUp == 0)
    &numXOffsets,               // numXOffsets =             1:8   8 = full test (8x longer than 1)
    &allowSliding1x1,           // allowSliding1x1 =         0:1   1 suggested (about same speed)
    &allowSliding1x1OutOfRow,   // allowSliding1x1OutOfRow = 0:1   1 cuts better but flicker has peaks (always 1 if sliding columns)
    &proceedAfterSliding1x1,    // proceedAfterSliding1x1 =  0;1   0 suggested (much faster, about as good)
    &weightObj,                 // weightObj =               0+    Weights control 1x1 and 2x2 choice.
    &weightFlicker,             // weightFlicker =           0+      A Bigger number gives that feature
    &weightVramChar,            // weightVramChar =          0+      more importance (therefore the cutter
    &weightRomChar,             // weightRomChar =           0+      will try to minimize their occurance)
    &allow1x1ToCopy2x2,         // allow1x1ToCopy2x2 =       0:1   1 allows a 1x1 to copy any vram char in a 2x2
    &allow2Char2x2Cut,          // allow2Char2x2Cut =        0:1   use iff 2 chars in 2x2 is worth trying
    &allow2x2WithLeftBlank,     // allow2x2WithLeftBlank =   0:1   ditto, ignored if allow2Char2x2Cut=0
    &allow2x2WithRightBlank,    // allow2x2WithRightBlank =  0:1   ditto, ignored if allow2Char2x2Cut=0                                

    &flipInputX,                // flipInputX =              0:1   1 = x flip input image about x=128
    &allowOutputFiles,          // allowOutputFiles =        0:1   1 allows 7 files to be output per master image name
    &allowShowCut,              // allowShowCut =            0:1   1 = output detail of 1x1 and 2x2 cuts
    &allowImageDump,            // allowImageDump =          0:1   1 = output dump of image
    &palette,                   // palette =                 0:7   (for output only)
    &charNumOfst,               // charNumOfst =             0:65535 offset to add to all char #'s

    &only1x1,                   // only1x1 =                 0:1   1 = enable only 1x1s
    &smallImageTbl,             // smallImageTbl =           0:1   1 = output small MiscData ImageTbl Indexes
    &priority                   // priority =                0:3   (for output only)
    };

static long controlValueMin[NUM_CONTROLS] =
    {
    0,          // maxWidthHalf2Simul =      0+    0 will disable feature
    0,          // allowSimulCutInMiddle =   0:1   1 will catch more near mirror chars & takes longer
    1,          // maxWidth1Piece =          1+    
    0,          // minWidthToMirror =        0+    strongly suggest = maxWidth1Piece+1 always
    0,          // allowSlidingColumns =     0:1   strongly suggest 1 for both (must have
    0,          // allowSlidingRows =        0:1     at least 1 on)
    1,          // numYOffsets =             1:16  16 = full test (16x longer than 1)
    0,          // allowDownUp =             0:1   1 = slightly better
    0,          // downUpForAllYOffsets =    0:1   1 = slightly better (ignore if allowDownUp == 0)
    1,          // numXOffsets =             1:8   8 = full test (8x longer than 1)
    0,          // allowSliding1x1 =         0:1   1 suggested (about same speed)
    0,          // allowSliding1x1OutOfRow = 0:1   1 cuts better but flicker has peaks (always 1 if sliding columns)
    0,          // proceedAfterSliding1x1 =  0;1   0 suggested (much faster, about as good)
    0,          // weightObj =               0+    Weights control 1x1 and 2x2 choice.
    0,          // weightFlicker =           0+      A Bigger number gives that feature
    0,          // weightVramChar =          0+      more importance (therefore the cutter
    0,          // weightRomChar =           0+      will try to minimize their occurance)
    0,          // allow1x1ToCopy2x2 =       0:1   1 allows a 1x1 to copy any vram char in a 2x2
    0,          // allow2Char2x2Cut =        0:1   use iff 2 chars in 2x2 is worth trying
    0,          // allow2x2WithLeftBlank =   0:1   ditto, ignored if allow2Char2x2Cut=0
    0,          // allow2x2WithRightBlank =  0:1   ditto, ignored if allow2Char2x2Cut=0                                

    0,          // flipInputX =              0:1   1 = x flip input image about x=128
    0,          // allowOutputFiles =        0:1   1 allows 7 files to be output per master image name
    0,          // allowShowCut =            0:1   1 = output detail of 1x1 and 2x2 cuts
    0,          // allowImageDump =          0:1   1 = output dump of image
    0,          // palette =                 0:7   (for output only)
    0,          // charNumOfst =             0:65535 offset to add to all char #'s

    0,          // only1x1 =                 0:1   1 = enable only 1x1s
    0,          // smallImageTbl =           0:1   1 = output small MiscData ImageTbl Indexes
    0           // priority =                0:3   (for output only)
    };

static long controlValueMax[NUM_CONTROLS] =
    {
    256,        // maxWidthHalf2Simul =      0+    0 will disable feature
    1,          // allowSimulCutInMiddle =   0:1   1 will catch more near mirror chars & takes longer
    256,        // maxWidth1Piece =          1+    
    257,        // minWidthToMirror =        0+    strongly suggest = maxWidth1Piece+1 always
    1,          // allowSlidingColumns =     0:1   strongly suggest 1 for both (must have
    1,          // allowSlidingRows =        0:1     at least 1 on)
    16,         // numYOffsets =             1:16  16 = full test (16x longer than 1)
    1,          // allowDownUp =             0:1   1 = slightly better
    1,          // downUpForAllYOffsets =    0:1   1 = slightly better (ignore if allowDownUp == 0)
    8,          // numXOffsets =             1:8   8 = full test (8x longer than 1)
    1,          // allowSliding1x1 =         0:1   1 suggested (about same speed)
    1,          // allowSliding1x1OutOfRow = 0:1   1 cuts better but flicker has peaks (always 1 if sliding columns)
    1,          // proceedAfterSliding1x1 =  0;1   0 suggested (much faster, about as good)
    0x10000,    // weightObj =               0+    Weights control 1x1 and 2x2 choice.
    0x10000,    // weightFlicker =           0+      A Bigger number gives that feature
    0x10000,    // weightVramChar =          0+      more importance (therefore the cutter
    0x10000,    // weightRomChar =           0+      will try to minimize their occurance)
    1,          // allow1x1ToCopy2x2 =       0:1   1 allows a 1x1 to copy any vram char in a 2x2
    1,          // allow2Char2x2Cut =        0:1   use iff 2 chars in 2x2 is worth trying
    1,          // allow2x2WithLeftBlank =   0:1   ditto, ignored if allow2Char2x2Cut=0
    1,          // allow2x2WithRightBlank =  0:1   ditto, ignored if allow2Char2x2Cut=0                                

    1,          // flipInputX =              0:1   1 = x flip input image about x=128
    1,          // allowOutputFiles =s       0:1   1 allows 7 files to be output per master image name
    1,          // allowShowCut =            0:1   1 = output detail of 1x1 and 2x2 cuts
    1,          // allowImageDump =          0:1   1 = output dump of image
    7,          // palette =                 0:7   (for output only)
    65535,      // charNumOfst =             0:65535 offset to add to all char #'s

    1,          // only1x1 =                 0:1   1 = enable only 1x1s
    1,          // smallImageTbl =           0:1   1 = output small MiscData ImageTbl Indexes
    3           // priority =                0:3   (for output only)
    };


 

/*
void debugchk (void)
{

if (    
        p1x1Beg[UL_X] ==        0x59 &&
        p1x1Beg[UL_Y] ==        0x5e &&
        p1x1Beg[CHAR] ==        0x7795E9F &&
        p1x1Beg[COPY] ==        0x77EEC58 &&
        p1x1Beg[CHAR_SAVE] ==   0x7795E9F &&
        p1x1Beg[COPY_SAVE] ==   0x1C2     )
        
    {
    printf("\ndebugCnt = $%x\n", debugCnt);
    printf("p1x1Beg[CHAR] = $%x\n", p1x1Beg[CHAR]);
    printf("p1x1Beg[COPY] = $%x\n", p1x1Beg[COPY]);
    printf("p1x1Beg[CHAR_SAVE] = $%x\n", p1x1Beg[CHAR_SAVE]);
    printf("p1x1Beg[COPY_SAVE] = $%x\n", p1x1Beg[COPY_SAVE]);
    printf("\n");
    }

debugCnt++;
}
*/




void    main    (long numArgs_, char *args_[])
{

/*
    cut a list of images, and output in Randy's Monster Truck format
*/


printf("\n%s\n\n", VERSTAG+6 );

if (numArgs_ < 2)
    {
    printf("Usage is:  'avail flush' (optional)\n");
    printf("           'cut imageList'\n");
    printf("  where imageList is the input filename of image structures.\n");
    
    exit(1);
    }

allocMems();
initControls();
initPack();

strcpy(imageListFileName, args_[1]);
initImageList();

while (getImage() == 1)  // image exists
    {
    cut();
    showCut();
    packObjs();
    packCharData();
    packMiscData();

    if (GetChe() != 0)
        {
        Write( ConFH, "Waiting...", 10L);
        while (GetChe() != 0)
            {
            }
        while (GetChe() == 0)
            {
            }
        while (GetChe() != 0)
            {
            }
        Write( ConFH, "\r          \r", 12L);
        }

    //delay(1);
    }

exit(0);
}





char*   allocMem    (long align, long size)
{

/*
    align = 1,2,4 usually for char, short, long data (may be any # >= 1)
    size = #bytes to allocate
*/

static long    i;


i = ( (long)pMem + align - 1) / align * align;
pMem = (char*) (i + size);

if (pMem >= & mem[MEM_SIZE])
    {
    printf("Error: ran out of memory.  Increase MEM_SIZE and recompile\n");
    
    exit(1);
    }

return( (char*) i);
}





void    allocMems (void)
{


// init mem array to recognizable trash
setmem ( (void*) & mem[0], (unsigned) MEM_SIZE, (int) 0x5a);



if ( (long)pMem & 0xC0000 < 0x40000 )
    {
    pChar =             (char*)     allocMem (0x40000, 0x40000);
    pXChar =  pChar + 0x10000;
    pYChar =  pChar + 0x20000;
    pXYChar = pChar + 0x30000;
    
    // note only 0x100000 bytes needed for pCopy during cut, but this
    //   buffer is also used to convert pPack data to encoded bit planes,
    //   and Randy's program could handle up to 1.5 Meg worth of data
    pCopy =             (long*)     allocMem (0x100000, 0x180000);
    pXCopy =  pCopy + 0x10000; // (0x10000 longs)
    pYCopy =  pCopy + 0x20000;
    pXYCopy = pCopy + 0x30000;
    }
else
    {
    // note only 0x100000 bytes needed for pCopy during cut, but this
    //   buffer is also used to convert pPack data to encoded bit planes,
    //   and Randy's program could handle up to 1.5 Meg worth of data
    pCopy =             (long*)     allocMem (0x100000, 0x180000);
    pXCopy =  pCopy + 0x10000; // (0x10000 longs)
    pYCopy =  pCopy + 0x20000;
    pXYCopy = pCopy + 0x30000;
    
    pChar =             (char*)     allocMem (0x40000, 0x40000);
    pXChar =  pChar + 0x10000;
    pYChar =  pChar + 0x20000;
    pXYChar = pChar + 0x30000;
    }




// stack for misc. push/pops
sp =                (long*)     allocMem (4, SP_SIZE);
sp += SP_SIZE/4; // sp points to last data on stack
                                    // (which is none here)

// stack for misc. push/pops
spwBeg =  (unsigned short*)     allocMem (2, SPW_SIZE);
spwMax = spwBeg + SPW_SIZE/2;
spw = spwMax;


// sort buffer
pSortBeg =          (long*)     allocMem (4, SORT_SIZE);
pSortEnd = pSortBeg;
pSortMax = pSortBeg + SORT_SIZE/4;


// miscData buffer
pmd =        (unsigned char*)   allocMem (1, MISC_DATA_SIZE);
mdOfstEnd = 0;


// charData stuff
pCharDataBufBeg = (unsigned short*) allocMem (2, CHAR_DATA_BUF_SIZE);
pCharDataBufEnd = pCharDataBufBeg;
pCharDataBufMax = pCharDataBufBeg + CHAR_DATA_BUF_SIZE/2;

if ( CHAR_DATA_SIZE > 0XFFFC)
    {
    printf("Error: reduce CHAR_DATA_SIZE to 0xFFFC and recompile\n");
    exit(1);
    }
pcd =          (unsigned char*) allocMem (1, CHAR_DATA_SIZE);
cdOfstEnd = 0;


// imageTbl stuff
if ( IMAGE_TBL_SIZE > (0X10000 / 5 * 5) )
    {
    printf("Error: reduce IMAGE_TBL_SIZE to (0x10000 / 5 * 5) and recompile\n");
    exit(1);
    }
pImageTblBeg = (unsigned char*) allocMem (1, IMAGE_TBL_SIZE);
imageTblOfstEnd = 0;


// mImgTbl structure vars
pScaleBeg =                     allocMem (1, SCALE_SIZE);
pScaleMax = pScaleBeg + SCALE_SIZE;

pRotateBeg =                    allocMem (1, ROTATE_SIZE);
pRotateMax = pRotateBeg + ROTATE_SIZE;





// note 1x1's MUST be before 2x2's
p1x1Beg =           (long*)     allocMem (4, 4 * _1X1_SIZE * 32*32);
p1x1BestBeg =       (long*)     allocMem (4, 4 * _1X1_SIZE * 32*32);
p1x1BestEverBeg =   (long*)     allocMem (4, 4 * _1X1_SIZE * 32*32);

p2x2Beg =           (long*)     allocMem (4, 4 * _2X2_SIZE * 16*16);
p2x2BestBeg =       (long*)     allocMem (4, 4 * _2X2_SIZE * 16*16);
p2x2BestEverBeg =   (long*)     allocMem (4, 4 * _2X2_SIZE * 16*16);



//pPackBeg allocated from its own memory so it can grab up to 27 megs

//pPackBeg = & packMem[0];

pPackBeg = AllocMem(PACK_MEM_SIZE, MEMF_PUBLIC);
if (pPackBeg == 0L) {
	printf("Error: No Memory for PackArray!\n");
	exit(1);
}
packSize = PACK_MEM_SIZE;

}





void    chkCopyOfReassign   (long *p1x1_)
{

/*
    if 1x1 is copy of a reassigned 1x1, then update it
    This is to be called ONLY by showCut code, and not during cut
*/

static long    i_;
static long    *p2nd_;


// (if char is a copy of another 1x1)
i_ = p1x1_[COPY];
if ( i_ >= 0x10000 && i_ < (long)p2x2Beg) // note 1x1's defined BEFORE 2x2's !!!!
    {
    p2nd_ = (long*)i_ - CHAR;
    
    // if other 1x1 is a copy of 1x1 or 2x2
    i_ = p2nd_[COPY];
    if (i_ >= 0x10000)
        {
        if (i_ < (long)p2x2Beg) // if other 1x1 is a copy of 1x1
            {
            printf("Error: chkCopyOfReassign software error #1\n");
            exit(1);
            }
        
        // if x flip bit changed by reassignment
        if ( (p2nd_[CHAR] ^ p2nd_[CHAR_SAVE]) & 0x10000 ) 
            p1x1_[CHAR] = (p1x1_[CHAR] ^ 0x100ff) - 7; // then also do x flip

        // if y flip bit changed by reassignment
        if ( (p2nd_[CHAR] ^ p2nd_[CHAR_SAVE]) & 0x20000 ) 
            p1x1_[CHAR] = (p1x1_[CHAR] ^ 0x2ff00) - 0x700; // then also do y flip

        p1x1_[COPY] = p2nd_[COPY];
       
        // ensure char matches (it always should the 1st time) 
        if ( pixelMatch( (char*)p1x1_[CHAR], * (char**)p1x1_[COPY]) == 0 )
            {
            printf("Error: chkCopyOfReassign software error #2\n");
            exit(1);
            }
        }
    }
}       

                


                
void    chkMir  (void)
{

/*
    start the cut of mirrored objs which were not auto-cut since they
      are close to the mirror line
*/


if (mirror)
    {
    if ( (long)p1x1CloseMirBeg == -1 ) // not set yet
        {
        p1x1CloseMirBeg = p1x1End;
        p2x2CloseMirBeg = p2x2End;
        }

    // get start of mirror objs which have not had their weight added:

    p1x1Mir = p1x1CloseMirBeg;
    p2x2Mir = p2x2CloseMirBeg;

    p1x1CloseMirEnd = p1x1End;    
    p2x2CloseMirEnd = p2x2End;    

    // (you must continue cutting while x (before mirror, before flip)
    //  is <= this (also check ...CloseMirEnd's)
    xLimitMir = rightXMirCenter -
             ( ( (long)pULchar & 0xff ) - leftXMirCenter );
    
    
    // debug check
    *--sp = weight;

    cutMir(); // (cut best mirror combination, check best weight)


    // debug check
    if (*sp++ != weight)
        {
        printf("Error: weight changed software error #1\n");
        exit(1);
        }

            
    }
else
    chkWeight();
}





long    chkRowStatus    (void)
{

/* 
    Input:
        leftX
        rightX
        topYRow

    Will return:
        0 if row totally blank
        1 if row contains data

        leftXRow        x of leftmost column containing non-transparent data
    
        rightXRow       x of rightmost column containing non-transparent data

        leftXMirCenter: leftXRow   + ( (rightXRow-leftXRow) >> 1)

        rightXMirCenter: rightXRow - ( (rightXRow-leftXRow) >> 1)

        mirror:         0 if row is not a mirror
                        1 if row is an x-flip mirror

        pULcloseMir     if pULchar > pULcloseMir when obj cut then there will
                            be no auto-add of mirror weights as the mirror obj
                            could be cut differently
*/

static long    *pCopy_;
static long    x_, y_;
static char    *pL_, *pR_;

// find left edge:

pCopy_ = pCopy + ( (topYRow << 8) + leftX - 7 );
for (leftXRow = leftX ; leftXRow <= rightX ; leftXRow++)
    {
    // (if column at x==leftXRow not transparent)
    if (*pCopy_ || pCopy_[0x800] )
        goto gotLeft;
    pCopy_++;
    }
return(0); // (row totally blank)

gotLeft:

pCopy_ = pCopy + ( (topYRow << 8) + rightX );
for (rightXRow = rightX ; 1 ; rightXRow--)
    {
    // (if column at x==rightXRow not transparent)
    if (*pCopy_ ||  pCopy_[0x800] )
        goto gotRight;
    pCopy_--;
    }

gotRight:

leftXMirCenter =  leftXRow  + ( (rightXRow-leftXRow) >> 1);

rightXMirCenter = rightXRow - ( (rightXRow-leftXRow) >> 1);

// check if row is a mirror image from left to right:

for (y_ = topYRow ; y_ <= topYRow+15 ; y_++)
    {
    pL_ = pChar + ( (y_ << 8) + leftXRow );
    pR_ = pChar + ( (y_ << 8) + rightXRow );
    while (pL_ < pR_)
        {
        if (*pL_++ != *pR_--)
            {
            mirror = 0;
            goto noMirror;
            }
        }
    }

// row is a mirror
mirror = 1;

// patch to disable mirror code if row is small enough.
//   This is done to enable 1x1's to slide out of a row
//   for a better cut (they can't if mirroring is on)

if (rightXRow - leftXRow + 1 < minWidthToMirror)
    mirror = 0;

noMirror:

if (mirror)
    {

    // if pULchar <= pULcloseMir then can auto-cut mirrored obj

    x_ = rightXMirCenter - 15 - 8;
    if (x_ < 0)
        x_ = 0;
    }
else
    x_ = 0;
    
pULcloseMir = pChar + ( (topYRow << 8) + x_ );

return(1); // row contains data
}





void    chkWeight   (void)
{

/*
    if this is new best weight so far
    then
        remember weight
        if mirroring
        then 
            cut mirrored objs that were not cut by cutMir()
        save 1x1 and 2x2 obj lists
        if mirroring
        then 
            uncut mirrored objs that were not cut by cutMir()
*/

static long    *p1x1Mir_, *p2x2Mir_;
static long    i_;


if (weight >= bestWeight)
    return;

bestWeight = weight;

if (mirror)
    {
    
    *--sp = (long)p1x1End;
    *--sp = (long)p2x2End;

    for (p1x1Mir_ = p1x1RowBeg ; p1x1Mir_ < p1x1CloseMirBeg ;
                 p1x1Mir_ += _1X1_SIZE)
        cut1x1Mir(p1x1Mir_, COPY, p1x1Mir_[UL_Y]);
        
    for (p2x2Mir_ = p2x2RowBeg ; p2x2Mir_ < p2x2CloseMirBeg ;
                     p2x2Mir_ += _2X2_SIZE)
        cut2x2Mir(p2x2Mir_); // (no effect on weight)
    }

// move mem from p1x1Beg to p1x1BestBeg
i_ = (long)p1x1End - (long)p1x1Beg;
if (i_ > 0)
    movmem (p1x1Beg, p1x1BestBeg, (unsigned) i_);
p1x1BestEnd = (long*) ( (long)p1x1BestBeg + i_ );

// move mem from p2x2Beg to p2x2BestBeg
i_ = (long)p2x2End - (long)p2x2Beg;
if (i_ > 0)
    movmem (p2x2Beg, p2x2BestBeg, (unsigned) i_);
p2x2BestEnd = (long*) ( (long)p2x2BestBeg + i_ );

pULcharBest = pULchar; // save in case a row is being cut in 2 seperate parts

if (mirror)
    {
    p2x2End = (long*) *sp++;
    p1x1End = (long*) *sp++;
    }

}





void    copyImageAllFlips   (void)
{

// copy image into 3 other image arrays for 3 other flips

static char    *p_, *pX_, *pY_, *pXY_;


p_ =    pChar;
pX_ =   (char*) ( (long)pXChar  ^ 0xff   );
pY_ =   (char*) ( (long)pYChar  ^ 0xff00 );
pXY_ =  (char*) ( (long)pXYChar ^ 0xffff );

while (p_ < pXChar) // (note pChar is followed by pXChar!)
    {
    *pX_--  = *p_;
    *pY_++  = *p_;
    *pXY_-- = *p_++;

    if ( ( (long)p_ & 0xff ) == 0 ) // (finished 1 row)
        {
        pX_ += 0x200;
        pY_ -= 0x200;
        }
    }

return;
}





void    cut (void)
{

static clock_t  start_, end_;


// set time starting cut
start_ = clock();

printf("        Find packed copies for all chars\n");

// declare nothing cut yet
p1x1BestEverEnd = p1x1BestEverBeg;
p2x2BestEverEnd = p2x2BestEverBeg;
bestEverWeight = 0x7fffffff;

if (setImageBounds() == 0) // test for blank image
    return;

copyImageAllFlips(); // copy image to other 3 arrays for 3 flips

//find transparent & packed char copies, no flip:
setCopyArray (pChar, pCopy, leftX, rightX, topY, bottomY);

// find transparent & packed char copies, x flip:
setCopyArray (pXChar, pXCopy, rightX ^ 0xff, leftX ^ 0xff, topY, bottomY);

// find transparent & packed char copies, y flip:
setCopyArray (pYChar, pYCopy, leftX, rightX, bottomY ^ 0xff, topY ^ 0xff);

// find transparent & packed char copies, xy flip:
setCopyArray (pXYChar, pXYCopy, rightX ^ 0xff, leftX ^ 0xff,
                bottomY ^ 0xff, topY ^ 0xff);

// debug printout of image as hex dump
dumpImage("\nImage initially input:\n");

// if a tie in cut weights between sliding rows and columns, then the one
//   done 1st will prevail.  Note that either the rows or columns
//   technique could be done 1st.

if (allowSlidingColumns)
    {
    columns = 1;
    cutImageSlidingRows(); // (cut image, using sliding columns idea)
    }

if (allowSlidingRows)
    {
    columns = 0;
    cutImageSlidingRows(); // (cut image, using sliding rows idea)
    }

// measure time taken in cut:
end_ = clock();

printf("        Cut took %d seconds\n",
        (end_ - start_ + CLOCKS_PER_SEC / 2) / CLOCKS_PER_SEC);
if ( (unsigned long) (sumOfWeightsLo + bestEverWeight) < sumOfWeightsLo )
    sumOfWeightsHi++;
sumOfWeightsLo += bestEverWeight;

if (sumOfWeightsHi != 0)
    printf("        Sum of weights = $%x%8x\n", sumOfWeightsHi, sumOfWeightsLo);
else
    printf("        Sum of weights = $%x\n", sumOfWeightsLo);

}





long    cut1x1AllFlips  (void)
{

/*
    ul_x_g, ul_y_g, pULchar_g are inputs
    
    Find best cut of 1x1 among 4 flips
    Note char is not transparent (checked before call)
    Note this code is not to be used for cutting a mirrored obj

Will return:
    0   best flip cut has weight >= bestEverWeight, so you must abort early
            (note 1x1 is uncut by this code)
    1   best flip cut has ok weight, caller can continue normally
*/

static long    bestFlip_, bestWeightFlip_;


// bestFlip_ need not be init'd
bestWeightFlip_ = bestWeight;

if (cut1x1XFlip() == 1) // (continue)
    {
    if (weight < bestWeightFlip_)
        {
        bestFlip_ = 1;
        bestWeightFlip_ = weight;
        }
    undoCut1x1();
    }

if (cut1x1YFlip() == 1) // (continue)
    {
    if (weight < bestWeightFlip_)
        {
        bestFlip_ = 2;
        bestWeightFlip_ = weight;
        }
    undoCut1x1();
    }

if (cut1x1XYFlip() == 1) // (continue)
    {
    if (weight < bestWeightFlip_)
        {
        bestFlip_ = 3;
        bestWeightFlip_ = weight;
        }
    undoCut1x1();
    }

if (cut1x1NoFlip() == 1) // (continue)
    {
    if (weight <= bestWeightFlip_)
        {
        bestFlip_ = 0;
        bestWeightFlip_ = weight;
        }
        
    // (note cut not undone)
    
    if (bestFlip_ != 0)
        {
        undoCut1x1();
        if (bestFlip_ == 1) 
            cut1x1XFlip(); // (ignore return value)
        else
            {
            if (bestFlip_ == 2)
                cut1x1YFlip(); // (ignore return value)
            else
                cut1x1XYFlip(); // (ignore return value)
            }
        }

    return(1); // (caller can continue)
    }

if (bestWeightFlip_ == bestWeight)
    return(0); // (caller must abort, no cut done)

if (bestFlip_ != 0)
    {
    if (bestFlip_ == 1) 
        cut1x1XFlip(); // (ignore return value)
    else
        {
        if (bestFlip_ == 2)
            cut1x1YFlip(); // (ignore return value)
        else
            cut1x1XYFlip(); // (ignore return value)
        }
    }

return(1); // (caller can continue)
}





long    cut1x1NoFlip    (void)
{

/*
    cut a 1x1, with no flip
    Note this code is not to be used for cutting a mirrored obj

    will return     0: caller must abort early, 1x1 uncut here
                    1: caller can continue, weight ok
*/


p1x1End[UL_X]   =   ul_x_g;
p1x1End[UL_Y]   =   ul_y_g;
p1x1End[CHAR]   =   (long)pULchar_g;

return  (cut1x1());
}





long    cut1x1XFlip (void)
{

/*
    cut a 1x1, with x flip
    Note this code is not to be used for cutting a mirrored obj

    will return     0: caller must abort early, 1x1 uncut here
                    1: caller can continue, weight ok
*/


p1x1End[UL_X]   =   ul_x_g;
p1x1End[UL_Y]   =   ul_y_g;
p1x1End[CHAR]   =   ( (long)pULchar_g ^ 0x100ff ) - 0x007;

return  (cut1x1());
}





long    cut1x1YFlip (void)
{

/*
    cut a 1x1, with y flip
    Note this code is not to be used for cutting a mirrored obj

    will return     0: caller must abort early, 1x1 uncut here
                    1: caller can continue, weight ok
*/


p1x1End[UL_X]   =   ul_x_g;
p1x1End[UL_Y]   =   ul_y_g;
p1x1End[CHAR]   =   ( (long)pULchar_g ^ 0x2ff00 ) - 0x700;

return  (cut1x1());
}





long    cut1x1XYFlip    (void)
{

/*
    cut a 1x1, with xy flip
    Note this code is not to be used for cutting a mirrored obj

    will return     0: caller must abort early, 1x1 uncut here
                    1: caller can continue, weight ok
*/


p1x1End[UL_X]   =   ul_x_g;
p1x1End[UL_Y]   =   ul_y_g;
p1x1End[CHAR]   =   ( (long)pULchar_g ^ 0x3ffff ) - 0x707;

return  (cut1x1());
}





long    cut1x1  (void)
{

/*

    set before call:
        p1x1End[UL_X]   
        p1x1End[UL_Y]
        p1x1End[CHAR]
        
    this will cut a 1x1 obj and char and update weights
    don't call if char is transparent
    Note this code is not to be used for cutting a mirrored obj

    note when obj is to be flipped, the flipped versions of the char
        must be passed
    note p1x1End[UL_COPY] = -1 or 0:64K-1 on input

    will return     0: caller must abort early, 1x1 uncut here
                    1: caller can continue, weight ok
*/

static long    *p1x1_, *p2x2_;


p1x1End[COPY] = pCopy[ p1x1End[CHAR] & 0x3ffff ];

weight += weightObj + weightFlicker;
if (pULchar <= pULcloseMir)
    weight += weightObj + weightFlicker;
//(vram and rom weight done below)

if (weight >= bestWeight)
    {
    
    cantCut:

    weight -= weightObj + weightFlicker;
    if (pULchar <= pULcloseMir)
        weight -= weightObj + weightFlicker;
    return(0); // (caller must abort early)
    }

// check to see if this char is a copy of another new 1x1 in the
//  image being cut:

for (p1x1_ = p1x1End - _1X1_SIZE ; p1x1_ >= p1x1Beg ; p1x1_ -= _1X1_SIZE)

    if (p1x1_[COPY] < 0x10000) // (if new char or copy of packed char)
        {

        // then see if match with char:
        // (note 1x1 chars never transparent)

        if (pixelMatch( (char*)p1x1End[CHAR], (char*)p1x1_[CHAR]) )
            {

            // (duplicate 1x1 found!)

            // (no vram or rom weight added)
            
            p1x1End[COPY] = (long) (p1x1_ + CHAR);
        
            p1x1End += _1X1_SIZE;

            return(1);
            }
        }

// (no 1x1 match found)

// check to see if this char is a copy of another char within
//  a 2x2 of the image being cut:

for (p2x2_ = p2x2End - _2X2_SIZE ; p2x2_ >= p2x2Beg ; p2x2_ -= _2X2_SIZE)
    {

    if (p2x2_[_2X2_COPY] < 0) // (if new obj, vram exists)
        {
    
        if (p2x2_[UL_COPY] < 0x10000) // (if new char or packed copy
            {

            // see if match with char:

            if (pixelMatch( (char*)p1x1End[CHAR], (char*)p2x2_[UL_CHAR]) )
                {

                // (duplicate char in 2x2 found!)

                if (!allow1x1ToCopy2x2)
                    {
                    if (weight + weightVramChar >= bestWeight)
                        goto cantCut;
                    weight += weightVramChar; // save rom char only
                    }
                    
                // (rom weight added)
            
                p1x1End[COPY] = (long) (p2x2_ + UL_CHAR);
        
                p1x1End += _1X1_SIZE;

                return(1);
                }
            }
        
        if (p2x2_[LL_COPY] < 0x10000) // (if new char or packed copy
            {

            // see if match with char:

            if (pixelMatch( (char*)p1x1End[CHAR], (char*)p2x2_[LL_CHAR]) )
                {

                // (duplicate char in 2x2 found!)

                if (!allow1x1ToCopy2x2)
                    {
                    if (weight + weightVramChar >= bestWeight)
                        goto cantCut;
                    weight += weightVramChar; // save rom char only
                    }
                    
                // (rom weight added)
            
                p1x1End[COPY] = (long)(p2x2_ + LL_CHAR);
        
                p1x1End += _1X1_SIZE;

                return(1);
                }
            }
        
        if (p2x2_[UR_COPY] < 0x10000) // (if new char or packed copy
            {

            // see if match with char:

            if (pixelMatch( (char*)p1x1End[CHAR], (char*)p2x2_[UR_CHAR]) )
                {

                // (duplicate char in 2x2 found!)

                if (!allow1x1ToCopy2x2)
                    {
                    if (weight + weightVramChar >= bestWeight)
                        goto cantCut;
                    weight += weightVramChar; // save rom char only
                    }
                    
                // (rom weight added)
            
                p1x1End[COPY] = (long)(p2x2_ + UR_CHAR);
        
                p1x1End += _1X1_SIZE;

                return(1);
                }
            }
        
        if (p2x2_[LR_COPY] < 0x10000) // (if new char or packed copy
            {

            // see if match with char:

            if (pixelMatch( (char*)p1x1End[CHAR], (char*)p2x2_[LR_CHAR]) )
                {

                // (duplicate char in 2x2 found!)

                if (!allow1x1ToCopy2x2)
                    {
                    if (weight + weightVramChar >= bestWeight)
                        goto cantCut;
                    weight += weightVramChar; // save rom char only
                    }
                    
                // (rom weight added)
            
                p1x1End[COPY] = (long)(p2x2_ + LR_CHAR);
        
                p1x1End += _1X1_SIZE;

                return(1);
                }
            }
        }
    }
        
// (no 1x1 or 2x2 char match found)

weight += weightVramChar;
// (no effect for mirror)
// (rom weight done below)

// check if char exists in packed chars

if (p1x1End[COPY] >= 0) // (0:<64K) (if packed copy exists)
    {
    // (no rom weight added)

    // (note p1x1End[COPY] already points to packed copy)
    }
else
    {
    // (no char copy anywhere, thus new char)

    weight += weightRomChar;
    // (no effect for mirror)

    // (note p1x1End[COPY] already is -1)
    }

p1x1End += _1X1_SIZE;

if (weight >= bestWeight)
    {
    undoCut1x1();
    return(0); // (caller must abort)
    }

return(1); // (call can continue)
}





void    cut1x1Mir   (long *p1x2_, long copy_, long ul_y_)
{

/*
    cut 1x1 mirror obj for EITHER a 1x1 or 1 char of a 2x2
*/

static long    char_;


p1x1End[UL_X] = rightXMirCenter + (leftXMirCenter - p1x2_[UL_X]) - 7;
p1x1End[UL_Y] = ul_y_; // (was p1x2_[UL_Y])

char_ = copy_ + CHAR-COPY;

p1x1End[CHAR] = (long) ( pChar + ( ( p1x1End[UL_Y] << 8 ) + p1x1End[UL_X] ) );
if ( (p1x2_[char_] & 0x10000) == 0 ) // if original 1x1 has no x flip
    p1x1End[CHAR] = ( p1x1End[CHAR] ^ 0x100ff ) - 7; // then x flip mirror
if (p1x2_[char_] & 0x20000) // if original 1x1 has y flip
    p1x1End[CHAR] = ( p1x1End[CHAR] ^ 0x2ff00 ) - 0x700; // then y flip mirror

if (p1x2_[copy_] < 0x10000) // if new or packed copy
    // point to original 1x1[CHAR]
    p1x1End[COPY] = (long)(p1x2_ + char_);
else
    // point to where original 1x1 is pointing to
    p1x1End[COPY] = p1x2_[copy_];

p1x1End += _1X1_SIZE;
}





long    cut2x2AllFlips  (void)
{

/*  
    must try all 4 flips of 2x2 to find best one:
    Note input chars are not flipped
    note rightColumnBlank = 0 (normally) or 1 to force right column to be blank

    Will return:
        0   caller must abort, best weight too much (nothing cut)
        1   call can continue, best weight ok
*/

static long    bestFlip_;
static long    bestWeightFlip_;


// bestFlip_ need not be init'd
bestWeightFlip_ = bestWeight;

if (cut2x2XFlip() == 1) // (continue)
    {
    if (weight < bestWeightFlip_)
        {
        bestFlip_ = 1;
        bestWeightFlip_ = weight;
        }
    undoCut2x2();
    }

if (cut2x2YFlip() == 1) // (continue)
    {
    if (weight < bestWeightFlip_)
        {
        bestFlip_ = 2;
        bestWeightFlip_ = weight;
        }
    undoCut2x2();
    }

if (cut2x2XYFlip() == 1) // (continue)
    {
    if (weight < bestWeightFlip_)
        {
        bestFlip_ = 3;
        bestWeightFlip_ = weight;
        }
    undoCut2x2();
    }

if (cut2x2NoFlip() == 1) // (continue)
    {
    if (weight <= bestWeightFlip_)
        {
        bestFlip_ = 0;
        bestWeightFlip_ = weight;
        }
        
    // (note cut not undone)
    
    if (bestFlip_ != 0)
        {
        undoCut2x2();
        if (bestFlip_ == 1) 
            cut2x2XFlip(); // (ignore return value)
        else
            {
            if (bestFlip_ == 2)
                cut2x2YFlip(); // (ignore return value)
            else
                cut2x2XYFlip(); // (ignore return value)
            }
        }

    return(1); // (caller can continue)
    }

if (bestWeightFlip_ == bestWeight)
    return(0); // (caller must abort, nothing cut)

if (bestFlip_ != 0)
    {
    if (bestFlip_ == 1) 
        cut2x2XFlip(); // (ignore return value)
    else
        {
        if (bestFlip_ == 2)
            cut2x2YFlip(); // (ignore return value)
        else
            cut2x2XYFlip(); // (ignore return value)
        }
    }

return(1); // (caller can continue)
}





long    cut2x2NoFlip    (void)
{

/*
    cut a 2x2, with no flip

    will return:    0 if caller must abort early, 2x2 uncut here
                    1 if caller can continue, weight ok
*/


p2x2End[UL_X]    =  (long)pULchar & 0xff;
p2x2End[UL_Y]    =  ( (long)pULchar & 0xff00) >> 8;

p2x2End[UL_CHAR] =  (long)(pULchar + 0x000);
p2x2End[LL_CHAR] =  (long)(pULchar + 0x800);
p2x2End[UR_CHAR] =  (long)(pULchar + 0x008);
p2x2End[LR_CHAR] =  (long)(pULchar + 0x808);                  

if (rightColumnBlank)
    {
    p2x2End[UR_CHAR] =  (long)pChar; // (blank, no flip)
    p2x2End[LR_CHAR] =  (long)pChar; // (blank, no flip)
    }
    
return(cut2x2());
}





long    cut2x2XFlip (void)
{

/*
    cut a 2x2, with x flip

    will return:    0 if caller must abort early, 2x2 uncut here
                    1 if caller can continue, weight ok
*/

static long     i_;


p2x2End[UL_X]    =  (long)pULchar & 0xff;
p2x2End[UL_Y]    =  ( (long)pULchar & 0xff00) >> 8;

i_ = (long)pULchar ^ 0x100ff;

p2x2End[UL_CHAR] =  i_ - 0x00f;
p2x2End[LL_CHAR] =  i_ + 0x800 - 0x00f;
p2x2End[UR_CHAR] =  i_ - 0x007;
p2x2End[LR_CHAR] =  i_ + 0x800 - 0x007;
    
if (rightColumnBlank)
    {
    p2x2End[UL_CHAR] =  (long)pXChar; // (blank, x flip bit)
    p2x2End[LL_CHAR] =  (long)pXChar; // (blank, x flip bit)
    }
    
return(cut2x2());
}





long    cut2x2YFlip (void)
{

/*
    cut a 2x2, with y flip

    will return:    0 if caller must abort early, 2x2 uncut here
                    1 if caller can continue, weight ok
*/


static long     i_;


p2x2End[UL_X]    =  (long)pULchar & 0xff;
p2x2End[UL_Y]    =  ( (long)pULchar & 0xff00 ) >> 8;
    
i_ = (long)pULchar ^ 0x2ff00;

p2x2End[UL_CHAR] =  i_ - 0xf00;
p2x2End[LL_CHAR] =  i_ - 0x700;
p2x2End[UR_CHAR] =  i_ - 0xf00 + 0x008;
p2x2End[LR_CHAR] =  i_ - 0x700 + 0x008;
    
if (rightColumnBlank)
    {
    p2x2End[UR_CHAR] =  (long)pYChar; // (blank, y flip bit)
    p2x2End[LR_CHAR] =  (long)pYChar; // (blank, y flip bit)        
    }
    
return(cut2x2());
}





long    cut2x2XYFlip    (void)
{

/*
    cut a 2x2, with x&y flip

    will return:    0 if caller must abort early, 2x2 uncut here
                    1 if caller can continue, weight ok
*/


static long     i_;


p2x2End[UL_X]    =  (long)pULchar & 0xff;
p2x2End[UL_Y]    =  ( (long)pULchar & 0xff00 ) >> 8;

i_ = (long)pULchar ^ 0x3ffff;

p2x2End[UL_CHAR] =  i_ - 0xf0f;
p2x2End[LL_CHAR] =  i_ - 0x70f;
p2x2End[UR_CHAR] =  i_ - 0xf07;
p2x2End[LR_CHAR] =  i_ - 0x707;

if (rightColumnBlank)
    {
    p2x2End[UL_CHAR] =  (long)pXYChar; // (blank, xy flip bits)
    p2x2End[LL_CHAR] =  (long)pXYChar; // (blank, xy flip bits)
    }
    
return(cut2x2());
}





long    cut2x2  (void)
{

/*
    this will cut a 2x2 obj and chars and update weights

    note when obj is to be flipped, the chars must be switched in
        position, and the flipped versions of the chars must be passed

    will return:    0 if caller must abort early (2x2 already uncut)
                    1 if call can continue (weight ok)
*/

static long    *p2x2_;


// get copy data
p2x2End[UL_COPY] = pCopy[ p2x2End[UL_CHAR] & 0x3ffff ];
p2x2End[LL_COPY] = pCopy[ p2x2End[LL_CHAR] & 0x3ffff ];
p2x2End[UR_COPY] = pCopy[ p2x2End[UR_CHAR] & 0x3ffff ];
p2x2End[LR_COPY] = pCopy[ p2x2End[LR_CHAR] & 0x3ffff ];

weight += weightObj + weightFlicker*4;
if (pULchar <= pULcloseMir)
    weight += weightObj + weightFlicker*4;
// (vram and rom weight done below)
    
if (weight >= bestWeight)
    {
    weight -= weightObj + weightFlicker*4;
    if (pULchar <= pULcloseMir)
        weight -= weightObj + weightFlicker*4;
    return(0); // (caller must abort early)
    }

p2x2End[UL_REASSIGN] = -1; // (assume no reassignments)
p2x2End[LL_REASSIGN] = -1;
p2x2End[UR_REASSIGN] = -1;
p2x2End[LR_REASSIGN] = -1;

// check to see if this 2x2 can use a copy of another 2x2 in the
//  image being cut:

for (p2x2_ = p2x2End - _2X2_SIZE ; p2x2_ >= p2x2Beg ; p2x2_ -= _2X2_SIZE)
    {
    if ( p2x2_[_2X2_COPY] < 0) // (if no vram copy, thus vram used)
        {

        // see if match with 4 chars:

        if (pixelMatch( (char*)p2x2End[UL_CHAR], (char*)p2x2_[UL_CHAR]) &&
            pixelMatch( (char*)p2x2End[LL_CHAR], (char*)p2x2_[LL_CHAR]) &&
            pixelMatch( (char*)p2x2End[UR_CHAR], (char*)p2x2_[UR_CHAR]) &&
            pixelMatch( (char*)p2x2End[LR_CHAR], (char*)p2x2_[LR_CHAR]) )

            {
            // (duplicate 2x2 found!)
            // (no vram or rom weight added)
            
            p2x2End[_2X2_COPY] = (long)p2x2_;
            p2x2End += _2X2_SIZE;
            return(1);
            }
        }
    }

// (no 2x2 match found)

weight += weightVramChar*4;
// (note it is remotely possible that each char could reassign a 1x1 and
//  thus no effective weight would be added)
// (no vram effect for mirror)
// (rom weight done below)

p2x2End[_2X2_COPY] = -1; // (no 2x2 copy, thus vram used)

// now we need to find where 4 chars are in rom:

// for each of 4 chars:

p2x2End[UL_COPY] = cut2x2Sub(p2x2End + UL_CHAR,0);

p2x2End[LL_COPY] = cut2x2Sub(p2x2End + LL_CHAR,1);

p2x2End[UR_COPY] = cut2x2Sub(p2x2End + UR_CHAR,2);

p2x2End[LR_COPY] = cut2x2Sub(p2x2End + LR_CHAR,3);

p2x2End += _2X2_SIZE;

if (weight >= bestWeight)
    {
    undoCut2x2();
    return(0); // (caller must abort)
    }

return(1); // (call may continue)
}





long    cut2x2Sub   (long *pCharIndex_, long _2x2Flag_)
{

/*
    pCharIndex_ = adrs of 2x2[UL_CHAR] (or LL,UR,LR) entry
    note 2x2 can already be flipped
    _2x2Flag_:  0 = ignore
                1:3 = #chars to allow to check in last 2x2 just being
                    created (not yet declared allocated)

    will return:
        (neg)   vram used for new char
        0:64K-1 vram used, packed copy
        >= 128K no vram used, adrs of copied 2x2[xx_CHAR]

main flow is:

if transparent
then
    already declared so, done
else
    if any previous new 1x1 char can be reassigned
            as copy of this 2x2 char
    then
        do so.  (save 1 vram char).  2x2 char is new or packed copy
        (note there won't be a copy in another 2x2)
    else
        if char exists in packed chars
        then
            use copy of it (already declared so, thus no code needed)
        else
            if char exists in any previous new 2x2
            then
                use copy of it
            else
                this is new 2x2 char (add rom weight)
*/              

static long    *pCopy_, *p1x1_, *p2x2_;
static char    *pChar_, *pChar1x1_;


pCopy_ = (long*) *(pCharIndex_ + UL_COPY-UL_CHAR);
if (pCopy_ == 0) // (blank)
    return(0);

pChar_ = (char*) *pCharIndex_; // (get adrs of char as would appear in vram)

// scan 1x1's to reassign a 1x1 char to the 2x2 char:
        
for (p1x1_ = p1x1End - _1X1_SIZE ; p1x1_ >= p1x1Beg ; p1x1_ -= _1X1_SIZE)
    {
    if ( p1x1_[COPY] < 0x10000 ) // (if new char or packed copy)
        {

        // see if char match:

        // (note a 1x1 char will never be transparent)

        pChar1x1_ = (char*) p1x1_[CHAR]; // (char as appears in vram)
        if (pixelMatch( pChar_, pChar1x1_) )
            {

            reassign:


            p1x1_[CHAR_SAVE] = p1x1_[CHAR]; // (save value)
            p1x1_[CHAR] = (long)pChar1x1_;

            if (allow1x1ToCopy2x2)
                weight -= weightVramChar; // (saved previous 1x1 vram char)
            // (no vram effect for mirror)
            // (rom weight not added)

            p1x1_[COPY_SAVE] = p1x1_[COPY]; // (save value)
            p1x1_[COPY] = (long)pCharIndex_;
            
            *(pCharIndex_ + (UL_REASSIGN - UL_CHAR) ) = (long) p1x1_;
                // (set 2x2[xx_REASSIGN])

            return( *(pCharIndex_ + (UL_COPY - UL_CHAR) ) ); // (new 2x2 or
                    // copy of packed char)
            }

        // try 3 other flips of 1x1 to get reassignment:

        // try x flip:

        pChar1x1_ = (char*) ( ( (long)pChar1x1_ ^ 0x100ff ) - 0x007);
        if (pixelMatch(pChar_, pChar1x1_) )
            goto reassign;

        // try xy flip:

        pChar1x1_ = (char*) ( ( (long)pChar1x1_ ^ 0x2ff00 ) - 0x700);
        if (pixelMatch(pChar_, pChar1x1_) )
            goto reassign;

        // try y flip:

        pChar1x1_ = (char*) ( ( (long)pChar1x1_ ^ 0x100ff ) - 0x007);
        if (pixelMatch(pChar_, pChar1x1_) )
            goto reassign;
        }
    }

// (no 1x1 match found)

// check if packed char exists:

if ( (long)pCopy_ >= 0 ) // (packed char exists)
    return( (long)pCopy_);

// scan 2x2's for a match with any of 4 chars:
            
for (p2x2_ = (_2x2Flag_ != 0) ? p2x2End : p2x2End-_2X2_SIZE
        ; p2x2_ >= p2x2Beg ; p2x2_ -= _2X2_SIZE)
    {
    if (p2x2_[_2X2_COPY] < 0) // (if new obj, thus vram used)
        {

        // see if match with any chars:

        if (p2x2_[UL_COPY] < 0 /* new char */ &&
                pixelMatch(pChar_, (char*) p2x2_[UL_CHAR]) )
            
            return( (long)(p2x2_ + UL_CHAR) ); // (match!)
            // (no rom weight added)

        if (p2x2_[LL_COPY] < 0 /* new char */ &&
                _2x2Flag_ != 1 &&
                pixelMatch(pChar_, (char*)p2x2_[LL_CHAR]) )
            
            return( (long)(p2x2_ + LL_CHAR) ); // (match!)
            // (no rom weight added)

        if (p2x2_[UR_COPY] < 0 /* new char */ &&
            (_2x2Flag_ == 0 || _2x2Flag_ == 3) &&
            pixelMatch(pChar_, (char*)p2x2_[UR_CHAR]) )
        
            return( (long)(p2x2_ + UR_CHAR) ); // (match!)
            // (no rom weight added)

        if (p2x2_[LR_COPY] < 0 /* new char */ &&
            _2x2Flag_ == 0 &&
            pixelMatch(pChar_, (char*) p2x2_[LR_CHAR]) )
        
            return( (long)(p2x2_ + LR_CHAR) ); // (match!)
            //(no rom weight added)

        _2x2Flag_ = 0;
        }
    }

// (no copy with 1x1, packed, or 2x2 found)
// (thus this is a new 2x2 char)

weight += weightRomChar;
// (no effect for mirror)

return(-1); // (new 2x2 char)
}
    




void    cut2x2Mir   (long *p2x2Mir_)
{

/*
    cut 2x2 which is x flip mirror of pre-cut 2x2 at p2x2Mir_
    Note weight is NOT updated
*/


p2x2End[UL_X] = rightXMirCenter +
        (leftXMirCenter - p2x2Mir_[UL_X] ) - 15;
p2x2End[UL_Y] = p2x2Mir_[UL_Y];

if (p2x2Mir_[_2X2_COPY] < 0) // if copy of new 2x2
    // point to copied 2x2 (beginning of it)
    p2x2End[_2X2_COPY] = (long)(p2x2Mir_);
else
    // point to what copied 2x2 points to
    p2x2End[_2X2_COPY] = p2x2Mir_[_2X2_COPY];
    
// set flip bits in UL_CHAR entry (rest of entry need not be set)
p2x2End[UL_CHAR] = 0; // assume no x flip
if ( (p2x2Mir_[UL_CHAR] & 0x10000) == 0 ) // if original 2x2 has no x flip
    p2x2End[UL_CHAR] = 0x10000; // then mirror must be x flipped
if (p2x2Mir_[UL_CHAR] & 0x20000) // if original 2x2 has y flip
    p2x2End[UL_CHAR] = p2x2End[UL_CHAR] ^ 0x20000; // then y flip mirror

//  NOTE THAT ALL THE OTHER INDICIES REALLY
//  DON'T HAVE TO BE SET, SO I'M SAVING SOME TIME IN WRITING THIS
//  AND HAVING IT EXECUTED.

p2x2End += _2X2_SIZE;

}





void    cutImageSlidingRows (void)
{

/*
    this will cut the image using sliding rows, each 16 pixels high,
    to find the best cut
*/

static long    tried_[16];
static long    offset_;


if (columns)
    {
    printf("        Cut image with sliding columns\n");
    flipAlongDiags(); // flip copy and char arrays along diagonals
    
    // debug printout of image as hex dump
    // dumpImage("\nImage before sliding columns cut:\n");

    }
else
    {
    printf("        Cut image with sliding rows\n");
    
    // debug printout of image as hex dump
    // dumpImage("\nImage before sliding rows cut:\n");
    }

// clear tried_[0:15] to 0
setmem ( (void*) & tried_[0], (unsigned) 4*16, (int) 0);

for (offset_ = 0 ; offset_ < numYOffsets ; offset_++)
    {

    printf("            Offset = %2ld,", offset_);
    
    // try rows going from top down

    topYRow = topY - offset_;
    if ( downUpForAllYOffsets || !tried_[topYRow & 0xf] )
        {
        tried_[topYRow & 0xf] = 1;
        deltaTopYRow = 16;
        cutRows();
        }

    // try rows going from bottom up

    if (allowDownUp)
        {
        topYRow = bottomY-15 + offset_;
        if ( downUpForAllYOffsets || !tried_[topYRow & 0xf] )
            {
            tried_[topYRow & 0xf] = 1;
            deltaTopYRow = -16;
            cutRows();
            }
        }

    printf("  weight = $%x\n", bestEverWeight);
    }
    
if (columns)
    flipAlongDiags(); // flip copy and char arrays along diagonals

}
        




void    cutMir  (void)
{

/*
    this will cut the next 1x1 or 2x2 as a mirror obj and recurse
    for the remaining objs.
    Note this is called only for objects close to mirror center
    The mirror objects which were far away (and weight auto-added)
        must be added to object lists by chkWeight
*/

static long    num1x1s_;
static long    i_, weight_;
static long    ULcopy_, LLcopy_;


// decide if 1x1 or 2x2 was 1st cut originally:

if (p1x1Mir != p1x1End)
    {

    // (1x1 exists)

    if (p2x2Mir != p2x2End)
        {

        // (1x1 and 2x2 exist)
        
        if ( p1x1Mir[UL_X] < p2x2Mir[UL_X] )
            goto do1x1;
        else
            goto do2x2;
    
        }   
    // (only 1x1 exists)

    goto do1x1;
    }
    
// (1x1 does not exist)

if (p2x2Mir == p2x2End)
    {

    // (neither 1x1 or 2x2 exist)

    chkWeight();
    return;
    }

do2x2:
    
if ( p2x2Mir >= p2x2CloseMirEnd || p2x2Mir[UL_X] > xLimitMir)
    {

    // (remaining mirror objs no longer needed) 

    chkWeight();
    return;
    }

// check if right half of 2x2 (before mirror) could be ignored:

if ( p2x2Mir[UL_X] + 8 > xLimitMir )
    {

    // right half of 2x2 (before mirror) could be ignored

    // cut left half of 2x2 (before mirror, before flip) into 0:2 1x1's

    i_ = p2x2Mir[UL_CHAR] & 0x30000; // (check for flips:)
    if (i_ == 0x00000)
        {
        ULcopy_ = UL_COPY;
        LLcopy_ = LL_COPY;
        }
    else
        if (i_ == 0x10000)
            {
            ULcopy_ = UR_COPY;
            LLcopy_ = LR_COPY;
            }
        else
            if (i_ == 0x20000)
                {
                ULcopy_ = LL_COPY;
                LLcopy_ = UL_COPY;
                }
            else
                {
                ULcopy_ = LR_COPY;
                LLcopy_ = UR_COPY;
                }

    // ensure weight will be ok:

    num1x1s_ = 0;
    if (p2x2Mir[ULcopy_] != 0)
        num1x1s_++;
    if (p2x2Mir[LLcopy_] != 0)
        num1x1s_++;

    weight_ = weight + (weightObj + weightFlicker +
              (allow1x1ToCopy2x2 ? 0 : weightVramChar) ) * num1x1s_;

    if (num1x1s_ != 0 && weight_ < bestWeight)
        {
    
        weight = weight_;

        // (weight is ok)

        if ( p2x2Mir[ULcopy_] != 0)
            cut1x1Mir(p2x2Mir, ULcopy_, p2x2Mir[UL_Y]);
        if ( p2x2Mir[LLcopy_] != 0)
            cut1x1Mir(p2x2Mir, LLcopy_, p2x2Mir[UL_Y] + 8);

        // recurse to next mirror obj:

        p2x2Mir += _2X2_SIZE;
        *--sp = num1x1s_;

        
        // debug check
        *--sp = weight;


        cutMir(); // (ALL LOCAL VARS USUALLY LOST!)


        // debug check
        if (*sp++ != weight)
            {
            printf("Error: weight changed software error #2\n");
            exit(1);
            }

            
        num1x1s_ = *sp++;
        p2x2Mir -= _2X2_SIZE;

        undoCut1x1Mir();
        if (num1x1s_ == 2)
            undoCut1x1Mir();

        if (num1x1s_ == 1)
            return; // (don't cut 2x2, as a single 1x1 will be better
                    // than a 2x2 which only needs to show 1 char)
        }
    }

// cut 2x2 mirror obj:

if (weight + weightObj + weightFlicker*4 < bestWeight)
    {

    weight += weightObj + weightFlicker*4;
    
    cut2x2Mir(p2x2Mir); // (won't effect weight)
    
    // recurse to next mirror obj:

    p2x2Mir += _2X2_SIZE;
    

    // debug check
    *--sp = weight;


    cutMir(); // (ALL LOCAL VARS USUALLY LOST!)


    // debug check
    if (*sp++ != weight)
        {
        printf("Error: weight changed software error #3\n");
        exit(1);
        }


    p2x2Mir -= _2X2_SIZE;
    
    // undoCut2x2Mir();
    p2x2End -= _2X2_SIZE; // (p2x2End now points to 2x2 being freed)
    weight -= weightObj + weightFlicker*4;
    
    }

return;

do1x1:

if ( p1x1Mir >= p1x1CloseMirEnd || p1x1Mir[UL_X] > xLimitMir )
    {

    // (remaining mirror objs no longer needed) 

    chkWeight();
    return;
    }

// ensure weight will be ok:

if (weight + weightObj + weightFlicker >= bestWeight)
    return; // (abort early)

weight += weightObj + weightFlicker;

// check if 1x1 is copy of 2x2 and can't share vram space
if ( p1x1Mir[COPY] >= (long)p2x2Beg && !allow1x1ToCopy2x2 )
    weight += weightVramChar;

cut1x1Mir(p1x1Mir, COPY, p1x1Mir[UL_Y]);

// recurse to next mirror obj:

p1x1Mir += _1X1_SIZE;


// debug check
*--sp = weight;


cutMir(); // (ALL LOCAL VARS USUALLY LOST!)


// debug check
if (*sp++ != weight)
    {
    printf("Error: weight changed software error #4\n");
    exit(1);
    }

    
p1x1Mir -= _1X1_SIZE;

undoCut1x1Mir();

}





long    cutRow  (void)
{

/*
    this will cut the entire 16 pixel high row as best as possible

    Will return: 0 if caller must abort early (cut worse than bestEverWeight)
                 1 if caller may continue
*/

static long    rightXRow_;
static long    blankWidth_, leftXBlank_, rightXBlank_;
static long    x_, x2_;
static char    *p_, *p2_;
static long    xOffset_, xOffset2_;
static long    bestWeight_;
static long    i_;
static long    numParts_, partWidth_, midRestart_, midEnd_, width_;


// get misc stuff about row:
if (chkRowStatus() == 0) // (row is all blank)
    return(1);

// if mirroring, then only need to cut the left half:

if (mirror)
    rightXRow_ = leftXMirCenter;
else
    rightXRow_ = rightXRow;

// find biggest area of transparency (if any) in middle of row to cut:

blankWidth_ = 0;
p_ = pChar + ( (topYRow << 8) + leftXRow + 1 );
for (x_ = leftXRow+1 ; x_ < rightXRow_ ; x_++)
    {
    // check if column is transparent
    if (*  p_          == 0 &&
        * (p_ + 0x100) == 0 &&
        * (p_ + 0x200) == 0 &&
        * (p_ + 0x300) == 0 &&
        * (p_ + 0x400) == 0 &&
        * (p_ + 0x500) == 0 &&
        * (p_ + 0x600) == 0 &&
        * (p_ + 0x700) == 0 &&
        * (p_ + 0x800) == 0 &&
        * (p_ + 0x900) == 0 &&
        * (p_ + 0xa00) == 0 &&
        * (p_ + 0xb00) == 0 &&
        * (p_ + 0xc00) == 0 &&
        * (p_ + 0xd00) == 0 &&
        * (p_ + 0xe00) == 0 &&
        * (p_ + 0xf00) == 0)
        
        {
        x2_ = x_;
        p2_ = p_;
        do
            {
            x2_++;
            p2_++;
            }
            while(x2_ <= rightXRow_  &&
                *  p2_          == 0 &&
                * (p2_ + 0x100) == 0 &&
                * (p2_ + 0x200) == 0 &&
                * (p2_ + 0x300) == 0 &&
                * (p2_ + 0x400) == 0 &&
                * (p2_ + 0x500) == 0 &&
                * (p2_ + 0x600) == 0 &&
                * (p2_ + 0x700) == 0 &&
                * (p2_ + 0x800) == 0 &&
                * (p2_ + 0x900) == 0 &&
                * (p2_ + 0xa00) == 0 &&
                * (p2_ + 0xb00) == 0 &&
                * (p2_ + 0xc00) == 0 &&
                * (p2_ + 0xd00) == 0 &&
                * (p2_ + 0xe00) == 0 &&
                * (p2_ + 0xf00) == 0);

        if (x2_ > rightXRow_)         // if blank columns in middle of mirror
            rightXRow_ -= (x2_ - x_); // then reduce amount of cut on right
        else
            {
            if (x2_ - x_ > blankWidth_)
                {
                blankWidth_ = x2_ - x_;
                leftXBlank_ = x_;
                rightXBlank_ = x2_ - 1;
                }
            }

        x_ = x2_ - 1;
        p_ = (char*) ( ( (long)p_ & 0xffffff00) + x_ );
        }
    p_++;
    }

// needed for any style cut
bestWeight = bestEverWeight;
bestWeight_ = bestWeight;

// check if row is small enough to be done in 1 piece

width_ = rightXRow_ - leftXRow + 1;
if (width_ <= maxWidth1Piece)
    {
    
    // (row can be done in 1 piece)

    // check if can do 2 seperated pieces simultaneously:

    if (blankWidth_ >= 1 &&
        leftXBlank_ - leftXRow    <= maxWidthHalf2Simul &&
        rightXRow_ - rightXBlank_ <= maxWidthHalf2Simul)

        {

        // do row in 2 seperated simultaneous parts:

        doSimul:

        for (xOffset_ = 0 ; xOffset_ < numXOffsets ; xOffset_++)
            {
            pULchar      = pChar + ( (topYRow << 8) + leftXRow-xOffset_ );
            pULcopy      = pCopy + ( (topYRow << 8) + leftXRow-xOffset_ );
            pULcharLimit = pChar + ( (topYRow << 8) + leftXBlank_-1);
            
            for (xOffset2_ = 0 ; xOffset2_ < numXOffsets ; xOffset2_++)
                {
                firstSimulPart = 1;

                pULchar2      = pChar + ( (topYRow << 8) +
                                rightXBlank_ + 1 - xOffset2_);
                pULcharLimit2 = pChar + ( (topYRow << 8) + rightXRow_);

                // mark start of row being cut
                p1x1RowBeg = p1x1End;
                p2x2RowBeg = p2x2End;
                
                p1x1CloseMirBeg = (long*) -1; // (not set yet)
                

                // debug check
                *--sp = weight;


                cutRowColumn();

                
                // debug check
                if (*sp++ != weight)
                    {
                    printf("Error: weight changed software error #5\n");
                    exit(1);
                    }


                }
            }   

        chkBest:

        if (bestWeight == bestWeight_)
            return(0); // (caller must abort)

        useBestCut(); // (declare best simultaneous cut is permanently done)
        
        return(1);
        }

    // see if could do 2 simultaneous parts if cut done in middle

    leftXBlank_ = leftXRow + (rightXRow_ - leftXRow + 1) / 2;
    rightXBlank_ = leftXBlank_ - 1;

    if (    allowSimulCutInMiddle &&
            leftXBlank_ - leftXRow    <= maxWidthHalf2Simul &&
            rightXRow_ - rightXBlank_ <= maxWidthHalf2Simul)
            
        goto doSimul;


    // do row in 1 big piece:

    firstSimulPart = 0; // (no simultaneous parts)

    for (xOffset_ = 0 ; xOffset_ < numXOffsets ; xOffset_++)
        {
        pULchar      = pChar + ( (topYRow << 8) + leftXRow-xOffset_ );
        pULcopy      = pCopy + ( (topYRow << 8) + leftXRow-xOffset_ );
        pULcharLimit = pChar + ( (topYRow << 8) + rightXRow_);

        p1x1RowBeg = p1x1End;
        p2x2RowBeg = p2x2End;
        
        p1x1CloseMirBeg = (long*)-1; // (not set yet)
        

        // debug check
        *--sp = weight;


        cutRowColumn();

        
        // debug check
        if (*sp++ != weight)
            {
            printf("Error: weight changed software error #6\n");
            exit(1);
            }

            
        }

    goto chkBest;
    }
    
// (row so big it must be done in seperate parts)

// see if 1 single seperation can be done in the gap in the middle (if any):

if (blankWidth_ >= 1 &&
        leftXBlank_ - leftXRow    <= maxWidth1Piece &&
        rightXRow_ - rightXBlank_ <= maxWidth1Piece)
        
    {
    // special case to allow 1 simultaneous cut even if total width
    //   is > maxWidth1Piece (since big enough gap in middle)
    if (leftXBlank_ - leftXRow    <= maxWidthHalf2Simul &&
        rightXRow_ - rightXBlank_ <= maxWidthHalf2Simul)
        goto doSimul;

    numParts_ = 2;
    // partWidth_ need not be set
    midEnd_ = leftXBlank_ - 1;
    midRestart_ = rightXBlank_ + 1;
    }
else
    {
    numParts_ = ( width_ + maxWidth1Piece - 1 ) / maxWidth1Piece;
    partWidth_ = maxWidth1Piece;
    while ( (partWidth_ - 16) * numParts_ >= width_ )
        partWidth_ -= 16;

    midRestart_ = rightXRow_ - (numParts_ - 1) * partWidth_ + 1;
    midEnd_ = midRestart_ - 1;
    }

firstSimulPart = 0; // (no simultaneous parts)

// do 1 row piece at a time:

do
    {
    bestWeight = bestEverWeight;
    bestWeight_ = bestWeight;

    for (xOffset_ = 0 ; xOffset_ < numXOffsets ; xOffset_++)
        {
        pULchar      = pChar + ( (topYRow << 8) + leftXRow-xOffset_ );
        pULcopy      = pCopy + ( (topYRow << 8) + leftXRow-xOffset_ );
        pULcharLimit = pChar + ( (topYRow << 8) + midEnd_);
        
        p1x1RowBeg = p1x1End;
        p2x2RowBeg = p2x2End;
        
        p1x1CloseMirBeg = (long*)-1; // (not set yet)
        

        // debug check
        *--sp = weight;


        cutRowColumn();

        
        // debug check
        if (*sp++ != weight)
            {
            printf("Error: weight changed software error #7\n");
            exit(1);
            }

            
        }
    
    if (bestWeight == bestWeight_)
        return(0); // (caller must abort)

    useBestCut(); // (declare best simultaneous cut is permanently done)

    numParts_--;

    leftXRow = midRestart_;
    // don't recut anything cut in 1st part
    i_ = ( (long)pULcharBest & 0xff );
    if (i_ > leftXRow)
        leftXRow = i_;

    midRestart_ = rightXRow_ - (numParts_ - 1) * partWidth_ + 1;
    midEnd_ = midRestart_ - 1;
    }
    while (numParts_ > 0);

return(1);
}





void    cutRowColumn    (void)
{

/*
    you are within a 16 pixel high row.
    Cut the left column into
    a 2x2 or 1x1s and recurse to the right
    note the input chars are NOT flipped

    pULchar = ptr to upper left pixel of upper char in left column (note
        this is the image with no flip)
    pULcopy = ptr to packed-char-copy word for upper char in left column
        (note this is the array for no flip)
*/

static long    numChars_;


// check if time to record start of objs which do not auto-add the
//  mirror weights:

*--sp = (long)p1x1CloseMirBeg;

if (pULchar > pULcloseMir && (long)p1x1CloseMirBeg == -1) // (not set yet)
    {
    p1x1CloseMirBeg = p1x1End;
    p2x2CloseMirBeg = p2x2End;
    }

if (pULchar > pULcharLimit)
    {

    // (this is the end of the row, or the piece of the row)

    if (firstSimulPart && pULchar <= pULcharLimit2)
        {
        
        // time to switch to 2nd of 2 simultaneous parts:

        *--sp = (long)pULchar;
        *--sp = (long)pULcharLimit;

        if (pULchar2 + 8 <= pULchar)
            pULchar = pULchar2 + 8;
        else
            pULchar = pULchar2;
        pULcopy = pCopy + ( (long)pULchar & 0x3ffff );
        pULcharLimit = pULcharLimit2;

        firstSimulPart = 0;


        // debug check
        *--sp = weight;


        cutRowColumn();

        
        // debug check
        if (*sp++ != weight)
            {
            printf("Error: weight changed software error #8\n");
            exit(1);
            }

            
        firstSimulPart = 1;

        pULcharLimit =  (char*) *sp++;
        pULchar = (char*) *sp++;
        pULcopy = pCopy + ( (long)pULchar & 0x3ffff );

        goto cutRowColumnReturn;
        }

    chkMir(); // (check mirroring, then check best weight) (done above)

    cutRowColumnReturn:

    p1x1CloseMirBeg = (long*) *sp++;

    return; // (end of row or piece of row)
    }

// count how many non-transparent chars exist among 4 chars
numChars_ = 0;
if ( pULcopy[0x000] != 0) // (upper left transparent?)
    numChars_++;
if ( pULcopy[0x800] != 0) // (lower left transparent?)
    numChars_++;
if ( pULcopy[0x008] != 0) // (upper right transparent?)
    numChars_++;
if ( pULcopy[0x808] != 0) // (lower right transparent?)
    numChars_++;

// see if 2x2 can be done
if (!only1x1 &&
    (numChars_ >= 3 ||
     (numChars_ == 2 && allow2Char2x2Cut &&
      ( pULcopy[0x000] != 0 || pULcopy[0x800] != 0 || allow2x2WithLeftBlank)
     )
    )
   )

    {
    // (2x2 will be tried)

    // see if typical weight of 1x1's is less than that of 2x2
    if ( (weightObj + weightFlicker + weightVramChar + weightRomChar)
            * numChars_   <   (weightObj + weightFlicker*4 +
            weightVramChar*4 + weightRomChar*numChars_) )

        {
        cutRowColumn1x1(); // cut in 1x1's first
        cutRowColumn2x2(); // cut in 2x2 second
        }
    else
        {
        cutRowColumn2x2(); // cut in 2x2 first
        cutRowColumn1x1(); // cut in 1x1's second
        }
    }

else
        cutRowColumn1x1(); // cut in 1x1's

goto cutRowColumnReturn;
}





void    cutRowColumn1x1 (void)
{

/*
    Cut left column into 0:2 1x1's and recurse:
*/

char    *p_;
long    top_, bot_, bestTop_;


// Check if a single 1x1 can be done.
// If so, then slide up/down within row to get best cut.

if ( allowSliding1x1 && ( pULcopy[0x000] != 0 || pULcopy[0x800] != 0 ) )
    {
    top_ = topYRow;
    p_ = pULchar;
    while ( * (long*)p_ == 0 &&  * (long*)(p_ + 4) == 0 )
        {
        top_++;
        p_ += 0x100;
        }

    bot_ = topYRow + 15; // inclusive
    p_ = pULchar + 0xf00;
    while ( * (long*)p_ == 0 &&  * (long*)(p_ + 4) == 0 )
        {
        bot_--;
        p_ -= 0x100;
        }

    if (bot_ - top_ <= 7)
        {
        // small enough for a single 1x1
        // find best shift position

        // if in sliding rows mode, then don't slide into another row
        //   since this will increase flicker at the overlap.
        // Note that there is no such problem in sliding columns mode
        //   since the slide is really along the row and therefore
        //   doesn't affect flicker no matter how far it slides.
        // Patch added so that mirrored row cannot have 1x1 slide out
        //   out of the row (whether in sliding rows or columns)

	if (columns == 0 && !allowSliding1x1OutOfRow    || mirror)
            {
            // ensure char is not below row
            if (top_ > topYRow + 8)
                top_ = topYRow + 8;

            // ensure char is not above row
            if (bot_ < topYRow + 7)
                bot_ = topYRow + 7;
            }

        *--sp = bestWeight; // will use bestWeight itself for quicker aborts

        // need not init bestTop_

        // set vars for cut1x1AllFlips
        ul_x_g      =   (long)pULchar & 0xff;
        ul_y_g      =   top_;
        pULchar_g   =   pChar + ( (ul_y_g << 8) + ul_x_g);
        
        // try all shifts
        do
            {
            if (cut1x1AllFlips() != 0) // may continue
                {
                // weight is < bestWeight
                bestWeight = weight;
                bestTop_ = ul_y_g;
                undoCut1x1();
                }
            ul_y_g--;
            pULchar_g -= 0x100;
            }
            while (bot_ - ul_y_g <= 7);

        // check if nothing could be cut
        if (bestWeight == *sp)
            {
            sp++;
            return; // abort early
            }

        bestWeight = *sp++; // restore value

        // cut best 1x1
        ul_y_g      =   bestTop_;
        pULchar_g   =   pChar + ( (ul_y_g << 8) + ul_x_g);
        cut1x1AllFlips(); // will be successful

        // go 1 column to right and recurse:
        
        pULchar += 8;
        pULcopy += 8;
        

        // debug check
        *--sp = weight;


        cutRowColumn(); // (NOTE ALL LOCAL VARS USUALLY LOST!)

        
        // debug check
        if (*sp++ != weight)
            {
            printf("Error: weight changed software error #9\n");
            exit(1);
            }

            
        pULchar -= 8;
        pULcopy -= 8;
        
        undoCut1x1();
    
        // check if cut can stop here, or if normal cut must proceed
        if ( !proceedAfterSliding1x1  ||
                ( (p1x1End[UL_Y] & 7) == 0  &&
                  ( pULcopy[0x000] == 0 || pULcopy[0x800] == 0 )
                )
           )

            return; // done
        }
    }

if ( pULcopy[0x000] != 0) // (upper left transparent?)
    {
    ul_x_g      =   (long)pULchar & 0xff;
    ul_y_g      =   ( (long)pULchar & 0xff00 ) >> 8;
    pULchar_g   =   pULchar;
    
    if (cut1x1AllFlips() == 0) // (abort early?))
        return;
    }

if ( pULcopy[0x800] != 0) // (lower left transparent?)
    {
    ul_x_g      =   (long)pULchar & 0xff;
    ul_y_g      =   ( ( (long)pULchar & 0xff00 ) >> 8 ) + 8;
    pULchar_g   =   pULchar + 0x800;
    
    if (cut1x1AllFlips() == 0) // (abort early?))
        {
        // (will abort early)
        if ( pULcopy[0x000] != 0) // (upper left transparent?)
            undoCut1x1();

        return;
        }
    }

// go 1 column to right and recurse:

pULchar += 8;
pULcopy += 8;


// debug check
*--sp = weight;


cutRowColumn(); // (NOTE ALL LOCAL VARS USUALLY LOST!)


// debug check
if (*sp++ != weight)
    {
    printf("Error: weight changed software error #10\n");
    exit(1);
    }

            
pULchar -= 8;
pULcopy -= 8;

if ( pULcopy[0x800] != 0) // (lower left transparent?)
    undoCut1x1();

if ( pULcopy[0x000] != 0) // (upper left transparent?)
    undoCut1x1();
}    
    
    
    
    
    
void    cutRowColumn2x2   (void)
{

// cutRowColumn subroutine to cut as 2x2

// see if possible to do 2x2 with forced blanking of right column:

if ( allow2x2WithRightBlank &&
        (long)pULchar + 8 > (long)pULcharLimit &&
        pULcopy[0x000] != 0 && pULcopy[0x800] != 0)
    {

    // try 2x2 with right column blank
    rightColumnBlank = 1;
    
    // try 4 flips, choose best one and recurse:
    if (cut2x2AllFlips() == 1) // (weight ok)
        {

        pULchar += 8;
        pULcopy += 8;

        
        // debug check
        *--sp = weight;


        cutRowColumn(); // (NOTE ALL LOCAL VARS USUALLY LOST!)


        // debug check
        if (*sp++ != weight)
            {
            printf("Error: weight changed software error #11\n");
            exit(1);
            }

            
        pULchar -= 8;
        pULcopy -= 8;

        undoCut2x2();
        }
    }

rightColumnBlank = 0; // (don't force right column to be blank)

// try 4 flips, choose best one and recurse:
if (cut2x2AllFlips() == 1) // (weight ok)
    {
    pULchar += 8*2;
    pULcopy += 8*2;
    

    // debug check
    *--sp = weight;


    cutRowColumn(); // (NOTE ALL LOCAL VARS USUALLY LOST!)


    // debug check
    if (*sp++ != weight)
        {
        printf("Error: weight changed software error #12\n");
        exit(1);
        }

            
    pULchar -= 8*2;
    pULcopy -= 8*2;

    undoCut2x2();
    }
}
    
    
    
    
    
void    cutRows (void)
{

/*
    scan the various 16 pixel high rows, and cut them.
    Note topYRow set at desired top y of topmost row
    deltaTopYRow is +16 or -16
*/

static long    i_;


// declare no best cut yet:
p1x1BestEnd = p1x1BestBeg;
p2x2BestEnd = p2x2BestBeg;
bestWeight = bestEverWeight;

// declare no cut yet:
p1x1End = p1x1Beg;
p2x2End = p2x2Beg;
weight = 0;

do
    {
    if (cutRow() == 0) // (0 == abort early)
        return;

    topYRow += deltaTopYRow; // (+16 or -16)
    }
    while (topYRow + 15 >= topY && topYRow <= bottomY);

// (if code reaches here then it found the best ever cut!)

bestEverWeight = bestWeight;
bestEverByColumns = columns;

// move mem from p1x1BestBeg to p1x1BestEverBeg
i_ = (long)p1x1BestEnd - (long)p1x1BestBeg;
if (i_ > 0)
    movmem (p1x1BestBeg, p1x1BestEverBeg, (unsigned) i_);
p1x1BestEverEnd = (long*) ( (long)p1x1BestEverBeg + i_ );

// move mem from p2x2BestBeg to p2x2BestEverBeg
i_ = (long)p2x2BestEnd - (long)p2x2BestBeg;
if (i_ > 0)
    movmem (p2x2BestBeg, p2x2BestEverBeg, (unsigned) i_);
p2x2BestEverEnd = (long*) ( (long)p2x2BestEverBeg + i_ );

}





void    delay   (long secs_)
{

static clock_t  start_, end_;


// set time starting cut
start_ = clock();

do
    {
    end_ = clock();
    }
    while ( (long) ( (end_ - start_) / CLOCKS_PER_SEC ) < secs_);

}





void    dumpImage   (char *str_)
{
static  long    x_, y_, xb_, xe_;
static  char    *p_;


if (allowImageDump) // 1 enables dump, 0 disables dump

    {
    printf("%s", str_);
    
    for (xb_ = leftX ; xb_ <= rightX ; xb_ += 76)
        {
        xe_ = xb_ + 76 - 1;
        if (xe_ > rightX)
            xe_ = rightX;

        printf("\n");
    
        printf("   ");
        for (x_ = xb_ ; x_ <= xe_ ; x_++)
            printf("%2x", x_ >> 4);
        printf("\n");

        printf("   ");
        for (x_ = xb_ ; x_ <= xe_ ; x_++)
            printf("%2x", x_ & 0xf);
        printf("\n");

        printf("   ");
        for (x_ = xb_ ; x_ <= xe_ ; x_++)
            printf("--");
        printf("\n");

        for (y_ = topY ; y_ <= bottomY ; y_++)
            {
            printf("%2x:", y_);
            p_ = pChar + (y_ << 8);
            
            for (x_ = xb_ ; x_ <= xe_ ; x_++)
            	{
		if (p_[x_] == 0)
	            printf(" .");
	        else
        	    printf("%2x",p_[x_]);
                }
         
            printf(" :%2x\n", y_);
            }

        printf("   ");
        for (x_ = xb_ ; x_ <= xe_ ; x_++)
            printf("--");
        printf("\n");

        printf("   ");
        for (x_ = xb_ ; x_ <= xe_ ; x_++)
            printf("%2x", x_ >> 4);
        printf("\n");

        printf("   ");
        for (x_ = xb_ ; x_ <= xe_ ; x_++)
            printf("%2x", x_ & 0xf);
        printf("\n");

        printf("\n");
        }
    }
}





long    findPacked  (char *pChar_)
{

/*
    scan all packed chars to see if this char (without further flipping)
     is found (check for transparent before calling this)
    Note that if you accidentally call this with a transparent char,
     then this code will indeed find it and return 0.
    This is will ignore the 'columns' variable.

    will return long word:
        -1      if not found
        0:64K-1 as packed char # if found
    

    Note this routine could be called up to a max of
     (256-15-15) * (256-15-15) * 4 = 204,304 times per image
     loaded, so I think we'd better make it go fast, don't you?
    A more realistic estimate would be for an image which
     was 64 x 64 pixels.  This would only take 64 * 64 * 4
     = 16,384 times per image.
*/

static char    c_;
static long    x_, y_;
static char    *pPack_, *p_;


pPack_ = pPackBeg;

for (y_ = 0 ; y_ <= 7 ; y_++)
    {
    for (x_ = 0 ; x_ <= 7 ; x_++)
        {
        c_ = *pChar_++;
        if (c_ != *pPack_)
            {
            while( (long) (p_ = * (char**)(pPack_ + 5) ) >= 0)
                {
                pPack_ = p_;
                if (c_ == *pPack_)
                    goto gotPixel;
                }
            return(-1);
            }
        else
            {
            gotPixel:
            pPack_ = * (char**) (pPack_ + 1);
            }
        }
    pChar_ += 0x100-8;
    }

return( (long)pPack_ ); // (0:64K-1)
}

            



void    flipAlongDiags  (void)
{

// flip data about coordinate diagonal for char and copy arrays
//   and misc vars

long    x_, y_, i1_, i2_, j1_, j2_;
char    c1_, c2_;
char    *p1_, *p2_;
long    *p3_, *p4_;


// flip vars containing image rectangle size
i1_ = leftX;
i2_ = topY;
leftX = i2_;
topY = i1_;

i1_ = rightX;
i2_ = bottomY;
rightX = i2_;
bottomY = i1_;

// flip about diagonal from upper left to bottom right
for (y_ = 8 ; y_ < 256-8 ; y_++) // outer 15 pixels are blank
    {
    for (x_ = 8 ; x_ < y_ ; x_++)
        {
        i1_ = (y_ << 8) + x_;
        i2_ = (x_ << 8) + y_;
        
        c1_ = pChar[i1_];
        c2_ = pChar[i2_];
        pChar[i1_] = c2_; 
        pChar[i2_] = c1_; 
        
        c1_ = pXChar[i1_];
        c2_ = pXChar[i2_];
        pXChar[i1_] = c2_; 
        pXChar[i2_] = c1_; 
        
        c1_ = pYChar[i1_];
        c2_ = pYChar[i2_];
        pYChar[i1_] = c2_; 
        pYChar[i2_] = c1_; 

        c1_ = pXYChar[i1_];
        c2_ = pXYChar[i2_];
        pXYChar[i1_] = c2_; 
        pXYChar[i2_] = c1_; 
        
        j1_ = pCopy[i1_];
        j2_ = pCopy[i2_];
        pCopy[i1_] = j2_; 
        pCopy[i2_] = j1_; 
        
        j1_ = pXCopy[i1_];
        j2_ = pXCopy[i2_];
        pXCopy[i1_] = j2_; 
        pXCopy[i2_] = j1_; 
        
        j1_ = pYCopy[i1_];
        j2_ = pYCopy[i2_];
        pYCopy[i1_] = j2_; 
        pYCopy[i2_] = j1_; 

        j1_ = pXYCopy[i1_];
        j2_ = pXYCopy[i2_];
        pXYCopy[i1_] = j2_; 
        pXYCopy[i2_] = j1_; 
        }
    }
    
// swap x flip and y flip arrays

p1_ = pXChar;
p2_ = pYChar;
p3_ = pXCopy;
p4_ = pYCopy;
while (p1_ < pYChar)
    {
    c1_ = *p1_;
    c2_ = *p2_;
    *p1_++ = c2_;
    *p2_++ = c1_;

    j1_ = *p3_;
    j2_ = *p4_;
    *p3_++ = j2_;
    *p4_++ = j1_;
    }

}
    




long    getImage    (void)
{

/*
    input image, display on screen in window
    
    will return:    0 if end of image list
                    1 if input file exists and is input
*/

static char line_[256], c_, word_[256], file_[256];
static long i_, j_, numMatches_, value_, nestedComments_ = 0;


input:

// ensure no overflow of mImgTbl structure
if (pScaleCnt >= pScaleMax)
    {
    printf("Error: array overflow, increase SCALE_SIZE and recompile\n");
    exit(1);
    }

if (pRotateEnd > pRotateMax)
    {
    printf("Error: array overflow, increase ROTATE_SIZE and recompile\n");
    exit(1);
    }

// input line        
if ( fgets(&line_[0], 256, imageListFile) == NULL )
    {
    if (imageDepth != 0)
        {
        printf("Error: unbalanced '{' and '}' lines in input file '%s'\n",
                imageListFileName);
        
        exit(1);
        }        

    if (nestedComments_ != 0)
        {
        printf("Error: unbalanced '<' and '>' lines in input file '%s'\n",
                imageListFileName);
        
        exit(1);
        }        

    return(0); // end of input file
    }

// get rid of '\n'
j_ = strlen(&line_[0]);
if (j_ >= 1)
    line_[j_ - 1] = '\0';

// ignore white space at start of line
i_ = -1;
do
    {
    c_ = line_[++i_];
    }
    while (c_ == ' ' || c_ == '\t');

// check for start of nested comments
if (c_ == '<')
    {
    nestedComments_++;
    goto input;
    }

// check for end of nested comments
if (c_ == '>')
    {
    nestedComments_--;
    if (nestedComments_ < 0)
        {
        printf("Error: too many '>' lines in input file '%s'\n",
                imageListFileName);
        
        exit(1);
        }        
    goto input;
    }

// check if nested comments cause line to be ignored:
if (nestedComments_)
    goto input;

// ignore blank or comment lines
if (c_ == '\0' || c_ == ';')
    goto input;

// check for cutter control line
if (c_ == '#')
    {
    // skip '#' char
    i_++;

    numMatches_ = sscanf (&line_[i_], " %s %d ", word_, &value_);
    
    // check for special commands
    if (numMatches_ >= 1)
        {
        if ( stricmp (word_, "loadDefaults") == 0 )
            {
            printf("loadDefaults\n");
            initControls();
            goto input;
            }

        if ( stricmp (word_, "loadFiles") == 0 )
            {
            numMatches_ = sscanf (&line_[i_], " %s %s ", word_, &file_);
            if (numMatches_ != 2)
                goto badLine2;

            printf("loadFiles %s\n", file_);
            strcpy(inputMasterFileName, outDir);
            if (file_[0] != '*')
                {
                strcat(inputMasterFileName, file_);
                }
            inputFiles();

            goto input;
            }

        if ( stricmp (word_, "inputDirectory") == 0 )
            {
            numMatches_ = sscanf (&line_[i_], " %s %s ", word_, &file_);
            if (numMatches_ != 2)
                goto badLine2;

            printf("inputDirectory %s\n", file_);
            strcpy(inDir, file_);

            goto input;
            }

        if ( stricmp (word_, "outputDirectory") == 0 )
            {
            numMatches_ = sscanf (&line_[i_], " %s %s ", word_, &file_);
            if (numMatches_ != 2)
                goto badLine2;

            printf("outputDirectory %s\n", file_);
            strcpy(outDir, file_);

            goto input;
            }

        if ( stricmp (word_, "outputName1Time") == 0 )
            {
            numMatches_ = sscanf (&line_[i_], " %s %s ", word_, &file_);
            if (numMatches_ != 2)
                goto badLine2;

            printf("outputName1Time %s\n", file_);
            strcpy(outputName1Time, file_);

            goto input;
            }

        }

    if (numMatches_ != 2)
        {
        
        badLine2:

        printf("Error: illegal # line in input file '%s'\n",
                imageListFileName);
        
        badLine:

        printf("Line is: '%s'\n", &line_[0]);
        printf("Legal format is:\n");
        printf("  '<optWhtSpc>#<optWhtSpc>word<whtSpc>value<opt WhtSpc&Cmt>'\n");
        
        exit(1);
        }

    for (j_ = 0 ; j_ < NUM_CONTROLS ; j_++)
        {
        
        // string comparison is case insensitive
        if ( stricmp (&word_[0], controlWord[j_]) == 0 )
            {

            // got control word match

            // ensure value in bounds
            if (value_ < controlValueMin[j_] || value_ > controlValueMax[j_])
                {
                printf("Error: illegal # line value in input file '%s'\n",
                        imageListFileName);
                printf("Line is: '%s'\n", &line_[0]);
                printf("Legal value range is: %d to %d\n",
                        controlValueMin[j_], controlValueMax[j_]);
                        
                exit(1);
                }
                
            printf("%s = %d\n", controlWord[j_], value_);
            *controlValueAdrs[j_] = value_;
            
            goto input;
            }
        }

    printf("Error: illegal # line word in input file '%s'\n",
            imageListFileName);
    goto badLine;
    }

// check for start of tilt-scale-Tilt structure    
if (c_ == '{')
    {
    imageDepth++;
    
    if (imageDepth == 1 && masterFileName[0] == '\0')
        {
        printf("Error: master filename unspecified in '%s'\n",
                imageListFileName);
        exit(1);
        }

    if (imageDepth > 3)
        {
        printf("Error: too many '{' lines in '%s'\n",
                imageListFileName);
        exit(1);
        }
        
    goto input;
    }

// check for end of tilt-scale-Tilt structure    
if (c_ == '}')
    {
    imageDepth--;
    
    if (imageDepth < 0)
        {
        printf("Error: too many '}' lines in '%s'\n",
                imageListFileName);
        exit(1);
        }
        
    if (imageDepth == 0)
        {
        
        // end of master image

        if (numTilts == 0)
            {
            printf("Error: no scale images within '{' '}' lines in '%s'\n",
                    imageListFileName);
            exit(1);
            }

        outputFiles();
        
        masterFileName[0] = '\0';
        initMImgTblStructure();
        
        goto input;
        }

    if (imageDepth == 1)
        {
        
        // end of tilt

        if (*pScaleCnt == 0)
            {
            printf("Error: no scale images within '{' '}' lines in '%s'\n",
                    imageListFileName);
            exit(1);
            }

        numTilts += 1;
        if (numTilts > 255)
            {
            
            tooManyTilts:

            printf("Error: # of tilts > 255 in '%s'\n", imageListFileName);
            exit(1);
            }

        * ++pScaleCnt = 0;

        goto input;
        }

    if (imageDepth == 2)
        {
        
        // end of scale

        if (*pRotateCnt == 0)
            {
            printf("Error: no rotate images within '{' '}' lines in '%s'\n",
                    imageListFileName);
            exit(1);
            }

        if (*pScaleCnt == 255)
            {
            
            tooManyScales:

            printf("Error: over 255 scales in '%s'\n", imageListFileName);
            exit(1);
            }

        *pScaleCnt += 1;
        
        pRotateCnt = pRotateEnd;
        *pRotateEnd++ = 0;

        goto input;
        }

    }

sscanf (&line_[i_], " %s ", &word_[0]);

// check for master filename:
if (imageDepth == 0)
    {

    strcpy(masterFileName, word_);
    printf("\n\nStart of master file '%s'\n\n", masterFileName);


    // check if too many master images    
    if (numMasterImages  == MIMGTBLI_SIZE)
        {
        printf("Error: too many master files, increase MIMGTBLI_SIZE and recompile\n");
        exit(1);
        }

    mImgTblI[numMasterImages++] = reverseWord(mImgTblOfstEnd);

    goto input;
    }

// only thing left is image filename:

strcpy(imageFileName, inDir);
strcat(imageFileName, word_);
strcat(imageFileName, ".lbm");

printf("\n    Reading ");
if (imageDepth == 1)
    printf("tilt ");
if (imageDepth == 2)
    printf("scale ");
if (imageDepth == 3)
    printf("rotate ");
printf("image file '%s'\n\n", imageFileName);

// update mImgTbl data

if (imageDepth == 1)
    {

    // new tilt image

    numTilts++;
    if (numTilts > 255)
        goto tooManyTilts;

    if (*pScaleCnt == 255)
        goto tooManyScales;

    *pScaleCnt += 1;
    * ++pScaleCnt = 0;

    if (*pRotateCnt == 255)
        {
        
        tooManyRotates:

        printf("Error: over 255 rotates in '%s'\n", imageListFileName);
        exit(1);
        }

    *pRotateCnt += 1;
    * (short*) (pRotateEnd) = imageNum;
    pRotateEnd += 2;
    pRotateCnt = pRotateEnd;
    *pRotateEnd++ = 0;
    }
else
    {
    if (imageDepth == 2)
        {
        
        // new scale image

        if (*pScaleCnt == 255)
            goto tooManyScales;

        *pScaleCnt += 1;

        if (*pRotateCnt == 255)
            goto tooManyRotates;        

        *pRotateCnt += 1;
        * (short*) (pRotateEnd) = imageNum;
        pRotateEnd += 2;
        pRotateCnt = pRotateEnd;
        *pRotateEnd++ = 0;
        }
    else
        {
        
        // new rotate image

        if (*pRotateCnt == 255)
            goto tooManyRotates;        

        *pRotateCnt += 1;
        * (short*) (pRotateEnd) = imageNum;
        pRotateEnd += 2;
        }
    }        

numImages++;
if (numImages > 0xffff)
    {
    printf("Error: # total images > $ffff\n");
    exit(1);
    }
imageNum += 4;
if (imageNum > 0xffff)
    {
    printf("Error: image # (0,4,8,...) > $ffff\n");
    exit(1);
    }

if (ReadIFF(imageFileName, pChar, flipInputX) == 0)
    {
	printf("Error: ReadIFF had error reading image file '%s'\n",
            imageFileName);

	exit (1);
	}

// image has been input, may now proceed:
return (1);

}





void    initControls    (void)
{

static long i_;


// init all control values:
for (i_ = 0 ; i_ < NUM_CONTROLS ; i_++)
    *controlValueAdrs[i_] = controlValueInit[i_];
    
}





void    initImageList   (void)
{

/*
    open imageListFileName for input
    init misc stuff for other 7 data files
*/


imageListFile = fopen(imageListFileName, "r");
if (imageListFile == NULL)
    {
    printf("Error: could not open image list file '%s' for input\n",
            imageListFileName);
            
    exit(1);
    }

// stuff for mImgTblI and mImgTbl and reading image list file
numMasterImages = 0;
numImages = 0;
imageNum = 0;
// note mImgTbl 1st 2 words are not stored until time to write file
mImgTblOfstEnd = 4;

masterFileName[0] = '\0';
imageDepth = 0;

initMImgTblStructure();
}





void    initMImgTblStructure (void)
{


numTilts = 0;

pScaleCnt = pScaleBeg;
*pScaleCnt = 0;

pRotateCnt = pRotateBeg;
pRotateEnd = pRotateBeg;
*pRotateEnd++ = 0;
}





void    initPack    (void)
{

/*
    initPack will clear the packed tree so that there is only the
      transparent char defined.
*/

static long x_, y_;
static char *p_;


pPackEnd = pPackBeg;

packNumEnd = 0;

*pPackBeg = 1; // any non-0 value (won't match transparent char)
* (char**) (pPackBeg + 5) = (char*)-1; // no sibling in tree

// ensure char is transparent
for (y_ = 0 ; y_ <= 7 ; y_++)
    {
    p_ = pChar + (y_ << 8);
    for (x_ = 0 ; x_ <= 7 ; x_++)
        p_[x_] = 0;
    }

packChar (pChar); // pack transparent char as char #0

// debug test only !!!!!!
// 	packChar (pChar + 0x2020);
// packChar (pChar + 0x3820);

return;
}





void    inputFiles (void)
{


inputPack();
inputMImgTbl();
inputMImgTblI();
inputImageTbl();
inputCharData();
inputMiscData();
}





void    inputPack  (void)
{

static long offset_;
static char *pPackBeg_, *pPackEnd_, *p_;
static long *p2_;


// input packed chars:
strcpy(packFileName, inputMasterFileName);
strcat(packFileName, ".pack");

printf("Inputting '%s'\n", packFileName);

// open .pack file for input
packFile = open(packFileName, O_RDONLY, 0);
if (packFile == -1)
    {
    printf("Error: could not open '%s' for input\n", packFileName);
    exit(1);
    }

// read sumOfWeights (hi 4 bytes, then lo 4 bytes)
if ( read(packFile, &sumOfWeightsHi, 4) != 4 )
    goto err4;
if ( read(packFile, &sumOfWeightsLo, 4) != 4 )
    goto err4;
    
// read value of packNumEnd (#packed chars)
if ( read(packFile, &packNumEnd, 4) != 4 )
    {
    err4:
    printf("Error: could not read 4 bytes from '%s'\n",
            packFileName);
    exit(1);
    }
    
// read value of pPackBeg_
if ( read(packFile, &pPackBeg_, 4) != 4 )
    {
    printf("Error: could not read 4 bytes from '%s'\n",
            packFileName);
    exit(1);
    }
    
// read value of pPackEnd_
if ( read(packFile, &pPackEnd_, 4) != 4 )
    {
    printf("Error: could not read 4 bytes from '%s'\n",
            packFileName);
    exit(1);
    }

// set pPackEnd
pPackEnd = pPackBeg + (pPackEnd_ - pPackBeg_);
    
// read all pPack data
if ( read(packFile, pPackBeg, pPackEnd_ - pPackBeg_) !=
            pPackEnd_ - pPackBeg_ )
    {
    printf("Error: could not read %d bytes from '%s'\n",
            pPackEnd_ - pPackBeg_, packFileName);
    exit(1);
    }

// correct all ptr references as pPackBeg may be at new location

offset_ = pPackBeg - pPackBeg_;

for (p_ = pPackBeg ; p_ < pPackEnd ; p_ += 9)
    {
    p2_ = (long*) (p_ + 1);
    if ( *p2_ >= 0x10000) {
        	*p2_ += offset_;
		if (((long) *p2_ < (long) pPackBeg) || ((long) *p2_ >= (long) pPackEnd)) {
	    		printf("Error: PackData (p_+1) Pointer Error!\n");
		    exit(1);
		}
	}
        
    p2_ = (long*) (p_ + 5);
    if ( *p2_ != -1) {
        	*p2_ += offset_;
		if (((long) *p2_ < (long) pPackBeg) || ((long) *p2_ >= (long) pPackEnd)) {
	    		printf("Error: PackData (p_+5) Pointer Error!\n");
		    exit(1);
		}
	}
    }
    
if (close(packFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", packFileName);

    exit(1);
    }

}





void    inputImageTbl  (void)
{

static unsigned short i_;
static unsigned char j_, k_;

// input image table
strcpy(imageTblFileName, inputMasterFileName);
strcat(imageTblFileName, ".imageTbl");

printf("Inputting '%s'\n", imageTblFileName);

// open .imageTbl file for input
imageTblFile = open(imageTblFileName, O_RDONLY, 0);
if (imageTblFile == -1)
    {
    printf("Error: could not open '%s' for input\n", imageTblFileName);
    exit(1);
    }

// read data
imageTblOfstEnd = read(imageTblFile, pImageTblBeg, IMAGE_TBL_SIZE );
if (imageTblOfstEnd > IMAGE_TBL_SIZE)
    {
    printf("Error: overflowed buffer reading '%s', increase IMAGE_TBL_SIZE and recompile\n",
            imageTblFileName);
    exit(1);
    }
    
if (close(imageTblFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", imageTblFileName);
    exit(1);
    }

// expand ImageTbl MiscData Indexes
if (smallImageTbl) expandImageTbl();

// reverse order of char data word, and misc data triplet

for (i_ = 0 ; i_ < imageTblOfstEnd ; i_ += 5)
    {
    j_ = pImageTblBeg [i_ + 0];
    k_ = pImageTblBeg [i_ + 1];
    pImageTblBeg [i_ + 0] = k_;
    pImageTblBeg [i_ + 1] = j_;

    j_ = pImageTblBeg [i_ + 2];
    k_ = pImageTblBeg [i_ + 4];
    pImageTblBeg [i_ + 2] = k_;
    pImageTblBeg [i_ + 4] = j_;
    }
}





void    inputMImgTbl  (void)
{


// input master image talble indicies:
strcpy(mImgTblFileName, inputMasterFileName);
strcat(mImgTblFileName, ".mImgTbl");

printf("Inputting '%s'\n", mImgTblFileName);

// open file for input
mImgTblFile = open(mImgTblFileName, O_RDONLY, 0);
if (mImgTblFile == -1)
    {
    printf("Error: could not open '%s' for input\n", mImgTblFileName);
    exit(1);
    }

// read data until end of file

mImgTblOfstEnd = read(mImgTblFile, mImgTbl, MIMGTBL_SIZE + 1);
    
// check if input file too big
if (mImgTblOfstEnd > MIMGTBL_SIZE)
    {
    printf("Error: input data from '%s' overflowed mImgTbl buffer\n",
            mImgTblFileName);
    exit(1);
    }

numMasterImages = reverseWord( * (short*) (mImgTbl + 0) );
numImages =       reverseWord( * (short*) (mImgTbl + 2) );
imageNum = numImages * 4;

if (close(mImgTblFile) == -1)
    {
    printf("Error: could not close '%s' for input\n", mImgTblFileName);
    exit(1);
    }

}





void    inputMImgTblI  (void)
{


// input master image talble indicies:
strcpy(mImgTblIFileName, inputMasterFileName);
strcat(mImgTblIFileName, ".mImgTblI");

printf("Inputting '%s'\n", mImgTblIFileName);

// open file for input
mImgTblIFile = open(mImgTblIFileName, O_RDONLY, 0);
if (mImgTblIFile == -1)
    {
    printf("Error: could not open '%s' for input\n", mImgTblIFileName);
    exit(1);
    }

// input data
if ( read(mImgTblIFile, mImgTblI, 2 * numMasterImages) != 2 * numMasterImages )
    {
    printf("Error: could not read %d bytes from '%s'\n",
            2 * numMasterImages, mImgTblIFileName);
    exit(1);
    }

if (close(mImgTblIFile) == -1)
    {
    printf("Error: could not close '%s' for input\n", mImgTblIFileName);
    exit(1);
    }

}





void    inputCharData  (void)
{


// input char data
strcpy(charDataFileName, inputMasterFileName);
strcat(charDataFileName, ".charData");

printf("Inputting '%s'\n", charDataFileName);

// open file for input
charDataFile = open(charDataFileName, O_RDONLY, 0);
if (charDataFile == -1)
    {
    printf("Error: could not open '%s' for input\n", charDataFileName);
    exit(1);
    }

// read data until end of file

cdOfstEnd = read(charDataFile, pcd, CHAR_DATA_SIZE);
    
// check if input file too big
if (cdOfstEnd > CHAR_DATA_SIZE)
    {
    printf("Error: overflowed buffer reading '%s', increase CHAR_DATA_SIZE and recompile\n",
            charDataFileName);
    exit(1);
    }

if (close(charDataFile) == -1)
    {
    printf("Error: could not close '%s' for input\n", charDataFileName);
    exit(1);
    }

}





void    inputMiscData  (void)
{


// input misc data
strcpy(miscDataFileName, inputMasterFileName);
strcat(miscDataFileName, ".miscData");

printf("Inputting '%s'\n", miscDataFileName);

// open file for input
miscDataFile = open(miscDataFileName, O_RDONLY, 0);
if (miscDataFile == -1)
    {
    printf("Error: could not open '%s' for input\n", miscDataFileName);
    exit(1);
    }

// read data until end of file

mdOfstEnd = (long) read(miscDataFile, pmd, MISC_DATA_SIZE);
    
// check if input file too big
if (mdOfstEnd > MISC_DATA_SIZE)
    {
    printf("Error: overflowed buffer reading '%s', increase MISC_DATA_SIZE and recompile\n",
            miscDataFileName);
    exit(1);
    }

if (close(miscDataFile) == -1)
    {
    printf("Error: could not close '%s' for input\n", miscDataFileName);
    exit(1);
    }

}





void    outputFiles (void)
{

static char tempFileName[256];
static long tempAllowOutputFiles;


if (outputName1Time[0] != '\0')
    {
    tempAllowOutputFiles = allowOutputFiles;
    allowOutputFiles = 1;
    strcpy (tempFileName, masterFileName);
    strcpy (masterFileName, outputName1Time);
    if (outputName1Time[0] == '*')
        {
        masterFileName[0] = '\0';
        }
    }
else
    {
    if (masterFileName[0] == '\0')
        {
        printf("Error: you must specify a master filename in '%s'\n",
                imageListFileName);
        exit(1);
        }
    }


printf("\n");

outputPack();
outputMImgTblI();
outputMImgTbl();
outputImageTbl();
outputCharData();
outputMiscData();
outputCharDef();

if (outputName1Time[0] != '\0')
    {
    allowOutputFiles = tempAllowOutputFiles;
    strcpy (masterFileName, tempFileName);
    outputName1Time[0] = '\0';
    }
    
}





void    outputPack  (void)
{


if (!allowOutputFiles)
    return;

// output packed chars:
strcpy(packFileName, outDir);
strcat(packFileName, masterFileName);
strcat(packFileName, ".pack");

printf("Outputting '%s'\n", packFileName);

// open .pack file for output
packFile = open(packFileName, O_WRONLY | O_TRUNC | O_CREAT, 0);
if (packFile == -1)
    {
    printf("Error: could not open '%s' for output\n", packFileName);
    exit(1);
    }

// write 8 byte sumOfWeights (hi 4 bytes, then lo 4 bytes)
if ( write(packFile, &sumOfWeightsHi, 4) != 4 )
    goto err4;
if ( write(packFile, &sumOfWeightsLo, 4) != 4 )
    goto err4;
    
// write value of packNumEnd (#packed chars)
if ( write(packFile, &packNumEnd, 4) != 4 )
    {
    
    err4:

    printf("Error: could not write 4 bytes to '%s'\n",
            packFileName);
    exit(1);
    }
    
// write value of pPackBeg
if ( write(packFile, &pPackBeg, 4) != 4 )
    goto err4;
    
// write value of pPackEnd
if ( write(packFile, &pPackEnd, 4) != 4 )
    goto err4;
    
// write all pPack data
if ( write(packFile, pPackBeg, pPackEnd - pPackBeg) !=
            pPackEnd - pPackBeg )
    {
    printf("Error: could not write %d bytes to '%s'\n",
            pPackEnd - pPackBeg, packFileName);

    exit(1);
    }
    
if (close(packFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", packFileName);

    exit(1);
    }

}





void    outputMImgTbl  (void)
{

static long ofstTilt_, ofstScale_, ofstRotate_;
static char *pScale_, *pRotate_;
static unsigned char numScales_, numRotates_;


if (allowOutputFiles)
    {
    // output master image talble
    strcpy(mImgTblFileName, outDir);
    strcat(mImgTblFileName, masterFileName);
    strcat(mImgTblFileName, ".mImgTbl");

    printf("Outputting '%s'\n", mImgTblFileName);

    // open file for output
    mImgTblFile = open(mImgTblFileName, O_WRONLY | O_TRUNC | O_CREAT, 0);
    if (mImgTblFile == -1)
        {
        printf("Error: could not open '%s' for output\n", mImgTblFileName);
        exit(1);
        }
    }

// compress data for last master image to Randy's format

* (short*) (mImgTbl + 0) = reverseWord( numMasterImages );
* (short*) (mImgTbl + 2) = reverseWord( numImages );

ofstTilt_ = mImgTblOfstEnd;

pScale_ = pScaleBeg;
pRotate_ = pRotateBeg;

mImgTbl[ofstTilt_++] = (unsigned char) numTilts;

ofstRotate_ = ofstTilt_ + 2 * numTilts;

do
    {
    
    // loop for each tilt

    ofstScale_ = ofstRotate_;

    * (short*) (mImgTbl + ofstTilt_) = reverseWord( ofstScale_ );
    ofstTilt_ += 2;

    numScales_ = *pScale_++;

    mImgTbl[ofstScale_++] = numScales_;
    
    ofstRotate_ = ofstScale_ + 2 * numScales_;

    do
        {
        
        // loop for each scale

        * (short*) (mImgTbl + ofstScale_) = reverseWord( ofstRotate_ );
        ofstScale_ += 2;

        numRotates_ = *pRotate_++;

        mImgTbl[ofstRotate_++] = numRotates_;
        
        do
            {
            
            // loop for each rotate

            // transfer image #
            * (short*) (mImgTbl + ofstRotate_) =
                    reverseWord( * (short*) pRotate_ );
            ofstRotate_ += 2;
            pRotate_ += 2;
    
            if (ofstRotate_ > 0x10000)
                {
                printf("Error: mImgTbl overflowed 64K, reduce images or change database structure\n");
                exit(1);
                }

            }
            while (--numRotates_ > 0);

        }
        while (--numScales_ > 0);

    }
    while (--numTilts > 0);

mImgTblOfstEnd = ofstRotate_;

if (allowOutputFiles)
    {
    // write data
    if ( write(mImgTblFile, mImgTbl, mImgTblOfstEnd) != mImgTblOfstEnd )
        {
        printf("Error: could not write %d bytes to '%s'\n",
                mImgTblOfstEnd, mImgTblFileName);
        exit(1);
        }

    if (close(mImgTblFile) == -1)
        {
        printf("Error: could not close '%s' for output\n", mImgTblFileName);
        exit(1);
        }
    }

}





void    outputMImgTblI  (void)
{


if (!allowOutputFiles)
    return;

// output master image talble indicies:
strcpy(mImgTblIFileName, outDir);
strcat(mImgTblIFileName, masterFileName);
strcat(mImgTblIFileName, ".mImgTblI");

printf("Outputting '%s'\n", mImgTblIFileName);

// open file for output
mImgTblIFile = open(mImgTblIFileName, O_WRONLY | O_TRUNC | O_CREAT, 0);
if (mImgTblIFile == -1)
    {
    printf("Error: could not open '%s' for output\n", mImgTblIFileName);
    exit(1);
    }

// write data
if ( write(mImgTblIFile, mImgTblI, 2 * numMasterImages) != 2 * numMasterImages )
    {
    printf("Error: could not write %d bytes to '%s'\n",
            2 * numMasterImages, mImgTblIFileName);
    exit(1);
    }

if (close(mImgTblIFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", mImgTblIFileName);
    exit(1);
    }

}





void    outputImageTbl  (void)
{

static unsigned short i_;
static unsigned char j_, k_;


if (!allowOutputFiles)
    return;

// output image table
strcpy(imageTblFileName, outDir);
strcat(imageTblFileName, masterFileName);
strcat(imageTblFileName, ".imageTbl");

printf("Outputting '%s'\n", imageTblFileName);

// reverse order of char data word, and misc data triplet

for (i_ = 0 ; i_ < imageTblOfstEnd ; i_ += 5)
    {
    j_ = pImageTblBeg [i_ + 0];
    k_ = pImageTblBeg [i_ + 1];
    pImageTblBeg [i_ + 0] = k_;
    pImageTblBeg [i_ + 1] = j_;

    j_ = pImageTblBeg [i_ + 2];
    k_ = pImageTblBeg [i_ + 4];
    pImageTblBeg [i_ + 2] = k_;
    pImageTblBeg [i_ + 4] = j_;
    }

// shrink ImageTbl MiscData Indexes
if (smallImageTbl) shrinkImageTbl();

// open file for output
imageTblFile = open(imageTblFileName, O_WRONLY | O_TRUNC | O_CREAT, 0);
if (imageTblFile == -1)
    {
    printf("Error: could not open '%s' for output\n", imageTblFileName);
    exit(1);
    }

// write data
if ( write(imageTblFile, pImageTblBeg, imageTblOfstEnd) != imageTblOfstEnd)
    {
    printf("Error: could not write %d bytes to '%s'\n",
            imageTblOfstEnd, imageTblFileName);
    exit(1);
    }

if (close(imageTblFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", imageTblFileName);
    exit(1);
    }

// expand ImageTbl MiscData Indexes
if (smallImageTbl) expandImageTbl();

// reverse order of char data word, and misc data triplet

for (i_ = 0 ; i_ < imageTblOfstEnd ; i_ += 5)
    {
    j_ = pImageTblBeg [i_ + 0];
    k_ = pImageTblBeg [i_ + 1];
    pImageTblBeg [i_ + 0] = k_;
    pImageTblBeg [i_ + 1] = j_;

    j_ = pImageTblBeg [i_ + 2];
    k_ = pImageTblBeg [i_ + 4];
    pImageTblBeg [i_ + 2] = k_;
    pImageTblBeg [i_ + 4] = j_;
    }

}





void    outputCharData      (void)
{


if (!allowOutputFiles)
    return;

// output char data
strcpy(charDataFileName, outDir);
strcat(charDataFileName, masterFileName);
strcat(charDataFileName, ".charData");

printf("Outputting '%s'\n", charDataFileName);

// open file for output
charDataFile = open(charDataFileName, O_WRONLY | O_TRUNC | O_CREAT, 0);
if (charDataFile == -1)
    {
    printf("Error: could not open '%s' for output\n", charDataFileName);
    exit(1);
    }

// write data
if ( write(charDataFile, pcd, cdOfstEnd) != cdOfstEnd)
    {
    printf("Error: could not write %d bytes to '%s'\n",
            cdOfstEnd, charDataFileName);
    exit(1);
    }

if (close(charDataFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", charDataFileName);
    exit(1);
    }

}





void    outputMiscData      (void)
{


if (!allowOutputFiles)
    return;

// output misc data
strcpy(miscDataFileName, outDir);
strcat(miscDataFileName, masterFileName);
strcat(miscDataFileName, ".miscData");

printf("Outputting '%s'\n", miscDataFileName);

// open file for output
miscDataFile = open(miscDataFileName, O_WRONLY | O_TRUNC | O_CREAT, 0);
if (miscDataFile == -1)
    {
    printf("Error: could not open '%s' for output\n", miscDataFileName);
    exit(1);
    }

// write data
if ( write(miscDataFile, pmd, mdOfstEnd) != mdOfstEnd)
    {
    printf("Error: could not write %ld bytes to '%s'\n",
            mdOfstEnd, miscDataFileName);
    exit(1);
    }

if (close(miscDataFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", miscDataFileName);
    exit(1);
    }

}





void    outputCharDef   (void)
{

/*
    this will recursively scan pPack tree and output chars
      in bit plane format
*/


if (!allowOutputFiles)
    return;

strcpy (charDefFileName, outDir);
strcat (charDefFileName, masterFileName);
strcat (charDefFileName, ".charDef");

printf("Outputting '%s'\n", charDefFileName);

// open .charDef file for output
charDefFile = open(charDefFileName, O_RDWR | O_TRUNC | O_CREAT, 0);
if (charDefFile == -1)
    {
    printf("Error: could not open '%s' for output\n", charDefFileName);

    exit(1);
    }

outputCharDefSub (0, 0, pPackBeg, pChar);

if ( write(charDefFile, pCopy, packNumEnd << 5) != (packNumEnd << 5) )
    {
    printf("Error: could not write %d bytes to '%s'\n",
            packNumEnd << 5, charDefFileName);

    exit(1);
    }
    
if (close(charDefFile) == -1)
    {
    printf("Error: could not close '%s' for output\n", charDefFileName);

    exit(1);
    }

}





void    outputCharDefSub (long x_, long y_, char *pPack_, char *pChar_)
{

char        *son_, *sibling_; // must be recursive

static char *pEncode_, c_;
static long mask_;

// get pixel
*pChar_ = *pPack_;

son_ =     * (char**) (pPack_ + 1);
sibling_ = * (char**) (pPack_ + 5);

if (x_ < 7)
    {
    outputCharDefSub( x_ + 1, y_, son_, pChar_ + 1 );
    if ( (long)sibling_ != -1 )
        outputCharDefSub( x_, y_, sibling_, pChar_ );
    }
else
    {
    if (y_ < 7)
        {
        outputCharDefSub( 0, y_ + 1, son_, pChar_ + (0x100 - 7) );
        if ( (long)sibling_ != -1 )
            outputCharDefSub( x_, y_, sibling_, pChar_ );
        }
    else
        {
        
        // encode char in bit planes

        // note son_ is now character # 0,1,2,...
        
        if ( (long)son_ >= 0xC000 )
            {
            printf("Error: # of chars >= 0xC000.  Reduce images or change database.\n");
            exit(1);
            }

        pEncode_ = (char*)pCopy + ( (long)son_ << 5 );
        
        * (long*) (pEncode_ + 0x00) = 0;
        * (long*) (pEncode_ + 0x04) = 0;
        * (long*) (pEncode_ + 0x08) = 0;
        * (long*) (pEncode_ + 0x0c) = 0;
        * (long*) (pEncode_ + 0x10) = 0;
        * (long*) (pEncode_ + 0x14) = 0;
        * (long*) (pEncode_ + 0x18) = 0;
        * (long*) (pEncode_ + 0x1c) = 0;

        for (y_ = 0 ; y_ <= 7 ; y_++)
            {
            pChar_ = pChar + (y_ << 8);
            mask_ = 0x80;
            
            for (x_ = 0 ; x_ <= 7 ; x_++)
                {
                c_ = pChar_[x_];
                
                if (c_ & 1)
                    pEncode_[0] += mask_;
                if (c_ & 2)
                    pEncode_[1] += mask_;
                if (c_ & 4)
                    pEncode_[16] += mask_;
                if (c_ & 8)
                    pEncode_[17] += mask_;
                    
                mask_ = mask_ >> 1; // unsigned
                }
                
            pEncode_ += 2;
            }                    
            
        if ( (long)sibling_ != -1 )
            outputCharDefSub( 7, 7, sibling_, pChar + 0x707 );
        }
    }        
}





long    packChar    (char *pChar_)
{

/*
    Append the image char to the packed chars.
    If the char already has been packed, then that packed # will be
      returned without further packing.
    Note the value of 'columns' is ignored

    Will return packed char # for it
*/

static char    c_;
static long    x_, y_;
static char    *pPack_, *p_;


pPack_ = pPackBeg;

for (y_ = 0 ; y_ <= 7 ; y_++)
    {
    for (x_ = 0 ; x_ <= 7 ; x_++)
        {
        c_ = *pChar_++;
        if (c_ != *pPack_)
            {
            while ( (long) (p_ = * (char**) (pPack_ + 5) ) >= 0)
                {
                pPack_ = p_;
                if (c_ == *pPack_)
                    goto gotPixel;
                }
            * (char**) (pPack_+5) = pPackEnd;
            goto noMatch;
            }
        else
            {
            gotPixel:
            pPack_ = * (char**) (pPack_ + 1);
            }
        }
    pChar_ += 0xf8;
    }       

// packed char already exists

return( (long)pPack_ );



// (above jump to noMatch will be inside the loop below)

for (y_ = 0 ; y_ <= 7 ; y_++)
    {
    for (x_ = 0 ; x_ <= 7 ; x_++)
        {
        
        // debug patch
        if ( (long)pChar_ < (long)pChar ||
             (long)pChar_ >= (long)pXYChar + 0x10000)
            {
            printf("Error:  packChar software error #1\n");
            exit(1);
            }

        c_ = *pChar_++;
        
        noMatch:
        
        pPack_ = pPackEnd;
        pPackEnd += 9;
        
        if ( (long)pPackEnd > (long)pPackBeg + packSize )
            {
            printf("Error: overflowed buffer, increase PACK_MEM_SIZE and recompile\n");
            exit(1);
            }

        *pPack_ = c_;
        * (char**) (pPack_ + 1) = pPackEnd;
        * (char**) (pPack_ + 5) = (char*)-1;
        }
    pChar_ += 0xf8;
    }

// set packed char # at bottom node:

* (long*) (pPack_ + 1) = packNumEnd;

return(packNumEnd++); // (long word, 0:64K-2)
}





unsigned short packCharDataOffset (long charNum)
{

if (charNum == 0)
    return(0);
else
    return((unsigned short) (charNum + charNumOfst) );
}





void    packCharData    (void)
{

static long             *p1x1_, *p2x2_;
static unsigned short   cdCharNumNT_;
static unsigned short   cdCharNumNT__;
static unsigned short   bestCdCharNumNT_;
static unsigned short   cdOfstTry_;
static unsigned short   *pCharDataBuf_;
static unsigned short   length_;
static unsigned short   bestLength_;
static unsigned char    bestCmd_;
static long             bestBytesPerCharNum_;
static unsigned short   bestCdOfst_;
static long             bytesPerCharNum_;


// create charDataBuf with packed char #'s

pCharDataBufEnd = pCharDataBufBeg;

for (pSort = pSortBeg ; pSort != pSortEnd ; pSort++)
    {
    if ( (long)*pSort >= (long)p2x2Beg ) // 2x2
        {
        p2x2_ = (long*) *pSort;
        if ( p2x2_[_2X2_COPY] != -1 )
            p2x2_ = (long*) p2x2_[_2X2_COPY];

        *pCharDataBufEnd++ = packCharDataOffset (p2x2_[UL_COPY]);
        *pCharDataBufEnd++ = packCharDataOffset (p2x2_[UR_COPY]);
        *pCharDataBufEnd++ = packCharDataOffset (p2x2_[LL_COPY]);
        *pCharDataBufEnd++ = packCharDataOffset (p2x2_[LR_COPY]);
        }
    else
        {
        p1x1_ = (long*) *pSort;
        if (p1x1_[COPY] >= 0x10000)
            p1x1_ = ( (long*)p1x1_[COPY] ) - CHAR;  // ok even if ptr to char
                                                    //   within 2x2
        *pCharDataBufEnd++ = packCharDataOffset (p1x1_[COPY]);
        }

    if ( (long)pCharDataBufEnd > (long)pCharDataBufMax)
        {
        printf("Error: overflowed buffer, increase CHAR_DATA_BUF_SIZE and recompile\n");
        exit(1);
        }
    }

/*
    note format for each cmd is:

    meaning            cmd  bits
    --------------------------------------------------------------------
    1 char:             00  bbbbbHHH    (note bbbbbHHH hhhhhLLL = 0:$bfff)
                            hhhhhLLL

                            adrs = 000bbbbb HHHhhhhh LLL00000

    duplicate           C0  110nnnnn
      2:33 chars            LLLLLLLL
                            hhhhhhhh

                            #chars = nnnnn + 2
                            bank = bank of charData
                            adrs = hhhhhhhh LLLLLLLL

    1:16 sequential     E0  1110nnnn
      chars
                            #chars = nnnn + 1
                            1st char# = last non-transparent char# + 1

    repeat 1:8 chars    F0  11110nnn

                            #chars = nnn + 1
                            char# = last non-transparent char#

    1:4 transparent     F8  111110nn
      chars
                            #chars = nn + 1
                            char# = 0

    duplicate           F8  111111mm
      34: 1023+34 chars     nnnnnnnn
                            LLLLLLLL
                            hhhhhhhh

                            #chars = mm nnnnnnnn + 34
                            bank = bank of charData
                            adrs = hhhhhhhh LLLLLLLL
*/

// pack buffer to above cmds:

if (imageTblOfstEnd >= IMAGE_TBL_SIZE)
    {
    if (IMAGE_TBL_SIZE == (0x10000 / 5 * 5) )
        printf("Error: overflowed 64K in image table\n");
    else
        printf("Error: overflowed buffer, increase IMAGE_TBL_SIZE and recompile\n");
    exit(1);
    }

// this is usually true unless entire buffer is a copy
move ( & pImageTblBeg[imageTblOfstEnd], (char*) & cdOfstEnd, 2);

pCharDataBuf = pCharDataBufBeg;

cdCharNumNT_ = 0;

while (pCharDataBuf != pCharDataBufEnd)
    {
    
    // get best cmd

    // assume 1 char cmd to be used
    bestCmd_ = 0x00;
    bestBytesPerCharNum_ = 2 << 16;

    // get longest duplication (must be >= 2 if used)
    bestLength_ = 1;
    
    for (cdOfstTry_ = 0 ; cdOfstTry_ < cdOfstEnd ; cdOfstTry_++)
        {
        
        // get # of duplications of char #'s

        // init stuff for cdNext:
        cdMode = 0;
        cdOfst = cdOfstTry_;
        cdNumDups = -1;
        // cdNumReps ignored
        // cdCharNum ignored
        cdCharNumNT = cdCharNumNT_;
        spw = spwMax;

        // init stuff for buffer:
        pCharDataBuf_ = pCharDataBuf - 1;

        do
            {
            pCharDataBuf_++;
            cdCharNumNT__ = cdCharNumNT; // save value before possibly
                                         //   trashed in next line
            cdNext(); // get next char # from charData (or 0xffff if none)
            }
            while ( cdCharNum == *pCharDataBuf_ &&
                pCharDataBuf_ != pCharDataBufEnd );

        // check for longest length
        length_ = ( (long)pCharDataBuf_ - (long)pCharDataBuf ) >> 1;
        if (length_ > bestLength_)
            {
            bestLength_ = length_;
            bestCdOfst_ = cdOfstTry_;
            bestCdCharNumNT_ = cdCharNumNT__;
            }
        }

    if (bestLength_ >= 2)
        {
        if (bestLength_ == ( ( (long)pCharDataBufEnd -
                (long)pCharDataBufBeg) >> 1) )
            {
            // entire char data is a duplicate, don't need any new char data 
            move ( & pImageTblBeg[imageTblOfstEnd], (char*) & bestCdOfst_, 2);
            imageTblOfstEnd += 2;
            return;
            }

        if (bestLength_ <= 33)
            {
            bestCmd_ = 0xC0;
            bestBytesPerCharNum_ = (3 << 16) / bestLength_;
            }
        else
            {
            bestCmd_ = 0xFC;
            bestBytesPerCharNum_ = (4 << 16) / bestLength_;
            }
        }

    if (*pCharDataBuf == 0) // transparent
        {

        // try 1:4 transparent chars

        length_ = 0;
            
        pCharDataBuf_ = pCharDataBuf;
        while ( 0 == *pCharDataBuf_ &&
                length_ < 4 &&
                pCharDataBuf_ < pCharDataBufEnd)

            {
            length_++;
            pCharDataBuf_++;
            }

        bytesPerCharNum_ = (1 << 16) / length_;
        if (bytesPerCharNum_ < bestBytesPerCharNum_)
            {
            bestBytesPerCharNum_ = bytesPerCharNum_;
            bestCmd_ = 0xF8;
            bestLength_ = length_;
            }
        }

    else
        {

        // try sequential chars

        length_ = 0;
            
        pCharDataBuf_ = pCharDataBuf;
        cdCharNum = cdCharNumNT_ + 1;
        while ( cdCharNum == *pCharDataBuf_ &&
                length_ < 16 &&
                pCharDataBuf_ < pCharDataBufEnd)

            {
            length_++;
            cdCharNum++;
            pCharDataBuf_++;
            }

        if (length_ != 0)
            {
            bytesPerCharNum_ = (1 << 16) / length_;
            if (bytesPerCharNum_ < bestBytesPerCharNum_)
                {
                bestBytesPerCharNum_ = bytesPerCharNum_;
                bestCmd_ = 0xE0;
                bestLength_ = length_;
                }
            }

        // try repeat chars

        length_ = 0;
            
        pCharDataBuf_ = pCharDataBuf;
        while ( cdCharNumNT_ == *pCharDataBuf_ &&
                length_ < 8 &&
                pCharDataBuf_ < pCharDataBufEnd)

            {
            length_++;
            pCharDataBuf_++;
            }

        if (length_ != 0)
            {
            bytesPerCharNum_ = (1 << 16) / length_;
            if (bytesPerCharNum_ < bestBytesPerCharNum_)
                {
                bestBytesPerCharNum_ = bytesPerCharNum_;
                bestCmd_ = 0xF0;
                bestLength_ = length_;
                }
            }
        }

    // now do best choice

    // ensure room for 1 byte
    if (cdOfstEnd >= CHAR_DATA_SIZE)
        {
        noMem:

        printf("Error: overflowed 64K in charData\n");
        exit(1);
        }

    if (bestCmd_ == 0x00) // 1 char
        {
        // ensure room for 2 byte
        if (cdOfstEnd + 1 >= CHAR_DATA_SIZE)
            goto noMem;

        pcd[cdOfstEnd++] = *pCharDataBuf >> 8;
        pcd[cdOfstEnd++] = *pCharDataBuf & 0xff;
        if (*pCharDataBuf != 0)
            cdCharNumNT_ = *pCharDataBuf;
        pCharDataBuf++;
        }
    else
    if (bestCmd_ == 0xC0) // 2:33 duplicates
        {
        // ensure room for 3 bytes
        if (cdOfstEnd + 2 >= CHAR_DATA_SIZE)
            goto noMem;

        pcd[cdOfstEnd++] = 0xC0 + bestLength_ - 2;
        goto duplicate;
        }
    else
    if (bestCmd_ == 0xE0) // 1:16 sequential
        {
        pcd[cdOfstEnd++] = 0xE0 + bestLength_ - 1;
        cdCharNumNT_ = cdCharNumNT_ + bestLength_;
        pCharDataBuf += bestLength_;
        }
    else
    if (bestCmd_ == 0xF0) // 1:8 repeats
        {
        pcd[cdOfstEnd++] = 0xF0 + bestLength_ - 1;
        pCharDataBuf += bestLength_;
        }
    else
    if (bestCmd_ == 0xF8) // 1:4 transpanents
        {
        pcd[cdOfstEnd++] = 0xF8 + bestLength_ - 1;
        pCharDataBuf += bestLength_;
        }
    else
        {
        // duplicate 34: 1023+34

        // ensure room for 4 bytes
        if (cdOfstEnd + 3 >= CHAR_DATA_SIZE)
            goto noMem;

        pcd[cdOfstEnd++] = 0xFC + ( (bestLength_ - 34) >> 8 );
        pcd[cdOfstEnd++] = (bestLength_ - 34) & 0xff;
        
        duplicate:

        pcd[cdOfstEnd++] =  bestCdOfst_ & 0xff;
        pcd[cdOfstEnd++] =  bestCdOfst_ >> 8;
        cdCharNumNT_ = bestCdCharNumNT_;
        pCharDataBuf += bestLength_;
        }
    }
imageTblOfstEnd += 2;
}





void    cdNext  (void)
{

/*
    get next charData char# from database
    
    in/out:
        cdMode = 0:3                                (initially 0)
        cdOfst = 0:$fffb                            (initially whatever)
        cdNumDups = 0:1023+34, or neg to disable    (initially neg)
        cdNumReps = 0:16                            (initially can ignore)
        cdCharNum = 0:$bfff                         (initially can ignore)
        cdCharNumNT = 0:$bfff                       (initially 0)
        spw = ptr to top of data stack (excl)       (initially spwMax)
*/


//debug1
//printf("spw = $%x\n", spw);

if (cdMode == 0) // ready for new cmd
    {

    newCmd0:
    
    if (cdNumDups == 0) // time to return to caller
        {

        // cdMode will stay 0
        
        // restore adrs after duplicate cmd
        cdOfst = *spw++;

        // restore #duplicates (if any) left to do after duplicate
        cdNumDups = *spw++;

        goto newCmd0;
        }

    // decode new cmd

    // ensure room for 1 byte
    if ( cdOfst >= cdOfstEnd)
        {
        noMem:

        cdCharNum = 0xffff; // ran out of memory
        return; 
        }

    if (pcd[cdOfst] < 0xC0) // (1 char #)
        {
        // ensure room for 2 bytes
        if (cdOfst + 1 >= cdOfstEnd)
            goto noMem;

        // get char # (0:$bfff)
        cdCharNum = (pcd[cdOfst] << 8) + pcd[cdOfst+1];

        cdOfst += 2;
        
        // last non-transparent char #
        cdCharNumNT = cdCharNum;

        if (cdNumDups >= 0)
            cdNumDups--;

        // cdMode will stay 0
        
        return;
        }
    
    if (pcd[cdOfst] < 0xE0) // duplicate 2:33 cmd
        {
        // ensure room for 3 bytes
        if (cdOfst + 2 >= cdOfstEnd)
            goto noMem;

        // get # of duplications of char #'s (2:33)
        cdNumReps = (pcd[cdOfst++] & 0x1f) + 2;
        
        goto cdDuplicate;
        }

    if (pcd[cdOfst] < 0xF0) // 1:16 sequential char #'s cmd
        {
        // in sequential cmd mode
        cdMode = 1;

        // get # chars, 1:16
        cdNumReps = (pcd[cdOfst++] & 0x0f) + 1;

        if (cdNumDups >= 0) // if in duplicate mode
            {
            if (cdNumDups < cdNumReps) // don't do too many
                cdNumReps = cdNumDups;
                
            cdNumDups -= cdNumReps;
            }
            
        goto newCmd1to3;
        }

    if (pcd[cdOfst] < 0xF8) // repeat 1:8 char #'s cmd
        {
        // in repeat cmd mode
        cdMode = 2;

        // get # chars, 1:8
        cdNumReps = (pcd[cdOfst++] & 0x07) + 1;

        if (cdNumDups >= 0) // if in duplicate mode
            {
            if (cdNumDups < cdNumReps) // don't do too many
                cdNumReps = cdNumDups;
                
            cdNumDups -= cdNumReps;
            }
            
        goto newCmd1to3;
        }

    if (pcd[cdOfst] < 0xFc) // 1:4 transparent chars cmd
        {
        // in transparent mode
        cdMode = 3;

        // get # chars, 1:4
        cdNumReps = (pcd[cdOfst++] & 0x03) + 1;

        if (cdNumDups >= 0) // if in duplicate mode
            {
            if (cdNumDups < cdNumReps) // don't do too many
                cdNumReps = cdNumDups;
                
            cdNumDups -= cdNumReps;
            }
            
        goto newCmd1to3;
        }

    // only thing left is uplicate 34: 1023+34 char #'s cmd

    // ensure room for 4 bytes
    if (cdOfst + 3 >= cdOfstEnd)
        goto noMem;

    // get # of duplications of char #'s (34 : 1023+34)
    cdNumReps = ( (pcd[cdOfst] & 0x03) << 8 ) + pcd[cdOfst+1] + 34;
    
    cdOfst += 2;

    cdDuplicate: // code for 2:33 duplicates joins here

    if (cdNumDups >= 0) // if in duplicate mode
        {
        if (cdNumDups < cdNumReps) // don't do too many
            cdNumReps = cdNumDups;
            
        cdNumDups -= cdNumReps;
        }
            
    if ( (long) (spw - 2) < (long) spwBeg )
        goto noMem;
//        {
//        printf("Error: overflowed stack, increase SPW_SIZE and recompile\n");
//        exit(1);
//        }

    // save #duplications remaining after duplications done
    *--spw = cdNumDups;

    // save ofst for next cmd after duplications done
    *--spw = cdOfst + 2;    

    //debug1
    //if ( (long)spw < (long)spwMax - 0x8 )
    //    printf("low spw = $%x\n", spw);

    cdNumDups = cdNumReps;

    // get offset for start of duplicated char #'s
    cdOfst = pcd[cdOfst] + ( pcd[cdOfst+1] << 8 );

    // cdMode stays 0

    goto newCmd0;
    }

// in sequential, repeat, or transparent modes

if (cdNumReps == 0) // if end of cmd causing mode
    {
    cdMode = 0;
    
    goto newCmd0;
    }

newCmd1to3:

cdNumReps--;
        
if (cdMode == 1) // if sequential mode
    {
    cdCharNum = ++cdCharNumNT;
    return;
    }

if (cdMode == 2) // if repeat mode
    {
    cdCharNum = cdCharNumNT;
    return;
    }

// transparent mode

cdCharNum = 0;
}



                        
                    
void    packMiscData    (void)
{

/*
    output oam data in Randy's format
*/

static long             *p1x2_, flags_, _2x2_;
static unsigned short   imageTblOfst_;
static unsigned long   mdOfst1_ = 0, mdOfst2_ = 0, mdOfst3_ = 0;


retry:

mdOfst3_ = mdOfstEnd;

// assume no copy of previous data
move ( & pImageTblBeg[imageTblOfstEnd], (char *) ((long) &mdOfstEnd + 1), 3);
if (imageTblOfstEnd >= IMAGE_TBL_SIZE)
    {
    if (IMAGE_TBL_SIZE == (0x10000 / 5 * 5) )
        printf("Error: overflowed 64K in image table\n");
    else
        printf("Error: overflowed buffer, increase IMAGE_TBL_SIZE and recompile\n");
    exit(1);
    }

// allocate oam data assuming no copy of previous data
if ( (long)mdOfstEnd + 6 > MISC_DATA_SIZE)
    goto noMem;



/* 

// offset from center to leftmost visible pixel / 2
// (this will point to leftmost pixel or 1 to right of it)
pmd[mdOfstEnd++] = (leftX - 128 + 1) / 2;

// offset from center to rightmost visible pixel (exclusive) / 2
// (this will point to either rightmost pixel or 1 to right of it)
pmd[mdOfstEnd++] = (rightX - 128 + 1) / 2;

// offset from center to topmost visible pixel / 2
// (this will point to topmost pixel or 1 below it)
pmd[mdOfstEnd++] = (topY - 128 + 1) / 2;

// offset from center to bottommost visible pixel (exclusive) / 2
// (this will point to bottommost pixel or 1 below it)
pmd[mdOfstEnd++] = (bottomY - 128 + 1) / 2;

*/


// # of 2x2's
if (!only1x1)
    {
        pmd[mdOfstEnd++] = ( (long)p2x2End - (long)p2x2Beg ) / (4 * _2X2_SIZE);
    }


// # of 1x1's
pmd[mdOfstEnd++] = ( (long)p1x1End - (long)p1x1Beg ) / (4 * _1X1_SIZE);

// output all objs, sorted with topmost 1st, (leftmost breaks any ties)
for (pSort = pSortBeg ; pSort != pSortEnd ; pSort++)
    {
    if ( (long)mdOfstEnd + 3 > MISC_DATA_SIZE)
        {
        noMem:

        printf("Error: buffer overflow, increase MISC_DATA_SIZE and recompile\n");
        exit(1);
        }

    p1x2_ = (long*) *pSort;

    // set oam flags:

    _2x2_ = (*pSort >= (long)p2x2Beg) ? 1 : 0;

    // bit 7 = 1 if 2x2
    flags_ = _2x2_ << 7;

    // bit 6 = 1 if y flip
    if (_2x2_)
        {
        if ( p1x2_[UL_CHAR] & 0x20000 )
            flags_ += 0x40;
        }
    else
        {
        if ( p1x2_[CHAR] & 0x20000 )
            flags_ += 0x40;
        }
    
    // bit 5 = 1 if x flip
    if (_2x2_)
        {
        if ( p1x2_[UL_CHAR] & 0x10000 )
            flags_ += 0x20;
        }
    else
        {
        if ( p1x2_[CHAR] & 0x10000 )
            flags_ += 0x20;
        }
    
    // bit 4 = sign of hznt offset
    if ( p1x2_[UL_X] - 128 < 0 )        
        flags_ += 0x10;

    // bit 3 = sign of vert offset
    if ( p1x2_[UL_Y] - 128 < 0 )        
        flags_ += 0x08;

    // bits 2:0 = color palette
    flags_ += palette;

    if (only1x1)
        {
            flags_ &= 0xe7;
            flags_ <<= 1;
            flags_ |= (priority << 4);
            pmd[mdOfstEnd++] = (flags_ | 1);

            // (oam x pixel offset)
            pmd[mdOfstEnd++] = ( p1x2_[UL_X] - 128 ) & 0xff;

            // (oam y pixel offset)
            pmd[mdOfstEnd++] = ( p1x2_[UL_Y] - 128 ) & 0xff;
        }

    if (!only1x1)
        {
            pmd[mdOfstEnd++] = flags_;

            // abs(oam x pixel offset)
            pmd[mdOfstEnd++] = labs( p1x2_[UL_X] - 128 );

            // abs(oam y pixel offset)
            pmd[mdOfstEnd++] = labs( p1x2_[UL_Y] - 128 );
        }


    }

// scan previous images to see if misc data is copied

for (imageTblOfst_ = 2 ; imageTblOfst_ < imageTblOfstEnd ; imageTblOfst_ += 5)
    {
    move ( (char*) ((long) &mdOfst1_ + 1), & pImageTblBeg[imageTblOfst_], 3);
    move ( (char*) ((long) &mdOfst2_ + 1), & pImageTblBeg[imageTblOfstEnd], 3);
    while (mdOfst2_ < mdOfstEnd)
        {

        /*
	// debug check
        if ((long) &pmd[mdOfst1_] >= 0x08000000 || (long) &pmd[mdOfst2_] >= 0x08000000)
            {
            // put breakpoint here!!!!!!!!!
            imageTblOfst_++;
            imageTblOfst_--;
            }
	*/

        if (pmd[mdOfst1_++] != pmd[mdOfst2_++])
            goto noMatch;
        }

    // got a match!
    move ( (char*) ((long) &mdOfstEnd + 1), & pImageTblBeg[imageTblOfstEnd], 3);
    move ( & pImageTblBeg[imageTblOfstEnd], & pImageTblBeg[imageTblOfst_], 3);
    imageTblOfstEnd += 3;

    // note misc data can't cross bank as there is no new misc data
    
    return;

    noMatch:
    ;

    }

// no match with any previous image

// check if bank crossed
if ( (mdOfst3_ ^ (mdOfstEnd-1) ) & 0x10000 )
    {
    mdOfstEnd = mdOfstEnd & 0xffff0000;
    goto retry;
    }

imageTblOfstEnd += 3;

}





void    packObjs  (void)
{

/*
    prepare objects for output to master files
*/

long    *p1x1_, *p2x2_;


// sort 1x1's and 2x2's by upper left x and y

pSortEnd = pSortBeg;

for (p2x2_ = p2x2Beg ; p2x2_ < p2x2End ; p2x2_ += _2X2_SIZE)
    {
    *pSortEnd++ =  ( (0x80 + p2x2_[UL_Y]) << 24) +
                (p2x2_[UL_X] << 16) +
                0x8000 +
                ( (long)p2x2_ - (long)p2x2Beg ) / (4 * _2X2_SIZE);
    }

for (p1x1_ = p1x1Beg ; p1x1_ < p1x1End ; p1x1_ += _1X1_SIZE)
    {
    *pSortEnd++ =  ( (0x80 + p1x1_[UL_Y]) << 24) +
                (p1x1_[UL_X] << 16) +
                ( (long)p1x1_ - (long)p1x1Beg ) / (4 * _1X1_SIZE);
    }

lqsort(pSortBeg, ( (long)pSortEnd - (long)pSortBeg ) / 4);

// put adrs of 1x1 or 2x2 in sort buffer
for (pSort = pSortBeg ; pSort != pSortEnd ; pSort++)
    {
    if (*pSort & 0x8000) // 2x2
        *pSort = (long)p2x2Beg + (*pSort & 0x7fff) * (4 * _2X2_SIZE);
    else
        *pSort = (long)p1x1Beg + (*pSort & 0x7fff) * (4 * _1X1_SIZE);
    }

// change new chars to packed chars:

for (pSort = pSortBeg ; pSort != pSortEnd ; pSort++)
    {
    if ( (long)*pSort >= (long)p2x2Beg ) // 2x2
        {
        p2x2_ = (long*) *pSort;
        if (p2x2_[_2X2_COPY] != -1) // (copy of 2x2)
            p2x2_ = (long*) p2x2_[_2X2_COPY];

		if (p2x2_[UL_COPY] < 0) // (new char)
			p2x2_[UL_COPY] = packChar( (char*)p2x2_[UL_CHAR]); // pack it
		else
    		if (p2x2_[UL_COPY] >= 0x10000) // (copy of char)
	    		p2x2_[UL_COPY] = packChar( *( (char**)p2x2_[UL_COPY] ) );
				
		if (p2x2_[UR_COPY] < 0) // (new char)
			p2x2_[UR_COPY] = packChar( (char*)p2x2_[UR_CHAR]); // pack it
		else
    		if (p2x2_[UR_COPY] >= 0x10000) // (copy of char)
	    		p2x2_[UR_COPY] = packChar( *( (char**)p2x2_[UR_COPY] ) );
				
		if (p2x2_[LL_COPY] < 0) // (new char)
			p2x2_[LL_COPY] = packChar( (char*)p2x2_[LL_CHAR]); // pack it
		else
    		if (p2x2_[LL_COPY] >= 0x10000) // (copy of char)
	    		p2x2_[LL_COPY] = packChar( *( (char**)p2x2_[LL_COPY] ) );
				
		if (p2x2_[LR_COPY] < 0) // (new char)
			p2x2_[LR_COPY] = packChar( (char*)p2x2_[LR_CHAR]); // pack it
		else
    		if (p2x2_[LR_COPY] >= 0x10000) // (copy of char)
	    		p2x2_[LR_COPY] = packChar( *( (char**)p2x2_[LR_COPY] ) );
        }
    else
        {
        p1x1_ = (long*) *pSort;
        if (p1x1_[COPY] < 0) // (new char)
		    p1x1_[COPY] = packChar( (char*)p1x1_[CHAR]);    // (pack it)
		else
    		if (p1x1_[COPY] >= 0x10000) // (copy of char)
	    		p1x1_[COPY] = packChar( *( (char**)p1x1_[COPY] ) );
        }
    }
}





long    pixelMatch  (char *pChar_, char *pChar2_)
{

/*
    Do 64 pixel compare to see if chars are the same.
    Note code should already check for either char being transparent
    before getting here (but if not then code works anyway)

    will return:    0 if no match
                    1 if match
*/


// (squeeze every cycle out of this code!)

if (* (long*) (pChar_ + 0x000) == * (long*) (pChar2_ + 0x000) &&
    * (long*) (pChar_ + 0x004) == * (long*) (pChar2_ + 0x004) &&
    * (long*) (pChar_ + 0x100) == * (long*) (pChar2_ + 0x100) &&
    * (long*) (pChar_ + 0x104) == * (long*) (pChar2_ + 0x104) &&
    * (long*) (pChar_ + 0x200) == * (long*) (pChar2_ + 0x200) &&
    * (long*) (pChar_ + 0x204) == * (long*) (pChar2_ + 0x204) &&
    * (long*) (pChar_ + 0x300) == * (long*) (pChar2_ + 0x300) &&
    * (long*) (pChar_ + 0x304) == * (long*) (pChar2_ + 0x304) &&
    * (long*) (pChar_ + 0x400) == * (long*) (pChar2_ + 0x400) &&
    * (long*) (pChar_ + 0x404) == * (long*) (pChar2_ + 0x404) &&
    * (long*) (pChar_ + 0x500) == * (long*) (pChar2_ + 0x500) &&
    * (long*) (pChar_ + 0x504) == * (long*) (pChar2_ + 0x504) &&
    * (long*) (pChar_ + 0x600) == * (long*) (pChar2_ + 0x600) &&
    * (long*) (pChar_ + 0x604) == * (long*) (pChar2_ + 0x604) &&
    * (long*) (pChar_ + 0x700) == * (long*) (pChar2_ + 0x700) &&
    * (long*) (pChar_ + 0x704) == * (long*) (pChar2_ + 0x704) )
    
    return(1);
else
    return(0);
}





void    restore (void)
{

long    i1_, i2_;
long    *p1x1_, *p2x2_;


for (p2x2_ = p2x2Beg ; p2x2_ < p2x2End ; p2x2_ += _2X2_SIZE)
    {
    // swap upper left x and y
    i1_ = p2x2_[UL_X];
    i2_ = p2x2_[UL_Y];
    p2x2_[UL_X] = i2_;
    p2x2_[UL_Y] = i1_;

    // If entire 2x2 is copy of another
    // Then: Only need to swap UL_CHAR flip y and x bits,
    //         and the remaining stuff done below is ignored

    // restore char & copy data for each char inside 2x2
    restoreSub(p2x2_ + UL_CHAR);
    restoreSub(p2x2_ + LL_CHAR);
    restoreSub(p2x2_ + UR_CHAR);
    restoreSub(p2x2_ + LR_CHAR);

    // swap lower left char with upper right
    i1_ = p2x2_[LL_CHAR];
    i2_ = p2x2_[UR_CHAR];
    p2x2_[LL_CHAR] = i2_;
    p2x2_[UR_CHAR] = i1_;

    i1_ = p2x2_[LL_COPY];
    i2_ = p2x2_[UR_COPY];
    p2x2_[LL_COPY] = i2_;
    p2x2_[UR_COPY] = i1_;
    }

for (p1x1_ = p1x1Beg ; p1x1_ < p1x1End ; p1x1_ += _1X1_SIZE)
    {
    // swap upper left x and y
    i1_ = p1x1_[UL_X];
    i2_ = p1x1_[UL_Y];
    p1x1_[UL_X] = i2_;
    p1x1_[UL_Y] = i1_;

    // restore char & copy data for each char inside 2x2
    restoreSub(p1x1_ + CHAR);
    restoreSub(p1x1_ + CHAR_SAVE);
    }
}





void    restoreSub (long *p_)
{

/*
    p_ is ptr to p2x2_[xx_CHAR], where xx = UL, LL, UR, or LR
*/

long    i_, j_, fy_, fx_, y_, x_;


// get char word, seperate various components
i_ = *p_;
fy_ = (i_ & 0x20000) >> 17;
fx_ = (i_ & 0x10000) >> 16;
y_ = (i_ & 0xff00) >> 8;
x_ = i_ & 0xff;

// put back together
*p_ = (long)pChar + (fx_ << 17) + (fy_ << 16) + (x_ << 8) + y_;

// get copy word
i_ = *(p_ + UL_COPY-UL_CHAR);
if (i_ >= (long)p2x2Beg) // if char is copy of another char in a 2x2
    {
    // swap references of LL and UR within 2x2
    j_ = ( (i_ - (long)p2x2Beg) / 4 ) % _2X2_SIZE;
    if (j_ == LL_CHAR)
        *(p_ + UL_COPY-UL_CHAR) = i_ + (UR_CHAR-LL_CHAR) * 4;
    if (j_ == UR_CHAR)
        *(p_ + UL_COPY-UL_CHAR) = i_ + (LL_CHAR-UR_CHAR) * 4;
    }
}





long    reverseWord       (long input_)
{

/*
    reverse endian order (hi to lo becomes lo to hi)
*/


return( ( (input_ & 0x000000ff) << 8) +
        ( (input_ & 0x0000ff00) >> 8)
      );

}





void    setCopyArray    (char *pCharBeg_, long *pCopyBeg_,
                     long leftX_, long rightX_, long topY_, long bottomY_)
{

/*
    scan image (base of 64K array at pCharBeg_, base of 128K array
     at pCopyBeg_, and with data within supplied bounds) and
     find if chars are transparent or packed copies.
*/

static long    x_, y_;
static char    *pChar_;
static long    *pCopy_;


//clear mem from pCopyBeg_ to pCopyBeg_+_3ffff with 0 (transparent)
setmem ( (void*)pCopyBeg_, (unsigned)0x40000, (int)0);

for (y_ = topY_ - 7 ; y_ <= bottomY_ ; y_++)
    {
    pChar_ = pCharBeg_ + ( (y_ << 8) + leftX_ - 7 );
    pCopy_ = pCopyBeg_ + ( (y_ << 8) + leftX_ - 7 );

    for (x_ = leftX_ - 7 ; x_ <= rightX_ ; x_++)
        {
        // quick check if char is transparent:
        if (* (long*) pChar_          || * (long*)(pChar_ + 0x004) ||
            * (long*)(pChar_ + 0x100) || * (long*)(pChar_ + 0x104) ||
            * (long*)(pChar_ + 0x200) || * (long*)(pChar_ + 0x204) ||
            * (long*)(pChar_ + 0x300) || * (long*)(pChar_ + 0x304) ||
            * (long*)(pChar_ + 0x400) || * (long*)(pChar_ + 0x404) ||
            * (long*)(pChar_ + 0x500) || * (long*)(pChar_ + 0x504) ||
            * (long*)(pChar_ + 0x600) || * (long*)(pChar_ + 0x604) ||
            * (long*)(pChar_ + 0x700) || * (long*)(pChar_ + 0x704) )
            
            {
            *pCopy_ = findPacked(pChar_); // (return -1 (not found)
                                    //  or 0:64K-1 as packed char# found)
            }
        pChar_++;
        pCopy_++;
        }
    }
return;
}





long    setImageBounds  (void)
{

/*
    this will scan the image and find the smallest rectangle that 
    surrounds it

    return:  0 if image is totally blank
             1 if image has data
*/

static char    *p_, *p1_;
static long    *pL_;


for (topY = 0 ; topY <= 255 ; topY++)
    {
    pL_ = (long*) ( pChar + (topY << 8) );
    p1_ = (char*) ( (long)pL_ + 256);
    while (pL_ < (long*)p1_)
        if (*pL_++ != 0)
            goto gotTop;
    }

// image is totally blank!
return(0);

gotTop:

for (bottomY = 255 ; 1 ; bottomY--)
    {
    pL_ = (long*) ( pChar + (bottomY << 8) );
    p1_ = (char*) ( (long)pL_ + 256);
    while (pL_ < (long*)p1_)
        if (*pL_++ != 0)
            goto gotBottom;
    }

gotBottom:

for (leftX = 0 ; 1 ; leftX++)
    {
    p_ = pChar + leftX;
    while (p_ < pXChar)
        {
        if (*p_ != 0)
            goto gotLeft;
        p_ += 0x100;
        }
    }

gotLeft:

for (rightX = 255 ; 1 ; rightX--)
    {
    p_ = pChar + rightX;
    while (p_ < pXChar)
        {
        if (*p_ != 0)
            goto gotRight;
        p_ += 0x100;
        }
    }

gotRight:

// (ensure >= 15 pixel blank border around edges of 256x256 screen)

if (topY < 15 | bottomY > 255-15 | leftX < 15 | rightX > 255-15)
    {
    printf ("Error: image must have >= 15 blank pixels around edge of 256x256 image\n");
    
    exit(1);
    }

//(image ok)
return(1);
}





void    showCut     (void)
{

static long    i_, color_;
static long    *p1x1_, *p2x2_, *p1x1__, *p1x1New_;


// move mem from p1x1BestEverBeg to p1x1Beg
i_ = (long)p1x1BestEverEnd - (long)p1x1BestEverBeg;
if (i_ > 0)
    movmem (p1x1BestEverBeg, p1x1Beg, (unsigned) i_);
p1x1End = (long*) ( (long)p1x1Beg + i_ );

// move mem from p2x2BestEverBeg to p2x2Beg
i_ = (long)p2x2BestEverEnd - (long)p2x2BestEverBeg;
if (i_ > 0)
    movmem (p2x2BestEverBeg, p2x2Beg, (unsigned) i_);
p2x2End = (long*) ( (long)p2x2Beg + i_ );

// if sliding columns, then restore stuff so data is correct
if (bestEverByColumns)
    restore();

// if 1x1 is copy of reassigned 1x1, then update 1x1 CHAR and COPY
for (p1x1_ = p1x1Beg ; p1x1_ < p1x1End ; p1x1_ += _1X1_SIZE)
	chkCopyOfReassign(p1x1_);





// patch to check if 1x1 slid out of row and became exact copy (in
//   same upper left xy) as another 1x1:

// get 1x1 adrs's after duplicates are removed:

p1x1New_ = p1x1Beg;
for (p1x1_ = p1x1Beg ; p1x1_ < p1x1End ; p1x1_ += _1X1_SIZE)
    {
    p1x1_[DUPLICATE] = (long)p1x1New_;
    for (p1x1__ = p1x1Beg ; p1x1__ < p1x1_ ; p1x1__ += _1X1_SIZE)
        {
        if (p1x1__[UL_X] == p1x1_[UL_X]  &&  p1x1__[UL_Y] == p1x1_[UL_Y])
            {
            p1x1_[DUPLICATE] = -1;
            goto dup;
            }
        }
    p1x1New_ += _1X1_SIZE;            
    
    dup:
    ;
    }

// remove duplicate 1x1's:

for (p1x1_ = p1x1Beg ; p1x1_ < p1x1End ; p1x1_ += _1X1_SIZE)
    {
    if (p1x1_[DUPLICATE] != -1)
        {
        // check if 1x1 is copy of another 1x1
        p1x1__ = (long*) p1x1_[COPY];
        if ( (long)p1x1__ >= 0x10000 && (long)p1x1__ < (long)p2x2Beg) // note 1x1's defined BEFORE 2x2's !!!!
            p1x1_[COPY] = (long)p1x1__[DUPLICATE - CHAR] + CHAR * 4;
        
        // check if 1x1 is copy of another 1x1
        p1x1__ = (long*) p1x1_[COPY_SAVE];
        if ( (long)p1x1__ >= 0x10000 && (long)p1x1__ < (long)p2x2Beg) // note 1x1's defined BEFORE 2x2's !!!!
            p1x1_[COPY_SAVE] = (long)p1x1__[DUPLICATE - CHAR] + CHAR * 4;
        
        if ((long)p1x1_ != p1x1_[DUPLICATE])
            movmem (p1x1_, (long*)p1x1_[DUPLICATE], _1X1_SIZE * 4);
        }
    }

p1x1End = p1x1New_;





// draw obj boxes on screen:
for (p2x2_ = p2x2Beg ; p2x2_ < p2x2End ; p2x2_ += _2X2_SIZE)
    {
    if (p2x2_[_2X2_COPY] == -1)
        {
        // if any of 4 chars is a non-transp packed char, or copy of
        //   another 2x2 char, then use mid level value
        if (    p2x2_[UL_COPY] > 0 ||
                p2x2_[LL_COPY] > 0 ||
                p2x2_[UR_COPY] > 0 ||
                p2x2_[LR_COPY] > 0    )
            color_ = 16+4;
        else
            color_ = 16;
        }
    else
        color_ = 16+8;
        
    MarkIFF2x2( p2x2_[UL_X], p2x2_[UL_Y], color_ );
    }

for (p1x1_ = p1x1Beg ; p1x1_ < p1x1End ; p1x1_ += _1X1_SIZE)
    {
    i_ = p1x1_[COPY];
    if (i_ == -1)
        color_ = 16;
    else
        {
        if (i_ < 0x10000)
            color_ = 16+4;
        else
            color_ = 16+8;
        }

    MarkIFF1x1( p1x1_[UL_X], p1x1_[UL_Y], color_ );
    }

if (!allowShowCut)
   return;

// print stuff for cut:

printf("\n");
printf("        (UL X,Y)    adrs       copy       flip  x    y\n");
printf("        --------------------------------------------------\n");

for (p2x2_ = p2x2Beg ; p2x2_ < p2x2End ; p2x2_ += _2X2_SIZE)
    {
    printf("        ($%2x, ",      p2x2_[UL_X]);
    
    printf("$%2x)  ",      p2x2_[UL_Y]);

    printf("$%8x  ",       (long)p2x2_);
    
    if (p2x2_[_2X2_COPY] == -1)
        {
        printf("(new)      ");
        color_ = 16;
        }
    else
        {
        printf("$%8x  ",   p2x2_[_2X2_COPY]);
        color_ = 16+8;
        }
        
    i_ = p2x2_[UL_CHAR] & 0x30000;
    if (i_ == 0)
        printf("(--)");
    else
        if (i_ == 0x10000)
            {
            printf("( X)");
            color_ += 1;
            }
        else
            if (i_ == 0x20000)
                {
                printf("( Y)");
                color_ += 2;
                }
            else
                {
                printf("(XY)");
                color_ += 3;
                }
    printf("\n");

    if (p2x2_[_2X2_COPY] == -1)
        {
        // print data for each char:

        showCutSub("uper left", p2x2_ + UL_CHAR);
        showCutSub("lowr left", p2x2_ + LL_CHAR);
        showCutSub("uper rght", p2x2_ + UR_CHAR);
        showCutSub("lowr rght", p2x2_ + LR_CHAR);
        }
    printf("\n");
    }

printf("\n");
printf("        (UL X,Y)    adrs       copy       flip\n");
printf("        --------------------------------------\n");

for (p1x1_ = p1x1Beg ; p1x1_ < p1x1End ; p1x1_ += _1X1_SIZE)
    {
    printf("        ($%2x, ",      p1x1_[UL_X]);
    
    printf("$%2x)  ",      p1x1_[UL_Y]);

    printf("$%8x  ",       (long)(p1x1_ + CHAR) );
    
    i_ = p1x1_[COPY];
    if (i_ == -1)
        {
        printf("(new)      ");
        color_ = 16;
        }
    else
        {
        if (i_ < 0x10000)
            {
            printf("pak $%4x  ", i_);
            color_ = 16+4;
            }
        else
            {
            printf("$%8x  ", i_);
            color_ = 16+8;
            }
        }
    i_ = p1x1_[CHAR] & 0x30000;
    if (i_ == 0)
        printf("(--)");
    else
        if (i_ == 0x10000)
            {
            printf("( X)");
            color_ += 1;
            }
        else
            if (i_ == 0x20000)
                {
                printf("( Y)");
                color_ += 2;
                }
            else
                {
                printf("(XY)");
                color_ += 3;
                }
    printf("\n");
    
    }
}




void    showCutSub  (char *s_, long *p_)
{

long    i_;


printf("         %s  ", s_);

printf("$%8x  ", (long)p_);

i_ = *(p_ + 1);
if (i_ == -1)
    printf("(new)      ");
else
    if (i_ == 0)
        printf("(trnspnt)  ");
    else
        if (i_ < 0x10000)
            printf("pak $%4x  ", i_);
        else
            printf("$%8x  ", i_);
        
printf(" \"    ");

i_ = *p_;
if (i_ & 0x10000)
    i_ = ( i_ ^ 0x100ff ) - 0x007;
if (i_ & 0x20000)
    i_ = ( i_ ^ 0x2ff00 ) - 0x700;

printf("$%2x  ", i_ & 0xff);

printf("$%2x  ", ( i_ & 0xff00 ) >> 8);

printf("\n");
}





void    undoCut1x1  (void)
{

/*
    this will undo the last cut of a 1x1 obj and char and update weights.
    Don't call to undo mirrored 1x1
*/


p1x1End -= _1X1_SIZE; // (p1x1End now at 1x1 obj to be uncut)

weight -= weightObj + weightFlicker;
if (pULchar <= pULcloseMir)
    weight -= (weightObj + weightFlicker);
// (vram and rom weight done below)

if ( p1x1End[COPY] < 0) // (new char)
    {

    weight -= weightVramChar + weightRomChar;
    // (no effect for mirror)

    return;
    }

if ( p1x1End[COPY] < 0x10000) // (packed char copy)
    {

    weight -= weightVramChar;
    // (no effect for mirror)
    // (no rom weight was added)

    return;
    }

// (char was copy of 1x1 or 2x2 image char)

// check if 1x1 is copy of 2x2 and can't share vram space
if ( p1x1End[COPY] >= (long)p2x2Beg && !allow1x1ToCopy2x2 )
    weight -= weightVramChar;

// (no rom weight was added)

}





void    undoCut1x1Mir   (void)
{

/*
    this will undo the last cut of a 1x1 mirror obj and char and update weights.
*/


p1x1End -= _1X1_SIZE; // (p1x1End now at 1x1 obj to be uncut)

weight -= weightObj + weightFlicker;

// check if 1x1 is copy of 2x2 and can't share vram space
if ( p1x1End[COPY] >= (long)p2x2Beg && !allow1x1ToCopy2x2 )
    weight -= weightVramChar;

}





void    undoCut2x2  (void)
{

/*
    this will undo the cutting of the last 2x2
    (do not call to undo mirrored 2x2)

    weight -= (weightObj + weightFlicker*4)
    if (pULchar <= pULcloseMir)
    then
        weight -= (weightObj + weightFlicker*4)
    (vram and rom weight done below)
*/

static long    *p1x1_;


p2x2End -= _2X2_SIZE; // (p2x2End now points to 2x2 being freed)

weight -= weightObj + weightFlicker*4;
if (pULchar <= pULcloseMir)
    weight -= weightObj + weightFlicker*4;
    
// check to see if this 2x2 was a copy of another 2x2 in the
//  image being cut:

if ( p2x2End[_2X2_COPY] >= 0) // (if obj copied)
    // (no vram or rom weight was added when cut)
    return;
        
// (no obj copy, new obj and vram used)

weight -= weightVramChar*4;
// (no effect for mirror)
// (rom weight done below)

// check for reassigned 1x1's and rom weights:

p1x1_ = (long*) p2x2End[UL_REASSIGN];
if ( (long)p1x1_ >= 0 )
    undoReassign(p1x1_); // (p1x1_ scope is passed) (code below)
    // note no rom weight was done, no effect for mirror
else
    if ( p2x2End[UL_COPY] < 0) // (if new char)
        weight -= weightRomChar;
        // (no effect for mirror)

p1x1_ = (long*) p2x2End[LL_REASSIGN];
if ( (long)p1x1_ >= 0 )
    undoReassign(p1x1_); // (p1x1_ scope is passed) (code below)
    // note no rom weight was done, no effect for mirror
else
    if ( p2x2End[LL_COPY] < 0) // (if new char)
        weight -= weightRomChar;
        // (no effect for mirror)

p1x1_ = (long*) p2x2End[UR_REASSIGN];
if ( (long)p1x1_ >= 0 )
    undoReassign(p1x1_); // (p1x1_ scope is passed) (code below)
    // note no rom weight was done, no effect for mirror
else
    if ( p2x2End[UR_COPY] < 0) // (if new char)
        weight -= weightRomChar;
        // (no effect for mirror)

p1x1_ = (long*) p2x2End[LR_REASSIGN];
if ( (long)p1x1_ >= 0 )
    undoReassign(p1x1_); // (p1x1_ scope is passed) (code below)
    // note no rom weight was done, no effect for mirror
else
    if ( p2x2End[LR_COPY] < 0) // (if new char)
        weight -= weightRomChar;
        // (no effect for mirror)
}





void    undoReassign    (long *p1x1_)
{

/*
    undo reassignment of 1x1 char as copy of char in 2x2:
*/


if (allow1x1ToCopy2x2)
    weight += weightVramChar; // (cut had saved previous 1x1 vram char)

// (no effect for mirror)
// (rom weight not added)
    
p1x1_[CHAR] = p1x1_[CHAR_SAVE];
p1x1_[COPY] = p1x1_[COPY_SAVE];

return;
}





void    useBestCut  (void)
{

//	declare best version of last cut to be permanent

static long	i_;


// move mem from p1x1BestBeg to p1x1Beg
i_ = (long)p1x1BestEnd - (long)p1x1BestBeg;
if (i_ > 0)
    movmem (p1x1BestBeg, p1x1Beg, (unsigned) i_);
p1x1End = (long*) ( (long)p1x1Beg + i_ );

// move mem from p2x2BestBeg to p2x2Beg
i_ = (long)p2x2BestEnd - (long)p2x2BestBeg;
if (i_ > 0)
    movmem (p2x2BestBeg, p2x2Beg, (unsigned) i_);
p2x2End = (long*) ( (long)p2x2Beg + i_ );

weight = bestWeight;
}



void    _STI_GetChe  (void)
{
    ConFH = Input ();
    SetMode( ConFH, 1L );
}


long    GetChe  (void)
{
    if ( WaitForChar ( ConFH, 0L ) == -1L ) {
        return Read( ConFH, ConData, 1L );
    }
    return 0L;
}


void    _STD_GetChe  (void)
{
    if (ConFH != 0) {
        SetMode( ConFH, 0L );
    }
}



void    move (char *dest_, char *src_, long numBytes_)
{
static long i_;

for (i_ = 0 ; i_ < numBytes_; i_++)
    *dest_++ = *src_++;
    
}



void	_STD_FreeMem()
{
	if (pPackBeg != 0) {
		FreeMem(pPackBeg, PACK_MEM_SIZE);
	}
}





void    shrinkImageTbl  (void)
{

static unsigned short i_, ii_;

// shrink MiscData Indexes

for (i_ = 0, ii_ = 0 ; i_ < imageTblOfstEnd ; i_ += 5, ii_ += 4)
    {
    pImageTblBeg [ii_ + 0] = pImageTblBeg [i_ + 0];
    pImageTblBeg [ii_ + 1] = pImageTblBeg [i_ + 1];
    pImageTblBeg [ii_ + 2] = pImageTblBeg [i_ + 2];
    pImageTblBeg [ii_ + 3] = pImageTblBeg [i_ + 3];
    }

imageTblOfstEnd = ((imageTblOfstEnd/5)*4);

}





void    expandImageTbl  (void)
{

static unsigned short i_, ii_;

// expand MiscData Indexes

for (i_ = 0, ii_ = ((imageTblOfstEnd/4)*5)-5 ; i_ < imageTblOfstEnd ; i_ += 4, ii_ -= 5)
    {
    pImageTblBeg [ii_ + 0] = pImageTblBeg [i_ + 0];
    pImageTblBeg [ii_ + 1] = pImageTblBeg [i_ + 1];
    pImageTblBeg [ii_ + 2] = pImageTblBeg [i_ + 2];
    pImageTblBeg [ii_ + 3] = pImageTblBeg [i_ + 3];
    pImageTblBeg [ii_ + 4] = 0;
    }

imageTblOfstEnd = ((imageTblOfstEnd/4)*5);

}
