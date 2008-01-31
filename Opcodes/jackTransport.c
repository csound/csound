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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * U S A G E
 *
 *  jack_transport 0 - to start jack transport
 *
 *  jack_transport 1 - to stop
 *
 *  jack_transport 2 - to rewind to 0.
 *
 */

#include <csdl.h> 
#include <jack/jack.h>
#include <jack/transport.h>

#include "cs_jack.h"

enum { START, STOP, REWIND };

typedef struct
{
  OPDS h;
  MYFLT *command ;
} JACKTRANSPORT;

static int jack_transport (CSOUND *csound, JACKTRANSPORT * p)
{
    RtJackGlobals *rtjack;
    jack_client_t *client;

    rtjack = (RtJackGlobals*) csound->QueryGlobalVariableNoCheck(csound,
                                                                 "_rtjackGlobals");
    client = rtjack->client;

    if (client == NULL) {
      csound->InitError(csound, Str("Cannot find Jack client.\n"));
      return NOTOK;
  }
    else {
      switch ((int)(*p->command)){
      case START: 
        csound->Message(csound, Str("jack_transport playing.\n"));
        jack_transport_start(client);
        break;
      case STOP:
        csound->Message(csound, Str("jack_transport stopped.\n"));
        jack_transport_stop(client);
        break;
      case REWIND:
        csound->Message(csound, Str("jack_transport rewinded.\n"));
        jack_transport_locate(client, 0);
        break;
      default:
        csound->Message(csound, Str("jack_transport: invalid parameter.\n"));
        break;
      }
    }

    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {

  { "jack_transport",  S(JACKTRANSPORT),  1, "", "i",
                       (SUBR)jack_transport, NULL, NULL   },

};

LINKAGE
