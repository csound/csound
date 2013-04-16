#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIST "all_string_files"

void process(char *name, int width)
{
    FILE *ff = fopen(name, "r");
    char buffer[256];
    int count = 0;

    while (1) {
      char *p;
      if (fgets(buffer, 255, ff)==NULL) break;
      count++;
      p = strchr(buffer, '\n');
      if (p==NULL || (p-buffer) > width) {
        printf("File=%s Line=%d:\n%s\n", name, count, buffer);
      }
      p = strchr(buffer, '\t');
      if (p!=NULL)
        printf("File=%s Line=%d tab at %d\n%s\n", name, count, p-buffer, buffer);
      p = strstr(buffer, " \n");
      if (p!=NULL)
        printf("File=%s Line=%d extra space at %d\n%s\n",
               name, count, p-buffer, buffer);
    }
    fclose(ff);
    return;
}

int main(int argc, char **argv)
{
    FILE *ff = fopen(LIST, "r");
    char buffer[128];
    int width = (argc==1 ? 85 : atoi(argv[1]));

    if (ff==NULL) {
      printf("No list of files\n");
      exit(1);
    }

    while (1) {
      char *p;
      if (fgets(buffer, 127, ff)==NULL) break;
      p = strchr(buffer, '\n');
      if (p) *p = '\0';
      process(buffer, width);
    }
}
