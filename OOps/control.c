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

#include "cs.h"
#include "control.h"

#ifdef TCLTK
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
#include <sys/types.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static char cmd[100];
static int wish_pid = 0;
int pip1[2];
int pip2[2];
FILE *wish_cmd, *wish_res;
static int *values = NULL;
static int *minvals = NULL;
static int *maxvals = NULL;
static int max_sliders  = 0;
static int *buttons = NULL;
static int *checks = NULL;
static int max_button  = 0;
static int max_check  = 0;

static void kill_wish(void)
{
    printf("Closing down wish(%d)\n", wish_pid);
    kill(wish_pid, 9);
    wish_pid = 0;
}

static void start_tcl_tk(void)
{
    int i;
    printf("TCL/Tk\n");
    pipe(pip1); pipe(pip2);
    if ((wish_pid = fork())<0) return;
    if (wish_pid==0) {        /* Child process */
      char *argv[7];
      argv[0] = "sh";
      argv[1] = "-c";
      argv[2] = "wish";
      argv[3] = "-name";
      argv[4] = "sliders";
      argv[5] = NULL;
      close(pip1[0]); close(pip2[1]);
      close(0); close(1);
      dup2(pip2[0], 0);
      dup2(pip1[1], 1);
      setvbuf(stdout, (char *)NULL, _IOLBF, 0);
      execvp("/bin/sh", argv);
      exit(127);
    }
                                /* Main process -- create communications */
    close(pip1[1]); close(pip2[0]);
    wish_res = fdopen(pip1[0], "r");
    wish_cmd = fdopen(pip2[1], "w");
    setvbuf(wish_cmd, (char *)NULL, _IOLBF, 0);
    setvbuf(wish_res, (char *)NULL, _IOLBF, 0);
    atexit(kill_wish);
    fprintf(wish_cmd, "source nsliders.tk\n");
    fgets(cmd, 100, wish_res);
    printf("Wish %s\n", cmd);
    values = (int*)malloc(sizeof(int)*8);
    minvals = (int*)malloc(sizeof(int)*8);
    maxvals = (int*)malloc(sizeof(int)*8);
    buttons = (int*)malloc(sizeof(int)*8);
    checks  = (int*)malloc(sizeof(int)*8);
    max_sliders = 8;
    max_button = 8;
    max_check = 8;
    for (i = 0; i<max_sliders; i++) {
      minvals[i] = 0; maxvals[i] = 127;
    }
    sleep(2);
}

static void ensure_slider(int n)
{
/*      printf("Ensure_slider %d\n", n); */
    if (wish_pid==0) start_tcl_tk();
    if (n > max_sliders) {
      int i, nn = n+1;
      values  = (int*)realloc(values, nn*sizeof(int));
      minvals = (int*)realloc(values, nn*sizeof(int));
      maxvals = (int*)realloc(values, nn*sizeof(int));
      for (i=max_sliders+1; i<n; i++) {
        minvals[i] = 0; maxvals[i] = 127;
      }
    }
/*      printf("displayslider %d\n", n); */
    fprintf(wish_cmd, "displayslider %d\n", n);
}

static void readvalues(void)
{
    fd_set rfds;
    struct timeval tv;

    /* Watch wish_res to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(pip1[0], &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
                                /* Read all changes */
    while (select(pip1[0]+1, &rfds, NULL, NULL, &tv)) {
      int n, val;
      fscanf(wish_res, "%d %d", &n, &val);
      if (n>0) values[n] = val;
      else if (n==0) buttons[val] = 1;
      else checks[-n] = val;
      tv.tv_sec = 0;
      tv.tv_usec = 0;
    }
}

int cntrl_set(CNTRL *p)
{
    ensure_slider((int)(*p->kcntl+FL(0.5)));
    return OK;
}

int control(CNTRL *p)
{
    readvalues();
    *p->kdest = values[(int)(*p->kcntl+FL(0.5))];
    return OK;
}

int ocontrol(SCNTRL *p)
{
    int c = (int)*p->which;
    int slider = (int)(*p->kcntl+FL(0.5));
/*      printf("ocontrol: %d %d %f\n", slider, c, *p->val); */
    ensure_slider(slider);
    switch (c) {
    case 1:
      fprintf(wish_cmd, "setvalue %d %d\n",
              slider, (int)*p->val);
      values[slider] = (int)*p->val;
      break;
    case 2:
      if (minvals[slider] != (int)*p->val) {
        fprintf(wish_cmd, "setmin %d %d\n",
                slider, (int)*p->val);
        minvals[slider] = (int)*p->val;
      }
      break;
    case 3:
      if (maxvals[slider] != (int)*p->val) {
        fprintf(wish_cmd, "setmax %d %d\n",
                slider, (int)*p->val);
        maxvals[slider] = (int)*p->val;
      }
      break;
    case 4:
      {
        char buffer[100];
        if (*p->val == SSTRCOD) {
          if (p->STRARG == NULL) strcpy(buffer,unquote(currevent->strarg));
          else strcpy(buffer,unquote(p->STRARG));    /* unquote it,  else use */
        }
        else sprintf(buffer, "Control %d", slider);
        printf("Slider %d set to %s\n", slider, buffer);
        fprintf(wish_cmd, "setlab %d \"%s\"\n", slider, buffer);
        break;
      }
    default:
      err_printf("Unknown control %d\n", c);
      return NOTOK;
    }
    return OK;
}

int button_set(CNTRL *p)
{
    int n = (int)(FL(0.5)+*p->kcntl);

    if (wish_pid==0) start_tcl_tk();
    if (n > max_button)
      buttons  = (int*)realloc(values, (n+1)*sizeof(int));
    fprintf(wish_cmd, "displaybutton %d\n", n);
    return OK;
}



int button(CNTRL *p)
{
    readvalues();
    *p->kdest = buttons[(int)(*p->kcntl+FL(0.5))];
    buttons[(int)*p->kcntl] = 0;
    return OK;
}

int check_set(CNTRL *p)
{
    int n = (int)(FL(0.5)+*p->kcntl);

    if (wish_pid==0) start_tcl_tk();
    if (n > max_check)
      checks  = (int*)realloc(values, (n+1)*sizeof(int));
    fprintf(wish_cmd, "displaycheck %d\n", n);
    return OK;
}


int check(CNTRL *p)
{
    readvalues();
    *p->kdest = checks[(int)(*p->kcntl+FL(0.5))];
    return OK;
}

/* **** Text Windows **** */
int textflash(TXTWIN *p)
{
    int wind = (int)(*p->kcntl+FL(0.5));
    char buffer[100];
    if (wish_pid==0) start_tcl_tk();
    if (*p->val == SSTRCOD) {
      if (p->STRARG == NULL) strcpy(buffer,unquote(currevent->strarg));
      else strcpy(buffer,unquote(p->STRARG));    /* unquote it,  else use */
/*       printf("settext %d \"%s\"\n", wind, buffer); */
      fprintf(wish_cmd, "settext %d \"%s\"\n", wind, buffer);
    }
    else {
/*       printf("deltext %d\n", wind); */
      fprintf(wish_cmd, "deltext %d\n", wind);
    }
    return OK;
}
#endif
