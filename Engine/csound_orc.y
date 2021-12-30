/*
    csound_orc.y:

    Copyright (C) 2006, 2007
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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
%pure-parser
%parse-param {PARSE_PARM *parm}
%parse-param {void *scanner}
%lex-param { CSOUND * csound }
%lex-param {yyscan_t *scanner}

%token NEWLINE
%token S_NEQ
%token S_AND
%token S_OR
%token S_LT
%token S_LE
%token S_EQ
%token S_ADDIN
%token S_SUBIN
%token S_MULIN
%token S_DIVIN
%token S_GT
%token S_GE
%token S_BITSHIFT_LEFT
%token S_BITSHIFT_RRIGHT

%token LABEL_TOKEN
%token IF_TOKEN

%token T_OPCODE0
%token T_OPCODE0B
%token T_OPCODE
%token T_OPCODEB

%token UDO_TOKEN
%token UDOSTART_DEFINITION
%token UDOEND_TOKEN
%token UDO_ANS_TOKEN
%token UDO_ARGS_TOKEN

%token ERROR_TOKEN

%token T_FUNCTION
%token T_FUNCTIONB

%token INSTR_TOKEN
%token ENDIN_TOKEN
%token GOTO_TOKEN
%token KGOTO_TOKEN
%token IGOTO_TOKEN

%token SRATE_TOKEN
%token KRATE_TOKEN
%token KSMPS_TOKEN
%token NCHNLS_TOKEN
%token NCHNLSI_TOKEN
%token ZERODBFS_TOKEN
%token A4_TOKEN
%token STRING_TOKEN
%token T_IDENT
%token T_IDENTB

%token INTEGER_TOKEN
%token NUMBER_TOKEN
%token THEN_TOKEN
%token ITHEN_TOKEN
%token KTHEN_TOKEN
%token ELSEIF_TOKEN
%token ELSE_TOKEN
%token ENDIF_TOKEN
%token UNTIL_TOKEN
%token WHILE_TOKEN
%token DO_TOKEN
%token OD_TOKEN

%token T_INSTLIST
%token S_ELIPSIS
%token T_ARRAY
%token T_ARRAY_IDENT
%token T_MAPI
%token T_MAPK

%start orcfile
%left '?'
%left S_AND S_OR
%nonassoc THEN_TOKEN ITHEN_TOKEN KTHEN_TOKEN ELSE_TOKEN /* IS THIS NECESSARY? */
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
%right S_ATAT
%right S_AT
%token S_GOTO
%token T_HIGHEST
%error-verbose
%parse-param { CSOUND * csound }
%parse-param { TREE ** astTree }


/* NOTE: Perhaps should use %union feature of bison */

%{
/* #define YYSTYPE ORCTOKEN* */
/* JPff thinks that line must be wrong and is trying this! */
#define YYSTYPE TREE*

#ifndef NULL
#define NULL 0L
#endif
#include "csoundCore.h"
#include <ctype.h>
#include <string.h>
#include "namedins.h"

#include "csound_orc.h"
#include "parse_param.h"

#ifdef PARCS
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#else
#define csp_orc_sa_instr_add(a,b)
#define csp_orc_sa_instr_add_tree(a,b)
#define csp_orc_sa_instr_finalize(a)
#define csp_orc_sa_global_read_write_add_list(a,b,c)
#define csp_orc_sa_globals_find(a,b)
#define csp_orc_sa_global_read_write_add_list1(a,b,c)
#define csp_orc_sa_interlocks(a, b)
#define csp_orc_sa_global_read_add_list(a,b)
#define csp_orc_sa_global_write_add_list(a,b);
#endif

#define udoflag csound->parserUdoflag

#define namedInstrFlag csound->parserNamedInstrFlag

    extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
    extern int csound_orclex(TREE**, CSOUND *, void *);
    extern void print_tree(CSOUND *, char *msg, TREE *);
    extern TREE* constant_fold(CSOUND *, TREE *);
    extern void csound_orcerror(PARSE_PARM *, void *, CSOUND *,
                                TREE**, const char*);
    extern int add_udo_definition(CSOUND*, char *, char *, char *);
    extern ORCTOKEN *lookup_token(CSOUND*,char*,void*);
#define LINE csound_orcget_lineno(scanner)
#define LOCN csound_orcget_locn(scanner)
    extern uint64_t csound_orcget_locn(void *);
    extern int csound_orcget_lineno(void *);
    extern ORCTOKEN *make_string(CSOUND *, char *);
%}
%%

orcfile           : rootstatement
                        {
                          csound->synterrcnt = csound_orcnerrs;
                          if (UNLIKELY(csound->oparms->odebug))
                            print_tree(csound, "ALL", $1);
                          $1 = constant_fold(csound, $1);
                          if (UNLIKELY(csound->oparms->odebug))
                            print_tree(csound, "Folded", $1);
                          if ($1 != NULL)
                            *astTree = ((TREE *)$1);
                        }
                  ;

rootstatement     : rootstatement topstatement
                        {
                        $$ = appendToTree(csound, $1, $2);
                        }
                  | rootstatement instrdecl
                        {
                        $$ = appendToTree(csound, $1, $2);
                        }
                  | rootstatement udodecl
                        {
                        $$ = appendToTree(csound, $1, $2);
                        }
                  | topstatement
                  | instrdecl
                  | udodecl
                  ;

instlist  : INTEGER_TOKEN ',' instlist
          { $$ = make_node(csound, LINE, LOCN, T_INSTLIST,
                               make_leaf(csound, LINE,LOCN,
                                         INTEGER_TOKEN, (ORCTOKEN *)$1), $3); }
          | label ',' instlist
              {
                  csp_orc_sa_instr_add(csound, ((ORCTOKEN *)$1)->lexeme);
                  $$ = make_node(csound,LINE,LOCN, T_INSTLIST,
                               make_leaf(csound, LINE,LOCN,
                                         T_IDENT, (ORCTOKEN *)$1), $3); }
          | '+' label ',' instlist
              {
                  TREE *ans;
                  ans = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)$2);
                  ans->rate = (int) '+';
                  csp_orc_sa_instr_add(csound, ((ORCTOKEN *)$2)->lexeme);
                  $$ = make_node(csound,LINE,LOCN, T_INSTLIST, ans, $4); }
          | '+' label
              {
                  TREE *ans;
                  ans = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)$2);
                  ans->rate = (int) '+';
                  $$ = ans; }
          | INTEGER_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                           INTEGER_TOKEN, (ORCTOKEN *)$1); }
          | label { $$ = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)$1); }
          ;

instrdecl : INSTR_TOKEN
                { namedInstrFlag = 1; csound_orcput_ilocn(scanner, LINE, LOCN); }
            instlist NEWLINE
                { namedInstrFlag = 0;
                  csp_orc_sa_instr_add_tree(csound, $3);
                }
            statementlist ENDIN_TOKEN NEWLINE
                {
                    $$ = make_node(csound, csound_orcget_iline(scanner),
                                   csound_orcget_ilocn(scanner), INSTR_TOKEN,
                                   $3, $6);
                    csp_orc_sa_instr_finalize(csound);
                }
          | INSTR_TOKEN NEWLINE error
                {
                    namedInstrFlag = 0;
                    csoundErrorMsg(csound, Str("No number following instr\n"));
                    csp_orc_sa_instr_finalize(csound);
                    $$ = NULL;
                }
          ;

udoname   : T_IDENT    { $$ = (TREE *)$1; }
          | T_OPCODE   { $$ = (TREE *)$1; }
          | T_OPCODE0  { $$ = (TREE *)$1; }

udodecl   : UDOSTART_DEFINITION
                                                { udoflag = -2; }
                  udoname
                                                { udoflag = -1; }
                  ','
                                                { udoflag = 0;}
              UDO_ANS_TOKEN
                        { udoflag = 1; }
              ',' UDO_ARGS_TOKEN NEWLINE
              {
                udoflag = 2;
                add_udo_definition(csound,
                        ((ORCTOKEN *)$3)->lexeme,
                        ((ORCTOKEN *)$7)->lexeme,
                        ((ORCTOKEN *)$10)->lexeme);
              }
              statementlist UDOEND_TOKEN NEWLINE
              {
                TREE *udoTop = make_leaf(csound, LINE,LOCN, UDO_TOKEN,
                                         (ORCTOKEN *)NULL);
                TREE *ident = make_leaf(csound, LINE,LOCN, T_IDENT,
                                        (ORCTOKEN *)$3);
                TREE *udoAns = make_leaf(csound, LINE,LOCN, UDO_ANS_TOKEN,
                                         (ORCTOKEN *)$7);
                TREE *udoArgs = make_leaf(csound, LINE,LOCN, UDO_ARGS_TOKEN,
                                          (ORCTOKEN *)$10);
                udoflag = -1;
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "UDO COMPLETE\n");

                udoTop->left = ident;
                ident->left = udoAns;
                ident->right = udoArgs;

                udoTop->right = (TREE *)$13;

                $$ = udoTop;

                if (UNLIKELY(PARSER_DEBUG))
                  print_tree(csound, "UDO\n", (TREE *)$$);

              }
            ;


statementlist : statementlist statement
                {
                    $$ = appendToTree(csound, (TREE *)$1, (TREE *)$2);
                }
                | /* null */          { $$ = NULL;  }
                ;

topstatement : rident '=' expr NEWLINE
                {

                  TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)$2);
                  ans->left = (TREE *)$1;
                  ans->right = (TREE *)$3;
                  $$ = ans;
                }
                | statement { $$ = $1; }

             ;

statement : ans '=' exprlist NEWLINE
                {
                    //int op = ($1->value->lexeme[0]!='a')?'=':LOCAL_ASSIGN;
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)$2);
                  ans->left = (TREE *)$1;
                  //print_tree(csound, "****assign", ans);
                  ans->right = (TREE *)$3;
                  $$ = ans;
                  if (namedInstrFlag!=2)
                    csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                }
          | ident S_ADDIN expr NEWLINE
                {
                    if ($1->value->lexeme[0]=='g') {
                      TREE *ans = $$ = make_leaf(csound,LINE,LOCN, T_OPCODE,
                                                 lookup_token(csound,
                                                              "##addin", NULL));
                       ans->right = $3;
                       ans->left = $1;
                       ans->value->optype = NULL;
                       if (namedInstrFlag!=2) {
                         csp_orc_sa_global_read_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->right));
                         csp_orc_sa_global_read_write_add_list1(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->left));
                       }
                       $$ = ans;
                    }
                    else {
                      TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                            make_token(csound, "="));
                      ORCTOKEN *repeat = make_token(csound, $1->value->lexeme);
                      ans->left = (TREE *)$1;
                      ans->right = make_node(csound,LINE,LOCN, '+',
                                             make_leaf(csound,LINE,LOCN,
                                                       $1->value->type, repeat),
                                         (TREE *)$3);
                      $$ = ans;
                      if (namedInstrFlag!=2)
                        csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                    }
                }
          | ident S_SUBIN expr NEWLINE
                {
                    if ($1->value->lexeme[0]=='g') {
                      TREE *ans = $$ = make_leaf(csound,LINE,LOCN, T_OPCODE,
                                                 lookup_token(csound,
                                                              "##subin", NULL));
                      ans->right = $3;
                      ans->left = $1;
                      ans->value->optype = NULL;
                      if (namedInstrFlag!=2) {
                         csp_orc_sa_global_read_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->right));
                        csp_orc_sa_global_read_write_add_list1(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->left));
                      }
                      $$ = ans;
                    }
                    else {
                      TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                            make_token(csound, "="));
                      ORCTOKEN *repeat = make_token(csound, $1->value->lexeme);
                      ans->left = (TREE *)$1;
                      ans->right = make_node(csound,LINE,LOCN, '-',
                                             make_leaf(csound,LINE,LOCN,
                                                       $1->value->type, repeat),
                                             (TREE *)$3);
                      //print_tree(csound, "-=", ans);
                      $$ = ans;
                      if (namedInstrFlag!=2)
                        csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                    }
                }
          | ident S_MULIN expr NEWLINE
                {
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                        make_token(csound, "="));
                  ORCTOKEN *repeat = make_token(csound, $1->value->lexeme);
                  ans->left = (TREE *)$1;
                  ans->right = make_node(csound,LINE,LOCN, '*',
                                         make_leaf(csound,LINE,LOCN,
                                                   $1->value->type, repeat),
                                         (TREE *)$3);
                  //print_tree(csound, "-=", ans);
                  $$ = ans;
                  if (namedInstrFlag!=2)
                    csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                }
          | ident S_DIVIN expr NEWLINE
                {
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                        make_token(csound, "="));
                  ORCTOKEN *repeat = make_token(csound, $1->value->lexeme);
                  ans->left = (TREE *)$1;
                  ans->right = make_node(csound,LINE,LOCN, '/',
                                         make_leaf(csound,LINE,LOCN,
                                                   $1->value->type, repeat),
                                         (TREE *)$3);
                  //print_tree(csound, "-=", ans);
                  $$ = ans;
                  if (namedInstrFlag!=2)
                    csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                }
          | arrayexpr '=' expr NEWLINE
          {
              TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)$2);
              ans->left = (TREE *)$1;
              ans->right = (TREE *)$3;
              $$ = ans;

          }
          | arrayident '=' expr NEWLINE
          {
              TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)$2);
              ans->left = (TREE *)$1;
              ans->right = (TREE *)$3;
              $$ = ans;

          }

          | ans opcode exprlist NEWLINE
                {
                  $2->left = $1;
                  $2->right = $3;
                  $2->value->optype = NULL;
                  $$ = $2;

                  if (namedInstrFlag!=2) {
                    csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, $2->left),
                                    csp_orc_sa_globals_find(csound, $2->right));
                    csp_orc_sa_interlocks(csound, $2->value);
                  }
                  query_deprecated_opcode(csound, $2->value);
                  //print_tree(csound, "opcode", $$);
                }

           | opcode0  exprlist NEWLINE
                {
                  ((TREE *)$1)->left = NULL;
                  ((TREE *)$1)->right = (TREE *)$2;
                  $1->value->optype = NULL;
                  $$ = $1;
                  if (namedInstrFlag!=2) {
                    csp_orc_sa_global_read_add_list(csound,
                                  csp_orc_sa_globals_find(csound,
                                                          $1->right));
                    if (UNLIKELY(query_reversewrite_opcode(csound, $1->value))) {
                      csp_orc_sa_global_write_add_list(csound,
                                   csp_orc_sa_globals_find(csound, $1->right));
                    }
                    csp_orc_sa_interlocks(csound, $1->value);
                    query_deprecated_opcode(csound, $1->value);
                  }
                }
            | opcode0b exprlist ')' NEWLINE
                {   /* VL: to allow general func ops with no answers */
                  ((TREE *)$1)->left = NULL;
                  ((TREE *)$1)->right = (TREE *)$2;
                  $1->value->optype = NULL;
                  $$ = $1;

                  if (namedInstrFlag!=2) {
                    csp_orc_sa_global_read_add_list(csound,
                                  csp_orc_sa_globals_find(csound,
                                                          $1->right));

                  csp_orc_sa_interlocks(csound, $1->value);
                  }
                  query_deprecated_opcode(csound, $1->value);

                }
          | LABEL_TOKEN
                {
                    //printf("label %s\n", ((ORCTOKEN *)$1)->lexeme);
                    $$ = make_leaf(csound,LINE,LOCN, LABEL_TOKEN, (ORCTOKEN *)$1);
                }
          | goto label NEWLINE
                {
                    $1->left = NULL;
                    $1->right = make_leaf(csound, LINE,LOCN,
                                          LABEL_TOKEN, (ORCTOKEN *)$2);
                    $$ = $1;
                }
          | IF_TOKEN bexpr goto label NEWLINE
                {
                    $3->left = NULL;
                    $3->right = make_leaf(csound, LINE,LOCN,
                                          LABEL_TOKEN, (ORCTOKEN *)$4);
                    $$ = make_node(csound,LINE,LOCN, IF_TOKEN, $2, $3);
                }
          | ifthen
          | UNTIL_TOKEN bexpr DO_TOKEN statementlist OD_TOKEN
              {
                  if ($2) {
                    $$ = make_leaf(csound,$2->line,$2->locn,
                                   UNTIL_TOKEN, (ORCTOKEN *)$1);
                    $$->left = $2;
                    $$->right = $4;
                  }
                  else $$ = NULL;
              }
          | WHILE_TOKEN bexpr DO_TOKEN statementlist OD_TOKEN
              {
                  if ($2) {
                    $$ = make_leaf(csound,$2->line,$2->locn,
                                    WHILE_TOKEN, (ORCTOKEN *)$1);
                    $$->left = $2;
                    $$->right = $4;
                  }
                  else $$ = NULL;
              }
          | NEWLINE { $$ = NULL; }
          ;
ans       : ident               { $$ = $1; }
          | arrayident          { $$ = $1; }
          | arrayexpr           { $$ = $1; }
          | T_IDENT error
              {
                 csoundErrorMsg(csound,
                      "Unexpected untyped word %s when expecting a variable\n",
                      ((ORCTOKEN*)$1)->lexeme);
                $$ = NULL;
              }
          | ans ',' ident     { $$ = appendToTree(csound, $1, $3); }
          | ans ',' T_IDENT error
              {
                 csoundErrorMsg(csound,
                      "Unexpected untyped word %s when expecting a variable\n",
                               ((ORCTOKEN*)$3)->lexeme);
                $$ = appendToTree(csound, $1, NULL);
              }
          | ans ',' arrayident     { $$ = appendToTree(csound, $1, $3); }
          | ans ',' arrayexpr     { $$ = appendToTree(csound, $1, $3); }
          ;

arrayexpr :  arrayexpr '[' iexp ']'
          {
            appendToTree(csound, $1->right, $3);
            $$ = $1;
          }
          | ident '[' iexp ']'
          {
           char* arrayName = $1->value->lexeme;
            $$ = make_node(csound, LINE, LOCN, T_ARRAY,
           make_leaf(csound, LINE, LOCN, T_IDENT,
                     make_token(csound, arrayName)), $3);

          }
          ;

ifthen    : IF_TOKEN bexpr then NEWLINE statementlist ENDIF_TOKEN NEWLINE
              {
                  if ($2)
                  $$ = make_node(csound,$2->line, $2->locn, IF_TOKEN, $2, $3);
                  else $$ = NULL;
                  $3->right = $5;
                  //print_tree(csound, "if-endif", $$);
              }
          | IF_TOKEN bexpr then NEWLINE statementlist ELSE_TOKEN
                                        statementlist ENDIF_TOKEN NEWLINE
          {
            $3->right = $5;
            if ($5 != NULL)
              $3->next = make_node(csound,$5->line, $5->locn, ELSE_TOKEN, NULL, $7);
            else
              $3->next = make_node(csound,1+($3->line),$3->locn,
                                   ELSE_TOKEN, NULL, $7);
            $$ = make_node(csound,$2->line, $2->locn, IF_TOKEN, $2, $3);
            //print_tree(csound, "if-else", $$);

          }
        | IF_TOKEN bexpr then NEWLINE statementlist elseiflist ENDIF_TOKEN NEWLINE
          {
            if (UNLIKELY(PARSER_DEBUG))
                csound->Message(csound, "IF-ELSEIF FOUND!\n");
            $3->right = $5;
            $3->next = $6;
            $$ = make_node(csound, $2->line, $2->locn, IF_TOKEN, $2, $3);
            //print_tree(csound, "if-elseif\n", $$);
          }
          | IF_TOKEN bexpr then NEWLINE statementlist elseiflist ELSE_TOKEN
            statementlist ENDIF_TOKEN NEWLINE
          {
            TREE * tempLastNode;

            $3->right = $5;
            $3->next = $6;

            $$ = make_node(csound, $2->line, $2->locn, IF_TOKEN, $2, $3);

            tempLastNode = $$;

            while (tempLastNode->right!=NULL && tempLastNode->right->next!=NULL) {
                tempLastNode = tempLastNode->right->next;
            }

            if ($8)
              tempLastNode->right->next = make_node(csound, $8->line,$8->locn,
                                                    ELSE_TOKEN, NULL, $8);
            else
              tempLastNode->right->next = make_node(csound, LINE,LOCN,
                                                    ELSE_TOKEN, NULL, $8);
            //print_tree(csound, "IF TREE", $$);
          }
          ;

elseiflist : elseiflist elseif
            {
                TREE * tempLastNode = $1;

                while (tempLastNode->right!=NULL &&
                       tempLastNode->right->next!=NULL) {
                    tempLastNode = tempLastNode->right->next;
                }

                tempLastNode->right->next = $2;
                $$ = $1;
            }
            | elseif { $$ = $1; }
           ;

elseif    : ELSEIF_TOKEN bexpr then NEWLINE statementlist
            {
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "ELSEIF FOUND!\n");
                $3->right = $5;
                $$ = make_node(csound,$2->line,$2->locn, ELSEIF_TOKEN, $2, $3);
                //print_tree(csound, "ELSEIF", $$);
            }
          ;

then      : THEN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, THEN_TOKEN, (ORCTOKEN *)$1); }
          | KTHEN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, KTHEN_TOKEN, (ORCTOKEN *)$1); }
          | ITHEN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, ITHEN_TOKEN, (ORCTOKEN *)$1); }
          ;

goto  : GOTO_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, GOTO_TOKEN, (ORCTOKEN *)$1); }
          | KGOTO_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, KGOTO_TOKEN, (ORCTOKEN *)$1); }
          | IGOTO_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, IGOTO_TOKEN, (ORCTOKEN *)$1); }
          ;

/* Allow all words as a label */
label : T_OPCODE    { $$ = (TREE *)$1; }
      | T_OPCODE0   { $$ = (TREE *)$1; }
      | T_FUNCTION  { $$ = (TREE *)$1; }
      | T_IDENT     { $$ = (TREE *)$1; }
      | IF_TOKEN    { $$ = (TREE *)$1; }
      | THEN_TOKEN  { $$ = (TREE *)$1; }
      | ITHEN_TOKEN { $$ = (TREE *)$1; }
      | KTHEN_TOKEN { $$ = (TREE *)$1; }
      | ELSEIF_TOKEN { $$ = (TREE *)$1; }
      | ENDIF_TOKEN { $$ = (TREE *)$1; }
      | UNTIL_TOKEN { $$ = (TREE *)$1; }
      | DO_TOKEN    { $$ = (TREE *)$1; }
      | OD_TOKEN    { $$ = (TREE *)$1; }
      | INTEGER_TOKEN { $$ = (TREE *)$1; }
      | ENDIN_TOKEN { $$ = (TREE *)$1; }
      ;


exprlist  : exprlist ',' expr
                {
                    /* $$ = make_node(',', $1, $3); */
                    $$ = appendToTree(csound, $1, $3);

                }
          | exprlist ',' label
                {
                    /* $$ = make_node(',', $1, $3); */
                    $$ = appendToTree(csound, $1,
                                      make_leaf(csound, LINE,LOCN,
                                                LABEL_TOKEN, (ORCTOKEN *)$3));
                }
          | exprlist ',' error { $$ = NULL; }
          | expr { $$ = $1; }
          | bexpr { $$ = $1; }
          | T_IDENT { $$ = make_leaf(csound, LINE,LOCN,
                                     LABEL_TOKEN, (ORCTOKEN *)$1);  }
          | T_OPCODE   { $$ = make_leaf(csound, LINE,LOCN,
                                        LABEL_TOKEN, (ORCTOKEN *)$1); }
          | T_FUNCTION { $$ = make_leaf(csound, LINE,LOCN,
                                        LABEL_TOKEN, (ORCTOKEN *)$1); }
          | /* null */          { $$ = NULL; }
          ;


bexpr     : '(' bexpr ')'       { $$ = $2; }
          | expr S_LE expr      { $$ = make_node(csound, LINE,LOCN, S_LE, $1, $3); }
          | expr S_LE error     { $$ = NULL; }
          | expr S_GE expr      { $$ = make_node(csound, LINE,LOCN, S_GE, $1, $3); }
          | expr S_GE error     { $$ = NULL; }
          | expr S_NEQ expr     { $$ = make_node(csound, LINE,LOCN, S_NEQ, $1, $3); }
          | expr S_NEQ error    { $$ = NULL; }
          | expr S_EQ expr      { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $3); }
          | expr S_EQ error     { $$ = NULL; }
          | expr '=' expr       { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $3); }
          | expr '=' error      { $$ = NULL; }
          | expr S_GT expr      { $$ = make_node(csound, LINE,LOCN, S_GT, $1, $3); }
          | expr S_GT error     { $$ = NULL; }
          | expr S_LT expr      { $$ = make_node(csound, LINE,LOCN, S_LT, $1, $3); }
          | expr S_LT error     { $$ = NULL; }
          | bexpr S_AND bexpr   { $$ = make_node(csound, LINE,LOCN, S_AND, $1, $3);}
          | bexpr S_AND error   { $$ = NULL; }
          | bexpr S_OR bexpr    { $$ = make_node(csound, LINE,LOCN, S_OR, $1, $3); }
          | bexpr S_OR error    { $$ = NULL; }
          | '!' bexpr %prec S_UNOT { $$ = make_node(csound, LINE,LOCN,
                                                    S_UNOT, $2, NULL); }
          | '!' error           { $$ = NULL; }
          ;

expr      : bexpr '?' expr ':' expr %prec '?'
            { $$ = make_node(csound,LINE,LOCN, '?', $1,
                             make_node(csound, LINE,LOCN, ':', $3, $5)); }
          | bexpr '?' expr ':' error     { $$ = NULL; }
          | bexpr '?' expr error { $$ = NULL; }
          | bexpr '?' error     { $$ = NULL; }
          | iexp                { $$ = $1; }
          ;

iexp      : iexp '+' iexp   { $$ = make_node(csound, LINE,LOCN, '+', $1, $3); }
          | iexp '+' error  { $$ = NULL; }
          | iexp '-' iexp   { $$ = make_node(csound ,LINE,LOCN, '-', $1, $3); }
          | iexp '-' error  { $$ = NULL; }
          | '-' iexp %prec S_UMINUS
            {
                $$ = make_node(csound,LINE,LOCN, S_UMINUS, NULL, $2);
            }
          | '-' error           { $$ = NULL; }
          | '+' iexp %prec S_UMINUS
            {
                $$ = $2;
            }
          | '+' error           { $$ = NULL; }
          | iterm               { $$ = $1; }
          ;

iterm     : iexp '*' iexp    { $$ = make_node(csound, LINE,LOCN, '*', $1, $3); }
          | iexp '*' error   { $$ = NULL; }
          | iexp '/' iexp    { $$ = make_node(csound, LINE,LOCN, '/', $1, $3); }
          | iexp '/' error   { $$ = NULL; }
          | iexp '^' iexp    { $$ = make_node(csound, LINE,LOCN, '^', $1, $3); }
          | iexp '^' error   { $$ = NULL; }
          | iexp '%' iexp    { $$ = make_node(csound, LINE,LOCN, '%', $1, $3); }
          | iexp '%' error   { $$ = NULL; }
          | ifac                { $$ = $1;  }
          ;

ifac      : ident               { $$ = $1; }
          | constant            { $$ = $1; }
          | arrayexpr           { $$ = $1; }
          | iexp '|' iexp        { $$ = make_node(csound, LINE,LOCN, '|', $1, $3); }
          | iexp '|' error       { $$ = NULL; }
          | iexp '&' iexp        { $$ = make_node(csound, LINE,LOCN, '&', $1, $3); }
          | iexp '&' error       { $$ = NULL; }
          | iexp '#' iexp        { $$ = make_node(csound, LINE,LOCN, '#', $1, $3); }
          | iexp '#' error       { $$ = NULL; }
          | iexp S_BITSHIFT_LEFT iexp
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_LEFT, $1, $3); }
          | iexp S_BITSHIFT_LEFT error { $$ = NULL; }
          | iexp S_BITSHIFT_RIGHT iexp
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_RIGHT, $1, $3); }
          | iexp S_BITSHIFT_RIGHT error { $$ = NULL; }
          | '~' iexp %prec S_UMINUS
            { $$ = make_node(csound, LINE,LOCN, '~', NULL, $2);}
          | '~' error         { $$ = NULL; }
          | '(' expr ')'      { $$ = $2;  }
          | '(' expr error    { $$ = NULL;  }
          | '(' error         { $$ = NULL; }
          | opcode exprlist ')'
            {

                $1->left = NULL;
                $1->right = $2;
                $1->type = T_FUNCTION;
                csp_orc_sa_interlocks(csound, $1->value);
                $$ = $1;
            }
          | opcode ':' opcodeb exprlist ')'   /* needed because a & k are opcodes */
            {
                $1->left = NULL;
                $1->right = $4;
                $1->type = T_FUNCTION;
                $1->value->optype = $3->value->lexeme;

                $$ = $1;
            }
          | opcodeb exprlist ')'
            {
                $1->left = NULL;
                $1->right = $2;
                $1->type = T_FUNCTION;
                $1->value->optype = NULL;
                csp_orc_sa_interlocks(csound, $1->value);
                $$ = $1;
                //print_tree(csound, "FUNCTION CALL", $$);
            }

          | identb error    { $$ = NULL; }
          | opcodeb error   { $$ = NULL; }
          ;

rident    : SRATE_TOKEN     { $$ = make_leaf(csound, LINE,LOCN,
                                             SRATE_TOKEN, (ORCTOKEN *)$1); }
          | KRATE_TOKEN     { $$ = make_leaf(csound, LINE,LOCN,
                                             KRATE_TOKEN, (ORCTOKEN *)$1); }
          | KSMPS_TOKEN     { $$ = make_leaf(csound, LINE,LOCN,
                                             KSMPS_TOKEN, (ORCTOKEN *)$1); }
          | NCHNLS_TOKEN    { $$ = make_leaf(csound, LINE,LOCN,
                                             NCHNLS_TOKEN, (ORCTOKEN *)$1); }
          | NCHNLSI_TOKEN   { $$ = make_leaf(csound, LINE,LOCN,
                                             NCHNLSI_TOKEN, (ORCTOKEN *)$1); }
          | ZERODBFS_TOKEN  { $$ = make_leaf(csound, LINE,LOCN,
                                             ZERODBFS_TOKEN, (ORCTOKEN *)$1); }
          | A4_TOKEN        { $$ = make_leaf(csound, LINE,LOCN,
                                             A4_TOKEN, (ORCTOKEN *)$1); }
          ;


arrayident: arrayident '[' ']' {
            appendToTree(csound, $1->right,
                 make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[")));
            $$ = $1;
          }
          | ident '[' ']' {
            $$ = make_leaf(csound, LINE, LOCN, T_ARRAY_IDENT,
                           make_token(csound, $1->value->lexeme));
            $$->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "["));
          };

ident : T_IDENT { $$ = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)$1); }
identb : T_IDENTB { $$ = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)$1); }

constant  : INTEGER_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                           INTEGER_TOKEN, (ORCTOKEN *)$1); }
          | NUMBER_TOKEN  { $$ = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)$1); }
          | STRING_TOKEN  { $$ = make_leaf(csound, LINE,LOCN,
                                           STRING_TOKEN, (ORCTOKEN *)$1); }
          | SRATE_TOKEN   { $$ = make_leaf(csound, LINE,LOCN,
                                           SRATE_TOKEN, (ORCTOKEN *)$1); }
          | KRATE_TOKEN   { $$ = make_leaf(csound, LINE,LOCN,
                                           KRATE_TOKEN, (ORCTOKEN *)$1); }
          | KSMPS_TOKEN   { $$ = make_leaf(csound, LINE,LOCN,
                                           KSMPS_TOKEN, (ORCTOKEN *)$1); }
          | NCHNLS_TOKEN  { $$ = make_leaf(csound, LINE,LOCN,
                                           NCHNLS_TOKEN, (ORCTOKEN *)$1); }
          | NCHNLSI_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                           NCHNLSI_TOKEN, (ORCTOKEN *)$1); }
          | ZERODBFS_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                            ZERODBFS_TOKEN, (ORCTOKEN *)$1); }
          | A4_TOKEN       { $$ = make_leaf(csound, LINE,LOCN,
                                            A4_TOKEN, (ORCTOKEN *)$1); }
          ;

opcode0   : T_OPCODE0
            {
              if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "opcode0 $1=%p (%s)\n",
                                  $1,((ORCTOKEN *)$1)->lexeme );
                $$ = make_leaf(csound,LINE,LOCN, T_OPCODE0, (ORCTOKEN *)$1);


            }
          ;

opcode0b  : T_OPCODE0B
            {
              if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "opcode0b $1=%p (%s)\n",
                                  $1,((ORCTOKEN *)$1)->lexeme );
                $$ = make_leaf(csound,LINE,LOCN, T_OPCODE0, (ORCTOKEN *)$1);


            }
          ;

opcode    : T_OPCODE
                 { $$ = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)$1); }
          | T_FUNCTION
                 { $$ = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)$1); }
          ;

opcodeb   : T_OPCODEB
                 { $$ = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)$1); }
          | T_FUNCTIONB
                 { $$ = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)$1); }
          ;

%%

#ifdef SOME_FINE_DAY
void
yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if (yylloc.first_line)
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

  if (t.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ", t.first_line, t.first_column,
            t.last_line, t.last_column);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}

#endif
