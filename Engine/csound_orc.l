%{

 /*
    csound_orc.l:

    Copyright (C) 2006
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
#define YYSTYPE TREE*
#define YYLTYPE ORCTOKEN*
#include "tok.h"
#define YY_DECL int yylex (YYLTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
#include "csound_orcparse.h"
#include "csound_orc.h"
#include "corfile.h"
YYSTYPE *yylval_param;
YYLTYPE *yylloc_param;
ORCTOKEN *make_string(CSOUND *, char *);
extern ORCTOKEN *lookup_token(CSOUND *, char *, void *);
extern  void    *fopen_path(CSOUND *, FILE **, char *, char *, char *, int);
ORCTOKEN *new_token(CSOUND *csound, int type);
ORCTOKEN *make_int(CSOUND *, char *);
ORCTOKEN *make_num(CSOUND *, char *);
ORCTOKEN *make_token(CSOUND *, char *s);
ORCTOKEN *make_label(CSOUND *, char *s);
#define udoflag csound->parserUdoflag
#define namedInstrFlag csound->parserNamedInstrFlag
//extern int udoflag;
//extern int namedInstrFlag;
//YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
#include "parse_param.h"

#define YY_EXTRA_TYPE  PARSE_PARM *
#define PARM    yyget_extra(yyscanner)

/* #define YY_INPUT(buf,result,max_size)  {\ */
/*     result = get_next_char(buf, max_size, yyg); \ */
/*     if ( UNLIKELY( result <= 0  )) \ */
/*       result = YY_NULL; \ */
/*     } */

#define YY_USER_INIT 

struct yyguts_t;
ORCTOKEN *do_at(CSOUND *, int, struct yyguts_t*);
int get_next_char(char *, int, struct yyguts_t*);
%}
%option reentrant
%option bison-bridge
%option bison-locations
%option prefix="csound_orc"
%option outfile="Engine/csound_orclex.c"
%option stdout

STRCONST        \"(\\.|[^\"])*\"
STRCONSTe       \"(\\.|[^\"])*$
LABEL           ^[ \t]*[a-zA-Z_][a-zA-Z0-9_]*:
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
IDENTN          [a-zA-Z0-9_]+
INTGR           [0-9]+
NUMBER          [0-9]+\.[0-9]*(e[-+]?[0-9]+)?|\.[0-9]+(e[-+]?[0-9]+)?
WHITE           [ \t]+
OPTWHITE        [ \t]*
CONT            \\[ \t]*(;.*)?\n
XSTR            "{{"
EXSTR           "}}"
LINE            ^"#line"
FILE            ^"#source"
FNAME           [a-zA-Z0-9/:.+-_]+
%x line
%x src
%x xstr

%%
"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */

{CONT}          { csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                       yyscanner);
 }
"\n"            { csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                       yyscanner);
                  return NEWLINE; }
"("             { return '('; }
")"             { return ')'; }
"["             { return '['; }
"]"             { return ']'; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"%"             { return '%'; }
"\^"            { return '^'; }
"?"             { return '?'; }
":"             { return ':'; }
","             { return ','; }
"!"             { return '!'; }
"!="            { return S_NEQ; }
"&&"            { return S_AND; }
"||"            { return S_OR; }
"<<"            { return S_BITSHIFT_LEFT; }
">>"            { return S_BITSHIFT_RIGHT; }
"<"             { return S_LT; }
"<="            { return S_LE; }
"=="            { return S_EQ; }
"="             { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = '=';
                  return '='; }
">"             { return S_GT; }
">="            { return S_GE; }
"|"             { return '|'; }
"&"             { return '&'; }
"#"             { return '#'; }
"¬"             { return '~'; }
"~"             { return '~'; }
"@@"{OPTWHITE}{INTGR}     { *lvalp = do_at(csound, 1, yyg); return INTEGER_TOKEN; }
"@"{OPTWHITE}{INTGR}      { *lvalp = do_at(csound, 0, yyg); return INTEGER_TOKEN; }
"if"            { return IF_TOKEN; }
"then"          { return THEN_TOKEN; }
"ithen"         { return ITHEN_TOKEN; }
"kthen"         { return KTHEN_TOKEN; }
"elseif"        { return ELSEIF_TOKEN; }
"else"          { return ELSE_TOKEN; }
"endif"         { return ENDIF_TOKEN; }
"until"         { return UNTIL_TOKEN; }
"do"            { return DO_TOKEN; }
"od"            { return OD_TOKEN; }

"goto"          { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = GOTO_TOKEN;
                  return GOTO_TOKEN; };
"igoto"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = IGOTO_TOKEN;
                  return IGOTO_TOKEN; };
"kgoto"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = KGOTO_TOKEN;
                  return KGOTO_TOKEN; };

"sr"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = SRATE_TOKEN;
                  return SRATE_TOKEN; }
"kr"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = KRATE_TOKEN;
                  return KRATE_TOKEN; }
"ksmps"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = KSMPS_TOKEN;
                  return KSMPS_TOKEN; }
"nchnls"        { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = NCHNLS_TOKEN;
                  return NCHNLS_TOKEN; }
"nchnls_i"      { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = NCHNLSI_TOKEN;
                  return NCHNLSI_TOKEN; }
"instr"         {
                  namedInstrFlag = 1;
                  return INSTR_TOKEN;
                }
"endin"         { return ENDIN_TOKEN; }
"opcode"        {
                  PARM->clearBufferAfterEOF = 0;
                  return UDOSTART_DEFINITION;
                }
"endop"         {
                  PARM->clearBufferAfterEOF = 0;
                  *lvalp = new_token(csound, UDOEND_TOKEN); return UDOEND_TOKEN;
                }

{LABEL}         { char *pp = yytext;
                  while (*pp==' ' || *pp=='\t') pp++;
                  *lvalp = make_label(csound, pp); return LABEL_TOKEN;
                }

"\{\{"          {
                  PARM->xstrbuff = (char *)malloc(128);
                  PARM->xstrptr = 0; PARM->xstrmax = 128;
                  PARM->xstrbuff[PARM->xstrptr++] = '"';
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                  BEGIN(xstr);
                }

<xstr>"}}"   {
                  BEGIN(INITIAL);
                  PARM->xstrbuff[PARM->xstrptr++] = '"';
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                  *lvalp = make_string(csound, PARM->xstrbuff);
                  free(PARM->xstrbuff);
                  return STRING_TOKEN;
                }

<xstr>"\n"     { /* THe next two should be one case but I cannot get that to work */
                  if (PARM->xstrptr+2==PARM->xstrmax) {
                      PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                      printf("Extending xstr buffer\n");
                  }
//                  printf("Adding newline (%.2x)\n", yytext[0]);
                  PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                }

<xstr>.     { if (PARM->xstrptr+2==PARM->xstrmax) {
                      PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                      printf("Extending xstr buffer\n");
                  }
//                  printf("Adding (%.2x)\n", yytext[0]);
                  PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                }

{STRCONST}      { *lvalp = make_string(csound, yytext); return (STRING_TOKEN); }

{STRCONSTe}     { *lvalp = make_string(csound, yytext);
                  csound->Message(csound,
                          Str("unterminated string found on line %d >>%s<<\n"),
                          csound_orcget_lineno(yyscanner),
                          yytext);
                  return (STRING_TOKEN); }

"0dbfs"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ZERODBFS_TOKEN;
                  /* csound->Message(csound,"%d\n", (*lvalp)->type); */
                  return ZERODBFS_TOKEN; }
{IDENT}         { *lvalp = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, (*lvalp)->type); */
                  return (*lvalp)->type; }
{INTGR}         {
                    if (udoflag == 0) {
                        *lvalp = make_string(csound, yytext);
                        (*lvalp)->type = UDO_ANS_TOKEN;
                    } else if (udoflag == 1) {
                        *lvalp = make_string(csound, yytext);
                        (*lvalp)->type = UDO_ARGS_TOKEN;
                    } else {
                        *lvalp = make_int(csound, yytext); return (INTEGER_TOKEN);
                    }

                    csound->Message(csound,"%d\n", (*lvalp)->type);
                    return ((*lvalp)->type);
                }
{NUMBER}        { *lvalp = make_num(csound, yytext); return (NUMBER_TOKEN); }
{WHITE}         { }

{LINE}          { BEGIN(line); }

<line>[ \t]*     /* eat the whitespace */
<line>{INTGR}   { csound_orcset_lineno(atoi(yytext), yyscanner); }
<line>"\n"      {BEGIN(INITIAL);}

{FILE}          { BEGIN(src); }

<src>[ \t]*     /* eat the whitespace */
<src>{FNAME}    { strncpy(PARM->loc, yytext, 48); }
<src>"\n"       {BEGIN(INITIAL);}


<<EOF>>         {
                  yyterminate();
                  /* int n; */
                  /* printf("Leaving buffer %p\n", YY_CURRENT_BUFFER); */
                  /* yypop_buffer_state(yyscanner); */
                  /* if ( !YY_CURRENT_BUFFER ) yyterminate(); */
                  /* printf("End of input; popping to %p\n", YY_CURRENT_BUFFER); */
                  /* if (PARM->clearBufferAfterEOF) */
                  /*   PARM->clearBufferAfterEOF = */
                  /*     PARM->isInclude = 0; */
                  /* n = PARM->alt_stack[--PARM->macro_stack_ptr].n; */
                  /* printf("n=%d\n", n); */
                  /* if (n!=0) { */
                  /*   /\* We need to delete n macros starting with y *\/ */
                  /*   y = PARM->alt_stack[PARM->macro_stack_ptr].s; */
                  /*   x = PARM->macros; */
                  /*   if (x==y) { */
                  /*     while (n>0) { */
                  /*       mfree(csound, y->name); x=y->next; */
                  /*       mfree(csound, y); y=x; n--; */
                  /*     } */
                  /*     PARM->macros = x; */
                  /*   } */
                  /*   else { */
                  /*     MACRO *nxt = y->next; */
                  /*     while (x->next != y) x = x->next; */
                  /*     while (n>0) { */
                  /*       nxt = y->next; */
                  /*       mfree(csound, y->name); mfree(csound, y); y=nxt; n--; */
                  /*     } */
                  /*     x->next = nxt; */
                  /*   } */
                  /*   y->next = x; */
                  /* } */
                  /* /\* printf("End of input segment: macro pop %p -> %p\n", */
                  /*            y, PARM->macros); *\/ */
                }

%%

static inline int isNameChar(int c, int pos)
{
    c = (int) ((unsigned char) c);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

ORCTOKEN *new_token(CSOUND *csound, int type)
{
    ORCTOKEN *ans = (ORCTOKEN*)mcalloc(csound, sizeof(ORCTOKEN));
    ans->type = type;
    return ans;
}

ORCTOKEN *make_token(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, STRING_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strcpy(ans->lexeme, s);
    return ans;
}

ORCTOKEN *make_label(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, LABEL_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len);
    strncpy(ans->lexeme, s, len - 1); /* Not the trailing colon */
    return ans;
}

ORCTOKEN *make_string(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, STRING_TOKEN);
    int len = strlen(s);
/* Keep the quote marks */
    /* ans->lexeme = (char*)mcalloc(csound, len-1); */
    /* strncpy(ans->lexeme, s+1, len-2); */
    /* ans->lexeme[len-2] = '\0';  */
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strcpy(ans->lexeme, s);
    return ans;
}

ORCTOKEN *do_at(CSOUND *csound, int k, struct yyguts_t *yyg)
{
    int n, i = 1;
    ORCTOKEN *ans;
    char buf[16];
    char *s = yytext;
    int len;
    while (*s=='@') s++;
    n = atoi(s);
    while (i<=n-k && i< 0x4000000) i <<= 1;
    ans = new_token(csound, INTEGER_TOKEN);
    sprintf(buf, "%d", i+k);
    len = strlen(buf);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, buf, len);
    ans->value = i;
    return ans;
}

ORCTOKEN *make_int(CSOUND *csound, char *s)
{
    int n = atoi(s);
    ORCTOKEN *ans = new_token(csound, INTEGER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->value = n;
    return ans;
}

ORCTOKEN *make_num(CSOUND *csound, char *s)
{
    double n = atof(s);
    ORCTOKEN *ans = new_token(csound, NUMBER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)mcalloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->fvalue = n;
    return ans;
}

#if 0
/* Functions for working with grabbing input one line at a time */
/* These two functions assume reading from a file */

int get_next_line(struct yyguts_t * yyg)
{
    char *p;
    void *yyscanner = yyg;

    PARM->nBuffer = 0;

    p = fgets(yyextra->buffer, lMaxBuffer, yyin);
    printf("get_next_line: %s\n", yyextra->buffer);
    if (UNLIKELY(p == NULL)) {
      if (ferror(yyin))
        return -1;
      return 1;
    }

    csound_orcset_lineno(1+csound_orcget_lineno(yyg),yyg);

    PARM->lBuffer = strlen(yyextra->buffer);

    return 0;
}

int get_next_char(char *b, int maxBuffer, struct yyguts_t* yyg)
{
    int frc;
    void *yyscanner = yyg;
    if (UNLIKELY( feof(yyin)  ))
      return 0;
    printf("get_next_char: %s\n", yyextra->buffer);
    while (  PARM->nBuffer >= PARM->lBuffer  ) {
      frc = get_next_line(yyg);
      if (UNLIKELY(  frc != 0  ))
        return 0;
    }

    b[0] = yyextra->buffer[PARM->nBuffer];
    PARM->nBuffer += 1;

    return b[0]=='\0'?0:1;
}
#endif

char *csound_orcget_current_pointer(void *yyscanner)
{
    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return yyg->yy_c_buf_p;
}
