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
ÄMondoBuildÄ = {MAKEFILE}
Includes     = 	-i "{TclSourceDir}:generic:" -i "{Sybase}:include"
SymÄPPC      = 
ObjDirÄPPC   =

PPCCOptions  = {Includes} {SymÄPPC} è
		-typecheck relaxed -d MAC_TCL è
		-shared_lib_export on -export_list syms.exp

ObjectsÄPPC  = è
		"{ObjDirÄPPC}sybtcl.c.x"


{PROJECT}.shlb üü {ÄMondoBuildÄ} {ObjectsÄPPC}
	PPCLink è
		-o {Targ} {SymÄPPC} è
		{ObjectsÄPPC} è
		-t 'shlb' è
		-c '????' è
		-xm s è
		-@export syms.exp è
		"{SharedLibraries}InterfaceLib" è
		"{SharedLibraries}StdCLib" è
		"{SharedLibraries}MathLib" è
		"{SybaseLib}" è
		"{TclLib}" è
		"{PPCLibraries}StdCRuntime.o" è
		"{PPCLibraries}PPCCRuntime.o" è
		"{PPCLibraries}PPCToolLibs.o"


"{ObjDirÄPPC}sybtcl.c.x" ü {ÄMondoBuildÄ} sybtcl.c
	{PPCC} sybtcl.c -o {Targ} {PPCCOptions}

sybtcl üü {PROJECT}.shlb

