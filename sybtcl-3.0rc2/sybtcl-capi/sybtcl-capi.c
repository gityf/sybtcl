/* sybtcl-capi - fake tcl core routines and other support */

/* Copyright Tom Poindexter, 1995 */

/* include our fake tcl.h & sybtcl-api.h */

#include "sybtcl-capi.h"  	


#include <varargs.h>
#include <string.h>

/* status indexes, normally in tcl sybmsg array */
/* these must agree with defines in sybtcl-capi.h !!    */

static char *SybStatus[] = {
	{  "msgno"	},
	{  "msgtext"	},
	{  "severity"	},
	{  "svrname"	},
	{  "procname"	},
	{  "line"	},
	{  "dberr"	},
	{  "oserr"	},
	{  "dberrstr"	},
	{  "oserrstr"	},
	{  "nextrow"	},
	{  "retstatus"	},
	{  "collengths" },
	{  "coltypes"	},
	{  "handle"	},
	{  "nullvalue"	},
	{  "floatprec"	},
	{  "maxtext"	},
	{  "dateformat" },
	{  "fixedchar"  },
	{  "version"    }
};

#define MAX_STATUS (sizeof SybStatus / sizeof (char *))


/* sybtclc helper functions */

Tcl_Interp * Sybtcl_MkInterp () 
{
  int i;
  Tcl_Interp *interp;

  interp = (Tcl_Interp *) malloc (sizeof(Tcl_Interp));
  interp->column = (char **) malloc ((sizeof (char *)) * 256); 
  for (i = 0; i < 256; i++) {
    interp->column[i] = (char *) NULL;
  }
  interp->result = (char *) NULL;

  interp->status = (char **) malloc ((sizeof (char *)) * MAX_STATUS); 
  for (i = 0; i < MAX_STATUS; i++) {
    interp->status[i] = (char *) NULL;
  }

  return interp;
}

void Sybtcl_FreeInterp (interp)
  Tcl_Interp *interp;
{
  int i;
  for (i = 0; i < MAX_STATUS; i++) {
    if (interp->status[i] != NULL) {
      free (interp->status[i]);
    }
  }
  Tcl_ResetResult(interp);
  free(interp);
}



/* sybtcl-capi emulated tcl functions */


void Tcl_ResetResult (interp) 
  Tcl_Interp *interp;
{
  int i;
  for (i = 0; i < 256; i++) {
    if (interp->column[i] != NULL) {
      free (interp->column[i]);
      interp->column[i] = (char *)NULL;
    }
  }
  interp->num_columns = 0;
}


void Tcl_SetResult (interp, buf, freeProc) 
  Tcl_Interp *interp;
  char *buf; 
  Tcl_FreeProc *freeProc; 
{
  Tcl_ResetResult(interp);
  interp->column[0] = (char *) malloc(strlen(buf)+1);
  strcpy(interp->column[interp->num_columns],buf);
  interp->num_columns = 1;
  interp->result = interp->column[0];
}



#ifndef lint
void
Tcl_AppendResult(va_alist)
#else
void
        /* VARARGS2 */ /* ARGSUSED */
Tcl_AppendResult(interp, p, va_alist)
    Tcl_Interp *interp;         /* Interpreter whose result is to be
                                 * extended. */
    char *p;                    /* One or more strings to add to the
                                 * result, terminated with NULL. */
#endif
    va_dcl
{
    va_list argList;
    register Tcl_Interp *iPtr;
    char *string;
    int newSpace;
    int oldSpace;
    char *buf2;
 
    /*
     * First, scan through all the arguments to see how much space is
     * needed.
     */
 
    va_start(argList);
    iPtr = va_arg(argList, Tcl_Interp *);
    newSpace = 0;
    while (1) {
        string = va_arg(argList, char *);
        if (string == NULL) {
            break;
        }
        newSpace += strlen(string);
    }
    va_end(argList);

    /*
     * If the append buffer isn't already setup and large enough
     * to hold the new data, set it up.
     */
 
    if (iPtr->column[0] != NULL) {
      oldSpace += strlen(iPtr->column[0]);
    } 

    newSpace += oldSpace + 1;
    buf2 = (char *) malloc (newSpace);
    if (iPtr->column[0] != NULL) {
      strcpy(buf2,iPtr->column[0]);
      free (iPtr->column[0]);
    } 
    iPtr->column[0] = buf2;
 
    /*
     * Final step:  go through all the argument strings again, copying
     * them into the buffer.
     */
 
    va_start(argList);
    (void) va_arg(argList, Tcl_Interp *);
    while (1) {
        string = va_arg(argList, char *);
        if (string == NULL) {
            break;
        }
        strcat(iPtr->column[0],string);
    }
    va_end(argList);
    iPtr->num_columns = 1;
    iPtr->result = iPtr->column[0];
}



void Tcl_AppendElement (interp, buf) 
  Tcl_Interp *interp; 
  char *buf;
{
  interp->column[interp->num_columns] = (char *) malloc(strlen(buf)+1);
  strcpy(interp->column[interp->num_columns],buf);
  interp->num_columns++;
  interp->result = interp->column[0];
}


char *Tcl_SetVar2 (interp, arrname, index, buf, flag) 
  Tcl_Interp *interp; 
  char *arrname; 
  char *index; 
  char *buf; 
  int flag; 
{
  int i;
  for (i = 0; i < MAX_STATUS; i++) {
    if (strcmp(index,SybStatus[i]) == 0) { 
      if (interp->status[i] != NULL) {
	free (interp->status[i]);
      }
      interp->status[i] = (char *) malloc (strlen(buf)+1);
      strcpy(interp->status[i],buf);
      return ( interp->status[i] );
    }
  }
  return "";
}
 
char *Tcl_GetVar2 (interp, arrname, index, flag) 
  Tcl_Interp *interp;
  char *arrname;
  char *index;
  int flag; 
{
  int i;
  for (i = 0; i < MAX_STATUS; i++) {
    if (strcmp(index,SybStatus[i]) == 0) { 
      return (interp->status[i] == NULL ? "" : interp->status[i]);
    }
  }
  return "";
}
 
char * Tcl_DStringAppend (dsPtr, string, length)
  Tcl_DString *dsPtr;
  char *string; 
  int length; 
{ return (char *) strcat(dsPtr->string, string); }

int Tcl_DStringLength (dsPtr)
  Tcl_DString *dsPtr;
{ return strlen(dsPtr->string);}

void Tcl_DStringFree (dsPtr)
  Tcl_DString *dsPtr;
{ dsPtr->string[0] = '\0'; return; }

void Tcl_DStringInit (dsPtr)
  Tcl_DString *dsPtr;
{ dsPtr->string[0] = '\0'; return;}



/* now define the C api interfaces to sybtcl */

/* count argv list, stop at first NULL */

static int count_args (argv, max)
char *argv[];
int   max;
{
  int i;
  for (i = 0; i < max; i++) {
    if (argv[i] == NULL) {
      break;
    }
  }
  return i;
}

int Sybconnect (interp, uid, pw, server, app, ifile)
  Tcl_Interp *interp;
  char *uid;
  char *pw;
  char *server; 
  char *app;
  char *ifile;
{
  char *argv[6];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp); 

  argv[0] = "sybconnect";
  argv[1] = uid;
  argv[2] = pw;
  argv[3] = server;
  argv[4] = app;
  argv[5] = ifile;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Connect (dummy, interp, argc, argv ) );
}


int Sybuse (interp, handle, db)
  Tcl_Interp *interp;
  char *handle;
  char *db;
{
  char *argv[3];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp); 

  argv[0] = "sybuse";
  argv[1] = handle;
  argv[2] = db;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Use (dummy, interp, argc, argv ) );

}

int Sybsql (interp, handle, sql, async) 
  Tcl_Interp *interp;
  char *handle;
  char *sql; 
  char *async;
{
  char *argv[4];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp);

  argv[0] = "sybsql";
  argv[1] = handle;
  argv[2] = sql;
  argv[3] = async;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Sql (dummy, interp, argc, argv ) );

}

int Sybpoll (interp, handle, timeout, all) 
  Tcl_Interp *interp;
  char *handle;
  char *timeout;
  char *all; 
{

  char *argv[4];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp);

  argv[0] = "sybpoll";
  argv[1] = handle;
  argv[2] = timeout;
  argv[3] = all;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Poll (dummy, interp, argc, argv ) );

}

int Sybnext (interp, handle) 
  Tcl_Interp *interp;
  char *handle;
{
  char *argv[2];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp); 

  argv[0] = "sybnext";
  argv[1] = handle;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Next (dummy, interp, argc, argv ) );

}

int Sybcols (interp, handle) 
  Tcl_Interp *interp;
  char *handle;
{
  char *argv[2];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp); 

  argv[0] = "sybcols";
  argv[1] = handle;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Cols (dummy, interp, argc, argv ) );

}

int Sybcancel (interp, handle) 
  Tcl_Interp *interp;
  char *handle;
{
  char *argv[2];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp);  

  argv[0] = "sybcancel";
  argv[1] = handle;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Cancel (dummy, interp, argc, argv ) );

}

int Sybretval (interp, handle) 
  Tcl_Interp *interp;
  char *handle;
{
  char *argv[2];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp);

  argv[0] = "sybretval";
  argv[1] = handle;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Retval (dummy, interp, argc, argv ) );

}

int Sybclose (interp, handle) 
  Tcl_Interp *interp;
  char *handle;
{
  char *argv[2];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp); 

  argv[0] = "sybclose";
  argv[1] = handle;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Close (dummy, interp, argc, argv ) );

}

int Sybwritetext (interp, handle, object, colnum, file, nolog) 
  Tcl_Interp *interp;
  char *handle;
  char *object;
  char *colnum;
  char *file;
  char *nolog;
{
  char *argv[6];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp); 

  argv[0] = "sybwritetext";
  argv[1] = handle;
  argv[2] = object;
  argv[3] = colnum;
  argv[4] = file;
  argv[5] = nolog;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Wrtext (dummy, interp, argc, argv ) );

}

int Sybreadtext (interp, handle, file) 
  Tcl_Interp *interp;
  char *handle;
  char *file; 
{
  char *argv[3];
  int  argc;
  ClientData dummy;

  Tcl_ResetResult(interp); 

  argv[0] = "sybreadtext";
  argv[1] = handle;
  argv[2] = file;

  argc = count_args(argv, sizeof(argv) / sizeof (char *));

  return (Sybtcl_Rdtext (dummy, interp, argc, argv ) );

}




/* sybtcl-capi dummied tcl functions */
/* these are referenced in sybtcl.c, but will/should never be called */

char * Tcl_DStringValue (dsPtr)
  Tcl_DString *dsPtr;
{ return (char *) NULL; }
 
void Tcl_DStringGetResult (interp, dsPtr)
  Tcl_Interp *interp;
  Tcl_DString *dsPtr;
{ return; }
 
void Tcl_DStringSetLength (dsPtr, length)
  Tcl_DString *dsPtr;
  int length;
{ return; }
 
char * Tcl_SetVar (interp, varName, newValue, flags)
  Tcl_Interp *interp;
  char *varName;
  char *newValue;
  int flags;
{ return (char *) NULL; }

void Tcl_CreateExitHandler (proc, clientData)
  Tcl_ExitProc *proc;
  ClientData clientData;
{ return; }


Tcl_Command  Tcl_CreateCommand (interp, cmdName, proc, 
			clientData, deleteProc)
  Tcl_Interp *interp;
  char *cmdName; 
  Tcl_CmdProc *proc;
  ClientData clientData;
  Tcl_CmdDeleteProc *deleteProc;
{ return (Tcl_Command) NULL; }

char * Tcl_DStringAppendElement (dsPtr, string)
  Tcl_DString *dsPtr;
  char *string;
{ return (char *) NULL;}

void Tcl_DStringResult (interp, dsPtr)
  Tcl_Interp *interp;
  Tcl_DString *dsPtr;
{ return; }

int Tcl_Eval (interp, cmd)
  Tcl_Interp *interp; 
  char *cmd;
{ return TCL_OK; }

int Tcl_SplitList (interp, list, argcPtr, argvPtr)
  Tcl_Interp *interp;
  char *list; 
  int *argcPtr;
  char ***argvPtr;
{ return TCL_OK;}


int Tcl_PkgProvide (interp, name, version)
  Tcl_Interp *interp;
  char *name;
  char *version;
{ return TCL_OK;}
 
