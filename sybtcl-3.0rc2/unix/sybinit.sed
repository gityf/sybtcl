/if.*Tcl[X]*_Init.*interp.*==.*TCL_ERROR/{
n
n
n
i\
\
\ \ \ \ if (Sybtcl_Init (interp) == TCL_ERROR) {\
\ \ \ \     return TCL_ERROR;\
\ \ \ \ }\
\ \ \ \ {int Sybtcl_SafeInit(Tcl_Interp *);\
\ \ \ \ Tcl_StaticPackage (interp, "Sybtcl", Sybtcl_Init, Sybtcl_SafeInit);}

}
