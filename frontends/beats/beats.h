/*
    csbeats.h:

    Copyright (C) 2012 John ffitch,

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


    typedef struct instr {
      struct instr *next;
      int n;                    /* self referencial */
      int largest;
      double *p;
    } INSTR;

extern INSTR *instr;
extern int yyline;
extern int last_note;
extern int last_integer;
extern double last_duration;
extern double bpm;
extern int permeasure;
extern int yydebug;
extern int yyparse(void);
extern void print_instr_structure(void);
extern INSTR *find_instr(int);

