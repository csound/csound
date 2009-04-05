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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

                                                        /* wiimote.c */
#include "csdl.h"
#include "wiiuse.h"
#define MAX_WIIMOTES				4
#include "wii_mac.h"

typedef struct {
  MYFLT axis_x_min;             /* 0 -> 255 */
  MYFLT axis_x_scale;           /* 1 */
  MYFLT axis_y_min;
  MYFLT axis_y_scale;
  MYFLT axis_z_min;
  MYFLT axis_z_scale;
  MYFLT tilt_x_min;             /* -90 -> +90 */
  MYFLT tilt_x_scale;           /* 1 */
  MYFLT tilt_y_min;
  MYFLT tilt_y_scale;
  /* MYFLT tilt_z_min; */
  /* MYFLT tilt_z_scale; */
  /* MYFLT joyx_min; */
  /* MYFLT joyx_max; */
  /* MYFLT joyy_min; */
  /* MYFLT joyy_max; */
  MYFLT naxis_x_min;
  MYFLT naxis_x_scale;
  MYFLT naxis_y_min;
  MYFLT naxis_y_scale;
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
} WIIMOTE;

typedef struct {
    OPDS      h;
    MYFLT     *res;
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

int wiimote_find(CSOUND *csound, WIIMOTE *p)
{
    int n, i;
    wiimote **wiimotes;
    wiirange_t *wiirange;

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
    i = (int)*p->kControl;
    if (i<=0) i = 10;           /* default timeout */
    n  = wiiuse_find(wiimotes, MAX_WIIMOTES, i);
    if (n!=0) n = wiiuse_connect(wiimotes, MAX_WIIMOTES);
    if (n==0) {
      return csound->InitError(csound, Str("unable to open wiimote\n"));
    }
    /* Initialise ranges */
    for (i=0; i<MAX_WIIMOTES; i++) {
      wiiuse_set_leds(wiimotes[i], WIIMOTE_LED_1<<i);
      wiirange[i].axis_x_min    = FL(0.0);
      wiirange[i].axis_x_scale  = FL(1.0);
      wiirange[i].axis_y_min    = FL(0.0);
      wiirange[i].axis_y_scale  = FL(1.0);
      wiirange[i].axis_z_min    = FL(0.0);
      wiirange[i].axis_z_scale  = FL(1.0);
      wiirange[i].tilt_x_min    = -FL(90.0);
      wiirange[i].tilt_x_scale  = FL(1.0);
      wiirange[i].tilt_y_min    = -FL(90.0);
      wiirange[i].tilt_y_scale  = FL(1.0);
      wiirange[i].naxis_x_min   = -FL(90.0);
      wiirange[i].naxis_x_scale = FL(1.0);
      wiirange[i].naxis_y_min   = -FL(90.0);
      wiirange[i].naxis_y_scale = FL(1.0);
      wiiuse_status(wiimotes[i]);
      wiiuse_motion_sensing(wiimotes[i], 1);
    }
    p->wii = wiimotes;
    *p->res = FL(1.0);
    return OK;
}

int wiimote_poll(CSOUND *csound, WIIMOTE *p)
{
    wiimote **wiimotes = p->wii;
    int i;

    wiiuse_poll(wiimotes, MAX_WIIMOTES);
    for (i=0; i < MAX_WIIMOTES; ++i) {
      switch (wiimotes[i]->event) {
      case WIIUSE_EVENT:
      case WIIUSE_STATUS:
      case WIIUSE_READ_DATA:
        break;
      case WIIUSE_DISCONNECT:
      case WIIUSE_UNEXPECTED_DISCONNECT:
        /* the wiimote disconnected */
        csound->Message(csound,
                        Str("wiimote %i disconnected\n"), wiimotes[i]->unid);
        *p->res = FL(0.0);
        return;
      case WIIUSE_NUNCHUK_INSERTED:
        /*
         *	This is a good place to set any nunchuk specific
         *	threshold values.  By default they are the same
         *	as the wiimote.
         */
        wiiuse_set_nunchuk_orient_threshold((struct nunchuk_t*)&wiimotes[i]->
                                            exp.nunchuk, 90.0f);
        wiiuse_set_nunchuk_accel_threshold((struct nunchuk_t*)&wiimotes[i]->
                                           exp.nunchuk, 100);
        csound->Message(csound, Str("Nunchuk inserted.\n"));
        break;
      case WIIUSE_NUNCHUK_REMOVED:
        /* some expansion was removed */
        csound->Message(csound, Str("Nunchuk for wiimote %i was removed.\n"), i);
        break;
      default:
        break;
      }
    }
    *p->res = FL(1.0);
    return OK;
}


int wii_data_init(CSOUND *csound, WIIMOTE *p)
{
    wiimote **wiimotes;
    wiirange_t *wiirange;
    wiimotes = (wiimote**)csound->QueryGlobalVariable(csound, "wiiMote");
    if (wiimotes==NULL) 
      return csound->InitError(csound, Str("No wii open"));
    wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (wiirange==NULL) 
      return csound->InitError(csound, "No wii open");
    p->wii = wiimotes;
    p->wiir = wiirange;
}

int wii_data(CSOUND *csound, WIIMOTE *p)
{
    wiimote **wii = p->wii;
    wiirange_t *wiir = p->wiir;
    int n = (int)*p->num;
    if (n>=MAX_WIIMOTES) 
      return csound->PerfError(csound, Str("wiimote %d does not exist"), n);
    if (*p->kControl<0) {
      printf("%d -- %.4x: "
             "tilt=[%f %f];\nforce=(%f %f %f)\n",
             100.0*wii[n]->battery_level, wii[n]->btns,
             /* wiir[n].axis_x_min+wiir[n].axis_x_scale*(MYFLT)wii[n]->axis.x, */
             /* wiir[n].axis_y_min+wiir[n].axis_y_scale*(MYFLT)wii[n]->axis.y, */
             /* wiir[n].axis_z_min+wiir[n].axis_z_scale*(MYFLT)wii[n]->axis.z, */
             wiir[n].tilt_x_min+wiir[n].tilt_x_scale*(FL(90.0)+(MYFLT)wii[n]->orient.pitch),
             wiir[n].tilt_y_min+wiir[n].tilt_y_scale*(FL(90.0)-(MYFLT)wii[n]->orient.roll),
             wii[n]->gforce.x, wii[n]->gforce.y, wii[n]->gforce.z);
      *p->res = FL(0.0);
      return OK;
    }
    else switch ((int)(*p->kControl+FL(0.5))) {
    case WII_BUTTONS:
      *p->res = (MYFLT)wii[n]->btns;
      return OK;
    case WII_LEFT:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_LEFT);
      return OK;
    case WII_RIGHT:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_RIGHT);
      return OK;
    case WII_UP:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_UP);
      return OK;
    case WII_PLUS:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_PLUS);
      return OK;
    case WII_TWO:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_TWO);
      return OK;
    case WII_ONE:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_ONE);
      return OK;
    case WII_B:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_B);
      return OK;
    case WII_A:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_A);
      return OK;
    case WII_MINUS:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_MINUS);
      return OK;
    case WII_HOME:
      *p->res = (MYFLT)IS_PRESSED(wii[n], WIIMOTE_BUTTON_HOME);
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
    case WII_TILT_X:
      /* I think the sign is wrong so negated; inplies negative is down */
      *p->res = wiir[n].tilt_x_min+
        wiir[n].tilt_x_scale*(FL(90.0)-(MYFLT)wii[n]->orient.pitch);
      return OK;
    case WII_TILT_Y:
      *p->res = wiir[n].tilt_y_min+
        wiir[n].tilt_y_scale*(FL(90.0)+(MYFLT)wii[n]->orient.roll);
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
    case WII_NUNCHUK_AXIS_X:
      *p->res = wiir[n].naxis_x_min+
        wiir[n].naxis_x_scale*(FL(90.0)-(MYFLT)wii[n]->exp.nunchuk.orient.pitch);
      return OK;
    case WII_NUNCHUK_AXIS_Y:
      *p->res = wiir[n].naxis_y_min+
        wiir[n].naxis_y_scale*(FL(90.0)-(MYFLT)wii[n]->exp.nunchuk.orient.roll);
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

int wii_send(CSOUND *csound, WIIMOTES *p)
{
#define WIIMOTE_STATE_CONNECTED		(0x0008)
    wiimote **wii = p->wii;
    int num = (int)*p->num;
    if (!(wii[num]->state & WIIMOTE_STATE_CONNECTED))
      return csound->PerfError(csound, "Not open");
    switch ((int)(*p->kControl+FL(0.5))) {
    /* case 1: */
    /*   wii->mode.acc = (int)*p->kValue; */
    /*   break; */
    /* case 2: */
    /*   wii->mode.ir = (int)*p->kValue; */
    /*   break; */
    case WII_RUMBLE:
      wiiuse_rumble(wii[num], (int)*p->kValue);
      break;
    case WII_SET_LEDS:
      wiiuse_set_leds(wii[num], (((uint16)*p->kValue)&0xF)<<8);
      break;
    }
    return OK;
}

int wiimote_range(CSOUND *csound, WIIRANGE *p)
{
    wiirange_t *wiirange =
      (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (wiirange==NULL) 
      return csound->InitError(csound, "No wii range");
    switch ((int)(*p->iControl+FL(0.5))) {
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
    case WII_TILT_X:
      wiirange->tilt_x_min = *p->iMin;
      wiirange->tilt_x_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    case WII_TILT_Y:
      wiirange->tilt_y_min = *p->iMin;
      wiirange->tilt_y_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    /* case 22: */
    /*   wiirange->tilt_z_min = *p->iMin; */
    /*   wiirange->tilt_z_scale = (*p->iMax-*p->iMin)/FL(180.0); */
      /* return OK; */
    case WII_NUNCHUK_AXIS_X:
      wiirange->naxis_x_min = *p->iMin;
      wiirange->naxis_x_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    case WII_NUNCHUK_AXIS_Y:
      wiirange->naxis_y_min = *p->iMin;
      wiirange->naxis_y_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    default:
      return NOTOK;
    }
}



#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"wiiconnect", S(WIIMOTE), 3, "i", "o", (SUBR)wiimote_find, wiimote_poll, NULL },
  {"wiidata", S(WIIMOTE), 3, "k", "ko", (SUBR)wii_data_init, (SUBR)wii_data },
  {"wiisend", S(WIIMOTES), 3, "", "kko", (SUBR)wii_data_init, (SUBR)wii_send },
  {"wiirange", S(WIIRANGE), 1, "", "iiio", (SUBR)wiimote_range, NULL, NULL }
};

LINKAGE
