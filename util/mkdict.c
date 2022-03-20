#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>


int main(void)
{
    FILE* inp = fopen("Engine/entry1.c", "r");
    FILE* outp = fopen("Dictionary", "w");
    char buffer[126];
    char tag[64];
    char opcode[64];
    char *p, *q;

    if (inp == NULL) {
      fprintf(stderr, "Failed to open input\n");
      exit(1);
    }
    fprintf(outp, "typedef struct {\n\tchar* opcode;\n\tchar* tag; } DICTIONARY;");
    fprintf(outp, "\nDICTIONARY dict[] = {\n");
 gettag:
    do {
      if (feof(inp)) {
        fprintf(outp, "{NULL, NULL}\n };");
        fclose(outp); exit(0);
      }
      fgets(buffer, 120, inp);
    } while ((p = strstr(buffer, "#ifdef INC_"))==NULL);
    strncpy(tag, p+11, 60);
    if (p=strchr(tag, '\r')) *p = '\0';
    if (p=strchr(tag, '\n')) *p = '\0';
    printf("Tag:>>%s<<\n", tag);
    // Read opcode
 opcode:
    fgets(buffer, 120, inp);
    if (p = strstr(buffer, "#endif")) goto gettag;
    if (p = strchr(buffer, '"')) {
      // Expect opcode
      q = strchr(p+1,'"');
      if (q) *(q+1) = '\0';
      printf("opcode>>%s<<\n", p+1);
      fprintf(outp, "{\"%s, \"INC_%s\" },\n", p+1, tag);
      goto opcode;
    }
    goto opcode;
}
