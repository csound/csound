/*
    scangen.c:

    Copyright (C) 2021 >Jon ffitch

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

#include "csdl.h"
#include <math.h>

static int isstrcod(MYFLT xx)
{
#ifdef USE_DOUBLE
    union {
      double d;
      int32_t i[2];
    } z;
    z.d = xx;
    return ((z.i[1]&0x7ff00000)==0x7ff00000);
#else
    union {
      float f;
      int32_t i;
    } z;
    z.f = xx;
    return ((z.i&0x7f800000) == 0x7f800000);
#endif
}

static int32_t scantable (FGDATA *ff, FUNC *ftp)
{
    /*
      This Gen routine calculates a stiffness matrix for scanu/scanu2.

    */

    MYFLT   *fp = ftp->ftable;
    CSOUND  *csound = ff->csound;
    FILE    *filp;
    void    *fd;
    char buff[80];
    int len;
    int i, j, n;
    MYFLT stiffness;
    
    if (isstrcod(ff->e.p[5]))
      strncpy(buff, (char *)(&ff->e.strarg[0]), 79);
    else
      csound->strarg2name(csound, buff, &(ff->e.p[5]), "stiff.", 0);
    fd = csound->FileOpen2(csound, &filp, CSFILE_STD, buff, "r",
                          "SFDIR;SSDIR;INCDIR", CSFTYPE_FLOATS_TEXT, 0);
    if (UNLIKELY(fd == NULL))
      return csound->InitError(csound, Str("Failed to open file %s\n"), buff);
    if (UNLIKELY(NULL==fgets(buff, 80, filp)))
      return csound->InitError(csound, Str("Failed to read matrix file"));
    if (UNLIKELY(strncmp(buff,"<MATRIX size=", sizeof("<MATRIX size="))!=0)) 
      return csound->InitError(csound, Str("No header in matrix file"));
    sscanf(buff+sizeof("<MATRIX size="), "%d>\n", &len);
    if (ff->e.p[3]< len*len)
      return csound->InitError(csound, Str("No header in matrix file"));
    memset(fp, '\0', sizeof(MYFLT)*ff->e.p[3]);
    while (NULL!=  fgets(buff, 80, filp)) {
      n = sscanf(buff, " %d %d %lf a\n", &i, &j, &stiffness);
      if (n==2)stiffness = FL(1.0);
      else if (n!=3) return csound->InitError(csound, Str("format error\n"));
      if (i<1 || i>=len || j<1 || j>=len)
        return csound->InitError(csound, Str("Out of range"));
      fp[(i-1)*len+j-1] = stiffness;
    }
    if (ff->e.p[4]>0) ff->e.p[4] = -ff->e.p[4];
    return OK;
}

static NGFENS scan_fgens[] = {
  { "stiffness", scantable },
  { NULL, NULL }
};

FLINKAGE_BUILTIN(scan_fgens)
