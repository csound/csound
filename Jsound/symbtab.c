#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tok.h"
#include "jsnd5.tab.h"

TOKEN** symbtab;
extern int yyline;

void init_symbtab(void)
{
    symbtab = (TOKEN**)calloc(HASH_SIZE, sizeof(TOKEN*));
    /* Now we need to populate with basic words */

}

int hash(char *s)
{
    int h = 0;
    while (*s != '\0') {
      h = (h<<4) ^ *s++;
    }
    return (h%HASH_SIZE);
}

TOKEN *add_token(char *s, int type)
{
    int h = hash(s);
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

TOKEN *lookup_token(char *s)
{
    int h = hash(s);
    int type = -1;
    TOKEN *a = symbtab[h];
    TOKEN *ans;
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
    else {
      printf("Unknown word type for %s on line %d\n", s, yyline);
      exit(1);
    }
    ans->type = type;
    symbtab[h] = ans;
    return ans;
}
