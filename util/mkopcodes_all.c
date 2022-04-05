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
    printf("#define MINITITLE\t\"all Opcodes\"\n");
    for (i=0; dict[i].tag!=NULL; i++) {
      printf("#define %s\n", dict[i].tag);
    }
    fclose(outp);
}
        
