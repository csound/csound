%{

 /*
    csound_prs.lex:

    Copyright (C) 2011, 2016
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
#include "corfile.h"
#define YY_DECL int yylex (CSOUND *csound, yyscan_t yyscanner)
static void comment(yyscan_t);
static void do_comment(yyscan_t);
static void do_include(CSOUND *, int, yyscan_t);
static void do_new_include(CSOUND *, yyscan_t);
extern int isDir(char *);
static void do_macro_arg(CSOUND *, char *, yyscan_t);
static void do_macro(CSOUND *, char *, yyscan_t);
static void do_umacro(CSOUND *, char *, yyscan_t);
static void do_ifdef(CSOUND *, char *, yyscan_t);
static void do_ifdef_skip_code(CSOUND *, yyscan_t);
//static void print_csound_prsdata(CSOUND *,char *,yyscan_t);
static void csound_prs_line(CORFIL*, yyscan_t);
static void delete_macros(CSOUND*, yyscan_t);
static int extract_int(CSOUND*, yyscan_t, int*);
static int on_EOF(CSOUND*, yyscan_t);
#define MACDEBUG 1

#define S_INC (10)
static inline int isNameChar(int cc, int pos)
{
    unsigned char c = ((unsigned char) cc);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

#include "score_param.h"

static void expand_macro(CSOUND*, MACRO*, yyscan_t);
static void expand_macroa(CSOUND*, MACRO*, yyscan_t);
//static void trace_alt_stack(CSOUND*, PRS_PARM*, int);

#define YY_EXTRA_TYPE  PRS_PARM *
#define PARM    yyget_extra(yyscanner)

#define YY_USER_INIT {csound_prs_scan_string(csound->scorestr->body, yyscanner); \
    csound_prsset_lineno(csound->scoLineOffset, yyscanner);             \
    /* yyg->yy_flex_debug_r=1;*/                                        \
    PARM->macro_stack_size = 0;                                         \
    PARM->alt_stack = NULL; PARM->macro_stack_ptr = 0;                  \
    PARM->path = ".";                                                   \
    PARM->cf = csound->expanded_sco;                                    \
  }
static MACRO *find_definition(MACRO *, char *);
%}
%option reentrant
%option noyywrap
%option prefix="csound_prs"
%option outfile="Engine/csound_prslex.c"
%option stdout

NEWLINE         (\n|\r\n?)
STSTR           \"
ESCAPE          \\.
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
MACRONAME       "$"`?[a-zA-Z_][a-zA-Z0-9_`]*
MACRONAMED      "$"`?[a-zA-Z_][a-zA-Z0-9_`]*\.
MACRONAMEA      "$"`?[a-zA-Z_][a-zA-Z0-9_`]*\(
MACRONAMEDA     "$"`?[a-zA-Z_][a-zA-Z0-9_`]*\.\(
MACROB          [a-zA-Z_][a-zA-Z0-9_]*\(
MACRO           [a-zA-Z_][a-zA-Z0-9_]*
NUMBER [0-9]+\.?[0-9]*([eE][-+]?[0-9]+)?|\.[0-9]+([eE][-+]?[0-9]+)?|0[xX][0-9a-fA-F]+

STCOM           \/\*
INCLUDE         "#include"
INCLUDESTR      "#includestr"
DEFINE          #[ \t]*define
UNDEF           "#undef"
IFDEF           #ifn?def
ELSE            #else[ \t]*(;.*)?$
END             #end(if)?[ \t]*(;.*)?(\n|\r\n?)
LOOP            #loop
EXIT            #exit
CONT            \\[ \t]*(;.*)?(\n|\r\n?)
SEND            ^[ \t]*[es]
ROP             ^[ \t]*r

NP              np[0-9]+
NM              [nm][ \t]+

%X incl
%x macro
%x umacro
%x ifdef
%x lname

%%

{CONT}          {
#if 0
                  char bb[80];
#endif
                  csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                                       yyscanner);
#if 0
                  if (PARM->isString==0) {
                    sprintf(bb, "#sline %d ", csound_prsget_lineno(yyscanner));
                    corfile_puts(csound, bb, PARM->cf);
                  }
#endif
                }
{NEWLINE}       {
                  corfile_putc(csound, '\n', PARM->cf);
                  csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                                       yyscanner);
                  csound_prs_line(PARM->cf, yyscanner);
                }
"//"            {
                  if (PARM->isString != 1) {
                    comment(yyscanner);
                    corfile_putc(csound, '\n', PARM->cf);
                    csound_prs_line(PARM->cf, yyscanner);
                  }
                  else {
                    corfile_puts(csound, yytext, PARM->cf);
                  }
                }
";"             {
                  if (PARM->isString != 1) {
                    comment(yyscanner);
                    corfile_putc(csound, '\n', PARM->cf);
                    csound_prs_line(PARM->cf, yyscanner);
                  }
                  else {
                    corfile_puts(csound, yytext, PARM->cf);
                  }
                }
"c"             {
                  if (PARM->isString != 1) {
                    comment(yyscanner);
                    corfile_putc(csound, '\n', PARM->cf);
                    csound_prs_line(PARM->cf, yyscanner);
                  }
                  else {
                    corfile_puts(csound, yytext, PARM->cf);
                  }
                }
{STCOM}         {
                  if (PARM->isString != 1)
                    do_comment(yyscanner);
                  else
                    corfile_puts(csound, yytext, PARM->cf);
                }
{ESCAPE}        { corfile_puts(csound, yytext, PARM->cf); }
{STSTR}         {
                  corfile_putc(csound, '"', PARM->cf);
                  //printf("string start/end: >>%s<<\n", PARM->cf->body);
                  PARM->isString = !PARM->isString;
                  if (PARM->isinclude && PARM->isString==0) {
                    do_new_include(csound, yyscanner);
                    PARM->isinclude = 0;
                  }
                }
{MACRONAME}|{MACRONAMED}     {
                   MACRO     *mm = PARM->macros;
                   mm = find_definition(mm, yytext+1);
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     //csound->LongJmp(csound, 1);
                     corfile_puts(csound, "$error", PARM->cf);
                   }
                   else {
                     expand_macro(csound, mm, yyscanner);
                   }
                }
{MACRONAMEA}|{MACRONAMEDA}    {
                   MACRO     *mm = PARM->macros;
                   //int err =0;
                   //char      *mname;
                   //int c, i, j, cnt=0;
                   //csound->DebugMsg(csound,"Macro with arguments call %s\n",
                   //                 yytext);
                   yytext[yyleng-1] = '\0';
                   mm = find_definition(mm, yytext+1);
                   if (UNLIKELY(mm == NULL)) {
                     csound->Message(csound,Str("Undefined macro: '%s'"), yytext);
                     corfile_puts(csound, "$error", PARM->cf);
                     //csound->LongJmp(csound, 1);
                   }
                   else {
                     expand_macroa(csound, mm, yyscanner);
                   }
                }
{INCLUDESTR}    {
                  if (PARM->isString != 1)
                    PARM->isinclude = 1;
                  else
                    corfile_puts(csound, yytext, csound->expanded_orc);
                }
{INCLUDE}       {
                  if (PARM->isString != 1)
                    BEGIN(incl);
                  else
                    corfile_puts(csound, yytext, PARM->cf);
                }
<incl>[ \t]*     /* eat the whitespace */
<incl>.         { /* got the include file name */
                  do_include(csound, yytext[0], yyscanner);
                  BEGIN(INITIAL);
                }
{NP}            { // special case for np3 type carries to avoid treatment as n
                 corfile_puts(csound, yytext, PARM->cf);
                }
{EXIT}          {
                 //printf("exit found: >>>%s<<<\n", PARM->cf->body);
                  corfile_putc(csound, '\0', PARM->cf);
                  corfile_putc(csound, '\0', PARM->cf);
                  delete_macros(csound, yyscanner);
                  return 0;
                }
<<EOF>>         {
                  if (on_EOF(csound, yyscanner)==0) yyterminate();
                }
{DEFINE}        {
                  if (PARM->isString != 1)
                    BEGIN(macro);
                  else
                    corfile_puts(csound, yytext, PARM->cf);
                }
<macro>[ \t]*    /* eat the whitespace */
<macro>{MACROB} {
                  yytext[yyleng-1] = '\0';
                  csound->DebugMsg(csound,"Define macro with args %s\n",
                                      yytext);
                  /* print_csound_prsdata(csound, "Before do_macro_arg",
                                          yyscanner); */
                  do_macro_arg(csound, yytext, yyscanner);
                  //print_csound_prsdata(csound,"After do_macro_arg", yyscanner);
                  BEGIN(INITIAL);
                }
<macro>{MACRO} {
                  csound->DebugMsg(csound,"Define macro %s\n", yytext);
                  /* print_csound_prsdata(csound,"Before do_macro", yyscanner); */
                  do_macro(csound, yytext, yyscanner);
                  //print_csound_prsdata(csound,"After do_macro", yyscanner);
                  BEGIN(INITIAL);
                }
<macro>.        { csound->Message(csound,
                                  Str("Unexpected character %c(%.2x) line %d\n"),
                                  yytext[0], yytext[0],
                                  csound_prsget_lineno(yyscanner));
                  csound->LongJmp(csound, 1);
                }
{UNDEF}         {
                  if (PARM->isString != 1)
                    BEGIN(umacro);
                  else
                    corfile_puts(csound, yytext, PARM->cf);
                }
<umacro>[ \t]*    /* eat the whitespace */
<umacro>{MACRO}  {
                  csound->DebugMsg(csound,"Undefine macro %s\n", yytext);
                  do_umacro(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }

{IFDEF}         {
                  if (PARM->isString != 1) {
                    PARM->isIfndef = (yytext[3] == 'n');  /* #ifdef or #ifndef */
                    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                                         yyscanner);
                    corfile_putc(csound, '\n', PARM->cf);
                    csound_prs_line(PARM->cf, yyscanner);
                    BEGIN(ifdef);
                  }
                  else {
                    corfile_puts(csound, yytext, PARM->cf);
                  }
                }
<ifdef>[ \t]*     /* eat the whitespace */
<ifdef>{IDENT}  {
                  do_ifdef(csound, yytext, yyscanner);
                  BEGIN(INITIAL);
                }
{ELSE}          {
                  if (PARM->isString != 1) {
                    if (UNLIKELY(PARM->ifdefStack == NULL)) {
                      csound->Message(csound, Str("#else without #if\n"));
                      csound->LongJmp(csound, 1);
                    }
                    else if (UNLIKELY(PARM->ifdefStack->isElse)) {
                      csound->Message(csound, Str("#else after #else\n"));
                      csound->LongJmp(csound, 1);
                    }
                    PARM->ifdefStack->isElse = 1;
                    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                                         yyscanner);
                    corfile_putc(csound, '\n', PARM->cf);
                    csound_prs_line(PARM->cf, yyscanner);
                    do_ifdef_skip_code(csound, yyscanner);
                  }
                  else {
                    corfile_puts(csound, yytext, PARM->cf);
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
                    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                                         yyscanner);
                    corfile_putc(csound, '\n', PARM->cf);
                    csound_prs_line(PARM->cf, yyscanner);
                    mfree(csound, pp);
                  }
                  else {
                    corfile_puts(csound, yytext, PARM->cf);
                  }
                }
{NM}            {
                  if (PARM->isString) {
                    corfile_putc(csound, yytext[0], PARM->cf);
                    BEGIN(lname);
                  } else {
                    corfile_puts(csound, yytext, PARM->cf);
                  }
                }
<lname>[ \t]*     /* eat the whitespace */
<lname>{IDENT}   {
                  corfile_putc(csound, ' ', PARM->cf);
                  corfile_puts(csound, yytext, PARM->cf);
                  BEGIN(INITIAL);
                }

"{"             {
                  int c=' ', i;
                  PARM->repeat_index++;
                  if (UNLIKELY(PARM->repeat_index >= RPTDEPTH))
                    csound->Die(csound, Str("Loops are nested too deeply"));
                  PARM->repeat_mm_n[PARM->repeat_index] =
                    (MACRO*)csound->Malloc(csound, sizeof(MACRO));
                  if (UNLIKELY(PARM->repeat_mm_n[PARM->repeat_index] == NULL)) {
                    csound->Message(csound, Str("Memory exhausted"));
                    csound->LongJmp(csound, 1);
                  }
                  PARM->repeat_cnt_n[PARM->repeat_index] =
                    extract_int(csound, yyscanner, &c);
                  if (UNLIKELY(PARM->repeat_cnt_n[PARM->repeat_index] <= 0))
                    csound->Die(csound, Str("{: invalid repeat count"));
                  if (PARM->repeat_index > 1) {
                    char st[41];
                    int j;
                    for (j = 0; j < PARM->repeat_index; j++) {
                      st[j] = ' ';
                      st[j+1] = '\0';
                    }
                    if (UNLIKELY(csound->oparms->odebug))
                      csound->Message(csound, Str("%s Nested LOOP=%d Level:%d\n"),
                                      st, PARM->repeat_cnt_n[PARM->repeat_index],
                              PARM->repeat_index);
                  }
                  else {
                    if (UNLIKELY(csound->oparms->odebug))
                      csound->Message(csound, Str("External LOOP=%d Level:%d\n"),
                              PARM->repeat_cnt_n[PARM->repeat_index],
                              PARM->repeat_index);
                  }
                  while (isblank(c)) {
                    c = input(yyscanner);
                  }
                  for (i = 0; isNameChar(c, i) && i < (NAMELEN-1); i++) {
                    PARM->repeat_name_n[PARM->repeat_index][i] = c;
                    c = input(yyscanner);
                  }
                  PARM->repeat_name_n[PARM->repeat_index][i] = '\0';
                  unput(c);
                  /* Define macro for counter */
                  PARM->repeat_mm_n[PARM->repeat_index]->name =
                    csound->Malloc(csound,
                               strlen(PARM->repeat_name_n[PARM->repeat_index])+1);
                  if (UNLIKELY(PARM->repeat_mm_n[PARM->repeat_index]->name==NULL)) {
                    csound->Message(csound, Str("Memory exhausted"));
                    csound->LongJmp(csound, 1);
                  }
                  strcpy(PARM->repeat_mm_n[PARM->repeat_index]->name,
                         PARM->repeat_name_n[PARM->repeat_index]);
                  PARM->repeat_mm_n[PARM->repeat_index]->acnt = -1;
                  PARM->repeat_mm_n[PARM->repeat_index]->body =
                    csound->Calloc(csound, 16); // ensure nulls
                  PARM->repeat_mm_n[PARM->repeat_index]->body[0] = '0';
                  PARM->repeat_indx[PARM->repeat_index] = 0;
                  csound->DebugMsg(csound,"csound_prs(%d): repeat %s zero %p\n",
                                   __LINE__,
                                   PARM->repeat_name_n[PARM->repeat_index],
                                   PARM->repeat_mm_n[PARM->repeat_index]->body);
                  PARM->repeat_mm_n[PARM->repeat_index]->next = PARM->macros;
                  PARM->macros = PARM->repeat_mm_n[PARM->repeat_index];
                  while (input(yyscanner)!='\n') {}
                  PARM->cf_stack[PARM->repeat_index] = PARM->cf;
                  PARM->cf = corfile_create_w(csound);
        }
"}"     {
          int temp;
          CORFIL *bdy = PARM->cf;
          if (UNLIKELY((temp=PARM->repeat_cnt_n[PARM->repeat_index])==0)) {
            csound->Message(csound, Str("unmatched } in score\n"));
            csound->LongJmp(csound, 1);
          }
          corfile_puts(csound, "#loop", PARM->cf);
          corfile_putc(csound, '\0', PARM->cf);
          corfile_putc(csound, '\0', PARM->cf);
          PARM->cf = PARM->cf_stack[PARM->repeat_index];
          //printf("****Repeat body\n>>>%s<<<\n", bdy->body);
          PARM->repeat_mm_n[PARM->repeat_index]->acnt = 0; /* uninhibit */
          if (UNLIKELY(PARM->macro_stack_ptr +1 >= PARM->macro_stack_size )) {
            //trace_alt_stack(csound, PARM, __LINE__);
            PARM->alt_stack =
              (MACRON*)csound->ReAlloc(csound, PARM->alt_stack,
                                   sizeof(MACRON)*(PARM->macro_stack_size+=S_INC));
            if (UNLIKELY(PARM->alt_stack == NULL)) {
              csound->Message(csound, Str("Memory exhausted"));
              csound->LongJmp(csound, 1);
            }
          }
          csound->DebugMsg(csound,"csound_ps(%d): stacking line %d at %d\n",
                           __LINE__,
                           csound_prsget_lineno(yyscanner),PARM->macro_stack_ptr);
          PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
          PARM->alt_stack[PARM->macro_stack_ptr].line =
            csound_prsget_lineno(yyscanner);
          PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
          PARM->depth++;
          csound_prspush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
          csound_prs_scan_string(bdy->body, yyscanner);
          PARM->cf_stack[PARM->repeat_index] = bdy;
       }

{LOOP} {
          //printf("#loop found\n");
          yypop_buffer_state(yyscanner);
          PARM->depth--;
          PARM->llocn = PARM->locn; PARM->locn = make_slocation(PARM);
          /* printf("Loop structures: count = %d, name = %s, indx = %d\n", */
          /*        PARM->repeat_cnt_n[PARM->repeat_index], */
          /*        PARM->repeat_name_n[PARM->repeat_index], */
          /*        PARM->repeat_indx[PARM->repeat_index]); */
          if (++PARM->repeat_indx[PARM->repeat_index] !=
              PARM->repeat_cnt_n[PARM->repeat_index]) {
            snprintf(PARM->repeat_mm_n[PARM->repeat_index]->body, 16, "%d",
                     PARM->repeat_indx[PARM->repeat_index]);
            //printf(">>%s<<\n", PARM->cf_stack[PARM->repeat_index]->body);
            PARM->depth++;
            csound_prspush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
            csound_prs_scan_string(PARM->cf_stack[PARM->repeat_index]->body,
                                   yyscanner);
          }
          else {
            //printf("*** end loop\n");
            //printf(">>%s<<\n", PARM->cf->body);
            corfile_rm(csound, &PARM->cf_stack[PARM->repeat_index]);
            PARM->repeat_index--;
            PARM->macro_stack_ptr--;
          }
       }
{ROP}  {
         if (PARM->isString) corfile_puts(csound, yytext, PARM->cf);
         else {
           int c, i;
           char buff[120];
           //printf("r detected %d\n",PARM->in_repeat_sect );
           if (PARM->in_repeat_sect==1) {
             char *tmp = strdup(yytext);
             // Previous r opcode not terminated so fake an s and redo
             //printf("unterminated r loop\n");
             for (i = yyleng-1; i >= 0; --i)
               unput(tmp[i]);
             unput('\n'); unput('s'); unput('\n');
             free(tmp);
           }
           else {
             PARM->repeat_sect_cnt = 0;
             PARM->in_repeat_sect = 1; /* Mark as recording */
             do {
               c = input(yyscanner);
             } while (isblank(c) || c=='\0');
             while (isdigit(c)) {
               PARM->repeat_sect_cnt =
                 10 * PARM->repeat_sect_cnt + c - '0';
               c = input(yyscanner);
             }
             if (UNLIKELY(PARM->repeat_sect_cnt <= 0
                          || !isspace(c))) {
               csound->Message(csound, Str("r: invalid repeat count %d"),
                               PARM->repeat_sect_cnt);
               csound->LongJmp(csound, 1);
             }
             if (UNLIKELY(csound->oparms->odebug))
               csound->Message(csound, Str("r LOOP=%d\n"), PARM->repeat_sect_cnt);
             while (isblank(c)) {
               c = input(yyscanner);
             }
             if (!isalpha(c)) { //no macro
               //printf("No macro\n");
               PARM->repeat_sect_mm =NULL;
             }
             else {
               for (i = 0; isNameChar(c, i) && i < (NAMELEN-1); i++) {
                 buff[i] = c;
                 c = input(yyscanner);
               }
               PARM->repeat_sect_mm =
                 (MACRO*)csound->Malloc(csound, sizeof(MACRO));
               if (UNLIKELY(PARM->repeat_sect_mm== NULL)) {
                 csound->Message(csound, Str("Memory exhausted"));
                 csound->LongJmp(csound, 1);
               }
               buff[i] = '\0';
               //printf("macro name %s\n", buff);
               /* Define macro for counter */
               PARM->repeat_sect_mm->name = cs_strdup(csound, buff);
               PARM->repeat_sect_mm->acnt = -1; /* inhibit */
               PARM->repeat_sect_mm->body = csound->Calloc(csound, 16);
               PARM->repeat_sect_mm->body[0] = '0';
               csound->DebugMsg(csound,"repeat %s zero %s\n",
                                buff, PARM->repeat_sect_mm->body);
               PARM->repeat_sect_mm->next = PARM->macros; /* add to chain */
               PARM->macros = PARM->repeat_sect_mm;
             }
             unput(c);
             PARM->repeat_sect_line = PARM->line;
             PARM->repeat_sect_index = 0;
             //while (input(yyscanner)!='\n') {}
             PARM->repeat_sect_cf = PARM->cf;
             PARM->cf = corfile_create_w(csound);
           }
         }
        }
{SEND}  {
          if (!PARM->isString) {
            int op = yytext[strlen(yytext)-1];
            if (PARM->in_repeat_sect==1) { // Record mode
              //printf("recording section end %d %c\n>>%s<<\n",
              //       PARM->in_repeat_sect, op, PARM->cf->body);
              corfile_putc(csound, 's', PARM->cf);
              corfile_putc(csound, '\n', PARM->cf);
              while (1) {
                int c = input(yyscanner);
                //printf("copying >>%c<<(%.2x)", c, c);
                if (c=='\n' ||c=='\0') break;
                if (!isblank(c)&&!isdigit(c)&&!strchr(".e+-",c)) { unput(c); break;}
                corfile_putc(csound, c, PARM->cf);
              }
              corfile_putc(csound, '\n', PARM->cf);
              //unput('\n'); unput(op);unput('\n');
              struct yyguts_t *yyg =(struct yyguts_t*)yyscanner;
              PARM->in_repeat_sect=2;
              //printf("****Repeat body\n>>>%s<<<\n", PARM->cf->body);
              if (PARM->repeat_sect_mm)
                PARM->repeat_sect_mm->acnt = 0; /* uninhibit */
              csound_prspush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
              csound_prs_scan_string(PARM->cf->body, yyscanner);
              { CORFIL *tmp = PARM->cf;
                PARM->cf = PARM->repeat_sect_cf;
                PARM->repeat_sect_cf = tmp;
              }
              PARM->line = PARM->repeat_sect_line;
            }
            else if (PARM->in_repeat_sect==2) { // expand loop
              corfile_putc(csound, 's', PARM->cf);
              while (1) {
                int c = input(yyscanner);
                corfile_putc(csound, c, PARM->cf);
                if (c=='\n') break;
              }
              corfile_putc(csound, '\n', PARM->cf);
              yypop_buffer_state(yyscanner);
              PARM->llocn = PARM->locn; PARM->locn = make_slocation(PARM);
              //printf("repeat section %d %d\n",
              //       PARM->repeat_sect_index,PARM->repeat_sect_cnt);
              PARM->repeat_sect_index++;
              if (PARM->repeat_sect_index<PARM->repeat_sect_cnt) {
                if (PARM->repeat_sect_mm) {
                  snprintf(PARM->repeat_sect_mm->body, 16, "%d",
                           PARM->repeat_sect_index);
                  //printf("%s now %s\n",
                  //       PARM->repeat_sect_mm->name,PARM->repeat_sect_mm->body);
                }
                csound_prspush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                csound_prs_scan_string(PARM->repeat_sect_cf->body, yyscanner);
                PARM->line = PARM->repeat_sect_line;
              }
              else { // finished loop
                //corfile_puts(csound, 'r', PARM->cf);
                /* corfile_putc(csound, '\n', PARM->cf); */
                //printf("end of loop\n");
                //printf("****>>%s<<****\n", PARM->cf->body);
                PARM->in_repeat_sect=0;
                corfile_rm(csound, &PARM->repeat_sect_cf);
                //csound->Free(csound, PARM->repeat_sect_mm->body);
                //PARM->repeat_sect_mm->body = NULL;
              }
            }
            else {
              corfile_putc(csound, op, PARM->cf);
              int c = input(yyscanner);
              if (isblank(c)) {
                while (1) {
                  //printf("**copy %.2x(%c)\n", c, c);
                  corfile_putc(csound, c, PARM->cf);
                  if (c=='\n') break;
                  if (c=='\0') {
                    corfile_putc(csound, '\n', PARM->cf);
                    break;
                  }
                  c = input(yyscanner);
                }
              }
              corfile_putc(csound, '\n', PARM->cf);
            }
          }
          else corfile_puts(csound, yytext, PARM->cf);
        }
.       { corfile_putc(csound, yytext[0], PARM->cf); }

%%
static void comment(yyscan_t yyscanner)              /* Skip until nextline */
{
    char c;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    while ((c = input(yyscanner)) != '\n' && c != '\r') { /* skip */
      if (UNLIKELY((int)c == EOF || c=='\0')) {
        YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
          YY_BUFFER_EOF_PENDING;
        return;
      }
    }
    if (c == '\r' && (c = input(yyscanner)) != '\n') {
      if (LIKELY((int)c != EOF && c!='\0'))
        unput(c);
      else
        YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
          YY_BUFFER_EOF_PENDING;
    }
    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
}

static void do_comment(yyscan_t yyscanner)         /* Skip until * and / chars */
{
    int c;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
 TOP:
    c = input(yyscanner);
    switch (c) {
    NL:
    case '\n':
      csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
      goto TOP;
    case '*':
    AST:
      c = input(yyscanner);
      switch (c) {
      case '*':
        goto AST;
      case '\n':
        goto NL;
      case '/':
        return;
      case EOF:
      case '\0':
        goto ERR;
      default:
        goto TOP;
      }
    case EOF:
    ERR:
      YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
        YY_BUFFER_EOF_PENDING;
      return;
    default:
      goto TOP;
    }
}

static void do_include(CSOUND *csound, int term, yyscan_t yyscanner)
{
    char buffer[100];
    int p=0;
    int c;
    CORFIL *cf;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    while ((c=input(yyscanner))!=term) {
      if (c=='\n' || c==EOF || c=='\0') {
        csound->Warning(csound, Str("Ill formed #include ignored"));
        return;
      }
      buffer[p] = c;
      p++;
    }
    buffer[p] = '\0';
    while ((c=input(yyscanner))!='\n');
    if (UNLIKELY(PARM->depth++>=1024)) {
      csound->Die(csound, Str("Includes nested too deeply"));
    }
    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner), yyscanner);
    csound->DebugMsg(csound,"line %d at end of #include line\n",
                     csound_prsget_lineno(yyscanner));
    {
      uint8_t n = file_to_int(csound, buffer);
      //char bb[128];
      PARM->lstack[PARM->depth] = n;
      //sprintf(bb, "#source %llu\n", PARM->locn = make_slocation(PARM));
      PARM->llocn = PARM->locn;
#ifdef SCORE_PARSER
      //corfile_puts(csound, bb, PARM->cf);
#endif
    }
    // printf("reading included file \"%s\"\n", buffer);
    csound->DebugMsg(csound,"reading included file \"%s\"\n", buffer);
    if (UNLIKELY(isDir(buffer)))
      csound->Warning(csound, Str("%s is a directory; not including"), buffer);
    csound->DebugMsg(csound, "path = %s\n", PARM->path);
    if (PARM->path && buffer[0]!= DIRSEP) { // if nested included directories
      char tmp[1024];
      csound->DebugMsg(csound, "using path %s\n", PARM->path);
      strncpy(tmp, PARM->path, 1023);
      strcat(tmp, "/");
      strncat(tmp, buffer, 1022-strlen(tmp));
      if ((cf = copy_to_corefile(csound, tmp, "INCDIR", 0))==NULL)
          cf = copy_to_corefile(csound, buffer, "INCDIR", 0);
    }
    else cf = copy_to_corefile(csound, buffer, "INCDIR", 0);
    if (UNLIKELY(cf == NULL))
      csound->Die(csound,
                  Str("Cannot open #include'd file %s\n"), buffer);
    printf("stack pointer = %d\n", PARM->macro_stack_ptr);
    if (UNLIKELY(PARM->macro_stack_ptr +1 >= PARM->macro_stack_size )) {
      //trace_alt_stack(csound, PARM, __LINE__);
      PARM->alt_stack =
        (MACRON*) csound->ReAlloc(csound, PARM->alt_stack,
                                  sizeof(MACRON)*(PARM->macro_stack_size+=S_INC));
      if (UNLIKELY(PARM->alt_stack == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      csound->DebugMsg(csound, "alt_stack now %d long,\n",
                       PARM->macro_stack_size);
    }
    csound->DebugMsg(csound,"csound_prs(%d): stacking line %d at %d\n", __LINE__,
           csound_prsget_lineno(yyscanner),PARM->macro_stack_ptr);
    PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
    PARM->alt_stack[PARM->macro_stack_ptr].line = csound_prsget_lineno(yyscanner);
    if (strrchr(buffer,DIRSEP)) {
      PARM->alt_stack[PARM->macro_stack_ptr].path = PARM->path;
      //printf("setting path from %s to ", PARM->path);
      PARM->path = strdup(buffer); /* wasteful! */
      *(strrchr(PARM->path,DIRSEP)) = '\0';
      //printf("%s\n",PARM->path);
    }
    else PARM->alt_stack[PARM->macro_stack_ptr].path = NULL;
    PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
    csound_prspush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
    csound_prs_scan_string(cf->body, yyscanner);
    corfile_rm(csound, &cf);
    csound->DebugMsg(csound,"Set line number to 1\n");
    csound_prsset_lineno(1, yyscanner);
}

void  do_new_include(CSOUND *csound, yyscan_t yyscanner)
{
    char buffer[128];
    CORFIL *cf = PARM->cf;
    int p = cf->p-2;
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;

    //printf("*** in do_new_include\n");
    cf->body[p+1] = '\0';
    while (cf->body[p]!='"') p--;
    //printf("*** name is >>%s<<\n", &cf->body[p]);
    cf->body[p] = '\0';
    strncpy(buffer, &cf->body[p+1],127); buffer[127]='\0';
    cf->p = p;
    //printf("****buffer >>%s<<\n", buffer);
    while ((input(yyscanner))!='\n');
    if (UNLIKELY(PARM->depth++>=1024)) {
      csound->Die(csound, Str("Includes nested too deeply"));
    }
    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner), yyscanner);
    csound->DebugMsg(csound,"line %d at end of #include line\n",
                     csound_prsget_lineno(yyscanner));
    {
      uint8_t n = file_to_int(csound, buffer);
      //char bb[128];
      PARM->lstack[PARM->depth] = n;
      //sprintf(bb, "#source %"PRIu64"\n", PARM->locn = make_slocation(PARM));
      PARM->llocn = PARM->locn;
      //corfile_puts(csound, bb, PARM->cf);
    }
    csound->DebugMsg(csound,"reading included file \"%s\"\n", buffer);
    if (UNLIKELY(isDir(buffer)))
      csound->Warning(csound, Str("%s is a directory; not including"), buffer);
    if (PARM->path && buffer[0]!=DIRSEP) {
      char tmp[1024];
      strncpy(tmp, PARM->path, 1023);
      strcat(tmp, "/");
      strncat(tmp, buffer, 1022-strlen(tmp));
      cf = copy_to_corefile(csound, tmp, "INCDIR", 0);
    }
    else cf = copy_to_corefile(csound, buffer, "INCDIR", 0);
    if (UNLIKELY(cf == NULL))
      csound->Die(csound,
                  Str("Cannot open #include'd file %s\n"), buffer);
    if (UNLIKELY(PARM->macro_stack_ptr +1 >= PARM->macro_stack_size )) {
      //trace_alt_stack(csound, PARM, __LINE__);
      PARM->alt_stack =
        (MACRON*) csound->ReAlloc(csound, PARM->alt_stack,
                                  sizeof(MACRON)*(PARM->macro_stack_size+=S_INC));
      if (UNLIKELY(PARM->alt_stack == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      /* csound->DebugMsg(csound, "alt_stack now %d long,\n", */
      /*                  PARM->macro_stack_size); */
    }
    csound->DebugMsg(csound,"csound_prs(%d): stacking line %d at %d\n", __LINE__,
           csound_prsget_lineno(yyscanner),PARM->macro_stack_ptr);
    PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
    PARM->alt_stack[PARM->macro_stack_ptr].line = csound_prsget_lineno(yyscanner);
    if (strrchr(buffer,DIRSEP)) {
      PARM->alt_stack[PARM->macro_stack_ptr].path = PARM->path;
      PARM->path = strdup(buffer); /* wasteful! */
      *(strrchr(PARM->path,DIRSEP)) = '\0';
    }
    else PARM->alt_stack[PARM->macro_stack_ptr].path = NULL;
    PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
    csound_prspush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
    csound_prs_scan_string(cf->body, yyscanner);
    corfile_rm(csound, &cf);
    csound->DebugMsg(csound,"Set line number to 1\n");
    csound_prsset_lineno(1, yyscanner);
}

static void do_macro_arg(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    MACRO *mm = (MACRO*) csound->Malloc(csound, sizeof(MACRO));
    int   arg = 0, i, c;
    int   size = 100;
    int mlen = 40;
    char *q = name0;
    char *mname = malloc(mlen);

    if (UNLIKELY(mm == NULL||mname == NULL)) {
      csound->Message(csound, Str("Memory exhausted"));
      csound->LongJmp(csound, 1);
    }
    mm->margs = MARGS;    /* Initial size */
    mm->name = (char*)csound->Malloc(csound, strlen(name0) + 1);
    if (UNLIKELY(mm->name == NULL)) {
      csound->Message(csound, Str("Memory exhausted"));
      csound->LongJmp(csound, 1);
    }
    strcpy(mm->name, name0);
    do {
      i = 0;
      q = name0;
      mname[i++] = '`';
      while ((c = *q++)) {
        mname[i++] = c;
        if (UNLIKELY(i==mlen))
          mname = (char *)realloc(mname, mlen+=40);
        if (UNLIKELY(mname == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
      }
      mname[i++] = '`';
      if (UNLIKELY(i==mlen)) {
          mname = (char *)realloc(mname, mlen+=40);
        if (UNLIKELY(mname == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
      }
      mname[i++] = '`';
      if (UNLIKELY(i==mlen)) {
        mname = (char *)realloc(mname, mlen+=40);
        if (UNLIKELY(mname == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
      }
      while (isspace((c = input(yyscanner))));

      while (isNameChar(c, i)) {
        mname[i++] = c;
        if (UNLIKELY(i==mlen)) {
          mname = (char *)realloc(mname, mlen+=40);
          if (UNLIKELY(mname == NULL)) {
            csound->Message(csound, Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
          }
        }
        c = input(yyscanner);
      }
      mname[i] = '\0';
      mm->arg[arg] = csound->Malloc(csound, i + 1);
      if (UNLIKELY(mm->arg[arg] == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      strcpy(mm->arg[arg++], mname);
      if (UNLIKELY(arg >= mm->margs)) {
        mm = (MACRO*) csound->ReAlloc(csound, mm, sizeof(MACRO)
                               + mm->margs * sizeof(char*));
        if (UNLIKELY(mm == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
        mm->margs += MARGS;
      }
      while (isspace(c))
        c = input(yyscanner);
    } while (c == '\'' || c == '#');
    if (UNLIKELY(c != ')')) {
      csound->Message(csound, Str("macro error\n"));
    }
    free(mname);
    c = input(yyscanner);
    while (c!='#') {
      if (UNLIKELY(c==EOF || c=='\0'))
        csound->Die(csound, Str("define macro runaway\n"));
      else if (c==';') {
        while ((c=input(yyscanner))!= '\n')
          if (UNLIKELY(c==EOF || c=='\0')) {
            csound->Die(csound, Str("define macro runaway\n"));
          }
      }
      else if (c=='/') {
        if ((c=input(yyscanner))=='/') {
          while ((c=input(yyscanner))!= '\n')
            if (UNLIKELY(c==EOF || c=='\0'))
              csound->Die(csound, Str("define macro runaway\n"));
        }
        else if (c=='*') {
          while ((c=input(yyscanner))!='*') {
          again:
            if (UNLIKELY(c==EOF || c=='\0'))
              csound->Die(csound, Str("define macro runaway\n"));
          }
          if ((c=input(yyscanner))!='/') goto again;
        }
      }
      else if (UNLIKELY(!isspace(c)))
        csound->Die(csound,
               Str("define macro unexpected character %c(0x%.2x) awaiting #\n"),
                    c, c);
      c = input(yyscanner); /* skip to start of body */
    }
    mm->acnt = arg;
    i = 0;
    mm->body = (char*) csound->Malloc(csound, 100);
    if (UNLIKELY(mm->body == NULL)) {
      csound->Message(csound, Str("Memory exhausted"));
      csound->LongJmp(csound, 1);
    }

    while ((c = input(yyscanner)) != '#') { /* read body */
      if (UNLIKELY(c == EOF || c=='\0'))
        csound->Die(csound, Str("define macro with args: unexpected EOF"));
      if (c=='$') {             /* munge macro name? */
        int n = strlen(name0)+4;
        if (UNLIKELY(i+n >= size)) {
          mm->body = csound->ReAlloc(csound, mm->body, size += 100);
          if (UNLIKELY(mm->body == NULL)) {
            csound->Message(csound, Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
          }
        }
        mm->body[i] = '$'; mm->body[i+1] = '`';
        strcpy(&mm->body[i+2], name0);
        mm->body[i + n - 2] = '`'; mm->body[i + n - 1] = '`';
        i+=n;
        continue;
      }
      mm->body[i++] = c=='\r'?'\n':c;
      if (UNLIKELY(i >= size)) {
        mm->body = csound->ReAlloc(csound, mm->body, size += 100);
        if (UNLIKELY(mm->body == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
      }
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size)) {
          mm->body = csound->ReAlloc(csound, mm->body, size += 100);
          if (UNLIKELY(mm->body == NULL)) {
            csound->Message(csound, Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
          }
        }
      }
      if (UNLIKELY(c == '\n' || c == '\r')) {
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
        corfile_putc(csound, '\n', PARM->cf);
        csound_prs_line(PARM->cf, yyscanner);
      }
    }
    mm->body[i] = '\0';
    mm->next = PARM->macros;
    PARM->macros = mm;
}

static void do_macro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    MACRO *mm = (MACRO*) csound->Malloc(csound, sizeof(MACRO));
    int   i, c;
    int   size = 100;
    if (UNLIKELY(mm == NULL)) {
      csound->Message(csound, Str("Memory exhausted"));
      csound->LongJmp(csound, 1);
    }

    mm->margs = MARGS;    /* Initial size */
    csound->DebugMsg(csound,"Macro definition for %s\n", name0);
    mm->name = (char*)csound->Malloc(csound, strlen(name0) + 1);
    if (UNLIKELY(mm->name == NULL)) {
      csound->Message(csound, Str("Memory exhausted"));
      csound->LongJmp(csound, 1);
    }
    strcpy(mm->name, name0);
    mm->acnt = 0;
    i = 0;
    while ((c = input(yyscanner)) != '#') {
      if (UNLIKELY(c==EOF || c=='\0'))
        csound->Die(csound, Str("define macro runaway\n"));
      else if (c==';') {
        while ((c=input(yyscanner))!= '\n')
          if (UNLIKELY(c==EOF || c=='\0')) {
            csound->Die(csound, Str("define macro runaway\n"));
          }
      }
      else if (c=='/') {
        if ((c=input(yyscanner))=='/') {
          while ((c=input(yyscanner))!= '\n')
            if (UNLIKELY(c==EOF || c=='\0'))
              csound->Die(csound, Str("define macro runaway\n"));
        }
        else if (c=='*') {
          while ((c=input(yyscanner))!='*') {
          again:
            if (UNLIKELY(c==EOF || c=='\0'))
              csound->Die(csound, Str("define macro runaway\n"));
          }
          if ((c=input(yyscanner))!='/') goto again;
        }
      }
      else if (UNLIKELY(!isspace(c)))
        csound->Die(csound,
                    Str("define macro unexpected character %c(0x%.2x)"
                        "awaiting #\n"),
                    c, c);
    }
    mm->body = (char*) csound->Malloc(csound, 100);
    if (UNLIKELY(mm->body == NULL)) {
      csound->Message(csound, Str("Memory exhausted"));
      csound->LongJmp(csound, 1);
    }
    while ((c = input(yyscanner)) != '#') {
      if (UNLIKELY(c == EOF || c==0))
        csound->Die(csound, Str("define macro: unexpected EOF"));
      mm->body[i++] = c=='\r'?'\n':c;
      if (UNLIKELY(i >= size)) {
        mm->body = csound->ReAlloc(csound, mm->body, size += 100);
        if (UNLIKELY(mm->body == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
      }
      if (c == '\\') {                    /* allow escaped # */
        mm->body[i++] = c = input(yyscanner);
        if (UNLIKELY(i >= size)) {
          mm->body = csound->ReAlloc(csound, mm->body, size += 100);
          if (UNLIKELY(mm->body == NULL)) {
            csound->Message(csound, Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
          }
        }
      }
      if (UNLIKELY(c == '\n' || c == '\r')) {
        csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
        corfile_putc(csound, '\n', PARM->cf);
        csound_prs_line(PARM->cf, yyscanner);
      }
    }
    mm->body[i] = '\0';
    csound->DebugMsg(csound,"Body #%s#\n", mm->body);
    mm->next = PARM->macros;
    PARM->macros = mm;
}

static void do_umacro(CSOUND *csound, char *name0, yyscan_t yyscanner)
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
    while ((c=input(yyscanner)) != '\n' &&
           c != EOF && c != '\r'); /* ignore rest of line */
    csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),yyscanner);
}

static void do_ifdef(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int c;
    MACRO *mm;
    IFDEFSTACK *pp;
    pp = (IFDEFSTACK*) csound->Calloc(csound, sizeof(IFDEFSTACK));
    if (UNLIKELY(pp == NULL)) {
      csound->Message(csound, Str("Memory exhausted"));
      csound->LongJmp(csound, 1);
    }
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

static void do_ifdef_skip_code(CSOUND *csound, yyscan_t yyscanner)
{
    int i, c, nested_ifdef = 0;
    char buf[8];
    IFDEFSTACK *pp;
    /* buf = (char*)malloc(8*sizeof(char)); */
    /* if (UNLIKELY(buf == NULL)) { */
    /*   csound->Message(csound, Str("Memory exhausted")); */
    /*   csound->LongJmp(csound, 1); */
    /* } */
    pp = PARM->ifdefStack;
    c = input(yyscanner);
    for (;;) {
      while (c!='\n' && c!= '\r') {
        if (UNLIKELY(c == EOF || c=='\0')) {
          csound->Message(csound, Str("Unmatched #if%sdef\n"),
                          PARM->isIfndef ? "n" : "");
          csound->LongJmp(csound, 1);
        }
        c = input(yyscanner);
    }
      csound_prsset_lineno(1+csound_prsget_lineno(yyscanner),
                           yyscanner);
      corfile_putc(csound, '\n', PARM->cf);
      csound_prs_line(PARM->cf, yyscanner);
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
          if (UNLIKELY(pp->isElse)) {
            csound->Message(csound, Str("#else after #else\n"));
            csound->LongJmp(csound, 1);
          }
          pp->isElse = 1;
          break;
        }
      }
    }
    //free(buf);
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

void cs_init_smacros(CSOUND *csound, PRS_PARM *qq, NAMES *nn)
{
    while (nn) {
      char  *s = nn->mac;
      char  *p = strchr(s, '=');
      char  *mname;
      MACRO *mm;

      if (p == NULL)
        p = s + strlen(s);
      if (UNLIKELY(csound->oparms->msglevel & 7))
        csound->Message(csound, Str("Macro definition for %*s\n"), (int)(p - s), s);
      s = strchr(s, ':') + 1;                   /* skip arg bit */
      if (UNLIKELY(s == NULL || s >= p)) {
        csound->Die(csound, Str("Invalid macro name for --smacro"));
      }
      mname = (char*) csound->Malloc(csound, (p - s) + 1);
      if (UNLIKELY(mname == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      strncpy(mname, s, p - s);
      mname[p - s] = '\0';
      /* check if macro is already defined */
      for (mm = qq->macros; mm != NULL; mm = mm->next) {
        if (strcmp(mm->name, mname) == 0)
          break;
      }
      if (mm == NULL) {
        mm = (MACRO*) csound->Calloc(csound, sizeof(MACRO));
        if (UNLIKELY(mm  == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
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
      if (UNLIKELY(mm->body == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      strcpy(mm->body, p);
      nn = nn->next;
    }
}

static void expand_macro(CSOUND* csound, MACRO* mm, yyscan_t yyscanner)
{
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    /* Need to read from macro definition */
    if (mm->acnt<0) { /* Macro inhibitted */
      corfile_puts(csound, yytext, PARM->cf);
    }
    else {
      if (UNLIKELY(PARM->macro_stack_ptr+1 >=
                   PARM->macro_stack_size)) {
        //trace_alt_stack(csound, PARM, __LINE__);
        //printf("***extending macro stack %p\n", PARM->alt_stack);
        PARM->alt_stack =
          (MACRON*)
          csound->ReAlloc(csound, PARM->alt_stack,
                          sizeof(MACRON)*(PARM->macro_stack_size+=S_INC));
        if (UNLIKELY(PARM->alt_stack == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
        /* csound->DebugMsg(csound, "alt_stack now %d long\n", */
        /*                  PARM->macro_stack_size); */
      }
      PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
      PARM->alt_stack[PARM->macro_stack_ptr].line =
        csound_prsget_lineno(yyscanner);
      PARM->alt_stack[PARM->macro_stack_ptr].path = NULL;
      PARM->alt_stack[PARM->macro_stack_ptr++].s = NULL;
      yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
      csound_prsset_lineno(1, yyscanner);
      if (UNLIKELY(PARM->depth>1022)) {
        csound->Message(csound,
                        Str("macros/include nested too deep: "));
        csound->LongJmp(csound, 1);
      }
      PARM->lstack[++PARM->depth] =
        (strchr(mm->body,'\n') ?file_to_int(csound, yytext) : 63);
      yy_scan_string(mm->body, yyscanner);
      /* csound->DebugMsg(csound,"%p\n", YY_CURRENT_BUFFER); */
    }
}

static void expand_macroa(CSOUND *csound, MACRO* mm, yyscan_t yyscanner)
{
    struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
    int err =0;
    //char      *mname;
    int c, i, j, cnt=0;
    //mname = buf;
    /* Need to read from macro definition */
    csound->DebugMsg(csound,"Looking for %d args\n", mm->acnt);
    for (j = 0; j < mm->acnt; j++) {
      char  term = (j == mm->acnt - 1 ? ')' : '\'');
      /* Compatability */
      char  trm1 = (j == mm->acnt - 1 ? ')' : '#');
      MACRO *nn = (MACRO*) csound->Malloc(csound, sizeof(MACRO));
      //int   size = 100;
      if (UNLIKELY(nn == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      nn->name = csound->Malloc(csound, strlen(mm->arg[j]) + 1);
      if (UNLIKELY(nn->name == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      csound->DebugMsg(csound,"Arg %d: %s\n", j+1, mm->arg[j]);
      strcpy(nn->name, mm->arg[j]);
      csound->Message(csound, "defining argument %s ",
                      nn->name);
      i = 0;
      nn->body = (char*) csound->Malloc(csound, 100);
      if (UNLIKELY(nn->body == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
      }
      while (1) {
        c = input(yyscanner);
        if (cnt==0 && ( c==term || c==trm1)) break;
        if (UNLIKELY(cnt==0 && c == ')')) {
          csound->Message(csound,
                          Str("Too few arguments to macro\n"));
          corfile_puts(csound, "$error", PARM->cf);
          err = 1; break;
        }
        if (c=='(') cnt++;
        if (c==')') cnt--;
        if (c == '\\') {
          int newc = input(yyscanner);
          if (newc != ')') nn ->body[i++] = c;
          c = newc;
        }
        if (UNLIKELY(i > 98)) {
          csound->Message(csound,
                          Str("Missing argument terminator\n%.98s"),
                          nn->body);
          corfile_puts(csound, "$error", PARM->cf);
          err = 1; break;
        }
        nn->body[i++] = c;
        /* if (UNLIKELY(i >= size)) { */
        /*   nn->body = csound->ReAlloc(csound, nn->body, */
        /*   size += 100); */
        /*   if (UNLIKELY(nn->body == NULL)) { */
        /*     csound->Message(csound, Str("Memory exhausted")); */
        /*     csound->LongJmp(csound, 1); */
        /*   } */
        /* } */
      }
      nn->body[i] = '\0';
      csound->Message(csound, "as...#%s#\n", nn->body);
      nn->acnt = 0;       /* No arguments for arguments */
      nn->next = PARM->macros;
      PARM->macros = nn;
    }
    if (!err) {
      //csound->DebugMsg(csound,"New body: ...#%s#\n", mm->body);
      if (UNLIKELY(PARM->macro_stack_ptr +1 >=
                   PARM->macro_stack_size )) {
        //trace_alt_stack(csound, PARM, __LINE__);
        PARM->alt_stack =
          (MACRON*)
          csound->ReAlloc(csound, PARM->alt_stack,
                          sizeof(MACRON)*(PARM->macro_stack_size+=S_INC));
        if (UNLIKELY(PARM->alt_stack == NULL)) {
          csound->Message(csound, Str("Memory exhausted"));
          csound->LongJmp(csound, 1);
        }
        /* csound->DebugMsg(csound, */
        /*        "macro_stack extends alt_stack to %d long\n", */
        /*                  PARM->macro_stack_size); */
      }
      PARM->alt_stack[PARM->macro_stack_ptr].n =
        PARM->macros->acnt;
      PARM->alt_stack[PARM->macro_stack_ptr].line =
        csound_prsget_lineno(yyscanner);
      PARM->alt_stack[PARM->macro_stack_ptr].path = NULL;
      PARM->alt_stack[PARM->macro_stack_ptr++].s = PARM->macros;
      PARM->alt_stack[PARM->macro_stack_ptr].n = 0;
      PARM->alt_stack[PARM->macro_stack_ptr].line =
        csound_prsget_lineno(yyscanner);
      PARM->alt_stack[PARM->macro_stack_ptr].path = NULL;
      /* printf("stacked line = %llu at %d\n", */
      /*  csound_prsget_lineno(yyscanner), */
      /*                       PARM->macro_stack_ptr-1); */
      PARM->alt_stack[PARM->macro_stack_ptr].s = NULL;
      //csound->DebugMsg(csound,"Push %p macro stack\n",
      //                 PARM->macros);
      yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
      csound_prsset_lineno(1, yyscanner);
      if (UNLIKELY(PARM->depth>1022)) {
        csound->Message(csound,
                        Str("macros/include nested too deep: "));
        //csound->LongJmp(csound, 1);
        corfile_puts(csound, "$error", PARM->cf);
        err = 1;
      }
    }
    if (!err) {
      PARM->lstack[++PARM->depth] =
        (strchr(mm->body,'\n') ?file_to_int(csound, mm->name) : 63);
      yy_scan_string(mm->body, yyscanner);
      csound_prsset_lineno(0, yyscanner);
    }
}

static void csound_prs_line(CORFIL* cf, void *yyscanner)
{
    int n = csound_prsget_lineno(yyscanner);
    //printf("line number %d\n", n);
    /* This assumes that the initial line was not written with this system  */
    if (cf->p>0 && cf->body[cf->p-1]=='\n') {
      uint64_t locn = PARM->locn;
#if 0
      uint64_t llocn = PARM->llocn;
      if (locn != llocn) {
        //char bb[80];
        //sprintf(bb, "#source %llu\n", locn);
        //corfile_puts(bb, cf);
      }
#endif
      PARM->llocn = locn;
#ifdef SCORE_PARSER
      //if (n!=PARM->line+1) {
      //char bb[80];
        //sprintf(bb, "#line   %d\n", n);
        //printf("#line %d\n", n);
        //corfile_puts(bb, cf);
      //}
#endif
    }
    PARM->line = n;
}

static MACRO *find_definition(MACRO *mmo, char *s)
{
    MACRO *mm = mmo;
    if (s[strlen(s)-1]=='.') s[strlen(s)-1]='\0';
    else if (s[strlen(s)-2]=='.' && s[strlen(s)-1]=='(') {
      s[strlen(s)-2] = '('; s[strlen(s)-1] = '\0'; }
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
      while (*s++!='`') { if (*s=='\0') return NULL; }
      if (*s++!='`') { s--; goto looking; }
      //printf("now try looking for %s\n", s);
      while (mm != NULL) {  /* Find the definition */
        //printf("looking at %p(%s) body #%s#\n", mm, mm->name, mm->body);
        if (!(strcmp(s, mm->name))) break;
        mm = mm->next;
      }
    }
    //if (mm) printf("found body #%s#%c\n****\n", mm->body, mm->acnt?'X':' ');
    return mm;
}

//#if 0

static int powintint(int x, int n)
{
    int32 ans = 1;
    if (n<0) return 0;
    while (n!=0) {
      if (n&1) ans = ans * x;
      n >>= 1;
      x = x*x;
    }
    return ans;
}

static int operate(CSOUND *csound, int a, int b, char c)
{
    int ans;

    switch (c) {
    case '+': ans = a + b; break;
    case '-': ans = a - b; break;
    case '*': ans = a * b; break;
    case '/': ans = a / b; break;
    case '%': ans = a % b; break;
    case '^': ans = powintint(a, b); break;
    case '&': ans = a & b; break;
    case '|': ans = a | b; break;
    case '#': ans = a ^ b; break;
    default:
      //csoundDie(csound, Str("Internal error op=%c"), c);
      ans = 0;
    }
    //printf("operate: %d %c %d => %d\n", a, c, b, ans);
    return ans;
}

static int bodmas(CSOUND *csound, yyscan_t yyscanner, int* term)
{
      char  stack[30];
      int   vv[30];
      char  *op = stack - 1;
      int   *pv = vv - 1;
      int   c;
      struct yyguts_t *yyg = (struct yyguts_t*)yyscanner;
      int   type = 0;  /* 1 -> expecting binary operator,')', or ']'; else 0 */
      *++op = '[';
      do {
      parseNumber:
        c = input(yyscanner);
      top:
        while (isblank(c)) c = input(yyscanner);
        //printf("bodmas: c='%c'\n", c);
        switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
          //printf("bodmas:number\n");
          int i = 0;
          while (isdigit(c)) {
            i = 10 * i + c - '0';
            c = input(yyscanner);
          }
          //printf("Number: %d terminated by %c(%.2x)\n", i, c, c);
          *++pv = i;
          type = 1;
          if (c=='\0') {
            if (on_EOF(csound,yyscanner)==0) return 0;
            c = input(yyscanner);
          }
          goto top;
        }
        case '$': {
          MACRO *mm = PARM->macros;
          char buf[256];
          int p=0;
          int first = 1;
          while ((c=input(yyscanner))) {
            if (isalpha(c) || c=='_' || c=='`' || (!first && isdigit(c)))
              buf[p++] = c;
            else break;
            first = 0;
          }
          if (c=='.') c = input(yyscanner);
          buf[p] = '\0';
          //printf("macro: %s\n", buf);
          mm = find_definition(mm, buf);
          if (mm==NULL) return 0;
          if (c=='(') {
            expand_macroa(csound, mm, yyscanner);
            goto parseNumber;
          }
          else {
            //printf("body: >>%s<<\n", mm->body);
            unput(c);
            expand_macro(csound, mm, yyscanner);
            c = input(yyscanner);
          }
          goto top;
        }
        case '+': case '-':
          //printf("bodmas:arith %d type %d\n", c, type);
          if (!type)
            goto parseNumber;
          if (*op != '[' && *op != '(') {
            int v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c;
          goto parseNumber;
        case '*':
        case '/':
        case '%':
          if (UNLIKELY(!type)) {
            csound->Message(csound, Str("illegal placement of operator %c in [] "
                                        "expression"), c);
            return 0;
          }
          if (*op == '*' || *op == '/' || *op == '%') {
            int v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c;
          goto parseNumber;
        case '&':
        case '|':
        case '#':
          if (UNLIKELY(!type)) {
            csound->Message(csound, Str("illegal placement of operator %c in [] "
                                        "expression"), c);
            return 0;
          }
          if (*op == '|' || *op == '&' || *op == '#') {
            int v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 0;
          *++op = c;
          goto parseNumber;
        case '(':
          //printf("bodmas: bra\n");
          if (UNLIKELY(type)) {
            csound->Message(csound,
                            Str("illegal placement of '(' in [] expression"));
            return 0;
          }
          type = 0;
          *++op = c;
          goto parseNumber;
        case ')':
          //printf("bodmas:bra switch close\n");
          if (UNLIKELY(!type)) {
            csound->Message(csound,
                            Str("missing operand before ')' in [] expression"));
            return 0;
          }
          while (*op != '(') {
            int v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          type = 1;
          op--;
          goto parseNumber;
        //        case '^':
        //type = 0;
        // *++op = c; c = getscochar(csound, 1); break;
        case '[':
          if (UNLIKELY(type)) {
            csound->Message(csound,
                            Str("illegal placement of '[' in [] expression"));
            return 0;
          }
          type = 1;
          {
            //int i;
            //MYFLT x;
            //for (i=0;i<=pv-vv;i++) printf(" %d ", vv[i]);
            //printf("| %ld\n", pv-vv);
            *++pv = bodmas(csound, yyscanner, term);
            //printf("recursion gives %lf (%d)\n", x,*(pv-1));
            //for (i=0;i<pv-vv;i++) printf(" %d ", vv[i]); printf("| %ld\n", pv-vv);
            c = input(yyscanner);
            break;
          }
        case ']':
          //printf("] case: type = %d *op=%c\n", type, *op);
          if (UNLIKELY(!type)) {
            csound->Message(csound,
                            Str("missing operand before closing bracket in []"));
            return 0;
          }
          while (*op != '[') {
            int v = operate(csound, *(pv-1), *pv, *op);
            op--; pv--;
            *pv = v;
          }
          //printf("done ]*** *op=%c v=%d (%c)\n", *op, *pv, c);
          *term = '~'; return *pv;
        case '~':
          break;
        case ' ':               /* Ignore spaces */
          c = input(yyscanner);
          continue;
        case EOF:
        case '\0':
          if (on_EOF(csound, yyscanner)==0) {
            yyterminate(); return 0;
          }
          c = input(yyscanner);
          goto top;
        default:
          csound->Message(csound,
                          Str("illegal character %c(%.2x) in [] expression"),
                          c, c);
          return 0;
        }
      } while (c != '~');
      return *pv;
}
//#endif

static int extract_int(CSOUND *csound, yyscan_t yyscanner, int* term)
{
    int c;
 top:
    do {
      c = input(yyscanner);
    } while (isblank(c));
    if (c=='$') { // macro to yield count
      char buf[256];
      int i=0;
      MACRO* mm;
      //printf("*** macro count\n");
      while (isalnum(c=input(yyscanner)) || c=='_') {
        buf[i++] = c;
      }
      if (c=='.') c = input(yyscanner);
      buf[i] = '\0';
      //printf("*** lookup macro %s\n", buf);
      if ((mm = find_definition(PARM->macros, buf))==NULL) {
        csound->Message(csound,Str("Undefined macro: '%s'"), buf);
        //csound->LongJmp(csound, 1);
        corfile_puts(csound, "$error", PARM->cf);
        *term = c;
        return 0;
      }
      else if (c=='(') {
        struct yyguts_t *yyg =(struct yyguts_t*)yyscanner;
        unput(c);
        expand_macroa(csound, mm, yyscanner);
        goto top;
      }
      else {
        *term = c;
        return atoi(mm->body);
      }
    }
    else if (c=='[') {
      //printf("** ] case\n");
      int n =  bodmas(csound, yyscanner,term);
      return n;
    }
    else if (c=='\0') {
      if (on_EOF(csound, yyscanner)==0) return 0;
      goto top;
    }
    else {
      int i = 0;
      while (isdigit(c)) {
        i = 10 * i + c - '0';
        c = input(yyscanner);
      }
      *term = c;
      return i;
    }
    *term = c;
    return 0;
}

static int on_EOF(CSOUND* csound, void* yyscanner)
{
    MACRO *x, *y=NULL;
    int n;
    struct yyguts_t *yyg =(struct yyguts_t*)yyscanner;
    csound->DebugMsg(csound,"*********Leaving buffer %p\n",
                     YY_CURRENT_BUFFER);
    //printf("stack pointer = %d\n", PARM->macro_stack_ptr);
    yypop_buffer_state(yyscanner);
    PARM->depth--;
    if (UNLIKELY(PARM->depth > 1024)) {
      //csound->Die(csound, Str("unexpected EOF!!"));
      csound->Message(csound, Str("unexpected EOF!!\n"));
      csound->LongJmp(csound, 1);
    }
    PARM->llocn = PARM->locn; PARM->locn = make_slocation(PARM);
    csound->DebugMsg(csound,"csound-prs(%d): loc=%u; lastloc=%u\n",
                     __LINE__, PARM->llocn, PARM->locn);
    if ( !YY_CURRENT_BUFFER ) return 0;
    csound->DebugMsg(csound,"End of input; popping to %p\n",
                     YY_CURRENT_BUFFER);
    csound_prs_line(PARM->cf, yyscanner);
    n = PARM->alt_stack[--PARM->macro_stack_ptr].n;
    if (PARM->alt_stack[PARM->macro_stack_ptr].path) {
      //printf("restoring path from %s to %s\n",
      //    PARM->path, PARM->alt_stack[PARM->macro_stack_ptr].path);
      free(PARM->path);
      PARM->path = PARM->alt_stack[PARM->macro_stack_ptr].path;
    }
    /* printf("lineno on stack is %llu\n", */
    /*        PARM->alt_stack[PARM->macro_stack_ptr].line); */
    csound->DebugMsg(csound,"n=%d\n", n);
    if (n!=0) {
      //printf("We need to delete %d macros starting with %d\n",
      //       n, PARM->macro_stack_ptr);
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
    csound_prsset_lineno(PARM->alt_stack[PARM->macro_stack_ptr].line,
                         yyscanner);
    /* csound->DebugMsg(csound, "csound_prs(%d): line now %d at %d\n", */
    /*                  __LINE__, */
    /*                  csound_prsget_lineno(yyscanner), */
    /*                  PARM->macro_stack_ptr); */
    csound->DebugMsg(csound,
                     "End of input segment: macro pop %p -> %p\n",
                     y, PARM->macros);
    csound_prsset_lineno(PARM->alt_stack[PARM->macro_stack_ptr].line,
                         yyscanner);
    //print_csound_prsdata(csound,"Before prs_line", yyscanner);
    csound_prs_line(PARM->cf, yyscanner);
    //print_csound_prsdata(csound,"After prs_line", yyscanner);
    return 1;
}

/* static void trace_alt_stack(CSOUND* csound, PRS_PARM* p, int line) */
/* { */
/*  printf("***Line %d extend alt_stack from %d/%d (%p)\n", */
/*         line, p->macro_stack_ptr, p->macro_stack_size, p->alt_stack); */
/* } */

#if 0
static void print_csound_prsdata(CSOUND *csound, char *mesg, void *yyscanner)
{
    struct yyguts_t *yyg =(struct yyguts_t*)yyscanner;
    csound->DebugMsg(csound,"********* %s extra data ************", mesg);
    csound->DebugMsg(csound,"yyscanner = %p", yyscanner);
    csound->DebugMsg(csound,"yyextra_r = %p, yyin_r = %p, yyout_r = %p,"
                     " yy_buffer_stack_top = %d",
           yyg->yyextra_r, yyg->yyin_r,yyg->yyout_r, yyg->yy_buffer_stack_top);
    csound->DebugMsg(csound,"yy_buffer_stack_max = %d1, yy_buffer_stack = %p, "
                     "yy_hold_char = %d '%c'",
           yyg->yy_buffer_stack_max, yyg->yy_buffer_stack, yyg->yy_hold_char,
           yyg->yy_hold_char);
    csound->DebugMsg(csound,"yy_n_chars = %d, yyleng_r = %d, yy_c_buf_p = %p %c",
           yyg->yy_n_chars, yyg->yyleng_r, yyg->yy_c_buf_p, *yyg->yy_c_buf_p);
    csound->DebugMsg(csound,"yy_init = %d, yy_start = %d, "
                     "yy_did_buffer_switch_on_eof = %d",
           yyg->yy_init, yyg->yy_start, yyg->yy_did_buffer_switch_on_eof);
    csound->DebugMsg(csound,"yy_start_stack_ptr = %d,"
                     " yy_start_stack_depth = %d, yy_start_stack = %p",
           yyg->yy_start_stack_ptr, yyg->yy_start_stack_depth, yyg->yy_start_stack);

    csound->DebugMsg(csound,"yy_last_accepting_state = %d, "
                     "yy_last_accepting_cpos = %p %c",
           yyg->yy_last_accepting_state, yyg->yy_last_accepting_cpos,
                     *yyg->yy_last_accepting_cpos);
    csound->DebugMsg(csound,"yylineno_r = %d, yy_flex_debug_r = %d, "
                     "yytext_r = %p \"%s\", yy_more_flag = %d, yy_more_len = %d",
           yyg->yylineno_r, yyg->yy_flex_debug_r, yyg->yytext_r, yyg->yytext_r,
                     yyg->yy_more_flag, yyg->yy_more_len);
    {
      PRS_PARM* pp = yyg->yyextra_r;
      printf("macros = %p, isIfndef = %d, isString = %d, line - %d loc = %d\n",
             pp->macros, pp->isIfndef, pp->isString, pp->line, pp->locn);
      printf("llocn = %d dept=%d\n", pp->llocn, pp->depth);
    }
    csound->DebugMsg(csound,"*********\n");
}
#endif
