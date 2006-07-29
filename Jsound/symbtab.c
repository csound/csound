#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tok.h"
#include "csound_orcparse.h"


TOKEN** symbtab;
extern int yyline;
extern int udoflag;

TOKEN *add_token(char *s, int type);

void init_symbtab(void)
{
    symbtab = (TOKEN**)calloc(HASH_SIZE, sizeof(TOKEN*));
    /* Now we need to populate with basic words */

	add_token("init", T_INIT);
	add_token("expseg", T_OPCODE);
	add_token("line", T_OPCODE);
	add_token("oscil", T_OPCODE);
	add_token("linseg", T_OPCODE);
	add_token("linen", T_OPCODE);
	add_token("outs", T_OPCODE0);
	add_token("randi", T_OPCODE);
	add_token("randh", T_OPCODE);
	add_token("gbuzz", T_OPCODE);
	add_token("buzz", T_OPCODE);
	add_token("expon", T_OPCODE);
	add_token("rand", T_OPCODE);
	add_token("reson", T_OPCODE);
	add_token("foscil", T_OPCODE);
	add_token("phasor", T_OPCODE);
	add_token("table", T_OPCODE);
	add_token("oscili", T_OPCODE);
	add_token("tablei", T_OPCODE);
	add_token("balance", T_OPCODE);
	add_token("delay", T_OPCODE);
	add_token("reverb", T_OPCODE);
	add_token("xout", T_OPCODE0);

	/* This adds all the T_FUNCTION tokens.  These should only be
	 * looked up in context when parsing a expression list (not yet done)
	 * and perhaps a more intelligent way needs to be done eventually to
	 * look up opcodes which can serve as functions, as well as for allowing
	 * multiple arguments to functions
	 */
	add_token("abs", T_FUNCTION);
	add_token("ampdb", T_FUNCTION);
	add_token("ampdbfs", T_FUNCTION);
	add_token("birnd", T_FUNCTION);
	add_token("cos", T_FUNCTION);
	add_token("cosh", T_FUNCTION);
	add_token("cosinv", T_FUNCTION);
	add_token("cps2pch", T_FUNCTION);
	add_token("cpsoct", T_FUNCTION);
	add_token("cpspch", T_FUNCTION);
	add_token("db", T_FUNCTION);
	add_token("dbamp", T_FUNCTION);
	add_token("dbfsamp", T_FUNCTION);
	add_token("exp", T_FUNCTION);
	add_token("filelen", T_FUNCTION);
	add_token("filenchnls", T_FUNCTION);
	add_token("filesr", T_FUNCTION);
	add_token("frac", T_FUNCTION);
	add_token("i", T_FUNCTION);
	add_token("int", T_FUNCTION);
	add_token("log", T_FUNCTION);
	add_token("log10", T_FUNCTION);
	add_token("octcps", T_FUNCTION);
	add_token("octpch", T_FUNCTION);
	add_token("p", T_FUNCTION);
	add_token("rnd", T_FUNCTION);
	add_token("rnd31", T_FUNCTION);
	add_token("sin", T_FUNCTION);
	add_token("sinh", T_FUNCTION);
	add_token("sininv", T_FUNCTION);
	add_token("sqrt", T_FUNCTION);
	add_token("tableng", T_FUNCTION);
	add_token("tan", T_FUNCTION);
	add_token("tanh", T_FUNCTION);
	add_token("taninv", T_FUNCTION);
	add_token("taninv2", T_FUNCTION);

}

unsigned int hash(char *s)
{
    unsigned int h = 0;
    while (*s != '\0') {
      h = (h<<4) ^ *s++;
    }
    return (h%HASH_SIZE);
}

TOKEN *add_token(char *s, int type)
{
    int h = hash(s);

    printf("Hash value for %s: %i\n", s, h);

    TOKEN *a = symbtab[h];
    TOKEN *ans;
    while (a!=NULL) {
      if (strcmp(a->lexeme, s)==0) {
        if (type == a->type) return a;
        printf("Type confusion for %s, stopping\n", s);
        exit(1);
      }
      a = a->next;
    }
    ans = new_token(T_IDENT);
    ans->lexeme = (char*)malloc(1+strlen(s));
    strcpy(ans->lexeme, s);
    ans->next = symbtab[h];
    ans->type = type;
    symbtab[h] = ans;
    return ans;
}

int isUDOArgList(char *s) {

	int len = strlen(s) - 1;

	while(len >= 0) {
		if(strchr("aijkKop0", s[len]) == NULL) {
			/* printf("Invalid char '%c' in '%s'", *p, s); */
			return 0;
		}

		len--;
	}

	return 1;
}

int isUDOAnsList(char *s) {
	int len = strlen(s) - 1;

	while(len >= 0) {
		if(strchr("aikK0", s[len]) == NULL) {
			return 0;
		}

		len--;
	}

	return 1;
}

TOKEN *lookup_token(char *s)
{
	int h = hash(s);
    int type = T_IDENT;
    TOKEN *a = symbtab[h];
    TOKEN *ans;

	if(udoflag == 0) {

		if(isUDOAnsList(s)) {
			ans = new_token(T_UDO_ANS);
		    ans->lexeme = (char*)malloc(1+strlen(s));
	    	strcpy(ans->lexeme, s);
	    	ans->next = symbtab[h];
	    	symbtab[h] = ans;
	    	printf("Found UDO Answer List\n");
			return ans;
		}
	}

	if(udoflag == 1) {
		if(isUDOArgList(s)) {
			ans = new_token(T_UDO_ARGS);
		    ans->lexeme = (char*)malloc(1+strlen(s));
	    	strcpy(ans->lexeme, s);
	    	ans->next = symbtab[h];
	    	symbtab[h] = ans;
	    	printf("Found UDO Arg List\n");
			return ans;
		}
	}

    while (a!=NULL) {
      if (strcmp(a->lexeme, s)==0) {
        return a;
      }
      a = a->next;
    }


    ans = new_token(T_IDENT);
    ans->lexeme = (char*)malloc(1+strlen(s));
    strcpy(ans->lexeme, s);
    ans->next = symbtab[h];
    if (s[0]=='i') type = T_IDENT_I;
    else if (s[0]=='k') type = T_IDENT_K;
    else if (s[0]=='a') type = T_IDENT_A;
    else if (s[0]=='p') type = T_IDENT_P;
    else if (s[0]=='f') type = T_IDENT_F;
    else if (s[0]=='w') type = T_IDENT_W;
    else if (s[0]=='S') type = T_IDENT_S;
    else if (s[0]=='g') {
      if (s[1]=='i') type = T_IDENT_GI;
      else if (s[1]=='k') type = T_IDENT_GK;
      else if (s[1]=='a') type = T_IDENT_GA;
      else if (s[1]=='f') type = T_IDENT_GF;
      else if (s[1]=='w') type = T_IDENT_GW;
      else if (s[1]=='S') type = T_IDENT_GS;
      else {
        printf("Unknown word type for %s on line %d\n", s, yyline);
        exit(1);
      }
    }
    /* else {
      printf("IDENT Token: %i : %i", ans->type, T_IDENT);
      printf("Unknown word type for %s on line %d\n", s, yyline);
      exit(1);
    } */
    ans->type = type;
    symbtab[h] = ans;

    return ans;
}
