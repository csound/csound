#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tok.h"
#include "jsnd.yacc.tab.h"

static TOKEN** symbtab;
#define HASH_SIZE (1000)

void init_symbtab(void)
{
    symbtab = (TOKEN**)calloc(HASH_SIZE, sizeof(TOKEN*));
}

int hash(char *s)
{
    int h = 0;
    while (*s != '\0') {
      h = (h<<4) ^ *s++;
    }
    return (h%HASH_SIZE);
}

TOKEN *lookup_token(char *s, int type)
{
    int	h = hash(s);
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

