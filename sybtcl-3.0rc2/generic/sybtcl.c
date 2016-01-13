/*
 * sybtcl.c --
 *
 * Sybase interface to Tcl
 *
 * Copyright 1992 Tom Poindexter and U S WEST Enhanced Services, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  
 * Tom Poindexter and U S WEST make no representations about the suitability 
 * of this software for any purpose.  It is provided "as is" without express or
 * implied warranty.  By use of this software the user agrees to 
 * indemnify and hold harmless Tom Poindexter and U S WEST from any 
 * claims or liability for loss arising out of such use.
 *
 *-----------------------------------------------------------------------------
 * Version 1.0 June, 1992
 * Tom Poindexter, Denver Colorado
 * tpoindex@nyx.cs.du.edu   
 *-----------------------------------------------------------------------------
 * Version 1.1 August, 1992
 * Tom Poindexter
 * tpoindex@nyx.cs.du.edu   
 *  -added "maxtext" index in sybmsg, limits amount of a returned text field
 *   in normal select 
 *  -added sybwritetext, sybreadtext, read/write text or image to/from files
 *-----------------------------------------------------------------------------
 * Version 1.2 October, 1992
 * Tom Poindexter
 * tpoindex@nyx.cs.du.edu   
 *  -move dbsetopt() that was in parse_column,Tcl_SybWrtext, & Tcl_SybRdtext
 *   to Tcl_SybSql, just before the dbsqlexec.  version 1.1 introduced a bug
 *   that would sometimes confuse the sql server by bombarding it with
 *   dbsetopt().
 *   (I should read the dbsetopt man page more carefully in the future)
 *   update man page concerning sybmsg(maxtext) and sybreadtext to
 *   match the code
 *  -append several sybmsg(msgno) the way sybmsg(msgtext) is done
 *-----------------------------------------------------------------------------
 * Version 1.3 May, 1993
 * Tom Poindexter
 * tpoind@advtech.uswest.com  or  tpoindex@nyx.cs.du.edu
 * -fix null pointers that appear on some db-libs in syb_tcl_err_handler
 * -fix strncpy in Tcl_SybSql, now just strcpy
 * -fix wrong buf in parse_columns, now buf2 for nullvalue
 *-----------------------------------------------------------------------------
 * Version 2.0, November, 1993
 * Tom Poindexter
 * tpoind@advtech.uswest.com  or  tpoindex@nyx.cs.du.edu
 * -support for Tcl 7.0, a few function calls have changed 
 * -consolidate .h includes to just tcl.h & tclUnix.h (not TclX's tclExtdInt.h)
 * -add ?appname? optional argument in sybconnect
 * -size message buffer dynamically when getting server messages
 * -change init function name to be more in line with de facto standards
 * -change malloc/free to ckalloc/ckfree in case TCL_MEM_DEBUG set
 *-----------------------------------------------------------------------------
 * Version 2.1, February, 1994
 * Tom Poindexter
 * tpoind@advtech.uswest.com  or  tpoindex@nyx.cs.du.edu
 * -add commands option to sybnext, hacked from Oratcl 2.1
 * -change Sybtcl_Init to (int)
 *-----------------------------------------------------------------------------
 * Version 2.11, April, 1994
 * Tom Poindexter
 * tpoind@advtech.uswest.com  or  tpoindex@nyx.cs.du.edu
 * -allow sybcols to return column names even if result set is empty
 *  (thanks Christopher Hall)
 *-----------------------------------------------------------------------------
 * Version 2.2, October, 1994
 * Tom Poindexter
 * tpoindex@nyx.cs.du.edu
 * -clean up rest of external function names (Tcl*** -> Sybtcl***)
 * -add sybase date formatting option  (thanks Ken Corey)
 * -allow numeric columns to have an actual null output (""), 
 *  (thanks Ken Corey) 
 *-----------------------------------------------------------------------------
 * Version 2.3, August, 1995
 * Tom Poindexter
 * tpoindex@nyx.cs.du.edu
 * -change behavior of sybnext when dbresults returns FAIL (no error)
 * -change behavior of sybnext with tcl-commands to break on compute rows
 *  buffering the compute row for the next sybnext
 * -change parse_columns to recognize float, and print in user precision
 * -added sybmsg(floatprec) to support float precisions
 * -added "async" option on sybsql, and sybpoll command to check async procs
 * -allow sybcancel to cancel anytime, not just on previous success 
 * -allow optional interfaces file parameter ?ifile? on sybconnect 
 * -added dbsetversion to get system 10 types reported correctly
 * -added some casts to make gcc happy
 * -small changes for Tcl 7.4 (remove need for tclUnix.h/tclPort.h)
 *-----------------------------------------------------------------------------
 * Version 2.4, December, 1996
 * Tom Poindexter
 * tpoindex@nyx.net   !! note !! new address
 * -execute dbsetopt in sybsql before actual sql to avoid confusing rep server
 *   & only execute dbsetopt if maxtext has changed
 * -malloc buffer for msgtext in msg_handler, so that it can grow
 * -added "fixedchar" index to sybmsg so that trailing spaces won't be trimmed
 * -include call to Tcl_PkgProvide for tcl7.5+ package scheme
 * -added another check in NextAll to prevent going past columns (from oratcl.c)
 * -make floats really look like floats, add .0 if needed 
 * -fix sybnextall to quote two occurances of substitution char, and to
 *  accept a user defined substitution char
 * -add sybtclInitFlag to prevent re-initializing when loaded at static 
 *-----------------------------------------------------------------------------
 * Version 2.5, November, 1997
 * Tom Poindexter
 * tpoindex@nyx.net
 * -add #ifdefs for window compilation, includes .dll entry point function
 * -add prototype cast for dberrhandle/dbmsghandle calls
 * -use Tcl_DStrings in msg handler, thanks to D. J. Hagberg (dhagberg@gds.com)
 * -change Wrtext some to accomodate dbtoct emulation layer and to enable
 *  write of null files (len = 0)
 * -add #ifdefs and various cast for mac compile, thanks to Scott Kelley 
 *  <sakelley@jeeves.ucsd.edu>
 * -add version number to sybmsg array
 * -change Sybtcl_NextAll to bind tcl variables to output columns per fetch
 * -change all keywords (async,nolog, etc.) to options (-async,-nolog)
 * -increase size of max procs to 50
 * -call Sybtcl_Kill as an exit handler, not on any command deletion
 * -changed Sybtcl_Kill to close each connection, not call dbexit() if Windows
 * -fix bug in NextAll that causes problems in 8.0, now copy commands string
 *  to a private copy.
 *-----------------------------------------------------------------------------
 * Version 3.0, April 1999
 * Tom Poindexter
 * tpoindex@nyx.net
 * -change MS VC defines for syb_tcl_err_handler & syb_tcl_msg_handler, thanks
 *  to Jeff Jacobi <jeff.jacobi@lmco.com>.
 * -change all Tcl interfaces to use Tcl 8.0 objects, use objects everywhere
 * -rewrote parse_columns() to return elements as reasonable tcl obj types
 * -SYBIMAGE and SYBBINARY data now returned bit for bit, no conversion to 
 *  hex strings.
 * -rewrote NextAll processing to use objs to build command string 
 * -NextAll varname column pairs can now be a list of varname column pairs
 * -sybmsg(floatprec) is deprecated, in favor of tcl_precision
 * -add charset argument on sybconnect
 * -new commands: syberrhandler & sybmsghandler, call tcl proc for err & msg
 * -new command: sybmoney, to manipulate sybase money values
 * -error and msg handlers always set sybmsg & call handlers, even without
 *  a valid dbproc 
 * -borrowed (stoled) utility function from TclX -thanks Mark & Karl!
 * -add -file, -variable  options for sybreadtext/sybwritetext commands
 * -change sybreadtext/sybwritetext to use Tcl_xxx channel functions instead
 *  of raw open/close/read/write/stat; mac gets filesize via Tcl_Seek()-ugh!
 * -add Sybtcl_SafeInit() initialization point, file read/write disabled in
 *  sybreadtext/sybwritetext when running in safe interp
 * -add Sybtcl_DeleteInterp to clean up dbprocs left open 
 * -add sybmsg(dblibinfo), "system10" if compiled with system 10 or higher libs
 * -trace all user options, mirror in per-interp struct for faster access
 * -add sybmsg(binaryashex), convert image/binary to hexstrings,  2.5 compat
 * -add sybmsg(isnull), reflects true null values in returned row
 * -add extra col_len2 to parse_column to pass max length (for fixedchar)
 * -add sybmsg(maxhandles), sybmsg(usedhandles)
 * -fix bug in sybclose where last_text was not reset-thanks Ben Vitale
 * -add 'sybevent' command based on code from Ben Vitale, non-unix versions
 *  use event source, mostly borrowed from tclWinChan.c
 * -add sybmsg(bgevents) to process events during sybnext/w commands,
 *  sybreadtext, & sybwritetext; sybmsg(bgpollinterval) set poll timeout ms
 * -simplify windows dll export to use Tcl 8.0.3+ EXTERN
 * -fix bug in sybpoll where previous results may yield a ready proc by
 *  canceling results in sybsql (cancel_prev)
 * -make dbclose() perform one last sql if mactinosh (see ./mac/README.mac)
 * -make macintosh do less events_waiting
 *
 * -added Stub enabling code. Reversed includes of tcl,h and windows.h
 *  because of compilation issues.
 *
 * RCS: @(#) $Id: sybtcl.c,v 1.5 2000/08/31 00:24:18 mtariq Exp $
 */


#define SYBTCL_VERSION "3.0"

#if defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif

#include "tcl.h"


/* simple memory debugging, to make sure we alloc and free the same amount */
#ifdef SYBTCL_MEM_DEBUG
#include <stdio.h>
#undef ckalloc
#undef ckfree
static long my_alloc = 0; /* alloc'ed in this module, free'ed in this module */
static long my_tcl_alloc= 0;    /* alloc'ed by tcl, free'ed in this module */
static long my_free  = 0;       /* free'ed in this module */
FILE *my_file;          /* file to dump stats */
#ifdef SYBTCL_MEM_FULL_DEBUG
#define WHERE_ALLOC fprintf(my_file,"ckalloc  %d\n",__LINE__),
#define WHERE_FREE  fprintf(my_file,"ckfree   %d\n",__LINE__),
#else
#define WHERE_ALLOC
#define WHERE_FREE
#endif
#define ckalloc(x) (WHERE_ALLOC my_alloc++, Tcl_Alloc(x))
#define ckfree(x)  (WHERE_FREE  my_free++,  Tcl_Free(x))
#define MY_STATS_INIT my_file=fopen("alloc.stats","w")
#define MY_STATS_DUMP fprintf(my_file,\
  "ckalloc   = %8ld\ntcl alloc = %8ld\nall alloc = %8ld\nckfree    = %8ld\n",\
  my_alloc,my_tcl_alloc,my_alloc+my_tcl_alloc,my_free);fclose(my_file)
#else
static long my_alloc = 0;
#define MY_STATS_INIT my_alloc=0
#define MY_STATS_DUMP my_alloc=my_alloc
#endif


#include <fcntl.h>

#ifdef MAC_TCL
#define NO_BCOPY
#define NO_STRING_H
#define LARGE_CHAR static char
#define CALLBACK_SCOPE /* mac needs this as an extern */
#include <stdlib.h>
#else
#include <sys/stat.h>
#define LARGE_CHAR char
#define CALLBACK_SCOPE static
#endif

#include <errno.h>

#include <sybfront.h>
#include <sybdb.h>
#include <syberror.h>

#ifdef NO_STRING_H
#include <strings.h>
#else
#include <string.h>
#endif

#include <ctype.h>

#if defined(__STDC__) || defined(HAS_STDARG) 
#   include <stdarg.h>
#else
#   include <varargs.h>
#endif

#if defined(WIN32) || defined(_WIN32)
#ifndef __WIN32__
#define __WIN32__
#endif
#endif

#ifdef __WIN32__
#include <memory.h>
#include <stdlib.h>
#include <io.h>
#endif

#ifdef CTCOMPATLIB
#ifndef DBVERSION_100
#define DBVERSION_100
#endif
#endif

/* special last_result code for async results pending */
#define PENDING -9

/* default time for events_waiting polling */
#define POLL_INTERVAL  200

/* default state for bgevents: 0 = none, 1 = idletasks, 2 = all */
#define BG_STATE  1

#ifdef NO_BCOPY
#define bcopy(from, to, length)    memmove((to), (from), (length))
#endif

#if defined(MAC_TCL) || defined(__WIN32__)
#define POLLED_EVENTS
#endif

typedef struct SybTclProcs {	/* struct for handle entries */
    int         in_use;		/* if this entry is opened */
    Tcl_Interp *interp;         /* the interp where this proc is used */
    DBPROCESS  *dbproc;		/* dbproc pointer for this entry */
    RETCODE     last_results;	/* return code from last dbresults() */
    RETCODE     last_next;	/* return code from last dbnextrow() */
    Tcl_Obj    *bufferedResult; /* buffered row obj when crossing result sets */
    Tcl_Obj    *bufferedIsnull; /* buffered isnull indicator */
    int         async; 		/* last sql was async */
    long        last_text;      /* last maxtext size sent */
    Tcl_Obj    *callBackScript; /* callback script when using sybevent */
    Tcl_Channel sybChan;        /* channel form of dblib read socket */
    int         in_event;	/* if a callback event is currently active */
    int         hasBgResults;   /* if dbresults called during event handler */
    RETCODE     bgResults;      /* dbresults during event handler */
} SybTclProcs;

typedef struct SybTclOptions {	/* struct for per interpreter options */
    Tcl_Interp *interp;         /* interpreter to which these options apply */
    Tcl_Obj    *sybMsgCmd;      /* command to execute for msg handler */
    Tcl_Obj    *sybErrCmd;      /* command to execute for err handler */
    char       *nullvalue;      /* value used to represent null value */
    char       *dateformat;     /* date formatting string */
    long        maxtext;        /* maximum text/image lenght retrieved */
    int         floatprec;      /* if gt 0, convert float with this precision */
    int         fixedchar;      /* blank fill char(x) types or not */
    int         binaryashex;    /* format binary/image as hex strings or not */
    int         bgevents;       /* if 1, process events while waiting */
    int         bgpollinterval; /* timeout for bgevent polling */
} SybTclOptions;

#define SYBTCLPROCS       50	/* default number of dbprocs available */
#define SYB_BUFF_SIZE	4096	/* conversion buffer size for various needs*/

/* defines for text/image handling - first is our buffer size;other is max */
/* use LARGE_CHAR macro to allocate TEXT_BUFF_SIZE in functions */
#define   TEXT_BUFF_SIZE   32768
#define   MAX_SERVER_TEXT  "2147483647"
#define   MAX_SERVER_TEXTI  2147483647 

/* for precision and scale conversion of SYBDECIMAL and SYBNUMERIC types */
#define   SYBTCL_NUM_PRECISION 18
#define   SYBTCL_NUM_SCALE      0
#define   SYBTCL_MAX_PRECISION 77
#define   SYBTCL_MAX_SCALE     77

static SybTclProcs   SybProcs[SYBTCLPROCS];  
static SybTclOptions SybOptions[SYBTCLPROCS];

static char *SybHandlePrefix = "sybtcl";  /* prefix used to identify handles*/

#define CMD_STR Tcl_GetStringFromObj(objv[0],NULL)

/* tcl objs for sybmsg array name and all indices */
static Tcl_Obj *SybMsgArray;
static Tcl_Obj *BoolTrue;
static Tcl_Obj *BoolFalse;
static Tcl_Obj *SM_handle;
static Tcl_Obj *SM_msgno;
static Tcl_Obj *SM_msgstate;
static Tcl_Obj *SM_msgtext;
static Tcl_Obj *SM_severity;
static Tcl_Obj *SM_svrname;
static Tcl_Obj *SM_procname;
static Tcl_Obj *SM_line;
static Tcl_Obj *SM_dberr;
static Tcl_Obj *SM_oserr;
static Tcl_Obj *SM_dberrstr;
static Tcl_Obj *SM_oserrstr;
static Tcl_Obj *SM_nextrow;
static Tcl_Obj *SM_retstatus;
static Tcl_Obj *SM_collengths;
static Tcl_Obj *SM_coltypes;
static Tcl_Obj *SM_version;
static Tcl_Obj *SM_handle;
static Tcl_Obj *SM_nullvalue;
static Tcl_Obj *SM_floatprec;
static Tcl_Obj *SM_maxtext;
static Tcl_Obj *SM_dateformat;
static Tcl_Obj *SM_fixedchar;
static Tcl_Obj *SM_dblibinfo;
static Tcl_Obj *SM_binaryashex;
static Tcl_Obj *SM_bgevents;
static Tcl_Obj *SM_bgpollinterval;
static Tcl_Obj *SM_isnull;
static Tcl_Obj *SM_maxhandles;
static Tcl_Obj *SM_usedhandles;


static Tcl_Interp *SybInterp;	    /* interpreter access in err&msg handler*/


/* windows: dll entry point function and export symbols*/

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
 
#undef  TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS  DLLEXPORT
 
BOOL APIENTRY
DllMain(hInst, reason, reserved)
    HINSTANCE hInst;            /* Library instance handle. */
    DWORD reason;               /* Reason this function is being called. */
    LPVOID reserved;            /* Not used. */
{
    return TRUE;
}
 

#else

extern void * malloc();

#endif


/* debugging 'puts' to work on all platforms, esp. Mac where I don't have */
/* a good debugger under MPW  :( */
#define sybdebug(msg) Tcl_Write(Tcl_GetStdChannel(TCL_STDERR),msg,strlen(msg))


#ifdef MAC_TCL

/* for dbclose(), make mac do one last sql to leave dbproc in a good state, */
/* especially if sybevent were used */

static int
syb_dbclose(dbproc)
    DBPROCESS *dbproc;
{
    dbcmd(dbproc, "select 1");
    dbsqlexec(dbproc);
    dbresults(dbproc);
    dbclose(dbproc);
}

#else

/* unix and windowes just call dbclose() */
#define syb_dbclose(dbproc)  dbclose(dbproc)

#endif


/* prototypes for all tcl command functions */

EXTERN Sybtcl_Init      _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN Sybtcl_SafeInit  _ANSI_ARGS_((Tcl_Interp *interp));

extern Tcl_ObjCmdProc  Sybtcl_Connect;
extern Tcl_ObjCmdProc  Sybtcl_ErrHandler;
extern Tcl_ObjCmdProc  Sybtcl_MsgHandler;
extern Tcl_ObjCmdProc  Sybtcl_Money;
extern Tcl_ObjCmdProc  Sybtcl_Use;
extern Tcl_ObjCmdProc  Sybtcl_Sql;
extern Tcl_ObjCmdProc  Sybtcl_Poll;
extern Tcl_ObjCmdProc  Sybtcl_Next;
extern Tcl_ObjCmdProc  Sybtcl_Cols;
extern Tcl_ObjCmdProc  Sybtcl_Cancel;
extern Tcl_ObjCmdProc  Sybtcl_Close;
extern Tcl_ObjCmdProc  Sybtcl_Retval;
extern Tcl_ObjCmdProc  Sybtcl_Wrtext;
extern Tcl_ObjCmdProc  Sybtcl_Rdtext;
extern Tcl_ObjCmdProc  Sybtcl_Event;


/* support for sybevent - define our own channel type */

static int   dbNoInputProc _ANSI_ARGS_((ClientData instanceData,
                            char *buf, int toRead, int *errorCode));
static int   dbNoOutputProc _ANSI_ARGS_((
                            ClientData instanceData, char *buf, int toWrite,
							int *errorCode));
static void  dbWatchProc _ANSI_ARGS_((ClientData instanceData, int mask));
static int   dbGetHandleProc _ANSI_ARGS_((ClientData instanceData,
			int direction, ClientData *handlePtr));
static int   dbCloseProc _ANSI_ARGS_((ClientData instanceData, 
			Tcl_Interp *interp));

static int dbEventProc _ANSI_ARGS_((Tcl_Event *evPtr, int flags));
static void dbSetupProc _ANSI_ARGS_((ClientData cd_hand, int flags));
static void dbCheckProc _ANSI_ARGS_((ClientData cd_hand, int flags));
void Sybtcl_Kill _ANSI_ARGS_((ClientData clientData));
static int CS_INTERNAL syb_tcl_err_handler _ANSI_ARGS_((
	DBPROCESS *db_proc,
    int      severity,
    int       dberr,
    int       oserr,
    char      *dberrstr,
    char      *oserrstr
    ));

static Tcl_ChannelType dbChannelType = {
    "sybtcl",
    NULL,
    dbCloseProc,
    (Tcl_DriverInputProc *)  dbNoInputProc,
    (Tcl_DriverOutputProc *) dbNoOutputProc,
    NULL,
    NULL,
    NULL,
    dbWatchProc,
    dbGetHandleProc,
};

typedef struct dbState {
    Tcl_Channel channel;        /* Channel associated with this file. */
    int fd;			/* sock from DBIORDESC */
    int hand;			/* the related SybProc entry */
} dbState;




/*
 *----------------------------------------------------------------------
 * dbSetupProc, dbCheckProc, dbEventProc
 *
 * for non-unix, we need to poll for events to make sybevent work 
 *
 */

typedef struct dbEvent {
    Tcl_Event header;
    int hand;
} dbEvent;

static void
dbSetupProc(cd_hand, flags)
    ClientData cd_hand;
    int        flags;
{
    int hand = (int) cd_hand;
    Tcl_Time blockTime = { 0, 50 };

    if (!(flags & TCL_FILE_EVENTS) || SybProcs[hand].in_use == 0) {
	return;
    }
    Tcl_SetMaxBlockTime(&blockTime);
}

static int
dbEventProc(evPtr, flags)
    Tcl_Event *evPtr;     
    int flags; 
{
    dbEvent *dbEvPtr = (dbEvent *)evPtr;
    int      hand    = dbEvPtr->hand;

    if (!(flags & TCL_FILE_EVENTS) || SybProcs[hand].in_use == 0) {
	return 0;
    }
    Tcl_NotifyChannel(SybProcs[hand].sybChan, TCL_READABLE);
    return 1;
}

static void
dbCheckProc(cd_hand, flags)
    ClientData cd_hand;
    int        flags;
{
    int hand = (int) cd_hand;
    DBPROCESS  *readyproc;
    RETCODE     dbret;
    int         reason;
    dbEvent    *evPtr;

    if (SybProcs[hand].in_use == 0 ||
	SybProcs[hand].in_event == 1 ||
	SybProcs[hand].last_results == NO_MORE_RESULTS) {
	return;
    }

    dbret = dbpoll(SybProcs[hand].dbproc, 0L, &readyproc, &reason);

    if (dbret == SUCCEED && readyproc == SybProcs[hand].dbproc &&
	 reason == DBRESULT) {
	
	evPtr = (dbEvent *) ckalloc(sizeof(dbEvent));
	evPtr->header.proc = dbEventProc;
	evPtr->hand = hand;
	Tcl_QueueEvent((Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
    }
}




/*-----------------------------------------------------------------------------
 * dbNoInputProc,dbNoOutputProc - just return 
 *
 */

static int
dbNoInputProc (instanceData, buf, toRead, errorCodePtr)
    ClientData instanceData;            
    char *buf; 
    int toRead;
    int *errorCodePtr; 

{
    return toRead;
}

static int
dbNoOutputProc (instanceData, buf, toWrite, errorCodePtr)
    ClientData instanceData;            
    char *buf; 
    int toWrite;
    int *errorCodePtr; 

{
    return toWrite;
}


/*-----------------------------------------------------------------------------
 * dbWatchProc - add sock to notifier
 *
 */

static void
dbWatchProc (instanceData, mask)
    ClientData instanceData;
    int mask;    
{
    dbState *statePtr = (dbState *) instanceData;
    int hand = statePtr->hand;

#ifdef POLLED_EVENTS
    /* events are enabled in Sybtcl_Event() */ 
    return;

#else
    if (mask) {
	Tcl_CreateFileHandler(statePtr->fd, mask,
		(Tcl_FileProc *) Tcl_NotifyChannel,
		(ClientData) statePtr->channel);
    } else {
	Tcl_DeleteFileHandler(statePtr->fd);
    }
#endif

}


/*-----------------------------------------------------------------------------
 * dbGetHandleProc - return sock
 *
 */

static int
dbGetHandleProc(instanceData, direction, handlePtr)
    ClientData instanceData;    
    int direction;             
    ClientData *handlePtr;    
{       
    dbState *statePtr = (dbState *) instanceData;

    *handlePtr = (ClientData)statePtr->fd; 
    return TCL_OK;                      
}   

/*-----------------------------------------------------------------------------
 * dbCloseProc - clean up stateptr
 *  
 */ 
    
static int
dbCloseProc(instanceData, interp)
    ClientData instanceData; 
    Tcl_Interp *interp;     
{
    dbState *statePtr = (dbState *) instanceData;

    ckfree((void*) statePtr);
    return 0;
}

Tcl_Channel
dbMakeChannel (sock, hand, channelName)
    int sock;
    int hand;
    char *channelName;
{
    dbState *statePtr;

    statePtr = (dbState *) ckalloc(sizeof(dbState));
    statePtr->fd =  sock;
    statePtr->hand = hand;
    statePtr->channel = Tcl_CreateChannel(&dbChannelType, channelName,
            (ClientData) statePtr, (TCL_READABLE));
    return statePtr->channel;
}


/*-----------------------------------------------------------------------------
 * Sybtcl_AppendObjResult -- stolen from TclX
 * TclX_AppendObjResult --
 *
 *   Append a variable number of strings onto the object result already
 * present for an interpreter.
 *
 * Parameters:
 *   o interp - Interpreter to set the result in.
 *   o args - Strings to append, terminated by a NULL.
 *-----------------------------------------------------------------------------
 */
static void Sybtcl_AppendObjResult 
			_ANSI_ARGS_(TCL_VARARGS_DEF (Tcl_Interp *,arg1));

static void
Sybtcl_AppendObjResult TCL_VARARGS_DEF (Tcl_Interp *, arg1)
{
    Tcl_Interp *interp;
    Tcl_Obj *resultPtr;
    va_list argList;
    char *string;
 
    interp = TCL_VARARGS_START (Tcl_Interp *, arg1, argList);
    resultPtr = Tcl_GetObjResult (interp);
 
    TCL_VARARGS_START(Tcl_Interp *,arg1,argList);
    while (1) {
        string = va_arg(argList, char *);
        if (string == NULL) {
            break;
        }
        Tcl_AppendToObj (resultPtr, string, -1);
    }
    va_end(argList);
}


/* 
 *----------------------------------------------------------------------
 * count_inuse
 *    count the number of handles in use and set sybmsg(usedhandles)
 */

static void
count_inuse (interp) 
    Tcl_Interp *interp;
{
    int i; 
    int used = 0;
    Tcl_Obj *tmp_obj;

    for (i = 0; i < SYBTCLPROCS; i++) {
        if (SybProcs[i].in_use) {
	    used++;
	}
    }
    tmp_obj = Tcl_NewIntObj(used);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray,SM_usedhandles,tmp_obj,TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
}


/* 
 *----------------------------------------------------------------------
 * get_syb_handle
 *    authenticate a handle string 
 *  return: SybProcs index number or -1 on error;
 */

static int
get_syb_handle (handle) 
    char *handle;
{
    unsigned int prefix_len = strlen(SybHandlePrefix);
    int h;

    if ( (strlen(handle) > prefix_len) &&
	 (strncmp(handle,SybHandlePrefix,prefix_len) == 0)  &&
	 (isdigit(*(handle + prefix_len))) ) {

	 h = atoi((handle + prefix_len));
	 if (h < 0 || h >= SYBTCLPROCS) {
	     return (-1);
	 }
	 if (SybProcs[h].in_use) {
	     return (h);
	 } else {
	     return (-1);
	 }
    } 

    return (-1);
}


/*
 *----------------------------------------------------------------------
 * get_syb_option
 *   get the SybOptions index for this interp
 */

static int
get_syb_option (interp)
    Tcl_Interp *interp;
{
    int i;
    for (i = 0; i < SYBTCLPROCS; i++) {
        if (SybOptions[i].interp == interp) {
            return (i);
        }
    }
    return (-1);
}


/*
 *----------------------------------------------------------------------
 * syb_tcl_err_handler
 *   deal with dblib (nee open client) errors
 */

#ifdef __WIN32__
static int CS_INTERNAL
#else
static int
#endif
syb_tcl_err_handler (db_proc, severity, dberr, oserr, dberrstr, oserrstr)
    DBPROCESS *db_proc;
    int        severity;
    int        dberr;
    int        oserr;
    char      *dberrstr;
    char      *oserrstr;
{

    int i;
    char buf[SYB_BUFF_SIZE];
    Tcl_Obj *hand_obj;
    Tcl_Obj *severity_obj;
    Tcl_Obj *dberr_obj;
    Tcl_Obj *oserr_obj;
    Tcl_Obj *dberrstr_obj;
    Tcl_Obj *oserrstr_obj;

    Tcl_Obj *callback_cmd;
    Tcl_Obj *save_result;
    int result;
    int sybase_ret = INT_CANCEL;
    Tcl_Interp *sybInterp;

    
    strcpy(buf,"");
    sybInterp = SybInterp;

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybProcs[i].dbproc == db_proc && SybProcs[i].in_use) {

            sprintf(buf,"%s%d",	SybHandlePrefix, i);
	    sybInterp = SybProcs[i].interp;
	    break;
	}
    }

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybOptions[i].interp == sybInterp) {
	    break;
	}
    }
    if (i >= SYBTCLPROCS) {
	return 0;		/* a big oops if we get here */
    }

    hand_obj = Tcl_NewStringObj(buf,-1);
    Tcl_IncrRefCount(hand_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_handle, hand_obj,TCL_GLOBAL_ONLY);

    severity_obj = Tcl_NewIntObj(severity);
    Tcl_IncrRefCount(severity_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_severity, severity_obj, 
							TCL_GLOBAL_ONLY);

    dberr_obj = Tcl_NewIntObj(dberr);
    Tcl_IncrRefCount(dberr_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_dberr, dberr_obj,TCL_GLOBAL_ONLY);

    oserr_obj = Tcl_NewIntObj(oserr);
    Tcl_IncrRefCount(oserr_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_oserr, oserr_obj,TCL_GLOBAL_ONLY);
    
    dberrstr_obj = Tcl_NewStringObj((dberrstr == NULL) ? "" : dberrstr, -1);
    Tcl_IncrRefCount(dberrstr_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_dberrstr, dberrstr_obj, 
							TCL_GLOBAL_ONLY);

    oserrstr_obj = Tcl_NewStringObj((oserrstr == NULL) ? "": oserrstr, -1);
    Tcl_IncrRefCount(oserrstr_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_oserrstr, oserrstr_obj, 
							TCL_GLOBAL_ONLY);
    

    /* call Tcl msg handler callback */
    if (strlen(Tcl_GetStringFromObj(SybOptions[i].sybErrCmd,NULL)) > 0) {
	callback_cmd = Tcl_DuplicateObj(SybOptions[i].sybErrCmd);
	Tcl_IncrRefCount(callback_cmd);
	Tcl_AppendToObj(callback_cmd, " ", 1);  

	/* construct callback command as list elements */
	Tcl_ListObjAppendElement (NULL, callback_cmd, hand_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, severity_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, dberr_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, oserr_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, dberrstr_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, oserrstr_obj);

	Tcl_AppendToObj(callback_cmd, " ;", 2);  /* force back into string */

	/* save existing result, eval callback command */
	save_result = Tcl_GetObjResult(sybInterp);
	Tcl_IncrRefCount(save_result);

	Tcl_Preserve((ClientData) sybInterp);
	result = Tcl_GlobalEvalObj(sybInterp, callback_cmd);
	if (result != TCL_OK) {
	    Tcl_AddErrorInfo(sybInterp,
				"\n    (\"syberrhandler\" script)");
	    Tcl_BackgroundError(sybInterp);
	} else {
	    /* get handler return code, if any */
	    if (Tcl_GetIntFromObj(sybInterp, Tcl_GetObjResult(sybInterp),
							&result) == TCL_OK) {

		switch (result) {
		    case INT_EXIT:
		    case INT_CONTINUE:
		    case INT_CANCEL:
		    case INT_TIMEOUT:
			sybase_ret = result;
			break;
		    default:
			break;
		}
	    }
	}
	Tcl_Release((ClientData) sybInterp);
	Tcl_SetObjResult(sybInterp, save_result);

	Tcl_DecrRefCount(callback_cmd);
	Tcl_DecrRefCount(save_result);
    }

    Tcl_DecrRefCount(hand_obj);
    Tcl_DecrRefCount(severity_obj);
    Tcl_DecrRefCount(dberr_obj);
    Tcl_DecrRefCount(oserr_obj);
    Tcl_DecrRefCount(dberrstr_obj);
    Tcl_DecrRefCount(oserrstr_obj);

    return (sybase_ret);
}



/*
 *----------------------------------------------------------------------
 * syb_tcl_msg_handler
 *   deal with server messages
 */

#ifdef __WIN32__
static int CS_INTERNAL
#else
static int
#endif
syb_tcl_msg_handler (db_proc, msgno, msgstate, severity, msgtext, svrname,
		     procname, line)
    DBPROCESS *db_proc;
    DBINT      msgno;
    int        msgstate;
    int        severity;
    char      *msgtext;
    char      *svrname;
    char      *procname;
    DBUSMALLINT line;
{

    int       i;
    char      buf[SYB_BUFF_SIZE];
    char     *p;
    char      conv_buf[20];

    Tcl_Obj *hand_obj;
    Tcl_Obj *msgno_obj;
    Tcl_Obj *msgstate_obj;
    Tcl_Obj *severity_obj;
    Tcl_Obj *msgtext_obj;
    Tcl_Obj *svrname_obj;
    Tcl_Obj *procname_obj;
    Tcl_Obj *line_obj;

    Tcl_Obj *tmp_obj;
    Tcl_Obj *old_obj;

    Tcl_Obj *callback_cmd;
    Tcl_Obj *save_result;
    int strLen;
    int result;
    Tcl_Interp *sybInterp;



    strcpy(buf,"");
    sybInterp = SybInterp;  

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybProcs[i].dbproc == db_proc && SybProcs[i].in_use) {

            sprintf(buf,"%s%d",	SybHandlePrefix, i);
	    sybInterp = SybProcs[i].interp;
	    break;
	}
    }

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybOptions[i].interp == sybInterp) {
	    break;
	}
    }
    if (i >= SYBTCLPROCS) {
	return 0;		/* a big oops if we get here */
    }

    hand_obj = Tcl_NewStringObj(buf,-1);
    Tcl_IncrRefCount(hand_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_handle, hand_obj,TCL_GLOBAL_ONLY);

    msgstate_obj = Tcl_NewIntObj(msgstate);
    Tcl_IncrRefCount(msgstate_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_msgstate, msgstate_obj,
							TCL_GLOBAL_ONLY);

    severity_obj = Tcl_NewIntObj(severity);
    Tcl_IncrRefCount(severity_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_severity, severity_obj,
							TCL_GLOBAL_ONLY);

    svrname_obj = Tcl_NewStringObj(svrname,-1);
    Tcl_IncrRefCount(svrname_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_svrname, svrname_obj,
							TCL_GLOBAL_ONLY);

    procname_obj = Tcl_NewStringObj(procname,-1);
    Tcl_IncrRefCount(procname_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_procname, procname_obj,
							TCL_GLOBAL_ONLY);

    line_obj = Tcl_NewIntObj(line);
    Tcl_IncrRefCount(line_obj);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_line, line_obj,TCL_GLOBAL_ONLY);

    /* it's possible that several messages may accumulate in one */
    /* sybtcl command, due to serveral dblib routines being called */
    /* append all msgno's & msgtext's together with newlines */
    /* msgno is kept as string value, for backwards compatibility */

    msgno_obj   = Tcl_NewIntObj(msgno);
    Tcl_IncrRefCount(msgno_obj);
    sprintf(conv_buf, "%d", msgno);
    old_obj = Tcl_ObjGetVar2(sybInterp, SybMsgArray, SM_msgno, TCL_GLOBAL_ONLY);
    tmp_obj = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(tmp_obj);
    if (old_obj != NULL) {
	p = Tcl_GetStringFromObj(old_obj, &strLen);
	if (strLen > 0) {
	    Tcl_AppendToObj(tmp_obj, p,  -1);
	    Tcl_AppendToObj(tmp_obj,"\n",-1);
	} 
    } 
    Tcl_AppendToObj(tmp_obj,conv_buf,-1);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_msgno, tmp_obj,TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);


    msgtext_obj = Tcl_NewStringObj(msgtext,-1);
    Tcl_IncrRefCount(msgtext_obj);
    old_obj = Tcl_ObjGetVar2(sybInterp, SybMsgArray,SM_msgtext,TCL_GLOBAL_ONLY);
    tmp_obj = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(tmp_obj);
    if (old_obj != NULL) {
	p = Tcl_GetStringFromObj(old_obj, &strLen);
	if (strLen > 0) {
	    Tcl_AppendToObj(tmp_obj, p,  -1);
	    Tcl_AppendToObj(tmp_obj,"\n",-1);
	} 
    } 
    Tcl_AppendToObj(tmp_obj,msgtext,-1);
    Tcl_ObjSetVar2(sybInterp, SybMsgArray, SM_msgtext, tmp_obj,TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);


    /* call Tcl msg handler callback */
    if (strlen(Tcl_GetStringFromObj(SybOptions[i].sybMsgCmd,NULL)) > 0) {
	callback_cmd = Tcl_DuplicateObj(SybOptions[i].sybMsgCmd);
        Tcl_IncrRefCount(callback_cmd);
	Tcl_AppendToObj(callback_cmd, " ", 1);

	/* construct callback command as list elements */ 
	Tcl_ListObjAppendElement (NULL, callback_cmd, hand_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, msgno_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, msgstate_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, severity_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, msgtext_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, svrname_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, procname_obj);
	Tcl_ListObjAppendElement (NULL, callback_cmd, line_obj);

	Tcl_AppendToObj(callback_cmd, " ;", 2);  /* force back into string */

	/* save existing result, eval callback command */
	save_result = Tcl_GetObjResult(sybInterp);
	Tcl_IncrRefCount(save_result);

	Tcl_Preserve((ClientData) sybInterp);
	result = Tcl_GlobalEvalObj(sybInterp, callback_cmd);
	if (result != TCL_OK) {
	    Tcl_AddErrorInfo(sybInterp,
				"\n    (\"sybmsghandler\" script)");
	    Tcl_BackgroundError(sybInterp);
	}
	Tcl_Release((ClientData) sybInterp);
	Tcl_SetObjResult(sybInterp, save_result);

	Tcl_DecrRefCount(callback_cmd);
	Tcl_DecrRefCount(save_result);

    }

    Tcl_DecrRefCount(hand_obj);
    Tcl_DecrRefCount(msgno_obj);
    Tcl_DecrRefCount(msgstate_obj);
    Tcl_DecrRefCount(severity_obj);
    Tcl_DecrRefCount(msgtext_obj);
    Tcl_DecrRefCount(svrname_obj);
    Tcl_DecrRefCount(procname_obj);
    Tcl_DecrRefCount(line_obj);

    return 0;
}


/*
 *----------------------------------------------------------------------
 * clear_msg --
 *
 * clears all transient error and message elements in the global array variable
 *
 */

#define SYB_TRANS_ELEM 16
static Tcl_Obj **Sybmsg_trans_elements[SYB_TRANS_ELEM];

static void
clear_msg(interp)
    Tcl_Interp *interp;
{
    Tcl_Obj *tmp_obj;
    Tcl_Obj *elem;
    int i;

    for (i = 0; i < SYB_TRANS_ELEM; i++) {
	elem = *Sybmsg_trans_elements[i];
	tmp_obj = Tcl_NewStringObj("",0);
	Tcl_IncrRefCount(tmp_obj);
        Tcl_ObjSetVar2(interp, SybMsgArray, elem, tmp_obj, TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(tmp_obj);
    }
}



/*
 *----------------------------------------------------------------------
 * remove_handler
 *
 * if channel event handler is active, by evidence of callBackScript != NULL
 * remove handler and free callBackScript
 */

static void
remove_handler (hand)
    int hand;
{
    CALLBACK_SCOPE Tcl_ChannelProc callback_handler;
    if (SybProcs[hand].callBackScript != NULL) {
        Tcl_DecrRefCount(SybProcs[hand].callBackScript);
	SybProcs[hand].callBackScript = NULL;
	if (SybProcs[hand].sybChan != NULL) {
	    Tcl_DeleteChannelHandler(SybProcs[hand].sybChan,
				callback_handler, (ClientData) hand);
        }
#ifdef POLLED_EVENTS
        Tcl_DeleteEventSource( dbSetupProc, dbCheckProc, (ClientData) hand);
#endif
    }
}



/*
 *----------------------------------------------------------------------
 * syb_prologue
 *
 * does most of standard command prologue, assumes handle is objv[1]
 * returns: handle index  or -1 on failure
 * 
 */

static int
syb_prologue (interp, objc, objv, num_args, err_msg)
    Tcl_Interp *interp;
    int         objc;
    Tcl_Obj *CONST objv[];
    int         num_args;
    char       *err_msg;
{
    int         hand;


    /* check number of minimum args*/

    if (objc < num_args) {
	Sybtcl_AppendObjResult (interp, "wrong # args: ", 
		CMD_STR, err_msg, (char *) NULL);
	return (-1);
    }

    /* parse the handle */
    hand = get_syb_handle(Tcl_GetStringFromObj(objv[1],NULL));

    if (hand == -1) {
	Sybtcl_AppendObjResult (interp, CMD_STR, 
	  ": handle ", Tcl_GetStringFromObj(objv[1],NULL),
			 " not open ", (char *) NULL);
	return (-1);
    }

    /* save the interp structure for the error & msg handlers */
    SybInterp = interp;

    /* clear sybmsg array for new messages & errors */
    Tcl_IncrRefCount(objv[1]);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_handle,  objv[1], TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(objv[1]);

    clear_msg(interp);

    return (hand);
}



/*
 *----------------------------------------------------------------------
 * events_waiting
 *   process Tcl events while waiting for server response
 *   count=0 if event is expected, loop until results available 
 *   count>1 to loop a specified number of times
 *
 */

static void
events_waiting (hand, maxCount)
    int hand;
    int maxCount;
{
    int         flags;
    int         opt_idx;
    DBPROCESS  *readyproc;
    RETCODE     dbret;
    int         reason;
    int         numLoops = 0;
    long        pollinterval;

    /* don't poll if no sql active */
    if (SybProcs[hand].last_results == NO_MORE_RESULTS) {
	return;
    }

    opt_idx = get_syb_option(SybProcs[hand].interp);

    switch (SybOptions[opt_idx].bgevents) {
        case 1:   /* bgevents = "idletasks" */
            flags = TCL_WINDOW_EVENTS|TCL_IDLE_EVENTS|TCL_DONT_WAIT;
	    break;

	case 2:   /* bgevents = "all" */
            flags = TCL_ALL_EVENTS|TCL_DONT_WAIT;
	    break;

	case 0:	  /* bgevents = "none", "", etc. */
        default:
	    /* don't do any Tcl events */
	    return;
    }
    
    pollinterval = SybOptions[opt_idx].bgpollinterval;

    dbret = dbpoll(SybProcs[hand].dbproc, 1L, &readyproc, &reason);

    while (dbret == SUCCEED && readyproc != SybProcs[hand].dbproc 
		&& reason == DBTIMEOUT) {
	       
	/* do all pending Tcl events, this is just like "update" command */
        while (Tcl_DoOneEvent(flags) != 0) {
	    ; /* empty */
	}

	if (maxCount > 0) {
	    if (++numLoops > maxCount) {
		return;
	    }
	}

	if (SybProcs[hand].last_results == NO_MORE_RESULTS) {
	    return;
	}

        dbret = dbpoll(SybProcs[hand].dbproc, pollinterval, &readyproc,&reason);
    }

    return;
}


/*
 *----------------------------------------------------------------------
 * cancel_prev
 *   cancel previous results until NO_MORE_RESULTS or FAIL
 *
 */

static void
cancel_prev(hand)
    int hand;
{   
    RETCODE dbret;
    do {
        dbcanquery(SybProcs[hand].dbproc);
        dbret = dbresults(SybProcs[hand].dbproc);
    } while (dbret != NO_MORE_RESULTS && dbret != FAIL);

    dbcancel(SybProcs[hand].dbproc);
}



/*
 *----------------------------------------------------------------------
 * parse_column
 *   parse a column result, append as element to current interpreter result
 *
 */

static int
parse_column (interp, hand, col_type, col_len, col_len2, col_ptr, iptr_idx, 
								isnull_list)
    Tcl_Interp *interp;
    int         hand;
    int         col_type;
    int         col_len;
    int         col_len2;
    BYTE       *col_ptr;
    int         iptr_idx;
    Tcl_Obj    *isnull_list;
{
    char       *buf;
    char        buf2[SYB_BUFF_SIZE];
    int         dst_len;
    long         text_buf_size;
    DBINT       int_val;
    DBFLT8	float_val;
    DBDATEREC   dateinfo;
    DBDATETIME  convert_date;
    int         j;
    char        tempstr[SYB_BUFF_SIZE];
    char       *p;
    int         decimal;
    Tcl_Obj    *resultPtr;
    char       *null_str;
    char       *date_str;
    int         rc;
    int         len;


    resultPtr = Tcl_GetObjResult(interp);

    /* if column is null, extract the current "nullvalue" */
    if ((col_len == 0) && (col_ptr == (BYTE *)NULL)) {

	/* note that column is not null */
	Tcl_ListObjAppendElement(NULL, isnull_list, BoolTrue);

	null_str = SybOptions[iptr_idx].nullvalue;

	/* if "nullvalue" is  the string "default", append the defaults,    */
	/* otherwise, append the user requested nullvalue, which could be ""*/

	if (*null_str == 'd' && strncmp(null_str,"default",7) == 0) {
	    /* default-if number or money type, then return "0", else ""     */

            if (
#ifdef DBVERSION_100
                col_type == SYBDECIMAL ||
                col_type == SYBNUMERIC ||
#endif
	        col_type == SYBINTN    ||
	        col_type == SYBINT1    ||
                col_type == SYBINT2    ||
                col_type == SYBINT4    ||
                col_type == SYBFLTN    ||
                col_type == SYBFLT8    ||
                col_type == SYBREAL    ||
                col_type == SYBMONEYN  ||
                col_type == SYBMONEY   ||
                col_type == SYBMONEY4
                ) {
                Tcl_ListObjAppendElement(interp, resultPtr, 
						Tcl_NewIntObj(0));
	    } else {
                Tcl_ListObjAppendElement(interp, resultPtr, 
						Tcl_NewStringObj("",0));
	    }

	} else {
	    /* return the null representation */
            Tcl_ListObjAppendElement(interp, resultPtr,
				Tcl_NewStringObj(null_str,-1));
	}

	return (1);
	  
    } 

    /* note that column is not null */
    Tcl_ListObjAppendElement(NULL, isnull_list, BoolFalse);

    switch (col_type) {

	case SYBDATETIME:
	case SYBDATETIME4:
	case SYBDATETIMN:
	    
	    date_str = SybOptions[iptr_idx].dateformat;
	    
	    if ((len = strlen(date_str)) > 0) {
	        buf = ckalloc((len * 2) + 1);
		if (buf == NULL) {
		    Tcl_SetObjResult (interp, Tcl_NewStringObj(
		     "parse_column cannot malloc memory for date format",-1));
		    return (0);
		}
		if (col_type == SYBDATETIME4 || col_type == SYBDATETIMN) {
		    dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, 
			     col_len, SYBDATETIME, (BYTE *) &convert_date, 
			     sizeof(convert_date));
		    col_ptr = (BYTE *) &convert_date;
		}
		buf[0] = '\0';

		dbdatecrack(SybProcs[hand].dbproc,
				&dateinfo,(DBDATETIME *) col_ptr);
		j=0;
		while((unsigned int) j < strlen(date_str)) {
		    if (!strncmp(&(date_str[j]),"YYYY",4)) {
		        sprintf(tempstr,"%04d",dateinfo.dateyear);
		        j += 4;
		    } else if (!strncmp(&(date_str[j]),"YY",2)) {
		        sprintf(tempstr,"%02d",dateinfo.dateyear % 100);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"MONTH",5)) {
		        strcpy(tempstr,dbmonthname(SybProcs[hand].dbproc,
				(char *)NULL, dateinfo.datemonth+1,FALSE));
		        j += 5;
		    } else if (!strncmp(&(date_str[j]),"MON",3)) {
		        strcpy(tempstr,dbmonthname(SybProcs[hand].dbproc,
				(char *)NULL, dateinfo.datemonth+1,TRUE));
		        j += 3;
		    } else if (!strncmp(&(date_str[j]),"MM",2)) {
		        sprintf(tempstr,"%02d",dateinfo.datemonth + 1);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"DD",2)) {
		        sprintf(tempstr,"%02d",dateinfo.datedmonth);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"hh",2)) {
		        sprintf(tempstr,"%02d",dateinfo.datehour);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"mm",2)) {
		        sprintf(tempstr,"%02d",dateinfo.dateminute);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"ss",2)) {
		        sprintf(tempstr,"%02d",dateinfo.datesecond);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"dy",2)) {
		        sprintf(tempstr,"%03d",dateinfo.datedyear);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"dw",2)) {
		        sprintf(tempstr,"%02d",dateinfo.datedweek + 1);
		        j += 2;
		    } else if (!strncmp(&(date_str[j]),"ms",2)) {
		        sprintf(tempstr,"%03d",dateinfo.datemsecond);
		        j += 2;
		    } else {
		        tempstr[0] = date_str[j];
		        tempstr[1] = '\0';
		        j++;
		    }
		    strcat(buf,tempstr);
		}
		Tcl_ListObjAppendElement(interp,resultPtr,
						Tcl_NewStringObj(buf,-1));
		ckfree(buf);
	    } else {
    		dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len, 
				    SYBCHAR, (BYTE *)buf2, -1);
	    	Tcl_ListObjAppendElement(interp,resultPtr,
				Tcl_NewStringObj(buf2,-1));

	    }
	    break;


	case SYBMONEYN:
	case SYBMONEY:
	    /* large money values may loose precsion, so convert to char */
	    /* smallmoney is handled as float */
	    dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len,
			  SYBCHAR, (BYTE *)buf2, -1);

	    /* make sure it looks like a float */
	    decimal = 0;
	    for (p = buf2; *p != 0; p++) {
		if (*p == '.') {
		    decimal = 1;
		    break;
		}
	    }
	    if (!decimal) {
		*p++ = '.';
		*p++ = '0';
		*p   = '\0';
	    }

	    Tcl_ListObjAppendElement(interp, resultPtr,
				  Tcl_NewStringObj(buf2,-1));
	    break;


	case SYBMONEY4:
	case SYBREAL:
	case SYBFLT8:
	case SYBFLTN:
	    /* convert sybase types of money, real, and float to double, */
	    /* or as char if loss of precision                           */

	    rc = dbconvert(NULL,col_type, col_ptr, col_len,
			      SYBFLT8, (BYTE *) &float_val, -1);


	    if (rc > 0) {
		/* conversion ok, save as Tcl double */
		Tcl_ListObjAppendElement(interp,resultPtr, 
					Tcl_NewDoubleObj(float_val));
		return (1);
	    } else {
		/* lost precision, convert to char */
		dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len,
			  SYBCHAR, (BYTE *)buf2, -1);

		/* make sure it looks like a float */
		decimal = 0;
		for (p = buf2; *p != 0; p++) {
		    if (*p == '.') {
			decimal = 1;
			break;
		    }
		}
		if (!decimal) {
		    *p++ = '.';
		    *p++ = '0';
		    *p   = '\0';
		}

		Tcl_ListObjAppendElement(interp, resultPtr,
				      Tcl_NewStringObj(buf2,-1));
	    }
	    break;


        case SYBBIT:
        case SYBINT1:
        case SYBINT2:
        case SYBINT4:
        case SYBINTN:
	    /* conversion to long integer */
	    dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len,
				  SYBINT4, (BYTE *)&int_val, -1);
	    Tcl_ListObjAppendElement(interp,resultPtr, 
						Tcl_NewLongObj(int_val));
	    break;


	case SYBBINARY:
	case SYBVARBINARY:
	case SYBTEXT:
	case SYBIMAGE:
    	    /* only convert text & image to the extent of buffer size */
    
	    if (col_type == SYBTEXT || col_type == SYBIMAGE) {
	        text_buf_size = SybOptions[iptr_idx].maxtext;
	        text_buf_size = (col_len < text_buf_size) ? col_len : 
								text_buf_size;
	    } else {
		text_buf_size = col_len;
	    }

	    /* for SYBTEXT or if explicit conversion to hex string */
	    /* we convert to SYBCHAR, else just grab col_ptr */

 	    if (col_type == SYBTEXT || SybOptions[iptr_idx].binaryashex) {
		if ((text_buf_size + 4) < SYB_BUFF_SIZE) {
		    buf = buf2;
		} else {
		    buf = ckalloc(text_buf_size + 4); /* 4 extra fudge bytes */
		    if (buf == NULL) {
			Sybtcl_AppendObjResult (interp, 
			 " parse_column cannot malloc memory for TEXT or IMAGE",
			       (char *) NULL );
			return (0);
		    }
		}
	        memset(buf,'\0',text_buf_size+2); 
		dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len,
			SYBCHAR, (BYTE *)buf, -1);
		text_buf_size = -1;
	    } else {
		/* just grab the raw buffer */
		buf = (char *) col_ptr;
	    }

	    Tcl_ListObjAppendElement(interp,resultPtr,
				Tcl_NewStringObj(buf, text_buf_size));
	    if ((buf != (char *) col_ptr) && (buf != buf2)) {
		ckfree(buf);
	    }

	    break;


	case SYBCHAR:
	case SYBVARCHAR:
	    /* conversion to char, with or without trailing blanks */
      	    if (SybOptions[iptr_idx].fixedchar) {
        	dst_len = col_len2;	/* use the max column length */
      	    } else {
        	dst_len = -1;	/* dbconvert will null terminate */
      	    }
    	    dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len, 
				    SYBCHAR, (BYTE *) buf2, dst_len);

	    Tcl_ListObjAppendElement(interp,resultPtr,
				Tcl_NewStringObj(buf2,dst_len));
	    break;


#ifdef DBVERSION_100
        case SYBDECIMAL:
        case SYBNUMERIC:
	    /* convert to char to keep exact representation */

	    dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len, 
				SYBCHAR, (BYTE *)buf2, -1);


	    Tcl_ListObjAppendElement(interp, resultPtr,
				Tcl_NewStringObj(buf2,-1));

	    break;
#endif  


#ifdef DBVERSION_100
	case SYBBOUNDARY:
	case SYBSENSITIVITY:
#endif  
	default:
    	    dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len, 
				    SYBCHAR, (BYTE *)buf2, -1);

	    Tcl_ListObjAppendElement(interp,resultPtr,
				Tcl_NewStringObj(buf2,-1));
	    break;
    }

    return (1);

}


/*
 *----------------------------------------------------------------------
 * Sybtcl_Kill --
 *   perform all cleanup upon at exit
 *
 */

void
Sybtcl_Kill (clientData)
    ClientData clientData;
{
    int i;

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybProcs[i].in_use && SybProcs[i].dbproc != NULL) {
	    syb_dbclose(SybProcs[i].dbproc);
	    /* remove call back handler and free callback script, if any */
	    remove_handler(i);
	    SybProcs[i].in_use = 0;
            Tcl_Close((Tcl_Interp *) NULL, SybProcs[i].sybChan);
	    SybProcs[i].sybChan = NULL;
	}
        SybProcs[i].in_use = 0;
	SybProcs[i].interp = NULL;
	SybProcs[i].dbproc = NULL;
	if (SybProcs[i].bufferedResult != NULL) {
	    Tcl_DecrRefCount(SybProcs[i].bufferedResult);
	    SybProcs[i].bufferedResult = NULL;
	}
	if (SybProcs[i].bufferedIsnull != NULL) {
	    Tcl_DecrRefCount(SybProcs[i].bufferedIsnull);
	    SybProcs[i].bufferedIsnull = NULL;
	}
    }

    /* don't call dbexit() if under Windows, sometimes causes crash */
#ifndef __WIN32__
    dbexit();		/* last cleanup */
#endif

    /* clean up allocated objects */

    Tcl_DecrRefCount(SybMsgArray);
    Tcl_DecrRefCount(BoolTrue);
    Tcl_DecrRefCount(BoolFalse);
    Tcl_DecrRefCount(SM_handle);
    Tcl_DecrRefCount(SM_msgno);
    Tcl_DecrRefCount(SM_msgstate);
    Tcl_DecrRefCount(SM_msgtext);
    Tcl_DecrRefCount(SM_severity);
    Tcl_DecrRefCount(SM_svrname);
    Tcl_DecrRefCount(SM_procname);
    Tcl_DecrRefCount(SM_line);
    Tcl_DecrRefCount(SM_dberr);
    Tcl_DecrRefCount(SM_oserr);
    Tcl_DecrRefCount(SM_dberrstr);
    Tcl_DecrRefCount(SM_oserrstr);
    Tcl_DecrRefCount(SM_nextrow);
    Tcl_DecrRefCount(SM_retstatus);
    Tcl_DecrRefCount(SM_collengths);
    Tcl_DecrRefCount(SM_coltypes);
    Tcl_DecrRefCount(SM_version);
    Tcl_DecrRefCount(SM_handle);
    Tcl_DecrRefCount(SM_nullvalue);
    Tcl_DecrRefCount(SM_floatprec);
    Tcl_DecrRefCount(SM_maxtext);
    Tcl_DecrRefCount(SM_dateformat);
    Tcl_DecrRefCount(SM_fixedchar);
    Tcl_DecrRefCount(SM_dblibinfo);
    Tcl_DecrRefCount(SM_binaryashex);
    Tcl_DecrRefCount(SM_bgevents);
    Tcl_DecrRefCount(SM_bgpollinterval);
    Tcl_DecrRefCount(SM_isnull);
    Tcl_DecrRefCount(SM_maxhandles);
    Tcl_DecrRefCount(SM_usedhandles);

    MY_STATS_DUMP;
}



/*
 *----------------------------------------------------------------------
 * Sybtcl_DeleteInterp --
 *   close any dbprocs left open when an interpreter is deleted
 *   and clear interp level options
 *
 */

void
Sybtcl_DeleteInterp (clientData, interp)
    ClientData clientData;
    Tcl_Interp *interp;
{
    int i;

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybProcs[i].in_use && SybProcs[i].interp == interp) {
	    if (SybProcs[i].dbproc != NULL) {
	        syb_dbclose(SybProcs[i].dbproc);
	        /* remove call back handler and free callback script, if any */
	        remove_handler(i);
	        SybProcs[i].in_use       = 0;
                Tcl_Close((Tcl_Interp *) NULL, SybProcs[i].sybChan);
		SybProcs[i].sybChan = NULL;
	    }
	    SybProcs[i].in_use       = 0;
	    SybProcs[i].interp       = NULL;
	    SybProcs[i].dbproc       = NULL;
	    SybProcs[i].last_results = NO_MORE_RESULTS;
	    SybProcs[i].last_next    = NO_MORE_ROWS;
	    SybProcs[i].in_event     = 0;
	    SybProcs[i].hasBgResults = 0;
	    SybProcs[i].bgResults    = 0;
	    SybProcs[i].async        = 0;
	    SybProcs[i].last_text    = 32768;
	    if (SybProcs[i].bufferedResult != NULL) {
		Tcl_DecrRefCount(SybProcs[i].bufferedResult);
		SybProcs[i].bufferedResult = NULL;
	    }
	    if (SybProcs[i].bufferedIsnull != NULL) {
		Tcl_DecrRefCount(SybProcs[i].bufferedIsnull);
		SybProcs[i].bufferedIsnull = NULL;
	    }
        }
    }

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybOptions[i].interp == interp) {
	    Tcl_DecrRefCount(SybOptions[i].sybMsgCmd);
	    Tcl_DecrRefCount(SybOptions[i].sybErrCmd);
            SybOptions[i].interp       = NULL;
            SybOptions[i].sybMsgCmd    = NULL;
            SybOptions[i].sybErrCmd    = NULL;
            SybOptions[i].nullvalue    = NULL;
            SybOptions[i].dateformat   = NULL;
            SybOptions[i].maxtext      = 0;
            SybOptions[i].fixedchar    = 0;
            SybOptions[i].binaryashex  = 0;
            SybOptions[i].bgevents     = BG_STATE;
            SybOptions[i].bgpollinterval = POLL_INTERVAL;
        }
    }
}



/*
 *----------------------------------------------------------------------
 * Sybtcl_MsgTrace --
 *   implement the traces for programmer option elements in sybmsg array
 *   
 */

static char *
Sybtcl_MsgTrace(clientData, interp, name1, name2, flags)
    ClientData clientData;
    Tcl_Interp *interp;
    char *name1;
    char *name2;
    int flags;
{   
    int i;
    long val;
    int bool;
    char *str;
    int len;

    Tcl_Obj *tmp_obj;


    i = get_syb_option(interp);
    if (i == -1) {
	return NULL;
    }

    if ((flags & TCL_TRACE_DESTROYED)  == TCL_TRACE_DESTROYED ||
	(flags & TCL_INTERP_DESTROYED) == TCL_INTERP_DESTROYED) {
	return NULL;
    }

    /* check if whole sybmsg array is unset */
    if ((name2 == NULL) && (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	SybOptions[i].nullvalue    = "default";
	SybOptions[i].dateformat   = "";
	SybOptions[i].maxtext      = 32768;
	SybOptions[i].fixedchar    = 0;
	SybOptions[i].binaryashex  = 0;
	SybOptions[i].bgevents     = BG_STATE;
	SybOptions[i].bgpollinterval = POLL_INTERVAL;

    } else if (strncmp(name2,"nullvalue",9) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_nullvalue, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].nullvalue = "default";
	} else {
	    SybOptions[i].nullvalue = Tcl_GetStringFromObj(tmp_obj,NULL);
	}

    } else if (strncmp(name2,"dateformat",10) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_dateformat, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].dateformat = "";
	} else {
	    SybOptions[i].dateformat = Tcl_GetStringFromObj(tmp_obj,NULL);
	}

    } else if (strncmp(name2,"maxtext",7) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_maxtext, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].maxtext = TEXT_BUFF_SIZE;
	} else {
	    if (Tcl_GetLongFromObj(NULL,tmp_obj,&val) == TCL_OK) {
		if (val < 0L) {
		    val = TEXT_BUFF_SIZE;
		} else if (val > MAX_SERVER_TEXTI) {
		    val = MAX_SERVER_TEXTI;
		}
	    } else {
		val = TEXT_BUFF_SIZE;
	    }
            Tcl_ObjSetVar2(interp, SybMsgArray, SM_maxtext,  
			Tcl_NewIntObj(val),  TCL_GLOBAL_ONLY);
	    SybOptions[i].maxtext = val;
	}

    } else if (strncmp(name2,"floatprec",9) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_floatprec, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].floatprec = -1;
	} else {
	    if (Tcl_GetLongFromObj(NULL,tmp_obj,&val) == TCL_OK) {
		if (val < 0L) {
		    val = -1;
		} else if (val > TCL_MAX_PREC) {
		    val = TCL_MAX_PREC;
		}
	    } else {
		val = -1;
	    }
            Tcl_ObjSetVar2(interp, SybMsgArray, SM_floatprec,  
			Tcl_NewStringObj("",0),  TCL_GLOBAL_ONLY);
	    SybOptions[i].floatprec = val;
	}

    } else if (strncmp(name2,"fixedchar",9) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_fixedchar, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].fixedchar = 0;
	} else {
	    if (Tcl_GetBooleanFromObj(NULL,tmp_obj,&bool) == TCL_OK) {
	        SybOptions[i].fixedchar = bool;
	    } else {
	        SybOptions[i].fixedchar = 0;
                Tcl_ObjSetVar2(interp, SybMsgArray, SM_fixedchar,  
			Tcl_NewStringObj("",0),  TCL_GLOBAL_ONLY);
	    }
	}

    } else if (strncmp(name2,"binaryashex",11) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_binaryashex, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].binaryashex = 0;
	} else {
	    if (Tcl_GetBooleanFromObj(NULL,tmp_obj,&bool) == TCL_OK) {
	        SybOptions[i].binaryashex = bool;
	    } else {
		SybOptions[i].binaryashex = 0;
                Tcl_ObjSetVar2(interp, SybMsgArray, SM_binaryashex,  
			Tcl_NewStringObj("",0),  TCL_GLOBAL_ONLY);
	    }
	}

    } else if (strncmp(name2,"bgevents",8) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_bgevents, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].bgevents = BG_STATE;
	} else {
	    str = Tcl_GetStringFromObj(tmp_obj,&len);
            if (strncmp(str,"idletasks",9) == 0) {
	        SybOptions[i].bgevents = 1;
	    } else if (strncmp(str,"all",3) == 0) {
	        SybOptions[i].bgevents = 2;
	    } else { /* anything else */
		SybOptions[i].bgevents = 0;
                Tcl_ObjSetVar2(interp, SybMsgArray, SM_bgevents,  
			Tcl_NewStringObj("none",-1),  TCL_GLOBAL_ONLY);
	    }
	}

    } else if (strncmp(name2,"bgpollinterval",14) == 0) {
        tmp_obj = Tcl_ObjGetVar2(interp, SybMsgArray, SM_bgpollinterval, 
		  TCL_GLOBAL_ONLY);
	if (tmp_obj == NULL || (flags & TCL_TRACE_UNSETS) == TCL_TRACE_UNSETS) {
	    SybOptions[i].bgpollinterval = POLL_INTERVAL;
	} else {
	    if (Tcl_GetLongFromObj(NULL,tmp_obj,&val) == TCL_OK) {
		if (val < 1L) {
		    val = 1;
		} else if (val > 1000) {
		    val = 1000;
		}
	    } else {
	        val = POLL_INTERVAL;
            } 
            Tcl_ObjSetVar2(interp, SybMsgArray, SM_bgpollinterval,  
			Tcl_NewIntObj(val),  TCL_GLOBAL_ONLY);
	    SybOptions[i].bgpollinterval = val;
	}

    }

    return NULL;

}



/*
 *----------------------------------------------------------------------
 * Sybtcl_Init --
 *   perform all initialization for the Sybase to Tcl interface.
 *   adds additional commands to interp, creates message array
 *
 *   a call to Sybtcl_Init should exist in Tcl_CreateInterp or
 *   Tcl_CreateExtendedInterp.
 */

int
Sybtcl_Init (interp)
    Tcl_Interp *interp;
{
    int i;
    int elements;
    static int SybtclInitFlag = 0;   /* set when initialized via Sybtcl_Init */
    Tcl_Obj *tmp_obj;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.0", 0) == NULL) {
        return TCL_ERROR;
    }
#endif


    /* save the interp structure for the error & msg handlers */
    SybInterp = interp;

    /* check if already initialized */
    if (!SybtclInitFlag) {

        MY_STATS_INIT;

	if (dbinit() == FAIL) {
	    return TCL_ERROR;
	}

#ifdef DBVERSION_100
	dbsetversion(DBVERSION_100); /*enable system 10 numeric/decimal types */
#endif

	dbsetmaxprocs(SYBTCLPROCS);  /*make sure db-lib match our table size*/

#ifdef __WIN32__
	dberrhandle(syb_tcl_err_handler);
	dbmsghandle((MHANDLEFUNC) syb_tcl_msg_handler);
#else
	dberrhandle(syb_tcl_err_handler);
	dbmsghandle(syb_tcl_msg_handler);
#endif

	/*
	 * Initialize sybase proc structures 
	 */

	for (i = 0; i < SYBTCLPROCS; i++) {
	    SybProcs[i].in_use         = 0;
	    SybProcs[i].interp         = NULL;
	    SybProcs[i].dbproc         = NULL;
	    SybProcs[i].last_results   = NO_MORE_RESULTS;
	    SybProcs[i].last_next      = NO_MORE_ROWS;
	    SybProcs[i].bufferedResult = NULL;
	    SybProcs[i].bufferedIsnull = NULL;
	    SybProcs[i].async          = 0;
	    SybProcs[i].last_text      = 32768;
	    SybProcs[i].callBackScript = NULL;
	    SybProcs[i].in_event       = 0;
	    SybProcs[i].hasBgResults   = 0;
	    SybProcs[i].bgResults      = 0;

	    SybOptions[i].interp       = NULL;
            SybOptions[i].sybMsgCmd    = NULL;
            SybOptions[i].sybErrCmd    = NULL;
            SybOptions[i].nullvalue    = NULL;
            SybOptions[i].dateformat   = NULL;
            SybOptions[i].maxtext      = 0;
            SybOptions[i].fixedchar    = 0;
            SybOptions[i].binaryashex  = 0;
            SybOptions[i].bgevents     = BG_STATE;
            SybOptions[i].bgpollinterval = POLL_INTERVAL;

	}

        SybtclInitFlag = 1;

	Tcl_CreateExitHandler (Sybtcl_Kill, (ClientData)NULL);

        BoolTrue  = Tcl_NewBooleanObj(1);
        BoolFalse = Tcl_NewBooleanObj(0);
	Tcl_IncrRefCount(BoolTrue);
	Tcl_IncrRefCount(BoolFalse);

	/*
	 * Initialize sybmsg global array, inital null elements
	 */
	
	/* first build string objects for array name and elements */
	SybMsgArray   = Tcl_NewStringObj("sybmsg",     -1);
	SM_handle     = Tcl_NewStringObj("handle",     -1);
	SM_msgno      = Tcl_NewStringObj("msgno",      -1);
	SM_msgstate   = Tcl_NewStringObj("msgstate",   -1);
	SM_msgtext    = Tcl_NewStringObj("msgtext",    -1);
	SM_severity   = Tcl_NewStringObj("severity",   -1);
	SM_svrname    = Tcl_NewStringObj("svrname",    -1);
	SM_procname   = Tcl_NewStringObj("procname",   -1);
	SM_line       = Tcl_NewStringObj("line",       -1);
	SM_dberr      = Tcl_NewStringObj("dberr",      -1);
	SM_oserr      = Tcl_NewStringObj("oserr",      -1);
	SM_dberrstr   = Tcl_NewStringObj("dberrstr",   -1);
	SM_oserrstr   = Tcl_NewStringObj("oserrstr",   -1);
	SM_nextrow    = Tcl_NewStringObj("nextrow",    -1);
	SM_retstatus  = Tcl_NewStringObj("retstatus",  -1);
	SM_collengths = Tcl_NewStringObj("collengths", -1);
	SM_coltypes   = Tcl_NewStringObj("coltypes",   -1);
	SM_version    = Tcl_NewStringObj("version",    -1);
	SM_handle     = Tcl_NewStringObj("handle",     -1);
	SM_nullvalue  = Tcl_NewStringObj("nullvalue",  -1);
	SM_floatprec  = Tcl_NewStringObj("floatprec",  -1);
	SM_maxtext    = Tcl_NewStringObj("maxtext",    -1);
	SM_dateformat = Tcl_NewStringObj("dateformat", -1);
	SM_fixedchar  = Tcl_NewStringObj("fixedchar",  -1);
	SM_dblibinfo  = Tcl_NewStringObj("dblibinfo",  -1);
	SM_binaryashex= Tcl_NewStringObj("binaryashex",-1);
	SM_bgevents   = Tcl_NewStringObj("bgevents",   -1);
	SM_bgpollinterval = Tcl_NewStringObj("bgpollinterval",   -1);
	SM_isnull     = Tcl_NewStringObj("isnull",     -1);
	SM_maxhandles = Tcl_NewStringObj("maxhandles", -1);
	SM_usedhandles= Tcl_NewStringObj("usedhandles",-1);

	Tcl_IncrRefCount(SybMsgArray);
	Tcl_IncrRefCount(SM_handle);
	Tcl_IncrRefCount(SM_msgno);
	Tcl_IncrRefCount(SM_msgstate);
	Tcl_IncrRefCount(SM_msgtext);
	Tcl_IncrRefCount(SM_severity);
	Tcl_IncrRefCount(SM_svrname);
	Tcl_IncrRefCount(SM_procname);
	Tcl_IncrRefCount(SM_line);
	Tcl_IncrRefCount(SM_dberr);
	Tcl_IncrRefCount(SM_oserr);
	Tcl_IncrRefCount(SM_dberrstr);
	Tcl_IncrRefCount(SM_oserrstr);
	Tcl_IncrRefCount(SM_nextrow);
	Tcl_IncrRefCount(SM_retstatus);
	Tcl_IncrRefCount(SM_collengths);
	Tcl_IncrRefCount(SM_coltypes);
	Tcl_IncrRefCount(SM_version);
	Tcl_IncrRefCount(SM_handle);
	Tcl_IncrRefCount(SM_nullvalue);
	Tcl_IncrRefCount(SM_floatprec);
	Tcl_IncrRefCount(SM_maxtext);
	Tcl_IncrRefCount(SM_dateformat);
	Tcl_IncrRefCount(SM_fixedchar);
	Tcl_IncrRefCount(SM_dblibinfo);
	Tcl_IncrRefCount(SM_binaryashex);
	Tcl_IncrRefCount(SM_bgevents);
	Tcl_IncrRefCount(SM_bgpollinterval);
	Tcl_IncrRefCount(SM_isnull);
	Tcl_IncrRefCount(SM_maxhandles);
	Tcl_IncrRefCount(SM_usedhandles);

	/* populate sybmsg transient element array */
	Sybmsg_trans_elements[0]  = &SM_msgno;
	Sybmsg_trans_elements[1]  = &SM_msgstate;
	Sybmsg_trans_elements[2]  = &SM_msgtext;
	Sybmsg_trans_elements[3]  = &SM_severity;
	Sybmsg_trans_elements[4]  = &SM_svrname;
	Sybmsg_trans_elements[5]  = &SM_procname;
	Sybmsg_trans_elements[6]  = &SM_line;
	Sybmsg_trans_elements[7]  = &SM_dberr;
	Sybmsg_trans_elements[8]  = &SM_oserr;
	Sybmsg_trans_elements[9]  = &SM_dberrstr;
	Sybmsg_trans_elements[10] = &SM_oserrstr;
	Sybmsg_trans_elements[11] = &SM_nextrow;
	Sybmsg_trans_elements[12] = &SM_retstatus;
	Sybmsg_trans_elements[13] = &SM_collengths;
	Sybmsg_trans_elements[14] = &SM_coltypes;
	Sybmsg_trans_elements[15] = &SM_isnull;
     
    }


    /*
     * Initialize the new Tcl commands
     */

    Tcl_CreateObjCommand (interp, "sybconnect", 
	Sybtcl_Connect,(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "syberrhandler",
	Sybtcl_ErrHandler,(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybmsghandler",
	Sybtcl_MsgHandler,(ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybuse",     
	Sybtcl_Use,    (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybmoney",     
	Sybtcl_Money,  (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybsql",     
	Sybtcl_Sql,    (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybpoll",    
	Sybtcl_Poll,   (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybnext",    
	Sybtcl_Next,   (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybcols",    
	Sybtcl_Cols,   (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybcancel",  
        Sybtcl_Cancel, (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybretval",  
	Sybtcl_Retval, (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybclose",   
	Sybtcl_Close,  (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybwritetext",
	Sybtcl_Wrtext,(ClientData)NULL,  (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybreadtext",
	Sybtcl_Rdtext, (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand (interp, "sybevent",
	Sybtcl_Event,  (ClientData)NULL, (Tcl_CmdDeleteProc *) NULL);

    /* find an unused interp option slot */
    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybOptions[i].interp == NULL) {
	    SybOptions[i].interp = interp;
	    break;
	}
    }
    if (i >= SYBTCLPROCS) {
	return TCL_ERROR;
    }

    /* objects for defining error and message handlers */

    SybOptions[i].sybErrCmd = Tcl_NewStringObj("",0);
    SybOptions[i].sybMsgCmd = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(SybOptions[i].sybErrCmd);
    Tcl_IncrRefCount(SybOptions[i].sybMsgCmd);


    /* index "version" - sybtcl version number*/
    tmp_obj = Tcl_NewStringObj(SYBTCL_VERSION, -1);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_version, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);

    /* index "handle" - the last handle that set any other elements */
    tmp_obj = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_handle,  tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);

    /* index "dblibinfo" - set to "system10" if decimal/numeric types allowed */
    /*                            "ctcompat" if compiled with ctcompat lib    */
#ifdef DBVERSION_100
    tmp_obj = Tcl_NewStringObj("system10", -1);
    Tcl_IncrRefCount(tmp_obj);
    elements = 1;
#else
    tmp_obj = Tcl_NewStringObj("", 0);
    Tcl_IncrRefCount(tmp_obj);
    elements = 0;
#endif
#ifdef CTCOMPATLIB
    if (elements > 0) {
        Tcl_AppendToObj(tmp_obj, " ", -1);
    }
    Tcl_AppendToObj(tmp_obj, "ctcompat", -1);
#endif
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_dblibinfo, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);

    /* index "maxhandles" - maximum number of handles available */
    tmp_obj = Tcl_NewIntObj(SYBTCLPROCS);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_maxhandles,tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);

    /* index "usedhandles" - number of handles in use */
    count_inuse(interp);

    /* the following sybmsg elements are mirrored via traces */

    /* index "nullvalue" - what to return if column is null */
    /* user should set this value if something else is wanted */
    tmp_obj = Tcl_NewStringObj("default", -1);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_nullvalue, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "nullvalue", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].nullvalue = "default";

    /* index "dateformat" - the format that the user wants datetime */
    /* values to return in. */
    tmp_obj = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_dateformat, tmp_obj,TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "dateformat", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].dateformat = "";

    /* index "maxtext" - the maximum length of a text column to return */
    /* in a select or sybreadtext , can be set by user                 */
    tmp_obj = Tcl_NewIntObj(TEXT_BUFF_SIZE);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_maxtext, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "maxtext", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].maxtext = TEXT_BUFF_SIZE;

    /* index "fixedchar" - set to "yes" if char/varchar should not trim */
    /* trailing spaces */
    tmp_obj = Tcl_NewIntObj(0);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_fixedchar, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "fixedchar", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].fixedchar = 0;

    /* index "binaryashex" - set to "yes" to convert bin/image to hex strings*/
    tmp_obj = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_binaryashex,tmp_obj,TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "binaryashex", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].binaryashex = 0;

    /* index "bgevents" - set to "idletasks"  -like "update idletasks" */
    /*                           "all"        -like "update"           */
    /*                           other        -no bgevents processed   */
    tmp_obj = Tcl_NewStringObj("idletasks",-1);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_bgevents, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "bgevents", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].bgevents = BG_STATE;

    /* index "bgpollinterval" - time in milli seconds to poll in events  */
    /*                           1-1000  */
    tmp_obj = Tcl_NewIntObj(POLL_INTERVAL);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_bgpollinterval, tmp_obj, 
			TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "bgpollinterval", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].bgpollinterval = POLL_INTERVAL;

    /* index "floatprec" - floating point precision, int if specified */
    /* deprecated! - use tcl_precision instead */
    /* user should set this value if something else is wanted */
    tmp_obj = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_floatprec, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);
    Tcl_TraceVar2(interp, "sybmsg", "floatprec", 
	      TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
              (Tcl_VarTraceProc *) Sybtcl_MsgTrace, (ClientData) NULL);
    SybOptions[i].floatprec = -1;


    /* other indices - correspond to error and message handler arguments */
    clear_msg(interp);


    /* callback to clean up any dbprocs left open on interpreter deletion */
    Tcl_CallWhenDeleted(interp, (Tcl_InterpDeleteProc *) Sybtcl_DeleteInterp,
			(ClientData) NULL);


    if (Tcl_PkgProvide(interp, "Sybtcl", SYBTCL_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    return TCL_OK;

}



/*
 *----------------------------------------------------------------------
 * Sybtcl_SafeInit --
 *   call the standard init point
 */

int
Sybtcl_SafeInit (interp)
    Tcl_Interp *interp;
{
    return (Sybtcl_Init(interp));
}




/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_ErrHandler --
 *    Implements the syberrhandler command:
 *    usage: syberrhandler ?errhandlerproc?
 *	                
 *    results:
 *      TCL_OK
 */

int
Sybtcl_ErrHandler (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int i;

    i = get_syb_option(interp);
    if (i == -1) {
	return TCL_ERROR;
    }

    if (objc > 1) {
	Tcl_SetStringObj(SybOptions[i].sybErrCmd, 
				Tcl_GetStringFromObj(objv[1],NULL), -1);
	Tcl_SetObjResult(interp,objv[1]);
    } else {
	Tcl_SetObjResult(interp,Tcl_DuplicateObj(SybOptions[i].sybErrCmd));
    }
    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_MsgHandler --
 *    Implements the sybmsghandler command:
 *    usage: sybmsghandler ?msghandlerproc?
 *	                
 *    results:
 *      TCL_OK 
 */

int
Sybtcl_MsgHandler (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int i;

    i = get_syb_option(interp);
    if (i == -1) {
	return TCL_ERROR;
    }

    if (objc > 1) {
	Tcl_SetStringObj(SybOptions[i].sybMsgCmd, 
			Tcl_GetStringFromObj(objv[1],NULL), -1);
	Tcl_SetObjResult(interp,objv[1]);
    } else {
	Tcl_SetObjResult(interp,Tcl_DuplicateObj(SybOptions[i].sybMsgCmd));
    }
    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Connect --
 *    Implements the sybconnect command:
 *    usage: sybconnect userid password ?server? ?appname? ?ifile? ?charset?
 *		
 *    results:
 *	handle - a character string of newly open handle
 *      TCL_OK - connect successful
 *      TCL_ERROR - connect not successful - error message returned
 */

int
Sybtcl_Connect (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{

    LOGINREC  *login;
    int        hand = -1;
    int        i;
    char       buf[SYB_BUFF_SIZE];
    Tcl_Obj   *tmp_obj;
    char      *appname_str;
    char      *ifile_str;
    char      *charset_str;
    char      *server_str;
    int        len;


    /* can't use syb_prologue, sybconnect creates a handle */

    if (objc < 3) {
	Sybtcl_AppendObjResult (interp, "wrong # args: ", CMD_STR,
	  " userid password ?server? ?appname? ?ifile? ?charset? ",
	  (char *) NULL);

	return TCL_ERROR;
    }

    /* find an unused handle */

    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybProcs[i].in_use == 0) {
            hand = i;
	    break;
	}
    }

    if (hand == -1) {
	Sybtcl_AppendObjResult (interp, CMD_STR,
		": no sybase dbprocs available", (char *) NULL);
	return TCL_ERROR;
    }

    /* save the interp structure for the error & msg handlers */
    SybInterp = interp;

    login = dblogin();

    DBSETLUSER(login,Tcl_GetStringFromObj(objv[1],NULL));
    DBSETLPWD(login, Tcl_GetStringFromObj(objv[2],NULL));

    /* check for server optional parameter */
    if (objc > 3) {
	server_str = Tcl_GetStringFromObj(objv[3], &len);
	if (len == 0) {
	    server_str = NULL;
	}
    } else {
	server_str = NULL;
    }

    /* check for appname optional parameter */
    if (objc > 4) {
	appname_str = Tcl_GetStringFromObj(objv[4], &len);
	if (len > 0) {
            DBSETLAPP(login, appname_str);
	}
    }

    /* check for interface file optional parameter */
    if (objc > 5) {
        ifile_str = Tcl_GetStringFromObj(objv[5], &len);
	if (len > 0) {
            dbsetifile(ifile_str);
	} else {
            dbsetifile( NULL);
	}
    } else {
        dbsetifile( NULL);
    }

    /* check for charset specification optional parameter */
    if (objc > 6) {
        charset_str = Tcl_GetStringFromObj(objv[6], &len);
	if (len > 0) {
            DBSETLCHARSET(login, charset_str);
	} else {
            DBSETLCHARSET(login, NULL);
	}
    } else {
        DBSETLCHARSET(login, NULL);
    }


    SybProcs[hand].dbproc = dbopen(login, server_str); 

    dbloginfree(login);                     /* login struct no longer needed */

    if (SybProcs[hand].dbproc == NULL) {
	Sybtcl_AppendObjResult (interp, CMD_STR,
	   ": sybconnect failed in dbopen", (char *) NULL);
	return TCL_ERROR;
    }

    SybProcs[hand].in_use     = 1;	/* handle ok, set in use flag */
    SybProcs[hand].interp     = interp;

    if (SybProcs[hand].bufferedResult != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedResult);
	SybProcs[hand].bufferedResult = NULL;
    }
    if (SybProcs[hand].bufferedIsnull != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedIsnull);
	SybProcs[hand].bufferedIsnull = NULL;
    }
    remove_handler(i); 
    SybProcs[i].last_results   = NO_MORE_RESULTS;
    SybProcs[i].last_next      = NO_MORE_ROWS;
    SybProcs[i].async          = 0;
    SybProcs[i].last_text      = 32768;
    SybProcs[i].hasBgResults   = 0;
    SybProcs[i].in_event       = 0;
    SybProcs[i].bgResults      = 0;


    /* update sybmsg(usedhandles) */
    count_inuse(interp);

    /* construct handle and return */
    sprintf(buf, "%s%d", SybHandlePrefix, hand);

    SybProcs[hand].sybChan  = dbMakeChannel( DBIORDESC(SybProcs[hand].dbproc), 
					hand, buf );

    tmp_obj = Tcl_NewStringObj(buf,-1);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_SetObjResult(interp,tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_handle,  tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);

    tmp_obj = Tcl_NewStringObj("",0);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);

    clear_msg(interp);

    return TCL_OK;

}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Money --
 *    Implements the sybmoney command:
 *    usage: sybmoney operator arg arg
 *	                
 *    results:
 *	returns sybmoney result as float or char object, cmp as integer object
 *      TCL_OK - 
 *      TCL_ERROR - wrong # args or values cannot be converted to money values
 */

Sybtcl_Money (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    DBMONEY  m1;
    DBMONEY  m2;
    DBMONEY  result;
    char     *cmd;
    char     *s;
    Tcl_Obj *tmp_obj;
    char     buf[SYB_BUFF_SIZE];
    int      len;

    if (objc < 4) {
	Sybtcl_AppendObjResult (interp, "wrong # args: ", 
		CMD_STR, ": cmd money1 money2", 
		(char *) NULL);
	return TCL_ERROR;
    }

    s = Tcl_GetStringFromObj(objv[2],&len);	
    if (dbconvert(NULL, SYBCHAR, (BYTE *)s, len, SYBMONEY, (BYTE *) &m1, -1) 
		    != sizeof(DBMONEY) ) {
	Sybtcl_AppendObjResult (interp, CMD_STR, 
	    ": cannot convert arg1 value to sybase money type", 
	    (char *) NULL);
	return TCL_ERROR;
    }

    s = Tcl_GetStringFromObj(objv[3],&len);	
    if (dbconvert(NULL, SYBCHAR, (BYTE *)s, len, SYBMONEY, (BYTE *) &m2, -1) 
		    != sizeof(DBMONEY) ) {
	Sybtcl_AppendObjResult (interp, CMD_STR, 
	    ": cannot convert arg2 value to sybase money type", 
	    (char *) NULL);
	return TCL_ERROR;
    }

    cmd = Tcl_GetStringFromObj(objv[1],NULL);

    if (*cmd == 'a' && strncmp(cmd,"add",3) == 0) {
	if (dbmnyadd(NULL, &m1, &m2, &result) == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR, 
	    ": value overflow",(char *) NULL);
	    return TCL_ERROR;
	}
	dbconvert(NULL, SYBMONEY, (BYTE *) &result, sizeof(SYBMONEY),
		SYBCHAR, (BYTE *) buf, -1);
	tmp_obj = Tcl_NewStringObj(buf,-1);

    } else if (*cmd == 's' && strncmp(cmd,"sub",3) == 0) {
	if (dbmnysub(NULL, &m1, &m2, &result) == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
	    ": value overflow",(char *) NULL);
	    return TCL_ERROR;
	}
	dbconvert(NULL, SYBMONEY, (BYTE *) &result, sizeof(SYBMONEY),
		SYBCHAR, (BYTE *) buf, -1);
	tmp_obj = Tcl_NewStringObj(buf,-1);

    } else if (*cmd == 'm' && strncmp(cmd,"mul",3) == 0) {
	if (dbmnymul(NULL, &m1, &m2, &result) == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
	    ": value overflow",(char *) NULL);
	    return TCL_ERROR;
	}
	dbconvert(NULL, SYBMONEY, (BYTE *) &result, sizeof(SYBMONEY),
		SYBCHAR, (BYTE *) buf, -1);
	tmp_obj = Tcl_NewStringObj(buf,-1);

    } else if (*cmd == 'd' && strncmp(cmd,"div",3) == 0) {
	if (dbmnydivide(NULL, &m1, &m2, &result) == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
	    ": value overflow or divide by zero",(char *) NULL);
	    return TCL_ERROR;
	}
	dbconvert(NULL, SYBMONEY, (BYTE *) &result, sizeof(SYBMONEY),
		SYBCHAR, (BYTE *) buf, -1);
	tmp_obj = Tcl_NewStringObj(buf,-1);

    } else if (*cmd == 'c' && strncmp(cmd,"cmp",3) == 0) {
	tmp_obj = Tcl_NewIntObj(dbmnycmp(NULL, &m1, &m2));

    } else {
	Sybtcl_AppendObjResult (interp, CMD_STR,
		": cmd must be one of 'add', 'sub', 'mul', 'div', 'cmp' ",
		(char *) NULL);
	return TCL_ERROR;
    }
    
    Tcl_IncrRefCount(tmp_obj);
    Tcl_SetObjResult(interp, tmp_obj);
    Tcl_DecrRefCount(tmp_obj);

    return TCL_OK;
}


    

/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Use --
 *    Implements the sybuse command:
 *    usage: sybuse handle ?dbname?
 *	                
 *    results:
 *	returns name of current database, or sets current database to dbname
 *      TCL_OK - handle is opened, "SUCCEED","FAIL"
 *      TCL_ERROR - wrong # args, or handle not opened
 */

Sybtcl_Use (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;
    RETCODE dbret;

    if ((hand = syb_prologue(interp, objc, objv, 2," handle ?dbname?")) == -1) {
	return TCL_ERROR;
    }

    remove_handler(hand);
    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;
    SybProcs[hand].async        = 0;
    SybProcs[hand].in_event     = 0;
    SybProcs[hand].hasBgResults = 0;
    SybProcs[hand].bgResults    = 0;
    if (SybProcs[hand].bufferedResult != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedResult);
	SybProcs[hand].bufferedResult = NULL;
    }
    if (SybProcs[hand].bufferedIsnull != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedIsnull);
	SybProcs[hand].bufferedIsnull = NULL;
    }

    if (objc > 2 ) {
	dbret = dbuse(SybProcs[hand].dbproc,Tcl_GetStringFromObj(objv[2],NULL));
	if (dbret == SUCCEED) {
	  Tcl_SetObjResult(interp,objv[2]);
	} else {
	  Tcl_SetObjResult(interp,
			Tcl_NewStringObj("sybuse: database cannot be used",-1));
	  return TCL_ERROR;
	}
    } else {
	Tcl_SetObjResult(interp,
			Tcl_NewStringObj(dbname(SybProcs[hand].dbproc),-1));
    }

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Sql --
 *    Implements the sybsql command:
 *    usage: sybsql handle sql-string ?-async?"
 *	                
 *    results:
 *	"REG_ROW" if rows returned, "NO_MORE_ROWS" if no rows returned
 *	"PENDING"  if executed with "-async"
 *      TCL_OK - handle is opened, sql executed ok
 *      TCL_ERROR - wrong # args, or handle not opened,  bad sql stmt
 */

Sybtcl_Sql (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int      hand;
    RETCODE  dbret;
    char     buf[SYB_BUFF_SIZE];
    char     conv_buf[20];
    char    *s;
    Tcl_Obj *tmp_obj;
    int      i;


    if ((hand = syb_prologue(interp,objc, objv, 3, 
		" handle sql_str ?-async?")) == -1) {
	return TCL_ERROR;
    }

    /* remove call back handler and free callback script, if any */
    remove_handler(hand);

    /* cancel any pending results */
    cancel_prev(hand);
    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;
    SybProcs[hand].in_event     = 0;
    SybProcs[hand].hasBgResults = 0;
    SybProcs[hand].bgResults    = 0;

    if (SybProcs[hand].bufferedResult != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedResult);
	SybProcs[hand].bufferedResult = NULL;
    }
    if (SybProcs[hand].bufferedIsnull != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedIsnull);
	SybProcs[hand].bufferedIsnull = NULL;
    }
    SybProcs[hand].async        = 0;

    if (objc > 3) {
        s = Tcl_GetStringFromObj(objv[3],NULL);
        if ((strncmp(s,"-async",6) == 0) || (strncmp(s,"async",5) == 0) ) {
            SybProcs[hand].async = 1;
	}
    }

    /* find maxtext option for this interp */
    i = get_syb_option(interp);
    if (i == -1) {
	Sybtcl_AppendObjResult (interp, CMD_STR, ": interp options not found ",
					(char *) NULL);
	return TCL_ERROR;
    }

    /* set server text size to value of maxtext */
    if (SybProcs[hand].last_text != SybOptions[i].maxtext) {
	SybProcs[hand].last_text = SybOptions[i].maxtext;
	sprintf(conv_buf,"%d",SybOptions[i].maxtext);
	dbsetopt(SybProcs[hand].dbproc, DBTEXTSIZE, conv_buf, -1);
	dbsqlexec(SybProcs[hand].dbproc);  /* execute dbsetopt() */
	cancel_prev(hand);                 /* and ignore results */
	clear_msg(interp);		   /* and flush out messages */
    }

    /* set up sql for execution */
    dbret = dbcmd(SybProcs[hand].dbproc, Tcl_GetStringFromObj(objv[2],NULL));

    if (dbret == FAIL) {
	Sybtcl_AppendObjResult (interp, CMD_STR, ": dbcmd failed ", 
							(char *) NULL);
	return TCL_ERROR;
    }

    /* send sql to server and get return code */
    SybProcs[hand].last_results = PENDING;
    if (SybProcs[hand].async == 1) {
        dbret = dbsqlsend(SybProcs[hand].dbproc);
    } else {
	dbret = dbsqlsend(SybProcs[hand].dbproc);
	events_waiting(hand,0);
        dbret = dbsqlok(SybProcs[hand].dbproc);
    }

    if (dbret == FAIL) {
	Sybtcl_AppendObjResult (interp, CMD_STR,
		": dbsqlexec failed ", (char *) NULL);
	return TCL_ERROR;
    }

    /* reset flags to worst case */

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;

    /* if async, return PENDING */ 
    if (SybProcs[hand].async == 1) {
        /* set msg array variable "nextrow" to pending status */
	tmp_obj = Tcl_NewStringObj("PENDING",-1);
	SybProcs[hand].last_results = PENDING;
	Tcl_IncrRefCount(tmp_obj);
        Tcl_SetObjResult(interp, tmp_obj);
        Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,tmp_obj,TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(tmp_obj);
        return TCL_OK;
    } 

    /* process first dbresults */
    dbret = dbresults(SybProcs[hand].dbproc);

    if (dbret == FAIL) {
	Sybtcl_AppendObjResult (interp, CMD_STR,
	    ": dbresults failed ", (char *) NULL);
	return TCL_ERROR;
    }

    /* save dbresults return code */
    SybProcs[hand].last_results = dbret;

    if (dbret == NO_MORE_RESULTS) {
        /* If there were no result sets, there may be a SP return code */
        /* NOTE: sybase docs say that dbhasretstat/dbretstatus should only */
	/* be called after all results have been processed... */
        if (dbhasretstat(SybProcs[hand].dbproc) == TRUE) {
            tmp_obj = Tcl_NewIntObj(dbretstatus(SybProcs[hand].dbproc));
            Tcl_IncrRefCount(tmp_obj);
            Tcl_ObjSetVar2(interp, SybMsgArray, SM_retstatus, 
                    tmp_obj, TCL_GLOBAL_ONLY);
            Tcl_DecrRefCount(tmp_obj);
        }
	strcpy(buf,"NO_MORE_ROWS"); /* yes, return this,not NO_MORE_RESULTS */
	SybProcs[hand].last_next    = NO_MORE_ROWS;

    } else {
	/* dbresults() returned SUCCEED, check for any rows */
	if (DBROWS(SybProcs[hand].dbproc) == FAIL) {
	    strcpy(buf,"NO_MORE_ROWS");
	    SybProcs[hand].last_next = NO_MORE_ROWS;
	} else {
	    /* DBROW returned SUCCEED, there must rows */
	    strcpy(buf,"REG_ROW");
	    SybProcs[hand].last_next = REG_ROW;
	}
    }


    /* set msg array variable "nextrow" to final status */
    tmp_obj = Tcl_NewStringObj(buf,-1);
    Tcl_IncrRefCount(tmp_obj);
    Tcl_SetObjResult(interp, tmp_obj);
    Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow, tmp_obj, TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(tmp_obj);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Poll --
 *    Implements the sybpoll command:
 *    usage: sybpoll handle ?timeout? ?-all?
 *	                
 *    results:
 *      null string - requested async proc or all procs not ready,
 *      list of async procs ready
 *      TCL_OK - handle is opened
 *      TCL_ERROR - wrong # args, or handle not opened
 */

Sybtcl_Poll (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int         hand;
    DBPROCESS  *readyproc, *readyproc_all;
    RETCODE     dbret, dbret_all;
    int         reason, reason_all;
    int         all;
    int         i;
    long        millisec;
    char        buf[200];
    char       *s;
    Tcl_Obj    *resultPtr;

    if ((hand = syb_prologue(interp, objc, objv, 2, 
		" handle ?timeout-ms? ?-all?")) == -1) {
	return TCL_ERROR;
    }

    if (objc > 2) {
	if (Tcl_GetLongFromObj(NULL, objv[2], &millisec) == TCL_OK) {
	    if (millisec < -1L) {
	        millisec = -1L;
	    }
	} else {
	    millisec = 0L;
	}
    } else {
	millisec = 0L;
    }

    if (objc > 3) {
        s = Tcl_GetStringFromObj(objv[3],NULL);
        if ((strncmp(s,"all",3) == 0) || (strncmp(s,"-all",4) == 0) ) {
	    all = 1;
	}
    } else {
	all = 0;
    }


    resultPtr = Tcl_GetObjResult (interp);

    /* if "all" requested, poll all other procs */
    if (all) {
	/* call dbpoll with user requested timeout for all (NULL) */
	dbret_all = dbpoll((DBPROCESS *) NULL, millisec, &readyproc_all,  
				&reason_all);
	/* now check through all procs */
	for (i = 0; i < SYBTCLPROCS; i++) {
	    if ((SybProcs[i].in_use == 1) && (SybProcs[i].async == 1)) {
		if (readyproc_all == SybProcs[i].dbproc) {
		    /* use previous dbpoll values to avoid second dbpoll */
		    dbret     = dbret_all;
		    readyproc = readyproc_all;
		    reason    = reason_all;
		} else {
		    dbret = dbpoll(SybProcs[i].dbproc, 0L, &readyproc,  
					       &reason);
		}
		if (dbret == SUCCEED && reason == DBRESULT && 
				     readyproc == SybProcs[i].dbproc) {
		    sprintf(buf,"%s%d",SybHandlePrefix,i);
		    Tcl_ListObjAppendElement(NULL, resultPtr, 
			Tcl_NewStringObj(buf,-1));
		}
	    }
	}
    } else {
	if (SybProcs[hand].async == 1) {
	    dbret = dbpoll(SybProcs[hand].dbproc, millisec, &readyproc,  
						  &reason);
	    if (dbret == SUCCEED && reason == DBRESULT && 
				   readyproc == SybProcs[hand].dbproc) {
		sprintf(buf,"%s%d",SybHandlePrefix,hand);
		Tcl_ListObjAppendElement(NULL, resultPtr,
			Tcl_NewStringObj(buf,-1));
	    }
	}
    }

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_NextAll --
 *    Implements the sybnext with iterative commands:
 *    hacked from Oratcl 2.1, nearly verbatim
 *    usage: sybnext cur_handle tcl_stmts sub_char
 *	                
 *    results:
 *	fetch all rows from existing query, execute tcl_stmt on each row
 *	sets message array element "rc" with value of final fetch rcode
 *      TCL_OK - handle is opened
 *      TCL_ERROR - wrong # args, or handle not opened, tcl_stmt failed,
 *                  or wrong number of columns
 */

static int
Sybtcl_NextAll (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int      hand;
    int      i;
    char    *s;
    char    *s2;
    char    *p;
    int      max_col = 0;	/* largest @col in command string */
    int      max_var = 0;       /* largest tclvar colnum          */
    int      prev_result;

#define NUMSTRS 1024		/* number of individual strings to allow*/
#define SUBCHAR '@'		/* default substitution character */

    char     subchar = SUBCHAR;	/* substitution character */

    int      num_str = 0;	/* number of strings to concat */
    int      colnum  = 0; 	/* number of colstr's used */
    int      colidx[NUMSTRS];   /* column number to concat at each point */
    Tcl_Obj *cmd_obj = NULL;	/* list to hold parsed command string */
    Tcl_Obj *eval_obj = NULL;	/* object used to assemble command string */
    Tcl_Obj *resultPtr = NULL;	/* result of sybnext, e.g. db columns */
    Tcl_Obj *tmp_obj;
    Tcl_Obj *varname_obj;
    Tcl_DString evalStr;
    int      parm_cnt;
    int      icol;
    int      inum;
    int      tcl_rc;
    int      id;
    int      colsCnt = 0;	/* number of columns in result from sybnext   */
    int      can_sub = 0;    	/* if subchar is defined & substitutions made */
    int      has_varcol = 0;	/* if command has varname column pairs        */
    int      varcol_list = 0;	/* if varname column pairs are list           */
    int      var_cnt = 0;	/* number of varname column pairs             */


    if ((hand = syb_prologue(interp, objc, objv, 3, 
	 " cursor_handle  commands  ?sub_char? ? [ tclvar colnum ] ...?")) 
	                                                              == -1) {
	return TCL_ERROR;
    }

    /* check if already exahusted or nothing to fetch */
    /* except if sql was submitted async */

    if (SybProcs[hand].last_results == NO_MORE_RESULTS &&
	SybProcs[hand].async == 0) {
	/* sybnext return  NO_MORE_RESULTS last time, tell'm again! */
	tmp_obj = Tcl_NewStringObj("NO_MORE_RESULTS",-1);
	Tcl_IncrRefCount(tmp_obj);
	Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,tmp_obj,TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(tmp_obj);
        SybProcs[hand].last_next = NO_MORE_ROWS;

	return TCL_OK;
    }
	
    /* check for user defined substitution character */

    if (objc > 3) {
	s = Tcl_GetStringFromObj(objv[3], &i);
	if (i > 0) {
	    subchar = *s;
	} else {
	    subchar = '\0';
	}
	/* don't let subchar be a space */
	if (subchar == ' ') {
	    subchar = '\0';
	}
    }
    if (subchar != '\0') {
	can_sub = 1;
    }

    /* get the command string */
    s = Tcl_GetStringFromObj(objv[2], NULL);

    /* parse tcl_stmt for all '@' (or whatever subchar)  substitutions */

    if (can_sub) {

        s2 = s;
	cmd_obj = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(cmd_obj);

	while ( (p = strchr(s2,subchar)) != NULL) {
	    if (num_str >= NUMSTRS || colnum >= NUMSTRS) {
		Sybtcl_AppendObjResult (interp, CMD_STR,
			      ": too many column substitutions, 300 max",
				(char *) NULL);
		Tcl_DecrRefCount(cmd_obj);
		return TCL_ERROR;
	    }

	    if (isdigit(*(p+1))) {	/* it's a substitution value ! */
		tmp_obj = Tcl_NewStringObj(s2, (p - s2));
		Tcl_IncrRefCount(tmp_obj);
		Tcl_ListObjAppendElement(NULL, cmd_obj, tmp_obj);
		Tcl_DecrRefCount(tmp_obj);

		i = strtoul(p + 1, &s2, 10);	/* the column number */

		/* collect highest column number used */
		max_col = (i > max_col) ? i : max_col;
		colidx[num_str] = i;
		num_str++;

	    } else {			/* it's a subchar without number */
		tmp_obj = Tcl_NewStringObj(s2, (p - s2 + 1));
		Tcl_IncrRefCount(tmp_obj);
		Tcl_ListObjAppendElement(NULL, cmd_obj, tmp_obj);
		Tcl_DecrRefCount(tmp_obj);
		colidx[num_str] = -1; /* no column to sub */
		num_str++;
		s2 = p + 1;		
		if (*s2 == subchar) {	/* another subchar? it's quoted  */
		    s2++;
		}
	    }
        }

	if (num_str > 0) {
	    /* add last piece of cmd string */
	    tmp_obj = Tcl_NewStringObj(s2, -1);
	    Tcl_IncrRefCount(tmp_obj);
	    Tcl_ListObjAppendElement(NULL, cmd_obj, tmp_obj);
	    Tcl_DecrRefCount(tmp_obj);
	} else {
	    /* no substitutions made, buid tcl obj from argument */
	    Tcl_DecrRefCount(cmd_obj);
	    cmd_obj = NULL;
	    can_sub = 0;
	    eval_obj = Tcl_DuplicateObj(objv[2]);
	    Tcl_IncrRefCount(eval_obj);
	}

    } else {
        /* no substitutions, build tcl obj from argument */
	eval_obj = Tcl_DuplicateObj(objv[2]);
	Tcl_IncrRefCount(eval_obj);
    }

    /* check if varname column pair is a list, or are argument elements */
    if (objc > 4) {
	has_varcol = 1;
	if (objc == 5) {
	    if (Tcl_ListObjLength(NULL, objv[4], &var_cnt) == TCL_OK) {
		if (var_cnt % 2 == 0) {
		    varcol_list = 1;
		} else {
		    Sybtcl_AppendObjResult (interp, ": ", CMD_STR,
			    ": odd number of varname column elements",
			    (char *) NULL);
		    goto errorExit;
		}
	    }

	} else {
	    var_cnt = (objc - 4) > 0 ? (objc - 4) : 0;
	    if (var_cnt % 2 != 0) {
		Sybtcl_AppendObjResult (interp, ": ", CMD_STR,
			": odd number of varname column elements",
			(char *) NULL);
		goto errorExit;
	    }
	}
    }

    /* check that varname column pairs are integer columns */
    if (var_cnt > 0) {
	parm_cnt = 0;
	while (var_cnt > parm_cnt + 1) {      
	    if (varcol_list) {
		Tcl_ListObjIndex(NULL, objv[4], parm_cnt, &tmp_obj);
		p = Tcl_GetStringFromObj(tmp_obj, NULL);
		Tcl_ListObjIndex(NULL, objv[4], parm_cnt + 1, &tmp_obj);
		i = Tcl_GetIntFromObj(NULL, tmp_obj, &icol);
	    } else {
		p = Tcl_GetStringFromObj(objv[parm_cnt + 4], NULL);
	        i = Tcl_GetIntFromObj(NULL, objv[parm_cnt + 4 + 1], &icol);
	    }
	    if (i == TCL_ERROR) {
		Sybtcl_AppendObjResult (interp, ": ", CMD_STR,
		      ": column specifier for variable ", p, 
		      " is not an integer ",
		      (char *) NULL);
		goto errorExit;

	    }
	    max_var = (icol > max_var) ? icol : max_var;
	    parm_cnt += 2;
	}
    }


    /* loop until fetch exhausted */

    /* Version 3.0b<4 tried to process pending Tcl events here while */
    /* waiting for data to arrive, but Sybase's DB-Lib documentation */
    /* (v. 11.1.x) EXPLICITLY says that it's a bad idea to call dbpoll */
    /* after dbsqlok while processing results with dbnextrow, so */
    /* unfortunately we cannot check for events here.  We need CT-Lib */
    /* to get read asynch row processing... */

    if (Sybtcl_Next(clientData, interp, 2, objv) == TCL_ERROR) {
	Sybtcl_AppendObjResult (interp, ": ", CMD_STR,
		": sybnext failed", (char *) NULL);
	goto errorExit;
    }
    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
        if (SybProcs[hand].last_results == NO_MORE_RESULTS) {
	    tmp_obj = Tcl_NewStringObj("NO_MORE_RESULTS",-1);
	    Tcl_IncrRefCount(tmp_obj);
	    Tcl_ObjSetVar2(interp, SybMsgArray,SM_nextrow,tmp_obj, 
							TCL_GLOBAL_ONLY);
	    Tcl_DecrRefCount(tmp_obj);
	} else {
	    tmp_obj = Tcl_NewStringObj("NO_MORE_ROWS",-1);
	    Tcl_IncrRefCount(tmp_obj);
	    Tcl_ObjSetVar2(interp, SybMsgArray,SM_nextrow,tmp_obj,
							TCL_GLOBAL_ONLY);
	    Tcl_DecrRefCount(tmp_obj);
	}
	goto okExit;
    }

    /* make sure there are enough columns in result */
    i = 0;

    /* check for a regular row, or a compute row */
    if (SybProcs[hand].last_next == REG_ROW) {
	i = dbnumcols(SybProcs[hand].dbproc);
    } else {
	id = SybProcs[hand].last_next;  /* the compute row id */
	i = dbnumalts(SybProcs[hand].dbproc,id);
    }

    prev_result = SybProcs[hand].last_next;

    if (max_col > i || max_var > i) {
	Sybtcl_AppendObjResult (interp, CMD_STR,
            ": @column number or tclvar column number", 
	    " execeeds number of result columns", (char *) NULL);
	goto errorExit;
    }

    while (SybProcs[hand].last_next != NO_MORE_ROWS) {

	resultPtr = Tcl_GetObjResult(interp);

	Tcl_ListObjLength(NULL, resultPtr, &colsCnt);

	/* if crossing result sets, e.g. REG_ROW to a compute row    */
	/* then buffer the result and return from NextAll processing */

        if (prev_result != SybProcs[hand].last_next) {
	    if (SybProcs[hand].bufferedResult != NULL) {
		Tcl_DecrRefCount(SybProcs[hand].bufferedResult);
                SybProcs[hand].bufferedResult  = NULL;
	    }
	    if (SybProcs[hand].bufferedIsnull != NULL) {
		Tcl_DecrRefCount(SybProcs[hand].bufferedIsnull);
		SybProcs[hand].bufferedIsnull  = NULL;
	    }
	    SybProcs[hand].bufferedResult  = resultPtr;
	    Tcl_IncrRefCount(SybProcs[hand].bufferedResult);
	    SybProcs[hand].bufferedIsnull = Tcl_ObjGetVar2(interp, 
		    SybMsgArray, SM_isnull, TCL_GLOBAL_ONLY);
	    if (SybProcs[hand].bufferedIsnull != NULL) {
		Tcl_IncrRefCount(SybProcs[hand].bufferedIsnull);
		Tcl_ObjSetVar2(interp,SybMsgArray, SM_isnull,
			    Tcl_NewStringObj("",-1), TCL_GLOBAL_ONLY);
	    } else {
		SybProcs[hand].bufferedIsnull = Tcl_NewStringObj("",-1);
		Tcl_IncrRefCount(SybProcs[hand].bufferedIsnull);
	    }
	    Tcl_ResetResult(interp);
	    goto okExit;
	}

        /* build a new eval string from literals and columns */
	if (can_sub) {
	    if (eval_obj != NULL) {
	       Tcl_DecrRefCount(eval_obj);
	    }
	    Tcl_DStringInit(&evalStr);
	    for (inum=0; inum < num_str; inum++) {
		Tcl_ListObjIndex(NULL, cmd_obj, inum, &tmp_obj);
		p = Tcl_GetStringFromObj(tmp_obj, &i);
		Tcl_DStringAppend(&evalStr, p, i);
                
		if (colidx[inum] == 0) {
		    /* get entire result list */
		    p = Tcl_GetStringFromObj(resultPtr, &i);
		    Tcl_DStringAppendElement(&evalStr, p);
		    Tcl_DStringAppend(&evalStr, " ", 1);
		} else if (colidx[inum] > 0) {
		    /* get one column element */
		    Tcl_ListObjIndex(NULL, resultPtr, colidx[inum]-1, &tmp_obj);
		    p = Tcl_GetStringFromObj(tmp_obj, &i);
		    Tcl_DStringAppendElement(&evalStr, p);
		    Tcl_DStringAppend(&evalStr, " ", 1);
		}
	    }
	    /* tack on remainder of command string */
	    Tcl_ListObjIndex(NULL, cmd_obj, num_str, &tmp_obj);
	    p = Tcl_GetStringFromObj(tmp_obj, &i);
	    Tcl_DStringAppend(&evalStr, p, i);
	    eval_obj = Tcl_NewStringObj(Tcl_DStringValue(&evalStr),-1);
	    Tcl_IncrRefCount(eval_obj);
	    Tcl_DStringFree(&evalStr);
	} 

	/* set any remaining "tclvar colnum" pairs as tcl variables */
	if (has_varcol) {
	    parm_cnt = 0;
	    while (var_cnt > parm_cnt + 1) {
		/* get from list or argument elements */
		if (varcol_list) {
		    Tcl_ListObjIndex(NULL, objv[4], parm_cnt, &varname_obj);
		    Tcl_ListObjIndex(NULL, objv[4], parm_cnt + 1, &tmp_obj);
		    Tcl_GetIntFromObj(NULL, tmp_obj, &icol);
		} else {
		    varname_obj = objv[parm_cnt + 4];
		    Tcl_GetIntFromObj(NULL, objv[parm_cnt + 4 + 1], &icol);
		}
		
		if (icol == 0) {
		    /* row as list */
		    Tcl_IncrRefCount(resultPtr);
		    Tcl_ObjSetVar2(interp, varname_obj, NULL, resultPtr, 
							TCL_PARSE_PART1); 
		    Tcl_DecrRefCount(resultPtr);
		} else {
		    /* individual column element */
		    Tcl_ListObjIndex(NULL, resultPtr, icol-1, &tmp_obj);
		    Tcl_IncrRefCount(tmp_obj);
		    Tcl_ObjSetVar2(interp, varname_obj, NULL, tmp_obj,
						    	TCL_PARSE_PART1); 
		    Tcl_DecrRefCount(tmp_obj);
		}
		parm_cnt += 2;
	    }

	}

        tcl_rc = Tcl_EvalObj(interp, eval_obj);	/* do it! */

	switch (tcl_rc) {		/* check on eval return code */
	  case TCL_ERROR:
	    Sybtcl_AppendObjResult (interp, ": eval failed while in ", 
		           CMD_STR, (char *) NULL);
	    goto errorExit;
	    break;

	  case TCL_BREAK:		/* return sooner */
	    goto okExit;
	    break;

	  default:
	    break;
	}

	Tcl_ResetResult(interp);	/* reset interp result for next */

        /* next fetch */

        /* Versions 3.0b<4 tried to process Tcl events here but that */
        /* turns out the be a bad idea with DB-Lib 11.1.x per the */
        /* comment above. */

        if (Sybtcl_Next(clientData, interp, 2, objv) == TCL_ERROR) {
	    Sybtcl_AppendObjResult (interp, ": ", CMD_STR,
	       				": sybnext failed", (char *) NULL);
	    goto errorExit;
        }

    }

    okExit:

	if (cmd_obj != NULL) {
	    Tcl_DecrRefCount(cmd_obj);
	}
	if (eval_obj != NULL) {
	    Tcl_DecrRefCount(eval_obj);
	}
	return TCL_OK;

    errorExit:

	if (cmd_obj != NULL) {
	    Tcl_DecrRefCount(cmd_obj);
	}
	if (eval_obj != NULL) {
	    Tcl_DecrRefCount(eval_obj);
	}
	return TCL_ERROR;

}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Next --
 *    Implements the sybnext command:
 *    usage: sybnext handle ?commands? ?sub_char? ?tclvar colnum ...?
 *	                
 *    results:
 *	next row from latest results as tcl list, or null list
 *	sets message array element "nextrow" with value of dbnextrow/dbresults
 *      TCL_OK - handle is opened
 *      TCL_ERROR - wrong # args, or handle not opened
 */

Sybtcl_Next (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;
    RETCODE dbret;
    char    buf[SYB_BUFF_SIZE];
    int     i;
    int     num_cols;
    int     col_type;		/* column type */
    int     col_len;		/* column data length */
    int     col_len2;		/* column max length */
    BYTE   *col_ptr;
    Tcl_Obj *tmp_obj;
    int     iptr_idx;
    Tcl_Obj *isnull_list;


    if ((hand = syb_prologue(interp, objc, objv, 2, 
	       " handle ?commands? ?subchar? ?tclvar colnum ...?")) == -1) {
	return TCL_ERROR;
    }

    /* if commands argument, then call Sybtcl_NextAll function to do work */
    if (objc > 2) {
	return (Sybtcl_NextAll(clientData, interp, objc, objv));
    }

    /* check to see if any results where buffered from NextAll */

    if (SybProcs[hand].bufferedResult != NULL) {
        Tcl_SetObjResult(interp, SybProcs[hand].bufferedResult);
        Tcl_DecrRefCount(SybProcs[hand].bufferedResult);
        SybProcs[hand].bufferedResult  = NULL;
	if (SybProcs[hand].last_next == REG_ROW) {
	    tmp_obj = Tcl_NewStringObj("REG_ROW",-1);
	} else {
	    /* it's a compute row number */
	    tmp_obj = Tcl_NewIntObj(SybProcs[hand].last_next);
	}

	Tcl_IncrRefCount(tmp_obj);
	Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,tmp_obj,TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(tmp_obj);

	Tcl_IncrRefCount(SybProcs[hand].bufferedIsnull);
	Tcl_ObjSetVar2(interp, SybMsgArray, SM_isnull,
			SybProcs[hand].bufferedIsnull,TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(SybProcs[hand].bufferedIsnull);
	SybProcs[hand].bufferedIsnull = NULL;
	return TCL_OK;
    }

    if (SybProcs[hand].async == 1) {
	/* if event handler not active, then process Tcl events while waiting */
	/* except on macintosh, where too many dbpoll() call gets it confused */
#ifndef MAC_TCL
	if (SybProcs[hand].callBackScript == NULL) {
	    events_waiting(hand,0);
	}
#endif
	dbret = dbsqlok(SybProcs[hand].dbproc);
	if (dbret == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
	        ": dbsqlok failed ", (char *) NULL);
            return TCL_ERROR;

	}
        SybProcs[hand].last_next    = NO_MORE_ROWS;
	SybProcs[hand].last_results = SUCCEED;
	SybProcs[hand].async        = 0;
    }

    /* check to see if next set of results */

    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
	if (SybProcs[hand].last_results == SUCCEED) {

	    /* if dbresults() called during event callback, use that value */
	    if (SybProcs[hand].hasBgResults) {
		dbret = SybProcs[hand].bgResults;
		SybProcs[hand].hasBgResults = 0;
		SybProcs[hand].bgResults    = 0;
	    } else {
	        dbret = dbresults(SybProcs[hand].dbproc);
	    }

	    /* save dbresults return code */
	    SybProcs[hand].last_results = dbret;

	    if (dbret == FAIL) {
		tmp_obj = Tcl_NewStringObj("FAIL",-1);
		Tcl_IncrRefCount(tmp_obj);
		Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,
			tmp_obj, TCL_GLOBAL_ONLY);
		Tcl_DecrRefCount(tmp_obj);
		SybProcs[hand].last_next = NO_MORE_ROWS;

		return TCL_OK;   /* null string returned */
	    }

	    if (dbhasretstat(SybProcs[hand].dbproc) == TRUE) {
		tmp_obj =Tcl_NewIntObj(dbretstatus(SybProcs[hand].dbproc));
		Tcl_IncrRefCount(tmp_obj);
		Tcl_ObjSetVar2(interp, SybMsgArray, SM_retstatus, 
			tmp_obj, TCL_GLOBAL_ONLY);
		Tcl_DecrRefCount(tmp_obj);
	    }

	    if (dbret == NO_MORE_RESULTS) {
		tmp_obj = Tcl_NewStringObj("NO_MORE_RESULTS",-1);
		Tcl_IncrRefCount(tmp_obj);
		Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,
			tmp_obj, TCL_GLOBAL_ONLY);
		Tcl_DecrRefCount(tmp_obj);
		SybProcs[hand].last_next = NO_MORE_ROWS;

		return TCL_OK;   /* null string returned */
	    } else {
	        /* dbresults() returned SUCCEED, check for any rows */
		if (DBROWS(SybProcs[hand].dbproc) == FAIL) {
		    /* previous set of results exhausted, but there are no   */
		    /* rows in the new set, could be procedure return values */
		    tmp_obj = Tcl_NewStringObj("NO_MORE_ROWS",-1);
		    Tcl_IncrRefCount(tmp_obj);
		    Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,
				 tmp_obj, TCL_GLOBAL_ONLY);
		    Tcl_DecrRefCount(tmp_obj);
		    SybProcs[hand].last_next = NO_MORE_ROWS;

		    return TCL_OK;   /* null string returned */
		}

		/* DBROWS returned SUCCEED */
		/* from here, just fall through to the outer context */
		/* and call dbnextrow on this new set of results */


	    }

	} else {
	    /* sybnext returned  NO_MORE_RESULTS last time, tell'm again! */
	    tmp_obj = Tcl_NewStringObj("NO_MORE_RESULTS",-1);
	    Tcl_IncrRefCount(tmp_obj);
	    Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,
			tmp_obj, TCL_GLOBAL_ONLY);
	    Tcl_DecrRefCount(tmp_obj);
	    SybProcs[hand].last_next = NO_MORE_ROWS;
	    return TCL_OK;   /* null string returned */
	}
    }



    dbret = dbnextrow(SybProcs[hand].dbproc);

    SybProcs[hand].last_next = dbret;

    if (dbret == NO_MORE_ROWS) {
	tmp_obj = Tcl_NewStringObj("NO_MORE_ROWS",-1);
	Tcl_IncrRefCount(tmp_obj);
	Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow,tmp_obj,TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(tmp_obj);
	if (dbhasretstat(SybProcs[hand].dbproc) == TRUE) {
	    tmp_obj = Tcl_NewIntObj(dbretstatus(SybProcs[hand].dbproc));
	    Tcl_IncrRefCount(tmp_obj);
	    Tcl_ObjSetVar2(interp,SybMsgArray, SM_retstatus, 
		tmp_obj, TCL_GLOBAL_ONLY);
	    Tcl_DecrRefCount(tmp_obj);
	}
	return TCL_OK;
    }

    /* process regular rows or a compute row (where dbret > 0) */

    if ((dbret == REG_ROW) || (dbret > 0)) {
	/* set the "nextrow" message, and get the number of columns */
	if (dbret == REG_ROW) {
	    tmp_obj = Tcl_NewStringObj("REG_ROW",-1);
	    Tcl_IncrRefCount(tmp_obj);
	    num_cols = dbnumcols(SybProcs[hand].dbproc);
	} else {
	    tmp_obj = Tcl_NewIntObj(dbret); /* the compute id as integer */
	    Tcl_IncrRefCount(tmp_obj);
	    num_cols = dbnumalts(SybProcs[hand].dbproc,dbret);
	}
	Tcl_ObjSetVar2(interp, SybMsgArray, SM_nextrow, 
		tmp_obj, TCL_GLOBAL_ONLY);
        Tcl_DecrRefCount(tmp_obj);

        /* find the interp option structure */
	iptr_idx = get_syb_option(interp);
	if (iptr_idx == -1) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
			      ": panic: can't find interp options",
				(char *) NULL);
	    return (TCL_ERROR);
	}

	isnull_list = Tcl_NewListObj(0,NULL);
	Tcl_IncrRefCount(isnull_list);

	/* parse the columns */

	for (i = 1; i <= num_cols; i++) {

            /* use normal row data access or alternate compute row access */
	    if (dbret == REG_ROW) {
		col_type = dbcoltype(SybProcs[hand].dbproc,i);
		col_len  = dbdatlen(SybProcs[hand].dbproc,i);
		col_len2 = dbcollen(SybProcs[hand].dbproc,i);
		col_ptr  = dbdata(SybProcs[hand].dbproc,i);
	    } else {
		col_type = dbalttype(SybProcs[hand].dbproc,dbret,i);
		col_len  = dbadlen(SybProcs[hand].dbproc,dbret,i);
		col_len2 = col_len;
		col_ptr  = dbadata(SybProcs[hand].dbproc,dbret,i);
	    }

	    /* call parse_column to append to result */
	    if (parse_column(interp, hand, col_type, col_len, col_len2, 
				col_ptr, iptr_idx, isnull_list) == 0) {

	        Tcl_DecrRefCount(isnull_list);
		remove_handler(hand);
		return TCL_ERROR;
	    }

	}

	Tcl_ObjSetVar2(interp, SybMsgArray, SM_isnull, isnull_list, 
				        TCL_GLOBAL_ONLY);
	Tcl_DecrRefCount(isnull_list);

	return TCL_OK;
    }



    /* if we get here, then the nextrow result was wacky, bail out */    
    sprintf(buf,"%d",dbret);
    Sybtcl_AppendObjResult (interp, CMD_STR,
		 ": handle ", Tcl_GetStringFromObj(objv[1],NULL),
		     " had bad results from dbnextrow, return code = ", buf, 
		     (char *) NULL);
    return TCL_ERROR;

}




/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Cols --
 *    Implements the sybcols command:
 *    usage: sybcols handle 
 *	                
 *    results:
 *	latest column names as tcl list, or param columns, or null list
 *      also set sybmsg(collengths) and sybmsg(coltypes) as tcl list
 *      TCL_OK - handle is opened, and at least one sybnext executed 
 *      TCL_ERROR - wrong # args, or handle not opened,
 */

Sybtcl_Cols (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;
    char   *bufptr;
    int     num_cols;
    int     id;
    int     i;
    char    buf2[SYB_BUFF_SIZE];
    Tcl_Obj *len_obj;
    Tcl_Obj *typ_obj;
    Tcl_Obj *col_obj;
    Tcl_Obj *tmp_obj;

    if ((hand = syb_prologue(interp, objc, objv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }


    if (SybProcs[hand].last_results == SUCCEED) {

        len_obj = Tcl_NewListObj(0, NULL);
        typ_obj = Tcl_NewListObj(0, NULL);
        col_obj = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(len_obj);
	Tcl_IncrRefCount(typ_obj);
	Tcl_IncrRefCount(col_obj);

	/* check first for a stored proc return row */
	if ( (SybProcs[hand].last_next == NO_MORE_ROWS) &&
	     (dbhasretstat(SybProcs[hand].dbproc) == TRUE) ) { 

	    num_cols = dbnumrets(SybProcs[hand].dbproc);

	    for (i = 1; i <= num_cols; i++) {

		/* get the return parm name and append to result */
		/* sql must have "exec storedproc @varname=@localname output" */
		/* in which case "@varname" is returned; otherwise null */

		bufptr = dbretname(SybProcs[hand].dbproc,i);

		if (bufptr != (char *) NULL) {
		    tmp_obj = Tcl_NewStringObj(bufptr,-1);
		} else {
		    tmp_obj = Tcl_NewStringObj("",0);
		}
		Tcl_IncrRefCount(tmp_obj);
		Tcl_ListObjAppendElement(NULL, col_obj, tmp_obj);
		Tcl_DecrRefCount(tmp_obj);

		Tcl_ListObjAppendElement(NULL, len_obj, 
			Tcl_NewIntObj(dbretlen(SybProcs[hand].dbproc,i)));

		Tcl_ListObjAppendElement(NULL, typ_obj, Tcl_NewStringObj(
		    dbprtype(dbrettype(SybProcs[hand].dbproc,i)),-1) );

	    }

	} else {
	    /* check for a regular row */
	    if ((SybProcs[hand].last_next == REG_ROW) ||
	        (SybProcs[hand].last_next == NO_MORE_ROWS)) {
		num_cols = dbnumcols(SybProcs[hand].dbproc);

		for (i = 1; i <= num_cols; i++) {

		    /* get the column name and append to result */
		    /* if column is an aggregate, null */

		    bufptr = dbcolname(SybProcs[hand].dbproc,i);

		    if (bufptr != (char *) NULL) {
			tmp_obj = Tcl_NewStringObj(bufptr,-1);
		    } else {
			tmp_obj = Tcl_NewStringObj("",0);
		    }
		    Tcl_IncrRefCount(tmp_obj);
		    Tcl_ListObjAppendElement(NULL, col_obj, tmp_obj);
		    Tcl_DecrRefCount(tmp_obj);

		    Tcl_ListObjAppendElement(NULL, len_obj,
			 Tcl_NewIntObj(dbcollen(SybProcs[hand].dbproc,i)));

		    Tcl_ListObjAppendElement(NULL, typ_obj, Tcl_NewStringObj(
			 dbprtype(dbcoltype(SybProcs[hand].dbproc,i)),-1) );

		}
	    } else {
		/* first check if really a compute row */
	        if (SybProcs[hand].last_next < 1) {
                    return TCL_OK;
		}

		/* must be a compute row */
		id = SybProcs[hand].last_next;  /* the compute row id */
		num_cols = dbnumalts(SybProcs[hand].dbproc,id);

		for (i = 1; i <= num_cols; i++) {

		    /* get the compute column name and append to result */

		    bufptr = dbcolname(SybProcs[hand].dbproc,
			     dbaltcolid(SybProcs[hand].dbproc,id,i));

		    sprintf(buf2,"%s(%s)",
			 dbprtype(dbaltop(SybProcs[hand].dbproc,id,i)),bufptr);
		    Tcl_ListObjAppendElement(NULL, col_obj, Tcl_NewStringObj(
				buf2,-1));

		    Tcl_ListObjAppendElement(NULL, len_obj,
			 Tcl_NewIntObj(dbadlen(SybProcs[hand].dbproc,id,i)));

		    Tcl_ListObjAppendElement(NULL, typ_obj, Tcl_NewStringObj(
			 dbprtype(dbalttype(SybProcs[hand].dbproc,id,i)),-1) );

		}
	    }
	}

	Tcl_SetObjResult(interp, col_obj);

	Tcl_ObjSetVar2(interp, SybMsgArray, SM_collengths, len_obj,
							TCL_GLOBAL_ONLY);
	Tcl_ObjSetVar2(interp, SybMsgArray, SM_coltypes,   typ_obj,
							TCL_GLOBAL_ONLY);

	Tcl_DecrRefCount(len_obj);
	Tcl_DecrRefCount(typ_obj);
	Tcl_DecrRefCount(col_obj);
    }

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Cancel --
 *    Implements the sybcancel command:
 *    usage: sybcancel  handle 
 *	                
 *    results:
 *	null string
 *      TCL_OK - handle is opened
 *      TCL_ERROR - wrong # args, or handle not opened,
 */

Sybtcl_Cancel (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;

    if ((hand = syb_prologue(interp, objc, objv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;
    SybProcs[hand].in_event     = 0;
    SybProcs[hand].hasBgResults = 0;
    SybProcs[hand].bgResults    = 0;
    SybProcs[hand].async        = 0;

    if (SybProcs[hand].bufferedResult != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedResult);
	SybProcs[hand].bufferedResult = NULL;
	Tcl_DecrRefCount(SybProcs[hand].bufferedIsnull);
	SybProcs[hand].bufferedIsnull = NULL;
    }
    /* remove call back handler and free callback script, if any */
    remove_handler(hand);

    cancel_prev(hand);

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Retval --
 *    Implements the sybretval command:
 *    usage: sybretval  handle 
 *	                
 *    results:
 *	return values from a stored procedure as tcl list
 *      TCL_OK - handle is opened, and dbnextrow has reached NO_MORE_ROWS
 *      TCL_ERROR - wrong # args, or handle not opened,
 */

Sybtcl_Retval (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;
    int     i;
    int     num_cols;
    int     col_type;
    int     col_len;
    BYTE   *col_ptr;
    int     iptr_idx;
    Tcl_Obj *isnull_list;


    if ((hand = syb_prologue(interp, objc, objv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }
    /* find the interp option structure */
    iptr_idx = get_syb_option(interp);
    if (iptr_idx == -1) {
	Sybtcl_AppendObjResult (interp, CMD_STR,
			  ": panic: can't find interp options",
			    (char *) NULL);
	return (TCL_ERROR);
    }

    isnull_list = Tcl_NewListObj(0,NULL);
    Tcl_IncrRefCount(isnull_list);

    /* dbnextrow() must have been exhausted before access to return values */
    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
	num_cols = dbnumrets(SybProcs[hand].dbproc);
	for (i = 1; i <= num_cols; i++) {
	    col_type = dbrettype(SybProcs[hand].dbproc,i);
	    col_len  = dbretlen (SybProcs[hand].dbproc,i);
	    col_ptr  = dbretdata(SybProcs[hand].dbproc,i);

	    /* call parse_column to convert to string and append to result */
	    if (parse_column(interp, hand, col_type, col_len, col_len, col_ptr, 
						iptr_idx, isnull_list) == 0) {

    		Tcl_DecrRefCount(isnull_list);
		return TCL_ERROR;
	    }

	}
    }

    Tcl_ObjSetVar2(interp, SybMsgArray, SM_isnull, isnull_list, 
				    TCL_GLOBAL_ONLY);
    Tcl_DecrRefCount(isnull_list);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Close --
 *    Implements the sybclose command:
 *    usage: sybcose handle 
 *	                
 *    results:
 *	null string
 *      TCL_OK - handle is closed
 *      TCL_ERROR - wrong # args, or handle not opened,
 */

Sybtcl_Close (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;

    if ((hand = syb_prologue(interp, objc, objv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }

    dbcancel(SybProcs[hand].dbproc);

    syb_dbclose(SybProcs[hand].dbproc);

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;
    SybProcs[hand].in_event     = 0;
    SybProcs[hand].hasBgResults = 0;
    SybProcs[hand].bgResults    = 0;
    SybProcs[hand].in_use       = 0;
    SybProcs[hand].last_text    = 32768;
    SybProcs[hand].async        = 0;
    SybProcs[hand].interp       = NULL;
    SybProcs[hand].dbproc       = NULL;
    if (SybProcs[hand].bufferedResult != NULL) {
	Tcl_DecrRefCount(SybProcs[hand].bufferedResult);
	SybProcs[hand].bufferedResult = NULL;
	Tcl_DecrRefCount(SybProcs[hand].bufferedIsnull);
	SybProcs[hand].bufferedIsnull = NULL;
    }

    /* remove call back handler and free callback script, if any */
    remove_handler(hand);

    /* remove the channel */
    Tcl_Close((Tcl_Interp *) NULL, SybProcs[hand].sybChan);
    SybProcs[hand].sybChan = NULL;

    /* update sybmsg(usedhandles) */
    count_inuse(interp);

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Wrtext --
 *    Implements the sybwritetext command:
 *    usage: sybwritetext handle object colnum file ?-nolog?
 *    usage: sybwritetext handle object colnum -file file ?-nolog?
 *    usage: sybwritetext handle object colnum -variable variable ?-nolog?
 *      where: object = "table.colum"
 *             column = number of text column from last select
 *             file   = file in which to get text 
 *             "nolog" = optional arg, don't log text write
 *
 *    sybwritetext can only be used as specified by DB-Lib docs!
 *       using sybsql....
 *    1. insert row, all values except text column
 *    2. update row, set text column = null
 *    3. select text column from table
 *       then....
 *    4. sybwritetext hand table.column colnum filename ?-nolog?
 *	                
 *    results:
 *	null string
 *      TCL_OK - handle is closed
 *      TCL_ERROR - wrong # args, or handle not opened, file not found,
 *                  or other bad karma in text/image handling
 */

Sybtcl_Wrtext (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;
    int     s;
    int     col;
    int     filesize;
    int     total_bytes = 0;
    int     log;
    LARGE_CHAR buf[TEXT_BUFF_SIZE];
    RETCODE dbret;
    DBBINARY *txtptr;
    DBBINARY *timestmp;
    DBBINARY text_pointer[DBTXPLEN];
    DBBINARY time_stamp[DBTXTSLEN];
    Tcl_Obj *var_obj = NULL;
    char    *p;
    char    *var_value;
    int      obj_parm;
    int      isvar = 0;
    Tcl_Channel fd;
#ifndef MAC_TCL
    struct stat stat_buf;		
#endif


    if ((hand = syb_prologue(interp, objc, objv, 5, 
	    " handle table.column colnum [ filename | -file filename | -variable variable ] ?-nolog? ")) == -1) {
	return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(NULL, objv[3], &col) == TCL_ERROR) {
        Sybtcl_AppendObjResult (interp, CMD_STR,
		": column number ", Tcl_GetStringFromObj(objv[3],NULL),
		     " not valid ", (char *) NULL);
        return TCL_ERROR;
    }

    /* if an event handler is active, remove it */
    remove_handler(hand);

    /* check for file or variable input */
    obj_parm = 4;
    p = Tcl_GetStringFromObj(objv[obj_parm],NULL);
    if ((objc > 5) && (*p == '-' && strncmp(p,"-file",5) == 0) ) {
	obj_parm = 5;
    } else if ((objc > 5) && (*p == '-' && strncmp(p,"-variable",9) == 0) ) {
	obj_parm = 5;
	isvar = 1;
    }

    /* check if nolog specified */
    log = TRUE;
    if (objc > obj_parm+1 && 
	(strncmp(Tcl_GetStringFromObj(objv[obj_parm+1],NULL), "nolog",5) ==0 ||
         strncmp(Tcl_GetStringFromObj(objv[obj_parm+1],NULL),"-nolog",6)==0)) {
      log = FALSE;
    }

    if (isvar) {
        var_obj = Tcl_ObjGetVar2(interp, objv[obj_parm], NULL,TCL_PARSE_PART1);
        if (var_obj == NULL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
		": -variable ", Tcl_GetStringFromObj(objv[obj_parm],NULL),
		" not found", (char *) NULL);
	    return TCL_ERROR;
        }
	var_value = Tcl_GetStringFromObj(var_obj, &filesize);

    } else {

	if (Tcl_IsSafe(interp)) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
		  ":  cannot read from file in safe interpreter",(char *)NULL);
	    return TCL_ERROR;
	}

	fd = Tcl_OpenFileChannel(NULL, 
		(p=Tcl_GetStringFromObj(objv[obj_parm],NULL)), "r", 0);

	if (fd == NULL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
		   ": file ", p,
			 " cannot be opened for reading ", (char *) NULL);
	    return TCL_ERROR;
	}

	Tcl_SetChannelOption(NULL, fd, "-translation", "binary");
	Tcl_SetChannelBufferSize(fd, TEXT_BUFF_SIZE);

#ifdef MAC_TCL
	/* ugly portable method for mac mpw, no stat() call */
	filesize = Tcl_Seek(fd, 0, SEEK_END);
	Tcl_Seek(fd, 0, SEEK_SET);
#else
        /* use stat on filename to get filesize */
	stat(p, &stat_buf);
	filesize = stat_buf.st_size;
#endif
    }

    /* if was sql was -async, wait for results */
    if (SybProcs[hand].async == 1) {
	events_waiting(hand,0);
	dbret = dbsqlok(SybProcs[hand].dbproc);
	if (dbret == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
	        ": dbsqlok failed ", (char *) NULL);
            return TCL_ERROR;

	}
        SybProcs[hand].last_next    = NO_MORE_ROWS;
	SybProcs[hand].last_results = SUCCEED;
	SybProcs[hand].async        = 0;
	
	/* get and save dbresults return code */
	dbret = dbresults(SybProcs[hand].dbproc);
	SybProcs[hand].last_results = dbret;
	if (dbret == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
		": dbresults failed ", (char *) NULL);
	    return TCL_ERROR;
	}

	/* get and save nextrow value */
	dbret = dbnextrow(SybProcs[hand].dbproc);
	SybProcs[hand].last_next = dbret;
    } else {
        /* get the first row */
        dbret = dbnextrow(SybProcs[hand].dbproc);
    }

    /* get textpointer and timestamp */
    txtptr = dbtxptr(SybProcs[hand].dbproc,col);
    if (txtptr == NULL) {
        Tcl_Close (NULL,fd);
        Sybtcl_AppendObjResult (interp, CMD_STR,
	   ": dbtxptr failed ", (char *) NULL);
        return TCL_ERROR;
    } else {
        bcopy(txtptr, text_pointer, DBTXPLEN);
        txtptr = &text_pointer[0];
    }
    timestmp = dbtxtimestamp(SybProcs[hand].dbproc,col);
    if (timestmp == NULL) {
        Tcl_Close (NULL,fd);
        Sybtcl_AppendObjResult (interp, CMD_STR,
	  	": dbtxtimestamp failed ", (char *) NULL);
        return TCL_ERROR;
    } else {
        bcopy(timestmp, time_stamp, DBTXTSLEN);
        timestmp = &time_stamp[0];
    }

    /* make sure previous query is finished */
    dbret = SybProcs[hand].last_results;
    while ( (dbret != NO_MORE_RESULTS) && (dbret != FAIL) ) {
        dbcanquery(SybProcs[hand].dbproc);
        dbret = dbresults(SybProcs[hand].dbproc);
    }

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;


    /* setup for text/image write */
    dbret = dbwritetext(SybProcs[hand].dbproc,
			Tcl_GetStringFromObj(objv[2],NULL), 
			txtptr, DBTXPLEN, timestmp, log, filesize, NULL);

    if (dbret == FAIL) {   /* fail probaly means bad dbtxptr() */
	Tcl_Close (NULL,fd);
        Sybtcl_AppendObjResult (interp, CMD_STR,
	        ": dbwritetext failed ", (char *) NULL);
        return TCL_ERROR;
    }
 
    /* process any Tcl events while waiting for data to arrive */
    SybProcs[hand].last_results = SUCCEED;
    events_waiting(hand,1);

    /* make sure server is idle */
    dbret = dbsqlok(SybProcs[hand].dbproc);
    if (dbret == FAIL) {
	Tcl_Close (NULL,fd);
        SybProcs[hand].last_results = NO_MORE_RESULTS;
        Sybtcl_AppendObjResult (interp, CMD_STR,
		    ": dbsqlok after dbwritetext failed ", (char *) NULL);
        SybProcs[hand].last_results = NO_MORE_RESULTS;
        return TCL_ERROR;
    } else {
        dbret = dbresults(SybProcs[hand].dbproc);
        while ( (dbret != NO_MORE_RESULTS) && (dbret != FAIL) ) {
            dbret = dbresults(SybProcs[hand].dbproc);
        }
    }

    /* send the text/image */
    if (isvar) {
	p = var_value;
	while (filesize > 0) {
	    s = (filesize < TEXT_BUFF_SIZE) ? filesize : TEXT_BUFF_SIZE;
	    total_bytes += s;
            events_waiting(hand,1);
	    dbret = dbmoretext(SybProcs[hand].dbproc, s, (BYTE *) p);
	    p += s;
	    filesize -= s;
	    if (dbret == FAIL) {
		Sybtcl_AppendObjResult (interp, CMD_STR,
		    ": dbmoretext failed ", (char *) NULL);
                SybProcs[hand].last_results = NO_MORE_RESULTS;
		return TCL_ERROR;
	    }
	}
    } else {
	while ((s=Tcl_Read(fd,buf,TEXT_BUFF_SIZE)) > 0) {
	    total_bytes += s;
            events_waiting(hand,1);
	    dbret = dbmoretext(SybProcs[hand].dbproc, s, (BYTE *) buf);
	    if (dbret == FAIL) {
		Tcl_Close (NULL,fd);
		Sybtcl_AppendObjResult (interp, CMD_STR,
		    ": dbmoretext failed ", (char *) NULL);
                SybProcs[hand].last_results = NO_MORE_RESULTS;
		return TCL_ERROR;
	    }
	}

	Tcl_Close (NULL,fd);
    }

    /* let server finish */
    events_waiting(hand,1);

    dbret = dbsqlok(SybProcs[hand].dbproc);
    SybProcs[hand].last_results = NO_MORE_RESULTS;
    if (dbret == FAIL) { 
        Sybtcl_AppendObjResult (interp, CMD_STR,
			": dbsqlok after dbmoretext failed ", (char *) NULL);
        return TCL_ERROR;
    }

    dbret = dbresults(SybProcs[hand].dbproc);
    while ( (dbret != NO_MORE_RESULTS) && (dbret != FAIL) ) {
	dbcanquery(SybProcs[hand].dbproc);
        dbret = dbresults(SybProcs[hand].dbproc);
    }

    /* return total bytes sent */
    Tcl_SetObjResult (interp,Tcl_NewLongObj(total_bytes));

    return TCL_OK;

}




/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Rdtext --
 *    Implements the sybreadtext command:
 *    usage: sybreadtext handle file 
 *    usage: sybreadtext handle -file file 
 *    usage: sybreadtext handle -variable variable
 *
 *    sybreadtext can only be used as specified by DB-Lib docs!
 *    1. select one text column only from a table
 *    2. sybreadtext hand filename
 *	                
 *    results:
 *	null string
 *      TCL_OK - handle is closed
 *      TCL_ERROR - wrong # args, or handle not opened, can't open file,
 *                  or other error in text/image handling
 */


Sybtcl_Rdtext (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;
    int     s;
    int     total_bytes = 0;
    LARGE_CHAR buf[TEXT_BUFF_SIZE];
    int     obj_parm;
    Tcl_Obj *tmp_obj;
    char    *p;
    int      isvar = 0;
    Tcl_Channel fd;
    RETCODE   dbret;

    if ((hand = syb_prologue(interp, objc, objv, 3, 
        " handle [ filename | -file filename | -variable variable ]")) == -1) {
	return TCL_ERROR;
    }

    /* if an event handler is active, remove it */
    remove_handler(hand);

    /* if was sql was -async, wait for results */
    if (SybProcs[hand].async == 1) {
	events_waiting(hand,0);
	dbret = dbsqlok(SybProcs[hand].dbproc);
	if (dbret == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
	        ": dbsqlok failed ", (char *) NULL);
            return TCL_ERROR;

	}
        SybProcs[hand].last_next    = NO_MORE_ROWS;
	SybProcs[hand].last_results = SUCCEED;
	SybProcs[hand].async        = 0;
	
	/* get and save dbresults return code */
	dbret = dbresults(SybProcs[hand].dbproc);
	SybProcs[hand].last_results = dbret;
	if (dbret == FAIL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
		": dbresults failed ", (char *) NULL);
	    return TCL_ERROR;
	}

	/* get and save nextrow value */
	if (DBROWS(SybProcs[hand].dbproc) == FAIL) {
	    SybProcs[hand].last_next = NO_MORE_ROWS;
	} else {
	    SybProcs[hand].last_next = REG_ROW;
	}
    }

    /* check for a row returned */
    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
        Tcl_SetObjResult (interp,Tcl_NewIntObj(0));
	SybProcs[hand].last_results = NO_MORE_RESULTS;
        return TCL_OK;
    }
    /* check for one and only one column */
    if (dbnumcols(SybProcs[hand].dbproc) != 1) {
        Tcl_SetObjResult (interp,Tcl_NewIntObj(0));
	SybProcs[hand].last_results = NO_MORE_RESULTS;
        return TCL_OK;
    }

    /* check for file or variable input */
    obj_parm = 2;
    p = Tcl_GetStringFromObj(objv[obj_parm],NULL);
    if ((objc > 3) && (*p == '-' && strncmp(p,"-file",5) == 0) ) {
	obj_parm = 3;
    } else if ((objc > 3) && (*p == '-' && strncmp(p,"-variable",9) == 0) ) {
	obj_parm = 3;
	isvar = 1;
    }

    if (isvar) {
        tmp_obj = Tcl_NewStringObj("",0);
        Tcl_IncrRefCount(tmp_obj);
    } else {
	if (Tcl_IsSafe(interp)) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
		  ":  cannot write to file in safe interpreter",(char *)NULL);
	    SybProcs[hand].last_results = NO_MORE_RESULTS;
	    return TCL_ERROR;
	}

        fd = Tcl_OpenFileChannel(NULL, 
		Tcl_GetStringFromObj(objv[obj_parm],NULL), "w", 0644);

	if (fd == NULL) {
	    Sybtcl_AppendObjResult (interp, CMD_STR,
		    ": file ", Tcl_GetStringFromObj(objv[2],NULL),
			 " could not be opened for writing ", (char *) NULL);
	    SybProcs[hand].last_results = NO_MORE_RESULTS;
	    return TCL_ERROR;
	}
	Tcl_SetChannelOption(NULL, fd, "-translation", "binary");
	Tcl_SetChannelBufferSize(fd, TEXT_BUFF_SIZE);
    }

    while ((s=dbreadtext(SybProcs[hand].dbproc,(BYTE *)buf,TEXT_BUFF_SIZE)) != 
							       NO_MORE_ROWS) {
	if (s == -1) {
            Sybtcl_AppendObjResult (interp, CMD_STR,
			": dbreadtext failed ", (char *) NULL);
	    Tcl_Close(NULL, fd);
	    SybProcs[hand].last_results = NO_MORE_RESULTS;
            return TCL_ERROR;
	}
	if (s > 0) {
	    total_bytes += s;
	    if (isvar) {
		Tcl_AppendToObj(tmp_obj, buf, s);
	    } else {
	        Tcl_Write(fd, buf, s);
	    }
	    /* process any Tcl events while waiting */
	    events_waiting(hand,0);
	}
    }

    if (isvar) {
	Tcl_ObjSetVar2(interp, objv[obj_parm], NULL, tmp_obj, TCL_PARSE_PART1);
	Tcl_DecrRefCount(tmp_obj);
    } else {
        Tcl_Close (NULL,fd);
    }

    /* return total bytes sent */
    Tcl_SetObjResult (interp,Tcl_NewLongObj(total_bytes));

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    return TCL_OK;

}


/*
 *----------------------------------------------------------------------
 *
 * invoke_handler --
 *   eval the callback handler script, remove if error
 *   return 0 if ok, 1 if error
 */

static int 
invoke_handler (hand, interp)
    int        hand;
    Tcl_Interp *interp;
{
    int  bgerror = 0;
 
    if (SybProcs[hand].callBackScript == NULL) {
	remove_handler(hand);
        return 1;
    }

    Tcl_Preserve((ClientData) interp);
    if (Tcl_GlobalEvalObj(interp,SybProcs[hand].callBackScript) == TCL_ERROR) {
	/* background error, remove the handler */
	remove_handler(hand);
	bgerror = 1;
	Tcl_BackgroundError(interp);
    }
    Tcl_Release((ClientData) interp);

    return bgerror;
}


/*
 *----------------------------------------------------------------------
 *
 * callback_handler --
 *   process callback scripts for sybevent
 *   invoke handler if dbpoll returns data ready, or if
 *   return code is avaiable, or if
 *   no more results are found
 */

CALLBACK_SCOPE void 
callback_handler (cd_hand, mask)
    ClientData cd_hand;
    int        mask;
{
    int         hand = (int) cd_hand;
    Tcl_Interp *interp = SybProcs[hand].interp;
    DBPROCESS  *readyproc;
    RETCODE     dbret;
    int         reason;
    int         bgerror = 0;
    int         flags;
    int         opt_idx;

    /* guard against Tcl invoking us again while already active */
    if (SybProcs[hand].in_event == 1 || 
	SybProcs[hand].last_results == NO_MORE_RESULTS) {
        return;
    }
    SybProcs[hand].in_event = 1;

    if (SybProcs[hand].in_use && SybProcs[hand].callBackScript != NULL) {

	/* get proper flags for tcl events */
	opt_idx = get_syb_option(SybProcs[hand].interp);
	switch (SybOptions[opt_idx].bgevents) {
	    case 1:   /* bgevents = "idletasks" */
		flags = TCL_WINDOW_EVENTS|TCL_IDLE_EVENTS|TCL_DONT_WAIT;
		break;

	    case 2:   /* bgevents = "all" */
		flags = TCL_ALL_EVENTS|TCL_DONT_WAIT;
		break;

	    case 0:	  /* bgevents = "none", "", etc. */
	    default:
		/* don't do any Tcl events */
		flags = 0;
	}

	/* repeatedly eval callback script while dbproc ready and data exists*/
        /* also eval callback once if dbpoll() says the connection is dead */

        while (1) {
            dbret = dbpoll(SybProcs[hand].dbproc, 0L, &readyproc, &reason);
            if ( (dbret == FAIL) ||
                    ((readyproc == SybProcs [hand].dbproc) && 
                     (SybProcs[hand].last_results != NO_MORE_RESULTS) &&
                     (bgerror == 0)) ) {
                bgerror = invoke_handler(hand, interp);
            } else {
                break;
            }

            /* let Tcl fire other events as needed */
            if (flags) {
                Tcl_DoOneEvent(flags);
            }

            if (dbret == FAIL) {
                break;
	    }
	}

        /* user called sybclose; can not use dbproc */

	if (SybProcs [hand].dbproc == NULL) {
            SybProcs[hand].in_event = 0;
            return;
	}

        /* no more bytes waiting in socket, check other conditions */
	/* check if retstatus is available, if so invoke callback again */

	if (dbhasretstat(SybProcs[hand].dbproc) == TRUE && ! bgerror) {
	    bgerror = invoke_handler(hand, interp);
	}

	/* check for more dbresults, if succeeds, we need servicing again */

	if (! bgerror) {
	    dbret = dbresults(SybProcs[hand].dbproc);
	    if (dbret != FAIL) {
		SybProcs[hand].hasBgResults = 1;
		SybProcs[hand].bgResults    = dbret;
		bgerror = invoke_handler(hand, interp);
	    }
	    /* if dbresults=NO_MORE_RESULTS, call handler once to finish sql */
	    if (dbret == NO_MORE_RESULTS && ! bgerror) {
		bgerror = invoke_handler(hand, interp);
		bgerror = invoke_handler(hand, interp);
	    }
	}

    } else {

	/* if we were called and don't have a callback script, remove handler*/
	remove_handler(hand);
    }

    SybProcs[hand].in_event = 0;

}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Event --
 *    Implements the sybevent command:
 *    usage: sybreadtext handle ?script?
 *
 *    results:
 *	return existing event script, or add script as event handler
 *	or delete existing handler if script is null
 *      TCL_OK - 
 *      TCL_ERROR - wrong # args, or handle not opened, can't open file,
 *                  or other error in text/image handling
 */


Sybtcl_Event (clientData, interp, objc, objv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          objc;
    Tcl_Obj *CONST objv[];
{
    int     hand;
    char   *str;
    int     len;


    if ((hand = syb_prologue(interp, objc, objv,2," handle ?script?")) == -1) {
	return TCL_ERROR;
    }

    /* make sure this handle has sql in progress */
    if (SybProcs[hand].last_results == NO_MORE_RESULTS) {
        Sybtcl_AppendObjResult (interp, CMD_STR,
		":  no active sql  ", (char *) NULL);
        return TCL_ERROR;
    }

    if (objc > 2) {

        str = Tcl_GetStringFromObj(objv[2], &len);
	if (len == 0) {
	    if (SybProcs[hand].callBackScript != NULL) {
                /* deleting handler */
		remove_handler(hand);
	    }
	    return TCL_OK;
	} else {
            /* check for previously installed script & handler */
	    if (SybProcs[hand].callBackScript == NULL) {
                /* create handler */
                Tcl_CreateChannelHandler(SybProcs[hand].sybChan, TCL_READABLE, 
    				    callback_handler, (ClientData) hand);

#ifdef POLLED_EVENTS
                Tcl_CreateEventSource(dbSetupProc, dbCheckProc,
				(ClientData) hand);
#endif

	    } else {
	        /* delete existing script */
		Tcl_DecrRefCount(SybProcs[hand].callBackScript);
	    }
	    SybProcs[hand].callBackScript = objv[2];
	    Tcl_IncrRefCount(SybProcs[hand].callBackScript);
        }

    } else {
        /* return existing script, if any */
	if (SybProcs[hand].callBackScript != NULL) {
	    Tcl_SetObjResult(interp, SybProcs[hand].callBackScript);
	}
    }

    return TCL_OK;
}



/* finis */
