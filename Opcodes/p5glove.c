/*
    p5glove.c:

    Copyright (C) 2009 by John ffitch,

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

                                                        /* p5glove.c */
#include "csdl.h"
#include <p5glove.h>
#include <errno.h>

typedef struct {
    OPDS      h;
 /* ------------------------------------- */
    void      *thread;
    int32_t       on;
} P5GLOVEINIT;

typedef struct {
    OPDS      h;
    MYFLT     *res;
    MYFLT     *kControl;
    MYFLT     *num;
 /* ------------------------------------- */
    uint32_t  last;
} P5GLOVE;

typedef struct {
    OPDS      h;
    MYFLT     *iControl;
    MYFLT     *iMin;
    MYFLT     *iMax;
    MYFLT     *num;
} P5GRANGE;

#define P5G_BUTTONS     0
#define P5G_BUTTON_A    1
#define P5G_BUTTON_B    2
#define P5G_BUTTON_C    4
#define P5G_JUSTPUSH    8
#define P5G_JUSTPU_A    9
#define P5G_JUSTPU_B    10
#define P5G_JUSTPU_C    12
#define P5G_RELEASED    16
#define P5G_RELSED_A    17
#define P5G_RELSED_B    18
#define P5G_RELSED_C    20
#define P5G_FINGER_INDEX 32
#define P5G_FINGER_MIDDLE 33
#define P5G_FINGER_RING  34
#define P5G_FINGER_PINKY 35
#define P5G_FINGER_THUMB 36
#define P5G_DELTA_X     37
#define P5G_DELTA_Y     38
#define P5G_DELTA_Z     39
#define P5G_DELTA_XR    40
#define P5G_DELTA_YR    41
#define P5G_DELTA_ZR    42
#define P5G_ANGLES      43

//P5Glove myGlove;

uintptr_t runp5thread(void *g)
{
    P5Glove    *glove = (P5Glove *)g;
    while (*glove) {
      p5glove_sample(*glove, 0.2);
    }
    return 0;
}

int32_t p5g_deinit(CSOUND *csound, P5GLOVEINIT *p)
{
    p->on = 0;
    return pthread_cancel((pthread_t)(p->thread));
}

int32_t p5glove_find(CSOUND *csound, P5GLOVEINIT *p)
{
    P5Glove    *glove = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    if (glove == NULL) {
      csound->CreateGlobalVariable(csound, "p5glove", sizeof(P5Glove));
      glove = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    }
    *glove = p5glove_open(0);
    if (UNLIKELY(*glove==NULL)) {
      return csound->InitError(csound, Str("unable to open p5glove\n"));
    }
    p->on = 1;
    csound->RegisterDeinitCallback(csound, p,
                                   (int32_t (*)(CSOUND *, void *)) p5g_deinit);
    p->thread = csound->CreateThread(runp5thread, glove);
    return OK;
}

int32_t p5glove_poll(CSOUND *csound, P5GLOVE *p)
{
    /* P5Glove *glove = (P5Glove*)csound->QueryGlobalVariable(csound,"p5glove"); */
    /* int32_t res; */
    /* if (glove == NULL) */
    /*   return csound->PerfError(csound,  p->h.insdshead, */
    /*                            Str("No glove open")); */
    /* res = p5glove_sample(*glove, -1); */
    /* if (res < 0 && errno == EAGAIN) return OK; */
    /* //res = p5glove_sample(*glove, -1); */
    /* if (UNLIKELY(res < 0)) */
    /*   return csound->PerfError(csound,  p->h.insdshead, */
    /*                            Str("P5Glove failure")); */
    return OK;
}

int32_t p5glove_closer(CSOUND *csound, P5GLOVE *p)
{
    P5Glove    *glove = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    if (glove==NULL) return NOTOK;
    printf("Closer called\n");
    p5glove_close(*glove);
    *glove = NULL;
    csound->DestroyGlobalVariable(csound, "p5glove");
    return OK;
}

int32_t p5g_data_init(CSOUND *csound, P5GLOVE *p)
{
    /* P5Glove p5g; */
    /* p5g = (P5Glove)csound->QueryGlobalVariable(csound, "p5glove"); */
    /* if (UNLIKELY(p5g==NULL)) */
    /*   return csound->InitError(csound, Str("No p5glove open")); */
    p->last = 0;
    return OK;
}

int32_t p5g_data(CSOUND *csound, P5GLOVE *p)
{
    P5Glove *glove = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    int32_t kontrol = (int)(*p->kControl+FL(0.5));
    uint32_t buttons, just, rels;
    if (glove==NULL)
      csound->PerfError(csound,  p->h.insdshead, Str("No open glove"));
    p5glove_get_buttons(*glove,&buttons);
    just = ((!p->last) & buttons);
    rels = (p->last & !buttons);
    p->last = buttons;
    if (kontrol<0) {
      printf("debug:\n");
      *p->res = FL(0.0);
      return OK;
    }
    else if (kontrol<P5G_FINGER_INDEX) {
      /* printf("buttons=%.1x\tjust=%.1x\trels=%.1x\n", buttons, just, rels); */
      switch (kontrol) {
      case P5G_BUTTONS:
        *p->res = (MYFLT)(buttons);
      case P5G_BUTTON_A:
        *p->res = (MYFLT)(buttons & P5GLOVE_BUTTON_A);
        return OK;
      case P5G_BUTTON_B:
        *p->res = (MYFLT)(buttons & P5GLOVE_BUTTON_B);
        return OK;
      case P5G_BUTTON_C:
        *p->res = (MYFLT)(buttons & P5GLOVE_BUTTON_C);
        return OK;
      case P5G_JUSTPUSH:
        *p->res = (MYFLT)just;
        return OK;
     case P5G_JUSTPU_A:
        *p->res = (MYFLT)(just & P5GLOVE_BUTTON_A);
        return OK;
      case P5G_JUSTPU_B:
        *p->res = (MYFLT)(just & P5GLOVE_BUTTON_B);
        return OK;
      case P5G_JUSTPU_C:
        *p->res = (MYFLT)(just & P5GLOVE_BUTTON_C);
        return OK;
      case P5G_RELEASED:
        *p->res = (MYFLT)rels;
        return OK;
      case P5G_RELSED_A:
        *p->res = (MYFLT)(rels & P5GLOVE_BUTTON_A);
        return OK;
      case P5G_RELSED_B:
        *p->res = (MYFLT)(rels & P5GLOVE_BUTTON_B);
        return OK;
      case P5G_RELSED_C:
        *p->res = (MYFLT)(rels & P5GLOVE_BUTTON_C);
        return OK;
      default:
        *p->res = 0.0;
        return NOTOK;
      }
    }
    else if (kontrol<=P5G_FINGER_THUMB) {
      double clench;
      p5glove_get_finger(*glove,kontrol-P5G_FINGER_INDEX,&clench);
      *p->res = (MYFLT)clench;
      /*      *p->res = (MYFLT)(p5->data.finger[finger]/63.0); */
      return OK;
    }
    else {
      double pos[3], axis[3], angle;
      p5glove_get_position(*glove, pos);
      p5glove_get_rotation(*glove, &angle, axis);
      /* printf("pos: %f %f %f\n", pos[0], pos[1], pos[2]); */
      /* printf("axis: %f %f %f\n", axis[0], axis[1], axis[2]); */
      /* printf("angle: %f\n", angle); */
      switch (kontrol) {
      case P5G_DELTA_X:
        *p->res = (MYFLT)pos[0];
        return OK;
      case P5G_DELTA_Y:
        *p->res = (MYFLT)pos[1];
        return OK;
      case P5G_DELTA_Z:
        *p->res = (MYFLT)pos[2];
        return OK;
      case P5G_DELTA_XR:
        *p->res = (MYFLT)axis[0];
        return OK;
      case P5G_DELTA_YR:
        *p->res = (MYFLT)axis[1];
        return OK;
      case P5G_DELTA_ZR:
        *p->res = (MYFLT)axis[2];
        return OK;
      case P5G_ANGLES:
        *p->res = (MYFLT)angle;
        return OK;
      default:
        break;
      }
    }
    return NOTOK;
}

#define S(x)    sizeof(x)

static OENTRY p5g_localops[] = {
  {"p5gconnect", S(P5GLOVEINIT), 0, 3, "", "",
        (SUBR)p5glove_find, (SUBR)p5glove_poll, NULL, (SUBR)p5glove_closer },
  {"p5gdata", S(P5GLOVE), 0, 3, "k", "k", (SUBR)p5g_data_init, (SUBR)p5g_data }
};

LINKAGE_BUILTIN(p5g_localops)
