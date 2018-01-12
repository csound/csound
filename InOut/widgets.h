/*
    widgets.h:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef CSOUND_WIDGETS_H
#define CSOUND_WIDGETS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    OPDS    h;
  MYFLT   *kout, *ihandle;
  STRINGDAT *name;
  MYFLT *imin, *imax, *iexp;
    MYFLT   *itype, *idisp, *iwidth, *iheight, *ix, *iy;
    MYFLT   min, base, *table;
    long    tablen;
} FLSLIDER;

#define MAXSLIDERBANK 128

typedef struct {
        void *widget_addr; //gab
        MYFLT min, max, *out;
        MYFLT base, *table;
        long tablen;
        int exp;
} SLDBK_ELEMENT;

// ---------------------------------------------------------------------

typedef struct {
    OPDS    h;
  MYFLT   *names;
   MYFLT  *inumsliders, *ioutable, *iwidth, *iheight;
    MYFLT   *ix, *iy, *itypetable, *iexptable, *ioutablestart_ndx;
    MYFLT   *iminmaxtable;
    SLDBK_ELEMENT slider_data[MAXSLIDERBANK];
    long    elements;
} FLSLIDERBANK;

typedef struct {
    OPDS    h;
  MYFLT  *names;
  MYFLT *inumsliders, *ioutable, *iconfigtable, *iwidth,
           *iheight, *ix, *iy, *ioutablestart_ndx;
    SLDBK_ELEMENT slider_data[MAXSLIDERBANK];
    long   elements;
} FLSLIDERBANK2;   //gab

// typedef struct       {
//      OPDS    h;
//     MYFLT   *kout, *args[PMAX+1];
// } FLKEYB;

typedef struct {
    OPDS    h;
    MYFLT   *kvalue, *ihandle;
} FLBUTTONCALL;

typedef struct {
    OPDS    h;
    MYFLT   *koutx, *kouty, *ihandle1, *ihandle2;
  STRINGDAT   *name;
  MYFLT *iminx, *imaxx, *iminy, *imaxy;
    MYFLT   *iexpx, *iexpy, *idispx, *idispy, *iwidth, *iheight, *ix, *iy;
    MYFLT   basex, basey, *tablex, *tabley;
    long    tablenx, tableny;
} FLJOYSTICK;

typedef struct {
    OPDS    h;
  MYFLT   *kout, *ihandle;
  STRINGDAT *name;
MYFLT *imin, *imax, *istep, *iexp;
    MYFLT   *itype, *idisp, *iwidth, *iheight, *ix, *iy;
    MYFLT   min, base, *table;
    long    tablen;
} FLROLLER;

typedef struct {
    OPDS    h;
  MYFLT   *kout, *ihandle;
  STRINGDAT *name;
MYFLT *imin, *imax, *iexp, *itype;
    MYFLT   *idisp, *iwidth, *ix, *iy, *icursorsize;
    MYFLT   min, base, *table;
    long    tablen;
} FLKNOB;

typedef struct {
    OPDS    h;
  MYFLT   *kout, *ihandle;
  STRINGDAT *name;
MYFLT *imin, *imax, *istep, *itype;
    MYFLT   *iwidth, *iheight, *ix, *iy;
} FLTEXT;

// ---------------------------------------------------------------------

typedef struct {
    OPDS    h;
    MYFLT   *red1, *green1, *blue1, *red2, *green2, *blue2;
} FLWIDGCOL;

typedef struct {
    OPDS    h;
    MYFLT   *red, *green, *blue;
} FLWIDGCOL2;

typedef struct {
    OPDS    h;
    MYFLT   *size, *font, *align, *red, *green, *blue;
} FLWIDGLABEL;

// ---------------------------------------------------------------------

typedef struct {
    OPDS    h;
    MYFLT   *ivalue, *ihandle;
} FL_SET_WIDGET_VALUE_I;

typedef struct {
    OPDS    h;
    MYFLT   *ktrig, *kvalue, *ihandle;
    int     handle;
    int     widgetType;
    MYFLT   log_base;
} FL_SET_WIDGET_VALUE;

typedef struct {
    OPDS    h;
    MYFLT   *red, *green, *blue, *ihandle;
} FL_SET_COLOR;

typedef struct {
    OPDS    h;
    MYFLT   *ivalue, *ihandle;
} FL_SET_TEXTSIZE;

typedef struct {
    OPDS    h;
    MYFLT   *itype, *ihandle;
} FL_SET_FONT;

typedef struct {
    OPDS    h;
    STRINGDAT  *itext;
    MYFLT   *ihandle;
} FL_SET_TEXT;

typedef struct {
    OPDS    h;
    MYFLT   *ndx;
    MYFLT   *ihandle;
} FL_SET_TEXTi;

typedef struct {
    OPDS    h;
    MYFLT   *itype, *ihandle;
} FL_TALIGN;

typedef struct {
    OPDS    h;
    MYFLT   *iwidth, *iheight, *ihandle;
} FL_SET_SIZE;

typedef struct {
    OPDS    h;
    MYFLT   *ix, *iy,  *ihandle;
} FL_SET_POSITION;

typedef struct {
    OPDS    h;
    MYFLT   *ihandle;
} FL_WIDHIDE;

typedef struct {
    OPDS    h;
    MYFLT   *ihandle;
} FL_WIDSHOW;

typedef struct {
    OPDS    h;
    MYFLT   *itype, *ihandle;
} FL_SETBOX;

typedef struct {
    OPDS    h;
  MYFLT   *ihandle;
  STRINGDAT *itext;
  MYFLT *itype, *ifont, *isize;
    MYFLT   *iwidth, *iheight, *ix, *iy;
} FL_BOX;

// ---------------------------------------------------------------------

typedef struct {
    OPDS    h;
  MYFLT   *ihandle;
  STRINGDAT *name;
  MYFLT *iwidth, *iheight, *ix, *iy;
} FLVALUE;

typedef struct {
    OPDS    h;
} FLRUN;

typedef struct {
    OPDS    h;
  STRINGDAT   *name;
   MYFLT *iwidth, *iheight, *ix, *iy, *border, *ikbdsense, *iclose;
} FLPANEL;

typedef struct {
    OPDS    h;
    MYFLT   *inum_snap, *inum_val, *index, *ifn, *group;
} FLSETSNAP;

typedef struct {
    OPDS    h;
    MYFLT   *inum_el, *index, *group;
} FLGETSNAP;

typedef struct {
    OPDS    h;
 STRINGDAT     *filename;
 MYFLT *group;
} FLSAVESNAPS;

typedef struct {
    OPDS    h;
  STRINGDAT   *filename;
  MYFLT *group;
} FLLOADSNAPS;

typedef struct {
    OPDS    h;
} FLPANELEND;

typedef struct {
    OPDS    h;
    MYFLT   *iwidth, *iheight, *ix, *iy;
} FLSCROLL;

typedef struct {
    OPDS    h;
} FLSCROLLEND;

typedef struct {
    OPDS    h;
    MYFLT   *iwidth, *iheight, *ix, *iy;
} FLTABS;

typedef struct {
    OPDS    h;
} FLTABSEND;

typedef struct {
    OPDS    h;
  STRINGDAT   *name;
  MYFLT  *iwidth, *iheight, *ix, *iy, *border;
} FLGROUP;

typedef struct {
    OPDS    h;
} FLGROUPEND;

typedef struct {
    OPDS    h;
    MYFLT   *iwidth, *iheight, *ix, *iy, *itype, *ispace, *iborder;
} FLPACK;

typedef struct {
    OPDS    h;
} FLPACKEND;

typedef struct {
    OPDS    h;
  MYFLT   *kout, *ihandle;
  STRINGDAT *name;
  MYFLT *ion, *ioff, *itype;
    MYFLT   *iwidth, *iheight, *ix, *iy, *args[PMAX];
} FLBUTTON;

typedef struct {
    OPDS    h;
    MYFLT   *kout, *ihandle;    /*  outs */
    MYFLT   *itype, *inumx, *inumy, *iwidth, *iheight, *ix, *iy, *args[PMAX];
} FLBUTTONBANK;

typedef struct {
    OPDS    h;
  MYFLT   *kout, *ihandle;
  STRINGDAT *name;
MYFLT *imin, *imax, *istep1, *istep2, *itype;
    MYFLT   *iwidth, *iheight, *ix, *iy, *args[PMAX];
} FLCOUNTER;

typedef struct {
    OPDS    h;
    MYFLT   *ptime, *val, *idisp;
    MYFLT   initime, ctime;
    long    cysofar;
} FLPRINTK;

typedef struct {
    OPDS    h;
    MYFLT   *val, *idisp;
    MYFLT   oldvalue;
 /* int     pspace; */
} FLPRINTK2;

typedef struct {
    OPDS    h;
  MYFLT   *ihandle;
  STRINGDAT *name;
    MYFLT   *iwidth, *iheight, *ix, *iy;
} FLCLOSEBUTTON;

typedef struct {
    OPDS    h;
  MYFLT   *ihandle;
  STRINGDAT *command;
    MYFLT   *iwidth, *iheight, *ix, *iy;
    char    *commandString;
    CSOUND  *csound;
} FLEXECBUTTON;

#ifdef __cplusplus
}
#endif

#if defined(CSOUND_WIDGETS_CPP)

/* ---- IV - Aug 23 2002 ---- included file: Fl_Ball.H */

class Fl_Ball : public Fl_Valuator {
private:
  CSOUND *csound;
  int ix, iy, drag;
  int spinning;

  int ballstacks, ballslices;
  float ballsize;

  float curquat[4];
  float lastquat[4];
  float mat[4][4];

  char soft_;
  uchar mouseobj;

  static PUBLIC void repeat_callback(void *);
  PUBLIC void increment_cb();
  PUBLIC void vertex_by_matrix(float &x, float &y, float &z);

  PUBLIC void transform_ball_vertex(float &x, float &y, float &z);
  PUBLIC void rotate(float &x, float &y, float &z,
                        float rotx=0.0f, float roty=0.0f, float rotz=0.0f);
  PUBLIC void draw_solid_ball(float radius, int slices, int stacks);
  PUBLIC void draw_wire_ball(float radius, int slices, int stacks);
public:
    PUBLIC void draw();
    PUBLIC void handle_drag(double v=0.0);
    PUBLIC void handle_release();
    PUBLIC int handle(int);
    PUBLIC Fl_Ball(CSOUND *cs, int x, int y, int w, int h, const char *l = 0);

  void soft(char x) {soft_ = x;}
  char soft() const {return soft_;}

  float ballscale() const {return ballsize;}
  void ballscale(float s) { ballsize=s;}

  void rotateball(float rotx=0.0f, float roty=0.0f, float rotz=0.0f);
  void getrot(float &rotx, float &roty, float &rotz);

  void stacks(int s) { ballstacks=s; }
  int stacks() const { return ballstacks; }

  void slices(int s) { ballslices=s; }
  int slices() const { return ballslices; }

  ~Fl_Ball();
};

/* ---- IV - Aug 23 2002 ---- included file: Fl_Knob.H */

/* generated by Fast Light User Interface Designer (fluid) version 2.00 */

class Fl_Knob : public Fl_Valuator {
 public:
    enum Fl_Knobtype {
      DOTLIN=0,
      DOTLOG_1,
      DOTLOG_2,
      DOTLOG_3,
      LINELIN,
      LINELOG_1,
      LINELOG_2,
      LINELOG_3
    };
 private:
    CSOUND *csound;
    int _type;
    float _percent;
    int _scaleticks;
    short a1, a2;
 public:
    Fl_Knob(CSOUND *cs, int xx, int yy, int ww, int hh, const char *l=0);
    ~Fl_Knob();
 private:
    void draw();
    int handle(int event);
 public:
    void type(int ty);
 private:
    void shadow(const int offs, const uchar r, uchar g, uchar b);
    void draw_scale(const int ox, const int oy, const int side);
    void draw_cursor(const int ox, const int oy, const int side);
 public:
    void cursor(const int pc);
    void scaleticks(const int tck);
};

/* ---- IV - Aug 23 2002 ---- included file: Fl_Spin.H */

class Fl_Spin : public Fl_Valuator {
 private:
    CSOUND * csound;
    int ix, iy, drag, indrag;
    int delta, deltadir;
    char soft_;
    uchar mouseobj;
    static PUBLIC void repeat_callback(void *);
    PUBLIC void increment_cb();

 public:
    PUBLIC void draw();
    PUBLIC int handle(int);
    PUBLIC Fl_Spin(CSOUND *cs, int x, int y, int w, int h, const char *l = 0);

    void soft(char x) {soft_ = x;}
    char soft() const {return soft_;}

    ~Fl_Spin();
};

/* ---- IV - Aug 23 2002 ---- included file: Fl_Value_Input_Spin.H */

class Fl_Value_Input_Spin : public Fl_Valuator {
 private:
    CSOUND * csound;
    int ix, iy, drag;
    int delta, deltadir;
    char soft_;
    uchar mouseobj;
    int butsize;
    static PUBLIC void input_cb(Fl_Widget*, void*);
    // cause damage() due to value() changing
    virtual PUBLIC void value_damage();
    static PUBLIC void repeat_callback(void *);
    PUBLIC void increment_cb();

 public:
    Fl_Input input;
    PUBLIC void draw();
    PUBLIC int handle(int);
    PUBLIC void resize(int, int, int, int);
    PUBLIC Fl_Value_Input_Spin(CSOUND *csound, int x, int y, int w, int h,
                                  const char *l = 0);

    void soft(char x) {soft_ = x;}
    char soft() const {return soft_;}

    Fl_Font textfont() const {return input.textfont();}
    void textfont(uchar s) { input.textfont(s);}
    uchar textsize() const {return input.textsize();}
    void textsize(uchar s) {input.textsize(s);}
    Fl_Color textcolor() const {return input.textcolor();}
    void textcolor(uchar n) {input.textcolor(n);}
    Fl_Color cursor_color() const {return input.cursor_color();}
    void cursor_color(uchar n) {input.cursor_color(n);}
    int buttonssize() const {return butsize;}
    void buttonssize(int s) { butsize=s;}
    ~Fl_Value_Input_Spin();
};

/* ---- IV - Aug 23 2002 ---- included file: Fl_Value_Slider_Input.H */

class Fl_Value_Slider_Input : public Fl_Value_Slider {
 private:
    CSOUND *csound;
    char soft_;
    int txtboxsize;

    static PUBLIC void input_cb(Fl_Widget*, void*);
    // cause damage() due to value() changing
    virtual PUBLIC void value_damage();

 public:
    Fl_Input input;
    PUBLIC void draw();
    PUBLIC int handle(int);
    PUBLIC void resize(int, int, int, int);
    PUBLIC Fl_Value_Slider_Input(CSOUND * cs, int x, int y, int w, int h,
                                    const char *l = 0);

    void soft(char x) {soft_ = x;}
    char soft() const {return soft_;}

    Fl_Font textfont() const {return input.textfont();}
    void textfont(uchar s) { input.textfont(s);}
    uchar textsize() const {return input.textsize();}
    void textsize(uchar s) {input.textsize(s);}
    Fl_Color textcolor() const {return input.textcolor();}
    void textcolor(uchar n) {input.textcolor(n);}
    Fl_Color cursor_color() const {return input.cursor_color();}
    void cursor_color(uchar n) {input.cursor_color(n);}
    void textboxsize(int s) { txtboxsize=s;}
    int textboxsize() const {return txtboxsize;}
};


// New widgets for Csound5.06 by Gabriel Maldonado
// Ported by Andres Cabrera. This section below comes
// from the file newwidgets.h

typedef struct {
        OPDS    h;
        MYFLT *ihandle;
} FLSLDBNK_GETHANDLE; //gab

typedef struct {
        OPDS    h;
        MYFLT *ihandle, *ifn, *startInd, *startSlid, *numSlid;
        //int oldx, oly;
} FLSLDBNK_SET;  //gab

typedef struct {
        OPDS    h;
        MYFLT *kflag, *ihandle, *ifn, *startInd, *startSlid, *numSlid;
        MYFLT oldValues[MAXSLIDERBANK];
        int numslid, startind, startslid;
        FLSLIDERBANK2 *q;
        MYFLT *table, *outable;
        //int oldx, oly;
} FLSLDBNK2_SETK;

typedef struct {
        OPDS    h;
        MYFLT *kflag, *ihandle, *ifn, *startInd, *startSlid, *numSlid;
        MYFLT oldValues[MAXSLIDERBANK];
        int numslid, startind, startslid;
        FLSLIDERBANK *q;
        MYFLT *table, *outable;
        //int oldx, oly;
} FLSLDBNK_SETK;

typedef struct {
        OPDS    h;
        MYFLT   *koutx, *kouty, *kinside; //outs
        MYFLT   *ioutx_min, *ioutx_max, *iouty_min, *iouty_max,
                    *iwindx_min, *iwindx_max, *iwindy_min, *iwindy_max,
                        *iexpx, *iexpy, *ioutx, *iouty; //ins
        MYFLT oldx, oldy, *tablex, *tabley;
        int expx, expy;
        long tablenx, tableny;
        double rangex, rangey, basex, basey;
        //int   pspace;
} FLXYIN; //gab

typedef struct  {
        OPDS    h;
//      MYFLT *ktrig, *kvalue, *ihandle;
        MYFLT *chan, *cc, *ihandle;
        int     ccVal, oldCCval;
        MYFLT log_base, min, max, range;
        void *WidgAddress, *opcode, *addrSetVal;
        int exp, widg_type;
} FL_MIDI_WIDGET_VALUE;


typedef struct  {
        OPDS    h;
        MYFLT   *kascii,*ifn;//, *ifnMap;
        MYFLT   *table;//, *tableMap;
        int             flag;
} FLKEYIN;

typedef struct  {
        OPDS    h;
        MYFLT  *group;
} FLSETSNAPGROUP;

typedef struct  {
        OPDS    h;
        MYFLT   *x,*y, *b1, *b2, *b3, *flagRaw;
        MYFLT height,width;
} FLMOUSE;

typedef struct  {
        OPDS    h;
        MYFLT *ihandle, *numlinesX, *numlinesY, *iwidth, *iheight, *ix, *iy,*image;
        int width, height;
} FL_HVSBOX;

typedef struct  {
        OPDS    h;
        MYFLT *kx, *ky, *ihandle;
        void *WidgAddress, *opcode;
        MYFLT old_x, old_y;
} FL_SET_HVS_VALUE;

typedef struct  {
        OPDS    h;
        MYFLT max[MAXCHNLS];
        unsigned long widg_address[MAXCHNLS];
        int dummycycles, dummycyc;
} FLTKMETER;

#endif          /* CSOUND_WIDGETS_CPP */
#endif          /* CSOUND_WIDGETS_H */
