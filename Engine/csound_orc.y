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
%token S_COM
%token S_Q
%token S_COL
%token S_NOT
%token S_PLUS
%token S_MINUS
%token S_TIMES
%token S_DIV
%token S_MOD
%token S_POW
%token S_NL
%token S_LB
%token S_RB
%token S_NEQ
%token S_AND
%token S_OR
%token S_LT
%token S_LE
%token S_EQ
%token S_ASSIGN
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

%token T_INTLIST

%start orcfile
%left S_AND S_OR
%nonassoc T_THEN T_ITHEN T_KTHEN T_ELSE /* NOT SURE IF THIS IS NECESSARY */
%left S_PLUS S_MINUS
%left S_STAR S_SLASH S_MOD
%left S_POW
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
%left S_Q
%nonassoc S_LT S_GT S_LEQ S_GEQ S_EQ S_NEQ
%token S_GOTO
%token T_HIGHEST
%pure_parser
%error-verbose
%parse-param { CSOUND * csound }
%parse-param { TREE * astTree }
%lex-param { CSOUND * csound }

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

    //int udoflag = -1; /* THIS NEEDS TO BE MADE NON-GLOBAL */
#define udoflag csound->parserUdoflag

   //int namedInstrFlag = 0; /* THIS NEEDS TO BE MADE NON-GLOBAL */
#define namedInstrFlag csound->parserNamedInstrFlag

extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
extern int csound_orclex(TREE*, CSOUND *);
extern void print_tree(CSOUND *, char *msg, TREE *);
extern void csound_orcerror(CSOUND *, TREE*, char*);
extern void add_udo_definition(CSOUND*, char *, char *, char *);
%}
%%

orcfile           : rootstatement
                        {
                            *astTree = *((TREE *)$1);
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

/* FIXME: Does not allow "instr 2,3,4,5,6" syntax */
intlist   :  T_INTGR S_COM intlist 
                { $$ = make_node(csound, T_INTLIST,
                                 make_leaf(csound, T_INTGR, (ORCTOKEN *)$1), $3); }
          | T_INTGR { $$ = make_leaf(csound, T_INTGR, (ORCTOKEN *)$1); }
          ;

instrdecl : T_INSTR
                { namedInstrFlag = 1; }
            intlist S_NL
                { namedInstrFlag = 0; }
            statementlist T_ENDIN S_NL
                {
                    $$ = make_node(csound, T_INSTR, $3, $6);
                }

          | T_INSTR
                { namedInstrFlag = 1; }
            T_IDENT S_NL
                { namedInstrFlag = 0; }
            statementlist T_ENDIN S_NL
                {
                    TREE *ident = make_leaf(csound, T_IDENT, (ORCTOKEN *)$3);
                    $$ = make_node(csound, T_INSTR, ident, $6);
                }

          | T_INSTR S_NL error
                {
                    namedInstrFlag = 0;
                    csound->Message(csound, Str("No number following instr\n"));
                }
          ;

udodecl   : T_UDOSTART
                                                { udoflag = -2; }
                  T_IDENT
                                                { udoflag = -1; }
                  S_COM
                                                { udoflag = 0;}
              T_UDO_ANS
                        { udoflag = 1; }
              S_COM T_UDO_ARGS S_NL
              {
                udoflag = 2;
                add_udo_definition(csound,
                        ((ORCTOKEN *)$3)->lexeme,
                        ((ORCTOKEN *)$7)->lexeme,
                        ((ORCTOKEN *)$10)->lexeme);
              }
              statementlist T_UDOEND S_NL
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

                if (UNLIKELY(PARSER_DEBUG)) print_tree(csound, "UDO\n", (TREE *)$$);

              }

            ;


statementlist : statementlist statement
                {
                    $$ = appendToTree(csound, (TREE *)$1, (TREE *)$2);
                }
                | /* null */          { $$ = NULL; }
                ;

topstatement : rident S_ASSIGN expr S_NL
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

statement : ident S_ASSIGN expr S_NL
                {

                    TREE *ans = make_leaf(csound, S_ASSIGN, (ORCTOKEN *)$2);
                    ans->left = (TREE *)$1;
                    ans->right = (TREE *)$3;
                    /* ans->value->lexeme = get_assignment_type(csound,
                       ans->left->value->lexeme, ans->right->value->lexeme); */

                    $$ = ans;
                }
          | ans opcode exprlist S_NL
                {

                    $2->left = $1;
                    $2->right = $3;

                    $$ = $2;
                }
          | opcode0 exprlist S_NL
                {
                    ((TREE *)$1)->left = NULL;
                    ((TREE *)$1)->right = (TREE *)$2;

                    $$ = $1;
                }
          | T_LABEL
                {
                    $$ = make_leaf(csound, T_LABEL, (ORCTOKEN *)yylval);
                }
          | goto label S_NL
                {
                    $1->left = NULL;
                    $1->right = make_leaf(csound, T_LABEL, (ORCTOKEN *)$2);
                    $$ = $1;
                }
          | T_IF expr goto label S_NL
                {
                    $3->left = NULL;
                    $3->right = make_leaf(csound, T_LABEL, (ORCTOKEN *)$4);
                    $$ = make_node(csound, T_IF, $2, $3);
                }
          | ifthen
          | S_NL { $$ = NULL; }
          ;

ans       : ident               { $$ = $1; }
          | ans S_COM ident     { $$ = appendToTree(csound, $1, $3); }
          ;

ifthen    : T_IF expr then S_NL statementlist T_ENDIF S_NL
          {
            $3->right = $5;
            $$ = make_node(csound, T_IF, $2, $3);
            //print_tree(csound, "if-endif\N", $$);
          }
          | T_IF expr then S_NL statementlist T_ELSE statementlist T_ENDIF S_NL
          {
            $3->right = $5;
            $3->next = make_node(csound, T_ELSE, NULL, $7);
            $$ = make_node(csound, T_IF, $2, $3);
            //print_tree(csound, "if-else"$$);

          }
          | T_IF expr then S_NL statementlist elseiflist T_ENDIF S_NL
          {
              if (UNLIKELY(PARSER_DEBUG))
                csound->Message(csound, "IF-ELSEIF FOUND!\n");
            $3->right = $5;
            $3->next = $6;
            $$ = make_node(csound, T_IF, $2, $3);
            //print_tree(csound, "if-elseif\n", $$);
          }
          | T_IF expr then S_NL statementlist elseiflist T_ELSE statementlist T_ENDIF S_NL
          {
              /*if (PARSER_DEBUG)*/ csound->Message(csound, "IF-ELSEIF-ELSE FOUND!\n");
            TREE * tempLastNode;

            $3->right = $5;
            $3->next = $6;

            $$ = make_node(csound, T_IF, $2, $3);

            tempLastNode = $$;

            while(tempLastNode->right != NULL && tempLastNode->right->next != NULL) {
                tempLastNode = tempLastNode->right->next;
            }

            tempLastNode->right->next = make_node(csound, T_ELSE, NULL, $8);
          }
          ;

elseiflist : elseiflist elseif
            {
                TREE * tempLastNode = $1;

                while(tempLastNode->right != NULL && tempLastNode->right->next != NULL) {
                    tempLastNode = tempLastNode->right->next;
                }

                tempLastNode->right->next = $2;
                $$ = $1;
            }
            | elseif { $$ = $1; }
           ;

elseif    : T_ELSEIF expr then S_NL statementlist
            {
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "ELSEIF FOUND!\n");
                $3->right = $5;
                $$ = make_node(csound, T_ELSEIF, $2, $3);
            }
          ;

then      : T_THEN
            { $$ = make_leaf(csound, T_THEN, (ORCTOKEN *)yylval); }
          | T_KTHEN
            { $$ = make_leaf(csound, T_KTHEN, (ORCTOKEN *)yylval); }
          | T_ITHEN
            { $$ = make_leaf(csound, T_ITHEN, (ORCTOKEN *)yylval); }
          ;


goto  : T_GOTO
            { $$ = make_leaf(csound, T_GOTO, (ORCTOKEN *)yylval); }
          | T_KGOTO
            { $$ = make_leaf(csound, T_KGOTO, (ORCTOKEN *)yylval); }
          | T_IGOTO
            { $$ = make_leaf(csound, T_IGOTO, (ORCTOKEN *)yylval); }
          ;

/* Allow all words as a label */
label : T_IDENT     { $$ = (TREE *)$1; }
      | T_OPCODE    { $$ = (TREE *)$1; }
      | T_OPCODE0   { $$ = (TREE *)$1; }
      ;


exprlist  : exprlist S_COM expr
                {
                    /* $$ = make_node(S_COM, $1, $3); */
                    $$ = appendToTree(csound, $1, $3);
                }
          | exprlist S_COM label
                {
                    /* $$ = make_node(S_COM, $1, $3); */
                    $$ = appendToTree(csound, $1,
                                      make_leaf(csound, T_LABEL, (ORCTOKEN *)$3));
                }
          | exprlist S_COM error
          | expr { $$ = $1;     }
          | T_IDENT { $$ = make_leaf(csound, T_LABEL, (ORCTOKEN *)yylval);     }
          | /* null */          { $$ = NULL; }
          ;

expr      : expr S_Q expr S_COL expr %prec S_Q
            { $$ = make_node(csound, S_Q, $1,
                             make_node(csound, S_COL, $3, $5)); }
          | expr S_Q expr S_COL error
          | expr S_Q expr error
          | expr S_Q error
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
          | S_NOT expr %prec S_UNOT { $$ = make_node(csound, S_UNOT, $2, NULL); }
          | S_NOT error
          | iexp                { $$ = $1; }
          ;

iexp      : iexp S_PLUS iterm   { $$ = make_node(csound, S_PLUS, $1, $3); }
          | iexp S_PLUS error
          | iexp S_MINUS iterm  { $$ = make_node(csound, S_MINUS, $1, $3); }
          | iexp S_MINUS error
          | iterm               { $$ = $1; }
          ;

iterm     : iterm S_TIMES ifac  { $$ = make_node(csound, S_TIMES, $1, $3); }
          | iterm S_TIMES error
          | iterm S_DIV ifac    { $$ = make_node(csound, S_DIV, $1, $3); }
          | iterm S_DIV error
          | iterm S_MOD ifac    { $$ = make_node(csound, S_MOD, $1, $3); }
          | iterm S_MOD error
          | ifac                { $$ = $1; }
          ;

ifac      : ident               { $$ = $1; }
          | constant            { $$ = $1; }
          | S_MINUS ifac %prec S_UMINUS
            {
                $$ = make_node(csound, S_UMINUS, NULL, $2);
            }
          | S_PLUS ifac %prec S_UMINUS
            {
                $$ = $2;
            }
          | ifac S_POW ifac   { $$ = make_node(csound, S_POW, $1, $3); }
          | ifac S_BITOR ifac   { $$ = make_node(csound, S_BITOR, $1, $3); }
          | ifac S_BITAND ifac   { $$ = make_node(csound, S_BITAND, $1, $3); }
          | ifac S_NEQV ifac   { $$ = make_node(csound, S_NEQV, $1, $3); }
          | ifac S_BITSHL ifac   { $$ = make_node(csound, S_BITSHL, $1, $3); }
          | ifac S_BITSHR ifac   { $$ = make_node(csound, S_BITSHR, $1, $3); }
          | S_BITNOT ifac %prec S_UMINUS
            { $$ = make_node(csound, S_BITNOT, NULL, $2);}
          | S_MINUS error
          | S_LB expr S_RB      { $$ = $2; }
          | S_LB expr error
          | S_LB error
          | function S_LB exprlist S_RB
            {
                $1->left = NULL;
                $1->right = $3;

                $$ = $1;
            }
          | function S_LB error
          ;

function  : T_FUNCTION  { $$ = make_leaf(csound, T_FUNCTION, (ORCTOKEN *)$1); }
          ;

/* exprstrlist    : exprstrlist S_COM expr
                                        { $$ = make_node(csound, S_COM, $1, $3); }
          | exprstrlist S_COM T_STRCONST
                 { $$ = make_node(csound, S_COM, $1,
                   make_leaf(csound, T_STRCONST, (ORCTOKEN *)yylval)); }
          | exprstrlist S_COM error
          | expr                { $$ = $1; }
          ;
 */

rident    : T_SRATE     { $$ = make_leaf(csound, T_SRATE, (ORCTOKEN *)yylval); }
          | T_KRATE     { $$ = make_leaf(csound, T_KRATE, (ORCTOKEN *)yylval); }
          | T_KSMPS     { $$ = make_leaf(csound, T_KSMPS, (ORCTOKEN *)yylval); }
          | T_NCHNLS    { $$ = make_leaf(csound, T_NCHNLS, (ORCTOKEN *)yylval); }
          | T_NCHNLSI   { $$ = make_leaf(csound, T_NCHNLSI, (ORCTOKEN *)yylval); }
          | T_0DBFS     { $$ = make_leaf(csound, T_0DBFS, (ORCTOKEN *)yylval); }
          ;

ident     : T_IDENT_I   { $$ = make_leaf(csound, T_IDENT_I, (ORCTOKEN *)yylval); }
          | T_IDENT_K   { $$ = make_leaf(csound, T_IDENT_K, (ORCTOKEN *)yylval); }
          | T_IDENT_F   { $$ = make_leaf(csound, T_IDENT_F, (ORCTOKEN *)yylval); }
          | T_IDENT_W   { $$ = make_leaf(csound, T_IDENT_W, (ORCTOKEN *)yylval); }
          | T_IDENT_S   { $$ = make_leaf(csound, T_IDENT_S, (ORCTOKEN *)yylval); }
          | T_IDENT_A   { $$ = make_leaf(csound, T_IDENT_A, (ORCTOKEN *)yylval); }
          | T_IDENT_P   { $$ = make_leaf(csound, T_IDENT_P, (ORCTOKEN *)yylval); }
          | gident      { $$ = $1; }
          ;

gident    : T_IDENT_GI  { $$ = make_leaf(csound, T_IDENT_GI, (ORCTOKEN *)yylval); }
          | T_IDENT_GK  { $$ = make_leaf(csound, T_IDENT_GK, (ORCTOKEN *)yylval); }
          | T_IDENT_GF  { $$ = make_leaf(csound, T_IDENT_GF, (ORCTOKEN *)yylval); }
          | T_IDENT_GW  { $$ = make_leaf(csound, T_IDENT_GW, (ORCTOKEN *)yylval); }
          | T_IDENT_GS  { $$ = make_leaf(csound, T_IDENT_GS, (ORCTOKEN *)yylval); }
          | T_IDENT_GA  { $$ = make_leaf(csound, T_IDENT_GA, (ORCTOKEN *)yylval); }
          ;

constant  : T_INTGR     { $$ = make_leaf(csound, T_INTGR, (ORCTOKEN *)yylval); }
          | T_NUMBER    { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)yylval); }
          | T_STRCONST  { $$ = make_leaf(csound, T_STRCONST, (ORCTOKEN *)yylval); }
          | T_SRATE     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)yylval); }
          | T_KRATE     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)yylval); }
          | T_KSMPS     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)yylval); }
          | T_NCHNLS    { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)yylval); }
          | T_NCHNLSI   { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)yylval); }
          | T_0DBFS     { $$ = make_leaf(csound, T_NUMBER, (ORCTOKEN *)yylval); }
          ;

opcode0   : T_OPCODE0
            {
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "opcode0 yylval=%p (%s)\n",
                                  yylval,((ORCTOKEN *)yylval)->lexeme );
                $$ = make_leaf(csound, T_OPCODE0, (ORCTOKEN *)yylval);
            }
          ;

opcode    : T_OPCODE    { $$ = make_leaf(csound, T_OPCODE, (ORCTOKEN *)yylval); }
          ;

%%
