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

#define YY_EXTRA_TYPE  PRE_PARM *
#define PARM    yyget_extra(yyscanner)

#include "parse_param.h"

#define YY_USER_INIT csound_pre_scan_string(csound->orchstr->body, yyscanner); yy_flex_debug=1;
%}
%option reentrant
%option prefix="csound_pre"

WHITE           ^[ \t]*
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
IDENTN          [a-zA-Z0-9_]+
MACRONAME       "$"[a-zA-Z0-9_]+
MACRONAMED      "$"[a-zA-Z0-9_]+\.
MACRONAMEA      "$"[a-zA-Z0-9_]+\(
MACRONAMEDA     "$"[a-zA-Z0-9_]+\.\(
MACRO           [a-zA-Z0-9_]+\(

STCOM           \/\*
INCLUDE         "#include"
DEFINE          "#define"
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

"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */

{CONT}          { csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                }
"\n"            { csound_preset_lineno(1+csound_preget_lineno(yyscanner),
                                       yyscanner);
                  //corfile_putline(csound_preget_lineno(yyscanner), PARM->cf); 
                }
"//"            { comment(yyscanner);
                  //corfile_putline(csound_preget_lineno(yyscanner), PARM->cf);
                }
";"             { comment(yyscanner); 
                  //corfile_putline(csound_preget_lineno(yyscanner), PARM->cf);
                }
{STCOM}         { do_comment(yyscanner); }
{MACRONAME}     {
                   MACRO     *mm = PARM->macros;
                   while (mm != NULL) {  /* Find the definition */
                     if (!(strcmp(yytext+1, mm->name)))
                       break;
                     mm = mm->next;
                   }
                   if (UNLIKELY(mm == NULL)) {
                     fprintf(stderr, "Undefined macro: '%s'", yytext);
                     exit(1);
                     //csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     //csound->LongJmp(csound, 1);
                   }
                   /* Need to read from macro definition */
                   /* ??fiddle with buffers I guess */
                   if (UNLIKELY(PARM->macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                     fprintf(stderr, "Includes nested too deeply");
                     //csound->Message(csound, Str("Includes nested too deeply"));
                     exit(1);
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   /* fprintf(stderr,"Push %p macro stack; new body #%s#\n",
                             PARM->macros, mm->body); */
                   /* fprintf(stderr,"Push buffer %p -> ", YY_CURRENT_BUFFER); */
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   yy_scan_string(mm->body, yyscanner);
                   /* fprintf(stderr,"%p\n", YY_CURRENT_BUFFER); */
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
                     fprintf(stderr, "Undefined macro: '%s'", yytext);
                     exit(3);
                     //csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     //csound->LongJmp(csound, 1);
                   }
                   /* Need to read from macro definition */
                   /* ??fiddle with buffers I guess */
                   if (UNLIKELY(PARM->macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                     fprintf(stderr, "Includes nested too deeply");
                     //csound->Message(csound, Str("Includes nested too deeply"));
                     exit(1);
                   }
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   /* fprintf(stderr,"Push buffer %p -> ", YY_CURRENBUFFER_TOKEN); */
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   yy_scan_string(mm->body, yyscanner);
                   /* fprintf(stderr,"%p\n", YY_CURRENT_BUFFER); */
                 }
{MACRONAMEA}    {
                   MACRO     *mm = PARM->macros;
                   int c, i, j;
                   fprintf(stderr,"Macro with arguments call %s\n", yytext);
                   yytext[yyleng-1] = '\0';
                   while (mm != NULL) {  /* Find the definition */
                     /* fprintf(stderr,"Check %s against %s\n", yytext+1, mm->name); */
                     if (!(strcmp(yytext+1, mm->name)))
                       break;
                     mm = mm->next;
                   }
                   if (UNLIKELY(mm == NULL)) {
                     fprintf(stderr,"Undefined macro: '%s'", yytext);
                     exit(5);
                     //csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     //csound->LongJmp(csound, 1);
                   }
                   /* Need to read from macro definition */
                   /* fprintf(stderr,"Looking for %d args\n", mm->acnt); */
                   for (j = 0; j < mm->acnt; j++) {
                     char  term = (j == mm->acnt - 1 ? ')' : '\'');
 /* Compatability */
                     char  trm1 = (j == mm->acnt - 1 ? ')' : '#');
                     MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                     int   size = 100;
                     nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                     /* fprintf(stderr,"Arg %d: %s\n", j+1, mm->arg[j]); */
                     strcpy(nn->name, mm->arg[j]);
                     /* csound->Message(csound, "defining argument %s ",
                                        nn->name); */
                     i = 0;
                     nn->body = (char*) mmalloc(csound, 100);
                     while ((c = input(yyscanner))!= term && c!=trm1) {
                       if (UNLIKELY(i > 98)) {
                         //csound->Die(csound,
                         //            Str("Missing argument terminator\n%.98s"),
                         //            nn->body);
                         fprintf(stderr, "Missing argument terminator\n%.98s", nn->body);
                         exit(9);
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
                   /* fprintf(stderr,"New body: ...#%s#\n", mm->body); */
                   PARM->alt_stack[PARM->macro_stack_ptr].n = mm->acnt;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   /* fprintf(stderr,"Push %p macro stack\n",PARM->macros); */
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   yy_scan_string(mm->body, yyscanner);
                 }
{MACRONAMEDA}    {
                   MACRO     *mm = PARM->macros;
                   int c, i, j;
                   fprintf(stderr,"Macro with arguments call %s\n", yytext);
                   yytext[yyleng-2] = '\0';
                   while (mm != NULL) {  /* Find the definition */
                     /* fprintf(stderr,"Check %s against %s\n", yytext+1, mm->name); */
                     if (!(strcmp(yytext+1, mm->name)))
                       break;
                     mm = mm->next;
                   }
                   if (UNLIKELY(mm == NULL)) {
                     fprintf(stderr, "Undefined macro: '%s'", yytext);
                     exit(6);
                     //csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     //csound->LongJmp(csound, 1);
                   }
                   /* Need to read from macro definition */
                   /* fprintf(stderr,"Looking for %d args\n", mm->acnt); */
                   for (j = 0; j < mm->acnt; j++) {
                     char  term = (j == mm->acnt - 1 ? ')' : '\'');
 /* Compatability */
                     char  trm1 = (j == mm->acnt - 1 ? ')' : '#');
                     MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                     int   size = 100;
                     nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                     /* fprintf(stderr,"Arg %d: %s\n", j+1, mm->arg[j]); */
                     strcpy(nn->name, mm->arg[j]);
                     /* csound->Message(csound, "defining argument %s ",
                                        nn->name); */
                     i = 0;
                     nn->body = (char*) mmalloc(csound, 100);
                     while ((c = input(yyscanner))!= term && c!=trm1) {
                       if (UNLIKELY(i > 98)) {
                         fprintf(stderr, "Missing argument terminator\n%.98s", nn->body);
                         exit(11);
                         //csound->Die(csound,
                         //            Str("Missing argument terminator\n%.98s"),
                         //            nn->body);
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
                   /* fprintf(stderr,"New body: ...#%s#\n", mm->body); */
                   PARM->alt_stack[PARM->macro_stack_ptr].n = mm->acnt;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
                   PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
                   PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
                   yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                   yy_scan_string(mm->body, yyscanner);
                 }
{INCLUDE}       BEGIN(incl);
<incl>[ \t]*     /* eat the whitespace */
<incl>.         { /* got the include file name */
                  do_include(csound, yytext[0], yyscanner);
                  BEGIN(INITIAL);
                }
#exit           { return 0;}
<<EOF>>         {
                  MACRO *x, *y;
                  int n;
                  fprintf(stderr,"*********Leaving buffer %p\n", YY_CURRENT_BUFFER);
                  yypop_buffer_state(yyscanner);
                  if ( !YY_CURRENT_BUFFER ) yyterminate();
                  fprintf(stderr,"End of input; popping to %p\n",
                          YY_CURRENT_BUFFER);
                  if (PARM->clearBufferAfterEOF)
                    PARM->clearBufferAfterEOF =
                      PARM->isInclude = 0;
                  n = PARM->alt_stack[--PARM->macro_stack_ptr].n;
                  fprintf(stderr,"n=%d\n", n);
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
                  fprintf(stderr,"End of input segment: macro pop %p -> %p\n",
                             y, PARM->macros);
                }
{DEFINE}       BEGIN(macro);
<macro>[ \t]*    /* eat the whitespace */
<macro>{MACRO}  {
                  yytext[yyleng-1] = '\0';
                  fprintf(stderr,"Define macro with args %s\n", yytext);
                  do_macro_arg(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }
<macro>{IDENTN} {
                  fprintf(stderr,"Define macro %s\n", yytext);
                  do_macro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }
{UNDEF}        BEGIN(umacro);
<umacro>[ \t]*    /* eat the whitespace */
<umacro>{IDENT}  {
                  fprintf(stderr,"Undefine macro %s\n", yytext);
                  do_umacro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
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
                    fprintf(stderr, "#else without #if\n");
                    exit(7);
                    //csound->Message(csound, Str("#else without #if\n"));
                    //csound->LongJmp(csound, 1); 
                  }
                  else if (PARM->ifdefStack->isElse) {
                    fprintf(stderr, "#else after #else\n");
                    exit(8);
                    //csound->Message(csound, Str("#else after #else\n"));
                    //csound->LongJmp(csound, 1);
                  }
                  PARM->ifdefStack->isElse = 1;
                  do_ifdef_skip_code(csound, yyscanner);
                }
{END}           {
                  IFDEFSTACK *pp = PARM->ifdefStack;
                  if (UNLIKELY(pp == NULL)) {
                    fprintf(stderr, "Unmatched #end\n");
                    exit(9);
                    //csound->Message(csound, Str("Unmatched #end\n"));
                    //csound->LongJmp(csound, 1);
                  }
                  PARM->ifdefStack = pp->prv;
                  mfree(csound, pp);
                }
.               { corfile_putc(yytext[0], PARM->cf); }

%%
void comment(yyscan_t yyscanner)              /* Skip until nextline */
{
    char c;

    while ((c = input(yyscanner)) != '\n'); /* skip */
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
    while ((c=input(yyscanner))!=term) {
      buffer[p] = c;
      p++;
    }
    buffer[p] = '\0';
    while ((c=input(yyscanner))!='\n');
    return; /* ***************************************** */
    cf = copy_to_corefile(csound, buffer, "INCDIR", 0);
    PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
    PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
    PARM->isInclude = PARM->clearBufferAfterEOF = 1;
    csound_prepush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
    csound_pre_scan_string(cf->body, yyscanner);
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
      fprintf(stderr, "macro error\n");
      //csound->Message(csound, Str("macro error\n"));
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
      if (UNLIKELY(c == '\n'))
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
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
    //    fprintf(stderr,"Macro definition for %s\n", name0);
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
        csound_preset_lineno(1+csound_preget_lineno(yyscanner),yyscanner);
    }
    mm->body[i] = '\0';
    //    fprintf(stderr,"Body #%s#\n", mm->body);
    mm->next = PARM->macros;
    PARM->macros = mm;
}

void do_umacro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int i,c;
    //if (UNLIKELY(csound->oparms->msglevel))
    //csound->Message(csound,Str("macro %s undefined\n"), name0);
    fprintf(stderr, "macro %s undefined\n", name0);
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
          fprintf(stderr, "Undefining undefined macro");
          exit(12);
          //csound->Message(csound, Str("Undefining undefined macro"));
          //csound->LongJmp(csound, 1);
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
          fprintf(stderr, "Unmatched #if%sdef\n", PARM->isIfndef ? "n" : "");
          //csound->Message(csound, Str("Unmatched #if%sdef\n"),
          //                PARM->isIfndef ? "n" : "");
          //csound->LongJmp(csound, 1);
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
            fprintf(stderr, "#else after #else\n");
            exit(14);
            //csound->Message(csound, Str("#else after #else\n"));
            //csound->LongJmp(csound, 1);
          }
          pp->isElse = 1;
          break;
        }
      }
    }
    free(buf);
    while (c != '\n' && c != EOF) c = input(yyscanner);
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

#if 0
void cs_init_omacros(CSOUND *csound, void *yyscanner, NAMES *nn)
{
    while (nn) {
      char  *s = nn->mac;
      char  *p = strchr(s, '=');
      char  *mname;
      MACRO *mm;

      if (p == NULL)
        p = s + strlen(s);
      //      if (csound->oparms->msglevel & 7)
      // csound->Message(csound, Str("Macro definition for %*s\n"), p - s, s);
      fprintf(strderr, "Macro definition for %*s\n", p - s, s);
      s = strchr(s, ':') + 1;                   /* skip arg bit */
      if (UNLIKELY(s == NULL || s >= p)) {
        //csound->Die(csound, Str("Invalid macro name for --omacro"));
        fprintf(stderr, "Invalid macro name for --omacro");
        exit(2);
      }
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
#endif


int csound_prewrap(yyscan_t yyscanner) { return 0;}
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
    csound_prelex(NULL, &qq.yyscanner);
    //fprintf(stderr,corfile_body(expanded_pre));
    //csound_prelex_destroy(&qq.yyscanner);
    return 0;
}
#endif
