/*
 * sybdb.h
 *
 * ct lib compatibilty layer
 *
 * Copyright 1997 Tom Poindexter 
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.
 * Tom Poindexter makes no representations about the suitability
 * of this software for any purpose.  It is provided "as is" without express or
 * implied warranty.  By use of this software the user agrees to
 * indemnify and hold harmless Tom Poindexter from any
 * claims or liability for loss arising out of such use.
 */

#ifndef SYBDBH
#define SYBDBH

/* rip off nice ansi args macro from tcl.h */

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32)
#       define __WIN32__
#   endif
#endif

#ifdef __WIN32__
#   ifndef STRICT
#       define STRICT
#   endif
#   ifndef USE_PROTOTYPE
#       define USE_PROTOTYPE 1
#   endif
#   ifndef HAS_STDARG
#       define HAS_STDARG 1
#   endif
#   ifndef USE_PROTOTYPE
#       define USE_PROTOTYPE 1
#   endif
#   ifndef USE_TCLALLOC
#       define USE_TCLALLOC 1
#   endif
#   ifndef STRINGIFY
#       define STRINGIFY(x)         STRINGIFY1(x)
#       define STRINGIFY1(x)        #x
#   endif
#endif /* __WIN32__ */

#ifdef MAC_TCL
#   ifndef HAS_STDARG
#       define HAS_STDARG 1
#   endif
#   ifndef USE_TCLALLOC
#       define USE_TCLALLOC 1
#   endif
#   ifndef NO_STRERROR
#       define NO_STRERROR 1
#   endif
#endif
 

#ifndef _ANSI_ARGS_
#if ((defined(__STDC__) || defined(SABER)) && !defined(NO_PROTOTYPE)) || defined(__cplusplus) || defined(USE_PROTOTYPE)
#   define _USING_PROTOTYPES_ 1
#   define _ANSI_ARGS_(x)       x
#   define CONST const
#else
#   define _ANSI_ARGS_(x)       ()
#   define CONST
#endif
#endif

#ifndef EXTERN
#ifdef __cplusplus
#   define EXTERN extern "C"
#else
#   define EXTERN extern
#endif
#endif

#ifndef VOID
#ifndef __WIN32__
#ifndef VOID
#   ifdef __STDC__
#       define VOID void
#   else
#       define VOID char
#   endif
#endif
#else /* __WIN32__ */
/*
 * The following code is copied from winnt.h
 */
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif
#endif /* __WIN32__ */
#endif

#ifndef NULL
#define NULL 0
#endif


/* prototypes */

EXTERN  DBINT     dbconvert _ANSI_ARGS_((DBPROCESS *, int,BYTE *,DBINT,int,
			VOID *,DBINT));
EXTERN  RETCODE   dbdatecrack _ANSI_ARGS_((DBPROCESS *,DBDATEREC *,
			DBDATETIME *));
EXTERN  char      *dbmonthname _ANSI_ARGS_((DBPROCESS *,char *,int,DBBOOL));
EXTERN  VOID      dbexit _ANSI_ARGS_((VOID));
EXTERN  RETCODE   dbinit _ANSI_ARGS_((VOID));
EXTERN  RETCODE   dbsetversion _ANSI_ARGS_((DBINT));
EXTERN  RETCODE   dbsetmaxprocs _ANSI_ARGS_((int));
EXTERN  void      dberrhandle _ANSI_ARGS_((int (*handler)()));/*not void, but */
EXTERN  void      dbmsghandle _ANSI_ARGS_((int (*handler)()));/*ok for sybtcl */
EXTERN  LOGINREC  *dblogin _ANSI_ARGS_((VOID));
EXTERN  VOID      DBSETLUSER _ANSI_ARGS_((LOGINREC *, char *));
EXTERN  VOID      DBSETLPWD _ANSI_ARGS_((LOGINREC *, char *));
EXTERN  VOID      DBSETLAPP _ANSI_ARGS_((LOGINREC *, char *));
EXTERN  VOID      DBSETLCHARSET _ANSI_ARGS_((LOGINREC *, char *));
EXTERN  VOID      dbsetifile _ANSI_ARGS_((char *));
EXTERN  VOID      dbloginfree _ANSI_ARGS_((LOGINREC *));
EXTERN  DBPROCESS *dbopen _ANSI_ARGS_((LOGINREC *,char *));
EXTERN  int       DBIORDESC _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbuse _ANSI_ARGS_((DBPROCESS *,char *));
EXTERN  char      *dbname _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbcancel _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbsetopt _ANSI_ARGS_((DBPROCESS *,int,char *,int));
EXTERN  RETCODE   dbsqlexec _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbcmd _ANSI_ARGS_((DBPROCESS *,char *));
EXTERN  RETCODE   dbsqlsend _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbresults _ANSI_ARGS_((DBPROCESS *));
EXTERN  int       DBROWS _ANSI_ARGS_((DBPROCESS *));
EXTERN  DBBOOL    dbhasretstat _ANSI_ARGS_((DBPROCESS *));
EXTERN  DBINT     dbretstatus _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbpoll _ANSI_ARGS_((DBPROCESS *,long,DBPROCESS **,int *));
EXTERN  RETCODE   dbsqlok _ANSI_ARGS_((DBPROCESS *));
EXTERN  int       dbnextrow _ANSI_ARGS_((DBPROCESS *));
EXTERN  int       dbnumcols _ANSI_ARGS_((DBPROCESS *));
EXTERN  int       dbnumalts _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  DBINT     dbaltlen _ANSI_ARGS_((DBPROCESS *,int,int));
EXTERN  int       dbcoltype _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  DBINT     dbcollen _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  BYTE      *dbdata _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  DBINT     dbdatlen _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  int       dbalttype _ANSI_ARGS_((DBPROCESS *,int,int));
EXTERN  DBINT     dbadlen _ANSI_ARGS_((DBPROCESS *,int,int));
EXTERN  BYTE      *dbadata _ANSI_ARGS_((DBPROCESS *,int,int));
EXTERN  char      *dbretname _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  DBINT     dbnumrets _ANSI_ARGS_((DBPROCESS *));
EXTERN  DBINT     dbretlen _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  int       dbrettype _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  BYTE      *dbretdata _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  char      *dbcolname _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  char      *dbprtype _ANSI_ARGS_((int));
EXTERN  int       dbaltcolid _ANSI_ARGS_((DBPROCESS *,int,int));
EXTERN  int       dbaltop _ANSI_ARGS_((DBPROCESS *,int,int));
EXTERN  VOID      dbclose _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbcanquery _ANSI_ARGS_((DBPROCESS *));
EXTERN  RETCODE   dbwritetext _ANSI_ARGS_((DBPROCESS *, char *, DBBINARY *, 
				int, DBBINARY *, DBBOOL, DBINT, BYTE *));
EXTERN  DBBINARY  *dbtxptr _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  DBBINARY  *dbtxtimestamp _ANSI_ARGS_((DBPROCESS *,int));
EXTERN  RETCODE   dbmoretext _ANSI_ARGS_((DBPROCESS *,DBINT, BYTE *));
EXTERN  int       dbreadtext _ANSI_ARGS_((DBPROCESS *,BYTE *,DBINT));

EXTERN  RETCODE   dbmnyadd _ANSI_ARGS_((DBPROCESS *,DBMONEY *,DBMONEY *,
				DBMONEY *));
EXTERN  RETCODE   dbmnysub _ANSI_ARGS_((DBPROCESS *,DBMONEY *,DBMONEY *,
				DBMONEY *));
EXTERN  RETCODE   dbmnymul _ANSI_ARGS_((DBPROCESS *,DBMONEY *,DBMONEY *,
				DBMONEY *));
EXTERN  RETCODE   dbmnydivide _ANSI_ARGS_((DBPROCESS *,DBMONEY *,DBMONEY *,
				DBMONEY *));
EXTERN  int       dbmnycmp _ANSI_ARGS_((DBPROCESS *,DBMONEY *,DBMONEY *));

#endif
