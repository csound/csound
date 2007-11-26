 /*
    symbtab.c:

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
#include <stdlib.h>
#include <string.h>
#include "csoundCore.h"
#include "tok.h"
#include "csound_orcparse.h"


ORCTOKEN** symbtab;
extern int yyline;
extern int udoflag;

ORCTOKEN *add_token(CSOUND *csound, char *s, int type);

int get_opcode_type(OENTRY *ep) {
    int retVal = 0;

//    if((ep->outypes == NULL || strlen(ep->outypes) == 0) &&
//       (ep->intypes == NULL || strlen(ep->intypes) == 0)) {
//        retVal = T_OPCODE00;
//    } else

    if(ep->outypes == NULL || strlen(ep->outypes) == 0) {
        retVal = T_OPCODE0;
    } else {
        retVal = T_OPCODE;
    }

    return retVal;
}

void init_symbtab(CSOUND *csound)
{
    symbtab = (ORCTOKEN**)mcalloc(csound, HASH_SIZE * sizeof(ORCTOKEN*));
    /* Now we need to populate with basic words */

    OENTRY *ep;
    OENTRY *temp;

    int len = 0;

    /* Add token types for opcodes to symbtab.  If a polymorphic opcode definition is
     * found (dsblksiz >= 0xfffb), look for implementations of that opcode to correctly
     * mark the type of opcode it is (T_OPCODE, T_OPCODE0, or T_OPCODE00)
     */

    for (ep = (OENTRY*) csound->opcodlst; ep < (OENTRY*) csound->oplstend; ep++) {

        if(ep->dsblksiz >= 0xfffb) {
            csound->Message(csound, "Found PolyMorphic Opcode %s\n",ep->opname);

            len = strlen(ep->opname) + 1;
            char * polyName = mcalloc(csound, len + 1);
            sprintf(polyName, "%s.", ep->opname);

            for(temp = (OENTRY*) csound->opcodlst; temp < (OENTRY*) csound->oplstend; temp++) {
                if(ep != temp && strncmp(polyName, temp->opname, len) == 0) {
                    add_token(csound, ep->opname, get_opcode_type(temp));

                    if(get_opcode_type(temp) == T_OPCODE) {
                        csound->Message(csound, "Using Type T_OPCODE\n");
                    } else {
                        csound->Message(csound, "Using Type T_OPCODE0\n");
                    }

                    break;
                }
            }

            mfree(csound, polyName);

//            if(strchr(ep->opname, '.') != NULL) {
//                csound->Message(csound, "Found PolyMorphic Opcode Definition %s\n",ep->opname);
//            }

        } else {
//            csound->Message(csound, "Found Regular Opcode %s\n",ep->opname);
          add_token(csound, ep->opname,get_opcode_type(ep));
        }


    }


    /* This adds all the T_FUNCTION tokens.  These should only be
     * looked up in context when parsing a expression list (not yet done)
     * and perhaps a more intelligent way needs to be done eventually to
     * look up opcodes which can serve as functions, as well as for allowing
     * multiple arguments to functions
     */
    add_token(csound, "int", T_FUNCTION);
    add_token(csound, "frac", T_FUNCTION);
    add_token(csound, "round", T_FUNCTION);
    add_token(csound, "floor", T_FUNCTION);
    add_token(csound, "ceil", T_FUNCTION);
    add_token(csound, "rnd", T_FUNCTION);
    add_token(csound, "birnd", T_FUNCTION);
    add_token(csound, "abs", T_FUNCTION);
    add_token(csound, "exp", T_FUNCTION);
    add_token(csound, "log", T_FUNCTION);
    add_token(csound, "sqrt", T_FUNCTION);
    add_token(csound, "sin", T_FUNCTION);
    add_token(csound, "cos", T_FUNCTION);
    add_token(csound, "tan", T_FUNCTION);
    add_token(csound, "sininv", T_FUNCTION);
    add_token(csound, "cosinv", T_FUNCTION);
    add_token(csound, "taninv", T_FUNCTION);
    add_token(csound, "log10", T_FUNCTION);
    add_token(csound, "sinh", T_FUNCTION);
    add_token(csound, "cosh", T_FUNCTION);
    add_token(csound, "tanh", T_FUNCTION);
    add_token(csound, "ampdb", T_FUNCTION);
    add_token(csound, "ampdbfs", T_FUNCTION);
    add_token(csound, "dbamp", T_FUNCTION);
    add_token(csound, "dbfsamp", T_FUNCTION);
    add_token(csound, "ftlen", T_FUNCTION);
    add_token(csound, "ftsr", T_FUNCTION);
    add_token(csound, "ftlptim", T_FUNCTION);
    add_token(csound, "ftchnls", T_FUNCTION);
    add_token(csound, "i", T_FUNCTION);
    add_token(csound, "k", T_FUNCTION);
    add_token(csound, "cpsoct", T_FUNCTION);
    add_token(csound, "octpch", T_FUNCTION);
    add_token(csound, "cpspch", T_FUNCTION);
    add_token(csound, "pchoct", T_FUNCTION);
    add_token(csound, "octcps", T_FUNCTION);
    add_token(csound, "nsamp", T_FUNCTION);
    add_token(csound, "powoftwo", T_FUNCTION);
    add_token(csound, "logbtwo", T_FUNCTION);
    add_token(csound, "a", T_FUNCTION);
    add_token(csound, "tb0", T_FUNCTION);
    add_token(csound, "tb1", T_FUNCTION);
    add_token(csound, "tb2", T_FUNCTION);
    add_token(csound, "tb3", T_FUNCTION);
    add_token(csound, "tb4", T_FUNCTION);
    add_token(csound, "tb5", T_FUNCTION);
    add_token(csound, "tb6", T_FUNCTION);
    add_token(csound, "tb7", T_FUNCTION);
    add_token(csound, "tb8", T_FUNCTION);
    add_token(csound, "tb9", T_FUNCTION);
    add_token(csound, "tb10", T_FUNCTION);
    add_token(csound, "tb11", T_FUNCTION);
    add_token(csound, "tb12", T_FUNCTION);
    add_token(csound, "tb13", T_FUNCTION);
    add_token(csound, "tb14", T_FUNCTION);
    add_token(csound, "tb15", T_FUNCTION);
    add_token(csound, "urd", T_FUNCTION);
    add_token(csound, "not", T_FUNCTION);

}

unsigned int hash(char *s)
{
    unsigned int h = 0;
    while (*s != '\0') {
      h = (h<<4) ^ *s++;
    }
    return (h%HASH_SIZE);
}

ORCTOKEN *add_token(CSOUND *csound, char *s, int type)
{
    int h = hash(s);

    //printf("Hash value for %s: %i\n", s, h);

    ORCTOKEN *a = symbtab[h];
    ORCTOKEN *ans;
    while (a!=NULL) {
      if (strcmp(a->lexeme, s)==0) {
        if (type == a->type) return a;
        //printf("Type confusion for %s, stopping\n", s);
        exit(1);
      }
      a = a->next;
    }
    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);
    ans->next = symbtab[h];
    ans->type = type;

    symbtab[h] = ans;
    return ans;
}

int isUDOArgList(char *s) {

    int len = strlen(s) - 1;

    while (len >= 0) {
      if (strchr("aijkKop0", s[len]) == NULL) {
        /* printf("Invalid char '%c' in '%s'", *p, s); */
        return 0;
      }

      len--;
    }

    return 1;
}

int isUDOAnsList(char *s) {
    int len = strlen(s) - 1;

    while (len >= 0) {
      if (strchr("aikK0", s[len]) == NULL) {
        return 0;
      }

      len--;
    }

    return 1;
}

ORCTOKEN *lookup_token(CSOUND *csound, char *s)
{
    int h = hash(s);
    int type = T_IDENT;
    ORCTOKEN *a = symbtab[h];
    ORCTOKEN *ans;

    csound->Message(csound, "Looking up token for: %d : %s\n", h, s);

    if (udoflag == 0) {

      if (isUDOAnsList(s)) {
        ans = new_token(csound, T_UDO_ANS);
        ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        ans->next = symbtab[h];
        symbtab[h] = ans;
        //printf("Found UDO Answer List\n");
        return ans;
      }
    }

    if (udoflag == 1) {
      if (isUDOArgList(s)) {
        ans = new_token(csound, T_UDO_ARGS);
        ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
        strcpy(ans->lexeme, s);
        ans->next = symbtab[h];
        symbtab[h] = ans;
        //printf("Found UDO Arg List\n");
        return ans;
      }
    }

    while (a!=NULL) {
      if (strcmp(s, "reverb") == 0) {
        csound->Message(csound, "Looking up token for: %d: %d: %s : %s\n",
                        hash("reverb"), hash("a4"), s, a->lexeme);
      }

      if (strcmp(a->lexeme, s)==0) {
        ans = (ORCTOKEN*)mmalloc(csound, sizeof(ORCTOKEN));
        memcpy(ans, a, sizeof(ORCTOKEN));
        ans->next = NULL;
        ans->lexeme = (char *)mmalloc(csound, strlen(a->lexeme) + 1);
        strcpy(ans->lexeme, a->lexeme);

        return ans;
      }
      a = a->next;
    }


    ans = new_token(csound, T_IDENT);
    ans->lexeme = (char*)mmalloc(csound, 1+strlen(s));
    strcpy(ans->lexeme, s);
    //ans->next = symbtab[h];

    if(udoflag == -2) {
        return ans;
    }

    // NEED TO FIX: In case of looking for label for kgoto or other opcodes, need
    // to return T_IDENT instead of any sub-type
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
        csound->Message(csound, "Unknown word type for %s on line %d\n", s, yyline);
        exit(1);
      }
    }
    /* else {
      printf("IDENT Token: %i : %i", ans->type, T_IDENT);
      printf("Unknown word type for %s on line %d\n", s, yyline);
      exit(1);
    } */
    ans->type = type;
    //symbtab[h] = ans;

    return ans;
}
