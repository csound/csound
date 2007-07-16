#include <stdio.h>
#include <stdlib>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char *LIBRARIES32 = "LIB32";
char *LIBRARIES64 = "LIB64";

char *prefix  = "/usr/local/";
/* char *headers = "include/csound/"; */
char *libdir32 = "lib/";
char *opcdir32 = "csound/opcodes/";
char *libdir64 = "lib64";
char *opcdir64 = "csound/opcodes64/";

/* Unsure that dir exists with all directories on the way. */
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
        printf("Directory %s OK\n", test);
      }
      *p = '/';
    }
    return;
}

/* Run an installer acording to version */
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
/*       else if (strncmp(*argv, "-headers=", 9)==0 || */
/*                strncmp(*argv, "-HEADERS=", 9)==0) { */
/*         headers = &(*argv[9]); */
/*       } */
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
      char *p;
      printf("Type Y to continue or N to abort\n");
      fgets(buff, 80, stdin);
      if ((p=strchr(buff, '\n'))!=NULL) *p='\0';
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
/*     { */
/*       /\* Install headers etc from hdr *\/ */
/*       char b[100], s[120]; */
/*       strcpy(b,prefix); */
/*       strcat(b,"include/csound"); */
/*       check_exists(b); */
/*       sprintf(s, "cp -pv hdr/\* %s", b); */
/*       system(s); */
/*     } */
    {
      /* Install libraries from lib[fd] or lib[fd]64 */
      char b[100], s[120];
      strcpy(b,prefix);
      strcat(b,"lib");
      if (size==2) strcat(b,"64");
      check_exists(b);
      if (single==1) {
        sprintf(s, "cp -pv libf%s/libcsound.a %s", (size==1?"":"64"),b);
        system(s);
      }
      else {
        sprintf(s, "cp -pv libd%s/libcsound.a %s", (size==1?"":"64"),b);
        system(s);
      }
    }
    /* Need to setup OPCODEDIR or OPCODEDIR64 */
    /* This differs according to which shell is being used, so for
       bash, sh,  add "OPCODEDIRxx=$prefix/$opcdirl export OPCODEDIRxx"
       csh, tcsh  add "setenv OPCODEDIRxx $prefix/$opcdirl"
    */
    {
      char *shell = getenv("SHELL");
      if (strstr(shell,"csh")!=NULL) { /* CShell and friends */
        char buff[120];
        char temp[32];
        FILE *rc;
        FILE *new;
        char name[32];
        strcpy(name, (single==2 ? "OPCODEDIR64 " : "OPCODEDIR "));
        strcpy(buff, getenv("HOME"));
        strcat(buff, "/.cshrc");
        rc = fopen(buff, "r");
        strcpy(temp, buff); strcat(temp, ".XXXXXX");
        if (mkstemp(temp)== -1) exit(2);
        new = fopen(temp, "w");
        while (!feof(rc)) {
          char b[100];
          fgets(b, 100, rc);
          if (strstr(b, name)!=0) {
            putc('#', new);
          }
          fputs(b, new);
        }
        fprintf(new, "setenv %s %s/%s\n", name, prefix , opcdir);
        fclose(new); fclose(rc); unlink(buff); link(temp, buff);
      }
      else {
        char buff[120];
        char temp[32];
        FILE *rc;
        FILE *new;
        char name[32];
        strcpy(name, (single==2 ? "OPCODEDIR64 " : "OPCODEDIR "));
        strcpy(buff, getenv("HOME"));
        strcat(buff, "/.profile");
        rc = fopen(buff, "r");
        strcpy(temp, buff); strcat(temp, ".XXXXXX");
        if (mkstemp(temp)== -1) exit(2);
        new = fopen(temp, "w");
        while (!feof(rc)) {
          char b[100];
          fgets(b, 100, rc);
          if (strstr(b, name)!=0) {
            putc('#', new);
          }
          fputs(b, new);
        }
        fprintf(new, "%s=%s/%s; export %s\n", name, prefix , opcdir, name);
        fclose(new); fclose(rc); unlink(buff); link(temp, buff);
      }
    }
    /* and check /etc/ld.so.conf. If that is not writable change users LD_PATH */
    /* Also need to check existence of libsndfile and other necessary libraries */

}

