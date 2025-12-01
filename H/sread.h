/*
    sread.h:

    Copyright (C) 1991, 1997 Barry Vercoe, John ffitch

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1335, USA
*/

#ifndef __SREAD_H
#define __SREAD_H
#include "score_param.h"
  int32_t csound_prslex_init(void *);
  void csound_prsset_extra(void *, void *);
  int32_t csound_prslex(CSOUND*, void*);
  int32_t csound_prslex_destroy(void *);
  void cs_init_smacros(CSOUND*, PRS_PARM*, NAMES*);
  void swritestr(CSOUND*, CORFIL *sco, int32_t first);
  void sfree(CSOUND *csound);
  int32_t  sread(CSOUND *csound);
  void sread_initstr(CSOUND *, CORFIL *sco);
  MYFLT stof(CSOUND *, char *);
#endif
