/*
    new_opts.c:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/**
 * Assignment to configuration variables from the command line can be done
 * with '-+NAME=VALUE'. Boolean variables can be set to true with any of
 * '-+NAME', '-+NAME=1', '-+NAME=yes', '-+NAME=on', and '-+NAME=true',
 * while setting to false is possible with any of '-+no-NAME', '-+NAME=0',
 *  '-+NAME=no', '-+NAME=off', and '-+NAME=false'.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csoundCore.h"
#include "csound.h"
#include "new_opts.h"

/* list command line usage of all registered configuration variables */

void dump_cfg_variables(void *csound)
{
    csCfgVariable_t **p;
    int             i;

    p = csoundListConfigurationVariables(csound);
    if (p == NULL)
      return;
    if (p[0] == NULL)
      return;
    err_printf("\n");
    i = 0;
    do {
      err_printf("-+%s=", (char*) ((p[i])->h.name));
      switch ((p[i])->h.type) {
        case CSOUNDCFG_INTEGER:
          err_printf(Str("<integer>"));
          if ((p[i])->i.min > -0x7FFFFFFF)
            err_printf(", %s%d", Str("min: "), (p[i])->i.min);
          if ((p[i])->i.max < 0x7FFFFFFF)
            err_printf(", %s%d", Str("max: "), (p[i])->i.max);
          if ((p[i])->i.flags & CSOUNDCFG_POWOFTWO)
            err_printf(", %s", Str("must be power of two"));
          break;
        case CSOUNDCFG_BOOLEAN:
          err_printf(Str("<boolean>"));
          break;
        case CSOUNDCFG_FLOAT:
          err_printf(Str("<float>"));
          if ((p[i])->f.min > -1.0e24f)
            err_printf(", %s%g", Str("min: "), (double) (p[i])->f.min);
          if ((p[i])->f.max < 1.0e24f)
            err_printf(", %s%g", Str("max: "), (double) (p[i])->f.max);
          break;
        case CSOUNDCFG_DOUBLE:
          err_printf(Str("<float>"));
          if ((p[i])->d.min > -1.0e24)
            err_printf(", %s%g", Str("min: "), (double) (p[i])->d.min);
          if ((p[i])->d.max < 1.0e24)
            err_printf(", %s%g", Str("max: "), (double) (p[i])->d.max);
          break;
        case CSOUNDCFG_MYFLT:
          err_printf(Str("<float>"));
          if ((p[i])->m.min > ((MYFLT) -1.0e24))
            err_printf(", %s%g", Str("min: "), (double) (p[i])->m.min);
          if ((p[i])->m.max < ((MYFLT) 1.0e24))
            err_printf(", %s%g", Str("max: "), (double) (p[i])->m.max);
          break;
        case CSOUNDCFG_STRING:
          err_printf(Str("<string> (max. length = %d characters)"),
                     (p[i])->s.maxlen - 1);
          break;
        default:
          err_printf(Str("<unknown>"));
      }
      err_printf("\n");
      if ((p[i])->h.longDesc != NULL)
        err_printf("\t%s\n", Str((p[i])->h.longDesc));
      else if ((p[i])->h.shortDesc != NULL)
        err_printf("\t%s\n", Str((p[i])->h.shortDesc));
    } while (p[++i] != NULL);
}

/* Parse 's' as an assignment to a configuration variable in the format */
/* '-+NAME=VALUE'. In the case of boolean variables, the format may also */
/* be '-+NAME' for true, and '-+no-NAME' for false. */
/* Return value is zero on success. */

int parse_option_as_cfgvar(void *csound, const char *s)
{
    csCfgVariable_t *p;

    if ((int) strlen(s) < 3) {
      err_printf(Str(" *** '%s' is not a valid Csound command line option.\n"),
                 s);
      err_printf(Str(" *** Type 'csound --help' for the list of "
                     "available options.\n"));
      return -1;
    }
    if (strncmp(s, "-+", 2) != 0) {
      err_printf(Str(" *** '%s' is not a valid Csound command line option.\n"),
                 s);
      err_printf(Str(" *** Type 'csound --help' for the list of "
                     "available options.\n"));
      return -1;
    }
    if (strchr(s, '=') == NULL) {
      /* there is no '=' character, must be a boolean */
      p = csoundQueryConfigurationVariable(csound, s + 2);
      if (p != NULL) {
        if (p->h.type != CSOUNDCFG_BOOLEAN) {
          err_printf(Str(" *** type of option '%s' is not boolean\n"), s + 2);
          return -1;
        }
        *(p->b.p) = 1;
      }
      else if ((int) strlen(s) > 5) {
        if (strncmp(s, "-+no-", 5) != 0) {
          err_printf(Str(" *** '%s': invalid option name\n"), s + 2);
          return -1;
        }
        p = csoundQueryConfigurationVariable(csound, s + 5);
        if (p == NULL) {
          err_printf(Str(" *** '%s': invalid option name\n"), s + 2);
          return -1;
        }
        if (p->h.type != CSOUNDCFG_BOOLEAN) {
          err_printf(Str(" *** type of option '%s' is not boolean\n"), s + 2);
          return -1;
        }
        *(p->b.p) = 0;
      }
      else {
        err_printf(Str(" *** '%s': invalid option name\n"), s + 2);
        return -1;
      }
    }
    else if ((int) strlen(s) > 3) {
      char *buf, *val;
      int  retval;
      buf = (char*) malloc(sizeof(char) * (size_t) ((int) strlen(s) - 1));
      if (buf == NULL) {
        err_printf(Str(" *** memory allocation failure\n"));
        return -1;
      }
      strcpy(buf, s + 2);
      val = strchr(buf, '=');
      *(val++) = '\0';  /* 'buf' is now the name, 'val' is the value string */
      retval = csoundParseConfigurationVariable(csound, buf, val);
      if (retval != CSOUNDCFG_SUCCESS) {
        err_printf(Str(" *** error setting option '%s' to '%s': %s\n"),
                   buf, val, csoundCfgErrorCodeToString(retval));
        free((void*) buf);
        return -1;
      }
      free((void*) buf);
    }
    else {
      err_printf(Str(" *** '%s' is not a valid Csound command line option.\n"),
                 s);
      err_printf(Str(" *** Type 'csound --help' for the list of "
                     "available options.\n"));
      return -1;
    }
    return 0;
}

