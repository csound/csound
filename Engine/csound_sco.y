/*
    csound_sco.y:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
%{
#ifndef NULL
#define NULL 0L
#endif
#include "csoundCore.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "score_param.h"
    extern void csound_scoerror(SCORE_PARM *, void *,
                            CSOUND *, ScoreTree *, const char*);
    extern int csound_scolex(ScoreTree**, CSOUND *, void *);

#define LINE csound_scoget_lineno(scanner)
#define LOCN csound_scoget_locn(scanner)
    extern int csound_scoget_locn(void *);
    extern int csound_scoget_lineno(void *);
    static ScoreTree* makesco(CSOUND *csound, int op, ListItem* car,
                              ScoreTree* cdr, int, int);
    static ScoreTree* makesco1(CSOUND *csound, ScoreTree* car, ScoreTree* cdr);
    static ListItem* makelist(CSOUND *csound, double car, ListItem* cdr);

%}

%pure_parser
%parse-param {SCORE_PARM *parm}
%parse-param {void *scanner}
%lex-param { CSOUND * csound }
%lex-param {yyscan_t *scanner}

%union {
  int    oper;
  double val;
  ScoreTree *t;
  ListItem  *l;
}

%type <oper> opcode
%type <val> arg exp term fac constant
%type <t> scoline scolines statement
%type <l> arglist

%token NEWLINE

%token T_ERROR

%token STRING_TOKEN
%token INTEGER_TOKEN
%token NUMBER_TOKEN

%start scofile
%left S_AND S_OR
%left '|'
%left '&'
%left S_LT S_GT S_LEQ S_GEQ S_EQ S_NEQ
%left S_BITSHIFT_LEFT S_BITSHIFT_RIGHT
%left '+' '-'
%left '*' '/' '%'
%left '^'
%left '#'
%right '~'
%right S_UNOT
%right S_UMINUS
%token T_HIGHEST
%pure-parser
%error-verbose

%token T_NP
%token T_PP
%token T_CNP
%token T_CPP
%pure_parser
/* %error-verbose */
%parse-param { CSOUND * csound }
%parse-param { ScoreTree * scoTree }
%%
scofile           : scolines { printf("result %p\n", $1); *scoTree = *$1;}
                  ;

scoline           : statement
                        {
                            $$ = $1;
                        }
                  | '{' scolines '}' NEWLINE
                  {
                      $$ = $2;
                  }
                  ;

scolines          : scoline scolines
                        {
                            $$ = makesco1(csound, $1, $2);
                        }
                  |  { $$ = NULL; }
                  ;

statement         : opcode arglist NEWLINE
                  {
                      printf("op=%c\n", $1);
                      $$ = makesco(csound, $1, NULL, NULL, LINE,LOCN);
                  }
                  ;

opcode            : 'i'    { $$ = 'i'; }
                  | 'f'    { $$ = 'f'; }
                  | 'a'    { $$ = 'a'; }
                  | 'e'    { $$ = 'e'; }
                  | 's'    { $$ = 's'; }
                  | 't'    { $$ = 't'; }
                  ;

arglist           : arg arglist { $$ = makelist(csound,
                                                $1, $2);}
                  |             { $$ = NULL; }
                  ;

arg       : NUMBER_TOKEN { $$ = parm->fval;}
          | INTEGER_TOKEN { $$ = (MYFLT)parm->ival;}
          | STRING_TOKEN {}
          | '[' exp ']' { $$ = $2; }
          | T_NP        { $$ = nan("1"); }
          | T_PP        { $$ = nan("2"); }
          | T_CNP       { $$ = nan("3"); }
          | T_CPP       { $$ = nan("4"); }
          ;

exp       : exp '+' exp             { $$ = $1 + $3; }
          | exp '+' error
          | exp '-' exp             { $$ = $1 - $3; }
          | exp '-' error
          | '-' exp %prec S_UMINUS  { $$ = - $2; }
          | '-' error               {  }
          | '+' exp %prec S_UMINUS  { $$ = $2; }
          | '+' error               {  }
          | term                    { $$ = $1; }
          ;

term      : exp '*' exp    { $$ = $1 * $3; }
          | exp '*' error
          | exp '/' exp    { $$ = $1 / $3; }
          | exp '/' error
          | exp '^' exp    { $$ = pow($1, $3); }
          | exp '^' error
          | exp '%' exp    { $$ = (double)((int)$1 % (int)$3); }
          | exp '%' error
          | fac            { $$ = $1; }
          ;

fac       : constant           { $$ = $1; }
          | exp '|' exp        { $$ = (int)$1 | (int)$3; }
          | exp '|' error
          | exp '&' exp        { $$ = (int)$1 & (int)$3; }
          | exp '&' error
          | exp '#' exp        { $$ = (int)$1 ^ (int)$3; }
          | exp '#' error
          | exp S_BITSHIFT_LEFT exp
                               { $$ = (int)$1 << (int)$3; }
          | exp S_BITSHIFT_LEFT error
          | exp S_BITSHIFT_RIGHT exp
                               { $$ = (int)$1 >> (int)$3; }
          | exp S_BITSHIFT_RIGHT error
          | '~' exp %prec S_UMINUS
            { $$ = ~(int)$2; }
          | '~' error         { $$ = 0; }
          | '(' exp ')'       { $$ = $2; }
          | '(' exp error     { $$ = 0; }
          | '(' error         { $$ = 0; }
          ;

constant  : NUMBER_TOKEN        { $$ = parm->fval;  }
          | INTEGER_TOKEN       { $$ = (MYFLT)parm->ival; }
          ;

%%

#ifdef SOME_FINE_DAY
void
yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if(yylloc.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ",
            yylloc.first_line, yylloc.first_column,
	    yylloc.last_line, yylloc.last_column);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");

}

void
lyyerror(YYLTYPE t, char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if(t.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ", t.first_line, t.first_column,
	    t.last_line, t.last_column);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}

#endif

extern void do_baktrace(CSOUND *csound, uint64_t files);
extern char *csound_scoget_text ( void *scanner );
extern char *csound_scoget_current_pointer(void *yyscanner);

void
csound_scoerror(SCORE_PARM *parm, void *yyscanner, CSOUND *cs, ScoreTree *t,
                const char* str)
{
    char ch;
    char *p = csound_scoget_current_pointer(yyscanner)-1;
    int line = csound_scoget_lineno(yyscanner);
    uint64_t files = csound_scoget_locn(yyscanner);
    if (*p=='\0') line--;
    cs->Message(cs, Str("\nerror: %s  (token \"%s\")"),
                    str, csound_scoget_text(yyscanner));
    do_baktrace(cs, files);
    cs->Message(cs, Str(" line %d:\n>>>"), line);
    while ((ch=*--p) != '\n' && ch != '\0');
    do {
      ch = *++p;
      if (ch == '\n') break;
      cs->Message(cs, "%c", ch);
    } while (ch != '\n' && ch != '\0');
    cs->Message(cs, " <<<\n");
}

int csound_scowrap()
{
#ifdef DEBUG
    printf("\n === END OF INPUT ===\n");
#endif
    return (1);
}

static ScoreTree* makesco(CSOUND *csound, int op, ListItem* car, ScoreTree* cdr,
                          int line, int locn)
{
    ScoreTree* a = (ScoreTree*)csound->Malloc(csound, sizeof(ScoreTree));
    a->op = op;
    a->args = car;
    a->next = cdr;
    a->line = line;
    a->locn  = locn;
    return a;
}

static ScoreTree* makesco1(CSOUND *csound, ScoreTree* car, ScoreTree* cdr)
{
    car->next = cdr;
    return car;
}

static ListItem* makelist(CSOUND *csound, double car, ListItem* cdr)
{
    ListItem* a = (ListItem*)csound->Malloc(csound, sizeof(ListItem));
    a->val = car;
    a->args = cdr;
    return a;
}

#if 0
int yylex(void)
{
    int c;
    while ((c=getchar())==' ');
    if (isdigit(c)) {
      int n = c-'0';
      while (isdigit(c=getchar())) {
        n = 10*n+c-'0';
      }
      ungetc(c, stdin);
      return NUMBER_TOKEN;
    }
    return c=='\n' ? NEWLINE : c;
}

extern int csound_scodebug;
int main(void)
{
    csound_scodebug = 1;
    yyparse();
    return 0;
}

#endif
