/*
  rdorch.c:

  Copyright (C) 1991-2002 Barry Vercoe, John ffitch, Istvan Varga

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

#include "csoundCore.h"         /*                      RDORCH.C        */
#include <ctype.h>
#include "namedins.h"   /* IV - Oct 31 2002 */
#include "typetabl.h"   /* IV - Oct 31 2002 */
#include "envvar.h"
#include <stddef.h>

/* The fromScore parameter should be 1 if opening a score include file,
   0 if opening an orchestra include file */
void *fopen_path(CSOUND *csound, FILE **fp, char *name, char *basename,
                 char *env, int fromScore)
{
  void *fd;
  int  csftype = (fromScore ? CSFTYPE_SCO_INCLUDE : CSFTYPE_ORC_INCLUDE);

  /* First try to open name given */
  fd = csound->FileOpen2(csound, fp, CSFILE_STD, name, "rb", NULL,
                         csftype, 0);
  if (fd != NULL)
    return fd;
  /* if that fails try in base directory */
  if (basename != NULL) {
    char *dir, *name_full;
    if ((dir = csoundSplitDirectoryFromPath(csound, basename)) != NULL) {
      name_full = csoundConcatenatePaths(csound, dir, name);
      fd = csound->FileOpen2(csound, fp, CSFILE_STD, name_full, "rb", NULL,
                             csftype, 0);
      mfree(csound, dir);
      mfree(csound, name_full);
      if (fd != NULL)
        return fd;
    }
  }
  /* or use env argument */
  fd = csound->FileOpen2(csound, fp, CSFILE_STD, name, "rb", env,
                         csftype, 0);
  return fd;
}


void synterr(CSOUND *csound, const char *s, ...)
{
  va_list args;

  csound->MessageS(csound, CSOUNDMSG_ERROR, Str("error:  "));
  va_start(args, s);
  csound->MessageV(csound, CSOUNDMSG_ERROR, s, args);
  va_end(args);


  /* FIXME - Removed temporarily for debugging
   * This function may not be necessary at all in the end if some of this is
   * done in the parser
   */
  csound->synterrcnt++;
}



