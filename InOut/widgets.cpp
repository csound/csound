/*
  widgets.cpp:

  Copyright (C) 2002 Gabriel Maldonado

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

#if defined(WIN32)
#include <FL/Fl_Output.H>
#endif
#include <unistd.h>
#include <csound.h>
#include "widglobals.h"
#include <FL/x.H>

Fl_Font FONT_TABLE[] = { FL_HELVETICA,                  FL_HELVETICA,
                         FL_HELVETICA_BOLD,             FL_HELVETICA_ITALIC,
                         FL_HELVETICA_BOLD_ITALIC,      FL_COURIER,
                         FL_COURIER_BOLD,               FL_COURIER_ITALIC,
                         FL_COURIER_BOLD_ITALIC,        FL_TIMES,
                         FL_TIMES_BOLD,                 FL_TIMES_ITALIC,
                         FL_TIMES_BOLD_ITALIC,          FL_SYMBOL,
                         FL_SCREEN,                     FL_SCREEN_BOLD,
                         FL_ZAPF_DINGBATS };

Fl_Align ALIGN_TABLE[] = { FL_ALIGN_BOTTOM,      FL_ALIGN_CENTER,
                           FL_ALIGN_TOP,         FL_ALIGN_BOTTOM,
                           FL_ALIGN_LEFT,        FL_ALIGN_RIGHT,
                           FL_ALIGN_TOP_LEFT,    FL_ALIGN_TOP_RIGHT,
                           FL_ALIGN_BOTTOM_LEFT, FL_ALIGN_BOTTOM_RIGHT };

Fl_Boxtype BOX_TABLE[] = {  FL_FLAT_BOX,         FL_FLAT_BOX,
                            FL_UP_BOX,           FL_DOWN_BOX,
                            FL_THIN_UP_BOX,      FL_THIN_DOWN_BOX,
                            FL_ENGRAVED_BOX,     FL_EMBOSSED_BOX,
                            FL_BORDER_BOX,      _FL_SHADOW_BOX,
                            _FL_ROUNDED_BOX,    _FL_RSHADOW_BOX,
                            _FL_RFLAT_BOX,      _FL_ROUND_UP_BOX,
                            _FL_ROUND_DOWN_BOX, _FL_DIAMOND_UP_BOX,
                            _FL_DIAMOND_DOWN_BOX,_FL_OVAL_BOX,
                            _FL_OSHADOW_BOX,    _FL_OFLAT_BOX};

void widget_init(CSOUND *csound)
{
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *) csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (widgetGlobals == NULL) {
      csound->CreateGlobalVariable(csound, "WIDGET_GLOBALS",
                                   sizeof(WIDGET_GLOBALS));
      widgetGlobals =
        (WIDGET_GLOBALS *) csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

      //csound->widgetGlobals = new WIDGET_GLOBALS;
      //csound->Calloc(csound, sizeof(WIDGET_GLOBALS));
      //      ST(indrag)            = 0;
      //      ST(sldrag)            = 0;
      //      ST(stack_count)       = 0;

      ST(FLcontrol_iheight) = 15;
      ST(FLroller_iheight)  = 18;
      ST(FLcontrol_iwidth)  = 400;
      ST(FLroller_iwidth)   = 150;
      ST(FLvalue_iwidth)    = 100;

      ST(FLcolor)           = -1;
      ST(FLcolor2)          = -1;
      // below was commented out, why? VL 24-04-08
      ST(FLtext_size)       = 0;
      ST(FLtext_color)      = -1;
      ST(FLtext_font)       = -1;
      //  below was commented out, why? VL 24-04-08
      ST(FLtext_align)      = 0;

      ST(FL_ix)             = 10;
      ST(FL_iy)             = 10;
      ST(currentSnapGroup)  = 0;
      ST(last_KEY)=0;
      ST(isKeyDown)=0;
    }
}

extern void graphs_reset(CSOUND *csound);

int widget_reset(CSOUND *csound, void *pp)
{
    IGN(pp);
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (widgetGlobals != NULL) {
      csound->DestroyGlobalVariable(csound, "WIDGET_GLOBALS");
    }
    graphs_reset(csound);
    return OK;
}

#ifndef NO_FLTK_THREADS
extern "C" {
  /* called by sensevents() once in every control period */
  static void evt_callback(CSOUND *csound, widgetsGlobals_t *p)
  {
      /* flush any pending real time events */
      if (UNLIKELY(p->eventQueue != NULL)) {
        csound->LockMutex(p->mutex_);
        while (p->eventQueue != NULL) {
          rtEvt_t *ep = p->eventQueue;
          p->eventQueue = ep->nxt;
          csound->UnlockMutex(p->mutex_);
          csound->insert_score_event_at_sample(csound, &(ep->evt),
                                     csound->GetCurrentTimeSamples(csound));
          free(ep);
          csound->LockMutex(p->mutex_);
        }
        csound->UnlockMutex(p->mutex_);
      }
      /* if all windows have been closed, terminate performance */
      if (UNLIKELY(p->exit_now)) {
        EVTBLK  e;
        memset(&e, 0, sizeof(EVTBLK));
        e.opcod = 'e';
        csound->insert_score_event_at_sample(csound, &e,
                                             csound->GetCurrentTimeSamples(csound));
      }
  }
}       // extern "C"
#endif  // NO_FLTK_THREADS

// ---- IV - Aug 23 2002 ---- included file: Fl_linux_stubs.cpp

/*
 * this is a file used in the linux distribution to be able to use
 * the DirectCsound 5.1 widget code by Gabriel Maldonado on linux
 * systems.
 *
 * This code has been written by Nicola Bernardini (nicb@centrotemporeale.it)
 * mostly based on ideas by Dave Phillips (dlphip@bright.net)
 */

extern "C" {
  void ButtonSched(CSOUND *csound, MYFLT *args[], int numargs)
  { /* based on code by rasmus */
#ifndef NO_FLTK_THREADS
      widgetsGlobals_t  *p;
      /* this is still not fully thread safe... */
      /* hope that no global variable is created just while this fn is called */
      p = (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                          "_widgets_globals");
      if (p != NULL) {
        rtEvt_t   *evt;
        int       i;

        /* Create the new event */
        evt = (rtEvt_t*) malloc(sizeof(rtEvt_t));
        evt->nxt = NULL;
        evt->evt.strarg = NULL; evt->evt.scnt = 0;
        evt->evt.opcod = (char) *args[0];
        if (evt->evt.opcod == '\0')
          evt->evt.opcod = 'i';
        evt->evt.pcnt = numargs - 1;
        evt->evt.p[1] = evt->evt.p[2] = evt->evt.p[3] = MYFLT(0.0);
        for (i = 1; i < numargs; i++)
          evt->evt.p[i] = *args[i];
        if (evt->evt.p[2] < MYFLT(0.0))
          evt->evt.p[2] = MYFLT(0.0);
        /* queue event for insertion by main Csound thread */
        csound->LockMutex(p->mutex_);
        if (p->eventQueue == NULL)
          p->eventQueue = evt;
        else {
          rtEvt_t *ep = p->eventQueue;
          while (ep->nxt != NULL)
            ep = ep->nxt;
          ep->nxt = evt;
        }
        csound->UnlockMutex(p->mutex_);
      }
      else
#endif  // NO_FLTK_THREADS
        {
          EVTBLK  e;
          int     i;

          /* Create the new event */
          e.strarg = NULL; e.scnt = 0;
          e.opcod = (char) *args[0];
          if (e.opcod == '\0')
            e.opcod = 'i';
          e.pcnt = numargs - 1;
          e.p[1] = e.p[2] = e.p[3] = MYFLT(0.0);
          for (i = 1; i < numargs; i++)
            e.p[i] = *args[i];
          if (e.p[2] < MYFLT(0.0))
            e.p[2] = MYFLT(0.0);
          csound->insert_score_event_at_sample(csound, &e,
                                    csound->GetCurrentTimeSamples(csound));
        }
  }
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Knob.cxx

// generated by Fast Light User Interface Designer (fluid) version 2.00

Fl_Knob::Fl_Knob(CSOUND *cs, int xx,int yy,int ww,int hh,const char *l)
  : Fl_Valuator(xx,yy,ww,hh,l)
{
    csound = cs;
    a1 = 35;
    a2 = 325;
    _type = DOTLIN;
    _percent = 0.3f;
    _scaleticks = 10;
}

Fl_Knob::~Fl_Knob(void)
{
}

void Fl_Knob::draw(void)
{
    int ox,oy,ww,hh,side;
    unsigned char rr,gg,bb;

    ox = x();
    oy = y();
    ww = w();
    hh = h();
    draw_label();
    fl_clip(ox,oy,ww,hh);
    if (ww > hh) {
      side = hh;
      ox = ox + (ww - side) / 2;
    }
    else {
      side = ww;
      oy = oy + (hh - side) / 2;
    }
    side = w() > h () ? hh:ww;
    int dam = damage();
    if (dam & FL_DAMAGE_ALL) {
      int col = parent()->color();
      fl_color(col);
      fl_rectf(ox,oy,side,side);
      Fl::get_color((Fl_Color)col,rr,gg,bb);
      shadow(-60,rr,gg,bb);
      fl_pie(ox+9,oy+9,side-12,side-12,0,360);
      draw_scale(ox,oy,side);
      col = color();
      Fl::get_color((Fl_Color)col,rr,gg,bb);
      shadow(15,rr,gg,bb);
      fl_pie(ox+6,oy+6,side-12,side-12,40,80);
      shadow(30,rr,gg,bb);
      fl_pie(ox+6,oy+6,side-12,side-12,80,220);
      shadow(-15,rr,gg,bb);
      fl_pie(ox+6,oy+6,side-12,side-12,220,260);
      shadow(-30,rr,gg,bb);
      fl_pie(ox+6,oy+6,side-12,side-12,260,400);
      fl_color(FL_BLACK);
      fl_arc(ox+6,oy+6,side-11,side-11,0,360);
      fl_color(col);
      fl_pie(ox+10,oy+10,side-20,side-20,0,360);
    }
    else {
      fl_color(color());
      fl_pie(ox+10,oy+10,side-20,side-20,0,360);
    }
    Fl::get_color((Fl_Color)color(),rr,gg,bb);
    shadow(10,rr,gg,bb);
    fl_pie(ox+10,oy+10,side-20,side-20,110,150);
    fl_pie(ox+10,oy+10,side-20,side-20,290,330);
    shadow(17,rr,gg,bb);
    fl_pie(ox+10,oy+10,side-20,side-20,120,140);
    fl_pie(ox+10,oy+10,side-20,side-20,300,320);
    shadow(25,rr,gg,bb);
    fl_pie(ox+10,oy+10,side-20,side-20,127,133);
    fl_pie(ox+10,oy+10,side-20,side-20,307,313);
    draw_cursor(ox,oy,side);
    fl_pop_clip();
}

int Fl_Knob::handle(int  event)
{
    int ox,oy,ww,hh;

    ox = x() + 10; oy = y() + 10;
    ww = w() - 20;
    hh = h() - 20;
    switch (event) {
    case FL_PUSH:
      handle_push();
      return 1;                 // CHECKME ***JPff added this; is that right?***
    case FL_DRAG:
      {
        int mx = Fl::event_x()-ox-ww/2;
        int my = Fl::event_y()-oy-hh/2;
        if (!mx && !my) return 1;
        double angle = 270-atan2((float)-my, (float)mx)*180.0/PI;
        double oldangle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
        while (angle < oldangle-180) angle += 360.0;
        while (angle > oldangle+180) angle -= 360.0;
        double val;
        if ((a1<a2) ? (angle <= a1) : (angle >= a1)) {
          val = minimum();
        }
        else if ((a1<a2) ? (angle >= a2) : (angle <= a2)) {
          val = maximum();
        }
        else {
          val = minimum() + (maximum()-minimum())*(angle-a1)/(a2-a1);
        }
        handle_drag(clamp(round(val)));
      }
      return 1;
    case FL_RELEASE:
      handle_release();
      return 1;
    default:
      return 0;
    }
    return 0;
}

void Fl_Knob::type(int ty)
{
    _type = ty;
}

void Fl_Knob::shadow(const int offs,const uchar r,uchar g,uchar b)
{
    int rr,gg,bb;

    rr = r + offs;
    rr = rr > 255 ? 255 : rr;
    rr = rr < 0 ? 0:rr;
    gg = g + offs;
    gg = gg > 255 ? 255 : gg;
    gg = gg < 0 ? 0 :gg ;
    bb = b + offs;
    bb = bb > 255 ? 255 : bb;
    bb = bb < 0 ? 0 : bb;
    fl_color((uchar)rr,(uchar)gg,(uchar)bb);
}

void Fl_Knob::draw_scale(const int ox,const int oy,const int side)
{
    float x1,y1,x2,y2,rds,cx,cy,ca,sa;

    rds = side * 0.5f;
    cx = ox + side * 0.5f;
    cy = oy + side * 0.5f;
    if (!(_type & DOTLOG_3)) {
      if (_scaleticks == 0) return;
      double a_step = (10.0*PI/6.0) / _scaleticks;
      double a_orig = -(PI/3.0);
      for (int a = 0; a <= _scaleticks; a++) {
        double na = a_orig + a * a_step;
        ca = cos(na);
        sa = sin(na);
        x1 = cx + rds * ca;
        y1 = cy - rds * sa;
        x2 = cx + (rds-6) * ca;
        y2 = cy - (rds-6) * sa;
        fl_color(FL_BLACK);
        fl_line((int)x1,(int)y1,(int)x2,(int)y2);
        fl_color(FL_WHITE);
        if (sa*ca >=0)
          fl_line((int)x1+1,(int)y1+1,(int)x2+1,(int)y2+1);
        else
          fl_line((int)x1+1,(int)y1-1,(int)x2+1,(int)y2-1);
      }
    }
    else {
      int nb_dec = (_type & DOTLOG_3);
      for (int k = 0; k < nb_dec; k++) {
        double a_step = (10.0*PI/6.0) / nb_dec;
        double a_orig = -(PI/3.0) + k * a_step;
        for (int a = (k) ? 2:1; a <= 10; ) {
          double na = a_orig + log10((double)a) * a_step;
          ca = cos(na);
          sa = sin(na);
          x1 = cx - rds * ca;
          y1 = cy - rds * sa;
          x2 = cx - (rds-6) * ca;
          y2 = cy - (rds-6) * sa;
          fl_color(FL_BLACK);
          fl_line((int)x1,(int)y1,(int)x2,(int)y2);
          fl_color(FL_WHITE);
          if (sa*ca <0)
            fl_line((int)x1+1,(int)y1+1,(int)x2+1,(int)y2+1);
          else
            fl_line((int)x1+1,(int)y1-1,(int)x2+1,(int)y2-1);
          if ((a == 1) || (nb_dec == 1))
            a += 1;
          else
            a += 2;
        }
      }
    }
}

void Fl_Knob::draw_cursor(const int ox,const int oy,const int side)
{
    float rds,cur,cx,cy;
    double angle;

    rds = (side - 20.0f) * 0.5f;
    cur = _percent * rds * 0.5f;
    cx = ox + side * 0.5f;
    cy = oy + side * 0.5f;
    angle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
    fl_push_matrix();
    fl_scale(1,1);
    fl_translate(cx,cy);
    fl_rotate(-angle);
    fl_translate(0,rds-cur-2.0f);
    if (_type<LINELIN) {
      fl_begin_polygon();
      fl_color(selection_color());
      fl_circle(0.0,0.0,cur);
      fl_end_polygon();
      fl_begin_loop();
      fl_color(FL_BLACK);
      fl_circle(0.0,0.0,cur);
      fl_end_loop();
    }
    else {
      fl_begin_polygon();
      fl_color(selection_color());
      fl_vertex(-1.5,-cur);
      fl_vertex(-1.5,cur);
      fl_vertex(1.5,cur);
      fl_vertex(1.5,-cur);
      fl_end_polygon();
      fl_begin_loop();
      fl_color(FL_BLACK);
      fl_vertex(-1.5,-cur);
      fl_vertex(-1.5,cur);
      fl_vertex(1.5,cur);
      fl_vertex(1.5,-cur);
      fl_end_loop();
    }
    fl_pop_matrix();
}

void Fl_Knob::cursor(const int pc)
{
    _percent = (float)pc*0.01f;

    if (_percent < 0.05f) _percent = 0.05f;
    if (_percent > 1.0f) _percent = 1.0f;
    if (visible()) damage(FL_DAMAGE_CHILD);
}

void Fl_Knob::scaleticks(const int tck)
{
    _scaleticks = tck;
    if (_scaleticks < 0) _scaleticks = 0;
    if (_scaleticks > 31) _scaleticks = 31;
    if (visible()) damage(FL_DAMAGE_ALL);
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Spin.cpp

void Fl_Spin::draw()
{
    int sxx = x(), syy = y(), sww = w(), shh = h();
    Fl_Boxtype box1 = (Fl_Boxtype)(box());
    int border_size=Fl::box_dx(box());
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

    if (damage()&~FL_DAMAGE_CHILD) clear_damage(FL_DAMAGE_ALL);

    if (!box1) box1 = (Fl_Boxtype)(box()&-2);

    if ((ST(indrag) || mouseobj) && deltadir!=0) {
      if (deltadir>0) {
        draw_box(fl_down(box1),sxx,syy,sww,shh/2,color());
        draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
      }
      else {
        draw_box(box1,sxx,syy,sww,shh/2,color());
        draw_box(fl_down(box1),sxx,syy+shh/2,sww,shh/2,color());
      }
    }
    else {
      draw_box(box1,sxx,syy,sww,shh/2,color());
      draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
    }
    sxx+=border_size;
    syy+=border_size;
    sww-=border_size*2;
    shh-=border_size*2;
    if (active_r()) {
      fl_color(selection_color());
    }
    else {
      fl_color(selection_color() | 8);
    }
    int w1 = (sww-1)|1; // use odd sizes only
    int X = sxx+w1/2;
    int W = w1/3;
    int h1 = shh/2-border_size-2;
    int Y= syy;
    fl_polygon(X, Y, X+W,Y+h1 , X-W, Y+h1);
    Y=syy+shh/2+border_size+1+h1;
    fl_polygon(X, Y, X-W, Y-h1, X+W, Y-h1);
    clear_damage();
}

void Fl_Spin::repeat_callback(void* v)
{
    Fl_Spin* b = (Fl_Spin*)v;
    if (b->mouseobj) {
      Fl::add_timeout(0.1, repeat_callback, b);
      b->increment_cb();
    }
}

void Fl_Spin::increment_cb(void)
{
    if (!mouseobj) return;
    delta += deltadir;
    double v;
    switch (drag) {
    case 3: v = increment(value(), deltadir*100); break;
    case 2: v = increment(value(), deltadir*10); break;
    default:v = increment(value(), deltadir); break;
    }
    v = round(v);
    handle_drag(soft()?softclamp(v):clamp(v));
}

int Fl_Spin::handle(int event)
{
    double v;
    int olddelta;
    int mx = Fl::event_x();
    int my = Fl::event_y();
    int sxx = x(), syy = y(), sww = w(), shh = h();
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

    switch (event) {
    case FL_PUSH:
      //    if (!step()) goto DEFAULT;
      iy = my;
      ix = mx;
      drag = Fl::event_button();
      handle_push();
      ST(indrag)=1;
      mouseobj=1;
      Fl::add_timeout(0.5, repeat_callback, this);
      delta=0;
      if (Fl::event_inside(sxx,syy,sww,shh/2)) {
        deltadir=1;
      }
      else if (Fl::event_inside(sxx,syy+shh/2,sww,shh/2)) {
        deltadir=-1;
      }
      else {
        deltadir=0;
      }
      increment_cb();
      redraw();
      return 1;
    case FL_DRAG:
      if (mouseobj) {
        mouseobj=0;
        Fl::remove_timeout(repeat_callback, this);
      }
      //    if (!step()) goto DEFAULT;
      olddelta=delta;
      delta = - (Fl::event_y()-iy);
      if ((delta>5) || (delta<-5)) {
        deltadir=((olddelta-delta)>0)?-1:(((olddelta-delta)<0)?1:0);
      }
      else {
        deltadir=0; delta = olddelta;
      }
      switch (drag) {
      case 3: v = increment(value(), deltadir*100); break;
      case 2: v = increment(value(), deltadir*10); break;
      default:v = increment(value(), deltadir); break;
      }
      v = round(v);
      handle_drag(soft()?softclamp(v):clamp(v));
      ST(indrag)=1;
      return 1;
    case FL_RELEASE:
      if (mouseobj) {
        Fl::remove_timeout(repeat_callback, this);
      }
      //    if (!step()) goto DEFAULT;
      ST(indrag)=0;
      delta=0;
      deltadir=0;
      mouseobj=0;
      handle_release();
      redraw();
      return 1;
    default:
      ST(indrag)=0;
      return Fl_Valuator::handle(event);
    }
}

Fl_Spin::~Fl_Spin(void)
{
    Fl::remove_timeout(repeat_callback, this);
}

Fl_Spin::Fl_Spin(CSOUND *cs, int x, int y, int w, int h, const char* l)
  : Fl_Valuator(x,y,w,h,l)
{

    csound = cs;
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    soft_ = 0;
    align(FL_ALIGN_LEFT);
    ix=x;
    iy=y;
    drag=0;
    ST(indrag)=0;
    mouseobj = 0;
    deltadir=0;
    delta=0;
    indrag=0;
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Value_Input_Spin.cpp

void Fl_Value_Input_Spin::input_cb(Fl_Widget*, void* v)
{
    Fl_Value_Input_Spin& t = *(Fl_Value_Input_Spin*)v;

    CSOUND *csound = t.csound;
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    double nv;
    if (t.step()>=1.0) nv = strtol(t.input.value(), 0, 0);
    else nv = csound->strtod((char*)t.input.value(), 0);
    ST(hack_o_rama1) = 1;
    t.handle_push();
    t.handle_drag(nv);
    t.handle_release();
    ST(hack_o_rama1) = 0;
}

void Fl_Value_Input_Spin::draw(void)
{
    int sxx = x(), syy = y(), sww = w(), shh = h();
    sxx += sww - buttonssize(); sww = buttonssize();
    Fl_Boxtype box1 = (Fl_Boxtype)(box()&-2);
    int border_size=Fl::box_dx(box());
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

    if (damage()&~FL_DAMAGE_CHILD) input.clear_damage(FL_DAMAGE_ALL);
    input.box(box());
    input.color(FL_WHITE, selection_color());
    input.redraw();
    input.clear_damage();
    sxx+=border_size;
    syy+=border_size;
    sww-=border_size*2;
    shh-=border_size*2;

    if (!box1) box1 = (Fl_Boxtype)(box()&-2);

    if ((ST(indrag) || mouseobj) && deltadir!=0) {
      if (deltadir>0) {
        draw_box(fl_down(box1),sxx,syy,sww,shh/2,color());
        draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
      }
      else {
        draw_box(box1,sxx,syy,sww,shh/2,color());
        draw_box(fl_down(box1),sxx,syy+shh/2,sww,shh/2,color());
      }
    }
    else {
      draw_box(box1,sxx,syy,sww,shh/2,color());
      draw_box(box1,sxx,syy+shh/2,sww,shh/2,color());
    }
    sxx+=border_size;
    syy+=border_size;
    sww-=border_size*2;
    shh-=border_size*2;
    if (active_r()) {
      fl_color(labelcolor());
    }
    else {
      fl_color(labelcolor() | 8);
    }
    int w1 = (sww-1)|1; // use odd sizes only
    int X = sxx+w1/2;
    int W = w1/3;
    int h1 = shh/2-border_size-2;
    int Y= syy;
    fl_polygon(X, Y, X+W,Y+h1 , X-W, Y+h1);
    Y=syy+shh/2+border_size+1+h1;
    fl_polygon(X, Y, X-W, Y-h1, X+W, Y-h1);
    clear_damage();
}

void Fl_Value_Input_Spin::resize(int X, int Y, int W, int H)
{
    input.resize(X,Y,W,H);
    Fl_Valuator::resize(X,Y,W,H);
}

void Fl_Value_Input_Spin::value_damage(void)
{
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (ST(hack_o_rama1)) return;
    char buf[128];
    format(buf);
    input.value(buf);
    input.mark(input.position()); // turn off selection highlight
}

void Fl_Value_Input_Spin::repeat_callback(void* v)
{
    Fl_Value_Input_Spin* b = (Fl_Value_Input_Spin*)v;
    if (b->mouseobj) {
      Fl::add_timeout(.1, repeat_callback, b);
      b->increment_cb();
    }
}

void Fl_Value_Input_Spin::increment_cb(void)
{
    if (!mouseobj) return;
    delta+=deltadir;
    double v;
    switch (drag) {
    case 3: v = increment(value(), deltadir*100); break;
    case 2: v = increment(value(), deltadir*10); break;
    default:v = increment(value(), deltadir); break;
    }
    v = round(v);
    handle_drag(soft()?softclamp(v):clamp(v));
}

int Fl_Value_Input_Spin::handle(int event)
{
    double v;
    int olddelta;
    int mx = Fl::event_x();
    int my = Fl::event_y();
    int sxx = x(), syy = y(), sww = w(), shh = h();
    sxx += sww - buttonssize(); sww = buttonssize();
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

    if (!ST(indrag) && ( !ST(sldrag) || !((mx>=sxx && mx<=(sxx+sww)) &&
                                          (my>=syy && my<=(syy+shh))))  ) {
      ST(indrag)=0;
      switch(event) {
      case FL_PUSH:
      case FL_DRAG:
        ST(sldrag)=1;
        break;
      case FL_FOCUS:
        input.take_focus();
        break;
      case FL_UNFOCUS:
        redraw();
        break;
      default:
        ST(sldrag)=0;
      }
      input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
      return input.handle(event);
    }

    switch (event) {
    case FL_PUSH:
      //    if (!step()) goto DEFAULT;
      iy = my;
      ix = mx;
      drag = Fl::event_button();
      handle_push();
      ST(indrag)=1;
      mouseobj=1;
      Fl::add_timeout(.5, repeat_callback, this);
      delta=0;
      if (Fl::event_inside(sxx,syy,sww,shh/2)) {
        deltadir=1;
      }
      else if (Fl::event_inside(sxx,syy+shh/2,sww,shh/2)) {
        deltadir=-1;
      }
      else {
        deltadir=0;
      }
      increment_cb();
      redraw();
      return 1;
    case FL_DRAG:
      if (mouseobj) {
        mouseobj=0;
        Fl::remove_timeout(repeat_callback, this);
      }
      //    if (!step()) goto DEFAULT;
      olddelta=delta;
      delta = - (Fl::event_y()-iy);
      if ((delta>5) || (delta<-5)  ) {
        deltadir=((olddelta-delta)>0)?-1:(((olddelta-delta)<0)?1:0);
      }
      else {
        deltadir=0; delta = olddelta;
      }
      switch (drag) {
      case 3: v = increment(value(), deltadir*100); break;
      case 2: v = increment(value(), deltadir*10); break;
      default:v = increment(value(), deltadir); break;
      }
      v = round(v);
      handle_drag(soft()?softclamp(v):clamp(v));
      ST(indrag)=1;
      return 1;
    case FL_RELEASE:
      if (mouseobj) {
        Fl::remove_timeout(repeat_callback, this);
      }
      //    if (!step()) goto DEFAULT;
      ST(indrag)=0;
      delta=0;
      deltadir=0;
      mouseobj=0;
      handle_release();
      redraw();
      return 1;
    case FL_FOCUS:
      ST(indrag)=0;
      return input.take_focus();
    default:
      ST(indrag)=0;
      input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
      return 1;
    }
}

Fl_Value_Input_Spin::~Fl_Value_Input_Spin(void)
{
    Fl::remove_timeout(repeat_callback, this);
}

Fl_Value_Input_Spin::Fl_Value_Input_Spin(CSOUND * cs, int x, int y,
                                         int w, int h, const char* l)
  : Fl_Valuator(x,y,w,h,l), input(x, y, w, h, 0)
{
    csound = cs;
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    soft_ = 0;
    //  if (input.parent())  // defeat automatic-add
    //    ((Fl_Group*)input.parent())->remove(input);
    //  input.parent(this); // kludge!
    this->parent()->add(input);
    input.callback(input_cb, this);
    input.when(FL_WHEN_CHANGED);
    selection_color(input.selection_color());
    align(FL_ALIGN_LEFT);
    box(input.box());
    value_damage();
    buttonssize(15);
    ix       = x;
    iy       = y;
    drag     = 0;
    ST(indrag)   = 0;
    ST(sldrag)   = 0;
    mouseobj = 0;
    deltadir = 0;
    delta    = 0;
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Value_Slider_Input.cpp

void Fl_Value_Slider_Input::input_cb(Fl_Widget*, void* v) {
    Fl_Value_Slider_Input& t = *(Fl_Value_Slider_Input*)v;
    CSOUND *csound = t.csound;
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    double nv;
    if (t.step()>=1.0) nv = strtol(t.input.value(), 0, 0);
    else nv = csound->strtod((char*)t.input.value(), 0);
    ST(hack_o_rama2) = 1;
    t.handle_push();
    t.handle_drag(nv);
    t.handle_release();
    ST(hack_o_rama2) = 0;
}

void Fl_Value_Slider_Input::draw(void)
{
    int sxx = x(), syy = y(), sww = w(), shh = h();
    //int bww = w();
    int X = x(), Y = y(), W = w(), H = h();

    int border_size=Fl::box_dx(box());

    if (horizontal()) {
      //bww = textboxsize();
      sxx += textboxsize(); sww -= textboxsize();
      input.resize(X,Y,W-sww,shh);
    }
    else {
      fl_font(input.textfont(), input.textsize());
      syy += fl_height()+(border_size+1)*2; shh -= fl_height()+(border_size+1)*2;
      input.resize(X,Y,W,H-shh);
    }
    if (damage()&~FL_DAMAGE_CHILD)  input.clear_damage(FL_DAMAGE_ALL);
    input.box(box());
    input.color(FL_WHITE, selection_color());
    input.redraw();
    input.resize(X,Y,W,H);
    input.clear_damage();
    //  if (horizontal())   input.draw();
    clear_damage();
    draw_box(box(),sxx,syy,sww,shh,color());
    sxx+=border_size;
    syy+=border_size;
    sww-=border_size*2;
    shh-=border_size*2;
    if (border_size<2) {
      sxx++;
      syy++;
      sww--;
      shh--;
    }
    Fl_Slider::draw(sxx,syy,sww,shh);
}

void Fl_Value_Slider_Input::resize(int X, int Y, int W, int H)
{
    input.resize(X,Y,W,H);
    Fl_Value_Slider::resize(X,Y,W,H);
}

void Fl_Value_Slider_Input::value_damage()
{
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (ST(hack_o_rama2)) return;
    char buf[128];
    format(buf);
    input.value(buf);
    input.mark(input.position()); // turn off selection highlight
}

int Fl_Value_Slider_Input::handle(int event)
{
    int mx = Fl::event_x();
    int my = Fl::event_y();
    int sxx = x(), syy = y(), sww = w(), shh = h();
    int border_size=Fl::box_dx(box());
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (horizontal()) {
      sxx += textboxsize(); sww -= textboxsize();
    }
    else {
      fl_font(input.textfont(), input.textsize());
      syy += fl_height()+(border_size+1)*2; shh -= fl_height()+(border_size+1)*2;
    }
    if ( !ST(indrag) && ( !ST(sldrag) || !((mx>=sxx && mx<=(sxx+sww)) &&
                                           (my>=syy && my<=(syy+shh))))  ) {
      ST(indrag)=0;
      switch(event) {
      case FL_PUSH:
      case FL_DRAG:
        ST(sldrag)=1;
        break;
      case FL_FOCUS:
        input.take_focus();
        break;
      case FL_UNFOCUS:
        redraw();
        break;
      default:
        ST(sldrag)=0;
      }
      input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
      return input.handle(event);
    }
    switch (event) {
    case FL_PUSH:
      ST(ix) = mx;
      ST(drag) = Fl::event_button();
      ST(indrag)=1;
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    case FL_DRAG:
      ST(indrag)=1;
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    case FL_RELEASE:
      //   if (!step()) goto DEFAULT;
      if (value() != previous_value() || !Fl::event_is_click())
        handle_release();
      else {
        input.handle(FL_PUSH);
        input.handle(FL_RELEASE);
      }
      ST(indrag)=0;
      return 1;
    case FL_FOCUS:
      ST(indrag)=0;
      input.take_focus();
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    default:
      ST(indrag)=0;
      input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
      input.handle(event);
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    }
}

Fl_Value_Slider_Input::Fl_Value_Slider_Input(CSOUND *cs, int x, int y,
                                             int w, int h, const char* l)
  : Fl_Value_Slider(x,y,w,h,l),input(x, y, w, h, 0)
{
    csound = cs;
    soft_ = 0;
    // if (input.parent())  // defeat automatic-add
    // ((Fl_Group*)input.parent())->remove(input);
    // input.parent(this); // kludge!
    // input.callback(input_cb, this);
    parent()->add(input);
    //  input.when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY);
    input.when(FL_WHEN_CHANGED );
    selection_color(input.selection_color());
    input.align(FL_ALIGN_LEFT);
    align(FL_ALIGN_LEFT);
    textboxsize(35);
    value_damage();
}

// ---- IV - Aug 23 2002 ---- end of included files

static char *GetString(CSOUND *csound, MYFLT *pname, int is_string);

//---------------

static void set_butbank_value(Fl_Group *o, MYFLT value)
{
    int       childr = o->children();
    Fl_Button *b;
    int       ival = (int) value, j, label;

    if (ival < 0 || ival >= childr || (MYFLT) ival != value)
      return;
    for (j = 0; j < childr; j++) {
      b = (Fl_Button *) o->child(j);
      label = atoi(b->label());
      if (label == ival) {
        b->value(1);
        b->do_callback();
      }
      else
        b->value(0);
    }
}

inline void FLlock() { //gab
    Fl::lock();
}

inline void FLunlock() { //gab
    Fl::unlock();
    Fl::awake((void*) 0);
}

SNAPSHOT::SNAPSHOT (vector<ADDR_SET_VALUE>& valuators, int snapGroup)
{ // the constructor captures current values of all widgets
  // by copying all current values from "valuators" vector (AddrSetValue)
  // to the "fields" vector
    is_empty = 1;
    FLlock(); //<=================
    int i,k;
    int vsize  = valuators.size();
    for (i = 0, k = 0; i < vsize && k < vsize; i++, k++) {
      while (valuators[k].group != snapGroup ) {
        k++;
        if (UNLIKELY(k >=vsize)) goto err;
      }
      ADDR_SET_VALUE& v  = valuators[k];
      CSOUND *csound = (CSOUND*) (((OPDS *) (v.opcode))->insdshead->csound);
      string  opcode_name, widg_name;
      fields.resize(i+1);
      if ((int) fields.size() < i+1)
        fields.resize(i+1);
      VALUATOR_FIELD* fld = &(fields[i]);
      float val, min, max;
      opcode_name = fld->opcode_name = ((OPDS *) (v.opcode))->optext->t.opcod;
      if (UNLIKELY(opcode_name.c_str() == NULL))
        {
          csound->InitError(csound, Str("Invalid snapshot. Perhaps you modified "
                                        "orchestra widget code after you saved "
                                        "the snapshot bank."));
          goto err;
        }
      else if (opcode_name == "FLslider") {
        FLSLIDER *p = (FLSLIDER *) (v.opcode);
        fld->widg_name = p->name->data;
        fld->exp = (int) *p->iexp;
        min = fld->min = *p->imin; max = fld->max = *p->imax;
        switch (fld->exp) {
        case LIN_:
        case EXP_:
          val = *p->kout;
          if (min < max) {
            if (val < min) val=min;
            else if(val>max) val=max;
          }
          else {
            if (val < max) val=max;
            else if(val>min) val=min;
          }
          break;
        default: val = ((Fl_Valuator *)v.WidgAddress)->value();
        }
        fld->value = val;
      }
      else if (opcode_name == "FLslidBnk" || opcode_name == "FLvslidBnk") {
        FLSLIDERBANK *p = (FLSLIDERBANK *) (v.opcode);
        fld->widg_name = ((STRINGDAT *)p->names)->data;
        int numsliders = (int) *p->inumsliders;
        fld->sldbnk = p->slider_data;
        //       fld->sldbnkValues = new MYFLT[numsliders];
        //       ST(allocatedStrings).push_back((char *) fld->sldbnkValues);
        fld->exp = numsliders; // EXCEPTIONAL CASE! fld->exp contains the number
        // of sliders and not the exponential flag
        for (int j =0; j < numsliders; j++) {
          switch (fld->sldbnk[j].exp) {
          case LIN_: case EXP_:
            val = *fld->sldbnk[j].out;
            min = fld->sldbnk[j].min; max = fld->sldbnk[j].max;
            if (min < max) {
              if (val < min) val=min;
              else if(val>max) val=max;
            }
            else {
              if (val < max) val=max;
              else if(val>min) val=min;
            }
            break;
          default:
            val = ((Fl_Valuator *) ((Fl_Group*)v.WidgAddress)->
                   child(j))->value();
          }
          fld->set_sldbnk(j, val);
        }
      }
      else if (opcode_name == "FLslidBnk2" || opcode_name == "FLvslidBnk2") {
        FLSLIDERBANK2 *p = (FLSLIDERBANK2 *) (v.opcode);
        fld->widg_name = ((STRINGDAT *)p->names)->data;
        int numsliders = (int) *p->inumsliders;
        fld->sldbnk = p->slider_data;
        // EXCEPTIONAL CASE! fld->exp contains the number of sliders
        // and not the exponential flag
        fld->exp = numsliders;
        for (int j =0; j < numsliders; j++) {
          switch (fld->sldbnk[j].exp) {
          case LIN_: case EXP_:
            val = *fld->sldbnk[j].out;
            min = fld->sldbnk[j].min; max = fld->sldbnk[j].max;
            if (min < max) {
              if (val < min) val=min;
              else if(val>max) val=max;
            }
            else {
              if (val < max) val=max;
              else if(val>min) val=min;
            }
            break;
          default:
            val = ((Fl_Valuator *) ((Fl_Group*) v.WidgAddress)->child(j))->value();
          }
          fld->set_sldbnk(j, val);
        }
      }
      else if (opcode_name == "FLknob") {
        FLKNOB *p = (FLKNOB *) (v.opcode);
        fld->widg_name = p->name->data;
        fld->exp = (int) *p->iexp;
        min = fld->min = *p->imin; max = fld->max = *p->imax;
        switch (fld->exp) {
        case LIN_:
        case EXP_:
          val = *p->kout;
          if (min < max) {
            if (val < min) val=min;
            else if(val>max) val=max;
          }
          else {
            if (val < max) val=max;
            else if(val>min) val=min;
          }
          break;
        default: val = ((Fl_Valuator *)v.WidgAddress)->value();
        }
        fld->value = val;
      }
      else if (opcode_name == "FLroller") {
        FLROLLER *p = (FLROLLER *) (v.opcode);
        fld->widg_name = p->name->data;
        fld->exp = (int) *p->iexp;
        min = fld->min = *p->imin; max = fld->max = *p->imax;
        switch (fld->exp) {
        case LIN_:
        case EXP_:
          val = *p->kout;
          if (min < max) {
            if (val < min) val=min;
            else if(val>max) val=max;
          }
          else {
            if (val < max) val=max;
            else if(val>min) val=min;
          }
          break;
        default: val = ((Fl_Valuator *)v.WidgAddress)->value();
        }
        fld->value = val;
      }
      else if (opcode_name == "FLtext") {
        FLTEXT *p = (FLTEXT *) (v.opcode);
        fld->widg_name = p->name->data;
        val = *p->kout; min = fld->min = *p->imin; max = fld->max = *p->imax;
        if (min < max) {
          if (val < min) val=min;
          else if(val>max) val=max;
        }
        else {
          if (val < max) val=max;
          else if(val>min) val=min;
        }
        fld->value = val;
        fld->exp = LIN_;
      }
      else if (opcode_name == "FLjoy") {
        FLJOYSTICK *p = (FLJOYSTICK *) (v.opcode);
        fld->widg_name = p->name->data;
        fld->exp  = (int) *p->iexpx;
        min = fld->min = *p->iminx; max = fld->max = *p->imaxx;
        switch (fld->exp) {
        case LIN_:
        case EXP_:
          val = *p->koutx;
          if (min < max) {
            if (val < min) val=min;
            else if(val>max) val=max;
          }
          else {
            if (val < max) val=max;
            else if(val>min) val=min;
          }
          break;
        default:
          val = ((Fl_Positioner *)v.WidgAddress)->xvalue();
        }
        fld->value = val;
        fld->exp2 = (int) *p->iexpy;
        min = fld->min2 = *p->iminy; max = fld->max2 = *p->imaxy;
        switch (fld->exp2) {
        case LIN_: case EXP_:
          val = *p->kouty;
          if (min < max) {
            if (val < min) val=min;
            else if(val>max) val=max;
          }
          else {
            if (val < max) val=max;
            else if(val>min) val=min;
          }
          break;
        default:
          val = ((Fl_Positioner *)v.WidgAddress)->yvalue();
        }
        fld->value2 =val;
      }
      else if (opcode_name == "FLbutton") {
        FLBUTTON *p = (FLBUTTON *) (v.opcode);
        fld->widg_name =  p->name->data;
        fld->value = *p->kout;
        fld->min = 0; fld->max = 1; fld->exp = LIN_;
      }
      else if (opcode_name == "FLbutBank") {
        FLBUTTONBANK *p = (FLBUTTONBANK *) (v.opcode);
        fld->widg_name = Str("No name for FLbutbank");
        //fld->widg_name = GetString(csound, p->name, p->XSTRCODE);
        fld->value = *p->kout;
        fld->min = 0; fld->max = 1; fld->exp = LIN_;
      }
      else if (opcode_name == "FLcount") {
        FLCOUNTER *p = (FLCOUNTER *) (v.opcode);
        fld->widg_name = p->name->data;
        val = *p->kout; min = *p->imin; max =*p->imax;
        if (min != max) {
          if (val < min) val=min;
          else if (val>max) val=max;
        }
        fld->value = val;
        fld->min = *p->imin; fld->max = *p->imax; fld->exp = LIN_;
      }
      else if (opcode_name == "FLvalue") {
        FLVALUE *p = (FLVALUE *) (v.opcode);
        fld->widg_name = p->name->data;
      }
      else if (opcode_name == "FLbox") {
        FL_BOX *p = (FL_BOX *) (v.opcode);
        fld->widg_name = p->itext->data;
      }
    }
 err:
    FLunlock(); //<=================
}

int SNAPSHOT::get(vector<ADDR_SET_VALUE>& valuators, int snapGroup)
{
    if (is_empty == 1) {
      /*  FIXME: should have CSOUND* pointer here */
      /*  return csound->InitError(csound, Str("empty snapshot")); */
      return -1;
    }
    FLlock(); //<=================
    int j,k;
    int siz =valuators.size();
    for (j = 0, k = 0; j< siz && k < siz; j++, k++) {
      //     int grp = valuators[k].group; //Not used
      while (valuators[k].group != snapGroup) {
        k++;
        if (k >= (int) valuators.size()) goto end_func;
      }
      Fl_Widget* o = (Fl_Widget*) (valuators[k].WidgAddress);
      void *opcode = valuators[k].opcode;
//CSOUND *csound = (CSOUND*) (((OPDS*) opcode)->insdshead->csound); //Not used
      VALUATOR_FIELD* fld = &fields[j];
      string opcode_name = fld->opcode_name;

      MYFLT val = fld->value, valtab = fld->value, min=fld->min,
        max=fld->max, range,base;
      if (min < max) {
        if (val < min) val = min;
        else if (val >max) val = max;
      }
      else {
        if (val < max) val=max;
        else if(val>min) val=min;
      }

      if (opcode_name == "FLjoy") {
        switch(fld->exp) {
        case LIN_:
          ((Fl_Positioner*) o)->xvalue(val);
          break;
        case EXP_:
          range  = fld->max - fld->min;
          #if defined(sun)
            base = ::pow(fld->max / (double)fld->min, 1.0/(double)range);
          #else
            base = ::pow(fld->max / fld->min, 1.0/(double)range);
          #endif
          ((Fl_Positioner*) o)->xvalue(log(val/fld->min) / log(base)) ;
          break;
        default:
          ((Fl_Positioner*) o)->xvalue(valtab);
          break;
        }
        val = fld->value2; min = fld->min2; max = fld->max2;
        //       if (val < min) val = min;
        //       else if (val >max) val = max;
        switch(fld->exp2) {
        case LIN_:
          ((Fl_Positioner*) o)->yvalue(val);
          break;
        case EXP_:
          range  = fld->max2 - fld->min2;
          #if defined(sun)
            base = ::pow(fld->max2 / (double)fld->min2, 1.0/(double)range);
          #else
            base = ::pow(fld->max2 / fld->min2, 1.0/(double)range);
          #endif
          ((Fl_Positioner*) o)->yvalue(log(val/fld->min2) / log(base)) ;
          break;
        default:
          ((Fl_Positioner*) o)->yvalue(valtab);
          break;
        }
        o->do_callback(o, opcode);
      }
      else if (opcode_name == "FLbutton") {
        FLBUTTON *p = (FLBUTTON*) (opcode);
        //  don't allow to retreive its value if >= 10 or between 21 and 29
        if ((*p->itype < 10) || (*p->itype < 30 && *p->itype > 20)) {
          if(fld->value >= *p->ioff - 0.0001 &&
             fld->value <= *p->ioff + 0.0001)  // to avoid eventual  math rounding
            ((Fl_Button*) o)->value(0);
          else
            if (fld->value >= *p->ion - 0.0001 &&
                fld->value <= *p->ion + 0.0001) // to avoid eventual math rounding
              ((Fl_Button*) o)->value(1);
            else
              ((Fl_Button*) o)->value((int)fld->value);
          o->do_callback(o, opcode);
        }
      }
      else if (opcode_name == "FLbutBank") {
        FLBUTTONBANK *p = (FLBUTTONBANK*) (opcode);
        if (*p->itype < 10 || (*p->itype < 30 && *p->itype > 20)) {
          //  don't allow to retreive its value if >= 10
          //((Fl_Group*) o)->value(fld->value);
          set_butbank_value((Fl_Group*) o, fld->value);
          //o->do_callback(o, opcode);
          *p->kout=fld->value;
          if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                            p->args, p->INOCOUNT-7);
        }
      }
      else if (opcode_name == "FLcount") {
        FLCOUNTER *p = (FLCOUNTER*) (opcode);
        if (*p->itype < 10 || (*p->itype < 30 && *p->itype > 20)) {
          //  don't allow to retreive its value if >= 10
          ((Fl_Counter*) o)->value(fld->value);
          o->do_callback(o, opcode);
        }
      }
      else if (opcode_name == "FLslidBnk" || opcode_name == "FLvslidBnk") {
        FLSLIDERBANK *p = (FLSLIDERBANK*) (opcode);
        int numsliders = (int) *p->inumsliders;
        Fl_Group * grup = (Fl_Group *) o;
        for (int j =0; j < numsliders; j++) {
          MYFLT val = fld->get_sldbnk(j);
          switch (p->slider_data[j].exp) {
          case LIN_:
            ((Fl_Valuator *) grup->child(j))->value(val);
            break;
          case EXP_:
            range  = p->slider_data[j].max - p->slider_data[j].min;
            #if defined(sun)
              base = ::pow(p->slider_data[j].max / (double)p->slider_data[j].min,
                           1.0/(double)range);
            #else
              base = ::pow(p->slider_data[j].max / p->slider_data[j].min,
                           1.0/(double)range);
            #endif
            ((Fl_Valuator*) grup->child(j))->
              value(log(val/p->slider_data[j].min) / log(base)) ;
            break;
          default:// TABLE the value must be in the 0 to 1 range...
            val = (val - fld->min ) / (fld->max - fld->min);
            ((Fl_Valuator *) grup->child(j))->value(val);
            break;
          }
          grup->child(j)->do_callback( grup->child(j),
                                       (void *) &(p->slider_data[j]));
        }
      }
      else if (opcode_name == "FLslidBnk2" || opcode_name == "FLvslidBnk2") {
        FLSLIDERBANK2 *p = (FLSLIDERBANK2*) (opcode);
        int numsliders = (int) *p->inumsliders;
        Fl_Group * grup = (Fl_Group *) o;
        for (int j =0; j < numsliders; j++) {
          MYFLT val = fld->get_sldbnk(j);
          switch (p->slider_data[j].exp) {
          case LIN_:
            ((Fl_Valuator *) grup->child(j))->value(val);
            break;
          case EXP_:
            range  = p->slider_data[j].max - p->slider_data[j].min;
            base = pow(p->slider_data[j].max / p->slider_data[j].min, 1/range);
            ((Fl_Valuator*) grup->child(j))->value(log(val/p->slider_data[j].min) /
                                                   log(base)) ;
            break;
          default:// TABLE the value must be in the 0 to 1 range...
            val = (val - fld->min ) / (fld->max - fld->min);
            ((Fl_Valuator *) grup->child(j))->value(val);
            break;
          }
          grup->child(j)->do_callback( grup->child(j),
                                       (void *) &(p->slider_data[j]));
        }
      }
      else {
        switch(fld->exp) {
        case LIN_:
          if (opcode_name == "FLbox" || opcode_name == "FLvalue" ) continue;
          else if (opcode_name == "FLtext" &&
                   *((FLTEXT *)opcode)->itype == 1) {
            ((Fl_Valuator*) o)->value(val);
            continue;
          }
          ((Fl_Valuator*) o)->value(val);
          break;
        case EXP_:
          range  = fld->max - fld->min;
          #if defined(sun)
            base = ::pow(fld->max / (double)fld->min, 1.0/(double)range);
          #else
            base = ::pow(fld->max / fld->min, 1.0/(double)range);
          #endif
          ((Fl_Valuator*) o)->value(log(val/fld->min) / log(base)) ;
          break;
        default: // TABLE the value must be in the 0 to 1 range...
          ((Fl_Valuator*) o)->value(valtab);
          break;
        }
        o->do_callback(o, opcode);
      }
    }
 end_func:
    FLunlock(); //<=================
    return OK;
}

extern "C" {

  static int set_snap(CSOUND *csound, FLSETSNAP *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      SNAPSHOT snap(ST(AddrSetValue), (int) *p->group );
      int numfields = snap.fields.size();
      int index = (int) *p->index;
      int group = (int) *p->group;
      SNAPVEC snapvec_init;
      SNAPSHOT snap_init;

      snap_init.fields.resize(1,VALUATOR_FIELD());
      snapvec_init.resize(1,snap_init);
      if (group+1 > (int) ST(snapshots).size())
        ST(snapshots).resize(group+1, snapvec_init);
      // *p->inum_snap = ST(snapshots).size();
      *p->inum_val = numfields; // number of snapshots
      if (*p->ifn >= 1) { // if the table number is valid
        FUNC    *ftp;   // store the snapshot into the table
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL)) {
          MYFLT *table = ftp->ftable;
          for (int j = 0; j < numfields; j++) {
            table[index*numfields+j] = snap.fields[j].value;
          }
        }
        else return csound->InitError(csound,
                                      Str("FLsetsnap: invalid table"));
      }
      else { // else store it into snapshot bank
        if ((int) ST(snapshots)[group].size() < index+1)
          ST(snapshots)[group].resize(index+1);
        csound->Message(csound, Str("setsnap saving\n"));
        ST(snapshots)[group][index]=snap;
        *p->inum_snap = ST(snapshots)[group].size();
      }
      return OK;
  }

  static int get_snap(CSOUND *csound, FLGETSNAP *p)
  {
      int index = (int) *p->index;
      int group = (int) *p->group;
      SNAPVEC snapvec_init;
      SNAPSHOT snap_init;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      snap_init.fields.resize(1,VALUATOR_FIELD());
      snapvec_init.resize(1,snap_init);
      if (group+1 > (int) ST(snapshots).size())
        ST(snapshots).resize(group+1, snapvec_init);
      if (!ST(snapshots)[group].empty()) {
        if (index >= (int) ST(snapshots)[group].size())
          index = ST(snapshots)[group].size()-1;
        else if (index < 0) index=0;
        if (ST(snapshots)[group][index].get(ST(AddrSetValue), (int) *p->group)!=OK)
          return NOTOK;
      }
      *p->inum_el = ST(snapshots)[group].size();
      return OK;
  }

  static int save_snap(CSOUND *csound, FLSAVESNAPS *p)
  {
      char    s[MAXNAME], *s2;
      string  filename;
#ifdef WIN32
      int id = MessageBox(NULL, Str("Saving could overwrite the old file.\n"
                                    "Are you sure to save ?"), Str("Warning"),
                          MB_SYSTEMMODAL | MB_ICONWARNING | MB_OKCANCEL);
      if (id != IDOK)
        return OK;
#else
#  if !defined(NO_FLTK_THREADS)
      if (!((getFLTKFlags(csound) & 260) ^ 4))
#  endif
        {
          int   n;
          Fl_lock(csound);
          n = fl_choice("%s", Str("Saving could overwrite the old file.\n"
                                  "Are you sure you want to save ?"),
                        Str("No"), Str("Yes"), (const char*)""); // used to be NULL
          Fl_unlock(csound);
          if (!n)
            return OK;
        }
#endif
      csound->strarg2name(csound, s, p->filename->data, "snap.", 1);
      s2 = csound->FindOutputFile(csound, s, "SNAPDIR");
      if (UNLIKELY(s2 == NULL))
        return csound->InitError(csound,
                                 Str("FLsavesnap: cannot open file"));
      strncpy(s, s2, MAXNAME-1);
      csound->Free(csound, s2);
      filename = s;

      fstream file(filename.c_str(), ios::out);
      int group = (int) *p->group;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      for (int j =0; j < (int) ST(snapshots)[group].size(); j++) {
        file << "----------- "<< j << " -----------\n";
        int siz = ST(snapshots)[group][j].fields.size();
        for ( int k = 0; k < siz; k++) {
          VALUATOR_FIELD& f = ST(snapshots)[group][j].fields[k];
          //      if (f.group != group) continue;
          if (f.opcode_name == "FLjoy") {
            file <<f.opcode_name<<" "<< f.value <<" "<< f.value2
                 <<" "<< f.min <<" "<< f.max <<" "<< f.min2 <<" "
                 << f.max2<<" "<<f.exp<<" "<<f.exp2<<" \"" <<f.widg_name<<"\"\n";
          }
          else if (f.opcode_name == "FLslidBnk"  ||
                   f.opcode_name == "FLvslidBnk" ||
                   f.opcode_name == "FLslidBnk2" ||
                   f.opcode_name == "FLvslidBnk2") {
            file << f.opcode_name << " " << f.exp
                 << " "; // EXCEPTIONAL CASE! fld.exp contains the
            //number of sliders and not the exponential flag
            for (int i=0; i < f.exp; i++) {
              file << f.get_sldbnk(i) << " ";
            }
            file << " \"" << f.widg_name << "\"\n";
          }
          else {
            const char *s = f.opcode_name.c_str();
            if (*s != '\0') {
              file <<f.opcode_name<<" "<< f.value
                   <<" "<< f.min <<" "<< f.max <<" "<<f.exp
                   <<" \"" <<f.widg_name<<"\"\n";
            }
          }
        }
      }
      file << "---------------------------";
      file.close();
      return OK;
  }

  static int load_snap(CSOUND *csound, FLLOADSNAPS* p)
  {
      char     s[MAXNAME], *s2;
      string   filename;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      csound->strarg2name(csound, s, p->filename->data, "snap.", 1);
      s2 = csound->FindInputFile(csound, s, "SNAPDIR");
      if (UNLIKELY(s2 == NULL))
        return csound->InitError(csound,
                                 Str("FLloadsnap: cannot open file"));
      strncpy(s, s2, MAXNAME-1);
      csound->Free(csound, s2);
      filename = s;

      fstream file(filename.c_str(), ios::in);
      int group = (int) *p->group;
      int j=0,k=-1,q=0;
      SNAPVEC snapvec_init;
      SNAPSHOT snap_init;
      snap_init.fields.resize(1,VALUATOR_FIELD());
      snapvec_init.resize(1,snap_init);
      if (group+1 > (int) ST(snapshots).size())
        ST(snapshots).resize(group+1, snapvec_init);
      while (!(file.eof())) {
        char buf[MAXNAME];
        file.getline(buf,MAXNAME-1);

        stringstream sbuf;
        sbuf << buf;

        string opc, opc_orig;
        getline(sbuf, opc, ' ');
        const char *ss = opc.c_str();
        if (*ss == '-') { // if it is a separation line
          k++; j=0; q=0;
          if ((int) ST(snapshots)[group].size() < k+1)
            ST(snapshots)[group].resize(k+1);
        }
        else if (*ss != '\0' && *ss != ' ' && *ss != '\n'){ //ignore blank lines
          ADDR_SET_VALUE* v = &(ST(AddrSetValue)[q]);
          while (ST(AddrSetValue)[q].group != group) {
            q++;
            if (q >= (int) ST(AddrSetValue).size()) continue;
            v = &(ST(AddrSetValue)[q]);
          }
          if (k<0) return NOTOK;
          if ((int) ST(snapshots)[group][k].fields.size() < j+1)
            ST(snapshots)[group][k].fields.resize(j+1);
          ST(snapshots)[group][k].is_empty = 0;
          VALUATOR_FIELD& fld = ST(snapshots)[group][k].fields[j];
          opc_orig = ((OPDS *) (v->opcode))->optext->t.opcod;

          if (UNLIKELY(!(opc_orig == opc))) {
            //return csound->InitError(csound,
            csound->Message(csound,
                            Str("unmatched widget, probably due to a "
                                "modified orchestra. Modifying an "
                                "orchestra makes it incompatible with "
                                "old snapshot files"));
          }
          else if (opc == "FLjoy") {
            fld.opcode_name = opc;
            string s;
            getline(sbuf,s, ' ');  fld.value  = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.value2 = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.min    = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.max    = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.min2   = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.max2   = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.exp    = atoi(s.c_str());
            getline(sbuf,s, ' ');  fld.exp2   = atoi(s.c_str());
            getline(sbuf,s, '\"');
            getline(sbuf,s, '\"');  fld.widg_name = s;
          }
          else if (opc == "FLslidBnk"  || opc == "FLvslidBnk"
                   || opc == "FLslidBnk2" || opc == "FLvslidBnk2" ) {
            fld.opcode_name = opc;
            string s;
            getline(sbuf,s, ' ');
            // EXCEPTIONAL CASE! fld.exp contains the number of sliders
            // and not the exponential flag
            fld.exp = atoi(s.c_str());
            //         fld.sldbnkValues = new MYFLT[fld.exp];
            //                              fld.insert_sldbnk(fld.exp);
            //              allocatedStrings.push_back((char *) fld.sldbnkValues);
            //         ST(allocatedStrings).push_back((char *) fld.sldbnkValues);

            for (int kk =0; kk < fld.exp; kk++) {
              getline(sbuf,s, ' ');
              //           fld.sldbnkValues[kk] = atof(s.c_str());
              fld.set_sldbnk(kk,atof(s.c_str()));

            }
            getline(sbuf,s, '\"');
            getline(sbuf,s, '\"');  fld.widg_name = s;
          }
          else {
            fld.opcode_name = opc;
            string s;
            getline(sbuf,s, ' ');  fld.value = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.min = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.max = atof(s.c_str());
            getline(sbuf,s, ' ');  fld.exp = atoi(s.c_str());
            getline(sbuf,s, '\"');
            getline(sbuf,s, '\"');  fld.widg_name = s;
          }
          j++; q++;
        }
      }

      file.close();
      return OK;
  }

  static int fltkKeyboardCallback(void *, void *, unsigned int);

}       // extern "C"

// -----------

static char *GetString(CSOUND *csound, MYFLT *pname, int is_string)
{
    char    *Name = new char[MAXNAME];
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    ST(allocatedStrings).push_back(Name);
    return csound->strarg2name(csound, Name, pname, "", is_string);
}

class CsoundFLTKKeyboardBuffer {
private:
  CSOUND  *csound;
  WIDGET_GLOBALS *widgetGlobals;
  void    *mutex_;
  char    kbdTextBuf[64];
  int     kbdEvtBuf[64];
  int     kbdTextBufRPos;
  int     kbdTextBufWPos;
  int     kbdEvtBufRPos;
  int     kbdEvtBufWPos;
  std::map<int, unsigned char> keyboardState;
  void lockMutex()
  {
      if (mutex_)
        csound->LockMutex(mutex_);
  }
  void unlockMutex()
  {
      if (mutex_)
        csound->UnlockMutex(mutex_);
  }
public:
  CsoundFLTKKeyboardBuffer(CSOUND *csound)
  {
      this->csound = csound;
      widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      mutex_ = csound->Create_Mutex(0);
      kbdTextBufRPos = 0;
      kbdTextBufWPos = 0;
      kbdEvtBufRPos = 0;
      kbdEvtBufWPos = 0;
  }
  ~CsoundFLTKKeyboardBuffer()
  {
      if (mutex_) {
        csound->DestroyMutex(mutex_);
        mutex_ = (void*) 0;
      }
  }
  CSOUND *GetCsound()
  {
      return csound;
  }
  void writeFLEvent(int evt)
  {
      const char  *s;
      int     keyCode;
      keyCode = (int) Fl::event_key() & (int) 0xFFFF;
      if (keyCode) {
        lockMutex();
        if (evt == FL_KEYDOWN) {
          s = Fl::event_text();
          while (*s != (char) 0) {
            kbdTextBuf[kbdTextBufWPos] = *(s++);
            kbdTextBufWPos = (kbdTextBufWPos + 1) & 63;
          }
          if (keyboardState[keyCode] == (unsigned char) 0) {
            keyboardState[keyCode] = (unsigned char) 1;
            kbdEvtBuf[kbdEvtBufWPos] = keyCode;
            kbdEvtBufWPos = (kbdEvtBufWPos + 1) & 63;
          }
        }
        else if (keyboardState[keyCode] != (unsigned char) 0) {
          keyboardState[keyCode] = (unsigned char) 0;
          kbdEvtBuf[kbdEvtBufWPos] = keyCode | (int) 0x10000;
          kbdEvtBufWPos = (kbdEvtBufWPos + 1) & 63;
        }
        unlockMutex();
      }
  }
  int getKeyboardText()
  {
      int     retval = 0;
      lockMutex();
      if (kbdTextBufRPos != kbdTextBufWPos) {
        retval = (int) ((unsigned char) kbdTextBuf[kbdTextBufRPos]);
        kbdTextBufRPos = (kbdTextBufRPos + 1) & 63;
      }
      unlockMutex();
      return retval;
  }
  int getKeyboardEvent()
  {
      int     retval = 0;
      lockMutex();
      if (kbdEvtBufRPos != kbdEvtBufWPos) {
        retval = kbdEvtBuf[kbdEvtBufRPos];
        kbdEvtBufRPos = (kbdEvtBufRPos + 1) & 63;
      }
      unlockMutex();
      return retval;
  }
};

class CsoundFLWindow : public Fl_Double_Window {
public:
  CSOUND *csound_; //gab
  WIDGET_GLOBALS *widgetGlobals;
  CsoundFLTKKeyboardBuffer  fltkKeyboardBuffer;
  CsoundFLWindow(CSOUND *csound, int w, int h, const char *title = 0)
    : Fl_Double_Window(w, h, title),
      csound_(csound),
      fltkKeyboardBuffer(csound)//gab
  {
      widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      csound->Set_KeyCallback(csound, fltkKeyboardCallback, (void*) this,
                              CSOUND_CALLBACK_KBD_EVENT | CSOUND_CALLBACK_KBD_TEXT);
  }
  CsoundFLWindow(CSOUND *csound,
                 int x, int y, int w, int h, const char *title = 0)
    : Fl_Double_Window(x, y, w, h, title),
      csound_(csound),
      fltkKeyboardBuffer(csound)//gab
  {
      widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      csound->Set_KeyCallback(csound, fltkKeyboardCallback, (void*) this,
                              CSOUND_CALLBACK_KBD_EVENT | CSOUND_CALLBACK_KBD_TEXT);
  }
  virtual ~CsoundFLWindow()
  {
      CSOUND  *csound = fltkKeyboardBuffer.GetCsound();
      csound->Remove_KeyCallback(csound, fltkKeyboardCallback);
  }
  virtual int handle(int evt)
  {
      CSOUND* csound = csound_; //gab
      switch (evt) {
      case FL_FOCUS:
        Fl::focus(this);
      case FL_UNFOCUS:
        return 1;
      case FL_KEYDOWN:
        ST(last_KEY) = Fl::event_key(); //gab
        ST(isKeyDown) = true;  //gab
        break;
      case FL_KEYUP:
        ST(last_KEY) = Fl::event_key(); //gab
        ST(isKeyDown) = false; //gab
        if (Fl::focus() == this)
          fltkKeyboardBuffer.writeFLEvent(evt);
        break;
      }
      return Fl_Window::handle(evt);
  }
};

extern "C"
{
  static int fltkKeyboardCallback(void *userData, void *p, unsigned int type)
  {
      switch (type) {
      case CSOUND_CALLBACK_KBD_EVENT:
        *((int*) p) =
          ((CsoundFLWindow*) userData)->fltkKeyboardBuffer.getKeyboardEvent();
        break;
      case CSOUND_CALLBACK_KBD_TEXT:
        *((int*) p) =
          ((CsoundFLWindow*) userData)->fltkKeyboardBuffer.getKeyboardText();
        break;
      default:
        return 1;
      }
      return 0;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
      int j;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
#ifndef NO_FLTK_THREADS
      int *fltkflags = getFLTKFlagsPtr(csound);
      if (fltkflags) {
        if ((*fltkflags & 260) ^ 4) {
          widgetsGlobals_t *p =
            (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                            "_widgets_globals");
          if (p) {
            if (!(*fltkflags & 256)) {
              /* if window(s) still open: */
              if (!p->exit_now) {
                /* notify GUI thread... */
                p->end_of_perf = -1;
                Fl_lock(csound);
                Fl_awake(csound);
                Fl_unlock(csound);
                /* ...and wait for it to close */
                csound->JoinThread(p->threadHandle);
                p->threadHandle = NULL;
              }
            }
            /* clean up */
            csound->LockMutex(p->mutex_);
            while (p->eventQueue != NULL) {
              rtEvt_t *nxt = p->eventQueue->nxt;
              free(p->eventQueue);
              p->eventQueue = nxt;
            }
            csound->UnlockMutex(p->mutex_);
            csound->DestroyMutex(p->mutex_);
            csound->DestroyGlobalVariable(csound, "_widgets_globals");
          }
        }
      }
#endif  // NO_FLTK_THREADS
      if(widgetGlobals != NULL) {
        for (j = ST(allocatedStrings).size() - 1; j >= 0; j--)  {
          delete[] ST(allocatedStrings)[j];
          ST(allocatedStrings).pop_back();
        }
        j = ST(fl_windows).size();
        if (j > 0) {
          // destroy all opened panels
          do {
            j--;
            if (ST(fl_windows)[j].is_subwindow == 0)
              delete ST(fl_windows)[j].panel;
            ST(fl_windows).pop_back();
          } while (j);
          Fl_wait_locked(csound, 0.0);
        }
        for (size_t si = 0, sn = ST(snapshots).size(); si < sn; ++si) {
          SNAPVEC &svec = ST(snapshots)[si];
          int ss = svec.size();
          for (j = 0; j < ss; j++) {
            svec[j].fields.erase(svec[j].fields.begin(),
                                 svec[j].fields.end());
            svec.resize(svec.size() + 1);
          }
        }
        ST(AddrSetValue).clear();
        ST(stack_count)       = 0;
        ST(FLcontrol_iheight) = 15;
        ST(FLroller_iheight)  = 18;
        ST(FLcontrol_iwidth)  = 400;
        ST(FLroller_iwidth)   = 150;
        ST(FLvalue_iwidth)    = 100;
        ST(FLcolor)           = -1;
        ST(FLcolor2)          = -1;
        ST(FLtext_size)       = 0;
        ST(FLtext_color)      = -1;
        ST(FLtext_font)       = -1;
        ST(FLtext_align)      = 0;
        ST(FL_ix)             = 10;
        ST(FL_iy)             = 10;

        //delete (WIDGET_GLOBALS*)csound->widgetGlobals;
        csound->DestroyGlobalVariable(csound, "WIDGET_GLOBALS");
        //csound->widgetGlobals = NULL;
      }
      return 0;
  }
} // extern "C"

//-----------

#ifndef NO_FLTK_THREADS

extern "C" {

  static uintptr_t fltkRun(void *userdata)
  {
      volatile widgetsGlobals_t *p;
      CSOUND    *csound = (CSOUND*) userdata;
      int       j;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      p = (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                          "_widgets_globals");
#ifdef LINUX
      {
        struct sched_param  sp;
        // IV - Aug 27 2002: widget thread is always run with normal priority
        memset(&sp, 0, sizeof(struct sched_param));
        pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);
      }
#endif

      if (!(p->fltkFlags & 8))
        Fl::lock();
      for (j = 0; j < (int) ST(fl_windows).size(); j++) {
        ST(fl_windows)[j].panel->show();
      }
#ifdef CS_VSTHOST
      for (size_t k=0; k < ST(VSTplugEditors).size(); k++) {
        int panelNum = ST(VSTplugEditors)[k]->targetFLpanel;
#ifdef WIN32
        HWND xid = fl_xid(ST(fl_windows)[panelNum].panel);
        ST(VSTplugEditors)[k]->SetEditWindow(xid);
#elif defined (LINUX) || defined(MACOSX)
        // put some appropriate alternative code here
        Fl_Window * xid = fl_find(fl_xid(ST(fl_windows)[panelNum].panel));
        ST(VSTplugEditors)[k]->SetEditWindow(xid);
#endif  // WIN32
      }
#endif  // CS_VSTHOST
      if (!(p->fltkFlags & 16))
        Fl::awake();
      if (!(p->fltkFlags & 8))
        Fl::unlock();
      do {
        if (!(p->fltkFlags & 8))
          Fl::lock();
        Fl::wait(0.02);
        j = (Fl::first_window() != (Fl_Window*) 0);
        if (!(p->fltkFlags & 8))
          Fl::unlock();
      } while (j && !p->end_of_perf);
      csound->Message(csound, Str("end of widget thread\n"));
      // IV - Jun 07 2005: exit if all windows are closed
      p->exit_now = -1;
      return (uintptr_t) 0;
  }

}   // extern "C"

#endif  // NO_FLTK_THREADS

extern "C" {

  int CsoundYield_FLTK(CSOUND *csound);

  int FL_run(CSOUND *csound, FLRUN *p)
  {
      int     *fltkFlags;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      fltkFlags = getFLTKFlagsPtr(csound);
      (*fltkFlags) |= 32;
#ifndef NO_FLTK_THREADS
      if (((*fltkFlags) & 260) ^ 4) {
        widgetsGlobals_t  *pp;

        if (UNLIKELY(csound->QueryGlobalVariable(csound,
                                                 "_widgets_globals") != NULL))
          return csound->InitError(csound, Str("FLrun was already called"));
        if (UNLIKELY(csound->CreateGlobalVariable(csound, "_widgets_globals",
                                                  sizeof(widgetsGlobals_t)) != 0))
          csound->Die(csound, Str("FL_run: memory allocation failure"));
        pp = (widgetsGlobals_t*) csound->QueryGlobalVariable(csound,
                                                             "_widgets_globals");
        pp->fltkFlags = *fltkFlags;
        /* create thread lock */
        pp->mutex_ = csound->Create_Mutex(0);
        /* register callback function to be called by sensevents() */
        csound->RegisterSenseEventCallback(csound, (void (*)(CSOUND *, void *))
                                           evt_callback,
                                           (void*) pp);
        if (!((*fltkFlags) & 256)) {
          pp->threadHandle = csound->CreateThread(fltkRun, (void*) csound);
          return OK;
        }
      }
#endif  // NO_FLTK_THREADS
      {
        int j;

        Fl_lock(csound);
        for (j = 0; j < (int) ST(fl_windows).size(); j++) {
          ST(fl_windows)[j].panel->show();
        }
        Fl_wait(csound, 0.0);
        Fl_unlock(csound);


        if (!((*fltkFlags) & 256))
          csound->SetInternalYieldCallback(csound, CsoundYield_FLTK);
      }
      return OK;
  }

  int fl_update(CSOUND *csound, FLRUN *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      Fl_lock(csound);
      for (int j=0; j< (int) ST(AddrSetValue).size()-1; j++) {
        ADDR_SET_VALUE v = ST(AddrSetValue)[j];
        Fl_Valuator *o = (Fl_Valuator *) v.WidgAddress;
        o->do_callback(o, v.opcode);
      }
      Fl_unlock(csound);
      return OK;
  }
}       // extern "C"

//----------------------------------------------

static inline void displ(MYFLT val, MYFLT index, CSOUND *csound)
{
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (index >= 0) {     // display current value of valuator
      char valString[MAXNAME];
      sprintf(valString, "%.5g", val);
      ((Fl_Output*) (ST(AddrSetValue)[(long) index]).WidgAddress)->
        value(valString);
    }
}

static void fl_callbackButton1(Fl_Button* w, void *a)
{
    FLBUTTON *p = (FLBUTTON *) a;
    *((FLBUTTON*) a)->kout =  *p->ion;
    if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                      p->args, p->INOCOUNT-8);
}

static void fl_callbackCloseButton(Fl_Button* w, void *a)
{
    Fl_Window *p = (Fl_Window *) a;
    p->hide();
}

static void fl_callbackExecButton(Fl_Button* w, void *a)
{
    FLEXECBUTTON *p = (FLEXECBUTTON *)a;
    CSOUND *csound = p->csound;
    char *command = (char *)csound->Malloc(csound, strlen(p->commandString) + 1);

#if defined(LINUX) || defined (MACOSX)


    pid_t pId = vfork();
    if (pId == 0) {
      char *th;
      char *v[40];
      int i = 0;

      strcpy(command, p->commandString);

      char *tok = csound->strtok_r(command,(char *) " ", &th);

      if(tok != NULL) {
        v[i++] = tok;
        while((tok = csound->strtok_r(NULL,(char *) " ", &th)) != NULL) {
          v[i++] = tok;
        }
        v[i] = NULL;
        execvp(v[0], v);
      }

      _exit(0);
    } else if (UNLIKELY(pId < 0)) {
      p->csound->Message(p->csound,
                         Str("Error: Unable to fork process\n"));
    }

    csound->Free(csound, command);
#elif defined(WIN32)
    {
#undef strtok_r // undefine from pthread.h on Windows
      char *th;
      char *v[40];
      int i = 0;

      strcpy(command, p->commandString);
      char *tok = csound->strtok_r(command,(char *) " ", &th);

      if(tok != NULL) {
        v[i++] = tok;
        while((tok = csound->strtok_r(NULL,(char *) " ", &th)) != NULL) {
          v[i++] = tok;
        }
        v[i] = NULL;
        csound->Free(csound, command); // Otherwise will lose space
        if (UNLIKELY(csound->RunCommand(v, 1)<0))
          p->csound->Message(p->csound, Str("Error: Unable to fork process\n"));
      }
    }
#endif
}

static void fl_callbackButton(Fl_Button* w, void *a)
{
    FLBUTTON *p = (FLBUTTON *) a;
    *((FLBUTTON*) a)->kout =  (w->value()) ? *p->ion : *p->ioff;
    if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                      p->args, p->INOCOUNT-8);
}

static void fl_callbackButtonBank(Fl_Button* w, void *a)
{
    FLBUTTONBANK *p = (FLBUTTONBANK *) a;
    *((FLBUTTONBANK*) a)->kout = (MYFLT) atoi(w->label());
    if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                      p->args, p->INOCOUNT-7);
}

static void fl_callbackCounter(Fl_Counter* w, void *a)
{
    FLCOUNTER *p = (FLCOUNTER *) a;
    *((FLCOUNTER*) a)->kout =  w->value();
    if (*p->args[0] >= 0) ButtonSched(p->h.insdshead->csound,
                                      p->args, p->INOCOUNT-10);
}

static void fl_callbackLinearSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    displ(*p->kout = w->value(), *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackExponentialSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    #if defined(sun)
      displ(*p->kout = p->min * ::pow ((double)p->base, w->value()),
            *p->idisp, p->h.insdshead->csound);
    #else
      displ(*p->kout = p->min * ::pow (p->base, w->value()),
            *p->idisp, p->h.insdshead->csound);
    #endif
}

static void fl_callbackInterpTableSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (int) ndx;
    MYFLT v1 = p->table[index];
    displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) *
                               (ndx - index)) * (*p->imax - p->min),
          *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackTableSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
          (*p->imax - p->min),
          *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackLinearSliderBank(Fl_Valuator* w, void *a)
{
    SLDBK_ELEMENT* p = (SLDBK_ELEMENT*) a;
    *p->out = w->value();
}

static void fl_callbackExponentialSliderBank(Fl_Valuator* w, void *a)
{
    SLDBK_ELEMENT* p = (SLDBK_ELEMENT*) a;
    #if defined(sun)
      *p->out = p->min * ::pow ((double)p->base, w->value());
    #else
      *p->out = p->min * ::pow (p->base, w->value());
    #endif
}

static void fl_callbackInterpTableSliderBank(Fl_Valuator* w, void *a)
{
    SLDBK_ELEMENT *p = ((SLDBK_ELEMENT*) a);

    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (int) ndx;
    MYFLT v1 = p->table[index];
    *p->out = p->min + ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
      (p->max - p->min);
}

static void fl_callbackTableSliderBank(Fl_Valuator* w, void *a)
{
    SLDBK_ELEMENT *p = ((SLDBK_ELEMENT*) a);
    *p->out = p->min + p->table[(long)(w->value() * p->tablen)] *
      (p->max - p->min);
}

static void fl_callbackJoystick(Fl_Widget* w, void *a)
{
    FLJOYSTICK *p = (FLJOYSTICK*) a;
    Fl_Positioner *j = (Fl_Positioner*) w;
    MYFLT val;
    int iexpx = (int) *p->iexpx, iexpy = (int) *p->iexpy;
    switch (iexpx) {
    case LIN_:
      val = j->xvalue();
      break;
    case EXP_:
      #if defined(sun)
        val = *p->iminx * ::pow ((double)p->basex, j->xvalue());
      #else
        val = *p->iminx * ::pow (p->basex, j->xvalue());
      #endif
      break;
    default:
      if (iexpx > 0) { //interpolated
        MYFLT ndx = j->xvalue() * (p->tablenx-1);
        int index = (int) ndx;
        MYFLT v1 = p->tablex[index];
        val = *p->iminx + ( v1 + (p->tablex[index+1] - v1) *
                            (ndx - index)) * (*p->imaxx - *p->iminx);
      }
      else // non-interpolated
        val = *p->iminx+ p->tablex[(long) (j->xvalue() * p->tablenx)] *
          (*p->imaxx - *p->iminx);
    }
    displ(*p->koutx = val,*p->idispx, p->h.insdshead->csound);
    switch (iexpy) {
    case LIN_:
      val = j->yvalue();
      break;
    case EXP_:
      #if defined(sun)
        val = *p->iminy * ::pow ((double)p->basey, j->yvalue());
      #else
        val = *p->iminy * ::pow (p->basey, j->yvalue());
      #endif
      break;
    default:
      if (iexpy > 0) { //interpolated
        MYFLT ndx = j->yvalue() * (p->tableny-1);
        long index = (long) ndx;
        MYFLT v1 = p->tabley[index];
        val = *p->iminy + ( v1 + (p->tabley[index+1] - v1) * (ndx - index))
          * (*p->imaxy - *p->iminy);
      }
      else { // non-interpolated
        long index = (long) (j->yvalue()* p->tableny);
        val = *p->iminy+ p->tabley[index] * (*p->imaxy - *p->iminy);

      }
    }
    displ(*p->kouty = val, *p->idispy, p->h.insdshead->csound);
}

static void fl_callbackLinearRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    displ(*p->kout =  w->value(),*p->idisp, p->h.insdshead->csound);
}

static void fl_callbackExponentialRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    #if defined(sun)
      displ(*p->kout = ((FLROLLER*) a)->min * ::pow ((double)p->base, w->value()),
            *p->idisp, p->h.insdshead->csound);
    #else
      displ(*p->kout = ((FLROLLER*) a)->min * ::pow (p->base, w->value()),
            *p->idisp, p->h.insdshead->csound);
    #endif
}

static void fl_callbackInterpTableRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (int) ndx;
    MYFLT v1 = p->table[index];
    displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
          (*p->imax - p->min), *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackTableRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
          (*p->imax - p->min), *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackLinearKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    displ( *p->kout = w->value(), *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackExponentialKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    #if defined(sun)
      displ(*p->kout = ((FLKNOB*) a)->min * ::pow ((double)p->base, w->value()),
            *p->idisp, p->h.insdshead->csound);
    #else
      displ(*p->kout = ((FLKNOB*) a)->min * ::pow (p->base, w->value()),
            *p->idisp, p->h.insdshead->csound);
    #endif
}

static void fl_callbackInterpTableKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (int) ndx;
    MYFLT v1 = p->table[index];
    displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
          (*p->imax - p->min), *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackTableKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
          (*p->imax - p->min), *p->idisp, p->h.insdshead->csound);
}

static void fl_callbackLinearValueInput(Fl_Valuator* w, void *a)
{
    *((FLTEXT*) a)->kout =  w->value();
}

//-----------

static int rand_31_i(CSOUND *csound, int maxVal)
{
    int seed = csound->GetRandSeed(csound,2);
    double  x = (double) (csound->Rand31(&seed) - 1);
    return (int) (x * (double) (maxVal + 1) / 2147483646.0);
}

static void widget_attributes(CSOUND *csound, Fl_Widget *o)
{
    WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (ST(FLtext_size) == -2 ) {
      ST(FLtext_size) = -1;
      ST(FLtext_color)= -1;
      ST(FLtext_font) = -1;
      ST(FLtext_align)= -1;
      ST(FLcolor) = -1;
    }
    if (ST(FLtext_size) > 0) // if > 0 assign it, else skip, leaving default
      o->labelsize(ST(FLtext_size));
    switch ((int) ST(FLtext_color)) {
    case -2: // random color
      o->labelcolor(fl_rgb_color(rand_31_i(csound, 255), rand_31_i(csound, 255),
                                 rand_31_i(csound, 255)));
      break;
    case -1:
      // if FLtext_color is == -1, color assignment is skipped,
      // leaving default color
      break;
    default:
      o->labelcolor(ST(FLtext_color));
      break;
    }
    if (ST(FLtext_font)> 0) {
      Fl_Font font;
      if (ST(FLtext_font)<0 || ST(FLtext_font)>16) font = FL_HELVETICA;
      else font = FONT_TABLE[ST(FLtext_font)];
      //     switch (FLtext_font) {
      //     case 1: font  = FL_HELVETICA; break;
      //     case 2: font  = FL_HELVETICA_BOLD; break;
      //     case 3: font  = FL_HELVETICA_ITALIC; break;
      //     case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
      //     case 5: font  = FL_COURIER; break;
      //     case 6: font  = FL_COURIER_BOLD; break;
      //     case 7: font  = FL_COURIER_ITALIC; break;
      //     case 8: font  = FL_COURIER_BOLD_ITALIC; break;
      //     case 9: font  = FL_TIMES; break;
      //     case 10: font = FL_TIMES_BOLD; break;
      //     case 11: font = FL_TIMES_ITALIC; break;
      //     case 12: font = FL_TIMES_BOLD_ITALIC; break;
      //     case 13: font = FL_SYMBOL; break;
      //     case 14: font = FL_SCREEN; break;
      //     case 15: font = FL_SCREEN_BOLD; break;
      //     case 16: font = FL_ZAPF_DINGBATS; break;
      //     default: font = FL_HELVETICA; break;
      //     }
      o->labelfont(font);
    }
    if (ST(FLtext_align) > 0) {
      Fl_Align type;
      if (ST(FLtext_align)<0 || ST(FLtext_align)>9) type = FL_ALIGN_BOTTOM;
      else type = ALIGN_TABLE[ST(FLtext_align)];
      //     switch (ST(FLtext_align)) {
      //     case 1: type  = FL_ALIGN_CENTER; break;
      //     case 2: type  = FL_ALIGN_TOP; break;
      //     case 3: type  = FL_ALIGN_BOTTOM; break;
      //     case 4: type  = FL_ALIGN_LEFT; break;
      //     case 5: type  = FL_ALIGN_RIGHT; break;
      //     case 6: type  = FL_ALIGN_TOP_LEFT; break;
      //     case 7: type  = FL_ALIGN_TOP_RIGHT; break;
      //     case 8: type  = FL_ALIGN_BOTTOM_LEFT; break;
      //     case 9: type  = FL_ALIGN_BOTTOM_RIGHT; break;
      //     case -1:                // What type is this?
      //     default: type = FL_ALIGN_BOTTOM; break;
      //     }
      o->align(type);
    }
    switch ((int) ST(FLcolor)) {  // random color
    case -2:
      o->color(FL_GRAY,
               fl_rgb_color(rand_31_i(csound, 255), rand_31_i(csound, 255),
                            rand_31_i(csound, 255)));
      break;
    case -1:
      // if FLcolor is == -1, color assignment is skipped,
      // leaving widget default color
      break;
    default:
      o->color(ST(FLcolor), ST(FLcolor2));
      break;
    }
}

//-----------

extern "C" {

  // static int FLkeyb(CSOUND *csound, FLKEYB *p)
  // {
  //     (void) csound;
  //     (void) p;
  //     return OK;
  // }

  //-----------

  void flpanel_cb(Fl_Widget *,void *) { //suppresses the close button
  }

  static int StartPanel(CSOUND *csound, FLPANEL *p)
  {
      char    *panelName;
      panelName =  p->name->data;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

      *(getFLTKFlagsPtr(csound)) |= 32;
      int      x = (int) *p->ix, y = (int) *p->iy;
      int  width = (int) *p->iwidth, height = (int) *p->iheight;
      if (width < 0) width = 400;   // default
      if (height < 0) height = 300;

      Fl_Boxtype borderType;
      int iborder = (int) *p->border;
      if (iborder<0 || iborder>7) borderType = FL_FLAT_BOX;
      else borderType = BOX_TABLE[iborder];
      //   switch( (int) *p->border ) {
      //   case 0: borderType = FL_FLAT_BOX; break;
      //   case 1: borderType = FL_DOWN_BOX; break;
      //   case 2: borderType = FL_UP_BOX; break;
      //   case 3: borderType = FL_ENGRAVED_BOX; break;
      //   case 4: borderType = FL_EMBOSSED_BOX; break;
      //   case 5: borderType = FL_BORDER_BOX; break;
      //   case 6: borderType = FL_THIN_DOWN_BOX; break;
      //   case 7: borderType = FL_THIN_UP_BOX; break;
      //   default: borderType = FL_FLAT_BOX;
      //   }

      Fl_Window *o;
      if (*(p->ikbdsense) == MYFLT(0.0)) {
        if (x < 0)
          o = new Fl_Window(width, height, panelName);
        else
          o = new Fl_Window(x, y, width, height, panelName);
      }
      else if (x < 0)
        o = new CsoundFLWindow(csound, width, height, panelName);
      else
        o = new CsoundFLWindow(csound, x, y, width, height, panelName);
      widget_attributes(csound, o);
      o->box(borderType);
      o->resizable(o);
      if (*p->iclose != 0)
        o->callback(flpanel_cb);
      widget_attributes(csound, o);
      ADDR_STACK adrstk(&p->h, (void *) o, ST(stack_count));
      ST(AddrStack).push_back(adrstk);
      PANELS panel(o, (ST(stack_count) > 0) ? 1 : 0);
      ST(fl_windows).push_back(panel);
      ST(stack_count)++;

      return OK;
  }

  static int EndPanel(CSOUND *csound, FLPANELEND *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ST(stack_count)--;
      ADDR_STACK adrstk = ST(AddrStack).back();
      if (UNLIKELY(strcmp( adrstk.h->optext->t.opcod, "FLpanel")))
        return csound->InitError(csound,
                                 Str("FLpanel_end: invalid stack pointer: "
                                     "verify its placement"));
      if (UNLIKELY(adrstk.count != ST(stack_count)))
        return csound->InitError(csound,
                                 Str("FLpanel_end: invalid stack count: "
                                     "verify FLpanel/FLpanel_end count and"
                                     " placement"));
      ((Fl_Window*) adrstk.WidgAddress)->end();
      ST(AddrStack).pop_back();
      return OK;
  }

  //-----------
  static int StartScroll(CSOUND *csound, FLSCROLL *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      Fl_Scroll *o = new Fl_Scroll ((int) *p->ix, (int) *p->iy,
                                    (int) *p->iwidth, (int) *p->iheight);
      ADDR_STACK adrstk(&p->h,o,ST(stack_count));
      ST(AddrStack).push_back(adrstk);
      ST(stack_count)++;
      return OK;
  }

  static int EndScroll(CSOUND *csound, FLSCROLLEND *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ST(stack_count)--;
      ADDR_STACK adrstk = ST(AddrStack).back();
      if (UNLIKELY(strcmp( adrstk.h->optext->t.opcod, "FLscroll")))
        return
          csound->InitError(csound,
                            Str("FLscroll_end: invalid stack pointer: "
                                "verify its placement"));
      if (UNLIKELY(adrstk.count != ST(stack_count)))
        return csound->InitError(csound,
                            Str("FLscroll_end: invalid stack count: "
                                "verify FLscroll/FLscroll_end count "
                                "and placement"));
      ((Fl_Scroll*) adrstk.WidgAddress)->end();

      ST(AddrStack).pop_back();
      return OK;
  }

  //-----------
  static int StartTabs(CSOUND *csound, FLTABS *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      Fl_Tabs *o = new Fl_Tabs ((int) *p->ix, (int) *p->iy,
                                (int) *p->iwidth, (int) *p->iheight);
      widget_attributes(csound, o);
      //   o->box(FL_PLASTIC_UP_BOX);
      ADDR_STACK adrstk(&p->h,o,ST(stack_count));
      ST(AddrStack).push_back(adrstk);
      ST(stack_count)++;
      return OK;
  }

  static int EndTabs(CSOUND *csound, FLTABSEND *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ST(stack_count)--;
      ADDR_STACK adrstk = ST(AddrStack).back();
      if (UNLIKELY(strcmp( adrstk.h->optext->t.opcod, "FLtabs")))
        return
          csound->InitError(csound,
                            Str("FLscroll_end: invalid stack pointer: "
                                "verify its placement"));
      if (UNLIKELY(adrstk.count != ST(stack_count)))
        return csound->InitError(csound,
                                 Str("FLtabs_end: invalid stack count: "
                                     "verify FLtabs/FLtabs_end count and "
                                     "placement"));
      ((Fl_Scroll*) adrstk.WidgAddress)->end();

      ST(AddrStack).pop_back();
      return OK;
  }

  //-----------
  static int StartGroup(CSOUND *csound, FLGROUP *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *Name = p->name->data;
      Fl_Group *o = new Fl_Group ((int) *p->ix, (int) *p->iy,
                                  (int) *p->iwidth, (int) *p->iheight,Name);
      widget_attributes(csound, o);
      Fl_Boxtype borderType;
      int iborder = (int)*p->border;
      if (iborder<0 || iborder>7) borderType = FL_FLAT_BOX;
      else borderType = BOX_TABLE[iborder];
      //   switch((int)*p->border ) {
      //   case 0: borderType = FL_FLAT_BOX; break;
      //   case 1: borderType = FL_DOWN_BOX; break;
      //   case 2: borderType = FL_UP_BOX; break;
      //   case 3: borderType = FL_ENGRAVED_BOX; break;
      //   case 4: borderType = FL_EMBOSSED_BOX; break;
      //   case 5: borderType = FL_BORDER_BOX; break;
      //   case 6: borderType = FL_THIN_DOWN_BOX; break;
      //   case 7: borderType = FL_THIN_UP_BOX; break;
      //   default: borderType = FL_FLAT_BOX;
      //   }
      o->box(borderType);
      widget_attributes(csound, o);
      ADDR_STACK adrstk(&p->h,o,ST(stack_count));
      ST(AddrStack).push_back(adrstk);
      ST(stack_count)++;
      return OK;
  }

  static int EndGroup(CSOUND *csound, FLGROUPEND *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ST(stack_count)--;
      ADDR_STACK adrstk = ST(AddrStack).back();
      if (UNLIKELY(strcmp( adrstk.h->optext->t.opcod, "FLgroup")))
        return csound->InitError(csound,
                                 Str("FLgroup_end: invalid stack pointer: "
                                     "verify its placement"));
      if (UNLIKELY(adrstk.count != ST(stack_count)))
        return csound->InitError(csound,
                                 Str("FLgroup_end: invalid stack count: "
                                     "verify FLgroup/FLgroup_end count and"
                                     " placement"));
      ((Fl_Scroll*) adrstk.WidgAddress)->end();

      ST(AddrStack).pop_back();
      return OK;
  }

  //-----------

  static int StartPack(CSOUND *csound, FLPACK *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      Fl_Pack *o = new Fl_Pack ((int) *p->ix, (int) *p->iy,
                                (int) *p->iwidth, (int) *p->iheight);
      Fl_Boxtype borderType = FL_FLAT_BOX;
      int iborder = (int)*p->iborder;
      // fl_window->resizable(o);
      if (!((iborder<0 || iborder>7)))
          borderType = BOX_TABLE[iborder];
      o->box(borderType);       // JPff added March 2012
      o->type((int)*p->itype);
      o->spacing((int)*p->ispace);

      ADDR_STACK adrstk(&p->h,o,ST(stack_count));
      ST(AddrStack).push_back(adrstk);
      ST(stack_count)++;
      return OK;
  }

  static int EndPack(CSOUND *csound, FLSCROLLEND *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ST(stack_count)--;
      ADDR_STACK adrstk = ST(AddrStack).back();
      if (UNLIKELY(strcmp( adrstk.h->optext->t.opcod, "FLpack")))
        return csound->InitError(csound,
                                 Str("FLpack_end: invalid stack pointer: "
                                     "verify its placement"));
      if (UNLIKELY(adrstk.count != ST(stack_count)))
        return csound->InitError(csound,
                                 Str("FLpack_end: invalid stack count: "
                                     "verify FLpack/FLpack_end count and "
                                     "placement"));
      ((Fl_Pack*) adrstk.WidgAddress)->end();

      ST(AddrStack).pop_back();
      return OK;
  }

  //-----------

  static int fl_widget_color(CSOUND *csound, FLWIDGCOL *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (*p->red1 < 0) { // reset colors to default
        ST(FLcolor) = (int) *p->red1; //when called without arguments
        ST(FLcolor2) =(int) *p->red1;
      }
      else {
        ST(FLcolor) = fl_rgb_color((int) *p->red1,
                                   (int) *p->green1,
                                   (int) *p->blue1);
        ST(FLcolor2) = fl_rgb_color((int) *p->red2,
                                    (int) *p->green2,
                                    (int) *p->blue2);
      }
      return OK;
  }

  static int fl_widget_color2(CSOUND *csound, FLWIDGCOL2 *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (*p->red < 0) { // reset colors to default
        ST(FLcolor2) =(int) *p->red;
      }
      else {
        ST(FLcolor2) = fl_rgb_color((int) *p->red,
                                    (int) *p->green,
                                    (int) *p->blue);
      }
      return OK;
  }

  static int fl_widget_label(CSOUND *csound, FLWIDGLABEL *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (*p->size <= 0) { // reset settings to default
        ST(FLtext_size) = 0; //when called without arguments
        ST(FLtext_font) = -1;
        ST(FLtext_align) = 0;
        ST(FLtext_color) = -1;
      }
      else {
        ST(FLtext_size) = (int) *p->size;

        if (*p->font > -1) ST(FLtext_font) = (int) *p->font;
        if (*p->align > 0)  ST(FLtext_align) =  (int) *p->align;
        if (*p->red > -1 && *p->green > -1 && *p->blue > -1) {
          ST(FLtext_color) = fl_rgb_color((int) *p->red,
                                          (int) *p->green,
                                          (int) *p->blue);
        }
      }
      return OK;
  }

}       // extern "C"

// -----------

static int fl_getWidgetTypeFromOpcodeName(CSOUND *csound, void *p)
{
    const char  *opname = csound->GetOpcodeName(p);

    if (strcmp(opname, "FLbutton") == 0)
      return 1;
    if (strcmp(opname, "FLbutBank") == 0)
      return 2;
    if (strcmp(opname, "FLjoy") == 0)
      return 3;
    if (strcmp(opname, "FLvalue") == 0)
      return 4;
    if (strcmp(opname, "FLbox") != 0)
      return 0;
    csound->Warning(csound, Str("System error: value() method called from "
                                "non-valuator object"));
    return -1;
}

static void fl_setWidgetValue_(CSOUND *csound,
                               ADDR_SET_VALUE &v, int widgetType,
                               MYFLT val, MYFLT log_base)
{
    Fl_Widget   *o = (Fl_Widget *) v.WidgAddress;
    void        *p = v.opcode;
    bool        fltkLockingIsEnabled;

    if ((!widgetType || widgetType > 2) &&
        (v.exponential == LIN_ || v.exponential == EXP_)) {
      if (val < v.min)
        val = v.min;
      else if (val > v.max)
        val = v.max;
      if (v.exponential == EXP_)
        val = (MYFLT) (log(val / v.min) / log_base);
    }
    fltkLockingIsEnabled = ((getFLTKFlags(csound) & 8) == 0);
    if (fltkLockingIsEnabled)
      Fl_lock(csound);
    switch (widgetType) {
    case 0:                                     // valuator
      ((Fl_Valuator *) o)->value(val);
      break;
    case 1:                                     // FLbutton
      if (val == *(((FLBUTTON *) v.opcode)->ion))
        ((Fl_Button *) o)->value(1);
      else if (val == *(((FLBUTTON *) v.opcode)->ioff))
        ((Fl_Button *) o)->value(0);
      break;
    case 2:                                     // FLbutBank
      set_butbank_value((Fl_Group *) o, val);
      break;
    case 3:                                     // FLjoy
      {
        static int  flag = 0;
        // FLsetVal always requires two adjacent calls when setting FLjoy
        if (!flag) {
          ((Fl_Positioner *) o)->xvalue(val);
          flag = 1;
        }
        else {
          ((Fl_Positioner *) o)->yvalue(val);
          flag = 0;
        }
      }
      break;
    default:                                    // invalid
      if (fltkLockingIsEnabled)
        Fl_unlock(csound);
      return;
    }
    o->do_callback(o, p);
    if (fltkLockingIsEnabled)
      Fl_unlock(csound);
}

extern "C" {

  static int fl_setWidgetValuei(CSOUND *csound, FL_SET_WIDGET_VALUE_I *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      MYFLT           log_base = MYFLT(1.0);
      ADDR_SET_VALUE  &v = ST(AddrSetValue)[(int) *p->ihandle];
      int             widgetType;

      widgetType = fl_getWidgetTypeFromOpcodeName(csound, v.opcode);
      if (UNLIKELY(widgetType == 4)) {
        csound->InitError(csound,
                          Str("FLvalue cannot be set by FLsetVal.\n"));
        return NOTOK;
      }
      if (widgetType < 0)
        return OK;
      if (!widgetType || widgetType > 2) {
        switch (v.exponential) {
        case LIN_:        // linear
          break;
        case EXP_:        // exponential
          #if defined(sun)
            log_base = (MYFLT) log(::pow(v.max / (double)v.min,
                                         1.0 / (v.max - v.min)));
          #else
            log_base = (MYFLT) log(::pow(v.max / v.min, 1.0 / (v.max - v.min)));
          #endif
          break;
        default:
          csound->Warning(csound, Str("(fl_setWidgetValuei): "
                                      "not fully implemented yet; exp=%d"),
                          v.exponential);
        }
      }
      fl_setWidgetValue_(csound, v, widgetType, *(p->ivalue), log_base);
      return OK;
  }

  static int fl_setWidgetValue_set(CSOUND *csound, FL_SET_WIDGET_VALUE *p)
  {
      p->handle = (int) *(p->ihandle);
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      MYFLT           log_base = MYFLT(1.0);
      ADDR_SET_VALUE  &v = ST(AddrSetValue)[p->handle];
      int             widgetType;

      widgetType = fl_getWidgetTypeFromOpcodeName(csound, v.opcode);
      if (UNLIKELY(widgetType == 4)) {
        csound->InitError(csound,
                          Str("FLvalue cannot be set by FLsetVal\n"));
        return NOTOK;
      }
      if (widgetType < 0)
        return OK;
      if (!widgetType || widgetType > 2) {
        switch (v.exponential) {
        case LIN_:        // linear
          break;
        case EXP_:        // exponential
          #if defined(sun)
            log_base = (MYFLT) log(::pow(v.max / (double)v.min,
                                   1.0 / (v.max - v.min)));
          #else
            log_base = (MYFLT) log(::pow(v.max / v.min, 1.0 / (v.max - v.min)));
          #endif
          break;
        default:
          csound->Warning(csound, Str("(fl_setWidgetValue_set): "
                                      "not fully implemented yet; exp=%d"),
                          v.exponential);
        }
      }
      p->widgetType = widgetType;
      p->log_base = log_base;

      return OK;
  }

  static int fl_setWidgetValue(CSOUND *csound, FL_SET_WIDGET_VALUE *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (*p->ktrig != MYFLT(0.0))
        fl_setWidgetValue_(csound, ST(AddrSetValue)[p->handle], p->widgetType,
                           *(p->kvalue), p->log_base);
      return OK;
  }

  //-----------
  //-----------

  static int fl_setColor1(CSOUND *csound, FL_SET_COLOR *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      int color = fl_rgb_color((int) *p->red,
                               (int) *p->green,
                               (int) *p->blue);
      o->color(color);
      return OK;
  }

  static int fl_setColor2(CSOUND *csound, FL_SET_COLOR *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      int color = fl_rgb_color((int) *p->red, (int) *p->green, (int) *p->blue);
      o->selection_color(color);
      return OK;
  }

  static int fl_setTextColor(CSOUND *csound, FL_SET_COLOR *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      int color = fl_rgb_color((int) *p->red, (int) *p->green, (int) *p->blue);
      o->labelcolor(color);
      return OK;
  }

  static int fl_setTextSize(CSOUND *csound, FL_SET_TEXTSIZE *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      o->labelsize((uchar) *p->ivalue);
      return OK;
  }

  static int fl_setFont(CSOUND *csound, FL_SET_FONT *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      Fl_Font font;
      int ifnt = (int) *p->itype;
      if (ifnt<0 || ifnt>16) font = FL_HELVETICA;
      else font = FONT_TABLE[ifnt];
      //   switch ((int) *p->itype) {
      //   case 1: font  = FL_HELVETICA; break;
      //   case 2: font  = FL_HELVETICA_BOLD; break;
      //   case 3: font  = FL_HELVETICA_ITALIC; break;
      //   case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
      //   case 5: font  = FL_COURIER; break;
      //   case 6: font  = FL_COURIER_BOLD; break;
      //   case 7: font  = FL_COURIER_ITALIC; break;
      //   case 8: font  = FL_COURIER_BOLD_ITALIC; break;
      //   case 9: font  = FL_TIMES; break;
      //   case 10: font = FL_TIMES_BOLD; break;
      //   case 11: font = FL_TIMES_ITALIC; break;
      //   case 12: font = FL_TIMES_BOLD_ITALIC; break;
      //   case 13: font = FL_SYMBOL; break;
      //   case 14: font = FL_SCREEN; break;
      //   case 15: font = FL_SCREEN_BOLD; break;
      //   case 16: font = FL_ZAPF_DINGBATS; break;
      //   default: font = FL_SCREEN;
      //   }
      o->labelfont(font);
      return OK;
  }

  static int fl_setTextType(CSOUND *csound, FL_SET_FONT *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      Fl_Labeltype type;
      switch ((int) *p->itype) {
      case 0: type  = FL_NORMAL_LABEL; break;
      case 1: type  = FL_NO_LABEL; break;
      case 2: type  = FL_SYMBOL_LABEL; break;
      case 3: type  = FL_SHADOW_LABEL; break;
      case 4: type  = FL_ENGRAVED_LABEL; break;
      case 5: type  = FL_EMBOSSED_LABEL; break;
        /*    case 6: type  = _FL_BITMAP_LABEL; break;
              case 7: type  = _FL_PIXMAP_LABEL; break;
              case 8: type  = _FL_IMAGE_LABEL; break;
              case 9: type  = _FL_MULTI_LABEL; break; */
      case 10: type = FL_FREE_LABELTYPE; break;
      default: type = FL_NORMAL_LABEL;
      }
      o->labeltype(type);
      o->window()->redraw();
      return OK;
  }

  static int fl_box_(CSOUND *csound, FL_BOX *p)
  {
      char *text = p->itext->data;
      Fl_Box *o =  new Fl_Box((int)*p->ix, (int)*p->iy,
                              (int)*p->iwidth, (int)*p->iheight, text);
      widget_attributes(csound, o);
      Fl_Boxtype type;
      int itype = (int) *p->itype;
      if (itype<0 || itype>19) type = FL_FLAT_BOX;
      else type = BOX_TABLE[itype];
      //   switch ((int) *p->itype) {
      //   case 1: type  = FL_FLAT_BOX; break;
      //   case 2: type  = FL_UP_BOX; break;
      //   case 3: type  = FL_DOWN_BOX; break;
      //   case 4: type  = FL_THIN_UP_BOX; break;
      //   case 5: type  = FL_THIN_DOWN_BOX; break;
      //   case 6: type  = FL_ENGRAVED_BOX; break;
      //   case 7: type  = FL_EMBOSSED_BOX; break;
      //   case 8: type  = FL_BORDER_BOX; break;
      //   case 9: type  = _FL_SHADOW_BOX; break;
      //   case 10: type = _FL_ROUNDED_BOX; break;
      //   case 11: type = _FL_RSHADOW_BOX; break;
      //   case 12: type = _FL_RFLAT_BOX; break;
      //   case 13: type = _FL_ROUND_UP_BOX; break;
      //   case 14: type = _FL_ROUND_DOWN_BOX; break;
      //   case 15: type = _FL_DIAMOND_UP_BOX; break;
      //   case 16: type = _FL_DIAMOND_DOWN_BOX; break;
      //   case 17: type = _FL_OVAL_BOX; break;
      //   case 18: type = _FL_OSHADOW_BOX; break;
      //   case 19: type = _FL_OFLAT_BOX; break;
      //   default: type = FL_FLAT_BOX;
      //   }
      o->box(type);
      Fl_Font font;
      int ifnt = (int) *p->ifont;
      if (ifnt<0 || ifnt>16) font = FL_HELVETICA;
      else font = FONT_TABLE[ifnt];
      //   switch ((int) *p->ifont) {
      //   case 1: font  = FL_HELVETICA; break;
      //   case 2: font  = FL_HELVETICA_BOLD; break;
      //   case 3: font  = FL_HELVETICA_ITALIC; break;
      //   case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
      //   case 5: font  = FL_COURIER; break;
      //   case 6: font  = FL_COURIER_BOLD; break;
      //   case 7: font  = FL_COURIER_ITALIC; break;
      //   case 8: font  = FL_COURIER_BOLD_ITALIC; break;
      //   case 9: font  = FL_TIMES; break;
      //   case 10: font = FL_TIMES_BOLD; break;
      //   case 11: font = FL_TIMES_ITALIC; break;
      //   case 12: font = FL_TIMES_BOLD_ITALIC; break;
      //   case 13: font = FL_SYMBOL; break;
      //   case 14: font = FL_SCREEN; break;
      //   case 15: font = FL_SCREEN_BOLD; break;
      //   case 16: font = FL_ZAPF_DINGBATS; break;
      //   default: font = FL_HELVETICA;
      //   }
      o->labelfont(font);
      o->labelsize((unsigned char)*p->isize);
      o->align(FL_ALIGN_WRAP);
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(0, 0, 0, (void *)o,
                                                (void *)p, ST(currentSnapGroup)));
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  static int fl_setText(CSOUND *csound, FL_SET_TEXT *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *text = p->itext->data;
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      o->label(text);
      return OK;
  }

  static int fl_setSize(CSOUND *csound, FL_SET_SIZE *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      o->size((short)  *p->iwidth, (short) *p->iheight);
      return OK;
  }

  static int fl_setPosition(CSOUND *csound, FL_SET_POSITION *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      o->position((short)  *p->ix, (short) *p->iy);
      return OK;
  }

  static int fl_hide(CSOUND *csound, FL_WIDHIDE *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      Fl_lock(csound);
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      o->hide();
      Fl_unlock(csound);
      return OK;
  }

  static int fl_show(CSOUND *csound, FL_WIDSHOW *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      Fl_lock(csound);
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      o->show();
      Fl_unlock(csound);
      return OK;
  }

  static int fl_setBox(CSOUND *csound, FL_SETBOX *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      Fl_Boxtype type;
      int itype = (int) *p->itype;
      if (itype<0||itype>19) type = FL_FLAT_BOX;
      else type = BOX_TABLE[itype];
      //   switch ((int) *p->itype) {
      //   case 1: type  = FL_FLAT_BOX; break;
      //   case 2: type  = FL_UP_BOX; break;
      //   case 3: type  = FL_DOWN_BOX; break;
      //   case 4: type  = FL_THIN_UP_BOX; break;
      //   case 5: type  = FL_THIN_DOWN_BOX; break;
      //   case 6: type  = FL_ENGRAVED_BOX; break;
      //   case 7: type  = FL_EMBOSSED_BOX; break;
      //   case 8: type  = FL_BORDER_BOX; break;
      //   case 9: type  = FL_SHADOW_BOX; break;
      //   case 10: type = FL_ROUNDED_BOX; break;
      //   case 11: type = FL_RSHADOW_BOX; break;
      //   case 12: type = FL_RFLAT_BOX; break;
      //   case 13: type = FL_ROUND_UP_BOX; break;
      //   case 14: type = FL_ROUND_DOWN_BOX; break;
      //   case 15: type = FL_DIAMOND_UP_BOX; break;
      //   case 16: type = FL_DIAMOND_DOWN_BOX; break;
      //   case 17: type = FL_OVAL_BOX; break;
      //   case 18: type = FL_OSHADOW_BOX; break;
      //   case 19: type = FL_OFLAT_BOX; break;
      //   default: type = FL_FLAT_BOX;
      //   }
      o->box(type);
      return OK;
  }

  static int fl_align(CSOUND *csound, FL_TALIGN *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
      Fl_Align type;
      int itype= (int) *p->itype;
      if (itype<0 || itype>9) type = FL_ALIGN_BOTTOM;
      else type = ALIGN_TABLE[itype];
      //   switch ((int) *p->itype) {
      //   case 1: type  = FL_ALIGN_CENTER; break;
      //   case 2: type  = FL_ALIGN_TOP; break;
      //   case 3: type  = FL_ALIGN_BOTTOM; break;
      //   case 4: type  = FL_ALIGN_LEFT; break;
      //   case 5: type  = FL_ALIGN_RIGHT; break;
      //   case 6: type  = FL_ALIGN_TOP_LEFT; break;
      //   case 7: type  = FL_ALIGN_TOP_RIGHT; break;
      //   case 8: type  = FL_ALIGN_BOTTOM_LEFT; break;
      //   case 9: type  = FL_ALIGN_BOTTOM_RIGHT; break;
      //   default: type = FL_ALIGN_BOTTOM;
      //   }
      o->align(type);
      return OK;
  }

  //-----------
  //-----------

  static int fl_value(CSOUND *csound, FLVALUE *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *controlName = p->name->data;
      int ix, iy, iwidth, iheight;
      if (*p->ix<0) ix = ST(FL_ix);       else  ST(FL_ix) = ix = (int) *p->ix;
      if (*p->iy<0) iy = ST(FL_iy);       else  ST(FL_iy) = iy = (int) *p->iy;
      if (*p->iwidth<0) iwidth = ST(FLvalue_iwidth);
      else ST(FLvalue_iwidth) = iwidth = (int) *p->iwidth;
      if (*p->iheight<0) iheight = ST(FLroller_iheight);
      else ST(FLroller_iheight) = iheight = (int) *p->iheight;

      Fl_Output *o = new Fl_Output(ix, iy, iwidth, iheight,controlName);
      o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
      if (ST(FLcolor) < 0 )
        o->color(FL_GRAY );
      else
        o->color(ST(FLcolor), ST(FLcolor2));
      widget_attributes(csound, o);
      //AddrValue.push_back((void *) o);
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o,
                                                (void *) p, ST(currentSnapGroup)));
      //*p->ihandle = AddrValue.size()-1;
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  //-----------

  static int fl_slider(CSOUND *csound, FLSLIDER *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *controlName = p->name->data;
      int ix,iy,iwidth, iheight,itype, iexp;
      bool plastic = false;

      if (*p->iy < 0) {
        iy = ST(FL_iy);
        ST(FL_iy) += ST(FLcontrol_iheight) + 5;
      }
      else {
        iy = (int) *p->iy;
        ST(FL_iy) = iy + ST(FLcontrol_iheight) + 5;
      }
      if (*p->ix < 0)  ix = ST(FL_ix); // omitted options: set default
      else  ST(FL_ix) = ix = (int) *p->ix;
      if (*p->iwidth < 0) iwidth = ST(FLcontrol_iwidth);
      else ST(FLcontrol_iwidth) = iwidth = (int) *p->iwidth;
      if (*p->iheight < 0) iheight = ST(FLcontrol_iheight);
      else ST(FLcontrol_iheight) = iheight = (int) *p->iheight;
      if (*p->itype < 1) itype = 1;
      else  itype = (int) *p->itype;

      //if (*p->iexp == LIN_) iexp = LIN_;
      //else  iexp = (int) *p->iexp;
      switch((int) *p->iexp) {
      case -1: iexp = EXP_; break;
      case 0: iexp = LIN_; break;
      default: iexp = (int) *p->iexp;
      }
      if (itype > 19) {
        plastic = true;
        itype = itype - 20;
      }
      if (UNLIKELY(itype > 10 && iexp == EXP_)) {
        csound->Warning(csound,
                        Str("FLslider exponential, using non-labeled slider"));
        itype -= 10;
      }

      Fl_Slider *o;
      if (itype <= 10) o = new Fl_Slider(ix, iy, iwidth, iheight, controlName);
      else {
        o = new Fl_Value_Slider_Input(csound, ix, iy,
                                      iwidth, iheight, controlName);
        itype -=10;
        //o->labelsize(20);
        ((Fl_Value_Slider_Input*) o)->textboxsize(50);
        ((Fl_Value_Slider_Input*) o)->textsize(13);
        o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
      }
      switch (itype) {
      case 1:  o->type(FL_HOR_FILL_SLIDER); break;
      case 2:  o->type(FL_VERT_FILL_SLIDER); break;
      case 3:  o->type(FL_HOR_SLIDER); break;
      case 4:  o->type(FL_VERT_SLIDER); break;
      case 5:  o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
      case 6:  o->type(FL_VERT_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
      default: return csound->InitError(csound,
                                        Str("FLslider: invalid slider type"));
      }
      if (plastic) o->box(FL_PLASTIC_DOWN_BOX);
      widget_attributes(csound, o);
      MYFLT min = p->min = *p->imin, max = *p->imax, range;
      switch (iexp) {
      case LIN_: //linear
        o->range(min,max);
        o->callback((Fl_Callback*)fl_callbackLinearSlider,(void *) p);
        break;
      case EXP_ : //exponential
        if (UNLIKELY(min == 0 || max == 0))
          return csound->InitError(csound,
                                   Str("FLslider: zero is illegal "
                                       "in exponential operations"));
        range = max - min;
        o->range(0,range);
        #if defined(sun)
          p->base = ::pow((max / (double)min), 1.0/(double)range);
        #else
          p->base = ::pow((max / min), 1.0/(double)range);
        #endif
        o->callback((Fl_Callback*)fl_callbackExponentialSlider,(void *) p);
        break;
      default:
        {
          FUNC *ftp;
          MYFLT fnum = abs(iexp);
          if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL) {
            p->table = ftp->ftable;
            p->tablen = ftp->flen;
          }
          else return NOTOK;
          o->range(0,0.99999999);
          if (iexp > 0) //interpolated
            o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
          else // non-interpolated
            o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
        }
      }
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                                (void *) o, (void *) p));
      /*ST(currentSnapGroup);*/
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  static int fl_slider_bank_(CSOUND *csound, FLSLIDERBANK *p, int istring)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char s[MAXNAME];
      bool plastic = false;
      if (istring)
        strncpy(s, ((STRINGDAT*) p->names)->data, MAXNAME-1);
      else if ((long) *p->names <= csound->GetStrsmax(csound) &&
               csound->GetStrsets(csound,(long) *p->names)) {
        strncpy(s, csound->GetStrsets(csound,(long) *p->names), MAXNAME-1);
      }
      string tempname(s);
      stringstream sbuf;
      sbuf << tempname;

      int width = (int) *p->iwidth;
      if (width <=0) width = 100;

      Fl_Group* w = new Fl_Group((int)*p->ix, (int)*p->iy,
                                 width, (int)*p->inumsliders*10);
      FUNC *ftp;
      MYFLT *minmaxtable = NULL, *typetable = NULL, *outable, *exptable = NULL;

      MYFLT *zkstart;
      int zklast = csound->GetZakBounds(csound, &zkstart);
      if (*p->ioutable  < 1) {
        if (LIKELY(zkstart != NULL &&
                   zklast > (long)(*p->inumsliders+*p->ioutablestart_ndx)))
          outable = zkstart + (long) *p->ioutablestart_ndx;
        else {
          return csound->InitError(csound,
                                   Str("invalid ZAK space allocation"));
        }
      }
      else {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->ioutable)) != NULL))
          outable = ftp->ftable + (long) *p->ioutablestart_ndx;
        else
          return NOTOK;
      }
      if ((int) *p->iminmaxtable > 0) {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->iminmaxtable)) != NULL))
          minmaxtable = ftp->ftable;
        else return NOTOK;
      }
      if ((int) *p->iexptable > 0) {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->iexptable)) != NULL))
          exptable = ftp->ftable;
        else return NOTOK;
      }
      if ((int) *p->itypetable > 0) {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->itypetable)) != NULL))
          typetable = ftp->ftable;
        else return NOTOK;
      }

      for (int j =0; j< *p->inumsliders; j++) {
        string stemp;
        if (tempname == " ") {
          char s[40];
          sprintf(s, "%d", j);
          stemp = s;
        }
        else
          getline(sbuf, stemp, '@');
        char *Name =  new char[stemp.size()+2];
        strcpy(Name,stemp.c_str());
        ST(allocatedStrings).push_back(Name);

        int x = (int) *p->ix,  y = (int) *p->iy + j*10;
        Fl_Slider *o;
        int slider_type;
        if ((int) *p->itypetable <= 0) {    // no slider type table
          if (*p->itypetable >= -7)         //  all sliders are of the same type
            slider_type = -((int) *p->itypetable);
          else if (*p->itypetable >= -27 && *p->itypetable < -20) {
            slider_type = -((int) *p->itypetable) - 20;
            plastic = true;
          }
          else                              // random type
            slider_type = rand_31_i(csound, 7) | 1;
        }
        else
          slider_type = (int) typetable[j];
        if (slider_type > 20) {
          plastic = true;
          slider_type -= 20;
        }
        if (slider_type < 10)
          o = new Fl_Slider(x, y, width, 10, Name);
        else {
          o = new Fl_Value_Slider_Input(csound, x, y, width, 10, Name);
          slider_type -=10;
          ((Fl_Value_Slider_Input*) o)->textboxsize(50);
          ((Fl_Value_Slider_Input*) o)->textsize(13);
        }
        switch((int) slider_type) { //type
        case 1: o->type(FL_HOR_FILL_SLIDER); break;
        case 3: o->type(FL_HOR_SLIDER); break;
        case 5: o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        case 7:  o->type(FL_HOR_NICE_SLIDER); o->box(FL_DOWN_BOX); break;
        default: o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        }
        if (plastic) o->box(FL_PLASTIC_DOWN_BOX);
        o->align(FL_ALIGN_LEFT);
        widget_attributes(csound, o);
        MYFLT min, max, range;
        if ((int) *p->iminmaxtable > 0) {
          min = minmaxtable[j*2];
          max = minmaxtable[j*2+1];
        }
        else {
          min = MYFLT(0.0);
          max = MYFLT(1.0);
        }
        int iexp;

        p->slider_data[j].min=min;
        p->slider_data[j].max=max;
        p->slider_data[j].out=&outable[j];

        if ((int) *p->iexptable <=0)
          // no table, all sliders have the same behaviour
          iexp = (int) *p->iexptable;
        else
          iexp = (int) exptable[j];
        switch (iexp) {
        case -1: iexp = EXP_; break;
        case 0: iexp = LIN_; break;
        }

        MYFLT val = 0;
        p->slider_data[j].exp = iexp;
        switch (iexp) {
        case LIN_: //linear
          o->range(min,max);
          o->callback((Fl_Callback*)fl_callbackLinearSliderBank,
                      (void *) &(p->slider_data[j]));
          val = outable[j];
          if (val > max) val = max;
          else if (val < min) val = min;
          break;
        case EXP_ : //exponential
          if (UNLIKELY(min == 0 || max == 0))
            return
              csound->InitError(csound,
                                Str("FLslidBnk: zero is illegal "
                                    "in exponential operations"));
          range = max - min;
          o->range(0,range);
          #if defined(sun)
            p->slider_data[j].base = ::pow((max / (double)min), 1.0/(double)range);
          #else
            p->slider_data[j].base = ::pow((max / min), 1.0/(double)range);
          #endif
          o->callback((Fl_Callback*)fl_callbackExponentialSliderBank,
                      (void *) &(p->slider_data[j]));
          {
            val = outable[j];
            MYFLT range = max-min;
            #if defined(sun)
              MYFLT base = ::pow(max / (double)min, 1.0/(double)range);
            #else
              MYFLT base = ::pow(max / min, 1.0/(double)range);
            #endif
            val = (log(val/min) / log(base)) ;
          }
          break;
        default:
          {
            FUNC *ftp;
            MYFLT fnum = abs(iexp);
            if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL)
              p->slider_data[j].table = ftp->ftable;
            else return NOTOK;
            p->slider_data[j].tablen = ftp->flen;
            o->range(0,0.99999999);
            if (iexp > 0) //interpolated
              o->callback((Fl_Callback*)fl_callbackInterpTableSliderBank,
                          (void *)  &(p->slider_data[j]));
            else // non-interpolated
              o->callback((Fl_Callback*)fl_callbackTableSliderBank,
                          (void *)  &(p->slider_data[j]));
          }
        }
        o->value(val);
      }
      w->resizable(w);
      if (*p->iwidth <=0 || *p->iheight <=0) {// default width and height
        int a,b;
        w->size( a= w->parent()->w() -50, b= w->parent()->h());
        w->position(50, 0);
      }
      else {
        w->size( (int)*p->iwidth, (int)*p->iheight);
        w->position((int)*p->ix, (int)*p->iy);
      }
      w->end();
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(LIN_, 0, 0, (void *) w,
                                                (void *) p, ST(currentSnapGroup)));
      //*p->ihandle = ST(AddrSetValue).size()-1;
      ST(last_sldbnk) = ST(AddrSetValue).size()-1;  //gab
      return OK;
  }

  static int fl_slider_bank(CSOUND *csound, FLSLIDERBANK *p){
    return fl_slider_bank_(csound,p,0);
  }

    static int fl_slider_bank_S(CSOUND *csound, FLSLIDERBANK *p){
    return fl_slider_bank_(csound,p,1);
  }

  static int fl_joystick(CSOUND *csound, FLJOYSTICK *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *Name = p->name->data;
      int ix,iy,iwidth, iheight, iexpx, iexpy;

      if (*p->ix < 0)  ix = 10; // omitted options: set default
      else  ST(FL_ix) = ix = (int) *p->ix;
      if (*p->iy < 0)  iy = 10; // omitted options: set default
      else  iy = (int) *p->iy;
      if (*p->iwidth < 0) iwidth = 130;
      else iwidth = (int) *p->iwidth;
      if (*p->iheight < 0) iheight = 130;
      else iheight = (int) *p->iheight;

      switch((int) *p->iexpx) {
      case -1: iexpx = EXP_; break;
      case 0: iexpx = LIN_; break;
      default: iexpx = (int) *p->iexpx;
      }
      switch((int) *p->iexpy) {
      case -1: iexpy = EXP_; break;
      case 0: iexpy = LIN_; break;
      default: iexpy = (int) *p->iexpy;
      }
      /*
        if (*p->iexpx == LIN_) iexpx = LIN_;
        else  iexpx = (int) *p->iexpx;
        if (*p->iexpy == LIN_) iexpy = LIN_;
        else  iexpy = (int) *p->iexpy;
      */

      Fl_Positioner *o = new Fl_Positioner(ix, iy, iwidth, iheight, Name);
      widget_attributes(csound, o);
      switch (iexpx) {
      case LIN_: //linear
        o->xbounds(*p->iminx,*p->imaxx); break;
      case EXP_: //exponential
        { if (UNLIKELY(*p->iminx == 0 || *p->imaxx == 0))
            return csound->InitError(csound,
                                     Str("FLjoy X axe: zero is illegal "
                                         "in exponential operations"));
          MYFLT range = *p->imaxx - *p->iminx;
          o->xbounds(0,range);
          #if defined(sun)
            p->basex = ::pow((*p->imaxx / (double)*p->iminx), 1.0/(double)range);
          #else
            p->basex = ::pow((*p->imaxx / *p->iminx), 1.0/(double)range);
          #endif
        } break;
      default:
        {
          FUNC *ftp;
          MYFLT fnum = abs(iexpx);
          if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL) {
            p->tablex = ftp->ftable;
            p->tablenx = ftp->flen;
          }
          else return NOTOK;
          o->xbounds(0,0.99999999);
          /*
            if (iexp > 0) //interpolated
            o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
            else // non-interpolated
            o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
          */
        }
      }
      switch (iexpy) {
      case LIN_: //linear
        o->ybounds(*p->imaxy,*p->iminy); break;
      case EXP_ : //exponential
        { if (UNLIKELY(*p->iminy == 0 || *p->imaxy == 0))
            return csound->InitError(csound,
                                     Str("FLjoy X axe: zero is illegal "
                                         "in exponential operations"));
          MYFLT range = *p->imaxy - *p->iminy;
          o->ybounds(range,0);
          #if defined(sun)
            p->basey = ::pow((*p->imaxy / (double)*p->iminy), 1.0/(double)range);
          #else
            p->basey = ::pow((*p->imaxy / *p->iminy), 1.0/(double)range);
          #endif
        } break;
      default:
        {
          FUNC *ftp;
          MYFLT fnum = abs(iexpy);
          if (LIKELY((ftp = csound->FTnp2Find(csound, &fnum)) != NULL)) {
            p->tabley = ftp->ftable;
            p->tableny = ftp->flen;
          }
          else return NOTOK;
          o->ybounds(0,0.99999999);
          /*  if (iexp > 0) //interpolated
              o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
              else // non-interpolated
              o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
          */
        }
      }
      o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
      o->callback((Fl_Callback*)fl_callbackJoystick,(void *) p);
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(iexpx, *p->iminx, *p->imaxx,
                                                (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->ihandle1 = ST(AddrSetValue).size()-1;
      ADDR_SET_VALUE *asv = &ST(AddrSetValue)[(int) *p->ihandle1];
      asv->widg_type = FL_JOY;
      asv->joy = JOY_X;
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(iexpy, *p->iminy, *p->imaxy,
                                                (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->ihandle2 = ST(AddrSetValue).size()-1;
      asv = &ST(AddrSetValue)[(int) *p->ihandle2];
      asv->widg_type = FL_JOY;
      asv->joy = JOY_Y;
      return OK;
  }

  static int fl_knob(CSOUND *csound, FLKNOB *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char    *controlName = p->name->data;
      int     ix, iy, iwidth, itype, iexp;

      if (*p->iy < 0) iy = ST(FL_iy);
      else  ST(FL_iy) = iy = (int) *p->iy;
      if (*p->ix < 0)  ix = ST(FL_ix);
      else  ST(FL_ix) = ix = (int) *p->ix;
      if (*p->iwidth < 0) iwidth = ST(FLcontrol_iwidth);
      else ST(FLcontrol_iwidth) = iwidth = (int) *p->iwidth;
      if (*p->itype < 1) itype = 1;
      else  itype = (int) *p->itype;
      /*
        if (*p->iexp < LIN_) iexp = LIN_;
        else  iexp = (int) *p->iexp;
      */
      switch((int) *p->iexp) {
      case -1: iexp = EXP_; break;
      case 0: iexp = LIN_; break;
      default: iexp = (int) *p->iexp;
      }

      Fl_Valuator* o;
      switch (itype) {
      case 1:
        o = new Fl_Knob(csound, ix, iy, iwidth, iwidth, controlName);
        o->box(FL_NO_BOX);
        if (*p->icursorsize > MYFLT(0.5))
          ((Fl_Knob*) o)->cursor((int) (*p->icursorsize + MYFLT(0.5)));
        break;
      case 2:
        o = new Fl_Dial(ix, iy, iwidth, iwidth, controlName);
        o->type(FL_FILL_DIAL);
        o->box(_FL_OSHADOW_BOX);
        ((Fl_Dial*) o)->angles(20, 340);
        break;
      case 3:
        o = new Fl_Dial(ix, iy, iwidth, iwidth, controlName);
        o->type(FL_LINE_DIAL);
        o->box(_FL_OSHADOW_BOX);
        break;
      case 4:
        o = new Fl_Dial(ix, iy, iwidth, iwidth, controlName);
        o->type(FL_NORMAL_DIAL);
        o->box(_FL_OSHADOW_BOX);
        break;
      default:
        return csound->InitError(csound,
                                 Str("FLknob: invalid knob type"));
      }
      widget_attributes(csound, o);
      o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
      o->range(*p->imin, *p->imax);
      switch (iexp) {
      case LIN_: //linear
        o->range(*p->imin,*p->imax);
        o->callback((Fl_Callback*)fl_callbackLinearKnob,(void *) p);
        o->step(0.001);
        break;
      case EXP_ : //exponential
        {
          MYFLT min = p->min = *p->imin, max = *p->imax;
          if (UNLIKELY(min == 0 || max == 0))
            return csound->InitError(csound,
                                     Str("FLknob: zero is illegal "
                                         "in exponential operations"));
          MYFLT range = max - min;
          o->range(0,range);
          #if defined(sun)
            p->base = ::pow((max / (double)min), 1.0/(double)range);
          #else
            p->base = ::pow((max / min), 1.0/(double)range);
          #endif
          o->callback((Fl_Callback*)fl_callbackExponentialKnob,(void *) p);
        } break;
      default:
        {
          FUNC *ftp;
          p->min = *p->imin;
          MYFLT fnum = abs(iexp);
          if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL) {
            p->table = ftp->ftable;
            p->tablen = ftp->flen;
          }
          else return OK;
          o->range(0,0.99999999);
          if (iexp > 0) //interpolated
            o->callback((Fl_Callback*)fl_callbackInterpTableKnob,(void *) p);
          else // non-interpolated
            o->callback((Fl_Callback*)fl_callbackTableKnob,(void *) p);
        }
      }
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                                (void *) o, (void *) p));
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  static int fl_text(CSOUND *csound, FLTEXT *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *controlName = p->name->data;
      int ix,iy,iwidth,iheight,itype;
      MYFLT   istep;

      if (*p->iy < 0) iy = ST(FL_iy);
      else  ST(FL_iy) = iy = (int) *p->iy;
      if (*p->ix < 0)  ix = ST(FL_ix);
      else  ST(FL_ix) = ix = (int) *p->ix;
      if (*p->iwidth < 0) iwidth = ST(FLcontrol_iwidth);
      else ST(FLcontrol_iwidth) = iwidth = (int) *p->iwidth;
      if (*p->iheight < 0) iheight = ST(FLcontrol_iheight);
      else ST(FLcontrol_iheight) = iheight = (int) *p->iheight;
      if (*p->itype < 1) itype = 1;
      else  itype = (int) *p->itype;
      if (*p->istep < 0) istep = MYFLT(.1);
      else  istep = *p->istep;

      Fl_Valuator* o;
      switch(itype) {
      case 1:
        {
          o = new Fl_Value_Input(ix, iy, iwidth, iheight, controlName);
          ((Fl_Value_Input *) o)->step(istep);
          ((Fl_Value_Input *) o)->range(*p->imin,*p->imax);
        }
        break;
      case 2:
        {
          o = new Fl_Value_Input_Spin(csound, ix, iy,
                                      iwidth, iheight, controlName);
          ((Fl_Value_Input *) o)->step(istep);
          ((Fl_Value_Input *) o)->range(*p->imin,*p->imax);
        }
        break;
      case 3:
        {
          o = new Fl_Value_Output(ix, iy, iwidth, iheight, controlName);
          ((Fl_Value_Output *) o)->step(istep);
          ((Fl_Value_Output *) o)->range(*p->imin,*p->imax);
        }
        break;
      default:
        return NOTOK;
      }
      o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
      widget_attributes(csound, o);
      o->callback((Fl_Callback*)fl_callbackLinearValueInput,(void *) p);
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(1, *p->imin, *p->imax,
                                                (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  static int fl_button(CSOUND *csound, FLBUTTON *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *Name = p->name->data;
      int type = (int) *p->itype;
      bool plastic = false;
      if (type > 19) {
        type = type - 20;
        plastic = true;
      }
      if (UNLIKELY(type > 9)) {       // ignored when getting ST(snapshots)
        csound->Warning(csound,
                        Str("FLbutton \"%s\" ignoring snapshot capture retrieve"),
                        Name);
        type = type - 10;
      }
      Fl_Button *w;
      *p->kout = *p->ioff;        // IV - Aug 27 2002

      switch (type) {
      case 1:
        w = new Fl_Button((int)*p->ix, (int)*p->iy,
                          (int)*p->iwidth, (int)*p->iheight, Name);
        if (plastic) {
          w->box(FL_PLASTIC_UP_BOX);
          w->down_box(FL_PLASTIC_DOWN_BOX);
        }
        break;
      case 2:
        w = new Fl_Light_Button((int)*p->ix, (int)*p->iy,
                                (int)*p->iwidth, (int)*p->iheight, Name);
        if (plastic) {
          w->box(FL_PLASTIC_UP_BOX);
          //       w->down_box(FL_PLASTIC_DOWN_BOX);  //Doesn't work
        }
        break;
      case 3:
        w = new Fl_Check_Button((int)*p->ix, (int)*p->iy,
                                (int)*p->iwidth, (int)*p->iheight, Name);
        if (plastic) {
          w->box(FL_PLASTIC_UP_BOX);
          w->down_box(FL_PLASTIC_DOWN_BOX);
        }
        break;
      case 4:
        w = new Fl_Round_Button((int)*p->ix, (int)*p->iy,
                                (int)*p->iwidth, (int)*p->iheight, Name);
        if (plastic) {
          w->box(FL_PLASTIC_UP_BOX);
          w->down_box(FL_PLASTIC_DOWN_BOX);
        }
        break;
      default:
        return csound->InitError(csound,
                                 Str("FLbutton: invalid button type"));
      }
      Fl_Button *o = w;
      o->align(FL_ALIGN_WRAP);
      widget_attributes(csound, o);
      if (type == 1)
        o->callback((Fl_Callback*) fl_callbackButton1, (void*) p);
      else
        o->callback((Fl_Callback*) fl_callbackButton, (void*) p);
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->ihandle = ST(AddrSetValue).size() - 1;

      return OK;
  }

  static int fl_close_button(CSOUND *csound, FLCLOSEBUTTON *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *Name = p->name->data;

      Fl_Button *w;

      w = new Fl_Button((int)*p->ix, (int)*p->iy,
                        (int)*p->iwidth, (int)*p->iheight, Name);

      Fl_Button *o = w;

      o->align(FL_ALIGN_WRAP);
      widget_attributes(csound, o);

      ADDR_STACK adrstk = ST(AddrStack).back();
      if (UNLIKELY(strcmp( adrstk.h->optext->t.opcod, "FLpanel")))
        return csound->InitError(csound,
                                 Str("FLcloseButton: invalid stack"
                                     " pointer: verify its placement"));

      o->callback((Fl_Callback*) fl_callbackCloseButton,
                  (void*) adrstk.WidgAddress);

      ST(AddrSetValue).push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p));
      *p->ihandle = ST(AddrSetValue).size() - 1;

      return OK;
  }

  static int fl_exec_button(CSOUND *csound, FLEXECBUTTON *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      p->csound = csound;
      Fl_Button *w;

      p->commandString = p->command->data;

      csound->Message(csound, Str("Command Found: %s\n"), p->commandString);

      w = new Fl_Button((int)*p->ix, (int)*p->iy,
                        (int)*p->iwidth, (int)*p->iheight, Str("About"));

      Fl_Button *o = w;

      o->align(FL_ALIGN_WRAP);
      widget_attributes(csound, o);

      o->callback((Fl_Callback*) fl_callbackExecButton, (void*) p);

      ST(AddrSetValue).push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->ihandle = ST(AddrSetValue).size() - 1;

      return OK;
  }

  static int fl_button_bank(CSOUND *csound, FLBUTTONBANK *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char *Name = (char*)"/0";
      int type = (int) *p->itype;
      bool plastic = false;
      if (type > 20) {
        plastic = true;
        type = type - 20;
      }
      if (UNLIKELY(type > 9)) {       // ignored when getting ST(snapshots)
        csound->Warning(csound,
                        Str("FLbutton \"%s\" ignoring snapshot capture retrieve"),
                        Name);
        type = type - 10;
      }
      Fl_Group* o = new Fl_Group((int)*p->ix, (int)*p->iy,
                                 (int)*p->inumx * 10, (int)*p->inumy*10);
      int z = 0;
      for (int j = 0; j < *p->inumx; j++) {
        for (int k = 0; k < *p->inumy; k++) {
          int       x = (int) *p->ix + j*10, y = (int) *p->iy + k*10;
          Fl_Button *w;
          char      *btName = new char[30];
          ST(allocatedStrings).push_back(btName);
          sprintf(btName, "%d", z);
          switch (type) {
          case 1:
            w = new Fl_Button(x, y, 10, 10, btName);
            if (plastic) {
              w->box(FL_PLASTIC_UP_BOX);
              w->down_box(FL_PLASTIC_DOWN_BOX);
            }
            break;
          case 2:
            w = new Fl_Light_Button(x, y, 10, 10, btName);
            if (plastic) {
              w->box(FL_PLASTIC_UP_BOX);
            }
            break;
          case 3:
            w = new Fl_Check_Button(x, y, 10, 10, btName);
            if (plastic) {
              w->box(FL_PLASTIC_UP_BOX);
              w->down_box(FL_PLASTIC_DOWN_BOX);
            }
            break;
          case 4:
            w = new Fl_Round_Button(x, y, 10, 10, btName);
            if (plastic) {
              w->box(FL_PLASTIC_UP_BOX);
              w->down_box(FL_PLASTIC_DOWN_BOX);
            }
            break;
          default: return csound->InitError(csound,
                                            Str("FLbuttonBank: "
                                                "invalid button type"));
          }
          widget_attributes(csound, w);
          w->type(FL_RADIO_BUTTON);
          w->callback((Fl_Callback*) fl_callbackButtonBank, (void *) p);
          if (!z)
            w->value(1);
          z++;
        }
      }
      o->resizable(o);
      //*p->ix, *p->iy,
      o->size( (int)*p->iwidth, (int)*p->iheight);
      o->position((int)*p->ix, (int)*p->iy);
      o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
      o->end();

      ST(AddrSetValue).push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->kout = MYFLT(0.0);
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  static int fl_counter(CSOUND *csound, FLCOUNTER *p)
  {
      char *controlName = p->name->data;
      //      int ix,iy,iwidth,iheight,itype;
      //      MYFLT   istep1, istep2;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      Fl_Counter* o = new Fl_Counter((int)*p->ix, (int)*p->iy,
                                     (int)*p->iwidth, (int)*p->iheight,
                                     controlName);
      widget_attributes(csound, o);
      int type = (int) *p->itype;
      if (UNLIKELY(type >9 )) { // ignored when getting ST(snapshots)
        csound->Warning(csound,
                        Str("FLcount \"%s\" ignoring snapshot capture retrieve"),
                        controlName);
        type = type-10;
      }
      switch(type) {
      case 1: o->type(FL_NORMAL_COUNTER);  break;
      case 2: o->type(FL_SIMPLE_COUNTER);  break;
      default:o->type(FL_NORMAL_COUNTER);  break;
      }

      o->step(*p->istep1);
      o->lstep(*p->istep2);
      o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
      // range is accepted only if min and max are different
      if (*p->imin != *p->imax)
        o->range(*p->imin,*p->imax); //otherwise no-range
      widget_attributes(csound, o);
      o->callback((Fl_Callback*)fl_callbackCounter,(void *) p);
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(1, 0, 100000, (void *) o,
                                                (void *) p, ST(currentSnapGroup)));
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  static int fl_roller(CSOUND *csound, FLROLLER *p)
  {
      char *controlName = p->name->data;
      int ix,iy,iwidth, iheight,itype, iexp ;
      double istep;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (*p->iy < 0) {
        iy = ST(FL_iy);
        ST(FL_iy) += ST(FLroller_iheight) + 15;
      }
      else {
        iy = (int) *p->iy;
        ST(FL_iy) = iy + ST(FLroller_iheight) + 15;
      }
      // omitted options: set defaults
      if (*p->ix<0) ix = ST(FL_ix);       else  ST(FL_ix) = ix = (int) *p->ix;
      if (*p->iy<0) iy = ST(FL_iy);       else  ST(FL_iy) = iy = (int) *p->iy;
      if (*p->iwidth<0) iwidth = ST(FLroller_iwidth);
      else ST(FLroller_iwidth) = iwidth = (int) *p->iwidth;
      if (*p->iheight<0) iheight = ST(FLroller_iheight);
      else ST(FLroller_iheight) = iheight = (int) *p->iheight;
      if (*p->itype<1) itype = 1;
      else  itype = (int) *p->itype;
      //if (*p->iexp<LIN_) iexp = LIN_;
      //else  iexp = (int) *p->iexp;
      switch((int) *p->iexp) {
      case -1: iexp = EXP_; break;
      case 0: iexp = LIN_; break;
      default: iexp = (int) *p->iexp;
      }

      if (*p->istep<0) istep = 1;     else  istep =  *p->istep;
      p->min = *p->imin;
      Fl_Roller *o;
      switch (itype) {
      case 1:
        o = new Fl_Roller(ix, iy, iwidth, iheight, controlName);
        o->type(FL_HORIZONTAL);
        break;
      case 2:
        o = new Fl_Roller(ix, iy, iwidth, iheight, controlName);
        o->type(FL_VERTICAL);
        break;
      default:
        return csound->InitError(csound,
                                 Str("FLroller: invalid roller type"));
      }
      widget_attributes(csound, o);
      o->step(istep);
      switch (iexp) {
      case LIN_: //linear
        o->range(*p->imin,*p->imax);
        o->callback((Fl_Callback*)fl_callbackLinearRoller,(void *) p);
        break;
      case EXP_ : //exponential
        {
          MYFLT min = p->min, max = *p->imax;
          if (UNLIKELY(min == 0 || max == 0))
            return csound->InitError(csound,
                                     Str("FLslider: zero is illegal "
                                         "in exponential operations"));
          MYFLT range = max - min;
          o->range(0,range);
          #if defined(sun)
            p->base = ::pow((max / (double)min), 1.0/(double)range);
          #else
            p->base = ::pow((max / min), 1.0/(double)range);
          #endif
          o->callback((Fl_Callback*)fl_callbackExponentialRoller,(void *) p);
        }
        break;
      default:
        {
          FUNC *ftp;
          MYFLT fnum = abs(iexp);
          if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL) {
            p->table = ftp->ftable;
            p->tablen = ftp->flen;
          }
          else return NOTOK;
          o->range(0,0.99999999);
          if (iexp > 0) //interpolated
            o->callback((Fl_Callback*)fl_callbackInterpTableRoller,(void *) p);
          else // non-interpolated
            o->callback((Fl_Callback*)fl_callbackTableRoller,(void *) p);
        }
      }
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                                (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;
  }

  static int FLprintkset(CSOUND *csound, FLPRINTK *p)
  {
    //WIDGET_GLOBALS *widgetGlobals =
    //(WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
    if (*p->ptime < MYFLT(1.0) / CS_EKR)
      p->ctime = MYFLT(1.0) / CS_EKR;
      else        p->ctime = *p->ptime;

    p->initime = (MYFLT) csound->GetKcounter(csound) * (1.0/CS_EKR);
      p->cysofar = -1;
      return OK;
  }

  static int FLprintk(CSOUND *csound, FLPRINTK *p)
  {
      MYFLT   timel;
      long    cycles;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

      timel = ((MYFLT) csound->GetKcounter(csound) *
               (1.0/CS_EKR)) - p->initime;
      cycles = (long)(timel / p->ctime);
      if (p->cysofar < cycles) {
        p->cysofar = cycles;
        char valString[MAXNAME];
        sprintf(valString,"%.5g", *p->val);
        ((Fl_Output*) (ST(AddrSetValue)[(long) *p->idisp]).WidgAddress)->
          value(valString );
      }
      return OK;
  }

  static int FLprintk2set(CSOUND *csound, FLPRINTK2 *p)   // IV - Aug 27 2002
  {
      p->oldvalue = MYFLT(-1.12123e35);        // hack to force printing first value
      return OK;
  }

  static int FLprintk2(CSOUND *csound, FLPRINTK2 *p)
  {
      MYFLT   value = *p->val;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (p->oldvalue != value) {
        char valString[MAXNAME];
        sprintf(valString,"%.5g", *p->val);
        ((Fl_Output*) (ST(AddrSetValue)[(long) *p->idisp]).WidgAddress)->
          value(valString );
        p->oldvalue = value;
      }
      return OK;
  }

  // New opcodes for Csound5.06 by Gabriel Maldonado, ported by Andres Cabrera

  void skin(CSOUND* csound, Fl_Widget *o, int imgNum, bool isTiled)
  {
    // WIDGET_GLOBALS *widgetGlobals =
    //  (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
#ifdef CS_IMAGE
      extern void* get_image(CSOUND* csound, int imgNum);
      ImageSTRUCT *bmp =  (ImageSTRUCT*) get_image(csound, imgNum);
      FLlock();
      Fl_RGB_Image *img = new Fl_RGB_Image((BYTE*) (bmp->data),
                                           bmp->xsize,bmp->ysize,bmp->csize);
      ST(allocatedStrings).push_back((char *) img);
      if (isTiled) {
        Fl_Tiled_Image *t_img = new Fl_Tiled_Image(img);
        o->image(t_img);
        ST(allocatedStrings).push_back((char *) t_img);
      }
      else {
        o->image(img);
      }
      o->align(FL_ALIGN_INSIDE);
      FLunlock();
#endif
  }

  class HVS_BOX : public Fl_Box{
    int rx,ry,rw,rh;
    int red, green, blue;
    //      Fl_PNG_Image *img;
  public:

    int numLinesX,numLinesY;
    MYFLT valueX, valueY;

    HVS_BOX(int numLinesX_, int numLinesY_, int x, int y, int w, int h) :
      Fl_Box(x,y,w,h) {
        numLinesX = numLinesX_ - 1;
        numLinesY = numLinesY_ - 1;
        valueX = valueY = .5;
        rx = rw = ry = rh = 0;
        red = green = blue = 0;
        //      if (filename != NULL) {
        //       img = new Fl_PNG_Image(filename);
        //       this->image(img);
        //      }
    }
    void  draw() {
        Fl_Box::draw();

        fl_color(selection_color());
        MYFLT Xincr = w()/(MYFLT) numLinesX;
        MYFLT Yincr = h()/(MYFLT) numLinesY;
        fl_color(FL_RED);
        for (int j = 1; j < numLinesX; j++) {
          fl_yxline((int) (j*Xincr+x()), y(),    h()+y());
        }
        for (int k = 1; k <numLinesY; k++) {
          fl_xyline(x(), (int) (k*Yincr+y()), w()+x()-2);
        }
        fl_color(FL_CYAN);
        fl_yxline((int) (valueX*w()+x()), y(),    h()+y());
        fl_xyline(x(), (int) (valueY*h()+y()), w()+x()-2);
        fl_color(FL_BLACK);
        fl_rect((int) (valueX*w()+x()-6), (int) (valueY*h()+y()-6), 12, 12);
        fl_color(FL_WHITE);
        fl_rect((int) (valueX*w()+x()-10), (int) (valueY*h()+y()-10), 20, 20);
    }

    virtual int handle(int event) {
        switch (event) {
        case FL_PUSH:
        case FL_DRAG:
        case FL_RELEASE:
          valueX = (MYFLT) (Fl::event_x() - x()) / (MYFLT) w();
          valueY = (MYFLT) (Fl::event_y() - y()) / (MYFLT) h();
          redraw();
          return 1;
        default:
          return 0;

        }
    }
    void setXY(MYFLT xx, MYFLT yy) {
        valueX = xx;
        valueY = yy;
        damage(FL_DAMAGE_ALL);
        redraw();
    }

    /*
      void set_rect (int newrx, int newry, int newrw, int newrh) {
      rx = newrx; ry=newry; rw=newrw; rh=newrh;
      redraw();
      }
      void set_color (int newred, int newgreen, int newblue){
      red=newred; blue=newblue; green=newgreen;
      }
    */
  };

  static int fl_hvsbox(CSOUND *csound,FL_HVSBOX *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (UNLIKELY(*p->numlinesX < 2 || *p->numlinesY < 2))
        return csound->InitError(csound,
                                 Str("FLhvsBox: a square area must be"
                                     " delimited by 2 lines at least"));

      HVS_BOX *o =  new HVS_BOX((int) *p->numlinesX,(int)  *p->numlinesY,
                                (int) *p->ix,(int)  *p->iy,
                                (int)  *p->iwidth, (int) *p->iheight);
      widget_attributes(csound,o);
      o->box(FL_DOWN_BOX);
      if (*p->image >= 0) skin(csound, o, (int) *p->image, false);

      ST(AddrSetValue).push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p,
                                                ST(currentSnapGroup)));
      *p->ihandle = ST(AddrSetValue).size()-1;
      return OK;

  }

  static int fl_setHvsValue_set(CSOUND *csound,FL_SET_HVS_VALUE *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ADDR_SET_VALUE v = ST(AddrSetValue)[(int) *p->ihandle];
      p->WidgAddress = v.WidgAddress;
      p->opcode = v.opcode;
      return OK;
  }

  static int fl_setHvsValue(CSOUND *csound,FL_SET_HVS_VALUE *p)
  {
      if(*p->kx != p->old_x || *p->ky != p->old_y ) {
        HVS_BOX *o = (HVS_BOX *) p->WidgAddress;
        FLlock();
        o->setXY(*p->kx, *p->ky);
        FLunlock();
        p->old_x = *p->kx;
        p->old_y = *p->ky;
      }
      return OK;
  }


  static int fl_keyin_set(CSOUND *csound,FLKEYIN *p)
  {
      FUNC* ftp;
      if (*p->ifn > 0) { // mapping values
        p->flag = 1;

        if (LIKELY((ftp = csound->FTnp2Find(csound,p->ifn)) != NULL))
          p->table = ftp->ftable;
        else {
          return csound->InitError(csound,
                                   Str("FLkeyIn: invalid table number"));
        }
        if (UNLIKELY(ftp->flen < 512)) {
          return csound->InitError(csound,
                                   Str("FLkeyIn: table too short!"));
        }
      }
      else p->flag = 0;
      return OK;
  }

  static int fl_keyin(CSOUND *csound,FLKEYIN *p)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (ST(last_KEY)) {
        int key;

        if ( ST(last_KEY) > 0 && ST(last_KEY) < 256)
          key = ST(last_KEY);
        else
          key = (ST(last_KEY) & 0xFF) + 256;  // function keys

        if(p->flag) {
          if(ST(isKeyDown))
            p->table[key] = 1;
          else
            p->table[key] = 0;
        }
        if (ST(isKeyDown)) *p->kascii = key;
        else *p->kascii = -key;
        ST(last_KEY) = 0;
      }
      return OK;
  }

  static int fl_setSnapGroup(CSOUND *csound, FLSETSNAPGROUP *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      ST(currentSnapGroup) = (int) *p->group;
      return OK;
  }

  static int fl_mouse_set(CSOUND *csound,FLMOUSE *p)
  {
      p->width= Fl::w();
      p->height= Fl::h();
      return OK;
  }

  static int fl_mouse(CSOUND *csound,FLMOUSE *p)
  {
      if (*p->flagRaw == 0) {
        *p->x = (MYFLT) Fl::event_x_root()/p->width;
        *p->y = 1.0 - ((MYFLT) Fl::event_y_root()/p->height);
      }
      else if (*p->flagRaw == 1) {
        *p->x = (MYFLT) Fl::event_x_root();
        *p->y = (MYFLT) Fl::event_y_root();
      }
      else if (*p->flagRaw == 2) {
        *p->x = (MYFLT) Fl::event_x();
        *p->y = (MYFLT) Fl::event_y();
      }
      //   *p->b1 = (MYFLT) (Fl::event_button1() >> 56);
      //   *p->b2 = (MYFLT) (Fl::event_button2() >> 57);
      //   *p->b3 = (MYFLT) (Fl::event_button3() >> 58);
      *p->b1 = (MYFLT) (Fl::event_button1() >> 24);
      *p->b2 = (MYFLT) (Fl::event_button2() >> 25);
      *p->b3 = (MYFLT) (Fl::event_button3() >> 26);

      return OK;
  }


  static int fl_vertical_slider_bank_(CSOUND *csound, FLSLIDERBANK *p, int istring)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char s[MAXNAME];
      bool plastic = false;
      if (istring)
        strncpy(s, ((STRINGDAT *)p->names)->data, MAXNAME-1);
      else if ((long) *p->names <= csound->GetStrsmax(csound) &&
               csound->GetStrsets(csound,(long) *p->names)) {
        strncpy(s, csound->GetStrsets(csound,(long) *p->names), MAXNAME-1);
      }
      string tempname(s);
      stringstream sbuf;
      sbuf << tempname;

      int height = (int) *p->iheight;
      if (height <=0) height = 100;
      Fl_Group* w = new Fl_Group((int)*p->ix, (int)*p->iy,
                                 (int)*p->inumsliders*10, height);
      FUNC *ftp;
      MYFLT *minmaxtable = NULL, *typetable = NULL, *outable, *exptable = NULL;

      MYFLT *zkstart;
      int zklast = csound->GetZakBounds(csound, &zkstart);
      if (*p->ioutable  < 1) {
        if (LIKELY(zkstart != NULL &&
                   zklast > (long)(*p->inumsliders+*p->ioutablestart_ndx)))
          outable = zkstart + (long) *p->ioutablestart_ndx;
        else {
          return csound->InitError(csound,
                                   Str("invalid ZAK space allocation"));
        }
      }
      else {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->ioutable)) != NULL))
          outable = ftp->ftable + (long) *p->ioutablestart_ndx;
        else
          return NOTOK;
      }
      if ((int) *p->iminmaxtable > 0) {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->iminmaxtable)) != NULL))
          minmaxtable = ftp->ftable;
        else return NOTOK;
      }
      if ((int) *p->iexptable > 0) {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->iexptable)) != NULL))
          exptable = ftp->ftable;
        else return NOTOK;
      }
      if ((int) *p->itypetable > 0) {
        if (LIKELY((ftp = csound->FTnp2Find(csound, p->itypetable)) != NULL))
          typetable = ftp->ftable;
        else return NOTOK;
      }
      p->elements = (long) *p->inumsliders;

      for (int j =0; j< p->elements; j++) {
        string stemp;
        if (tempname == " ") {
          char s[40];
          sprintf(s, "%d", j);
          stemp = s;
        }
        else
          getline(sbuf, stemp, '@');
        char *Name =  new char[stemp.size()+2];
        strcpy(Name,stemp.c_str());
        ST(allocatedStrings).push_back(Name);

        int x = (int) *p->ix+j*10,  y = (int) *p->iy /*+ j*10*/;
        Fl_Slider *o;
        int slider_type;
        if ((int) *p->itypetable <= 0) {    // no slider type table
          if (*p->itypetable >= -7)         //  all sliders are of the same type
            slider_type = -((int) *p->itypetable);
          else if (*p->itypetable >= -27 && *p->itypetable < -20) {
            slider_type = -((int) *p->itypetable) - 20;
            plastic = true;
          }
          else  {                      // random type
            slider_type = rand_31_i(csound, 7) | 1;
            switch(slider_type) {
            case 0: slider_type = 1; break;
            case 2: slider_type = 3; break;
            case 4: slider_type = 5; break;
            case 6: slider_type = 7; break;
            default: slider_type = 1;
            }
          }
        }
        else
          slider_type = (int) typetable[j];
        if (slider_type > 20) {
          plastic = true;
          slider_type -= 20;
        }
        if (slider_type < 10)
          o = new Fl_Slider(x, y, 10, height, Name);
        else {
          o = new Fl_Value_Slider_Input(csound, x, y, 10, height, Name);
          slider_type -=10;
          ((Fl_Value_Slider_Input*) o)->textboxsize(50);
          ((Fl_Value_Slider_Input*) o)->textsize(13);
        }
        switch((int) slider_type) { //type
        case 1: o->type(FL_VERT_FILL_SLIDER); break;
        case 3: o->type(FL_VERT_SLIDER); break;
        case 5: o->type(FL_VERT_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        case 7:  o->type(FL_VERT_NICE_SLIDER); o->box(FL_DOWN_BOX); break;
        default: o->type(FL_VERT_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        }
        o->align(FL_ALIGN_BOTTOM);
        if (plastic) o->box(FL_PLASTIC_DOWN_BOX);
        widget_attributes(csound, o);
        MYFLT min, max, range;
        if ((int) *p->iminmaxtable > 0) {
          min = minmaxtable[j*2];
          max = minmaxtable[j*2+1];
        }
        else {
          min = MYFLT(0.0);
          max = MYFLT(1.0);
        }
        int iexp;

        p->slider_data[j].min=min;
        p->slider_data[j].max=max;
        p->slider_data[j].out=&outable[j];

        if ((int) *p->iexptable <=0)
          // no table, all sliders have the same behaviour
          iexp = (int) *p->iexptable;
        else
          iexp = (int) exptable[j];
        switch (iexp) {
        case -1: iexp = EXP_; break;
        case 0: iexp = LIN_; break;
        }

        MYFLT val = 0;
        p->slider_data[j].exp = iexp;
        switch (iexp) {
        case LIN_: //linear
          o->range(min,max);
          o->callback((Fl_Callback*)fl_callbackLinearSliderBank,
                      (void *) &(p->slider_data[j]));
          val = outable[j];
          if (val > max) val = max;
          else if (val < min) val = min;
          break;
        case EXP_ : //exponential
          if (UNLIKELY(min == 0 || max == 0))
            return
              csound->InitError(csound,
                                Str("FLvslidBnk: zero is illegal "
                                    "in exponential operations"));
          range = max - min;
          o->range(range,0);
          #if defined(sun)
            p->slider_data[j].base = ::pow((max / (double)min), 1.0/(double)range);
          #else
            p->slider_data[j].base = ::pow((max / min), 1.0/(double)range);
          #endif
          o->callback((Fl_Callback*)fl_callbackExponentialSliderBank,
                      (void *) &(p->slider_data[j]));
          {
            val = outable[j];
            MYFLT range = max-min;
            #if defined(sun)
              MYFLT base = ::pow(max / (double)min, 1.0/(double)range);
            #else
              MYFLT base = ::pow(max / min, 1.0/(double)range);
            #endif
            val = (log(val/min) / log(base)) ;
          }
          break;
        default:
          {
            FUNC *ftp;
            MYFLT fnum = abs(iexp);
            if (LIKELY((ftp = csound->FTnp2Find(csound, &fnum)) != NULL))
              p->slider_data[j].table = ftp->ftable;
            else return NOTOK;
            p->slider_data[j].tablen = ftp->flen;
            o->range(.99999999,0);
            if (iexp > 0) //interpolated
              o->callback((Fl_Callback*)fl_callbackInterpTableSliderBank,
                          (void *)  &(p->slider_data[j]));
            else // non-interpolated
              o->callback((Fl_Callback*)fl_callbackTableSliderBank,
                          (void *)  &(p->slider_data[j]));
          }
        }
        o->value(val);
        p->slider_data[j].widget_addr= (void*) o;
      }
      w->resizable(w);
      if (*p->iwidth <=0 || *p->iheight <=0) {// default width and height
        int a,b;
        w->size( a= w->parent()->w(), b= w->parent()->h()-50);
        w->position(0, 0);
      }
      else {
        w->size( (int)*p->iwidth, (int)*p->iheight);
        w->position((int)*p->ix, (int)*p->iy);
      }
      w->end();
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(LIN_, 0, 0, (void *) w,
                                                (void *) p, ST(currentSnapGroup)));
      ST(last_sldbnk) = ST(AddrSetValue).size()-1;  //gab

      return OK;
  }

  static int fl_vertical_slider_bank(CSOUND *csound, FLSLIDERBANK *p){
    return fl_vertical_slider_bank_(csound,p,0);
  }

 static int fl_vertical_slider_bank_S(CSOUND *csound, FLSLIDERBANK *p){
    return fl_vertical_slider_bank_(csound,p,1);
  }


  static int fl_slider_bank2_(CSOUND *csound, FLSLIDERBANK2 *p, int istring)
  {
     WIDGET_GLOBALS *widgetGlobals =
       (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char s[MAXNAME];
      bool plastic = false;
      if (istring)
        strncpy(s, ((STRINGDAT*) p->names)->data, MAXNAME-1);
      else if ((long) *p->names <= csound->GetStrsmax(csound) &&
               csound->GetStrsets(csound,(long) *p->names)) {
        strncpy(s, csound->GetStrsets(csound,(long) *p->names), MAXNAME-1);
      }
      string tempname(s);
      stringstream sbuf;
      sbuf << tempname;

      int width = (int) *p->iwidth;
      if (width <=0) width = 100;

      Fl_Group* w = new Fl_Group((int)*p->ix, (int)*p->iy,
                                 width, (int)*p->inumsliders*10);
      FUNC *ftp;
      MYFLT *configtable, *outable;

      MYFLT *zkstart;
      int zklast = csound->GetZakBounds(csound, &zkstart);
      if (*p->ioutable  < 1) {
        if (LIKELY(zkstart != NULL &&
                   zklast>(long)(*p->inumsliders + *p->ioutablestart_ndx)))
          outable = zkstart + (long) *p->ioutablestart_ndx;
        else {
          return csound->InitError(csound,
                                   Str("invalid ZAK space allocation"));
        }
      }
      else {
        if ((ftp = csound->FTnp2Find(csound, p->ioutable)) != NULL)
          outable = ftp->ftable + (long) *p->ioutablestart_ndx;
        else
          return NOTOK;
      }
      if((ftp = csound->FTnp2Find(csound, p->iconfigtable)) != NULL)
        configtable = ftp->ftable;
      else return NOTOK;

      p->elements = (long) *p->inumsliders;
      for (int j =0; j< p->elements; j++) {
        string stemp;
        if (tempname == " ") {
          char s[40];
          sprintf(s, "%d", j);
          stemp = s;
        }
        else
          getline(sbuf, stemp, '@');
        char *Name =  new char[stemp.size()+2];
        strcpy(Name,stemp.c_str());
        ST(allocatedStrings).push_back(Name);

        int x = (int) *p->ix,  y = (int) *p->iy + j*10;
        Fl_Slider *o;
        int slider_type = (int) configtable[j*4+3];
        int iexp = (int) configtable[j*4+2];

        if (slider_type > 19) {
          plastic = true;
          slider_type = slider_type - 20;
        }
        if (UNLIKELY(slider_type > 10 && iexp == EXP_)) {
          csound->Warning(csound,
                          Str("FLslider exponential, using non-labeled slider"));
          slider_type -= 10;
        }
        if (slider_type <= 10)
          o = new Fl_Slider(x, y, width, 10, Name);
        else {
          o = new Fl_Value_Slider_Input( csound, x, y, width, 10, Name);
          slider_type -=10;
          ((Fl_Value_Slider_Input*) o)->textboxsize(20);
          ((Fl_Value_Slider_Input*) o)->textsize(13);
        }
        switch((int) slider_type) { //type
        case 1: o->type(FL_HOR_FILL_SLIDER); break;
        case 3: o->type(FL_HOR_SLIDER); break;
        case 5: o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        case 7:  o->type(FL_HOR_NICE_SLIDER); o->box(FL_DOWN_BOX); break;
        default: o->type(FL_HOR_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        }
        o->align(FL_ALIGN_LEFT);
        if (plastic) o->box(FL_PLASTIC_DOWN_BOX);
        o->step(0.0);
        widget_attributes(csound, o);
        MYFLT min, max, range;
        min = configtable[j*4];
        max = configtable[j*4+1];

        p->slider_data[j].min=min;
        p->slider_data[j].max=max;
        p->slider_data[j].out=&outable[j];

        switch (iexp) {
        case -1: iexp = EXP_; break;
        case 0: iexp = LIN_; break;
        }

        MYFLT val = 0;
        p->slider_data[j].exp = iexp;

        switch (iexp) {
        case LIN_: //linear
          o->range(min,max);
          o->callback((Fl_Callback*)fl_callbackLinearSliderBank,
                      (void *) &(p->slider_data[j]));
          val = outable[j];
          if (val > max) val = max;
          else if (val < min) val = min;
          break;
        case EXP_ : //exponential
          if (UNLIKELY(min == 0 || max == 0))
            return
              csound->InitError(csound,
                                Str("FLsliderBank: zero is illegal "
                                    "in exponential operations"));
          range = max - min;
          o->range(0,range);
          #if defined(sun)
            p->slider_data[j].base = ::pow((max / (double)min), 1.0/(double)range);
          #else
            p->slider_data[j].base = ::pow((max / min), 1.0/(double)range);
          #endif
          o->callback((Fl_Callback*)fl_callbackExponentialSliderBank,
                      (void *) &(p->slider_data[j]));
          {
            val = outable[j];
            MYFLT range = max-min;
            #if defined(sun)
              MYFLT base = ::pow(max / (double)min, 1.0/(double)range);
            #else
              MYFLT base = ::pow(max / min, 1.0/(double)range);
            #endif
            val = (log(val/min) / log(base)) ;
          }
          break;
        default:
          {
            FUNC *ftp;
            MYFLT fnum = abs(iexp);
            if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL)
              p->slider_data[j].table = ftp->ftable;
            else return NOTOK;
            p->slider_data[j].tablen = ftp->flen;
            o->range(0,0.99999999);
            if (iexp > 0) //interpolated
              o->callback((Fl_Callback*)fl_callbackInterpTableSliderBank,
                          (void *)  &(p->slider_data[j]));
            else // non-interpolated
              o->callback((Fl_Callback*)fl_callbackTableSliderBank,
                          (void *)  &(p->slider_data[j]));
          }
        }
        o->value(val);
        p->slider_data[j].widget_addr= (void*) o;
      }
      w->resizable(w);
      if (*p->iwidth <=0 || *p->iheight <=0) {// default width and height
        int a,b;
        w->size( a= w->parent()->w() -50, b= w->parent()->h());
        w->position(50, 0);
      }
      else {
        w->size( (int)*p->iwidth, (int)*p->iheight);
        w->position((int)*p->ix, (int)*p->iy);
      }
      w->end();
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(LIN_, 0, 0, (void *) w,
                                                (void *) p, ST(currentSnapGroup)));
      //*p->ihandle = ST(AddrSetValue).size()-1;
      ST(last_sldbnk) = ST(AddrSetValue).size()-1;  //gab

      return OK;
  }

  static int fl_slider_bank2(CSOUND *csound, FLSLIDERBANK2 *p){
    return fl_slider_bank2_(csound, p,0);
  }

 static int fl_slider_bank2_S(CSOUND *csound, FLSLIDERBANK2 *p){
    return fl_slider_bank2_(csound, p,1);
  }

  static int fl_vertical_slider_bank2_(CSOUND *csound, FLSLIDERBANK2 *p, int istring)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      char s[MAXNAME];
      bool plastic = false;
      if (istring)
        strncpy(s, ((STRINGDAT*) p->names)->data, MAXNAME-1);
      else if ((long) *p->names <= csound->GetStrsmax(csound) &&
               csound->GetStrsets(csound,(long) *p->names)) {
        strncpy(s, csound->GetStrsets(csound,(long) *p->names), MAXNAME-1);
      }
      string tempname(s);
      stringstream sbuf;
      sbuf << tempname;

      int height = (int) *p->iheight;
      if (height <=0) height = 100;

      Fl_Group* w = new Fl_Group((int)*p->ix, (int)*p->iy,
                                 (int) *p->inumsliders*10, height);
      FUNC *ftp;
      MYFLT *configtable, *outable;

      MYFLT *zkstart;
      int zklast = csound->GetZakBounds(csound, &zkstart);
      if (*p->ioutable  < 1) {
        if (LIKELY(zkstart != NULL &&
                   zklast>(long)(*p->inumsliders + *p->ioutablestart_ndx)))
          outable = zkstart + (long) *p->ioutablestart_ndx;
        else {
          return csound->InitError(csound,
                                   Str("invalid ZAK space allocation"));
        }
      }
      else {
        if ((ftp = csound->FTnp2Find(csound, p->ioutable)) != NULL)
          outable = ftp->ftable + (long) *p->ioutablestart_ndx;
        else
          return NOTOK;
      }
      if((ftp = csound->FTnp2Find(csound, p->iconfigtable)) != NULL)
        configtable = ftp->ftable;
      else
        return NOTOK;

      p->elements = (long) *p->inumsliders;
      for (int j =0; j< p->elements; j++) {
        string stemp;
        if (tempname == " ") {
          char s[40];
          sprintf(s, "%d", j);
          stemp = s;
        }
        else
          getline(sbuf, stemp, '@');
        char *Name =  new char[stemp.size()+2];
        strcpy(Name,stemp.c_str());
        ST(allocatedStrings).push_back(Name);

        int x = (int) *p->ix+j*10,  y = (int) *p->iy /*+ j*10*/;
        Fl_Slider *o;
        int slider_type= (int) configtable[j*4+3];
        int iexp = (int) configtable[j*4+2];

        if (slider_type > 19) {
          plastic = true;
          slider_type -= 20;
        }
        if (UNLIKELY(slider_type > 10 && iexp == EXP_)) {
          csound->Warning(csound,
                          Str("FLslidBnk2: FLslider exponential, "
                              "using non-labeled slider"));
          slider_type -= 10;
        }
        if (slider_type <= 10)
          o = new Fl_Slider(x, y, 10, height, Name);
        else {
          o = new Fl_Value_Slider_Input( csound, x, y, 10, height, Name);
          slider_type -=10;
          ((Fl_Value_Slider_Input*) o)->textboxsize(50);
          ((Fl_Value_Slider_Input*) o)->textsize(13);
        }
        switch((int) slider_type) { //type
        case 1: case 2: o->type(FL_VERT_FILL_SLIDER); break;
        case 3: case 4: o->type(FL_VERT_SLIDER); break;
        case 5: case 6: o->type(FL_VERT_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        case 7: case 8: o->type(FL_VERT_NICE_SLIDER); o->box(FL_DOWN_BOX); break;
        default: o->type(FL_VERT_NICE_SLIDER); o->box(FL_FLAT_BOX); break;
        }
        o->align(FL_ALIGN_BOTTOM);
        if (plastic) o->box(FL_PLASTIC_DOWN_BOX);
        widget_attributes(csound, o);
        MYFLT min, max, range;
        min = configtable[j*4];
        max = configtable[j*4+1];

        p->slider_data[j].min=min;
        p->slider_data[j].max=max;
        p->slider_data[j].out=&outable[j];

        switch (iexp) {
        case -1: iexp = EXP_; break;
        case 0: iexp = LIN_; break;
        }

        MYFLT val = 0;
        p->slider_data[j].exp = iexp;

        switch (iexp) {
        case LIN_: //linear
          o->range(min,max);
          o->callback((Fl_Callback*)fl_callbackLinearSliderBank,
                      (void *) &(p->slider_data[j]));
          val = outable[j];
          if (val > max) val = max;
          else if (val < min) val = min;
          break;
        case EXP_ : //exponential
          if (UNLIKELY(min == 0 || max == 0))
            return
              csound->InitError(csound,
                                Str("FLsliderBank: zero is illegal "
                                    "in exponential operations"));
          range = max - min;
          o->range(range,0);
          #if defined(sun)
            p->slider_data[j].base = ::pow((max / (double)min), 1.0/(double)range);
          #else
            p->slider_data[j].base = ::pow((max / min), 1.0/(double)range);
          #endif
          o->callback((Fl_Callback*)fl_callbackExponentialSliderBank,
                      (void *) &(p->slider_data[j]));
          {
            val = outable[j];
            MYFLT range = max-min;
            #if defined(sun)
              MYFLT base = ::pow(max / (double)min, 1.0/(double)range);
            #else
              MYFLT base = ::pow(max / min, 1.0/(double)range);
            #endif
            val = (log(val/min) / log(base)) ;
          }
          break;
        default:
          {
            FUNC *ftp;
            MYFLT fnum = abs(iexp);
            if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL)
              p->slider_data[j].table = ftp->ftable;
            else return NOTOK;
            p->slider_data[j].tablen = ftp->flen;
            o->range(.99999999,0);
            if (iexp > 0) //interpolated
              o->callback((Fl_Callback*)fl_callbackInterpTableSliderBank,
                          (void *)  &(p->slider_data[j]));
            else // non-interpolated
              o->callback((Fl_Callback*)fl_callbackTableSliderBank,
                          (void *)  &(p->slider_data[j]));
          }
        }
        o->value(val);
        p->slider_data[j].widget_addr= (void*) o;

      }
      w->resizable(w);
      if (*p->iwidth <=0 || *p->iheight <=0) {// default width and height
        int a,b;
        w->size( a= w->parent()->w(), b= w->parent()->h()-50);
        w->position(0, 0);
      }
      else {
        w->size( (int)*p->iwidth, (int)*p->iheight);
        w->position((int)*p->ix, (int)*p->iy);
      }
      w->end();
      ST(AddrSetValue).push_back(ADDR_SET_VALUE(LIN_, 0, 0, (void *) w,
                                                (void *) p, ST(currentSnapGroup)));
      //*p->ihandle = ST(AddrSetValue).size()-1;
      ST(last_sldbnk) = ST(AddrSetValue).size()-1;  //gab

      return OK;
  }

 static int fl_vertical_slider_bank2(CSOUND *csound, FLSLIDERBANK2 *p){
    return fl_vertical_slider_bank2_(csound,p,0);
  }

 static int fl_vertical_slider_bank2_S(CSOUND *csound, FLSLIDERBANK2 *p){
    return fl_vertical_slider_bank2_(csound,p,1);
  }



  static int fl_slider_bank_getHandle(CSOUND *csound, FLSLDBNK_GETHANDLE *p)
  //valid only if called immediately after FLslidBnk
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      *p->ihandle = ST(last_sldbnk);
      return OK;
  }

  static int fl_slider_bank_setVal(CSOUND *csound, FLSLDBNK_SET *p)
  {
      FUNC* ftp;
      MYFLT *table, *outable;
      int numslid = (int)*p->numSlid;
      int startInd = (int)*p->startInd;
      int startSlid = (int)*p->startSlid;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

      if (LIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL))
        table = ftp->ftable;
      else {
        return csound->InitError(csound,
                                 Str("FLsldBnkSet: invalid table number"));
      }
      // *startInd, *startSlid, *numSlid
      if (UNLIKELY( ftp->flen < startInd + numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSet: table too short!"));
      }
      FLSLIDERBANK *q = (FLSLIDERBANK *)ST(AddrSetValue)[ (int) *p->ihandle].opcode;

      if (LIKELY((ftp = csound->FTnp2Find(csound, q->ioutable)) != NULL))
        outable = ftp->ftable;
      else {
        return csound->InitError(csound,
                                 Str("FLsldBnkSet: invalid outable number"));
      }
      if (numslid == 0) numslid = (int)(q->elements - *p->startSlid);
      if (UNLIKELY( q->elements > startSlid + numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSet: too many sliders to reset!"));
      }
      for (int j = startSlid, k = startInd; j< numslid + startSlid; j++, k++) {

        MYFLT val = 0;
        //      MYFLT base = q->slider_data[j].base;
        int iexp = q->slider_data[j].exp;
        MYFLT min = q->slider_data[j].min;
        MYFLT max = q->slider_data[j].max;
        switch (iexp) {
        case LIN_: //linear
          val = table[k];
          if (val > max) val = max;
          else if (val < min) val = min;
          break;
        case EXP_ : //exponential
          {
            MYFLT range = max - min;
            MYFLT base2 = pow(max / min, 1/range);
            val = (log(table[k]/min) / log(base2)) ;
          }
          break;
        default:
          return csound->InitError(csound,
                                   Str("FLslidBnkSet: "
                                       "function mapping not available"));
        }

        FLlock();
        ((Fl_Slider*) (q->slider_data[j].widget_addr))->value(val);
        FLunlock();
        outable[j] = table[k];
      }
      return OK;
  }

  static int fl_slider_bank2_setVal(CSOUND *csound, FLSLDBNK_SET *p)
  {
      FUNC* ftp;
      MYFLT *table, *outable;
      int numslid = (int)*p->numSlid;
      int startInd = (int)*p->startInd;
      int startSlid = (int)*p->startSlid;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

      if (LIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL))
        table = ftp->ftable;
      else {
        return csound->InitError(csound,
                                 Str("FLsldBnkSet: invalid table number"));
      }
      // *startInd, *startSlid, *numSlid
      if (UNLIKELY( ftp->flen < startInd + numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSet: table too short!"));
      }
      FLSLIDERBANK2 *q =
        (FLSLIDERBANK2 *)ST(AddrSetValue)[ (int) *p->ihandle].opcode;

      if (LIKELY((ftp = csound->FTnp2Find(csound, q->ioutable)) != NULL))
        outable = ftp->ftable;
      else {
        return csound->InitError(csound,
                                 Str("FLsldBnkSet: invalid outable number"));
      }

      if (numslid == 0) numslid = (int)(q->elements - *p->startSlid);
      if (UNLIKELY( q->elements > startSlid + numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSet: too many sliders to reset!"));
      }

      for (int j = startSlid, k = startInd; j< numslid + startSlid; j++, k++) {

        MYFLT val = 0;
        //      MYFLT base = q->slider_data[j].base;
        int iexp = q->slider_data[j].exp;
        MYFLT min = q->slider_data[j].min;
        MYFLT max = q->slider_data[j].max;
        switch (iexp) {
        case LIN_: //linear
          val = table[k];
          if (val > max) val = max;
          else if (val < min) val = min;
          break;
        case EXP_ : //exponential
          {
            MYFLT range = max - min;
            MYFLT base2 = pow(max / min, 1/range);
            val = (log(table[k]/min) / log(base2));
          }
          break;
        default:  // table
          {
            //      val = table[k];
            if (UNLIKELY(val < 0 || val > 1)) { // input range must be 0 to 1
              csound->PerfError(csound, p->h.insdshead,
                                Str("FLslidBnk2Setk: value out of range: "
                                    "function mapping requires a 0 to 1 "
                                    "range for input"));
            }
          }
          //{
          //      initerror("FLslidBnkSet: function mapping not available");
          //      return;
          //}
        }

        FLlock();
        ((Fl_Slider*) (q->slider_data[j].widget_addr))->value(val);
        FLunlock();
        outable[j] = table[k];
      }
      return OK;
  }
  static int fl_slider_bank2_setVal_k_set(CSOUND *csound, FLSLDBNK2_SETK *p)
  {
      FUNC* ftp;

      p->numslid = (int)*p->numSlid;
      p->startind = (int)*p->startInd;
      p->startslid = (int)*p->startSlid;
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

      if (LIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL))
        p->table = ftp->ftable;
      else {
        return csound->InitError(csound,
                                  Str("FLsldBnkSetk: invalid table number"));
      }
      // *startInd, *startSlid, *numSlid
      if (UNLIKELY( ftp->flen < p->startind + p->numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSetk: table too short!"));
      }
      p->q = (FLSLIDERBANK2 *) ST(AddrSetValue)[ (int) *p->ihandle].opcode;

      if (LIKELY((ftp = csound->FTnp2Find(csound, p->q->ioutable)) != NULL))
        p->outable = ftp->ftable;
      else {
        return csound->InitError(csound,
                                 Str("FLsldBnkSetk: invalid outable number"));
      }

      if (p->numslid == 0) p->numslid = p->q->elements - p->startslid;
      if (UNLIKELY( p->q->elements < p->startslid + p->numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSetk: too many sliders to reset!"));
      }
      return OK;
  }



  static int fl_slider_bank2_setVal_k(CSOUND *csound, FLSLDBNK2_SETK *p)
  {
    //WIDGET_GLOBALS *widgetGlobals =
    //    (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");

      if(*p->kflag) {
        FLSLIDERBANK2 *q = p->q;
        MYFLT *table=p->table;
        for (int j = p->startslid, k = p->startind;
             j< p->numslid + p->startslid;
             j++, k++) {

          MYFLT val = 0;
          //  MYFLT base= q->slider_data[j].base;
          int iexp =  q->slider_data[j].exp;
          MYFLT min = q->slider_data[j].min;
          MYFLT max = q->slider_data[j].max;
          switch (iexp) {
          case LIN_: //linear
            val = table[k];
            if (val > max) val = max;
            else if (val < min) val = min;
            break;
          case EXP_ : //exponential
            {
              MYFLT range = max - min;
              MYFLT base2 = pow(max / min, 1/range);
              val = (log(table[k]/min) / log(base2));
            }
            break;
          default: // table
            {
              val = table[k];
              if (UNLIKELY(val < 0 || val > 1)) { // input range must be 0 to 1
                csound->PerfError(csound, p->h.insdshead,
                                  Str("FLslidBnk2Setk: value out of range:"
                                      " function mapping requires a 0 to 1"
                                      " range for input"));
              }
            }
          }
          if (val != p->oldValues[j]) {
            FLlock();
            ((Fl_Slider*) (q->slider_data[j].widget_addr))->value(val);
            ((Fl_Slider*) (q->slider_data[j].widget_addr))->do_callback();
            FLunlock();
            p->oldValues[j] = val;
          }
        }
      }
      return OK;
  }

  static int fl_slider_bank_setVal_k_set(CSOUND *csound, FLSLDBNK_SETK *p)
  {
      FUNC* ftp;
      WIDGET_GLOBALS *widgetGlobals =
      (WIDGET_GLOBALS *)csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      p->numslid = (int)*p->numSlid;
      p->startind = (int)*p->startInd;
      p->startslid = (int)*p->startSlid;

      if (LIKELY((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL))
        p->table = ftp->ftable;
      else {
        return csound->InitError(csound,
                                 Str("FLslidBnkSetk: invalid table number"));
      }
      // *startInd, *startSlid, *numSlid
      if (UNLIKELY( ftp->flen < p->startind + p->numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSetk: table too short!"));
      }
      p->q = (FLSLIDERBANK *) ST(AddrSetValue)[ (int) *p->ihandle].opcode;

      if (LIKELY((ftp = csound->FTnp2Find(csound, p->q->ioutable)) != NULL))
        p->outable = ftp->ftable;
      else {
        return csound->InitError(csound,
                                 Str("FLslidBnkSetk: invalid outable number"));
      }

      if (p->numslid == 0) p->numslid = p->q->elements - p->startslid;
      if (UNLIKELY( p->q->elements < p->startslid + p->numslid)) {
        return csound->InitError(csound,
                                 Str("FLslidBnkSetk:"
                                     " too many sliders to reset!"));
      }
      return OK;
  }



  static int fl_slider_bank_setVal_k(CSOUND *csound, FLSLDBNK_SETK *p)
  {
      WIDGET_GLOBALS *widgetGlobals =
        (WIDGET_GLOBALS *) csound->QueryGlobalVariable(csound, "WIDGET_GLOBALS");
      if (*p->kflag) {
        FLSLIDERBANK *q = p->q;
        MYFLT *table=p->table;
        for (int j = p->startslid, k = p->startind;
             j< p->numslid + p->startslid;
             j++, k++) {

          MYFLT val = 0;
          //         MYFLT base= q->slider_data[j].base;  //Not used
          int iexp =  q->slider_data[j].exp;
          MYFLT min = q->slider_data[j].min;
          MYFLT max = q->slider_data[j].max;
          switch (iexp) {
          case LIN_: //linear
            val = table[k];
            if (val > max) val = max;
            else if (val < min) val = min;
            break;
          case EXP_ : //exponential
            {
              MYFLT range = max - min;
              MYFLT base2 = pow(max / min, 1/range);
              val = (log(table[k]/min) / log(base2));
            }
            break;
          default: // table
            {
              val = table[k];
              if (UNLIKELY(val < 0 || val > 1)) { // input range must be 0 to 1
                csound->PerfError(csound, p->h.insdshead,
                                  Str("FLslidBnk2Setk: value out of range: "
                                      "function mapping requires a 0 to 1 range "
                                      "for input"));
              }
            }
          }
          if (val != p->oldValues[j]) {
            FLlock();
            ((Fl_Slider*) (q->slider_data[j].widget_addr))->value(val);
            ((Fl_Slider*) (q->slider_data[j].widget_addr))->do_callback();
            FLunlock();
            p->oldValues[j] = val;
          }
        }
      }
      return OK;
  }

  static int FLxyin_set(CSOUND *csound, FLXYIN *p)
  {

      *p->koutx = *p->ioutx; //initial values
      *p->kouty = *p->iouty;
      p->rangex = *p->ioutx_max - *p->ioutx_min;
      p->rangey = *p->iouty_max - *p->iouty_min;

      switch((int) *p->iexpx) {
      case 0: // LIN
        p->expx = LIN_; break;
      case -1: // EXP
        p->expx = EXP_;
        if (UNLIKELY(*p->ioutx_min == 0 || *p->ioutx_max==0))
          return csound->InitError(csound,
                                   Str("FLxyin: none of X limits can be zero in"
                                       " exponential mode!"));
        p->basex = pow((double) (*p->ioutx_max / *p->ioutx_min),
                       (double) (1/p->rangex));

        break;
      default: // TABLE
        p->expx = (int) *p->iexpx;
        {
          FUNC *ftp;
          MYFLT fnum = abs(p->expx);
          if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL) {
            p->tablex = ftp->ftable;
            p->tablenx = ftp->flen;
          }
          else return NOTOK;
        }

      }

      switch((int) *p->iexpy) {
      case 0: // LIN
        p->expy = LIN_; break;
      case -1: // EXP
        p->expy = EXP_;
        if (UNLIKELY(*p->iouty_min == 0 || *p->iouty_max==0))
          return csound->InitError(csound,
                                   Str("FLxyin: none of Y limits can "
                                       "be zero in exponential mode!"));
        p->basey = pow((double) (*p->iouty_max / *p->iouty_min),
                       (double) (1/p->rangey));
        break;
      default: // TABLE
        p->expy = (int) *p->iexpy;
        {
          FUNC *ftp;
          MYFLT fnum = abs(p->expy);
          if ((ftp = csound->FTnp2Find(csound, &fnum)) != NULL) {
            p->tabley = ftp->ftable;
            p->tableny = ftp->flen;
          }
          else return NOTOK;
        }

      }
      return OK;
  }

  static int FLxyin(CSOUND *csound, FLXYIN *p)
  {
      int windx_min = (int)*p->iwindx_min, windx_max = (int)*p->iwindx_max;
      int windy_min = (int)*p->iwindy_min, windy_max = (int)*p->iwindy_max;
      MYFLT outx_min = *p->ioutx_min, outy_min = *p->iouty_min;

      MYFLT x=Fl::event_x();
      MYFLT y=Fl::event_y();

      *p->kinside = 1;

      if (x< windx_min) { x = windx_min; *p->kinside = 0; }
      else if (x > windx_max) { x = windx_max; *p->kinside = 0; }

      if (y< windy_min) { y = windy_min; *p->kinside = 0; }
      else if (y > windy_max) { y = windy_max; *p->kinside = 0; }

      MYFLT xx = x - windx_min;
      xx /= windx_max - windx_min;

      MYFLT yy = windy_max - y;
      yy /= windy_max - windy_min;

      switch (p->expx) {
      case LIN_:
        *p->koutx = outx_min + xx * p->rangex;
        break;
      case EXP_:
        *p->koutx  = outx_min * pow (p->basex, xx * p->rangex);
        break;
      default: // TABLE
        if (p->expx > 0) { //interpolated
          MYFLT ndx = xx * (p->tablenx-1);
          int index = (int) ndx;
          MYFLT v1 = p->tablex[index];
          *p->koutx = outx_min + ( v1 + (p->tablex[index+1] - v1) *
                                   (ndx - index)) * p->rangex;
        }
        else // non-interpolated
          *p->koutx = outx_min + p->tablex[(int) (xx*p->tablenx)] * p->rangex;
      }

      switch (p->expy) {
      case LIN_:
        *p->kouty = outy_min + yy *  p->rangey;
        break;
      case EXP_:
        *p->kouty  = outy_min * pow (p->basey, yy * p->rangey);
        break;
      default: // TABLE
        if (p->expy > 0) { //interpolated
          MYFLT ndx = yy * (p->tableny-1);
          int index = (int) ndx;
          MYFLT v1 = p->tabley[index];
          *p->kouty = outy_min + ( v1 + ( p->tabley[index+1] - v1) *
                                   (ndx - index)) * p->rangey;
        }
        else // non-interpolated
          *p->kouty = outy_min + p->tabley[(int) (yy*p->tableny)] * p->rangey;
      }
      return OK;
  }

  //   //FIXME: Should this be defined to 0dbfs?
  // #define MAXAMPVU 32767
  //
  //   //TODO: Implement FLTK meter
  //   void VuMeter(CSOUND *csound);
  //
  //   static int VuMeter_set(CSOUND *csound, FLTKMETER *p) {
  //      //isVumeterActive = 1;
  //     csoundSetVuMeterCallback(csound, VuMeter);
  //     //OPARMS    *O = csound->oparms;
  //     int nchnls = csound->nchnls;
  //     Fl_Window *o;
  //     int iw;
  //     iw = (csound->oparms->sfread) ? 30 : 15;
  //     o = new Fl_Window(0,0, 410,nchnls * iw+35, "Meter");
  //     Fl_Box *box = new Fl_Box(10,3,260,10,"Outputs");
  //     box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  //      //else  o = new Fl_Panel(x,y,width,height, panelName);
  //      //o->box((Fl_Boxtype) borderType);
  //     o->resizable(o);
  //     o->callback(flpanel_cb);
  //      //o->set_modal();
  //     widget_attributes(csound,o);
  //
  //     PANELS panel(o, (ST(stack_count)>0) ? 1 : 0);
  //     ST(fl_windows).push_back(panel);
  //     int j;
  //
  //     for ( j = 0; j < nchnls; j++) {
  //       string stemp;
  //       char s[10];
  //       stemp = itoa(j+1,s,10);
  //       char *Name =  new char[stemp.size()+2];
  //       strcpy(Name,stemp.c_str());
  //       ST(allocatedStrings).push_back(Name);
  //
  //       Fl_Slider *w = new Fl_Slider(20, 18+15*j, 380, 10, Name);
  //       w->align(FL_ALIGN_LEFT);
  //       p->widg_address[j] = (unsigned long) w;
  //       w->type(FL_HOR_FILL_SLIDER);
  //       w->color(FL_BACKGROUND_COLOR,FL_GREEN);
  //       w->box(FL_PLASTIC_DOWN_BOX);
  //       w->range(0,MAXAMPVU);
  //     }
  //
  //     if (csound->oparms->sfread) {
  //       box = new Fl_Box(10,18 + 15 * nchnls ,260,10,"Inputs");
  //       box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  //       for (  ; j < nchnls*2; j++) {
  //         string stemp;
  //         char s[10];
  //         stemp = itoa(j+1- nchnls,s,10);
  //         char *Name =  new char[stemp.size()+2];
  //         strcpy(Name,stemp.c_str());
  //         ST(allocatedStrings).push_back(Name);
  //         Fl_Slider *w = new Fl_Slider(20, 30+15*j , 380, 10, Name);
  //         w->align(FL_ALIGN_LEFT);
  //         p->widg_address[j] = (unsigned long) w;
  //         w->type(FL_HOR_FILL_SLIDER);
  //         w->color(FL_BACKGROUND_COLOR,FL_YELLOW);
  //         w->box(FL_PLASTIC_DOWN_BOX);
  //         w->range(0,MAXAMPVU);
  //       }
  //     }
  //     o->end();
  //     ST(p_vumeter) = p;
  //     p->dummycycles = csound->ekr / 16 ;
  //     p->dummycyc = 0;
  //     return OK;
  //   }
  //
  // //extern "C" MYFLT *spout,*spin; //GAB
  //
  //   void VuMeter(CSOUND *csound){
  //     FLTKMETER *p = ST(p_vumeter);
  //     Fl_Slider *w;
  //     int nchnls = csound->nchnls;
  //     int      n = (csound->oparms->sfread) ? nchnls * 2 : nchnls;
  //     MYFLT temp[MAXCHNLS];
  //     int smps = csound->GetKsmps(csound);
  //     MYFLT max[MAXCHNLS];
  //     MYFLT *spo = csound->spout;
  //     MYFLT *spi = csound->spin;
  //
  //     static int overfl[MAXCHNLS];
  //     int i;
  //     for (i=0;i<n;i++) {
  //       max[i]=0;
  //       temp[i] = 0;
  //     }
  //     do       {
  //       for ( i=0; i<nchnls; i++) {
  //         if ((temp[i]=(MYFLT) fabs(*spo++)) > max[i] )
  //           max[i]=    temp[i];
  //       }
  //       if (csound->oparms->sfread) {
  //         for ( ; i<n; i++) {
  //
  //           if ((temp[i]=(MYFLT) fabs(*spi++)) > max[i] )
  //             max[i]=  temp[i];
  //         }
  //       }
  //              //if ((temp= (MYFLT) fabs(*a++)) > max) max = temp;
  //     } while (--smps);
  //
  //     for (i=0; i<n; i++) {
  //       if (max[i] > p->max[i])
  //         p->max[i] = (overfl[i] = max[i]>MAXAMPVU) ? MAXAMPVU : max[i];
  //     }
  //     p->dummycyc++;
  //     if (!(p->dummycyc % p->dummycycles)) {
  //       for (int j=0; j<n; j++) {
  //         w = (Fl_Slider *) p->widg_address[j];
  //
  //         FLlock();
  //         if (overfl[j]) {
  //           w->selection_color(FL_RED);
  //           overfl[j] = 0;
  //         }
  //         else {
  //           if (j < nchnls) w->selection_color(FL_GREEN);
  //           else w->selection_color(FL_YELLOW);
  //         }
  //         w->value( p->max[j]);
  //         w->do_callback(w);
  //         FLunlock();
  //       }
  // #ifdef WIN32
  // //                   PostMessage(callback_target,0,0,0);
  // #endif
  //       for (i=0; i<n; i++)
  //         p->max[i] = 0;
  //     }
  //   }
  //

}       // extern "C"

#define S(x)    sizeof(x)

const OENTRY widgetOpcodes_[] = {
  { (char*)"FLslider",    S(FLSLIDER), 0, 1,  (char*)"ki",   (char*)"Siijjjjjjj",
    (SUBR) fl_slider,     (SUBR) NULL,    (SUBR) NULL },
  { (char*)"FLslidBnk",   S(FLSLIDERBANK), 0, 1, (char*)"", (char*)"Siooooooooo",
    (SUBR) fl_slider_bank_S, (SUBR) NULL,   (SUBR) NULL },
    { (char*)"FLslidBnk.i",   S(FLSLIDERBANK), 0, 1, (char*)"", (char*)"iiooooooooo",
    (SUBR) fl_slider_bank, (SUBR) NULL,   (SUBR) NULL },
  { (char*)"FLknob",      S(FLKNOB), 0, 1,  (char*)"ki",   (char*)"Siijjjjjjo",
    (SUBR) fl_knob,       (SUBR) NULL,     (SUBR) NULL },
  { (char*)"FLroller",    S(FLROLLER), 0, 1,  (char*)"ki",   (char*)"Siijjjjjjjj",
    (SUBR) fl_roller,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLtext",      S(FLTEXT), 0, 1,  (char*)"ki",   (char*)"Siijjjjjj",
    (SUBR) fl_text,                 (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLjoy",  S(FLJOYSTICK), 0, 1,  (char*)"kkii", (char*)"Siiiijjjjjjjj",
    (SUBR) fl_joystick,             (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLcount",     S(FLCOUNTER), 0, 1,  (char*)"ki",   (char*)"Siiiiiiiiiz",
    (SUBR) fl_counter,              (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLbutton",    S(FLBUTTON), 0, 1,  (char*)"ki",   (char*)"Siiiiiiiz",
    (SUBR) fl_button,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLbutBank",   S(FLBUTTONBANK), 0, 1,  (char*)"ki",   (char*)"iiiiiiiz",
    (SUBR) fl_button_bank,          (SUBR) NULL,              (SUBR) NULL },
  //     { (char*)"FLkeyb", S(FLKEYB),    0, 1,  (char*)"k",    (char*)"z",
  //         (SUBR) FLkeyb,         (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLcolor",     S(FLWIDGCOL),    0, 1,  (char*)"",     (char*)"jjjjjj",
    (SUBR) fl_widget_color,         (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLcolor2",    S(FLWIDGCOL2),   0, 1,  (char*)"",     (char*)"jjj",
    (SUBR) fl_widget_color2,        (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLlabel",     S(FLWIDGLABEL),  0, 1,  (char*)"",     (char*)"ojojjj",
    (SUBR) fl_widget_label,         (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetVal_i", S(FL_SET_WIDGET_VALUE_I), 0, 1,  (char*)"", (char*)"ii",
    (SUBR) fl_setWidgetValuei,      (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetVali",  S(FL_SET_WIDGET_VALUE_I), 0, 1,  (char*)"", (char*)"ii",
    (SUBR) fl_setWidgetValuei,      (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetVal",    S(FL_SET_WIDGET_VALUE),0,  3,  (char*)"", (char*)"kki",
    (SUBR) fl_setWidgetValue_set,   (SUBR) fl_setWidgetValue, (SUBR) NULL },
  { (char*)"FLsetColor",  S(FL_SET_COLOR), 0, 1,  (char*)"",     (char*)"iiii",
    (SUBR) fl_setColor1,            (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetColor2", S(FL_SET_COLOR), 0, 1,  (char*)"",     (char*)"iiii",
    (SUBR) fl_setColor2,            (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetTextSize", S(FL_SET_TEXTSIZE), 0, 1,  (char*)"",  (char*)"ii",
    (SUBR) fl_setTextSize,          (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetTextColor", S(FL_SET_COLOR), 0, 1,  (char*)"",  (char*)"iiii",
    (SUBR) fl_setTextColor,       (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetFont",   S(FL_SET_FONT),  0, 1,  (char*)"",     (char*)"ii",
    (SUBR) fl_setFont,              (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetTextType", S(FL_SET_FONT), 0, 1,  (char*)"",     (char*)"ii",
    (SUBR) fl_setTextType,          (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetText",   S(FL_SET_TEXT),  0, 1,  (char*)"",     (char*)"Ti",
    (SUBR) fl_setText,              (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetSize",   S(FL_SET_SIZE),  0, 1,  (char*)"",     (char*)"iii",
    (SUBR) fl_setSize,              (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetPosition", S(FL_SET_POSITION), 0, 1,  (char*)"", (char*)"iii",
    (SUBR) fl_setPosition,          (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLhide",      S(FL_WIDHIDE),   0, 1,  (char*)"",     (char*)"i",
    (SUBR) fl_hide,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLshow",      S(FL_WIDSHOW),   0, 1,  (char*)"",     (char*)"i",
    (SUBR) fl_show,                 (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetBox",    S(FL_SETBOX),    0, 1,  (char*)"",     (char*)"ii",
    (SUBR) fl_setBox,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetAlign",  S(FL_TALIGN),    0, 1,  (char*)"",     (char*)"ii",
    (SUBR) fl_align,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLbox",       S(FL_BOX),       0, 1,  (char*)"i", (char*)"Siiiiiii",
    (SUBR) fl_box_,                  (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLvalue",     S(FLVALUE),      0, 1,  (char*)"i",    (char*)"Sjjjj",
    (SUBR) fl_value,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLpanel",     S(FLPANEL),      0, 1,  (char*)"",  (char*)"Sjjjoooo",
    (SUBR) StartPanel,              (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLpanelEnd",  S(FLPANELEND),   0, 1,  (char*)"",     (char*)"",
    (SUBR) EndPanel,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLpanel_end", S(FLPANELEND),   0, 1,  (char*)"",     (char*)"",
    (SUBR) EndPanel,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLscroll",    S(FLSCROLL),     0, 1,  (char*)"",     (char*)"iiii",
    (SUBR) StartScroll,             (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLscrollEnd", S(FLSCROLLEND),  0, 1,  (char*)"",     (char*)"",
    (SUBR) EndScroll,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLscroll_end",S(FLSCROLLEND),  0, 1,  (char*)"",     (char*)"",
    (SUBR) EndScroll,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLpack",      S(FLPACK),       0, 1,  (char*)"",  (char*)"iiiiooo",
    (SUBR) StartPack,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLpackEnd",   S(FLPACKEND),    0, 1,  (char*)"",     (char*)"",
    (SUBR) EndPack,                 (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLpack_end",  S(FLPACKEND),    0, 1,  (char*)"",     (char*)"",
    (SUBR) EndPack,                 (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLtabs",      S(FLTABS),       0, 1,  (char*)"",     (char*)"iiii",
    (SUBR) StartTabs,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLtabsEnd",   S(FLTABSEND),    0, 1,  (char*)"",     (char*)"",
    (SUBR) EndTabs,                 (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLtabs_end",  S(FLTABSEND),    0, 1,  (char*)"",     (char*)"",
    (SUBR) EndTabs,                 (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLgroup",     S(FLGROUP),      0, 1,  (char*)"",     (char*)"Siiiij",
    (SUBR) StartGroup,              (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLgroupEnd",  S(FLGROUPEND),   0, 1,  (char*)"",     (char*)"",
    (SUBR) EndGroup,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLgroup_end", S(FLGROUPEND),   0, 1,  (char*)"",     (char*)"",
    (SUBR) EndGroup,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetsnap",   S(FLSETSNAP),    0, 1,  (char*)"ii",   (char*)"ioo",
    (SUBR) set_snap,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsetSnapGroup", S(FLSETSNAPGROUP), 0, 1,   (char*)"", (char*)"i",
    (SUBR)fl_setSnapGroup,  (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLgetsnap",   S(FLGETSNAP),    0, 1,  (char*)"i",    (char*)"io",
    (SUBR) get_snap,                (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLsavesnap",  S(FLSAVESNAPS),  0, 1,  (char*)"",     (char*)"So",
    (SUBR) save_snap,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLloadsnap",  S(FLLOADSNAPS),  0, 1,  (char*)"",     (char*)"So",
    (SUBR) load_snap,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLrun",       S(FLRUN),        0, 1,  (char*)"",     (char*)"",
    (SUBR) FL_run,                  (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLupdate",    S(FLRUN),        0, 1,  (char*)"",     (char*)"",
    (SUBR) fl_update,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLprintk",    S(FLPRINTK),     0, 3,  (char*)"",     (char*)"iki",
    (SUBR) FLprintkset,             (SUBR) FLprintk,          (SUBR) NULL },
  { (char*)"FLprintk2",   S(FLPRINTK2),    0, 3,  (char*)"",     (char*)"ki",
    (SUBR) FLprintk2set,            (SUBR) FLprintk2,         (SUBR) NULL },
  { (char*)"FLcloseButton",    S(FLCLOSEBUTTON), 0, 1,  (char*)"i", (char*)"Siiii",
    (SUBR) fl_close_button,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLexecButton",    S(FLEXECBUTTON), 0, 1,  (char*)"i", (char*)"Siiii",
    (SUBR) fl_exec_button,               (SUBR) NULL,              (SUBR) NULL },
  { (char*)"FLkeyIn",    S(FLKEYIN),       0, 3,  (char*)"k",    (char*)"o",
    (SUBR)fl_keyin_set,             (SUBR)fl_keyin,           (SUBR) NULL  },
  { (char*)"FLxyin",      S(FLXYIN), 0, 3,  (char*)"kkk",(char*)"iiiiiiiioooo",
    (SUBR)FLxyin_set,               (SUBR)FLxyin,            (SUBR) NULL  },
  { (char*)"FLmouse",     S(FLMOUSE),             0, 3,  (char*)"kkkkk",(char*)"o",
    (SUBR)fl_mouse_set,             (SUBR)fl_mouse,           (SUBR) NULL  },
  { (char*)"FLvslidBnk",  S(FLSLIDERBANK), 0, 1,  (char*)"",  (char*)"Siooooooooo",
    (SUBR)fl_vertical_slider_bank_S,   (SUBR) NULL,             (SUBR) NULL  },
   { (char*)"FLvslidBnk.i", S(FLSLIDERBANK), 0, 1, (char*)"", (char*)"iiooooooooo",
    (SUBR)fl_vertical_slider_bank,   (SUBR) NULL,             (SUBR) NULL  },
  { (char*)"FLslidBnk2",  S(FLSLIDERBANK2),0, 1,  (char*)"",  (char*)"Siiiooooo",
    (SUBR)fl_slider_bank2_S ,          (SUBR) NULL,             (SUBR) NULL  },
    { (char*)"FLslidBnk2.i", S(FLSLIDERBANK2),0, 1, (char*)"", (char*)"Iiiiooooo",
    (SUBR)fl_slider_bank2 ,          (SUBR) NULL,             (SUBR) NULL  },
  { (char*)"FLvslidBnk2", S(FLSLIDERBANK2),0, 1,  (char*)"",  (char*)"Siiiooooo",
    (SUBR)fl_vertical_slider_bank2_S,  (SUBR) NULL,             (SUBR) NULL  },
    { (char*)"FLvslidBnk2.i", S(FLSLIDERBANK2),0, 1, (char*)"", (char*)"iiiiooooo",
    (SUBR)fl_vertical_slider_bank2,  (SUBR) NULL,             (SUBR) NULL  },
  { (char*)"FLslidBnkGetHandle",S(FLSLDBNK_GETHANDLE),0, 1, (char*)"i", (char*)"",
    (SUBR)fl_slider_bank_getHandle,  (SUBR) NULL,             (SUBR) NULL  },
  { (char*)"FLslidBnkSet",S(FLSLDBNK_SET), 0, 1,  (char*)"",  (char*)"iiooo",
    (SUBR)fl_slider_bank_setVal,     (SUBR) NULL,             (SUBR) NULL  },
  { (char*)"FLslidBnkSetk",  S(FLSLDBNK2_SETK), 0, 3,  (char*)"",  (char*)"kiiooo",
    (SUBR)fl_slider_bank_setVal_k_set,(SUBR)fl_slider_bank_setVal_k,(SUBR) NULL },
  { (char*)"FLslidBnk2Set",  S(FLSLDBNK_SET), 0, 1,  (char*)"",  (char*)"iiooo",
    (SUBR)fl_slider_bank2_setVal,    (SUBR) NULL,             (SUBR) NULL  },
  { (char*)"FLslidBnk2Setk", S(FLSLDBNK2_SETK), 0, 3,  (char*)"",  (char*)"kiiooo",
    (SUBR)fl_slider_bank2_setVal_k_set, (SUBR)fl_slider_bank2_setVal_k,
    (SUBR) NULL },
  { (char*)"FLhvsBox",    S(FL_HVSBOX),    0, 1,  (char*)"i",    (char*)"iiiiiio",
    (SUBR)fl_hvsbox,                (SUBR) NULL,              (SUBR) NULL  },
  { (char*)"FLhvsBoxSetValue",S(FL_SET_HVS_VALUE), 0, 3, (char*)"",  (char*)"kki",
    (SUBR)fl_setHvsValue_set,       (SUBR)fl_setHvsValue,     (SUBR) NULL  },
  //     { (char*)"FLmeter",   S(FLTKMETER),      0, 1,  (char*)"",  (char*)"",
  //         (SUBR)VuMeter_set,  (SUBR) NULL,       (SUBR) NULL  },
  { NULL,             0,                      0, 0,  NULL,   NULL,
    (SUBR) NULL,                    (SUBR) NULL,              (SUBR) NULL }
};
