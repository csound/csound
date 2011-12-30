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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/
%pure_parser
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
%token S_TASSIGN
%token S_TABREF
%token S_GT
%token S_GE
%token S_BITSHIFT_LEFT
%token S_BITSHIFT_RRIGHT

%token LABEL_TOKEN
%token IF_TOKEN

%token T_OPCODE0
%token T_OPCODE

%token UDO_TOKEN
%token UDOSTART_DEFINITION
%token UDOEND_TOKEN
%token UDO_ANS_TOKEN
%token UDO_ARGS_TOKEN

%token T_ERROR

%token T_FUNCTION

%token INSTR_TOKEN
%token ENDIN_TOKEN
%token T_STRSET
%token T_PSET
%token T_CTRLINIT
%token T_MASSIGN
%token T_TURNON
%token T_PREALLOC
%token T_ZAKINIT
%token T_FTGEN
%token T_INIT
%token GOTO_TOKEN
%token KGOTO_TOKEN
%token IGOTO_TOKEN

%token SRATE_TOKEN
%token KRATE_TOKEN
%token KSMPS_TOKEN
%token NCHNLS_TOKEN
%token NCHNLSI_TOKEN
%token ZERODBFS_TOKEN
%token STRING_TOKEN
%token T_IDENT

%token T_IDENT_I
%token T_IDENT_GI
%token T_IDENT_K
%token T_IDENT_GK
%token T_IDENT_A
%token T_IDENT_GA
%token T_IDENT_W
%token T_IDENT_GW
%token T_IDENT_F
%token T_IDENT_GF
%token T_IDENT_S
%token T_IDENT_GS
%token T_IDENT_T
%token T_IDENT_GT
%token T_IDENT_P
%token T_IDENT_B
%token T_IDENT_b
%token INTEGER_TOKEN
%token NUMBER_TOKEN
%token THEN_TOKEN
%token ITHEN_TOKEN
%token KTHEN_TOKEN
%token ELSEIF_TOKEN
%token ELSE_TOKEN
%token ENDIF_TOKEN
%token UNTIL_TOKEN
%token DO_TOKEN
%token OD_TOKEN

%token T_INSTLIST

%start orcfile
%left '?'
%left S_AND S_OR
%nonassoc THEN_TOKEN ITHEN_TOKEN KTHEN_TOKEN ELSE_TOKEN /* NOT SURE IF THIS IS NECESSARY */
%left '+' '-'
%left '*' '/' '%'
%left '^'
%left '|'
%left '&'
%left '#'
%left S_BITSHIFT_LEFT
%left S_BITSHIFT_RIGHT
%right '~'
%right S_UNOT
%right S_UMINUS
%right S_ATAT
%right S_AT
%nonassoc S_LT S_GT S_LEQ S_GEQ S_EQ S_NEQ
%token S_GOTO
%token T_HIGHEST
%pure_parser
%error-verbose
%parse-param { CSOUND * csound }
%parse-param { TREE * astTree }

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
#include "namedins.h"

#include "csound_orc.h"
#ifdef PARCS
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#endif
#include "parse_param.h"

    //int udoflag = -1; /* THIS NEEDS TO BE MADE NON-GLOBAL */
#define udoflag csound->parserUdoflag

   //int namedInstrFlag = 0; /* THIS NEEDS TO BE MADE NON-GLOBAL */
#define namedInstrFlag csound->parserNamedInstrFlag

extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
extern int csound_orclex(TREE**, CSOUND *, void *);
extern void print_tree(CSOUND *, char *msg, TREE *);
extern void csound_orcerror(PARSE_PARM *, void *, CSOUND *, TREE*, const char*);
extern void add_udo_definition(CSOUND*, char *, char *, char *);
extern ORCTOKEN *lookup_token(CSOUND*,char*,void*);
#define LINE csound_orcget_lineno(scanner)
#define LOCN csound_orcget_locn(scanner)
extern int csound_orcget_lineno(void *);
%}
%%

orcfile           : rootstatement
                        {
                            *astTree = *((TREE *)$1);
                            csound->synterrcnt = csound_orcnerrs;
                            //print_tree(csound, "ALL", $1);
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
          | T_IDENT ',' instlist
              {
#ifdef PARCS
                  csp_orc_sa_instr_add(csound, ((ORCTOKEN *)$1)->lexeme);
#endif
                  $$ = make_node(csound,LINE,LOCN, T_INSTLIST,
                               make_leaf(csound, LINE,LOCN,
                                         T_IDENT, (ORCTOKEN *)$1), $3); }
          | INTEGER_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                           INTEGER_TOKEN, (ORCTOKEN *)$1); }
          | T_IDENT { $$ = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)$1); }
          ;

instrdecl : INSTR_TOKEN
                { namedInstrFlag = 1; }
            instlist NEWLINE
                { namedInstrFlag = 0;
#ifdef PARCS
                  csp_orc_sa_instr_add_tree(csound, $3);
#endif
                }
            statementlist ENDIN_TOKEN NEWLINE
                {
                    $$ = make_node(csound, LINE,LOCN, INSTR_TOKEN, $3, $6);
#ifdef PARCS
                    csp_orc_sa_instr_finalize(csound);
#endif
                }
          | INSTR_TOKEN NEWLINE error
                {
                    namedInstrFlag = 0;
                    csound->Message(csound, Str("No number following instr\n"));
#ifdef PARCS
                    csp_orc_sa_instr_finalize(csound);
#endif
                }
          ;

udodecl   : UDOSTART_DEFINITION
                                                { udoflag = -2; }
                  T_IDENT
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
                | /* null */          { $$ = NULL; }
                ;

topstatement : rident '=' expr NEWLINE
                {

                  TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)$2);
                  ans->left = (TREE *)$1;
                  ans->right = (TREE *)$3;
                  /* ans->value->lexeme = get_assignment_type(csound,
                      ans->left->value->lexeme, ans->right->value->lexeme); */

                  $$ = ans;
                }
                | statement { $$ = $1; }

             ;

statement : ident '=' expr NEWLINE
                {
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)$2);
                  ans->left = (TREE *)$1;
                  ans->right = (TREE *)$3;
                  /* ans->value->lexeme = get_assignment_type(csound,
                     ans->left->value->lexeme, ans->right->value->lexeme); */

                  $$ = ans;
#ifdef PARCS
                  csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
#endif
                }
          | T_IDENT_T '=' T_IDENT_T NEWLINE
          {
              ORCTOKEN *op = lookup_token(csound, "#copytab", NULL);
              TREE *ans = make_leaf(csound,LINE,LOCN, T_OPCODE, op);
              ans->left = make_leaf(csound,LINE,LOCN, T_IDENT_T, (ORCTOKEN *)$1);
              ans->right = make_leaf(csound,LINE,LOCN, T_IDENT_T, (ORCTOKEN *)$3);
              $$ = ans;
          }
          | T_IDENT_T '[' iexp ']' '=' expr NEWLINE
          {
              TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)$5);
              ans->left = make_leaf(csound,LINE,LOCN, T_IDENT_T, (ORCTOKEN *)$1);
              ans->right = appendToTree(csound, $3, $6);
                  /* ans->value->lexeme = get_assignment_type(csound,
                     ans->left->value->lexeme, ans->right->value->lexeme); */
              //print_tree(csound, "TABLE ASSIGN", ans);
              $$ = ans;
  /* #ifdef PARCS */
  /*                   csp_orc_sa_global_read_write_add_list(csound, */
  /*                      csp_orc_sa_globals_find(csound, ans->left) */
  /*                   csp_orc_sa_globals_find(csound, ans->right)); */
  /* #endif */
          }
          | ans opcode exprlist NEWLINE
                {

                  $2->left = $1;
                  $2->right = $3;

                  $$ = $2;
#ifdef PARCS
                  csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, $2->left),
                                    csp_orc_sa_globals_find(csound, $2->right));
                  csp_orc_sa_interlocks(csound, $2->value);
#endif
                }
          | opcode0 exprlist NEWLINE
                {
                  ((TREE *)$1)->left = NULL;
                  ((TREE *)$1)->right = (TREE *)$2;

                  $$ = $1;
#ifdef PARCS
                  csp_orc_sa_global_read_add_list(csound,
                                  csp_orc_sa_globals_find(csound,
                                                          $1->right));
                  csp_orc_sa_interlocks(csound, $1->value);
#endif
                }
          | LABEL_TOKEN
                {
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
                  $$ = make_leaf(csound,LINE,LOCN, UNTIL_TOKEN, (ORCTOKEN *)$1);
                  $$->left = $2;
                  $$->right = $4;
              }
          | NEWLINE { $$ = NULL; }
          ;
ans       : ident               { $$ = $1; }
          | T_IDENT error       
              { csound->Message(csound,
                      "Unexpected untyped word %s when expecting a variable\n", 
                      ((ORCTOKEN*)$1)->lexeme);
                $$ = NULL;
              }
          | ans ',' ident     { $$ = appendToTree(csound, $1, $3); }
          | ans ',' T_IDENT error  
              { csound->Message(csound,
                      "Unexpected untyped word %s when expecting a variable\n", 
                               ((ORCTOKEN*)$3)->lexeme);
                $$ = appendToTree(csound, $1, NULL);
              }
          ;

ifthen    : IF_TOKEN bexpr then NEWLINE statementlist ENDIF_TOKEN NEWLINE
          {
            $3->right = $5;
            $$ = make_node(csound,LINE,LOCN, IF_TOKEN, $2, $3);
            //print_tree(csound, "if-endif", $$);
          }
          | IF_TOKEN bexpr then NEWLINE statementlist ELSE_TOKEN
                                        statementlist ENDIF_TOKEN NEWLINE
          {
            $3->right = $5;
            $3->next = make_node(csound,LINE,LOCN, ELSE_TOKEN, NULL, $7);
            $$ = make_node(csound,LINE,LOCN, IF_TOKEN, $2, $3);
            //print_tree(csound, "if-else", $$);

          }
        | IF_TOKEN bexpr then NEWLINE statementlist elseiflist ENDIF_TOKEN NEWLINE
          {
            if (UNLIKELY(PARSER_DEBUG))
                csound->Message(csound, "IF-ELSEIF FOUND!\n");
            $3->right = $5;
            $3->next = $6;
            $$ = make_node(csound, LINE,LOCN, IF_TOKEN, $2, $3);
            //print_tree(csound, "if-elseif\n", $$);
          }
          | IF_TOKEN bexpr then NEWLINE statementlist elseiflist ELSE_TOKEN
            statementlist ENDIF_TOKEN NEWLINE
          {
            TREE * tempLastNode;

            $3->right = $5;
            $3->next = $6;

            $$ = make_node(csound, LINE,LOCN, IF_TOKEN, $2, $3);

            tempLastNode = $$;

            while (tempLastNode->right!=NULL && tempLastNode->right->next!=NULL) {
                tempLastNode = tempLastNode->right->next;
            }

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
                $$ = make_node(csound,LINE,LOCN, ELSEIF_TOKEN, $2, $3);
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
      | T_IDENT_P   { $$ = (TREE *)$1; }
      | T_IDENT_I   { $$ = (TREE *)$1; }
      | T_IDENT_GI  { $$ = (TREE *)$1; }
      | T_IDENT_K   { $$ = (TREE *)$1; }
      | T_IDENT_GK  { $$ = (TREE *)$1; }
      | T_IDENT_A   { $$ = (TREE *)$1; }
      | T_IDENT_GA  { $$ = (TREE *)$1; }
      | T_IDENT_W   { $$ = (TREE *)$1; }
      | T_IDENT_GW  { $$ = (TREE *)$1; }
      | T_IDENT_F   { $$ = (TREE *)$1; }
      | T_IDENT_GF  { $$ = (TREE *)$1; }
      | T_IDENT_S   { $$ = (TREE *)$1; }
      | T_IDENT_GS  { $$ = (TREE *)$1; }
      | T_IDENT_T   { $$ = (TREE *)$1; }
      | T_IDENT_GT  { $$ = (TREE *)$1; }
      | T_IDENT     { $$ = (TREE *)$1; }
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
          | exprlist ',' error
          | expr { $$ = $1;     }
          | bexpr { $$ = $1; }
          | T_IDENT { $$ = make_leaf(csound, LINE,LOCN, LABEL_TOKEN, (ORCTOKEN *)$1); }
          | /* null */          { $$ = NULL; }
          ;

bexpr     : '(' bexpr ')'       { $$ = $2; }
          | expr S_LE expr      { $$ = make_node(csound, LINE,LOCN, S_LE, $1, $3); }
          | expr S_LE error
          | expr S_GE expr      { $$ = make_node(csound, LINE,LOCN, S_GE, $1, $3); }
          | expr S_GE error
          | expr S_NEQ expr     { $$ = make_node(csound, LINE,LOCN, S_NEQ, $1, $3); }
          | expr S_NEQ error
          | expr S_EQ expr      { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $3); }
          | expr S_EQ error
          | expr '=' expr       { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $3); }
          | expr '=' error
          | expr S_GT expr      { $$ = make_node(csound, LINE,LOCN, S_GT, $1, $3); }
          | expr S_GT error
          | expr S_LT expr      { $$ = make_node(csound, LINE,LOCN, S_LT, $1, $3); }
          | expr S_LT error
          | bexpr S_AND bexpr   { $$ = make_node(csound, LINE,LOCN, S_AND, $1, $3); }
          | bexpr S_AND error
          | bexpr S_OR bexpr    { $$ = make_node(csound, LINE,LOCN, S_OR, $1, $3); }
          | bexpr S_OR error
          | '!' bexpr %prec S_UNOT { $$ = make_node(csound, LINE,LOCN,
                                                    S_UNOT, $2, NULL); }
          | '!' error
          ;

expr      : bexpr '?' expr ':' expr %prec '?'
            { $$ = make_node(csound,LINE,LOCN, '?', $1,
                             make_node(csound, LINE,LOCN, ':', $3, $5)); }
          | bexpr '?' expr ':' error
          | bexpr '?' expr error
          | bexpr '?' error
          | iexp                { $$ = $1; }
          ;

iexp      : iexp '+' iterm   { $$ = make_node(csound, LINE,LOCN, '+', $1, $3); }
          | iexp '+' error
          | iexp '-' iterm  { $$ = make_node(csound ,LINE,LOCN, '-', $1, $3); }
          | iexp '-' error
          | iterm               { $$ = $1; }
          ;

iterm     : iterm '*' ifac  { $$ = make_node(csound, LINE,LOCN, '*', $1, $3); }
          | iterm '*' error
          | iterm '/' ifac    { $$ = make_node(csound, LINE,LOCN, '/', $1, $3); }
          | iterm '/' error
          | iterm '%' ifac    { $$ = make_node(csound, LINE,LOCN, '%', $1, $3); }
          | iterm '%' error
          | ifac                { $$ = $1; }
          ;

ifac      : ident               { $$ = $1; }
          | constant            { $$ = $1; }
          | T_IDENT_T '[' iexp ']'
          {
              $$ = make_node(csound,LINE,LOCN, S_TABREF,
                             make_leaf(csound, LINE,LOCN,
                                       T_IDENT_T, (ORCTOKEN*)$1), $3);
          }
          | '-' ifac %prec S_UMINUS
            {
                $$ = make_node(csound,LINE,LOCN, S_UMINUS, NULL, $2);
            }
          | '+' ifac %prec S_UMINUS
            {
                $$ = $2;
            }
          | ifac '^' ifac        { $$ = make_node(csound, LINE,LOCN, '^', $1, $3); }
          | ifac '|' ifac        { $$ = make_node(csound, LINE,LOCN, '|', $1, $3); }
          | ifac '&' ifac        { $$ = make_node(csound, LINE,LOCN, '&', $1, $3); }
          | ifac '#' ifac        { $$ = make_node(csound, LINE,LOCN, '#', $1, $3); }
          | ifac S_BITSHIFT_LEFT ifac   
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_LEFT, $1, $3); }
          | ifac S_BITSHIFT_RIGHT ifac
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_RIGHT, $1, $3); }
          | '~' ifac %prec S_UMINUS
            { $$ = make_node(csound, LINE,LOCN, '~', NULL, $2);}
          | '-' error
          | '(' expr ')'      { $$ = $2; }
          | '(' expr error
          | '(' error
          | function '(' exprlist ')'
            {
                $1->left = NULL;
                $1->right = $3;

                $$ = $1;
            }
          | function '(' error
          ;

function  : T_FUNCTION  { 
             printf("FUNCTION ans=%p, token=%p %p\n",
                    $1, ((ORCTOKEN *)$1)->value);
#ifdef PARCS
    //                if ((ORCTOKEN *)$1->value != 0)
             csp_orc_sa_interlocksf(csound, ((ORCTOKEN *)$1)->value);
#endif
             $$ = make_leaf(csound, LINE,LOCN, T_FUNCTION, (ORCTOKEN *)$1); 
                }

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
          ;

ident     : T_IDENT_I  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_I, (ORCTOKEN *)$1); }
          | T_IDENT_K  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_K, (ORCTOKEN *)$1); }
          | T_IDENT_F  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_F, (ORCTOKEN *)$1); }
          | T_IDENT_W  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_W, (ORCTOKEN *)$1); }
          | T_IDENT_S  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_S, (ORCTOKEN *)$1); }
          | T_IDENT_T  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_T, (ORCTOKEN *)$1); }
          | T_IDENT_A  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_A, (ORCTOKEN *)$1); }
          | T_IDENT_P  { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_P, (ORCTOKEN *)$1); }
          | gident     { $$ = $1; }
          ;

gident    : T_IDENT_GI { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_GI, (ORCTOKEN *)$1); }
          | T_IDENT_GK { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_GK, (ORCTOKEN *)$1); }
          | T_IDENT_GF { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_GF, (ORCTOKEN *)$1); }
          | T_IDENT_GW { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_GW, (ORCTOKEN *)$1); }
          | T_IDENT_GS { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_GS, (ORCTOKEN *)$1); }
          | T_IDENT_GT { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_GS, (ORCTOKEN *)$1); }
          | T_IDENT_GA { $$ = make_leaf(csound, LINE,LOCN, T_IDENT_GA, (ORCTOKEN *)$1); }
          ;

constant  : INTEGER_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                           INTEGER_TOKEN, (ORCTOKEN *)$1); }
          | NUMBER_TOKEN  { $$ = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)$1); }
          | STRING_TOKEN  { $$ = make_leaf(csound, LINE,LOCN,
                                           STRING_TOKEN, (ORCTOKEN *)$1); }
          | SRATE_TOKEN   { $$ = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)$1); }
          | KRATE_TOKEN   { $$ = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)$1); }
          | KSMPS_TOKEN   { $$ = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)$1); }
          | NCHNLS_TOKEN  { $$ = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)$1); }
          | NCHNLSI_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)$1); }
          | ZERODBFS_TOKEN { $$ = make_leaf(csound, LINE,LOCN,
                                            NUMBER_TOKEN, (ORCTOKEN *)$1); }
          ;

opcode0   : T_OPCODE0
            {
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "opcode0 $1=%p (%s)\n",
                                  $1,((ORCTOKEN *)$1)->lexeme );
                $$ = make_leaf(csound,LINE,LOCN, T_OPCODE0, (ORCTOKEN *)$1);
            }
          ;

opcode    : T_OPCODE    { $$ = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)$1); }
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

