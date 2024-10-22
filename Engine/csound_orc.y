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
%define api.pure
//define api.pure full
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

%token DECLARE_TOKEN
%token UDO_TOKEN
%token UDOSTART_DEFINITION
%token UDOEND_TOKEN
%token UDO_ANS_TOKEN
%token UDO_ARGS_TOKEN
%token UDO_IDENT
%token VOID_TOKEN

%token ERROR_TOKEN

%token T_OPCALL
%token T_FUNCTION
%token T_ASSIGNMENT

%token STRUCT_TOKEN
%token INSTR_TOKEN
%token ENDIN_TOKEN
%token GOTO_TOKEN
%token KGOTO_TOKEN
%token IGOTO_TOKEN

%token STRING_TOKEN
%token T_IDENT
%token T_TYPED_IDENT
%token T_PLUS_IDENT

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
%token FOR_TOKEN
%token IN_TOKEN

%token S_ELIPSIS
%token T_ARRAY
%token T_ARRAY_IDENT
%token T_DECLARE
%token STRUCT_EXPR
%token T_MAPI
%token T_MAPK

%start orcfile

/* Precedence Rules */
%right '?'
%left S_AND S_OR
%left '|'
%left '&'
%left S_LT S_GT S_LE S_GE S_EQ S_NEQ
%left S_BITSHIFT_LEFT S_BITSHIFT_RIGHT
%left '+' '-'
%left '*' '/' '%'
%left '^'
%left '#'
%right S_UNOT
%right S_UMINUS
%right S_UPLUS
%token S_GOTO
%token T_HIGHEST

 //%error-verbose
%define parse.error verbose
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

#define namedInstrFlag csound->parserNamedInstrFlag

    extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
    extern int csound_orclex(TREE**, CSOUND *, void *);
    extern void print_tree(CSOUND *, char *msg, TREE *);
    extern TREE* constant_fold(CSOUND *, TREE *);
    extern void csound_orcerror(PARSE_PARM *, void *, CSOUND *,
                                TREE**, const char*);
    extern ORCTOKEN *lookup_token(CSOUND*,char*,void*);
#define LINE csound_orcget_lineno(scanner)
#define LOCN csound_orcget_locn(scanner)
    extern uint64_t csound_orcget_locn(void *);
    extern int csound_orcget_lineno(void *);
    extern ORCTOKEN *make_string(CSOUND *, char *);
    extern char* UNARY_PLUS;
    extern TREE* make_opcall_from_func_start(CSOUND*, int32_t, uint64_t, int32_t, TREE*, TREE*);
    extern void add_instr_variable(CSOUND *csound,  TREE *x);
%}
%%

/* TODO

 - add csp_orc_sa_instr_add calls in later csp analyze phase
   => these seem to be done now below, in the instr_definition rules
 - add csp_orc_sa_global_read_write_add_list etc
   => not sure where exactly?
*/

orcfile : root_statement_list
          {
              if ($1 != NULL)
                *astTree = ((TREE *)$1);
              csound->synterrcnt = csound_orcnerrs;
              if (csound->oparms->odebug)
                print_tree(csound, "ALL", $1);
          }
          ;


root_statement_list : root_statement_list root_statement
                      { $$ = appendToTree(csound, $1, $2); }
                    | root_statement
                    ;

root_statement : statement
               | instr_definition
               | udo_definition
               | struct_definition
               | declare_definition
               ;

/* Data declarations */

struct_definition : STRUCT_TOKEN identifier struct_arg_list
                  { $$ = make_node(csound,LINE,LOCN, STRUCT_TOKEN, $2, $3); }
                  ;

struct_arg_list : struct_arg_list ',' struct_arg
                { $$ = appendToTree(csound, $1, $3); }
                | struct_arg
                ;

struct_arg : identifier
        | typed_identifier
        | array_identifier;

instr_definition : INSTR_TOKEN instr_id_list NEWLINE
                    { csound_orcput_ilocn(scanner, LINE, LOCN); }
                  statement_list ENDIN_TOKEN NEWLINE
                  {  $$ = make_node(csound, (int32_t) csound_orcget_iline(scanner),
                                  csound_orcget_ilocn(scanner), INSTR_TOKEN,
                                  $2, $5);
                    csp_orc_sa_instr_finalize(csound);
                 }
                | INSTR_TOKEN NEWLINE error
                   { csound->ErrorMsg(csound, Str("No number following instr\n"));
                     csp_orc_sa_instr_finalize(csound);
                   }
                ;


instr_id_list : instr_id_list ',' instr_id
                  { $$ = appendToTree(csound, $1, $3); }
              | instr_id  { csp_orc_sa_instr_add_tree(csound, $1);
                    add_instr_variable(csound, $1);
                }
              ;

instr_id : integer
          | identifier
          | plus_identifier
          ;


udo_definition   : UDOSTART_DEFINITION identifier ',' UDO_IDENT ',' UDO_IDENT NEWLINE
                   statement_list UDOEND_TOKEN NEWLINE
              {
                TREE *udoTop = make_leaf(csound, LINE,LOCN, UDO_TOKEN,
                                         (ORCTOKEN *)NULL);
                TREE *ident = $2;
                TREE *udoAns = make_leaf(csound, LINE,LOCN, UDO_ANS_TOKEN,
                                         (ORCTOKEN *)$4);
                TREE *udoArgs = make_leaf(csound, LINE,LOCN, UDO_ARGS_TOKEN,
                                          (ORCTOKEN *)$6);
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "UDO COMPLETE\n");

                udoTop->left = ident;
                ident->left = udoAns;
                ident->right = udoArgs;

                udoTop->right = (TREE *)$8;

                $$ = udoTop;

                if (UNLIKELY(PARSER_DEBUG))
                  print_tree(csound, "UDO\n", (TREE *)$$);

              }
              | UDOSTART_DEFINITION identifier udo_arg_list ':' udo_out_arg_list NEWLINE
                statement_list UDOEND_TOKEN NEWLINE
              {
                TREE *udoTop = make_leaf(csound, LINE, LOCN, UDO_TOKEN,
                                        (ORCTOKEN*)NULL);
                $$ = udoTop;
                udoTop->left = $2;
                $2->left = $5;
                $2->right = $3;
                $$->right = $7;
              }
            ;

udo_arg_list : '(' out_arg_list ')'
             { $$ = $2;  }
             | '(' ')'
             { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, "0")); }
             ;

udo_out_arg_list : '(' out_type_list ')'
             { $$ = $2; }
             | '(' ')'
             { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, "0")); }
             | VOID_TOKEN
             { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, "0")); }
             | out_type
             ;

out_type_list : out_type_list ',' out_type
              { $$ = appendToTree(csound, $1, $3); }
             | out_type
             ;

out_type : identifier
        | array_identifier
        ;

/* Opcode and Function calls */

/* opcall is an ambiguous rule.  We use it to catch no out-arg function calls, as well as old-style opcode line calls. While ambiguous, it *should* only match valid code. The ambiguity is resolved by the semantic analyzer.  */
opcall  : identifier NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            $$->left = $1;
          }
        | out_arg_list expr_list NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            $$->left = $1;
            $$->right = $2;
          }
        | out_arg_list '(' ')' NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            $$->left = $1;
            /*$$->right = $2; */
          }
        | out_arg_list identifier expr_list NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            $$->left = $2;
            $2->type = T_OPCALL;
            $2->left = $1;
            $2->right = $3;
          }

        | function_call NEWLINE
        | function_call '+' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '+', $1, $3); }
        | function_call '-' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '-', $1, $3); }
        | function_call '*' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '*', $1, $3); }
        | function_call '/' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '/', $1, $3); }
        | function_call '^' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '^', $1, $3); }
        | function_call '%' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '%', $1, $3); }
        | function_call '|' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '|', $1, $3); }
        | function_call '&' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '&', $1, $3); }
        | function_call '#' expr_list NEWLINE
          { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '#', $1, $3); }
        ;

function_call : typed_identifier '(' expr_list ')'
              { $$ = $1;
                $1->type = T_FUNCTION;
                $1->right = $3; }
             | typed_identifier '(' ')'
              { $$ = $1;
                $1->type = T_FUNCTION; }
             | identifier '(' expr_list ')'
              { $$ = $1;
                $1->type = T_FUNCTION;
                $1->right = $3; }
             | identifier '(' ')'
              { $$ = $1;
                $1->type = T_FUNCTION; }
             ;

statement_list : statement_list statement
                {
                    $$ = appendToTree(csound, (TREE *)$1, (TREE *)$2);
                }
                | statement
                | {
                    /* This rule allows for empty statement lists, but
                    in turn causes a lot of shift/reduce errors to be
                    reported.  The parser works with this, but we should
                    perhaps look at expanding the other rules to work
                    without statement_list in them. */
                    $$ = NULL;
                  }
                ;

statement : out_arg_list assignment expr_list NEWLINE
                {
                  $$ = (TREE *)$2;
                  $$->left = (TREE *)$1;

                  if($2->right != NULL) {
                    TREE* op = $2->right;
                    $2->right = NULL;
                    op->right = (TREE *)$3;
                    op->left = copy_node(csound, $1);
                    $$->right = op;
                  } else {
                    $$->right = (TREE *)$3;
                  }
                }
          | opcall
          | goto identifier NEWLINE
                {
                    $1->left = NULL;
                    $1->right = $2;
                    $$ = $1;
                }

          | if_goto
          | if_then
          | until
          | while
          | for_in
          | LABEL_TOKEN
            { $$ = make_leaf(csound, LINE, LOCN, LABEL_TOKEN, (ORCTOKEN *)$1); }
          | NEWLINE
            { $$ = NULL; }

          ;


if_goto : IF_TOKEN expr goto T_IDENT NEWLINE
              {
                  $3->left = NULL;
                  $3->right = make_leaf(csound, LINE,LOCN,
                                        T_IDENT, (ORCTOKEN *)$4);
                  $$ = make_node(csound,LINE,LOCN, IF_TOKEN, $2, $3);
              }
        ;

if_then : if_then_base ENDIF_TOKEN NEWLINE
          { $$ = $1; }
        | if_then_base ELSE_TOKEN statement_list ENDIF_TOKEN NEWLINE
          { $$ = $1;
            $$->right->next = make_node(csound,LINE,LOCN, ELSE_TOKEN, NULL, $3); }
        | if_then_base elseif_list ENDIF_TOKEN NEWLINE
          { $$ = $1;
            $$->right->next = $2; }
        | if_then_base elseif_list ELSE_TOKEN statement_list ENDIF_TOKEN NEWLINE
          { TREE * tempLastNode;
            $$ = $1;
            $$->right->next = $2;

            tempLastNode = $$;

            while (tempLastNode->right!=NULL && tempLastNode->right->next!=NULL) {
              tempLastNode = tempLastNode->right->next;
            }
            tempLastNode->right->next = make_node(csound, LINE,LOCN, ELSE_TOKEN, NULL, $4);
            }
        ;

if_then_base : IF_TOKEN expr then NEWLINE statement_list
              { $3->right = $5;
                $$ = make_node(csound,LINE,LOCN, IF_TOKEN, $2, $3); }
              ;

elseif_list : elseif_list elseif
              { TREE * tempLastNode = $1;
                while (tempLastNode->right!=NULL &&
                  tempLastNode->right->next!=NULL) {
                  tempLastNode = tempLastNode->right->next;
                }
                tempLastNode->right->next = $2;
                $$ = $1; }
            | elseif { $$ = $1; }
            ;

elseif : ELSEIF_TOKEN expr then NEWLINE statement_list
            { $3->right = $5;
              $$ = make_node(csound,LINE,LOCN, ELSEIF_TOKEN, $2, $3); }
       ;

until : UNTIL_TOKEN expr DO_TOKEN statement_list OD_TOKEN
              { $$ = make_leaf(csound,LINE,LOCN, UNTIL_TOKEN, (ORCTOKEN *)$1);
                $$->left = $2;
                $$->right = $4; }
      ;

while : WHILE_TOKEN expr DO_TOKEN statement_list OD_TOKEN
              { $$ = make_leaf(csound,LINE,LOCN, WHILE_TOKEN, (ORCTOKEN *)$1);
                $$->left = $2;
                $$->right = $4; }
      ;

for_in : FOR_TOKEN identifier in expr DO_TOKEN statement_list OD_TOKEN
        {
          $3->left = $4;
          $3->right = $6;
          $$ = make_node(csound,LINE,LOCN, FOR_TOKEN, $2, $3);
        }
        | FOR_TOKEN identifier ',' identifier in expr DO_TOKEN statement_list OD_TOKEN
        {
          $2->next = $4;
          $5->left = $6;
          $5->right = $8;
          $$ = make_node(csound,LINE,LOCN, FOR_TOKEN, $2, $5);
        }
        ;

declare_definition : DECLARE_TOKEN identifier udo_arg_list ':' udo_out_arg_list NEWLINE
 {
   $$ = make_leaf(csound, LINE, LOCN, T_DECLARE, make_token(csound, $2->value->lexeme));
   $$->left = $2;
   $$->left->left = $5;
   $$->left->right = $3;
 }

/* Expressions */
expr_list : expr_list ',' expr
              { $$ = appendToTree(csound, $1, $3); }
         | expr_list ',' NEWLINE expr
              { $$ = appendToTree(csound, $1, $4); }
         | expr
         ;

expr    : function_call
        | '(' expr ')'
          { $$ = $2 ; }
        | '(' expr error    { $$ = NULL;  }
        | '(' error         { $$ = NULL; }
        | ternary_expr
        | unary_expr
        | binary_expr
        | identifier
        | integer
        | number
        | string
        | array_expr
        | static_array
        | struct_expr
        ;

static_array : '[' expr_list ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "fillarray"));
            $$->right = $2;
          }

/* TODO: Investigate whether this should allow for expressions as base before brackets to make more generic
*/
array_expr :  array_expr '[' expr ']'
          {
            appendToTree(csound, $1->right, $3);
            $$ = $1;
          }
          | identifier '[' expr ']'
          {
           char* arrayName = $1->value->lexeme;
            $$ = make_node(csound, LINE, LOCN, T_ARRAY,
              	   make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, arrayName)), $3);
          }
          | function_call '[' expr ']'
          {
            $$ = make_node(csound, LINE, LOCN, T_ARRAY, $1, $3);
          }
          ;

struct_expr : struct_expr '.' identifier
            {  $$ = $1;
               appendToTree(csound, $1->right, $3); }
            | identifier '.' identifier
            {  $$ = make_node(csound, LINE, LOCN, STRUCT_EXPR, $1, $3); }
            ;

ternary_expr : expr '?' expr ':' expr %prec '?'
            { $$ = make_node(csound,LINE,LOCN, '?', $1,
                             make_node(csound, LINE,LOCN, ':', $3, $5)); }
          | expr '?' expr ':' error
          | expr '?' expr error
          | expr '?' error
          ;

unary_expr : '~' expr %prec S_UMINUS
            { $$ = make_node(csound, LINE,LOCN, '~', NULL, $2);}
        | '~' error         { $$ = NULL; }
        | '!' expr %prec S_UNOT { $$ = make_node(csound, LINE,LOCN,
                                                    S_UNOT, $2, NULL); }
        | '!' error           { $$ = NULL; }
        | '-' expr %prec S_UMINUS
          {
              $$ = make_node(csound,LINE,LOCN, S_UMINUS, NULL, $2);
          }
        | '-' error           { $$ = NULL; }
        | '+' expr %prec S_UPLUS
          /* { */
          /*     $$ = $2; */
          /*     /\* added to left for disambiguation of opcall in semantic analyzer *\/ */
          /*     /\*$2->next = make_leaf(csound, LINE, LOCN, '+', (ORCTOKEN *) $1); *\/ */
          /*     $2->markup = &UNARY_PLUS; */
          /* } */

          /* VL 14 Sep 21: the only way I could find to make this rule correctly
              parse a statement such as
                 out a1 + (a1 * 2)
             was to abandon the code above and invent and implement a S_UPLUS type
             (here and in the semantics, expression and optimize code)
             otherwise the rule did not create an expression at all and was
             parsed as if the variable following 'out' was an OPCALL
             This comment can be removed once the rule has been reviewed and
             accepted.
          */
          {
              $$ = make_node(csound,LINE,LOCN, S_UPLUS, NULL, $2);
          }

        | '+' error           { $$ = NULL; }
        ;

binary_expr : expr '+' optnewline expr   { $$ = make_node(csound, LINE,LOCN, '+', $1, $4); }
          | expr '+' error
          | expr '-' optnewline expr  { $$ = make_node(csound ,LINE,LOCN, '-', $1, $4); }
          | expr '-' error
          | expr S_LE optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_LE, $1, $4); }
          | expr S_LE error
          | expr S_GE optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_GE, $1, $4); }
          | expr S_GE error     { $$ = NULL; }
          | expr S_NEQ optnewline expr     { $$ = make_node(csound, LINE,LOCN, S_NEQ, $1, $4); }
          | expr S_NEQ error    { $$ = NULL; }
           /* VL: 18.09.21 added the rule for if x = y for backwards compatibility */
          | expr '=' expr_list  { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $3); }
          | expr '=' error
          | expr S_EQ optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $4); }
          | expr S_EQ error
          | expr S_GT optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_GT, $1, $4); }
          | expr S_GT error
          | expr S_LT optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_LT, $1, $4); }
          | expr S_LT error
          | expr S_AND optnewline expr   { $$ = make_node(csound, LINE,LOCN, S_AND, $1, $4); }
          | expr S_AND error
          | expr S_OR optnewline expr    { $$ = make_node(csound, LINE,LOCN, S_OR, $1, $4); }
          | expr S_OR error
          | expr '*' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '*', $1, $4); }
          | expr '*' error
          | expr '/' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '/', $1, $4); }
          | expr '/' error
          | expr '^' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '^', $1, $4); }
          | expr '^' error
          | expr '%' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '%', $1, $4); }
          | expr '%' error
          | expr '|' optnewline expr        { $$ = make_node(csound, LINE,LOCN, '|', $1, $4); }
          | expr '|' error
          | expr '&' optnewline expr        { $$ = make_node(csound, LINE,LOCN, '&', $1, $4); }
          | expr '&' error
          | expr '#' optnewline expr        { $$ = make_node(csound, LINE,LOCN, '#', $1, $4); }
          | expr '#' error
          | expr S_BITSHIFT_LEFT optnewline expr
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_LEFT, $1, $4); }
          | expr S_BITSHIFT_LEFT error
          | expr S_BITSHIFT_RIGHT optnewline expr
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_RIGHT, $1, $4); }
          | expr S_BITSHIFT_RIGHT error
          ;


out_arg_list : out_arg_list ',' out_arg
              { $$ = appendToTree(csound, $1, $3); }
             | out_arg
             ;

out_arg : identifier
        | typed_identifier
        | array_identifier
        | array_expr
        | struct_expr
        ;

array_identifier: array_identifier '[' ']' {
            appendToTree(csound, $1->right,
	             make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[")));
            $$ = $1;
          }
          | identifier '[' ']' {
            $$ = $1;
            $1->type = T_ARRAY_IDENT;
	          $$->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "["));
          }
          | typed_identifier '[' ']' {
            $$ = $1;
            $1->type = T_ARRAY_IDENT;
	          $$->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "["));
          }
          ;

/* ORCTOKEN wrappings and simplifications */

assignment : '='
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "=")); }
              | S_ADDIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "="));
                  $$->right = make_leaf(csound, LINE, LOCN, '+', make_token(csound, "+"));
                }
              | S_SUBIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "="));
                  $$->right = make_leaf(csound, LINE, LOCN, '-', make_token(csound, "-"));
                }
              | S_DIVIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "="));
                  $$->right = make_leaf(csound, LINE, LOCN, '/', make_token(csound, "/"));
                }
              | S_MULIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "="));
                  $$->right = make_leaf(csound, LINE, LOCN, '*', make_token(csound, "*"));
                }
              ;

in        : IN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, IN_TOKEN, (ORCTOKEN *)$1); }

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

optnewline: NEWLINE
          | /* empty */
          ;

string : STRING_TOKEN
        { $$ = make_leaf(csound, LINE,LOCN, STRING_TOKEN, (ORCTOKEN *)$1); }
        ;


number : NUMBER_TOKEN
       { $$ = make_leaf(csound, LINE,LOCN, NUMBER_TOKEN, (ORCTOKEN *)$1); }
       ;

integer : INTEGER_TOKEN
        { $$ = make_leaf(csound, LINE, LOCN, INTEGER_TOKEN, (ORCTOKEN *)$1); }
        ;

/* VL 8.2.22 this rule is only used in instr definition
   it can't use a lexer token because that is ambiguous with expressions
   so we have to construct it as the concatentation of two tokens
*/
plus_identifier : '+' T_IDENT
        {
	  $$ = make_leaf(csound, LINE, LOCN, T_PLUS_IDENT, (ORCTOKEN *)$2);
	}
        ;

typed_identifier : T_TYPED_IDENT
        { $$ = make_leaf(csound, LINE, LOCN, T_TYPED_IDENT, (ORCTOKEN *)$1); }
        ;

identifier : T_IDENT
        { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, (ORCTOKEN *)$1); }
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
