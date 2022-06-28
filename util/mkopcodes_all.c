#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int verbose = 1;

#include "./Dictionary.c"

int main(void)
{
    FILE* outp = fopen("opcodes_all.h", "w");
    int i;

    if (outp == NULL) {
      fprintf(stderr, "Failed to open output\n");
      exit(1);
    }
    fprintf(outp, "#define MINITITLE\t\"all Opcodes\"\n");
    for (i=0; dict[i].tag!=NULL; i++) 
      fprintf(outp, "#define %s\n", dict[i].tag);
    for(i=1; i<60; i++)
      fprintf(outp, "#define INC_GEN%02d\n", i);
    
    fclose(outp);
}
        
