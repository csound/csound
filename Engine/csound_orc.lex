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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
// to shut up the lexer writing to stdout
#define ECHO if(csound->oparms->odebug) { csoundErrorMsg(csound, "%s", "--lexer echo:"); \
             fwrite(yytext, (size_t) yyleng, 1, stderr); \
             csoundErrorMsg(csound, "%s", "--\n");}
#define YYSTYPE TREE*
#define YYLTYPE ORCTOKEN*
#define YY_DECL int yylex (YYLTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
#include "csound_orc.h"
#include "corfile.h"
#include "filesys.h"
YYSTYPE *yylval_param;
YYLTYPE *yylloc_param;
 ORCTOKEN *make_string(CSOUND *, char *, void*);
 ORCTOKEN *lookup_token(CSOUND *, char *, void *);
 ORCTOKEN *new_token(CSOUND *csound, int32_t type, void *yyscanner);
 ORCTOKEN *make_int(CSOUND *, char *, void*);
 ORCTOKEN *make_num(CSOUND *, char *,  void*);
 ORCTOKEN *make_token(CSOUND *, char *s,  void*);
 ORCTOKEN *make_label(CSOUND *, char *s,  void*);

#include "parse_param.h"

#define YY_EXTRA_TYPE  PARSE_PARM *
#define PARM    yyget_extra(yyscanner)

#define YY_USER_ACTION PARM->first_column = yycolumn; PARM->last_column = yycolumn + yyleng - 1; yycolumn += yyleng;
#define YY_USER_INIT

struct yyguts_t;
 ORCTOKEN *do_at(CSOUND *, int32_t, void*, char*);
int get_next_char(char *, int32_t, void*);
%}
%option reentrant
%option bison-bridge
%option bison-locations
%option prefix="csound_orc"
%option outfile="Engine/csound_orclex.c"
%option stdout
%option 8bit
   /* to avoid unused function errors */
%option nounput

IDENT           [a-zA-Z_][a-zA-Z0-9_]*(@global)?
IDENTB          [a-zA-Z_][a-zA-Z0-9_]*\([ \t]*\n?
TYPED_IDENTIFIER  [a-zA-Z_][a-zA-Z0-9_]*(@global)?:[a-zA-Z_][a-zA-Z0-9_]*(\[\])?
TYPED_IDENTIFIERB [a-zA-Z_][a-zA-Z0-9_]*:[a-zA-Z_][a-zA-Z0-9_]*\[?\]?\([ \t]*\n?
XIDENT          0|[aijkftKOJVPopS\[\]]+
INTGR           [0-9]+
NUMBER          [0-9]+\.?[0-9]*([eE][-+]?[0-9]+)?|\.[0-9]+([eE][-+]?[0-9]+)?|0[xX][0-9a-fA-F]+
WHITE           [ \t]+
OPTWHITE        [ \t]*
CONT            \\[ \t]*(;.*)?\n
XSTR            "{{"
EXSTR           "}}"
LINE            ^[ \t]*"#line"
SLINE           "#sline "
FILE            ^[ \t]*"#source"
FNAME           [a-zA-Z0-9/:.+-_]+
LPAREN          "("
RPAREN          ")"
SYMBOL          [\[\]+\-*/%\^\?:.,!]
RSTR            "R{"
ERSTR           "}R"


%x line
%x sline
%x src
%x xstr
%x rstr
%x declare
%x udodef
%x udoarg
%x forloop

%%
<*>"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */

{CONT}          {
                  yycolumn = 1;
                  csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                       yyscanner);
                }
"->"            { return S_ELIPSIS; }
"..."           { return S_ELIPSIS2; }

"!="            { return S_NEQ; }
"&&"            { return S_AND; }
"||"            { return S_OR; }
"<<"            { return S_BITSHIFT_LEFT; }
">>"            { return S_BITSHIFT_RIGHT; }
"<"             { return S_LT; }
"<="            { return S_LE; }
"=="            { return S_EQ; }
"+="            { return S_ADDIN; }
"-="            { return S_SUBIN; }
"*="            { return S_MULIN; }
"/="            { return S_DIVIN; }
"="             { *lvalp = make_token(csound, "=", yyscanner);
                  (*lvalp)->type = '=';
                  return '='; }
">"             { return S_GT; }
">="            { return S_GE; }
"|"             { return '|'; }
"&"             { return '&'; }
"#"             { return '#'; }
"¬"             { return '~'; } /* \xC2?\xAC */
"~"             { return '~'; }

\xC2?\xAC{OPTWHITE} { return '~'; } /* BACKWARDS COMPATABILITY */

"@@"{OPTWHITE}{INTGR}     { *lvalp = do_at(csound, 1, yyscanner, yytext); return INTEGER_TOKEN; }
"@"{OPTWHITE}{INTGR}      { *lvalp = do_at(csound, 0, yyscanner, yytext); return INTEGER_TOKEN; }
"@i"            { return T_MAPI; }
"@k"            { return T_MAPK; }
"false"         { return FALSE_TOKEN; }
"true"          { return TRUE_TOKEN; }
"falsek"        { return FALSEK_TOKEN; }
"truek"         { return TRUEK_TOKEN; }
"if"\([ \t]*    { yyless(2);
                  *lvalp = make_token(csound, "if", yyscanner);
                  (*lvalp)->type = IF_TOKEN;
                  return IF_TOKEN; }
"if"            { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = IF_TOKEN;
                  return IF_TOKEN; }
"then"          { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = THEN_TOKEN;
                  return THEN_TOKEN; }
"ithen"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = ITHEN_TOKEN;
                  return ITHEN_TOKEN; }
"kthen"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = KTHEN_TOKEN;
                  return KTHEN_TOKEN; }
"elseif"\([ \t]* { yyless(6);
                  *lvalp = make_token(csound, "elseif", yyscanner);
                  (*lvalp)->type = ELSEIF_TOKEN;
                  return ELSEIF_TOKEN; }
"elseif"        { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = ELSEIF_TOKEN;
                  return ELSEIF_TOKEN; }
"else"          { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = ELSE_TOKEN;
                  return ELSE_TOKEN; }
"endif"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = ENDIF_TOKEN;
                  return ENDIF_TOKEN; }
"fi"            { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = ENDIF_TOKEN;
                  return ENDIF_TOKEN; }
"until"\([ \t]* { yyless(5);
                  *lvalp = make_token(csound, "until", yyscanner);
                  (*lvalp)->type = UNTIL_TOKEN;
                  return UNTIL_TOKEN; }
"until"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = UNTIL_TOKEN;
                  return UNTIL_TOKEN; }
"while"\([ \t]* { yyless(5);
                  *lvalp = make_token(csound, "while", yyscanner);
                  (*lvalp)->type = WHILE_TOKEN;
                  return WHILE_TOKEN; }
"while"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = WHILE_TOKEN;
                  return WHILE_TOKEN; }
"do"            { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = DO_TOKEN;
                  return DO_TOKEN; }
"od"            { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = OD_TOKEN;
                  return OD_TOKEN; }
"enduntil"      { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = OD_TOKEN;
                  return OD_TOKEN; }
"break"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = BREAK_TOKEN;
                  return BREAK_TOKEN; }
"continue"      { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = CONTINUE_TOKEN;
                  return CONTINUE_TOKEN; }
"switch"        { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = SWITCH_TOKEN;
                  return SWITCH_TOKEN; }
"case"          { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = CASE_TOKEN;
                  return CASE_TOKEN; }
"default"       { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = DEFAULT_TOKEN;
                  return DEFAULT_TOKEN; }
"endsw"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = ENDSW_TOKEN;
                  return ENDSW_TOKEN; }

"goto"          { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = GOTO_TOKEN;
                  return GOTO_TOKEN; };
"igoto"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = IGOTO_TOKEN;
                  return IGOTO_TOKEN; };
"kgoto"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = KGOTO_TOKEN;
                  return KGOTO_TOKEN; };
"struct"        {
                  return STRUCT_TOKEN;
                }
"instr"         {
                  return INSTR_TOKEN;
                }
"endin"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = ENDIN_TOKEN;
                  return ENDIN_TOKEN; }
"void"          { return VOID_TOKEN; }


"for"           {  *lvalp = make_token(csound, yytext, yyscanner);
                   (*lvalp)->type = FOR_TOKEN;
                   BEGIN(forloop);
                   return FOR_TOKEN; }

<forloop>{
   ","            { return ','; }

  [ \t]*          /* eat the whitespace */
  {IDENT}/[ \t]*   { char *pp = yytext;
                    while (*pp==' ' || *pp=='\t') pp++;
                    *lvalp = make_token(csound, pp, yyscanner);
                    if (strcmp(pp, "in") == 0) {
                      BEGIN(INITIAL);
                      return IN_TOKEN;
                    } else {
                      return T_IDENT;
                    }
                  }
  
  [ \t]*          /* eat the whitespace */
  {TYPED_IDENTIFIER}/[ \t]*   { char *pp = yytext;
                    while (*pp==' ' || *pp=='\t') pp++;
                    *lvalp = make_token(csound, pp, yyscanner);
                    if (strcmp(pp, "in") == 0) {
                      BEGIN(INITIAL);
                      return IN_TOKEN;
                    } else {
                      return T_TYPED_IDENT;
                    }
                  }
}

"\{\{"          {
                  PARM->xstrbuff = (char *)malloc(128);
                  PARM->xstrptr = 0; PARM->xstrmax = 128;
                  PARM->xstrbuff[PARM->xstrptr++] = '"';
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                  PARM->xsubstr = 0;
                  BEGIN(xstr);
                }
<xstr>{
  "\{\{" {
             PARM->xsubstr += 1; // substr start
             if (PARM->xstrptr+3>=PARM->xstrmax) {
                PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
               csound->DebugMsg(csound,"Extending xstr buffer\n");
             }
             PARM->xstrbuff[PARM->xstrptr++] = '{';
             PARM->xstrbuff[PARM->xstrptr++] = '{';
             PARM->xstrbuff[PARM->xstrptr] = '\0';
         }

  "}}"   {
    if(PARM->xsubstr) {
            PARM->xsubstr -= 1; // substr end
           if (PARM->xstrptr+3>=PARM->xstrmax) {
                PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
               csound->DebugMsg(csound,"Extending xstr buffer\n");
           }
           PARM->xstrbuff[PARM->xstrptr++] = '}';
           PARM->xstrbuff[PARM->xstrptr++] = '}';
           PARM->xstrbuff[PARM->xstrptr] = '\0';
    } else {
           BEGIN(INITIAL);
           PARM->xstrbuff[PARM->xstrptr++] = '"';
           PARM->xstrbuff[PARM->xstrptr] = '\0';
           /* printf("xstrbuff:>>%s<<\n", PARM->xstrbuff); */
           *lvalp = make_string(csound, PARM->xstrbuff, yyscanner);
            free(PARM->xstrbuff);
            return STRING_TOKEN;
          }
  }

  "\n"  { /* The next two should be one case but I cannot get that to work */
           yycolumn = 1;
           if (PARM->xstrptr+2>=PARM->xstrmax) {
               PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
               csound->DebugMsg(csound,"Extending xstr buffer\n");
           }
            //csound->DebugMsg(csound,"Adding newline (%.2x)\n", yytext[0]);
               PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
               PARM->xstrbuff[PARM->xstrptr] = '\0';
           }

  .        {
            if (PARM->xstrptr+2>=PARM->xstrmax) {
                PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                 csound->DebugMsg(csound,"Extending xstr buffer\n");
               }
              //csound->DebugMsg(csound,"Adding (%.2x)\n", yytext[0]);
              PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
              PARM->xstrbuff[PARM->xstrptr] = '\0';
            }
}

"R{"   {
                  PARM->xstrbuff = (char *)malloc(128);
                  PARM->xstrptr = 0; PARM->xstrmax = 128;
                  PARM->xstrbuff[PARM->xstrptr++] = '"';
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                  PARM->xsubstr = 0;
                  BEGIN(rstr);
                }

<rstr>{
  "R{" {
             PARM->xsubstr += 1; // substr start
             if (PARM->xstrptr+4>=PARM->xstrmax) {
                PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
               csound->DebugMsg(csound,"Extending rstr buffer\n");
             }
             PARM->xstrbuff[PARM->xstrptr++] = 'R';
             PARM->xstrbuff[PARM->xstrptr++] = '{';
             PARM->xstrbuff[PARM->xstrptr] = '\0';
         }

  "}R"   {
    if(PARM->xsubstr) {
            PARM->xsubstr -= 1; // substr end
           if (PARM->xstrptr+4>=PARM->xstrmax) {
                PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
               csound->DebugMsg(csound,"Extending rstr buffer\n");
           }
             PARM->xstrbuff[PARM->xstrptr++] = '}';
             PARM->xstrbuff[PARM->xstrptr++] = 'R';
           PARM->xstrbuff[PARM->xstrptr] = '\0';
    } else {
           BEGIN(INITIAL);
           PARM->xstrbuff[PARM->xstrptr++] = '"';
           PARM->xstrbuff[PARM->xstrptr] = '\0';
           *lvalp = make_string(csound, PARM->xstrbuff, yyscanner);
            free(PARM->xstrbuff);
            return STRING_TOKEN;
          }
  }

  "\n"  { /* The next two should be one case but I cannot get that to work */
           yycolumn = 1;
           if (PARM->xstrptr+2>=PARM->xstrmax) {
               PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
               csound->DebugMsg(csound,"Extending xstr buffer\n");
           }
            //csound->DebugMsg(csound,"Adding newline (%.2x)\n", yytext[0]);
               PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
               PARM->xstrbuff[PARM->xstrptr] = '\0';
           }

  .        {
            if (PARM->xstrptr+2>=PARM->xstrmax) {
                PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                 csound->DebugMsg(csound,"Extending xstr buffer\n");
               }
              //csound->DebugMsg(csound,"Adding (%.2x)\n", yytext[0]);
              PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
              PARM->xstrbuff[PARM->xstrptr] = '\0';
            }
 }


^[ \t]*{IDENT}:/[ \t\n]  { char *pp = yytext;
                  while (*pp==' ' || *pp=='\t') pp++;
                  *lvalp = make_label(csound, pp, yyscanner); return LABEL_TOKEN;
                }

"declare"       { BEGIN(declare);
                  return DECLARE_TOKEN;
                }

"opcode"        { BEGIN(udodef);
                  return UDOSTART_DEFINITION;
                }
"endop"         {
  *lvalp = new_token(csound, UDOEND_TOKEN, yyscanner); return UDOEND_TOKEN;
                }


<udodef>{


  {IDENT}/[ \t]*\( { BEGIN(INITIAL);
                    *lvalp = lookup_token(csound, yytext, yyscanner);
                    /*csound->Message(csound, ">>>> NEW UDO DEF <<<<<<<\n");*/
                    /*csound->Message(csound,"%s -> %d\n",*/
                    /*                   yytext, (*lvalp)->type); */
                    return (*lvalp)->type; }


  {IDENT} { BEGIN(udoarg);
                    /*csound->Message(csound, ">>>> OLD UDO DEF <<<<<<<\n");*/
                    *lvalp = lookup_token(csound, yytext, yyscanner);
                    /* csound->Message(csound,"%s -> %d\n",
                                       yytext, (*lvalp)->type); */
                    return (*lvalp)->type; }


}

<declare>{
  {IDENT} { BEGIN(INITIAL);
            *lvalp = lookup_token(csound, yytext, yyscanner);
            return (*lvalp)->type; }
}

<udoarg>{
  ","     { return ','; }
 {XIDENT} { BEGIN(udoarg);
                  *lvalp = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, (*lvalp)->type); */
                  (*lvalp)->type = UDO_IDENT;
                  return (*lvalp)->type; }
  "\n"     { BEGIN(INITIAL);
             yycolumn = 1;
             csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                   yyscanner);
             return NEWLINE; }
  {IDENT} {
    csound->Message(csound, "unsupported UDO arg type: %s", yytext);
    return ERROR_TOKEN;
  }
}


\"              { /* String decode by c-code not rexp */
                  int cnt = 80;
                  char *buff = malloc(cnt);
                  int n = 1;
                  int ch;
                  buff[0] = '"';
                  for (;;) {
                    ch = input(yyscanner);
                    if (ch=='"') {
                      if (n>=cnt-2) buff = realloc(buff, cnt+=20);
                      buff[n++] = ch;
                      buff[n] = '\0';
                      break;
                    }
                    else if (ch=='\\') {
                      ch = input(yyscanner);
                      switch (ch) {
                      case 'a': case 'b': case 'n': case 'r':
                      case 't': case '\\':
                        if (n>=cnt-2) buff = realloc(buff, cnt+=20);
                        buff[n++] = '\\'; buff[n++]= ch;
                        break;
                        /* VL - 21-1-17 fix for octals in strings */
                      case '0':case '1':case '2':case '3':
                      case '4':case '5':case '6':case '7':
                        if (n>=cnt-2) buff = realloc(buff, cnt+=20);
                        buff[n++] = '\\'; buff[n++]= ch;
                        break;
                      default:
                        if (n>=cnt-2) buff = realloc(buff, cnt+=20);
                        buff[n++] = ch;
                        break;
                      }
                    }
                    else if (UNLIKELY(ch=='\n')) {
                      if (UNLIKELY(n>=cnt-2)) buff = realloc(buff, cnt+=20);
                      buff[n++] = '"';
                      buff[n] = '\0';
                      csound->Message(csound,
                              Str("unterminated string found on line %d >>%s<<\n"),
                                      csound_orcget_lineno(yyscanner), buff);
                      break;
                    }
                    else {
                      if (UNLIKELY(n>=cnt-2)) buff = realloc(buff, cnt+=20);
                      buff[n++] = ch;
                    }
                  }
                  *lvalp = make_string(csound, buff, yyscanner);
                  free(buff);
                  return (STRING_TOKEN);
                }

"0dbfs"         { *lvalp = make_token(csound, yytext, yyscanner);
                  (*lvalp)->type = T_IDENT;
                  /* csound->Message(csound,"%d\n", (*lvalp)->type); */
                  return T_IDENT; }
{IDENT}         { *lvalp = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, (*lvalp)->type); */
                  return (*lvalp)->type; }
{IDENTB}        { PARM->paren_depth++;
                  if (UNLIKELY(strchr(yytext, '\n'))) {
                       yycolumn = 1;
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                  }
                  *strrchr(yytext, '(') = '\0';
                  *lvalp = lookup_token(csound, yytext, yyscanner);
                  return (*lvalp)->type+1; }
{TYPED_IDENTIFIER} { *lvalp = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, (*lvalp)->type); */
                  return (*lvalp)->type; }
{TYPED_IDENTIFIERB} { PARM->paren_depth++;
                      if (UNLIKELY(strchr(yytext, '\n'))) {
                           yycolumn = 1;
                           csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                                yyscanner);
                      }
                      *strrchr(yytext, '(') = '\0';
                      *lvalp = lookup_token(csound, yytext, yyscanner);
                      return (*lvalp)->type+1; }
{INTGR}         {
                    *lvalp = make_int(csound, yytext, yyscanner); return (INTEGER_TOKEN);
                    /*csound->Message(csound,"%d\n", (*lvalp)->type);*/
                    return ((*lvalp)->type);
                }
{LPAREN}     { PARM->paren_depth++;
               return *yytext; }

{RPAREN}     { if (PARM->paren_depth > 0)
                 PARM->paren_depth--;
               return *yytext; }

{SYMBOL}     { return *yytext;}

{NUMBER}        { *lvalp = make_num(csound, yytext, yyscanner); return (NUMBER_TOKEN); }
{WHITE}         { }

{SLINE}         { BEGIN(sline); }
<sline>{INTGR}   { csound_orcset_lineno(atoi(yytext), yyscanner);
                  yycolumn = 0; /* reset for new source line;
                                    0 accounts for trailing space in #sline format */ }
<sline>[ \t]*   { BEGIN(INITIAL);}
{LINE}          { BEGIN(line); }

<line>{
  [ \t]*     /* eat the whitespace */
  {INTGR}   { csound_orcset_lineno(atoi(yytext), yyscanner); }
  "\n"      {BEGIN(INITIAL); yycolumn = 1;}
}

{FILE}          { BEGIN(src); }

<src>{
  [ \t]*     /* eat the whitespace */
  {FNAME}    { PARM->locn = atoll(yytext); }
  "\n"       { BEGIN(INITIAL); yycolumn = 1;}
}

.               {
                  { int c = yytext[0]&0xff;
                    printf("Error: character %c(%.2x)\n", c, c);
                  }
                  return ERROR_TOKEN;
                }

<<EOF>>         {
                  yyterminate();
                }

<INITIAL>"\n" { yycolumn = 1;
                 csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                      yyscanner);
                 if (PARM->paren_depth == 0)
                   return NEWLINE; }

%%

ORCTOKEN *lookup_token(CSOUND *csound, char *s, void *yyscanner)
{
    int32_t type = T_IDENT;
    ORCTOKEN *ans;

    if(UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS))
      csound->Message(csound, "Looking up token for: %s\n", s);
    ans = new_token(csound, T_IDENT, yyscanner);
    if (strchr(s, ':') != NULL) {
        char* th;
        char* baseName = strtok_r(s, ":", &th);
        char* annotation = strtok_r(NULL, ":", &th);
        ans->lexeme = csoundStrdup(csound, baseName);
        ans->optype = csoundStrdup(csound, annotation);
        type = T_TYPED_IDENT;
    } else {
        ans->lexeme = csoundStrdup(csound, s);
    }
    ans->type = type;
    return ans;
}


ORCTOKEN *new_token(CSOUND *csound, int32_t type, void *yyscanner)
{
    ORCTOKEN *ans = (ORCTOKEN*)csound->Calloc(csound, sizeof(ORCTOKEN));
    ans->type = type;
    if(yyscanner) {
     ans->first_column = PARM->first_column;
     ans->last_column = PARM->last_column;
    }
    return ans;
}

ORCTOKEN *make_token(CSOUND *csound, char *s, void *yyscanner)
{
  ORCTOKEN *ans = new_token(csound, STRING_TOKEN, yyscanner);
    ans->lexeme = csoundStrdup(csound, s);
    return ans;
}

ORCTOKEN *make_label(CSOUND *csound, char *s, void *yyscanner)
{
    ORCTOKEN *ans = new_token(csound, LABEL_TOKEN, yyscanner);
    int32_t len;
    char *ps = s;
    while (*ps != ':') ps++;
    *(ps+1) = '\0';
    len = (int32_t) strlen(s);
    ans->lexeme = (char*)csound->Calloc(csound, len);
    strNcpy(ans->lexeme, s, len); /* Not the trailing colon */
    return ans;
}

ORCTOKEN *make_string(CSOUND *csound, char *s, void *yyscanner)
{
  ORCTOKEN *ans = new_token(csound, STRING_TOKEN, yyscanner);
    int32_t len = (int32_t) strlen(s);
/* Keep the quote marks */
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
    strcpy(ans->lexeme, s);
    //printf(">>%s<<\n", ans->lexeme);
    return ans;
}

ORCTOKEN *do_at(CSOUND *csound, int32_t k, void *yyscanner, char *yytex){
    int n, i = 1;
    ORCTOKEN *ans;
    char buf[16];
    char *s = yytex;
    int32_t len;
    while (*s=='@') s++;
    n = atoi(s);
    while (i<=n-k && i< 0x4000000) i <<= 1;
    ans = new_token(csound, INTEGER_TOKEN, yyscanner);
    snprintf(buf, 16, "%d", i+k);
    len = (int32_t) strlen(buf);
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
    strNcpy(ans->lexeme, buf, len+1);
    ans->value = i;
    return ans;
}

ORCTOKEN *make_int(CSOUND *csound, char *s, void *yyscanner)
{
    int n = atoi(s);
    ORCTOKEN *ans = new_token(csound, INTEGER_TOKEN, yyscanner);
    int32_t len = (int32_t) strlen(s);
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
    strNcpy(ans->lexeme, s, len+1);
    ans->value = n;
    return ans;
}

ORCTOKEN *make_num(CSOUND *csound, char *s, void *yyscanner)
{
    double n = atof(s);
    ORCTOKEN *ans = new_token(csound, NUMBER_TOKEN, yyscanner);
    int32_t len = (int32_t) strlen(s);
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
    strNcpy(ans->lexeme, s, len+1);
    ans->fvalue = n;
    return ans;
}

char *csound_orcget_current_pointer(void *yyscanner)
{
    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return yyg->yy_c_buf_p;
}

uint64_t csound_orcget_locn(void *yyscanner)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->locn;
}

void csound_orcput_ilocn(void *yyscanner, uint64_t num, uint64_t fil)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    PARM->iline = num;
    PARM->ilocn = fil;
}

uint64_t csound_orcget_iline(void *yyscanner)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->iline;
}

uint64_t csound_orcget_ilocn(void *yyscanner)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->ilocn;
}

uint32_t csound_orcget_first_column(void *yyscanner)
{
  //   struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->first_column;
}

uint32_t csound_orcget_last_column(void *yyscanner)
{
  //   struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->last_column;
}

