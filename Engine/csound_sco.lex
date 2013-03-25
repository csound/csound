%{

 /*
    csound_sco.l:

    Copyright (C) 2013
    John ffitch, Steven Yi

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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
#define YYSTYPE TREE*
#define YYLTYPE SCOTOKEN*
#define YY_DECL int yylex (YYLTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
#include "csound_sco.h"
#include "corfile.h"
YYSTYPE *yylval_param;
YYLTYPE *yylloc_param;
SCOTOKEN *make_string(CSOUND *, char *);
extern SCOTOKEN *lookup_token(CSOUND *, char *, void *);
extern  void    *fopen_path(CSOUND *, FILE **, char *, char *, char *, int);
SCOTOKEN *new_token(CSOUND *csound, int type);
SCOTOKEN *make_int(CSOUND *, char *);
SCOTOKEN *make_num(CSOUND *, char *);
SCOTOKEN *make_token(CSOUND *, char *s);
SCOTOKEN *make_label(CSOUND *, char *s);
#define udoflag csound->parserUdoflag
#define namedInstrFlag csound->parserNamedInstrFlag
//extern int udoflag;
//extern int namedInstrFlag;
//YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
#include "parse_param.h"

#define YY_EXTRA_TYPE  PARSE_PARM *
#define PARM    yyget_extra(yyscanner)

/* #define YY_INPUT(buf,result,max_size)  {\ */
/*     result = get_next_char(buf, max_size, yyg); \ */
/*     if ( UNLIKELY( result <= 0  )) \ */
/*       result = YY_NULL; \ */
/*     } */

#define YY_USER_INIT 

struct yyguts_t;
SCOTOKEN *do_at(CSOUND *, int, struct yyguts_t*);
int get_next_char(char *, int, struct yyguts_t*);
%}
%option reentrant
%option bison-bridge
%option bison-locations
%option prefix="csound_sco"
%option outfile="Engine/csound_scolex.c"
%option stdout

STRCONST        \"(\\.|[^\"])*\"
STRCONSTe       \"(\\.|[^\"])*$
INTGR           [0-9]+
NUMBER          [0-9]+\.?[0-9]*(e[-+]?[0-9]+)?|\.[0-9]+(e[-+]?[0-9]+)?|0[xX][0-9a-fA-F]+
WHITE           [ \t]+
OPTWHITE        [ \t]*
CONT            \\[ \t]*(;.*)?\n
LINE            ^[ \t]*"#line"
FILE            ^[ \t]*"#source"
FNAME           [a-zA-Z0-9/:.+-_]+
NPX             "np^"${INTGR}
PPX             "pp^"${INTGR}

%x line
%x src
%x xstr

%%
"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */

{CONT}          { csound_scoset_lineno(1+csound_scoget_lineno(yyscanner),
                                       yyscanner);
 }
"\n"            { csound_scoset_lineno(1+csound_scoget_lineno(yyscanner),
                                       yyscanner);
                  return NEWLINE; }
"a"             { return yytext[0];}
"b"             { return yytext[0];}
"e"             { return yytext[0];}
"f"             { return yytext[0];}
"i"             { return yytext[0];}
"m"             { return yytext[0];}
"n"             { return yytext[0];}
"q"             { return yytext[0];}
"r"             { return yytext[0];}
"r"             { return yytext[0];}
"t"             { return yytext[0];}
"v"             { return yytext[0];}
"x"             { return yytext[0];}
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
"@@"{OPTWHITE}{INTGR}     { *lvalp = do_at(csound, 1, yyg); return INTEGER_TOKEN; }
"@"{OPTWHITE}{INTGR}      { *lvalp = do_at(csound, 0, yyg); return INTEGER_TOKEN; }

{STRCONST}      { *lvalp = make_string(csound, yytext); return (STRING_TOKEN); }

{STRCONSTe}     { *lvalp = make_string(csound, yytext);
                  csound->Message(csound,
                          Str("unterminated string found on line %d >>%s<<\n"),
                          csound_scoget_lineno(yyscanner),
                          yytext);
                  return (STRING_TOKEN); }

{INTGR}         {
                    if (udoflag == 0) {
                        *lvalp = make_string(csound, yytext);
                        (*lvalp)->type = UDO_ANS_TOKEN;
                    } else if (udoflag == 1) {
                        *lvalp = make_string(csound, yytext);
                        (*lvalp)->type = UDO_ARGS_TOKEN;
                    } else {
                        *lvalp = make_int(csound, yytext); return (INTEGER_TOKEN);
                    }

                    csound->Message(csound,"%d\n", (*lvalp)->type);
                    return ((*lvalp)->type);
                }
{NUMBER}        { *lvalp = make_num(csound, yytext); return (NUMBER_TOKEN); }
{WHITE}         { }

{LINE}          { BEGIN(line); }

<line>[ \t]*     /* eat the whitespace */
<line>{INTGR}   { csound_scoset_lineno(atoi(yytext), yyscanner); }
<line>"\n"      {BEGIN(INITIAL);}

{FILE}          { BEGIN(src); }

<src>[ \t]*     /* eat the whitespace */
<src>{FNAME}    { PARM->locn = atoi(yytext); }
<src>"\n"       { BEGIN(INITIAL); }


<<EOF>>         {
                  yyterminate();
                }

%%

static inline int isNameChar(int c, int pos)
{
    c = (int) ((unsigned char) c);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

SCOTOKEN *make_string(CSOUND *csound, char *s)
{
    SCOTOKEN *ans = new_token(csound, STRING_TOKEN);
    int len = strlen(s);
/* Keep the quote marks */
    /* ans->lexeme = (char*)mcalloc(csound, len-1); */
    /* strncpy(ans->lexeme, s+1, len-2); */
    /* ans->lexeme[len-2] = '\0';  */
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strcpy(ans->lexeme, s);
    return ans;
}

SCOTOKEN *do_at(CSOUND *csound, int k, struct yyguts_t *yyg)
{
    int n, i = 1;
    SCOTOKEN *ans;
    char buf[16];
    char *s = yytext;
    int len;
    while (*s=='@') s++;
    n = atoi(s);
    while (i<=n-k && i< 0x4000000) i <<= 1;
    ans = new_token(csound, INTEGER_TOKEN);
    sprintf(buf, "%d", i+k);
    len = strlen(buf);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, buf, len);
    ans->value = i;
    return ans;
}

SCOTOKEN *make_int(CSOUND *csound, char *s)
{
    int n = atoi(s);
    SCOTOKEN *ans = new_token(csound, INTEGER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->value = n;
    return ans;
}

SCOTOKEN *make_num(CSOUND *csound, char *s)
{
    double n = atof(s);
    SCOTOKEN *ans = new_token(csound, NUMBER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->fvalue = n;
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
