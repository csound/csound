/*
    new_opts.h:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef CSOUND_NEW_OPTS_H
#define CSOUND_NEW_OPTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Assignment to configuration variables from the command line can be done
 * with '-+NAME=VALUE'. Boolean variables can be set to true with any of
 * '-+NAME', '-+NAME=1', '-+NAME=yes', '-+NAME=on', and '-+NAME=true',
 * while setting to false is possible with any of '-+no-NAME', '-+NAME=0',
 *  '-+NAME=no', '-+NAME=off', and '-+NAME=false'.
 */

/* ------------------------ INTERNAL API FUNCTIONS ------------------------ */

/* list command line usage of all registered configuration variables */

void dump_cfg_variables(CSOUND *csound);

/* Parse 's' as an assignment to a configuration variable in the format */
/* '-+NAME=VALUE'. In the case of boolean variables, the format may also */
/* be '-+NAME' for true, and '-+no-NAME' for false. */
/* Return value is zero on success. */

int parse_option_as_cfgvar(CSOUND *csound, const char *s);

#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_NEW_OPTS_H */

