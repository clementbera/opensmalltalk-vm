#include "sq.h"
#include "sqMacFileLogic.h"
#include "DropPlugin.h"
#include <AppleEvents.h>
#include <Dialogs.h>
#include <Deskbus.h>
#include <Devices.h>
#include <Files.h>
#include <Fonts.h>
#include <Gestalt.h>
#include <LowMem.h>
#include <Memory.h>
#include <Menus.h>
#include <OSUtils.h>
#include <Power.h>
#include <QuickDraw.h>
#include <Scrap.h>
#include <Strings.h>
#include <Timer.h>
#include <ToolUtils.h>
#include <Windows.h>
#if !defined( __MPW__ ) &&  !defined ( __APPLE__ ) && !defined ( __MACH__ )
#include <profiler.h>
#endif
#include <sound.h>
#include <Math64.h>
#if !defined( __MPW__ ) &&  !defined ( __APPLE__ ) && !defined ( __MACH__ )
#include <cstddef>
#endif
#include <processes.h>
#include <OpenTransport.h>
#include <Threads.h>
#include <DriverServices.h>
#include <stddef.h>



/*** Compilation Options:
*
*	define PLUGIN		to compile code for Netscape or IE Plug-in
*	define MAKE_PROFILE	to compile code for profiling
*
***/

//#define PLUGIN
//#define MAKE_PROFILE
//#define IHAVENOHEAD
#define MINIMALVM 0

//Aug 7th 2000,JMM Added logic for interrupt driven dispatching
//Sept 1st 2000, JMM fix problem with modifier information being passed back incorrectly.
//Sept 1st 2000, JMM use floating point for time versus 64bit math (faster!)
//Sept 1st 2000, JMM watch mouse movement foreground only, ignore when squeak in background.
//Sept 18th 2000, JMM fix to cmpSize 
//Sept 19th 2000, JMM Sept 1st fix to keyboard modifier info broke cmd shift
//Sept 27 2000, JMM fix to documentPath
//Nov 13 2000, JMM logic to read/write image from VM. 
//Nov 22 2000, JMM Bob Arning found a bug with the duplicate mouse event logic (we were altering the event then recording the altered value)
//Nov 30 2000, JMM Use Open Transport clock versus upTime, solves some issues for jitter and it's faster
//Dec 5th 2000, JMM poll 60 times a second... do event polling via checkForInterrupts and drive semaphore
//Dec 6th 2000, JMM added logic to interface with power manger (1997 was there but dropped..., back again for ibooks)
//Jan 14th 2001, KG Did some carbon porting.
//Feb 2nd 2001, JMM V3.0 added zoom window support, full path support
//Feb 2nd 2001, JMM V3.04 do an open window dialog at startup time if no image file
//Feb 14th 2001, JMM V3.06 don't cache image read/writes
//Feb 17th 2001, JMM V3.07 fix OS bug in 7.5.5 on activate event
//Feb 22nd 2001, JMM v3.08 no caps lock
//Mar 9th  2001, JMM v3.10 broken full screen update redaw on cmd-tab via ignoring activate events 3.0.7.
//Mar 15th 2001, JMM v3.10 added minimal vm logic, fix mouse down reporting.
//Mar 23rd 2001, JMM v3.10 start interpreter suspended for plugin to fix race at startup with netscape 4.x
//Apr 4th 2001, JMM  V3.11 fix for carbon get keyboard data eventrecord is broken for nullevents. 
//                         Rework event duplication logic, New menubar show logic, fix position bug on window move restore after full screen 
//Apr 30th 2001, JMM V3.14 pass interpreter version back for get attribute 1004, delay if idle, set mouse down flag only on mouse down in content, and restrict window movement/sizing if in full screen mode
//May 24th 2001, JMM V3.17 add logic to sleep/wait in carbon on excessive idle time, plus change data return on attribute gets
//June 18th 2001, JMM V3.18 fix for cast of version info for CW 6.1
//June 18th 2001, JMM V3.19 fix for saveAsEmbeddedImage. Broken in 3.0 Also added fix for powerpc only cfrg, and rework of security interface for VMMaker via Tim
//Oct 1,2001, JMM V3.1.2  open document support and fix scrap issues. 

#if TARGET_API_MAC_CARBON
    #define EnableMenuItemCarbon(m1,v1)  EnableMenuItem(m1,v1);
    #define DisableMenuItemCarbon(m1,v1)  DisableMenuItem(m1,v1);
#else
    #ifndef NewAEEventHandlerUPP
    	#define NewAEEventHandlerUPP NewAEEventHandlerProc 
    #endif
    #define EnableMenuItemCarbon(m1,v1)  EnableItem(m1,v1);
    #define DisableMenuItemCarbon(m1,v1)  DisableItem(m1,v1);
        inline Rect *GetPortBounds(CGrafPtr w,Rect *r) { *r = w->portRect; return &w->portRect;}  
        inline Rect *GetRegionBounds(RgnHandle region, Rect * bounds) { *bounds = (*region)->rgnBBox; return &(*region)->rgnBBox;}
        inline BitMap *GetQDGlobalsScreenBits(BitMap *bm){*bm = qd.screenBits; return &qd.screenBits; }
        inline BitMap * GetPortBitMapForCopyBits (CGrafPtr w) { return &((GrafPtr)w)->portBits;}
        inline pascal long InvalWindowRect(WindowRef  window,  const Rect * bounds) {InvalRect (bounds);}
#endif

#if MINIMALVM
pascal short YieldToAnyThread(void) {
}
#endif

/*** Enumerations ***/
enum { appleID = 1, fileID, editID };
enum { quitItem = 1 };

/* The following prototype is missing from the CW11 header files: */
pascal void ExitToShell(void);

/*** Variables -- Imported from Browser Plugin Module ***/
#ifdef PLUGIN
extern int pluginArgCount;
extern char *pluginArgName[100];
extern char *pluginArgValue[100];
#endif

/*** Variables -- Imported from Virtual Machine ***/
extern int fullScreenFlag;
extern int interruptCheckCounter;
extern int interruptKeycode;
extern int interruptPending;  /* set to true by recordKeystroke if interrupt key is pressed */
extern unsigned char *memory;
extern int savedWindowSize;   /* set from header when image file is loaded */

/*** Variables -- image and path names ***/
#define IMAGE_NAME_SIZE 300
char imageName[IMAGE_NAME_SIZE + 1];  /* full path to image file */

#define SHORTIMAGE_NAME_SIZE 100
char shortImageName[SHORTIMAGE_NAME_SIZE + 1];  /* just the image file name */

#define DOCUMENT_NAME_SIZE 300
char documentName[DOCUMENT_NAME_SIZE + 1];  /* full path to document file */

#define VMPATH_SIZE 300
char vmPath[VMPATH_SIZE + 1];  /* full path to interpreter's directory */

/*** Variables -- Mac Related ***/
MenuHandle		appleMenu = nil;
MenuHandle		editMenu = nil;
int				menuBarHeight = 20;
RgnHandle		menuBarRegion = nil;  /* if non-nil, then menu bar has been hidden */
MenuHandle		fileMenu = nil;
CTabHandle		stColorTable = nil;
PixMapHandle	stPixMap = nil;
WindowPtr		stWindow = nil;
OTTimeStamp     timeStart;
Boolean         gTapPowerManager=false;
Boolean         gDisablePowerManager=false;
const long      gDisableIdleTickCount=60*10;
long            gDisableIdleTickLimit=0;
Boolean         gThreadManager=false;
ThreadID        gSqueakThread = kNoThreadID;
ThreadEntryUPP  gSqueakThreadUPP;
#ifdef __MPW__
QDGlobals 		qd;
#endif


/*** Variables -- Event Recording ***/
#if MINIMALVM
#define MAX_EVENT_BUFFER 128
#else
#define MAX_EVENT_BUFFER 1024
#endif

int inputSemaphoreIndex = 0;/* if non-zero the event semaphore index */

sqInputEvent eventBuffer[MAX_EVENT_BUFFER];
int eventBufferGet = 0;
int eventBufferPut = 0;

/* declaration of the event message hook */
typedef int (*eventMessageHook)(EventRecord* event);
eventMessageHook messageHook = NULL;
eventMessageHook postMessageHook = NULL;


#define KEYBUF_SIZE 64
int keyBuf[KEYBUF_SIZE];	/* circular buffer */
int keyBufGet = 0;			/* index of next item of keyBuf to read */
int keyBufPut = 0;			/* index of next item of keyBuf to write */
int keyBufOverflows = 0;	/* number of characters dropped */

int buttonState = 0;		/* mouse button and modifier state when mouse
							   button went down or 0 if not pressed */
int cachedButtonState = 0;	/* buffered mouse button and modifier state for
							   last mouse click even if button has since gone up;
							   this cache is kept until the next time ioGetButtonState()
							   is called to avoid missing short clicks */
int gButtonIsDown = 0;

Point savedMousePosition;	/* mouse position when window is inactive */
int windowActive = true;	/* true if the Squeak window is the active window */
int	gDelayTime=0;

/* This table maps the 5 Macintosh modifier key bits to 4 Squeak modifier
   bits. (The Mac shift and caps lock keys are both mapped to the single
   Squeak shift bit).  This was true for squeak upto 3.0.7. Then in 3.0.8 we 
   decided to not map the cap lock key to shift
   
		Mac bits: <control><option><caps lock><shift><command>
		ST bits:  <command><option><control><shift>
*/
char modifierMap[32] = {
//	0,  8, 1,  9, 1,  9, 1,  9, 4, 12, 5, 13, 5, 13, 5, 13, caps lock
//	2, 10, 3, 11, 3, 11, 3, 11, 6, 14, 7, 15, 7, 15, 7, 15
    0,  8, 1,  9, 0,  8, 1,  9, 4, 12, 5, 13, 4, 12, 5, 13, //no caps lock
    2, 10, 3, 11, 2, 10, 3, 11, 6, 14, 7, 15, 6, 14, 7, 15
	
 };

// requestFlags bit values in VideoRequestRec (example use: 1<<kAbsoluteRequestBit)
enum {
	kBitDepthPriorityBit		= 0,	// Bit depth setting has priority over resolution
	kAbsoluteRequestBit			= 1,	// Available setting must match request
	kShallowDepthBit			= 2,	// Match bit depth less than or equal to request
	kMaximizeResBit				= 3,	// Match screen resolution greater than or equal to request
	kAllValidModesBit			= 4		// Match display with valid timing modes (may include modes which are not marked as safe)
};

// availFlags bit values in VideoRequestRec (example use: 1<<kModeValidNotSafeBit)
enum {
	kModeValidNotSafeBit		= 0		//  Available timing mode is valid but not safe (requires user confirmation of switch)
};

// video request structure
struct VideoRequestRec	{
	GDHandle		screenDevice;		// <in/out>	nil will force search of best device, otherwise search this device only
	short			reqBitDepth;		// <in>		requested bit depth
	short			availBitDepth;		// <out>	available bit depth
	unsigned long	reqHorizontal;		// <in>		requested horizontal resolution
	unsigned long	reqVertical;		// <in>		requested vertical resolution
	unsigned long	availHorizontal;	// <out>	available horizontal resolution
	unsigned long	availVertical;		// <out>	available vertical resolution
	unsigned long	requestFlags;		// <in>		request flags
	unsigned long	availFlags;			// <out>	available mode flags
	unsigned long	displayMode;		// <out>	mode used to set the screen resolution
	unsigned long	depthMode;			// <out>	mode used to set the depth
	VDSwitchInfoRec	switchInfo;			// <out>	DM2.0 uses this rather than displayMode/depthMode combo
};
typedef struct VideoRequestRec VideoRequestRec;
typedef struct VideoRequestRec *VideoRequestRecPtr;

struct DepthInfo {
	VDSwitchInfoRec			depthSwitchInfo;			// This is the switch mode to choose this timing/depth
	VPBlock					depthVPBlock;				// VPBlock (including size, depth and format)
};
typedef struct DepthInfo DepthInfo;

struct ListIteratorDataRec {
	VDTimingInfoRec			displayModeTimingInfo;		// Contains timing flags and such
	unsigned long			depthBlockCount;			// How many depths available for a particular timing
	DepthInfo				*depthBlocks;				// Array of DepthInfo
};
typedef struct ListIteratorDataRec ListIteratorDataRec;
void GetRequestTheDM2Way (		VideoRequestRecPtr requestRecPtr,
								GDHandle walkDevice,
								DMDisplayModeListIteratorUPP myModeIteratorProc,
								DMListIndexType theDisplayModeCount,
								DMListType *theDisplayModeList);

pascal void ModeListIterator (	void *userData,
								DMListIndexType itemIndex,
								DMDisplayModeListEntryPtr displaymodeInfo);

Boolean FindBestMatch (			VideoRequestRecPtr requestRecPtr,
								short bitDepth,
								unsigned long horizontal,
								unsigned long vertical);

/*** Functions ***/
void AdjustMenus(void);
void FreeClipboard(void);
void FreePixmap(void);
char * GetAttributeString(int id);
int  HandleEvents(void);
void HandleMenu(int mSelect);
void HandleMouseDown(EventRecord *theEvent);
void InitMacintosh(void);
void InstallAppleEventHandlers(void);
int  IsImageName(char *name);
CFragConnectionID LoadLibViaPath(char *libName, char *pluginDirPath);
void MenuBarHide(void);
void MenuBarRestore(void);
void SetColorEntry(int index, int red, int green, int blue);
void SetUpClipboard(void);
void SetUpMenus(void);
void SetUpPixmap(void);
void SetUpWindow(void);
void SetWindowTitle(char *title);
void SqueakTerminate();
void ExitCleanup();
int calculateStartLocationForImage();
Boolean RunningOnCarbonX(void);
void DoZoomWindow (EventRecord* theEvent, WindowPtr theWindow, short zoomDir, short hMax, short vMax);
GDHandle getDominateDevice(WindowPtr theWindow,Rect *windRect);
void getDominateGDeviceRect(GDHandle dominantGDevice,Rect *dGDRect,Boolean forgetMenuBar);
int getFirstImageNameIfPossible(AEDesc	*fileList);
void processDocumentsButExcludeOne(AEDesc	*fileList,long whichToExclude);

/* event capture */
sqInputEvent *nextEventPut(void);

int recordKeystroke(EventRecord *theEvent);
int recordModifierButtons(EventRecord *theEvent);
int recordMouseDown(EventRecord *theEvent);
int recordMouseEvent(EventRecord *theEvent, int theButtonState);
int recordDragDropEvent(EventRecord *theEvent, int numberOfItems, int dragType);
int recordKeyboardEvent(EventRecord *theEvent, int keyType);
int MouseModifierState(EventRecord *theEvent);
WindowPtr getSTWindow(void);
int setMessageHook(eventMessageHook theHook);
int setPostMessageHook(eventMessageHook theHook);
int checkForModifierKeys();
void ignoreLastEvent();

void PowerMgrCheck(void);
static pascal void* squeakThread(void *threadParm);
OSErr   createNewThread();
void ADBIOCompletionPPC(Byte *dataBufPtr, Byte *opDataPtr, long command);
void SetupKeyboard(void);    
Boolean IsKeyDown(short n);    
  
 
/*** Apple Event Handlers ***/
static pascal OSErr HandleOpenAppEvent(const AEDescList *aevt,  AEDescList *reply, long refCon);
static pascal OSErr HandleOpenDocEvent(const AEDescList *aevt,  AEDescList *reply, long refCon);
static pascal OSErr HandlePrintDocEvent(const AEDescList *aevt, AEDescList *reply, long refCon);
static pascal OSErr HandleQuitAppEvent(const AEDescList *aevt,  AEDescList *reply, long refCon);

/*** Apple Event Handling ***/

void InstallAppleEventHandlers() {
	OSErr	err;
	long	result;

	shortImageName[0] = 0;
	err = Gestalt(gestaltAppleEventsAttr, &result);
	if (err == noErr) {
		AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, NewAEEventHandlerUPP(HandleOpenAppEvent),  0, false);
		AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,   NewAEEventHandlerUPP(HandleOpenDocEvent),  0, false);
		AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,  NewAEEventHandlerUPP(HandlePrintDocEvent), 0, false);
		AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(HandleQuitAppEvent),  0, false);
	}
}

pascal OSErr HandleOpenAppEvent(const AEDescList *aevt,  AEDescList *reply, long refCon) {
	/* User double-clicked application; look for "squeak.image" in same directory */
    int                 checkValueForEmbeddedImage;
    OSErr               err;
	ProcessSerialNumber processID;
	ProcessInfoRec      processInformation;
	Str32                name; 
	
/* record path to VM's home folder */
	short vRefNum;
	long dirID;

	// Get the Volume ref and Directory id of the Application's directory.
    err = GetApplicationDirectory(&vRefNum, &dirID);
    if (err != noErr) return err;

	// Convert that to a full path string.
	PathToWorkingDir(vmPath, VMPATH_SIZE, vRefNum, dirID);

	checkValueForEmbeddedImage = calculateStartLocationForImage();
	if (checkValueForEmbeddedImage == 0) {
	    /* use default image name in same directory as the VM */
	    strcpy(shortImageName, "squeak.image");
	    return noErr;
	}

    GetCurrentProcess(&processID); 
    processInformation.processInfoLength = sizeof(ProcessInfoRec);
    processInformation.processAppSpec = nil;
    processInformation.processName = (StringPtr) &name;
	err = GetProcessInformation(&processID,&processInformation);

	if (err != noErr) {
		strcpy(shortImageName, "squeak.image");
	    return noErr;
	}
	
	CopyPascalStringToC(name,shortImageName);
	StoreFullPathForLocalNameInto(shortImageName, imageName, IMAGE_NAME_SIZE, vRefNum, dirID);

    return noErr;
}

pascal OSErr HandleOpenDocEvent(const AEDescList *aevt, AEDescList *reply, long refCon) {
	/* User double-clicked an image file. Record the path to the VM's directory,
	   then set the default directory to the folder containing the image and
	   record the image name. */

	OSErr		err;
	AEDesc		fileList = {'NULL', NULL};
	long		numFiles, size, imageFileIsNumber;
	DescType	type;
	AEKeyword	keyword;
	FSSpec		fileSpec;
	WDPBRec		pb;
	FInfo		finderInformation;
	char 		tempShortName[SHORTIMAGE_NAME_SIZE + 1];	
	short       vRefNum;
	long        dirID;
	
	reply; refCon;  /* reference args to avoid compiler warnings */

	/* record path to VM's home folder */
	
    if (vmPath[0] == 0) {
        err = GetApplicationDirectory(&vRefNum, &dirID);
        if (err != noErr) 
            return err;    
	    PathToWorkingDir(vmPath, VMPATH_SIZE, vRefNum, dirID);
	}
	
	/* copy document list */
	err = AEGetKeyDesc(aevt, keyDirectObject, typeAEList, &fileList);
	if (err) 
	    return errAEEventNotHandled;;

	/* count list elements */
	err = AECountItems( &fileList, &numFiles);
	if (err) 
	    goto done;
	
	if (shortImageName[0] != 0) {
        /* Do the rest of the documents */
        processDocumentsButExcludeOne(&fileList,-1);
		goto done;
	}

    imageFileIsNumber = getFirstImageNameIfPossible(&fileList);
    
    if (imageFileIsNumber == 0) { 
        // Test is open change set 
		strcpy(shortImageName, "squeak.image");
		StoreFullPathForLocalNameInto(shortImageName, imageName, IMAGE_NAME_SIZE, vRefNum, dirID);
        fileSpec.vRefNum = vRefNum;
        fileSpec.parID = dirID;
    } else {
    	/* get image name */
    	err = AEGetNthPtr(&fileList, imageFileIsNumber, typeFSS, &keyword, &type, (Ptr) &fileSpec, sizeof(fileSpec), &size);
    	if (err) 
    	    goto done;
    	    
    	err = FSpGetFInfo(&fileSpec,&finderInformation);
    	if (err) 
    	    goto done;
    		
    	CopyPascalStringToC(fileSpec.name,shortImageName);
 		StoreFullPathForLocalNameInto(shortImageName, imageName, IMAGE_NAME_SIZE,fileSpec.vRefNum,fileSpec.parID);
   }
    
	/* make the image or document directory the working directory */
	pb.ioNamePtr = NULL;
	pb.ioVRefNum = fileSpec.vRefNum;
	pb.ioWDDirID = fileSpec.parID;
	PBHSetVolSync(&pb);

    /* Do the rest of the documents */
    processDocumentsButExcludeOne(&fileList,imageFileIsNumber);
done:
	AEDisposeDesc(&fileList);
	return err;
}

void processDocumentsButExcludeOne(AEDesc	*fileList,long whichToExclude) {
	OSErr		err;
	long		numFiles, size, i, actualFilteredNumber=0,actualFilteredIndexNumber;
	DescType	type;
	AEKeyword	keyword;
	FSSpec		fileSpec;
	FInfo		finderInformation;
    EventRecord theEvent;
    HFSFlavor   dropFile;
    Point       where;
    
	/* count list elements */
	err = AECountItems( fileList, &numFiles);
	if (err)
	    return;
	
	theEvent.what = 0;
	theEvent.message = 0;
	theEvent.when = TickCount();
	where.v = 1;
	where.h = 1;
	LocalToGlobal(&where);
	theEvent.where = where;
	theEvent.modifiers = 0;
	
	for(i=1;i<=numFiles;i++) {
	    err = AEGetNthPtr(fileList, i, typeFSS,  &keyword, &type, (Ptr) &fileSpec, sizeof(fileSpec), &size);
	    if (err) 
	        goto done;
	
	    err = FSpGetFInfo(&fileSpec,&finderInformation);
	    if (err) 
	        goto done;
		
	    if (i == whichToExclude || (finderInformation.fdCreator == 'MACS' && 
	        (finderInformation.fdType == 'fold' ||
    		finderInformation.fdType == 'disk'))) 
	        continue;
	     
	    actualFilteredNumber++;

    }
    if (actualFilteredNumber == 0) 
        goto done;
        
    sqSetNumberOfDropFiles(actualFilteredNumber);
    actualFilteredIndexNumber=1;
    
    recordDragDropEvent(&theEvent, actualFilteredNumber, DragEnter);
    for(i=1;i<=numFiles;i++) {
	    err = AEGetNthPtr(fileList, i, typeFSS,  &keyword, &type, (Ptr) &fileSpec, sizeof(fileSpec), &size);
	    if (err) 
	        goto done;
	
	    err = FSpGetFInfo(&fileSpec,&finderInformation);
	    if (err) 
	        goto done;
		
	    if (i == whichToExclude || (finderInformation.fdCreator == 'MACS' && 
	        (finderInformation.fdType == 'fold' ||
    		finderInformation.fdType == 'disk'))) 
	        continue;
	        
	    dropFile.fileType = finderInformation.fdType;
	    dropFile.fileCreator = finderInformation.fdCreator;
	    dropFile.fdFlags = finderInformation.fdFlags;
	    memcpy(&dropFile.fileSpec,&fileSpec,sizeof(FSSpec));
	     
        sqSetFileInformation(actualFilteredIndexNumber, &dropFile);
        actualFilteredIndexNumber++;
    }
	theEvent.where = where;
    recordDragDropEvent(&theEvent, actualFilteredNumber, DragDrop);
	theEvent.where = where;
    recordDragDropEvent(&theEvent, actualFilteredNumber, DragLeave);
   
   done: 
   return;
    
}


int getFirstImageNameIfPossible(AEDesc	*fileList) {
	OSErr		err;
	long		numFiles, size, i;
	DescType	type;
	AEKeyword	keyword;
	FSSpec		fileSpec;
	FInfo		finderInformation;

	/* count list elements */
	err = AECountItems( fileList, &numFiles);
	if (err) 
	    goto done;
	
	/* get image name */
	for(i=1;i<=numFiles;i++) {
	    err = AEGetNthPtr(fileList, i, typeFSS,
					  &keyword, &type, (Ptr) &fileSpec, sizeof(fileSpec), &size);
	    if (err) 
	        goto done;
	
	    err = FSpGetFInfo(&fileSpec,&finderInformation);
	    if (err) 
	        goto done;
		CopyPascalStringToC(fileSpec.name,shortImageName);

        if ((IsImageName(shortImageName) && finderInformation.fdType == 'TEXT')  || finderInformation.fdType == 'STim')
            return i;
    }
    done: 
        return 0;
}       


pascal OSErr HandlePrintDocEvent(const AEDescList *aevt,  AEDescList *reply, long refCon) {
	aevt; reply; refCon;  /* reference args to avoid compiler warnings */
	return errAEEventNotHandled;
}

pascal OSErr HandleQuitAppEvent(const AEDescList *aevt,  AEDescList *reply, long refCon) {
	aevt; reply; refCon;  /* reference args to avoid compiler warnings */
	return noErr;  //Note under Carbon it sends us a Quit event, but we don't process because image might not get saved?
}

/*** VM Home Directory Path ***/

int vmPathSize(void) {
	return strlen(vmPath);
}

int vmPathGetLength(int sqVMPathIndex, int length) {
	char *stVMPath = (char *) sqVMPathIndex;
	int count, i;

	count = strlen(vmPath);
	count = (length < count) ? length : count;

	/* copy the file name into the Squeak string */
	for (i = 0; i < count; i++) {
		stVMPath[i] = vmPath[i];
	}
	return count;
}

/*** Mac-related Functions ***/

void AdjustMenus(void) {
	WindowRef		wp;
	int				isDeskAccessory;

	wp = FrontWindow();
	if (wp != NULL) {
		isDeskAccessory = GetWindowKind(wp) < 0;
	} else {
		isDeskAccessory = false;
	}

	if (isDeskAccessory) {
		/* Enable items in the Edit menu */
		EnableMenuItemCarbon(editMenu, 1);
		EnableMenuItemCarbon(editMenu, 3);
		EnableMenuItemCarbon(editMenu, 4);
		EnableMenuItemCarbon(editMenu, 5);
		EnableMenuItemCarbon(editMenu, 6);
	} else {
		/* Disable items in the Edit menu */
		DisableMenuItemCarbon(editMenu, 1);
		DisableMenuItemCarbon(editMenu, 3);
		DisableMenuItemCarbon(editMenu, 4);
		DisableMenuItemCarbon(editMenu, 5);
		DisableMenuItemCarbon(editMenu, 6);
	}
}

int HandleEvents(void) {
	EventRecord		theEvent;
	int				ok;

	ok = WaitNextEvent(everyEvent, &theEvent,gDelayTime,null);
	gDelayTime = 0;
	if((messageHook) && (messageHook(&theEvent))) {
        return ok;
    }
	if (ok) {
		switch (theEvent.what) {
			case mouseDown:
				HandleMouseDown(&theEvent);
				if(postMessageHook) postMessageHook(&theEvent);
				return false;
			break;

			case mouseUp:
			    gButtonIsDown = false;
				if(inputSemaphoreIndex) {
					recordMouseEvent(&theEvent,MouseModifierState(&theEvent));
    				if(postMessageHook) postMessageHook(&theEvent);
					return false;
				}
				recordModifierButtons(&theEvent);
				if(postMessageHook) postMessageHook(&theEvent);
				return false;
			break;

			case keyDown:
			case autoKey:
                if ((theEvent.modifiers & cmdKey) != 0) {
					AdjustMenus();
					HandleMenu(MenuKey(theEvent.message & charCodeMask));
				}
				if(inputSemaphoreIndex) {
					recordKeyboardEvent(&theEvent,EventKeyDown);
					break;
				}
				recordModifierButtons(&theEvent);
				recordKeystroke(&theEvent);
			break;
			
			case keyUp:
				if(inputSemaphoreIndex) {
					recordKeyboardEvent(&theEvent,EventKeyUp);
				}
			break;

#ifndef IHAVENOHEAD
			case updateEvt:
				BeginUpdate(stWindow);
				fullDisplayUpdate();  /* this makes VM call ioShowDisplay */
				EndUpdate(stWindow);
			break;

			case activateEvt:
				if (theEvent.modifiers & activeFlag) {
					windowActive = true;
				} else {
					GetMouse(&savedMousePosition);
					windowActive = false;
				}
				fullDisplayUpdate();  /* Fix for full screen menu bar tabbing*/
			break;
#endif

			case kHighLevelEvent:
				AEProcessAppleEvent(&theEvent);
			break;
			
			case osEvt: 
				if (((theEvent.message>>24)& 0xFF) == suspendResumeMessage) {
				
					//JMM July 4th 2000
					//Fix for menu bar tabbing, thanks to Javier Diaz-Reinoso for pointing this out
					//
					if ((theEvent.message & resumeFlag) == 0) {
						GetMouse(&savedMousePosition);
						windowActive = false;
						if (fullScreenFlag)
							MenuBarRestore();
					} else {
						windowActive = true;
 						if (fullScreenFlag) {
							MenuBarHide();
            				fullDisplayUpdate();  /* Fix for full screen menu bar tabbing*/
						}
					}
				}
				break;
		}
	}
	else {
		if(inputSemaphoreIndex && windowActive ) {
    		theEvent.modifiers = checkForModifierKeys();
 			recordMouseEvent(&theEvent,MouseModifierState(&theEvent));
 		}
 	}
	if(postMessageHook) postMessageHook(&theEvent); 
	return ok;
}

void HandleMenu(int mSelect) {
	int			menuID, menuItem;
	Str255		name;
	GrafPtr		savePort;

	menuID = HiWord(mSelect);
	menuItem = LoWord(mSelect);
	switch (menuID) {
		case appleID:
			GetPort(&savePort);
			GetMenuItemText(appleMenu, menuItem, name);
#if !TARGET_API_MAC_CARBON
			OpenDeskAcc(name);
#endif 
			SetPort(savePort);
		break;

		case fileID:
			if (menuItem == quitItem) {
				ioExit();
			}
		break;

		case editID:
#if !TARGET_API_MAC_CARBON
			if (!SystemEdit(menuItem - 1)) {
				SysBeep(5);
			}
#endif
		break;
	}
}

void HandleMouseDown(EventRecord *theEvent) {
	WindowPtr	theWindow;
	static Rect		growLimits = { 20, 20, 10000, 10000 };
	Rect        dragBounds;
	int			windowCode, newSize;

	windowCode = FindWindow(theEvent->where, &theWindow);
	switch (windowCode) {
		case inSysWindow:
#if !TARGET_API_MAC_CARBON
			SystemClick(theEvent, theWindow);
#endif
		break;

		case inMenuBar:
			AdjustMenus();
			HandleMenu(MenuSelect(theEvent->where));
		break;

#ifndef IHAVENOHEAD
		case inDrag:

			if (fullScreenFlag) 	
				break;
				
			GetRegionBounds(GetGrayRgn(), &dragBounds);
			if (theWindow == stWindow) {
				DragWindow(stWindow, theEvent->where, &dragBounds);
			}
		break;

		case inGrow:
			if (theWindow == stWindow) {
				if (fullScreenFlag) 	
					break;
				newSize = GrowWindow(stWindow, theEvent->where, &growLimits);
				if (newSize != 0) {
					SizeWindow(stWindow, LoWord(newSize), HiWord(newSize), true);
				}
			}
		break;

		case inZoomIn:
		case inZoomOut:
			if (theWindow == stWindow) {
				if (fullScreenFlag) 	
					break;
					DoZoomWindow(theEvent,stWindow, windowCode,10000, 10000);
				}

		break;

		case inContent:
			gButtonIsDown = true;
			if (theWindow == stWindow) {
				if (theWindow != FrontWindow()) {
					SelectWindow(stWindow);
				}
				if(inputSemaphoreIndex) {
					recordMouseEvent(theEvent,MouseModifierState(theEvent));
					break;
				}
				recordMouseDown(theEvent);
			}
		break;

		case inGoAway:
			if ((theWindow == stWindow) &&
				(TrackGoAway(stWindow, theEvent->where))) {
					/* HideWindow(stWindow); noop for now */
			}
		break;
#endif
	}
}


#if TARGET_API_MAC_CARBON
void InitMacintosh(void) {
	FlushEvents(everyEvent, 0);
	LoadScrap();
	InitCursor();
}

void MenuBarHide(void) {
 	if (menuBarRegion != nil) return;  /* saved state, so menu bar is already hidden */
    menuBarRegion = (RgnHandle) 1;
    HideMenuBar();
}
void MenuBarRestore(void) {
	if (menuBarRegion == nil) return;  /* no saved state, so menu bar is not hidden */
    ShowMenuBar();
    menuBarRegion = nil;
}

/*** Clipboard Support (text only for now) ***/

void SetUpClipboard(void) {
}

void FreeClipboard(void) {
}

int clipboardReadIntoAt(int count, int byteArrayIndex, int startIndex) {
	long clipSize, charsToMove;
	ScrapRef scrap;
	OSStatus err;

    err = GetCurrentScrap (&scrap);
    if (err != noErr) return 0;       
	clipSize = clipboardSize();
 	charsToMove = (count < clipSize) ? count : clipSize;
    err = GetScrapFlavorData(scrap,kScrapFlavorTypeText,(long *) &charsToMove,(char *) byteArrayIndex + startIndex);
    if (err != noErr) { 
        FreeClipboard();
        return 0;       
    }
	return charsToMove;
}

int clipboardSize(void) {
	long count;
	ScrapRef scrap;
	OSStatus err;

    err = GetCurrentScrap (&scrap);
    if (err != noErr) return 0;       
    err = GetScrapFlavorSize (scrap, kScrapFlavorTypeText, &count); 
	if (err != noErr) {
		return 0;
	} else {
		return count;
	}
}

int clipboardWriteFromAt(int count, int byteArrayIndex, int startIndex) {
	ScrapRef scrap;
	OSErr err;
	err = ClearCurrentScrap();
    err = GetCurrentScrap (&scrap);
	err = PutScrapFlavor ( scrap, kScrapFlavorTypeText, kScrapFlavorMaskNone , count,  (const void *) (byteArrayIndex + startIndex));
}

#else 
void InitMacintosh(void) {
#if TARGET_CPU_68K
    long currentLimit;
    currentLimit = (long) GetApplLimit();
    SetApplLimit((char *) currentLimit-32*1024);
#endif
	MaxApplZone();
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(NULL);
	InitCursor();
	LoadScrap();
    SetupKeyboard();	
}

void MenuBarHide(void) {
  /* Remove the menu bar, saving its old state. */
  /* Many thanks to John McIntosh for this code! */
	Rect screenRect, mBarRect;

	if (menuBarRegion != nil) return;  /* saved state, so menu bar is already hidden */
	screenRect = (**GetMainDevice()).gdRect;
	menuBarHeight = GetMBarHeight();
	SetRect(&mBarRect, screenRect.left, screenRect.top, screenRect.right, screenRect.top + menuBarHeight);
	menuBarRegion = NewRgn();
	if (menuBarRegion != nil) {
		LMSetMBarHeight(0);
		RectRgn(menuBarRegion, &mBarRect);
		UnionRgn(GetGrayRgn(), menuBarRegion, GetGrayRgn());
		PaintOne(NULL,menuBarRegion);
		CalcVisBehind(stWindow,menuBarRegion);
	}
}

void MenuBarRestore(void) {
  /* Restore the menu bar from its saved state. Do nothing if it isn't hidden. */
  /* Many thanks to John McIntosh for this code! */
 
 	WindowPtr win;
 	
	if (menuBarRegion == nil) return;  /* no saved state, so menu bar is not hidden */
	DiffRgn(GetGrayRgn(), menuBarRegion, GetGrayRgn());
	LMSetMBarHeight(menuBarHeight);
	
	win = FrontWindow();
	if (win) {
		CalcVis(win);
		CalcVisBehind(win,menuBarRegion);
	}
	HiliteMenu(0);
	DisposeRgn(menuBarRegion);
	
	menuBarRegion = nil;
	DrawMenuBar();
}

/*** Clipboard Support (text only for now) ***/
Handle			clipboardBuffer = nil;

void SetUpClipboard(void) {
	/* allocate clipboard in the system heap to support really big copy/paste */
	THz oldZone;

	oldZone = GetZone();
	SetZone(SystemZone());
	clipboardBuffer = NewHandle(0);
	SetZone(oldZone);
}

void FreeClipboard(void) {
	if (clipboardBuffer != nil) {
		DisposeHandle(clipboardBuffer);
		clipboardBuffer = nil;
	}
}

int clipboardReadIntoAt(int count, int byteArrayIndex, int startIndex) {
	long clipSize, charsToMove;
	char *srcPtr, *dstPtr, *end;

	clipSize = clipboardSize();
	charsToMove = (count < clipSize) ? count : clipSize;
    //JMM locking
    HLock(clipboardBuffer); 
	srcPtr = (char *) *clipboardBuffer;
	dstPtr = (char *) byteArrayIndex + startIndex;
	end = srcPtr + charsToMove;
	while (srcPtr < end) {
		*dstPtr++ = *srcPtr++;
	}
    HUnlock(clipboardBuffer); 
	return charsToMove;
}

int clipboardSize(void) {
	long count, offset;

	count = GetScrap(clipboardBuffer, 'TEXT', &offset);
	if (count < 0) {
		return 0;
	} else {
		return count;
	}
}

int clipboardWriteFromAt(int count, int byteArrayIndex, int startIndex) {
	ZeroScrap();
	PutScrap(count, 'TEXT', (char *) (byteArrayIndex + startIndex));
}

#endif

void SetUpMenus(void) {
	long decideOnQuitMenu;
	
	InsertMenu(appleMenu = NewMenu(appleID, "\p\024"), 0);
	InsertMenu(fileMenu  = NewMenu(fileID,  "\pFile"), 0);
	InsertMenu(editMenu  = NewMenu(editID,  "\pEdit"), 0);
	DrawMenuBar();
#if TARGET_API_MAC_CARBON
    Gestalt( gestaltMenuMgrAttr, &decideOnQuitMenu);
    if (!(decideOnQuitMenu & gestaltMenuMgrAquaLayoutMask) || true)	
        AppendMenu(fileMenu, "\pQuit");
#else
	AppendResMenu(appleMenu, 'DRVR');
    AppendMenu(fileMenu, "\pQuit");
#endif
 	AppendMenu(editMenu, "\pUndo/Z;(-;Cut/X;Copy/C;Paste/V;Clear");
}

void SetColorEntry(int index, int red, int green, int blue) {
	(*stColorTable)->ctTable[index].value = index;
	(*stColorTable)->ctTable[index].rgb.red = red;
	(*stColorTable)->ctTable[index].rgb.green = green;
	(*stColorTable)->ctTable[index].rgb.blue = blue;
}

void FreePixmap(void) {
	if (stPixMap != nil) {
		DisposePixMap(stPixMap);
		stPixMap = nil;
	}

	if (stColorTable != nil) {
		DisposeHandle((void *) stColorTable);
		stColorTable = nil;
	}
}

void SetUpPixmap(void) {
	int i, r, g, b;

	stColorTable = (CTabHandle) NewHandle(sizeof(ColorTable) + (256 * sizeof(ColorSpec)));
	(*stColorTable)->ctSeed = GetCTSeed();
	(*stColorTable)->ctFlags = 0;
	(*stColorTable)->ctSize = 255;

	/* 1-bit colors (monochrome) */
	SetColorEntry(0, 65535, 65535, 65535);	/* white or transparent */
	SetColorEntry(1,     0,     0,     0);	/* black */

	/* additional colors for 2-bit color */
	SetColorEntry(2, 65535, 65535, 65535);	/* opaque white */
	SetColorEntry(3, 32768, 32768, 32768);	/* 1/2 gray */

	/* additional colors for 4-bit color */
	SetColorEntry( 4, 65535,     0,     0);	/* red */
	SetColorEntry( 5,     0, 65535,     0);	/* green */
	SetColorEntry( 6,     0,     0, 65535);	/* blue */
	SetColorEntry( 7,     0, 65535, 65535);	/* cyan */
	SetColorEntry( 8, 65535, 65535,     0);	/* yellow */
	SetColorEntry( 9, 65535,     0, 65535);	/* magenta */
	SetColorEntry(10,  8192,  8192,  8192);	/* 1/8 gray */
	SetColorEntry(11, 16384, 16384, 16384);	/* 2/8 gray */
	SetColorEntry(12, 24576, 24576, 24576);	/* 3/8 gray */
	SetColorEntry(13, 40959, 40959, 40959);	/* 5/8 gray */
	SetColorEntry(14, 49151, 49151, 49151);	/* 6/8 gray */
	SetColorEntry(15, 57343, 57343, 57343);	/* 7/8 gray */

	/* additional colors for 8-bit color */
	/* 24 more shades of gray (does not repeat 1/8th increments) */
	SetColorEntry(16,  2048,  2048,  2048);	/*  1/32 gray */
	SetColorEntry(17,  4096,  4096,  4096);	/*  2/32 gray */
	SetColorEntry(18,  6144,  6144,  6144);	/*  3/32 gray */
	SetColorEntry(19, 10240, 10240, 10240);	/*  5/32 gray */
	SetColorEntry(20, 12288, 12288, 12288);	/*  6/32 gray */
	SetColorEntry(21, 14336, 14336, 14336);	/*  7/32 gray */
	SetColorEntry(22, 18432, 18432, 18432);	/*  9/32 gray */
	SetColorEntry(23, 20480, 20480, 20480);	/* 10/32 gray */
	SetColorEntry(24, 22528, 22528, 22528);	/* 11/32 gray */
	SetColorEntry(25, 26624, 26624, 26624);	/* 13/32 gray */
	SetColorEntry(26, 28672, 28672, 28672);	/* 14/32 gray */
	SetColorEntry(27, 30720, 30720, 30720);	/* 15/32 gray */
	SetColorEntry(28, 34815, 34815, 34815);	/* 17/32 gray */
	SetColorEntry(29, 36863, 36863, 36863);	/* 18/32 gray */
	SetColorEntry(30, 38911, 38911, 38911);	/* 19/32 gray */
	SetColorEntry(31, 43007, 43007, 43007);	/* 21/32 gray */
	SetColorEntry(32, 45055, 45055, 45055);	/* 22/32 gray */
	SetColorEntry(33, 47103, 47103, 47103);	/* 23/32 gray */
	SetColorEntry(34, 51199, 51199, 51199);	/* 25/32 gray */
	SetColorEntry(35, 53247, 53247, 53247);	/* 26/32 gray */
	SetColorEntry(36, 55295, 55295, 55295);	/* 27/32 gray */
	SetColorEntry(37, 59391, 59391, 59391);	/* 29/32 gray */
	SetColorEntry(38, 61439, 61439, 61439);	/* 30/32 gray */
	SetColorEntry(39, 63487, 63487, 63487);	/* 31/32 gray */

	/* The remainder of color table defines a color cube with six steps
	   for each primary color. Note that the corners of this cube repeat
	   previous colors, but simplifies the mapping between RGB colors and
	   color map indices. This color cube spans indices 40 through 255.
	*/
	for (r = 0; r < 6; r++) {
		for (g = 0; g < 6; g++) {
			for (b = 0; b < 6; b++) {
				i = 40 + ((36 * r) + (6 * b) + g);
				if (i > 255) error("index out of range in color table compuation");
				SetColorEntry(i, (r * 65535) / 5, (g * 65535) / 5, (b * 65535) / 5);
			}
		}
	}

	stPixMap = NewPixMap();
	(*stPixMap)->pixelType = 0; /* chunky */
	(*stPixMap)->cmpCount = 1;
	(*stPixMap)->pmTable = stColorTable;
}

void SetUpWindow(void) {
	Rect windowBounds = {44, 8, 300, 500};

#ifndef IHAVENOHEAD
	stWindow = NewCWindow(
		0L, &windowBounds,
		"\p Welcome to Squeak!  Reading Squeak image file... ",
		true, zoomDocProc, (WindowPtr) -1L, false, 0);
#endif
}

void SetWindowTitle(char *title) {
    Str255 tempTitle;
	CopyCStringToPascal(title,tempTitle);
#ifndef IHAVENOHEAD
	SetWTitle(stWindow, tempTitle);
#endif
}

/*** Event Recording Functions ***/

int recordKeystroke(EventRecord *theEvent) {
	int asciiChar, modifierBits, keystate;

	/* keystate: low byte is the ascii character; next 4 bits are modifier bits */
	asciiChar = theEvent->message & charCodeMask;
	modifierBits = modifierMap[(theEvent->modifiers >> 8) & 0x1F];
	if ((modifierBits & 0x9) == 0x9) {  /* command and shift */
		if ((asciiChar >= 97) && (asciiChar <= 122)) {
			/* convert ascii code of command-shift-letter to upper case */
			asciiChar = asciiChar - 32;
		}
	}

	keystate = (modifierBits << 8) | asciiChar;
	if (keystate == interruptKeycode) {
		/* Note: interrupt key is "meta"; it not reported as a keystroke */
		interruptPending = true;
		interruptCheckCounter = 0;
	} else {
		keyBuf[keyBufPut] = keystate;
		keyBufPut = (keyBufPut + 1) % KEYBUF_SIZE;
		if (keyBufGet == keyBufPut) {
			/* buffer overflow; drop the last character */
			keyBufGet = (keyBufGet + 1) % KEYBUF_SIZE;
			keyBufOverflows++;
		}
	}
}

int recordMouseDown(EventRecord *theEvent) {

	/* button state: low three bits are mouse buttons; next 4 bits are modifier bits */
	buttonState = MouseModifierState(theEvent);
	cachedButtonState = cachedButtonState | buttonState;
}

int MouseModifierState(EventRecord *theEvent) {
	int stButtons;

	stButtons = 0;
	if ((theEvent->modifiers & btnState) == false) {  /* is false if button is down */
		stButtons = 4;		/* red button by default */
		if ((theEvent->modifiers & optionKey) != 0) {
			stButtons = 2;	/* yellow button if option down */
		}
		if ((theEvent->modifiers & cmdKey) != 0) {
			stButtons = 1;	/* blue button if command down */
		}
	} 

	/* button state: low three bits are mouse buttons; next 4 bits are modifier bits */
	return ((modifierMap[(theEvent->modifiers >> 8) & 0x1F] << 3) |
		(stButtons & 0x7));
}

int recordModifierButtons(EventRecord *theEvent) {
	int stButtons = 0;

	if ((theEvent->modifiers & btnState) == false) {
		stButtons = buttonState & 0x7;
	} else {
		stButtons = 0;
	}
	/* button state: low three bits are mouse buttons; next 4 bits are modifier bits */
	buttonState =
		(modifierMap[(theEvent->modifiers >> 8) & 0x1F] << 3) |
		(stButtons & 0x7);
}

int recordMouseEvent(EventRecord *theEvent, int theButtonState) {
	sqMouseEvent *evt;
	static sqMouseEvent oldEvent;
	
	evt = (sqMouseEvent*) nextEventPut();

	/* first the basics */
	evt->type = EventTypeMouse;
	evt->timeStamp = ioMSecs(); 
	GlobalToLocal((Point *) &theEvent->where);
	evt->x = theEvent->where.h;
	evt->y = theEvent->where.v;
	/* then the buttons */
	evt->buttons = theButtonState & 0x07;
	/* then the modifiers */
	evt->modifiers = theButtonState >> 3;
	/* clean up reserved */
	evt->reserved1 = 0;
	evt->reserved2 = 0;
	
	if (oldEvent.buttons == evt->buttons && 
	    oldEvent.x == evt->x &&
	    oldEvent.y == evt->y &&
	    oldEvent.modifiers == evt->modifiers) 
	    ignoreLastEvent();
	    
    oldEvent = *evt;

	
//	signalSemaphoreWithIndex(inputSemaphoreIndex);
	return 1;
}

int recordDragDropEvent(EventRecord *theEvent, int numberOfItems, int dragType) {
	sqDragDropFilesEvent *evt;
	int theButtonState;
	
	evt = (sqDragDropFilesEvent*) nextEventPut();

	/* first the basics */
	theButtonState = MouseModifierState(theEvent);
	evt->type = EventTypeDragDropFiles;
	evt->timeStamp = ioMSecs(); 
	GlobalToLocal((Point *) &theEvent->where);
	evt->x = theEvent->where.h;
	evt->y = theEvent->where.v;
	evt->numFiles = numberOfItems;
	evt->dragType = dragType;
	
	/* then the modifiers */
	evt->modifiers = theButtonState >> 3;
	/* clean up reserved */
	evt->reserved1 = 0;
//	signalSemaphoreWithIndex(inputSemaphoreIndex);
	return 1;
}

int recordKeyboardEvent(EventRecord *theEvent, int keyType) {
	int stButtons = 0;
	int asciiChar, modifierBits;
	sqKeyboardEvent *evt, *extra;

	evt = (sqKeyboardEvent*) nextEventPut();

	/* keystate: low byte is the ascii character; next 4 bits are modifier bits */
	asciiChar = theEvent->message & charCodeMask;
	modifierBits = MouseModifierState(theEvent); //Capture mouse/option states
	if (((modifierBits >> 3) & 0x9) == 0x9) {  /* command and shift */
		if ((asciiChar >= 97) && (asciiChar <= 122)) {
			/* convert ascii code of command-shift-letter to upper case */
			asciiChar = asciiChar - 32;
		}
	}

	/* first the basics */
	evt->type = EventTypeKeyboard;
	evt->timeStamp = ioMSecs();
	/* now the key code */
	/* press code must differentiate */
	evt->charCode = (theEvent->message & keyCodeMask) >> 8;
	evt->pressCode = keyType;
	evt->modifiers = modifierBits >> 3;
	/* clean up reserved */
	evt->reserved1 = 0;
	evt->reserved2 = 0;
	/* generate extra character event */
	if (keyType == EventKeyDown) {
		extra = (sqKeyboardEvent*)nextEventPut();
		*extra = *evt;
		extra->charCode = asciiChar;
		extra->pressCode = EventKeyChar;
	}
//	signalSemaphoreWithIndex(inputSemaphoreIndex);
	return 1;
}

static sqInputEvent *nextEventPut(void) {
	sqInputEvent *evt;
	evt = eventBuffer + eventBufferPut;
	eventBufferPut = (eventBufferPut + 1) % MAX_EVENT_BUFFER;
	if (eventBufferGet == eventBufferPut) {
		/* buffer overflow; drop the last event */
		eventBufferGet = (eventBufferGet + 1) % MAX_EVENT_BUFFER;
	}
	return evt;
}

void ignoreLastEvent() {
    eventBufferPut -= 1;
    if (eventBufferPut < 0) 
        eventBufferPut = MAX_EVENT_BUFFER -1;
}
int ioSetInputSemaphore(int semaIndex) {
	inputSemaphoreIndex = semaIndex;
	return 1;
}

int ioGetNextEvent(sqInputEvent *evt) {
	if (eventBufferGet == eventBufferPut) {
		if (gThreadManager)
    		YieldToAnyThread();
		else
		    ioProcessEvents();
	}
	if (eventBufferGet == eventBufferPut) 
		return false;

	*evt = eventBuffer[eventBufferGet];
	eventBufferGet = (eventBufferGet+1) % MAX_EVENT_BUFFER;
	return true;
}

int checkForModifierKeys() {
	enum {
			/* modifier keys */
		kVirtualCapsLockKey = 0x039,
		kVirtualShiftKey = 0x038,
		kVirtualControlKey = 0x03B,
		kVirtualOptionKey = 0x03A,
		kVirtualCommandKey = 0x037
	};
	KeyMap theKeys;
	unsigned char *keybytes;
	int result;
	
	GetKeys(theKeys);
	keybytes = (unsigned char *) theKeys;
	result  = gButtonIsDown ?  0 : btnState ;
	result += ((keybytes[kVirtualCapsLockKey>>3] & (1 << (kVirtualCapsLockKey&7))) != 0) ? alphaLock : 0;
	result += ((keybytes[kVirtualShiftKey>>3] & (1 << (kVirtualShiftKey&7))) != 0)       ? shiftKey : 0;
	result += ((keybytes[kVirtualControlKey>>3] & (1 << (kVirtualControlKey&7))) != 0)   ? controlKey : 0;
	result += ((keybytes[kVirtualOptionKey>>3] & (1 << (kVirtualOptionKey&7))) != 0)     ? optionKey : 0;
	result += ((keybytes[kVirtualCommandKey>>3] & (1 << (kVirtualCommandKey&7))) != 0)   ? cmdKey : 0;
	
	return result;
}


/*** Mac Specific External Primitive Support ***/

/* ioLoadModule:
	Load a module from disk.
	WARNING: this always loads a *new* module. Don't even attempt to find a loaded one.
	WARNING: never primitiveFail() within, just return 0
*/
int ioLoadModule(char *pluginName) {
	char pluginDirPath[1000];
	CFragConnectionID libHandle;
	Ptr mainAddr;
	Str255 errorMsg,tempPluginName;
	OSErr err;
    
    	/* first, look in the "<Squeak VM directory>Plugins" directory for the library */
	strcpy(pluginDirPath, vmPath);
	
#ifdef PLUGIN
	strcat(pluginDirPath, ":Plugins");
#else
	strcat(pluginDirPath, "Plugins");
#endif 	
    
    libHandle = LoadLibViaPath(pluginName, pluginDirPath);
	if (libHandle != nil) return (int) libHandle;

#ifndef PLUGIN
	/* second, look directly in Squeak VM directory for the library */
	libHandle = LoadLibViaPath(pluginName, vmPath);
	if (libHandle != nil) return (int) libHandle;
    
    /* Lastly look for it as a shared import library */
    
    CopyCStringToPascal(pluginName,tempPluginName);
    err = GetSharedLibrary(tempPluginName, kAnyCFragArch, kLoadCFrag, &libHandle, &mainAddr, errorMsg);
	if (err == noErr) 
	    err = GetSharedLibrary(tempPluginName, kAnyCFragArch, kFindCFrag, &libHandle, &mainAddr, errorMsg);
	if (libHandle != nil) return (int) libHandle;
#endif
    
	return nil;
}

/* ioFindExternalFunctionIn:
	Find the function with the given name in the moduleHandle.
	WARNING: never primitiveFail() within, just return 0.
*/
int ioFindExternalFunctionIn(char *lookupName, int moduleHandle) {
	CFragSymbolClass ignored;
	Ptr functionPtr = 0;
	OSErr err;
    Str255 tempLookupName;
    
	if (!moduleHandle) return 0;

	/* get the address of the desired primitive function */
	CopyCStringToPascal(lookupName,tempLookupName);
	err = FindSymbol(
		(CFragConnectionID) moduleHandle, (unsigned char *) tempLookupName,
		&functionPtr, &ignored);
	if (err) return 0;

	return (int) functionPtr;
}

/* ioFreeModule:
	Free the module with the associated handle.
	WARNING: never primitiveFail() within, just return 0.
*/
int ioFreeModule(int moduleHandle) {
	CFragConnectionID libHandle;
	OSErr err;

	if (!moduleHandle) return 0;
	libHandle = (CFragConnectionID) moduleHandle;
	err = CloseConnection(&libHandle);
	return 0;
}

CFragConnectionID LoadLibViaPath(char *libName, char *pluginDirPath) {
	short 				vRefNum;
	long				ignore;
	CInfoPBRec 			pb;
	FSSpec				fileSpec;
	Str255				problemLibName,fileSpecName,tempPlugindirPath;
    Ptr					junk;
	CFragConnectionID	libHandle = 0;
	OSErr				err = noErr;

	/* get the default volume */
	HGetVol( nil, &vRefNum, &ignore);

	/* get the directory ID for the given path */
	CopyCStringToPascal(pluginDirPath,tempPlugindirPath);
	pb.hFileInfo.ioNamePtr = tempPlugindirPath;
	pb.hFileInfo.ioVRefNum = 0;  /* use the default volume */
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = 0;
	err = PBGetCatInfoSync(&pb);
	if (err) return nil; /* bad plugin directory path */

	/* make a file spec for the given file name in the plugin directory */
	CopyCStringToPascal(libName,fileSpecName);
	FSMakeFSSpec(vRefNum,pb.hFileInfo.ioDirID,fileSpecName,&fileSpec);

	err = GetDiskFragment(
		&fileSpec, 0, kCFragGoesToEOF, nil, kLoadCFrag, &libHandle, &junk, problemLibName);
	if (err) 
	    return nil;
	return libHandle;
}
/*** I/O Primitives ***/

int ioBeep(void) {
	SysBeep(1000);
}

#ifndef PLUGIN
int ioExit(void) {
	UnloadScrap();
    ioShutdownAllModules();
	MenuBarRestore();
	ExitToShell();
}
#endif

int ioForceDisplayUpdate(void) {
	/* do nothing on a Mac */
}

int ioFormPrint(int bitsAddr, int width, int height, int depth, double hScale, double vScale, int landscapeFlag) {
	/* experimental: print a form with the given bitmap, width, height, and depth at
	   the given horizontal and vertical scales in the given orientation */
	printf("ioFormPrint width %d height %d depth %d hScale %f vScale %f landscapeFlag %d\n",
		width, height, depth, hScale, vScale, landscapeFlag);
	bitsAddr;
	return true;
}

int ioGetButtonState(void) {
	if (gThreadManager)
		YieldToAnyThread();
	else
	    ioProcessEvents();
	if ((cachedButtonState & 0x7) != 0) {
		int result = cachedButtonState;
		cachedButtonState = 0;  /* clear cached button state */
		return result;
	}
	cachedButtonState = 0;  /* clear cached button state */
	return buttonState;
}

int ioGetKeystroke(void) {
	int keystate;

	if (gThreadManager)
		YieldToAnyThread();
	else
	    ioProcessEvents();
	if (keyBufGet == keyBufPut) {
		return -1;  /* keystroke buffer is empty */
	} else {
		keystate = keyBuf[keyBufGet];
		keyBufGet = (keyBufGet + 1) % KEYBUF_SIZE;
		/* set modifer bits in buttonState to reflect the last keystroke fetched */
		buttonState = ((keystate >> 5) & 0xF8) | (buttonState & 0x7);
	}
	return keystate;
}

int ioHasDisplayDepth(int depth) {
	/* Return true if this platform supports the given color display depth. */

	switch (depth) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
		return true;
	}
	return false;
}


int ioMicroMSecsExpensive(void);

int ioMicroMSecsExpensive(void) {
	UnsignedWide microTicks;
	Microseconds(&microTicks);
	return (microTicks.lo / 1000) + (microTicks.hi * 4294967);
}

#if TARGET_CPU_PPC & !MINIMALVM
#if TARGET_API_MAC_CARBON
int ioMicroMSecs(void) { //Some versions of mac os/x return 0 when using OTElapsedMilliseconds
	Nanoseconds elapsedNanoseconds;   // an UnsignedWide integer

	if((Ptr)UpTime!=(Ptr)kUnresolvedCFragSymbolAddress){
		elapsedNanoseconds=AbsoluteToNanoseconds(UpTime());
		// NOTE: 4294967296.0 == (double)0x10000*0x10000
		return (4294967296.0*elapsedNanoseconds.hi+elapsedNanoseconds.lo)*1e-6;
	}else {
	    return ioMicroMSecsExpensive();
	}
}
#else
int ioMicroMSecs(void) {
	/* Note: This function and ioMSecs() both return a time in milliseconds. The difference
	   is that ioMicroMSecs() is called only when precise millisecond resolution is essential,
	   and thus it can use a more expensive timer than ioMSecs, which is called frequently.
	   However, later VM optimizations reduced the frequency of calls to ioMSecs to the point
	   where clock performance became less critical, and we also started to want millisecond-
	   resolution timers for real time applications such as music. */
	
	register long check;
	
	if((Ptr)OTElapsedMilliseconds!=(Ptr)kUnresolvedCFragSymbolAddress){
    	check = OTElapsedMilliseconds(&timeStart);
    	if (check != -1) 
    	    return check;
    	OTGetTimeStamp(&timeStart);
	    return ioMicroMSecs();
	}else {
	    return ioMicroMSecsExpensive();
	}
}
#endif
#else
int ioMicroMSecs(void) {
    return ioMicroMSecsExpensive();
}
#endif

int ioMSecs(void) {
	/* return a time in milliseconds for use in Delays and Time millisecondClockValue */
	/* Note: This was once a macro based on clock(); it now uses the microsecond clock for
	   greater resolution. See the comment in ioMicroMSecs(). */
	return ioMicroMSecs();
}

int ioMousePoint(void) {
	Point p;

	if (gThreadManager)
		YieldToAnyThread();
	else
	    ioProcessEvents();
	if (windowActive) {
		GetMouse(&p);
	} else {
		/* don't report mouse motion if window is not active */
		p = savedMousePosition;
	}
	return (p.h << 16) | (p.v & 0xFFFF);  /* x is high 16 bits; y is low 16 bits */
}

int ioPeekKeystroke(void) {
	int keystate;

	if (gThreadManager)
		YieldToAnyThread();
	else
	    ioProcessEvents();
	if (keyBufGet == keyBufPut) {
		return -1;  /* keystroke buffer is empty */
	} else {
		keystate = keyBuf[keyBufGet];
		/* set modifer bits in buttonState to reflect the last keystroke peeked at */
		buttonState = ((keystate >> 5) & 0xF8) | (buttonState & 0x7);
	}
	return keystate;
}

int ioProcessEvents(void) {
	/* This is a noop when running as a plugin; the browser handles events. */
	const int nextPollOffsetCheck = CLOCKS_PER_SEC/60, nextPowerCheckOffset=CLOCKS_PER_SEC/2; 
	static clock_t nextPollTick = 0, nextPowerCheck=0;
	long    clockTime;

#ifndef PLUGIN
	if (clock() >= nextPollTick) {
		/* time to process events! */
		while (HandleEvents()) {
			/* process all pending events */
		}
		
        clockTime = clock();
        
        if (gDisablePowerManager && gTapPowerManager) {
            if (clockTime > gDisableIdleTickLimit)
                gDisableIdleTickLimit = IdleUpdate() + gDisableIdleTickCount;
                
#if TARGET_CPU_PPC & !MINIMALVM
            if (clockTime > nextPowerCheck) {
                 UpdateSystemActivity(UsrActivity);
                 nextPowerCheck = clockTime + nextPowerCheckOffset;
            }
#endif
        }
        
		/* wait a while before trying again */
		nextPollTick = clockTime + nextPollOffsetCheck;
	}
#endif
	return interruptPending;
}

int ioRelinquishProcessorForMicroseconds(int microSeconds) {
	/* This operation is platform dependent. 	 */
    static long counter = 0;
    microSeconds;
    
    if (counter++ >= 100) {
#if TARGET_API_MAC_CARBON
    	gDelayTime = 1;  // wait 1/30 second next time because we are idle
#endif
       	counter = 0;
    }
    if (gThreadManager)
		YieldToAnyThread();
	else
	    ioProcessEvents();
	
}

int ioScreenDepth(void) {
    Rect ignore;
    
    GDHandle mainDevice = getDominateDevice(stWindow,&ignore);
    if (mainDevice == null) 
        return 8;
    
    return (*(*mainDevice)->gdPMap)->pixelSize;
}

#ifndef PLUGIN
int ioScreenSize(void) {
	int w = 10, h = 10;
    Rect portRect;
    
#ifndef IHAVENOHEAD
	if (stWindow != nil) {
        GetPortBounds(GetWindowPort(stWindow),&portRect);
		w =  portRect.right -  portRect.left;
		h =  portRect.bottom - portRect.top;
	}
#endif
	return (w << 16) | (h & 0xFFFF);  /* w is high 16 bits; h is low 16 bits */
}
#endif

int ioSeconds(void) {
	struct tm timeRec;
	time_t time1904, timeNow;

	/* start of ANSI epoch is midnight of Jan 1, 1904 */
	timeRec.tm_sec   = 0;
	timeRec.tm_min   = 0;
	timeRec.tm_hour  = 0;
	timeRec.tm_mday  = 1;
	timeRec.tm_mon   = 0;
	timeRec.tm_year  = 4;
	timeRec.tm_wday  = 0;
	timeRec.tm_yday  = 0;
	timeRec.tm_isdst = 0;
	time1904 = mktime(&timeRec);

	timeNow = time(NULL);

	/* Squeak epoch is Jan 1, 1901, 3 non-leap years earlier than ANSI one */
	return (timeNow - time1904) + (3 * 365 * 24 * 60 * 60);
}

int ioSetCursor(int cursorBitsIndex, int offsetX, int offsetY) {
	/* Old version; forward to new version. */
	ioSetCursorWithMask(cursorBitsIndex, nil, offsetX, offsetY);
}

int ioSetCursorWithMask(int cursorBitsIndex, int cursorMaskIndex, int offsetX, int offsetY) {
	/* Set the 16x16 cursor bitmap. If cursorMaskIndex is nil, then make the mask the same as
	   the cursor bitmap. If not, then mask and cursor bits combined determine how cursor is
	   displayed:
			mask	cursor	effect
			 0		  0		transparent (underlying pixel shows through)
			 1		  1		opaque black
			 1		  0		opaque white
			 0		  1		invert the underlying pixel
	*/
	Cursor macCursor;
	int i;

	if (cursorMaskIndex == nil) {
		for (i = 0; i < 16; i++) {
			macCursor.data[i] = (checkedLongAt(cursorBitsIndex + (4 * i)) >> 16) & 0xFFFF;
			macCursor.mask[i] = (checkedLongAt(cursorBitsIndex + (4 * i)) >> 16) & 0xFFFF;
		}
	} else {
		for (i = 0; i < 16; i++) {
			macCursor.data[i] = (checkedLongAt(cursorBitsIndex + (4 * i)) >> 16) & 0xFFFF;
			macCursor.mask[i] = (checkedLongAt(cursorMaskIndex + (4 * i)) >> 16) & 0xFFFF;
		}
	}

	/* Squeak hotspot offsets are negative; Mac's are positive */
	macCursor.hotSpot.h = -offsetX;
	macCursor.hotSpot.v = -offsetY;
	SetCursor(&macCursor);
}

int ioSetDisplayMode(int width, int height, int depth, int fullscreenFlag) {
	/* Set the window to the given width, height, and color depth. Put the window
	   into the full screen mode specified by fullscreenFlag. */
	/* Note: Changing display depth is not yet, and may never be, implemented
	   on the Macintosh, where (a) it is considered inappropriate and (b) it may
	   not even be a well-defined operation if the Squeak window spans several
	   displays (which display's depth should be changed?). */

    GDHandle		dominantGDevice;
	Rect 			windRect;
	Handle			displayState;
	UInt32			depthMode=depth;
	long			value = 0,displayMgrPresent;
	DMDisplayModeListIteratorUPP	myModeIteratorProc = nil;	
	OSErr			error;
	DisplayIDType	theDisplayID;				
	DMListIndexType	theDisplayModeCount;		
	DMListType		theDisplayModeList;			
	VideoRequestRec	request;
	
#ifndef IHAVENOHEAD


	Gestalt(gestaltDisplayMgrAttr,&value);
	displayMgrPresent=value&(1<<gestaltDisplayMgrPresent);
    if (!displayMgrPresent) {
    	success(false);
    	return;
    }

    dominantGDevice = getDominateDevice(stWindow,&windRect);
	request.screenDevice  = dominantGDevice;
	request.reqBitDepth = depth;
	request.reqHorizontal = width;
	request.reqVertical = height;
	request.requestFlags = 1<<kAbsoluteRequestBit;
	request.displayMode = 0;
	myModeIteratorProc = NewDMDisplayModeListIteratorUPP(ModeListIterator);	// for DM2.0 searches

	if  (dominantGDevice && myModeIteratorProc) {
		if( noErr == DMGetDisplayIDByGDevice( dominantGDevice, &theDisplayID, false ) ) {
			theDisplayModeCount = 0;
			if (noErr == DMNewDisplayModeList(theDisplayID, 0, 0, &theDisplayModeCount, &theDisplayModeList) ) {
				GetRequestTheDM2Way (&request, dominantGDevice, myModeIteratorProc, theDisplayModeCount, &theDisplayModeList);
				DMDisposeList(theDisplayModeList);	
			} else {
			}
		}
	}
	
	if (myModeIteratorProc)
		DisposeDMDisplayModeListIteratorUPP(myModeIteratorProc);
	if (request.displayMode == 0)  {
    	success(false);
    	return;
    }
	DMBeginConfigureDisplays(&displayState);
	DMSetDisplayMode(dominantGDevice,request.displayMode,&depthMode,null,displayState);
	DMEndConfigureDisplays(displayState);
	ioSetFullScreen(fullscreenFlag);
	
	
#endif
}

#ifndef PLUGIN
int ioSetFullScreen(int fullScreen) {
	Rect                screen,portRect,windRect;
	int                 width, height, maxWidth, maxHeight;
	int                 oldWidth, oldHeight;
    static Rect		    rememberOldLocation = {44,8,0,0};		
    GDHandle            	dominantGDevice;

    dominantGDevice = getDominateDevice(stWindow,&windRect);
    getDominateGDeviceRect(dominantGDevice,&screen,true);
    
	if (fullScreen) {
		GetPortBounds(GetWindowPort(stWindow),&rememberOldLocation);
		LocalToGlobal((Point*) &rememberOldLocation.top);
		LocalToGlobal((Point*) &rememberOldLocation.bottom);
		MenuBarHide();
		GetPortBounds(GetWindowPort(stWindow),&portRect);
		oldWidth =  portRect.right -  portRect.left;
		oldHeight =  portRect.bottom -  portRect.top;
		width  = screen.right - screen.left; 
		height = (screen.bottom - screen.top);
		if ((oldWidth < width) || (oldHeight < height)) {
			/* save old size if it wasn't already full-screen */ 
			savedWindowSize = (oldWidth << 16) + (oldHeight & 0xFFFF);
		}
		MoveWindow(stWindow, screen.left, screen.top, true);
		SizeWindow(stWindow, width, height, true);
		fullScreenFlag = true;
	} else {
		MenuBarRestore();

		/* get old window size */
		width  = (unsigned) savedWindowSize >> 16;
		height = savedWindowSize & 0xFFFF;

		/* minimum size is 64 x 64 */
		width  = (width  > 64) ?  width : 64;
		height = (height > 64) ? height : 64;

		/* maximum size is screen size inset slightly */
		maxWidth  = (screen.right  - screen.left) - 16;
		maxHeight = (screen.bottom - screen.top)  - 52;
		width  = (width  <= maxWidth)  ?  width : maxWidth;
		height = (height <= maxHeight) ? height : maxHeight;
		MoveWindow(stWindow, rememberOldLocation.left, rememberOldLocation.top, true);
		SizeWindow(stWindow, width, height, true);
		fullScreenFlag = false;
	}
}

int ioShowDisplay(
	int dispBitsIndex, int width, int height, int depth,
	int affectedL, int affectedR, int affectedT, int affectedB) {

	Rect		dstRect = { 0, 0, 0, 0 };
	Rect		srcRect = { 0, 0, 0, 0 };
	RgnHandle	maskRect = nil;

	if (stWindow == nil) {
		return;
	}

	dstRect.left	= 0;
	dstRect.top		= 0;
	dstRect.right	= width;
	dstRect.bottom	= height;

	srcRect.left	= 0;
	srcRect.top		= 0;
	srcRect.right	= width;
	srcRect.bottom	= height;

	(*stPixMap)->baseAddr = (void *) dispBitsIndex;
	/* Note: top three bits of rowBytes indicate this is a PixMap, not a BitMap */
	(*stPixMap)->rowBytes = (((((width * depth) + 31) / 32) * 4) & 0x1FFF) | 0x8000;
	(*stPixMap)->bounds = srcRect;
	(*stPixMap)->pixelSize = depth;

    if (depth<=8) { /*Duane Maxwell <dmaxwell@exobox.com> fix cmpSize Sept 18,2000 */
    	(*stPixMap)->cmpSize = depth;
    	(*stPixMap)->cmpCount = 1;
    } else if (depth==16) {
    	(*stPixMap)->cmpSize = 5;
    	(*stPixMap)->cmpCount = 3;
    } else if (depth==32) {
    	(*stPixMap)->cmpSize = 8;
    	(*stPixMap)->cmpCount = 3;
    }

	/* create a mask region so that only the affected rectangle is copied */
	maskRect = NewRgn();
	SetRectRgn(maskRect, affectedL, affectedT, affectedR, affectedB);

	SetPortWindowPort(stWindow);
	CopyBits((BitMap *) *stPixMap, GetPortBitMapForCopyBits(GetWindowPort(stWindow)), &srcRect, &dstRect, srcCopy, maskRect);
#if TARGET_API_MAC_CARBON
	QDFlushPortBuffer (GetWindowPort(stWindow), maskRect);
#endif
	DisposeRgn(maskRect);
}
#endif


/*** Image File Naming ***/

int imageNameSize(void) {
	return strlen(imageName);
}

int imageNameGetLength(int sqImageNameIndex, int length) {
	char *sqImageName = (char *) sqImageNameIndex;
	int count, i;

	count = strlen(imageName);
	count = (length < count) ? length : count;

	/* copy the file name into the Squeak string */
	for (i = 0; i < count; i++) {
		sqImageName[i] = imageName[i];
	}
	return count;
}

int imageNamePutLength(int sqImageNameIndex, int length) {
	char *sqImageName = (char *) sqImageNameIndex;
	int count, i, ch, j;
	int lastColonIndex = -1;

	count = (IMAGE_NAME_SIZE < length) ? IMAGE_NAME_SIZE : length;

	/* copy the file name into a null-terminated C string */
	for (i = 0; i < count; i++) {
		ch = imageName[i] = sqImageName[i];
		if (ch == ':') {
			lastColonIndex = i;
		}
	}
	imageName[count] = 0;

	/* copy short image name into a null-terminated C string */
	for (i = lastColonIndex + 1, j = 0; i < count; i++, j++) {
		shortImageName[j] = imageName[i];
	}
	shortImageName[j] = 0;

	SetWindowTitle(shortImageName);
	return count;
}


/*** Profiling ***/

int clearProfile(void) {
#ifdef MAKE_PROFILE
	ProfilerClear();
#endif
}

int dumpProfile(void) {
#ifdef MAKE_PROFILE
	ProfilerDump("\pProfile.out");
#endif
}

int startProfiling(void) {
#ifdef MAKE_PROFILE
	ProfilerSetStatus(true);
#endif
}

int stopProfiling(void) {
#ifdef MAKE_PROFILE
	ProfilerSetStatus(false);
#endif
}

/*** Plugin Support ***/

int plugInInit(char *fullImagePath) {

	fullImagePath;
	/* check the interpreter's size assumptions for basic data types */
	if (sizeof(int) != 4) {
		error("This C compiler's integers are not 32 bits.");
	}
	if (sizeof(double) != 8) {
		error("This C compiler's floats are not 64 bits.");
	}
	if (sizeof(time_t) != 4) {
		error("This C compiler's time_t's are not 32 bits.");
	}

	/* clear all path and file names */
	imageName[0] = shortImageName[0] = documentName[0] = vmPath[0] = 0;

#if TARGET_CPU_PPC & !MINIMALVM
	if((Ptr)OTGetTimeStamp!=(Ptr)kUnresolvedCFragSymbolAddress)
 	    OTGetTimeStamp(&timeStart);
#endif

	PowerMgrCheck();
	SetUpClipboard();
	SetUpPixmap();
}

int plugInShutdown(void) {
	ioShutdownAllModules();
	FreeClipboard();
	FreePixmap();
	if (memory != nil) {
	    DisposeThread(gSqueakThread,null,true);
		DisposePtr((void *) memory);
		memory = nil;
	}
}


/*** System Attributes ***/

int IsImageName(char *name) {
	char *suffix;

	suffix = strrchr(name, '.');  /* pointer to last period in name */
	if (suffix == NULL) return false;
	if (strcmp(suffix, ".ima") == 0) return true;
	if (strcmp(suffix, ".image") == 0) return true;
	if (strcmp(suffix, ".IMA") == 0) return true;
	if (strcmp(suffix, ".IMAGE") == 0) return true;
	return false;
}

char * GetAttributeString(int id) {
	/* This is a hook for getting various status strings back from
	   the OS. In particular, it allows Squeak to be passed arguments
	   such as the name of a file to be processed. Command line options
	   are reported this way as well, on platforms that support them.
	*/

	// id #0 should return the full name of VM; for now it just returns its path
	if (id == 0) return vmPath;
	/* Note: 1.3x images will try to read the image as a document because they
	   expect attribute #1 to be the document name. A 1.3x image can be patched
	   using a VM of 2.6 or earlier. */
	if (id == 1) return imageName;
	if (id == 2) return documentName;

#ifdef PLUGIN
	/* When running in browser, return the EMBED tag info */
	if ((id > 2) && (id <= (2 + (2 * pluginArgCount)))) {
		int i = id - 3;
		if ((i & 1) == 0) {  /* i is even */
			return pluginArgName[i/2];
		} else {
			return pluginArgValue[i/2];
		}
	}
#endif

	if (id == 1001) return "Mac OS";
	if (id == 1002) {
		long myattr;
		static char data[32];
		
		Gestalt(gestaltSystemVersion, &myattr);
		sprintf(data,"%X",myattr);
		return data;
	}
	if (id == 1003) {
		long myattr;
		
		Gestalt(gestaltSysArchitecture, &myattr);
		if (myattr == gestalt68k) 
			return "68K";
		else
			return "PowerPC";
	}
	if (id == 1004) return (char *) interpreterVersion;

	/* attribute undefined by this platform */
	success(false);
	return "";
}

int attributeSize(int id) {
	return strlen(GetAttributeString(id));
}

int getAttributeIntoLength(int id, int byteArrayIndex, int length) {
	char *srcPtr, *dstPtr, *end;
	int charsToMove;

	srcPtr = GetAttributeString(id);
	charsToMove = strlen(srcPtr);
	if (charsToMove > length) {
		charsToMove = length;
	}

	dstPtr = (char *) byteArrayIndex;
	end = srcPtr + charsToMove;
	while (srcPtr < end) {
		*dstPtr++ = *srcPtr++;
	}
	return charsToMove;
}

/*** Image File Operations ***/

void sqImageFileClose(sqImageFile f) {
	FSClose(f);
}

sqImageFile sqImageFileOpen(char *fileName, char *mode) {
	short int err, err2, fRefNum;
	Str255 tempPascalFileName; 
    FInfo fileInfo;

	CopyCStringToPascal(fileName,tempPascalFileName);
	if (strchr(mode, 'w') != null) 
	    err = HOpenDF(0,0,tempPascalFileName,fsRdWrPerm, &fRefNum);
	 else
	    err = HOpenDF(0,0,tempPascalFileName,fsRdPerm, &fRefNum);
	    
	if ((err != 0) && (strchr(mode, 'w') != null)) {
		/* creating a new file for "save as" */
		err2 = HCreate(0,0,tempPascalFileName,  'FAST', 'STim');
		if (err2 == 0) {
			err = HOpenDF(0,0,tempPascalFileName,fsRdWrPerm, &fRefNum);
		}
	}

	if (err != 0) return null;

	if (strchr(mode, 'w') != null) {
        err = HGetFInfo(0,0,tempPascalFileName,&fileInfo);
        if (err != noErr) return 0; //This should not happen
        
        //On the mac we start at location 0 if this isn't an VM
        
    	if (!(fileInfo.fdType == 'APPL' && fileInfo.fdCreator == 'FAST')){
    		/* truncate non-VM file if opening in write mode */
    		err = SetEOF(fRefNum, 0);
    		if (err != 0) {
    			FSClose(fRefNum);
    			return null;
    		}
	    }
	}
	return (sqImageFile) fRefNum;
}

int sqImageFilePosition(sqImageFile f) {
	long int currentPosition = 0;

	GetFPos(f, &currentPosition);
	return currentPosition;
}

int sqImageFileRead(void *ptr, int elementSize, int count, sqImageFile f) {
	long int byteCount = elementSize * count;
	ParamBlockRec pb;
    OSErr error;

	pb.ioParam.ioRefNum = f;
    pb.ioParam.ioCompletion = NULL;
    pb.ioParam.ioBuffer = (Ptr)ptr;
    pb.ioParam.ioReqCount = byteCount;
    pb.ioParam.ioPosMode = fsAtMark + noCacheMask;
    pb.ioParam.ioPosOffset = 0;
    error = PBReadSync(&pb);
    byteCount = pb.ioParam.ioActCount;       
    
	if (error != 0) return 0;
	return byteCount / elementSize;
}

void sqImageFileSeek(sqImageFile f, int pos) {
	SetFPos(f, fsFromStart, pos);
}

int sqImageFileWrite(void *ptr, int elementSize, int count, sqImageFile f) {
	long int byteCount = elementSize * count;
	ParamBlockRec pb;
    OSErr error;

    pb.ioParam.ioRefNum = f;
    pb.ioParam.ioCompletion = NULL;
    pb.ioParam.ioBuffer = (Ptr)ptr;
    pb.ioParam.ioReqCount = byteCount;
    pb.ioParam.ioPosMode = fsAtMark + noCacheMask;
    pb.ioParam.ioPosOffset = 0;
    error = PBWriteSync(&pb);
    byteCount = pb.ioParam.ioActCount;       
    
	if (error != 0) 
	    return 0;
	return byteCount / elementSize;
}

int calculateStartLocationForImage() { 

	Handle cfrgResource;  
	long	memberCount,i;
	CFragResourceMember *target;
	
	cfrgResource = GetResource(kCFragResourceType,0); 
	if (cfrgResource == nil || ResError() != noErr) { return 0;};  
	
	memberCount = ((CFragResource *)(*cfrgResource))->memberCount;
	if (memberCount <= 1) {
        ReleaseResource(cfrgResource);
	    return 0; //Need FAT to get counters right
	}
	
	target = &((CFragResource *)(*cfrgResource))->firstMember;
	for(i=0;i<memberCount;i++) {
		if (target->architecture == 'FAST') {			
		    ReleaseResource(cfrgResource);
		    return target->offset;
		}
		target = NextCFragResourceMemberPtr(target); 
	}
    ReleaseResource(cfrgResource);
	return 0;
}

int sqImageFileStartLocation(int fileRef, char *filename, int imageSize){
    FInfo fileInfo;
	Str255 tempPascalFileName;
	OSErr   err; 
    int     resFileRef;
	Handle  cfrgResource,newcfrgResource;  
	UInt32	maxOffset=0,maxOffsetLength,targetOffset;
	long    memberCount,i;
	CFragResourceMember *target;
  
    
	CopyCStringToPascal(filename,tempPascalFileName);
    err = HGetFInfo(0,0,tempPascalFileName,&fileInfo);
    if (err != noErr) return 0; //This should not happen
    
    //On the mac we start at location 0 if this isn't an VM
    
	if (!(fileInfo.fdType == 'APPL' && fileInfo.fdCreator == 'FAST')) return 0;
    
    //Ok we have an application file, open the resource part and attempt to find the crfg
    
    resFileRef = HOpenResFile(0,0,tempPascalFileName,fsWrPerm);
    if (resFileRef == -1) return 0;
    
	cfrgResource = GetResource(kCFragResourceType,0);
	if (cfrgResource == nil || ResError() != noErr) {CloseResFile(resFileRef); return 0;};  
	
	memberCount = ((CFragResource *)(*cfrgResource))->memberCount;
	if (memberCount == 0) {ReleaseResource(cfrgResource); CloseResFile(resFileRef); return 0;};  //Need FAT to get counters right
	
	target = &((CFragResource *)(*cfrgResource))->firstMember;
	
	if (memberCount == 1) {
	   if (target->length == 0) {
        	SInt16 fileRef;
        	long lengthOfDataFork;
        	
        	err = HOpenDF(0,0,tempPascalFileName,fsRdPerm,&fileRef);
        	if (err) {ReleaseResource(cfrgResource); CloseResFile(resFileRef); FSClose(fileRef); return 0;}; 
        	
        	GetEOF(fileRef,&lengthOfDataFork);
            FSClose(fileRef);
            
	        target->length = lengthOfDataFork; //Fix up zero length targets
			maxOffset = target->offset;
			maxOffsetLength = target->length;
       } else {
			maxOffset = target->offset;
			maxOffsetLength = target->length;
       }
    } else {
	    for(i=0;i<memberCount;i++) {
    		if (target->architecture == 'FAST') {
    		    targetOffset = target->offset;
    		    target->length = imageSize;
    		    ChangedResource(cfrgResource);
            	if (ResError() != noErr) {ReleaseResource(cfrgResource); CloseResFile(resFileRef); return 0;}; 
    		    UpdateResFile(resFileRef);
            	if (ResError() != noErr) {ReleaseResource(cfrgResource); CloseResFile(resFileRef); return 0;}; 
                ReleaseResource(cfrgResource); 
    		    CloseResFile(resFileRef);
    			return targetOffset;
    		}
    		if (target->offset > maxOffset) {
    			maxOffset = target->offset;
    			maxOffsetLength = target->length;
    		}
    		target = NextCFragResourceMemberPtr(target);
    	}
	}
	//Ok at this point we need to alter the crfg to add the new tag for the image part
	
	newcfrgResource = cfrgResource;
	err = HandToHand(&newcfrgResource);
	if (err != noErr || MemError() != noErr)  {ReleaseResource(cfrgResource); CloseResFile(resFileRef); return 0;}; 
	SetHandleSize(newcfrgResource,GetHandleSize(cfrgResource)+AlignToFour(kBaseCFragResourceMemberSize + 1));
	if (MemError() != noErr)  {ReleaseResource(cfrgResource); CloseResFile(resFileRef); return 0;}; 
	
	target = &((CFragResource *)(*newcfrgResource))->firstMember; 
	for(i=0;i<memberCount;i++) {
		target = NextCFragResourceMemberPtr(target); 
	}

    target->architecture = 'FAST';
    target->reservedA = 0;                  /* ! Must be zero!*/
    target->reservedB = 0;                  /* ! Must be zero!*/
    target->updateLevel = 0;
    target->currentVersion = 0;
    target->oldDefVersion = 0;
    target->uUsage1.appStackSize = 0;
    target->uUsage2.appSubdirID = 0;
    target->uUsage2.libFlags = 0;
    target->usage = kApplicationCFrag;
    target->where = kDataForkCFragLocator;
    target->offset = maxOffset + maxOffsetLength;
    targetOffset = target->offset;
    target->length = imageSize;
    target->uWhere1.spaceID = 0;
    target->extensionCount = 0;             /* The number of extensions beyond the name.*/
    target->memberSize = AlignToFour(kBaseCFragResourceMemberSize + 1);   /* Size in bytes, includes all extensions.*/
    target->name[0] = 0x00;

	((CFragResource *)(*newcfrgResource))->memberCount = memberCount+1;
	RemoveResource(cfrgResource);
	if (ResError() != noErr) {CloseResFile(resFileRef); return 0;}; 
 	AddResource(newcfrgResource,kCFragResourceType,0,nil);
	if (ResError() != noErr) {CloseResFile(resFileRef); return 0;}; 
    UpdateResFile(resFileRef);
	if (ResError() != noErr) {CloseResFile(resFileRef); return 0;}; 
    CloseResFile(resFileRef);
    
	return targetOffset;
}

#ifndef PLUGIN
void * sqAllocateMemory(int minHeapSize, int desiredHeapSize) {
	/* Application allocates Squeak object heap memory from its own heap. */	
	minHeapSize;
	return NewPtr(desiredHeapSize);;
}
#endif

#if TARGET_API_MAC_CARBON
int sqGrowMemoryBy(int memoryLimit, int delta) {
    SetPtrSize ((char *) memory,  GetPtrSize((char *) memory) + delta);
    if (MemError() == noErr) 
        return memoryLimit + delta;
   return memoryLimit;
}

int sqShrinkMemoryBy(int memoryLimit, int delta) {
    return sqGrowMemoryBy(memoryLimit,0-delta);
}

int sqMemoryExtraBytesLeft(Boolean flag) {
    if (flag) 
        return FreeMem();
    else
        return 0;
}
#else
int sqGrowMemoryBy(int memoryLimit, int delta) {
    return memoryLimit;
}

int sqShrinkMemoryBy(int memoryLimit, int delta) {
    return memoryLimit;
}

int sqMemoryExtraBytesLeft(Boolean flag) {
    return 0;
}
#endif

void PowerMgrCheck(void) {
	long pmgrAttributes;
	
	gTapPowerManager = false;
	gDisablePowerManager = false;
#if !MINIMALVM
	if (! Gestalt(gestaltPowerMgrAttr, &pmgrAttributes))
		if ((pmgrAttributes & (1<<gestaltPMgrExists)) 
		    && (pmgrAttributes & (1<<gestaltPMgrDispatchExists))
		    && (PMSelectorCount() >= 0x24)) {
		    gTapPowerManager = true;
			gDisableIdleTickLimit = clock();
		}
#endif
}

int ioDisablePowerManager(int disableIfNonZero) {
    gDisablePowerManager = disableIfNonZero;
}

Boolean RunningOnCarbonX(void)
{
    UInt32 response;
    
    return (Gestalt(gestaltSystemVersion, 
                    (SInt32 *) &response) == noErr)
                && (response >= 0x01000);
}

/*** Main ***/

#ifndef PLUGIN
void main(void) {
	EventRecord theEvent;
	sqImageFile f;
	int reservedMemory, availableMemory;

	short vRefNum;
	long dirID;
	OSErr err;
    long threadGestaltInfo;
 
	InitMacintosh();
	PowerMgrCheck();
	
	SetUpMenus();
	SetUpClipboard();
	SetUpWindow();
	SetUpPixmap();
	
	SetEventMask(everyEvent); // also get key up events
	
#if TARGET_CPU_PPC & !MINIMALVM
	if((Ptr)OTGetTimeStamp!=(Ptr)kUnresolvedCFragSymbolAddress)
 	    OTGetTimeStamp(&timeStart);
#endif 

	/* install apple event handlers and wait for open event */
	imageName[0] = shortImageName[0] = documentName[0] = vmPath[0] = 0;
	InstallAppleEventHandlers();
	while (shortImageName[0] == 0) {
		GetNextEvent(everyEvent, &theEvent);
		if (theEvent.what == kHighLevelEvent) {
			AEProcessAppleEvent(&theEvent);
		}
	}
	if (imageName[0] == 0) {
		err = GetApplicationDirectory(&vRefNum, &dirID);
		if (err != noErr) error("Could not obtain default directory");
		StoreFullPathForLocalNameInto(shortImageName, imageName, IMAGE_NAME_SIZE, vRefNum, dirID);
	}

	/* check the interpreter's size assumptions for basic data types */
	if (sizeof(int) != 4) {
		error("This C compiler's integers are not 32 bits.");
	}
	if (sizeof(double) != 8) {
		error("This C compiler's floats are not 64 bits.");
	}
	if (sizeof(time_t) != 4) {
		error("This C compiler's time_t's are not 32 bits.");
	}

#ifdef MAKE_PROFILE
	ProfilerInit(collectDetailed, bestTimeBase, 1000, 50);
	ProfilerSetStatus(false);
	ProfilerClear();
#endif

	/* compute the desired memory allocation */
#ifdef JITTER
	reservedMemory = 1000000;
#else
#if MINIMALVM
	reservedMemory = 128000;
#else
	reservedMemory = 500000;
#endif
#endif

	if (RunningOnCarbonX())
	    availableMemory = 1000*1024*1024 - reservedMemory;
	else 
    	availableMemory = MaxBlock() - reservedMemory;

	/******
	  Note: This is platform-specific. On the Mac, the user specifies the desired
	    memory partition for each application using the Finder's Get Info command.
	    MaxBlock() returns the amount of memory in the partition minus space for
	    the code segment and other resources. On other platforms, the desired heap
	    size would be specified in other ways (e.g, via a command line argument).
	    The maximum size of the object heap is fixed at at startup. If you run low
	    on space, you must save the image and restart with more memory.

	  Note: Some memory must be reserved for Mac toolbox calls, sound buffers, etc.
	    A 30K reserve is too little. 40K allows Squeak to run but crashes if the
	    console is opened. 50K allows the console to be opened (with and w/o the
	    profiler). I added another 30K to provide for sound buffers and reliability.
	    (Note: Later discovered that sound output failed if SoundManager was not
	    preloaded unless there is about 100K reserved. Added 50K to that.)
	    
	    JMM Note changed to 500k for Open Transport support on 68K machines
	******/

	/* uncomment the following when using the C transcript window for debugging: */
	//printf("Move this window, then hit CR\n"); getchar();

	/* read the image file and allocate memory for Squeak heap */
	f = sqImageFileOpen(imageName, "rb");
	while (f == NULL) {
	    //Failure attempt to ask the user to find the image file
	    
	    FSSpec vmfsSpec,imageFsSpec;
	    WDPBRec wdPB;
	    
	    err =  FSpLocationFromFullPath(vmPathSize(),vmPath,&vmfsSpec);
	    if (err) 
	        ioExit();
		err = squeakFindImage(&vmfsSpec,&imageFsSpec);
	    if (err) 
	        ioExit();
	    CopyPascalStringToC(imageFsSpec.name,shortImageName);
        StoreFullPathForLocalNameInto(shortImageName, imageName, IMAGE_NAME_SIZE, imageFsSpec.vRefNum, imageFsSpec.parID);

			/* make the image or document directory the working directory */
    	wdPB.ioNamePtr = NULL;
    	wdPB.ioVRefNum = imageFsSpec.vRefNum;
    	wdPB.ioWDDirID = imageFsSpec.parID;
    	PBHSetVolSync(&wdPB);
    	f = sqImageFileOpen(imageName, "rb");
 	}
	
	readImageFromFileHeapSizeStartingAt(f, availableMemory, calculateStartLocationForImage());
	sqImageFileClose(f);
#if !MINIMALVM
	 ioLoadFunctionFrom(NULL, "DropPlugin");
#endif
    
#ifndef IHAVENOHEAD
	SetWindowTitle(shortImageName);
	ioSetFullScreen(fullScreenFlag);
#endif

#if (!(defined JITTER) && defined(__MPW__))
	atexit(SqueakTerminate);
#endif

#if TARGET_CPU_PPC & !MINIMALVM 
    if( Gestalt( gestaltThreadMgrAttr, &threadGestaltInfo) == noErr &&
        threadGestaltInfo & (1<<gestaltThreadMgrPresent) &&
        ((Ptr) NewThread != (Ptr)kUnresolvedCFragSymbolAddress)) {
        gThreadManager = true;
        err = createNewThread();
        if (err == noErr) {
            while(true)  {
                ioProcessEvents();
        		YieldToAnyThread();
            }
            return;
        }
    }        
#endif

    gThreadManager = false;
    /* run Squeak */
    squeakThread(0);
}
#endif



OSErr createNewThread() {
#if TARGET_CPU_PPC
    gSqueakThreadUPP = NewThreadEntryUPP(squeakThread); //We should dispose of someday
#else
    gSqueakThreadUPP = (ThreadEntryProcPtr) NewRoutineDescriptor((ProcPtr)(squeakThread), uppThreadEntryProcInfo, GetCurrentArchitecture());
 #endif
 
#ifndef PLUGIN
	return NewThread( kCooperativeThread, gSqueakThreadUPP, nil, 80*1024, kCreateIfNeeded, 0L, &gSqueakThread);
#else
	return NewThread( kCooperativeThread, gSqueakThreadUPP, nil, 80*1024, kCreateIfNeeded+kNewSuspend, 0L, &gSqueakThread);
#endif
}

static pascal void* squeakThread(void *threadParm) {
	/* run Squeak */
#	ifdef JITTER
	j_interpret();
#	else
	interpret();
#	endif
}

void SqueakTerminate() {
#ifdef PLUGIN
	ExitCleanup();
#else
	UnloadScrap();
	ioShutdownAllModules();
#endif
}

WindowPtr getSTWindow(void) {
    return stWindow;
}

int setMessageHook(eventMessageHook theHook) {
    messageHook = theHook;
}

int setPostMessageHook(eventMessageHook theHook) {
    postMessageHook = theHook;
}

#if !TARGET_API_MAC_CARBON
//
//	CopyPascalStringToC converts the source pascal string to a destination
//	C string as it copies. 
//
void CopyPascalStringToC(ConstStr255Param src, char* dst)
{
	if ( src != NULL )
	{
		short   length  = *src++;
	
		while ( length > 0 ) 
		{
			*dst++ = *(char*)src++;
			--length;
		}
	}
	*dst = '\0';
}


//
//	CopyCStringToPascal converts the source C string to a destination
//	pascal string as it copies. The dest string will
//	be truncated to fit into an Str255 if necessary.
//  If the C String pointer is NULL, the pascal string's length is set to zero
//
void CopyCStringToPascal(const char* src, Str255 dst)
{
	short 	length  = 0;
	
	// handle case of overlapping strings
	if ( (void*)src == (void*)dst )
	{
		unsigned char*		curdst = &dst[1];
		unsigned char		thisChar;
				
		thisChar = *(const unsigned char*)src++;
		while ( thisChar != '\0' ) 
		{
			unsigned char	nextChar;
			
			// use nextChar so we don't overwrite what we are about to read
			nextChar = *(const unsigned char*)src++;
			*curdst++ = thisChar;
			thisChar = nextChar;
			
			if ( ++length >= 255 )
				break;
		}
	}
	else if ( src != NULL )
	{
		unsigned char*		curdst = &dst[1];
		short 				overflow = 255;		// count down so test it loop is faster
		register char		temp;
	
		// Can't do the K&R C thing of �while (*s++ = *t++)� because it will copy trailing zero
		// which might overrun pascal buffer.  Instead we use a temp variable.
		while ( (temp = *src++) != 0 ) 
		{
			*(char*)curdst++ = temp;
				
			if ( --overflow <= 0 )
				break;
		}
		length = 255 - overflow;
	}
	dst[0] = length;
}
#endif

#define rectWidth(aRect) ((aRect).right - (aRect).left)
#define rectHeight(aRect) ((aRect).bottom - (aRect).top)
#define MinWindowWidth(foo) 72*3
#define MinWindowHeight(foo) 72*3

#define max(X, Y) ( ((X)>(Y)) ? (X) : (Y) )
#define min(X, Y) (  ((X)>(Y)) ? (Y) : (X) )

#define pin(VALUE, MIN, MAX) ( ((VALUE) < (MIN)) ? (MIN) : ( ((VALUE) > (MAX)) ? (MAX) : (VALUE) ) )

void DoZoomWindow (EventRecord* theEvent, WindowPtr theWindow, short zoomDir, short hMax, short vMax)
{

	Rect				zoomRect,windRect,globalPortRect, dGDRect;
	GDHandle			dominantGDevice;
   
	if (TrackBox(theWindow, theEvent->where, zoomDir)) {
		SetPortWindowPort(theWindow);
		GetPortBounds(GetWindowPort(stWindow),&windRect);
		EraseRect(&windRect);	// recommended for cosmetic reasons

		if (zoomDir == inZoomOut) {

			/*
			 *	ZoomWindow() is a good basic tool, but it doesn't do everything necessary to
			 *	implement a good human interface when zooming. In fact it's not even close for
			 *	more high-end hardware configurations. We must help it along by calculating an
			 *	appropriate window size and location any time a window zooms out.
			 */

            dominantGDevice = getDominateDevice(theWindow,&windRect);
			/*
			 *	At this point, we know the dimensions of the window we're zooming, and we know
			 *	what screen we're going to put it on. To be more specific, however, we need a
			 *	rectangle which defines the maximum dimensions of the resized window's contents.
			 *	This rectangle accounts for the thickness of the window frame, the menu bar, and
			 *	one or two pixels around the edges for cosmetic compatibility with ZoomWindow().
			 */

            getDominateGDeviceRect(dominantGDevice,&dGDRect,false);
            
			GetPortBounds(GetWindowPort(theWindow),&globalPortRect);
			LocalToGlobal(&(((Point *) &(globalPortRect))[0]));		// calculate the window's portRect
			LocalToGlobal(&(((Point *) &(globalPortRect))[1]));		// in global coordinates

			// account for the window frame and inset it a few pixels
			dGDRect.left	+= 2 + globalPortRect.left - windRect.left;
			dGDRect.top		+= 2 + globalPortRect.top - windRect.top;
			dGDRect.right	-= 1 + windRect.right - globalPortRect.right;
			dGDRect.bottom	-= 1 + windRect.bottom - globalPortRect.bottom;

			/*
			 *	Now we know exactly what our limits are, and since there are input parameters
			 *	specifying the dimensions we'd like to see, we can move and resize the zoom
			 *	state rectangle for the best possible results. We have three goals in this:
			 *	1. Display the window entirely visible on a single device.
			 *	2. Resize the window to best represent the dimensions of the document itself.
			 *	3. Move the window as short a distance as possible to achieve #1 and #2.
			 */

			GetWindowStandardState(theWindow, &zoomRect);

			/*
			 *	Initially set the zoom rectangle to the size requested by the input parameters,
			 *	although not smaller than a minimum size. We do this without moving the origin.
			 */

			zoomRect.right = (zoomRect.left = globalPortRect.left) +
									max(hMax, MinWindowWidth(theWindow));
			zoomRect.bottom = (zoomRect.top = globalPortRect.top) +
									max(vMax, MinWindowHeight(theWindow));

			// Shift the entire rectangle if necessary to bring its origin inside dGDRect.
			OffsetRect(&zoomRect,
						max(dGDRect.left - zoomRect.left, 0),
						max(dGDRect.top - zoomRect.top, 0));

			/*
			 *	Shift the rectangle up and/or to the left if necessary to accomodate the view,
			 *	and if it is possible to do so. The rectangle may not be moved such that its
			 *	origin would fall outside of dGDRect.
			 */

			OffsetRect(&zoomRect,
						-pin(zoomRect.right - dGDRect.right, 0, zoomRect.left - dGDRect.left),
						-pin(zoomRect.bottom - dGDRect.bottom, 0, zoomRect.top - dGDRect.top));

			// Clip expansion to dGDRect, in case view is larger than dGDRect.
			zoomRect.right = min(zoomRect.right, dGDRect.right);
			zoomRect.bottom = min(zoomRect.bottom, dGDRect.bottom);
			SetWindowStandardState(theWindow, &zoomRect);
		}

		ZoomWindow(theWindow, zoomDir, false);		// all it needed was a brain transplant
	}
}

GDHandle getDominateDevice( WindowPtr theWindow,Rect *windRect) {
	GDHandle			nthDevice, dominantGDevice;
	long				sectArea, greatestArea;
    long                quickDrawAttributes;
	Rect				theSect;
    RgnHandle           windowRegion;

    
#if TARGET_API_MAC_CARBON
			windowRegion = NewRgn();
			GetWindowRegion(theWindow,kWindowStructureRgn,windowRegion);
			GetRegionBounds(windowRegion,windRect);
#else
			*windRect = (**((WindowPeek) theWindow)->strucRgn).rgnBBox;
#endif
			dominantGDevice = nil;
    	    if (! Gestalt(gestaltQuickdrawFeatures, &quickDrawAttributes) && 
    	        (quickDrawAttributes & (1<<gestaltHasColor))) {

				/*
				 *	Color QuickDraw implies the possibility of multiple monitors. This is where
				 *	zooming becomes more interesting. One should zoom onto the monitor containing
				 *	the greatest portion of the window. This requires walking the gDevice list.
				 */

				nthDevice = GetDeviceList();
				greatestArea = 0;
				while (nthDevice != nil) {
					if (TestDeviceAttribute(nthDevice, screenDevice)) {
						if (TestDeviceAttribute(nthDevice, screenActive)) {
							SectRect(windRect, &(**nthDevice).gdRect, &theSect);
							sectArea = (long) rectWidth(theSect) * (long) rectHeight(theSect);
							if (sectArea > greatestArea) {
								greatestArea = sectArea;		// save the greatest intersection
								dominantGDevice = nthDevice;	// and which device it belongs to
							}
						}
					}
					nthDevice = GetNextDevice(nthDevice);
				}
			}
    return dominantGDevice;
}

void getDominateGDeviceRect(GDHandle dominantGDevice,Rect *dGDRect,Boolean forgetMenuBar) {
    BitMap              bmap;

	if (dominantGDevice != nil) {
			*dGDRect = (**dominantGDevice).gdRect;
			if (dominantGDevice == GetMainDevice())		// account for menu bar on main device
				if (!forgetMenuBar) 
				        dGDRect->top += GetMBarHeight();
		}
		else {
			GetQDGlobalsScreenBits(&bmap);
			*dGDRect = bmap.bounds;				// if no gDevice, use default monitor
			if (!forgetMenuBar)
			    dGDRect->top += GetMBarHeight();
		}
}

/*#	MacOS� Sample Code
#	
#	Written by: Eric Anderson
#	 email: eric3@apple.com
#
#	Display Manager sample code
#	RequestVideo demonstrates the usage of the Display Manager introduced
#	with the PowerMacs and integrated into the system under System 7.5. With
#	the RequestVideo sample code library, developers will be able to explore
#	the Display Manager API by changing bit depth and screen resolution on
#	multisync displays on built-in, NuBus, and PCI based video. Display Manager 1.0
#	is built into the Systems included with the first PowerMacs up through System 7.5.
#	Display Manager 2.0 is included with the release of the new PCI based PowerMacs,
#	and will be included in post 7.5 System Software releases. 
*/

pascal void ModeListIterator(void *userData, DMListIndexType, DMDisplayModeListEntryPtr displaymodeInfo)
{
	unsigned long			depthCount;
	short					iCount;
	ListIteratorDataRec		*myIterateData		= (ListIteratorDataRec*) userData;
	DepthInfo				*myDepthInfo;
	
	// set user data in a round about way
	myIterateData->displayModeTimingInfo		= *displaymodeInfo->displayModeTimingInfo;
	
	// now get the DMDepthInfo info into memory we own
	depthCount = displaymodeInfo->displayModeDepthBlockInfo->depthBlockCount;
	myDepthInfo = (DepthInfo*)NewPtrClear(depthCount * sizeof(DepthInfo));

	// set the info for the caller
	myIterateData->depthBlockCount = depthCount;
	myIterateData->depthBlocks = myDepthInfo;

	// and fill out all the entries
	if (depthCount) for (iCount=0; iCount < depthCount; iCount++)
	{
		myDepthInfo[iCount].depthSwitchInfo = 
			*displaymodeInfo->displayModeDepthBlockInfo->depthVPBlock[iCount].depthSwitchInfo;
		myDepthInfo[iCount].depthVPBlock = 
			*displaymodeInfo->displayModeDepthBlockInfo->depthVPBlock[iCount].depthVPBlock;
	}
}

void GetRequestTheDM2Way (	VideoRequestRecPtr requestRecPtr,
							GDHandle walkDevice,
							DMDisplayModeListIteratorUPP myModeIteratorProc,
							DMListIndexType theDisplayModeCount,
							DMListType *theDisplayModeList)
{
	short					jCount;
	short					kCount;
	ListIteratorDataRec		searchData;

	searchData.depthBlocks = nil;
	// get the mode lists for this GDevice
	for (jCount=0; jCount<theDisplayModeCount; jCount++)		// get info on all the resolution timings
	{
		DMGetIndexedDisplayModeFromList(*theDisplayModeList, jCount, 0, myModeIteratorProc, &searchData);
		
		// for all the depths for this resolution timing (mode)...
		if (searchData.depthBlockCount) for (kCount = 0; kCount < searchData.depthBlockCount; kCount++)
		{
			// only if the mode is valid and is safe or we override it with the kAllValidModesBit request flag
			if	(	searchData.displayModeTimingInfo.csTimingFlags & 1<<kModeValid && 
					(	searchData.displayModeTimingInfo.csTimingFlags & 1<<kModeSafe ||
						requestRecPtr->requestFlags & 1<<kAllValidModesBit
					)
				)
			{
				if (FindBestMatch (	requestRecPtr,
									searchData.depthBlocks[kCount].depthVPBlock.vpPixelSize,
									searchData.depthBlocks[kCount].depthVPBlock.vpBounds.right,
									searchData.depthBlocks[kCount].depthVPBlock.vpBounds.bottom))
				{
					requestRecPtr->screenDevice = walkDevice;
					requestRecPtr->availBitDepth = searchData.depthBlocks[kCount].depthVPBlock.vpPixelSize;
					requestRecPtr->availHorizontal = searchData.depthBlocks[kCount].depthVPBlock.vpBounds.right;
					requestRecPtr->availVertical = searchData.depthBlocks[kCount].depthVPBlock.vpBounds.bottom;
					
					// now set the important info for DM to set the display
					requestRecPtr->depthMode = searchData.depthBlocks[kCount].depthSwitchInfo.csMode;
					requestRecPtr->displayMode = searchData.depthBlocks[kCount].depthSwitchInfo.csData;
					requestRecPtr->switchInfo = searchData.depthBlocks[kCount].depthSwitchInfo;
					if (searchData.displayModeTimingInfo.csTimingFlags & 1<<kModeSafe)
						requestRecPtr->availFlags = 0;							// mode safe
					else requestRecPtr->availFlags = 1<<kModeValidNotSafeBit;	// mode valid but not safe, requires user validation of mode switch
	
				}
			}

		}
	
		if (searchData.depthBlocks)
		{
			DisposePtr ((Ptr)searchData.depthBlocks);	// toss for this timing mode of this gdevice
			searchData.depthBlocks = nil;				// init it just so we know
		}
	}
}

Boolean FindBestMatch (VideoRequestRecPtr requestRecPtr, short bitDepth, unsigned long horizontal, unsigned long vertical)
{
	// �� do the big comparison ��
	// first time only if	(no mode yet) and
	//						(bounds are greater/equal or kMaximizeRes not set) and
	//						(depth is less/equal or kShallowDepth not set) and
	//						(request match or kAbsoluteRequest not set)
	if	(	nil == requestRecPtr->displayMode
			&&
			(	(horizontal >= requestRecPtr->reqHorizontal &&
				vertical >= requestRecPtr->reqVertical)
				||														
				!(requestRecPtr->requestFlags & 1<<kMaximizeResBit)	
			)
			&&
			(	bitDepth <= requestRecPtr->reqBitDepth ||	
				!(requestRecPtr->requestFlags & 1<<kShallowDepthBit)		
			)
			&&
			(	(horizontal == requestRecPtr->reqHorizontal &&	
				vertical == requestRecPtr->reqVertical &&
				bitDepth == requestRecPtr->reqBitDepth)
				||
				!(requestRecPtr->requestFlags & 1<<kAbsoluteRequestBit)	
			)
		)
		{
			// go ahead and set the new values
			return (true);
		}
	else	// can we do better than last time?
	{
		// if	(kBitDepthPriority set and avail not equal req) and
		//		((depth is greater avail and depth is less/equal req) or kShallowDepth not set) and
		//		(avail depth less reqested and new greater avail)
		//		(request match or kAbsoluteRequest not set)
		if	(	(	requestRecPtr->requestFlags & 1<<kBitDepthPriorityBit && 
					requestRecPtr->availBitDepth != requestRecPtr->reqBitDepth
				)
				&&
				(	(	bitDepth > requestRecPtr->availBitDepth &&
						bitDepth <= requestRecPtr->reqBitDepth
					)
					||
					!(requestRecPtr->requestFlags & 1<<kShallowDepthBit)	
				)
				&&
				(	requestRecPtr->availBitDepth < requestRecPtr->reqBitDepth &&
					bitDepth > requestRecPtr->availBitDepth	
				)
				&&
				(	(horizontal == requestRecPtr->reqHorizontal &&	
					vertical == requestRecPtr->reqVertical &&
					bitDepth == requestRecPtr->reqBitDepth)
					||
					!(requestRecPtr->requestFlags & 1<<kAbsoluteRequestBit)	
				)
			)
		{
			// go ahead and set the new values
			return (true);
		}
		else
		{
			// match resolution: minimize �h & �v
			if	(	abs((requestRecPtr->reqHorizontal - horizontal)) <=
					abs((requestRecPtr->reqHorizontal - requestRecPtr->availHorizontal)) &&
					abs((requestRecPtr->reqVertical - vertical)) <=
					abs((requestRecPtr->reqVertical - requestRecPtr->availVertical))
				)
			{
				// now we have a smaller or equal delta
				//	if (h or v greater/equal to request or kMaximizeRes not set) 
				if (	(horizontal >= requestRecPtr->reqHorizontal &&
						vertical >= requestRecPtr->reqVertical)
						||
						!(requestRecPtr->requestFlags & 1<<kMaximizeResBit)
					)
				{
					// if	(depth is equal or kBitDepthPriority not set) and
					//		(depth is less/equal or kShallowDepth not set) and
					//		([h or v not equal] or [avail depth less reqested and new greater avail] or depth equal avail) and
					//		(request match or kAbsoluteRequest not set)
					if	(	(	requestRecPtr->availBitDepth == bitDepth ||			
								!(requestRecPtr->requestFlags & 1<<kBitDepthPriorityBit)
							)
							&&
							(	bitDepth <= requestRecPtr->reqBitDepth ||	
								!(requestRecPtr->requestFlags & 1<<kShallowDepthBit)		
							)
							&&
							(	(requestRecPtr->availHorizontal != horizontal ||
								requestRecPtr->availVertical != vertical)
								||
								(requestRecPtr->availBitDepth < requestRecPtr->reqBitDepth &&
								bitDepth > requestRecPtr->availBitDepth)
								||
								(bitDepth == requestRecPtr->reqBitDepth)
							)
							&&
							(	(horizontal == requestRecPtr->reqHorizontal &&	
								vertical == requestRecPtr->reqVertical &&
								bitDepth == requestRecPtr->reqBitDepth)
								||
								!(requestRecPtr->requestFlags & 1<<kAbsoluteRequestBit)	
							)
						)
					{
						// go ahead and set the new values
						return (true);
					}
				}
			}
		}
	}
	return (false);
}
  
  

#if !TARGET_API_MAC_CARBON
Boolean IsKeyDown(short n)
 {
   KeyMap theKeys;
   GetKeys(theKeys);
   return ((theKeys[n>>3]  >>  (n & 7)) & 1);
 }

void ADBIOCompletionPPC(Byte *dataBufPtr, Byte *opDataPtr, long command) {
	*opDataPtr = true;
}
  
 
void SetupKeyboard(void) {
 	ADBAddress     currentDev;
 	ADBDataBlock   info;
 	short       data = 0,number;
 	Byte        buffer[3], ADBReg;
 	short       talk, listen,i;
 	OSErr       myErr;
    ADBCompletionUPP  compProcPtr;      // PPC completion routine 
    ADBDataBlock    adbData;
    
    return;
    
    number = CountADBs();
    for(i=1;i<=number;i++) {
        currentDev = GetIndADB(&info, i);
        if (currentDev < 0)
           return;
           
        myErr = GetADBInfo(&adbData, currentDev);
        if (!(adbData.origADBAddr == 2))
                continue;
                        
        buffer[0] = 2;             // length byte
        buffer[1] = 0;
        buffer[2] = 0;
        ADBReg = 3;                // get register 3
     
        talk = (currentDev << 4) + 0x0C + ADBReg;    
        compProcPtr = NewADBCompletionProc(ADBIOCompletionPPC);
     
        myErr = ADBOp((Ptr)&data, (ADBCompletionUPP)compProcPtr, (Ptr)buffer, talk);
        if (myErr != noErr) 
           goto done;
     
        while(!data); 
        
             
    	data = 0;
        buffer[2] = 3; // change from 2 to 3 so we can differentiate between left and right shift keys
        listen = (currentDev << 4) + 0x08 + ADBReg; 
     
        myErr = ADBOp((Ptr)&data, (ADBCompletionUPP)compProcPtr, (Ptr)buffer, listen);
        while(!data); 
    }
done:  
    DisposeRoutineDescriptor(compProcPtr);
}
#endif