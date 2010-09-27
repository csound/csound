%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "beats.tab.h"
void comment(void);
void comment1(void);
int yyline = 0;
 extern int last_note;
 extern int last_integer;
 extern double last_float;
 extern double last_duration;
 extern double bpm;
 int next_instr=0;
 extern int instrument;
%}

INTGR           [0-9]+
FLOAT           [[0-9]+\.([0-9]*)?
WHITE           [ \t+]
BEATS           [Bb][Ee][Aa][Tt][Ss]
BPS             [Bb][Pp][Mm]
PERMEASURE      [Pp][Ee][Rr][Mm][Ee][Aa][Ss][Uu][Rr][Ee]
QUIT            ([Qq][Uu][Ii][Tt])|([Ee][Nn][Dd])
BAR             [Bb][Aa][Rr]
%%

{WHITE}         {}
"\n"            { yyline++; return S_NL; }
"//"            { comment(); return S_NL; }
";"             { comment(); return S_NL; }
"="             { return S_EQ; }
"/*"            { comment1(); }
{BEATS}         { return T_BEATS;}
{BPS}           { return T_BEATS;}
{PERMEASURE}    { return T_PERMEASURE;}
{QUIT}          { return T_QUIT; }
{BAR}           { return T_BAR; }

"Ab"            { last_note = 8; return T_NOTE;}
"Abb"           { last_note = 7; return T_NOTE;}
"A#"            { last_note = 10; return T_NOTE;}
"Ax"            { last_note = 11; return T_NOTE;}
"A"             { last_note = 9; return T_NOTE;}
"Bb"            { last_note = 10; return T_NOTE;}
"Bbb"           { last_note = 9; return T_NOTE;}
"B#"            { last_note = 12; return T_NOTE;}
"Bx"            { last_note = 13; return T_NOTE;}
"B"             { last_note = 11; return T_NOTE;}
"Cb"            { last_note = -1; return T_NOTE;}
"Cbb"           { last_note = -2; return T_NOTE;}
"C#"            { last_note = 1; return T_NOTE;}
"Cx"            { last_note = 2; return T_NOTE;}
"C"             { last_note = 0; return T_NOTE;}
"Db"            { last_note = 1; return T_NOTE;}
"Dbb"           { last_note = 0; return T_NOTE;}
"D#"            { last_note = 3; return T_NOTE;}
"Dx"            { last_note = 4; return T_NOTE;}
"D"             { last_note = 2; return T_NOTE;}
"Eb"            { last_note = 3; return T_NOTE;}
"Ebb"           { last_note = 2; return T_NOTE;}
"E#"            { last_note = 5; return T_NOTE;}
"Ex"            { last_note = 6; return T_NOTE;}
"E"             { last_note = 4; return T_NOTE;}
"Fb"            { last_note = 4; return T_NOTE;}
"Fbb"           { last_note = 3; return T_NOTE;}
"F#"            { last_note = 6; return T_NOTE;}
"Fx"            { last_note = 7; return T_NOTE;}
"F"             { last_note = 5; return T_NOTE;}
"Gb"            { last_note = 6; return T_NOTE;}
"Gbb"           { last_note = 5; return T_NOTE;}
"G#"            { last_note = 8; return T_NOTE;}
"Gx"            { last_note = 9; return T_NOTE;}
"G"             { last_note = 7; return T_NOTE;}
"R"             { last_note = -3; return T_NOTE;}
"Z"             { last_note = -3; return T_NOTE;}

"ed"            { last_duration = 0.75; return T_DURATION;}
"et"		{ last_duration = (1.0 / 3.0); return T_DURATION;}
"e"		{ last_duration = 0.5; return T_DURATION;}
"hd"		{ last_duration = 3.0; return T_DURATION;}
"ht"		{ last_duration = (4.0/3.0); return T_DURATION;}
"h"		{ last_duration = 2.0; return T_DURATION;}
"qd"		{ last_duration = 1.5; return T_DURATION;}
"qt"		{ last_duration = (2.0/3.0); return T_DURATION;}
"q"		{ last_duration = 1.0; return T_DURATION;}
"sd"		{ last_duration = 0.375; return T_DURATION;}
"st"		{ last_duration = (1.0 / 6.0); return T_DURATION;}
"s"		{ last_duration = 0.25; return T_DURATION;}
"th"		{ last_duration = 0.125; return T_DURATION;}
"w"		{ last_duration = 4.0; return T_DURATION;}

"fff"           { last_integer =  0; return T_DYN; }
"ff"            { last_integer = -1; return T_DYN; }
"f"             { last_integer = -2; return T_DYN; }
"mf"            { last_integer = -3; return T_DYN; }
"mp"            { last_integer = -4; return T_DYN; }
"p"             { last_integer = -5; return T_DYN; }
"pp"            { last_integer = -6; return T_DYN; }
"ppp"           { last_integer = -7; return T_DYN; }

"m"             { return T_m;}
"b"             { return T_b;}

"i"|"I"         { next_instr = 1; return T_INSTR;}

{INTGR}         { last_integer = atoi(yytext); if (next_instr) { instrument = last_integer; next_instr = 0;} return T_INTEGER;}
{FLOAT}         { last_float = atof(yytext); return T_FLOAT;}
.               { fprintf(stderr, "Unknown character %c(%.2x)\n",
                          yytext[0], yytext[0]);}
%%


void comment(void)
{
    int c;

    while ((c = input()) != '\n')
      if (c==EOF) return; /* skip */
    yyline++;
}

void comment1(void)
{
    int c;

    while ((c = input()) != '*') {
    top:
      if (c=='\n') yyline++; /* skip */
      else if (c==EOF) exit(1);
    }
    if ((c = input())!='/') {
      if (c=='\n') yyline++;
      goto top;
    }
    return;
}

int yywrap(void)
{
    extern FILE *myout;
    fprintf(myout, "e\n");
    return 1;
}

void yyerror (char *s) {fprintf (stderr, "%s\n", s);}

