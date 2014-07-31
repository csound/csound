%{

 /*
    csound_prs.l:

    Copyright (C) 2013
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
#include "corfile.h"
#include "score_param.h"

#define YY_DECL int yylex (CSOUND *csound, yyscan_t yyscanner)
static void comment(yyscan_t);
static void do_comment(yyscan_t);
static void do_include(CSOUND *, int, yyscan_t);
static void do_macro_arg(CSOUND *, char *, yyscan_t);
static void do_macro(CSOUND *, char *, yyscan_t);
static void do_umacro(CSOUND *, char *, yyscan_t);
static void do_ifdef(CSOUND *, char *, yyscan_t);
static void do_ifdef_skip_code(CSOUND *, yyscan_t);
// static void print_csound_prsdata(CSOUND *,char *,yyscan_t);
static void csound_prs_line(CORFIL*, yyscan_t);

#define YY_EXTRA_TYPE  PRS_PARM *
#define PARM    yyget_extra(yyscanner)

#define YY_USER_INIT {csound_prs_scan_string(csound->scorestr->body, yyscanner); \
    csound_prsset_lineno(csound->scoLineOffset, yyscanner); yyg->yy_flex_debug_r=1;\
    PARM->macro_stack_size = 0; PARM->alt_stack = NULL; PARM->macro_stack_ptr = 0;}
%}
%option reentrant
%option noyywrap
%option prefix="csound_prs"
%option outfile="Engine/csound_prslex.c"
%option stdout

WHITE           ^[ \t]*
NEWLINE         (\n|\r\n?)
STSTR           \"
ESCAPE          \\.
XSTR            \{\{([^}]|\}[^}])*\}\}
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
IDENTN          [a-zA-Z0-9_]+
MACRONAME       "$"[a-zA-Z0-9_]+
MACRONAMED      "$"[a-zA-Z0-9_]+\.
MACRONAMEA      "$"[a-zA-Z0-9_]+\(
MACRONAMEDA     "$"[a-zA-Z0-9_]+\.\(
MACRO           [a-zA-Z0-9_]+\(

STCOM           \/\*
INCLUDE         "#include"
DEFINE          #[ \t]*define
UNDEF           "#undef"
IFDEF           #ifn?def
ELSE            #else[ \t]*(;.*)?$
END             #end(if)?[ \t]*(;.*)?(\n|\r\n?)
CONT            \\[ \t]*(;.*)?(\n|\r\n?)

%x incl
%x macro
%x umacro
%x ifdef

%%

{CONT}          {  int n = csound_prsget_lineno(yyscanner)+1;
                   csound_prsset_lineno(n, yyscanner);
                   //printf("cont case: %d\n", n);
                   csound_prs_line(csound->expanded_sco, yyscanner);
                   if (csound->expanded_sco->body[csound->expanded_sco->p-1]!='\n')
                     corfile_putc('\n', csound->expanded_sco);
                }
{NEWLINE}       {
                {  int n = csound_prsget_lineno(yyscanner)+1;
                   csound_prsset_lineno(n, yyscanner);
                   //printf("newline: %d\n", n);
                   csound_prs_line(csound->expanded_sco, yyscanner);
                   if (csound->expanded_sco->body[csound->expanded_sco->p-1]!='\n')
                     corfile_putc('\n', csound->expanded_sco); 
                 }
                }
"//"            {
                  if (PARM->isString != 1) {
                    int n = csound_prsget_lineno(yyscanner)+1;
                    comment(yyscanner);
                    //printf("comment++: %d\n", n);
                    csound_prsset_lineno(n, yyscanner);
                    csound_prs_line(csound->expanded_sco, yyscanner);
                    if (csound->expanded_sco->body[csound->expanded_sco->p-1]!='\n')
                      corfile_putc('\n', csound->expanded_sco); 
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_sco);
                  }
                }
";"             {
                  if (PARM->isString != 1) {
                    comment(yyscanner); 
                    csound_prsset_lineno(csound_prsget_lineno(yyscanner)+1,
                                         yyscanner);
                    //printf("comment: %d\n", csound_prsget_lineno(yyscanner));
                    csound_prs_line(csound->expanded_sco, yyscanner);
                    if (csound->expanded_sco->body[csound->expanded_sco->p-1]!='\n')
                      corfile_putc('\n', csound->expanded_sco);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_sco);
                  }
                }
{STCOM}         {
                  if (PARM->isString != 1)
                    do_comment(yyscanner);
                  else
                    corfile_puts(yytext, csound->expanded_sco);
                }
{ESCAPE}        { corfile_puts(yytext, csound->expanded_sco); }
{STSTR}         {
                  corfile_putc('"', csound->expanded_sco);
                  PARM->isString = !PARM->isString;
                }
{XSTR}          {
                  char c, *str = yytext;
                  if (PARM->isString == 1)
                    yyless(2);
                  while ((c = *str++) != '\0') {
                    switch(c) {
                    case '\r': if (*str == '\n') continue;
                    case '\n':
                      csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                                           yyscanner);
                      break;
                    default: break;
                    }
                  }
                  corfile_puts(yytext, csound->expanded_sco);
                }
{MACRONAME}     {
                   MACRO     *mm, *mfound=NULL;
                   unsigned int i, len, mlen;
                   //print_csound_prsdata(csound, "Macro call", yyscanner);
                   len = strlen(yytext)-1;
                   mlen = 0;
                   for (i=len; i>0; i--) { /* Find the definition */
                     mm = PARM->macros;
                     while (mm != NULL) {
                       if (!(strncmp(yytext+1, mm->name, i))) {
                         mfound = mm;
                         mlen = (unsigned)i;
                         if (strlen(mm->name) == mlen)
                           goto cont;
                       }
                       mm = mm->next;
                     }
                   }
                   cont:
                   mm = mfound;
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     csound->LongJmp(csound, 1);
                   }
                   if (mlen<len) yyless(mlen+1);
                   /* Need to read from macro definition */
                   /* csound->DebugMsg(csound, "found macro %s\nstack ptr = %d\n", */
                   /*         yytext+1, PARM->macro_stack_ptr); */
                   /* print_csound_prsdata(csound, "macro found", yyscanner); */
                   /* ??fiddle with buffers I guess */
                   if (UNLIKELY(PARM->macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                     csound->Die(csound, Str("Includes nested too deeply"));
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_prsget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   /* csound->DebugMsg(csound,"Push %p macro stack; new body #%s#\n", */
                   /*           PARM->macros, mm->body); */
                   /* csound->DebugMsg(csound,"Push buffer %p -> ", YY_CURRENT_BUFFER); */
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_prsset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
                   yy_scan_string(mm->body, yyscanner);
                   csound->DebugMsg(csound,"%p\n", YY_CURRENT_BUFFER);
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
                     csound->Message(csound, Str("Includes nested too deeply\n"));
                     exit(1);
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_prsget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_prsset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
                   yy_scan_string(mm->body, yyscanner);
                   csound->DebugMsg(csound,"%p\n", YY_CURRENT_BUFFER);
                 }
{MACRONAMEA}    {
                   MACRO     *mm = PARM->macros;
                   char      *mname;
                   int c, i, j;
                   /* csound->DebugMsg(csound,"Macro with arguments call %s\n",
                      yytext); */
                   yytext[yyleng-1] = '\0';
                   while (mm != NULL) {  /* Find the definition */
                     csound->DebugMsg(csound,"Check %s against %s\n",
                                      yytext+1, mm->name);
                     if (!(strcmp(yytext+1, mm->name)))
                       break;
                     mm = mm->next;
                   }
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     csound->LongJmp(csound, 1);
                   }
                   mname = yytext;
                   /* Need to read from macro definition */
                   csound->DebugMsg(csound,"Looking for %d args\n", mm->acnt);
                   for (j = 0; j < mm->acnt; j++) {
                     char  term = (j == mm->acnt - 1 ? ')' : '\'');
 /* Compatability */
                     char  trm1 = (j == mm->acnt - 1 ? ')' : '#');
                     MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                     int   size = 100;
                     nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                     csound->DebugMsg(csound,"Arg %d: %s\n", j+1, mm->arg[j]);
                     strcpy(nn->name, mm->arg[j]);
                     csound->Message(csound, "defining argument %s ",
                                        nn->name);
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
                     csound->Message(csound, "as...#%s#\n", nn->body);
                     nn->acnt = 0;       /* No arguments for arguments */
                     nn->next = PARM->macros;
                     PARM->macros = nn;
                   }
                   /* csound->DebugMsg(csound,"New body: ...#%s#\n", mm->body); */
                   if (UNLIKELY(PARM->macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                     csound->Message(csound,
                                     Str("macro_stack_ptr beyond end: %d \n"),
                                     PARM->macro_stack_ptr);
                     exit(1);
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = PARM->macros->acnt;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_prsget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr].s = NULL;
                   csound->DebugMsg(csound,"Push %p macro stack\n",PARM->macros);
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_prsset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, mname) : 63);
                   yy_scan_string(mm->body, yyscanner);
                 }
{MACRONAMEDA}    {
                   MACRO     *mm = PARM->macros;
                   char      *mname;
                   int c, i, j;
                   /* csound->DebugMsg(csound,"Macro with arguments call %s\n",
                      yytext); */
                   yytext[yyleng-2] = '\0';
                   while (mm != NULL) {  /* Find the definition */
                     csound->DebugMsg(csound,"Check %s against %s\n",
                                      yytext+1, mm->name);
                     if (!(strcmp(yytext+1, mm->name)))
                       break;
                     mm = mm->next;
                   }
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     csound->LongJmp(csound, 1);
                   }
                   mname = yytext;
                   /* Need to read from macro definition */
                   csound->DebugMsg(csound,"Looking for %d args\n", mm->acnt);
                   for (j = 0; j < mm->acnt; j++) {
                     char  term = (j == mm->acnt - 1 ? ')' : '\'');
 /* Compatability */
                     char  trm1 = (j == mm->acnt - 1 ? ')' : '#');
                     MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                     int   size = 100;
                     nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                     csound->DebugMsg(csound,"Arg %d: %s\n", j+1, mm->arg[j]);
                     strcpy(nn->name, mm->arg[j]);
                     csound->Message(csound, "defining argument %s ",
                                        nn->name);
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
                     csound->Message(csound, "as...#%s#\n", nn->body);
                     nn->acnt = 0;       /* No arguments for arguments */
                     nn->next = PARM->macros;
                     PARM->macros = nn;
                   }
                   csound->DebugMsg(csound,"New body: ...#%s#\n", mm->body);
                   PARM->alt_stack[PARM->macro_stack_ptr].n = PARM->macros->acnt;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_prsget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   if (PARM->depth++>1024) {
                     csound->Die(csound, Str("Includes nested too deeply"));
                   }
                   csound_prsset_lineno(1, yyscanner);
                   PARM->lstack[PARM->depth] = 
                     (strchr(mm->body,'\n') ?file_to_int(csound, mname) : 63);
                   yy_scan_string(mm->body, yyscanner);
                 }
{INCLUDE}       {
                  if (PARM->isString != 1)
                    BEGIN(incl);
                  else
                    corfile_puts(yytext, csound->expanded_sco);
                }
<incl>[ \t]*     /* eat the whitespace */
<incl>.         { /* got the include file name */
                  do_include(csound, yytext[0], yyscanner);
                  BEGIN(INITIAL);
                }
#exit           {
                  corfile_puts("#exit\n", csound->expanded_sco);
                  corfile_putc('\0', csound->expanded_sco);
                  corfile_putc('\0', csound->expanded_sco);
                  return 0;}
<<EOF>>         {
                  MACRO *x, *y=NULL;
                  int n;
                  csound->DebugMsg(csound,"*********Leaving buffer %p\n", YY_CURRENT_BUFFER);
                  yypop_buffer_state(yyscanner);
                  //printf("depth = %d\n", PARM->depth);
                  if (UNLIKELY(PARM->depth > 1024))
                    csound->Die(csound, Str("unexpected EOF"));
                  PARM->llocn = PARM->locn; PARM->locn = make_location(PARM);
                  csound->DebugMsg(csound,"%s(%d): loc=%d; lastloc=%d\n",
                                   __FILE__, __LINE__,
                                   PARM->llocn, PARM->locn);
                  if ( !YY_CURRENT_BUFFER ) yyterminate();
                  PARM->depth--;
                  /* csound->DebugMsg(csound,"End of input; popping to %p\n", */
                  /*         YY_CURRENT_BUFFER); */
                  csound_prs_line(csound->expanded_sco, yyscanner);
                  n = PARM->alt_stack[--PARM->macro_stack_ptr].n;
                  csound_prsset_lineno(PARM->alt_stack[PARM->macro_stack_ptr].line,
                                       yyscanner);
                  csound->DebugMsg(csound,"%s(%d): line now %d at %d\n",
                                   __FILE__, __LINE__,
                         csound_prsget_lineno(yyscanner), PARM->macro_stack_ptr);
                  /* csound->DebugMsg(csound,"n=%d\n", n); */
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
                  /* csound->DebugMsg(csound,
                               "End of input segment: macro pop %p -> %p\n",
                               y, PARM->macros); */
                  csound_prs_line(csound->scorestr, yyscanner);
                }
{DEFINE}        {
                  if (PARM->isString != 1)
                    BEGIN(macro);
                  else
                    corfile_puts(yytext, csound->expanded_sco);
                }
<macro>[ \t]*    /* eat the whitespace */
<macro>{MACRO}  {
                  yytext[yyleng-1] = '\0';
                  /* csound->DebugMsg(csound,"Define macro with args %s\n", yytext); */
                  /* print_csound_prsdata(csound, "Before do_macro_arg", yyscanner); */
                  do_macro_arg(csound, yytext, yyscanner);
                  //print_csound_prsdata(csound,"After do_macro_arg", yyscanner);
                  BEGIN(INITIAL);
                }
<macro>{IDENTN} {
                  /* csound->DebugMsg(csound,"Define macro %s\n", yytext); */
                  /* print_csound_prsdata(csound,"Before do_macro", yyscanner); */
                  do_macro(csound, yytext, yyscanner);
                  //print_csound_prsdata(csound,"After do_macro", yyscanner);
                  BEGIN(INITIAL);
                }
{UNDEF}         {
                  if (PARM->isString != 1)
                    BEGIN(umacro);
                  else
                    corfile_puts(yytext, csound->expanded_sco);
                }
<umacro>[ \t]*    /* eat the whitespace */
<umacro>{IDENT}  {
                  /* csound->DebugMsg(csound,"Undefine macro %s\n", yytext); */
                  do_umacro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }

{IFDEF}         {
                  if (PARM->isString != 1) {
                    int n = csound_prsget_lineno(yyscanner)+1;
                    PARM->isIfndef = (yytext[3] == 'n');  /* #ifdef or #ifndef */
                    if (csound->expanded_sco->body[csound->expanded_sco->p-1]!='\n')
                      corfile_putc('\n', csound->expanded_sco);
                    csound_prsset_lineno(n, yyscanner);
                    csound_prs_line(csound->expanded_sco, yyscanner);
                    BEGIN(ifdef);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_sco);
                  }
                }
<ifdef>[ \t]*     /* eat the whitespace */
<ifdef>{IDENT}  {
                  do_ifdef(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }
{ELSE}          { 
                  if (PARM->isString != 1) {
                    if (PARM->ifdefStack == NULL) {
                      csound->Message(csound, Str("#else without #if\n"));
                      csound->LongJmp(csound, 1); 
                    }
                    else if (PARM->ifdefStack->isElse) {
                      csound->Message(csound, Str("#else after #else\n"));
                      csound->LongJmp(csound, 1);
                    }
                    PARM->ifdefStack->isElse = 1;
                    {
                      int n = csound_prsget_lineno(yyscanner)+1;
                      if (csound->expanded_sco->body[csound->expanded_sco->p-1]!='\n')
                        corfile_putc('\n', csound->expanded_sco);
                      csound_prsset_lineno(n, yyscanner);
                    }
                    csound_prs_line(csound->expanded_sco, yyscanner);
                    do_ifdef_skip_code(csound, yyscanner);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_sco);
                  }
                }
{END}           {
                  if (PARM->isString != 1) {
                    IFDEFSTACK *pp = PARM->ifdefStack;
                    if (UNLIKELY(pp == NULL)) {
                      csound->Message(csound, Str("Unmatched #end\n"));
                      csound->LongJmp(csound, 1);
                    }
                    PARM->ifdefStack = pp->prv;
                    {
                      int n = csound_prsget_lineno(yyscanner)+1;
                      if (csound->expanded_sco->body[csound->expanded_sco->p-1]!='\n')
                        corfile_putc('\n', csound->expanded_sco);
                      csound_prsset_lineno(n, yyscanner);
                    }
                    csound_prs_line(csound->expanded_sco, yyscanner);
                    mfree(csound, pp);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_sco);
                  }
                }
.               { corfile_putc(yytext[0], csound->expanded_sco); }

%%
void comment(yyscan_t yyscanner)              /* Skip until nextline */
{
    char c;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    while ((c = input(yyscanner)) != '\n' && c != '\r') { /* skip */
      if (c == EOF) {
        YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
          YY_BUFFER_EOF_PENDING;
        return;
      }
    }
    if (c == '\r' && (c = input(yyscanner)) != '\n') {
      if (c != EOF)
        unput(c);
      else
        YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
          YY_BUFFER_EOF_PENDING;
    }
}

void do_comment(yyscan_t yyscanner)              /* Skip until * and / chars */
{
    char c;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    for (;;) {
      c = input(yyscanner);
      if (UNLIKELY(c == '\r')) { /* skip */
        if ((c = input(yyscanner)) != '\n')
          unput(c);
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
      }
      else if (UNLIKELY(c=='\n')) { /* skip */
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
      }
      if (c == EOF) {
        YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
          YY_BUFFER_EOF_PENDING;
        return;
      }
      if (c != '*') continue;
      while ((c=input(yyscanner))=='*');
      if (c=='/') return;
      if (UNLIKELY(c == '\r')) {
        if ((c = input(yyscanner)) != '\n')
          unput(c);
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
      }
      else if (UNLIKELY(c=='\n' || c=='\r')) {
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
      }
      if (c == EOF) {
        YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
          YY_BUFFER_EOF_PENDING;
        return;
      }
    }
}

void do_include(CSOUND *csound, int term, yyscan_t yyscanner)
{
    char buffer[100];
    int p=0;
    int c;
    CORFIL *cf;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    while ((c=input(yyscanner))!=term) {
      buffer[p] = c;
      p++;
    }
    buffer[p] = '\0';
    while ((c=input(yyscanner))!='\n' && c!='\r');
    if (PARM->depth++>=1024) {
      csound->Die(csound, Str("Includes nested too deeply"));
    }
    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner), yyscanner);
    csound->DebugMsg(csound,"line %d at end of #include line\n", csound_prsget_lineno(yyscanner));
    {
      uint8_t n = file_to_int(csound, buffer);
      char bb[16];
      PARM->lstack[PARM->depth] = n;
      sprintf(bb, "#source %d\n", PARM->locn = make_location(PARM));
      PARM->llocn = PARM->locn;
      corfile_puts(bb, csound->expanded_sco);
    }
    if (strstr(buffer, "://")) cf = copy_url_corefile(csound, buffer, 1);
    else                       cf = copy_to_corefile(csound, buffer, "INCDIR", 1);
    if (cf == NULL)
      csound->Die(csound,
                  Str("Cannot open #include'd file %s\n"), buffer);
    csound->DebugMsg(csound,"%s(%d): stacking line %d at %d\n", __FILE__, __LINE__,
           csound_prsget_lineno(yyscanner),PARM->macro_stack_ptr);
    PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
    PARM->alt_stack[PARM->macro_stack_ptr].line = csound_prsget_lineno(yyscanner);
    PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
    csound_prspush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
    csound_prs_scan_string(cf->body, yyscanner);
    corfile_rm(&cf);
    csound->DebugMsg(csound,"Set line number to 1\n");
    csound_prsset_lineno(1, yyscanner);
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
    if (UNLIKELY(c != ')')) {
      csound->Message(csound, Str("macro error\n"));
    }
    free(mname);
    while (c!='#') c = input(yyscanner);
    mm->acnt = arg;
    i = 0;
    mm->body = (char*) mmalloc(csound, 100);
    while ((c = input(yyscanner)) != '#') {
      if (UNLIKELY(c == EOF))
        csound->Die(csound, Str("define macro with args: unexpected EOF"));
      mm->body[i++] = c;
      if (UNLIKELY(i >= size))
        mm->body = mrealloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = mrealloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n' || c == '\r')) {
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
        corfile_putc('\n', csound->expanded_sco);
        csound_prs_line(csound->expanded_sco, yyscanner);
      }
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
    /* csound->DebugMsg(csound,"Macro definition for %s\n", name0); */
    mm->name = (char*)mmalloc(csound, strlen(name0) + 1);
    strcpy(mm->name, name0);
    mm->acnt = 0;
    i = 0;
    while ((c = input(yyscanner)) != '#');
    mm->body = (char*) mmalloc(csound, 100);
    while ((c = input(yyscanner)) != '#') {
      if (UNLIKELY(c == EOF))
        csound->Die(csound, Str("define macro: unexpected EOF"));
      mm->body[i++] = c;
      if (UNLIKELY(i >= size))
        mm->body = mrealloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = mrealloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n' || c == '\r')) {
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
        corfile_putc('\n', csound->expanded_sco);
        csound_prs_line(csound->expanded_sco, yyscanner);
      }
    }
    mm->body[i] = '\0';
    /* csound->DebugMsg(csound,"Body #%s#\n", mm->body); */
    mm->next = PARM->macros;
    PARM->macros = mm;
}

void do_umacro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int i,c;
    if (UNLIKELY(csound->oparms->msglevel))
      csound->Message(csound,Str("macro %s undefined\n"), name0);
    /* csound->DebugMsg(csound, "macro %s undefined\n", name0); */
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
    /* ignore rest of line */
    while ((c=input(yyscanner)) != '\n' && c != '\r'&& c != EOF);
    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
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
      while ((c = input(yyscanner)) != '\n' && c != '\r' && c != EOF);
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
      while (c!='\n' && c != '\r') {
        if (UNLIKELY(c == EOF)) {
          csound->Message(csound, Str("Unmatched #if%sdef\n"),
                          PARM->isIfndef ? "n" : "");
          csound->LongJmp(csound, 1);
        }
        c = input(yyscanner);
      }
      csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                           yyscanner);
      corfile_putc('\n', csound->expanded_sco);
      csound_prs_line(csound->expanded_sco, yyscanner);
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
    while (c != '\n' && c != '\r' && c != EOF) c = input(yyscanner);
}

void cs_init_smacros(CSOUND *csound, PRS_PARM *qq, NAMES *nn)
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
      if (UNLIKELY(s == NULL || s >= p)) {
        csound->Die(csound, Str("Invalid macro name for --smacro"));
      }
      mname = (char*) mmalloc(csound, (p - s) + 1);
      strncpy(mname, s, p - s);
      mname[p - s] = '\0';
      /* check if macro is already defined */
      for (mm = qq->macros; mm != NULL; mm = mm->next) {
        if (strcmp(mm->name, mname) == 0)
          break;
      }
      if (mm == NULL) {
        mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
        mm->name = mname;
        mm->next = qq->macros;
        qq->macros = mm;
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

void csound_prs_line(CORFIL* cf, void *yyscanner)
{
    int n = csound_prsget_lineno(yyscanner);
    /* This assumes that the initial line was not written with this system  */
    if (cf->body[cf->p-1]=='\n') {
      int locn = PARM->locn;
      int llocn = PARM->llocn;
      if (locn != llocn) {
        char bb[80];
        sprintf(bb, "#source %d\n", locn);
        corfile_puts(bb, cf);
      }
      PARM->llocn = locn;
      if (n!=PARM->line+1) {
        char bb[80];
        //printf("old line %d, new %d\n", PARM->line+1, n); 
        sprintf(bb, "#line %d\n", n);
        corfile_puts(bb, cf);
      }
    }
    PARM->line = n;
}

#ifdef MAIN_NEEDED
int main(void)
{
    PRS_PARM  qq;
    int len=100, p=0, n;
    char buff[1024];
    /* FILE *fd = fopen("III", "r"); */

    /* inp = (char*)calloc(100,1); */
    /* memset(buff, '\0', 1024); */
    /* while ((n = fread(buff, 1, 1023, fd))) { */
    /*   while (p+n+1>=len) */
    /*     inp = (char*) realloc(inp, len+=100); */
    /*   strcat(inp, buff); */
    /*   p += n; */
    /*   memset(buff, '\0', 1024); */
    /* } */
    /* if (n+8>= len) inp = (char*) realloc(inp, len = n+9); */
    /* strcat(inp, "\n#exit\0\0"); */
    /* rewind(fd); */

    memset(&qq, '\0', sizeof(PRS_PARM));
    csound_prslex_init(&qq.yyscanner);
    csound_prsset_extra(&qq, qq.yyscanner);
    //csound_prsset_debug(1, &qq.yyscanner);
    //csound_prspush_buffer_state(NULL, &qq.yyscanner);
    //csound_prs_scan_string(inp, qq.yyscanner);
    //csound_prsset_in(NULL, qq.yyscanner);
    qq.line = 1;
    csound_prslex(NULL, qq.yyscanner);
    //csound->DebugMsg(csound,corfile_body(expanded_prs));
    //csound_prslex_destroy(&qq.yyscanner);
    return 0;
}
#endif




