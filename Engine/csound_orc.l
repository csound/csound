%{

 /*
    csound_orc.l:

    Copyright (C) 2006
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
#include "csoundCore.h"
#define YYSTYPE ORCTOKEN*
#define YY_DECL int yylex (YYSTYPE *lvalp, CSOUND *csound)
#include "tok.h"
#include "csound_orcparse.h"
ORCTOKEN *make_string(CSOUND *, char *);
extern ORCTOKEN *lookup_token(CSOUND *csound, char *);
ORCTOKEN *new_token(CSOUND *csound, int type);
ORCTOKEN *make_int(CSOUND *, char *);
ORCTOKEN *make_num(CSOUND *, char *);
ORCTOKEN *make_token(CSOUND *, char *s);
ORCTOKEN *make_label(CSOUND *, char *s);
void comment(void);
void do_comment(void);
void do_include(CSOUND *, int);
extern int udoflag;
extern int namedInstrFlag;
int yyline = 0;
#define MAX_INCLUDE_DEPTH 10
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;
%}

STRCONST        \"(\\.|[^\"])*\"
LABEL           [[a-zA-Z_][a-zA-Z0-9_]*:
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
IDENTN          [a-zA-Z0-9_]+
INTGR           [0-9]+
NUMBER          [0-9]*(\.[0-9]*)?(e[-+]?[0-9]+)?|-?\.[0-9]*(e[-+]?[0-9]+)?
STCOM           "/"\*
WHITE           [ \t]+
INCLUDE		"#include"
%x incl

%%
"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */
"\n"            { yyline++; return S_NL; }
"//"            { comment(); return S_NL; }
";"             { comment(); return S_NL; }
{STCOM}         { do_comment(); }
"("             { return S_LB; }
")"             { return S_RB; }
"+"             { return S_PLUS; }
"-"             { return S_MINUS; }
"*"             { return S_TIMES; }
"/"             { return S_DIV; }
"?"             { return S_Q; }
":"             { return S_COL; }
","             { return S_COM; }
"!"             { return S_NOT; }
"!="            { return S_NEQ; }
"&&"            { return S_AND; }
"||"            { return S_OR; }
"<"             { return S_LT; }
"<="            { return S_LE; }
"=="            { return S_EQ; }
"="             { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = S_ASSIGN;
                  return S_ASSIGN; }
">"             { return S_GT; }
">="            { return S_GE; }

"if"            { return T_IF; }
"then"          { return T_THEN; }
"ithen"         { return T_ITHEN; }
"kthen"         { return T_KTHEN; }
"elseif"        { return T_ELSEIF; }
"else"          { return T_ELSE; }
"endif"         { return T_ENDIF; }

"goto"          { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_GOTO;
                  return T_GOTO; };
"igoto"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_IGOTO;
                  return T_IGOTO; };
"kgoto"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_KGOTO;
                  return T_KGOTO; };

"sr"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_SRATE;
                  return T_SRATE; }
"kr"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_KRATE;
                  return T_KRATE; }
"ksmps"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_KSMPS;
                  return T_KSMPS; }
"nchnls"        { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_NCHNLS;
                  return T_NCHNLS; }
"instr"         {
                namedInstrFlag = 1;
                return T_INSTR;
                }
"endin"         { return T_ENDIN; }
"opcode"        { return T_UDOSTART; }
"endop"         { *lvalp = new_token(csound, T_UDOEND); return T_UDOEND; }

{LABEL}         { *lvalp = make_label(csound, yytext); return T_LABEL; }

{STRCONST}      { *lvalp = make_string(csound, yytext); return (T_STRCONST); }

{IDENT}         { *lvalp = lookup_token(csound, yytext);
                  csound->Message(csound,"%d\n", (*lvalp)->type);
                  return (*lvalp)->type; }

{INTGR}         {
                    if(udoflag == 0) {
                        *lvalp = lookup_token(csound, yytext);
                    } else if(udoflag == 1) {
                        *lvalp = lookup_token(csound, yytext);
                        (*lvalp)->type = T_UDO_ARGS;
                    } else {
                        *lvalp = make_int(csound, yytext); return (T_INTGR);
                    }

                    csound->Message(csound,"%d\n", (*lvalp)->type);
                    return ((*lvalp)->type);
                }
{NUMBER}        { *lvalp = make_num(csound, yytext); return (T_NUMBER); }
{WHITE}         { }
{INCLUDE}       BEGIN(incl);
<incl>[ \t]*     /* eat the whitespace */
<incl>.         { do_include(csound, yytext[0]); /* got the include file name */
                  BEGIN(INITIAL);
                }
<<EOF>>         {
                  if ( --include_stack_ptr != 0 ) {
                    yyterminate();
                  }
                  else {
                    yy_delete_buffer( YY_CURRENT_BUFFER );
                    yy_switch_to_buffer(include_stack[include_stack_ptr] );
                  }
                }

.               { csound->Message(csound,
                                 Str("Line %d: Unknown character: '%s'\n"),
                                 yyline,yytext);
                }

%%
void comment(void)              /* Skip until nextline */
{
    char c;

    while ((c = input()) != '\n'); /* skip */
    yyline++;
}

void do_comment(void)              /* Skip until * and / chars */
{
    char c;
    for(;;) {
      while ((c=input()) != '*') if (c=='\n') yyline++; /* skip */
      if ((c=input())=='/') return;
      if (c=='\n') yyline++;
    }
}

void do_include(CSOUND *csound, int term)
{
    char buffer[100];
    int p=0;
    int c;
    while ((c=input())!=term) {
      buffer[p] = c;
      p++;
    }
    buffer[p] = '\0';
    while ((c=input())!='\n');
    if ( include_stack_ptr >= MAX_INCLUDE_DEPTH ) {
      csound->Message(csound, Str("Includes nested too deeply"));
      exit(1);
    }

    include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
    yyin = fopen( buffer, "r" );
    if ( ! yyin ) {
      csound->Message(csound, Str("Cannot open file \"%s\"\n"), buffer);
      exit(1);
    }
    yy_switch_to_buffer(yy_create_buffer( yyin, YY_BUF_SIZE ) );
}

ORCTOKEN *new_token(CSOUND *csound, int type)
{
    ORCTOKEN *ans = (ORCTOKEN*)mcalloc(csound, sizeof(ORCTOKEN));
    ans->type = type;
    return ans;
}

ORCTOKEN *make_token(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, T_STRCONST);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strcpy(ans->lexeme, s);
    return ans;
}

ORCTOKEN *make_label(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, T_LABEL);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len);
    strncpy(ans->lexeme, s, len - 1);
    return ans;
}

ORCTOKEN *make_string(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, T_STRCONST);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len-1);
    strncpy(ans->lexeme, s+1, len-2);
    ans->lexeme[len-2] = '\0';
    return ans;
}

ORCTOKEN *make_int(CSOUND *csound, char *s)
{
    int n = atoi(s);
    ORCTOKEN *ans = new_token(csound, T_INTGR);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->value = n;
    return ans;
}

ORCTOKEN *make_num(CSOUND *csound, char *s)
{
    double n = atof(s);
    ORCTOKEN *ans = new_token(csound, T_NUMBER);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->fvalue = n;
    return ans;
}

