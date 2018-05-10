/*
    wiimote.c:

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

                                                        /* wiimote.c */
#include "csdl.h"
#include "wiiuse.h"             /* Uses WIIUSE library which is LGPL */
#define MAX_WIIMOTES                            4
#include "wii_mac.h"

typedef struct {
  MYFLT axis_x_min;             /* 0 -> 255 */
  MYFLT axis_x_scale;           /* 1 */
  MYFLT axis_y_min;
  MYFLT axis_y_scale;
  MYFLT axis_z_min;
  MYFLT axis_z_scale;
  MYFLT pitch_min;             /* -90 -> +90 */
  MYFLT pitch_scale;           /* 1 */
  MYFLT roll_min;
  MYFLT roll_scale;
  /* MYFLT tilt_z_min; */
  /* MYFLT tilt_z_scale; */
  MYFLT joy_min;
  MYFLT joy_scale;
  MYFLT nunchuk_pitch_min;
  MYFLT nunchuk_pitch_scale;
  MYFLT nunchuk_roll_min;
  MYFLT nunchuk_roll_scale;
  /* MYFLT naxis_z_min; */
  /* MYFLT naxis_z_max; */
} wiirange_t;


typedef struct {
    OPDS      h;
    MYFLT     *res;
    MYFLT     *kControl;
    MYFLT     *num;
 /* ------------------------------------- */
    wiimote **wii;
    wiirange_t *wiir;
    int32_t       max_wiimotes;
} WIIMOTE;

typedef struct {
    OPDS      h;
    MYFLT     *kControl;
    MYFLT     *kValue;
    MYFLT     *num;
 /* ------------------------------------- */
    wiimote **wii;
    wiirange_t *wiir;
} WIIMOTES;

typedef struct {
    OPDS      h;
    MYFLT     *iControl;
    MYFLT     *iMin;
    MYFLT     *iMax;
    MYFLT     *num;
 } WIIRANGE;

#ifndef WIIMOTE_STATE_CONNECTED
# if defined(WIIUSE_0_13)
#  define WIIMOTE_STATE_CONNECTED          (0x0010)
# else
#  define WIIMOTE_STATE_CONNECTED          (0x0008)
# endif
#endif

int32_t wiimote_find(CSOUND *csound, WIIMOTE *p)
{
    int32_t n, i;
    wiimote **wiimotes;
    wiirange_t *wiirange;
    int32_t max_wiimotes;

    wiimotes = (wiimote**)csound->QueryGlobalVariable(csound, "wiiMote");
    if (wiimotes == NULL) {
      csound->CreateGlobalVariable(csound, "wiiMote",
                                           MAX_WIIMOTES*sizeof(wiimote*));
      wiimotes = (wiimote**)csound->QueryGlobalVariable(csound, "wiiMote");
    }
    wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (wiirange == NULL) {
      csound->CreateGlobalVariable(csound, "wiiRange",
                                           MAX_WIIMOTES*sizeof(wiirange_t));
      wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    }
    {                           /* Use our copy not his */
      wiimote** ww = wiiuse_init(MAX_WIIMOTES);
      memcpy(wiimotes, ww, MAX_WIIMOTES*sizeof(wiimote*));
      free(ww);
    }
    i = (int32_t)*p->kControl;
    if (i<=0) i = 10;           /* default timeout */
    max_wiimotes = (int32_t)*p->num;
    if (max_wiimotes<=0 || max_wiimotes>MAX_WIIMOTES) max_wiimotes = MAX_WIIMOTES;
    n  = wiiuse_find(wiimotes, max_wiimotes, i);
    if (LIKELY(n!=0)) n = wiiuse_connect(wiimotes, max_wiimotes);
    if (UNLIKELY(n==0)) {
      return csound->InitError(csound, "%s", Str("unable to open wiimote\n"));
    }
    /* Initialise ranges */
    for (i=0; i<n; i++) {
      wiiuse_set_leds(wiimotes[i], WIIMOTE_LED_1<<i);
      wiirange[i].axis_x_min    = FL(0.0);
      wiirange[i].axis_x_scale  = FL(1.0);
      wiirange[i].axis_y_min    = FL(0.0);
      wiirange[i].axis_y_scale  = FL(1.0);
      wiirange[i].axis_z_min    = FL(0.0);
      wiirange[i].axis_z_scale  = FL(1.0);
      wiirange[i].pitch_min    = -FL(90.0);
      wiirange[i].pitch_scale  = FL(1.0);
      wiirange[i].roll_min    = -FL(90.0);
      wiirange[i].roll_scale  = FL(1.0);
      wiirange[i].nunchuk_pitch_min   = -FL(90.0);
      wiirange[i].nunchuk_pitch_scale = FL(1.0);
      wiirange[i].nunchuk_roll_min   = -FL(90.0);
      wiirange[i].nunchuk_roll_scale = FL(1.0);
      wiiuse_status(wiimotes[i]);
      wiiuse_motion_sensing(wiimotes[i], 1);
    }
    p->wii = wiimotes;
    p->max_wiimotes = n;
    *p->res = FL(1.0);
    return OK;
}

int32_t wiimote_poll(CSOUND *csound, WIIMOTE *p)
{
    wiimote **wiimotes = p->wii;
    int32_t i;
    int32_t max_wiimotes = p->max_wiimotes;

    wiiuse_poll(wiimotes, max_wiimotes);
    for (i=0; i < max_wiimotes; ++i) {
      switch (wiimotes[i]->event) {
      case WIIUSE_EVENT:
      case WIIUSE_STATUS:
      case WIIUSE_READ_DATA:
        break;
      case WIIUSE_DISCONNECT:
      case WIIUSE_UNEXPECTED_DISCONNECT:
        /* the wiimote disconnected */
        csound->Warning(csound,
                        Str("wiimote %i disconnected\n"), wiimotes[i]->unid);
        *p->res = FL(0.0);
        return OK;
      case WIIUSE_NUNCHUK_INSERTED:
        /*
         *      This is a good place to set any nunchuk specific
         *      threshold values.  By default they are the same
         *      as the wiimote.
         */
        /* wiiuse_set_nunchuk_orient_threshold((struct nunchuk_t*)&wiimotes[i]-> */
        /*                                     exp.nunchuk, 90.0f); */
        /* wiiuse_set_nunchuk_accel_threshold((struct nunchuk_t*)&wiimotes[i]-> */
        /*                                    exp.nunchuk, 100); */
        csound->Warning(csound, "%s", Str("Nunchuk inserted.\n"));
        break;
      case WIIUSE_NUNCHUK_REMOVED:
        /* some expansion was removed */
        csound->Warning(csound, Str("Nunchuk for wiimote %i was removed.\n"), i);
        break;
      default:
        break;
      }
    }
    *p->res = FL(1.0);
    return OK;
}


int32_t wii_data_init(CSOUND *csound, WIIMOTE *p)
{
    wiimote **wiimotes;
    wiirange_t *wiirange;
    wiimotes = (wiimote**)csound->QueryGlobalVariable(csound, "wiiMote");
    if (UNLIKELY(wiimotes==NULL))
      return csound->InitError(csound, "%s", Str("No wii open"));
    wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (UNLIKELY(wiirange==NULL))
      return csound->InitError(csound, "%s", Str("No wii open"));
    p->wii = wiimotes;
    p->wiir = wiirange;
    return OK;
}

int32_t wii_data(CSOUND *csound, WIIMOTE *p)
{
    wiimote **wii = p->wii;
    wiirange_t *wiir = p->wiir;
    int32_t n = (int32_t)*p->num;
    int32_t kontrol = (int32_t)(*p->kControl+FL(0.5));
    if (UNLIKELY(n>=MAX_WIIMOTES || !(wii[n]->state & WIIMOTE_STATE_CONNECTED))) {
      printf("state of wii %d is %x\n", n, wii[n]->state);
      return csound->PerfError(csound, p->h.insdshead,
                               Str("wiimote %d does not exist"), n);
    }
    if (kontrol<0) {
      printf("%f -- %.4x: "
             "tilt=[%f %f];\nforce=(%f %f %f)\n",
             100.0*wii[n]->battery_level, wii[n]->btns,
             wiir[n].pitch_min+wiir[n].pitch_scale*(FL(90.0)+
                                                    (MYFLT)wii[n]->orient.pitch),
             wiir[n].roll_min+wiir[n].roll_scale*(FL(90.0)-
                                                  (MYFLT)wii[n]->orient.roll),
             wii[n]->gforce.x, wii[n]->gforce.y, wii[n]->gforce.z);
      *p->res = FL(0.0);
      return OK;
    }
    if (kontrol>0 && kontrol<17) {
      *p->res = (MYFLT)IS_JUST_PRESSED(wii[n], 1<<(kontrol-1));
    }
    if (kontrol>100 && kontrol<117) {
      *p->res = (MYFLT)IS_PRESSED(wii[n], 1<<(kontrol-101));
      return OK;
     }
    if (kontrol>200 && kontrol<217) {
      *p->res = (MYFLT)IS_HELD(wii[n], 1<<(kontrol-201));
      return OK;
     }
    if (kontrol>300 && kontrol<317) {
      *p->res = (MYFLT)IS_RELEASED(wii[n], 1<<(kontrol-301));
      return OK;
     }
    else switch (kontrol) {
    case WII_BUTTONS:
      *p->res = (MYFLT)(wii[n]->btns&WIIMOTE_BUTTON_ALL);
      return OK;
    /* case 17: */
    /*   *p->res = wiir[n].axis_x_min+wiir[n].axis_x_scale*(MYFLT)wii[n]->axis.x; */
    /*   return OK; */
    /* case 18: */
    /*   *p->res = wiir[n].axis_y_min+wiir[n].axis_y_scale*(MYFLT)wii[n]->axis.y; */
    /*   return OK; */
    /* case 19: */
    /*   *p->res = wiir[n].axis_z_min+wiir[n].axis_z_scale*(MYFLT)wii[n]->axis.z; */
    /*   return OK; */
    case WII_PITCH:
      /* I think the sign is wrong so negated; inplies negative is down */
      *p->res = wiir[n].pitch_min+
        wiir[n].pitch_scale*(FL(90.0)-(MYFLT)wii[n]->orient.pitch);
      return OK;
    case WII_ROLL:
      *p->res = wiir[n].roll_min+
        wiir[n].roll_scale*(FL(90.0)+(MYFLT)wii[n]->orient.roll);
      return OK;
    /* case 22: */
    /*   *p->res = wiir[n].tilt_z_min+
         wiir[n].tilt_z_scale*(FL(90.0)-(MYFLT)wii[n]->tilt.z); */
    /*   return OK; */
    case WII_FORCE_X:
      *p->res = (MYFLT)wii[n]->gforce.x;
      return OK;
    case WII_FORCE_Y:
      *p->res = (MYFLT)wii[n]->gforce.y;
      return OK;
    case WII_FORCE_Z:
      *p->res = (MYFLT)wii[n]->gforce.z;
      return OK;
    case WII_FORCE_TOTAL:
      *p->res = SQRT(wii[n]->gforce.x*wii[n]->gforce.x+
                     wii[n]->gforce.y*wii[n]->gforce.y+
                     wii[n]->gforce.z*wii[n]->gforce.z);
      return OK;
    case WII_BATTERY:
      *p->res = FL(100.0)*(MYFLT)wii[n]->battery_level;
      return OK;
    case WII_NUNCHUK_ANG:
      *p->res = (MYFLT)wii[n]->exp.nunchuk.js.ang;
      return OK;
    case WII_NUNCHUK_MAG:
      *p->res = (MYFLT)wii[n]->exp.nunchuk.js.mag;
      return OK;
    case WII_NUNCHUK_PITCH:
      *p->res = wiir[n].nunchuk_pitch_min+
        wiir[n].nunchuk_pitch_scale*(FL(90.0)-
                                     (MYFLT)wii[n]->exp.nunchuk.orient.pitch);
      return OK;
    case WII_NUNCHUK_ROLL:
      *p->res = wiir[n].nunchuk_roll_min+
        wiir[n].nunchuk_roll_scale*(FL(90.0)-
                                    (MYFLT)wii[n]->exp.nunchuk.orient.roll);
     return OK;
    /* case 32: */
    /*   *p->res = (MYFLT)wii[n]->exp.nunchuk.axis.z; */
    /*   return OK; */
    case WII_NUNCHUK_Z:
      *p->res = (MYFLT)((wii[n]->exp.nunchuk.btns & NUNCHUK_BUTTON_Z)==
                        NUNCHUK_BUTTON_Z);
      return OK;
    case WII_NUNCHUK_C:
      *p->res = (MYFLT)((wii[n]->exp.nunchuk.btns & NUNCHUK_BUTTON_C)==
                        NUNCHUK_BUTTON_C);
      return OK;
    case WII_IR1_X:
      *p->res = (MYFLT)wii[n]->ir.x;
      return OK;
    case WII_IR1_Y:
      *p->res = (MYFLT)wii[n]->ir.y;
      return OK;
    case WII_IR1_Z:
      *p->res = (MYFLT)wii[n]->ir.z;
      return OK;
    }
    return NOTOK;
}

int32_t wii_data_inits(CSOUND *csound, WIIMOTES *p)
{
    wiimote **wiimotes;
    wiirange_t *wiirange;
    wiimotes = (wiimote**)csound->QueryGlobalVariable(csound, "wiiMote");
    if (UNLIKELY(wiimotes==NULL))
      return csound->InitError(csound, "%s", Str("No wii open"));
    wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (UNLIKELY(wiirange==NULL))
      return csound->InitError(csound, "%s", Str("No wii open"));
    p->wii = wiimotes;
    p->wiir = wiirange;
    return OK;
}

int32_t wii_send(CSOUND *csound, WIIMOTES *p)
{
    wiimote **wii = p->wii;
    int32_t num = (int32_t)*p->num;
    if (UNLIKELY(!(wii[num]->state & WIIMOTE_STATE_CONNECTED)))
      return csound->PerfError(csound, p->h.insdshead, "%s", Str("Not open"));
    switch ((int32_t)(*p->kControl+FL(0.5))) {
    /* case 1: */
    /*   wii->mode.acc = (int32_t)*p->kValue; */
    /*   break; */
    /* case 2: */
    /*   wii->mode.ir = (int32_t)*p->kValue; */
    /*   break; */
    case WII_RUMBLE:
      wiiuse_rumble(wii[num], (int32_t)*p->kValue);
      break;
    case WII_SET_LEDS:
      wiiuse_set_leds(wii[num], ((uint16)*p->kValue)<<4);
      break;
    }
    return OK;
}

int32_t wiimote_range(CSOUND *csound, WIIRANGE *p)
{
    wiirange_t *wiirange =
      (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (UNLIKELY(wiirange==NULL))
      return csound->InitError(csound, "%s", Str("No wii range"));
    switch ((int32_t)(*p->iControl+FL(0.5))) {
    /* case 17: */
    /*   wiirange->axis_x_min = *p->iMin; */
    /*   wiirange->axis_x_scale = (*p->iMax-*p->iMin)/FL(255.0); */
    /*   return OK; */
    /* case 18: */
    /*   wiirange->axis_y_min = *p->iMin; */
    /*   wiirange->axis_y_scale = (*p->iMax-*p->iMin)/FL(255.0); */
    /*   return OK; */
    /* case 19: */
    /*   wiirange->axis_z_min = *p->iMin; */
    /*   wiirange->axis_z_scale = (*p->iMax-*p->iMin)/FL(255.0); */
    /*   return OK; */
    case WII_PITCH:
      wiirange->pitch_min = *p->iMin;
      wiirange->pitch_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    case WII_ROLL:
      wiirange->roll_min = *p->iMin;
      wiirange->roll_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    /* case 22: */
    /*   wiirange->tilt_z_min = *p->iMin; */
    /*   wiirange->tilt_z_scale = (*p->iMax-*p->iMin)/FL(180.0); */
      /* return OK; */
    case WII_NUNCHUK_PITCH:
      wiirange->nunchuk_pitch_min = *p->iMin;
      wiirange->nunchuk_pitch_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    case WII_NUNCHUK_ROLL:
      wiirange->nunchuk_roll_min = *p->iMin;
      wiirange->nunchuk_roll_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    default:
      return NOTOK;
    }
}



#define S(x)    sizeof(x)

static OENTRY wiimote_localops[] = {
  {"wiiconnect", S(WIIMOTE), 0, 3, "i", "oo", (SUBR)wiimote_find,
                                                           (SUBR)wiimote_poll },
  {"wiidata", S(WIIMOTE), 0, 3, "k", "ko", (SUBR)wii_data_init, (SUBR)wii_data },
  {"wiisend", S(WIIMOTES), 0, 3, "", "kko", (SUBR)wii_data_inits, (SUBR)wii_send },
  {"wiirange", S(WIIRANGE), 0, 1, "", "iiio", (SUBR)wiimote_range, NULL, NULL }
};

LINKAGE_BUILTIN(wiimote_localops)
