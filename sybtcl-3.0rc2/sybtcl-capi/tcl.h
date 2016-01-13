/* sybtcl-capi - C api to Sybtcl */
/* tcl.h - fake tcl core stuff   */

#ifndef _TCL
#define _TCL

#include <varargs.h>
#include <string.h>

/* swipe some defines from Tcl .... */

#undef _ANSI_ARGS_
#undef CONST
#if ((defined(__STDC__) || defined(SABER)) && !defined(NO_PROTOTYPE)) || defined (__cplusplus)
#   define _USING_PROTOTYPES_ 1
#   define _ANSI_ARGS_(x)       x
#   define CONST const
#   ifdef __cplusplus
#       define VARARGS(first) (first, ...)
#   else
#       define VARARGS(first) ()
#   endif
#else
#   define _ANSI_ARGS_(x)       ()
#   define CONST
#endif

#ifdef __cplusplus
#   define EXTERN extern "C"
#else
#   define EXTERN extern
#endif
 
/*
 * Macro to use instead of "void" for arguments that must have
 * type "void *" in ANSI C;  maps them to type "char *" in
 * non-ANSI systems.
 */
 
#ifndef VOID
#   ifdef __STDC__
#       define VOID void
#   else
#       define VOID char
#   endif
#endif

#define ckalloc malloc
#define ckfree  free 


#ifndef NULL
#define NULL 0
#endif


typedef int *ClientData;

/* fake interp struct, holds status and column pointers */

typedef struct Tcl_Interp {	
  char 	**status;
  int 	num_columns;
  char 	**column;
  char 	*result;
} Tcl_Interp;


#define TCL_DSTRING_STATIC_SIZE 200
typedef struct Tcl_DString {
    char string[20000];                 /* dummy big string stuff */
} Tcl_DString;



typedef void (Tcl_FreeProc) _ANSI_ARGS_((char *blockPtr));
typedef int *Tcl_Command;
typedef void (Tcl_CmdDeleteProc) _ANSI_ARGS_((ClientData clientData));
typedef int (Tcl_CmdProc) _ANSI_ARGS_((ClientData clientData,
        Tcl_Interp *interp, int argc, char *argv[]));
typedef void (Tcl_ExitProc) _ANSI_ARGS_((ClientData clientData));

#define TCL_OK		0
#define TCL_ERROR	1
#define TCL_RETURN      2
#define TCL_BREAK       3
#define TCL_CONTINUE    4
#define TCL_MAX_PREC	17

#define TCL_VOLATILE    ((Tcl_FreeProc *) 1)
#define TCL_GLOBAL_ONLY         1



/* sybtcl-capi emulated functions */


EXTERN void             Tcl_AppendElement _ANSI_ARGS_((Tcl_Interp *interp,
                            char *string));
EXTERN void             Tcl_AppendResult _ANSI_ARGS_(
                            VARARGS(Tcl_Interp *interp));
EXTERN Tcl_Command      Tcl_CreateCommand _ANSI_ARGS_((Tcl_Interp *interp,
                            char *cmdName, Tcl_CmdProc *proc,
                            ClientData clientData,
                            Tcl_CmdDeleteProc *deleteProc));
EXTERN char *           Tcl_GetVar2 _ANSI_ARGS_((Tcl_Interp *interp,
                            char *part1, char *part2, int flags));
EXTERN void             Tcl_ResetResult _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN void             Tcl_SetResult _ANSI_ARGS_((Tcl_Interp *interp,
                            char *string, Tcl_FreeProc *freeProc));
EXTERN char *           Tcl_SetVar2 _ANSI_ARGS_((Tcl_Interp *interp,
                            char *part1, char *part2, char *newValue,
                            int flags));

EXTERN char *           Tcl_DStringAppend _ANSI_ARGS_((Tcl_DString *dsPtr,
                            char *string, int length));
EXTERN void             Tcl_DStringFree _ANSI_ARGS_((Tcl_DString *dsPtr));
EXTERN void             Tcl_DStringInit _ANSI_ARGS_((Tcl_DString *dsPtr));
EXTERN int              Tcl_DStringLength _ANSI_ARGS_((Tcl_DString *dsPtr));


/* sybtcl-capi dummied functions */

EXTERN char *           Tcl_DStringValue _ANSI_ARGS_((Tcl_DString *dsPtr));
EXTERN void             Tcl_DStringGetResult _ANSI_ARGS_((Tcl_Interp *interp,
                            Tcl_DString *dsPtr));
EXTERN void             Tcl_DStringSetLength _ANSI_ARGS_((Tcl_DString *dsPtr,
                            int length));
EXTERN char *           Tcl_SetVar _ANSI_ARGS_((Tcl_Interp *interp,
                            char *varName, char *newValue, int flags));
EXTERN void             Tcl_CreateExitHandler _ANSI_ARGS_((Tcl_ExitProc *proc,
                            ClientData clientData));

EXTERN char *           Tcl_DStringAppendElement _ANSI_ARGS_((
                            Tcl_DString *dsPtr, char *string));
EXTERN void             Tcl_DStringResult _ANSI_ARGS_((Tcl_Interp *interp,
                            Tcl_DString *dsPtr));

EXTERN int              Tcl_Eval _ANSI_ARGS_((Tcl_Interp *interp, char *cmd));

EXTERN int              Tcl_SplitList _ANSI_ARGS_((Tcl_Interp *interp,
                            char *list, int *argcPtr, char ***argvPtr));
EXTERN int              Tcl_PkgProvide _ANSI_ARGS_((Tcl_Interp *interp,
			    char *name, char *version));


#endif /* _TCL */

