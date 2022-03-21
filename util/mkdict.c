#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int verbose = 1;

int process_file(char *input, FILE* outp)
{
    char buffer[126];
    char tag[64];
    char opcode[64];
    char *p, *q;

    FILE* inp = fopen(input, "r");
    if (inp == NULL) {
      fprintf(stderr, "Failed to open input\n");
      return 1;
    }
    // Find start of OENTRY table
    do {
      fgets(buffer, 120, inp);
      if (feof(inp)) {
        fclose(inp);
        return 0;
      }
    } while ((p = strstr(buffer, "OENTRY"))==NULL);

    // Process fior opcode/tag pairs
 gettag:


    do {
      fgets(buffer, 120, inp);
      if (feof(inp)) {
        fclose(inp);
        return 0;
      }
    } while ((p = strstr(buffer, "#ifdef INC_"))==NULL);
    strncpy(tag, p+11, 60);
    if (p=strchr(tag, '\r')) *p = '\0';
    if (p=strchr(tag, '\n')) *p = '\0';
    if (verbose) printf("Tag:>>%s<<\n", tag);
    // Read opcode
 opcode:
    fgets(buffer, 120, inp);
    if (p = strstr(buffer, "#endif")) goto gettag;
    if (p = strchr(buffer, '"')) {
      // Expect opcode
      q = strchr(p+1,'"');
      if (q) *(q+1) = '\0';
      if (verbose) printf("opcode>>%s<<\n", p+1);
      fprintf(outp, "\t{\"%s, \"INC_%s\" },\n", p+1, tag);
      goto opcode;
    }
    goto opcode;
}



int main(void)
{
    FILE* outp = fopen("Dictionary.c", "w");
    if (outp == NULL) {
      fprintf(stderr, "Failed to open output\n");
      exit(1);
    }
    fprintf(outp, "typedef struct {\n\tchar* opcode;\n\tchar* tag; } DICTIONARY;");
    fprintf(outp, "\nDICTIONARY dict[] = {\n");
    process_file("Engine/entry1.c", outp);
    process_file("Opcodes/babo.c", outp);

    fprintf(outp, "\t{ NULL, NULL } };\n\n");
    fclose(outp);
}
        
