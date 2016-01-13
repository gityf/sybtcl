/* sybtcl-capi - C api to Sybtcl */

/* sybtcl-capi.h interface stuff */

#ifndef _SYBTCL_CAPI
#define _SYBTCL_CAPI

/* include our fake tcl.h */
#include "tcl.h"

/* make defines for each "element" of sybmsg */
/* these must be in the same order as the index values !! */

#define Msgno		0
#define Msgtext		1
#define Severity	2
#define Svrname		3
#define Procname	4
#define Line		5
#define Dberr		6
#define Oserr		7
#define Dberrstr	8
#define Oserrstr	9
#define Nextrow		10
#define Retstatus	11
#define Collengths	12
#define Coltypes	13
#define Handle		14
#define Nullvalue	15
#define Floatprec	16
#define Maxtext		17
#define Dateformat	18
#define Fixedchar  	19
#define Version    	20

/* make Maxstatus same as last status message */
#define Maxstatus	20

/*
"msgno"
"msgtext"
"severity"
"svrname"
"procname"
"line"
"dberr"
"oserr"
"dberrstr"
"oserrstr"
"nextrow"
"retstatus"
"collengths"
"coltypes"
"handle"
"nullvalue"
"floatprec"
"maxtext"
"dateformat"
"fixedchar"
"version"
*/

#ifndef NULL
#define NULL 0
#endif


/* macros to access status, results, columns, number of columns */

#define GET_STATUS(interp,i)     \
    (interp->status[i]==NULL||i<0||i>Maxstatus?"":interp->status[i])

#define RESULT(interp) 		 (interp->column[0]==NULL?"":interp->column[0])

#define COLUMN(interp,i) 	 \
    (interp->column[i]==NULL||i<0||i>=interp->num_columns?"":interp->column[i])

#define NUMCOLS(interp)		 (interp->num_columns)



/* the sybtcl-capi functions */

EXTERN Tcl_Interp * Sybtcl_MkInterp _ANSI_ARGS_((void));

EXTERN int  Sybtcl_Init _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void Sybtcl_FreeInterp _ANSI_ARGS_((Tcl_Interp *interp));


EXTERN int Sybconnect _ANSI_ARGS_((Tcl_Interp *interp, char *uid, char *pw, 
			char *server, char *app, char *ifile));

EXTERN int Sybuse _ANSI_ARGS_((Tcl_Interp *interp, char *handle, char *db));

EXTERN int Sybsql _ANSI_ARGS_((Tcl_Interp *interp, char *handle, char *sql, 
			char *async));

EXTERN int Sybpoll _ANSI_ARGS_((Tcl_Interp *interp, char *handle, 
			char *timeout, char *all));

EXTERN int Sybnext _ANSI_ARGS_((Tcl_Interp *interp, char *handle));

EXTERN int Sybcols _ANSI_ARGS_((Tcl_Interp *interp, char *handle));

EXTERN int Sybcancel _ANSI_ARGS_((Tcl_Interp *interp, char *handle));

EXTERN int Sybretval _ANSI_ARGS_((Tcl_Interp *interp, char *handle));

EXTERN int Sybclose _ANSI_ARGS_((Tcl_Interp *interp, char *handle));

EXTERN int Sybwritetext _ANSI_ARGS_((Tcl_Interp *interp, char *handle, 
			char *object, char *colnum, char *file, char *nolog));

EXTERN int Sybreadtext _ANSI_ARGS_((Tcl_Interp *interp, char *handle, 
			char *file));


#endif /* _SYBTCL_CAPI */

