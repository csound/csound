%token S_COM
%token S_Q
%token S_COL
%token S_NOT
%token S_PLUS
%token S_MINUS
%token S_TIMES
%token S_DIV
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
%token S_XOR
%token S_MOD

%token T_IF

%token T_OPCODE0
%token T_OPCODE

%token T_ABS
%token T_AMPDB
%token T_AMPDBFS
%token T_BIRND
%token T_COS
%token T_COSH
%token T_COSINV
%token T_CPS2PCH
%token T_CPSOCT
%token T_CPSPCH
%token T_DB
%token T_DBAMP
%token T_DBFSAMP
%token T_EXP
%token T_FILELEN
%token T_FILENCHNLS
%token T_FILESR
%token T_FRAC
%token T_I
%token T_INT
%token T_LOG
%token T_LOG10
%token T_OCTCPS
%token T_OCTPCH
%token T_P
%token T_RND
%token T_RND31
%token T_SIN
%token T_SINH
%token T_SININV
%token T_SQRT
%token T_TABLENG
%token T_TAN
%token T_TANH
%token T_TANINV
%token T_TANINV2

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
%token T_IDENT_P
%token T_IDENT_S
%token T_IDENT_GS
%token T_INTGR
%token T_NUMBER

%start orcfile
%left S_AND S_OR
%nonassoc S_LT S_GT S_LEQ S_GEQ S_EQ S_NEQ
%left S_PLUS S_MINUS
%left S_STAR S_SLASH
%right S_UNOT
%right S_UMINUS
%token S_GOTO
%token T_HIGHEST

%{
#define YYSTYPE TREE*
#include "jsnd.h"
%}
%%

orcfile           : header instrlist	{ }
                  ;

header            : header rtparam      { }
                  | S_NL		{ }
                  | /* null */
		  | error               { printf("Ill formed header\n"); }
                  ;

instrlist         : instrlist instrdecl { }
		  | instrdecl           { }
		  | error		{ printf("Ill formed instrument\n");}
		  ;

instrdecl	  : T_INSTR T_INTGR S_NL statementlist T_ENDIN S_NL
                        { start_instr(((TOKEN*)$2)->value);
                          statement_list = $4;
                          end_instr(); }
                  | S_NL {}
		  | T_INSTR error
                        { printf("No number following instr\n"); }
		  ;

rtparam		  : T_SRATE S_ASSIGN T_NUMBER S_NL
                        { sr = ((TOKEN*)$3)->fvalue;
                          printf("sr set to %f\n", sr); }
                  | T_SRATE S_ASSIGN T_INTGR S_NL
                        { sr = (double)((TOKEN*)$3)->value;
                          printf("sr set to %f\n", sr); }
		  | T_KRATE S_ASSIGN T_NUMBER S_NL
                        { kr = ((TOKEN*)$3)->fvalue;
                          printf("kr set to %f\n", kr); }
		  | T_KRATE S_ASSIGN T_INTGR S_NL
                        { kr = (double)((TOKEN*)$3)->value;
                          printf("kr set to %f\n", kr); }
		  | T_KSMPS S_ASSIGN T_INTGR S_NL
                        { ksmps = ((TOKEN*)$3)->value;
                          printf("ksmps set to %d\n", ksmps);}
		  | T_NCHNLS S_ASSIGN T_INTGR S_NL
                        { nchnls = ((TOKEN*)$3)->value;
                          printf("nchnlc set to %d\n", nchnls); }
                  | gans initop exprlist S_NL
                                { instr0($2, $1, 
                                         check_opcode($2, $1, $3)); }
		  |    initop0 exprlist S_NL
                                { instr0($1, NULL,
                                         check_opcode0($1, $2)); }
                  | S_NL                { }
                  ;

initop0           : T_STRSET		{ $$ = make_leaf(T_STRSET, NULL); }
                  | T_PSET		{ $$ = make_leaf(T_PSET, NULL); }
                  | T_CTRLINIT		{ $$ = make_leaf(T_CTRLINIT, NULL); }
                  | T_MASSIGN		{ $$ = make_leaf(T_MASSIGN, NULL); }
                  | T_TURNON		{ $$ = make_leaf(T_TURNON, NULL); }
                  | T_PREALLOC		{ $$ = make_leaf(T_PREALLOC, NULL); }
                  | T_ZAKINIT		{ $$ = make_leaf(T_ZAKINIT, NULL); }
                  ;
initop            : T_FTGEN		{ $$ = make_leaf(T_FTGEN, NULL); }
                  | T_INIT              { $$ = make_leaf(T_INIT, NULL); }
                  ;

gans              : gident              { $$ = $1; }
                  | gans S_COM gident   { $$ = make_node(S_COM, $1, $3); }
                  | error               { }
                  ;

gident		  : T_IDENT_GI          { $$ = make_leaf(T_IDENT_GI, yylval); }
		  | T_IDENT_GK          { $$ = make_leaf(T_IDENT_GK, yylval); }
		  | T_IDENT_GA          { $$ = make_leaf(T_IDENT_GA, yylval); }
                  | error               { }
                  ;

statementlist     : statementlist statement S_NL
                        { if ($2 == NULL) $$ = $1;
                          else            $$ = make_node(S_ANDTHEN, $1, $2); }
                  | /* null */          { $$ = NULL; }
		  | error 
                  ;

goto		  : T_GOTO              { $$ = make_leaf(T_GOTO, NULL); }
                  | T_KGOTO             { $$ = make_leaf(T_KGOTO, NULL); }
                  | T_IGOTO             { $$ = make_leaf(T_IGOTO, NULL); }
                  | error
                  ;

statement	  : lvalue S_ASSIGN expr
                                { $$ = make_node(S_ASSIGN, $1, $3); }
		  | ans opcode exprlist
                                { $$ = make_node($2->type, $1,
                                                 check_opcode($2, $1, $3)); }
		  |    opcode0 exprlist
                                { $$ = make_node($1->type, NULL,
                                                 check_opcode0($1, $2)); }
                  | /* NULL */  { $$ = NULL; }
                  | goto T_IDENT        { $$ = make_node(S_GOTO, $1,
                                                 make_leaf(T_IDENT, yylval)); }
		  | T_IF S_LB expr S_RB goto T_IDENT
                                        { $$ = make_node(T_IF, $3, 
                                                make_node(S_GOTO, $5,
                                                 make_leaf(T_IDENT, yylval))); }
		  | T_IF S_LB expr S_RB error 
		  | T_IF S_LB expr error 
		  | T_IF error 
		  ;
ident		  : T_IDENT_I           { $$ = make_leaf(T_IDENT_I, yylval); }
		  | T_IDENT_K           { $$ = make_leaf(T_IDENT_K, yylval); }
		  | T_IDENT_A           { $$ = make_leaf(T_IDENT_A, yylval); }
		  | T_IDENT_S           { $$ = make_leaf(T_IDENT_S, yylval); }
		  | T_IDENT_P           { $$ = make_leaf(T_IDENT_P, yylval); }
                  | gident              { $$ = $1; }
                  | error
                  ;

lvalue		  : ident               { $$ = $1; }
                  | error               { printf("Invalid lvalue\n"); }
		  ;

identlist	  : identlist S_COM T_IDENT     { $$ = make_node(S_COM, $1, $3); }
		  | T_IDENT             { $$ = make_leaf(T_IDENT, yylval); }
		  | /* null */          { $$ = NULL; }
		  | identlist S_COM error       { printf("Illformed list\n"); }
		  ;

ans               : ident               { $$ = $1; }
                  | ans S_COM ident     { $$ = make_node(S_COM, $1, $3); }
                  | error               { }
                  ;

function          : T_ABS		{ $$ = make_leaf(T_ABS, NULL); }
                  | T_AMPDB		{ $$ = make_leaf(T_AMPDB, NULL); }
                  | T_AMPDBFS		{ $$ = make_leaf(T_AMPDBFS, NULL); }
                  | T_BIRND		{ $$ = make_leaf(T_BIRND, NULL); }
                  | T_COS		{ $$ = make_leaf(T_COS, NULL); }
                  | T_COSH		{ $$ = make_leaf(T_COSH, NULL); }
                  | T_COSINV		{ $$ = make_leaf(T_COSINV, NULL); }
                  | T_CPS2PCH		{ $$ = make_leaf(T_CPS2PCH, NULL); }
                  | T_CPSOCT		{ $$ = make_leaf(T_CPSOCT, NULL); }
                  | T_CPSPCH		{ $$ = make_leaf(T_CPSPCH, NULL); }
                  | T_DB		{ $$ = make_leaf(T_DB, NULL); }
                  | T_DBAMP		{ $$ = make_leaf(T_DBAMP, NULL); }
                  | T_DBFSAMP		{ $$ = make_leaf(T_DBFSAMP, NULL); }
                  | T_EXP		{ $$ = make_leaf(T_EXP, NULL); }
                  | T_FILELEN		{ $$ = make_leaf(T_FILELEN, NULL); }
                  | T_FILENCHNLS	{ $$ = make_leaf(T_FILENCHNLS, NULL); }
                  | T_FILESR		{ $$ = make_leaf(T_FILESR, NULL); }
                  | T_FRAC		{ $$ = make_leaf(T_FRAC, NULL); }
                  | T_I		        { $$ = make_leaf(T_I, NULL); }
                  | T_INT		{ $$ = make_leaf(T_INT, NULL); }
                  | T_LOG		{ $$ = make_leaf(T_LOG, NULL); }
                  | T_LOG10		{ $$ = make_leaf(T_LOG10, NULL); }
                  | T_OCTCPS		{ $$ = make_leaf(T_OCTCPS, NULL); }
                  | T_OCTPCH		{ $$ = make_leaf(T_OCTPCH, NULL); }
                  | T_P	        	{ $$ = make_leaf(T_P, NULL); }
                  | T_RND		{ $$ = make_leaf(T_RND, NULL); }
                  | T_RND31		{ $$ = make_leaf(T_RND31, NULL); }
                  | T_SIN		{ $$ = make_leaf(T_SIN, NULL); }
                  | T_SINH		{ $$ = make_leaf(T_SINH, NULL); }
                  | T_SININV		{ $$ = make_leaf(T_SININV, NULL); }
                  | T_SQRT		{ $$ = make_leaf(T_SQRT, NULL); }
                  | T_TABLENG		{ $$ = make_leaf(T_TABLENG, NULL); }
                  | T_TAN		{ $$ = make_leaf(T_TAN, NULL); }
                  | T_TANH		{ $$ = make_leaf(T_TANH, NULL); }
                  | T_TANINV		{ $$ = make_leaf(T_TANINV, NULL); }
                  | T_TANINV2		{ $$ = make_leaf(T_TANINV2, NULL); }
                  | error
                  ;

expr              : expr S_Q expr S_COL expr %prec S_Q 
                                        { $$ = make_node(S_Q, $1,
                                                make_node(S_COL, $3, $5)); } 
		  | expr S_Q expr S_COL error 
		  | expr S_Q expr error 
		  | expr S_Q error 
		  | expr S_LE expr      { $$ = make_node(S_LE, $1, $3); } 
		  | expr S_LE error 
		  | expr S_GE expr      { $$ = make_node(S_GE, $1, $3); } 
		  | expr S_GE error   
		  | expr S_NEQ expr     { $$ = make_node(S_NEQ, $1, $3); } 
		  | expr S_NEQ error   
		  | expr S_EQ expr      { $$ = make_node(S_EQ, $1, $3); } 
		  | expr S_EQ error   
		  | expr S_GT expr      { $$ = make_node(S_GT, $1, $3); } 
		  | expr S_GT error   
		  | expr S_LT expr      { $$ = make_node(S_LT, $1, $3); } 
		  | expr S_LT error   
		  | expr S_AND expr     { $$ = make_node(S_AND, $1, $3); }   
		  | expr S_AND error   
		  | expr S_OR expr      { $$ = make_node(S_OR, $1, $3); } 
		  | expr S_OR error   
		  | S_NOT expr %prec S_UNOT { $$ = make_node(S_UNOT, $2, NULL); }
		  | S_NOT error 
                  | iexp                { $$ = $1; }
                  ;

iexp              : iexp S_PLUS iterm   { $$ = make_node(S_PLUS, $1, $3); }
		  | iexp S_PLUS error   
		  | iexp S_MINUS iterm  { $$ = make_node(S_MINUS, $1, $3); }
		  | expr S_MINUS error   
                  | iterm               { $$ = $1; }
                  ;
iterm             : iterm S_TIMES ifac   { $$ = make_node(S_TIMES, $1, $3); }
		  | iterm S_TIMES ifac   
		  | iterm S_DIV ifac     { $$ = make_node(S_DIV, $1, $3); }
		  | iterm S_DIV error 
                  | ifac                { $$ = $1; }
                  ;
ifac              : ident               { $$ = $1; }
                  | const               { $$ = $1; }
		  | S_MINUS ifac %prec S_UMINUS
                                        { $$ = make_node(S_UMINUS, $2, NULL); }
		  | S_MINUS error       
                  | S_LB expr S_RB      { $$ = $2; }
		  | S_LB expr error 
		  | S_LB error 
		  | function S_LB exprlist S_RB
                                        { $$ = make_node(S_APPLY, $1, $3); }
		  | function S_LB error 
                  ;
exprlist          : exprlist S_COM expr { $$ = make_node(S_COM, $1, $3); }
		  | exprlist S_COM error 
		  | expr                { $$ = $1; }
		  | /* null */          { $$ = NULL; }
		  ;
/* exprstrlist	  : exprstrlist S_COM expr
                                        { $$ = make_node(S_COM, $1, $3); }
		  | exprstrlist S_COM T_STRCONST 
                                        { $$ = make_node(S_COM, $1,
                                                make_leaf(T_STRCONST, yylval)); }
		  | exprstrlist S_COM error 
		  | expr                { $$ = $1; }
		  ;		 
 */
const   	  : T_INTGR 		{ $$ = make_leaf(T_INTGR, yylval); }
		  | T_NUMBER 		{ $$ = make_leaf(T_NUMBER, yylval); }
		  | T_STRCONST 		{ $$ = make_leaf(T_STRCONST, yylval); }
                  | T_SRATE             { $$ = make_leaf(T_NUMBER, yylval); }
                  | T_KRATE             { $$ = make_leaf(T_NUMBER, yylval); }
                  | T_KSMPS             { $$ = make_leaf(T_NUMBER, yylval); }
                  | T_NCHNLS            { $$ = make_leaf(T_NUMBER, yylval); }
		  ;

opcode0           : T_IDENT             { $$ = make_leaf(yylval->type, NULL); }
                  | error
		  ;

opcode            : T_IDENT		{ $$ = make_leaf(yylval->type, NULL); }
                  | error
                  ;
%%
