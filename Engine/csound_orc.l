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
#define YYSTYPE TREE*
#define YYLTYPE ORCTOKEN*
#include "tok.h"
#define YY_DECL int yylex (YYLTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
#include "csound_orcparse.h"
#include "csound_orc.h"
YYSTYPE *yylval_param;
YYLTYPE *yylloc_param;
ORCTOKEN *make_string(CSOUND *, char *);
extern ORCTOKEN *lookup_token(CSOUND *csound, char *);
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
#define udoflag csound->parserUdoflag
#define namedInstrFlag csound->parserNamedInstrFlag
//extern int udoflag;
//extern int namedInstrFlag;
int yyline = 0;
#define MAX_INCLUDE_DEPTH 100
//YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
#define MARGS   (3)
#include "parse_param.h"

#define YY_EXTRA_TYPE  PARSE_PARM *
#define PARM    yyget_extra(yyscanner)

static int eof = 0;
static int nBuffer = 0;
static int lBuffer = 0;
int lMaxBuffer = 1000;
//char *buffer;

#define YY_INPUT(buf,result,max_size)  {\
    result = get_next_char(buf, max_size, yyg); \
    if ( UNLIKELY( result <= 0  )) \
      result = YY_NULL; \
    }


typedef struct MACRO {          /* To store active macros */
    char          *name;        /* Use is by name */
    int           acnt;         /* Count of arguments */
    char          *body;        /* The text of the macro */
    struct MACRO  *next;        /* Chain of active macros */
    int           margs;        /* amount of space for args */
    char          *arg[MARGS];  /* With these arguments */
} MACRO;
static MACRO* macros = NULL;
static MACRO* alt_stack[MAX_INCLUDE_DEPTH];
static unsigned int macro_stack_ptr = 0;
struct yyguts_t;
ORCTOKEN *do_at(CSOUND *, int, struct yyguts_t*);
%}
%option reentrant
%option bison-bridge
%option bison-locations


STRCONST        \"(\\.|[^\"])*\"
STRCONSTe       \"(\\.|[^\"])*$
LABEL           [a-zA-Z_][a-zA-Z0-9_]*:
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
IDENTN          [a-zA-Z0-9_]+
MACRO           [a-zA-Z0-9_]+\(
MACRONAME       "$"[a-zA-Z0-9_]+
MACRONAMED      "$"[a-zA-Z0-9_]+\.
MACRONAMEA      "$"[a-zA-Z0-9_]+\(
MACRONAMEDA     "$"[a-zA-Z0-9_]+\.\(
INTGR           [0-9]+
NUMBER          [0-9]+\.[0-9]*(e[-+]?[0-9]+)?|\.[0-9]+(e[-+]?[0-9]+)?
STCOM           "/"\*
WHITE           [ \t]+
OPTWHITE        [ \t]*
INCLUDE         "#include"
DEFINE          "#define"
UNDEF           "#undef"
CONT            \\[[ \t]*\n

%x incl
%x macro
%x umacro

%%
"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */
{CONT}          { /* printf("****Continuation found\n"); */ yyline++; }
"\n"            { //yyline++; 
                  return S_NL; }
"//"            { comment(yyscanner); return S_NL; }
";"             { comment(yyscanner); return S_NL; }
{STCOM}         { do_comment(yyscanner); }
"("             { return S_LB; }
")"             { return S_RB; }
"+"             { return S_PLUS; }
"-"             { return S_MINUS; }
"*"             { return S_TIMES; }
"/"             { return S_DIV; }
"%"             { return S_MOD; }
"\^"            { return S_POW; }
"?"             { return S_Q; }
":"             { return S_COL; }
","             { return S_COM; }
"!"             { return S_NOT; }
"!="            { return S_NEQ; }
"&&"            { return S_AND; }
"||"            { return S_OR; }
"<<"            { return S_BITSHL; }
">>"            { return S_BITSHR; }
"<"             { return S_LT; }
"<="            { return S_LE; }
"=="            { return S_EQ; }
"="             { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = S_ASSIGN;
                  return S_ASSIGN; }
">"             { return S_GT; }
">="            { return S_GE; }
"|"             { return S_BITOR; }
"&"             { return S_BITAND; }
"#"             { return S_NEQV; }
"¬"             { return S_BITNOT; }
"~"             { return S_BITNOT; }
"@@"{OPTWHITE}{INTGR}     { *lvalp = do_at(csound, 1, yyg); return T_INTGR; }
"@"{OPTWHITE}{INTGR}      { *lvalp = do_at(csound, 0, yyg); return T_INTGR; }
"if"            { return T_IF; }
"then"          { return T_THEN; }
"ithen"         { return T_ITHEN; }
"kthen"         { return T_KTHEN; }
"elseif"        { return T_ELSEIF; }
"else"          { return T_ELSE; }
"endif"         { return T_ENDIF; }
"until"         { return T_UNTIL; }
"do"            { return T_DO; }
"endloop"       { return T_ENDLOOP; }

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
"nchnls_i"      { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_NCHNLSI;
                  return T_NCHNLSI; }
"instr"         {
                  namedInstrFlag = 1;
                  return T_INSTR;
                }
"endin"         { return T_ENDIN; }
"opcode"        { return T_UDOSTART; }
"endop"         { *lvalp = new_token(csound, T_UDOEND); return T_UDOEND; }

{LABEL}         { *lvalp = make_label(csound, yytext); return T_LABEL; }

{STRCONST}      { *lvalp = make_string(csound, yytext); return (T_STRCONST); }

{STRCONSTe}     { *lvalp = make_string(csound, yytext); 
                  csound->Message(csound, 
                          Str("unterminated string found on line %d >>%s<<\n"),
                          yyline, yytext);
                  return (T_STRCONST); }

"0dbfs"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = T_0DBFS;
                  /* csound->Message(csound,"%d\n", (*lvalp)->type); */
                  return T_0DBFS; }
{IDENT}         { *lvalp = lookup_token(csound, yytext);
                  /* csound->Message(csound,"%d\n", (*lvalp)->type); */
                  return (*lvalp)->type; }
{MACRONAME}     {
                  MACRO     *mm = macros;
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
                  if (UNLIKELY(macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                    csound->Message(csound, Str("Includes nested too deeply"));
                    exit(1);
                  }
                  //include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
                  alt_stack[macro_stack_ptr++] = macros;
                  /* printf("Push %p macro stack; new body #%s#\n", macros, mm->body); */
                  /* printf("Push buffer %p -> ", YY_CURRENT_BUFFER); */
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                  /* printf("%p\n", YY_CURRENT_BUFFER); */
                 }
{MACRONAMED}    {
                  MACRO     *mm = macros;
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
                  if (UNLIKELY(macro_stack_ptr >= MAX_INCLUDE_DEPTH )) {
                    csound->Message(csound, Str("Includes nested too deeply"));
                    exit(1);
                  }
                  //include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
                  alt_stack[macro_stack_ptr++] = macros;
                  /* printf("Push %p macro stack\n", mm_save); */
                  /* printf("Push buffer %p -> ", YY_CURRENT_BUFFER); */
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                  /* printf("%p\n", YY_CURRENT_BUFFER); */
                }
{MACRONAMEA}    {
                  MACRO     *mm = macros, *mm_save = macros;
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
                    char  trm1 = (j == mm->acnt - 1 ? ')' : '#');   /* Compatability */
                    MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                    int   size = 100;
                    nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                    /* printf("Arg %d: %s\n", j+1, mm->arg[j]); */
                    strcpy(nn->name, mm->arg[j]);
                    /* csound->Message(csound, "defining argument %s ", nn->name); */
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
                    nn->next = macros;
                    macros = nn;
                  }
                  /* printf("New body: ...#%s#\n", mm->body); */
                  //include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
                  alt_stack[macro_stack_ptr++] = mm_save;
                  /* printf("Push %p macro stack\n", mm_save); */
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                }
{MACRONAMEDA}    {
                  MACRO     *mm = macros, *mm_save = macros;
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
                    char  trm1 = (j == mm->acnt - 1 ? ')' : '#');   /* Compatability */
                    MACRO *nn = (MACRO*) mmalloc(csound, sizeof(MACRO));
                    int   size = 100;
                    nn->name = mmalloc(csound, strlen(mm->arg[j]) + 1);
                    /* printf("Arg %d: %s\n", j+1, mm->arg[j]); */
                    strcpy(nn->name, mm->arg[j]);
                    /* csound->Message(csound, "defining argument %s ", nn->name); */
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
                    nn->next = macros;
                    macros = nn;
                  }
                  /* printf("New body: ...#%s#\n", mm->body); */
                  //include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
                  alt_stack[macro_stack_ptr++] = mm_save;
                  /* printf("Push %p macro stack\n", mm_save); */
                  yypush_buffer_state(YY_CURRENT_BUFFER, yyscanner);
                  yy_scan_string(mm->body, yyscanner);
                }
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
<incl>.         { do_include(csound, yytext[0], yyscanner); /* got the include file name */
                  BEGIN(INITIAL);
                }
<<EOF>>         {
                  MACRO *x, *y=macros;
                  /* printf("Leaving buffer %p\n", YY_CURRENT_BUFFER); */
                  yypop_buffer_state(yyscanner);
                  if ( !YY_CURRENT_BUFFER ) yyterminate();
                  /* printf("End of input; popping to %p\n", YY_CURRENT_BUFFER); */
                  macros = alt_stack[--macro_stack_ptr];
                  /* printf("End of input segment: macro pop %p -> %p\n", y, macros); */
                  /* while (y!=macros) { */
                  /*   int c = y->acnt; */
                  /*   printf("macros %p,%p\n", y, macros); */
                  /*   printf("Free %p\n", y->name); */
                  /*   //free(y->name); */
                  /*   while (c>0) { */
                  /*      printf("Arg %d: %p\n", c, y->arg[c-1]); */
                  /*      //free(y->arg[--c]); */
                  /*      --c; */
                  /*   } */
                  /*   y = y->next; */
                  /* } */
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
                                 yyline,yytext[0],yytext[0]);
                }

%%
void comment(yyscan_t yyscanner)              /* Skip until nextline */
{
    char c;

    while ((c = input(yyscanner)) != '\n'); /* skip */
    yyline++;
}

void do_comment(yyscan_t yyscanner)              /* Skip until * and / chars */
{
    char c;
    for (;;) {
      while ((c=input(yyscanner)) != '*')
          if (UNLIKELY(c=='\n')) yyline++; /* skip */
      if ((c=input(yyscanner))=='/') return;
      if (UNLIKELY(c=='\n')) yyline++;
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
    yyin = fopen( buffer, "r" );
    if (UNLIKELY( ! yyin )) {
      csound->Message(csound, Str("Cannot open file \"%s\"\n"), buffer);
      exit(1);
    }
    alt_stack[macro_stack_ptr++] = macros;
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
      csound->Message(csound, "M-arguments: ");
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
        yyline++;
    }
    mm->body[i] = '\0';
    mm->next = macros;
    macros = mm;
}

void do_macro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    MACRO *mm = (MACRO*) mmalloc(csound, sizeof(MACRO));
    int   arg = 0, i, c;
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
        yyline++;
    }
    mm->body[i] = '\0';
    //    printf("Body #%s#\n", mm->body);
    mm->next = macros;
    macros = mm;
}

void do_umacro(CSOUND *csound, char *name0, yyscan_t yyscanner)
{
    int i,c;
    if (UNLIKELY(csound->oparms->msglevel))
      csound->Message(csound,Str("macro %s undefined\n"), name0);
    if (strcmp(name0, macros->name)==0) {
      MACRO *mm=macros->next;
      mfree(csound, macros->name); mfree(csound, macros->body);
      for (i=0; i<macros->acnt; i++)
        mfree(csound, macros->arg[i]);
      mfree(csound, macros); macros = mm;
    }
    else {
      MACRO *mm = macros;
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
    while ((c=input(yyscanner)) != '\n' && c != EOF);       /* ignore rest of line */
    yyline++;
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
    strncpy(ans->lexeme, s, len - 1); /* Not the trailing colon */
    return ans;
}

ORCTOKEN *make_string(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, T_STRCONST);
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
    ans = new_token(csound, T_INTGR);
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

static void add_math_const_macro(CSOUND *csound, char * name, char *body)
{
    MACRO *mm;

    mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
    mm->name = (char*) mcalloc(csound, strlen(name) + 3);
    sprintf(mm->name, "M_%s", name);
    mm->next = macros;
    macros = mm;
    mm->margs = MARGS;    /* Initial size */
    mm->acnt = 0;
    mm->body = (char*) mcalloc(csound, strlen(body) + 1);
    mm->body = strcpy(mm->body, body);
}

/**
 * Add math constants from math.h as orc macros
 */
void cs_init_math_constants_macros(CSOUND *csound)
 {
     yyline = csound->orcLineOffset; /* deal with offset */
     add_math_const_macro(csound, "E", "2.7182818284590452354");
     add_math_const_macro(csound, "LOG2E", "1.4426950408889634074");
     add_math_const_macro(csound, "LOG10E", "0.43429448190325182765");
     add_math_const_macro(csound, "LN2", "0.69314718055994530942");
     add_math_const_macro(csound, "LN10", "2.30258509299404568402");
     add_math_const_macro(csound, "PI", "3.14159265358979323846");
     add_math_const_macro(csound, "PI_2", "1.57079632679489661923");
     add_math_const_macro(csound, "PI_4", "0.78539816339744830962");
     add_math_const_macro(csound, "1_PI", "0.31830988618379067154");
     add_math_const_macro(csound, "2_PI", "0.63661977236758134308");
     add_math_const_macro(csound, "2_SQRTPI", "1.12837916709551257390");
     add_math_const_macro(csound, "SQRT2", "1.41421356237309504880");
     add_math_const_macro(csound, "SQRT1_2", "0.70710678118654752440");
     add_math_const_macro(csound, "INF", "2147483647.0"); /* ~7 years */
}

void cs_init_omacros(CSOUND *csound, NAMES *nn)
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
      for (mm = macros; mm != NULL; mm = mm->next) {
        if (strcmp(mm->name, mname) == 0)
          break;
      }
      if (mm == NULL) {
        mm = (MACRO*) mcalloc(csound, sizeof(MACRO));
        mm->name = mname;
        mm->next = macros;
        macros = mm;
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
    int i;
    char *p;
  
    nBuffer = 0;
    eof = 0;

    p = fgets(yyextra->buffer, lMaxBuffer, yyin);
    if (UNLIKELY(p == NULL)) {
      if (ferror(yyin))
        return -1;
      eof = 1;
      return 1;
    }

    yyline++;
  
    lBuffer = strlen(yyextra->buffer);

    return 0;
}

int get_next_char(char *b, int maxBuffer, struct yyguts_t* yyg)
{
    int frc;
  
    if (UNLIKELY(  eof  ))
      return 0;
  
    while (  nBuffer >= lBuffer  ) {
      frc = get_next_line(yyg);
      if (UNLIKELY(  frc != 0  ))
        return 0;
    }

    b[0] = yyextra->buffer[nBuffer];
    nBuffer += 1;

    return b[0]==0?0:1;
}

