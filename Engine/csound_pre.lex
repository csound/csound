%{

 /*
    csound_pre.l:

    Copyright (C) 2011
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
#define YY_DECL int yylex (CSOUND *csound, yyscan_t yyscanner)
void comment(yyscan_t);
void do_comment(yyscan_t);
void do_include(CSOUND *, int, yyscan_t);
void do_macro_arg(CSOUND *, char *, yyscan_t);
void do_macro(CSOUND *, char *, yyscan_t);
void do_umacro(CSOUND *, char *, yyscan_t);
void do_ifdef(CSOUND *, char *, yyscan_t);
void do_ifdef_skip_code(CSOUND *, yyscan_t);
 static void print_csound_predata(CSOUND *,char *,yyscan_t);
void csound_pre_line(CORFIL*, yyscan_t);

#include "parse_param.h"

#define YY_EXTRA_TYPE  PRE_PARM *
#define PARM    yyget_extra(yyscanner)

#define YY_USER_INIT {csound_pre_scan_string(csound->orchstr->body, yyscanner); \
    csound_preset_lineno(csound->orcLineOffset, yyscanner); yyg->yy_flex_debug_r=1;}
%}
%option reentrant
%option noyywrap
%option prefix="csound_pre"
%option outfile="Engine/csound_prelex.c"
%option stdout

WHITE           ^[ \t]*
NEWLINE         (\n|\r\n?)
STRCONST        \"(\\.|[^\"])*\"
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
END             #end(if)?[ \t]*(;.*)?\n
CONT            \\[ \t]*(;.*)?\n

%x incl
%x macro
%x umacro
%x ifdef
%x xstr

%%

{CONT}          { csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                }
{NEWLINE}       { csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                  corfile_putc('\n', csound->expanded_orc); 
                  csound_pre_line(csound->expanded_orc, yyscanner);
                }
"//"            { comment(yyscanner);
                  corfile_putc('\n', csound->expanded_orc); 
                  csound_pre_line(csound->expanded_orc, yyscanner);
                }
";"             { comment(yyscanner); 
                  corfile_putc('\n', csound->expanded_orc); 
                  csound_pre_line(csound->expanded_orc, yyscanner);
                  //corfile_putline(csound_preget_lineno(yyscanner), csound->expanded_orc);
                }
{STCOM}         { do_comment(yyscanner); }
{STRCONST}      { corfile_puts(yytext, csound->expanded_orc); }
{MACRONAME}     {
                   MACRO     *mm, *mfound;
                   int       i, len, mlen;
                   //print_csound_predata(csound, "Macro call", yyscanner);
                   len = strlen(yytext)-1;
                   mlen = 0;
                   for (i=len; i>0; i--) { /* Find the definition */
                     mm = PARM->macros;
                     while (mm != NULL) {
                       if (!(strncmp(yytext+1, mm->name, i))) {
                         mfound = mm;
                         mlen = i;
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
                   /* print_csound_predata(csound, "macro found", yyscanner); */
                   /* ??fiddle with buffers I guess */
                   if (UNLIKELY(PARM->macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                     csound->Die(csound, Str("Includes nested too deeply"));
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr].line =
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   /* csound->DebugMsg(csound,"Push %p macro stack; new body #%s#\n", */
                   /*           PARM->macros, mm->body); */
                   /* csound->DebugMsg(csound,"Push buffer %p -> ", YY_CURRENT_BUFFER); */
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_preset_lineno(1, yyscanner);
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
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_preset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
                   yy_scan_string(mm->body, yyscanner);
                   csound->DebugMsg(csound,"%p\n", YY_CURRENT_BUFFER);
                 }
{MACRONAMEA}    {
                   MACRO     *mm = PARM->macros;
                   int c, i, j;
                   /* csound->DebugMsg(csound,"Macro with arguments call %s\n", yytext); */
                   yytext[yyleng-1] = '\0';
                   while (mm != NULL) {  /* Find the definition */
                     csound->DebugMsg(csound,"Check %s against %s\n", yytext+1, mm->name);
                     if (!(strcmp(yytext+1, mm->name)))
                       break;
                     mm = mm->next;
                   }
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     csound->LongJmp(csound, 1);
                   }
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
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr].s = NULL;
                   csound->DebugMsg(csound,"Push %p macro stack\n",PARM->macros);
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   csound_preset_lineno(1, yyscanner);
                   PARM->lstack[++PARM->depth] =
                     (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
                   yy_scan_string(mm->body, yyscanner);
                 }
{MACRONAMEDA}    {
                   MACRO     *mm = PARM->macros;
                   int c, i, j;
                   /* csound->DebugMsg(csound,"Macro with arguments call %s\n", yytext); */
                   yytext[yyleng-2] = '\0';
                   while (mm != NULL) {  /* Find the definition */
                     csound->DebugMsg(csound,"Check %s against %s\n", yytext+1, mm->name);
                     if (!(strcmp(yytext+1, mm->name)))
                       break;
                     mm = mm->next;
                   }
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     csound->LongJmp(csound, 1);
                   }
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
                     csound_preget_lineno(yyscanner);
                   PARM->alt_stack[PARM->macro_stack_ptr].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   if (PARM->depth++>1024) {
                     csound->Die(csound, Str("Includes nested too deeply"));
                   }
                   csound_preset_lineno(1, yyscanner);
                   PARM->lstack[PARM->depth] = 
                     (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
                   yy_scan_string(mm->body, yyscanner);
                 }
{INCLUDE}       BEGIN(incl);
<incl>[ \t]*     /* eat the whitespace */
<incl>.         { /* got the include file name */
                  do_include(csound, yytext[0], yyscanner);
                  BEGIN(INITIAL);
                }
#exit           { corfile_putc('\0', csound->expanded_orc);
                  corfile_putc('\0', csound->expanded_orc);
                  return 0;}
<<EOF>>         {
                  MACRO *x, *y=NULL;
                  int n;
                  csound->DebugMsg(csound,"*********Leaving buffer %p\n", YY_CURRENT_BUFFER);
                  yypop_buffer_state(yyscanner);
                  PARM->depth--;
                  PARM->llocn = PARM->locn; PARM->locn = make_location(PARM);
                  csound->DebugMsg(csound,"%s(%d): loc=%d; lastloc=%d\n", __FILE__, __LINE__,
                         PARM->llocn, PARM->locn);
                  if ( !YY_CURRENT_BUFFER ) yyterminate();
                  /* csound->DebugMsg(csound,"End of input; popping to %p\n", */
                  /*         YY_CURRENT_BUFFER); */
                  csound_pre_line(csound->expanded_orc, yyscanner);
                  if (PARM->clearBufferAfterEOF)
                    PARM->clearBufferAfterEOF =
                      PARM->isInclude = 0;
                  n = PARM->alt_stack[--PARM->macro_stack_ptr].n;
                  csound_preset_lineno(PARM->alt_stack[PARM->macro_stack_ptr].line,
                                       yyscanner);
                  csound->DebugMsg(csound,"%s(%d): line now %d at %d\n", __FILE__, __LINE__,
                         csound_preget_lineno(yyscanner), PARM->macro_stack_ptr);
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
                  /* csound->DebugMsg(csound,"End of input segment: macro pop %p -> %p\n", */
                  /*            y, PARM->macros); */
                  csound_pre_line(csound->orchstr, yyscanner);
                }
{DEFINE}       BEGIN(macro);
<macro>[ \t]*    /* eat the whitespace */
<macro>{MACRO}  {
                  yytext[yyleng-1] = '\0';
                  /* csound->DebugMsg(csound,"Define macro with args %s\n", yytext); */
                  /* print_csound_predata(csound, "Before do_macro_arg", yyscanner); */
                  do_macro_arg(csound, yytext, yyscanner);
                  //print_csound_predata(csound,"After do_macro_arg", yyscanner);
                  BEGIN(INITIAL);
                }
<macro>{IDENTN} {
                  /* csound->DebugMsg(csound,"Define macro %s\n", yytext); */
                  /* print_csound_predata(csound,"Before do_macro", yyscanner); */
                  do_macro(csound, yytext, yyscanner);
                  //print_csound_predata(csound,"After do_macro", yyscanner);
                  BEGIN(INITIAL);
                }
{UNDEF}        BEGIN(umacro);
<umacro>[ \t]*    /* eat the whitespace */
<umacro>{IDENT}  {
                  /* csound->DebugMsg(csound,"Undefine macro %s\n", yytext); */
                  do_umacro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }

{IFDEF}         {
                  PARM->isIfndef = (yytext[3] == 'n');  /* #ifdef or #ifndef */
                  csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                  corfile_putc('\n', csound->expanded_orc);
                  csound_pre_line(csound->expanded_orc, yyscanner);
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
                  csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                  corfile_putc('\n', csound->expanded_orc);
                  csound_pre_line(csound->expanded_orc, yyscanner);
                  do_ifdef_skip_code(csound, yyscanner);
                }
{END}           {
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
.               { corfile_putc(yytext[0], csound->expanded_orc); }

%%
void comment(yyscan_t yyscanner)              /* Skip until nextline */
{
    char c;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    while ((c = input(yyscanner)) != '\n' && c != '\r'); /* skip */
    if (c == '\r' && (c = input(yyscanner)) != '\n')
      unput(c);
    csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
}

void do_comment(yyscan_t yyscanner)              /* Skip until * and / chars */
{
    char c;
    for (;;) {
      c = input(yyscanner);
      if (UNLIKELY(c=='\n')) /* skip */
          csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
      if (c != '*') continue;
      while ((c=input(yyscanner))=='*');
      if (c=='/') return;
      if (UNLIKELY(c=='\n'))
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
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
    csound->DebugMsg(csound,"line %d at end of #include line\n", csound_preget_lineno(yyscanner));
    {
      uint8_t n = file_to_int(csound, buffer);
      char bb[16];
      PARM->lstack[PARM->depth] = n;
      sprintf(bb, "#source %d\n", PARM->locn = make_location(PARM));
      PARM->llocn = PARM->locn;
      corfile_puts(bb, csound->expanded_orc);
    }
    cf = copy_to_corefile(csound, buffer, "INCDIR", 0);
    csound->DebugMsg(csound,"%s(%d): stacking line %d at %d\n", __FILE__, __LINE__,
           csound_preget_lineno(yyscanner),PARM->macro_stack_ptr);
    PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
    PARM->alt_stack[PARM->macro_stack_ptr].line = csound_preget_lineno(yyscanner);
    PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
    PARM->isInclude = PARM->clearBufferAfterEOF = 1;
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
      mm->body[i++] = c;
      if (UNLIKELY(i >= size))
        mm->body = mrealloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = mrealloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n')) {
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
      mm->body[i++] = c;
      if (UNLIKELY(i >= size))
        mm->body = mrealloc(csound, mm->body, size += 100);
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size))
          mm->body = mrealloc(csound, mm->body, size += 100);
      }
      if (UNLIKELY(c == '\n')) {
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
        corfile_putc('\n', csound->expanded_orc);
        csound_pre_line(csound->expanded_orc, yyscanner);
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
    while ((c=input(yyscanner)) != '\n' && c != EOF); /* ignore rest of line */
    csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
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
    while (c != '\n' && c != EOF) c = input(yyscanner);
}

static void add_math_const_macro(CSOUND *csound, PRE_PARM* qq,
                                 char * name, char *body)
{
    MACRO *mm;

    mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
    mm->name = (char*) mcalloc(csound, strlen(name) + 3);
    sprintf(mm->name, "M_%s", name);
    mm->next = qq->macros;
    qq->macros = mm;
    mm->margs = MARGS;    /* Initial size */
    mm->acnt = 0;
    mm->body = (char*) mcalloc(csound, strlen(body) + 1);
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
     add_math_const_macro(csound, qq, "INF",   "2147483647.0"); /* ~7 years */
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

/* int csound_prewrap(void *yyscanner)  */
/* {  */
/*     return 1; */
/* } */

void csound_pre_line(CORFIL* cf, void *yyscanner)
{
    int n = csound_preget_lineno(yyscanner);
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
        sprintf(bb, "#line %d\n", n);
        corfile_puts(bb, cf);
      }
    }
    PARM->line = n;
}

#ifdef MAIN_NEEDED
int main(void)
{
    PRE_PARM  qq;
    int len=100, p=0, n;
    char buff[1024];
    FILE *fd = fopen("III", "r");

    inp = (char*)calloc(100,1);
    memset(buff, '\0', 1024);
    while ((n = fread(buff, 1, 1023, fd))) {
      while (p+n+1>=len)
        inp = (char*) realloc(inp, len+=100);
      strcat(inp, buff);
      p += n;
      memset(buff, '\0', 1024);
    }
    if (n+8>= len) inp = (char*) realloc(inp, len = n+9);
    strcat(inp, "\n#exit\0\0");
    rewind(fd);

    memset(&qq, '\0', sizeof(PRE_PARM));
    csound_prelex_init(&qq.yyscanner);
    csound_preset_extra(&qq, qq.yyscanner);
    //csound_preset_debug(1, &qq.yyscanner);
    //csound_prepush_buffer_state(NULL, &qq.yyscanner);
    //csound_pre_scan_string(inp, qq.yyscanner);
    //csound_preset_in(NULL, qq.yyscanner);
    qq.line = 1;
    csound_prelex(NULL, qq.yyscanner);
    //csound->DebugMsg(csound,corfile_body(expanded_pre));
    //csound_prelex_destroy(&qq.yyscanner);
    return 0;
}
#endif

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


