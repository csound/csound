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
#define YY_DECL int yylex (YYLTYPE *lvalp, CSOUND *csound, yyscan_t yyscanner)
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
LABEL           ^[ \t]*[a-zA-Z0-9_][a-zA-Z0-9_]*:
IDENT           [a-zA-Z_][a-zA-Z0-9_]*
IDENTB          [a-zA-Z_][a-zA-Z0-9_]*\([ \t]*("\n")?
XIDENT          0|[aijkftKOVPopS\[\]]+
INTGR           [0-9]+
NUMBER          [0-9]+\.?[0-9]*(e[-+]?[0-9]+)?|\.[0-9]+(e[-+]?[0-9]+)?|0[xX][0-9a-fA-F]+
WHITE           [ \t]+
OPTWHITE        [ \t]*
CONT            \\[ \t]*(;.*)?\n
XSTR            "{{"
EXSTR           "}}"
LINE            ^[ \t]*"#line"
FILE            ^[ \t]*"#source"
FNAME           [a-zA-Z0-9/:.+-_]+
%x line
%x src
%x xstr
%x udodef
%x udoarg

%%
"\r"            { } /* EATUP THIS PART OF WINDOWS NEWLINE */

{CONT}          { csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                       yyscanner);
                }
"\n"            { csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                       yyscanner);
                  return NEWLINE; }
"("{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '('; 
                }
")"             { return ')'; }
"["{OPTWHITE}"\n"? { if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '[';
                }
"]"             { return ']'; }
"+"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '+';
                   }
"-"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '-';
                   }
"*"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '*';
                   }
"/"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '/';
                   }
"%"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '%';
                   }
"\^"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '^';
                   }
"?"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '?';
                   }
":"             { return ':'; }
","{OPTWHITE}"\n"? {
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return ','; 
                   }
"!"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '!'; }
"->"               { return S_ELIPSIS; }
"!="{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_NEQ; }
"&&"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_AND; }
"||"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_OR; }
"<<"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_BITSHIFT_LEFT; }
">>"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_BITSHIFT_RIGHT; }
"<"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_LT; }
"<="{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_LE; }
"=="{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_EQ; }
"+="{OPTWHITE}"\n"? {  
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_ADDIN; }
"-="{OPTWHITE}"\n"? {  
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_SUBIN; }
"*="{OPTWHITE}"\n"? {  
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_MULIN; }
"/="{OPTWHITE}"\n"? {  
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_DIVIN; }
"="{OPTWHITE}"\n"? {   
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     *lvalp = make_token(csound, "=");
                     (*lvalp)->type = '=';
                     return '='; }
">"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_GT; }
">="{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return S_GE; }
"|"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '|'; }
"&"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '&'; }
"#"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '#'; }
"¬"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '~'; }
"~"{OPTWHITE}"\n"? { 
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return '~'; }
"@@"{OPTWHITE}{INTGR}     { *lvalp = do_at(csound, 1, yyg); return INTEGER_TOKEN; }
"@"{OPTWHITE}{INTGR}      { *lvalp = do_at(csound, 0, yyg); return INTEGER_TOKEN; }
"@i"            { return T_MAPI; }
"@k"            { return T_MAPK; }
"if"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = IF_TOKEN;
                  return IF_TOKEN; }
"if("{OPTWHITE}"\n"? { 
                  if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                  *lvalp = make_token(csound, yytext);
                  unput('(');
                  (*lvalp)->type = IF_TOKEN;
                  return IF_TOKEN; }
"then"          { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = THEN_TOKEN;
                  return THEN_TOKEN; }
"ithen"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ITHEN_TOKEN;
                  return ITHEN_TOKEN; }
"kthen"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = KTHEN_TOKEN;
                  return KTHEN_TOKEN; }
"elseif"        { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ELSEIF_TOKEN;
                  return ELSEIF_TOKEN; }
"elseif("{OPTWHITE}"\n"? { 
                  if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                  unput('(');
                  *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ELSEIF_TOKEN;
                  return ELSEIF_TOKEN; }
"else"          { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ELSE_TOKEN;
                  return ELSE_TOKEN; }
"endif"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ENDIF_TOKEN;
                  return ENDIF_TOKEN; }
"fi"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ENDIF_TOKEN;
                  return ENDIF_TOKEN; }
"until"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = UNTIL_TOKEN;
                  return UNTIL_TOKEN; }
"while"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = WHILE_TOKEN;
                  return WHILE_TOKEN; }
"do"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = DO_TOKEN;
                  return DO_TOKEN; }
"od"            { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = OD_TOKEN;
                  return OD_TOKEN; }
"enduntil"      { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = OD_TOKEN;
                  return OD_TOKEN; }

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
"endin"         { *lvalp = make_token(csound, yytext);
                  (*lvalp)->type = ENDIN_TOKEN;
                  return ENDIN_TOKEN; }
"opcode"        { BEGIN(udodef);
                  return UDOSTART_DEFINITION;
                }
"endop"         {
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
                  /* printf("xstrbuff:>>%s<<\n", PARM->xstrbuff); */
                  *lvalp = make_string(csound, PARM->xstrbuff);
                  free(PARM->xstrbuff);
                  return STRING_TOKEN;
                }

<xstr>"\n"     { /* The next two should be one case but I cannot get that to work */
                  if (PARM->xstrptr+2==PARM->xstrmax) {
                      PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                      csound->DebugMsg(csound,"Extending xstr buffer\n");
                  }
                  //csound->DebugMsg(csound,"Adding newline (%.2x)\n", yytext[0]);
                  PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                }

<xstr>"\r"     { }
<xstr>.         { if (PARM->xstrptr+2==PARM->xstrmax) {
                      PARM->xstrbuff = (char *)realloc(PARM->xstrbuff,
                                                       PARM->xstrmax+=80);
                      csound->DebugMsg(csound,"Extending xstr buffer\n");
                  }
                  //csound->DebugMsg(csound,"Adding (%.2x)\n", yytext[0]);
                  PARM->xstrbuff[PARM->xstrptr++] = yytext[0];
                  PARM->xstrbuff[PARM->xstrptr] = '\0';
                }

<udodef>{WHITE} { } 
<udodef>{IDENT} { BEGIN(udoarg);
                  *lvalp = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, (*lvalp)->type); */
                  return (*lvalp)->type; }
<udoarg>","{OPTWHITE}"\n"? {
                     if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                     return ','; 
                   }
<udoarg>{XIDENT} { BEGIN(udoarg);
                  *lvalp = lookup_token(csound, yytext, yyscanner);
                  /* csound->Message(csound,"%s -> %d\n",
                                     yytext, (*lvalp)->type); */
                  return (*lvalp)->type; }
<udoarg>"\n"     { BEGIN(INITIAL);
                   csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                        yyscanner);
                  return NEWLINE; }


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
{IDENTB}        { if (strchr(yytext, '\n'))
                       csound_orcset_lineno(1+csound_orcget_lineno(yyscanner),
                                            yyscanner);
                  *strrchr(yytext, '(') = '\0';
                  *lvalp = lookup_token(csound, yytext, yyscanner);
                  return (*lvalp)->type+1; }
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
<src>{FNAME}    { PARM->locn = atoll(yytext); }
<src>"\n"       { BEGIN(INITIAL); }


<<EOF>>         {
                  yyterminate();
                }

%%

static inline int isNameChar(int c, int pos)
{
    c = (int) ((unsigned char) c);
    return (isalpha(c) || (pos && (c == '_' || isdigit(c))));
}

ORCTOKEN *new_token(CSOUND *csound, int type)
{
    ORCTOKEN *ans = (ORCTOKEN*)csound->Calloc(csound, sizeof(ORCTOKEN));
    ans->type = type;
    return ans;
}

ORCTOKEN *make_token(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, STRING_TOKEN);
    ans->lexeme = cs_strdup(csound, s);
    return ans;
}

ORCTOKEN *make_label(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, LABEL_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)csound->Calloc(csound, len);
    strncpy(ans->lexeme, s, len - 1); /* Not the trailing colon */
    return ans;
}

ORCTOKEN *make_string(CSOUND *csound, char *s)
{
    ORCTOKEN *ans = new_token(csound, STRING_TOKEN);
    int len = strlen(s);
/* Keep the quote marks */
    /* ans->lexeme = (char*)csound->Calloc(csound, len-1); */
    /* strncpy(ans->lexeme, s+1, len-2); */
    /* ans->lexeme[len-2] = '\0';  */
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
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
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
    strncpy(ans->lexeme, buf, len);
    ans->value = i;
    return ans;
}

ORCTOKEN *make_int(CSOUND *csound, char *s)
{
    int n = atoi(s);
    ORCTOKEN *ans = new_token(csound, INTEGER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->value = n;
    return ans;
}

ORCTOKEN *make_num(CSOUND *csound, char *s)
{
    double n = atof(s);
    ORCTOKEN *ans = new_token(csound, NUMBER_TOKEN);
    int len = strlen(s);
    ans->lexeme = (char*)csound->Calloc(csound, len + 1);
    strncpy(ans->lexeme, s, len);
    ans->fvalue = n;
    return ans;
}

char *csound_orcget_current_pointer(void *yyscanner)
{
    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return yyg->yy_c_buf_p;
}

uint64_t csound_orcget_locn(void *yyscanner)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->locn;
}

void csound_orcput_ilocn(void *yyscanner, uint64_t num, uint64_t fil)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    PARM->iline = num;
    PARM->ilocn = fil;
}

uint64_t csound_orcget_iline(void *yyscanner)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->iline;
}

uint64_t csound_orcget_ilocn(void *yyscanner)
{
//    struct yyguts_t *yyg  = (struct yyguts_t*)yyscanner;
    return PARM->ilocn;
}
