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

%token UDO_TOKEN
%token UDOSTART_DEFINITION
%token UDOEND_TOKEN
%token UDO_ANS_TOKEN
%token UDO_ARGS_TOKEN

%token T_ERROR

%token T_OPCALL
%token T_FUNCTION
%token T_ASSIGNMENT

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
%token DO_TOKEN
%token OD_TOKEN

%token S_ELIPSIS
%token T_ARRAY
%token T_ARRAY_IDENT
%token T_MAPI
%token T_MAPK

%start orcfile
%left '?'
%left S_AND S_OR
%nonassoc THEN_TOKEN ITHEN_TOKEN KTHEN_TOKEN ELSE_TOKEN /* NOT SURE IF THIS IS NECESSARY */
%left '|'
%left '&'
%left S_LT S_GT S_LE S_GE S_EQ S_NEQ
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
#include <string.h>
#include "namedins.h"

#include "csound_orc.h"
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#include "parse_param.h"

#define udoflag csound->parserUdoflag

#define namedInstrFlag csound->parserNamedInstrFlag

    extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
    extern int csound_orclex(TREE**, CSOUND *, void *);
    extern void print_tree(CSOUND *, char *msg, TREE *);
    extern void csound_orcerror(PARSE_PARM *, void *, CSOUND *, TREE*, const char*);
    extern void add_udo_definition(CSOUND*, char *, char *, char *);
    extern ORCTOKEN *lookup_token(CSOUND*,char*,void*);
#define LINE csound_orcget_lineno(scanner)
#define LOCN csound_orcget_locn(scanner)
    extern int csound_orcget_locn(void *);
    extern int csound_orcget_lineno(void *);
    extern ORCTOKEN *make_string(CSOUND *, char *);
%}
%%

/* TODO
  
 - add csp_orc_sa_instr_add calls in later csp analyze phase

*/

orcfile : root_statement_list
          {
              if ($1 != NULL)
                *astTree = *((TREE *)$1);
              csound->synterrcnt = csound_orcnerrs;
              //print_tree(csound, "ALL", $1);
          }
          ;

root_statement_list : root_statement_list root_statement
                      { $$ = appendToTree(csound, $1, $2); }
                    | root_statement
                    ;

root_statement : statement
               | instr_definition
               | udo_definition
               ;

/* Data declarations */

instr_definition : INSTR_TOKEN instr_id_list NEWLINE 
                    { csound_orcput_ilocn(scanner, LINE, LOCN); }
                  statement_list ENDIN_TOKEN NEWLINE
                 {  $$ = make_node(csound, csound_orcget_iline(scanner),
                                  csound_orcget_ilocn(scanner), INSTR_TOKEN,
                                  $2, $5); }
                | INSTR_TOKEN NEWLINE error
                   { csound->Message(csound, Str("No number following instr\n")); }
                ;
    

instr_id_list : instr_id_list ',' instr_id
                  { $$ = appendToTree(csound, $1, $3); }
              | instr_id
              ;

instr_id : integer 
          | identifier
          | plus_identifier
          ;
         

udo_definition   : UDOSTART_DEFINITION
                                                { udoflag = -2; }
                 identifier 
                                                { udoflag = -1; }
                  ','
                                                { udoflag = 0;}
              UDO_ANS_TOKEN
                        { udoflag = 1; }
              ',' UDO_ARGS_TOKEN NEWLINE
              {
                udoflag = 2;
                /*add_udo_definition(csound,*/
                /*        ((ORCTOKEN *)$3)->lexeme,*/
                /*        ((ORCTOKEN *)$7)->lexeme,*/
                /*        ((ORCTOKEN *)$10)->lexeme);*/
              }
              statement_list UDOEND_TOKEN NEWLINE
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


/* Opcode and Function calls */



/* opcall is a slightly ambiguous rule.  We use it to catch no out-arg function calls, as well as old-style opcode line calls. While slightly ambiguous, it does only match valid code. The ambiguity is resolved by the semantic analyzer.  */

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
                ;

statement : out_arg_list assignment expr NEWLINE
                {
                  $$ = $2;
                  $$->left = (TREE *)$1;
                  $$->right = (TREE *)$3;
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
          | LABEL_TOKEN NEWLINE
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
            $$->right->next = make_node(csound,LINE,LOCN, ELSE_TOKEN, NULL, $3); }
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

/* Expressions */

expr_list : expr_list ',' expr
              { $$ = appendToTree(csound, $1, $3); } 
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
        ;


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
        | '+' expr %prec S_UMINUS
          {
              $$ = $2;
          }
        | '+' error           { $$ = NULL; }
        ;

binary_expr : expr '+' expr   { $$ = make_node(csound, LINE,LOCN, '+', $1, $3); }
          | expr '+' error
          | expr '-' expr  { $$ = make_node(csound ,LINE,LOCN, '-', $1, $3); }
          | expr '-' error
          | expr S_LE expr      { $$ = make_node(csound, LINE,LOCN, S_LE, $1, $3); }
          | expr S_LE error
          | expr S_GE expr      { $$ = make_node(csound, LINE,LOCN, S_GE, $1, $3); }
          | expr S_GE error
          | expr S_NEQ expr     { $$ = make_node(csound, LINE,LOCN, S_NEQ, $1, $3); }
          | expr S_NEQ error
          | expr S_EQ expr      { $$ = make_node(csound, LINE,LOCN, S_EQ, $1, $3); }
          | expr S_EQ error
          | expr S_GT expr      { $$ = make_node(csound, LINE,LOCN, S_GT, $1, $3); }
          | expr S_GT error
          | expr S_LT expr      { $$ = make_node(csound, LINE,LOCN, S_LT, $1, $3); }
          | expr S_LT error
          | expr S_AND expr   { $$ = make_node(csound, LINE,LOCN, S_AND, $1, $3); }
          | expr S_AND error
          | expr S_OR expr    { $$ = make_node(csound, LINE,LOCN, S_OR, $1, $3); }
          | expr S_OR error
          | expr '*' expr    { $$ = make_node(csound, LINE,LOCN, '*', $1, $3); }
          | expr '*' error
          | expr '/' expr    { $$ = make_node(csound, LINE,LOCN, '/', $1, $3); }
          | expr '/' error
          | expr '^' expr    { $$ = make_node(csound, LINE,LOCN, '^', $1, $3); }
          | expr '^' error
          | expr '%' expr    { $$ = make_node(csound, LINE,LOCN, '%', $1, $3); }
          | expr '%' error
          | expr '|' expr        { $$ = make_node(csound, LINE,LOCN, '|', $1, $3); }
          | expr '|' error
          | expr '&' expr        { $$ = make_node(csound, LINE,LOCN, '&', $1, $3); }
          | expr '&' error
          | expr '#' expr        { $$ = make_node(csound, LINE,LOCN, '#', $1, $3); }
          | expr '#' error
          | expr S_BITSHIFT_LEFT expr   
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_LEFT, $1, $3); }
          | expr S_BITSHIFT_LEFT error
          | expr S_BITSHIFT_RIGHT expr
                 { $$ = make_node(csound, LINE,LOCN, S_BITSHIFT_RIGHT, $1, $3); }
          | expr S_BITSHIFT_RIGHT error
          ;


out_arg_list : out_arg_list ',' out_arg
              { $$ = appendToTree(csound, $1, $2); }
             | out_arg
             ;

out_arg : identifier 
        | typed_identifier 
        | array_identifier
        | array_expr
        ;


array_identifier: array_identifier '[' ']' {          
            appendToTree(csound, $1->right, 
	             make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[")));
            $$ = $1;
          }
          | identifier '[' ']' {
            $$ = make_leaf(csound, LINE, LOCN, T_ARRAY_IDENT, $1); 
	          $$->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "["));
          }
          ;



/* ORCTOKEN wrappings and simplifications */

assignment : '='
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, (ORCTOKEN *)$1); }
              | S_ADDIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, (ORCTOKEN *)$1); }
              | S_SUBIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, (ORCTOKEN *)$1); }
              | S_DIVIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, (ORCTOKEN *)$1); }
              | S_MULIN
                { $$ = make_leaf(csound,LINE,LOCN, T_ASSIGNMENT, (ORCTOKEN *)$1); }
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

string : STRING_TOKEN 
        { $$ = make_leaf(csound, LINE,LOCN, STRING_TOKEN, (ORCTOKEN *)$1); }
        ;
 

number : NUMBER_TOKEN
       { $$ = make_leaf(csound, LINE,LOCN, NUMBER_TOKEN, (ORCTOKEN *)$1); }
       ;

integer : INTEGER_TOKEN
        { $$ = make_leaf(csound, LINE, LOCN, INTEGER_TOKEN, (ORCTOKEN *)$1); }
        ;

plus_identifier : T_PLUS_IDENT
        { $$ = make_leaf(csound, LINE, LOCN, T_PLUS_IDENT, (ORCTOKEN *)$1); }
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

