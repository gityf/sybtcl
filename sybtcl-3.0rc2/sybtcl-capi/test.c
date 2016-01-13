#include "sybtcl-capi.h"

#include <stdio.h>

main(argc, argv) 
int argc;
char *argv[];
{

  Tcl_Interp 	*interp;
  char 		handle[20];
  int		result;
  int 		i;

  if (argc < 4) {
     printf("usage: %s userid password sql-string\n",argv[0]);
     exit(0);
  }


  interp = Sybtcl_MkInterp();
  Sybtcl_Init(interp);

  result = Sybconnect(interp, argv[1], argv[2], NULL, NULL, NULL );
  printf("sybconnect = %d, result = %s\n", result, RESULT(interp));

  if (result != TCL_OK) exit();

  /* save the connection handle */
  strcpy(handle, RESULT(interp));

  result = Sybsql(interp, handle, argv[3], NULL);
  printf("sybsql = %d, result = %s\n", result, RESULT(interp));

  while (strcmp(GET_STATUS(interp,Nextrow),"REG_ROW")==0) {
    result = Sybnext(interp, handle);
    printf("sybnext = %d, result = %s\n", result, RESULT(interp));
    for (i = 0; i < NUMCOLS(interp); i++) {
      printf("column %d value is: %s\n", i, COLUMN(interp,i));
    }
  }

  result = Sybcols(interp, handle);
  printf("sybcols = %d, colnames = ", result);
  for (i = 0; i < NUMCOLS(interp); i++) {
    printf("%s ", COLUMN(interp,i));
  }
  printf("\ntypes = %s\n",GET_STATUS(interp,Coltypes));
  printf("lengths = %s\n",GET_STATUS(interp,Collengths));


  result = Sybclose(interp, handle); 

  Sybtcl_FreeInterp(interp);

}


