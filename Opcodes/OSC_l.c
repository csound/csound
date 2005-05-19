/*
    OSC.c:

    Copyright (C) 2005 by John ffitch

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
#include "csdl.h"
#include <lo/lo.h>

typedef struct
{
    OPDS h;             /* default header */
    MYFLT *kwhen;
    MYFLT *host;
    MYFLT *port;       /* UDP port */
    MYFLT *dest;
    MYFLT *type;
    MYFLT *arg[25];
    lo_address addr;
    MYFLT last;
    int   cnt;
} OSCSEND;

int osc_send_set(ENVIRON *csound, OSCSEND *p)
{
    char port[8];
    char *pp= port;
    char *hh;
    lo_address t;
    /* with too many args, XINCODE/XSTRCODE may not work correctly */
    if (p->INOCOUNT > 31)
      csound->InitError(csound, "Too many arguments to OSCsend");
    /* a-rate arguments are not allowed */
    if (p->XINCODE) csound->InitError(csound, "No a-rate arguments allowed");

    if (*p->port<0)
      pp = NULL;
    else
      sprintf(port, "%d", (int)(*p->port+FL(0.5)));
    hh = (char*) p->host;
    if (*hh=='\0') hh = NULL;
    t = lo_address_new(hh, pp);
    p->addr = t;
    p->cnt = 0;
    return OK;
}

int osc_send(ENVIRON *csound, OSCSEND *p)
{
    /* Types I allow at present:
       0) int
       1) float
       2) string
       3) double
       4) char
    */
    if (p->cnt++ && *p->kwhen!=p->last) {
      int i=0;
      int msk = 1;
      lo_message msg = lo_message_new();
      char *type = (char*)p->type;
      MYFLT **arg = p->arg;
      p->last = *p->kwhen;
      for (i=0; type[i]!='\0'; i++, msk <<=1) {
        /* Need to add type checks */
        switch (type[i]) {
        case 'i':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_int32(msg, (int32_t)(*arg[i]+FL(0.5)));
          break;
        case 'c':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_char(msg, (char)(*arg[i]+FL(0.5)));
          break;
        case 'f':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_float(msg, (float)(*arg[i]));
          break;
        case 'd':
          if (p->XSTRCODE&msk)
            return csound->PerfError(csound, Str("String not expected"));
          lo_message_add_double(msg, (double)(*arg[i]));
          break;
        case 's':
          if (p->XSTRCODE&msk)
            lo_message_add_string(msg, (char*)arg[i]);
          else
            return csound->PerfError(csound, Str("Not a string when needed"));
          break;
        default:
          csound->Message(csound, "Unknown OSC type %c\n", type[1]);
        }
      }
      lo_send_message(p->addr, (char*)p->dest, msg);
      lo_message_free(msg);
    }
    return OK;
}

#define S(x) sizeof(x)

static OENTRY localops[] = {
{ "OSCsendl", S(OSCSEND),  3, "",  "kSiSSN", (SUBR)osc_send_set, (SUBR)osc_send }
};

LINKAGE

