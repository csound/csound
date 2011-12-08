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
%token S_ASSIGN
%token S_TASSIGN
%token S_TABREF
%token S_GT
%token S_GE
%token S_BITAND
%token S_BITOR
%token S_NEQV
%token S_BITSHL
%token S_BITSHR
%token S_BITNOT

%token T_LABEL
%token T_IF

%token T_OPCODE0
%token T_OPCODE

%token T_UDO
%token T_UDOSTART
%token T_UDOEND
%token T_UDO_ANS
%token T_UDO_ARGS

%token T_ERROR

%token T_FUNCTION

%token T_INSTR
%token T_ENDIN
%token T_STRSET
%token T_PSET
%token T_CTRLINIT
%token T_MASSIGN
%token T_TURNON
%token T_PREALLOC
%token T_ZAKINIT
%token T_FTGEN
%token T_INIT
%token T_GOTO
%token T_KGOTO
%token T_IGOTO

%token T_SRATE
%token T_KRATE
%token T_KSMPS
%token T_NCHNLS
%token T_NCHNLSI
%token T_0DBFS
%token T_STRCONST
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
%token T_INTGR
%token T_NUMBER
%token T_THEN
%token T_ITHEN
%token T_KTHEN
%token T_ELSEIF
%token T_ELSE
%token T_ENDIF
%token T_UNTIL
%token T_DO
%token T_OD

%token T_INSTLIST

%start orcfile
%left S_AND S_OR
%nonassoc T_THEN T_ITHEN T_KTHEN T_ELSE /* NOT SURE IF THIS IS NECESSARY */
%left '+' '-'
%left '*' '/' '%'
%left '^'
%left S_BITOR
%left S_BITAND
%left S_NEQV
%left S_BITSHL
%left S_BITSHR
%right S_BITNOT
%right S_UNOT
%right S_UMINUS
%right S_ATAT
%right S_AT
%left '?'
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

instlist  : T_INTGR ',' instlist
              { $$ = make_node(csound, T_INSTLIST,
                               make_leaf(csound, T_INTGR, (ORCTOKEN *)$1), $3); }
          | T_IDENT ',' instlist
              {
#ifdef PARCS
                  csp_orc_sa_instr_add(csound, ((ORCTOKEN *)$1)->lexeme);
#endif
                  $$ = make_node(csound, T_INSTLIST,
                               make_leaf(csound, T_IDENT, (ORCTOKEN *)$1), $3); }
          | T_INTGR { $$ = make_leaf(csound, T_INTGR, (ORCTOKEN *)$1); }
          | T_IDENT { $$ = make_leaf(csound, T_IDENT, (ORCTOKEN *)$1); }
          ;

instrdecl : T_INSTR
                { namedInstrFlag = 1; }
            instlist NEWLINE
                { namedInstrFlag = 0;
#ifdef PARCS
                  csp_orc_sa_instr_add_tree(csound, $3);
#endif
                }
            statementlist T_ENDIN NEWLINE
                {
                    $$ = make_node(csound, T_INSTR, $3, $6);
#ifdef PARCS
                    csp_orc_sa_instr_finalize(csound);
#endif
                }

/*           | T_INSTR */
/*                 { namedInstrFlag = 1; } */
/*             T_IDENT NEWLINE */
/*                 { namedInstrFlag = 0; */
/* #ifdef PARCS */
/*                   csp_orc_sa_instr_add(csound, ((ORCTOKEN *)$3)->lexeme); */
/* #endif */
/*                 } */
/*             statementlist T_ENDIN NEWLINE */
/*                 { */
/*                     TREE *ident = make_leaf(csound, T_IDENT, (ORCTOKEN *)$3); */
/*                     $$ = make_node(csound, T_INSTR, ident, $6); */
/* #ifdef PARCS */
/*                     csp_orc_sa_instr_finalize(csound); */
/* #endif */
/*                 } */

          | T_INSTR NEWLINE error
                {
                    namedInstrFlag = 0;
                    csound->Message(csound, Str("No number following instr\n"));
#ifdef PARCS
                    csp_orc_sa_instr_finalize(csound);
#endif
                }
          ;

udodecl   : T_UDOSTART
                                                { udoflag = -2; }
                  T_IDENT
                                                { udoflag = -1; }
                  ','
                                                { udoflag = 0;}
              T_UDO_ANS
                        { udoflag = 1; }
              ',' T_UDO_ARGS NEWLINE
              {
                udoflag = 2;
                add_udo_definition(csound,
                        ((ORCTOKEN *)$3)->lexeme,
                        ((ORCTOKEN *)$7)->lexeme,
                        ((ORCTOKEN *)$10)->lexeme);
              }
              statementlist T_UDOEND NEWLINE
              {
                TREE *udoTop = make_leaf(csound, T_UDO, (ORCTOKEN *)NULL);
                TREE *ident = make_leaf(csound, T_IDENT, (ORCTOKEN *)$3);
                TREE *udoAns = make_leaf(csound, T_UDO_ANS, (ORCTOKEN *)$7);
                TREE *udoArgs = make_leaf(csound, T_UDO_ARGS, (ORCTOKEN *)$10);
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

topstatement : rident S_ASSIGN expr NEWLINE
                {

                  TREE *ans = make_leaf(csound, S_ASSIGN, (ORCTOKEN *)$2);
                  ans->left = (TREE *)$1;
                  ans->right = (TREE *)$3;
                  /* ans->value->lexeme = get_assignment_type(csound,
                      ans->left->value->lexeme, ans->right->value->lexeme); */

                  $$ = ans;
                }
                | statement { $$ = $1; }

             ;

statement : ident S_ASSIGN expr NEWLINE
                {
                  TREE *ans = make_leaf(csound, S_ASSIGN, (ORCTOKEN *)$2);
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
          | T_IDENT_T S_ASSIGN T_IDENT_T NEWLINE
          {
              ORCTOKEN *op = lookup_token(csound, "#copytab", NULL);
              TREE *ans = make_leaf(csound, T_OPCODE, op);
              ans->left = make_leaf(csound, T_IDENT_T, (ORCTOKEN *)$1);
              ans->right = make_leaf(csound, T_IDENT_T, (ORCTOKEN *)$3);
              $$ = ans;
          }
          | T_IDENT_T '[' iexp ']' S_ASSIGN expr NEWLINE
          {
              TREE *ans = make_leaf(csound, S_ASSIGN, (ORCTOKEN *)$5);
              ans->left = make_leaf(csound, T_IDENT_T, (ORCTOKEN *)$1);
              ans->right = appendToTree(csound, $3, $6);
                  /* ans->value->lexeme = get_assignment_type(csound,
                     ans->left->value->lexeme, ans->right->value->lexeme); */
              //print_tree(csound, "TABLE ASSIGN", ans);
              $$ = ans;
  /* #ifdef PARCS */
  /*                   csp_orc_sa_global_read_write_add_list(csound, */
  /*                                     csp_orc_sa_globals_find(csound, ans->left) */
  /*                                     csp_orc_sa_globals_find(csound, ans->right)); */
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
          | T_LABEL
                {
                    $$ = make_leaf(csound, T_LABEL, (ORCTOKEN *)$1);
                }
          | goto label NEWLINE
                {
                    $1->left = NULL;
                    $1->right = make_leaf(csound, T_LABEL, (ORCTOKEN *)$2);
                    $$ = $1;
                }
          | T_IF expr goto label NEWLINE
                {
                    $3->left = NULL;
                    $3->right = make_leaf(csound, T_LABEL, (ORCTOKEN *)$4);
                    $$ = make_node(csound, T_IF, $2, $3);
                }
          | ifthen
          | T_UNTIL expr T_DO statementlist T_OD
              {
                  $$ = make_leaf(csound, T_UNTIL, (ORCTOKEN *)$1);
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
                $$ = make_leaf(csound, T_SRATE, (ORCTOKEN *)$1);
              }
          | ans ',' ident     { $$ = appendToTree(csound, $1, $3); }
          | ans ',' T_IDENT error  
              { csound->Message(csound,
                      "Unexpected untyped word %s when expecting a variable\n", 
                               ((ORCTOKEN*)$3)->lexeme);
                $$ = appendToTree(csound, $1, 
                               make_leaf(csound, T_SRATE,
                               (ORCTOKEN *)$3));
              }
          ;

ifthen    : T_IF expr then NEWLINE statementlist T_ENDIF NEWLINE
          {
            $3->right = $5;
            $$ = make_node(csound, T_IF, $2, $3);
            //print_tree(csound, "if-endif", $$);
          }
          | T_IF expr then NEWLINE statementlist T_ELSE statementlist T_ENDIF NEWLINE
          {
            $3->right = $5;
            $3->next = make_node(csound, T_ELSE, NULL, $7);
            $$ = make_node(csound, T_IF, $2, $3);
            //print_tree(csound, "if-else", $$);

          }
          | T_IF expr then NEWLINE statementlist elseiflist T_ENDIF NEWLINE
          {
            if (UNLIKELY(PARSER_DEBUG))
                csound->Message(csound, "IF-ELSEIF FOUND!\n");
            $3->right = $5;
            $3->next = $6;
            $$ = make_node(csound, T_IF, $2, $3);
            //print_tree(csound, "if-elseif\n", $$);
          }
          | T_IF expr then NEWLINE statementlist elseiflist T_ELSE
            statementlist T_ENDIF NEWLINE
          {
            TREE * tempLastNode;

            $3->right = $5;
            $3->next = $6;

            $$ = make_node(csound, T_IF, $2, $3);

            tempLastNode = $$;

            while (tempLastNode->right!=NULL && tempLastNode->right->next!=NULL) {
                tempLastNode = tempLastNode->right->next;
            }

            tempLastNode->right->next = make_node(csound, T_ELSE, NULL, $8);
            //print_tree(csound, "IF TREE", $$);
          }
          ;

elseiflist : elseiflist elseif
            {
                TREE * tempLastNode = $1;

                while (tempLastNode->right!=NULL && tempLastNode->right->next!=NULL) {
                    tempLastNode = tempLastNode->right->next;
                }

                tempLastNode->right->next = $2;
                $$ = $1;
            }
            | elseif { $$ = $1; }
           ;

elseif    : T_ELSEIF expr then NEWLINE statementlist
            {
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "ELSEIF FOUND!\n");
                $3->right = $5;
                $$ = make_node(csound, T_ELSEIF, $2, $3);
                //print_tree(csound, "ELSEIF", $$);
            }
          ;

then      : T_THEN
            { $$ = make_leaf(csound, T_THEN, (ORCTOKEN *)$1); }
          | T_KTHEN
            { $$ = make_leaf(csound, T_KTHEN, (ORCTOKEN *)$1); }
          | T_ITHEN
            { $$ = make_leaf(csound, T_ITHEN, (ORCTOKEN *)$1); }
          ;

goto  : T_GOTO
            { $$ = make_leaf(csound, T_GOTO, (ORCTOKEN *)$1); }
          | T_KGOTO
            { $$ = make_leaf(csound, T_KGOTO, (ORCTOKEN *)$1); }
          | T_IGOTO
            { $$ = make_leaf(csound, T_IGOTO, (ORCTOKEN *)$1); }
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
                                      make_leaf(csound, T_LABEL, (ORCTOKEN *)$3));
                }
          | exprlist ',' error
          | expr { $$ = $1;     }
          | T_IDENT { $$ = make_leaf(csound, T_LABEL, (ORCTOKEN *)$1);     }
          | /* null */          { $$ = NULL; }
          ;

expr      : expr '?' expr ':' expr %prec '?'
            { $$ = make_node(csound, '?', $1,
                             make_node(csound, ':', $3, $5)); }
          | expr '?' expr ':' error
          | expr '?' expr error
          | expr '?' error
          | expr S_LE expr      { $$ = make_node(csound, S_LE, $1, $3); }
          | expr S_LE error
          | expr S_GE expr      { $$ = make_node(csound, S_GE, $1, $3); }
          | expr S_GE error
          | expr S_NEQ expr     { $$ = make_node(csound, S_NEQ, $1, $3); }
          | expr S_NEQ error
          | expr S_EQ expr      { $$ = make_node(csound, S_EQ, $1, $3); }
          | expr S_EQ error
          | expr S_ASSIGN expr  { $$ = make_node(csound, S_EQ, $1, $3); }
          | expr S_ASSIGN error
          | expr S_GT expr      { $$ = make_node(csound, S_GT, $1, $3); }
          | expr S_GT error
          | expr S_LT expr      { $$ = make_node(csound, S_LT, $1, $3); }
          | expr S_LT error
          | expr S_AND expr     { $$ = make_node(csound, S_AND, $1, $3); }
          | expr S_AND error
          | expr S_OR expr      { $$ = make_node(csound, S_OR, $1, $3); }
          | expr S_OR error
          | '!' expr %prec S_UNOT { $$ = make_node(csound, S_UNOT, $2, NULL); }
          | '!' error
          | iexp                { $$ = $1; }
          ;

iexp      : iexp '+' iterm   { $$ = make_node(csound, '+', $1, $3); }
          | iexp '+' error
          | iexp '-' iterm  { $$ = make_node(csound, '-', $1, $3); }
          | iexp '-' error
          | iterm               { $$ = $1; }
          ;

iterm     : iterm '*' ifac  { $$ = make_node(csound, '*', $1, $3); }
          | iterm '*' error
          | iterm '/' ifac    { $$ = make_node(csound, '/', $1, $3); }
          | iterm '/' error
          | iterm '%' ifac    { $$ = make_node(csound, '%', $1, $3); }
          | iterm '%' error
          | ifac                { $$ = $1; }
          ;

ifac      : ident               { $$ = $1; }
          | constant            { $$ = $1; }
          | T_IDENT_T '[' iexp ']'
          {
              $$ = make_node(csound, S_TABREF,
                             make_leaf(csound, T_IDENT_T, (ORCTOKEN*)$1), $3);
          }
          | '-' ifac %prec S_UMINUS
            {
                $$ = make_node(csound, S_UMINUS, NULL, $2);
            }
          | '+' ifac %prec S_UMINUS
            {
                $$ = $2;
            }
          | ifac '^' ifac   { $$ = make_node(csound, '^', $1, $3); }
          | ifac S_BITOR ifac   { $$ = make_node(csound, S_BITOR, $1, $3); }
          | ifac S_BITAND ifac   { $$ = make_node(csound, S_BITAND, $1, $3); }
          | ifac S_NEQV ifac   { $$ = make_node(csound, S_NEQV, $1, $3); }
          | ifac S_BITSHL ifac   { $$ = make_node(csound, S_BITSHL, $1, $3); }
          | ifac S_BITSHR ifac   { $$ = make_node(csound, S_BITSHR, $1, $3); }
          | S_BITNOT ifac %prec S_UMINUS
            { $$ = make_node(csound, S_BITNOT, NULL, $2);}
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
    printf("FUNCTION ans=%p, token=%p %p\n", $1, ((ORCTOKEN *)$1)->value);
#ifdef PARCS
    //                if ((ORCTOKEN *)$1->value != 0)
    csp_orc_sa_interlocksf(csound, ((ORCTOKEN *)$1)->value);
#endif
    $$ = make_leaf(csound, T_FUNCTION, (ORCTOKEN *)$1); 
                }

rident    : T_SRATE     { $$ = make_leaf(csound, T_SRATE, (ORCTOKEN *)$1); }
          | T_KRATE     { $$ = make_leaf(csound, T_KRATE, (ORCTOKEN *)$1); }
          | T_KSMPS     { $$ = make_leaf(csound, T_KSMPS, (ORCTOKEN *)$1); }
          | T_NCHNLS    { $$ = make_leaf(csound, T_NCHNLS, (ORCTOKEN *)$1); }
          | T_NCHNLSI   { $$ = make_leaf(csound, T_NCHNLSI, (ORCTOKEN *)$1); }
          | T_0DBFS     { $$ = make_leaf(csound, T_0DBFS, (ORCTOKEN *)$1); }
          ;

ident     : T_IDENT_I   { $$ = make_leaf(csound, T_IDENT_I, (ORCTOKEN *)$1); }
          | T_IDENT_K   { $$ = make_leaf(csound, T_IDENT_K, (ORCTOKEN *)$1); }
          | T_IDENT_F   { $$ = make_leaf(csound, T_IDENT_F, (ORCTOKEN *)$1); }
          | T_IDENT_W   { $$ = make_leaf(csound, T_IDENT_W, (ORCTOKEN *)$1); }
          | T_IDENT_S   { $$ = make_leaf(csound, T_IDENT_S, (ORCTOKEN *)$1); }
          | T_IDENT_T   { $$ = make_leaf(csound, T_IDENT_T, (ORCTOKEN *)$1); }
          | T_IDENT_A   { $$ = make_leaf(csound, T_IDENT_A, (ORCTOKEN *)$1); }
          | T_IDENT_P   { $$ = make_leaf(csound, T_IDENT_P, (ORCTOKEN *)$1); }
          | gident      { $$ = $1; }
          ;

gident    : T_IDENT_GI  { $$ = make_leaf(csound, T_IDENT_GI, (ORCTOKEN *)$1); }
          | T_IDENT_GK  { $$ = make_leaf(csound, T_IDENT_GK, (ORCTOKEN *)$1); }
          | T_IDENT_GF  { $$ = make_leaf(csound, T_IDENT_GF, (ORCTOKEN *)$1); }
          | T_IDENT_GW  { $$ = make_leaf(csound, T_IDENT_GW, (ORCTOKEN *)$1); }
          | T_IDENT_GS  { $$ = make_leaf(csound, T_IDENT_GS, (ORCTOKEN *)$1); }
          | T_IDENT_GT  { $$ = make_leaf(csound, T_IDENT_GS, (ORCTOKEN *)$1); }
          | T_IDENT_GA  { $$ = make_leaf(csound, T_IDENT_GA, (ORCTOKEN *)$1); }
          ;

constant  : T_INTGR     { $$ = make_leaf(csound, T_INTGR, (ORCTOKEN *)$1); }
          | T_NUMBER    { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)$1); }
          | T_STRCONST  { $$ = make_leaf(csound, T_STRCONST, (ORCTOKEN *)$1); }
          | T_SRATE     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)$1); }
          | T_KRATE     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)$1); }
          | T_KSMPS     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)$1); }
          | T_NCHNLS    { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)$1); }
          | T_NCHNLSI   { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)$1); }
          | T_0DBFS     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)$1); }
          ;

opcode0   : T_OPCODE0
            {
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "opcode0 $1=%p (%s)\n",
                                  $1,((ORCTOKEN *)$1)->lexeme );
                $$ = make_leaf(csound, T_OPCODE0, (ORCTOKEN *)$1);
            }
          ;

opcode    : T_OPCODE    { $$ = make_leaf(csound, T_OPCODE, (ORCTOKEN *)$1); }
          ;

%%

#ifdef SOME_FINE_DAY
void
yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if(yylloc.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ", yylloc.first_line, yylloc.first_column,
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

