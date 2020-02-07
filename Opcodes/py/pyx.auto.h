/*
 * pyx.auto.
 *
 * Copyright (C) 2002 Maurizio Umberto Puxeddu, Michael Gogins
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

typedef struct {
    OPDS    h;
    STRINGDAT *string;
} PYEXEC;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    STRINGDAT *string;
} PYEXECT;

typedef struct {
    OPDS    h;
    STRINGDAT *string;
} PYRUN;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    STRINGDAT *string;
} PYRUNT;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    STRINGDAT *string;
} PYEVAL;

typedef struct {
    OPDS    h;
    MYFLT   *result;
    MYFLT   *trigger;
    STRINGDAT *string;
    MYFLT   oresult;
} PYEVALT;

typedef struct {
    OPDS    h;
    STRINGDAT *string;
    MYFLT   *value;
} PYASSIGN;

typedef struct {
    OPDS    h;
    MYFLT   *trigger;
    STRINGDAT *string;
    MYFLT   *value;
} PYASSIGNT;

