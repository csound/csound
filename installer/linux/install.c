#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char *EXECUTABLES = "EXE";
char *OPCODES32 = "OPCODES32";
char *OPCODES64 = "OPCODES64";
char *HEADERS = "HDR";
char *LIBRARIES32 = "LIB32";
char *LIBRARIES64 = "LIB64";

char *prefix  = "/usr/local/";
char *headers = "include/csound/";
char *libdir32 = "lib/";
char *opcdir32 = "csound/opcodes/";
char *libdir64 = "lib64";
char *opcdir64 = "csound/opcodes64/";

void check_exists(char *dir)
{
    char test[80];
    char *p;
    int ans;
    struct stat buf;
    strcpy(test,dir);
    p = test;
    while ((p = strchr(p+1, '/')) != NULL) {
      *p = '\0';
      ans = stat(test, &buf);
      if (ans!=0 || !S_ISDIR(buf.st_mode)) {
        if (ans!=0) {
          printf("Directory %s does not exist; creating...\n", test);
          mkdir(test, 0766);
        }
        else {
          perror("Trouble with file; stopping");
          exit(1);
        }
      }
      else {
        printf("Directort %s OK\n", test);
      }
      *p = '/';
    }
    return;
}

int main(int argc, char **argv)
{
    char *opcdir = opcdir32;
    char *libdir = libdir32;
    int single = 0;
    int size = 0;
    
    argv++;
    while (argc>1) {
      if (strncmp(*argv, "-PREFIX=", 8)==0) {
        prefix = &(*argv[8]);
        if (prefix[strlen(prefix)-1] != '/') {
          char *p = (char *)malloc(strlen(prefix+2));
          strcpy(p, prefix);
          strcat(p, "/");
        }
      }      
      else if (strcmp(*argv, "-float")==0 || strcmp(*argv, "-FLOAT")==0) {
        opcdir = opcdir32;
        single |= 1;
      }
      else if (strcmp(*argv, "-double")==0 || strcmp(*argv, "-DOUBLE")==0) {
        opcdir = opcdir64;
        single |= 2;
      }
      else if (strcmp(*argv, "-32")==0) {
        libdir = libdir32;
        size |= 1;
      }
      else if (strcmp(*argv, "-64")==0) {
        libdir = libdir64;
        size |= 2;
      }
      else if (strncmp(*argv, "-headers=", 9)==0 ||
               strncmp(*argv, "-HEADERS=", 9)==0) {
        headers = &(*argv[9]);
      }
      argc--; argv++;
    }
    if (single == 0) single = 2;
    if (size == 0) size = 1;
    printf("Installing Csound5 in %s mode into $sbin\n",
           (single==1 ? "float(32bit)" :
            "double(64bit)"), prefix);
    printf("Opcodes are going into %s%s%s\n", prefix, libdir, opcdir);
    printf("Libraries are going into %s%s\n", prefix, libdir);
    printf("Installing for %s processors\n",
           size==1 ? "32bit" : size==2 ? "64bit" : "both 32 and 64 bit");
    for (;;) {
      char buff[80];
      printf("Type Y to continue or N to abort\n");
      gets(buff);
      if (strcmp("Y", buff)==0 || strcmp("y", buff)==0 ||
          strcmp("Yes", buff)==0 || strcmp("yes", buff)==0 ||
          strcmp("YES", buff)==0)  break;
      if (strcmp("N", buff)==0 || strcmp("n", buff)==0 ||
          strcmp("No", buff)==0 || strcmp("no", buff)==0 ||
          strcmp("NO", buff)==0) exit(1);
    }
    /* Start the installation */
    {
      /* Install binaries from bin[fd] or bin[fd]64 directory */
      char b[100], s[120];
      strcpy(b,prefix);
      strcat(b,"bin");
      if (size==2) strcat(b,"64");
      check_exists(b);
      if (single==1) {
        sprintf(s, "cp -pv binf%s/* %s/csound", (size==1?"":"64"),b);
        system(s);
      }
      else {
        sprintf(s, "cp -pv bind%s/* %s/csound", (size==1?"":"64"),b);
        system(s);
      }
    }
    {
      /* Install Opcodes etc from opc[fd] or opc[fd]64 */
      char b[100], s[120];
      strcpy(b,prefix);
      strcat(b,"lib");
      if (size==2) strcat(b,"64");
      strcat(b,"/csound/opcodes");
      if (single==2) strcat(b,"64");
      check_exists(b);
      if (single==1) {
        sprintf(s, "cp -pv opcf%s/* %s", (size==1?"":"64"),b);
        system(s);
      }
      else {
        sprintf(s, "cp -pv opcd%s/* %s", (size==1?"":"64"),b);
        system(s);
      }
    }
    {
      /* Install headers etc from hdr */
      char b[100], s[120];
      strcpy(b,prefix);
      strcat(b,"include/csound");
      check_exists(b);
      sprintf(s, "cp -pv hdr/* %s", b);
      system(s);
    }
    {
      /* Install Opcodes etc from opc[fd] or opc[fd]64 */
      char b[100], s[120];
      strcpy(b,prefix);
      strcat(b,"lib");
      if (size==2) strcat(b,"64");
      check_exists(b);
      if (single==1) {
        sprintf(s, "cp -pv libf%s/* %s", (size==1?"":"64"),b);
        system(s);
      }
      else {
        sprintf(s, "cp -pv libd%s/* %s", (size==1?"":"64"),b);
        system(s);
      }
    }


}

