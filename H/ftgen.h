#ifndef FTGEN_H
#define FTGEN_H
/*  
    ftgen.h:

    Copyright (C) 1991 Barry Vercoe

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

                                                /*      FTGEN.H        */
void    makevt(void);
FUNC    *hfgens(EVTBLK *);

#define MAXFNUM    100

typedef struct {
        OPDS    h;
        MYFLT   *ifno, *p1, *p2, *p3, *p4, *p5, *argums[VARGMAX];
} FTGEN;

typedef struct { 
        OPDS    h;
        MYFLT   *ifilno, *iflag, *argums[VARGMAX];
} FTLOAD;  /* gab 30 jul 2002 */


typedef struct { 
        OPDS    h;
        MYFLT   *ifilno, *ktrig, *iflag, *argums[VARGMAX];
        FTLOAD  p;
} FTLOAD_K; /* gab 30 jul 2002 */

#define GENMAX  42
#define FTPMAX  (150)

#endif /* FTGEN_H */
