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
    OPDS      h;
    MYFLT     *res;
    MYFLT     *kControl;
    MYFLT     *kValue;
 /* ------------------------------------- */
    wiimote_t *wii;
} WIIMOTE;

int wiimote_find(CSOUND *csound, WIIMOTE *p)
{
    int n;
    int cont = (int)(*p->kControl+FL(0.5));
    int val = (int)(*p->kValue+FL(0.5));
    wiimote_t *wiimote;
    wiimote = (wiimote_t *)csound->QueryGlobalVariable(csound, "wiiMote");
    if (wiimote == NULL) {
      csound->CreateGlobalVariable(csound, "wiiMote",
                                           sizeof(wiimote_t));
      wiimote = csound->QueryGlobalVariable(csound, "wiiMote");
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
    wiimote = (wiimote_t *)csound->QueryGlobalVariable(csound, "wiiMote");
    if (wiimote==NULL) 
      return csound->InitError(csound, "No wii open");
    p->wii = wiimote;
}

int wii_data(CSOUND *csound, WIIMOTE *p)
{
    wiimote_t *wii = p->wii;
    if (!wiimote_is_open(wii)) return csound->PerfError(csound, "Not open");
    wiimote_update(wii);
    if (*p->kControl<0) {
      printf("%d -- %.4x: axis=(%d %d %d); tilt=(%f %f %f); force=(%f %f %f)\n",
             wii->battery, wii->keys.bits,
             wii->axis.x, wii->axis.y, wii->axis.z,
             wii->tilt.x, wii->tilt.y, wii->tilt.z,
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
      *p->res = (MYFLT)wii->axis.x;
      return OK;
    case 18:
      *p->res = (MYFLT)wii->axis.y;
      return OK;
    case 19:
      *p->res = (MYFLT)wii->axis.z;
      return OK;
    case 20:
      *p->res = (MYFLT)wii->tilt.x;
      return OK;
    case 21:
      *p->res = (MYFLT)wii->tilt.y;
      return OK;
    case 22:
      *p->res = (MYFLT)wii->tilt.z;
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


#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"wiiconnect", S(WIIMOTE), 1, "i", "To", (SUBR)wiimote_find, NULL, NULL },
  {"wiidata", S(WIIMOTE), 3, "k", "k", (SUBR)wii_data_init, (SUBR)wii_data, NULL },
  {"wiisend", S(WIIMOTE), 3, "", "k", (SUBR)wii_data_init, (SUBR)wii_send, NULL }
};

LINKAGE
