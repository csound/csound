#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int do_install = 1;
int end_alert = 1;

char type[256];
char bin[256];
char opc[256];
char doh[256];
char lib[256];

#include "installer.cxx"

void set_system(Fl_Check_Button*, void*)
{
    bindir->value(bin);
    opcdir->value(opc);
    doc->value(doh);
    libdir->value(lib);
}

Fl_Double_Window* err;

/* Unsure that dir exists with all directories on the way. */
void check_exists(const char *dir)
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
          //          printf("Directory %s does not exist; creating...\n", test);
          mkdir(test, 0766);
        }
        else {
          err_text->value("Trouble with file; stopping");
          err->show();
          while (end_alert==0) Fl::wait(1);
          exit(1);
        }
      }
      else {
        //       printf("Directory %s OK\n", test);
      }
      *p = '/';
    }
    return;
}

int main(void)
{
    FILE *defs = fopen("defs.ins", "r");
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
    www = make_window(type);
    www->show();
    err = make_alert();
 again:
    while (do_install==0) Fl::wait(1);
    // Check that install is correct
    if (strlen(bindir->value())==0) {
      err_text->value("No binary directory");
      err->show();
      while (end_alert==0) Fl::wait(1);
      err->hide();
      goto again;
    }
    if (strlen(opcdir->value())==0) {
      err_text->value("No opcode directory");
      err->show();
      while (end_alert==0) Fl::wait(1);
      err->hide();
      goto again;
    }
    //Copy binaries
    {
      struct dirent **namelist;
      int n = scandir("./bin", &namelist, NULL, alphasort);
      int i;
      progress->label("binaries");
      check_exists(bindir->value());
      progress->value(0.0f);
      for (i=0; i<n; i++) {
        char buff[256];
        sprintf(buff,"cp -pv %s %s", namelist[i]->d_name,bindir->value());
        progress->value((float)(i+1)/(float)(n));
      }
    }
    //Copy opcodes
    {
      struct dirent **namelist;
      int n = scandir("./opc", &namelist, NULL, alphasort);
      int i;
      progress->label("opcodes");
      check_exists(opcdir->value());
      progress->value(0.0f);
      for (i=0; i<n; i++) {
        char buff[256];
        sprintf(buff,"cp -pv %s %s", namelist[i]->d_name,opcdir->value());
        progress->value((float)(i+1)/(float)(n));
      }
    }
    if (strlen(doc->value())!=0) {
      struct dirent **namelist;
      int n = scandir("./html", &namelist, NULL, alphasort);
      int i;
      progress->label("documentation");
      check_exists(doc->value());
      progress->value(0.0f);
      for (i=0; i<n; i++) {
        char buff[256];
        sprintf(buff,"cp -pv %s %s", namelist[i]->d_name,opcdir->value());
        progress->value((float)(i+1)/(float)(n));
      }
    }
}
