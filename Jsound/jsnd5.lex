%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define YYSTYPE TOKEN*
#include "tok.h"
#include "jsnd5.tab.h"
TOKEN *make_string(char *);
extern TOKEN *lookup_token(char *);
TOKEN *make_int(char *);
TOKEN *make_num(char *);
void comment(void);
int yyline = 0;
%}

STRCONST	\"(\\.|[^\\"])*\"
IDENT		[a-zA-Z_][a-zA-Z0-9_]*
IDENTN		[a-zA-Z0-9_]+
INTGR		[0-9]+
NUMBER	[0-9]*(\.[0-9]*)?(e[-+]?[0-9]+)?|-?\.[0-9]*(e[-+]?[0-9]+)?
COMMENT		/\*.*\*/
WHITE		[ \t]

%%

"\n"		{ yyline++; return S_NL; }
"//"	    	{ comment(); return S_NL; }
";"		{ comment(); return S_NL; }
COMMENT         { }
"("		{ return S_LB; }
")"		{ return S_RB; }
"+"		{ return S_PLUS; }
"-"		{ return S_MINUS; }
"*"		{ return S_TIMES; }
"/"		{ return S_DIV; }
"?"		{ return S_Q; }
":"		{ return S_COL; }
","		{ return S_COM; }
"!"		{ return S_NOT; }
"!="		{ return S_NEQ; }
"&&"		{ return S_AND; }
"||"		{ return S_OR; }
"<"		{ return S_LT; }
"<="		{ return S_LE; }
"=="		{ return S_EQ; }
"="		{ return S_ASSIGN; }
">"		{ return S_GT; }
">="		{ return S_GE; }

"if"		{ return T_IF; }
"then"		{ return T_THEN; }

"sr"		{ return T_SRATE; }
"kr"		{ return T_KRATE; }
"ksmps"		{ return T_KSMPS; }
"nchnls"	{ return T_NCHNLS; }
"instr"		{ return T_INSTR; }
"endin"		{ return T_ENDIN; }

{STRCONST}	{ yylval = make_string(yytext); return (T_STRCONST); }

{IDENT} 	{ yylval = lookup_token(yytext); printf("%d\n", yylval->type);
                  return (yylval->type); }

{INTGR}		{ yylval = make_int(yytext); return (T_INTGR); }
{NUMBER}	{ yylval = make_num(yytext); return (T_NUMBER); }
{WHITE}		{ }
.		{ printf("Line %d: Unknown character: '%s'\n",yyline,yytext); }

%%
void comment(void)              /* Skip until nextline */
{
    char c;

    while ((c = input()) != '\n'); /* skip */
    yyline++;
}

TOKEN *new_token(int type)
{
    TOKEN *ans = (TOKEN*)malloc(sizeof(TOKEN));
    ans->type = type;
    return ans;
}

TOKEN *make_string(char *s)
{
    TOKEN *ans = new_token(T_STRCONST);
    int len = strlen(s);
    ans->lexeme = (char*)calloc(1, len-1);
    strncpy(ans->lexeme, s+1, len-2);
    return ans;
}

TOKEN *make_int(char *s)
{
    int n = atoi(s);
    TOKEN *ans = new_token(T_INTGR);
    ans->value = n;
    return ans;
}

TOKEN *make_num(char *s)
{
    double n = atof(s);
    TOKEN *ans = new_token(T_NUMBER);
    ans->fvalue = n;
    return ans;
}

