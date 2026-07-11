/*
  readline.h:

  Copyright (C) 2026 The Csound Developers

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

#pragma once

#include "csoundCore.h"

/* status is 1 for a completed line, 0 while waiting, and -1 at EOF. */
typedef struct {
  OPDS h;
  STRINGDAT *line;
  MYFLT *status;
  STRINGDAT *prompt;
  void *state;
} READLINE_OPCODE;

int32_t readline_init(CSOUND *, READLINE_OPCODE *);
int32_t readline_perf(CSOUND *, READLINE_OPCODE *);
int32_t readline_deinit(CSOUND *, READLINE_OPCODE *);
