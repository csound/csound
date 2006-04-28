/* Copyright 2006 John ffitch
   This code is NOT released under LGPL or any similar licence
   A licence is hereby granted to anyone to use, copy, or modify
   this code for any purpose whatsoever.
 */
#include "csound.h"
#include "winsound.h"
#include <stdarg.h>
#include <FL/Fl_Preferences.H>
extern int do_exit;
extern Fl_Double_Window *ew, *xw, *uw, *textw;
CSOUND *csound;
void cs_compile_run(void);

void mytextOutput(CSOUND *csound, int attr, const char *format, va_list valist)
{
    char b[2048];
    vsprintf(b, format, valist);
    text->insert(b);
    text->show_insert_position();
    Fl::wait(0);
}

void mytextNull(CSOUND *csound, int attr, const char *format, va_list valist)
{
    // Quieten messages for a while
}

int yieldCallback(CSOUND *csound)
{
    Fl::wait(0);
    return 0;
}

Fl_Preferences app(Fl_Preferences::USER, "csounds.com", "winsound");
Fl_Preferences prof(app, "winsound");
int main(int argc, char **argv)
{
    Fl_Double_Window* mw = make_mainwindow();

    ew = make_environ();
    uw = make_utils();
    xw = make_extras();
    textw = make_textwindow();
    csoundInitialize(&argc, &argv, CSOUNDINIT_NO_SIGNAL_HANDLER);
    csound = csoundCreate(NULL);
    csoundSetMessageCallback(csound, mytextNull);
    csoundPreCompile(csound);
    csoundSetMessageCallback(csound, mytextOutput);
    csoundSetYieldCallback(csound, yieldCallback);
    mw->show();
    do_exit = 0;
    while (!do_exit) {
      do_perf = 0;
      while (!do_exit && !do_perf) Fl::wait();
      if (do_perf) cs_compile_run();
    }
    csoundCleanup(csound);
}

void cs_compile_run(void)
{
    int res=0;
    textw->show();
    if (do_load) {
      char *argv[100];
      char b1[12], b2[12], b3[12], b4[12], b5[12], b6[12], b7[12];
      int nxt=1;

      /* Remember profile */
      prof.set("orchestra", orchname->value());
      prof.set("score", scorename->value());
      prof.set("output", output->value());
      prof.set("W",wav->value());
      prof.set("A",aiff->value());
      prof.set("J",ircam->value());
      prof.set("h",raw->value());
      prof.set("I",mI->value());
      prof.set("F",mi->value());
      prof.set("n",mn->value());
      prof.set("b",mb->value());
      prof.set("B",mB->value());
      prof.set("c",size_8->value());
      prof.set("s",size_16->value());
      prof.set("l",size_32->value());
      prof.set("f",size_f->value());
      prof.set("3",size_24->value());
      prof.set("r",mr->value());
      prof.set("k",mk->value());
      prof.set("K",mK->value());
      prof.set("v",mv->value());
      prof.set("m",mm->value());
      prof.set("t",mt->value());
      prof.set("t0",mSave->value());
      prof.set("M",mM->value());
      prof.set("R",mR->value());
      prof.set("H",mH->value());
      prof.set("N",mN->value());
      prof.set("Z",mZ->value());
      argv[0] = "winsound5";
      argv[nxt++] = (char *)orchname->value();
      if (strstr(argv[nxt-1], ".csd")==NULL)
        argv[nxt++] = (char *)scorename->value();
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
        sprintf(b2, "-B%d", (int)mB->value());
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

//       for (n=1; n<nxt; n++)
//         printf("arg %d: %s\n", n, argv[n]);

      csoundReset(csound);
      res = csoundCompile(csound, nxt-1, argv);
    }
    else
      csoundRewindScore(csound);

    while (res==0 && do_perf) {
      if (do_exit) return;
      res = csoundPerformKsmps(csound);
    }
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
      textw->show();
      argv[0] = "sndinfo";
      argv[1] = (char *)sndinfo_file->value();
      csoundPreCompile(csound);
      csoundRunUtility(csound, "sndinfo", 2, argv);
      csoundReset(csound);
    }
}

void cs_util_opc(int full)
{
    char *argv[2];
    argv[0] = "csound";
    if (full) argv[1] = "-z1";
    else argv[1] = "-z0";
    textw->show();
    csoundCompile(csound, 2, argv);
    csoundCleanup(csound);

//     csoundPreCompile(csound);
//     csoundLoadExternals(csound);
//     if (csoundInitModules(csound) == 0)
//       list_opcodes(csound, full);
//     csoundReset(csound);
}

void cs_util_het(void)
{
    Fl_Double_Window *hw = make_hetanal();
    char *argv[100];
    char buffer[1024];
    char *b = buffer;
    int nxt = 1;
    hw->show();
    while (do_util==0) Fl::wait();
    if (do_util>0) {
      textw->show();
      hw->hide();
      argv[0] = "hetro";
      if (het_s->value()!=0) {
        sprintf(b, "-s%d", (int)(het_s->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_b->value()!=0) {
        sprintf(b, "-b%d", (int)(het_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_d->value()!=0) {
        sprintf(b, "-d%d", (int)(het_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_f->value()!=100) {
        sprintf(b, "-f%d", (int)(het_f->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_h->value()!=10) {
        sprintf(b, "-h%d", (int)(het_h->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_M->value()!=32767) {
        sprintf(b, "-M%d", (int)(het_M->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_n->value()!=256) {
        sprintf(b, "-n%d", (int)(het_n->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_I->value()!=0) {
        sprintf(b, "-I%d", (int)(het_I->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_m->value()!=64) {
        sprintf(b, "-m%d", (int)(het_m->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (het_c1->value()) argv[nxt++] = "-c1";
      else if (het_c2->value()) argv[nxt++] = "-c2";
      else if (het_c3->value()) argv[nxt++] = "-c3";
      else if (het_c4->value()) argv[nxt++] = "-c4";
      argv[nxt++] = (char *)het_analin->value();
      argv[nxt++] = (char *)het_analout->value();
      csoundPreCompile(csound);
      csoundRunUtility(csound, "hetro", nxt, argv);
      csoundReset(csound);
    }
}

void cs_util_lpc(void)
{
    Fl_Double_Window *hw = make_lpcanal();
    char *argv[100];
    char buffer[1024];
    char *b = buffer;
    int nxt = 1;
    hw->show();
    while (do_util==0) Fl::wait();
    if (do_util>0) {
      textw->show();
      hw->hide();
      argv[0] = "lpanal";
      if (lpc_s->value()!=0) {
        sprintf(b, "-s%d", (int)(lpc_s->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (lpc_b->value()!=0) {
        sprintf(b, "-b%d", (int)(lpc_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (lpc_d->value()!=0) {
        sprintf(b, "-d%d", (int)(lpc_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (lpc_p->value()!=34) {
        sprintf(b, "-p%d", (int)(lpc_p->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (lpc_h->value()!=200) {
        sprintf(b, "-h%d", (int)(lpc_h->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (lpc_P->value()!=70) {
        sprintf(b, "-P%d", (int)(lpc_P->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (lpc_Q->value()!=200) {
        sprintf(b, "-Q%d", (int)(lpc_Q->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (lpc_c1->value()) argv[nxt++] = "-c1";
      else if (lpc_c2->value()) argv[nxt++] = "-c2";
      else if (lpc_c3->value()) argv[nxt++] = "-c3";
      else if (lpc_c4->value()) argv[nxt++] = "-c4";
      argv[nxt++] = (char *)lpc_analin->value();
      argv[nxt++] = (char *)lpc_analout->value();
      csoundPreCompile(csound);
      csoundRunUtility(csound, "lpanal", nxt, argv);
      csoundReset(csound);
    }
}

void cs_util_pvc(void)
{
    textw->show();
    csoundMessage(csound, "***PVOC analysis not yet written***\n");
}

void cs_util_cvl(void)
{
    textw->show();
    csoundMessage(csound, "***Convolution analysis not yet written***\n");
}

void cs_util_pinfo(void)
{
    textw->show();
    csoundMessage(csound, "***PVOV info not yet written***\n");
}

void cs_util_dnoise(void)
{
    textw->show();
    csoundMessage(csound, "***DNoise not yet written***\n");
}

void savetext(Fl_Text_Buffer *b, int type)
{
  Fl_Double_Window *hw = make_saver();
  do_util = 0;
  hw->show();
  while (do_util==0) Fl::wait();
  hw->hide();
  if (do_util>0) {
    if (type==0) b->outputfile(savefile->value(), 0, b->length());
    else {
      int start,end;
      b->selection_position(&start, &end); 
      b->outputfile(savefile->value(), start, end);
    }    
  }
}
