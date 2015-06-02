#include <cstdio>
#include <cstdlib>
#include <cmath>

#if defined(WIN32)
#  include <windows.h>
#endif /* defined(WIN32) */

#if defined(LINUX)
#  include <pthread.h>
#  include <sched.h>
#  include <sys/time.h>
#endif /* defined(LINUX) */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>

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
#include <FL/fl_ask.H>

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;

/*
** Undefine CS_VSTHOST to build FLTK without VST.
*/
#ifdef CS_VSTHOST
#include "Opcodes/vst4cs/src/vsthost.h"
#endif

#include "csdl.h"
#include "winFLTK.h"

#ifdef CS_IMAGE
#  include "stdpch.h"
#  include "anydec.h"
#  include "anybmp.h"
#  include "imagestruct.h"
#endif

#define CSOUND_WIDGETS_CPP 1
#include "widgets.h"

//#include "newwidgets.h" //GAB
// file newwidgets.h has been included in widgets.h

#define LIN_ (0)
#define EXP_ (-1)
#undef min
#undef max

typedef struct rtEvt_s {
    struct rtEvt_s  *nxt;
    EVTBLK          evt;
} rtEvt_t;

#ifndef NO_FLTK_THREADS
typedef struct widgetsGlobals_s {
    rtEvt_t     *eventQueue;
    void        *mutex_;
    int         exit_now;       /* set by GUI when all windows are closed   */
    int         end_of_perf;    /* set by main thread at end of performance */
    void        *threadHandle;
    int         fltkFlags;
} widgetsGlobals_t;
#endif


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

struct VALUATOR_FIELD {
  MYFLT value, value2;
  MYFLT min,max, min2,max2;
  int     exp, exp2;
  string widg_name;
  string opcode_name;
  SLDBK_ELEMENT *sldbnk;
private:
  vector<MYFLT> sldbnkValues;
public:
  VALUATOR_FIELD() {
    value = 0; value2 =0; widg_name= ""; opcode_name ="";
    min = 0; max =1; min2=0; max2=1; exp=LIN_; exp2=LIN_; sldbnk=0;
  }
  void set_sldbnk(int ndx, MYFLT val) {
    if (ndx >= (int)sldbnkValues.size())
            sldbnkValues.resize(ndx+1);
    sldbnkValues[ndx]= val;
  }
  MYFLT get_sldbnk(int ndx) {
          return sldbnkValues[ndx];
  }
//   ~VALUATOR_FIELD() { if (sldbnk != 0) delete sldbnk;
//   if (sldbnkValues !=0) delete sldbnkValues; }
};


enum FL_ENUM {FL_WIDG=0, FL_JOY, JOY_X=128, JOY_Y }; //gab

struct ADDR_SET_VALUE /*: ADDR*/{
  int exponential;
  MYFLT min,max;
  void  *WidgAddress,  *opcode;
  FL_ENUM widg_type; //gab
  FL_ENUM joy;  //gab
  int group; // group for snapshot groups
  ADDR_SET_VALUE(int new_exponential,MYFLT new_min, MYFLT new_max,
                 void *new_WidgAddress, void *new_opcode,  int grp = 0) :
  exponential(new_exponential),min(new_min), max(new_max),
    WidgAddress(new_WidgAddress),opcode(new_opcode),
    widg_type(FL_WIDG), group(grp), joy(FL_JOY) {}
  ADDR_SET_VALUE() {
      exponential=LIN_; min=0; max=0; WidgAddress=NULL; opcode=NULL;
      widg_type = FL_WIDG; group = 0;
      joy = FL_JOY;
  }
};

// struct ADDR_SET_VALUE /*: ADDR*/{
//   int exponential;
//   MYFLT min,max;
//   void  *WidgAddress,  *opcode;
//   ADDR_SET_VALUE(int new_exponential,MYFLT new_min, MYFLT new_max,
//                  void *new_WidgAddress, void *new_opcode) :
//     exponential(new_exponential),min(new_min), max(new_max),
//     WidgAddress(new_WidgAddress),opcode(new_opcode) {}
//   ADDR_SET_VALUE() {
//     exponential=LIN_; min=0; max=0;
//     WidgAddress=NULL; opcode=NULL;
//   }
// };


struct PANELS {
  Fl_Window *panel;
  int     is_subwindow;
  PANELS(Fl_Window *new_panel, int flag) : panel(new_panel),
                                           is_subwindow(flag) { }
  PANELS() {panel = NULL; is_subwindow=0; }
};

struct SNAPSHOT {
  int is_empty;
  vector<VALUATOR_FIELD> fields;
  SNAPSHOT(vector<ADDR_SET_VALUE>& valuators, int snapGroup = 0);
  SNAPSHOT() { is_empty = 1; }
  int get(vector<ADDR_SET_VALUE>& valuators, int snapGroup = 0);
};

extern "C"
{
typedef vector<SNAPSHOT> SNAPVEC; //gab
typedef struct {
    char hack_o_rama1;       // IV - Aug 23 2002
    char hack_o_rama2;
    int ix, drag, indrag, sldrag;
    int stack_count;

    int FLcontrol_iheight;
    int FLroller_iheight;
    int FLcontrol_iwidth;
    int FLroller_iwidth;
    int FLvalue_iwidth;

    int FLcolor;
    int FLcolor2;
    int FLtext_size;
    int FLtext_color;
    int FLtext_font;
    int FLtext_align;

    int currentSnapGroup; // GAB for snapshot groups
    int last_KEY;  // GAB
    bool isKeyDown;  //GAB


    int FL_ix;
    int FL_iy;

    vector<PANELS> fl_windows; // all panels
    //static vector<void*> AddrValue;
    //        addresses of widgets that display current value of valuators
    vector<ADDR_STACK> AddrStack; //addresses of containers
    vector<ADDR_SET_VALUE> AddrSetValue; //addresses of valuators
    vector<char*> allocatedStrings;
//      map<int, SNAPVEC> snapshots; //gab
//      map<int, SNAPVEC>::iterator snapshots_iterator; // iterator of the map
    int last_sldbnk;

    FL_MIDI_WIDGET_VALUE *midiFLaddress[16][128]; // gab128 cc * 16 midi channels
    int midiFLold_val[16][128]; //gab
// GAB (MAKING snapshots GLOBAL IS CERTAINLY A TEMPORARY UGLY HACK, but map
// seems not to function in the WIDGET_GLOBALS structure)
    vector<SNAPVEC> snapshots;
 //GAB (MAKING snapshots_iterator GLOBAL IS A TEMPORARY UGLY HACK)
    vector<SNAPVEC>::iterator snapshots_iterator;
    FLTKMETER *p_vumeter;

#ifdef CS_IMAGE
    vector<ImageSTRUCT> Bm_image; // map of pointers to CAnyBmp objects
    vector<ImageSTRUCT>::iterator Bm_image_iterator; // iterator of the map
#endif

} WIDGET_GLOBALS;
}

#define ST(x)   ((widgetGlobals)->x)
