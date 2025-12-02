/*
    environ.h: environment vars

    Copyright (C) 2005 Istvan Varga

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

#ifndef CSOUND_ENVIRON_H
#define CSOUND_ENVIRON_H

#if !defined(__BUILDING_LIBCSOUND)
#  error "Csound plugins and host applications should not include files.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Set environment variable 'name' to 'value'.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
   * if the environment variable could not be set for some reason.
   */
  int32_t csoundSetEnv(CSOUND *csound, const char *name, const char *value);

  /**
   * Append 'value' to environment variable 'name', using ';' as
   * separator character.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
   * if the environment variable could not be set for some reason.
   */
  int32_t csoundAppendEnv(CSOUND *csound, const char *name, const char *value);

  /**
   * Prepend 'value' to environment variable 'name', using ';' as
   * separator character.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
   * if the environment variable could not be set for some reason.
   */
  int32_t csoundPrependEnv(CSOUND *csound, const char *name, const char *value);

  /**
   * Initialise environment variable database, and copy system
   * environment variables.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY in case of an error.
   */
  int32_t csoundInitEnv(CSOUND *csound);

  /**
   * Parse 's' as an assignment to environment variable, in the format
   * "NAME=VALUE" for replacing the previous value, or "NAME+=VALUE"
   * for appending.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY in case of an error.
   */
  int32_t csoundParseEnv(CSOUND *csound, const char *s);

  char    **csoundGetSearchPathFromEnv(CSOUND *, const char *);


#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_ENVIRON_H */
