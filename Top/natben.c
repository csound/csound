/*
    natben.c:

    Copyright (C) 1995 Barry Vercoe, John ffitch

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

                                                  /*                NATBEN.C */

static const char FORM_ID[4] = {'F','O','R','M'};

int bytrevhost(void)
{
/*    return(*(long *)FORM_ID != 'FORM'); */
    return(*(long *)FORM_ID != 0x464f524d);
}

short benshort(short sval)   /* coerce a natural short into a bigendian short */
{
    char  benchar[2];
    char *p = benchar;

    *p++ = 0xFF & (sval >> 8);
    *p   = 0xFF & sval;
    return(*(short *)benchar);
}

long benlong(long lval)       /* coerce a natural long into a bigendian long */
{
    char  benchar[4];
    char *p = benchar;

    *p++ = (char)(0xFF & (lval >> 24));
    *p++ = (char)(0xFF & (lval >> 16));
    *p++ = (char)(0xFF & (lval >> 8));
    *p   = (char)(0xFF & lval);
    return(*(long *)benchar);
}

short natshort(short sval)          /* coerce a bigendian short into a natural short */
{
    unsigned char benchar[2];
    short natshort;

    *(short *)benchar = sval;
    natshort = benchar[0];
    natshort <<= 8;
    natshort |= benchar[1];
    return(natshort);
}

long natlong(long lval)             /* coerce a bigendian long into a natural long */
{
    unsigned char benchar[4];
    unsigned char *p = benchar;
    long natlong;

    *(long *)benchar = lval;
    natlong = *p++;
    natlong <<= 8;
    natlong |= *p++;
    natlong <<= 8;
    natlong |= *p++;
    natlong <<= 8;
    natlong |= *p;
    return(natlong);
}

