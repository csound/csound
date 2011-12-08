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
#include <ctype.h>
#include "csoundCore.h"
#define YYSTYPE TREE*
#define YYLTYPE ORCTOKEN*
#include "tok.h"
#define YY_DECL int yylex (YYLTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
#include "csound_orcparse.h"
#include "csound_orc.h"
YYSTYPE *yylval_param;
YYLTYPE *yylloc_param;
ORCTOKEN *make_string(CSOUND *, char *);
extern ORCTOKEN *lookup_token(CSOUND *, char *, void *);
extern  void    *fopen_path(CSOUND *, FILE **, char *, char *, char *, int);
ORCTOKEN *new_token(CSOUND *csound, int type);
ORCTOKEN *make_int(CSOUND *, char *);
ORCTOKEN *make_num(CSOUND *, char *);
ORCTOKEN *make_token(CSOUND *, char *s);
ORCTOKEN *make_label(CSOUND *, char *s);
void comment(yyscan_t);
void do_comment(yyscan_t);
void do_include(CSOUND *, int, yyscan_t);
void do_macro_arg(CSOUND *, char *, yyscan_t);
void do_macro(CSOUND *, char *, yyscan_t);
void do_umacro(CSOUND *, char *, yyscan_t);
void do_ifdef(CSOUND *, char *, yyscan_t);
void do_ifdef_skip_code(CSOUND *, yyscan_t);
#define udoflag csound->parserUdoflag
#define namedInstrFlag csound->parserNamedInstrFlag
//extern int udoflag;
//extern int namedInstrFlag;
//YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
#include "parse_param.h"

#define YY_EXTRA_TYPE  PARSE_PARM *
#define PARM    yyget_extra(yyscanner)

#define YY_INPUT(buf,result,max_size)  {\
    result = get_next_char(buf, max_size, yyg); \
    if ( UNLIKELY( result <= 0  )) \
      result = YY_NULL; \
    }
#define YY_USER_INIT csound_orcset_lineno(csound->orcLineOffset, yyscanner);

struct yyguts_t;
ORCTOKEN *do_at(CSOUND *, int, struct yyguts_t*);
int get_next_char(char *, int, struct yyguts_t*);
%}
%option reentrant
%option bison-bridge
%option bison-locations


STRCONST        \"(\\.|[^\"])*\"
STRCONSTe       \"(\\.|[^\"])*$
LABEL           ^[ \t]*[a-zA-Z_][a-zA-Z0-9_]*:
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
IDENTN          [a-zA-Z0-9_]+
MACRO           [a-zA-Z0-9_]+\(
MACRONAME       "$"[a-zA-Z0-9_]+
MACRONAMED      "$"[a-zA-Z0-9_]+\.
MACRONAMEA      "$"[a-zA-Z0-9_]+\(
MACRONAMEDA     "$"[a-zA-Z0-9_]+\.\(
INTGR           [0-9]+
NUMBER          [0-9]+\.[0-9]*(e[-+]?[0-9]+)?|\.[0-9]+(e[-+]?[0-9]+)?
STCOM           \/\*
WHITE           [ \t]+
OPTWHITE        [ \t]*
INCLUDE         "#include"
DEFINE          "#define"
UNDEF           "#undef"
IFDEF           #ifn?def
ELSE            #else[ \t]*(;.*)?$
END             #end(if)?[ \t]*(;.*)?\n
CONT            \\[[ \t]*\n
XSTR            "{{"
EXSTR           "}}"

%x incl
%x macro
%x umacro
%x ifdef
%x xstr

%%
"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */
{CONT}          { csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                       yyscanner);
 }
"\n"            { csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                       yyscanner);
                  return NEWLINE; }
"//"            { comment(yyscanner); return NEWLINE; }
";"             { comment(yyscanner); return NEWLINE; }
{STCOM}         { do_comment(yyscanner); }
"("             { return '('; }
")"             { return ')'; }
"["             { return '['; }
"]"             { return ']'; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"%"             { return '%'; }
"\^"            { return '^'; }
"?"             { return '?'; }
":"             { return ':'; }
","             { return ','; }
"!"             { return '!'; }
"!="            { return S_NEQ; }
"&&"            { return S_AND; }
"||"            { return S_OR; }
"<<"            { return S_BITSHL; }
">>"            { return S_BITSHR; }
"<"             { return S_LT; }
"<="            { return S_LE; }
"=="            { return S_EQ; }
"="             { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = '=';
                  return '='; }
">"             { return S_GT; }
">="            { return S_GE; }
"|"             { return S_BITOR; }
"&"             { return S_BITAND; }
"#"             { return '#'; }
"¬"             { return S_BITNOT; }
"~"             { return S_BITNOT; }
"@@"{OPTWHITE}{INTGR}     { *lvalp = do_at(csound, 1, yyg); return INTEGER_TOKEN; }
"@"{OPTWHITE}{INTGR}      { *lvalp = do_at(csound, 0, yyg); return INTEGER_TOKEN; }
"if"            { return IF_TOKEN; }
"then"          { return THEN_TOKEN; }
"ithen"         { return ITHEN_TOKEN; }
"kthen"         { return KTHEN_TOKEN; }
"elseif"        { return ELSEIF_TOKEN; }
"else"          { return ELSE_TOKEN; }
"endif"         { return ENDIF_TOKEN; }
"until"         { return UNTIL_TOKEN; }
"do"            { return DO_TOKEN; }
"od"            { return OD_TOKEN; }

"goto"          { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = GOTO_TOKEN;
                  return GOTO_TOKEN; };
"igoto"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = IGOTO_TOKEN;
                  return IGOTO_TOKEN; };
"kgoto"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = KGOTO_TOKEN;
                  return KGOTO_TOKEN; };

"sr"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = SRATE_TOKEN;
                  return SRATE_TOKEN; }
"kr"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = KRATE_TOKEN;
                  return KRATE_TOKEN; }
"ksmps"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = KSMPS_TOKEN;
                  return KSMPS_TOKEN; }
"nchnls"        { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = NCHNLS_TOKEN;
                  return NCHNLS_TOKEN; }
"nchnls_i"      { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = NCHNLSI_TOKEN;
                  return NCHNLSI_TOKEN; }
"instr"         {
                  namedInstrFlag = 1;
                  return INSTR_TOKEN;
                }
"endin"         { return ENDIN_TOKEN; }
"opcode"        {
                  PARM->clearBufferAfterEOF = 0;
                  return UDOSTART_TOKEN;
                }
"endop"         {
                  PARM->clearBufferAfterEOF = PARM->isInclude;
                  *lvalp = new_token(csound, UDOEND_TOKEN); return UDOEND_TOKEN;
                }

{LABEL}         { char *pp = yytext;
                  while (*pp==' ' || *pp=='\t') pp++;
                  *lvalp = make_label(csound, pp); return LABEL_TOKEN;
                }

"\{\{"          {
                  PARM->xstrbuff = (char *)malloc(128);
                  PARM->xstrptr = 0; PARM->xstrmax = 128;
                  PARM->xstrbuff[PARM->xstrptr++] = '"';
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                  BEGIN(xstr);
                }

<xstr>"}}"   {
                  BEGIN(INITIAL);
                  PARM->xstrbuff[PARM->xstrptr++] = '"';
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                  *lvalp = make_string(csound, PARM->xstrbuff);
                  free(PARM->xstrbuff);
                  return STRING_TOKEN;
                }

<xstr>"\n"     { /* THe next two should be one case but I cannot get that to work */
                  if (PARM->xstrptr+2==PARM->xstrmax) {
                      PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                      printf("Extending xstr buffer\n");
                  }
//                  printf("Adding newline (%.2x)\n", yytext[0]);
                  PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                }

<xstr>.     { if (PARM->xstrptr+2==PARM->xstrmax) {
                      PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                      printf("Extending xstr buffer\n");
                  }
//                  printf("Adding (%.2x)\n", yytext[0]);
                  PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                }

{STRCONST}      { *lvalp = make_string(csound, yytext); return (STRING_TOKEN); }

{STRCONSTe}     { *lvalp = make_string(csound, yytext);
                  csound->Message(csound,
                          Str("unterminated string found on line %d >>%s<<\n"),
                          csound_orcget_lineno(yyscanner),
                          yytext);
                  return (STRING_TOKEN); }

"0dbfs"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ZERODBFS_TOKEN;
                  /* csound->Message(csound,"%d\n", (*lvalp)->type); */
                  return ZERODBFS_TOKEN; }
{IDENT}         { *lvalp = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, (*lvalp)->type); */
                  return (*lvalp)->type; }
{MACRONAME}     {
                  MACRO     *mm = PARM->macros;
                  while (mm != NULL) {  /* Find the definition */
                    if (!(strcmp(yytext+1, mm->name)))
                      break;
                    mm = mm->next;
                  }
                  if (UNLIKELY(mm == NULL)) {
                    csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                    csound->LongJmp(csound, 1);
                  }
                  /* Need to read from macro definition */
                  /* ??fiddle with buffers I guess */
                  if (UNLIKELY(PARM->macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                    csound->Message(csound, Str("Includes nested too deeply"));
                    exit(1);
                  }
                  PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                  PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                  /* printf("Push %p macro stack; new body #%s#\n",
                            PARM->macros, mm->body); */
                  /* printf("Push buffer %p -> ", YY_CURRENT_BUFFER); */
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                  /* printf("%p\n", YY_CURRENT_BUFFER); */
                 }
{MACRONAMED}    {
                  MACRO     *mm = PARM->macros;
                  yytext[yyleng-1] = '\0';
                  while (mm != NULL) {  /* Find the definition */
                    if (!(strcmp(yytext+1, mm->name)))
                      break;
                    mm = mm->next;
                  }
                  if (UNLIKELY(mm == NULL)) {
                    csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                    csound->LongJmp(csound, 1);
                  }
                  /* Need to read from macro definition */
                  /* ??fiddle with buffers I guess */
                  if (UNLIKELY(PARM->macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                    csound->Message(csound, Str("Includes nested too deeply"));
                    exit(1);
                  }
                  PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                  PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                  /* printf("Push buffer %p -> ", YY_CURRENBUFFER_TOKEN); */
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                  /* printf("%p\n", YY_CURRENT_BUFFER); */
                }
{MACRONAMEA}    {
                  MACRO     *mm = PARM->macros;
                  int c, i, j;
                  printf("Macro with arguments call %s\n", yytext);
                  yytext[yyleng-1] = '\0';
                  while (mm != NULL) {  /* Find the definition */
                    /* printf("Check %s against %s\n", yytext+1, mm->name); */
                    if (!(strcmp(yytext+1, mm->name)))
                      break;
                    mm = mm->next;
                  }
                  if (UNLIKELY(mm == NULL)) {
                    csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                    csound->LongJmp(csound, 1);
                  }
                  /* Need to read from macro definition */
                  /* printf("Looking for %d args\n", mm->acnt); */
                  for (j = 0; j < mm->acnt; j++) {
                    char  term = (j == mm->acnt - 1 ? ')' : '\'');
/* Compatability */
                    char  trm1 = (j == mm->acnt - 1 ? ')' : '#');
                    MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                    int   size = 100;
                    nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                    /* printf("Arg %d: %s\n", j+1, mm->arg[j]); */
                    strcpy(nn->name, mm->arg[j]);
                    /* csound->Message(csound, "defining argument %s ",
                                       nn->name); */
                    i = 0;
                    nn->body = (char*) mmalloc(csound, 100);
                    while ((c = input(yyscanner))!= term && c!=trm1) {
                      if (UNLIKELY(i > 98)) {
                        csound->Die(csound,
                                    Str("Missing argument terminator\n%.98s"),
                                    nn->body);
                      }
                      nn->body[i++] = c;
                      if (UNLIKELY(i >= size))
                        nn->body = mrealloc(csound, nn->body, size += 100);
                    }
                    nn->body[i] = '\0';
                    /* csound->Message(csound, "as...#%s#\n", nn->body); */
                    nn->acnt = 0;       /* No arguments for arguments */
                    nn->next = PARM->macros;
                    PARM->macros = nn;
                  }
                  /* printf("New body: ...#%s#\n", mm->body); */
                  PARM->alt_stack[PARM->macro_stack_ptr].n = mm->acnt;
                  PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                  /* printf("Push %p macro stack\n",PARM->macros); */
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                }
{MACRONAMEDA}    {
                  MACRO     *mm = PARM->macros;
                  int c, i, j;
                  printf("Macro with arguments call %s\n", yytext);
                  yytext[yyleng-2] = '\0';
                  while (mm != NULL) {  /* Find the definition */
                    /* printf("Check %s against %s\n", yytext+1, mm->name); */
                    if (!(strcmp(yytext+1, mm->name)))
                      break;
                    mm = mm->next;
                  }
                  if (UNLIKELY(mm == NULL)) {
                    csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                    csound->LongJmp(csound, 1);
                  }
                  /* Need to read from macro definition */
                  /* printf("Looking for %d args\n", mm->acnt); */
                  for (j = 0; j < mm->acnt; j++) {
                    char  term = (j == mm->acnt - 1 ? ')' : '\'');
/* Compatability */
                    char  trm1 = (j == mm->acnt - 1 ? ')' : '#');
                    MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                    int   size = 100;
                    nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                    /* printf("Arg %d: %s\n", j+1, mm->arg[j]); */
                    strcpy(nn->name, mm->arg[j]);
                    /* csound->Message(csound, "defining argument %s ",
                                       nn->name); */
                    i = 0;
                    nn->body = (char*) mmalloc(csound, 100);
                    while ((c = input(yyscanner))!= term && c!=trm1) {
                      if (UNLIKELY(i > 98)) {
                        csound->Die(csound,
                                    Str("Missing argument terminator\n%.98s"),
                                    nn->body);
                      }
                      nn->body[i++] = c;
                      if (UNLIKELY(i >= size))
                        nn->body = mrealloc(csound, nn->body, size += 100);
                    }
                    nn->body[i] = '\0';
                    /* csound->Message(csound, "as...#%s#\n", nn->body); */
                    nn->acnt = 0;       /* No arguments for arguments */
                    nn->next = PARM->macros;
                    PARM->macros = nn;
                  }
                  /* printf("New body: ...#%s#\n", mm->body); */
                  PARM->alt_stack[PARM->macro_stack_ptr].n = mm->acnt;
                  PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                }
{INTGR}         {
                    if (udoflag == 0) {
                        *lvalp = lookup_token(csound, yytext, yyscanner);
                    } else if (udoflag == 1) {
                        *lvalp = lookup_token(csound, yytext, yyscanner);
                        (*lvalp)->type = UDO_ARGS_TOKEN;
                    } else {
                        *lvalp = make_int(csound, yytext); return (INTEGER_TOKEN);
                    }

                    csound->Message(csound,"%d\n", (*lvalp)->type);
                    return ((*lvalp)->type);
                }
{NUMBER}        { *lvalp = make_num(csound, yytext); return (NUMBER_TOKEN); }
{WHITE}         { }
{INCLUDE}       BEGIN(incl);
<incl>[ \t]*     /* eat the whitespace */
<incl>.         { /* got the include file name */
                  do_include(csound, yytext[0], yyscanner);
                  BEGIN(INITIAL);
                }
<<EOF>>         {
                  MACRO *x, *y;
                  int n;
                  printf("Leaving buffer %p\n", YY_CURRENT_BUFFER);
                  yypop_buffer_state(yyscanner);
                  if ( !YY_CURRENT_BUFFER ) yyterminate();
                  printf("End of input; popping to %p\n", YY_CURRENT_BUFFER);
                  if (PARM->clearBufferAfterEOF)
                    PARM->lBuffer =
                      PARM->clearBufferAfterEOF =
                      PARM->isInclude = 0;
                  n = PARM->alt_stack[--PARM->macro_stack_ptr].n;
                  printf("n=%d\n", n);
                  if (n!=0) {
                    /* We need to delete n macros starting with y */
                    y = PARM->alt_stack[PARM->macro_stack_ptr].s;
                    x = PARM->macros;
                    if (x==y) {
                      while (n>0) {
                        mfree(csound, y->name); x=y->next;
                        mfree(csound, y); y=x; n--;
                      }
                      PARM->macros = x;
                    }
                    else {
                      MACRO *nxt = y->next;
                      while (x->next != y) x = x->next;
                      while (n>0) {
                        nxt = y->next;
                        mfree(csound, y->name); mfree(csound, y); y=nxt; n--;
                      }
                      x->next = nxt;
                    }
                    y->next = x;
                  }
                  /* printf("End of input segment: macro pop %p -> %p\n",
                             y, PARM->macros); */
                }
{DEFINE}       BEGIN(macro);
<macro>[ \t]*    /* eat the whitespace */
<macro>{MACRO}  {
                  yytext[yyleng-1] = '\0';
                  printf("Define macro with args %s\n", yytext);
                  do_macro_arg(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }
<macro>{IDENTN} {
                  printf("Define macro %s\n", yytext);
                  do_macro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }
{UNDEF}        BEGIN(umacro);
<umacro>[ \t]*    /* eat the whitespace */
<umacro>{IDENT}  {
                  printf("Undefine macro %s\n", yytext);
                  do_umacro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }

.               { csound->Message(csound,
                                 Str("Line %d: Unknown character: '%c'(%2x)\n"),
                                 csound_orcget_lineno(yyscanner),
                                 yytext[0],yytext[0]);
                }
{IFDEF}         {
                  PARM->isIfndef = (yytext[3] == 'n');  /* #ifdef or #ifndef */
                  BEGIN(ifdef);
                }
<ifdef>[ \t]*     /* eat the whitespace */
<ifdef>{IDENT}  {
                  do_ifdef(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }
{ELSE}          { 
                  if (PARM->ifdefStack == NULL) {
                    csound->Message(csound, Str("#else without #if\n"));
                    csound->LongJmp(csound, 1); 
                  }
                  else if (PARM->ifdefStack->isElse) {
                    csound->Message(csound, Str("#else after #else\n"));
                    csound->LongJmp(csound, 1);
                  }
                  PARM->ifdefStack->isElse = 1;
                  do_ifdef_skip_code(csound, yyscanner);
                }
{END}           {
                  IFDEFSTACK *pp = PARM->ifdefStack;
                  if (UNLIKELY(pp == NULL)) {
                    csound->Message(csound, Str("Unmatched #end\n"));
                    csound->LongJmp(csound, 1);
                  }
                  PARM->ifdefStack = pp->prv;
                  mfree(csound, pp);
                }


%%
void comment(yyscan_t yyscanner)              /* Skip until nextline */
{
    char c;

    while ((c = input(yyscanner)) != '\n'); /* skip */
    csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),yyscanner);
}

void do_comment(yyscan_t yyscanner)              /* Skip until * and / chars */
{
    char c;
    for (;;) {
      c = input(yyscanner);
      if (UNLIKELY(c=='\n')) /* skip */
          csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),yyscanner);
      if (c != '*') continue;
      while ((c=input(yyscanner))=='*');
      if (c=='/') return;
      if (UNLIKELY(c=='\n'))
        csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),yyscanner);
    }
}

void do_include(CSOUND *csound, int term, yyscan_t yyscanner)
{
    char buffer[100];
    int p=0;
    int c;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    while ((c=input(yyscanner))!=term) {
      buffer[p] = c;
      p++;
    }
    buffer[p] = '\0';
    while ((c=input(yyscanner))!='\n');
    fopen_path(csound, &yyin, buffer, NULL, "INCDIR", 0);
    if (UNLIKELY( ! yyin )) {
      csound->Message(csound, Str("Cannot open file \"%s\"\n"), buffer);
      exit(1);
    }
    PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
    PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
    PARM->isInclude = PARM->clearBufferAfterEOF = 1;
    yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE, yyscanner), yyscanner);
}

static inline int isNameChar(int c, int pos)
{
    c = (int) ((unsigned char) c);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

void do_macro_arg(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    MACRO *mm = (MACRO*) mmalloc(csound, sizeof(MACRO));
    int   arg = 0, i, c;
    int   size = 100;
    int mlen = 40;
    char *mname = malloc(mlen);
    mm->margs = MARGS;    /* Initial size */
    mm->name = (char*)mmalloc(csound, strlen(name0) + 1);
    strcpy(mm->name, name0);
    do {
      while (isspace((c = input(yyscanner))));
      i = 0;

      while (isNameChar(c, i)) {
        mname[i++] = c;
        if (UNLIKELY(i==mlen))
          mname = (char *)realloc(mname, mlen+=40);
        c = input(yyscanner);
      }
      mname[i] = '\0';
      mm->arg[arg] = mmalloc(csound, i + 1);
      strcpy(mm->arg[arg++], mname);
      if (UNLIKELY(arg >= mm->margs)) {
        mm = (MACRO*) mrealloc(csound, mm, sizeof(MACRO)
                               + mm->margs * sizeof(char*));
        mm->margs += MARGS;
      }
      while (isspace(c))
        c = input(yyscanner);
    } while (c == '\'' || c == '#');
    if (UNLIKELY(c != ')'))
      csound->Message(csound, Str("macro error\n"));
    free(mname);
    while (c!='#') c = input(yyscanner);
    mm->acnt = arg;
    i = 0;
    mm->body = (char*) mmalloc(csound, 100);
    while ((c = input(yyscanner)) != '#') {
      mm->body[i++] = c;
      if (UNLIKELY(i >= size))
        mm->body = mrealloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = mrealloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n'))
        csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),yyscanner);
    }
    mm->body[i] = '\0';
    mm->next = PARM->macros;
    PARM->macros = mm;
}

void do_macro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    MACRO *mm = (MACRO*) mmalloc(csound, sizeof(MACRO));
    int   i, c;
    int   size = 100;
    mm->margs = MARGS;    /* Initial size */
    //    printf("Macro definition for %s\n", name0);
    mm->name = (char*)mmalloc(csound, strlen(name0) + 1);
    strcpy(mm->name, name0);
    mm->acnt = 0;
    i = 0;
    while ((c = input(yyscanner)) != '#');
    mm->body = (char*) mmalloc(csound, 100);
    while ((c = input(yyscanner)) != '#') {
      mm->body[i++] = c;
      if (UNLIKELY(i >= size))
        mm->body = mrealloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = mrealloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n'))
        csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),yyscanner);
    }
    mm->body[i] = '\0';
    //    printf("Body #%s#\n", mm->body);
    mm->next = PARM->macros;
    PARM->macros = mm;
}

void do_umacro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int i,c;
    if (UNLIKELY(csound->oparms->msglevel))
      csound->Message(csound,Str("macro %s undefined\n"), name0);
    if (strcmp(name0, PARM->macros->name)==0) {
      MACRO *mm=PARM->macros->next;
      mfree(csound, PARM->macros->name); mfree(csound, PARM->macros->body);
      for (i=0; i<PARM->macros->acnt; i++)
        mfree(csound, PARM->macros->arg[i]);
      mfree(csound, PARM->macros); PARM->macros = mm;
    }
    else {
      MACRO *mm = PARM->macros;
      MACRO *nn = mm->next;
      while (strcmp(name0, nn->name) != 0) {
        mm = nn; nn = nn->next;
        if (UNLIKELY(nn == NULL)) {
          csound->Message(csound, Str("Undefining undefined macro"));
          csound->LongJmp(csound, 1);
        }
      }
      mfree(csound, nn->name); mfree(csound, nn->body);
      for (i=0; i<nn->acnt; i++)
        mfree(csound, nn->arg[i]);
      mm->next = nn->next; mfree(csound, nn);
    }
    while ((c=input(yyscanner)) != '\n' && c != EOF); /* ignore rest of line */
    csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),yyscanner);
}

void do_ifdef(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int c;
    MACRO *mm;
    IFDEFSTACK *pp;
    pp = (IFDEFSTACK*) mcalloc(csound, sizeof(IFDEFSTACK));
    pp->prv = PARM->ifdefStack;
    pp->isDef = PARM->isIfndef;
    for (mm = PARM->macros; mm != NULL; mm = mm->next) {
      if (strcmp(name0, mm->name) == 0) {
        pp->isDef ^= (unsigned char) 1;
        break;
      }
    }
    PARM->ifdefStack = pp;
    pp->isSkip = pp->isDef ^ (unsigned char) 1;
    if (pp->isSkip)
      do_ifdef_skip_code(csound, yyscanner);
    else
      while ((c = input(yyscanner)) != '\n' && c != EOF);
}

void do_ifdef_skip_code(CSOUND *csound, yyscan_t yyscanner)
{
    int i, c, nested_ifdef = 0;
    char *buf;
    IFDEFSTACK *pp;
    buf = (char*)malloc(8*sizeof(char));
    pp = PARM->ifdefStack;
    c = input(yyscanner);
    for (;;) {
      while (c!='\n') {
        if (UNLIKELY(c == EOF)) {
          csound->Message(csound, Str("Unmatched #if%sdef\n"),
                          PARM->isIfndef ? "n" : "");
          csound->LongJmp(csound, 1);
        }
        c = input(yyscanner);
      }
      while (isblank(c = input(yyscanner)));  /* eat the whitespace */
      if (c == '#') {
        for (i=0; islower(c = input(yyscanner)) && i < 7; i++)
          buf[i] = c;
        buf[i] = '\0';
        if (strcmp("end", buf) == 0 || strcmp("endif", buf) == 0) {
          if (nested_ifdef-- == 0) {
            PARM->ifdefStack = pp->prv;
            mfree(csound, pp);
            break;
          }
        }
        else if (strcmp("ifdef", buf) == 0 || strcmp("ifndef", buf) == 0) {
          nested_ifdef++;
        }
        else if (strcmp("else", buf) == 0 && nested_ifdef == 0) {
          if (pp->isElse) {
            csound->Message(csound, Str("#else after #else\n"));
            csound->LongJmp(csound, 1);
          }
          pp->isElse = 1;
          break;
        }
      }
    }
    free(buf);
    while (c != '\n' && c != EOF) c = input(yyscanner);
}

ORCTOKEN *new_token(CSOUND *csound, int type)
{
    ORCTOKEN *ans = (ORCTOKEN*)mcalloc(csound, sizeof(ORCTOKEN));
    ans->type = type;
    return ans;
}

ORCTOKEN *make_token(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, STRING_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strcpy(ans->lexeme, s);
    return ans;
}

ORCTOKEN *make_label(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, LABEL_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len);
    strncpy(ans->lexeme, s, len - 1); /* Not the trailing colon */
    return ans;
}

ORCTOKEN *make_string(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, STRING_TOKEN);
    int len = strlen(s);
/* Keep the quote marks */
    /* ans->lexeme = (char*)mcalloc(csound, len-1); */
    /* strncpy(ans->lexeme, s+1, len-2); */
    /* ans->lexeme[len-2] = '\0';  */
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strcpy(ans->lexeme, s);
    return ans;
}

ORCTOKEN *do_at(CSOUND *csound, int k, struct yyguts_t *yyg)
{
    int n, i = 1;
    ORCTOKEN *ans;
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

ORCTOKEN *make_int(CSOUND *csound, char *s)
{
    int n = atoi(s);
    ORCTOKEN *ans = new_token(csound, INTEGER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->value = n;
    return ans;
}

ORCTOKEN *make_num(CSOUND *csound, char *s)
{
    double n = atof(s);
    ORCTOKEN *ans = new_token(csound, NUMBER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->fvalue = n;
    return ans;
}

static void add_math_const_macro(CSOUND *csound, void* yyscanner,
                                 char * name, char *body)
{
    MACRO *mm;

    mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
    mm->name = (char*) mcalloc(csound, strlen(name) + 3);
    sprintf(mm->name, "M_%s", name);
    mm->next = PARM->macros;
    PARM->macros = mm;
    mm->margs = MARGS;    /* Initial size */
    mm->acnt = 0;
    mm->body = (char*) mcalloc(csound, strlen(body) + 1);
    mm->body = strcpy(mm->body, body);
}

/**
 * Add math constants from math.h as orc PARM->macros
 */
void cs_init_math_constants_macros(CSOUND *csound, void* yyscanner)
 {
     PARM->macros = NULL;
     add_math_const_macro(csound, yyscanner, "E",     "2.71828182845904523536");
     add_math_const_macro(csound, yyscanner, "LOG2E", "1.44269504088896340736");
     add_math_const_macro(csound, yyscanner, "LOG10E","0.43429448190325182765");
     add_math_const_macro(csound, yyscanner, "LN2",   "0.69314718055994530942");
     add_math_const_macro(csound, yyscanner, "LN10",  "2.30258509299404568402");
     add_math_const_macro(csound, yyscanner, "PI",    "3.14159265358979323846");
     add_math_const_macro(csound, yyscanner, "PI_2",  "1.57079632679489661923");
     add_math_const_macro(csound, yyscanner, "PI_4",  "0.78539816339744830962");
     add_math_const_macro(csound, yyscanner, "1_PI",  "0.31830988618379067154");
     add_math_const_macro(csound, yyscanner, "2_PI",  "0.63661977236758134308");
     add_math_const_macro(csound, yyscanner,"2_SQRTPI", "1.12837916709551257390");
     add_math_const_macro(csound, yyscanner, "SQRT2", "1.41421356237309504880");
     add_math_const_macro(csound, yyscanner,"SQRT1_2","0.70710678118654752440");
     add_math_const_macro(csound, yyscanner, "INF",   "2147483647.0"); /* ~7 years */
}

void cs_init_omacros(CSOUND *csound, void *yyscanner, NAMES *nn)
{
    while (nn) {
      char  *s = nn->mac;
      char  *p = strchr(s, '=');
      char  *mname;
      MACRO *mm;

      if (p == NULL)
        p = s + strlen(s);
      if (csound->oparms->msglevel & 7)
        csound->Message(csound, Str("Macro definition for %*s\n"), p - s, s);
      s = strchr(s, ':') + 1;                   /* skip arg bit */
      if (UNLIKELY(s == NULL || s >= p))
        csound->Die(csound, Str("Invalid macro name for --omacro"));
      mname = (char*) mmalloc(csound, (p - s) + 1);
      strncpy(mname, s, p - s);
      mname[p - s] = '\0';
      /* check if macro is already defined */
      for (mm = PARM->macros; mm != NULL; mm = mm->next) {
        if (strcmp(mm->name, mname) == 0)
          break;
      }
      if (mm == NULL) {
        mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
        mm->name = mname;
        mm->next = PARM->macros;
        PARM->macros = mm;
      }
      else
        mfree(csound, mname);
      mm->margs = MARGS;    /* Initial size */
      mm->acnt = 0;
      if (*p != '\0')
        p++;
      mm->body = (char*) mmalloc(csound, strlen(p) + 1);
      strcpy(mm->body, p);
      nn = nn->next;
    }
}


/* Functions for working with grabbing input one line aa time */

int get_next_line(struct yyguts_t * yyg)
{
    char *p;
    void *yyscanner = yyg;

    PARM->nBuffer = 0;

    p = fgets(yyextra->buffer, lMaxBuffer, yyin);
    printf("get_next_line: %s\n", yyextra->buffer);
    if (UNLIKELY(p == NULL)) {
      if (ferror(yyin))
        return -1;
      return 1;
    }

    csound_orcset_lineno(1+csound_orcget_lineno(yyg),yyg);

    PARM->lBuffer = strlen(yyextra->buffer);

    return 0;
}

int get_next_char(char *b, int maxBuffer, struct yyguts_t* yyg)
{
    int frc;
    void *yyscanner = yyg;
    if (UNLIKELY( feof(yyin)  ))
      return 0;
    printf("get_next_char: %s\n", yyextra->buffer);
    while (  PARM->nBuffer >= PARM->lBuffer  ) {
      frc = get_next_line(yyg);
      if (UNLIKELY(  frc != 0  ))
        return 0;
    }

    b[0] = yyextra->buffer[PARM->nBuffer];
    PARM->nBuffer += 1;

    return b[0]=='\0'?0:1;
}

char *csound_orcget_current_pointer(void *yyscanner)
{
    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return yyg->yy_c_buf_p;
}
