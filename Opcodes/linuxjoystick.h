/*
  linuxjoystick.c:
  Copyright (C) 2010 Justin Glenn Smith <noisesmith@gmail.com>

  This Csound plugin is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this plugin; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA

*/
#pragma once

#include <unistd.h>
#include "csdl.h"

#include "linux/joystick.h"

typedef struct
{
  OPDS h;
  MYFLT *kresult, *kdev, *ktable;
  int32_t devFD;
  uint32_t numk, numb;
  int32_t timeout, initme;
  MYFLT table;
  int32_t
  dev;
  FUNC *ftp;
} LINUXJOYSTICK;
