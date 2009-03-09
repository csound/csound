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
#define _ENABLE_TILT
#define _ENABLE_FORCE
#include "wiimote_api.h"

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
  MYFLT tilt_z_min;
  MYFLT tilt_z_scale;
  /* MYFLT joyx_min; */
  /* MYFLT joyx_max; */
  /* MYFLT joyy_min; */
  /* MYFLT joyy_max; */
  /* MYFLT naxis_x_min; */
  /* MYFLT naxis_x_max; */
  /* MYFLT naxis_y_min; */
  /* MYFLT naxis_y_max; */
  /* MYFLT naxis_z_min; */
  /* MYFLT naxis_z_max; */
} wiirange_t;


typedef struct {
    OPDS      h;
    MYFLT     *res;
    MYFLT     *kControl;
    MYFLT     *kValue;
 /* ------------------------------------- */
    wiimote_t *wii;
    wiirange_t *wiir;
} WIIMOTE;

typedef struct {
    OPDS      h;
    MYFLT     *iControl;
    MYFLT     *iMin;
    MYFLT     *iMax;
} WIIRANGE;

int wiimote_find(CSOUND *csound, WIIMOTE *p)
{
    int n;
    int cont = (int)(*p->kControl+FL(0.5));
    int val = (int)(*p->kValue+FL(0.5));
    wiimote_t *wiimote;
    wiirange_t *wiirange;

    wiimote = (wiimote_t *)csound->QueryGlobalVariable(csound, "wiiMote");
    if (wiimote == NULL) {
      csound->CreateGlobalVariable(csound, "wiiMote",
                                           sizeof(wiimote_t));
      wiimote = (wiimote_t *)csound->QueryGlobalVariable(csound, "wiiMote");
    }
    wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (wiirange == NULL) {
      csound->CreateGlobalVariable(csound, "wiiRange",
                                           sizeof(wiirange_t));
      wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    }
    if (p->XSTRCODE==0) {
      n  = wiimote_discover(wiimote, 1); /* Only one for now -- could be 4 */
      if (n==1) n = wiimote_connect(wiimote, wiimote->link.r_addr);
      else n = -1;
    }
    else
      n = wiimote_connect(wiimote, csound->currevent->strarg);
    if (n < 0) {
      char buffer[256];
      snprintf(buffer, 256,
               Str("unable to open wiimote: %s\n"), wiimote_get_error());
      csound->InitError(csound, buffer);
    }
    /* Initialise ranges */
    wiirange->axis_x_min = FL(0.0);
    wiirange->axis_x_scale = FL(1.0);
    wiirange->axis_y_min = FL(0.0);
    wiirange->axis_y_scale = FL(1.0);
    wiirange->axis_z_min = FL(0.0);
    wiirange->axis_z_scale = FL(1.0);
    wiirange->tilt_x_min = -FL(90.0);
    wiirange->tilt_x_scale = FL(1.0);
    wiirange->tilt_y_min = -FL(90.0);
    wiirange->tilt_y_scale = FL(1.0);
    wiirange->tilt_z_min = -FL(90.0);
    wiirange->tilt_z_scale = FL(1.0);
    wiimote->led.bits  = 1;
    wiimote->mode.bits = WIIMOTE_MODE_ACC_EXT;
    /* wiimote->mode.acc = (val&1); */
    /* wiimote->mode.ir = (val&2); */
    wiimote_update(wiimote);
    csound->Message(csound, Str("connected to %s on %s in mode %d\n"),
                    wiimote->link.r_addr, wiimote->link.l_addr, val&3);
    *p->res = FL(1.0);
    return OK;
}

int wii_data_init(CSOUND *csound, WIIMOTE *p)
{
    wiimote_t *wiimote;
    wiirange_t *wiirange;
    wiimote = (wiimote_t *)csound->QueryGlobalVariable(csound, "wiiMote");
    if (wiimote==NULL) 
      return csound->InitError(csound, "No wii open");
    wiirange = (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (wiirange==NULL) 
      return csound->InitError(csound, "No wii open");
    p->wii = wiimote;
    p->wiir = wiirange;
}

int wii_data(CSOUND *csound, WIIMOTE *p)
{
    wiimote_t *wii = p->wii;
    wiirange_t *wiir = p->wiir;
    if (!wiimote_is_open(wii)) return csound->PerfError(csound, "Not open");
    wiimote_update(wii);
    if (*p->kControl<0) {
      printf("%d -- %.4x: axis=(%3d %3d %3d); tilt=(%f %f %f);\tforce=(%f %f %f)\n",
             wii->battery, wii->keys.bits,
             wii->axis.x, wii->axis.y, wii->axis.z,
             wii->tilt.x, wii->tilt.y, wii->tilt.z,
             wii->force.x, wii->force.y, wii->force.z);
      printf("%d -- %.4x: axis=[%f %f %f]; tilt=[%f %f %f];\nforce=(%f %f %f)\n",
             wii->battery, wii->keys.bits,
             wiir->axis_x_min+wiir->axis_x_scale*(MYFLT)wii->axis.x,
             wiir->axis_y_min+wiir->axis_y_scale*(MYFLT)wii->axis.y,
             wiir->axis_z_min+wiir->axis_z_scale*(MYFLT)wii->axis.z,
             wiir->tilt_x_min+wiir->tilt_x_scale*(FL(90.0)+(MYFLT)wii->tilt.x),
             wiir->tilt_y_min+wiir->tilt_y_scale*(FL(90.0)+(MYFLT)wii->tilt.y),
             wiir->tilt_z_min+wiir->tilt_z_scale*(FL(90.0)+(MYFLT)wii->tilt.z),
             wii->force.x, wii->force.y, wii->force.z);
      *p->res = FL(0.0);
      return OK;
    }
    else switch ((int)(*p->kControl+FL(0.5))) {
    case 0:
      *p->res = (MYFLT)wii->keys.bits;
      return OK;
    case 1:
      *p->res = (MYFLT)wii->keys.left;
      return OK;
    case 2:
      *p->res = (MYFLT)wii->keys.right;
      return OK;
    case 3:
      *p->res = (MYFLT)wii->keys.up;
      return OK;
    case 4:
      *p->res = (MYFLT)wii->keys.plus;
      return OK;
    case 9:
      *p->res = (MYFLT)wii->keys.two;
      return OK;
    case 10:
      *p->res = (MYFLT)wii->keys.one;
      return OK;
    case 11:
      *p->res = (MYFLT)wii->keys.b;
      return OK;
    case 12:
      *p->res = (MYFLT)wii->keys.a;
      return OK;
    case 13:
      *p->res = (MYFLT)wii->keys.minus;
      return OK;
    case 16:
      *p->res = (MYFLT)wii->keys.home;
      return OK;
    case 17:
      *p->res = wiir->axis_x_min+wiir->axis_x_scale*(MYFLT)wii->axis.x;
      return OK;
    case 18:
      *p->res = wiir->axis_y_min+wiir->axis_y_scale*(MYFLT)wii->axis.y;
      return OK;
    case 19:
      *p->res = wiir->axis_z_min+wiir->axis_z_scale*(MYFLT)wii->axis.z;
      return OK;
    case 20:
      *p->res = wiir->tilt_x_min+wiir->tilt_x_scale*(FL(90.0)+(MYFLT)wii->tilt.x);
      return OK;
    case 21:
      *p->res = wiir->tilt_y_min+wiir->tilt_y_scale*(FL(90.0)+(MYFLT)wii->tilt.y);
      return OK;
    case 22:
      *p->res = wiir->tilt_z_min+wiir->tilt_z_scale*(FL(90.0)+(MYFLT)wii->tilt.z);
      return OK;
    case 23:
      *p->res = (MYFLT)wii->force.x;
      return OK;
    case 24:
      *p->res = (MYFLT)wii->force.y;
      return OK;
    case 25:
      *p->res = (MYFLT)wii->force.z;
      return OK;
    case 26:
      *p->res = SQRT(wii->force.x*wii->force.x+
                     wii->force.y*wii->force.y+
                     wii->force.z*wii->force.z);
      return OK;
    case 27:
      *p->res = (MYFLT)wii->battery;
      return OK;
    case 28:
      *p->res = (MYFLT)wii->ext.nunchuk.joyx;
      return OK;
    case 29:
      *p->res = (MYFLT)wii->ext.nunchuk.joyy;
      return OK;
    case 30:
      *p->res = (MYFLT)wii->ext.nunchuk.axis.x;
      return OK;
    case 31:
      *p->res = (MYFLT)wii->ext.nunchuk.axis.y;
      return OK;
    case 32:
      *p->res = (MYFLT)wii->ext.nunchuk.axis.z;
      return OK;
    case 33:
      *p->res = (MYFLT)wii->ext.nunchuk.keys.z;
      return OK;
    case 34:
      *p->res = (MYFLT)wii->ext.nunchuk.keys.c;
      return OK;
    case 35:
      *p->res = (MYFLT)wii->ir1.x;
      return OK;
    case 36:
      *p->res = (MYFLT)wii->ir1.y;
      return OK;
    case 37:
      *p->res = (MYFLT)wii->ir1.size;
      return OK;
    }
    return NOTOK;
}

int wii_send(CSOUND *csound, WIIMOTE *p)
{
    wiimote_t *wii = p->wii;
    if (!wiimote_is_open(wii)) return csound->PerfError(csound, "Not open");
    switch ((int)(*p->kControl+FL(0.5))) {
    case 1:
      wii->mode.acc = (int)*p->kValue;
      break;
    case 2:
      wii->mode.ir = (int)*p->kValue;
      break;
    case 3:
      wii->rumble = (int)*p->kValue;
      break;
    case 4:
      wii->led.bits = ((uint16)*p->kValue)%5;
      break;
    }
    wiimote_update(wii);
    return OK;
}

int wiimote_range(CSOUND *csound, WIIRANGE *p)
{
    wiirange_t *wiirange =
      (wiirange_t *)csound->QueryGlobalVariable(csound, "wiiRange");
    if (wiirange==NULL) 
      return csound->InitError(csound, "No wii range");
    switch ((int)(*p->iControl+FL(0.5))) {
    case 17:
      wiirange->axis_x_min = *p->iMin;
      wiirange->axis_x_scale = (*p->iMax-*p->iMin)/FL(255.0);
      return OK;
    case 18:
      wiirange->axis_y_min = *p->iMin;
      wiirange->axis_y_scale = (*p->iMax-*p->iMin)/FL(255.0);
      return OK;
    case 19:
      wiirange->axis_z_min = *p->iMin;
      wiirange->axis_z_scale = (*p->iMax-*p->iMin)/FL(255.0);
      return OK;
    case 20:
      wiirange->tilt_x_min = *p->iMin;
      wiirange->tilt_x_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    case 21:
      wiirange->tilt_y_min = *p->iMin;
      wiirange->tilt_y_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    case 22:
      wiirange->tilt_z_min = *p->iMin;
      wiirange->tilt_z_scale = (*p->iMax-*p->iMin)/FL(180.0);
      return OK;
    default:
      return NOTOK;
    }
}



#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"wiiconnect", S(WIIMOTE), 1, "i", "To", (SUBR)wiimote_find, NULL, NULL },
  {"wiidata", S(WIIMOTE), 3, "k", "k", (SUBR)wii_data_init, (SUBR)wii_data, NULL },
  {"wiisend", S(WIIMOTE), 3, "", "k", (SUBR)wii_data_init, (SUBR)wii_send, NULL },
  {"wiirange", S(WIIRANGE), 1, "", "iii", (SUBR)wiimote_range, NULL, NULL }
};

LINKAGE
