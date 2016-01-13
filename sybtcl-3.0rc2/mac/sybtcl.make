# Sybtcl - make ppc shared lib for mac
# requires Sybase 10.0.4 or better client libraries

# define PROJECT, project_VERSION, and DLL_VERSION for use by install.tcl

SYBTCL_VERSION = 3.0
DLL_VERSION = 30
PROJECT = sybtcl{DLL_VERSION}

DriveName    =  dogbert
TclLibDir    = {DriveName}:System Folder:Extensions:Tool Command Language
TclLib       = {TclLibDir}:Tcl8.0.shlb
TclSourceDir = {DriveName}:Tcl/Tk Folder:tcl8.0
Sybase       = {DriveName}:Sybase
SybaseLibDir = {Sybase}:lib:CFM (FAT)
SybaseLib    = {SybaseLibDir}:libsybdb

MAKEFILE     = sybtcl.make
�MondoBuild� = {MAKEFILE}
Includes     = 	-i "{TclSourceDir}:generic:" -i "{Sybase}:include"
Sym�PPC      = 
ObjDir�PPC   =

PPCCOptions  = {Includes} {Sym�PPC} �
		-typecheck relaxed -d MAC_TCL �
		-shared_lib_export on -export_list syms.exp

Objects�PPC  = �
		"{ObjDir�PPC}sybtcl.c.x"


{PROJECT}.shlb �� {�MondoBuild�} {Objects�PPC}
	PPCLink �
		-o {Targ} {Sym�PPC} �
		{Objects�PPC} �
		-t 'shlb' �
		-c '????' �
		-xm s �
		-@export syms.exp �
		"{SharedLibraries}InterfaceLib" �
		"{SharedLibraries}StdCLib" �
		"{SharedLibraries}MathLib" �
		"{SybaseLib}" �
		"{TclLib}" �
		"{PPCLibraries}StdCRuntime.o" �
		"{PPCLibraries}PPCCRuntime.o" �
		"{PPCLibraries}PPCToolLibs.o"


"{ObjDir�PPC}sybtcl.c.x" � {�MondoBuild�} sybtcl.c
	{PPCC} sybtcl.c -o {Targ} {PPCCOptions}

sybtcl �� {PROJECT}.shlb

