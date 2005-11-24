#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int do_install = 0;
int end_alert = 0;

char type[256];
char bin[256];
char opc[256];
char doh[256];
char lib[256];
char envy[256];

#include "installer.cxx"

void set_system(Fl_Check_Button*, void*)
{
    bindir->value(bin);
    opcdir->value(opc);
    doc->value(doh);
    libdir->value(lib);
}

Fl_Double_Window* err;

void do_alert(char *msg)
{
      err_text->value(msg);
      end_alert = 0;
      err->show();
      while (end_alert==0) Fl::wait();
      err->hide();
}


/* Unsure that dir exists with all directories on the way. */
void check_exists(const char *dir)
{
    char test[80];
    char *p;
    int ans;
    struct stat buf;
    strcpy(test,dir);
    printf("Checking %s\n", dir);
    p = test;
    while ((p = strchr(p+1, '/')) != NULL) {
      *p = '\0';
      if (test[0]=='\0') break; // Should not happen
      printf("..Checking %s\n", test);
      ans = stat(test, &buf);
      if (ans!=0 || !S_ISDIR(buf.st_mode)) {
        if (ans!=0) {
          printf("Directory %s does not exist; creating...\n", test);
          mkdir(test, 0766);
        }
        else {
          do_alert("Trouble with file; stopping");
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

int main(void)
{
    FILE *defs = fopen("def.ins", "r");
    Fl_Double_Window* www;
    if (defs==0) exit(1);
    fgets(type,256,defs);
    *(strchr(type,'\n'))= '\0';
    fgets(bin,256,defs);
    *(strchr(bin,'\n'))= '\0';
    fgets(opc,256,defs);
    *(strchr(opc,'\n'))= '\0';
    fgets(doh,256,defs);
    *(strchr(doh,'\n'))= '\0';
    fgets(lib,256,defs);
    *(strchr(lib,'\n'))= '\0';
    fgets(envy,256,defs);
    *(strchr(envy,'\n'))= '\0';
    www = make_window(type);
    www->show();
    err = make_alert();
 again:
    while (do_install==0) Fl::wait(1.0);
    // Check that install is correct
    if (strlen(bindir->value())==0) {
      do_alert("No binary directory");
      goto again;
    }
    if (strlen(opcdir->value())==0) {
      do_alert("No opcode directory");
      goto again;
    }
    //Copy binaries
    {
      struct dirent **namelist;
      char b[256];
      int n = scandir("./bin", &namelist, NULL, alphasort);
      int i;
      progress->label("binaries");
      strcpy(b, bindir->value());
      if (b[strlen(b)-1]!='/')
        strcat(b, "/");
      check_exists(b);
      progress->value(0.0f);
      for (i=0; i<n; i++) {
        char buff[256];
        sprintf(buff,"cp -pv ./bin/%s %s", namelist[i]->d_name,b);
        system(buff);
        progress->value((float)(i+1)/(float)(n));
      }
    }
    //Copy opcodes
    {
      struct dirent **namelist;
      char b[256];
      int n = scandir("./opc", &namelist, NULL, alphasort);
      int i;
      progress->label("opcodes");
      strcpy(b, opcdir->value());
      if (b[strlen(b)-1]!='/')
        strcat(b, "/");
      check_exists(b);
      progress->value(0.0f);
      for (i=0; i<n; i++) {
        char buff[256];
        sprintf(buff,"cp -pv ./opc/%s %s", namelist[i]->d_name,b);
        system(buff);
        progress->value((float)(i+1)/(float)(n));
      }
    }
    if (strlen(doc->value())!=0) {
      struct dirent **namelist;
      char b[256];
      int n = scandir("./html", &namelist, NULL, alphasort);
      int i;
      progress->label("documentation");
      strcpy(b, doc->value());
      check_exists(b);
      if (b[strlen(b)-1]!='/')
        strcat(b, "/");
      check_exists(b);
      check_exists(doc->value());
      progress->value(0.0f);
      for (i=0; i<n; i++) {
        char buff[256];
        Fl::wait(0.2);
        sprintf(buff,"cp -pv ./html/%s %s", namelist[i]->d_name,b);
        system(buff);
        progress->value((float)(i+1)/(float)(n));
        Fl::wait(0.2);
      }
    }
    if (strlen(libdir->value())!=0) {
      struct dirent **namelist;
      char b[256];
      int n = scandir("./lib", &namelist, NULL, alphasort);
      int i;
      progress->label("libraries");
      strcpy(b, libdir->value());
      if (b[strlen(b)-1]!='/')
        strcat(b, "/");
      check_exists(b);
      progress->value(0.0f);
      for (i=0; i<n; i++) {
        char buff[256];
        Fl::wait(0.2);
        sprintf(buff,"cp -pv ./lib/%s %s", namelist[i]->d_name,b);
        system(buff);
        progress->value((float)(i+1)/(float)(n));
        Fl::wait(0.2);
      }
    }
    if (profile->value()) {
      /* Need to setup OPCODEDIR or OPCODEDIR64 */
      /* This differs according to which shell is being used, so for
         bash/sh add to .profile "OPCODEDIRxx=yyy; export OPCODEDIRxx"
         csh/tcsh add to .cshrc "setenv OPCODEDIRxx yyyy"
    */
      char *myshell = getenv("SHELL");
      Fl::wait(0.2);
      if (cshell->value() ||
          (!shell->value() &&
           strstr(myshell,"csh")!=NULL)) { /* CShell and friends */
        char buff[120];
        char temp[32];
        FILE *rc;
        FILE *nw;
        strcpy(buff, getenv("HOME"));
        strcat(buff, "/.cshrc");
        rc = fopen(buff, "r");
        strcpy(temp, buff); strcat(temp, ".XXXXXX");
        if (mkstemp(temp)== -1) exit(2);
        nw = fopen(temp, "w");
        while (!feof(rc)) {
          char b[100];
          Fl::wait(0.1);
          fgets(b, 100, rc);
          if (strstr(b, envy)!=0) {
            putc('#', nw);
          }
          fputs(b, nw);
        }
        fprintf(nw, "setenv %s \n", envy, opcdir->value());
        fclose(nw); fclose(rc); unlink(buff); link(temp, buff);
      }
      else {
        char buff[120];
        char temp[32];
        FILE *rc;
        FILE *nw;
        strcpy(buff, getenv("HOME"));
        strcat(buff, "/.profile");
        rc = fopen(buff, "r");
        strcpy(temp, buff); strcat(temp, ".XXXXXX");
        if (mkstemp(temp)== -1) exit(2);
        nw = fopen(temp, "w");
        while (!feof(rc)) {
          char b[100];
          fgets(b, 100, rc);
          if (strstr(b, envy)!=0) {
            putc('#', nw);
          }
          fputs(b, nw);
        }
        fprintf(nw, "%s=%s; export %s\n", envy, opcdir->value(), envy);
        fclose(nw); fclose(rc); unlink(buff); link(temp, buff);
      }
    }
    err->color(FL_GREEN);
    do_alert("Installation finished");
}
