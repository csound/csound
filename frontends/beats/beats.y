%{
/*
    beats.y:

    Copyright (C) 2009,2010 John ffitch,

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beats.h"

    int last_note;
    int last_integer;
    double last_float;
    double last_duration;
    int ampl = 1;
    int last_ampl = -1;
    double length = 0;
    double onset0 = -1;
    double onset1 = -1;
    double last_length = -1;
    double last_onset0 = -1;
    double last_onset1 = -1;
    double base_time = 0;
    double last_base_time = 0;
    double bpm;
    int pnum;

    int permeasure;
    int instrument;
    double pitch = -1.0;
    double last_pitch = -1.0;
#define YYDEBUG 1
    extern double pt[12];
    double int2pow(int);
    extern int yyline;
    extern FILE* myout;
    extern int debug;
    extern int yylex(void);
    extern void yyerror (char *);
    static void extend_instruments(void);
%}
%token  S_NL
%token  S_EQ
%token  S_PLS
%token  T_BEATS
%token  T_PERMEASURE
%token  T_NOTE
%token  T_DURATION
%token  T_INTEGER
%token  T_INSTR
%token  T_FLOAT
%token  T_DYN
%token  T_m
%token  T_b
%token  T_QUIT
%token  T_BAR
%token  T_PARA

%%

goal	: goal statement {}
        | statement      {}
        ;

statement : S_NL                                { }
        | T_QUIT                                { fprintf(myout ,"e\n"); return 0; }
        | T_BEATS S_EQ T_INTEGER S_NL		{ base_time = last_base_time; bpm = (double)last_integer; 
                                                  fprintf(myout, ";;;setting bpm=%f\n", bpm);}
        | T_PERMEASURE S_EQ T_INTEGER S_NL 	{ permeasure = last_integer; fprintf(myout,";;;setting permeasure=%d\n", permeasure);}
        | T_BAR S_NL                            { onset0 += permeasure; }
        | T_BAR T_INTEGER S_NL                  { onset0 = permeasure*(last_integer-1); }
        | T_INSTR T_INTEGER attributes S_NL {
            int i;
            if (debug) {
              fprintf(stderr, "instr=%d onset0,onset1,last0,last1=%f,%f,%f,%f\n",
                      instrument, onset0, onset1, last_onset0, last_onset1);
              fprintf(stderr, "length,last=%f,%f pitch,last=%f,%f amp,last=%d,%d\n",
                      length,last_length, pitch, last_pitch, ampl, last_ampl);
            }
            //print_instr_structure();
            if (instrument>maxinstr) extend_instruments();
            if (onset0<0) onset0 = last_onset0;
            if (onset1<0) onset1 = last_onset1;
            if (length==0) length = last_length;
            if (pitch<0 && pitch != -99) pitch = last_pitch;
            if (ampl>0) ampl = last_ampl;
            if (pitch>=0) {
              fprintf(myout,"i%d %f %f %f %d",
                      instrument, base_time+60.0*(onset0+onset1)/bpm,
                      60.0*length/bpm, pitch, ampl);
              for (i=6; i<=instr[instrument].largest; i++)
                fprintf(myout, " %f", instr[instrument].p[i]);
            }
            else fprintf(myout, ";;rest at %f for %f",
                         base_time+60.0*(onset0+onset1)/bpm,60.0*length/bpm);
            fprintf(myout, "\n");
            last_base_time = base_time+60.0*(onset0+onset1+length)/bpm;
            last_onset0 = onset0;
            last_onset1 = onset1 + length;
            last_length = length;
            last_pitch = pitch;
            last_ampl = ampl;
            onset0 = -1;
            onset1 = -1;
            length = 0;
            pitch = -1;
            ampl = +1;
          }
        ;

attributes: attribute attributes
	| attribute
        | 
        ;

attribute: T_NOTE T_INTEGER { if (last_note>=-2) {
                                if (last_note<0)
                                  last_integer -= 1, last_note += 12;
                                pitch = int2pow(last_integer + 1)*pt[last_note];
                              }
                              else pitch = -99; }
        | T_NOTE           { if (last_note>=-2)
                                fprintf(stderr,
                                        "Octave missing from note on line %d\n",
                                        yyline);
                             /* rest */ pitch = -99;}
        | T_DURATION       { length += last_duration; }
        | T_m T_INTEGER    { onset0 = permeasure*(last_integer-1); }
        | T_m              { fprintf(stderr, 
                                     "Measure missing from note on line %d\n", 
                                     yyline);
                             onset0 += permeasure; };
        | T_b T_FLOAT      { if (onset1<0) onset1 = (last_float-1.0); };
        | T_b T_INTEGER    { if (onset1<0) onset1 = (last_integer-1); };
        | T_b              { fprintf(stderr, "Beat missing from note on line %d\n",
                                     yyline);
                             onset1 += 1; };
        | T_DYN            { ampl = last_integer; }
//        | S_PLS            { onset0 = last_onset1; onset1 += last_length; }
        | T_PARA S_EQ T_FLOAT { 
            if (instrument>maxinstr) extend_instruments();
            //print_instr_structure();
            if (pnum<6)
              fprintf(stderr, "p field %d ignored as less than 6\n", pnum);
            else {
              if (pnum>instr[instrument].largest) {
                int i;
                instr[instrument].p =
                  (double*)realloc(instr[instrument].p, sizeof(double)*(pnum+1));
                for (i=instr[instrument].largest+1; i<=pnum; i++)
                  instr[instrument].p[i]=0.0;
                instr[instrument].largest = pnum;
              }
              instr[instrument].p[pnum] = last_float;
            }
            //print_instr_structure();
          }
        | T_PARA S_EQ T_INTEGER { 
            if (instrument>maxinstr) extend_instruments();
            //print_instr_structure();
            if (pnum<6)
              fprintf(stderr, "p field %d ignored as less than 6\n", pnum);
            else {
              if (pnum>instr[instrument].largest) {
                int i;
                instr[instrument].p =
                  (double*)realloc(instr[instrument].p, sizeof(double)*(pnum+1));
                for (i=instr[instrument].largest+1; i<=pnum; i++)
                  instr[instrument].p[i]=0.0;
                instr[instrument].largest = pnum;
              }
              instr[instrument].p[pnum] = (double)last_integer;
            }
            //print_instr_structure();
          }
        ;

%%

          /* Faster than calling pow */
double int2pow(int n)
{
    double ans = 1.0;
    double xx = 2.0;
    while (n!=0) {
      if (n&1) ans = ans * xx;
      n >>= 1;
      xx = xx*xx;
    }
    return ans;
}

static void extend_instruments(void)
{
    int i;
    int tmp = maxinstr+1;
    instr = (INSTR*)realloc(instr, tmp*sizeof(INSTR));
    maxinstr = instrument;
    for (i=tmp; i<=maxinstr; i++) {
      instr[i].p = (double*) calloc(6, sizeof(double));
      instr[i].largest = 5;
      instr[i].n = i;
    }
}
