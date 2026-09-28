/*
    grain.h:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/*      Granular synthesizer designed and coded by Paris Smaragdis      */
/*      Berklee College of Music Csound development team                */
/*      Copyright (c) May 1994.  All rights reserved                    */

#pragma once

typedef struct {
    OPDS        h;
    cs_float       *sr, *xamp, *xlfr, *xdns, *kabnd, *kbnd, *kglen;
    cs_float       *igfn, *iefn, *imkglen, *opt;
    cs_float       gcount;
    cs_float       pr;
    AUXCH       aux;
    cs_float       *x, *y;
    FUNC        *gftp, *eftp;
    int16       dnsadv, ampadv, lfradv;
} PGRA;
