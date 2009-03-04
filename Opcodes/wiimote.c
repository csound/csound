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

                                                        /* mp3in.c */
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
    wiimote_t *wiimote;
    wiimote = (wiimote_t *)csound->QueryGlobalVariable(csound, "wiiMote");
    if (wiimote == NULL) {
      csound->CreateGlobalVariable(csound, "wiiMote",
                                           sizeof(wiimote_t));
      wiimote = csound->QueryGlobalVariable(csound, "wiiMote");
    }
    n  = wiimote_discover(wiimote, 1); /* Only one for now -- could be 4 */
    if (n!=1) {
      return NOTOK;
    }
    if (wiimote_connect(wiimote, wiimote->link.r_addr) < 0) {
      char buffer[256];
      snprintf(buffer, 256,
               Str("unable to open wiimote: %s\n"), wiimote_get_error());
      csound->InitError(csound, buffer);
    }
    wiimote->led.bits  = 1;
    csound->Message(csound, Str("connected to %s on %s\n"),
                    wiimote->link.r_addr, wiimote->link.l_addr);
    wiimote->mode.acc = (cont&1);
    wiimote->mode.ir = (cont&2);
    wiimote_update(wiimote);
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
    switch ((int)(*p->kControl+FL(0.5))) {
    case 1:
      *p->res = (MYFLT)wii->keys.one;
      return OK;
    case 2:
      *p->res = (MYFLT)wii->keys.two;
      return OK;
    case 3:
      *p->res = (MYFLT)wii->keys.a;
      return OK;
    case 4:
      *p->res = (MYFLT)wii->keys.b;
      return OK;
    case 5:
      *p->res = (MYFLT)wii->keys.left;
      return OK;
    case 6:
      *p->res = (MYFLT)wii->keys.right;
      return OK;
    case 7:
      *p->res = (MYFLT)wii->keys.up;
      return OK;
    case 8:
      *p->res = (MYFLT)wii->keys.down;
      return OK;
    case 9:
      *p->res = (MYFLT)wii->keys.home;
      return OK;
    case 10:
      *p->res = (MYFLT)wii->keys.plus;
      return OK;
    case 11:
      *p->res = (MYFLT)wii->keys.minus;
      return OK;
    case 12:
      *p->res = (MYFLT)wii->axis.x;
      return OK;
    case 13:
      *p->res = (MYFLT)wii->axis.y;
      return OK;
    case 14:
      *p->res = (MYFLT)wii->axis.z;
      return OK;
    case 15:
      *p->res = (MYFLT)wii->tilt.x;
      return OK;
    case 16:
      *p->res = (MYFLT)wii->tilt.y;
      return OK;
    case 17:
      *p->res = (MYFLT)wii->tilt.z;
      return OK;
    case 18:
      *p->res = (MYFLT)wii->force.x;
      return OK;
    case 19:
      *p->res = (MYFLT)wii->force.y;
      return OK;
    case 20:
      *p->res = (MYFLT)wii->force.z;
      return OK;
    case 21:
      *p->res = SQRT(wii->force.x*wii->force.x+
                     wii->force.y*wii->force.y+
                     wii->force.z*wii->force.z);
      return OK;
    }

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
  {"wiiconnect", S(WIIMOTE), 1, "i", "", (SUBR)wiimote_find, NULL, NULL },
  {"wiidata", S(WIIMOTE), 3, "k", "k", (SUBR)wii_data_init, (SUBR)wii_data, NULL },
  {"wiisend", S(WIIMOTE), 3, "", "k", (SUBR)wii_data_init, (SUBR)wii_send, NULL }
};

LINKAGE
