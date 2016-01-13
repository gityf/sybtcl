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
 * Version 2.6, May 1998
 * Tom Poindexter
 * tpoindex@nyx.net
 * -change MS VC defines for syb_tcl_err_handler & syb_tcl_msg_handler, thanks
 *  to Jeff Jacobi <jeff.jacobi@lmco.com>.
 *
 */


#define SYBTCL_VERSION "2.5"


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
/* the only place we get Tcl alloc'ed is from Tcl_SplitList, use wrapper */
static int MyTcl_SplitList(interp,list,argc,argv)
  Tcl_Interp *interp;
  char *list;
  int *argc;
  char ***argv;
{
#ifdef SYBTCL_MEM_FULL_DEBUG
  fprintf(my_file,"tclalloc %d\n",__LINE__);
#endif
  my_tcl_alloc++;
  return Tcl_SplitList(interp,list,argc,argv);
}
#define Tcl_SplitList MyTcl_SplitList
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

/* sakelley@ucsd.edu 1/31/97: Macintosh needs different path to stat.h */
/* plus define macros for isdigit and force using BCOPY */
#ifdef macintosh
#define isdigit(c) (((c)>='0')&&((c)<='9'))
#define NO_BCOPY
#define LARGE_CHAR static char
#include <stat.h>
#include <unistd.h>
#else
#include <sys/stat.h>
#define LARGE_CHAR char
#endif


#include <sybfront.h>
#include <sybdb.h>
#include <syberror.h>

#ifdef NO_STRING_H
#include <strings.h>
#else
#include <string.h>
#endif

#include <ctype.h>

#ifdef __WIN32__
#include <memory.h>
#include <stdlib.h>
#include <io.h>
#endif


#ifdef NO_BCOPY
#define bcopy(from, to, length)    memmove((to), (from), (length))
#endif


typedef struct SybTclProcs {	/* struct for handle entries */
    int         in_use;		/* if this entry is opened */
    DBPROCESS  *dbproc;		/* dbproc pointer for this entry */
    RETCODE     last_results;	/* return code from last dbresults() */
    RETCODE     last_next;	/* return code from last dbnextrow() */
    int         bufferedRow;    /* buffered row indicator and str when */
    Tcl_DString bufferedStr;    /* Sybtcl_NextAll changes row types */
    char	float_format[20];/* format for float/real values */ 
    int         async; 		/* last sql was async */
    long        last_text;      /* last maxtext size sent */
} SybTclProcs;


#define SYBTCLPROCS       50	/* default number of dbprocs available */
#define SYB_BUFF_SIZE	4096	/* conversion buffer size for various needs*/

/* defines for text/image handling - first is our buffer size;other is max */
/* use LARGE_CHAR macro to allocate TEXT_BUFF_SIZE in functions */
#define   TEXT_BUFF_SIZE   32768
#define   MAX_SERVER_TEXT  "2147483647"

static SybTclProcs   SybProcs[SYBTCLPROCS];  

static char *SybHandlePrefix = "sybtcl";  /* prefix used to identify handles*/

static char *SybMsgArray = "sybmsg";  /* array to place errors & messages */

static Tcl_Interp *SybInterp;	    /* interpreter access in err&msg handler*/


/* rip off from example.c: defines plus .dll entry point function */

#ifdef __WIN32__
#if defined(__WIN32__)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   undef WIN32_LEAN_AND_MEAN
 
#   if defined(_MSC_VER)
#       define EXPORT(a,b) __declspec(dllexport) a b
#       define DllEntryPoint DllMain
#   else
#       if defined(__BORLANDC__)
#           define EXPORT(a,b) a _export b
#       else
#           define EXPORT(a,b) a b
#       endif
#   endif
#else
#   define EXPORT(a,b) a b
#endif
 
EXTERN EXPORT(int,Sybtcl_Init) _ANSI_ARGS_((Tcl_Interp *interp));
 
BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
HINSTANCE hInst;            /* Library instance handle. */
DWORD reason;               /* Reason this function is being called. */
LPVOID reserved;            /* Not used. */
{
    return TRUE;
}
 

#else


/* Windows binary flag for sybreadtext/sybwritetext,unix doesn't need anything*/

#define _O_BINARY    0

/* prototype for malloc */
extern void * malloc();

#endif


/* prototypes for all functions */

extern Tcl_CmdProc  Sybtcl_Connect;
extern Tcl_CmdProc  Sybtcl_Use;
extern Tcl_CmdProc  Sybtcl_Sql;
extern Tcl_CmdProc  Sybtcl_Poll;
extern Tcl_CmdProc  Sybtcl_Next;
extern Tcl_CmdProc  Sybtcl_Cols;
extern Tcl_CmdProc  Sybtcl_Cancel;
extern Tcl_CmdProc  Sybtcl_Close;
extern Tcl_CmdProc  Sybtcl_Retval;
extern Tcl_CmdProc  Sybtcl_Wrtext;
extern Tcl_CmdProc  Sybtcl_Rdtext;




/* 
 *----------------------------------------------------------------------
 * get_syb_handle
 *    authenticate a handle string 
 *  return: SybProcs index number or -1 on error;
 */

#ifdef __WIN32__
int CS_PUBLIC
#else
static int
#endif
get_syb_handle (handle) 
    char *handle;
{
    unsigned int prefix_len = strlen(SybHandlePrefix);
    int h;

    if ( (strlen(handle) > prefix_len) &&
	 (strncmp(handle,SybHandlePrefix,prefix_len) == 0)  &&
	 (isdigit(*(handle + prefix_len))) ) {

	 h = atoi((handle + prefix_len));
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


    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybProcs[i].dbproc == db_proc) {

            sprintf(buf,"%s%d",	SybHandlePrefix, i);
            Tcl_SetVar2(SybInterp,SybMsgArray,"handle", buf,TCL_GLOBAL_ONLY);

	    sprintf(buf,"%d",severity);
            Tcl_SetVar2(SybInterp,SybMsgArray,"severity",buf,TCL_GLOBAL_ONLY);

	    sprintf(buf,"%d",dberr);
            Tcl_SetVar2(SybInterp,SybMsgArray,"dberr",buf,TCL_GLOBAL_ONLY);

	    sprintf(buf,"%d",oserr);
            Tcl_SetVar2(SybInterp,SybMsgArray,"oserr",buf,TCL_GLOBAL_ONLY);
	    
            Tcl_SetVar2(SybInterp,SybMsgArray,"dberrstr",
			(dberrstr == NULL) ? "" : dberrstr, TCL_GLOBAL_ONLY);
            Tcl_SetVar2(SybInterp,SybMsgArray,"oserrstr",
			(oserrstr == NULL) ? "" : oserrstr, TCL_GLOBAL_ONLY);
	    

	    break;

	}
    }

    return (INT_CANCEL);
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
syb_tcl_msg_handler (db_proc, msgno, msgstate, severity, msgtext, srvname,
		     procname, line)
    DBPROCESS *db_proc;
    DBINT      msgno;
    int        msgstate;
    int        severity;
    char      *msgtext;
    char      *srvname;
    char      *procname;
    DBUSMALLINT line;
{

    int       i;
    char      buf[SYB_BUFF_SIZE];
    char     *msgbuf;
    char     *p;
    unsigned  msgsize;
    char      conv_buf[20];
    Tcl_DString myStr;


    for (i = 0; i < SYBTCLPROCS; i++) {
	if (SybProcs[i].dbproc == db_proc) {

            sprintf(buf,"%s%d",	SybHandlePrefix, i);
            Tcl_SetVar2(SybInterp,SybMsgArray,"handle", buf,TCL_GLOBAL_ONLY);

	    sprintf(buf,"%d",msgstate);
            Tcl_SetVar2(SybInterp,SybMsgArray,"msgstate",buf,TCL_GLOBAL_ONLY);

	    sprintf(buf,"%d",severity);
            Tcl_SetVar2(SybInterp,SybMsgArray,"severity",buf,TCL_GLOBAL_ONLY);

            Tcl_SetVar2(SybInterp,SybMsgArray,"srvname",srvname,
			TCL_GLOBAL_ONLY);
            Tcl_SetVar2(SybInterp,SybMsgArray,"procname",procname,
			TCL_GLOBAL_ONLY);

	    sprintf(buf,"%d",(int) line);
            Tcl_SetVar2(SybInterp,SybMsgArray,"line",buf,TCL_GLOBAL_ONLY);

	    /* it's possible that several messages may accumulate in one */
	    /* sybtcl command, due to serveral dblib routines being called */
	    /* append all msgno's & msgtext's together with newlines */
           
	    sprintf(conv_buf,"%d",msgno);
	    strcpy(buf,Tcl_GetVar2(SybInterp,SybMsgArray,"msgno",
							TCL_GLOBAL_ONLY));
            if (strlen(buf) > (unsigned int) 0) {
		strcat(buf,"\n");
	    }
	    strcat(buf,conv_buf);
            Tcl_SetVar2(SybInterp,SybMsgArray,"msgno",buf,TCL_GLOBAL_ONLY);

            /* msgtext buffer can get large, so use Tcl's DStrings      */
            /* Tcl_DString code by dhagberg@gds.com                  */
 
            Tcl_DStringInit(&myStr);
            Tcl_DStringAppend(&myStr, 
                             Tcl_GetVar2(SybInterp,SybMsgArray,
                                          "msgtext",TCL_GLOBAL_ONLY), -1);
            if (Tcl_DStringLength(&myStr) > 0) {
                Tcl_DStringAppend(&myStr,"\n",-1);
            }
            Tcl_SetVar2(SybInterp,SybMsgArray,"msgtext",
                        Tcl_DStringAppend(&myStr, msgtext, -1),
                        TCL_GLOBAL_ONLY);
            Tcl_DStringFree(&myStr);

	    break;

	}
    }

    return 0;
}


/*
 *----------------------------------------------------------------------
 * clear_msg --
 *
 * clears all error and message elements in the global array variable
 *
 */

static void
clear_msg(interp)
    Tcl_Interp *interp;
{
    /* indices associated with error and message handlers */
    /* ("severity" is shared between err & msg )          */
    Tcl_SetVar2(interp, SybMsgArray, "msgno",      "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "msgtext",    "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "severity",   "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "svrname",    "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "procname",   "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "line",       "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "dberr",      "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "oserr",      "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "dberrstr",   "", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "oserrstr",   "", TCL_GLOBAL_ONLY);

    /* index "nextrow" - the last return from dbnextrow/dbresults     */
    /* text message, or compute column id as integer                  */
    Tcl_SetVar2(interp, SybMsgArray, "nextrow",  "", TCL_GLOBAL_ONLY);

    /* index "retstatus" only meaningful after a stored proc execs */
    Tcl_SetVar2(interp, SybMsgArray, "retstatus","", TCL_GLOBAL_ONLY);

    /* index "collengths" & "coltypes" only meaningful after sybcols command */
    Tcl_SetVar2(interp, SybMsgArray, "collengths","", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "coltypes",  "", TCL_GLOBAL_ONLY);
}



/*
 *----------------------------------------------------------------------
 * syb_prologue
 *
 * does most of standard command prologue, assumes handle is argv[1]
 * returns: handle index  or -1 on failure
 * 
 */

static int
syb_prologue (interp, argc, argv, num_args, err_msg)
    Tcl_Interp *interp;
    int         argc;
    char      **argv;
    int         num_args;
    char       *err_msg;
{
    int         hand;


    /* check number of minimum args*/

    if (argc < num_args) {
	Tcl_AppendResult (interp, "wrong # args: ", argv[0],
			  err_msg, (char *) NULL);
	return (-1);
    }

    /* parse the handle */
    hand = get_syb_handle(argv[1]);

    if (hand == -1) {
	Tcl_AppendResult (interp, argv[0], ": handle ", argv[1],
			 " not open ", (char *) NULL);
	return (-1);
    }

    /* save the interp structure for the error & msg handlers */
    SybInterp = interp;

    /* clear sybmsg array for new messages & errors */
    Tcl_SetVar2(interp, SybMsgArray, "handle",  argv[1], TCL_GLOBAL_ONLY);
    clear_msg(interp);

    return (hand);
}


/*
 *----------------------------------------------------------------------
 * parse_column
 *   parse a column result, append as element to current interpreter result
 *
 */

static int
parse_column (interp, hand, col_type, col_len, col_ptr)
    Tcl_Interp *interp;
    int         hand;
    int         col_type;
    int         col_len;
    BYTE       *col_ptr;
{
    char       *buf;
    char        buf2[SYB_BUFF_SIZE];
    int         dst_len;
    int         text_buf_size;
    DBFLT8	float_val;
    DBDATEREC   dateinfo;
    DBDATETIME  convert_date;
    int         j;
    char        tempstr[SYB_BUFF_SIZE];
    char       *p;
    int         decimal;


    /* if column is null, extract the current "nullvalue" */
    if ((col_len == 0) && (col_ptr == (BYTE *)NULL)) {

	strcpy(buf2, Tcl_GetVar2(interp,SybMsgArray,"nullvalue",
							   TCL_GLOBAL_ONLY));

	/* if "nullvalue" is  the string "default", append the defaults,    */
	/* otherwise, append the user requested nullvalue, which could be ""*/

	if (*buf2 == 'd' && strcmp(buf2,"default") == 0) {
	  /* default-if number or money type, then return "0", else ""       */

          if (
#ifdef DBVERSION_100
              col_type == SYBDECIMAL ||
              col_type == SYBNUMERIC ||
#endif
	      col_type == SYBINT1 ||
              col_type == SYBINT2 ||
              col_type == SYBINT4 ||
              col_type == SYBFLT8 ||
              col_type == SYBREAL ||
              col_type == SYBMONEY ||
              col_type == SYBMONEY4
              ) {
            Tcl_AppendElement(interp, "0");
	  } else {
            Tcl_AppendElement(interp, "");
	  }

	} else {
          Tcl_AppendElement(interp, buf2);
	}

	return (1);
	  
    } 

    /* copy whatever value's in sybmsg(dateformat) to buf2 */
    strcpy(buf2, Tcl_GetVar2(interp,SybMsgArray,"dateformat",TCL_GLOBAL_ONLY));

    /* only convert text & image to the extent of buffer size */
    if ((col_type == SYBTEXT) || (col_type == SYBIMAGE)) {
        text_buf_size = atoi(
	    Tcl_GetVar2(interp,SybMsgArray,"maxtext",TCL_GLOBAL_ONLY) );
        /* start with null buffer */
        buf = ckalloc(text_buf_size + 2); /* 2 extra fudge bytes */
        if (buf == NULL) {
	    Tcl_SetResult (interp, 
	   "parse_column cannot malloc memory for TEXT or IMAGE", TCL_VOLATILE);
	    return (0);
        }
        memset(buf,'\0',text_buf_size+2); 
        if (col_len >= (text_buf_size)) {
	    dst_len = text_buf_size;  /* leave room for null */
        } else {
	    dst_len = -1;	/* dbconvert will null terminate */
        }


    /* if buf2 has a value in it, then there's a particular format */
    /* the user wants us to use for the date.  If not, we'll default */
    /* to the standard date output string. */
    } else if (((col_type == SYBDATETIME) || (col_type == SYBDATETIME4))
               && strlen(buf2) > (unsigned int) 0) {
      buf = ckalloc(SYB_BUFF_SIZE);
      if (buf == NULL) {
        Tcl_SetResult (interp, 
          "parse_column cannot malloc memory for date format", TCL_VOLATILE);
        return (0);
      }
      if (col_type == SYBDATETIME4) {
          dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len, 
		   SYBDATETIME, (BYTE *) &convert_date, sizeof(convert_date));
	  col_ptr = (BYTE *) &convert_date;
      }
      buf[0] = '\0';
      dbdatecrack(SybProcs[hand].dbproc,&dateinfo,(DBDATETIME *) col_ptr);
      j=0;
      while((unsigned int) j < strlen(buf2)) {
        if (!strncmp(&(buf2[j]),"YYYY",4)) {
          sprintf(tempstr,"%04d",dateinfo.dateyear);
          j += 4;
        } else if (!strncmp(&(buf2[j]),"YY",2)) {
          sprintf(tempstr,"%02d",dateinfo.dateyear % 100);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"MONTH",5)) {
          strcpy(tempstr,dbmonthname(SybProcs[hand].dbproc,(char *)NULL,
                                     dateinfo.datemonth+1,FALSE));
          j += 5;
        } else if (!strncmp(&(buf2[j]),"MON",3)) {
          strcpy(tempstr,dbmonthname(SybProcs[hand].dbproc,(char *)NULL,
                                     dateinfo.datemonth+1,TRUE));
          j += 3;
        } else if (!strncmp(&(buf2[j]),"MM",2)) {
          sprintf(tempstr,"%02d",dateinfo.datemonth + 1);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"DD",2)) {
          sprintf(tempstr,"%02d",dateinfo.datedmonth);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"hh",2)) {
          sprintf(tempstr,"%02d",dateinfo.datehour);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"mm",2)) {
          sprintf(tempstr,"%02d",dateinfo.dateminute);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"ss",2)) {
          sprintf(tempstr,"%02d",dateinfo.datesecond);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"dy",2)) {
          sprintf(tempstr,"%03d",dateinfo.datedyear);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"dw",2)) {
          sprintf(tempstr,"%02d",dateinfo.datedweek + 1);
          j += 2;
        } else if (!strncmp(&(buf2[j]),"ms",2)) {
          sprintf(tempstr,"%03d",dateinfo.datemsecond);
          j += 2;
        } else {
          tempstr[0] = buf2[j];
          tempstr[1] = '\0';
          j++;
        }
        strcat(buf,tempstr);
      }
      Tcl_AppendElement(interp,buf);
      ckfree(buf);
      return (1);

    /* if float or real, check sybmsg(floatprc) to determine float precision*/
    } else if ( (col_type == SYBREAL) || (col_type == SYBFLT8) ) {

      dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len,
				      SYBFLT8, (unsigned char *)&float_val, -1);
      sprintf(buf2,SybProcs[hand].float_format,float_val);
      /* make sure it looks like a float so tcl expr won't choke */
      decimal = 0;
      for (p = buf2; *p != 0; p++) {
	  if (*p == '.' || *p == 'e') {
	    decimal = 1;
	    break;
	  }
      }
      if (!decimal) {
	*p++ = '.';
	*p++ = '0';
	*p   = '\0';
      }
      Tcl_AppendElement(interp,buf2);
      return(1);

    /* if money, make sure there's a decimal */
    } else if ( (col_type == SYBMONEY) || (col_type == SYBMONEY4) ) {

      dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len,
				      SYBCHAR, (unsigned char *)buf2, -1);
      /* make sure it looks like a float so tcl expr won't choke */
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
	*p++ = '0';
	*p   = '\0';
      }
      Tcl_AppendElement(interp,buf2);
      return(1);

    /* see if we should not trim spaces from end of chars */
    } else if ( (col_type == SYBCHAR) || (col_type == SYBVARCHAR) ) {
      if (strcmp("yes", 
	  Tcl_GetVar2(interp,SybMsgArray,"fixedchar",TCL_GLOBAL_ONLY)) == 0) {
        buf = buf2;       /* use buf2 storage */
        dst_len = col_len;	
	memset(buf,'\0',col_len+1);  /* make sure dbconvert puts a null in */
      } else {
        buf = buf2;       /* use buf2 storage */
        dst_len = -1;	/* dbconvert will null terminate */
      }
    } else {
      buf = buf2;       /* use buf2 storage */
      dst_len = -1;	/* dbconvert will null terminate */
    }

    dbconvert(SybProcs[hand].dbproc,col_type, col_ptr, col_len, 
				    SYBCHAR, (unsigned char *)buf, dst_len);

    /* now append the buf to the tcl result */
    Tcl_AppendElement(interp,buf);

    if ((col_type == SYBTEXT) || (col_type == SYBIMAGE)) {
        ckfree(buf);
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
	if (SybProcs[i].in_use) {
	    dbclose(SybProcs[i].dbproc);
	}
	SybProcs[i].in_use = 0;
	SybProcs[i].bufferedRow = 0;
	Tcl_DStringFree(&SybProcs[i].bufferedStr);
    }

    /* don't call dbexit() if under Windows, sometimes causes crash */
#ifndef __WIN32__
    dbexit();		/* last cleanup */
#endif

    MY_STATS_DUMP;
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
    static int SybtclInitFlag = 0;   /* set when initialized via Sybtcl_Init */


    /* check if already initialized */
    if (!SybtclInitFlag) {

        MY_STATS_INIT;

	/* save the interp structure for the error & msg handlers */
	SybInterp = interp;

	if (dbinit() == FAIL) {
	    return TCL_ERROR;
	}

#ifdef DBVERSION_100
	dbsetversion(DBVERSION_100); /*enable system 10 numeric/decimal types */
#endif

	dbsetmaxprocs(SYBTCLPROCS);  /*make sure db-lib match our table size*/

#ifdef __WIN32__
	dberrhandle((MHANDLEFUNC) syb_tcl_err_handler);
	dbmsghandle((MHANDLEFUNC) syb_tcl_msg_handler);
#else
	dberrhandle(syb_tcl_err_handler);
	dbmsghandle(syb_tcl_msg_handler);
#endif

	/*
	 * Initialize sybase proc structures 
	 */

	for (i = 0; i < SYBTCLPROCS; i++) {
	    SybProcs[i].in_use        = 0;
	    SybProcs[i].dbproc        = NULL;
	    SybProcs[i].last_results  = NO_MORE_RESULTS;
	    SybProcs[i].last_next     = NO_MORE_ROWS;
	    SybProcs[i].bufferedRow   = 0;
	    Tcl_DStringInit(&SybProcs[i].bufferedStr);
	    strcpy(SybProcs[i].float_format,"%g");
	    SybProcs[i].async          = 0;
	    SybProcs[i].last_text      = 32768;
	}

        SybtclInitFlag = 1;

	Tcl_CreateExitHandler (Sybtcl_Kill, (ClientData)NULL);
    }


    /*
     * Initialize the new Tcl commands
     */

    Tcl_CreateCommand (interp, "sybconnect", Sybtcl_Connect, (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybuse",     Sybtcl_Use,     (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybsql",     Sybtcl_Sql,     (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybpoll",    Sybtcl_Poll,    (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybnext",    Sybtcl_Next,    (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybcols",    Sybtcl_Cols,    (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybcancel",  Sybtcl_Cancel,  (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybretval",  Sybtcl_Retval,  (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybclose",   Sybtcl_Close,   (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybwritetext",Sybtcl_Wrtext, (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand (interp, "sybreadtext",Sybtcl_Rdtext,  (ClientData)NULL,
		      (Tcl_CmdDeleteProc *) NULL);

    /*
     * Initialize sybmsg global array, inital null elements
     */
    
    /* index "version" - sybtcl version number*/
    Tcl_SetVar2(interp, SybMsgArray, "version", SYBTCL_VERSION,TCL_GLOBAL_ONLY);

    /* index "handle" - the last handle that set any other elements */
    Tcl_SetVar2(interp, SybMsgArray, "handle",   "", TCL_GLOBAL_ONLY);

    /* index "nullvalue" - what to return if column is null */
    /* user should set this value if something else is wanted */
    Tcl_SetVar2(interp, SybMsgArray, "nullvalue", "default",TCL_GLOBAL_ONLY);

    /* index "floatprec" - floating point precision, int if specified */
    /* user should set this value if something else is wanted */
    Tcl_SetVar2(interp, SybMsgArray, "floatprec", "",TCL_GLOBAL_ONLY);

    /* index "maxtext" - the maximum length of a text column to return */
    /* in a select or sybreadtext , can be set by user                 */
    Tcl_SetVar2(interp, SybMsgArray, "maxtext", "32768",TCL_GLOBAL_ONLY);

    /* index "dateformat" - the format that the user wants datetime */
    /* values to return in. */
    Tcl_SetVar2(interp, SybMsgArray, "dateformat", "",TCL_GLOBAL_ONLY);

    /* index "fixedchar" - set to "yes" if char/varchar should not trim */
    /* trailing spaces */
    Tcl_SetVar2(interp, SybMsgArray, "fixedchar", "",TCL_GLOBAL_ONLY);

    /* other indices - correspond to error and message handler arguments */
    clear_msg(interp);


#if ((TCL_MAJOR_VERSION > 7) || ((TCL_MAJOR_VERSION == 7) && (TCL_MINOR_VERSION >= 5)))

    if (Tcl_PkgProvide(interp, "Sybtcl", SYBTCL_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

#endif

    return TCL_OK;

}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Connect --
 *    Implements the sybconnect command:
 *    usage: sybconnect userid password ?server? ?appname? ?ifile?
 *	                
 *    results:
 *	handle - a character string of newly open handle
 *      TCL_OK - connect successful
 *      TCL_ERROR - connect not successful - error message returned
 */

int
Sybtcl_Connect (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{

    LOGINREC  *login;
    int        hand = -1;
    int        i;
    char       buf[SYB_BUFF_SIZE];


    /* can't use syb_prologue, sybconnect creates a handle */

    if (argc < 3) {
	Tcl_AppendResult (interp, "wrong # args: ", argv[0],
		  " userid password ?server? ?appname? ?ifile? ",(char *) NULL);
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
	Tcl_AppendResult (interp, argv[0], ": no sybase dbprocs available",
			  (char *) NULL);
	return TCL_ERROR;
    }

    /* save the interp structure for the error & msg handlers */
    SybInterp = interp;

    login = dblogin();

    DBSETLUSER(login,argv[1]);
    DBSETLPWD(login, argv[2]);

    if (argc > 4) {
        DBSETLAPP(login, argv[4]);
    }

    if (argc > 5) {
        dbsetifile(argv[5]);
    } else {
        dbsetifile( NULL);
    }


    SybProcs[hand].dbproc = dbopen(login, ((argc>3) ? argv[3] : (char *)NULL));

    dbloginfree(login);                     /* login struct no longer needed */

    if (SybProcs[hand].dbproc == NULL) {
	Tcl_AppendResult (interp, argv[0], ": sybconnect failed in dbopen",
			  (char *) NULL);
	return TCL_ERROR;
    }

    SybProcs[i].in_use = 1;	/* handle ok, set in use flag */
    SybProcs[i].bufferedRow   = 0;
    Tcl_DStringFree(&SybProcs[i].bufferedStr);

    /* construct handle and return */
    sprintf(buf,"%s%d",SybHandlePrefix,hand);

    Tcl_SetVar2(interp, SybMsgArray, "handle",      buf, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, SybMsgArray, "nextrow",      "", TCL_GLOBAL_ONLY);
    clear_msg(interp);

    Tcl_SetResult(interp,buf,TCL_VOLATILE);

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

Sybtcl_Use (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    RETCODE dbret;

    if ((hand = syb_prologue(interp,argc, argv, 2," handle ?dbname?")) == -1) {
	return TCL_ERROR;
    }

    if (argc > 2 ) {
	dbret = dbuse(SybProcs[hand].dbproc, argv[2]);
	if (dbret == SUCCEED) {
	  Tcl_SetResult(interp,argv[2],TCL_VOLATILE);
	} else {
	  Tcl_SetResult(interp,"sybuse: database cannot be used",TCL_VOLATILE);
	  return TCL_ERROR;
	}
    } else {
	Tcl_SetResult(interp,dbname(SybProcs[hand].dbproc), TCL_VOLATILE);
    }

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Sql --
 *    Implements the sybsql command:
 *    usage: sybsql handle sql-string ?async"
 *	                
 *    results:
 *	"REG_ROW" if rows returned, "NO_MORE_ROWS" if no rows returned
 *	"PENDING"  if executed with "async"
 *      TCL_OK - handle is opened, sql executed ok
 *      TCL_ERROR - wrong # args, or handle not opened,  bad sql stmt
 */

Sybtcl_Sql (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    RETCODE dbret;
    char    buf[SYB_BUFF_SIZE];
    char    conv_buf[20];
    int     text_buf_size;
    int     float_prec;


    if ((hand = syb_prologue(interp,argc, argv, 3, " handle sql_str")) == -1) {
	return TCL_ERROR;
    }

    /* cancel any pending results */
    dbcancel(SybProcs[hand].dbproc);
    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;
    SybProcs[hand].bufferedRow  = 0;
    Tcl_DStringFree(&SybProcs[hand].bufferedStr);
    SybProcs[hand].async        = 0;

    if (argc >= 4 && strncmp(argv[3],"async",5) == 0 ) {
        SybProcs[hand].async = 1;
    }

    /* set server text size to value of maxtext */
    text_buf_size = atoi(
	    Tcl_GetVar2(interp,SybMsgArray,"maxtext",TCL_GLOBAL_ONLY) );
    if (text_buf_size > 0) {	/* validate maxtext to be > 0 */
	if (SybProcs[hand].last_text != text_buf_size) {
	    SybProcs[hand].last_text = text_buf_size;
            sprintf(conv_buf,"%d",text_buf_size);
            dbsetopt(SybProcs[hand].dbproc, DBTEXTSIZE, conv_buf, -1);
	    dbsqlexec(SybProcs[hand].dbproc);  /* execute dbsetopt() */
	    dbcancel(SybProcs[hand].dbproc);   /* and ignore results */
	}
    }

    /* set float precision for this sql */
    float_prec = atoi(Tcl_GetVar2(interp,SybMsgArray,"floatprec",
				 TCL_GLOBAL_ONLY));
    if (float_prec > 0 && float_prec <= TCL_MAX_PREC) {
	sprintf(SybProcs[hand].float_format,"%%.%dg",float_prec);
    } else {
	sprintf(SybProcs[hand].float_format,"%%.%dg",TCL_MAX_PREC);
    }

    /* set up sql for execution */
    dbret = dbcmd(SybProcs[hand].dbproc, argv[2]);

    if (dbret == FAIL) {
	Tcl_AppendResult (interp, argv[0], ": dbcmd failed ", 
			  (char *) NULL);
	return TCL_ERROR;
    }

    /* send sql to server and get return code */
    if (SybProcs[hand].async == 1) {
        dbret = dbsqlsend(SybProcs[hand].dbproc);
    } else {
        dbret = dbsqlexec(SybProcs[hand].dbproc);
    }

    if (dbret == FAIL) {
	Tcl_AppendResult (interp, argv[0], ": dbsqlexec failed ", 
			  (char *) NULL);
	return TCL_ERROR;
    }

    /* reset flags to worst case */

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;

    /* if async, return PENDING */ 
    if (SybProcs[hand].async == 1) {
        /* set msg array variable "nextrow" to pending status */
        Tcl_SetVar2(interp, SybMsgArray, "nextrow", "PENDING", TCL_GLOBAL_ONLY);
        Tcl_SetResult(interp, "PENDING", TCL_VOLATILE);
        return TCL_OK;
    } 

    /* process first dbresults */
    dbret = dbresults(SybProcs[hand].dbproc);

    if (dbret == FAIL) {
	Tcl_AppendResult (interp, argv[0], ": dbresults failed ", 
			  (char *) NULL);
	return TCL_ERROR;
    }

    /* save dbresults return code */
    SybProcs[hand].last_results = dbret;

    if (dbhasretstat(SybProcs[hand].dbproc) == TRUE) {
	sprintf(buf,"%d",dbretstatus(SybProcs[hand].dbproc));
        Tcl_SetVar2(interp, SybMsgArray, "retstatus", buf, TCL_GLOBAL_ONLY);
    }

    if (dbret == NO_MORE_RESULTS) {
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
    Tcl_SetVar2(interp, SybMsgArray, "nextrow", buf, TCL_GLOBAL_ONLY);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);

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

Sybtcl_Poll (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int         hand;
    DBPROCESS  *readyproc;
    RETCODE     dbret;
    int         reason;
    int         all;
    int         i;
    long        millisec;
    char        buf[200];

    if ((hand = syb_prologue(interp, argc, argv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }

    if (argc >= 3) {
	millisec = strtol(argv[2],NULL,10);
	if (millisec < -1L) {
	    millisec = -1L;
	}
    } else {
	millisec = 0L;
    }

    if ((argc >= 4 && strncmp(argv[3],"all",3) == 0) || 
        (argc >= 4 && strncmp(argv[3],"-all",4) == 0)  ) {
	all = 1;
    } else {
	all = 0;
    }


    /* if "all" requested, poll all other procs */
    if (all) {
	/* call dbpoll with user requested timeout for all (NULL) */
	dbret = dbpoll((DBPROCESS *) NULL, millisec, &readyproc,  &reason);
	/* now check through all procs */
	for (i = 0; i < SYBTCLPROCS; i++) {
	    if ((SybProcs[i].in_use == 1) && (SybProcs[i].async == 1)) {
		dbret = dbpoll(SybProcs[i].dbproc, 0L, &readyproc,  
					       &reason);
		if (dbret == SUCCEED && reason == DBRESULT && 
						     readyproc != NULL) {
		    sprintf(buf,"%s%d",SybHandlePrefix,i);
		    Tcl_AppendElement(interp,buf);
		}
	    }
	}
    } else {
	if (SybProcs[hand].async == 1) {
	    dbret = dbpoll(SybProcs[hand].dbproc, millisec, &readyproc,  
						  &reason);
	    if (dbret == SUCCEED && reason == DBRESULT && 
						   readyproc != NULL) {
		sprintf(buf,"%s%d",SybHandlePrefix,hand);
		Tcl_AppendElement(interp,buf);
	    }
	}
    }

    return TCL_OK;
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

Sybtcl_Next (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    RETCODE dbret;
    char    buf[SYB_BUFF_SIZE];
    int     i;
    int     num_cols;
    int     col_type;
    int     col_len;
    BYTE   *col_ptr;
    int     dst_len;
    int     Sybtcl_NextAll();


    if ((hand = syb_prologue(interp, argc, argv, 2, " handle ?commands? ?subchar? ?tclvar colnum ...?")) == -1) {
	return TCL_ERROR;
    }

    /* if commands argument, then call next function to do work */
    if (argc >= 3) {
	return (Sybtcl_NextAll(clientData, interp, argc, argv));
    }

    /* check to see if any results where buffered from NextAll */

    if (SybProcs[hand].bufferedRow == 1) {
        Tcl_DStringResult(interp, &SybProcs[hand].bufferedStr);
        SybProcs[hand].bufferedRow     = 0;
        Tcl_DStringFree(&SybProcs[hand].bufferedStr);
	if (SybProcs[hand].last_next == REG_ROW) {
	    strcpy(buf,"REG_ROW");
	} else {
	    sprintf(buf,"%d",SybProcs[hand].last_next);
	}
	Tcl_SetVar2(interp, SybMsgArray, "nextrow", buf, TCL_GLOBAL_ONLY);
	return TCL_OK;
    }

    if (SybProcs[hand].async == 1) {
	dbret = dbsqlok(SybProcs[hand].dbproc);
	if (dbret == FAIL) {
	    Tcl_AppendResult (interp, argv[0], ": dbsqlok failed ",
		  (char *) NULL);
            return TCL_ERROR;

	}
        SybProcs[hand].last_next    = NO_MORE_ROWS;
	SybProcs[hand].last_results = SUCCEED;
	SybProcs[hand].async        = 0;
    }

    /* check to see if next set of results */

    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
	if (SybProcs[hand].last_results == SUCCEED) {

	    dbret = dbresults(SybProcs[hand].dbproc);

	    /* save dbresults return code */
	    SybProcs[hand].last_results = dbret;

	    if (dbret == FAIL) {
		Tcl_SetVar2(interp, SybMsgArray, "nextrow","FAIL", 
			    TCL_GLOBAL_ONLY);
		SybProcs[hand].last_next = NO_MORE_ROWS;

		return TCL_OK;   /* null string returned */
	    }

	    if (dbhasretstat(SybProcs[hand].dbproc) == TRUE) {
		sprintf(buf,"%d",dbretstatus(SybProcs[hand].dbproc));
		Tcl_SetVar2(interp, SybMsgArray, "retstatus", buf, 
			    TCL_GLOBAL_ONLY);
	    }

	    if (dbret == NO_MORE_RESULTS) {
		Tcl_SetVar2(interp, SybMsgArray, "nextrow","NO_MORE_RESULTS", 
			    TCL_GLOBAL_ONLY);
		SybProcs[hand].last_next = NO_MORE_ROWS;

		return TCL_OK;   /* null string returned */
	    } else {
	        /* dbresults() returned SUCCEED, check for any rows */
		if (DBROWS(SybProcs[hand].dbproc) == FAIL) {
		    /* previous set of results exhausted, but there are no   */
		    /* rows in the new set, could be procedure return values */
		    Tcl_SetVar2(interp, SybMsgArray, "nextrow",
				 "NO_MORE_ROWS", TCL_GLOBAL_ONLY);
		    SybProcs[hand].last_next = NO_MORE_ROWS;

		    return TCL_OK;   /* null string returned */
		}

		/* DBROWS returned SUCCEED */
		/* from here, just fall through to the outer context */
		/* and call dbnextrow on this new set of results */


	    }

	} else {
	    /* sybnext return  NO_MORE_RESULTS last time, tell'm again! */
	    Tcl_SetVar2(interp, SybMsgArray, "nextrow","NO_MORE_RESULTS", 
			TCL_GLOBAL_ONLY);
	    SybProcs[hand].last_next = NO_MORE_ROWS;
	    return TCL_OK;   /* null string returned */
	}
    }



    dbret = dbnextrow(SybProcs[hand].dbproc);

    SybProcs[hand].last_next = dbret;

    if (dbret == NO_MORE_ROWS) {
	Tcl_SetVar2(interp, SybMsgArray, "nextrow","NO_MORE_ROWS", 
		    TCL_GLOBAL_ONLY);
	if (dbhasretstat(SybProcs[hand].dbproc) == TRUE) {
	    sprintf(buf,"%d",dbretstatus(SybProcs[hand].dbproc));
	    Tcl_SetVar2(interp,SybMsgArray, "retstatus", buf, TCL_GLOBAL_ONLY);
	}
	return TCL_OK;
    }

    /* process regular rows or a compute row (where dbret > 0) */

    if ((dbret == REG_ROW) || (dbret > 0)) {
	/* set the "nextrow" message, and get the number of columns */
	if (dbret == REG_ROW) {
	    strcpy(buf,"REG_ROW");
	    num_cols = dbnumcols(SybProcs[hand].dbproc);
	} else {
	    sprintf(buf,"%d",dbret);	/* the compute id as integer */
	    num_cols = dbnumalts(SybProcs[hand].dbproc,dbret);
	}
	Tcl_SetVar2(interp, SybMsgArray, "nextrow", buf, TCL_GLOBAL_ONLY);

	/* parse the columns */

	for (i = 1; i <= num_cols; i++) {

            /* use normal row data access or alternate compute row access */
	    if (dbret == REG_ROW) {
		col_type = dbcoltype(SybProcs[hand].dbproc,i);
		col_len  = dbdatlen(SybProcs[hand].dbproc,i);
		col_ptr  = dbdata(SybProcs[hand].dbproc,i);
	    } else {
		col_type = dbalttype(SybProcs[hand].dbproc,dbret,i);
		col_len  = dbadlen(SybProcs[hand].dbproc,dbret,i);
		col_ptr  = dbadata(SybProcs[hand].dbproc,dbret,i);
	    }

	    /* call parse_column to convert to string and append to result */
	    if (parse_column(interp, hand, col_type, col_len, col_ptr) == 0) {
		return TCL_ERROR;
	    }

	}

	return TCL_OK;
    }



    /* if we get here, then the nextrow result was wacky, bail out */    
    sprintf(buf,"%d",dbret);
    Tcl_AppendResult (interp, argv[0], ": handle ", argv[1],
		     " had bad results from dbnextrow, return code = ", buf, 
		     (char *) NULL);
    return TCL_ERROR;

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
Sybtcl_NextAll (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int      hand;
    char     buf[SYB_BUFF_SIZE];
    char    *eval_ptr;
    int      colsCnt;
    char   **colsPtr;
    int      i;
    char    *s;
    char    *s2;
    char    *p;
    int      max_col = 0;
    int      prev_result;

#define NUMSTRS 300		/* number of individual strings to allow*/
#define SUBCHAR '@'		/* default substitution character */

    char     subchar = SUBCHAR;	/* substitution character */
    int      num_str = 0;	/* number of strings to concat */
    char    *str[NUMSTRS];      /* strings to pass to concat   */

    int      colnum  = 0; 	/* number of colstr's used */

    struct {			
      int      column; 		/* column number index */
      int      strnum;          /* str array index */
    } colstr[NUMSTRS];

    char     static_command[SYB_BUFF_SIZE];
    char    *cmd_str;

#define FREE_CMD_STR  if (cmd_str != &static_command[0]) ckfree(cmd_str)

    Tcl_DString evalStr;	/* tcl dynamic string for building eval */
    Tcl_DString varStr;         /* tcl dynamic string for building varname */
    Tcl_DString resStr;         /* tcl dynamic string for prev interp result */
    int      parm_cnt;
    int        inum;
    int        icol;
    int        tcl_rc;
    int        id;


    if ((hand = syb_prologue(interp, argc, argv, 3, 
	 " cur_handle  commands  ?sub_char? ? [ tclvar colnum ] ...?")) == -1) {
	return TCL_ERROR;
    }

    /* check if already exahusted or nothing to fetch */
    /* except if sql was submitted async */

    if (SybProcs[hand].last_results == NO_MORE_RESULTS &&
	SybProcs[hand].async == 0) {
	/* sybnext return  NO_MORE_RESULTS last time, tell'm again! */
	Tcl_SetVar2(interp, SybMsgArray, "nextrow","NO_MORE_RESULTS", 
			TCL_GLOBAL_ONLY);
        SybProcs[hand].last_next = NO_MORE_ROWS;

	return TCL_OK;
    }
	
    /* check for user defined substitution character */

    if (argc >= 4) {
	subchar = *argv[3];
	/* don't let subchar be a space */
	if (subchar == ' ') {
	    subchar = '\0';
	}
    }

    /* make a copy of the command string, dont use a DString */
    i = strlen(argv[2]);
    if (i+1 < SYB_BUFF_SIZE) {
        cmd_str = &static_command[0];
    } else {
        cmd_str = ckalloc(i+1);
    }
    strcpy(cmd_str, argv[2]);


    /* parse tcl_stmt for all '@' substitutions */

    s  = cmd_str;			/* start of tcl_stmt */
    s2 = s;

    while ( (p = strchr(s2,subchar)) != NULL && subchar != '\0') {

      if (num_str >= NUMSTRS || colnum >= NUMSTRS) {
	  Tcl_AppendResult (interp, argv[0],
			": too many column substitutions, 300 max",
			  (char *) NULL);
	  FREE_CMD_STR;
	  return TCL_ERROR;
      }

      if (isdigit(*(p+1))) {		/* it's a substituion value ! */

	  if (s != p) {			/* subchar not at the front of string?*/
	      str[num_str] = s;		/* save the current front of string */
	      *p = '\0';  		/* terminate the literal string     */
	      num_str++;		/* into str array and               */
	  }

          i = atoi(p + 1);		/* the column number */

	  max_col = (i > max_col) ? i : max_col;/* sanity check, max col num */

	  colstr[colnum].column = i;		/* save column number */
	  colstr[colnum].strnum = num_str;	/* and str positiion  */
	  str[num_str] = "";			/* sanity check       */
	  colnum++;
	  num_str++;
	  p++;				/* skip past col number */
	  while (isdigit(*p)) {
	      p++;
	  }
	  s  = p;			/* next point to look for subchar */
	  s2 = p;			


      } else {				/* it's a subchar without number */
	  s2 = p + 1;			/* ignore and keep searching     */
	  if (*s2 == subchar) {		/* another subchar? it's quoted  */
	     bcopy(s2+1,s2,strlen(s2)+1);  /* so collapse memory by one  */
	     s2++;
	  }
      }

    }

    if (num_str == 0) {    		/* check if no substitutions */
	str[num_str] = cmd_str;	
	num_str = 1;	
    } else {		
	if (strlen(s) > (unsigned int) 0) {  /* remainder of tcl_stmt string */
            str[num_str] = s;
	    num_str++;
	}
    }


    /* loop until fetch exhausted */

    if (Sybtcl_Next(clientData, interp, 2, argv) == TCL_ERROR) {
	Tcl_AppendResult (interp, ": ", argv[0], ": sybnext failed", 
			  (char *) NULL);
	FREE_CMD_STR;
	return TCL_ERROR;
    }
    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
        if (SybProcs[hand].last_results == NO_MORE_RESULTS) {
	    Tcl_SetVar2(interp, SybMsgArray, "nextrow","NO_MORE_RESULTS", 
			    TCL_GLOBAL_ONLY);
	} else {
	    Tcl_SetVar2(interp, SybMsgArray, "nextrow","NO_MORE_ROWS", 
			    TCL_GLOBAL_ONLY);
	}
	FREE_CMD_STR;
	return TCL_OK;
    }

    /* make sure there are enough columns in result */
    i = 0;

    /* check for a regular row */
    if (SybProcs[hand].last_next == REG_ROW) {
	i = dbnumcols(SybProcs[hand].dbproc);
    } else {
	id = SybProcs[hand].last_next;  /* the compute row id */
	i = dbnumalts(SybProcs[hand].dbproc,id);
    }

    prev_result = SybProcs[hand].last_next;

    if (max_col > i) {
	Tcl_AppendResult (interp, argv[0], ": @column number execeeds results", 
			  (char *) NULL);
	FREE_CMD_STR;
	return TCL_ERROR;
    }

    Tcl_DStringInit(&resStr);

    while (SybProcs[hand].last_next != NO_MORE_ROWS) {

	Tcl_DStringGetResult(interp, &resStr);

	/* if crossing result sets, e.g. REG_ROW to a compute row    */
	/* then buffer the result and return from NextAll processing */

        if (prev_result != SybProcs[hand].last_next) {
            Tcl_DStringFree(&SybProcs[hand].bufferedStr);
            SybProcs[hand].bufferedRow     = 1;
	    Tcl_DStringAppend(&SybProcs[hand].bufferedStr,
						Tcl_DStringValue(&resStr),-1);
	    Tcl_ResetResult(interp);
	    Tcl_DStringFree(&resStr);
	    FREE_CMD_STR;
	    return TCL_OK;
	}


        /* split the result columns left in interp->result */
	if (Tcl_SplitList(interp,Tcl_DStringValue(&resStr),
		                        &colsCnt,&colsPtr) == TCL_ERROR) {
	   Tcl_AppendResult (interp, argv[0], ": split columns failed", 
			      (char *) NULL);
	   Tcl_DStringFree(&resStr);
	   FREE_CMD_STR;
	   return TCL_ERROR;
	}

        /* build eval string from literals and columns */
        Tcl_DStringInit(&evalStr);
	eval_ptr = "";
	for (inum=0, icol=0; inum < num_str; inum++) {
	    if (icol < colnum && inum == colstr[icol].strnum) {
		eval_ptr = Tcl_DStringAppend(&evalStr, " ", -1);
		if (colstr[icol].column == 0) {    /* col 0 is entire result */
		    eval_ptr = Tcl_DStringAppendElement(&evalStr,
					        Tcl_DStringValue(&resStr));
                } else {
		    if (colstr[icol].column > colsCnt) {/* another sanity chk*/
			Tcl_ResetResult(interp);
			Tcl_AppendResult (interp, argv[0], 
			 ": column sanity failed on column sub",(char *) NULL);
	                Tcl_DStringFree(&evalStr); 	/* free eval string */
	                ckfree((char *) colsPtr); 	/* free split array */
			Tcl_DStringFree(&resStr);
			FREE_CMD_STR;
			return TCL_ERROR;
		    }
		    eval_ptr = Tcl_DStringAppendElement(&evalStr,
					  colsPtr[colstr[icol].column - 1]);
		}
		eval_ptr = Tcl_DStringAppend(&evalStr, " ", -1);
	        icol++;
	    } else {
	        eval_ptr = Tcl_DStringAppend(&evalStr, str[inum], -1);
	    }
	}

	/* set any remaining "tclvar colnum" pairs as tcl variables */
	if (argc >= 5) {
	    Tcl_DStringInit(&varStr);
	    parm_cnt = 4;
	    while (argc > parm_cnt + 1) {       /* always in pairs of two */
		p    =  argv[parm_cnt];          /* variable name */
		icol =  atoi(argv[parm_cnt+1]);  /* column number */
		/* Tcl_SetVar can muck with varname, so save it off */
		Tcl_DStringAppend(&varStr, p, -1);
		if (icol == 0) {
		    Tcl_SetVar(interp, Tcl_DStringValue(&varStr),
			   Tcl_DStringValue(&resStr), 0);  /* row as list */
		} else {
		    if (icol <= colsCnt + 1) {
			Tcl_SetVar(interp, Tcl_DStringValue(&varStr),
			   colsPtr[icol-1], 0);
		    } else {
			Tcl_ResetResult(interp);
			Tcl_AppendResult (interp, argv[0],
			 ": column sanity failed on tclvar bind",(char *) NULL);
			Tcl_DStringFree(&evalStr);      /* free eval string */
			ckfree((char *) colsPtr);       /* free split array */
			Tcl_DStringFree(&resStr);
			FREE_CMD_STR;
			return TCL_ERROR;
		    }
		}
		parm_cnt += 2;
		Tcl_DStringSetLength(&varStr,0);
	    }

	    Tcl_DStringFree(&varStr);   /* free var string */
	}

        tcl_rc = Tcl_Eval(interp, eval_ptr);	/* do it! */

	Tcl_DStringFree(&evalStr); 	/* free eval string */

	ckfree((char *) colsPtr); 	/* free split array */

	switch (tcl_rc) {		/* check on eval return code */
	  case TCL_ERROR:
	    Tcl_AppendResult (interp, ": eval failed while in ", argv[0], 
			      (char *) NULL);
	    Tcl_DStringFree(&resStr);
	    FREE_CMD_STR;
	    return TCL_ERROR;
	    break;

	  case TCL_BREAK:		/* return sooner */
	    Tcl_DStringFree(&resStr);
	    FREE_CMD_STR;
	    return TCL_OK;
	    break;

	  default:
	    break;
	}

	Tcl_ResetResult(interp);	/* reset interp result for next */
	Tcl_DStringSetLength(&resStr,0);
 
        /* next fetch */
        if (Sybtcl_Next(clientData, interp, 2, argv) == TCL_ERROR) {
	    Tcl_AppendResult (interp, ": ", argv[0], ": sybnext failed", 
			      (char *) NULL);
	    Tcl_DStringFree(&resStr);
	    FREE_CMD_STR;
	    return TCL_ERROR;
        }

    }

    Tcl_DStringFree(&resStr);
    FREE_CMD_STR;

    return TCL_OK;

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

Sybtcl_Cols (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    RETCODE dbret;
    char   *bufptr;
    int     num_cols;
    int     id;
    int     i;
    char    len_buf[SYB_BUFF_SIZE];
    char    typ_buf[SYB_BUFF_SIZE];
    char    buf2[SYB_BUFF_SIZE];

    if ((hand = syb_prologue(interp, argc, argv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }

    len_buf[0] = '\0';   /* initial null buffers */
    typ_buf[0] = '\0';   
    buf2[0]= '\0';

    if (SybProcs[hand].last_results == SUCCEED) {

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
		    Tcl_AppendElement(interp,bufptr);
		} else {
		    Tcl_AppendElement(interp," ");
		}

		/* get the return parm length and append to "collengths" */
		sprintf(buf2, (i>1) ? " %d" : "%d",
				     dbretlen(SybProcs[hand].dbproc,i));
		strcat(len_buf,buf2);

		/* get the column type and append to "coltypes" */
		sprintf(buf2, (i>1) ? " %s" : "%s",
		     dbprtype(dbrettype(SybProcs[hand].dbproc,i)) );
		strcat(typ_buf,buf2);

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
			Tcl_AppendElement(interp,bufptr);
		    } else {
			Tcl_AppendElement(interp," ");
		    }

		    /* get the column length and append to "collengths" */
		    sprintf(buf2, (i>1) ? " %d" : "%d",
					 dbcollen(SybProcs[hand].dbproc,i));
		    strcat(len_buf,buf2);

		    /* get the column type and append to "coltypes" */
		    sprintf(buf2, (i>1) ? " %s" : "%s",
			 dbprtype(dbcoltype(SybProcs[hand].dbproc,i)) );
		    strcat(typ_buf,buf2);

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
			Tcl_AppendElement(interp,buf2);

		    /* get the return parm length and append to "collengths" */
		    sprintf(buf2, (i>1) ? " %d" : "%d",
					 dbadlen(SybProcs[hand].dbproc,id,i));
		    strcat(len_buf,buf2);

		    /* get the column type and append to "coltypes" */
		    sprintf(buf2, (i>1) ? " %s" : "%s",
			 dbprtype(dbalttype(SybProcs[hand].dbproc,id,i)) );
		    strcat(typ_buf,buf2);

		}
	    }
	}

	Tcl_SetVar2(interp, SybMsgArray, "collengths", len_buf,TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, SybMsgArray, "coltypes",   typ_buf,TCL_GLOBAL_ONLY);

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

Sybtcl_Cancel (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    RETCODE dbret;
    char   *bufptr;

    if ((hand = syb_prologue(interp, argc, argv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;
    SybProcs[hand].bufferedRow  = 0;
    Tcl_DStringFree(&SybProcs[hand].bufferedStr);

    dbcancel(SybProcs[hand].dbproc);

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

Sybtcl_Retval (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    RETCODE dbret;
    int     i;
    int     num_cols;
    int     col_type;
    int     col_len;
    BYTE   *col_ptr;
    int     dst_len;


    if ((hand = syb_prologue(interp, argc, argv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }

    /* dbnextrow() must have been exhausted before access to return values */
    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
	num_cols = dbnumrets(SybProcs[hand].dbproc);
	for (i = 1; i <= num_cols; i++) {
	    col_type = dbrettype(SybProcs[hand].dbproc,i);
	    col_len  = dbretlen (SybProcs[hand].dbproc,i);
	    col_ptr  = dbretdata(SybProcs[hand].dbproc,i);

	    /* call parse_column to convert to string and append to result */
	    if (parse_column(interp, hand, col_type, col_len, col_ptr) == 0) {
		return TCL_ERROR;
	    }

	}
    }


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

Sybtcl_Close (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    RETCODE dbret;

    if ((hand = syb_prologue(interp, argc, argv, 2, " handle")) == -1) {
	return TCL_ERROR;
    }

    dbclose(SybProcs[hand].dbproc);

    SybProcs[hand].last_results = NO_MORE_RESULTS;
    SybProcs[hand].last_next    = NO_MORE_ROWS;
    SybProcs[hand].in_use       = 0;
    SybProcs[hand].dbproc       = NULL;
    SybProcs[hand].bufferedRow     = 0;
    Tcl_DStringFree(&SybProcs[hand].bufferedStr);

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Wrtext --
 *    Implements the sybwritetext command:
 *    usage: sybwritetext handle object colnum file ?-nolog?
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

Sybtcl_Wrtext (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    int     fd;
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
    struct  stat stat_buf;

    if ((hand = syb_prologue(interp, argc, argv, 5, 
	    " handle object colnum filename ?-nolog? ")) == -1) {
	return TCL_ERROR;
    }

    col = atoi(argv[3]);
    if (col <= 0) {
        Tcl_AppendResult (interp, argv[0], ": column number ", argv[3],
		     " not valid ", (char *) NULL);
        return TCL_ERROR;
    }

#ifdef macintosh
    fd = open(argv[4], O_RDONLY);
#else
    fd = open(argv[4], O_RDONLY | _O_BINARY, 0);
#endif

    if (fd < 0) {
        Tcl_AppendResult (interp, argv[0], ": file ", argv[4],
		     " cannot be opened for reading ", (char *) NULL);
        return TCL_ERROR;
    }

    fstat(fd, &stat_buf);
    filesize = stat_buf.st_size;

    /* check if nolog specified */
    if ( (argc >= 6 && strncmp(argv[5],"nolog",5) == 0)  ||
         (argc >= 6 && strncmp(argv[5],"-nolog",6)== 0) ) {
      log = FALSE;
    } else {
      log = TRUE;
    }

    dbret = dbnextrow(SybProcs[hand].dbproc);

    /* get textpointer and timestamp */
    txtptr = dbtxptr(SybProcs[hand].dbproc,col);
    if (txtptr == NULL) {
        close (fd);
        Tcl_AppendResult (interp, argv[0], ": dbtxptr failed ",
                      (char *) NULL);
        return TCL_ERROR;
    } else {
        bcopy(txtptr, text_pointer, DBTXPLEN);
        txtptr = &text_pointer[0];
    }
    timestmp = dbtxtimestamp(SybProcs[hand].dbproc,col);
    if (timestmp == NULL) {
        close (fd);
        Tcl_AppendResult (interp, argv[0], ": dbtxtimestamp failed ",
                      (char *) NULL);
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
    dbret = dbwritetext(SybProcs[hand].dbproc, argv[2], 
			txtptr, DBTXPLEN, timestmp, log, filesize, NULL);

    if (dbret == FAIL) {   /* fail probaly means bad dbtxptr() */
	close (fd);
        Tcl_AppendResult (interp, argv[0], ": dbwritetext failed ",
		      (char *) NULL);
        return TCL_ERROR;
    }

    /* make sure server is idle */
    dbret = dbsqlok(SybProcs[hand].dbproc);
    if (dbret == FAIL) {
	close (fd);
        Tcl_AppendResult (interp, argv[0], 
		    ": dbsqlok after dbwritetext failed ", (char *) NULL);
        return TCL_ERROR;
    } else {
        dbret = dbresults(SybProcs[hand].dbproc);
        while ( (dbret != NO_MORE_RESULTS) && (dbret != FAIL) ) {
            dbret = dbresults(SybProcs[hand].dbproc);
        }
    }


    /* send the text/image */
    while ((s=read(fd,buf,TEXT_BUFF_SIZE)) > 0) {
        total_bytes += s;
	dbret = dbmoretext(SybProcs[hand].dbproc, s, (BYTE *) buf);
	if (dbret == FAIL) {
	    close (fd);
	    Tcl_AppendResult (interp, argv[0], ": dbmoretext failed ",
			  (char *) NULL);
	    return TCL_ERROR;
	}
    }

    close (fd);

    /* let server finish */
    dbret = dbsqlok(SybProcs[hand].dbproc);
    if (dbret == FAIL) { 
        Tcl_AppendResult (interp, argv[0], 
			": dbsqlok after dbmoretext failed ", (char *) NULL);
        return TCL_ERROR;
    }

    dbret = dbresults(SybProcs[hand].dbproc);
    while ( (dbret != NO_MORE_RESULTS) && (dbret != FAIL) ) {
	dbcanquery(SybProcs[hand].dbproc);
        dbret = dbresults(SybProcs[hand].dbproc);
    }

    /* return total bytes sent */
    sprintf(buf,"%d",total_bytes);
    Tcl_SetResult (interp,buf,TCL_VOLATILE);

    return TCL_OK;

}




/*
 *----------------------------------------------------------------------
 *
 * Sybtcl_Rdtext --
 *    Implements the sybreadtext command:
 *    usage: sybreadtext handle file 
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


Sybtcl_Rdtext (clientData, interp, argc, argv)
    ClientData   clientData;
    Tcl_Interp  *interp;
    int          argc;
    char       **argv;
{
    int     hand;
    int     fd;
    int     s;
    int     total_bytes = 0;
    LARGE_CHAR buf[TEXT_BUFF_SIZE];
    RETCODE dbret;

    if ((hand = syb_prologue(interp, argc, argv, 3, 
			    " handle filename")) == -1) {
	return TCL_ERROR;
    }

    /* check for a row returned */
    if (SybProcs[hand].last_next == NO_MORE_ROWS) {
        Tcl_SetResult (interp,"0",TCL_VOLATILE);
        return TCL_OK;
    }
    /* check for one and only one column */
    if (dbnumcols(SybProcs[hand].dbproc) != 1) {
        Tcl_SetResult (interp,"0",TCL_VOLATILE);
        return TCL_OK;
    }

#ifdef macintosh
    fd = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT);
#else
    fd = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT | _O_BINARY, 0644);
#endif

    if (fd == -1) {
        Tcl_AppendResult (interp, argv[0], ": file ", argv[2],
		     " could not be opened for writing ", (char *) NULL);
        return TCL_ERROR;
    }

    while ((s=dbreadtext(SybProcs[hand].dbproc, (BYTE *)buf, TEXT_BUFF_SIZE)) != 
							       NO_MORE_ROWS) {
	if (s == -1) {
            Tcl_AppendResult (interp, argv[0], ": dbreadtext failed ",
		      (char *) NULL);
	    close(fd);
            return TCL_ERROR;
	}
	if (s > 0) {
	    total_bytes += s;
	    write(fd, buf, s);
	}
    }

    close (fd);

    /* return total bytes sent */
    sprintf(buf,"%d",total_bytes);
    Tcl_SetResult (interp,buf,TCL_VOLATILE);

    return TCL_OK;

}


/* finis */
