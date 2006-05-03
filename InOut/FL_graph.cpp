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

static  Fl_Window   *form = (Fl_Window *) 0;
static  Fl_Choice   *choice = (Fl_Choice *) 0;
static  Fl_Button   *end = (Fl_Button *) 0;

static Fl_Menu_Item menu[] = {
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  0
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  1
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  2
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  3
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  4
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  5
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  6
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  7
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  8
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, //  9
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 10
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 11
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 12
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 13
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 14
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 15
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 16
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 17
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 18
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 19
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 20
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 21
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 22
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 23
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 24
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 25
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 26
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 27
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 28
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }, // 29
// -----------------------------------------------------------------
  { (char*) 0, 0, (Fl_Callback*) 0, (void*) 0, 0, 0, 0, 0, 0 }
};

class graph_box : public Fl_Window {
  void draw();
public:
  int curr;
  int last;
  graph_box(int x, int y, int w, int h, const char *l = 0)
    : Fl_Window(x, y, w, h, l)
  {
    curr = last = -1;
  }
  void add_graph(WINDAT *wdptr);
};

static  graph_box   *graph = (graph_box *) 0;

void graph_box::draw()
{
  Fl_Window::draw();
  fl_color(0, 0, 0);
  fl_line_style(FL_SOLID);
  fl_rect(0, 0, w(), h());
  if (curr >= 0) {
    WINDAT      *win   = (WINDAT*) menu[curr].user_data_;
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
    fl_line(gra_x, gra_y, gra_x, (gra_y + gra_h));
    if (win->danflag) {       /* flag to add dotted divider */
      fl_line_style(FL_DOT);
      fl_line(win_x+w()/2, win_y, win_x+w()/2, win_y+win_h);
    }
    sprintf(string, "%s  %ld points, max %5.3f", msg, npts, win->oabsmax);
    form->label(string);
  }
  fl_line_style(FL_SOLID);
}

void add_graph(WINDAT *wdptr)
{
  WINDAT *n = (WINDAT*) malloc(sizeof(WINDAT));
  int    m;
  WINDAT *old;
  int    replacing = 0;

  memcpy(n, wdptr, sizeof(WINDAT));
  n->fdata = (MYFLT*) malloc(n->npts * sizeof(MYFLT));
  memcpy(n->fdata, wdptr->fdata, n->npts * sizeof(MYFLT));
  for (m = 0; m < NUMOFWINDOWS; m++) {  // If text the same use slot
    if (menu[m].text != NULL && strcmp(wdptr->caption, menu[m].text) == 0) {
      replacing = 1;
      goto replace;
    }
  }
  // Use a new slot, cycling round
  graph->last++;
  m = graph->last;
  if (m >= NUMOFWINDOWS)
    m = graph->last = 0;
 replace:
  old = (WINDAT*)menu[m].user_data_;
  if (old) {
    free((void*) old->fdata);
    free((void*) old);
  }
  menu[m].user_data_ = n;
  if (replacing == 0) {
    if (menu[m].text != NULL)
      free((void*) menu[m].text);
    menu[m].text = (const char*) malloc(strlen(n->caption) + 1);
    strcpy((char*) menu[m].text, n->caption);
  }
  graph->curr = m;
  choice->value(m);
  graph->redraw();
}

void do_redraw(Fl_Widget *, void *)
{
  graph->curr = choice->value();
  graph->redraw();
}

void makeWindow(char *name)
{
  if (form)
    return;
  form = new Fl_Window(WIDTH, HEIGHT, name);
  choice = new Fl_Choice(140, 0, 140, 20, "Choose Graph");
  choice->menu(menu);
  choice->value(0);
  choice->callback((Fl_Callback*) do_redraw);
  graph = new graph_box(BDR, 30 + BDR, WIDTH - 2 * BDR, HEIGHT - 30 - 2 * BDR);
  graph->end();
  end = new Fl_Button(WIDTH - 30, 0, 25, 15, "Quit");
  end->hide();
  form->resizable(graph);
  form->end();
}

extern "C" {

  void DrawGraph_FLTK(CSOUND *csound, WINDAT *wdptr)
  {
    add_graph(wdptr);
    csound->CheckEvents(csound);
  }

  uintptr_t MakeWindow_FLTK(char *name)
  {
    if (form == NULL) {
      makeWindow(name);
      form->show();
    }
    return (uintptr_t) form;
  }

  int CsoundYield_FLTK(CSOUND *csound)
  {
#ifndef NO_FLTK_THREADS
    /* nothing to do, unless no widget thread is running */
    if (csound->QueryGlobalVariable(csound, "_widgets_globals") != NULL)
      return 1;
#endif
    Fl_wait_locked(csound, 0.0);
    return 1;
  }

  void kill_graph(uintptr_t m)
  {
    for (int i = 0; i < NUMOFWINDOWS; i++) {
      WINDAT *n = (WINDAT*) menu[i].user_data_;
      if (n != NULL && n->windid == m) {
        free(n->fdata);
        free(n);
        free((void*) menu[i].text);
        menu[i].user_data_ = (void*) 0;
        menu[i].text = (char*) 0;
        return;
      }
    }
  }

  int ExitGraph_FLTK(CSOUND *csound)
  {
    if (form) {
      if (form->shown() && !(getFLTKFlags(csound) & 256)) {
        const char *env = csound->GetEnv(csound, "CSNOSTOP");
        if (env == NULL || strcmp(env, "yes") != 0) {
          /* print click-Exit message in most recently active window */
          end->show();
          while (end->value() == 0 && form->shown() /* && kcnt */) {
            Fl_wait_locked(csound, 0.03);
          }
        }
      }
      delete form;
      Fl_wait_locked(csound, 0.0);
    }
    form = (Fl_Window *) 0;
    choice = (Fl_Choice *) 0;
    end = (Fl_Button *) 0;
    for (int i = 0; i < NUMOFWINDOWS; i++) {
      WINDAT *n = (WINDAT*) menu[i].user_data_;
      if (n)
        kill_graph((uintptr_t) ((void*) n));
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

