###############################################################################
#
# Sybtcl-2.6
# Makefile for sybtcl-capi
#
# Tom Poindexter 

SHELL = /bin/sh

###############################################################################
#
# set the following defines as needed: 


#------------------------------------------------------------------------------
# define where Sybase db-lib libraries are located

#SYBASE =  /define/this/if/SYBASE/is/not/in/your/environment

SYBINC = $(SYBASE)/include
SYBLIB = $(SYBASE)/lib/libsybdb.a


#------------------------------------------------------------------------------
# compiler to use

CC = cc -g
#CC = cc -g -Aa

CCOPTS = -I$(SYBINC) -I.

#------------------------------------------------------------------------------
# Other libs, e.g. this is what solaris needs to link socket code

LIBS = -lnsl -lsocket -lm
#LIBS = -lm

#
# end of defines
#
###############################################################################


all:	testcapi


#------------------------------------------------------------------------------
# how to build objects for sybtcl-capi

sybtcl26.o:	sybtcl26.c tcl.h
	$(CC) -c $(CCOPTS) sybtcl26.c

sybtcl-capi.o:	sybtcl-capi.c sybtcl-capi.h tcl.h
	$(CC) -c $(CCOPTS) sybtcl-capi.c

test.o:		test.c sybtcl-capi.h 
	$(CC) -c $(CCOPTS) test.c

testcapi:	test.o sybtcl-capi.o sybtcl26.o
	$(CC) -o testcapi test.o sybtcl-capi.o sybtcl26.o $(SYBLIB) $(LIBS)


clean:	
	/bin/rm -f sybtcl26.o sybtcl-capi.o test.o testcapi


###############################################################################
# end of Makefile
###############################################################################

