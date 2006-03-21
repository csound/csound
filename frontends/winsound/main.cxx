#include "csound.h"
#include "winsound.h"
extern int do_exit;
extern Fl_Double_Window *ew, *xw, *uw;
CSOUND *csound;
int cs_compile_run(void);

int main(int argc, char **argv)
{
    Fl_Double_Window* mw = make_mainwindow();
    ew = make_environ();
    uw = make_utils();
    xw = make_extras();
    csoundInitialize(&argc, &argv, CSOUNDINIT_NO_SIGNAL_HANDLER);
    csound = csoundCreate(NULL);
    csoundPreCompile(csound);
    mw->show();
    do_exit = 0;
    while (!do_exit) {
      do_perf = 0;
      while (!do_exit && !do_perf) Fl::wait();
      if (do_perf) cs_compile_run();
    }
}

int cs_compile_run(void)
{
    int res=0;

    if (do_load) {
      int argc = 1;
      char *argv[100];
      char b1[6], b2[6], b3[6], b4[6], b5[6], b6[6], b7[6];
      int nxt=1;
      int n;

      argv[0] = "winsound5";
      argv[nxt++] = (char *)orchname->value();
      if (strstr(argv[nxt-1], ".csd")==NULL) argv[nxt++] = (char *)scorename->value();
      if (strlen(output->value())!=0) {
        argv[nxt++] = "-o";
        argv[nxt++] = (char *)output->value();
      }
      if (wav->value()) argv[nxt++] = "-W";
      else if (aiff->value()) argv[nxt++] = "-A";
      else if (ircam->value()) argv[nxt++] = "-J";
      else if (raw->value()) argv[nxt++] = "-h";
      if (mI->value()) argv[nxt++] = "-I";
      if (mn->value()) argv[nxt++] = "-n";
      if (strlen(mi->value())!=0) {
        argv[nxt++] = "-F";
        argv[nxt++] = (char *)mi->value();
      }
      if (mb->value()>0) {
        sprintf(b1, "-b%d", (int)mb->value());
        argv[nxt++] = b1;
      }
      if (mB->value()>0) {
        sprintf(b2, "-B%d", (int)mb->value());
        argv[nxt++] = b2;
      }
      if (size_8->value()) argv[nxt++] = "-c";
      else if (size_16->value()) argv[nxt++] = "-s";
      else if (size_32->value()) argv[nxt++] = "-l";
      else if (size_f->value()) argv[nxt++] = "-f";
      else if (size_24->value()) argv[nxt++] = "-3";
      if (mr->value()>0) {
        sprintf(b3, "-r%d", (int)mr->value());
        argv[nxt++] = b3;
      }
      if (mk->value()>0) {
        sprintf(b4, "-k%d",(int)mk->value());
        argv[nxt++] = b4;
      }
      if (mK->value()==0) argv[nxt++] = "-K";
      if (mv->value())  argv[nxt++] = "-v";
      if (mm->value()>0) {
        sprintf(b5, "-m%d", (int)mm->value());
        argv[nxt++] = b5;
      }
      if (mt->value()>0) {
        sprintf(b6, "-t%d", (int)mt->value());
        argv[nxt++] = b6;
      }
      if (mSave->value()) 
        argv[nxt++] = "-t0";
      
      if (strlen(mM->value())>0) {
        argv[nxt++] = "-M";
        argv[nxt++] = (char *)mM->value();
      }
      if (mR->value()) argv[nxt++] = "-R";
      if (mH->value()>0) {
        sprintf(b7, "-H%d", (int)mH->value());
        argv[nxt++] = b7;
      }
      if (mN->value()) argv[nxt++] = "-N";
      if (mZ->value()) argv[nxt++] = "-Z";

      for (n=1; n<nxt; n++)
        printf("arg %d: %s\n", n, argv[n]);

      csoundReset(csound);
      res = csoundCompile(csound, nxt-1, argv);
    }
    else 
      csoundRewindScore(csound);

    if (res==0) csoundPerform(csound);
    csoundCleanup(csound);
}

void cs_util_sndinfo(void)
{
    Fl_Double_Window *siw = make_info();
    char *argv[2];
    siw->show();
    while (do_util==0) Fl::wait();
    siw->hide();
    if (do_util>0) {
      argv[0] = "sndinfo";
      argv[1] = (char *)sndinfo_file->value();
      csoundPreCompile(csound);
      csoundRunUtility(csound, "sndinfo", 2, argv);
      csoundReset(csound);
    }
}
