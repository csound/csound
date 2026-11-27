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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/
%define api.pure
%parse-param {PARSE_PARM *parm}
%parse-param {void *scanner}
%lex-param { CSOUND * csound }
%lex-param {yyscan_t *scanner}

%code requires {
#include "tok.h"
#include "parse_param.h"
}

%union {
    TREE *tree;
    ORCTOKEN *token;
}


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
%token S_BITSHIFT_RIGHT

%token <token> LABEL_TOKEN
%token IF_TOKEN

%token DECLARE_TOKEN
%token UDO_TOKEN
%token UDOSTART_DEFINITION
%token UDOEND_TOKEN
%token UDO_ANS_TOKEN
%token UDO_ARGS_TOKEN
%token <token> UDO_IDENT
%token VOID_TOKEN

%token ERROR_TOKEN

%token T_OPCALL
%token T_FUNCTION
%token T_ASSIGNMENT

%token STRUCT_TOKEN
%token INSTR_TOKEN
%token ENDIN_TOKEN
%token <token> GOTO_TOKEN
%token <token> KGOTO_TOKEN
%token <token> IGOTO_TOKEN

%token <token> STRING_TOKEN
%token <token> T_IDENT
%token <token> T_IDENTB
%token <token> T_TYPED_IDENT
%token <token> T_TYPED_IDENTB
%token <token> T_MEMBER_IDENT
%token <token> T_PLUS_IDENT

%token <token> INTEGER_TOKEN
%token <token> NUMBER_TOKEN
%token <token> THEN_TOKEN
%token <token> ITHEN_TOKEN
%token <token> KTHEN_TOKEN
%token ELSEIF_TOKEN
%token ELSE_TOKEN
%token ENDIF_TOKEN
%token <token> UNTIL_TOKEN
%token <token> WHILE_TOKEN
%token DO_TOKEN
%token OD_TOKEN
%token <token> BREAK_TOKEN
%token <token> CONTINUE_TOKEN
%token <token> SWITCH_TOKEN
%token <token> CASE_TOKEN
%token <token> DEFAULT_TOKEN
%token ENDSW_TOKEN
%token FOR_TOKEN
%token <token> IN_TOKEN
%token TRUE_TOKEN
%token FALSE_TOKEN
%token TRUEK_TOKEN
%token FALSEK_TOKEN


%token S_ELIPSIS2
%token T_ARRAY
%token <token> T_ARRAY_IDENT
%token T_DECLARE
%token STRUCT_EXPR

%type <tree> root_statement_list root_statement
%type <tree> struct_definition struct_arg_list struct_arg
%type <tree> instr_definition instr_id_list instr_id
%type <tree> udo_definition udo_arg_list udo_out_arg_list out_type_list out_type
%type <tree> opcall function_call statement_list statement
%type <tree> if_goto if_then if_then_base elseif_list elseif until while
%type <tree> case case_list switch for_var for_in declare_definition
%type <tree> expr_list expr gen_array slice_array static_array array_expr struct_expr
%type <tree> ternary_expr unary_expr binary_expr
%type <tree> out_arg_list out_arg out_arg_list_array array_identifier
%type <tree> assignment assignment_array in then goto
%type <tree> string false_const true_const number integer plus_identifier
%type <tree> typed_identifier typed_identifierb identifier identifierb

%start orcfile

/* Precedence Rules */
%right '?'
%left S_AND S_OR
%left S_LT S_GT S_LE S_GE S_EQ S_NEQ '=' // VL 6.8.25 for backwards compat
%left '|'
%left '#'
%left '&'
%left S_BITSHIFT_LEFT S_BITSHIFT_RIGHT
%left '+' '-'
%left '*' '/' '%'
%left '^'
%right S_UNOT
%right S_UMINUS
%right S_UPLUS
%token T_HIGHEST

 //%error-verbose
%define parse.error verbose
%parse-param { CSOUND * csound }
%parse-param { TREE ** astTree }


%{
#ifndef NULL
#define NULL 0L
#endif
#include "csoundCore.h"
#include <ctype.h>
#include <string.h>
#include "namedins.h"

#include "csound_orc.h"
#include "parse_param.h"

    extern TREE* parser_append(CSOUND * csound, TREE *first, TREE *newlast);
    extern int csound_orclex(YYSTYPE *, CSOUND *, void *);
    extern void print_tree(CSOUND *, char *msg, TREE *);
    extern void csound_orcerror(PARSE_PARM *, void *, CSOUND *,
                                TREE**, const char*);
    extern ORCTOKEN *lookup_token(CSOUND*,char*,void*);
#define LINE csound_orcget_lineno(scanner)
#define LOCN csound_orcget_locn(scanner)
    extern uint64_t csound_orcget_locn(void *);
    extern int csound_orcget_lineno(void *);
    extern TREE* make_opcall_from_func_start(CSOUND*, int32_t, uint64_t, int32_t, TREE*, TREE*);
%}
%%

orcfile : root_statement_list
          {
              if ($1 != NULL)
                *astTree = $1;
              else
                *astTree = NULL;
              csound->synterrcnt = csound_orcnerrs;
              if (csoundGetDebug(csound) & DEBUG_PARSER ||
		  csoundGetDebug(csound) & DEBUG_TREE)
                print_tree(csound, "ALL:\n", $1);
          }
          ;


root_statement_list : root_statement_list root_statement
                      { $$ = parser_append(csound, $1, $2); }
                    | /* empty */
                      { $$ = NULL; }
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
                { $$ = parser_append(csound, $1, $3); }
                | struct_arg_list ',' NEWLINE struct_arg
                 { $$ = parser_append(csound, $1, $4); }
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
                 }
                | INSTR_TOKEN NEWLINE error
                 { csound->ErrorMsg(csound, Str("No number following instr\n"));
                  $$ = NULL;
                 }
                ;


instr_id_list : instr_id_list ',' instr_id
                  { $$ = parser_append(csound, $1, $3); }
              | instr_id
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
                TREE *udoAns = make_leaf(csound, LINE,LOCN, UDO_ANS_TOKEN, $4);
                TREE *udoArgs = make_leaf(csound, LINE,LOCN, UDO_ARGS_TOKEN, $6);
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "UDO COMPLETE\n");

                udoTop->left = ident;
                ident->left = udoAns;
                ident->right = udoArgs;

                udoTop->right = $8;

                $$ = udoTop;

                if (UNLIKELY(PARSER_DEBUG))
                  print_tree(csound, "UDO\n", $$);

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
              | UDOSTART_DEFINITION identifier udo_arg_list ':' NEWLINE udo_out_arg_list NEWLINE
                statement_list UDOEND_TOKEN NEWLINE
              {
                TREE *udoTop = make_leaf(csound, LINE, LOCN, UDO_TOKEN,
                                        (ORCTOKEN*)NULL);
                $$ = udoTop;
                udoTop->left = $2;
                $2->left = $6;
                $2->right = $3;
                $$->right = $8;
              }
              | UDOSTART_DEFINITION identifier udo_arg_list NEWLINE ':' udo_out_arg_list NEWLINE
                statement_list UDOEND_TOKEN NEWLINE
              {
                TREE *udoTop = make_leaf(csound, LINE, LOCN, UDO_TOKEN,
                                        (ORCTOKEN*)NULL);
                $$ = udoTop;
                udoTop->left = $2;
                $2->left = $6;
                $2->right = $3;
                $$->right = $8;
              }
            ;


udo_arg_list : '(' out_arg_list ')'
             { $$ = $2;  }
             | '(' out_arg_list_array ')'
             { $$ = $2;  }
             | '(' ')'
             { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, "0", NULL)); }
             ;

udo_out_arg_list : '(' out_type_list ')'
             { $$ = $2; }
             | '(' ')'
             { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, "0", NULL)); }
             | VOID_TOKEN
             { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, "0", NULL)); }
             | out_type
             ;

out_type_list : out_type_list ',' out_type
              { $$ = parser_append(csound, $1, $3); }
             | out_type
             ;

out_type : identifier
        | array_identifier
        ;

/* Opcode and Function calls */

/* opcall is an ambiguous rule.  We use it to catch no out-arg function calls,
  as well as old-style opcode line calls. While ambiguous, it *should* only match valid code.
  The ambiguity is resolved by the semantic analyzer.
*/
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
        | out_arg_list identifier NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            if ($2->value != NULL && $2->value->lexeme != NULL &&
                strcmp($2->value->lexeme, "init") == 0) {
              $$->left = $2;
              $2->type = T_OPCALL;
              $2->left = $1;
            } else {
              $$->left = $1;
              $$->right = $2;
            }
          }
        | out_arg_list_array expr_list NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            $$->left = $1;
            $$->right = $2;
          }
        | out_arg_list_array '(' ')' NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            $$->left = $1;
            /*$$->right = $2; */
          }
        | out_arg_list_array identifier expr_list NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            $$->left = $2;
            $2->type = T_OPCALL;
            $2->left = $1;
            $2->right = $3;
          }
        | out_arg_list_array identifier NEWLINE
          { $$ = make_leaf(csound, LINE,LOCN, T_OPCALL, NULL);
            if ($2->value != NULL && $2->value->lexeme != NULL &&
                strcmp($2->value->lexeme, "init") == 0) {
              $$->left = $2;
              $2->type = T_OPCALL;
              $2->left = $1;
            } else {
              $$->left = $1;
              $$->right = $2;
            }
          }
        | function_call NEWLINE
          { $$ = $1; }
        | function_call '+' expr_list NEWLINE
        { $$ = make_opcall_from_func_start(csound, LINE, LOCN, '+', $1, $3);  }
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

function_call : typed_identifierb expr_list ')'
              { $$ = $1;
                $1->type = T_FUNCTION;
                $1->right = $2; }
             | typed_identifierb ')'
              { $$ = $1;
                $1->type = T_FUNCTION; }
             | identifierb expr_list ')'
              { $$ = $1;
                $1->type = T_FUNCTION;
                $1->right = $2; }
             | identifierb ')'
              { $$ = $1;
                $1->type = T_FUNCTION; }
             ;

statement_list : statement_list statement
                {
                    $$ = parser_append(csound, $1, $2);
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
                  $$ = $2;
                  $$->left = $1;

                  if($2->right != NULL) {
                    TREE* op = $2->right;
                    $2->right = NULL;
                    op->right = $3;
                    op->left = copy_node(csound, $1);
                    $$->right = op;
                  } else {
                    $$->right = $3;
                  }
                }
          | out_arg_list_array assignment_array expr_list NEWLINE
                {
                  $$ = $2;
                  $$->left = $1;

                  if($2->right != NULL) {
                    TREE* op = $2->right;
                    $2->right = NULL;
                    op->right = $3;
                    op->left = copy_node(csound, $1);
                    $$->right = op;
                  } else {
                    $$->right = $3;
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
          | switch
          | for_in
          | BREAK_TOKEN
            { $$ = make_leaf(csound, LINE, LOCN, BREAK_TOKEN, $1); }
          | CONTINUE_TOKEN
            { $$ = make_leaf(csound, LINE, LOCN, CONTINUE_TOKEN, $1); }
          | LABEL_TOKEN
            { $$ = make_leaf(csound, LINE, LOCN, LABEL_TOKEN, $1); }
          | NEWLINE
            { $$ = NULL; }

          ;


if_goto : IF_TOKEN expr goto T_IDENT NEWLINE
              {
                  $3->left = NULL;
                  $3->right = make_leaf(csound, LINE,LOCN, T_IDENT, $4);
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
              { $$ = make_leaf(csound,LINE,LOCN, UNTIL_TOKEN, $1);
                $$->left = $2;
                $$->right = $4; }
      ;

while : WHILE_TOKEN expr DO_TOKEN statement_list OD_TOKEN
              { $$ = make_leaf(csound,LINE,LOCN, WHILE_TOKEN, $1);
                $$->left = $2;
                $$->right = $4; }
      ;

case  : CASE_TOKEN expr_list NEWLINE statement_list
      {
        $$ = make_leaf(csound, LINE, LOCN, CASE_TOKEN, $1);
        $$->left = $2;
        $$->right = $4;
      }
      | DEFAULT_TOKEN NEWLINE statement_list
      {
        $$ = make_leaf(csound, LINE, LOCN, DEFAULT_TOKEN, $1);
        $$->right = $3;
      }
      ;

case_list : case_list case
            { TREE * tempLastNode = $1;
                while (tempLastNode->next != NULL) {
                  tempLastNode = tempLastNode->next;
                }
                tempLastNode->next = $2;
                $$ = $1; }
            | case { $$ = $1; }
            | NEWLINE case_list
             {
              $$ = $2;
             }
            ;

switch  : SWITCH_TOKEN expr NEWLINE case_list ENDSW_TOKEN
        {
          $$ = make_leaf(csound,LINE,LOCN, SWITCH_TOKEN, $1);
          $$->left = $2;
          $$->right = $4;
        }
        ;

for_var : identifier
        | typed_identifier
        ;

for_in : FOR_TOKEN for_var in expr DO_TOKEN statement_list OD_TOKEN
        {
          $3->left = $4;
          $3->right = $6;
          $$ = make_node(csound, LINE, LOCN, FOR_TOKEN, $2, $3);
        }
        | FOR_TOKEN for_var ',' for_var in expr DO_TOKEN statement_list OD_TOKEN
        {
          $2->next = $4;
          $5->left = $6;
          $5->right = $8;
          $$ = make_node(csound, LINE, LOCN, FOR_TOKEN, $2, $5);
        }
        ;

declare_definition : DECLARE_TOKEN identifier udo_arg_list ':' udo_out_arg_list NEWLINE
 {
   $$ = make_leaf(csound, LINE, LOCN, T_DECLARE, make_token(csound, $2->value->lexeme, NULL));
   $$->left = $2;
   $$->left->left = $5;
   $$->left->right = $3;
 }

/* Expressions */
expr_list : expr_list ',' expr
              { $$ = parser_append(csound, $1, $3); }
         | expr_list ',' NEWLINE expr
              { $$ = parser_append(csound, $1, $4); }
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
        | gen_array
        | static_array
        | slice_array
        | struct_expr
        | true_const
        | false_const
        ;


gen_array : '[' expr S_ELIPSIS2 expr_list  ']' {
            $$ = make_leaf(csound, LINE,LOCN, T_FUNCTION, make_token(csound, "genarray", NULL));
            $$->right = $2;
            parser_append(csound, $$->right, $4);
             }

slice_array :
          identifier '[' expr ':' expr_list  ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "slicearray", NULL));
            $$->right = $1;
            $$->right = parser_append(csound, $$->right, $3);
            parser_append(csound, $$->right, $5);
           }
           |
           identifier '[' ':' expr_list  ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "slicearray", NULL));
            $$->right = $1;
            $$->right = parser_append(csound, $$->right,
                                       make_leaf(csound,LINE,LOCN, T_IDENT,
                                                 make_int(csound, "0", NULL)));
            parser_append(csound, $$->right, $4);
           }
           |
           identifier '[' expr ':' ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "slicearray", NULL));
            $$->right = $1;
            $$->right = parser_append(csound, $$->right, $3);
           }
          |
          expr '[' expr ':' expr_list  ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "slicearray", NULL));
            $$->right = $1;
            $$->right = parser_append(csound, $$->right, $3);
            parser_append(csound, $$->right, $5);
           }
           |
           expr '[' ':' expr_list  ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "slicearray", NULL));
            $$->right = $1;
            $$->right = parser_append(csound, $$->right,
                                       make_leaf(csound,LINE,LOCN, T_IDENT,
                                                 make_int(csound, "0", NULL)));
            parser_append(csound, $$->right, $4);
           }
           |
           expr '[' expr ':' ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "slicearray", NULL));
            $$->right = $1;
            $$->right = parser_append(csound, $$->right, $3);
           }


static_array : '[' expr_list ']' {
            $$ = make_leaf(csound,LINE,LOCN, T_FUNCTION, make_token(csound, "fillarray", NULL));
            $$->right = $2;
          }


/* TODO: Investigate whether this should allow for expressions as base before brackets to make more generic
*/
array_expr :  array_expr '[' expr ']'
          {
            parser_append(csound, $1->right, $3);
            $$ = $1;
          }
          | identifier '[' expr ']'
          {
            $$ = make_node(csound, LINE, LOCN, T_ARRAY, $1, $3);
          }
          | function_call '[' expr ']'
          {
            $$ = make_node(csound, LINE, LOCN, T_ARRAY, $1, $3);
          }
          ;

struct_expr : struct_expr '.' identifier
            {
              $3->type = T_MEMBER_IDENT;
              // Important: Clear the next pointer of $3 to prevent it from being processed separately
              $3->next = NULL;
              $$ = make_node(csound, LINE, LOCN, STRUCT_EXPR, $1, $3);
            }
            | struct_expr '.' array_expr
            {
              /* Build a struct member node from array_expr's base identifier, then
                 attach the array indexing so nested chains like a.b[0] work. */
              TREE *memberLeaf;
              if ($3->value == NULL && $3->left != NULL && $3->left->type == T_IDENT) {
                memberLeaf = $3->left;
                memberLeaf->type = T_MEMBER_IDENT;
                memberLeaf->next = NULL;
              } else {
                char* memberName = $3->value == NULL ?
                                    $3->left->value->lexeme :
                                    $3->value->lexeme;
                memberLeaf = make_leaf(csound, LINE, LOCN, T_MEMBER_IDENT,
                                       make_token(csound, memberName, NULL));
              }
              /* Now make the array_expr index the struct member */
              $3->left = make_node(csound, LINE, LOCN, STRUCT_EXPR, $1, memberLeaf);
              $$ = $3;
            }
            | array_expr '.' identifier
            {
              $3->type = T_MEMBER_IDENT;
              $3->next = NULL;
              $$ = make_node(csound, LINE, LOCN, STRUCT_EXPR, $1, $3);
            }
            | identifier '.' array_expr
            {
              $1->next = NULL;
              TREE *memberLeaf;
              if ($3->value == NULL && $3->left != NULL && $3->left->type == T_IDENT) {
                memberLeaf = $3->left;
                memberLeaf->type = T_MEMBER_IDENT;
                memberLeaf->next = NULL;
              } else {
                char* memberName = $3->value == NULL ?
                                    $3->left->value->lexeme :
                                    $3->value->lexeme;
                memberLeaf = make_leaf(csound, LINE, LOCN, T_MEMBER_IDENT,
                                       make_token(csound, memberName, NULL));
              }
              $3->left = make_node(csound, LINE, LOCN, STRUCT_EXPR, $1, memberLeaf);
              $$ = $3;
            }
            | struct_expr '[' expr ']'
            {
              $$ = make_node(csound, LINE, LOCN, T_ARRAY, $1, $3);
            }
            | identifier '.' identifier
            {
              $1->next = NULL;
              $3->type = T_MEMBER_IDENT;
              // Important: Clear the next pointer of $3 to prevent it from being processed separately
              $3->next = NULL;
              $$ = make_node(csound, LINE, LOCN, STRUCT_EXPR, $1, $3);
            }
            ;

ternary_expr : expr '?' expr ':' expr %prec '?'
            { $$ = make_node(csound,LINE,LOCN, '?', $1,
                             make_node(csound, LINE,LOCN, ':', $3, $5)); }
          | expr '?' expr ':' error { $$ = NULL; }
          | expr '?' expr error { $$ = NULL; }
          | expr '?' error { $$ = NULL; }
          ;

unary_expr : '~' expr %prec S_UMINUS
            { $$ = make_node(csound, LINE,LOCN, '~', NULL, $2);}
        | '~' error         { $$ = NULL; }
        | '!' expr %prec S_UNOT { $$ = make_node(csound, LINE,LOCN,
                                                    S_UNOT, $2, NULL); }
        | '!' error           { $$ = NULL; }
        | '-' expr //%prec S_UMINUS  /* is precedence right? - removing fixes opcall parsing issue */
          {
              $$ = make_node(csound,LINE,LOCN, S_UMINUS, NULL, $2);
          }
        | '-' error           { $$ = NULL; }
        | '+' expr //%prec S_UPLUS  /* is precedence right? - removing fixes parsing issue */
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
          | expr '+' error { $$ = NULL; }
          | expr '-' optnewline expr  { $$ = make_node(csound ,LINE,LOCN, '-', $1, $4); }
          | expr '-' error { $$ = NULL; }
          | expr S_LE optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_LE, $1, $4); }
          | expr S_LE error { $$ = NULL; }
          | expr S_GE optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_GE, $1, $4); }
          | expr S_GE error     { $$ = NULL; }
          | expr S_NEQ optnewline expr     { $$ = make_node(csound, LINE,LOCN, S_NEQ, $1, $4); }
          | expr S_NEQ error    { $$ = NULL; }
           /* VL: 18.09.21 added the rule for if x = y for backwards compatibility */
          | expr '=' optnewline expr  { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $4); }
          | expr '=' error { $$ = NULL; }
          | expr S_EQ optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $4); }
          | expr S_EQ error { $$ = NULL; }
          | expr S_GT optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_GT, $1, $4); }
          | expr S_GT error { $$ = NULL; }
          | expr S_LT optnewline expr      { $$ = make_node(csound, LINE,LOCN, S_LT, $1, $4); }
          | expr S_LT error { $$ = NULL; }
          | expr S_AND optnewline expr   { $$ = make_node(csound, LINE,LOCN, S_AND, $1, $4); }
          | expr S_AND error { $$ = NULL; }
          | expr S_OR optnewline expr    { $$ = make_node(csound, LINE,LOCN, S_OR, $1, $4); }
          | expr S_OR error { $$ = NULL; }
          | expr '*' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '*', $1, $4); }
          | expr '*' error { $$ = NULL; }
          | expr '/' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '/', $1, $4); }
          | expr '/' error { $$ = NULL; }
          | expr '^' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '^', $1, $4); }
          | expr '^' error { $$ = NULL; }
          | expr '%' optnewline expr    { $$ = make_node(csound, LINE,LOCN, '%', $1, $4); }
          | expr '%' error { $$ = NULL; }
          | expr '|' optnewline expr        { $$ = make_node(csound, LINE,LOCN, '|', $1, $4); }
          | expr '|' error { $$ = NULL; }
          | expr '&' optnewline expr        { $$ = make_node(csound, LINE,LOCN, '&', $1, $4); }
          | expr '&' error { $$ = NULL; }
          | expr '#' optnewline expr        { $$ = make_node(csound, LINE,LOCN, '#', $1, $4); }
          | expr '#' error { $$ = NULL; }
          | expr S_BITSHIFT_LEFT optnewline expr
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_LEFT, $1, $4); }
          | expr S_BITSHIFT_LEFT error { $$ = NULL; }
          | expr S_BITSHIFT_RIGHT optnewline expr
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_RIGHT, $1, $4); }
          | expr S_BITSHIFT_RIGHT error { $$ = NULL; }
          ;


out_arg_list : out_arg_list ',' out_arg
              { $$ = parser_append(csound, $1, $3); }
             | out_arg
             ;

out_arg : identifier
        | typed_identifier
        | array_identifier
        | struct_expr
        ;

out_arg_list_array : out_arg_list_array ',' array_expr
              { $$ = parser_append(csound, $1, $3); }
             | array_expr
             ;

array_identifier: array_identifier '[' ']' {
            parser_append(csound, $1->right,
                           make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[", NULL)));
            $$ = $1;
          }
          | identifier '[' ']' {
            $$ = $1;
            $1->type = T_ARRAY_IDENT;
            $$->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[", NULL));
          }
          | typed_identifier '[' ']' {
            $$ = $1;
            // Check if this is a type annotation (e.g., "var:Type[]") vs array access
            // If the typed_identifier already has a type annotation ending with "[]",
            // keep it as T_TYPED_IDENT rather than converting to T_ARRAY_IDENT
            if ($1->value && $1->value->optype) {
              size_t len = strlen($1->value->optype);
              if (len >= 2 && $1->value->optype[len-2] == '[' && $1->value->optype[len-1] == ']') {
                // This is a type annotation like "Person[]", keep as T_TYPED_IDENT
                // Don't attach the '[' token or change the type
              } else {
                // This is array access syntax, convert to T_ARRAY_IDENT
                $1->type = T_ARRAY_IDENT;
                $$->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[", NULL));
              }
            } else {
              // No type annotation, treat as array access
              $1->type = T_ARRAY_IDENT;
              $$->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[", NULL));
            }
          }
          ;

/* ORCTOKEN wrappings and simplifications */
assignment : '='
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "=", NULL)); }
              | S_ADDIN
                { $$ = make_leaf(csound,LINE,LOCN, S_ADDIN, make_token(csound, "##addin", NULL)); }
              | S_SUBIN
                { $$ = make_leaf(csound,LINE,LOCN, S_SUBIN, make_token(csound, "##subin", NULL)); }
              | S_DIVIN
                { $$ = make_leaf(csound,LINE,LOCN, S_DIVIN, make_token(csound, "##divin", NULL)); }
              | S_MULIN
                { $$ = make_leaf(csound,LINE,LOCN, S_MULIN, make_token(csound, "##mulin", NULL)); }
              ;

/* special case for array expressions */
assignment_array : '='
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "=", NULL)); }
              | S_ADDIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "=", NULL));
                  $$->right = make_leaf(csound, LINE, LOCN, '+', make_token(csound, "+", NULL));
                }
              | S_SUBIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "=", NULL));
                  $$->right = make_leaf(csound, LINE, LOCN, '-', make_token(csound, "-", NULL));
                }
              | S_DIVIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "=", NULL));
                  $$->right = make_leaf(csound, LINE, LOCN, '/', make_token(csound, "/", NULL));
                }
              | S_MULIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, make_token(csound, "=", NULL));
                  $$->right = make_leaf(csound, LINE, LOCN, '*', make_token(csound, "*", NULL));
                }
              ;

in        : IN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, IN_TOKEN, $1); }

then      : THEN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, THEN_TOKEN, $1); }
          | KTHEN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, KTHEN_TOKEN, $1); }
          | ITHEN_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, ITHEN_TOKEN, $1); }
          ;

goto  : GOTO_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, GOTO_TOKEN, $1); }
          | KGOTO_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, KGOTO_TOKEN, $1); }
          | IGOTO_TOKEN
            { $$ = make_leaf(csound,LINE,LOCN, IGOTO_TOKEN, $1); }
          ;

optnewline: NEWLINE
          | /* empty */
          ;

string : STRING_TOKEN
        { $$ = make_leaf(csound, LINE,LOCN, STRING_TOKEN, $1); }
        ;

false_const: FALSE_TOKEN
       { $$ = make_leaf(csound, LINE,LOCN, FALSE_TOKEN,
                        make_token(csound,"false", NULL)); }
       | FALSEK_TOKEN
       { $$ = make_leaf(csound, LINE,LOCN, FALSEK_TOKEN,
                        make_token(csound,"falsek", NULL)); }

       ;

true_const: TRUE_TOKEN
           { $$ = make_leaf(csound, LINE,LOCN, TRUE_TOKEN,
                            make_token(csound,"true", NULL)); }
       | TRUEK_TOKEN
       { $$ = make_leaf(csound, LINE,LOCN, TRUEK_TOKEN,
                        make_token(csound,"truek", NULL)); }
       ;


number : NUMBER_TOKEN
       { $$ = make_leaf(csound, LINE,LOCN, NUMBER_TOKEN, $1); }
       ;

integer : INTEGER_TOKEN
        { $$ = make_leaf(csound, LINE, LOCN, INTEGER_TOKEN, $1); }
        ;

/* VL 8.2.22 this rule is only used in instr definition
   it can't use a lexer token because that is ambiguous with expressions
   so we have to construct it as the concatentation of two tokens
*/
plus_identifier : '+' T_IDENT
        {
	  $$ = make_leaf(csound, LINE, LOCN, T_PLUS_IDENT, $2);
	}
        ;

typed_identifier : T_TYPED_IDENT
        { $$ = make_leaf(csound, LINE, LOCN, T_TYPED_IDENT, $1); }
        ;

typed_identifierb : T_TYPED_IDENTB
        { $$ = make_leaf(csound, LINE, LOCN, T_TYPED_IDENT, $1); }
        ;

identifier : T_IDENT
        { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, $1); }
        ;

identifierb : T_IDENTB
        { $$ = make_leaf(csound, LINE, LOCN, T_IDENT, $1); }
        ;

%%
