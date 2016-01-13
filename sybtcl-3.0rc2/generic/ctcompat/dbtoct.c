/*
 * dbtoct.c
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
 * version 2.5
 *----------------------------------------------------------------------------
 * version 3.0 - use dbexit() from Mephis Liang <liang@ttinet.com.tw>
 *             - add dbmny routines, add, sub, mult, div, cmp
 *             - add DBSETLCHARSET support
 *             - remove unused command2 from dblogin struct
 *             - add DBIORDESC()
 *             - fix problem with compute rows, now buffers type, name, len
 *             - rework a great deal of dbresults/dbnextrow to use binds
 *             - fix dbmonthname() to use correct month
 *             - fix dbconvert() to return number of bytes converted
 *             - fix dbname() to convert to null terminated string
 */

#ifdef NO_STRING_H
#include <strings.h>
#else
#include <string.h>
#endif


#ifdef TESTIT
extern void *malloc(int);
extern void free(void *);
#define ckalloc malloc
#define ckfree free
#else
/* use tcl's headers for ckalloc/ckfree */
#include "tcl.h"
#endif


/* include our version of these headers */
/* sybfront.h include all needed ct headers */
#include "sybfront.h"
#include "sybdb.h"
#include "syberror.h"

 
#ifdef NO_BCOPY
#define bcopy(from, to, length)    memmove((to), (from), (length))
#define bzero(from, length)        memset((from), (0), (length))
#define bcmp(s1, s2, length)	   memcmp((s1), (s2), (length))
#endif


/* globals to this file only */

static CS_CONTEXT   *context = NULL;	/* context for all ct/cs calls   */
static CS_IODESC    iodesc;		/* for writetext/readtext/txtptr */
static int          last_io_col = 0;    /*   "    "         "       "    */
static int (*dbtoct_err)()   = NULL;	/* when installed by dberrhandle */
static int (*dbtoct_msg)()   = NULL;	/* when installed by dbmsghandle */


/* struct and linked list to keep track of open dbproc structs */

typedef struct ProcList {
  struct ProcList *next;
  DBPROCESS       *dbp;
} ProcList;

static ProcList *start_proc_list = NULL;


/* helper functions */

/* ----------------------------------------------------------------------------
 * drain_results
 *
 */

static void
drain_results (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE  retcode;
    CS_INT      result = CS_SUCCEED;

    while (retcode == CS_SUCCEED) {
	retcode = ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
	retcode = ct_results(dbproc->command, &result);
	if (retcode == CS_SUCCEED && result == CS_END_RESULTS) {
	    dbproc->last_result = CS_END_RESULTS;
	    return;
	}
    } 
    dbproc->last_result = 0;
}

/* ----------------------------------------------------------------------------
 * finddbproc - find dbproc that has a connection
 *
 */

static DBPROCESS *
finddbproc(a_connection)
    CS_CONNECTION *a_connection;
{
    ProcList *proc_list = NULL;

    for (proc_list = start_proc_list; proc_list != NULL; 
         proc_list = proc_list->next) {

        if (proc_list->dbp->connection == a_connection) {
            break;
        }
    }
    return (proc_list != NULL ? proc_list->dbp : NULL);
}

/* ----------------------------------------------------------------------------
 * freefmtprev - free fmtprev
 *
 */

static void
freefmtprev(dbproc)
      DBPROCESS *dbproc;
{
    if (dbproc->fmtprev != NULL) {
        ckfree(dbproc->fmtprev);
        dbproc->fmtprev = NULL;
    }
}


/* ----------------------------------------------------------------------------
 * freefmtlist - free fmtlist
 *
 */

static void
freefmtlist(dbproc)
    DBPROCESS *dbproc;
{
    if (dbproc->fmtlist != NULL) {
	ckfree(dbproc->fmtlist);
	dbproc->fmtlist = NULL;
    }
}


/* ----------------------------------------------------------------------------
 * cttodbtype - convert ct types to db types 
 *
 */

static int
cttodbtype(cttype)
    int cttype;
{
    switch (cttype) {
        case CS_ILLEGAL_TYPE    : return SYBVOID;
        case CS_BINARY_TYPE     : return SYBBINARY;
        case CS_BIT_TYPE        : return SYBBIT;
        case CS_CHAR_TYPE       : return SYBCHAR;
        case CS_DATETIME4_TYPE  : return SYBDATETIME4;
        case CS_DATETIME_TYPE   : return SYBDATETIME;
        case CS_DECIMAL_TYPE    : return SYBDECIMAL;
        case CS_FLOAT_TYPE      : return SYBFLT8;
        case CS_REAL_TYPE       : return SYBREAL;
        case CS_IMAGE_TYPE      : return SYBIMAGE;
        case CS_TINYINT_TYPE    : return SYBINT1;
        case CS_SMALLINT_TYPE   : return SYBINT2;
        case CS_INT_TYPE        : return SYBINT4;
        case CS_MONEY4_TYPE     : return SYBMONEY4;
        case CS_MONEY_TYPE      : return SYBMONEY;
        case CS_NUMERIC_TYPE    : return SYBNUMERIC;
        case CS_TEXT_TYPE       : return SYBTEXT;
        case CS_VARBINARY_TYPE  : return SYBVARBINARY;
        case CS_VARCHAR_TYPE    : return SYBVARCHAR;
        case CS_SENSITIVITY_TYPE: return SYBSENSITIVITY;
        case CS_BOUNDARY_TYPE   : return SYBBOUNDARY;
        case CS_OP_AVG          : return SYBAOPAVG;
        case CS_OP_SUM          : return SYBAOPSUM;
        case CS_OP_COUNT        : return SYBAOPCNT;
        case CS_OP_MIN          : return SYBAOPMIN;
        case CS_OP_MAX          : return SYBAOPMAX;
        default                 : return SYBVOID;
    }
}


/* ----------------------------------------------------------------------------
 * dbtocttype - convert db types to ct types 
 *
 */

static int
dbtocttype(dbtype)
    int dbtype;
{
    switch (dbtype) {
        case SYBVOID         : return CS_ILLEGAL_TYPE;
        case SYBBINARY       : return CS_BINARY_TYPE;
        case SYBBIT          : return CS_BIT_TYPE;
        case SYBCHAR         : return CS_CHAR_TYPE;
        case SYBDATETIME4    : return CS_DATETIME4_TYPE;
        case SYBDATETIME     : return CS_DATETIME_TYPE;
        case SYBDATETIMN     : return CS_DATETIME_TYPE;
        case SYBDECIMAL      : return CS_DECIMAL_TYPE;
        case SYBFLT8         : return CS_FLOAT_TYPE;
        case SYBFLTN         : return CS_FLOAT_TYPE;
        case SYBREAL         : return CS_REAL_TYPE;
        case SYBIMAGE        : return CS_IMAGE_TYPE;
        case SYBINT1         : return CS_TINYINT_TYPE;
        case SYBINT2         : return CS_SMALLINT_TYPE;
        case SYBINT4         : return CS_INT_TYPE;
        case SYBINTN         : return CS_INT_TYPE;
        case SYBMONEY4       : return CS_MONEY4_TYPE;
        case SYBMONEY        : return CS_MONEY_TYPE;
        case SYBMONEYN       : return CS_MONEY_TYPE;
        case SYBNUMERIC      : return CS_NUMERIC_TYPE;
        case SYBTEXT         : return CS_TEXT_TYPE;
        case SYBVARBINARY    : return CS_VARBINARY_TYPE;
        case SYBVARCHAR      : return CS_VARCHAR_TYPE;
        case SYBSENSITIVITY  : return CS_SENSITIVITY_TYPE;
        case SYBBOUNDARY     : return CS_BOUNDARY_TYPE;
        default              : return CS_ILLEGAL_TYPE;
    }
}


/* ----------------------------------------------------------------------------
 * getcolumninfo - get column info for ct results
 *
 */

static int
getcolumninfo(dbproc)
    DBPROCESS *dbproc;
{
    CS_INT    num_cols = 0;
    CS_DATAFMT        *fmt;
    int               i;
    int compcol = 0;
    int compop  = 0;
    CS_RETCODE  retcode;
 
    freefmtlist(dbproc);

    if (dbproc->last_result == CS_END_RESULTS) {
      freefmtprev(dbproc);
        return 0;
    }

    ct_res_info(dbproc->command, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
    dbproc->num_cols = num_cols;

    if (num_cols > 0) {
	fmt = (CS_DATAFMT *)ckalloc(sizeof (CS_DATAFMT) * num_cols);
	dbproc->fmtlist = fmt;
	for (i = 1; i <= num_cols; i++) {
	    retcode = ct_describe(dbproc->command, i, fmt);
	    if (dbproc->last_result == CS_COMPUTE_RESULT) {
		/* for compute rows, get extra info */
		retcode = ct_compute_info(dbproc->command,
		       CS_COMP_COLID, i, &compcol, CS_UNUSED, NULL);
		if (retcode == CS_SUCCEED) {
		    dbproc->compute_colid[i-1] = compcol;
		} else {
		    dbproc->compute_colid[i-1] = 0;
		}

                retcode = ct_compute_info(dbproc->command,
                      CS_COMP_OP,   i, &compop, CS_UNUSED, NULL);

                if (retcode == CS_SUCCEED) {
                    dbproc->compute_altop[i-1] = cttodbtype(compop);
                } else {
                    dbproc->compute_altop[i-1] = 0;
                }
            } else {
                dbproc->compute_colid[i-1] = 0;
                dbproc->compute_altop[i-1] = 0;
            }
            fmt++;
        }
    }

    return num_cols;
}


/* ----------------------------------------------------------------------------
 * bind_cols - bind columns into data areas
 *
 */
static void
bind_cols (dbproc)
    DBPROCESS *dbproc;
{
    int i;
    CS_RETCODE  retcode;
    CS_DATAFMT *bindfmt;
    CS_DATAFMT *fmt;

    bindfmt = (CS_DATAFMT *) ckalloc(sizeof(CS_DATAFMT));
    if (dbproc->fmtlist == NULL) {
	getcolumninfo(dbproc);
    }
    fmt = dbproc->fmtlist;
    for (i = 1; i <= dbproc->num_cols; i++) {
	if (dbproc->fmtlist[i-1].datatype == CS_TEXT_TYPE ||
	    dbproc->fmtlist[i-1].datatype == CS_IMAGE_TYPE) {
	    if (dbproc->fmtlist[i-1].maxlength > dbproc->maxtext) {
		dbproc->fmtlist[i-1].maxlength = dbproc->maxtext;
	    }
	}
	if (dbproc->bufalloc[i] < dbproc->fmtlist[i-1].maxlength) {
	    ckfree(dbproc->buf[i-1]);
	    dbproc->buf[i-1] = (BYTE*) ckalloc(dbproc->fmtlist[i-1].maxlength);
	    dbproc->bufalloc[i] = dbproc->fmtlist[i-1].maxlength;
	}
	bcopy(fmt, bindfmt, sizeof(CS_DATAFMT));
	switch (bindfmt->datatype) {
	    case CS_CHAR_TYPE:
	    case CS_VARCHAR_TYPE:
	    case CS_TEXT_TYPE:
	        bindfmt->format    = CS_FMT_NULLTERM;
	        bindfmt->format    = CS_FMT_UNUSED;
		break;
	    case CS_BINARY_TYPE:
	    case CS_VARBINARY_TYPE:
	    case CS_IMAGE_TYPE:
	        bindfmt->format    = CS_FMT_UNUSED;
		break;
	    default:
	        bindfmt->format    = CS_FMT_UNUSED;
		break;
	}
	bindfmt->scale     = CS_SRC_VALUE;
	bindfmt->precision = CS_SRC_VALUE;
	bindfmt->count     = 1;
	bindfmt->locale    = NULL;
	bindfmt->maxlength = dbproc->bufalloc[i];
	retcode = ct_bind(dbproc->command, i, bindfmt, 
		dbproc->buf[i-1], &dbproc->buflen[i-1], 
		&dbproc->isnull[i-1]);
	fmt++;
    }
    dbproc->bind_done = 1;
    ckfree(bindfmt);
}



/* ----------------------------------------------------------------------------
 * __dbtoct_err_handler - callback for ct messages
 *
 */

CS_RETCODE
__dbtoct_err_handler (context, connection, message)
    CS_CONTEXT    *context;
    CS_CONNECTION *connection;
    CS_CLIENTMSG  *message;
{
    int retcode;
    DBPROCESS *dbproc = NULL;

    /* dbproc = finddbproc(connection); */
    if (connection != NULL) {
        ct_con_props(connection, CS_GET, CS_USERDATA, &dbproc, 
            sizeof(DBPROCESS *), NULL);
    }

    if (dbtoct_err != NULL) {
    
        retcode = (*dbtoct_err) (dbproc,  CS_SEVERITY(message->msgnumber),
            CS_NUMBER(message->msgnumber), 
            message->osnumber,
            message->msgstring, 
            (message->osstringlen > 0 ? message->osstring : "") );	
    }
    switch (retcode) {
        case INT_EXIT    : return CS_FAIL;
        case INT_CONTINUE: return CS_SUCCEED;
        case INT_CANCEL  : return CS_FAIL;
        case INT_TIMEOUT : return CS_SUCCEED;
        default     	 : return CS_SUCCEED;
    }
}


/* ----------------------------------------------------------------------------
 * dbtoct_msg_handler - callback for ct messages
 *
 */

CS_RETCODE
__dbtoct_msg_handler (context, connection, message)
    CS_CONTEXT    *context;
    CS_CONNECTION *connection;
    CS_SERVERMSG  *message;
{
    DBPROCESS *dbproc = NULL;

    /*dbproc = finddbproc(connection);*/

    if (connection != NULL) {
        ct_con_props(connection, CS_GET, CS_USERDATA, &dbproc, 
            sizeof(DBPROCESS *), NULL);
    }

    if (dbproc != NULL && dbtoct_msg != NULL) {
    
        (*dbtoct_msg) (dbproc, message->msgnumber, message->state, 
                message->severity, 
                message->text, 
                (message->svrnlen > 0 ? message->svrname : ""),
             	(message->proclen > 0 ? message->proc    : ""), 
                message->line);
    }
    return CS_SUCCEED;
}





/* db to ct lib emulation functions */




/* ----------------------------------------------------------------------------
 * dbconvert  
 *
 */

DBINT
dbconvert (dbproc, srctype, src, srclen, desttype, dest, destlen)
    DBPROCESS *dbproc; 
    int       srctype;
    BYTE *    src;
    DBINT     srclen;
    int       desttype;
    VOID *    dest;
    DBINT     destlen;
{
    CS_RETCODE retcode;
    CS_DATAFMT in;
    CS_DATAFMT out;
    CS_INT     resultlen;


    switch (desttype) {
	case SYBCHAR: 
	case SYBTEXT: 
	    if (destlen < 0) {
	        out.format = CS_FMT_NULLTERM;
	        destlen = 2147483647;
	    } else {
		out.format = CS_FMT_PADBLANK;
	    }
	    break;

	default: 
	    if (destlen < 0) {
		destlen = 2147483647;
	    }
	    out.format = CS_FMT_UNUSED;
	    break;
    }
    out.datatype  = dbtocttype(desttype);
    out.maxlength = destlen;
    out.locale    = NULL;
    out.scale     = 17;
    out.precision = 37;


    in.datatype   = dbtocttype(srctype);
    in.locale     = NULL;
    in.maxlength  = srclen;
    if (srclen >= 0) {
        in.maxlength = srclen;
    } else {
	switch (srctype) {
	    case SYBBIT          : srclen = sizeof(CS_BIT);		break;
	    case SYBDATETIME4    : srclen = sizeof(CS_DATETIME4);	break;
	    case SYBDATETIME     : srclen = sizeof(CS_DATETIME);	break;
	    case SYBDATETIMN     : srclen = sizeof(CS_DATETIME);	break;
	    case SYBFLT8         : srclen = sizeof(CS_FLOAT);		break;
	    case SYBFLTN         : srclen = sizeof(CS_FLOAT);		break;
	    case SYBREAL         : srclen = sizeof(CS_REAL);		break;
	    case SYBINT1         : srclen = sizeof(CS_TINYINT);		break;
	    case SYBINT2         : srclen = sizeof(CS_SMALLINT);	break;
	    case SYBINT4         : srclen = sizeof(CS_INT);		break;
	    case SYBINTN         : srclen = sizeof(CS_INT);		break;
	    case SYBMONEY4       : srclen = sizeof(CS_MONEY4);		break;
	    case SYBMONEY        : srclen = sizeof(CS_MONEY);		break;
	    case SYBMONEYN       : srclen = sizeof(CS_MONEY);		break;
	    case SYBDECIMAL      : srclen = sizeof(CS_DECIMAL);		break;
	    case SYBNUMERIC      : srclen = sizeof(CS_NUMERIC);		break;
	    case SYBVARBINARY    : srclen = sizeof(CS_VARBINARY);	break;
	    case SYBVARCHAR      : srclen = sizeof(CS_VARCHAR);		break;

	    case SYBBINARY       :
	    case SYBCHAR         :
	    case SYBTEXT         :
	    case SYBIMAGE        : 
	    default              : srclen = strlen(src);
	}
	in.maxlength = srclen;
    }

    retcode = cs_convert(context, &in, src, &out, dest, &resultlen);
    
    if (retcode == CS_SUCCEED) {
        return resultlen;
    } else {
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbdatecrack  
 *
 */

RETCODE
dbdatecrack (dbproc, dateinfo, datetime)
    DBPROCESS *dbproc;
    DBDATEREC *dateinfo;
    DBDATETIME *datetime;
{
    CS_RETCODE retcode;

    /* CS_DATETIME == DBDATETIME  && DBDATEREC == CS_DATEREC */
    
    retcode = cs_dt_crack(context, CS_DATETIME_TYPE, (CS_DATETIME *) datetime, 
        (CS_DATEREC *) dateinfo);

    if (retcode == CS_SUCCEED) {
        return SUCCEED;
    } else {
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbmonthname  
 *
 */

char *
dbmonthname (dbproc, language, monthnum, shortform)
    DBPROCESS *dbproc;
    char      *language;
    int       monthnum;
    DBBOOL    shortform;
{
    static char buf[32];
    CS_INT len;

    monthnum--;   /* ct-lib version is one less */
    cs_dt_info(context, CS_GET, NULL, (shortform ? CS_SHORTMONTH : CS_MONTH),
            monthnum, buf, 32, &len);
    return buf;
}


/* ----------------------------------------------------------------------------
 * dbexit  
 *
 */

VOID
dbexit ()
{
    ProcList *proc_ptr, *next_ptr;
    
    /* free all ProcLists */
    for( proc_ptr = start_proc_list; proc_ptr != NULL; proc_ptr = next_ptr ) {
        next_ptr = proc_ptr->next;
        ct_close(proc_ptr->dbp->connection, CS_FORCE_CLOSE);
        ct_cmd_drop(proc_ptr->dbp->command);
        ct_con_drop(proc_ptr->dbp->connection);
        ckfree(proc_ptr->dbp->buf);
        freefmtlist(proc_ptr->dbp);
        freefmtprev(proc_ptr->dbp);
        ckfree(proc_ptr->dbp);
        ckfree(proc_ptr);
    }   
    start_proc_list = NULL;
    ct_exit(context,CS_UNUSED);
    cs_ctx_drop(context);
 
    dbtoct_err = NULL;
    dbtoct_msg = NULL;

}


/* ----------------------------------------------------------------------------
 * dbinit  
 *
 */

RETCODE
dbinit ()
{
    CS_INT retcode;

    retcode = cs_ctx_alloc(CS_VERSION_100, &context);
    if (retcode == CS_SUCCEED) {
        retcode = ct_init(context, CS_VERSION_100);
        if (retcode != CS_SUCCEED) {
            cs_ctx_drop(context);
            context = NULL;
            return FAIL;
        }
        return SUCCEED;
    }
    return FAIL;
}


/* ----------------------------------------------------------------------------
 * dbsetversion  
 *
 */

RETCODE
dbsetversion (version)
    DBINT version;
{
    return SUCCEED;
}


/* ----------------------------------------------------------------------------
 * dbsetmaxprocs  
 *
 */

RETCODE
dbsetmaxprocs (max)
    int max;
{
    CS_RETCODE 	retcode;
    CS_INT	maxprocs = max;

    retcode = ct_config(context, CS_SET, CS_MAX_CONNECT, (CS_VOID *) &maxprocs, 
            CS_UNUSED, NULL);

    if (retcode == CS_SUCCEED) {
        return SUCCEED;
    } else {
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dberrhandle  
 *
 */

void
dberrhandle (handler)
    int (*handler)();
{
    dbtoct_err = handler;
    ct_callback(context, NULL, CS_SET, CS_CLIENTMSG_CB, __dbtoct_err_handler);
}


/* ----------------------------------------------------------------------------
 * dbmsghandle  
 *
 */

void
dbmsghandle (handler)
    int (*handler)();
{
    dbtoct_msg = handler;
    ct_callback(context, NULL, CS_SET, CS_SERVERMSG_CB, __dbtoct_msg_handler);
}


/* ----------------------------------------------------------------------------
 * dblogin  
 *
 */

LOGINREC *
dblogin()
{
    LOGINREC *loginrec;

    loginrec = (LOGINREC *) ckalloc(sizeof(LOGINREC));
    *(loginrec->user)        = '\0';
    *(loginrec->password)    = '\0';
    *(loginrec->application) = '\0';
    (loginrec->locale)       = NULL;

    return loginrec;
}


/* ----------------------------------------------------------------------------
 * DBSETLUSER  
 *
 */

VOID
DBSETLUSER (loginrec, username)
    LOGINREC *loginrec;
    char     *username;
{
    if (strlen(username) > 256) {
        username[256] = '\0';   /* turkey! */
    }
    strcpy(loginrec->user, username);
}


/* ----------------------------------------------------------------------------
 * DBSETLPWD  
 *
 */

VOID
DBSETLPWD (loginrec, password)
    LOGINREC *loginrec;
    char     *password;
{
    if (strlen(password) > 256) {
        password[256] = '\0';   /* turkey! */
    }
    strcpy(loginrec->password, password);
}


/* ----------------------------------------------------------------------------
 * DBSETLAPP  
 *
 */

VOID
DBSETLAPP (loginrec, appname)
    LOGINREC *loginrec;
    char     *appname;
{
    if (appname != NULL) {
        if (strlen(appname) > 256) {
            appname[256] = '\0';   /* turkey! */
        }
	if (strlen(appname) > 0) {
            strcpy(loginrec->application, appname);
	}
    }
}


/* ----------------------------------------------------------------------------
 * DBSETLCHARSET
 *
 */

VOID
DBSETLCHARSET (loginrec, charset)
    LOGINREC *loginrec;
    char     *charset;
{
    CS_RETCODE rc;

    if (loginrec->locale != NULL) {
	cs_loc_drop(context,loginrec->locale);
	loginrec->locale = NULL;
    }
    if (charset != NULL) {
        if (strlen(charset) > 0) {
            cs_loc_alloc(context, &(loginrec->locale));
            rc = cs_locale(context, CS_SET, loginrec->locale, CS_SYB_CHARSET, 
	          charset, CS_NULLTERM, NULL);
            if (rc != CS_SUCCEED) {
	        cs_loc_drop(context,loginrec->locale);
	        loginrec->locale = NULL;
            }
        }
    }
}


/* ----------------------------------------------------------------------------
 * dbsetifile  
 *
 */

VOID
dbsetifile (filename)
    char *filename;
{
    if (filename != NULL) {
        if (strlen(filename) > 0) {
            ct_config(context, CS_SET, CS_IFILE, filename, CS_NULLTERM, NULL);
        }
    }
}


/* ----------------------------------------------------------------------------
 * dbloginfree  
 *
 */

VOID
dbloginfree (loginrec)
    LOGINREC *loginrec;
{
    if (loginrec->locale != NULL) {
	cs_loc_drop(context,loginrec->locale);
    }
    ckfree(loginrec);
}


/* ----------------------------------------------------------------------------
 * dbopen  
 *
 */

DBPROCESS *
dbopen (loginrec, server)
    LOGINREC *loginrec;
    char     *server;
{
    DBPROCESS *dbproc;
    ProcList  *proc_ptr;
    CS_RETCODE retcode;
    CS_BOOL pollprop = CS_FALSE;
    int size = 32768;
    int i;

    /* alloc new dbproc */
    dbproc = (DBPROCESS *) ckalloc(sizeof(DBPROCESS));

    /* alloc connection and set connection props */
    ct_con_alloc(context, &(dbproc->connection));
    ct_con_props(dbproc->connection, CS_SET, CS_USERNAME, loginrec->user,
            CS_NULLTERM, NULL);
    ct_con_props(dbproc->connection, CS_SET, CS_PASSWORD, loginrec->password,
            CS_NULLTERM, NULL);
    ct_con_props(dbproc->connection, CS_SET, CS_APPNAME, loginrec->application,
            CS_NULLTERM, NULL);
    if (loginrec->locale != NULL) {
        ct_con_props(dbproc->connection, CS_SET, CS_LOC_PROP, 
		loginrec->locale, CS_UNUSED, NULL);
    }
    
    retcode = ct_connect(dbproc->connection, server, 
                    	(server == NULL) ? 0 : CS_NULLTERM);

    if (retcode != CS_SUCCEED) {
        ct_con_drop(dbproc->connection);	
        ckfree(dbproc);
        return NULL;
    }

    /* alloc command */
    ct_cmd_alloc(dbproc->connection, &(dbproc->command));

    /* alloc column data buffer */
    dbproc->bind_done = 0;
    for (i=0; i<256; i++) {
        dbproc->bufalloc[i] = 300;
        dbproc->buflen[i] = 0;
        dbproc->isnull[i] = 0;
	dbproc->buf[i] = (BYTE*)ckalloc(dbproc->bufalloc[i]);
    }
    dbproc->async_mode = 0;
    dbproc->last_result = 0;
    dbproc->has_rows = 0;
    dbproc->has_status = 0;
    dbproc->first_fetch = 0;
    dbproc->fetch_result = 0;
    dbproc->ret_status = 0;
    dbproc->textsize = 0;
    dbproc->textsent = 0;
    dbproc->textsent = 32768;
    dbproc->ignore = 0;
    dbproc->num_cols = 0;
    dbproc->fmtlist = NULL;
    dbproc->fmtprev = NULL;
    dbproc->maxtext = 32768;

    /* keep track of dbprocs for err & msg handlers */
    proc_ptr = (ProcList *) ckalloc(sizeof(ProcList));
    proc_ptr->next  = start_proc_list;
    proc_ptr->dbp   = dbproc;
    start_proc_list = proc_ptr;

    /* use connection userdata to store dbproc addr */
    ct_con_props(dbproc->connection, CS_SET, CS_USERDATA, &dbproc, 
            sizeof(DBPROCESS *), NULL);

    /* make sure connection can poll for dbsqlsend/dbpoll */
    ct_con_props(dbproc->connection, CS_SET, CS_DISABLE_POLL,
			  &pollprop, CS_UNUSED, NULL);

    return dbproc;
}



/* ----------------------------------------------------------------------------
 * DBIORDESC
 *
 */

int
DBIORDESC (dbproc)
    DBPROCESS *dbproc;
{
    int sock;
    ct_con_props(dbproc->connection, CS_GET, CS_ENDPOINT,
			  &sock, CS_UNUSED, NULL);
    return sock;
}




/* ----------------------------------------------------------------------------
 * dbuse  
 *
 */

RETCODE
dbuse (dbproc, dbname)
    DBPROCESS *dbproc;
    char      *dbname;
{
    char buf[1024];
    int  retcode;
    CS_CLIENTMSG message;

    strcpy(buf, "use ");
    if (strlen(dbname) > 1000) {
        dbname[1000] = '\0';  /* probably a turkey! */
    }
    strcat(buf, dbname);

    dbcancel(dbproc);
    dbcmd(dbproc, buf);
    retcode = dbsqlexec(dbproc);
    if (retcode == SUCCEED) {
	drain_results(dbproc);
        return SUCCEED;
    } else {
	drain_results(dbproc);
        /* dblib dbuse() signals an error, ctlib doesn't since it is a */
        /* server msg.  on failure, call the err handler */

        message.msgnumber   =  /*severity*/ (5 <<8) || /*errno*/ (5 &0xff);
	message.osnumber    =  -1;   /* oserr */
	strcpy(message.msgstring, 
	       "General SQL Server error: Check messages from the SQL Server.");
	message.osstringlen = 0;
	message.osstring[0] = '\0';
        __dbtoct_err_handler(context, dbproc->connection, &message);

        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbname  
 *
 */

char *
dbname (dbproc)
    DBPROCESS *dbproc;
{
    int retcode;
    static char buf[1024];

    buf[0] = '\0';
    dbcancel(dbproc);
    dbcmd(dbproc, "select db_name()");
    dbsqlexec(dbproc);
    retcode = dbresults(dbproc);
    if (retcode == SUCCEED) {
        dbnextrow(dbproc);
        dbconvert(dbproc, dbcoltype(dbproc,1), dbdata(dbproc,1), 
            dbdatlen(dbproc,1), SYBCHAR, buf, -1);
	drain_results(dbproc);
        return buf;
    } else {
        dbcancel(dbproc);
        return "";
    }
}


/* ----------------------------------------------------------------------------
 * dbcancel  
 *
 */

RETCODE
dbcancel (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE retcode;

/*
    if (dbproc->last_result == CS_END_RESULTS) {
        return SUCCEED;
    }
*/
    drain_results(dbproc);
    ct_cancel(dbproc->connection, NULL, CS_CANCEL_ALL);
    dbproc->last_result = CS_END_RESULTS;
    if (retcode == CS_SUCCEED) {
        return SUCCEED;
    } else {
        return FAIL;
    }
}



/* ----------------------------------------------------------------------------
 * dbsetopt  
 *
 */

RETCODE
dbsetopt (dbproc, option, char_parm, int_parm)
    DBPROCESS *dbproc;
    int       option;
    char      *char_parm;
    int       int_parm;
{
    CS_RETCODE retcode;
    CS_INT     size = 0;
    long   	atol(char *);
    char	buf[256];

    switch (option) {
        case DBTEXTSIZE:
            size = atol(char_parm);
            if (size > 0) {
        	retcode = ct_con_props(dbproc->connection, CS_SET, CS_TEXTLIMIT,
            		&size, CS_UNUSED, NULL);
            } else {
        	retcode = CS_FAIL;
            }
            if (retcode == CS_SUCCEED) {
        	/* now set it on the server */
        	sprintf(buf,"set textsize %ld\n",size);
        	dbcmd(dbproc, buf);
		dbproc->maxtext = size;
        	return SUCCEED;
            } else {
        	return FAIL;
            }

        default:
            return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbsqlexec  
 *
 */

RETCODE
dbsqlexec (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE 	retcode;
    CS_INT	result;
    CS_INT	sync_io = CS_SYNC_IO;

    last_io_col = 0;

    dbproc->last_result = 0;
    dbproc->bind_done = 0;
    dbproc->has_rows = 0;
    dbproc->has_status = 0;
    dbproc->first_fetch = 0;
    dbproc->fetch_result = 0;
    dbproc->textsize = 0;
    dbproc->textsent = 0;
    dbproc->ignore = 0;
    dbproc->ret_status = 0;
    dbproc->num_cols = 0;
    freefmtlist(dbproc);
    freefmtprev(dbproc);
    if (dbproc->async_mode) {
        dbproc->async_mode = 0;
        ct_con_props(dbproc->connection, CS_SET, CS_NETIO, &sync_io,
                    CS_UNUSED, NULL);
    }
    retcode = ct_send(dbproc->command);
    if (retcode == CS_SUCCEED) {
        /* send is always async, so get first results and save for later */
        retcode = ct_results(dbproc->command, &result);
        dbproc->last_result = result;
        if (retcode == CS_SUCCEED) {
            switch (result) {
        	case CS_CMD_FAIL:
            	    return FAIL;
        	case CS_CMD_SUCCEED:
        	case CS_CMD_DONE:
        	case CS_ROW_RESULT:
        	case CS_COMPUTE_RESULT:
        	case CS_STATUS_RESULT:
        	case CS_PARAM_RESULT:
        	default:
            	    return SUCCEED;
            }
        } else {
            return FAIL;
        }
    } else {
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbcmd
 *
 */

RETCODE
dbcmd (dbproc,cmdstring)
    DBPROCESS *dbproc;
    char      *cmdstring;
{
    CS_RETCODE retcode;

    retcode = ct_command(dbproc->command, CS_LANG_CMD, cmdstring, 
        CS_NULLTERM, CS_UNUSED);
    if (retcode == CS_SUCCEED) {
        return SUCCEED;
    } else {
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbsqlsend  
 *
 */

RETCODE
dbsqlsend (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE retcode, retcode2;
    CS_INT		async_io = CS_ASYNC_IO;

    last_io_col = 0;

    dbproc->last_result = 0;
    dbproc->bind_done = 0;
    dbproc->has_rows = 0;
    dbproc->has_status = 0;
    dbproc->first_fetch = 0;
    dbproc->fetch_result = 0;
    dbproc->textsize = 0;
    dbproc->textsent = 0;
    dbproc->ignore = 0;
    dbproc->ret_status = 0;
    dbproc->num_cols = 0;
    freefmtlist(dbproc);
    freefmtprev(dbproc);
    if (!dbproc->async_mode) {
        dbproc->async_mode = 1;
        retcode2=ct_con_props(dbproc->connection, CS_SET, CS_NETIO, &async_io, 
                    	CS_UNUSED, NULL);
    }
    retcode = ct_send(dbproc->command);
    if (retcode == CS_SUCCEED || retcode == CS_PENDING) {
        return SUCCEED;
    } else {
        return FAIL;
    }
}



/* ----------------------------------------------------------------------------
 * dbresults  
 *
 */

RETCODE
dbresults (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE retcode;
    CS_INT     result = CS_CMD_FAIL;
    CS_INT     computeid;
    CS_INT     status;
    RETCODE    dbresult = NO_MORE_RESULTS;
    int	       cols;
    int        num_cols;
    CS_DATAFMT fmt;
    char       *t;
    int        result_boundary = 0;
    int        tmp_done;


    /* and ugly hack to let sybwritetext work (nevermind, it is all ugly) */
    if (dbproc->ignore) {
	return NO_MORE_RESULTS;
    }

    if (dbproc->last_result == CS_END_RESULTS) {
        freefmtlist(dbproc);
        freefmtprev(dbproc);
        return NO_MORE_RESULTS;
    }

    last_io_col = 0;

    dbproc->bind_done = 0;
    dbproc->has_rows = 0;
    dbproc->has_status = 0;
    dbproc->first_fetch = 0;
    dbproc->fetch_result = 0;

more_results:

    if (dbproc->last_result == 0) {
        retcode = ct_results(dbproc->command, &result);
        dbproc->last_result = retcode;
    } else {
        result = dbproc->last_result;
        dbproc->last_result = 0;
        retcode = CS_SUCCEED;
    }

    freefmtlist(dbproc);
    if (retcode == CS_END_RESULTS) {
        freefmtlist(dbproc);
        freefmtprev(dbproc);
        dbproc->last_result = CS_END_RESULTS;
        return NO_MORE_RESULTS;
    }

    if (retcode == CS_END_RESULTS) {
	freefmtprev(dbproc);
	dbproc->last_result = CS_END_RESULTS;
	return NO_MORE_ROWS;
    }

    if (retcode == CS_SUCCEED) {
        switch (result) {
            case CS_CMD_FAIL:		
	        dbproc->has_rows = 0;
	        dbproc->first_fetch = 0;
	        dbproc->fetch_result = 0;
	        dbproc->last_result = 0;
	        dbresult = FAIL;
		goto more_results;

            case CS_CMD_SUCCEED:	
	        dbproc->has_rows = 0;
	        dbproc->first_fetch = 0;
	        dbproc->fetch_result = 0;
	        dbproc->last_result = 0;
	        dbresult = SUCCEED;
	        goto more_results;

            case CS_CMD_DONE:	
		dbproc->last_result = 0;
		goto more_results;
		return NO_MORE_RESULTS;

            case CS_ROW_RESULT:		
	        /* suck-o-rama:no DBROWS() in ctlib, gotta fetch the first row*/
                freefmtlist(dbproc);
                freefmtprev(dbproc);
		getcolumninfo(dbproc);
                /* don't fetch if only one column and that column is */
		/* text or image. most likely will read/write text/image */
		if (dbproc->num_cols == 1 && 
		     (dbproc->fmtlist[0].datatype == CS_TEXT_TYPE ||
		      dbproc->fmtlist[0].datatype == CS_IMAGE_TYPE)     ) {
		    dbproc->fetch_result =  
		       ct_fetch(dbproc->command,CS_UNUSED,CS_UNUSED,CS_UNUSED,
			       NULL);
		    dbproc->first_fetch = 1;
		    if (dbproc->fetch_result == CS_SUCCEED) {
			dbproc->has_rows = 1;
		    } else {
			dbproc->has_rows = 0;
		        ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
			dbproc->fetch_result = CS_END_DATA;
		    }
		} else {
		    bind_cols(dbproc);
		    dbproc->fetch_result =  
		       ct_fetch(dbproc->command,CS_UNUSED,CS_UNUSED,CS_UNUSED,
			       NULL);
		    dbproc->first_fetch = 1;
		    if (dbproc->fetch_result == CS_SUCCEED) {
			dbproc->has_rows = 1;
		    } else {
			dbproc->has_rows = 0;
		        ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
			dbproc->fetch_result = CS_END_DATA;
		    }
		}
	        dbresult = SUCCEED;
	        break;

            case CS_COMPUTE_RESULT:	
	        /* shouldn't ever get here, should have a ROW_RESULT first */
		return FAIL;

	    case CS_STATUS_RESULT:	
		result_boundary = 1;
		/* fetch the status row */
		retcode = ct_fetch(dbproc->command,
			CS_UNUSED, CS_UNUSED, CS_UNUSED, NULL);
		dbproc->has_rows = 0;
		dbproc->has_status = 1;
		dbproc->first_fetch = 0;
		dbproc->fetch_result = 0;
		status = 0;
		if (retcode == CS_SUCCEED) {
		    tmp_done = dbproc->bind_done;
		    dbproc->bind_done = 0;
		    bcopy(dbdata(dbproc,1), &status, sizeof(CS_INT));
		    dbproc->bind_done = tmp_done;;
		}
		dbproc->ret_status = status;
		ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
		goto more_results;

	
	    case CS_PARAM_RESULT:
		result_boundary = 1;
		/* get param columns for later dbretxxxx()  */
		dbproc->has_rows = 0;
		dbproc->has_status = 1;
		dbproc->first_fetch = 0;
		dbproc->fetch_result = 0;
		freefmtlist(dbproc);
		freefmtprev(dbproc);
		bind_cols(dbproc);
		/* fetch the param row */
		retcode = ct_fetch(dbproc->command,
			CS_UNUSED, CS_UNUSED, CS_UNUSED, NULL);
		num_cols = 0;
		ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
		freefmtprev(dbproc);
		goto more_results;
		    
            default:
		if (result_boundary) {
	            dbresult = SUCCEED;
		} else {
	            dbresult = FAIL;
		}
	        break;
        }

    }

    switch (retcode) {
        case CS_END_RESULTS:
            freefmtlist(dbproc);
            freefmtprev(dbproc);
            dbproc->last_result = CS_END_RESULTS;
            return NO_MORE_RESULTS;
            break;
        case CS_FAIL:
            dbresult = FAIL;
            break;
        default:
            break;
    }

    dbproc->last_result = 0;
    return dbresult;
}


/* ----------------------------------------------------------------------------
 * DBROWS  
 *
 */

int
DBROWS (dbproc)
    DBPROCESS *dbproc;
{
    return (dbproc->has_rows);
}



/* ----------------------------------------------------------------------------
 * dbhasretstat  
 *
 */

DBBOOL
dbhasretstat (dbproc)
    DBPROCESS *dbproc;
{
    return (dbproc->has_status);
}



/* ----------------------------------------------------------------------------
 * dbretstatus  
 *
 */

DBINT
dbretstatus (dbproc)
    DBPROCESS *dbproc;
{
    if (dbproc->has_status) {
        return dbproc->ret_status;
    }
    return 0;
}



/* ----------------------------------------------------------------------------
 * dbpoll  
 *
 */

RETCODE
dbpoll (dbproc, milliseconds, ready_dbproc, return_reason)
    DBPROCESS *dbproc;
    long      milliseconds;
    DBPROCESS **ready_dbproc;
    int       *return_reason;
{
    CS_RETCODE 		retcode;
    CS_CONNECTION	*compconn = NULL;
    CS_COMMAND		*compcmd = NULL;
    CS_INT		compid = 0;
    CS_RETCODE		compstatus = 0;

    if (milliseconds < 0L) {
        milliseconds = CS_NO_LIMIT;
    }
    if (dbproc == NULL) {
        retcode = ct_poll(context, NULL, milliseconds, &compconn, 
                &compcmd, &compid, &compstatus);
    } else {
        if (dbproc->last_result == CS_END_RESULTS) {
            *ready_dbproc = NULL;
            *return_reason = DBTIMEOUT;
            return FAIL;
        } else {
            retcode = ct_poll(NULL, dbproc->connection, milliseconds, NULL,
                  &compcmd, &compid, &compstatus);
        }
    }

    if (compstatus == CS_SUCCEED) {
	if (dbproc == NULL) {
	    /* get the dbproc from the connection returned */
            ct_con_props(compconn, CS_GET, CS_USERDATA, ready_dbproc, 
                     sizeof(DBPROCESS *), NULL);
	} else {
	    *ready_dbproc = dbproc;
	}
        *return_reason = DBRESULT;
        return SUCCEED;
    } else  if (compstatus == CS_QUIET) {
        *ready_dbproc = NULL;
        return SUCCEED;
    } else {
        *ready_dbproc = NULL;
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbsqlok  
 *
 */

RETCODE
dbsqlok (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE 		retcode;
    CS_CONNECTION	*compconn = NULL;
    CS_COMMAND		*compcmd = NULL;
    CS_INT		compid = 0;
    CS_RETCODE		compstatus = 0;
    CS_INT     		result;
    CS_INT		sync_io = CS_SYNC_IO;

    if (dbproc->last_result == CS_END_RESULTS) {
        return FAIL;
    }

    if (dbproc->async_mode) {
	if (dbproc->last_result == 0) {
	    /* go out of async mode and get ct_results */
            dbproc->async_mode = 0;
            ct_con_props(dbproc->connection, CS_SET, CS_NETIO, &sync_io,
                    CS_UNUSED, NULL);
	    /* send is always async, so get first results and save for later */
	    retcode = ct_results(dbproc->command, &result);
	    dbproc->last_result = result;
	    if (retcode == CS_END_RESULTS) {
                dbproc->last_result = retcode;
                return SUCCEED;
            }
	    if (retcode == CS_SUCCEED) {
		switch (result) {
		    case CS_CMD_FAIL:
			return FAIL;
		    case CS_CMD_SUCCEED:
		    case CS_CMD_DONE:
		    case CS_ROW_RESULT:
		    case CS_COMPUTE_RESULT:
		    case CS_STATUS_RESULT:
		    case CS_PARAM_RESULT:
		    default:
			return SUCCEED;
		}
	    } else {
		return FAIL;
	    }
	}
	/* else, do a poll */
        retcode = ct_poll(NULL, dbproc->connection, CS_NO_LIMIT, NULL,
            &compcmd, &compid, &compstatus);
        if (compstatus == CS_SUCCEED || compstatus == CS_QUIET) {
            return SUCCEED;
        } else {
            return FAIL;
        }
    } else {
        return SUCCEED;
    }
}



/* ----------------------------------------------------------------------------
 * dbnextrow  
 *
 */

int
dbnextrow (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE 	retcode;
    CS_INT	result;
    CS_INT	computeid;
    CS_INT	status;
    int		cols;
    int		num_cols;
    CS_DATAFMT  fmt;
    int         result_boundary = 0;
    int         tmp_done;


    if (dbproc->last_result == CS_END_RESULTS) {
        return NO_MORE_ROWS;
    }

    if (dbproc->first_fetch) {
        retcode = dbproc->fetch_result;
        dbproc->first_fetch = 0;
        dbproc->fetch_result = 0;
    } else {
        retcode = ct_fetch(dbproc->command,CS_UNUSED,CS_UNUSED,CS_UNUSED,NULL);
    }


    switch (retcode) {
        case CS_SUCCEED:
            dbproc->has_status = 0;
            return REG_ROW;

        case CS_END_DATA:
	    /* get column info in case app wants previous info */
	    if (dbproc->fmtlist == NULL) {
		    getcolumninfo(dbproc);
	    }

            /* process compute, param, & status results, if any */

get_more_results:
            
            retcode = ct_results(dbproc->command, &result);
	    dbproc->last_result = retcode;
            if (retcode == CS_END_RESULTS) {
                freefmtprev(dbproc);
                dbproc->last_result = CS_END_RESULTS;
                return NO_MORE_ROWS;
            }

            if (retcode == CS_SUCCEED) {
	        dbproc->last_result = result;
	        switch (result) {

	            case CS_CMD_DONE:
                        dbproc->last_result = 0;
                        freefmtprev(dbproc);
			if (result_boundary) {
	                    return NO_MORE_ROWS;
			}
			goto get_more_results;

	            case CS_ROW_RESULT:
			dbproc->first_fetch = 0;
			dbproc->fetch_result = 0;
			dbproc->has_status = 0;
	                return NO_MORE_ROWS;
			break;

	            case CS_COMPUTE_RESULT:
		        result_boundary = 1;
                        if (dbproc->last_result != CS_COMPUTE_RESULT) {
                            /* save rew select column information */
                            freefmtprev(dbproc);
                            dbproc->fmtprev = NULL;
                            if (dbproc->num_cols > 0) {
                                dbproc->fmtprev = (CS_DATAFMT *)
                                  ckalloc(dbproc->num_cols*sizeof(CS_DATAFMT) );
                                bcopy(dbproc->fmtlist, dbproc->fmtprev,
                                  dbproc->num_cols*sizeof(CS_DATAFMT) );
                                dbproc->fmtprev[0].maxlength=dbproc->num_cols;
                            }
                        }
	                getcolumninfo(dbproc);
			bind_cols(dbproc);
	                dbproc->has_rows = 0;
	                dbproc->has_status = 0;
	                retcode = ct_compute_info(dbproc->command, CS_COMP_ID, 
	                    	CS_UNUSED, &computeid, CS_UNUSED, NULL);
	                retcode = ct_fetch(dbproc->command,CS_UNUSED,
	                    		CS_UNUSED,CS_UNUSED,NULL);
                
			/* flush compute results */
	                ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
	                dbproc->last_result = CS_COMPUTE_RESULT;
			/* make next fetch do results first */
	                dbproc->first_fetch = 1;
	                dbproc->fetch_result = CS_END_DATA;
	                return computeid;

	            case CS_STATUS_RESULT:	
		        result_boundary = 1;
	                /* fetch the param row */
	                retcode = ct_fetch(dbproc->command,
	                    	CS_UNUSED, CS_UNUSED, CS_UNUSED, NULL);
	                dbproc->has_rows = 0;
	                dbproc->has_status = 1;
	                dbproc->first_fetch = 0;
	                dbproc->fetch_result = 0;
			status = 0;
			if (retcode == CS_SUCCEED) {
		            tmp_done = dbproc->bind_done;
		            dbproc->bind_done = 0;
	                    bcopy(dbdata(dbproc,1), &status, sizeof(CS_INT));
		            dbproc->bind_done = tmp_done;
			}
	                dbproc->ret_status = status;
	                ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
	                goto get_more_results;

                
	            case CS_PARAM_RESULT:
		        result_boundary = 1;
	                /* get param columns for later dbretxxxx()  */
	                dbproc->has_rows = 0;
	                dbproc->has_status = 1;
	                dbproc->first_fetch = 0;
	                dbproc->fetch_result = 0;
                        freefmtlist(dbproc);
                        freefmtprev(dbproc);
			bind_cols(dbproc);
	                /* fetch the param row */
	                retcode = ct_fetch(dbproc->command,
	                    	CS_UNUSED, CS_UNUSED, CS_UNUSED, NULL);
			num_cols = 0;
	                ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
                        freefmtprev(dbproc);
	                goto get_more_results;
	                

	            default:
	                goto get_more_results;
	                
	        }
            }

            return NO_MORE_ROWS;
            break;

        case CS_ROW_FAIL:
        case CS_FAIL:
        case CS_CANCELED:
        case CS_PENDING:
        case CS_BUSY:
        default:
            return FAIL;
    }

}



/* ----------------------------------------------------------------------------
 * dbnumcols  
 *
 */

int
dbnumcols (dbproc)
    DBPROCESS *dbproc;
{
    
    if (dbproc->fmtlist == NULL) {
	getcolumninfo(dbproc);
    }
    return dbproc->num_cols;
}



/* ----------------------------------------------------------------------------
 * dbnumalts  
 *
 */

int
dbnumalts (dbproc, computeid)
    DBPROCESS *dbproc;
    int       computeid;
{
    return dbnumcols(dbproc);
}



/* ----------------------------------------------------------------------------
 * dbaltlen  
 *
 */

DBINT
dbaltlen (dbproc, computeid, column)
    DBPROCESS *dbproc;
    int       computeid;
    int       column;
{
    return dbcollen(dbproc,column);
}


/* ----------------------------------------------------------------------------
 * dbcoltype  
 *
 */

int
dbcoltype (dbproc, column)
    DBPROCESS *dbproc;
    int       column;
{
    CS_DATAFMT *fmt;

    if (dbproc->fmtlist == NULL) {
	getcolumninfo(dbproc);
    }
    if (column >= 1 && column <= dbproc->num_cols) {
	fmt = dbproc->fmtlist;
        fmt += (column - 1);
        return (cttodbtype(fmt->datatype));
    } else {
	return SYBVOID;
    }
}


/* ----------------------------------------------------------------------------
 * dbcollen  
 *
 */

DBINT
dbcollen (dbproc, column)
    DBPROCESS *dbproc;
    int       column;
{
    CS_DATAFMT *fmt;

    if (dbproc->fmtlist == NULL) {
	getcolumninfo(dbproc);
    }
    if (column >= 1 && column <= dbproc->num_cols) {
	fmt = dbproc->fmtlist;
        fmt += (column - 1);
        return (fmt->maxlength);
    } else {
	return 0;
    }
}



/* ----------------------------------------------------------------------------
 * dbdata  
 *
 */

BYTE *
dbdata (dbproc, column)
    DBPROCESS *dbproc;
    int       column;
{
    CS_INT    len;
    CS_INT    type;

    if (dbproc->fmtlist == NULL) {
        getcolumninfo(dbproc);
    }
    if (dbproc->bind_done) {
	return (dbproc->isnull[column-1] != -1 ? dbproc->buf[column-1] : NULL);
    } else {
        len = dbcollen(dbproc, column);
        if (dbproc->fmtlist[column-1].datatype == CS_TEXT_TYPE ||
            dbproc->fmtlist[column-1].datatype == CS_IMAGE_TYPE) {
            if (len > dbproc->maxtext) {
                len = dbproc->maxtext;
            }
        }
        if (len > dbproc->buflen[column-1]) {
            ckfree(dbproc->buf[column-1]);
            dbproc->bufalloc[column-1] = len;
            dbproc->buf[column-1]  = (BYTE *) ckalloc(len);
        }
    
        bzero(dbproc->buf[column-1], dbproc->bufalloc[column-1]);
        ct_get_data(dbproc->command, column,dbproc->buf[column-1], 
		dbproc->bufalloc[column-1], &len);
        return (len > 0 ? dbproc->buf[column-1] : NULL);
    }
}



/* ----------------------------------------------------------------------------
 * dbdatlen  
 *
 */

DBINT
dbdatlen (dbproc, column)
    DBPROCESS *dbproc;
    int       column;
{
    if (dbproc->fmtlist == NULL) {
        getcolumninfo(dbproc);
    }
    if (dbproc->bind_done) {
	return (dbproc->isnull[column-1] != -1 ? dbproc->buflen[column-1] : 0);
    } else {
        return dbcollen(dbproc,column);
    }

}



/* ----------------------------------------------------------------------------
 * dbalttype  
 *
 */

int
dbalttype (dbproc, computeid, column)
    DBPROCESS *dbproc;
    int       computeid;
    int       column;
{
    return dbcoltype(dbproc,column);
}



/* ----------------------------------------------------------------------------
 * dbadlen  
 *
 */

DBINT
dbadlen (dbproc, computeid, column)
    DBPROCESS *dbproc;
    int       computeid;
    int       column;
{
    return dbdatlen(dbproc,column);
}



/* ----------------------------------------------------------------------------
 * dbadata  
 *
 */

BYTE *
dbadata (dbproc, computeid, column)
    DBPROCESS *dbproc;
    int       computeid;
    int       column;
{
    return (dbdata(dbproc, column));
}


/* ----------------------------------------------------------------------------
 * dbcolname  
 *
 */

char *
dbcolname (dbproc, column)
    DBPROCESS *dbproc;
    int       column;
{
    CS_DATAFMT *fmt;

    if (dbproc->fmtlist == NULL) {
	getcolumninfo(dbproc);
    }
    if (column < 0) {
        column = (-column);
        if (dbproc->fmtprev != NULL) {
            fmt = dbproc->fmtprev;
	} else {
            return "";
	}
    } else {
	if (column > dbproc->num_cols) {
	    return "";
	}
	fmt = dbproc->fmtlist;
    }
    fmt += (column - 1);
    return (fmt->namelen > 0 && fmt->namelen <= 30 ? fmt->name : "");
}



/* ----------------------------------------------------------------------------
 * dbprtype  
 *
 */

char *
dbprtype (token)
    int token;
{
    switch (token) {
      case SYBCHAR       : return "char";
      case SYBTEXT       : return "text";
      case SYBBINARY     : return "binary";
      case SYBIMAGE      : return "image";
      case SYBINT1       : return "tinyint";
      case SYBINT2       : return "smallint";
      case SYBINT4       : return "int";
      case SYBFLT8       : return "float";
      case SYBREAL       : return "real";
      case SYBNUMERIC    : return "numeric";
      case SYBDECIMAL    : return "decimal";
      case SYBBIT        : return "bit";
      case SYBMONEY      : return "money";
      case SYBMONEY4     : return "smallmoney";
      case SYBDATETIME   : return "datetime";
      case SYBDATETIME4  : return "smalldatetime";
      case SYBBOUNDARY   : return "boundary";
      case SYBSENSITIVITY: return "sensitivity";
      case SYBAOPSUM     : return "sum";
      case SYBAOPAVG     : return "avg";
      case SYBAOPCNT     : return "count";
      case SYBAOPMIN     : return "min";
      case SYBAOPMAX     : return "max";
      default            : return "";
    }
}


/* ----------------------------------------------------------------------------
 * dbaltcolid  
 *
 */

int 
dbaltcolid (dbproc, computeid, column)
    DBPROCESS *dbproc;
    int       computeid;
    int       column;
{
    if (dbproc->fmtlist == NULL) {
      getcolumninfo(dbproc);
    }
    if (column >= 1 && column <= dbproc->num_cols) {
        return (-(dbproc->compute_colid[column-1]));
    } else {
      return 0;
    }
}


/* ----------------------------------------------------------------------------
 * dbaltop  
 *
 */

int 
dbaltop (dbproc, computeid, column)
    DBPROCESS *dbproc;
    int       computeid;
    int       column;
{
    if (dbproc->fmtlist == NULL) {
      getcolumninfo(dbproc);
    }
    if (column >= 1 && column <= dbproc->num_cols) {
        return (dbproc->compute_altop[column-1]);
    } else {
      return 0;
    }
}



/* ----------------------------------------------------------------------------
 * dbretname  
 *
 */

char *
dbretname (dbproc, retnum)
    DBPROCESS *dbproc;
    int       retnum;
{
    if (dbproc->has_status) {
	return dbcolname(dbproc,retnum);
    } else {
	return "";
    }
}



/* ----------------------------------------------------------------------------
 * dbretlen  
 *
 */

DBINT
dbretlen (dbproc, retnum)
    DBPROCESS *dbproc;
    int       retnum;
{
    if (dbproc->has_status) {
	return dbdatlen(dbproc,retnum);
    } else {
	return 0;
    }
}



/* ----------------------------------------------------------------------------
 * dbrettype  
 *
 */

int
dbrettype (dbproc, retnum)
    DBPROCESS *dbproc;
    int       retnum;
{
    if (dbproc->has_status) {
	return dbcoltype(dbproc,retnum);
    } else {
        return SYBVOID;
    }
}



/* ----------------------------------------------------------------------------
 * dbnumrets  
 *
 */

DBINT
dbnumrets (dbproc)
    DBPROCESS *dbproc;
{
    if (dbproc->has_status) {
	return dbproc->num_cols;
    } else {
	return 0;
    }
}



/* ----------------------------------------------------------------------------
 * dbretdata  
 *
 */

BYTE *
dbretdata (dbproc, retnum)
    DBPROCESS *dbproc;
    int       retnum;
{
    if (dbproc->has_status) {
	return dbdata(dbproc,retnum);
    } else {
        return NULL;
    }
}




/* ----------------------------------------------------------------------------
 * dbclose  
 *
 */

VOID
dbclose (dbproc)
    DBPROCESS *dbproc;
{
    ProcList  *proc_ptr, **prev_ptr;

    ct_close(dbproc->connection, CS_FORCE_CLOSE);

    ct_cmd_drop(dbproc->command);
    ct_con_drop(dbproc->connection);
    ckfree(dbproc->buf);
    freefmtlist(dbproc);
    freefmtprev(dbproc);

    /* remove proclist struct from proc list */

    proc_ptr = start_proc_list;
    prev_ptr = (ProcList **) &start_proc_list;
    while (proc_ptr != NULL) {
        if (proc_ptr->dbp == dbproc) {
            break;
        }
        prev_ptr = (ProcList **) &proc_ptr->next;
        proc_ptr = proc_ptr->next;
    }
    if (proc_ptr != NULL) {
        *prev_ptr = proc_ptr->next;
        ckfree(proc_ptr);
    }
    ckfree(dbproc);

}


/* ----------------------------------------------------------------------------
 * dbcanquery  
 *
 */

RETCODE
dbcanquery (dbproc)
    DBPROCESS *dbproc;
{
    CS_RETCODE retcode;

    if (dbproc->ignore || 
	dbproc->last_result == 0 || 
	dbproc->last_result == CS_END_RESULTS) {
	dbproc->last_result = CS_END_RESULTS;
	return SUCCEED;
    }

    retcode = ct_cancel(NULL, dbproc->command, CS_CANCEL_CURRENT);
    dbproc->last_result = 0;
    if (retcode == CS_SUCCEED) {
        return SUCCEED;
    } else {
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbwritetext  
 *
 */

RETCODE
dbwritetext (dbproc, objname, textptr, textptrlen, timestamp, log, size, text)
    DBPROCESS *dbproc;
    char      *objname;
    DBBINARY  *textptr;
    int       textptrlen;
    DBBINARY  *timestamp;
    DBBOOL    log;
    DBINT     size;
    BYTE      *text;
{
    CS_RETCODE  retcode;
    CS_INT	sync_io = CS_SYNC_IO;
    CS_INT	column;
    CS_INT	num_cols;

    /* start with 0 sizes */
    dbproc->textsize = 0;
    dbproc->textsent = 0;
    dbproc->ignore = 1;

    /* and make sure not in async mode */
    if (dbproc->async_mode) {
        dbproc->async_mode = 0;
        ct_con_props(dbproc->connection, CS_SET, CS_NETIO, &sync_io,
                    CS_UNUSED, NULL);
    }

    /* start in a fresh state */
    ct_cancel(dbproc->connection, NULL, CS_CANCEL_ALL);
    ct_cancel(NULL, dbproc->command, CS_CANCEL_ALL);

    retcode = ct_command(dbproc->command, CS_SEND_DATA_CMD, NULL, CS_UNUSED,
                CS_COLUMN_DATA);

    if (retcode != CS_SUCCEED) {
        return FAIL;
    }

    
/* iodesc should have same info, so just leave it alone ?
    strncpy(iodesc.name, objname, CS_OBJ_NAME);
    iodesc.name[CS_OBJ_NAME-1] = '\0';  
    iodesc.namelen = CS_NULLTERM;
    bcopy(textptr,   iodesc.textptr, CS_TP_SIZE);
    bcopy(timestamp, iodesc.timestamp, CS_TS_SIZE);
*/
    iodesc.total_txtlen = size;
    iodesc.log_on_update = (log) ? CS_TRUE : CS_FALSE;

    retcode = ct_data_info(dbproc->command, CS_SET, CS_UNUSED, &iodesc);

    dbproc->last_result = 0;
    last_io_col = 0;
    if (retcode == CS_SUCCEED) {
        dbproc->textsize = size;
	/* if text supplied, call moretext */
	if (text != NULL) {
	    return (dbmoretext(dbproc, size, (BYTE *)text));
	} else {
            return SUCCEED;
	}
    } else {
        return FAIL;
    }
}


/* ----------------------------------------------------------------------------
 * dbtxptr  
 *
 */

DBBINARY *
dbtxptr (dbproc, column)
    DBPROCESS *dbproc;
    int       column; 
{
    CS_RETCODE retcode;

    if (last_io_col == 0 || last_io_col != column) {
	last_io_col = column;
        retcode = ct_get_data(dbproc->command, column, &iodesc, 0, NULL);
        retcode = ct_data_info(dbproc->command, CS_GET, column, &iodesc);
    } else {
	retcode = CS_SUCCEED;
    }

    if (retcode == CS_SUCCEED) {
        return (DBBINARY *) &iodesc.textptr[0];
    } else {
        return NULL;
    }
}


/* ----------------------------------------------------------------------------
 * dbtxtimestamp  
 *
 */

DBBINARY *
dbtxtimestamp (dbproc, column)
    DBPROCESS *dbproc;
    int       column;
{
    CS_RETCODE retcode;
    
    if (last_io_col == 0 || last_io_col != column) {
	last_io_col = column;
        retcode = ct_get_data(dbproc->command, column, &iodesc, 0, NULL);
        retcode = ct_data_info(dbproc->command, CS_GET, column, &iodesc);
    } else {
	retcode = CS_SUCCEED;
    }

    if (retcode == CS_SUCCEED) {
        return (DBBINARY *) &iodesc.timestamp[0];
    } else {
        return NULL;
    }
}


/* ----------------------------------------------------------------------------
 * dbmoretext  
 *
 */

RETCODE
dbmoretext (dbproc, size, text)
    DBPROCESS *dbproc;
    DBINT     size;
    BYTE      *text;
{
    CS_RETCODE retcode;
    CS_INT	result;
    CS_INT	succeed = 0;

    retcode = ct_send_data(dbproc->command, text, size);

    if (retcode != CS_SUCCEED) {
        dbproc->ignore = 0;
        return FAIL;
    }
    
    dbproc->textsent += size;

    /* if all data sent, then finish */

    if (dbproc->textsent >= dbproc->textsize) {
        dbproc->textsent = 0;
        dbproc->textsize = 0;
        dbproc->ignore = 0;
        retcode = ct_send(dbproc->command);
        if (retcode == CS_SUCCEED) {
            return SUCCEED;
        } else {
            return FAIL;
        }
    } else {
        return SUCCEED;
    }
}


/* ----------------------------------------------------------------------------
 * dbreadtext  
 *
 */

int
dbreadtext (dbproc, buf, bufsize)
    DBPROCESS *dbproc;
    BYTE      *buf;
    DBINT     bufsize;
{
    CS_RETCODE retcode;
    CS_INT   column = 1;	/* dbreadtext only reads from column 1 */
    CS_INT   outlen = 0;;
    
    if (dbproc->textsize == 0) {
        retcode = ct_get_data(dbproc->command, column, &iodesc, 0, NULL);
        retcode = ct_data_info(dbproc->command, CS_GET, column, &iodesc);
        dbproc->textsize = iodesc.total_txtlen;
        dbproc->textsent = 0;
    }

    if (dbproc->textsent >= dbproc->textsize) {
        dbproc->textsent = 0;
        dbproc->textsize = 0;
        return NO_MORE_ROWS;
    }

    retcode = ct_get_data(dbproc->command, column, buf, bufsize, &outlen);

    last_io_col = 0;

    if (retcode == CS_END_ITEM || retcode == CS_END_DATA) {
	dbproc->textsent += outlen;
	return outlen;
    }
    if (retcode == CS_SUCCEED) {
        dbproc->textsent += outlen;
        return outlen;
    } else {
        return -1;
    }
}


/* ----------------------------------------------------------------------------
 * dbmnyadd
 *
 */

RETCODE
dbmnyadd (dbproc, m1, m2, result)
    DBPROCESS *dbproc;
    DBMONEY   *m1;
    DBMONEY   *m2;
    DBMONEY   *result;
{
    return  (cs_calc(context, CS_ADD, CS_MONEY_TYPE,
    	(CS_VOID *) m1, (CS_VOID *) m2, (CS_VOID *) result) == CS_SUCCEED) ?
	SUCCEED : FAIL;
}

/* ----------------------------------------------------------------------------
 * dbmnysub
 *
 */

RETCODE
dbmnysub (dbproc, m1, m2, result)
    DBPROCESS *dbproc;
    DBMONEY   *m1;
    DBMONEY   *m2;
    DBMONEY   *result;
{
    return  (cs_calc(context, CS_SUB, CS_MONEY_TYPE,
    	(CS_VOID *) m1, (CS_VOID *) m2, (CS_VOID *) result) == CS_SUCCEED) ?
	SUCCEED : FAIL;
}

/* ----------------------------------------------------------------------------
 * dbmnymul
 *
 */

RETCODE
dbmnymul (dbproc, m1, m2, result)
    DBPROCESS *dbproc;
    DBMONEY   *m1;
    DBMONEY   *m2;
    DBMONEY   *result;
{
    return  (cs_calc(context, CS_MULT, CS_MONEY_TYPE,
    	(CS_VOID *) m1, (CS_VOID *) m2, (CS_VOID *) result) == CS_SUCCEED) ?
	SUCCEED : FAIL;
}

/* ----------------------------------------------------------------------------
 * dbmnydivide
 *
 */

RETCODE
dbmnydivide (dbproc, m1, m2, result)
    DBPROCESS *dbproc;
    DBMONEY   *m1;
    DBMONEY   *m2;
    DBMONEY   *result;
{
    return  (cs_calc(context, CS_DIV, CS_MONEY_TYPE,
    	(CS_VOID *) m1, (CS_VOID *) m2, (CS_VOID *) result) == CS_SUCCEED) ?
	SUCCEED : FAIL;
}

/* ----------------------------------------------------------------------------
 * dbmnycmp
 *
 */

int
dbmnycmp (dbproc, m1, m2)
    DBPROCESS *dbproc;
    DBMONEY   *m1;
    DBMONEY   *m2;
{
    CS_INT result;

    cs_cmp(context, CS_MONEY_TYPE,
    			(CS_VOID *) m1, (CS_VOID *) m2, &result);

    return result;
}

/* finis */

