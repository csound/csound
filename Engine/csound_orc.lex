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
#include <limits.h>
#include "csoundCore.h"

#ifdef ECHO
#undef ECHO
#endif
// to shut up the lexer writing to stdout
#define ECHO if(csound->oparms->odebug) { csoundErrorMsg(csound, "%s", "--lexer echo:"); \
             fwrite(yytext, (size_t) yyleng, 1, stderr); \
             csoundErrorMsg(csound, "%s", "--\n");}
#include "csound_orc.h"
#include "corfile.h"
#include "filesys.h"
#define YY_DECL int yylex (YYSTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
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

#define YY_USER_ACTION \
  PARM->first_column = yycolumn; \
  PARM->last_column = (uint32_t)(yycolumn + yyleng - 1); \
  PARM->previous_char = yyleng ? (unsigned char)yytext[yyleng - 1] : 0; \
  yycolumn += yyleng;
#define YY_USER_INIT

struct yyguts_t;
 ORCTOKEN *do_at(CSOUND *, int32_t, void*, char*);

static void append_xstr(CSOUND *csound, PARSE_PARM *parm,
                        const char *s, size_t len)
{
    if (parm->xstrbuff == NULL) {
        size_t cap = 128;
        while (cap <= len + 2) cap *= 2;
        parm->xstrbuff = (char *)malloc(cap);
        if (UNLIKELY(parm->xstrbuff == NULL)) {
            csound->ErrorMsg(csound, "%s\n", Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
        }
        parm->xstrmax = (int)cap;
        parm->xstrptr = 0;
        parm->xstrbuff[parm->xstrptr++] = '"';
    } else if ((size_t)parm->xstrptr + len + 2 >= (size_t)parm->xstrmax) {
        int newmax = parm->xstrmax;
        while ((size_t)parm->xstrptr + len + 2 >= (size_t)newmax) {
            if (UNLIKELY(newmax > INT_MAX / 2)) {
                csound->ErrorMsg(csound, "%s\n", Str("Memory exhausted"));
                csound->LongJmp(csound, 1);
            }
            newmax *= 2;
        }
        char *tmp = (char *)realloc(parm->xstrbuff, (size_t)newmax);
        if (UNLIKELY(tmp == NULL)) {
            csound->ErrorMsg(csound, "%s\n", Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
        }
        parm->xstrbuff = tmp;
        parm->xstrmax = newmax;
        csound->DebugMsg(csound, "Extending xstr buffer\n");
    }
    if (len > 0) {
        memcpy(parm->xstrbuff + parm->xstrptr, s, len);
        parm->xstrptr += (int)len;
    }
    parm->xstrbuff[parm->xstrptr] = '\0';
}
%}
%option reentrant
%option prefix="csound_orc"
%option outfile="Engine/csound_orclex.c"
%option stdout
%option 8bit
   /* to avoid unused function errors */
%option nounput

IDENT           [a-zA-Z_][a-zA-Z0-9_]*(@global)?
IDENTB          [a-zA-Z_][a-zA-Z0-9_]*\([ \t]*\n?
TYPED_IDENTIFIER  [a-zA-Z_][a-zA-Z0-9_]*(@global)?:[a-zA-Z_][a-zA-Z0-9_]*([ \t]*\[\])*
TYPED_IDENTIFIERB [a-zA-Z_][a-zA-Z0-9_]*:[a-zA-Z_][a-zA-Z0-9_]*(([ \t]*\[\])+[ \t]*)?\([ \t]*\n?
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
"="             { return '='; }
">"             { return S_GT; }
">="            { return S_GE; }
"|"             { return '|'; }
"&"             { return '&'; }
"#"             { return '#'; }
"¬"             { return '~'; } /* \xC2?\xAC */
"~"             { return '~'; }

\xC2?\xAC{OPTWHITE} { return '~'; } /* BACKWARDS COMPATABILITY */

"@@"{OPTWHITE}{INTGR}     { lvalp->token = do_at(csound, 1, yyscanner, yytext); return INTEGER_TOKEN; }
"@"{OPTWHITE}{INTGR}      { lvalp->token = do_at(csound, 0, yyscanner, yytext); return INTEGER_TOKEN; }
"false"         { return FALSE_TOKEN; }
"true"          { return TRUE_TOKEN; }
"falsek"        { return FALSEK_TOKEN; }
"truek"         { return TRUEK_TOKEN; }
"if"\([ \t]*    { yyless(2);
                  PARM->previous_char = (unsigned char)yytext[yyleng - 1];
                  return IF_TOKEN; }
"if"            { return IF_TOKEN; }
"then"          { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = THEN_TOKEN;
                  return THEN_TOKEN; }
"ithen"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = ITHEN_TOKEN;
                  return ITHEN_TOKEN; }
"kthen"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = KTHEN_TOKEN;
                  return KTHEN_TOKEN; }
"elseif"\([ \t]* { yyless(6);
                  PARM->previous_char = (unsigned char)yytext[yyleng - 1];
                  return ELSEIF_TOKEN; }
"elseif"        { return ELSEIF_TOKEN; }
"else"          { return ELSE_TOKEN; }
"endif"         { return ENDIF_TOKEN; }
"fi"            { return ENDIF_TOKEN; }
"until"\([ \t]* { yyless(5);
                  PARM->previous_char = (unsigned char)yytext[yyleng - 1];
                  lvalp->token = make_token(csound, "until", yyscanner);
                  lvalp->token->type = UNTIL_TOKEN;
                  return UNTIL_TOKEN; }
"until"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = UNTIL_TOKEN;
                  return UNTIL_TOKEN; }
"while"\([ \t]* { yyless(5);
                  PARM->previous_char = (unsigned char)yytext[yyleng - 1];
                  lvalp->token = make_token(csound, "while", yyscanner);
                  lvalp->token->type = WHILE_TOKEN;
                  return WHILE_TOKEN; }
"while"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = WHILE_TOKEN;
                  return WHILE_TOKEN; }
"do"            { return DO_TOKEN; }
"od"            { return OD_TOKEN; }
"enduntil"      { return OD_TOKEN; }
"break"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = BREAK_TOKEN;
                  return BREAK_TOKEN; }
"continue"      { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = CONTINUE_TOKEN;
                  return CONTINUE_TOKEN; }
"switch"        { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = SWITCH_TOKEN;
                  return SWITCH_TOKEN; }
"case"          { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = CASE_TOKEN;
                  return CASE_TOKEN; }
"default"       { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = DEFAULT_TOKEN;
                  return DEFAULT_TOKEN; }
"endsw"         { return ENDSW_TOKEN; }

"goto"          { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = GOTO_TOKEN;
                  return GOTO_TOKEN; };
"igoto"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = IGOTO_TOKEN;
                  return IGOTO_TOKEN; };
"kgoto"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = KGOTO_TOKEN;
                  return KGOTO_TOKEN; };
"struct"        {
                  return STRUCT_TOKEN;
                }
"instr"         {
                  return INSTR_TOKEN;
                }
"endin"         { return ENDIN_TOKEN; }
"void"          { return VOID_TOKEN; }


"for"           { BEGIN(forloop);
                  return FOR_TOKEN; }

<forloop>{
   ","            { return ','; }

  [ \t]+          /* eat the whitespace */
  {IDENT}/[ \t]*   { lvalp->token = make_token(csound, yytext, yyscanner);
                    if (strcmp(yytext, "in") == 0) {
                      BEGIN(INITIAL);
                      return IN_TOKEN;
                    } else {
                      lvalp->token->type = T_IDENT;
                      return T_IDENT;
                    }
                  }

  {TYPED_IDENTIFIER}/[ \t]*   { lvalp->token = lookup_token(csound, yytext, yyscanner);
                                return lvalp->token->type; }

  "\n"            { BEGIN(INITIAL);
                    yycolumn = 1;
                    csound_orcset_lineno(1 + csound_orcget_lineno(yyscanner),
                                         yyscanner);
                    return NEWLINE; }

  .               { BEGIN(INITIAL);
                    return ERROR_TOKEN; }
}

"\{\{"          {
                  PARM->xstrbuff = NULL;
                  PARM->xstrptr = 0; PARM->xstrmax = 0;
                  PARM->xsubstr = 0;
                  append_xstr(csound, PARM, "", 0);
                  BEGIN(xstr);
                }
<xstr>{
  "\{\{" {
             PARM->xsubstr += 1; // substr start
             append_xstr(csound, PARM, "{{", 2);
         }

  "}}"   {
    if (PARM->xsubstr) {
            PARM->xsubstr -= 1; // substr end
            append_xstr(csound, PARM, "}}", 2);
    } else {
           BEGIN(INITIAL);
           append_xstr(csound, PARM, "\"", 1);
           lvalp->token = make_string(csound, PARM->xstrbuff, yyscanner);
           free(PARM->xstrbuff);
           PARM->xstrbuff = NULL;
           return STRING_TOKEN;
    }
  }

  "\n"  {
           yycolumn = 1;
           csound_orcset_lineno(1 + csound_orcget_lineno(yyscanner),
                                yyscanner);
           append_xstr(csound, PARM, "\n", 1);
        }

  .     {
           append_xstr(csound, PARM, yytext, 1);
        }
}

"R{"   {
                  PARM->xstrbuff = NULL;
                  PARM->xstrptr = 0; PARM->xstrmax = 0;
                  PARM->xsubstr = 0;
                  append_xstr(csound, PARM, "", 0);
                  BEGIN(rstr);
                }

<rstr>{
  "R{" {
             PARM->xsubstr += 1; // substr start
             append_xstr(csound, PARM, "R{", 2);
         }

  "}R"   {
    if (PARM->xsubstr) {
            PARM->xsubstr -= 1; // substr end
            append_xstr(csound, PARM, "}R", 2);
    } else {
           BEGIN(INITIAL);
           append_xstr(csound, PARM, "\"", 1);
           lvalp->token = make_string(csound, PARM->xstrbuff, yyscanner);
           free(PARM->xstrbuff);
           PARM->xstrbuff = NULL;
           return STRING_TOKEN;
    }
  }

  "\n"  {
           yycolumn = 1;
           csound_orcset_lineno(1 + csound_orcget_lineno(yyscanner),
                                yyscanner);
           append_xstr(csound, PARM, "\n", 1);
        }

  .     {
           append_xstr(csound, PARM, yytext, 1);
        }
 }

<xstr,rstr><<EOF>> {
                  BEGIN(INITIAL);
                  csound->ErrorMsg(csound,
                                   Str("unterminated string at end of input, line %d\n"),
                                   csound_orcget_lineno(yyscanner));
                  free(PARM->xstrbuff);
                  PARM->xstrbuff = NULL;
                  return ERROR_TOKEN;
                }


^[ \t]*{IDENT}:/[ \t\n]  { char *pp = yytext;
                  while (*pp==' ' || *pp=='\t') pp++;
                  lvalp->token = make_label(csound, pp, yyscanner); return LABEL_TOKEN;
                }

"declare"       { BEGIN(declare);
                  return DECLARE_TOKEN;
                }

"opcode"        { BEGIN(udodef);
                  return UDOSTART_DEFINITION;
                }
"endop"         { return UDOEND_TOKEN; }


<udodef>{
  [ \t]+          { }

  {IDENT}/[ \t]*\( { BEGIN(INITIAL);
                    lvalp->token = lookup_token(csound, yytext, yyscanner);
                    /*csound->Message(csound, ">>>> NEW UDO DEF <<<<<<<\n");*/
                    /*csound->Message(csound,"%s -> %d\n",*/
                    /*                   yytext, lvalp->token->type); */
                    return lvalp->token->type; }


  {IDENT} { BEGIN(udoarg);
                    /*csound->Message(csound, ">>>> OLD UDO DEF <<<<<<<\n");*/
                    lvalp->token = lookup_token(csound, yytext, yyscanner);
                    /* csound->Message(csound,"%s -> %d\n",
                                       yytext, lvalp->token->type); */
                    return lvalp->token->type; }

  "\n"            { BEGIN(INITIAL);
                    yycolumn = 1;
                    csound_orcset_lineno(1 + csound_orcget_lineno(yyscanner),
                                         yyscanner);
                    return NEWLINE; }

  ","             { BEGIN(INITIAL);
                    return ','; }

  .               { BEGIN(INITIAL);
                    return ERROR_TOKEN; }
}

<declare>{
  [ \t]+          { }
  {IDENT} { BEGIN(INITIAL);
            lvalp->token = lookup_token(csound, yytext, yyscanner);
            return lvalp->token->type; }
  "\n"            { BEGIN(INITIAL);
                    yycolumn = 1;
                    csound_orcset_lineno(1 + csound_orcget_lineno(yyscanner),
                                         yyscanner);
                    return NEWLINE; }
  .               { BEGIN(INITIAL);
                    return ERROR_TOKEN; }
}

<udoarg>{
  [ \t]+  { }
  \"      { } /* Legacy UDO arg type lists may be quoted, e.g. "i[]i". */
  ","     { return ','; }
 {XIDENT} { BEGIN(udoarg);
                  lvalp->token = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, lvalp->token->type); */
                  lvalp->token->type = UDO_IDENT;
                  return lvalp->token->type; }
  "\n"     { BEGIN(INITIAL);
             yycolumn = 1;
             csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                   yyscanner);
             return NEWLINE; }
  {IDENT} {
    BEGIN(INITIAL);
    csound->Message(csound, Str("unsupported UDO arg type: %s"), yytext);
    return ERROR_TOKEN;
  }
  .        { BEGIN(INITIAL);
             return ERROR_TOKEN; }
}


\"              { /* String decode by c-code not rexp */
                  int cnt = 80;
                  char *buff = (char *)malloc((size_t)cnt);
                  int n = 1;
                  int ch;
                  int unterminated_eof = 0;
                  if (UNLIKELY(buff == NULL)) {
                    csound->ErrorMsg(csound, "%s\n", Str("Memory exhausted"));
                    csound->LongJmp(csound, 1);
                  }
                  buff[0] = '"';
                  for (;;) {
                    if (UNLIKELY(n >= cnt - 3)) {
                      int newcnt = cnt * 2;
                      char *tmp = (char *)realloc(buff, (size_t)newcnt);
                      if (UNLIKELY(tmp == NULL)) {
                        free(buff);
                        csound->ErrorMsg(csound, "%s\n", Str("Memory exhausted"));
                        csound->LongJmp(csound, 1);
                      }
                      buff = tmp;
                      cnt = newcnt;
                    }
                    ch = input(yyscanner);
                    if (ch=='"') {
                      buff[n++] = ch;
                      buff[n] = '\0';
                      break;
                    }
                    else if (ch=='\\') {
                      ch = input(yyscanner);
                      if (UNLIKELY(ch == 0 || ch == EOF)) {
                        buff[n++] = '"';
                        buff[n] = '\0';
                        csound->ErrorMsg(csound,
                                Str("unterminated string found on line %d >>%s<<\n"),
                                        csound_orcget_lineno(yyscanner), buff);
                        unterminated_eof = 1;
                        break;
                      }
                      if (ch == '\n') {
                        yycolumn = 1;
                        csound_orcset_lineno(1 + csound_orcget_lineno(yyscanner),
                                             yyscanner);
                      }
                      switch (ch) {
                      case 'a': case 'b': case 'n': case 'r':
                      case 't': case '\\':
                        buff[n++] = '\\'; buff[n++]= ch;
                        break;
                        /* VL - 21-1-17 fix for octals in strings */
                      case '0':case '1':case '2':case '3':
                      case '4':case '5':case '6':case '7':
                        buff[n++] = '\\'; buff[n++]= ch;
                        break;
                      default:
                        buff[n++] = ch;
                        break;
                      }
                    }
                    else if (UNLIKELY(ch == 0 || ch == EOF)) {
                      buff[n++] = '"';
                      buff[n] = '\0';
                      csound->ErrorMsg(csound,
                              Str("unterminated string found on line %d >>%s<<\n"),
                                      csound_orcget_lineno(yyscanner), buff);
                      unterminated_eof = 1;
                      break;
                    }
                    else if (UNLIKELY(ch=='\n')) {
                      buff[n++] = '"';
                      buff[n] = '\0';
                      csound->Message(csound,
                              Str("unterminated string found on line %d >>%s<<\n"),
                                      csound_orcget_lineno(yyscanner), buff);
                      yycolumn = 1;
                      csound_orcset_lineno(1 + csound_orcget_lineno(yyscanner),
                                           yyscanner);
                      break;
                    }
                    else {
                      buff[n++] = ch;
                    }
                  }
                  /* input() leaves a NUL after the token for error reporting. */
                  PARM->previous_char = 0;
                  if (UNLIKELY(unterminated_eof)) {
                    free(buff);
                    return ERROR_TOKEN;
                  }
                  lvalp->token = make_string(csound, buff, yyscanner);
                  free(buff);
                  return (STRING_TOKEN);
                }

"0dbfs"         { lvalp->token = make_token(csound, yytext, yyscanner);
                  lvalp->token->type = T_IDENT;
                  /* csound->Message(csound,"%d\n", lvalp->token->type); */
                  return T_IDENT; }
{IDENT}         { lvalp->token = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, lvalp->token->type); */
                  return lvalp->token->type; }
{IDENTB}        { PARM->paren_depth++;
                  if (UNLIKELY(strchr(yytext, '\n'))) {
                       yycolumn = 1;
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                  }
                  char *opening = strrchr(yytext, '(');
                  if (opening == yytext + yyleng - 1)
                    PARM->previous_char = '\0';
                  *opening = '\0';
                  lvalp->token = lookup_token(csound, yytext, yyscanner);
                  return lvalp->token->type+1; }
{TYPED_IDENTIFIER} { lvalp->token = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, lvalp->token->type); */
                  return lvalp->token->type; }
{TYPED_IDENTIFIERB} { PARM->paren_depth++;
                      if (UNLIKELY(strchr(yytext, '\n'))) {
                           yycolumn = 1;
                           csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                                yyscanner);
                      }
                      char *opening = strrchr(yytext, '(');
                      if (opening == yytext + yyleng - 1)
                        PARM->previous_char = '\0';
                      *opening = '\0';
                      lvalp->token = lookup_token(csound, yytext, yyscanner);
                      return lvalp->token->type+1; }
{INTGR}         {
                    lvalp->token = make_int(csound, yytext, yyscanner);
                    return (INTEGER_TOKEN);
                }
{LPAREN}     { PARM->paren_depth++;
               return *yytext; }

{RPAREN}     { if (PARM->paren_depth > 0)
                 PARM->paren_depth--;
               return *yytext; }

{SYMBOL}     { return *yytext;}

{NUMBER}        { lvalp->token = make_num(csound, yytext, yyscanner); return (NUMBER_TOKEN); }
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
                    csound->ErrorMsg(csound,
                                     Str("Error: character %c(%.2x)\n"), c, c);
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
        ans->lexeme = csoundParserStrdup(csound, baseName);
        ans->optype = csoundParserStrdup(csound, annotation);
        /* Normalize the copy. Error locations depend on the scanner buffer. */
        char *out = ans->optype;
        for (const char *in = ans->optype; *in != '\0'; in++) {
            if (*in != ' ' && *in != '\t')
                *out++ = *in;
        }
        *out = '\0';
        type = T_TYPED_IDENT;
    } else {
        ans->lexeme = csoundParserStrdup(csound, s);
    }
    ans->type = type;
    return ans;
}


ORCTOKEN *new_token(CSOUND *csound, int32_t type, void *yyscanner)
{
    ORCTOKEN *ans = (ORCTOKEN*)csoundParserOwn(csound, csound->Calloc(csound, sizeof(ORCTOKEN)));
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
    ans->lexeme = csoundParserStrdup(csound, s);
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
    ans->lexeme = (char*)csoundParserOwn(csound, csound->Calloc(csound, len));
    strNcpy(ans->lexeme, s, len); /* Not the trailing colon */
    return ans;
}

ORCTOKEN *make_string(CSOUND *csound, char *s, void *yyscanner)
{
  ORCTOKEN *ans = new_token(csound, STRING_TOKEN, yyscanner);
    int32_t len = (int32_t) strlen(s);
/* Keep the quote marks */
    ans->lexeme = (char*)csoundParserOwn(csound, csound->Calloc(csound, len + 1));
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
    ans->lexeme = (char*)csoundParserOwn(csound, csound->Calloc(csound, len + 1));
    strNcpy(ans->lexeme, buf, len+1);
    ans->value = i + k;
    return ans;
}

ORCTOKEN *make_int(CSOUND *csound, char *s, void *yyscanner)
{
    int n = atoi(s);
    ORCTOKEN *ans = new_token(csound, INTEGER_TOKEN, yyscanner);
    int32_t len = (int32_t) strlen(s);
    ans->lexeme = (char*)csoundParserOwn(csound, csound->Calloc(csound, len + 1));
    strNcpy(ans->lexeme, s, len+1);
    ans->value = n;
    return ans;
}

ORCTOKEN *make_num(CSOUND *csound, char *s, void *yyscanner)
{
    cs_double n = csoundStrtod(s, NULL);
    ORCTOKEN *ans = new_token(csound, NUMBER_TOKEN, yyscanner);
    int32_t len = (int32_t) strlen(s);
    ans->lexeme = (char*)csoundParserOwn(csound, csound->Calloc(csound, len + 1));
    strNcpy(ans->lexeme, s, len+1);
    ans->fvalue = n;
    return ans;
}

int csound_orcget_previous_char(void *yyscanner)
{
    return PARM->previous_char;
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

