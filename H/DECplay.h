/*  
    DECplay.h:

    Copyright (C) 1991 Dan Ellis

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

/*******************************************************************\
*   DECplay.h                                                       *
*   Play via LoFi 56k & Sony D/a on DecStation for dspBench         *
*   07aug90 dpwe                                                    *
\*******************************************************************/

#ifdef __STDC__
int  play_set(int chans, int dsize, double srate, int scale);
void play_rls(void);
void play_on(short *buf, long siz);     /* siz = number of frames */
int get_playbuf_remains(void);  /* return number of unplayed sample frames */
#else
int  play_set();
void play_rls();
void play_on();
int get_playbuf_remains();
#endif
