/*
    pvl_main.c:

    Copyright (C) 1992 Richard Karpen

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

/*******************************************************************/
/* PVLOOK.C by Richard Karpen 1993 */
/*******************************************************************/

int main(int argc, char *argv[])
{
    if ( argc == 1 ) {
      fprintf( stderr,"pvlook is a program which reads a Csound pvanal's pvoc.n "
               "file and outputs frequency and magnitude trajectories for each "
               "of the analysis bins. \n");
      fprintf(stderr, "usage: pvlook [-bb X] [-eb X] [-bf X] [-ef X] [-i X]  "
              "file > output\n" ) ;
      fprintf( stderr,
               "        -bb X  begin at anaysis bin X. Numbered from 1 "
               "[defaults to 1]\n" ) ;
      fprintf( stderr,
               "        -eb X  end at anaysis bin X [defaults to highest]\n" ) ;
      fprintf( stderr,
               "        -bf X  begin at anaysis frame X. Numbered from 1 "
               "[defaults to 1]\n" ) ;
      fprintf( stderr,
               "        -ef X  end at anaysis frame X [defaults to last]\n" ) ;
      fprintf( stderr,
               "        -i X  prints values as integers [defaults to "
               "floating point]\n" ) ;
      exit( -1 ) ;
    }
    pvlook_main(argc, argv);
}

