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

#include "config.h"

#ifdef _WIN32
#       pragma warning(disable: 4117 4804)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(WIN32)
#       include <process.h>
#		include <windows.h>
#endif /* defined(WIN32) */
#if defined(LINUX)
#       include <pthread.h>
#       include <sched.h>
#       include <sys/time.h>
#endif /* defined(LINUX) */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Adjuster.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Value_Output.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Positioner.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Valuator.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Value_Slider.H>

#include <vector>
#include <string>
#include <fstream.h>
#include <strstream.h>

using namespace std ;

extern "C" {
    #undef __cplusplus
    #include "cs.h"
    #include "oload.h"
    #define __cplusplus
//    extern EVTBLK *currevent;    IV - Aug 23 2002
    extern char *unquote(char *name);
#ifdef LINUX
    extern int cleanup();
    extern long srngcnt[], orngcnt[];   // from musmon.c
#endif
}
#undef exit

#define CSOUND_WIDGETS_CPP 1
#include "widgets.h"

#define LIN_ 0
#define EXP_ -1
#undef min
#undef max

extern "C" void deact(INSDS*);

static char hack_o_rama1;       // IV - Aug 23 2002
static char hack_o_rama2;

// ---- IV - Aug 23 2002 ---- included file: Fl_linux_stubs.cpp

/*
 * $Id$
 *
 * this is a file used in the linux distribution to be able to use
 * the DirectCsound 5.1 widget code by Gabriel Maldonado on linux
 * systems.
 *
 * This code has been written by Nicola Bernardini (nicb@centrotemporeale.it)
 * mostly based on ideas by Dave Phillips (dlphip@bright.net)
 *
 */

// #if defined(LINUX)   IV - Sep 8 2002: use this code on all platforms

int
FLkeyboard_sensing(void)
{
    /*
     * since we still don't know what is FLkeyboard_sensing(void)
     * should be returning, but we know that 0 will avoid troubles (?)
     * we return this for now... [nicb@axnet.it]
     */

     return (0);
}

Fl_Window *
FLkeyboard_init(void)
{
    /*
     * here too - we don't know what is going on...
     * we fake it through and hope for the best...
     */
    Fl_Window *result = new Fl_Window(0, 0);

    return result;
}

#if defined(__cplusplus)
extern "C"
{
#endif /* defined(__cplusplus) */

#if !defined(HAVE_ITOA)

#include <stdlib.h>

static char *           /* IV - Sep 8 2002 */
itoa(int value, char *buffer, int radix)
{
    const char *r10 = "%d";
    const char *r16 = "%x";
    const char *default_radix_fmt = "unknown radix value";
    char *radix_fmt = (char *) NULL;
    char *result = buffer != (char *) NULL ? buffer :
      ((char *) malloc(256));

    switch(radix) {
    case 10:
      radix_fmt = (char *) r10;
      break;
    case 16:
      radix_fmt = (char *) r16;
      break;
    default:
      radix_fmt = (char *) default_radix_fmt;
      break;
    };

    sprintf(result, radix_fmt, value);

    return result;
}

#endif /* !defined(HAVE_ITOA) */

static void infoff(MYFLT p1)           /*  turn off an indef copy of instr p1  */
{                               /*      called by musmon                */
    INSDS *ip;
    int   insno;
//      extern INSTRTXT **instrtxtp;       IV - Aug 23 2002
    insno = (int)p1;
    if ((ip = (instrtxtp[insno])->instance) != NULL) {
      do  if (ip->insno == insno          /* if find the insno */
          && ip->actflg                 /*      active       */
          //&& ip->offtim < 0             /*      but indef,   */
          && ip->p1 == p1) {
        //VMSG(printf("turning off inf copy of instr %d\n",insno);)
	printf("turning off inf copy of instr %d\n",insno);
        deact(ip);
        return;                     /*  turn it off */
      }
      while ((ip = ip->nxtinstance) != NULL);
    }
    //printf(Str(X_669,"could not find indefinitely playing instr %d\n"),insno);
}

void ButtonSched(MYFLT  *args[], int numargs)
{ /* based on code by rasmus */
    int i, absinsno, argnum;
    MYFLT insno, starttime;
    EVTNODE *evtlist, *newnode;
    EVTBLK  *newevt;

    /* Get absolute instr num */
    insno = *args[1];
    if (insno < FL(0.0)) {
      infoff(-insno);
      return;
    }
    absinsno = abs((int)insno);
    /* Create the new event */
    newnode = (EVTNODE *) mmalloc((long)sizeof(EVTNODE));
    newevt = &newnode->evt;
    newevt->opcod = (char) *args[0];
    if (newevt->opcod == 0) newevt->opcod = 'i';
    /* Set start time from kwhen */
    starttime = *args[2];
    if (starttime < FL(0.0)) {
      starttime = FL(0.0);
    }
    /* Add current time (see note about kadjust in triginset() above) */
    starttime += (MYFLT) (global_kcounter-1L) * global_onedkr;  // IV - Sep 8 2002
    newnode->kstart = (long)(starttime * global_ekr + FL(0.5)); // IV - Sep 8 2002
    newevt->p2orig = starttime;
    newevt->p3orig = *args[3];
    /* Copy all arguments to the new event */
    newevt->pcnt = (argnum = numargs) -1;
    for (i = 1; i < argnum; i++)
      newevt->p[i] = *args[i];
    newevt->p[2] = starttime;    /* Set actual start time in p2 */

    newnode->insno = absinsno;
    /* Set event activation time in k-cycles */
    newnode->kstart = (long)(starttime * global_ekr + FL(0.5)); // IV - Sep 8 2002

    /* Insert eventnode in list of generated events */
    evtlist = &OrcTrigEvts;
    while (evtlist->nxtevt) {
      if (newnode->kstart < evtlist->nxtevt->kstart) break;
      evtlist = evtlist->nxtevt;
    }
    newnode->nxtevt = evtlist->nxtevt;
    evtlist->nxtevt = newnode;
    O.RTevents = 1;     /* Make sure kperf() looks for RT events */
    O.ksensing = 1;
    O.OrcEvts  = 1;     /* - of the appropriate type */
}

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

// #else        IV - Sep 8 2002

// extern Fl_Window * FLkeyboard_init();
// extern int FLkeyboard_sensing();

// #endif /* defined(LINUX) */  IV - Sep 8 2002

// ---- IV - Aug 23 2002 ---- included file: Fl_Knob.cxx

// generated by Fast Light User Interface Designer (fluid) version 2.00

Fl_Knob::Fl_Knob(int xx,int yy,int ww,int hh,const char *l): Fl_Valuator(xx,yy,ww,hh,l)
{
    a1 = 35;
    a2 = 325;
    _type = DOTLIN;
    _percent = 0.3;
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
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
int Fl_Knob::handle(int  event)
{
    int ox,oy,ww,hh;

    ox = x() + 10; oy = y() + 10;
    ww = w() - 20;
    hh = h()-20;
    switch (event) {
    case FL_PUSH:
      handle_push();
    case FL_DRAG:
      {
        int mx = Fl::event_x()-ox-ww/2;
        int my = Fl::event_y()-oy-hh/2;
        if (!mx && !my) return 1;
        double angle = 270-atan2((float)-my, (float)mx)*180/M_PI;
        double oldangle = (a2-a1)*(value()-minimum())/(maximum()-minimum()) + a1;
        while (angle < oldangle-180) angle += 360;
        while (angle > oldangle+180) angle -= 360;
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

    rds = side / 2;
    cx = ox + side / 2;
    cy = oy + side / 2;
    if (!(_type & DOTLOG_3)) {
      if (_scaleticks == 0) return;
      double a_step = (10.0*3.14159/6.0) / _scaleticks;
      double a_orig = -(3.14159/3.0);
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
        double a_step = (10.0*3.14159/6.0) / nb_dec;
        double a_orig = -(3.14159/3.0) + k * a_step;
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

    rds = (side - 20.0f) / 2.0f;
    cur = _percent * rds / 2.0f;
    cx = ox + side / 2.0f;
    cy = oy + side / 2.0f;
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
    _percent = (float)pc/100.0f;

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

void Fl_Spin::draw(void)
{
    int sxx = x(), syy = y(), sww = w(), shh = h();
    Fl_Boxtype box1 = (Fl_Boxtype)(box());
    int border_size=Fl::box_dx(box());

    if (damage()&~FL_DAMAGE_CHILD) clear_damage(FL_DAMAGE_ALL);

    if (!box1) box1 = (Fl_Boxtype)(box()&-2);

    if ((indrag || mouseobj) && deltadir!=0) {
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

    switch (event) {
    case FL_PUSH:
//    if (!step()) goto DEFAULT;
      iy = my;
      ix = mx;
      drag = Fl::event_button();
      handle_push();
      indrag=1;
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
      indrag=1;
      return 1;
    case FL_RELEASE:
      if (mouseobj) {
        Fl::remove_timeout(repeat_callback, this);
      }
//    if (!step()) goto DEFAULT;
      indrag=0;
      delta=0;
      deltadir=0;
      mouseobj=0;
      handle_release();
      redraw();
      return 1;
    default:
      indrag=0;
      return Fl_Valuator::handle(event);
    }
}

Fl_Spin::~Fl_Spin(void)
{
    Fl::remove_timeout(repeat_callback, this);
}

Fl_Spin::Fl_Spin(int x, int y, int w, int h, const char* l)
  : Fl_Valuator(x,y,w,h,l)
{
    soft_ = 0;
    align(FL_ALIGN_LEFT);
    ix=x;
    iy=y;
    drag=0;
    indrag=0;
    mouseobj = 0;
    deltadir=0;
    delta=0;
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Value_Input_Spin.cpp

void Fl_Value_Input_Spin::input_cb(Fl_Widget*, void* v)
{
    Fl_Value_Input_Spin& t = *(Fl_Value_Input_Spin*)v;
    double nv;
    if (t.step()>=1.0) nv = strtol(t.input.value(), 0, 0);
    else nv = strtod(t.input.value(), 0);
    hack_o_rama1 = 1;
    t.handle_push();
    t.handle_drag(nv);
    t.handle_release();
    hack_o_rama1 = 0;
}

void Fl_Value_Input_Spin::draw(void)
{
    int sxx = x(), syy = y(), sww = w(), shh = h();
    sxx += sww - buttonssize(); sww = buttonssize();
    Fl_Boxtype box1 = (Fl_Boxtype)(box()&-2);
    int border_size=Fl::box_dx(box());
    
    if (damage()&~FL_DAMAGE_CHILD) input.clear_damage(FL_DAMAGE_ALL);
    input.box(box());
    input.color(FL_WHITE, selection_color());
    input.draw();
    input.clear_damage();
    sxx+=border_size;
    syy+=border_size;
    sww-=border_size*2;
    shh-=border_size*2;
    
    if (!box1) box1 = (Fl_Boxtype)(box()&-2);

    if ((indrag || mouseobj) && deltadir!=0) {
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
    if (hack_o_rama1) return;
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

    if (!indrag && ( !sldrag || !((mx>=sxx && mx<=(sxx+sww)) &&
                                  (my>=syy && my<=(syy+shh))))  ) {
      indrag=0;
      switch(event) {
      case FL_PUSH:
      case FL_DRAG:
        sldrag=1;
        break;
      case FL_FOCUS:
        input.take_focus();
        break;
      case FL_UNFOCUS:
        redraw();
        break;
      default:
        sldrag=0;
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
      indrag=1;
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
      indrag=1;
      return 1;
    case FL_RELEASE:
      if (mouseobj) {
        Fl::remove_timeout(repeat_callback, this);
      }
      //    if (!step()) goto DEFAULT;
      indrag=0;
      delta=0;
      deltadir=0;
      mouseobj=0;
      handle_release();
      redraw();
      return 1;
    case FL_FOCUS:
      indrag=0;
      return input.take_focus();
    default:
      indrag=0;
      input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
      return 1;
    }
}

Fl_Value_Input_Spin::~Fl_Value_Input_Spin(void)
{
    Fl::remove_timeout(repeat_callback, this);
}

Fl_Value_Input_Spin::Fl_Value_Input_Spin(int x, int y, int w, int h, const char* l)
  : Fl_Valuator(x,y,w,h,l), input(x, y, w, h, 0)
{
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
    indrag   = 0;
    sldrag   = 0;
    mouseobj = 0;
    deltadir = 0;
    delta    = 0;
}

// ---- IV - Aug 23 2002 ---- included file: Fl_Value_Slider_Input.cpp

void Fl_Value_Slider_Input::input_cb(Fl_Widget*, void* v) {
  Fl_Value_Slider_Input& t = *(Fl_Value_Slider_Input*)v;
  double nv;
  if (t.step()>=1.0) nv = strtol(t.input.value(), 0, 0);
  else nv = strtod(t.input.value(), 0);
  hack_o_rama2 = 1;
  t.handle_push();
  t.handle_drag(nv);
  t.handle_release();
  hack_o_rama2 = 0;
}

void Fl_Value_Slider_Input::draw(void)
{
    int sxx = x(), syy = y(), sww = w(), shh = h();
    int bww = w();
    int X = x(), Y = y(), W = w(), H = h();

    int border_size=Fl::box_dx(box());

    if (horizontal()) {
      bww = textboxsize();  sxx += textboxsize(); sww -= textboxsize();
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
    input.draw();
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
    if (hack_o_rama2) return;
    char buf[128];
    format(buf);
    input.value(buf);
    input.mark(input.position()); // turn off selection highlight
}

int Fl_Value_Slider_Input::handle(int event)
{
    int mx = Fl::event_x();
    int my = Fl::event_y();
    static int ix, drag, indrag=0,sldrag=0;
    int sxx = x(), syy = y(), sww = w(), shh = h();
    int border_size=Fl::box_dx(box());
    if (horizontal()) {
      sxx += textboxsize(); sww -= textboxsize();
    }
    else {
      fl_font(input.textfont(), input.textsize());
      syy += fl_height()+(border_size+1)*2; shh -= fl_height()+(border_size+1)*2;
    }
    if ( !indrag && ( !sldrag || !((mx>=sxx && mx<=(sxx+sww)) &&
                                   (my>=syy && my<=(syy+shh))))  ) {
      indrag=0;
      switch(event) {
      case FL_PUSH:
      case FL_DRAG:
        sldrag=1;
        break;
      case FL_FOCUS:
        input.take_focus();
        break;
      case FL_UNFOCUS:
        redraw();
        break;
      default:
        sldrag=0;
      }
      input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
      return input.handle(event);
    }
    switch (event) {
    case FL_PUSH:
      ix = mx;
      drag = Fl::event_button();
      indrag=1;
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    case FL_DRAG:
      indrag=1;
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    case FL_RELEASE:
      //   if (!step()) goto DEFAULT;
      if (value() != previous_value() || !Fl::event_is_click())
        handle_release();
      else {
        input.handle(FL_PUSH);
        input.handle(FL_RELEASE);
      }
      indrag=0;
      return 1;
    case FL_FOCUS:
      indrag=0;
      input.take_focus();
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    default:
      indrag=0;
      input.type(step()>=1.0 ? FL_INT_INPUT : FL_FLOAT_INPUT);
      input.handle(event);
      return Fl_Slider::handle(event,sxx,syy,sww,shh);
    }
}

Fl_Value_Slider_Input::Fl_Value_Slider_Input(int x, int y,
                                             int w, int h, const char* l)
  : Fl_Value_Slider(x,y,w,h,l),input(x, y, w, h, 0)
{
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

struct PANELS {
    Fl_Window *panel;
    int     is_subwindow;
    PANELS(Fl_Window *new_panel, int flag) : panel(new_panel), is_subwindow(flag){}
    PANELS() {panel = NULL; is_subwindow=0; }
};

struct ADDR {
    void *opcode;
    void *WidgAddress;
};

struct ADDR_STACK /*: ADDR*/{
    OPDS *h;
    void *WidgAddress;
    int     count;
    ADDR_STACK(OPDS *new_h,void *new_address,  int new_count)
      : h(new_h),WidgAddress(new_address), count(new_count) {}
    ADDR_STACK() { h=NULL; WidgAddress = NULL;      count=0; }
};

struct ADDR_SET_VALUE /*: ADDR*/{
    int exponential;
    MYFLT min,max;
    void  *WidgAddress,  *opcode;
    ADDR_SET_VALUE(int new_exponential,MYFLT new_min, MYFLT new_max,
                   void *new_WidgAddress, void *new_opcode) :
      exponential(new_exponential),min(new_min), max(new_max),
      WidgAddress(new_WidgAddress),opcode(new_opcode) {}
    ADDR_SET_VALUE() {
        exponential=LIN_; min=0; max=0;
        WidgAddress=NULL; opcode=NULL;
    }
};

struct VALUATOR_FIELD {
    MYFLT value, value2;
    MYFLT min,max, min2,max2;
    int     exp, exp2;
    string widg_name;
    string opcode_name;
    SLDBK_ELEMENT *sldbnk;
    MYFLT  *sldbnkValues;
    VALUATOR_FIELD() {
        value = 0; value2 =0; widg_name= ""; opcode_name ="";
        min = 0; max =1; min2=0; max2=1; exp=LIN_; exp2=LIN_;
        sldbnk=0; sldbnkValues=0; }
//  ~VALUATOR_FIELD() { if (sldbnk != 0) delete sldbnk;
//  if (sldbnkValues !=0) delete sldbnkValues; }
};

char * GetString(MYFLT pname, char *t);

struct SNAPSHOT {
    int is_empty;
    vector<VALUATOR_FIELD> fields;
    SNAPSHOT(vector<ADDR_SET_VALUE>& valuators);
    SNAPSHOT() { is_empty = 1; }
    void get(vector<ADDR_SET_VALUE>& valuators);
};


static void set_butbank_value (Fl_Group *o, MYFLT value)
{
    int childr = o->children();
    Fl_Button *b;
    MYFLT label;
    int j;
    for (j=0; j< childr; j++) {
      b = (Fl_Button *) o->child(j);
      b->value(0); //deactivate all buttons
    }
    for (j=0; j< childr; j++) {
      b = (Fl_Button *) o->child(j);
      label = atof(b->label());
      if (label == value)
        b->value(1);
    }
}


SNAPSHOT::SNAPSHOT (vector<ADDR_SET_VALUE>& valuators)
{ // the constructor captures current values of all widgets by copying all current
  //  values from "valuators" vector (AddrSetValue) to the "fields" vector
    is_empty =0;
    for (int i=0; i < (int) valuators.size(); i++) {
        string  opcode_name, widg_name;
        ADDR_SET_VALUE& v  = valuators[i];
        if ((int) fields.size() < i+1)
          fields.resize(i+1);
        VALUATOR_FIELD& fld = fields[i];
        float val, min, max;
        opcode_name = fld.opcode_name = ((OPDS *) (v.opcode))->optext->t.opcod;
        if (opcode_name == "FLslider") {
          FLSLIDER *p = (FLSLIDER *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
          val = *p->kout; min = *p->imin; max =*p->imax;
          if (val < min) val=min;
          else if (val>max) val=max;
          fld.value = val;
          fld.min = *p->imin; fld.max = *p->imax; fld.exp = (int) *p->iexp;
        }
else if (opcode_name == "FLslidBnk") {
          FLSLIDERBANK *p = (FLSLIDERBANK *) (v.opcode);
          fld.widg_name = GetString(*p->names, p->STRARG);
          int numsliders = (int) *p->inumsliders;
          fld.sldbnk = p->slider_data;
          fld.sldbnkValues = new MYFLT[numsliders];
          extern vector<char*> allocatedStrings;
          allocatedStrings.push_back((char *) fld.sldbnkValues);
          fld.exp = numsliders; // EXCEPTIONAL CASE! fld.exp contains the number
                                // of sliders and not the exponential flag
          for (int j =0; j < numsliders; j++) {
            switch (fld.sldbnk[j].exp) {
              case LIN_: case EXP_:
                val = *fld.sldbnk[j].out;
                min = fld.sldbnk[j].min; max = fld.sldbnk[j].max;
                if (val < min) val = min;
                if (val > max) val = max;
                break;
              default:
                val = ((Fl_Valuator *) ((Fl_Group*)v.WidgAddress)->
                       child(j))->value();
            }
            fld.sldbnkValues[j]= val;
          }
        }
        else if (opcode_name == "FLknob") {
          FLKNOB *p = (FLKNOB *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
          val = *p->kout; min = *p->imin; max =*p->imax;
          if (val < min) val=min;
          else if (val>max) val=max;
          fld.value = val;
          fld.min = *p->imin; fld.max = *p->imax; fld.exp = (int) *p->iexp;
        }
        else if (opcode_name == "FLroller") {
          FLROLLER *p = (FLROLLER *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
          val = *p->kout; min = *p->imin; max =*p->imax;
          if (val < min) val=min;
          else if (val>max) val=max;
          fld.value = val;
          fld.min = *p->imin; fld.max = *p->imax; fld.exp = (int) *p->iexp;
        }
        else if (opcode_name == "FLtext") {
          FLTEXT *p = (FLTEXT *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
          val = *p->kout; min = *p->imin; max =*p->imax;
          if (val < min) val=min;
          else if (val>max) val=max;
          fld.value = val;
          fld.min = *p->imin; fld.max = *p->imax; fld.exp = LIN_;
        }
        else if (opcode_name == "FLjoy") {
          FLJOYSTICK *p = (FLJOYSTICK *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
          val = *p->koutx; min = *p->iminx; max =*p->imaxx;
          if (val < min) val=min;
          else if (val>max) val=max;
          fld.value = val;
          val = *p->kouty; min = *p->iminy; max =*p->imaxy;
          if (val < min) val=min;
          else if (val>max) val=max;
          fld.value2 =val;
          fld.min =  *p->iminx; fld.max  = *p->imaxx; fld.exp  = (int) *p->iexpx;
          fld.min2 = *p->iminy; fld.max2 = *p->imaxy; fld.exp2 = (int) *p->iexpy;
        }
        else if (opcode_name == "FLbutton") {
          FLBUTTON *p = (FLBUTTON *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
          fld.value = (*p->kout == *p->ion ? 1 : 0);    // IV - Aug 27 2002
          fld.min = 0; fld.max = 1; fld.exp = LIN_;
        }
        else if (opcode_name == "FLbutBank") {
          FLBUTTONBANK *p = (FLBUTTONBANK *) (v.opcode);
          fld.widg_name = "No name for FLbutbank";
          //fld.widg_name = GetString(*p->name, p->STRARG);
          fld.value = *p->kout;
          fld.min = 0; fld.max = 1; fld.exp = LIN_;
        }
        else if (opcode_name == "FLcount") {
          FLCOUNTER *p = (FLCOUNTER *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
          val = *p->kout; min = *p->imin; max =*p->imax;
          if (min != max) {
            if (val < min) val=min;
            else if (val>max) val=max;
          }
          fld.value = val;
          fld.min = *p->imin; fld.max = *p->imax; fld.exp = LIN_;
        }
        else if (opcode_name == "FLvalue") {
          FLVALUE *p = (FLVALUE *) (v.opcode);
          fld.widg_name = GetString(*p->name, p->STRARG);
        }
        else if (opcode_name == "FLbox") {
          FL_BOX *p = (FL_BOX *) (v.opcode);
          fld.widg_name = GetString(*p->itext, p->STRARG);
        }
    }
}

void SNAPSHOT::get(vector<ADDR_SET_VALUE>& valuators)
{
    if (is_empty) {
      initerror("empty snapshot");
      return;
    }
    for (int j =0; j< (int) valuators.size(); j++) {
      Fl_Widget* o = (Fl_Widget*) (valuators[j].WidgAddress);
      void *opcode = valuators[j].opcode;
      VALUATOR_FIELD& fld = fields[j];
      string opcode_name = fld.opcode_name;

      MYFLT val = fld.value, min=fld.min, max=fld.max, range,base;
      if (val < min) val =min;
      else if (val >max) val = max;

      if (opcode_name == "FLjoy") {
        switch(fld.exp) {
          case LIN_:
            ((Fl_Positioner*) o)->xvalue(val);
            break;
          case EXP_:
            range  = fld.max - fld.min;
            base = ::pow(fld.max / fld.min, 1/range);
            ((Fl_Positioner*) o)->xvalue(log(val/fld.min) / log(base)) ;
            break;
          default:
            warning("not implemented yet");
            break;
        }
        val = fld.value2; min = fld.min2; max = fld.max2;
        if (val < min) val =min;
        else if (val >max) val = max;
        switch(fld.exp2) {
          case LIN_:
            ((Fl_Positioner*) o)->yvalue(val);
            break;
          case EXP_:
            range  = fld.max2 - fld.min2;
            base = ::pow(fld.max2 / fld.min2, 1/range);
            ((Fl_Positioner*) o)->yvalue(log(val/fld.min2) / log(base)) ;
            break;
          default:
            warning("not implemented yet");
            break;
        }
        o->do_callback(o, opcode);
      }
      else if (opcode_name == "FLbutton") {
        FLBUTTON *p = (FLBUTTON*) (opcode);
        if (*p->itype < 10) {//  don't allow to retreive its value if >= 10
          ((Fl_Button*) o)->value((int)fld.value);
          o->do_callback(o, opcode);
        }
      }
      else if (opcode_name == "FLbutBank") {
        FLBUTTONBANK *p = (FLBUTTONBANK*) (opcode);
        if (*p->itype < 10) {//  don't allow to retreive its value if >= 10
          //((Fl_Group*) o)->value(fld.value);
          set_butbank_value((Fl_Group*) o, fld.value);
          //o->do_callback(o, opcode);
          *p->kout=fld.value;
        }
      }
      else if (opcode_name == "FLcount") {
        FLCOUNTER *p = (FLCOUNTER*) (opcode);
        if (*p->itype < 10) { //  don't allow to retreive its value if >= 10
          ((Fl_Counter*) o)->value(fld.value);
          o->do_callback(o, opcode);
        }
      }
      else if (opcode_name == "FLslidBnk") {
        FLSLIDERBANK *p = (FLSLIDERBANK*) (opcode);
        int numsliders = (int) *p->inumsliders;
        Fl_Group * grup = (Fl_Group *) o;
        for (int j =0; j < numsliders; j++) {
          MYFLT val = fld.sldbnkValues[j];
          switch (p->slider_data[j].exp) {
            case LIN_:
              ((Fl_Valuator *) grup->child(j))->value(val);
              break;
            case EXP_:
              range  = p->slider_data[j].max - p->slider_data[j].min;
              base = ::pow(p->slider_data[j].max / p->slider_data[j].min, 1/range);
              ((Fl_Valuator*) grup->child(j))->
                value(log(val/p->slider_data[j].min) / log(base)) ;
              break;
            default:
              ((Fl_Valuator *) grup->child(j))->value(val);
              //warning("not implemented yet (bogus)");
              break;
          }
          grup->child(j)->do_callback( grup->child(j),
                                       (void *) &(p->slider_data[j]));
        }
      }
      else {
        switch(fld.exp) {
          case LIN_:
            if (opcode_name == "FLbox" || opcode_name == "FLvalue" ) continue;
            ((Fl_Valuator*) o)->value(val);
            break;
          case EXP_:
            range  = fld.max - fld.min;
            base = ::pow(fld.max / fld.min, 1/range);
            ((Fl_Valuator*) o)->value(log(val/fld.min) / log(base)) ;
            break;
          default:
            warning("not implemented yet (bogus)");
            break;
        }
        o->do_callback(o, opcode);
      }
    }
    Fl::check();
}



//---------------

static int stack_count       = 0;

static int FLcontrol_iheight = 15;
static int FLroller_iheight  = 18;
static int FLcontrol_iwidth  = 400;
static int FLroller_iwidth   = 150;
static int FLvalue_iwidth    = 100;

static int FLcolor           = -1;
static int FLcolor2          = -1;
static int FLtext_size       = 0;
static int FLtext_color      = -1;
static int FLtext_font       = -1;
static int FLtext_align      = 0;

static int FL_ix             = 10;
static int FL_iy             = 10;

static vector<PANELS> fl_windows; // all panels
//static vector<void*> AddrValue;
//        addresses of widgets that display current value of valuators
static vector<ADDR_STACK> AddrStack; //addresses of containers
static vector<ADDR_SET_VALUE> AddrSetValue; //addresses of valuators
static vector<char*> allocatedStrings;
static unsigned long threadHandle;
static vector<SNAPSHOT> snapshots;

static Fl_Window *oKeyb;
static int isActivatedKeyb=0;
//static int keyb_out=0;
static FLKEYB* keybp = NULL;

extern "C" void set_snap(FLSETSNAP *p)
{
    SNAPSHOT snap(AddrSetValue);
    int numfields = snap.fields.size();
    int index = (int) *p->index;
    *p->inum_snap = snapshots.size();
    *p->inum_val = numfields; // number of snapshots

    if (*p->ifn >= 1) { // if the table number is valid
      FUNC    *ftp;   // store the snapshot into the table
      if ((ftp = ftfind(p->ifn)) != NULL) {
        MYFLT * table = ftp->ftable;
        for ( int j=0; j < numfields; j++) {
          table[index*numfields+j] = snap.fields[j].value;
        }
      }
      else initerror("FLsetsnap: invalid table");
    }
    else { // else store it into snapshot bank
      if ((int) snapshots.size() < index+1)
        snapshots.resize(index+1);
      snapshots[index]=snap;
    }
}


extern "C" void get_snap(FLGETSNAP *p)
{
    int index = (int) *p->index;
    if (!snapshots.empty()) {
      if (index > (int) snapshots.size()) index = snapshots.size();
      else if (index < 0 ) index=0;
      snapshots[index].get(AddrSetValue);
    }
    *p->inum_el = snapshots.size();
}


extern "C" void save_snap(FLSAVESNAPS* p)
{
#ifdef WIN32
    int id = MessageBox( NULL,
                         "Saving could overwrite the old file.\n"
                         "Are you sure to save?",
                         "Warning",
                         MB_TASKMODAL|MB_ICONWARNING |MB_OKCANCEL
                         );
    if (id != IDOK ) return;
#elif defined(LINUX)
    // put here some warning message!!
#endif
    char     s[MAXNAME];
    string  filename;
    if (*p->filename == SSTRCOD) { // if char string name given
      if (p->STRARG == NULL) strcpy(s,unquote(currevent->strarg));
      else strcpy(s, unquote(p->STRARG));
    }
    else if ((long)*p->filename <= strsmax &&
             strsets != NULL && strsets[(long)*p->filename])
      strcpy(s, strsets[(long)*p->filename]);
    else sprintf(s,"snap.%d", (int)*p->filename);

    //  if (isfullpath(s) || snapdir_path == NULL)
    filename = s;
    //  else
    //    filename = catpath(snapdir_path, s);
    
    //char* s[MAXNAME] = GetString(*p->filename,p->STRARG);

    fstream file(filename.c_str(), ios::out);
    for (int j =0; j < (int) snapshots.size(); j++) {
      file << "----------- "<< j << " -----------\n";
      for ( int k =0; k < (int) snapshots[j].fields.size(); k++) {
        VALUATOR_FIELD& f = snapshots[j].fields[k];
        if (f.opcode_name == "FLjoy") {
          file <<f.opcode_name<<" "<< f.value <<" "<< f.value2
               <<" "<< f.min <<" "<< f.max <<" "<< f.min2 <<" "
               << f.max2<<" "<<f.exp<<" "<<f.exp2<<" \"" <<f.widg_name<<"\"\n";
        }
        else if (f.opcode_name == "FLslidBnk") {
// EXCEPTIONAL CASE! fld.exp contains the number of sliders and
// not the exponential flag
          file << f.opcode_name << " " << f.exp << " "; 
          for (int i=0; i < f.exp; i++) {
            file << f.sldbnkValues[i] << " ";
          }
          file << " \"" << f.widg_name << "\"\n";
        }
        else {
          file <<f.opcode_name<<" "<< f.value
               <<" "<< f.min <<" "<< f.max <<" "<<f.exp<<" \""
               <<f.widg_name<<"\"\n";
        }
      }
    }
    file << "---------------------------";
    file.close();
}

extern "C" void load_snap(FLLOADSNAPS* p)
{
    char     s[MAXNAME];
    string  filename;
    if (*p->filename == SSTRCOD) { // if char string name given
      if (p->STRARG == NULL) strcpy(s,unquote(currevent->strarg));
      else strcpy(s, unquote(p->STRARG));
    }
    else if ((long)*p->filename <= strsmax &&
             strsets != NULL && strsets[(long)*p->filename])
      strcpy(s, strsets[(long)*p->filename]);
    else sprintf(s,"snap.%d", (int)*p->filename);

//  if (isfullpath(s) || snapdir_path == NULL)
      filename = s;
//  else
//    filename = catpath(snapdir_path, s);

    //char* s[MAXNAME] = GetString(*p->filename,p->STRARG);

    fstream file(filename.c_str(), ios::in);

    int j=0,k=-1;
    while (!(file.eof())) {
      char buf[MAXNAME];
      file.getline(buf,MAXNAME);

      strstream sbuf;
      sbuf << buf;

      string opc, opc_orig;
      getline(sbuf, opc, ' ');
      const char *ss = opc.c_str();
      if (*ss == '-') { // if it is a separation line
        k++; j=0;
        if ((int) snapshots.size() < k+1)
        snapshots.resize(k+1);
      }
      else if (*ss != '\0' && *ss != ' ' && *ss != '\n'){ //ignore blank lines
        ADDR_SET_VALUE& v = AddrSetValue[j];
        if ((int) snapshots[k].fields.size() < j+1)
          snapshots[k].fields.resize(j+1);
        snapshots[k].is_empty = 0;
        VALUATOR_FIELD& fld = snapshots[k].fields[j];
        opc_orig = ((OPDS *) (v.opcode))->optext->t.opcod;

        if (!(opc_orig == opc)) {
          initerror("unmatched widget, probably due to a modified"
                    " orchestra. Modifying an orchestra makes it "
                    "incompatible with old snapshot files");
          return;
        }
        if (opc == "FLjoy") {
          fld.opcode_name = opc;
          string s;
          getline(sbuf,s, ' ');  fld.value = atof(s.c_str());
          getline(sbuf,s, ' ');  fld.value2 = atof(s.c_str());
          getline(sbuf,s, ' ');  fld.min = atof(s.c_str());
          getline(sbuf,s, ' ');  fld.max = atof(s.c_str());
          getline(sbuf,s, ' ');  fld.min2 = atof(s.c_str());
          getline(sbuf,s, ' ');  fld.max2 = atof(s.c_str());
          getline(sbuf,s, ' ');  fld.exp = atoi(s.c_str());
          getline(sbuf,s, ' ');  fld.exp2 = atoi(s.c_str());
          getline(sbuf,s, '\"');
          getline(sbuf,s, '\"');  fld.widg_name = s;
        }
        else if (opc == "FLslidBnk") {
          fld.opcode_name = opc;
          string s;
          getline(sbuf,s, ' ');
          // EXCEPTIONAL CASE! fld.exp contains the number of sliders
          // and not the exponential flag
          fld.exp = atoi(s.c_str()); 
          fld.sldbnkValues = new MYFLT[fld.exp];
          allocatedStrings.push_back((char *) fld.sldbnkValues);

          for (int k =0; k < fld.exp; k++) {
            getline(sbuf,s, ' ');  fld.sldbnkValues[k] = atof(s.c_str());

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
        j++;
      }
    }

    file.close();
}


//-----------

char * GetString(MYFLT pname, char *t)
{
    char    *Name=  new char[MAXNAME];
    allocatedStrings.push_back(Name);
    if (pname == SSTRCOD) {
      if (t == NULL) strcpy(Name,unquote(currevent->strarg));
      else strcpy(Name, unquote(t));
    }
    else if ((long)pname < strsmax && strsets != NULL && strsets[(long)pname])
      strcpy(Name, strsets[(long)pname]);
    return Name;
}


extern "C" void widgetRESET(void)
{
    int j;
    for (j = allocatedStrings.size()-1; j >=0; j--)  {
      delete allocatedStrings[j];
      allocatedStrings.pop_back();
    }
    for (j=fl_windows.size()-1; j >=0 ; j--) { //destroy all opened panels
      if  (fl_windows[j].is_subwindow == 0)   delete fl_windows[j].panel;
      fl_windows.pop_back();
    }
    //for (j = AddrValue.size()-1; j >=0; j--)  {
    //      AddrValue.pop_back();
    //}
    int ss = snapshots.size();
    for (j=0; j< ss; j++) {
      snapshots[j].fields.erase(snapshots[j].fields.begin(),
                                snapshots[j].fields.end());
      snapshots.resize(snapshots.size() + 1);
    }
    if (isActivatedKeyb) {
      delete oKeyb;
    }
    //keyb_out=0;
    isActivatedKeyb=0;
    keybp = NULL;

    AddrSetValue.erase(AddrSetValue.begin(), AddrSetValue.end());


    //    curr_x = 0 , curr_y = 0;
    stack_count       = 0;

    FLcontrol_iheight = 15;
    FLroller_iheight  = 18;
    FLcontrol_iwidth  = 400;
    FLroller_iwidth   = 150;
    FLvalue_iwidth    = 100;

    FLcolor           = -1;
    FLcolor2          = -1;
    FLtext_size       = 0;
    FLtext_color      = -1;
    FLtext_font       = -1;
    FLtext_align      = 0;
//      keyb_out      = 0;
    FL_ix             = 10;
    FL_iy             = 10;
}

//-----------


static void __cdecl fltkRun(void *s)
{
    int j;
#ifdef LINUX
    struct itimerval t1;
    // IV - Aug 27 2002: set up timer to automatically update display at 25 Hz
    t1.it_value.tv_sec = t1.it_interval.tv_sec = 0;
    t1.it_value.tv_usec = 1; t1.it_interval.tv_usec = 40000;
    setitimer (ITIMER_REAL, &t1, NULL);
#endif
    for (j=0; j < (int) fl_windows.size(); j++) {
      fl_windows[j].panel->show();
    }
#ifdef WIN32 // to make this thread to update GUI when no events are present
    SetTimer(0,0,200,NULL);
#endif
    Fl::run();
    warning("end of widget thread\n");
#ifdef LINUX
    // IV - Aug 27 2002: exit if all windows are closed
    for (j = 0; j < nchnls; j++) {              // set overall maxamp
      orngcnt[j] += (rngcnt[j] + srngcnt[j]);   // based on current section
      rngcnt[j] = srngcnt[j] = 0;               // and note
      if (maxamp[j] > omaxamp[j]) omaxamp[j] = maxamp[j];
      if (smaxamp[j] > omaxamp[j]) omaxamp[j] = smaxamp[j];
    }
    cleanup();
    exit(0);
#endif
}

static void __cdecl fltkKeybRun(void *s)
{
    oKeyb->show();
    //Fl::run();

    while(Fl::wait()) {
      int temp = FLkeyboard_sensing();
      if (temp != 0 && *keybp->args[1] >=1 ) {
        *keybp->kout = temp;
        ButtonSched(keybp->args, keybp->INOCOUNT);
      }

    }
    warning("end of keyboard thread\n");
}

extern "C" void FL_run(FLRUN *p)
{
#ifdef _WINDOWS
    threadHandle = _beginthread(fltkRun, 0, NULL);
    if (isActivatedKeyb)
      threadHandle = _beginthread(fltkKeybRun, 0, NULL);
#elif defined( LINUX ) || defined(someotherUNIXes)
    pthread_attr_t a;
    pthread_t thread1;
    // IV - Aug 27 2002: widget thread is always run with normal priority
    pthread_attr_init(&a);
    pthread_attr_setschedpolicy(&a, SCHED_OTHER);
    pthread_attr_setinheritsched(&a, PTHREAD_EXPLICIT_SCHED);
    threadHandle = pthread_create(&thread1, &a, (void *(*)(void *)) fltkRun,
                                  NULL);
#endif
}

extern "C" void fl_update(FLRUN *p)
{
    for (int j=0; j< (int) AddrSetValue.size()-1; j++) {
      ADDR_SET_VALUE v = AddrSetValue[j];
      Fl_Valuator *o = (Fl_Valuator *) v.WidgAddress;
      o->do_callback(o, v.opcode);
    }
}

//----------------------------------------------

inline void displ(MYFLT val, MYFLT index)
{
    if (index >= 0) {//display current value of valuator
      char valString[MAXNAME];
      ((Fl_Output*) (AddrSetValue[(long) index]).WidgAddress)->
        value(gcvt( val, 5, valString ));
    }
}


static void fl_callbackButton(Fl_Button* w, void *a)
{
    FLBUTTON *p = (FLBUTTON *) a;
    *((FLBUTTON*) a)->kout =  (w->value()) ? *p->ion : *p->ioff;
    if (*p->args[0] >= 0) ButtonSched(p->args, p->INOCOUNT-8);
}

static void fl_callbackButtonBank(Fl_Button* w, void *a)
{
    FLBUTTONBANK *p = (FLBUTTONBANK *) a;
    *((FLBUTTONBANK*) a)->kout = (MYFLT) atoi(w->label());
    if (*p->args[0] >= 0) ButtonSched(p->args, p->INOCOUNT-7);
}


static void fl_callbackCounter(Fl_Counter* w, void *a)
{
    FLCOUNTER *p = (FLCOUNTER *) a;
    *((FLCOUNTER*) a)->kout =  w->value();
    if (*p->args[0] >= 0) ButtonSched(p->args, p->INOCOUNT-10);
}

static void fl_callbackLinearSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    displ(*p->kout = w->value(), *p->idisp);
}

static void fl_callbackExponentialSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    displ(*p->kout = p->min * ::pow (p->base, w->value()), *p->idisp);
}



static void fl_callbackInterpTableSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (long) ndx;
    MYFLT v1 = p->table[index];
    displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) *
                               (ndx - index)) * (*p->imax - p->min),
          *p->idisp);
}


static void fl_callbackTableSlider(Fl_Valuator* w, void *a)
{
    FLSLIDER *p = ((FLSLIDER*) a);
    displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
          (*p->imax - p->min),
          *p->idisp);
}


static void fl_callbackLinearSliderBank(Fl_Valuator* w, void *a)
{
    SLDBK_ELEMENT* p = (SLDBK_ELEMENT*) a;
    *p->out = w->value();
}

static void fl_callbackExponentialSliderBank(Fl_Valuator* w, void *a)
{
    SLDBK_ELEMENT* p = (SLDBK_ELEMENT*) a;
    *p->out = p->min * ::pow (p->base, w->value());
}


static void fl_callbackInterpTableSliderBank(Fl_Valuator* w, void *a)
{
    SLDBK_ELEMENT *p = ((SLDBK_ELEMENT*) a);

    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (long) ndx;
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
        val = *p->iminx * ::pow (p->basex, j->xvalue());
        break;
      default:
        if (iexpx > 0) { //interpolated
          MYFLT ndx = j->xvalue() * (p->tablenx-1);
          int index = (long) ndx;
          MYFLT v1 = p->tablex[index];
          val = *p->iminx + ( v1 + (p->tablex[index+1] - v1) *
                              (ndx - index)) * (*p->imaxx - *p->iminx);
        }
        else // non-interpolated
          val = *p->iminx+ p->tablex[(long) (j->xvalue() * p->tablenx)] *
            (*p->imaxx - *p->iminx);
    }
    displ(*p->koutx = val,*p->idispx);
    switch (iexpy) {
      case LIN_:
        val = j->yvalue();
        break;
      case EXP_:
        val = *p->iminy * ::pow (p->basey, j->yvalue());
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
    displ(*p->kouty = val, *p->idispy);
}

static void fl_callbackLinearRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    displ(*p->kout =  w->value(),*p->idisp);
}


static void fl_callbackExponentialRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    displ(*p->kout = ((FLROLLER*) a)->min * ::pow (p->base, w->value()), *p->idisp);
}


static void fl_callbackInterpTableRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (long) ndx;
    MYFLT v1 = p->table[index];
    displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
          (*p->imax - p->min), *p->idisp);
}


static void fl_callbackTableRoller(Fl_Valuator* w, void *a)
{
    FLROLLER *p = ((FLROLLER*) a);
    displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
          (*p->imax - p->min), *p->idisp);
}


static void fl_callbackLinearKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    displ( *p->kout = w->value(), *p->idisp);
}


static void fl_callbackExponentialKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    displ( *p->kout = ((FLKNOB*) a)->min * ::pow (p->base, w->value()), *p->idisp);
}


static void fl_callbackInterpTableKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    MYFLT ndx = w->value() * (p->tablen-1);
    int index = (long) ndx;
    MYFLT v1 = p->table[index];
    displ(*p->kout = p->min+ ( v1 + (p->table[index+1] - v1) * (ndx - index)) *
          (*p->imax - p->min), *p->idisp);
}


static void fl_callbackTableKnob(Fl_Valuator* w, void *a)
{
    FLKNOB *p = ((FLKNOB*) a);
    displ(*p->kout = p->min+ p->table[(long) (w->value() * p->tablen)] *
          (*p->imax - p->min), *p->idisp);
}


static void fl_callbackLinearValueInput(Fl_Valuator* w, void *a)
{
    *((FLTEXT*) a)->kout =  w->value();
}

//-----------
void widget_attributes(Fl_Widget *o)
{
    if (FLtext_size == -2 ) {
      FLtext_size = -1;
      FLtext_color= -1;
      FLtext_font = -1;
      FLtext_align= -1;
      FLcolor = -1;
    }
    if (FLtext_size)
      o->labelsize(FLtext_size); // if > 0 assign it, else skip, leaving default
    switch ((int) FLtext_color) {
      case -2: // random color
        o->labelcolor(
          fl_rgb_color(
            (int) (FL(0.5) + (MYFLT) rand() * FL(255.0) / (MYFLT) RAND_MAX),
            (int) (FL(0.5) + (MYFLT) rand() * FL(255.0) / (MYFLT) RAND_MAX),
            (int) (FL(0.5) + (MYFLT) rand() * FL(255.0) / (MYFLT) RAND_MAX)
          )
        );
        break;
      case -1:
        // if FLtext_color is == -1, color assignment is skipped,
        // leaving default color
        break;
      default:
        o->labelcolor(FLtext_color);
        break;
    }
    if (FLtext_font> 0) {
      Fl_Font font;
      switch (FLtext_font) {
        case 1: font  = FL_HELVETICA; break;
        case 2: font  = FL_HELVETICA_BOLD; break;
        case 3: font  = FL_HELVETICA_ITALIC; break;
        case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
        case 5: font  = FL_COURIER; break;
        case 6: font  = FL_COURIER_BOLD; break;
        case 7: font  = FL_COURIER_ITALIC; break;
        case 8: font  = FL_COURIER_BOLD_ITALIC; break;
        case 9: font  = FL_TIMES; break;
        case 10: font = FL_TIMES_BOLD; break;
        case 11: font = FL_TIMES_ITALIC; break;
        case 12: font = FL_TIMES_BOLD_ITALIC; break;
        case 13: font = FL_SYMBOL; break;
        case 14: font = FL_SCREEN; break;
        case 15: font = FL_SCREEN_BOLD; break;
        case 16: font = FL_ZAPF_DINGBATS; break;
        default: font = FL_HELVETICA; break;
      }
      o->labelfont(font);
    }
    if (FLtext_align > 0) {
      Fl_Align type;
      switch (FLtext_align) {
        case -1: break;
        case 1: type  = FL_ALIGN_CENTER; break;
        case 2: type  = FL_ALIGN_TOP; break;
        case 3: type  = FL_ALIGN_BOTTOM; break;
        case 4: type  = FL_ALIGN_LEFT; break;
        case 5: type  = FL_ALIGN_RIGHT; break;
        case 6: type  = FL_ALIGN_TOP_LEFT; break;
        case 7: type  = FL_ALIGN_TOP_RIGHT; break;
        case 8: type  = FL_ALIGN_BOTTOM_LEFT; break;
        case 9: type  = FL_ALIGN_BOTTOM_RIGHT; break;
        default: type = FL_ALIGN_BOTTOM; break;
      }
      o->align(type);
    }
    switch ((int) FLcolor) { // random color
      case -2:
        o->color(FL_GRAY,
          fl_rgb_color(
            (int) (FL(0.5) + (MYFLT) rand() * FL(255.0) / (MYFLT) RAND_MAX),
            (int) (FL(0.5) + (MYFLT) rand() * FL(255.0) / (MYFLT) RAND_MAX),
            (int) (FL(0.5) + (MYFLT) rand() * FL(255.0) / (MYFLT) RAND_MAX)
          )
        );
        break;
      case -1:
        // if FLcolor is == -1, color assignment is skipped,
        // leaving widget default color
        break;
      default:
        o->color(FLcolor, FLcolor2);
        break;
    }
}


//-----------


extern "C" void FLkeyb(FLKEYB *p)
{
    isActivatedKeyb = 1;
    oKeyb = FLkeyboard_init();
    keybp = p; //output of the keyboard is stored into a global variable pointer
}


//-----------


extern "C" void StartPanel(FLPANEL *p)
{
    char* panelName = GetString(*p->name,p->STRARG);

    int x = (int) *p->ix, y = (int) *p->iy,
      width = (int) *p->iwidth, height = (int) *p->iheight;
    if (width <0) width = 400; //default
    if (height <0) height = 300;

    int borderType;
    switch( (int) *p->border ) {
      case 0: borderType = FL_FLAT_BOX; break;
      case 1: borderType = FL_DOWN_BOX; break;
      case 2: borderType = FL_UP_BOX; break;
      case 3: borderType = FL_ENGRAVED_BOX; break;
      case 4: borderType = FL_EMBOSSED_BOX; break;
      case 5: borderType = FL_BORDER_BOX; break;
      case 6: borderType = FL_THIN_DOWN_BOX; break;
      case 7: borderType = FL_THIN_UP_BOX; break;
      default: borderType = FL_FLAT_BOX;
    }

    Fl_Window *o;
    if (x < 0) o = new Fl_Window(width,height, panelName);
    else    o = new Fl_Window(x,y,width,height, panelName);
    widget_attributes(o);
    o->box((Fl_Boxtype) borderType);
    o->resizable(o);
    widget_attributes(o);
    ADDR_STACK adrstk(&p->h, (void *) o, stack_count);
    AddrStack.push_back(adrstk);
    PANELS panel(o, (stack_count>0) ? 1 : 0);
    fl_windows.push_back(panel);
    stack_count++;
}

extern "C" void EndPanel(FLPANELEND *p)
{
    stack_count--;
    ADDR_STACK adrstk = AddrStack.back();
    if (strcmp( adrstk.h->optext->t.opcod, "FLpanel"))
      initerror("FLpanel_end: invalid stack pointer: verify its placement");
    if (adrstk.count != stack_count)
      initerror("FLpanel_end: invalid stack count: "
                "verify FLpanel/FLpanel_end count and placement");
    ((Fl_Window*) adrstk.WidgAddress)->end();
    AddrStack.pop_back();
}

//-----------
extern "C" void StartScroll(FLSCROLL *p)
{
    Fl_Scroll *o = new Fl_Scroll ((int) *p->ix, (int) *p->iy,
                                  (int) *p->iwidth, (int) *p->iheight);
    ADDR_STACK adrstk(&p->h,o,stack_count);
    AddrStack.push_back(adrstk);
    stack_count++;
}

extern "C" void EndScroll(FLSCROLLEND *p)
{
    stack_count--;
    ADDR_STACK adrstk = AddrStack.back();
    if (strcmp( adrstk.h->optext->t.opcod, "FLscroll"))
      initerror("FLscroll_end: invalid stack pointer: verify its placement");
    if (adrstk.count != stack_count)
      initerror("FLscroll_end: invalid stack count: "
                "verify FLscroll/FLscroll_end count and placement");
    ((Fl_Scroll*) adrstk.WidgAddress)->end();

    AddrStack.pop_back();
}

//-----------
extern "C" void StartTabs(FLTABS *p)
{
    Fl_Tabs *o = new Fl_Tabs ((int) *p->ix, (int) *p->iy,
                              (int) *p->iwidth, (int) *p->iheight);
    widget_attributes(o);
    ADDR_STACK adrstk(&p->h,o,stack_count);
    AddrStack.push_back(adrstk);
    stack_count++;
}

extern "C" void EndTabs(FLTABSEND *p)
{
    stack_count--;
    ADDR_STACK adrstk = AddrStack.back();
    if (strcmp( adrstk.h->optext->t.opcod, "FLtabs"))
      initerror("FLscroll_end: invalid stack pointer: verify its placement");
    if (adrstk.count != stack_count)
      initerror("FLtabs_end: invalid stack count: "
                "verify FLtabs/FLtabs_end count and placement");
    ((Fl_Scroll*) adrstk.WidgAddress)->end();

    AddrStack.pop_back();
}

//-----------
extern "C" void StartGroup(FLGROUP *p)
{
    char *Name = GetString(*p->name,p->STRARG);
    Fl_Group *o = new Fl_Group ((int) *p->ix, (int) *p->iy,
                                (int) *p->iwidth, (int) *p->iheight,Name);
    widget_attributes(o);
    int borderType;
    switch((int)*p->border ) {
      case 0: borderType = FL_FLAT_BOX; break;
      case 1: borderType = FL_DOWN_BOX; break;
      case 2: borderType = FL_UP_BOX; break;
      case 3: borderType = FL_ENGRAVED_BOX; break;
      case 4: borderType = FL_EMBOSSED_BOX; break;
      case 5: borderType = FL_BORDER_BOX; break;
      case 6: borderType = FL_THIN_DOWN_BOX; break;
      case 7: borderType = FL_THIN_UP_BOX; break;
      default: borderType = FL_FLAT_BOX;
    }
    o->box((Fl_Boxtype) borderType);
    widget_attributes(o);
    ADDR_STACK adrstk(&p->h,o,stack_count);
    AddrStack.push_back(adrstk);
    stack_count++;
}

extern "C" void EndGroup(FLGROUPEND *p)
{
    stack_count--;
    ADDR_STACK adrstk = AddrStack.back();
    if (strcmp( adrstk.h->optext->t.opcod, "FLgroup"))
      initerror("FLgroup_end: invalid stack pointer: verify its placement");
    if (adrstk.count != stack_count)
      initerror("FLgroup_end: invalid stack count: "
                "verify FLgroup/FLgroup_end count and placement");
    ((Fl_Scroll*) adrstk.WidgAddress)->end();

    AddrStack.pop_back();
}

//-----------

extern "C" void StartPack(FLSCROLL *p)
{
    Fl_Pack *o = new Fl_Pack ((int) *p->ix, (int) *p->iy,
                              (int) *p->iwidth, (int) *p->iheight);
    //fl_window->resizable(o);
    o->box(FL_ENGRAVED_FRAME);
    o->spacing(15);

    ADDR_STACK adrstk(&p->h,o,stack_count);;
    AddrStack.push_back(adrstk);
    stack_count++;
}

extern "C" void EndPack(FLSCROLLEND *p)
{
    stack_count--;
    ADDR_STACK adrstk = AddrStack.back();
    if (strcmp( adrstk.h->optext->t.opcod, "FLpack"))
      initerror("FLpack_end: invalid stack pointer: verify its placement");
    if (adrstk.count != stack_count)
      initerror("FLpack_end: invalid stack count: "
                "verify FLpack/FLpack_end count and placement");
    ((Fl_Pack*) adrstk.WidgAddress)->end();

    AddrStack.pop_back();

}

//-----------

extern "C" void fl_widget_color(FLWIDGCOL *p)
{
    if (*p->red1 < 0) { // reset colors to default
      FLcolor = (int) *p->red1; //when called without arguments
      FLcolor2 =(int) *p->red1;
    }
    else {
      FLcolor = fl_rgb_color((int) *p->red1,
                             (int) *p->green1,
                             (int) *p->blue1);
      FLcolor2 = fl_rgb_color((int) *p->red2,
                              (int) *p->green2,
                              (int) *p->blue2);
    }
}

extern "C" void fl_widget_color2(FLWIDGCOL2 *p)
{
    if (*p->red < 0) { // reset colors to default
      FLcolor2 =(int) *p->red;
    }
    else {
      FLcolor2 = fl_rgb_color((int) *p->red,
                              (int) *p->green,
                              (int) *p->blue);
    }
}

extern "C" void fl_widget_label(FLWIDGLABEL *p)
{
    if (*p->size <= 0) { // reset settings to default
      FLtext_size = 0; //when called without arguments
      FLtext_font = -1;
      FLtext_align = 0;
      FLtext_color = -1;
    }
    else {
      FLtext_size = (int) *p->size;

      if (*p->font > -1) FLtext_font = (int) *p->font;
      if (*p->align > 0)  FLtext_align =  (int) *p->align;
      if (*p->red > -1 && *p->green > -1 && *p->blue > -1) {
        FLtext_color = fl_rgb_color((int) *p->red,
                                    (int) *p->green,
                                    (int) *p->blue);
      }
    }
}
//-----------

extern "C" void fl_setWidgetValuei(FL_SET_WIDGET_VALUE_I *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    MYFLT val = *p->ivalue, range, base;
    switch (v.exponential) {
      case LIN_: //linear
        if (val > v.max) val = v.max;
        else if (val < v.min) val = v.min;
        break;
      case EXP_: //exponential
        range = v.max-v.min;
        base = ::pow(v.max / v.min, 1/range);
        val = (log(val/v.min) / log(base)) ;
        break;
      default:
        warning("not implemented yet");
    }
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
#if 0 /* this is broken */
    if (strcmp(((OPDS *) v.opcode)->optext->t.opcod, "FLbutton"))
      ((Fl_Valuator *)o)->value(val);
    else
      ((Fl_Button *)o)->value(val);
#endif
    if (!strcmp(((OPDS *) v.opcode)->optext->t.opcod, "FLbutton")) {
      if (*(p->ivalue) == *(((FLBUTTON *) v.opcode)->ion))
        ((Fl_Button *)o)->value(1);
      else if (*(p->ivalue) == *(((FLBUTTON *) v.opcode)->ioff))
        ((Fl_Button *)o)->value(0);
    }
else if (!(strcmp(((OPDS *) v.opcode)->optext->t.opcod, "FLbutBank"))) {
      set_butbank_value((Fl_Group *)o, val);
    }
else if (!(strcmp(((OPDS *) v.opcode)->optext->t.opcod, "FLjoy"))) {
      static int flag=1;
      if (flag) { //FLsetVal  always requires two adjacent calls when setting FLjoy
        ((Fl_Positioner *)o)->xvalue(val);
        flag = 0;
      }
      else {
        ((Fl_Positioner *)o)->yvalue(val);
        flag = 1;
      }
    }
    else if (strcmp(((OPDS *) v.opcode)->optext->t.opcod, "FLbox"))
      ((Fl_Valuator *)o)->value(val);
    else
      warning("System error: value() method called from non-valuator object");
    o->do_callback(o, v.opcode);
}

extern "C" void fl_setWidgetValue(FL_SET_WIDGET_VALUE *p)
{

}


//-----------
//-----------

extern "C" void fl_setColor1(FL_SET_COLOR *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    int color = fl_rgb_color((int) *p->red,
                             (int) *p->green,
                             (int) *p->blue);
    o->color(color);
}

extern "C" void fl_setColor2(FL_SET_COLOR *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    int color = fl_rgb_color((int) *p->red,
                             (int) *p->green,
                             (int) *p->blue);
    o->selection_color(color);
}

extern "C" void fl_setTextColor(FL_SET_COLOR *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    int color = fl_rgb_color((int) *p->red,
                             (int) *p->green,
                             (int) *p->blue);
    o->labelcolor(color);
}

extern "C" void fl_setTextSize(FL_SET_TEXTSIZE *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    o->labelsize((uchar) *p->ivalue );
}

extern "C" void fl_setFont(FL_SET_FONT *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    Fl_Font font;
    switch ((int) *p->itype) {
      case 1: font  = FL_HELVETICA; break;
      case 2: font  = FL_HELVETICA_BOLD; break;
      case 3: font  = FL_HELVETICA_ITALIC; break;
      case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
      case 5: font  = FL_COURIER; break;
      case 6: font  = FL_COURIER_BOLD; break;
      case 7: font  = FL_COURIER_ITALIC; break;
      case 8: font  = FL_COURIER_BOLD_ITALIC; break;
      case 9: font  = FL_TIMES; break;
      case 10: font = FL_TIMES_BOLD; break;
      case 11: font = FL_TIMES_ITALIC; break;
      case 12: font = FL_TIMES_BOLD_ITALIC; break;
      case 13: font = FL_SYMBOL; break;
      case 14: font = FL_SCREEN; break;
      case 15: font = FL_SCREEN_BOLD; break;
      case 16: font = FL_ZAPF_DINGBATS; break;
      default: font = FL_SCREEN;
    }
    o->labelfont(font);
}

extern "C" void fl_setTextType(FL_SET_FONT *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
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
}

extern "C" void fl_box(FL_BOX *p)
{
    char *text = GetString(*p->itext,p->STRARG);
    Fl_Box *o =  new Fl_Box((int)*p->ix, (int)*p->iy, 
                            (int)*p->iwidth, (int)*p->iheight, text);
    widget_attributes(o);
    Fl_Boxtype type;
    switch ((int) *p->itype) {
      case 1: type  = FL_FLAT_BOX; break;
      case 2: type  = FL_UP_BOX; break;
      case 3: type  = FL_DOWN_BOX; break;
      case 4: type  = FL_THIN_UP_BOX; break;
      case 5: type  = FL_THIN_DOWN_BOX; break;
      case 6: type  = FL_ENGRAVED_BOX; break;
      case 7: type  = FL_EMBOSSED_BOX; break;
      case 8: type  = FL_BORDER_BOX; break;
      case 9: type  = _FL_SHADOW_BOX; break;
      case 10: type = _FL_ROUNDED_BOX; break;
      case 11: type = _FL_RSHADOW_BOX; break;
      case 12: type = _FL_RFLAT_BOX; break;
      case 13: type = _FL_ROUND_UP_BOX; break;
      case 14: type = _FL_ROUND_DOWN_BOX; break;
      case 15: type = _FL_DIAMOND_UP_BOX; break;
      case 16: type = _FL_DIAMOND_DOWN_BOX; break;
      case 17: type = _FL_OVAL_BOX; break;
      case 18: type = _FL_OSHADOW_BOX; break;
      case 19: type = _FL_OFLAT_BOX; break;
      default: type = FL_FLAT_BOX;
    }
    o->box(type);
    Fl_Font font;
    switch ((int) *p->ifont) {
      case 1: font  = FL_HELVETICA; break;
      case 2: font  = FL_HELVETICA_BOLD; break;
      case 3: font  = FL_HELVETICA_ITALIC; break;
      case 4: font  = FL_HELVETICA_BOLD_ITALIC; break;
      case 5: font  = FL_COURIER; break;
      case 6: font  = FL_COURIER_BOLD; break;
      case 7: font  = FL_COURIER_ITALIC; break;
      case 8: font  = FL_COURIER_BOLD_ITALIC; break;
      case 9: font  = FL_TIMES; break;
      case 10: font = FL_TIMES_BOLD; break;
      case 11: font = FL_TIMES_ITALIC; break;
      case 12: font = FL_TIMES_BOLD_ITALIC; break;
      case 13: font = FL_SYMBOL; break;
      case 14: font = FL_SCREEN; break;
      case 15: font = FL_SCREEN_BOLD; break;
      case 16: font = FL_ZAPF_DINGBATS; break;
      default: font = FL_HELVETICA;
    }
    o->labelfont(font);
    o->labelsize((unsigned char)*p->isize);
    o->align(FL_ALIGN_WRAP);
    AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *)o, (void *)p));
    *p->ihandle = AddrSetValue.size()-1;

}


extern "C" void fl_setText(FL_SET_TEXT *p)
{
    char *text = GetString(*p->itext,p->STRARG);
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    o->label(text);
}

extern "C" void fl_setSize(FL_SET_SIZE *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    o->size((short)  *p->iwidth, (short) *p->iheight);
}

extern "C" void fl_setPosition(FL_SET_POSITION *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    o->position((short)  *p->ix, (short) *p->iy);
}

extern "C" void fl_hide(FL_WIDHIDE *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    o->hide();
}

extern "C" void fl_show(FL_WIDSHOW *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    o->show();
}

extern "C" void fl_setBox(FL_SETBOX *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    Fl_Boxtype type;
    switch ((int) *p->itype) {
      case 1: type  = FL_FLAT_BOX; break;
      case 2: type  = FL_UP_BOX; break;
      case 3: type  = FL_DOWN_BOX; break;
      case 4: type  = FL_THIN_UP_BOX; break;
      case 5: type  = FL_THIN_DOWN_BOX; break;
      case 6: type  = FL_ENGRAVED_BOX; break;
      case 7: type  = FL_EMBOSSED_BOX; break;
      case 8: type  = FL_BORDER_BOX; break;
      case 9: type  = FL_SHADOW_BOX; break;
      case 10: type = FL_ROUNDED_BOX; break;
      case 11: type = FL_RSHADOW_BOX; break;
      case 12: type = FL_RFLAT_BOX; break;
      case 13: type = FL_ROUND_UP_BOX; break;
      case 14: type = FL_ROUND_DOWN_BOX; break;
      case 15: type = FL_DIAMOND_UP_BOX; break;
      case 16: type = FL_DIAMOND_DOWN_BOX; break;
      case 17: type = FL_OVAL_BOX; break;
      case 18: type = FL_OSHADOW_BOX; break;
      case 19: type = FL_OFLAT_BOX; break;
      default: type = FL_FLAT_BOX;
    }
    o->box(type);
}

extern "C" void fl_align(FL_TALIGN *p)
{
    ADDR_SET_VALUE v = AddrSetValue[(int) *p->ihandle];
    Fl_Widget *o = (Fl_Widget *) v.WidgAddress;
    Fl_Align type;
    switch ((int) *p->itype) {
      case 1: type  = FL_ALIGN_CENTER; break;
      case 2: type  = FL_ALIGN_TOP; break;
      case 3: type  = FL_ALIGN_BOTTOM; break;
      case 4: type  = FL_ALIGN_LEFT; break;
      case 5: type  = FL_ALIGN_RIGHT; break;
      case 6: type  = FL_ALIGN_TOP_LEFT; break;
      case 7: type  = FL_ALIGN_TOP_RIGHT; break;
      case 8: type  = FL_ALIGN_BOTTOM_LEFT; break;
      case 9: type  = FL_ALIGN_BOTTOM_RIGHT; break;
      default: type = FL_ALIGN_BOTTOM;
    }
    o->align(type);
}

//-----------
//-----------




extern "C" void fl_value(FLVALUE *p)
{
    char *controlName = GetString(*p->name,p->STRARG);
    int ix, iy, iwidth, iheight;
    if (*p->ix<0) ix = FL_ix;       else  FL_ix = ix = (int) *p->ix;
    if (*p->iy<0) iy = FL_iy;       else  FL_iy = iy = (int) *p->iy;
    if (*p->iwidth<0) iwidth = FLvalue_iwidth;
    else FLvalue_iwidth = iwidth = (int) *p->iwidth;
    if (*p->iheight<0) iheight = FLroller_iheight;
    else FLroller_iheight = iheight = (int) *p->iheight;

    Fl_Output *o = new Fl_Output(ix, iy, iwidth, iheight,controlName);
    o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
    if (FLcolor < 0 )
      o->color(FL_GRAY );
    else
      o->color(FLcolor, FLcolor2);
    widget_attributes(o);
    //AddrValue.push_back((void *) o);
    AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p));
    //*p->ihandle = AddrValue.size()-1;
    *p->ihandle = AddrSetValue.size()-1;

}


//-----------

extern "C" void fl_slider(FLSLIDER *p)
{
    char *controlName = GetString(*p->name,p->STRARG);
    int ix,iy,iwidth, iheight,itype, iexp;

    if (*p->iy < 0) {
      iy = FL_iy;
      FL_iy += FLcontrol_iheight + 5;
    }
    else {
      iy = (int) *p->iy;
      FL_iy = iy + FLcontrol_iheight + 5;
    }
    if (*p->ix < 0)  ix = FL_ix; // omitted options: set default
    else  FL_ix = ix = (int) *p->ix;
    if (*p->iwidth < 0) iwidth = FLcontrol_iwidth;
    else FLcontrol_iwidth = iwidth = (int) *p->iwidth;
    if (*p->iheight < 0) iheight = FLcontrol_iheight;
    else FLcontrol_iheight = iheight = (int) *p->iheight;
    if (*p->itype < 1) itype = 1;
    else  itype = (int) *p->itype;

    //if (*p->iexp == LIN_) iexp = LIN_;
    //else  iexp = (int) *p->iexp;
    switch((int) *p->iexp) {
      case -1: iexp = EXP_; break;
      case 0: iexp = LIN_; break;
      default: iexp = (int) *p->iexp;
    }

    if (itype > 10 && iexp == EXP_) {
      warning("FLslider exponential, using non-labeled slider");
      itype -= 10;
    }

    Fl_Slider *o;
    if (itype < 10) o = new Fl_Slider(ix, iy, iwidth, iheight, controlName);
    else {
      o = new Fl_Value_Slider_Input(ix, iy, iwidth, iheight, controlName);
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
      default: initerror("FLslider: invalid slider type");
    }
    widget_attributes(o);
    MYFLT min = p->min = *p->imin, max = *p->imax, range;
    switch (iexp) {
      case LIN_: //linear
        o->range(min,max);
        o->callback((Fl_Callback*)fl_callbackLinearSlider,(void *) p);
        break;
      case EXP_ : //exponential
        if (min == 0 || max == 0)
          initerror("FLslider: zero is illegal in exponential operations");
        range = max - min;
        o->range(0,range);
        p->base = ::pow((max / min), 1/range);
        o->callback((Fl_Callback*)fl_callbackExponentialSlider,(void *) p);
        break;
      default:
        {
          FUNC *ftp;
          MYFLT fnum = abs(iexp);
          if ((ftp = ftfind(&fnum)) != NULL) {
            p->table = ftp->ftable;
            p->tablen = ftp->flen;
          }
          else return;
          o->range(0,.99999999);
          if (iexp > 0) //interpolated
            o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
          else // non-interpolated
            o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
        }
    }
    AddrSetValue.push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                          (void *) o, (void *) p));
    *p->ihandle = AddrSetValue.size()-1;
}


extern "C" void fl_slider_bank(FLSLIDERBANK *p)
{
    char s[MAXNAME];
    char *t = p->STRARG;
    if (*p->names == SSTRCOD) {
      if (t == NULL) strcpy(s,unquote(currevent->strarg));
      else strcpy(s, unquote(t));

    }
    else if ((long)*p->names <= strsmax &&
             strsets != NULL && strsets[(long)*p->names]) {
      strcpy(s, strsets[(long)*p->names]);
    }
    string tempname(s);
    strstream sbuf;
    sbuf << tempname;

    int width = (int) *p->iwidth;
    if (width <=0) width = 100;

    Fl_Group* w = new Fl_Group((int)*p->ix, (int)*p->iy,
                               width, (int)*p->inumsliders*10);
    FUNC *ftp;
    MYFLT *minmaxtable = NULL, *typetable = NULL, *outable, *exptable = NULL;

    if (*p->ioutable  < 1) {
      extern    MYFLT *zkstart;
      extern long       zklast;
      if (zkstart != NULL &&
          zklast > (long) (*p->inumsliders + *p->ioutablestart_ndx))
        outable = zkstart + (long) *p->ioutablestart_ndx;
      else {
        initerror("invalid ZAK space allocation");
        return;
      }
    }
    else {
      if ((ftp = ftfind(p->ioutable)) != NULL)
        outable = ftp->ftable + (long) *p->ioutablestart_ndx;
      else
        return;
    }
    if ((int) *p->iminmaxtable > 0) {
      if ((ftp = ftfind(p->iminmaxtable)) != NULL) minmaxtable = ftp->ftable;
      else return;
    }
    if ((int) *p->iexptable > 0) {
      if ((ftp = ftfind(p->iexptable)) != NULL) exptable = ftp->ftable;
      else return;
    }
    if ((int) *p->itypetable >0) {
      if ((ftp = ftfind(p->itypetable)) != NULL) typetable = ftp->ftable;
      else return;
    }

    for (int j =0; j< *p->inumsliders; j++) {
      string stemp;
      if (tempname == " ") {
        char s[10];
        stemp = itoa(j,s,10);
      }
      else
        getline(sbuf, stemp, '@');
      char *Name =  new char[stemp.size()+2];
      strcpy(Name,stemp.c_str());
      allocatedStrings.push_back(Name);

      int x = (int) *p->ix,  y = (int) *p->iy + j*10;
      Fl_Slider *o;
      int slider_type;
      if ((int) *p->itypetable <=0)  // no slider type table
        if (*p->itypetable >= -7) //  all sliders are of the same type
          slider_type = - (int) *p->itypetable;
        else { // random type
          //(rand()*128. / RAND_MAX) * FL_NUM_RED/256,
          slider_type = (int) (rand()*7. / RAND_MAX);
          switch(slider_type) {
          case 0: slider_type = 1; break;
          case 2: slider_type = 3; break;
          case 4: slider_type = 5; break;
          case 6: slider_type = 7; break;
          default: slider_type = 1;
          }
        }
      else
        slider_type = (int) typetable[j];

      if (slider_type < 10)
        o = new Fl_Slider(x, y, width, 10, Name);
      else {
        o = new Fl_Value_Slider_Input(x, y, width, 10, Name);
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
      o->align(FL_ALIGN_LEFT);
      widget_attributes(o);
      MYFLT min, max, range;
      if ((int) *p->iminmaxtable > 0) {
        min = minmaxtable[j*2];
        max = minmaxtable[j*2+1];
      }
      else {
        min = 0.;
        max = 1.;
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
        if (min == 0 || max == 0)
          initerror("FLsliderBank: zero is illegal in exponential operations");
        range = max - min;
        o->range(0,range);
        p->slider_data[j].base = ::pow((max / min), 1/range);
        o->callback((Fl_Callback*)fl_callbackExponentialSliderBank,
                    (void *) &(p->slider_data[j]));
        {
          val = outable[j];
          MYFLT range = max-min;
          MYFLT base = ::pow(max / min, 1/range);
          val = (log(val/min) / log(base)) ;
        }
        break;
      default:
        {
          FUNC *ftp;
          MYFLT fnum = abs(iexp);
          if ((ftp = ftfind(&fnum)) != NULL)
            p->slider_data[j].table = ftp->ftable;
          else return;
          p->slider_data[j].tablen = ftp->flen;
          o->range(0,.99999999);
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
    AddrSetValue.push_back(ADDR_SET_VALUE(LIN_, 0, 0, (void *) w, (void *) p));
    //*p->ihandle = AddrSetValue.size()-1;
}


extern "C" void fl_joystick(FLJOYSTICK *p)
{
    char *Name = GetString(*p->name,p->STRARG);
    int ix,iy,iwidth, iheight, iexpx, iexpy;

    if (*p->ix < 0)  ix = 10; // omitted options: set default
    else  FL_ix = ix = (int) *p->ix;
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
    widget_attributes(o);
    switch (iexpx) {
    case LIN_: //linear
      o->xbounds(*p->iminx,*p->imaxx); break;
    case EXP_: //exponential
      { if (*p->iminx == 0 || *p->imaxx == 0)
        initerror("FLjoy X axe: zero is illegal in exponential operations");
      MYFLT range = *p->imaxx - *p->iminx;
      o->xbounds(0,range);
      p->basex = ::pow((*p->imaxx / *p->iminx), 1/range);
      } break;
    default:
      {
        FUNC *ftp;
        MYFLT fnum = abs(iexpx);
        if ((ftp = ftfind(&fnum)) != NULL) {
          p->tablex = ftp->ftable;
          p->tablenx = ftp->flen;
        }
        else return;
        o->xbounds(0,.99999999);
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
      o->ybounds(*p->iminy,*p->imaxy); break;
    case EXP_ : //exponential
      { if (*p->iminy == 0 || *p->imaxy == 0)
        initerror("FLjoy X axe: zero is illegal in exponential operations");
      MYFLT range = *p->imaxy - *p->iminy;
      o->ybounds(0,range);
      p->basey = ::pow((*p->imaxy / *p->iminy), 1/range);
      } break;
    default:
      {
        FUNC *ftp;
        MYFLT fnum = abs(iexpy);
        if ((ftp = ftfind(&fnum)) != NULL) {
          p->tabley = ftp->ftable;
          p->tableny = ftp->flen;
        }
        else return;
        o->ybounds(0,.99999999);
        /*      if (iexp > 0) //interpolated
                o->callback((Fl_Callback*)fl_callbackInterpTableSlider,(void *) p);
                else // non-interpolated
                o->callback((Fl_Callback*)fl_callbackTableSlider,(void *) p);
        */
      }
    }
    o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
    o->callback((Fl_Callback*)fl_callbackJoystick,(void *) p);
    AddrSetValue.push_back(ADDR_SET_VALUE(iexpx, *p->iminx, *p->imaxx,
                                          (void *) o, (void *) p));
    *p->ihandle1 = AddrSetValue.size()-1;
    AddrSetValue.push_back(ADDR_SET_VALUE(iexpy, *p->iminy, *p->imaxy,
                                          (void *) o, (void *) p));
    *p->ihandle2 = AddrSetValue.size()-1;

}


extern "C" void fl_knob(FLKNOB *p)
{
    char *controlName = GetString(*p->name,p->STRARG);
    int ix,iy,iwidth, itype, iexp;

    if (*p->iy < 0) iy = FL_iy;
    else  FL_iy = iy = (int) *p->iy;
    if (*p->ix < 0)  ix = FL_ix;
    else  FL_ix = ix = (int) *p->ix;
    if (*p->iwidth < 0) iwidth = FLcontrol_iwidth;
    else FLcontrol_iwidth = iwidth = (int) *p->iwidth;
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
      o = new Fl_Knob(ix, iy, iwidth, iwidth, controlName);
      break;
    case 2:
      o = new Fl_Dial(ix, iy, iwidth, iwidth, controlName);
      o->type(FL_FILL_DIAL);
      o->box(_FL_OSHADOW_BOX);
      ((Fl_Dial*) o)->angles(20,340);
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
      initerror("FLknob: invalid knob type");
    }
    widget_attributes(o);
    o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
    o->range(*p->imin,*p->imax);
    switch (iexp) {
    case LIN_: //linear
      o->range(*p->imin,*p->imax);
      o->callback((Fl_Callback*)fl_callbackLinearKnob,(void *) p);
      o->step(0.001);
      break;
    case EXP_ : //exponential
      { MYFLT min = p->min = *p->imin, max = *p->imax;
      if (min == 0 || max == 0)
        initerror("FLknob: zero is illegal in exponential operations");
      MYFLT range = max - min;
      o->range(0,range);
      p->base = ::pow((max / min), 1/range);
      o->callback((Fl_Callback*)fl_callbackExponentialKnob,(void *) p);
      } break;
    default:
      {
        FUNC *ftp;
        p->min = *p->imin;
        MYFLT fnum = abs(iexp);
        if ((ftp = ftfind(&fnum)) != NULL) {
          p->table = ftp->ftable;
          p->tablen = ftp->flen;
        }
        else return;
        o->range(0,.99999999);
        if (iexp > 0) //interpolated
          o->callback((Fl_Callback*)fl_callbackInterpTableKnob,(void *) p);
        else // non-interpolated
          o->callback((Fl_Callback*)fl_callbackTableKnob,(void *) p);
      }
    }
    AddrSetValue.push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                          (void *) o, (void *) p));
    *p->ihandle = AddrSetValue.size()-1;
}




extern "C" void fl_text(FLTEXT *p)
{
    char *controlName = GetString(*p->name,p->STRARG);
    int ix,iy,iwidth,iheight,itype;
    MYFLT   istep;

    if (*p->iy < 0) iy = FL_iy;
    else  FL_iy = iy = (int) *p->iy;
    if (*p->ix < 0)  ix = FL_ix;
    else  FL_ix = ix = (int) *p->ix;
    if (*p->iwidth < 0) iwidth = FLcontrol_iwidth;
    else FLcontrol_iwidth = iwidth = (int) *p->iwidth;
    if (*p->iheight < 0) iheight = FLcontrol_iheight;
    else FLcontrol_iheight = iheight = (int) *p->iheight;
    if (*p->itype < 1) itype = 1;
    else  itype = (int) *p->itype;
    if (*p->istep < 0) istep = FL(.1);
    else  istep = *p->istep;

    Fl_Valuator* o;
    switch(itype) {
    case 1:
      {
        o = new Fl_Value_Input(ix, iy, iwidth, iheight, controlName);
        ((Fl_Value_Output *) o)->step(istep);
        ((Fl_Value_Output *) o)->range(*p->imin,*p->imax);
      }
      break;
    case 2:
      {
        o = new Fl_Value_Input_Spin(ix, iy, iwidth, iheight, controlName);
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
    }
    o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
    widget_attributes(o);
    o->callback((Fl_Callback*)fl_callbackLinearValueInput,(void *) p);
    AddrSetValue.push_back(ADDR_SET_VALUE(1, *p->imin, *p->imax,
                                          (void *) o, (void *) p));
    *p->ihandle = AddrSetValue.size()-1;
}

extern "C" void fl_button(FLBUTTON *p)
{
    char *Name = GetString(*p->name,p->STRARG);
    int type = (int) *p->itype;
    if (type >9 ) { // ignored when getting snapshots
      char s[512];
      sprintf(s,"FLbutton \"%s\" ignoring snapshot capture retreive", Name);
      warning(s);
      type = type-10;
    }
    Fl_Button *w;
    *p->kout = *p->ioff;        // IV - Aug 27 2002

    switch (type) {
      case 1:
        w = new Fl_Button((int)*p->ix, (int)*p->iy,
                               (int)*p->iwidth, (int)*p->iheight, Name);
        break;
      case 2:
        w = new Fl_Light_Button((int)*p->ix, (int)*p->iy,
                                (int)*p->iwidth, (int)*p->iheight, Name);
        break;
      case 3:
        w = new Fl_Check_Button((int)*p->ix, (int)*p->iy,
                                (int)*p->iwidth, (int)*p->iheight, Name);
        break;
      case 4:
        w = new Fl_Round_Button((int)*p->ix, (int)*p->iy,
                                (int)*p->iwidth, (int)*p->iheight, Name);
        break;
    default:
      initerror("FLbutton: invalid button type");
    }
    Fl_Button *o = w;
    o->align(FL_ALIGN_WRAP);
    widget_attributes(o);
    o->callback((Fl_Callback*)fl_callbackButton,(void *) p);
    AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p));
    *p->ihandle = AddrSetValue.size()-1;
    O.RTevents = 1;     /* Make sure kperf() looks for RT events */
    O.ksensing = 1;
    O.OrcEvts  = 1;     /* - of the appropriate type */

}

extern "C" void fl_button_bank(FLBUTTONBANK *p)
{
    char *Name = "/0"; //GetString(*p->name,p->STRARG);
    int type = (int) *p->itype;
    if (type >9 ) { // ignored when getting snapshots
      char s[512];
      sprintf(s,"FLbutton \"%s\" ignoring snapshot capture retreive", Name);
      warning(s);
      type = type-10;
    }
    Fl_Group* o = new Fl_Group((int)*p->ix, (int)*p->iy,
                               (int)*p->inumx * 10, (int)*p->inumy*10);
    int z = 0;
    for (int j =0; j<*p->inumx; j++) {
      for (int k=0; k< *p->inumy; k++) {
        int x = (int) *p->ix + j*10, y = (int) *p->iy + k*10;
        Fl_Button *w;
        char    *btName=  new char[30];
        allocatedStrings.push_back(btName);
        itoa( z, btName, 10); z++;
        switch (type) {
        case 1: w= new Fl_Button(x, y, 10, 10, btName); break;
        case 2: w= new Fl_Light_Button(x, y, 10, 10, btName); break;
        case 3: w= new Fl_Check_Button(x, y, 10, 10, btName); break;
        case 4: w= new Fl_Round_Button(x, y, 10, 10, btName); break;
        default: initerror("FLbuttonBank: invalid button type");
        }
        widget_attributes(w);
        w->type(FL_RADIO_BUTTON);
        w->callback((Fl_Callback*)fl_callbackButtonBank,(void *) p);
      }
    }
    o->resizable(o);
    //*p->ix, *p->iy,
    o->size( (int)*p->iwidth, (int)*p->iheight);
    o->position((int)*p->ix, (int)*p->iy);
    o->align(FL_ALIGN_BOTTOM | FL_ALIGN_WRAP);
    o->end();

    //AddrSetValue.push_back(ADDR_SET_VALUE(0, 0, 0, (void *) o, (void *) p));
    *p->ihandle = AddrSetValue.size()-1;
    O.RTevents = 1;     /* Make sure kperf() looks for RT events */
    O.ksensing = 1;
    O.OrcEvts  = 1;     /* - of the appropriate type */
}

extern "C" void fl_counter(FLCOUNTER *p)
{
    char *controlName = GetString(*p->name,p->STRARG);
//      int ix,iy,iwidth,iheight,itype;
//      MYFLT   istep1, istep2;

    Fl_Counter* o = new Fl_Counter((int)*p->ix, (int)*p->iy,
                                   (int)*p->iwidth, (int)*p->iheight, controlName);
    widget_attributes(o);
    int type = (int) *p->itype;
    if (type >9 ) { // ignored when getting snapshots
      char s[512];
      sprintf(s,"FLcount \"%s\" ignoring snapshot capture retreive", controlName);
      warning(s);
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
    widget_attributes(o);
    o->callback((Fl_Callback*)fl_callbackCounter,(void *) p);
    AddrSetValue.push_back(ADDR_SET_VALUE(1, 0, 100000, (void *) o, (void *) p));
    *p->ihandle = AddrSetValue.size()-1;
    O.RTevents = 1;     /* Make sure kperf() looks for RT events */
    O.ksensing = 1;
    O.OrcEvts  = 1;     /* - of the appropriate type */
}



extern "C" void fl_roller(FLROLLER *p)
{
    char *controlName = GetString(*p->name,p->STRARG);
    int ix,iy,iwidth, iheight,itype, iexp ;
    double istep;
    if (*p->iy < 0) {
      iy = FL_iy;
      FL_iy += FLroller_iheight + 15;
    }
    else {
      iy = (int) *p->iy;
      FL_iy = iy + FLroller_iheight + 15;
    }
    // omitted options: set defaults
    if (*p->ix<0) ix = FL_ix;       else  FL_ix = ix = (int) *p->ix;
    if (*p->iy<0) iy = FL_iy;       else  FL_iy = iy = (int) *p->iy;
    if (*p->iwidth<0) iwidth = FLroller_iwidth;
    else FLroller_iwidth = iwidth = (int) *p->iwidth;
    if (*p->iheight<0) iheight = FLroller_iheight;
    else FLroller_iheight = iheight = (int) *p->iheight;
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
      initerror("FLroller: invalid roller type");
    }
    widget_attributes(o);
    o->step(istep);
    switch (iexp) {
    case LIN_: //linear
      o->range(*p->imin,*p->imax);
      o->callback((Fl_Callback*)fl_callbackLinearRoller,(void *) p);
      break;
    case EXP_ : //exponential
      {
        MYFLT min = p->min, max = *p->imax;
        if (min == 0 || max == 0)
          initerror("FLslider: zero is illegal in exponential operations");
        MYFLT range = max - min;
        o->range(0,range);
        p->base = ::pow((max / min), 1/range);
        o->callback((Fl_Callback*)fl_callbackExponentialRoller,(void *) p);
      }
      break;
    default:
      {
        FUNC *ftp;
        MYFLT fnum = abs(iexp);
        if ((ftp = ftfind(&fnum)) != NULL) {
          p->table = ftp->ftable;
          p->tablen = ftp->flen;
        }
        else return;
        o->range(0,0.99999999);
        if (iexp > 0) //interpolated
          o->callback((Fl_Callback*)fl_callbackInterpTableRoller,(void *) p);
        else // non-interpolated
          o->callback((Fl_Callback*)fl_callbackTableRoller,(void *) p);
      }
    }
    AddrSetValue.push_back(ADDR_SET_VALUE(iexp, *p->imin, *p->imax,
                                          (void *) o, (void *) p));
    *p->ihandle = AddrSetValue.size()-1;
}


// extern "C" long kcounter;       IV - Aug 23 2002

extern "C" void FLprintkset(FLPRINTK *p)
{
    if (*p->ptime < FL(1.0) / global_ekr)
      p->ctime = FL(1.0) / global_ekr;
    else        p->ctime = *p->ptime;

    p->initime = (MYFLT) kcounter * onedkr;
    p->cysofar = -1;
}

extern "C" void FLprintk(FLPRINTK *p)
{
    MYFLT   timel;
    long    cycles;

    timel = ((MYFLT) kcounter * onedkr) - p->initime;
    cycles = (long)(timel / p->ctime);
    if (p->cysofar < cycles) {
      p->cysofar = cycles;
      char valString[MAXNAME];
      ((Fl_Output*) (AddrSetValue[(long) *p->idisp]).WidgAddress)->
        value(gcvt( *p->val, 5, valString ));
    }
}


extern "C" void FLprintk2set(FLPRINTK2 *p)              // IV - Aug 27 2002
{
    p->oldvalue = FL(-1.12123e35);      // hack to force printing first value
}

extern "C" void FLprintk2(FLPRINTK2 *p)
{
    MYFLT   value = *p->val;
    if (p->oldvalue != value) {
      char valString[MAXNAME];
      ((Fl_Output*) (AddrSetValue[(long) *p->idisp]).WidgAddress)->
        value(gcvt( *p->val, 5, valString ));
      p->oldvalue = value;
    }
}

