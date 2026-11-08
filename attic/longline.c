#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIST "all_string_files"

void process(char *name, int width)
{
    FILE *ff = fopen(name, "r");
    char buffer[256];
    int count = 0;
    if (ff==NULL) {
      printf("File=%s does not exist\n", name);
      return;
    }
    while (1) {
      char *p;
      if (fgets(buffer, 255, ff)==NULL) break;
      count++;
      p = strchr(buffer, '\n');
      if (p==NULL || (p-buffer) > width) {
        printf("File=%s Line=%d long\n%s\n", name, count, buffer);
      }
      p = strchr(buffer, '\t');
      if (p!=NULL)
        printf("File=%s Line=%d TAB at %d\n%s\n", name, count, p-buffer, buffer);
      p = strstr(buffer, " \n");
      if (p!=NULL)
        printf("File=%s Line=%d extra space at %d\n%s\n",
               name, count, p-buffer, buffer);
      p = strstr(buffer,"if(");
      if (p!=NULL)
        printf("File=%s Line=%d if( at %d\n%s\n",
               name, count, p-buffer, buffer);
      p = strstr(buffer,"while(");
      if (p!=NULL)
        printf("File=%s Line=%d while( at %d\n%s\n",
               name, count, p-buffer, buffer);
      p = strstr(buffer,"for(");
      if (p!=NULL)
        printf("File=%s Line=%d for( at %d\n%s\n",
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
    fclose(ff);
}
