/* Copyright 2006 John ffitch
   This code is NOT released under LGPL or any similar licence
   A licence is hereby granted to anyone to use, copy, or modify
   this code for any purpose whatsoever.
 */
#include "csound.h"
#include "winsound.h"
#ifndef _MSC_VER
# include <unistd.h>
#endif
#include <errno.h>
#include <stdarg.h>
#if defined(WIN32)
# include <direct.h>
#endif
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

#ifdef WIN32
static char* thisprogram;
#endif
char *getopcodedir(void)
{
#ifdef WIN32
    // Ideally this should default to the installation directory at
    // least on windows
    char *name = strrchr(thisprogram, '\\');
    if (name) {
      *name = '\0';
      return name;
    }
#endif
    return getenv("OPCODEDIR");
}

Fl_Preferences app(Fl_Preferences::USER, "csounds.com", "winsound");
Fl_Preferences prof(app, "winsound");
int main(int argc, const char **argv)
{
    Fl_Double_Window* mw = make_mainwindow();
#ifdef WIN32
    thisprogram = strdup(argv[0]);
#endif
    ew = make_environ();
    uw = make_utils();
    xw = make_extras();
    textw = make_textwindow();
    csoundInitialize(CSOUNDINIT_NO_SIGNAL_HANDLER);
    csound = csoundCreate(NULL);
    csoundSetMessageCallback(csound, mytextNull);
    csoundSetMessageCallback(csound, mytextOutput);
    csoundSetYieldCallback(csound, yieldCallback);
    mw->show();
    do_load = 1;
    do_exit = 0;
    while (!do_exit && mw->shown()) {
      do_perf = 0;
      while (!do_exit && !do_perf) Fl::wait();
      if (do_perf) cs_compile_run();
    }
    csoundReset(csound);
}

void cs_compile_run(void)
{
    int res=0;
    textw->show();
    Fl::wait(0);
    csoundSetYieldCallback(csound, yieldCallback);
    if (do_load) {
      char olddir[256];
      const char *argv[100];
      char b1[12], b2[12], b3[12], b4[12], b5[12], b6[12], b7[12];
      int nxt=1;
      int itmp;
      char *stmp;
      do_load = 0;
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
      argv[0] = (char*)"csound6";
      argv[nxt++] = (char *)orchname->value();
      if (strstr(argv[nxt-1], ".csd")==NULL)
        argv[nxt++] = (char *)scorename->value();
      if (strlen(output->value())!=0) {
        argv[nxt++] = (char*)"-o";
        argv[nxt++] = (char *)output->value();
      }
      if (wav->value()) argv[nxt++] = (char*)"-W";
      else if (aiff->value()) argv[nxt++] = (char*)"-A";
      else if (ircam->value()) argv[nxt++] = (char*)"-J";
      else if (raw->value()) argv[nxt++] = (char*)"-h";
      prof.get("I",itmp, 0); if (itmp) argv[nxt++] = (char*)"-I";
      prof.get("n",itmp, 0); if (itmp) argv[nxt++] = (char*)"-n";
      prof.get("F", stmp, ""); if (strlen(stmp)!=0) {
        argv[nxt++] = (char*)"-F";
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
      if (size_8->value()) argv[nxt++] = (char*)"-c";
      else if (size_16->value()) argv[nxt++] = (char*)"-s";
      else if (size_32->value()) argv[nxt++] = (char*)"-l";
      else if (size_f->value()) argv[nxt++] = (char*)"-f";
      else if (size_24->value()) argv[nxt++] = (char*)"-3";
      prof.get("r", itmp, -1);if (itmp>=0) {
        sprintf(b3, "-r%d", itmp);
        argv[nxt++] = b3;
      }
      prof.get("k",itmp, -1);if (itmp>=0) {
        sprintf(b4, "-k%d",itmp);
        argv[nxt++] = b4;
      }
      if (mK->value()==0) argv[nxt++] = (char*)"-K";
      prof.get("v",itmp,0); if (itmp) argv[nxt++] = (char*)"-v";
      prof.get("m",itmp,7);
      sprintf(b5, "-m%d", itmp);
      argv[nxt++] = b5;
      prof.get("t",itmp,-1); if (itmp>=0) {
        sprintf(b6, "-t%d", itmp);
        argv[nxt++] = b6;
      }
      prof.get("t0",itmp, 0); if (itmp)
        argv[nxt++] = (char*)"-t0";

      prof.get("M", stmp, ""); if (strlen(stmp)>0) {
        argv[nxt++] = (char*)"-M";
        argv[nxt++] = stmp;
      }
      free(stmp);
      prof.get("R",itmp,0); if (itmp) argv[nxt++] = (char*)"-R";
      prof.get("H",itmp,0); if (itmp>0) {
        sprintf(b7, "-H%d", itmp);
        argv[nxt++] = b7;
      }
      prof.get("N", itmp, 0); if (itmp) argv[nxt++] = (char*)"-N";
      prof.get("Z", itmp, 0); if (itmp) argv[nxt++] = (char*)"-Z";
   /* argv[nxt++] = "-d"; */    // for the moment
      // If orch name starts with / do a chdir
      if (getcwd(olddir, 255)) {      // remember current directory
        text->insert(strerror(errno));
          text->show_insert_position();
          Fl::wait(0);
      }
      if ((orchname->value())[0]=='/') {
        char dir[256];
        strcpy(dir, orchname->value());
        *(strrchr(dir,'/')) = '\0';
        if (dir[0]=='\0') strcpy(dir, "/"); // if nothing left....
        if (chdir(dir)) {
          text->insert(strerror(errno));
          text->show_insert_position();
          Fl::wait(0);
        }
      }
      else if ((orchname->value())[0]=='\\') {
        char dir[256];
        strcpy(dir, orchname->value());
        *(strrchr(dir,'\\')) = '\0';
        if (dir[0]=='\0') strcpy(dir, "\\"); // if nothing left....
        if (chdir(dir)) {
          text->insert(strerror(errno));
          text->show_insert_position();
          Fl::wait(0);
        }
      };
//       {
//         int n;
//         printf("nxt=%d\n", nxt);
//         for (n=0; n<nxt; n++) printf("%d: \"%s\"\n", n, argv[n]);
//       }

        // set default, but allow to be overridden by .csoundrc or <CsOptions>
#if defined(WIN32)
      csoundParseConfigurationVariable(csound, "rtaudio", "pa_cb");
#elif defined(LINUX)
      csoundParseConfigurationVariable(csound, "rtaudio", "alsa");
#elif defined(OSX)
      csoundParseConfigurationVariable(csound, "rtaudio", "CoreAudio");
#else
#endif
      // disable threading in widgets plugin, and also graphs for safety
      // (the latter is only needed with a static FLTK library)
      csoundCreateGlobalVariable(csound, "FLTK_Flags", sizeof(int));
      *((int*) csoundQueryGlobalVariable(csound, "FLTK_Flags")) = 30;
      res = csoundCompile(csound, nxt, argv);

      if (chdir(olddir)) {
          text->insert(strerror(errno));
          text->show_insert_position();
          Fl::wait(0);
      }
    }
    else
      csoundRewindScore(csound);
    Fl::wait(0);
    while (res == 0 && do_perf) {
      if (do_exit) {
        do_exit = 0;    // allow stopping performance without quitting program
        break;
      }
      res = csoundPerformKsmps(csound);
    }
 // if (res < 0 || res == CSOUND_EXITJMP_SUCCESS) {
      do_load = 1;
      csoundReset(csound);
 // }
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
      argv[0] = (char*)"sndinfo";
      argv[1] = (char *)sndinfo_file->value();
      csoundRunUtility(csound, "sndinfo", 2, argv);
      csoundReset(csound);
    }
    delete siw;
}

void cs_util_opc(int full)
{
    const char *argv[2];
    argv[0] = (char*)"csound";
    if (full) argv[1] = (char*)"-z1";
    else argv[1] = (char*)"-z0";
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
    hw->show();
    while (do_util==0) Fl::wait();
    if (do_util>0) {
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
      textw->show();
      hw->hide();
      argv[0] = (char*)"hetro";
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
      if (het_c1->value()) argv[nxt++] = (char*)"-c1";
      else if (het_c2->value()) argv[nxt++] = (char*)"-c2";
      else if (het_c3->value()) argv[nxt++] = (char*)"-c3";
      else if (het_c4->value()) argv[nxt++] = (char*)"-c4";
      argv[nxt++] = (char *)het_analin->value();
      argv[nxt++] = (char *)het_analout->value();
      csoundRunUtility(csound, "hetro", nxt, argv);
      csoundReset(csound);
    }
    else hw->hide();
    delete hw;
}

Fl_Preferences prof_l(app, "lpc");
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
      prof_l.set("s",lpc_s->value());
      prof_l.set("b",lpc_b->value());
      prof_l.set("d",lpc_d->value());
      prof_l.set("p",lpc_p->value());
      prof_l.set("h",lpc_h->value());
      prof_l.set("P",lpc_P->value());
      prof_l.set("Q",lpc_Q->value());
      prof_l.set("input",lpc_analin->value());
      prof_l.set("output",lpc_analout->value());
      prof_l.set("c", lpc_c1->value()?1:
                 lpc_c2->value()?2:
                 lpc_c3->value()?3:
                 lpc_c4->value()?4:0);
      textw->show();
      hw->hide();
      argv[0] = (char*)"lpanal";
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
      if (lpc_c1->value()) argv[nxt++] = (char*)"-c1";
      else if (lpc_c2->value()) argv[nxt++] = (char*)"-c2";
      else if (lpc_c3->value()) argv[nxt++] = (char*)"-c3";
      else if (lpc_c4->value()) argv[nxt++] = (char*)"-c4";
      argv[nxt++] = (char *)lpc_analin->value();
      argv[nxt++] = (char *)lpc_analout->value();
      csoundRunUtility(csound, "lpanal", nxt, argv);
      csoundReset(csound);
    }
    else hw->hide();
    delete hw;
}

Fl_Preferences prof_p(app, "pvoc");
void cs_util_pvc(void)
{
    Fl_Double_Window *hw = make_pvanal();
    char *argv[100];
    char buffer[1024];
    char *b = buffer;
    int nxt = 1;
    hw->show();
    while (do_util==0) Fl::wait();
    if (do_util>0) {
      prof_p.set("s",pvc_s->value());
      prof_p.set("b",pvc_b->value());
      prof_p.set("d",pvc_d->value());
      prof_p.set("w",pvc_w->value());
      prof_p.set("h",pvc_h->value());
      prof_p.set("H",pvc_H->value());
      prof_p.set("K",pvc_K->value());
      prof_p.set("V",pvc_V->value());
      prof_p.set("input",pvc_analin->value());
      prof_p.set("output",pvc_analout->value());
      prof_p.set("c", pvc_c1->value()?1:
                 pvc_c2->value()?2:
                 pvc_c3->value()?3:
                 pvc_c4->value()?4:0);
      textw->show();
      hw->hide();
      argv[0] = (char*)"pvanal";
      if (pvc_s->value()!=0) {
        sprintf(b, "-s%d", (int)(pvc_s->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (pvc_b->value()!=0) {
        sprintf(b, "-b%d", (int)(pvc_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (pvc_d->value()!=0) {
        sprintf(b, "-d%d", (int)(pvc_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (pvc_w->value()!=0) {
        sprintf(b, "-w%d", (int)(pvc_w->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (pvc_h->value()!=0) {
        sprintf(b, "-h%d", (int)(pvc_h->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (pvc_K->value()!=0) {
        argv[nxt++] = (char*)"-K";
      }
      if (pvc_H->value()!=0) {
        argv[nxt++] = (char*)"H";
      }
      if (pvc_c1->value()) argv[nxt++] = (char*)"-c1";
      else if (pvc_c2->value()) argv[nxt++] = (char*)"-c2";
      else if (pvc_c3->value()) argv[nxt++] = (char*)"-c3";
      else if (pvc_c4->value()) argv[nxt++] = (char*)"-c4";
      argv[nxt++] = (char *)pvc_analin->value();
      argv[nxt++] = (char *)pvc_analout->value();
      csoundRunUtility(csound, "pvanal", nxt, argv);
      csoundReset(csound);
    }
    else hw->hide();
    delete hw;
}

Fl_Preferences prof_c(app, "cvanal");
void cs_util_cvl(void)
{
    Fl_Double_Window *hw = make_cvlanal();
    char *argv[100];
    char buffer[1024];
    char *b = buffer;
    int nxt = 1;
    hw->show();
    while (do_util==0) Fl::wait();
    if (do_util>0) {
      prof_c.set("s",cvl_s->value());
      prof_c.set("b",cvl_b->value());
      prof_c.set("d",cvl_d->value());
      prof_c.set("input",cvl_analin->value());
      prof_c.set("output",cvl_analout->value());
      prof_c.set("c", cvl_c1->value()?1:
                 cvl_c2->value()?2:
                 cvl_c3->value()?3:
                 cvl_c4->value()?4:0);
      textw->show();
      hw->hide();
      argv[0] = (char*)"cvanal";
      if (cvl_s->value()!=0) {
        sprintf(b, "-s%d", (int)(cvl_s->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (cvl_b->value()!=0) {
        sprintf(b, "-b%d", (int)(cvl_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (cvl_d->value()!=0) {
        sprintf(b, "-d%d", (int)(cvl_d->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (cvl_c1->value()) argv[nxt++] = (char*)"-c1";
      else if (cvl_c2->value()) argv[nxt++] = (char*)"-c2";
      else if (cvl_c3->value()) argv[nxt++] = (char*)"-c3";
      else if (cvl_c4->value()) argv[nxt++] = (char*)"-c4";
      argv[nxt++] = (char *)cvl_analin->value();
      argv[nxt++] = (char *)cvl_analout->value();
      csoundRunUtility(csound, "cvanal", nxt, argv);
      csoundReset(csound);
    }
    else hw->hide();
    delete hw;
}

Fl_Preferences prof_k(app, "pvlook");
void cs_util_pinfo(void)
{
    Fl_Double_Window *hw = make_pvlook();
    char *argv[100];
    char buffer[1024];
    char *b = buffer;
    int nxt = 1;
    hw->show();
    while (do_util==0) Fl::wait();
    if (do_util>0) {
      prof_k.set("bb",plk_bb->value());
      prof_k.set("eb",plk_eb->value());
      prof_k.set("bf",plk_bf->value());
      prof_k.set("ef",plk_ef->value());
      prof_k.set("input",plk_analin->value());
      prof_k.set("i", plk_i->value());
      textw->show();
      hw->hide();
      argv[0] = (char*)"pvlook";
      if (plk_bb->value()!=0) {
        sprintf(b, "-bb%d", (int)(plk_bb->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (plk_eb->value()!=0) {
        sprintf(b, "-eb%d", (int)(plk_eb->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (plk_bf->value()!=0) {
        sprintf(b, "-bf%d", (int)(plk_bf->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (plk_ef->value()!=0) {
        sprintf(b, "-ef%d", (int)(plk_ef->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (plk_i->value()) argv[nxt++] = (char*)"-i";
      argv[nxt++] = (char *)plk_analin->value();
      csoundRunUtility(csound, "pvlook", nxt, argv);
      csoundReset(csound);
    }
    else hw->hide();
    delete hw;
}

Fl_Preferences prof_n(app, "cvanal");
void cs_util_dnoise(void)
{
    Fl_Double_Window *hw = make_dnoise();
    char *argv[100];
    char buffer[1024];
    char *b = buffer;
    int nxt = 1;
    hw->show();
    while (do_util==0) Fl::wait();
    if (do_util>0) {
      prof_n.set("t", dns_t->value());
      prof_n.set("S", dns_S->value());
      prof_n.set("m", dns_m->value());
      prof_n.set("n", dns_n->value());
      prof_n.set("b", dns_b->value());
      prof_n.set("e", dns_e->value());
      prof_n.set("N", dns_N->value());
      prof_n.set("M", dns_M->value());
      prof_n.set("L", dns_L->value());
      prof_n.set("w", dns_w->value());
      prof_n.set("D", dns_D->value());
      prof_n.set("V", dns_V->value());
      prof_n.set("input",dns_analin->value());
      prof_n.set("output",dns_analout->value());
      prof_n.set("noise",dns_noise->value());
      textw->show();
      hw->hide();
      argv[0] = (char*)"dnoise";
      if (dns_t->value()!=30) {
        sprintf(b, "-t%d", (int)(dns_t->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_S->value()!=0) {
        sprintf(b, "-S%d", (int)(dns_S->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_m->value()!=0) {
        sprintf(b, "-m%d", (int)(dns_m->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_n->value()!=5) {
        sprintf(b, "-n%d", (int)(dns_n->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_b->value()!=0) {
        sprintf(b, "-b%d", (int)(dns_b->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_e->value()!=0) {
        sprintf(b, "-e%d", (int)(dns_e->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_w->value()>=0) {
        sprintf(b, "-w%d", (int)(dns_w->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      else if (dns_M->value()!=0) {
        sprintf(b, "-M%d", (int)(dns_M->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_N->value()!=0) {
        sprintf(b, "-N%d", (int)(dns_N->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_L->value()!=0) {
        sprintf(b, "-L%d", (int)(dns_L->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_D->value()!=0) {
        sprintf(b, "-D%d", (int)(dns_D->value()));
        argv[nxt++] = b;
        b += strlen(b)+1;
      }
      if (dns_V->value()) {
        argv[nxt++] = (char*)"-V";
      }
      argv[nxt++] = (char*)"-o"; argv[nxt++] = (char *)dns_analout->value();
      argv[nxt++] = (char*)"-i"; argv[nxt++] = (char *)dns_noise->value();
      argv[nxt++] = (char *)dns_analin->value();
      csoundRunUtility(csound, "dnoise", nxt, argv);
      csoundReset(csound);
    }
    else hw->hide();
    delete hw;
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
  delete hw;
}
