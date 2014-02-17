
/*
  FL_graph.cpp: code for drawing graphs using FLTK library

  Copyright (C) 2003 John ffitch

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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>

#ifdef MSVC
#include <math.h>
#endif
#include "csdl.h"
#include "cwindow.h"
#include "winFLTK.h"

#define NUMOFWINDOWS (30)
#define XINIT    10      /* set default window location */
#define YINIT    50
#define WIDTH    450     /* start off small - user resizes */
#define HEIGHT   150
#define BDR      5
#define BORDERW  10      /* inset from L & R edge */
#define TXHGHT   14      /* baseline offset for text */
#define MAXLSEGS 4096    /* X can only deal with so many linesegs .. */

class graph_box : public Fl_Window {
  void draw();
public:
  int curr;
  int last;
  CSOUND *csound;
  graph_box(void *cs, int x, int y, int w, int h, const char *l = 0)
    : Fl_Window(x, y, w, h, l)
  {
      curr = last = -1;
      csound = (CSOUND*)cs;
  }
  //void add_graph(WINDAT *wdptr);
};

typedef struct {

  Fl_Choice   *choice;
  Fl_Button   *end;
  Fl_Menu_Item *menu;
  graph_box   *graph;
  int graph_created;
  Fl_Window   *form;
} FLGRAPH_GLOBALS;

#define ST(x)   ((flgraphGlobals)->x)

/*void printouts(CSOUND *csound){
    csound->Message(csound, "menu object: %p\n", ST(menu));
    csound->Message(csound, "form object: %p\n", ST(form));
    csound->Message(csound, "graph object: %p\n", ST(graph));
    csound->Message(csound, "choice object: %p\n", ST(choice));
    csound->Message(csound, "globals %p\n",  csound->flgraphGlobals);
}*/

void flgraph_init(CSOUND *csound)
{

    FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
    if (flgraphGlobals==NULL) {
     csound->CreateGlobalVariable(csound, "FLGRAPH_GLOBALS",
                                  sizeof(FLGRAPH_GLOBALS));
     flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
      //csound->flgraphGlobals =
      //(FLGRAPH_GLOBALS*) csound->Malloc(csound,sizeof(FLGRAPH_GLOBALS));
    }
      ST(form) = (Fl_Window *) 0;
      ST(choice) = (Fl_Choice *) 0;
      ST(end) = (Fl_Button *) 0;
      ST(graph) = (graph_box *) 0;
      ST(menu) = (Fl_Menu_Item *) 0;
      ST(graph_created) = 0;


      /*ST(menu) =  (Fl_Menu_Item*) csound->Calloc(csound,
        sizeof(Fl_Menu_Item)*(1+NUMOFWINDOWS));*/
      /* VL: moved menu object to be built at each new compilation */

}

/* static  graph_box   *graph = (graph_box *) 0; */

void graph_box::draw()
{
    FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
    Fl_Window::draw();
    fl_color(0, 0, 0);
    fl_line_style(FL_SOLID);
    fl_rect(0, 0, w(), h());
    if (curr >= 0) {
      WINDAT      *win   = (WINDAT*) ST(menu)[curr].user_data_;
      if (!win)
        return;
      MYFLT       *fdata = win->fdata;
      long        npts   = win->npts;
      char        *msg   = win->caption;
      short       win_x, win_y,        win_h;     /* window rect */
      short       gra_x, gra_y, gra_w, gra_h;     /* graph rect is inset */
      short       y_axis;
      int         lsegs, pts_pls;
      int         pol;
      char        string[80];

      pol  = win->polarity;

      win_x = 0;  win_y = 0;              /* window pixels addressed relative */
      win_h = h();

      /* set new width and height so we leave a 20% border around the plot */
      gra_w = w() - 2*BORDERW;
      gra_h = h();
      gra_x = win_x + BORDERW;
      gra_y = win_y;
      /* figure height of Y axis - top, middle or bottom */
      if (pol == (short)BIPOL)
        y_axis = gra_y + (gra_h/2);
      else if (pol == (short)NEGPOL)
        y_axis = gra_y;
        else                /* POSPOL */
        y_axis = gra_y + gra_h;

      if (npts < MAXLSEGS) {
        lsegs = npts;                   /* one lineseg per datum */
        pts_pls = 1;
      }
      else {
        pts_pls = npts/MAXLSEGS;
        if (npts%MAXLSEGS) pts_pls++;
        lsegs = npts/pts_pls;
      }
      fl_begin_line();
      {       /* take scale factors out of for-loop for faster run-time */
        MYFLT x_scale = gra_w / (MYFLT) (lsegs - 1);
        MYFLT y_scale = gra_h / win->oabsmax; /* unipolar default */
        MYFLT f, ma, mi, *fdptr = fdata;
        int   c, i = 0, j = lsegs;

        if (pol == (short) BIPOL)
          y_scale /= 2.0;             /* max data scales to h/2 */
        /* put x-y pairs into a point list for XDraw */
        while (j--) {
          int x = gra_x + (short) ((MYFLT) i++ * x_scale);
          int y;
          if (pts_pls == 1)
            f = *fdptr++;
          else {
            ma = mi = *fdptr++;
            for (c = 1; c < pts_pls; ++c)
              if ((f = *fdptr++) > ma)    ma = f;
              else if ( f<mi )            mi = f;
            if (ma < 0)             f = mi;
            else if (mi > 0)        f = ma;
            else if (ma > -mi)      f = ma;
            else f = mi;
          }
          y = y_axis - (short) (f * y_scale);
          fl_vertex(x, y);
        }
      }
      fl_end_line();

      /* now draw axes: y-axis is always on the left edge,
         x-axis height is determined by the case we're in */
      fl_line(gra_x, y_axis, (gra_x + gra_w), y_axis);
      fl_line(gra_x, y_axis, (gra_x + gra_w), y_axis);


      fl_line(gra_x, gra_y, gra_x, (gra_y + gra_h));
      if (win->danflag) {       /* flag to add dotted divider */
        fl_line_style(FL_DOT);
        fl_line(win_x+w()/2, win_y, win_x+w()/2, win_y+win_h);
      }
      if(pol != NEGPOL)
      sprintf(string, "%s  %ld points, max %5.3f", msg, npts, win->oabsmax);
      else
      sprintf(string, "%s  %ld points, max %5.3f", msg, npts, win->max);

      ST(form)->label(string);
    }
    fl_line_style(FL_SOLID);
}



void add_graph(CSOUND *csound, WINDAT *wdptr)
{
    FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
    WINDAT *n = (WINDAT*) malloc(sizeof(WINDAT));
    int    m;
    WINDAT *old;
    int    replacing = 0;

    memcpy(n, wdptr, sizeof(WINDAT));
    n->fdata = (MYFLT*) malloc(n->npts * sizeof(MYFLT));
    memcpy(n->fdata, wdptr->fdata, n->npts * sizeof(MYFLT));


    for (m = 0; m < NUMOFWINDOWS; m++) {  // If text the same use slot
      if(ST(menu) != NULL) {
       if (ST(menu)[m].text != NULL && wdptr->caption != NULL){
        if(strcmp(wdptr->caption, ST(menu)[m].text) == 0) {
        replacing = 1;
        goto replace;
        }
       }
      }
    }
    // Use a new slot, cycling round
    ST(graph)->last++;
    m = ST(graph)->last;
    if (m >= NUMOFWINDOWS)
      m = ST(graph)->last = 0;
 replace:

    old = (WINDAT*)ST(menu)[m].user_data_;
    if (old) {
      free((void*) old->fdata);
      free((void*) old);
    }
    ST(menu)[m].user_data_ = n;
    if (replacing == 0) {
      if (ST(menu)[m].text != NULL)
        free((void*) ST(menu)[m].text);
      ST(menu)[m].text = (const char*) malloc(strlen(n->caption) + 1);
      strcpy((char*) ST(menu)[m].text, n->caption);
    }


    // ST(graph)->curr = m;
    //  ST(choice)->value(m);

    ST(graph)->curr = ST(choice)->value();  /* VL: 29.04.09 fix */
    ST(graph)->redraw();
}

void do_redraw(Fl_Widget *, void *cs)
{
    CSOUND *csound = (CSOUND*)cs;
    FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
    ST(graph)->curr = ST(choice)->value();
    ST(graph)->redraw();
}

void makeWindow(CSOUND *csound, char *name)
{
    FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
    if (ST(form))
      return;

    ST(form) = new Fl_Window(WIDTH, HEIGHT, name);
    ST(menu) = new Fl_Menu_Item[1+NUMOFWINDOWS];
    memset(ST(menu), 0, sizeof(Fl_Menu_Item)*(1+NUMOFWINDOWS));
    ST(choice) = new Fl_Choice(140, 0, 140, 20, "Choose Graph");
    ST(choice)->menu(ST(menu));
    ST(choice)->value(0);
    ST(choice)->callback((Fl_Callback*) do_redraw,  (void*)csound);
    ST(graph) = new graph_box(csound,
                          BDR, 30 + BDR, WIDTH - 2 * BDR, HEIGHT - 30 - 2 * BDR);
    ST(graph)->end();
    ST(end) = new Fl_Button(WIDTH - 40, 0, 35, 20, "Quit");
    ST(end)->hide();
    ST(form)->resizable(ST(graph));
    ST(form)->end();
    ST(graph_created) = 1;

}

void graphs_reset(CSOUND * csound){
  //if(csound->flgraphGlobals != NULL)csound->Free(csound, csound->flgraphGlobals);
}

extern "C" {

  void DrawGraph_FLTK(CSOUND *csound, WINDAT *wdptr)
  {

      add_graph(csound, wdptr);
      csound->CheckEvents(csound);
  }

  uintptr_t MakeWindow_FLTK(CSOUND *csound, char *name)
  {
      FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
      if (ST(form) == NULL) {
        makeWindow(csound, name);
        ST(form)->show();
      }

      return (uintptr_t) ST(form);
  }

  int CsoundYield_FLTK(CSOUND *csound){

#ifndef NO_FLTK_THREADS

      /* nothing to do, unless no widget thread is running */
      if (csound->QueryGlobalVariable(csound, "_widgets_globals") != NULL)
        return 1;
#endif
      Fl_wait_locked(csound, 0.0);
      return 1;
  }

  void kill_graph(CSOUND *csound, uintptr_t m)
  {
      FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
      for (int i = 0; i < NUMOFWINDOWS; i++) {
        WINDAT *n = (WINDAT*) ST(menu)[i].user_data_;
        if (n != NULL && ((uintptr_t) n == m ||n->windid == m)) {
          free(n->fdata);
          free(n);
          free((void*) ST(menu)[i].text);
          ST(menu)[i].user_data_ = (void*) 0;
          ST(menu)[i].text = (char*) 0;
          return;
        }
      }
  }

  int ExitGraph_FLTK(CSOUND *csound)
  {
      FLGRAPH_GLOBALS *flgraphGlobals =
         (FLGRAPH_GLOBALS *) csound->QueryGlobalVariable(csound,
                                                         "FLGRAPH_GLOBALS");
      if (ST(form) && ST(graph_created) == 1) {

        if (ST(form)->shown() && !(getFLTKFlags(csound) & 256)) {
        const char *env = csound->GetEnv(csound, "CSNOSTOP");
        if (env == NULL || strcmp(env, "yes") != 0) {
          ST(end)->show();
           // print click-Exit message in most recently active window
          while (ST(end)->value() == 0 && ST(form)->shown()) {
            Fl_wait_locked(csound, 0.03);
          }
          }
         }

        delete ST(form);
        ST(form) = (Fl_Window *) 0;
        Fl_wait_locked(csound, 0.0);

      ST(choice) = (Fl_Choice *) 0;
      ST(graph) = (graph_box *) 0;
      ST(end) = (Fl_Button *) 0;
      ST(graph_created) = 0;

       for (int i = 0; i < NUMOFWINDOWS; i++) {
        WINDAT *n = (WINDAT*) ST(menu)[i].user_data_;
        if (n)
          kill_graph(csound, (uintptr_t) ((void*) n));
          }
       if(ST(menu)){
             delete[] ST(menu);
             ST(menu) = (Fl_Menu_Item *) 0;
       }



      }

      return 0;
  }

#define GUTTERH 20           /* space for text at top & bottom */
#define BORDERW 10           /* inset from L & R edge */

  void MakeXYin_FLTK(CSOUND *csound, XYINDAT *w, MYFLT x, MYFLT y)
  {
      if (w->windid == (uintptr_t) 0) {
        Fl_Window *xyin = new Fl_Window(WIDTH, WIDTH, "XY input");
        short   win_x, win_y;
        short   gra_x, gra_y, gra_w, gra_h;

        win_x = 0;  win_y = 0;           /* window pixels addressed relative */

        Fl_lock(csound);
        xyin->show();
        Fl_wait(csound, 0.0);
        Fl_unlock(csound);

        /* set new width and height so we leave a 20% border around the plot */
        gra_w = xyin->w() - 2*BORDERW;      gra_h = xyin->h() - 2*GUTTERH;
        gra_x = win_x + BORDERW;            gra_y = win_y + GUTTERH;
        w->m_x = gra_x + (int) (x * (MYFLT) gra_w);
        w->m_y = gra_y + (int) (y * (MYFLT) gra_h);
        w->down = 0;

        Fl_lock(csound);
        Fl_wait(csound, 0.0);
        xyin->make_current();
        fl_color(0, 0, 0);
        fl_line_style(FL_DOT);
        fl_line(gra_x, w->m_y, (gra_x + gra_w), w->m_y);
        fl_line(w->m_x, gra_y, w->m_x, (gra_y + gra_h));
        Fl_unlock(csound);
        w->windid = (uintptr_t) xyin;
      }
  }

  void ReadXYin_FLTK(CSOUND *csound, XYINDAT *wdptr)
  {
      short     win_x, win_y;
      short     gra_x, gra_y, gra_w, gra_h;
      short     m_x, m_y;
      Fl_Window *xwin = (Fl_Window*) wdptr->windid;

      Fl_lock(csound);
      Fl_wait(csound, 0.0);
      m_x = Fl::event_x();
      m_y = Fl::event_y();
      wdptr->down = (short) (Fl::event_button1() ? 1 : 0);
      Fl_unlock(csound);

      if (!wdptr->down)
        return;

      win_x = 0;  win_y = 0;      /* window pixels addressed relative */

      /* set new width and height so we leave a 20% border around the plot */
      gra_w = xwin->w() - 2*BORDERW;      gra_h = xwin->h() - 2*GUTTERH;
      gra_x = win_x + BORDERW;            gra_y = win_y + GUTTERH;

      /* clip mouse position */
      if (m_x < gra_x)            m_x = gra_x;
      else if (m_x > gra_x+gra_w) m_x = gra_x+gra_w;
      if (m_y < gra_y)            m_y = gra_y;
      else if (m_y > gra_y+gra_h) m_y = gra_y+gra_h;

      if (m_x != wdptr->m_x || m_y != wdptr->m_y) { /* only redo if changed */
        Fl_lock(csound);
        xwin->make_current();
        /* undraw old crosshairs */
        fl_color(FL_BACKGROUND_COLOR);
        fl_line_style(FL_SOLID);
        fl_line(gra_x, wdptr->m_y, (gra_x + gra_w), wdptr->m_y);
        fl_line(wdptr->m_x, gra_y, wdptr->m_x, (gra_y + gra_h));
        // Draw new
        fl_color(0, 0, 0);
        fl_line_style(FL_SOLID);
        fl_line(gra_x, m_y, (gra_x + gra_w), m_y);
        fl_line(m_x, gra_y, m_x, (gra_y + gra_h));
        Fl_unlock(csound);
        wdptr->m_x = m_x;       wdptr->m_y = m_y;
        wdptr->x = ((MYFLT) m_x - gra_x) / (MYFLT) gra_w;
        wdptr->y = ((MYFLT) m_y - gra_y) / (MYFLT) gra_h;
      }
  }

  void KillXYin_FLTK(CSOUND *csound, XYINDAT *wdptr)
  {

      delete ((Fl_Window*) wdptr->windid);
      wdptr->windid = (uintptr_t) 0;
  }
}
