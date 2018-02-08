/**
 * jack_transport.c
 *
 * Copyright (c) 2008 by Cesare Marilungo. All rights reserved.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * U S A G E
 *
 *  jack_transport 1 - to start jack transport
 *
 *  jack_transport 0 - to stop
 *
 *  the second optional parameter tells where the transport should be located
 *
 */

#include <csdl.h>
#include <jack/jack.h>
#include <jack/transport.h>

#include "cs_jack.h"

enum { STOP, START };

typedef struct
{
  OPDS h;
  MYFLT *command;
  MYFLT *location;
} JACKTRANSPORT;

static int32_t jack_transport (CSOUND *csound, JACKTRANSPORT * p)
{
    RtJackGlobals *rtjack;
    jack_client_t *client;

    rtjack = (RtJackGlobals*)
      csound->QueryGlobalVariableNoCheck(csound,
                                         "_rtjackGlobals");
    client = rtjack->client;

    if (UNLIKELY(client == NULL)) {
      return csound->InitError(csound, "%s", Str("Cannot find Jack client.\n"));
    }
    else {
      //move to specified location (in seconds)
      if ((int32_t)(*p->location)>=0) {
        MYFLT loc_sec = *p->location;
        MYFLT loc_sec_per_sr = loc_sec*csound->GetSr(csound);
        jack_transport_locate(client, loc_sec_per_sr);
        csound->Warning(csound,
                        Str("jacktransport: playback head moved "
                            "at %f seconds\n"), loc_sec);
      }
      //start or stop
      switch ((int32_t
               )(*p->command)) {
      case START:
        csound->Warning(csound, "%s", Str("jacktransport: playing.\n"));
        jack_transport_start(client);
        break;
      case STOP:
        csound->Warning(csound, "%s", Str("jacktransport: stopped.\n"));
        jack_transport_stop(client);
        break;
      default:
        csound->Warning(csound, "%s", Str("jacktransport: invalid parameter.\n"));
        break;
      }
    }

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY jackTransport_localops[] = {

  { "jacktransport",  S(JACKTRANSPORT),  0, 1, "", "ij",
                       (SUBR)jack_transport, NULL, NULL   },
};

LINKAGE_BUILTIN(jackTransport_localops)
