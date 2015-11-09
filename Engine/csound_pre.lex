%{

 /*
    csound_pre.l:

    Copyright (C) 2011
    John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MAc
    02111-1307 USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
#include "corfile.h"
#define YY_DECL int yylex (CSOUND *csound, yyscan_t yyscanner)
void comment(yyscan_t);
void do_comment(yyscan_t);
void do_include(CSOUND *, int, yyscan_t);
void do_macro_arg(CSOUND *, char *, yyscan_t);
void do_macro(CSOUND *, char *, yyscan_t);
void do_umacro(CSOUND *, char *, yyscan_t);
void do_ifdef(CSOUND *, char *, yyscan_t);
void do_ifdef_skip_code(CSOUND *, yyscan_t);
void do_function(char *, CORFIL*);
   // static void print_csound_predata(CSOUND *,char *,yyscan_t);
void csound_pre_line(CORFIL*, yyscan_t);
 static void delete_macros(CSOUND*, yyscan_t);
#include "parse_param.h"

#define YY_EXTRA_TYPE  PRE_PARM *
#define PARM    yyget_extra(yyscanner)

#define YY_USER_INIT {csound_pre_scan_string(csound->orchstr->body, yyscanner); \
    csound_preset_lineno(csound->orcLineOffset, yyscanner);             \
    yyg->yy_flex_debug_r=1; PARM->macro_stack_size = 0;                 \
    PARM->alt_stack = NULL; PARM->macro_stack_ptr = 0;                  \
  }
 MACRO *find_definition(MACRO *, char *);
%}
%option reentrant
%option noyywrap
%option prefix="csound_pre"
%option outfile="Engine/csound_prelex.c"
%option stdout

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
RESET           "###\n"

INT             "int"[ \t]*\(
FRAC		"frac"[ \t]*\(
ROUND		"round"[ \t]*\(
FLOOR		"floor"[ \t]*\(
CEIL		"ceil"[ \t]*\(
RND		"rnd"[ \t]*\(
BIRND		"birnd"[ \t]*\(
ABS		"abs"[ \t]*\(
EXP		"exp"[ \t]*\(
LOG		"log"[ \t]*\(
SQRT		"sqrt"[ \t]*\(
SIN		"sin"[ \t]*\(
COS		"cos"[ \t]*\(
TAN		"tan"[ \t]*\(
SININV		"sininv"[ \t]*\(
COSINV		"cosinv"[ \t]*\(
TANINV		"taninv"[ \t]*\(
LOG10		"log10"[ \t]*\(
LOG2		"log2"[ \t]*\(
SINH		"sinh"[ \t]*\(
COSH		"cosh"[ \t]*\(
TANH		"tanh"[ \t]*\(
AMPDB		"ampdb"[ \t]*\(
AMPDBFS		"ampdbfs"[ \t]*\(
DBAMP		"dbamp"[ \t]*\(
DBFSAMP		"dbfsamp"[ \t]*\(
FTCPS		"ftcps"[ \t]*\(
FTLEN		"ftlen"[ \t]*\(
FTSR		"ftsr"[ \t]*\(
FTLPTIM		"ftlptim"[ \t]*\(
FTCHNLS		"ftchnls"[ \t]*\(
I		"i"[ \t]*\(
K		"k"[ \t]*\(
CPSOCT		"cpsoct"[ \t]*\(
OCTPCH		"octpch"[ \t]*\(
CPSPCH		"cpspch"[ \t]*\(
PCHOCT		"pchoct"[ \t]*\(
OCTCPS		"octcps"[ \t]*\(
NSAMP		"nsamp"[ \t]*\(
POWOFTWO	"powoftwo"[ \t]*\(
LOGBTWO		"logbtwo"[ \t]*\(
A		"a"[ \t]*\(
TB0		"tb0"[ \t]*\(
TB1		"tb1"[ \t]*\(
TB2		"tb2"[ \t]*\(
TB3		"tb3"[ \t]*\(
TB4		"tb4"[ \t]*\(
TB5		"tb5"[ \t]*\(
TB6		"tb6"[ \t]*\(
TB7		"tb7"[ \t]*\(
TB8		"tb8"[ \t]*\(
TB9		"tb9"[ \t]*\(
TB10		"tb10"[ \t]*\(
TB11		"tb11"[ \t]*\(
TB12		"tb12"[ \t]*\(
TB13		"tb13"[ \t]*\(
TB14		"tb14"[ \t]*\(
TB15		"tb15"[ \t]*\(
URD		"urd"[ \t]*\(
NOT		"not"[ \t]*\(
CENT		"cent"[ \t]*\(
OCTAVE		"octave"[ \t]*\(
SEMITONE	"semitone"[ \t]*\(
CPSMIDIN	"cpsmidinn"[ \t]*\(
OCTMIDIN	"octmidinn"[ \t]*\(
PCHMIDIN	"pchmidinn"[ \t]*\(
DB		"db"[ \t]*\(
P		"p"[ \t]*\(
QINF		"qinf"[ \t]*\(
QNAN		"qnan"[ \t]*\(

%X incl
%x macro
%x umacro
%x ifdef

%%

{RESET}         { csound_preset_lineno(csound->orcLineOffset, yyscanner);
                  csound->Free(csound, PARM->alt_stack);
                }
{CONT}          { csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                }
{NEWLINE}       { csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                  corfile_putc('\n', csound->expanded_orc); 
                  csound_pre_line(csound->expanded_orc, yyscanner);
                }
"//"            {
                  if (PARM->isString != 1) {
                    comment(yyscanner);
                    corfile_putc('\n', csound->expanded_orc); 
                    csound_pre_line(csound->expanded_orc, yyscanner);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_orc);
                  }
                }
";"             {
                  if (PARM->isString != 1) {
                    comment(yyscanner); 
                    corfile_putc('\n', csound->expanded_orc); 
                    csound_pre_line(csound->expanded_orc, yyscanner);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_orc);
                  }
                  //corfile_putline(csound_preget_lineno(yyscanner), csound->expanded_orc);
                }
{STCOM}         {
                  if (PARM->isString != 1)
                    do_comment(yyscanner);
                  else
                    corfile_puts(yytext, csound->expanded_orc);
                }
{ESCAPE}        { corfile_puts(yytext, csound->expanded_orc); }
{STSTR}         {
                  corfile_putc('"', csound->expanded_orc);
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
                      csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                           yyscanner);
                      break;
                    default: break;
                    }
                  }
                  corfile_puts(yytext, csound->expanded_orc);
                }
{MACRONAME}     {
                   MACRO     *mm = PARM->macros;
                   mm = find_definition(mm, yytext+1);
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     csound->LongJmp(csound, 1);
                   }
                   /* Need to read from macro definition */
                   /* ??fiddle with buffers I guess */
                   if (UNLIKELY(PARM->macro_stack_ptr >= PARM->macro_stack_size )) {
                     PARM->alt_stack =
                       (MACRON*)
                       csound->ReAlloc(csound, PARM->alt_stack,
                                       sizeof(MACRON)*(PARM->macro_stack_size+=10));
                     /* csound->DebugMsg(csound, "alt_stack now %d long\n", */
                     /*                  PARM->macro_stack_size); */
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_preset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
                   yy_scan_string(mm->body, yyscanner);
                   /* csound->DebugMsg(csound,"%p\n", YY_CURRENT_BUFFER); */
                }
{MACRONAMED}    {
                   MACRO     *mm = PARM->macros;
                   yytext[yyleng-1] = '\0';
                   mm = find_definition(mm, yytext+1);
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     csound->LongJmp(csound, 1);
                   }
                   /* Need to read from macro definition */
                   /* ??fiddle with buffers I guess */
                   if (UNLIKELY(PARM->macro_stack_ptr >= PARM->macro_stack_size )) {
                     PARM->alt_stack =
                       (MACRON*)
                       csound->ReAlloc(csound, PARM->alt_stack,
                                       sizeof(MACRON)*(PARM->macro_stack_size+=10));
                     /* csound->DebugMsg(csound, "alt_stack now %d long\n", */
                     /*                  PARM->macro_stack_size); */
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_preset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
                   yy_scan_string(mm->body, yyscanner);
                   /* csound->DebugMsg(csound,"%p\n", YY_CURRENT_BUFFER); */
                 }
{MACRONAMEA}    {
                   MACRO     *mm = PARM->macros;
                   char      *mname;
                   int c, i, j;
                   csound->DebugMsg(csound,"Macro with arguments call %s\n", yytext);
                   yytext[yyleng-1] = '\0';
                   mm = find_definition(PARM->macros, yytext+1);
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
                     MACRO *nn = (MACRO*) csound->Malloc(csound, sizeof(MACRO));
                     int   size = 100;
                     nn->name = csound->Malloc(csound, strlen(mm->arg[j]) + 1);
                     csound->DebugMsg(csound,"Arg %d: %s\n", j+1, mm->arg[j]);
                     strcpy(nn->name, mm->arg[j]);
                     csound->Message(csound, "defining argument %s ",
                                        nn->name);
                     i = 0;
                     nn->body = (char*) csound->Malloc(csound, 100);
                     while ((c = input(yyscanner))!= term && c!=trm1) {
                       if (UNLIKELY(i > 98)) {
                         csound->Die(csound,
                                     Str("Missing argument terminator\n%.98s"),
                                     nn->body);
                       }
                       nn->body[i++] = c;
                       if (UNLIKELY(i >= size))
                         nn->body = csound->ReAlloc(csound, nn->body, size += 100);
                     }
                     nn->body[i] = '\0';
                     csound->Message(csound, "as...#%s#\n", nn->body);
                     nn->acnt = 0;       /* No arguments for arguments */
                     nn->next = PARM->macros;
                     PARM->macros = nn;
                   }
                   csound->DebugMsg(csound,"New body: ...#%s#\n", mm->body);
                   if (UNLIKELY(PARM->macro_stack_ptr >= PARM->macro_stack_size )) {
                     PARM->alt_stack =
                       (MACRON*)
                       csound->ReAlloc(csound, PARM->alt_stack,
                                       sizeof(MACRON)*(PARM->macro_stack_size+=10));
                     /* csound->DebugMsg(csound, */
                     /*                  "macro_stack extends alt_stack to %d long\n", */
                     /*                  PARM->macro_stack_size); */
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = PARM->macros->acnt;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr].s = NULL;
                   csound->DebugMsg(csound,"Push %p macro stack\n",PARM->macros);
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_preset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, mname) : 63);
                   yy_scan_string(mm->body, yyscanner);
                 }
{MACRONAMEDA}    {
                   MACRO     *mm = PARM->macros;
                   char      *mname;
                   int c, i, j;
                   csound->DebugMsg(csound,"Macro with arguments call %s\n",
                                       yytext);
                   yytext[yyleng-2] = '\0';
                   mm = find_definition(PARM->macros, yytext+1);
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
                     MACRO *nn = (MACRO*) csound->Malloc(csound, sizeof(MACRO));
                     int   size = 100;
                     nn->name = csound->Malloc(csound, strlen(mm->arg[j]) + 1);
                     csound->DebugMsg(csound,"Arg %d: %s\n", j+1, mm->arg[j]);
                     strcpy(nn->name, mm->arg[j]);
                     csound->Message(csound, "defining argument %s ",
                                        nn->name);
                     i = 0;
                     nn->body = (char*) csound->Malloc(csound, 100);
                     while ((c = input(yyscanner))!= term && c!=trm1) {
                       if (UNLIKELY(i > 98)) {
                         csound->Die(csound,
                                     Str("Missing argument terminator\n%.98s"),
                                     nn->body);
                       }
                       nn->body[i++] = c;
                       if (UNLIKELY(i >= size))
                         nn->body = csound->ReAlloc(csound, nn->body, size += 100);
                     }
                     nn->body[i] = '\0';
                     csound->Message(csound, "as...#%s#\n", nn->body);
                     nn->acnt = 0;       /* No arguments for arguments */
                     nn->next = PARM->macros;
                     PARM->macros = nn;
                   }
                   csound->DebugMsg(csound,"New body: ...#%s#\n", mm->body);
                   if (UNLIKELY(PARM->macro_stack_ptr >= PARM->macro_stack_size )) {
                     PARM->alt_stack =
                       (MACRON*)
                       csound->ReAlloc(csound, PARM->alt_stack,
                                       sizeof(MACRON)*(PARM->macro_stack_size+=10));
                     /* csound->DebugMsg(csound, "alt_stack now %d long\n", */
                     /*                  PARM->macro_stack_size); */
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = PARM->macros->acnt;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   if (PARM->depth++>1024) {
                     csound->Die(csound, Str("Includes nested too deeply"));
                   }
                   csound_preset_lineno(1, yyscanner);
                   PARM->lstack[PARM->depth] = 
                     (strchr(mm->body,'\n') ?file_to_int(csound, mname) : 63);
                   yy_scan_string(mm->body, yyscanner);
                 }
{INCLUDE}       {
                  if (PARM->isString != 1)
                    BEGIN(incl);
                  else
                    corfile_puts(yytext, csound->expanded_orc);
                }
<incl>[ \t]*     /* eat the whitespace */
<incl>.         { /* got the include file name */
                  do_include(csound, yytext[0], yyscanner);
                  BEGIN(INITIAL);
                }
#exit           { corfile_putc('\0', csound->expanded_orc);
                  corfile_putc('\0', csound->expanded_orc);
                  delete_macros(csound, yyscanner);
                  return 0;}
<<EOF>>         {
                  MACRO *x, *y=NULL;
                  int n;
                  csound->DebugMsg(csound,"*********Leaving buffer %p\n",
                                   YY_CURRENT_BUFFER);
                  yypop_buffer_state(yyscanner);
                  PARM->depth--;
                  if (UNLIKELY(PARM->depth > 1024))
                    csound->Die(csound, Str("unexpected EOF"));
                  PARM->llocn = PARM->locn; PARM->locn = make_location(PARM);
                  /* csound->DebugMsg(csound,"%s(%d): loc=%Ld; lastloc=%Ld\n", */
                  /*                  __FILE__, __LINE__, */
                  /*        PARM->llocn, PARM->locn); */
                  if ( !YY_CURRENT_BUFFER ) yyterminate();
                  /* csound->DebugMsg(csound,"End of input; popping to %p\n", */
                  /*         YY_CURRENT_BUFFER); */
                  csound_pre_line(csound->expanded_orc, yyscanner);
                  if (UNLIKELY(PARM->macro_stack_ptr >= PARM->macro_stack_size )) {
                     PARM->alt_stack =
                       (MACRON*)
                       csound->ReAlloc(csound, PARM->alt_stack,
                                       sizeof(MACRON)*(PARM->macro_stack_size+=10));
                     /* csound->DebugMsg(csound, "alt_stack now %d long\n", */
                     /*                  PARM->macro_stack_size); */
                  }
                  n = PARM->alt_stack[--PARM->macro_stack_ptr].n;
                  csound_preset_lineno(PARM->alt_stack[PARM->macro_stack_ptr].line,
                                       yyscanner);
                  /* csound->DebugMsg(csound,"%s(%d): line now %d at %d\n", */
                  /*                  __FILE__, __LINE__, */
                  /*        csound_preget_lineno(yyscanner), */
                  /*        PARM->macro_stack_ptr); */
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
                  csound->DebugMsg(csound,
                                   "End of input segment: macro pop %p -> %p\n",
                                   y, PARM->macros);
                  csound_pre_line(csound->orchstr, yyscanner);
                }
{DEFINE}        {
                  if (PARM->isString != 1)
                    BEGIN(macro);
                  else
                    corfile_puts(yytext, csound->expanded_orc);
                }
<macro>[ \t]*    /* eat the whitespace */
<macro>{MACRO}  {
                  yytext[yyleng-1] = '\0';
                  csound->DebugMsg(csound,"Define macro with args %s\n",
                                      yytext);
                  /* print_csound_predata(csound, "Before do_macro_arg",
                                          yyscanner); */
                  do_macro_arg(csound, yytext, yyscanner);
                  //print_csound_predata(csound,"After do_macro_arg", yyscanner);
                  BEGIN(INITIAL);
                }
<macro>{IDENTN} {
                  csound->DebugMsg(csound,"Define macro %s\n", yytext);
                  /* print_csound_predata(csound,"Before do_macro", yyscanner); */
                  do_macro(csound, yytext, yyscanner);
                  //print_csound_predata(csound,"After do_macro", yyscanner);
                  BEGIN(INITIAL);
                }
{UNDEF}         {
                  if (PARM->isString != 1)
                    BEGIN(umacro);
                  else
                    corfile_puts(yytext, csound->expanded_orc);
                }
<umacro>[ \t]*    /* eat the whitespace */
<umacro>{IDENT}  {
                  csound->DebugMsg(csound,"Undefine macro %s\n", yytext);
                  do_umacro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }

{IFDEF}         {
                  if (PARM->isString != 1) {
                    PARM->isIfndef = (yytext[3] == 'n');  /* #ifdef or #ifndef */
                    csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                         yyscanner);
                    corfile_putc('\n', csound->expanded_orc);
                    csound_pre_line(csound->expanded_orc, yyscanner);
                    BEGIN(ifdef);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_orc);
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
                    csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                         yyscanner);
                    corfile_putc('\n', csound->expanded_orc);
                    csound_pre_line(csound->expanded_orc, yyscanner);
                    do_ifdef_skip_code(csound, yyscanner);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_orc);
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
                    csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                         yyscanner);
                    corfile_putc('\n', csound->expanded_orc);
                    csound_pre_line(csound->expanded_orc, yyscanner);
                    mfree(csound, pp);
                  }
                  else {
                    corfile_puts(yytext, csound->expanded_orc);
                  }
}
{IDENT}         { corfile_puts(yytext,csound->expanded_orc); }
{INT}     	{ do_function(yytext,csound->expanded_orc); }
{FRAC}		{ do_function(yytext,csound->expanded_orc); }
{ROUND}		{ do_function(yytext,csound->expanded_orc); }
{FLOOR}		{ do_function(yytext,csound->expanded_orc); }
{CEIL}		{ do_function(yytext,csound->expanded_orc); }
{RND}		{ do_function(yytext,csound->expanded_orc); }
{BIRND}		{ do_function(yytext,csound->expanded_orc); }
{ABS}		{ do_function(yytext,csound->expanded_orc); }
{EXP}		{ do_function(yytext,csound->expanded_orc); }
{LOG}		{ do_function(yytext,csound->expanded_orc); }
{SQRT}		{ do_function(yytext,csound->expanded_orc); }
{SIN}		{ do_function(yytext,csound->expanded_orc); }
{COS}		{ do_function(yytext,csound->expanded_orc); }
{TAN}		{ do_function(yytext,csound->expanded_orc); }
{SININV}	{ do_function(yytext,csound->expanded_orc); }
{COSINV}	{ do_function(yytext,csound->expanded_orc); }
{TANINV}	{ do_function(yytext,csound->expanded_orc); }
{LOG10}		{ do_function(yytext,csound->expanded_orc); }
{LOG2}		{ do_function(yytext,csound->expanded_orc); }
{SINH}		{ do_function(yytext,csound->expanded_orc); }
{COSH}		{ do_function(yytext,csound->expanded_orc); }
{TANH}		{ do_function(yytext,csound->expanded_orc); }
{AMPDB}		{ do_function(yytext,csound->expanded_orc); }
{AMPDBFS}	{ do_function(yytext,csound->expanded_orc); }
{DBAMP}		{ do_function(yytext,csound->expanded_orc); }
{DBFSAMP}	{ do_function(yytext,csound->expanded_orc); }
{FTCPS}		{ do_function(yytext,csound->expanded_orc); }
{FTLEN}		{ do_function(yytext,csound->expanded_orc); }
{FTSR}		{ do_function(yytext,csound->expanded_orc); }
{FTLPTIM}	{ do_function(yytext,csound->expanded_orc); }
{FTCHNLS}	{ do_function(yytext,csound->expanded_orc); }
{I}		{ do_function(yytext,csound->expanded_orc); }
{K}		{ do_function(yytext,csound->expanded_orc); }
{CPSOCT}	{ do_function(yytext,csound->expanded_orc); }
{OCTPCH}	{ do_function(yytext,csound->expanded_orc); }
{CPSPCH}	{ do_function(yytext,csound->expanded_orc); }
{PCHOCT}	{ do_function(yytext,csound->expanded_orc); }
{OCTCPS}	{ do_function(yytext,csound->expanded_orc); }
{NSAMP}		{ do_function(yytext,csound->expanded_orc); }
{POWOFTWO}	{ do_function(yytext,csound->expanded_orc); }
{LOGBTWO}	{ do_function(yytext,csound->expanded_orc); }
{A}		{ do_function(yytext,csound->expanded_orc); }
{TB0}		{ do_function(yytext,csound->expanded_orc); }
{TB1}		{ do_function(yytext,csound->expanded_orc); }
{TB2}		{ do_function(yytext,csound->expanded_orc); }
{TB3}		{ do_function(yytext,csound->expanded_orc); }
{TB4}		{ do_function(yytext,csound->expanded_orc); }
{TB5}		{ do_function(yytext,csound->expanded_orc); }
{TB6}		{ do_function(yytext,csound->expanded_orc); }
{TB7}		{ do_function(yytext,csound->expanded_orc); }
{TB8}		{ do_function(yytext,csound->expanded_orc); }
{TB9}		{ do_function(yytext,csound->expanded_orc); }
{TB10}		{ do_function(yytext,csound->expanded_orc); }
{TB11}		{ do_function(yytext,csound->expanded_orc); }
{TB12}		{ do_function(yytext,csound->expanded_orc); }
{TB13}		{ do_function(yytext,csound->expanded_orc); }
{TB14}		{ do_function(yytext,csound->expanded_orc); }
{TB15}		{ do_function(yytext,csound->expanded_orc); }
{URD}		{ do_function(yytext,csound->expanded_orc); }
{NOT}		{ do_function(yytext,csound->expanded_orc); }
{CENT}		{ do_function(yytext,csound->expanded_orc); }
{OCTAVE}	{ do_function(yytext,csound->expanded_orc); }
{SEMITONE}	{ do_function(yytext,csound->expanded_orc); }
{CPSMIDIN}	{ do_function(yytext,csound->expanded_orc); }
{OCTMIDIN}	{ do_function(yytext,csound->expanded_orc); }
{PCHMIDIN}	{ do_function(yytext,csound->expanded_orc); }
{DB}		{ do_function(yytext,csound->expanded_orc); }
{P}		{ do_function(yytext,csound->expanded_orc); }
{QINF}		{ do_function(yytext,csound->expanded_orc); }
{QNAN}		{ do_function(yytext,csound->expanded_orc); }

.               { corfile_putc(yytext[0], csound->expanded_orc); }

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
    csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
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
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
      }
      else if (UNLIKELY(c=='\n')) { /* skip */
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
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
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
      }
      else if (UNLIKELY(c=='\n')) {
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
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
    while ((c=input(yyscanner))!='\n');
    if (PARM->depth++>=1024) {
      csound->Die(csound, Str("Includes nested too deeply"));
    }
    csound_preset_lineno(1+csound_preget_lineno(yyscanner), yyscanner);
    csound->DebugMsg(csound,"line %d at end of #include line\n",
                     csound_preget_lineno(yyscanner));
    {
      uint8_t n = file_to_int(csound, buffer);
      char bb[128];
      PARM->lstack[PARM->depth] = n;
      sprintf(bb, "#source %llud\n", PARM->locn = make_location(PARM));
      PARM->llocn = PARM->locn;
      corfile_puts(bb, csound->expanded_orc);
    }
    csound->DebugMsg(csound,"reading included file \"%s\"\n", buffer);
    cf = copy_to_corefile(csound, buffer, "INCDIR", 0);
    if (cf == NULL)
      csound->Die(csound,
                  Str("Cannot open #include'd file %s\n"), buffer);
    if (UNLIKELY(PARM->macro_stack_ptr >= PARM->macro_stack_size )) {
      PARM->alt_stack =
        (MACRON*) csound->ReAlloc(csound, PARM->alt_stack,
                                  sizeof(MACRON)*(PARM->macro_stack_size+=10));
      /* csound->DebugMsg(csound, "alt_stack now %d long, \n", */
      /*                  PARM->macro_stack_size); */
    }
    csound->DebugMsg(csound,"%s(%d): stacking line %d at %d\n", __FILE__, __LINE__,
           csound_preget_lineno(yyscanner),PARM->macro_stack_ptr);
    PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
    PARM->alt_stack[PARM->macro_stack_ptr].line = csound_preget_lineno(yyscanner);
    PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
    csound_prepush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
    csound_pre_scan_string(cf->body, yyscanner);
    corfile_rm(&cf);
    csound->DebugMsg(csound,"Set line number to 1\n");
    csound_preset_lineno(1, yyscanner);
}

static inline int isNameChar(int c, int pos)
{
    c = (int) ((unsigned char) c);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

void do_macro_arg(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    MACRO *mm = (MACRO*) csound->Malloc(csound, sizeof(MACRO));
    int   arg = 0, i, c;
    int   size = 100;
    int mlen = 40;
    char *q = name0;
    char *mname = malloc(mlen);
    mm->margs = MARGS;    /* Initial size */
    mm->name = (char*)csound->Malloc(csound, strlen(name0) + 1);
    strcpy(mm->name, name0);
    do {
      i = 0;
      q = name0;
      mname[i++] = '_';
      while ((c = *q++)) {
        mname[i++] = c;
        if (UNLIKELY(i==mlen))
          mname = (char *)realloc(mname, mlen+=40);
      }
      mname[i++] = '_';
      if (UNLIKELY(i==mlen))
          mname = (char *)realloc(mname, mlen+=40);
      mname[i++] = '_';
      if (UNLIKELY(i==mlen))
          mname = (char *)realloc(mname, mlen+=40);
      while (isspace((c = input(yyscanner))));

      while (isNameChar(c, i)) {
        mname[i++] = c;
        if (UNLIKELY(i==mlen))
          mname = (char *)realloc(mname, mlen+=40);
        c = input(yyscanner);
      }
      mname[i] = '\0';
      mm->arg[arg] = csound->Malloc(csound, i + 1);
      strcpy(mm->arg[arg++], mname);
      if (UNLIKELY(arg >= mm->margs)) {
        mm = (MACRO*) csound->ReAlloc(csound, mm, sizeof(MACRO)
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
    while (c!='#') {
      if (c==EOF) csound->Die(csound, Str("define macro runaway\n"));
      else if (c==';')
        while ((c=input(yyscanner))!= '\n') 
          if (c==EOF) {
            csound->Die(csound, Str("define macro runaway\n"));
          }
      else if (c=='/') {
        if ((c=input(yyscanner))=='/') {
          while ((c=input(yyscanner))!= '\n')
            if (c==EOF)
              csound->Die(csound, Str("define macro runaway\n"));
        }
        else if (c=='*') {
          while ((c=input(yyscanner))!='*') {
          again:
            if (c==EOF) csound->Die(csound, Str("define macro runaway\n"));
          }
          if ((c=input(yyscanner))!='/') goto again;
        }
      }
      else if (!isspace(c))
        csound->Die(csound,
                    Str("define macro unexpected character %c(0x%.2x) awaiting #\n"),
                    c, c);
      c = input(yyscanner); /* skip to start of body */
    }
    mm->acnt = arg;
    i = 0;
    mm->body = (char*) csound->Malloc(csound, 100);
    while ((c = input(yyscanner)) != '#') { /* read body */
      if (UNLIKELY(c == EOF))
        csound->Die(csound, Str("define macro with args: unexpected EOF"));
      if (c=='$') {             /* munge macro name? */
        int n = strlen(name0)+4;
        if (UNLIKELY(i+n >= size))
          mm->body = csound->ReAlloc(csound, mm->body, size += 100);
        mm->body[i] = '$'; mm->body[i+1] = '_';
        strcpy(&mm->body[i+2], name0);
        mm->body[i + n - 2] = '_'; mm->body[i + n - 1] = '_';
        i+=n;
        continue;
      }
      mm->body[i++] = c=='\r'?'\n':c;
      if (UNLIKELY(i >= size))
        mm->body = csound->ReAlloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = csound->ReAlloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n' || c == '\r')) {
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
        corfile_putc('\n', csound->expanded_orc);
        csound_pre_line(csound->expanded_orc, yyscanner);
      }
    }
    mm->body[i] = '\0';
    mm->next = PARM->macros;
    PARM->macros = mm;
}

void do_macro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    MACRO *mm = (MACRO*) csound->Malloc(csound, sizeof(MACRO));
    int   i, c;
    int   size = 100;
    mm->margs = MARGS;    /* Initial size */
    csound->DebugMsg(csound,"Macro definition for %s\n", name0);
    mm->name = (char*)csound->Malloc(csound, strlen(name0) + 1);
    strcpy(mm->name, name0);
    mm->acnt = 0;
    i = 0;
    while ((c = input(yyscanner)) != '#') {
      if (c==EOF) csound->Die(csound, Str("define macro runaway\n"));
      else if (c==';') {
        while ((c=input(yyscanner))!= '\n')
          if (c==EOF) {
            csound->Die(csound, Str("define macro runaway\n"));
          }
      }
      else if (c=='/') {
        if ((c=input(yyscanner))=='/') {
          while ((c=input(yyscanner))!= '\n')
            if (c==EOF)
              csound->Die(csound, Str("define macro runaway\n"));
        }
        else if (c=='*') {
          while ((c=input(yyscanner))!='*') {
          again:
            if (c==EOF) csound->Die(csound, Str("define macro runaway\n"));
          }
          if ((c=input(yyscanner))!='/') goto again;
        }
      }
      else if (!isspace(c))
        csound->Die(csound,
                    Str("define macro unexpected character %c(0x%.2x) awaiting #\n"),
                    c, c);
    }
    mm->body = (char*) csound->Malloc(csound, 100);
    while ((c = input(yyscanner)) != '#') {
      if (UNLIKELY(c == EOF))
        csound->Die(csound, Str("define macro: unexpected EOF"));
      mm->body[i++] = c=='\r'?'\n':c;
      if (UNLIKELY(i >= size))
        mm->body = csound->ReAlloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = csound->ReAlloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n' || c == '\r')) {
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
        corfile_putc('\n', csound->expanded_orc);
        csound_pre_line(csound->expanded_orc, yyscanner);
      }
    }
    mm->body[i] = '\0';
    csound->DebugMsg(csound,"Body #%s#\n", mm->body);
    mm->next = PARM->macros;
    PARM->macros = mm;
}

void do_umacro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int i,c;
    if (UNLIKELY(csound->oparms->msglevel))
      csound->Message(csound,Str("macro %s undefined\n"), name0);
    csound->DebugMsg(csound, "macro %s undefined\n", name0);
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
    while ((c=input(yyscanner)) != '\n' && c != EOF && c != '\r'); /* ignore rest of line */
    csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
}

void do_ifdef(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int c;
    MACRO *mm;
    IFDEFSTACK *pp;
    pp = (IFDEFSTACK*) csound->Calloc(csound, sizeof(IFDEFSTACK));
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
      while (c!='\n' && c!= '\r') {
        if (UNLIKELY(c == EOF)) {
          csound->Message(csound, Str("Unmatched #if%sdef\n"),
                          PARM->isIfndef ? "n" : "");
          csound->LongJmp(csound, 1);
        }
        c = input(yyscanner);
    }
      csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                           yyscanner);
      corfile_putc('\n', csound->expanded_orc);
      csound_pre_line(csound->expanded_orc, yyscanner);
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
    while (c != '\n' && c != EOF && c != '\r') c = input(yyscanner);
}

static void delete_macros(CSOUND *csound, yyscan_t yyscanner)
{
    MACRO * qq = PARM->macros;
    if (qq) {
      MACRO *mm = qq;
      while (mm) {
        csound->Free(csound, mm->body);
        csound->Free(csound, mm->name);
        qq = mm->next;
        csound->Free(csound, mm);
        mm = qq;
       }
    }
}

static void add_math_const_macro(CSOUND *csound, PRE_PARM* qq,
                                 char * name, char *body)
{
    MACRO *mm;

    mm = (MACRO*) csound->Calloc(csound, sizeof(MACRO));
    mm->name = (char*) csound->Calloc(csound, strlen(name) + 3);
    sprintf(mm->name, "M_%s", name);
    mm->next = qq->macros;
    qq->macros = mm;
    mm->margs = MARGS;    /* Initial size */
    mm->acnt = 0;
    mm->body = (char*) csound->Calloc(csound, strlen(body) + 1);
    mm->body = strcpy(mm->body, body);
}

/**
 * Add math constants from math.h as orc PARM->macros
 */
void cs_init_math_constants_macros(CSOUND *csound, PRE_PARM* qq)
{
    qq->macros = NULL;
     add_math_const_macro(csound, qq, "E",     "2.71828182845904523536");
     add_math_const_macro(csound, qq, "LOG2E", "1.44269504088896340736");
     add_math_const_macro(csound, qq, "LOG10E","0.43429448190325182765");
     add_math_const_macro(csound, qq, "LN2",   "0.69314718055994530942");
     add_math_const_macro(csound, qq, "LN10",  "2.30258509299404568402");
     add_math_const_macro(csound, qq, "PI",    "3.14159265358979323846");
     add_math_const_macro(csound, qq, "PI_2",  "1.57079632679489661923");
     add_math_const_macro(csound, qq, "PI_4",  "0.78539816339744830962");
     add_math_const_macro(csound, qq, "1_PI",  "0.31830988618379067154");
     add_math_const_macro(csound, qq, "2_PI",  "0.63661977236758134308");
     add_math_const_macro(csound, qq,"2_SQRTPI", "1.12837916709551257390");
     add_math_const_macro(csound, qq, "SQRT2", "1.41421356237309504880");
     add_math_const_macro(csound, qq,"SQRT1_2","0.70710678118654752440");
     add_math_const_macro(csound, qq, "INF",   "800000000000.0"); /* ~25367 years */
}

void cs_init_omacros(CSOUND *csound, PRE_PARM *qq, NAMES *nn)
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
        csound->Die(csound, Str("Invalid macro name for --omacro"));
      }
      mname = (char*) csound->Malloc(csound, (p - s) + 1);
      strncpy(mname, s, p - s);
      mname[p - s] = '\0';
      /* check if macro is already defined */
      for (mm = qq->macros; mm != NULL; mm = mm->next) {
        if (strcmp(mm->name, mname) == 0)
          break;
      }
      if (mm == NULL) {
        mm = (MACRO*) csound->Calloc(csound, sizeof(MACRO));
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
      mm->body = (char*) csound->Malloc(csound, strlen(p) + 1);
      strcpy(mm->body, p);
      nn = nn->next;
    }
}

/* int csound_prewrap(void *yyscanner)  */
/* {  */
/*     return 1; */
/* } */

void csound_pre_line(CORFIL* cf, void *yyscanner)
{
    int n = csound_preget_lineno(yyscanner);
    /* This assumes that the initial line was not written with this system  */
    if (cf->body[cf->p-1]=='\n') {
      uint64_t locn = PARM->locn;
      uint64_t llocn = PARM->llocn;
      if (locn != llocn) {
        char bb[80];
        sprintf(bb, "#source %llud\n", locn);
        corfile_puts(bb, cf);
      }
      PARM->llocn = locn;
      if (n!=PARM->line+1) {
        char bb[80];
        sprintf(bb, "#line %d\n", n);
        corfile_puts(bb, cf);
      }
    }
    PARM->line = n;
}

void do_function(char *text, CORFIL *cf)
{
    char *p = text;
    //printf("do_function on >>%s<<\n", text);
    while (*p != '\0') {
      if (!isspace(*p)) corfile_putc(*p, cf);
      p++;
    }
    return;
}

MACRO *find_definition(MACRO *mmo, char *s)
{
    MACRO *mm = mmo;
    //printf("****Looking for %s\n", s);
    while (mm != NULL) {  /* Find the definition */
      //printf("looking at %p(%s) body #%s#\n", mm, mm->name, mm->body);
      if (!(strcmp(s, mm->name))) break;
      mm = mm->next;
    }
    if (mm == NULL) {
      mm = mmo;
      s++;                      /* skip _ */
    looking:
      while (*s++!='_') { if (*s=='\0') return NULL; }
      if (*s++!='_') { s--; goto looking; }
      //printf("now try looking for %s\n", s);
      while (mm != NULL) {  /* Find the definition */
        //printf("looking at %p(%s) body #%s#\n", mm, mm->name, mm->body);
        if (!(strcmp(s, mm->name))) break;
        mm = mm->next;
      }
    }
    //if (mm) printf("found body #%s#\n****\n", mm->body);
    return mm;
}


#if 0
static void print_csound_predata(CSOUND *csound, char *mesg, void *yyscanner)
{
    struct yyguts_t *yyg =(struct yyguts_t*)yyscanner;
    csound->DebugMsg(csound,"********* %s extra data ************\n", mesg);
    csound->DebugMsg(csound,"yyscanner = %p\n", yyscanner);
    csound->DebugMsg(csound,"yyextra_r = %p, yyin_r = %p, yyout_r = %p, yy_buffer_stack_top = %d\n", 
           yyg->yyextra_r, yyg->yyin_r,yyg->yyout_r, yyg->yy_buffer_stack_top);
    csound->DebugMsg(csound,"yy_buffer_stack_max = %d1, yy_buffer_stack = %p, yy_hold_char = %d '%c'\n", 
           yyg->yy_buffer_stack_max, yyg->yy_buffer_stack, yyg->yy_hold_char,
           yyg->yy_hold_char);
    csound->DebugMsg(csound,"yy_n_chars = %d, yyleng_r = %d, yy_c_buf_p = %p %c\n",
           yyg->yy_n_chars, yyg->yyleng_r, yyg->yy_c_buf_p, *yyg->yy_c_buf_p);
    csound->DebugMsg(csound,"yy_init = %d, yy_start = %d, yy_did_buffer_switch_on_eof = %d\n",
           yyg->yy_init, yyg->yy_start, yyg->yy_did_buffer_switch_on_eof);
    csound->DebugMsg(csound,"yy_start_stack_ptr = %d, yy_start_stack_depth = %d, yy_start_stack = %p\n",
           yyg->yy_start_stack_ptr, yyg->yy_start_stack_depth, yyg->yy_start_stack);

    csound->DebugMsg(csound,"yy_last_accepting_state = %d, yy_last_accepting_cpos = %p %c\n",
           yyg->yy_last_accepting_state, yyg->yy_last_accepting_cpos, *yyg->yy_last_accepting_cpos);
    csound->DebugMsg(csound,"yylineno_r = %d, yy_flex_debug_r = %d, yytext_r = %p \"%s\", yy_more_flag = %d, yy_more_len = %d\n",
           yyg->yylineno_r, yyg->yy_flex_debug_r, yyg->yytext_r, yyg->yytext_r, yyg->yy_more_flag, yyg->yy_more_len);
    csound->DebugMsg(csound,"*********\n");
}
#endif


