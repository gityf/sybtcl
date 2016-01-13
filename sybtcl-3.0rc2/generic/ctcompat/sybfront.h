/*
 * sybfront.h
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

/*
 * sybtcl-3.0: fix typdefs for DBFLT8 and DBREAL -thanks to Philip Quaife
 *             changes to dbproc structure, use bind areas
 *
 */


#ifndef SYBFRONTH
#define SYBFRONTH

#define CTCOMPATLIB		/* let sybtcl know what it's compiled with */

#define DBVERSION_100  2

#include <ctpublic.h>

/* typedef sybfront.h needed by sybtcl */

/* simple types */

typedef unsigned char  BYTE;

/* declare a 32 bit int, long for most, int for dec alpha and possibly others */
#if defined(__alpha)
typedef int            RETCODE;
typedef int            DBINT;
typedef int            DBBOOL;
#else
typedef long           RETCODE;
typedef long           DBINT;
typedef long           DBBOOL;
#endif

typedef unsigned char  DBTINYINT;
typedef short          DBSMALLINT;
typedef unsigned short DBUSMALLINT;
typedef char           DBCHAR;
typedef unsigned char  DBBINARY;
typedef unsigned char  DBBIT;
typedef unsigned char  DBBYTE;
typedef double         DBFLT8;
typedef float          DBREAL;

/* structs */

typedef struct {
  DBINT	dateyear, datemonth,datedmonth, datedyear, datedweek, datehour, 
	dateminute, datesecond, datemsecond, datetzone;
} DBDATEREC;

typedef struct {
  DBINT	dtdays, dttime;
} DBDATETIME;

typedef struct {
  unsigned short	days, minutes;
} DBDATETIME4;

typedef struct {
  DBINT	mnyhigh, mnylow;
} DBMONEY;

typedef struct {
  DBINT mny4;
} DBMONEY4;

typedef struct {
  BYTE precision, scale, array[33];
} DBNUMERIC;

typedef struct {
  BYTE precision, scale, array[33];
} DBDECIMAL;

typedef struct {
  DBSMALLINT len;
  DBCHAR     str[256];
} DBVARCHAR;

typedef struct {
  DBSMALLINT len;
  DBBYTE     array[256];
} DBVARBIN;


/* make ct-lib structs void if in dblib code */
#ifndef __CSTYPES_H__
#define CS_CONNECTION void
#define CS_COMMAND    void
#define CS_LOCALE     void
#define CS_DATAFMT    void
#endif


typedef struct {
    CS_CONNECTION *connection;	/* ct lib connection ptr */
    CS_COMMAND	  *command;	/* ct lib command ptr    */
    int   last_col;		/* last column number returned */
    BYTE  *buf[256];		/* pointers to column buffer data */
    CS_INT  buflen[256];	/* lengths of data fetched into buf */
    CS_INT  bufalloc[256];	/* current length of alloced column buffer */
    CS_SMALLINT isnull[256];	/* is null indicators (-1) fetched into buf */
    int   bind_done;		/* if bind was performed */
    int   async_mode;		/* in connection in async mode */
    int   last_result;		/* what was last ct_result */
    int	  has_rows;		/* if rows are ready to be fetched */
    int	  has_status;		/* if stored procedure return status ready */
    int	  ret_status;		/* stored procedure return status  */
    int	  first_fetch;		/* if the first fetch of rows has been done */
    int	  fetch_result;		/* result of first fetch */
    long  textsize;		/* total size of text/image to read/write */
    long  textsent;		/* total text/image sent so far */
    long  maxtext;		/* maxtext set via dbsetopt */
    int   ignore; 		/* in dbwritetext/moretext-ignore db() calls */ 
    int   num_cols;		/* number of columns in results */
    int   compute_colid[256];	/* compute row: column id of name */
    int   compute_altop[256];	/* compute row: operation code */
    CS_DATAFMT *fmtlist;	/* prefetched column info */
    CS_DATAFMT *fmtprev;	/* previous column info, when compute results */
} DBPROCESS;


typedef struct {
  char user[256], password[256], application[256];
  CS_LOCALE     *locale;      /* locale for charset info */
} LOGINREC;


/* other constants */

#define TRUE        1
#define FALSE       0
#define FAIL        0
#define SUCCEED     1
#define INT_EXIT     0
#define INT_CONTINUE 1
#define INT_CANCEL   2
#define INT_TIMEOUT  3

#define DBTXTSLEN       8
#define DBTEXTSIZE      17

#if !VMS
#define DBTXPLEN        ((DBTINYINT)16)
#else
#define DBTXPLEN        ((unsigned char)16)
#endif

/* sybase internal types */

#define SYBVOID         (BYTE)0x1F
#define SYBBINARY       (BYTE)0x2D
#define SYBBIT          (BYTE)0x32
#define SYBCHAR         (BYTE)0x2F
#define SYBDATETIME4    (BYTE)0x3A
#define SYBDATETIME     (BYTE)0x3D
#define SYBDATETIMN     (BYTE)0x6F
#define SYBDECIMAL      (BYTE)0x6A
#define SYBFLT8         (BYTE)0x3E
#define SYBFLTN         (BYTE)0x6D
#define SYBREAL         (BYTE)0x3B
#define SYBIMAGE        (BYTE)0x22
#define SYBINT1         (BYTE)0x30
#define SYBINT2         (BYTE)0x34
#define SYBINT4         (BYTE)0x38
#define SYBINTN         (BYTE)0x26
#define SYBMONEY4       (BYTE)0x7A
#define SYBMONEY        (BYTE)0x3C
#define SYBMONEYN       (BYTE)0x6E
#define SYBNUMERIC      (BYTE)0x6C
#define SYBTEXT         (BYTE)0x23
#define SYBVARBINARY    (BYTE)0x25
#define SYBVARCHAR      (BYTE)0x27
#define SYBSENSITIVITY  (BYTE)0x67
#define SYBBOUNDARY 	(BYTE)0x68
#define SYBAOPCNT   	(BYTE)0x4b
#define SYBAOPSUM   	(BYTE)0x4d
#define SYBAOPAVG   	(BYTE)0x4f
#define SYBAOPMIN   	(BYTE)0x51
#define SYBAOPMAX   	(BYTE)0x52


 
/* result codes  and poll results */

#define MORE_ROWS       -1
#define NO_MORE_ROWS    -2
#define REG_ROW          MORE_ROWS
#define BUF_FULL        -3
#define NO_MORE_PARAMS  -4
#define NO_MORE_RESULTS  2
#define DBRESULT         1
#define DBTIMEOUT        3

#endif

