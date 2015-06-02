/*
    control.c:

    Copyright (C) 2000 John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"
#include "control.h"

#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

#if defined(__MACH__)
#include <unistd.h>
#endif

static CS_NOINLINE CONTROL_GLOBALS *get_globals_(CSOUND *csound)
{
    CONTROL_GLOBALS *p;
    p = (CONTROL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                       "controlGlobals_");
    if (p != NULL)
      return p;
    if (csound->CreateGlobalVariable(csound, "controlGlobals_",
                                     sizeof(CONTROL_GLOBALS)) != 0){
      csound->Warning(csound, Str("control: failed to allocate globals"));
      return NULL;
    }
    p = (CONTROL_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                       "controlGlobals_");
    p->csound = csound;
    return p;
}

static inline CONTROL_GLOBALS *get_globals(CSOUND *csound, CONTROL_GLOBALS **p)
{
    if (*p == NULL)
      *p = get_globals_(csound);
    return (*p);
}

static int kill_wish(CSOUND *csound, CONTROL_GLOBALS *p)
{
    csound->Message(csound, Str("Closing down wish(%d)\n"), p->wish_pid);
    kill(p->wish_pid, 9);
    if (p->values != NULL)  free(p->values);
    if (p->minvals != NULL) free(p->minvals);
    if (p->maxvals != NULL) free(p->maxvals);
    if (p->buttons != NULL) free(p->buttons);
    if (p->checks != NULL)  free(p->checks);
    fclose(p->wish_cmd);
    fclose(p->wish_res);
    return OK;
}

static void start_tcl_tk(CONTROL_GLOBALS *p)
{
    int i;

    p->csound->Message(p->csound, "TCL/Tk\n");
    if (UNLIKELY(pipe(p->pip1) || pipe(p->pip2))) {
      printf("Failed to create pipes");
      return;
    }
    if ((p->wish_pid = fork()) < 0)
      return;
    if (p->wish_pid == 0) {     /* Child process */
      char *argv[7];
      argv[0] = "sh";
      argv[1] = "-c";
      argv[2] = "wish";
      argv[3] = "-name";
      argv[4] = "sliders";
      argv[5] = NULL;
      close(p->pip1[0]); close(p->pip2[1]);
      close(0); close(1);
      dup2(p->pip2[0], 0);
      dup2(p->pip1[1], 1);
      setvbuf(stdout, (char*) NULL, _IOLBF, 0);
      signal(SIGINT, SIG_IGN);  /* child process should ignore ^C */
      execvp("/bin/sh", argv);
      exit(127);
    }
                                /* Main process -- create communications */
    close(p->pip1[1]); close(p->pip2[0]);
    p->wish_res = fdopen(p->pip1[0], "r");
    p->wish_cmd = fdopen(p->pip2[1], "w");
    setvbuf(p->wish_cmd, (char*) NULL, _IOLBF, 0);
    setvbuf(p->wish_res, (char*) NULL, _IOLBF, 0);
    p->csound->RegisterResetCallback(p->csound, (void*) p,
                                     (int (*)(CSOUND *, void *)) kill_wish);
    fprintf(p->wish_cmd, "source nsliders.tk\n");
    if (UNLIKELY(NULL==fgets(p->cmd, 100, p->wish_res))) {
      printf("Failed to read from child");
      return;
    };
    p->csound->Message(p->csound, "Wish %s\n", p->cmd);
    p->values = (int*) calloc(8, sizeof(int));
    p->minvals = (int*) calloc(8, sizeof(int));
    p->maxvals = (int*) calloc(8, sizeof(int));
    p->buttons = (int*) calloc(8, sizeof(int));
    p->checks  = (int*) calloc(8, sizeof(int));
    p->max_sliders = 8;
    p->max_button = 8;
    p->max_check = 8;
    for (i = 0; i < p->max_sliders; i++) {
      p->minvals[i] = 0; p->maxvals[i] = 127;
    }
    p->csound->Sleep(1500);
}

static void ensure_slider(CONTROL_GLOBALS *p, int n)
{
/*  p->csound->Message(p->csound, "Ensure_slider %d\n", n); */
    if (p->wish_pid == 0)
      start_tcl_tk(p);
    if (n > p->max_sliders) {
      int i, nn = n + 1;
      p->values  = (int*) realloc(p->values, nn * sizeof(int));
      p->minvals = (int*) realloc(p->minvals, nn * sizeof(int));
      p->maxvals = (int*) realloc(p->maxvals, nn * sizeof(int));
      for (i = p->max_sliders + 1; i < nn; i++) {
        p->values[i] = 0; p->minvals[i] = 0; p->maxvals[i] = 127;
      }
      p->max_sliders = n;
    }
/*  p->csound->Message(p->csound, "displayslider %d\n", n); */
    fprintf(p->wish_cmd, "displayslider %d\n", n);
}

static void readvalues(CONTROL_GLOBALS *p)
{
    fd_set rfds;
    struct timeval tv;

    /* Watch wish_res to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(p->pip1[0], &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
                                /* Read all changes */
    while (select(p->pip1[0] + 1, &rfds, NULL, NULL, &tv)) {
      int n, val;
      if (UNLIKELY(2!=fscanf(p->wish_res, "%d %d", &n, &val))) {
        printf("Failed to read from child");
        return;
      }
      if (n>p->max_sliders);    /* ignore error case */
      else if (n > 0) p->values[n] = val;
      else if (n == 0) p->buttons[val] = 1;
      else p->checks[-n] = val;
      tv.tv_sec = 0;
      tv.tv_usec = 0;
    }
}

static int cntrl_set(CSOUND *csound, CNTRL *p)
{
    ensure_slider(get_globals(csound, &(p->p)), (int) MYFLT2LONG(*p->kcntl));
    return OK;
}

static int control(CSOUND *csound, CNTRL *p)
{
    CONTROL_GLOBALS *pp = get_globals(csound, &(p->p));
    readvalues(pp);
    *p->kdest = pp->values[(int)MYFLT2LONG(*p->kcntl)];
    return OK;
}

static int ocontrol_(CSOUND *csound, SCNTRL *p, int istring)
{
    CONTROL_GLOBALS *pp = get_globals(csound, &(p->p));
    int c = (int) *p->which;
    int slider = (int) MYFLT2LONG(*p->kcntl);

/*  csound->Message(csound, "ocontrol: %d %d %f\n", slider, c, *p->val); */
    ensure_slider(pp, slider);
    switch (c) {
    case 1:
      fprintf(pp->wish_cmd, "setvalue %d %d\n", slider, (int) *p->val);
      pp->values[slider] = (int) *p->val;
      break;
    case 2:
      if (pp->minvals[slider] != (int) *p->val) {
        fprintf(pp->wish_cmd, "setmin %d %d\n", slider, (int) *p->val);
        pp->minvals[slider] = (int) *p->val;
      }
      break;
    case 3:
      if (pp->maxvals[slider] != (int) *p->val) {
        fprintf(pp->wish_cmd, "setmax %d %d\n", slider, (int) *p->val);
        pp->maxvals[slider] = (int) *p->val;
      }
      break;
    case 4:
      {
        char buffer[100];
        if(istring) {
          csound->strarg2name(csound, buffer,
                              ((STRINGDAT *)p->val)->data, "Control ",istring);
        }
        else
         csound->strarg2name(csound, buffer, p->val, "Control ",istring);
        csound->Message(csound, Str("Slider %d set to %s\n"), slider, buffer);
        fprintf(pp->wish_cmd, "setlab %d \"%s\"\n", slider, buffer);
        break;
      }
    default:
      return csound->InitError(csound, Str("Unknown control %d"), c);
    }
    return OK;
}

static int ocontrol(CSOUND *csound, SCNTRL *p){
  return ocontrol_(csound,p,0);
}

static int ocontrol_S(CSOUND *csound, SCNTRL *p){
  return ocontrol_(csound,p,1);
}

static int button_set(CSOUND *csound, CNTRL *p)
{
    CONTROL_GLOBALS *pp = get_globals(csound, &(p->p));
    int n = (int) MYFLT2LONG(*p->kcntl);

    if (pp->wish_pid == 0)
      start_tcl_tk(pp);
    if (n > pp->max_button) {
      pp->buttons = (int*) realloc(pp->buttons, (n + 1) * sizeof(int));
      do {
        pp->buttons[++(pp->max_button)] = 0;
      } while (pp->max_button < n);
    }
    fprintf(pp->wish_cmd, "displaybutton %d\n", n);
    return OK;
}

static int button(CSOUND *csound, CNTRL *p)
{
    CONTROL_GLOBALS *pp = get_globals(csound, &(p->p));
    int t = (int)MYFLT2LONG(*p->kcntl);
    readvalues(pp);
    *p->kdest = pp->buttons[t];
    pp->buttons[t] = 0;
    return OK;
}

static int check_set(CSOUND *csound, CNTRL *p)
{
    CONTROL_GLOBALS *pp = get_globals(csound, &(p->p));
    int n = (int) MYFLT2LONG(*p->kcntl);

    if (pp->wish_pid == 0)
      start_tcl_tk(pp);
    if (n > pp->max_check) {
      pp->checks = (int*) realloc(pp->checks, (n + 1) * sizeof(int));
      do {
        pp->checks[++(pp->max_check)] = 0;
      } while (pp->max_check < n);
    }
    fprintf(pp->wish_cmd, "displaycheck %d\n", n);
    return OK;
}

static int check(CSOUND *csound, CNTRL *p)
{
    CONTROL_GLOBALS *pp = get_globals(csound, &(p->p));
    readvalues(pp);
    *p->kdest = pp->checks[(int) MYFLT2LONG(*p->kcntl)];
    return OK;
}

/* **** Text Windows **** */

static int textflash_(CSOUND *csound, TXTWIN *p, int istring)
{
    CONTROL_GLOBALS *pp = get_globals(csound, &(p->p));
    int   wind = (int) MYFLT2LONG(*p->kcntl);
    char  buffer[100];

    if (pp->wish_pid == 0)
      start_tcl_tk(pp);
    if (istring) {
      csound->strarg2name(csound, buffer, ((STRINGDAT *)p->val)->data, "", istring);
/*    csound->Message(csound, "settext %d \"%s\"\n", wind, buffer); */
      fprintf(pp->wish_cmd, "settext %d \"%s\"\n", wind, buffer);
    }
    else if(ISSTRCOD(*p->val)) {
      csound->strarg2name(csound, buffer,
                          csound->GetString(csound, *p->val), "", 1);
    }
    else {
/*    csound->Message(csound, "deltext %d\n", wind); */
      fprintf(pp->wish_cmd, "deltext %d\n", wind);
    }
    return OK;
}

static int textflash(CSOUND *csound, TXTWIN *p){
  return textflash_(csound, p, 0);
}

static int textflash_S(CSOUND *csound, TXTWIN *p){
  return textflash_(csound, p, 1);
}


#define S(x)    sizeof(x)

static OENTRY control_localops[] = {
  { "control",  S(CNTRL), 0, 3, "k", "k", (SUBR) cntrl_set, (SUBR) control, NULL },
{ "setctrl",  S(SCNTRL), 0, 1, "",  "iii", (SUBR) ocontrol, NULL, NULL           },
{ "setctrl.S",  S(SCNTRL), 0, 1, "",  "iSi", (SUBR) ocontrol_S, NULL, NULL       },
{ "button",   S(CNTRL),  0, 3, "k", "k",  (SUBR) button_set, (SUBR) button, NULL },
{ "checkbox", S(CNTRL),  0, 3, "k", "k",   (SUBR) check_set, (SUBR) check, NULL  },
{ "flashtxt", S(TXTWIN), 0, 1, "",  "ii",  (SUBR) textflash, NULL, NULL          },
{ "flashtxt.S", S(TXTWIN), 0, 1, "",  "iS",  (SUBR) textflash_S, NULL, NULL      },
};

LINKAGE_BUILTIN(control_localops)
