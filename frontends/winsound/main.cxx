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
    return 1;
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
    do_load = 0;
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
    Fl::wait(0);
    if (do_load) {
      char *argv[100];
      char b1[12], b2[12], b3[12], b4[12], b5[12], b6[12], b7[12];
      int nxt=1;
      int itmp;
      char *stmp;
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
      prof.set("c",size_8->value());
      prof.set("s",size_16->value());
      prof.set("l",size_32->value());
      prof.set("f",size_f->value());
      prof.set("3",size_24->value());
      prof.set("K",mK->value());
      argv[0] = "csound5";
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
      prof.get("I",itmp, 0); if (itmp) argv[nxt++] = "-I";
      prof.get("n",itmp, 0); if (itmp) argv[nxt++] = "-n";
      prof.get("F", stmp, ""); if (strlen(stmp)!=0) {
        argv[nxt++] = "-F";
        argv[nxt++] = (char *)mi->value();
      }
      free(stmp);
      prof.get("b",itmp,512); if (itmp!=512) {
        sprintf(b1, "-b%d", itmp);
        argv[nxt++] = b1;
      }
      prof.get("B",itmp,1024); if (itmp!=1024) {
        sprintf(b2, "-B%d", itmp);
        argv[nxt++] = b2;
      }
      if (size_8->value()) argv[nxt++] = "-c";
      else if (size_16->value()) argv[nxt++] = "-s";
      else if (size_32->value()) argv[nxt++] = "-l";
      else if (size_f->value()) argv[nxt++] = "-f";
      else if (size_24->value()) argv[nxt++] = "-3";
      prof.get("r", itmp, -1);if (itmp>=0) {
        sprintf(b3, "-r%d", itmp);
        argv[nxt++] = b3;
      }
      prof.get("k",itmp, -1);if (itmp>=0) {
        sprintf(b4, "-k%d",itmp);
        argv[nxt++] = b4;
      }
      if (mK->value()==0) argv[nxt++] = "-K";
      prof.get("v",itmp,0); if (itmp) argv[nxt++] = "-v";
      prof.get("m",itmp,7);
      sprintf(b5, "-m%d", itmp);
      argv[nxt++] = b5;
      prof.get("t",itmp,-1); if (itmp>=0) {
        sprintf(b6, "-t%d", itmp);
        argv[nxt++] = b6;
      }
      prof.get("t0",itmp, 0); if (itmp)
        argv[nxt++] = "-t0";

      prof.get("M", stmp, ""); if (strlen(stmp)>0) {
        argv[nxt++] = "-M";
        argv[nxt++] = stmp;
      }
      free(stmp);
      prof.get("R",itmp,0); if (itmp) argv[nxt++] = "-R";
      prof.get("H",itmp,0); if (itmp>0) {
        sprintf(b7, "-H%d", itmp);
        argv[nxt++] = b7;
      }
      prof.get("N", itmp, 0); if (itmp) argv[nxt++] = "-N";
      prof.get("Z", itmp, 0); if (itmp) argv[nxt++] = "-Z";
      argv[nxt++] = "-d";       // for the moment
      csoundReset(csound);
//       {
//         int n;
//         printf("nxt=%d\n", nxt);
//         for (n=0; n<nxt; n++) printf("%d: \"%s\"\n", n, argv[n]);
//       }
      res = csoundCompile(csound, nxt, argv);
    }
    else
      csoundRewindScore(csound);
    Fl::wait(0);
    fprintf(stderr, "Starting call\n");
    res = 0;
    csoundSetYieldCallback(csound, yieldCallback);
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

Fl_Preferences prof_h(app, "adsyn");
void cs_util_het(void)
{
    Fl_Double_Window *hw = make_hetanal();
    char *argv[100];
    char buffer[1024];
    char *b = buffer;
    int nxt = 1;
    prof_h.set("s",het_s->value());
    prof_h.set("b",het_b->value());
    prof_h.set("d",het_d->value());
    prof_h.set("f",het_f->value());
    prof_h.set("h",het_h->value());
    prof_h.set("M",het_M->value());
    prof_h.set("n",het_n->value());
    prof_h.set("I",het_I->value());
    prof_h.set("m",het_m->value());
    prof_h.set("input",het_analin->value());
    prof_h.set("output",het_analout->value());
    prof_h.set("c", het_c1->value()?1:
                    het_c2->value()?2:
                    het_c3->value()?3:
                    het_c4->value()?4:0);
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
        sprintf(b, "-d%d", (int)(het_d->value()));
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
