%{

 /*
    csound_sco.lex:

    Copyright (C) 2013 March
    John ffitch

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
#include "score_param.h"
#include "csound_scoparse.h"

#define YYSTYPE MYFLT
#define YYLTYPE SCOTOKEN
#define YY_DECL int yylex (YYLTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
#include "corfile.h"
YYLTYPE *yylval_param;
YYLTYPE *yylloc_param;
static  SCOTOKEN *make_string(CSOUND *, char *);
static  SCOTOKEN *make_int(CSOUND *, int);
static  SCOTOKEN *make_num(CSOUND *, double);
extern  void *fopen_path(CSOUND *, FILE **, char *, char *, char *, int);

#define YY_EXTRA_TYPE  SCORE_PARM *
#define PARM    yyget_extra(yyscanner)

#define YY_USER_INIT

struct yyguts_t;
static SCOTOKEN *do_at(CSOUND *, int, struct yyguts_t*);
%}
%option reentrant
%option bison-bridge
%option bison-locations
%option prefix="csound_sco"
%option outfile="Engine/csound_scolex.c"
%option stdout

STRCONST        \"(\\.|[^\"])*\"
STRCONSTe       \"(\\.|[^\"])*$
INTGR           -?[0-9]+
NUMBER          -?[0-9]+\.?[0-9]*([eE][-+]?[0-9]+)?|\.[0-9]+([eE][-+]?[0-9]+)?|0[xX][0-9a-fA-F]+
WHITE           [ \t]+
OPTWHITE        [ \t]*
CONT            \\[ \t]*(;.*)?\n
LINE            "#line"
FILE            "#source"
EXIT            "#exit"
FNAME           [a-zA-Z0-9/:.+-_]+
NPX             "np^"${INTGR}
PPX             "pp^"${INTGR}
CNPX            "NP^"${INTGR}
CPPX            "PP^"${INTGR}

%x line
%x src

%%
"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */

{CONT}          { csound_scoset_lineno(1+csound_scoget_lineno(yyscanner),
                                       yyscanner);
 }
"\n"            { int n = csound_scoget_lineno(yyscanner)+1;
                  csound_scoset_lineno(n, yyscanner);
                  return NEWLINE; }
${NPX}          { return T_NP; }
${PPX}          { return T_PP; }
${CNPX}         { return T_CNP; }
${CPPX}         { return T_CPP; }
"a"             { return yytext[0];}
"b"             { return yytext[0];}
"e"             { return yytext[0];}
"f"             { return yytext[0];}
"i"             { return yytext[0];}
"m"             { return yytext[0];}
"n"             { return yytext[0];}
"q"             { return yytext[0];}
"r"             { return yytext[0];}
"s"             { return yytext[0];}
"t"             { return yytext[0];}
"v"             { return yytext[0];}
"w"             { return yytext[0];}
"x"             { return yytext[0];}
"y"             { return yytext[0];}
"z"             { return yytext[0];}
"("             { return '('; }
")"             { return ')'; }
"["             { return '['; }
"]"             { return ']'; }
"{"             { return '}'; }
"}"             { return '}'; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"%"             { return '%'; }
"\^"            { return '^'; }
"!"             { return '!'; }
">"             { return '>'; }
"<"             { return '<'; }
"|"             { return '|'; }
"&"             { return '&'; }
"#"             { return '#'; }
"~"             { return '~'; }
"."             { return '.'; }
"@@"{OPTWHITE}{INTGR}     { lvalp = do_at(csound, 1, yyg);
                            return INTEGER_TOKEN; }
"@"{OPTWHITE}{INTGR}      { lvalp = do_at(csound, 0, yyg);
                            return INTEGER_TOKEN; }

{STRCONST}      { lvalp = make_string(csound, yytext);
                  return (STRING_TOKEN); }

{STRCONSTe}     { lvalp = make_string(csound, yytext);
                  csound->Message(csound,
                          Str("unterminated string found on line %d >>%s<<\n"),
                          csound_scoget_lineno(yyscanner),
                          yytext);
                  return (STRING_TOKEN); }

{INTGR}         {
                  lvalp = make_int(csound, atoi(yytext)); return (INTEGER_TOKEN);
                }
{NUMBER}        { lvalp = make_num(csound, atof(yytext)); return (NUMBER_TOKEN); }

{WHITE}         { }

{LINE}          { BEGIN(line); }

<line>[ \t]*     /* eat the whitespace */
<line>{INTGR}   { csound_scoset_lineno(atoi(yytext), yyscanner);
  printf("set line to %d (%s)\n", csound_scoget_lineno(yyscanner), yytext); }
<line>"\n"      {BEGIN(INITIAL);}

{FILE}          { BEGIN(src); }

<src>[ \t]*     /* eat the whitespace */
<src>{FNAME}    { PARM->locn = atoi(yytext); }
<src>"\n"       { BEGIN(INITIAL); }


{EXIT}          { yyterminate(); }

<<EOF>>         {
                  yyterminate();
                }

.               { fprintf(stderr, "unknown character %c(%.2x)\n",
                          yytext[0], yytext[0]);
                }

%%

static SCOTOKEN *make_string(CSOUND *csound, char *s)
{
    SCOTOKEN *ans = (SCOTOKEN*)mcalloc(csound, sizeof(SCOTOKEN));
    ans->type = STRING_TOKEN;
    ans->strbuff = strdup(s);
    return ans;
}

static SCOTOKEN *make_int(CSOUND *csound, int i)
{
    SCOTOKEN *ans = (SCOTOKEN*)mcalloc(csound, sizeof(SCOTOKEN));
    ans->type = INTEGER_TOKEN;
    ans->ival = i;
    return ans;
}

static SCOTOKEN *make_num(CSOUND *csound, double f)
{
    SCOTOKEN *ans = (SCOTOKEN*)mcalloc(csound, sizeof(SCOTOKEN));
    ans->type = NUMBER_TOKEN;
    ans->fval = (MYFLT)f;
    return ans;
}

static SCOTOKEN *do_at(CSOUND *csound, int k, struct yyguts_t *yyg)
{
    SCOTOKEN *ans = (SCOTOKEN*)mcalloc(csound, sizeof(SCOTOKEN));
    int n, i = 1;
    char *s = yytext;
    while (*s=='@') s++;
    n = atoi(s);
    while (i<=n-k && i< 0x4000000) i <<= 1;
    ans->type = INTEGER_TOKEN;
    ans->ival = i;
    return ans;
}

char *csound_scoget_current_pointer(void *yyscanner)
{
    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return yyg->yy_c_buf_p;
}

int csound_scoget_locn(void *yyscanner)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->locn;
}
