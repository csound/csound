 /*
    csound_orc_semantics.c:

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
#include "csoundCore.h"
#include "csound_orc.h"
#include "namedins.h"
#include "parse_param.h"
#include "csound_type_system.h"

char *csound_orcget_text ( void *scanner );

extern  char argtyp2(char*);
extern  int tree_arg_list_count(TREE *);
void print_tree(CSOUND *, char *, TREE *);

/* from csound_orc_compile.c */
extern int argsRequired(char* arrayName);
extern char** splitArgs(CSOUND* csound, char* argString);
extern int pnum(char*);

/* from csound_orc_expressions.c */
extern int is_expression_node(TREE *node);
    

char* cs_strdup(CSOUND* csound, char* str) {
    size_t len = strlen(str);
    
    if(len == 0) {
        return NULL;
    }
    
    char* retVal = mmalloc(csound, (len + 1) * sizeof(char));
    memcpy(retVal, str, len * sizeof(char));
    retVal[len] = NULL;
    
    return retVal;
}

char* cs_strndup(CSOUND* csound, char* str, size_t size) {
    size_t len = strlen(str);
    
    if(len == 0) {
        return NULL;
    } else if (size > len) {
        return cs_strdup(csound, str);
    } 
    
    char* retVal = mmalloc(csound, (size + 1) * sizeof(char));
    memcpy(retVal, str, size * sizeof(char));
    retVal[size] = NULL;
    
    return retVal;
}


PUBLIC char* get_arg_type(CSOUND* csound, TREE* tree)
{                   /* find arg type:  d, w, a, k, i, c, p, r, S, B, b, t */
    char* s;
    CS_TYPE* type;
    
    switch(tree->type) {
        case NUMBER_TOKEN:
        case INTEGER_TOKEN:
            return cs_strdup(csound, "c");                              /* const */
        case STRING_TOKEN:
            return cs_strdup(csound, "S");                              /* quoted String */
        case SRATE_TOKEN:
        case KRATE_TOKEN:
        case KSMPS_TOKEN:
        case ZERODBFS_TOKEN:
        case NCHNLS_TOKEN:
        case NCHNLSI_TOKEN:
            return cs_strdup(csound, "r");                              /* rsvd */
        case LABEL_TOKEN:
            //FIXME: Need to review why label token is used so much in parser, for now treat as
            //T_IDENT
        case T_IDENT:
            s = tree->value->lexeme;
            
            if (pnum(s) >= 0)
                return cs_strdup(csound, "p");                              /* pnum */
            if (*s == '#')
                s++;
            if (*s == 'g')
                s++;
            type = csoundGetTypeForVarName(csound->typePool, s);
            if (type != NULL) {
                return cs_strdup(csound, type->varTypeName);
            } else {
                return cs_strdup(csound, "l"); // assume it is a label
            }
        case T_ARRAY:
        case T_ARRAY_IDENT:
            // not yet handled, falling through for now:
            //        int len = 1;
            //        while(c != '[') {
            //            c = *(++s);
            //            len++;
            //        }
            //        char* retVal = mmalloc(csound, (len + 2) * sizeof(char)); // add 2 for semicolon and NULL
            //        memcpy(retVal, identifier, len);
            //        retVal[len] = ';';
            //        retVal[len + 1] = NULL;
            //        return retVal;
        default:
//            csoundWarning(csound, "Unknown arg type: %d\n", tree->type);
//            print_tree(csound, "Arg Tree\n", tree);
            return NULL;
    }
}

char* get_arg_type_for_expression(CSOUND* csound, TREE* root) {
    return NULL;
}

int verify_expression(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {
    return 0;
}

int out_arg_type_matches(char* foundArg, char* specifiedArg) {
    
}

int in_arg_type_matches(char* foundArg, char* specifiedArg) {
}

/* Finds OENTRIES that match the given opcode name.  May return multiple OENTRY*'s for each
 * entry in a polyMorphic opcode.
 */
PUBLIC OENTRIES* find_opcode2(CSOUND* csound, OENTRY* opcodeList, OENTRY* endOpcode, char* opname) {
    
    if (opname == NULL) {
        return NULL;
    }
    
    int listIndex = 0;
    int i;
    
    OENTRY* opc = opcodeList;
    OENTRIES* retVal = mcalloc(csound, sizeof(OENTRIES));
    
    int opLen = strlen(opname);
    
    for (i=0; opc < endOpcode; opc++, i++) {
        if(strncmp(opname, opc->opname, opLen) == 0) {
            // hack to work with how opcodes are currently defined with ".x" endings for polymorphism
            if(opc->opname[opLen] == 0 || opc->opname[opLen] == '.') {
                retVal->entries[listIndex++] = opc;
            }
        }
    }
    retVal->count = listIndex;
    
    return retVal;
}

/* Given an OENTRIES list, resolve to a single OENTRY* based on the found in- and out- argtypes.
 * Returns NULL if opcode could not be resolved. If more than one entry matches, mechanism assumes
 * there are multiple opcode entries with same types and last one should override previous definitions.
 */
PUBLIC OENTRY* resolve_opcode(OENTRIES* entries, char* outArgTypes, char* inArgTypes) {
    
    return NULL;
}

PUBLIC char* get_arg_string_from_tree(CSOUND* csound, TREE* tree) {

    int len = tree_arg_list_count(tree);
    int i;
    
    if (len == 0) {
        return NULL;
    }
    
    char** argTypes = mmalloc(csound, len * sizeof(char*));
    char* argString = NULL;
    TREE* current = tree;
    int index = 0;
    int argsLen = 0;
    
    while (current != NULL) {
        char* argType = get_arg_type(csound, current);

        //FIXME - fix if argType is NULL and remove the below hack
        if(argType == NULL) {
            argsLen += 1;
            argTypes[index++] = "?";
        } else {
            argsLen += strlen(argType);
            argTypes[index++] = argType;
        }

        
        current = current->next;
    }

    argString = mmalloc(csound, argsLen);
    char* temp = argString;
    
    for (i = 0; i < len; i++) {
        int size = strlen(argTypes[i]);
        memcpy(temp, argTypes[i], size);
        temp += size;
    }

    argString[argsLen] = NULL;
    
//    for (i = 0; i < len; i++) {
//         csoundMessage(csound, "%d) Found arg type: %s\n", i, argTypes[i]);
//    }
    
//    return argString;
    return argString;
    
}

/*
 * Verifies: number of args correct, types of arg correct
 */
int verify_opcode(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {
    
    TREE* left = root->left;
    TREE* right = root->right;
    
    char* leftArgString = get_arg_string_from_tree(csound, left);
    char* rightArgString = get_arg_string_from_tree(csound, right);
    
//    csound->Message(csound, "Verifying Opcode: %s\n", root->value->lexeme);
//    csound->Message(csound, "    Arg Types Found: %s | %s\n", leftArgString, rightArgString);

//    OENTRY* entry = find_opcode(csound, root->value->lexeme);
//    
//    if(entry == NULL) {
//        synterr(csound, "Unknown opcode: %s\n", root->value->lexeme);
//        return CSOUND_ERROR;
//    }
    
    //csound->Message(csound, "    Arg Types Required: %s | %s\n", entry->outypes, entry->intypes);
    
//    print_tree(csound, "OP LEFT: ", left);
//    print_tree(csound, "OP RIGHT: ", right);
    
    //        synterr(csound,
    //                Str("input arg '%s' used before defined (in opcode %s),"
    //                    " line %d\n"),
    //                s, ep->opname, line);
    
    return 0;
}

TREE* verify_tree(CSOUND * csound, TREE *root, TYPE_TABLE* typeTable)
{
    if (PARSER_DEBUG) csound->Message(csound, "Verifying AST\n");
    
    TREE *anchor = NULL;
    
    TREE *current = root;
    TREE *previous = NULL;
    
    while(current != NULL) {
        switch(current->type) {
            case INSTR_TOKEN:
                if (PARSER_DEBUG) csound->Message(csound, "Instrument found\n");
                typeTable->localPool = mcalloc(csound, sizeof(CS_VAR_POOL));
                current->right = verify_tree(csound, current->right, typeTable);
                mfree(csound, typeTable->localPool);
                typeTable->localPool = NULL;
                break;
            case UDO_TOKEN:
                if (PARSER_DEBUG) csound->Message(csound, "UDO found\n");
                
                if (current->left == NULL || current->right == NULL) {
                    
                }
                
                
                typeTable->localPool = mcalloc(csound, sizeof(CS_VAR_POOL));
                current->right = verify_tree(csound, current->right, typeTable);
                mfree(csound, typeTable->localPool);
                typeTable->localPool = NULL;
                break;
                
            case IF_TOKEN:
            case UNTIL_TOKEN:
                
                break;
                
            default:
                
//                csound->Message(csound, "Statement: %s\n", current->value->lexeme);
                
                verify_opcode(csound, current, typeTable);
                
//                if (current->right != NULL) {
//                    if (PARSER_DEBUG) csound->Message(csound, "Found Statement.\n");
//                    
//                    if (current->type == '=' && previous != NULL) {
//                        /* '=' should be guaranteed to have left and right
//                         * arg by the time it gets here */
//                        if (previous->left != NULL && previous->left->value != NULL) {
//                            if (strcmp(previous->left->value->lexeme,
//                                       current->right->value->lexeme) == 0) {
//                                
//                            }
//                            
//                        }
//                        
//                    }
//                }
        }
        
        if (anchor == NULL) {
            anchor = current;
        }
        
        previous = current;
        current = current->next;
        
    }
    
    if (PARSER_DEBUG) csound->Message(csound, "[End Verifying AST]\n");
    
    return anchor;
    
}


/* BISON PARSER FUNCTION */
int csound_orcwrap()
{
#ifdef DEBUG
    printf("\n === END OF INPUT ===\n");
#endif
    return (1);
}

/* UTILITY FUNCTIONS */

extern int csound_orcget_lineno(void*);
extern char *csound_orcget_current_pointer(void *);
/* BISON PARSER FUNCTION */
void csound_orcerror(PARSE_PARM *pp, void *yyscanner,
                     CSOUND *csound, TREE *astTree, const char *str)
{
    char ch;
    char *p = csound_orcget_current_pointer(yyscanner)-1;
    int line = csound_orcget_lineno(yyscanner);
    if (*p=='\0') line--;
    csound->Message(csound, Str("\nerror: %s  (token \"%s\")"),
                    str, csound_orcget_text(yyscanner));
    csound->Message(csound, Str(" line %d:\n>>>"), line);
    /* if(!strcmp(csound_orcget_text(yyscanner), "\n")) { */
    /*  csound->Message(csound, Str("error: %s (\"\\n\")"), */
    /*                 str); */
    /*  csound->Message(csound, Str(" line %d:\n>>> "), */
    /*               csound_orcget_lineno(yyscanner)); */
    /* } */
    /* else { */
    /*  csound->Message(csound, Str("\nerror: %s  (token \"%s\")"), */
    /*                 str, csound_orcget_text(yyscanner)); */
    /* csound->Message(csound, Str(" line %d:\n>>> "), */
    /*                 csound_orcget_lineno(yyscanner)+1); */
    /* } */
    while ((ch=*--p) != '\n' && ch != '\0');
    do {
      ch = *++p;
      if(ch == '\n') break;
      csound->Message(csound, "%c", ch);
    } while (ch != '\n' && ch != '\0');
    csound->Message(csound, " <<<\n");
}


/**
 * Appends TREE * node to TREE * node using ->next field in struct; walks
 * down the linked list to append at end; checks for NULL's and returns
 * appropriate nodes
 */
TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast) {
    TREE *current;
    if (first == NULL) {
        return newlast;
    }

    if (newlast == NULL) {
        return first;
    }

    /* HACK - Checks to see if first node is uninitialized (sort of)
     * This occurs for rules like in topstatement where the left hand
     * topstatement the first time around is not initialized to anything
     * useful; the number 400 is arbitrary, chosen as it seemed to be a
     * value higher than all the type numbers that were being printed out
     */
    if (first->type > 400 || first-> type < 0) {
        return newlast;
    }

    current = first;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = newlast;

    return first;
}


/* USED BY PARSER TO ASSEMBLE TREE NODES */
TREE* make_node(CSOUND *csound, int line, int locn, int type,
                TREE* left, TREE* right)
{
    TREE *ans;
    ans = (TREE*)mmalloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      /* fprintf(stderr, "Out of memory\n"); */
      exit(1);
    }
    ans->type = type;
    ans->left = left;
    ans->right = right;
    ans->value = NULL;          /* New code -- JPff */
    ans->next = NULL;
    ans->len = 2;
    ans->rate = -1;
    ans->line = line;
    ans->locn  = locn;
    csound->DebugMsg(csound, "%s(%d) line = %d\n", __FILE__, __LINE__, line);
    return ans;
}

TREE* make_leaf(CSOUND *csound, int line, int locn, int type, ORCTOKEN *v)
{
    TREE *ans;
    ans = (TREE*)mmalloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      /* fprintf(stderr, "Out of memory\n"); */
      exit(1);
    }
    ans->type = type;
    ans->left = NULL;
    ans->right = NULL;
    ans->next = NULL;
    ans->len = 0;
    ans->rate = -1;
    ans->value = v;
    ans->line = line;
    ans->locn  = locn;
    csound->DebugMsg(csound, "%s(%d) line = %d\n", __FILE__, __LINE__, line);
    return ans;
}

/** Utility function to rewrite array names from xxx to [xxx, as csound
 *  uses first letter to denote type.  Also handles if name starts with g;
 *  used by parser (csound_orc.y)
 */
PUBLIC char* convertArrayName(CSOUND* csound, char* arrayName) {
    if(arrayName == NULL) {
        return NULL;
    }
    int len = strlen(arrayName);
    
    if(len == 0) {
        return NULL;
    }
    
    char* newArrayName = mmalloc(csound, (len + 3)* sizeof(char));
    if(arrayName[0] == 'g') {
        newArrayName[0] = 'g';
        newArrayName[1] = '[';
        newArrayName[2] = arrayName[1];
        newArrayName[3] = ';';
        memcpy(newArrayName + 4, arrayName + 2, len - 2);
    } else {
        newArrayName[0] = '[';
        newArrayName[1] = arrayName[0];
        newArrayName[2] = ';';
        memcpy(newArrayName + 3, arrayName + 1, len - 1);
    }
    newArrayName[len + 2] = 0;
    
    return newArrayName;
}

PUBLIC char* addDimensionToArrayName(CSOUND* csound, char* arrayName) {
    if(arrayName == NULL) {
        return NULL;
    }
    int len = strlen(arrayName);
    
    if(len == 0) {
        return NULL;
    }
    
    char* newArrayName = mmalloc(csound, (len + 2)* sizeof(char));
    if(arrayName[0] == 'g') {
        newArrayName[0] = 'g';
        newArrayName[1] = '[';
        memcpy(newArrayName + 2, arrayName + 1, len - 1);
    } else {
        newArrayName[0] = '[';
        memcpy(newArrayName + 1, arrayName , len);
    }
    newArrayName[len + 1] = 0;
    
    return newArrayName;
}

/** Utility function to create assignment statements
 *  Replaces = with correct version for args
 */
char* get_assignment_type(CSOUND *csound, char * ans, TREE* arg1) {
    char c = argtyp2( ans);
    char* str = (char*)mcalloc(csound, 65);

    switch (c) {
    case 'S':
      strcpy(str, "strcpy");
      break;
    case 'a':
      c = argtyp2( arg1->value->lexeme);
      strcpy(str, (c == 'a' ? "=.a" : "upsamp"));
      /* strcpy(str, "=.a"); */
      break;
    case 'p':
      c = 'i'; /* purposefully fall through */
    default:
      sprintf(str, "=.%c", c);
    }

    if (PARSER_DEBUG)
      csound->Message(csound, "Found Assignment type: %s\n", str);

    return str;
}

void delete_tree(CSOUND *csound, TREE *l)
{
    while (1) {
      TREE *old = l;
      if (UNLIKELY(l==NULL)) {
        return;
      }
      if (l->value) {
       if (l->value->lexeme) {
         printf("Free %p (%s)\n", l->value->lexeme, l->value->lexeme);
        mfree(csound, l->value->lexeme);
        //l->value->lexeme = NULL;
       }
      //printf("Free val %p\n", l->value);
       mfree(csound, l->value);
       //l->value = NULL;
      }
      delete_tree(csound, l->left);
      //l->left = NULL;
      delete_tree(csound, l->right);
      //l->right = NULL;
      l = l->next;
      //printf("Free %p\n", old);
      mfree(csound, old);
    }
}


/* DEBUGGING FUNCTIONS */
void print_tree_i(CSOUND *csound, TREE *l, int n)
{
    int i;
    if (UNLIKELY(l==NULL)) {
      return;
    }
    for (i=0; i<n; i++) {
      csound->Message(csound, " ");
    }

    csound->Message(csound, "TYPE: %d ", l->type);

    switch (l->type) {
    case ',':
    case '?':
    case ':':
    case '!':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '^':
    case '(':
    case ')':
    case '=':
      csound->Message(csound,"%c:(%d:%d)\n", l->type, l->line, l->locn); break;
    case NEWLINE:
      csound->Message(csound,"NEWLINE:(%d:%d)\n", l->line, l->locn); break;
    case S_NEQ:
      csound->Message(csound,"S_NEQ:(%d:%d)\n", l->line, l->locn); break;
    case S_AND:
      csound->Message(csound,"S_AND:(%d:%d)\n", l->line, l->locn); break;
    case S_OR:
      csound->Message(csound,"S_OR:(%d:%d)\n", l->line, l->locn); break;
    case S_LT:
      csound->Message(csound,"S_LT:(%d:%d)\n", l->line, l->locn); break;
    case S_LE:
      csound->Message(csound,"S_LE:(%d:%d)\n", l->line, l->locn); break;
    case S_EQ:
      csound->Message(csound,"S_EQ:(%d:%d)\n", l->line, l->locn); break;
    case S_TASSIGN:
      csound->Message(csound,"S_TASSIGN:(%d:%d)\n", l->line, l->locn); break;
    case S_TABRANGE:
      csound->Message(csound,"S_TABRANGE:(%d:%d)\n", l->line, l->locn); break;
    case S_TABREF:
      csound->Message(csound,"S_TABREF:(%d:%d)\n", l->line, l->locn); break;
    case T_MAPK:
      csound->Message(csound,"T_MAPK:(%d:%d)\n", l->line, l->locn); break;
    case T_MAPI:
      csound->Message(csound,"T_MAPI:(%d:%d)\n", l->line, l->locn); break;
    case S_GT:
      csound->Message(csound,"S_GT:(%d:%d)\n", l->line, l->locn); break;
    case S_GE:
      csound->Message(csound,"S_GE:(%d:%d)\n", l->line, l->locn); break;
    case LABEL_TOKEN:
      csound->Message(csound,"LABEL_TOKEN: %s\n", l->value->lexeme); break;
    case IF_TOKEN:
      csound->Message(csound,"IF_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case THEN_TOKEN:
          csound->Message(csound,"THEN_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ITHEN_TOKEN:
          csound->Message(csound,"ITHEN_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KTHEN_TOKEN:
          csound->Message(csound,"KTHEN_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ELSEIF_TOKEN:
          csound->Message(csound,"ELSEIF_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ELSE_TOKEN:
          csound->Message(csound,"ELSE_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case UNTIL_TOKEN:
          csound->Message(csound,"UNTIL_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case DO_TOKEN:
          csound->Message(csound,"DO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case OD_TOKEN:
          csound->Message(csound,"OD_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case GOTO_TOKEN:
      csound->Message(csound,"GOTO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case IGOTO_TOKEN:
      csound->Message(csound,"IGOTO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KGOTO_TOKEN:
      csound->Message(csound,"KGOTO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case SRATE_TOKEN:
      csound->Message(csound,"SRATE_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KRATE_TOKEN:
      csound->Message(csound,"KRATE_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ZERODBFS_TOKEN:
      csound->Message(csound,"ZERODFFS_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KSMPS_TOKEN:
      csound->Message(csound,"KSMPS_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case NCHNLS_TOKEN:
      csound->Message(csound,"NCHNLS_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case NCHNLSI_TOKEN:
      csound->Message(csound,"NCHNLSI_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case INSTR_TOKEN:
      csound->Message(csound,"INSTR_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case STRING_TOKEN:
      csound->Message(csound,"STRING_TOKEN: %s\n", l->value->lexeme); break;
    case T_IDENT:
      csound->Message(csound,"T_IDENT: %s\n", l->value->lexeme); break;
    /*case T_IDENT_I:
      csound->Message(csound,"IDENT_I: %s\n", l->value->lexeme); break;
    case T_IDENT_GI:
      csound->Message(csound,"IDENT_GI: %s\n", l->value->lexeme); break;
    case T_IDENT_K:
      csound->Message(csound,"IDENT_K: %s\n", l->value->lexeme); break;
    case T_IDENT_GK:
      csound->Message(csound,"IDENT_GK: %s\n", l->value->lexeme); break;
    case T_IDENT_A:
      csound->Message(csound,"IDENT_A: %s\n", l->value->lexeme); break;
    case T_IDENT_GA:
      csound->Message(csound,"IDENT_GA: %s\n", l->value->lexeme); break;
    case T_IDENT_S:
      csound->Message(csound,"IDENT_S: %s\n", l->value->lexeme); break;
    case T_IDENT_GS:
      csound->Message(csound,"IDENT_GS: %s\n", l->value->lexeme); break;
    case T_IDENT_T:
      csound->Message(csound,"IDENT_T: %s\n", l->value->lexeme); break;
    case T_IDENT_GT:
      csound->Message(csound,"IDENT_GT: %s\n", l->value->lexeme); break;
    case T_IDENT_W:
      csound->Message(csound,"IDENT_W: %s\n", l->value->lexeme); break;
    case T_IDENT_GW:
      csound->Message(csound,"IDENT_GW: %s\n", l->value->lexeme); break;
    case T_IDENT_F:
      csound->Message(csound,"IDENT_F: %s\n", l->value->lexeme); break;
    case T_IDENT_GF:
      csound->Message(csound,"IDENT_GF: %s\n", l->value->lexeme); break;
    case T_IDENT_P:
      csound->Message(csound,"IDENT_P: %s\n", l->value->lexeme); break;
    case T_IDENT_B:
      csound->Message(csound,"IDENT_B: %s\n", l->value->lexeme); break;
    case T_IDENT_b:
      csound->Message(csound,"IDENT_b: %s\n", l->value->lexeme); break; */
    case INTEGER_TOKEN:
      csound->Message(csound,"INTEGER_TOKEN: %d\n", l->value->value); break;
    case NUMBER_TOKEN:
      csound->Message(csound,"NUMBER_TOKEN: %f\n", l->value->fvalue); break;
    case S_ANDTHEN:
      csound->Message(csound,"S_ANDTHEN:(%d:%d)\n", l->line, l->locn); break;
    case S_APPLY:
      csound->Message(csound,"S_APPLY:(%d:%d)\n", l->line, l->locn); break;
    case T_OPCODE0:
      csound->Message(csound,"T_OPCODE0: %s\n", l->value->lexeme); break;
    case T_OPCODE:
      csound->Message(csound,"T_OPCODE: %s\n", l->value->lexeme); break;
    case T_FUNCTION:
      csound->Message(csound,"T_FUNCTION: %s\n", l->value->lexeme); break;
    case S_UMINUS:
        csound->Message(csound,"S_UMINUS:(%d:%d)\n", l->line, l->locn); break;
    case T_INSTLIST:
        csound->Message(csound,"T_INSTLIST:(%d:%d)\n", l->line, l->locn); break;
    case T_TADD:
      csound->Message(csound,"T_TADD:(%d:%d)\n", l->line, l->locn); break;
    case T_SUB:
      csound->Message(csound,"T_SUB:(%d:%d)\n", l->line, l->locn); break;
    case S_TUMINUS:
      csound->Message(csound,"S_TUMINUS:(%d:%d)\n", l->line, l->locn); break;
    case T_TMUL:
      csound->Message(csound,"T_TMUL:(%d:%d)\n", l->line, l->locn); break;
    case T_TDIV:
      csound->Message(csound,"T_TDIV:(%d:%d)\n", l->line, l->locn); break;
    case T_TREM:
      csound->Message(csound,"T_TREM:(%d:%d)\n", l->line, l->locn); break;
    default:
      csound->Message(csound,"unknown:%d(%d:%d)\n", l->type, l->line, l->locn);
    }

    print_tree_i(csound, l->left,n+1);
    print_tree_i(csound, l->right,n+1);

    if (l->next != NULL) {
        print_tree_i(csound, l->next, n);
    }
}

enum {TREE_NONE, TREE_LEFT, TREE_RIGHT, TREE_NEXT};
static void print_tree_xml(CSOUND *csound, TREE *l, int n, int which)
{
    int i;
    char *child[4] = {"", "left", "right", "next"};
    if (l==NULL) {
      return;
    }
    for (i=0; i<n; i++) {
      csound->Message(csound, " ");
    }

    csound->Message(csound, "<tree%s type=\"%d\" ", child[which], l->type);

    switch (l->type) {
    case ',':
    case '?':
    case ':':
    case '!':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '^':
    case '(':
    case ')':
    case '=':
    case '|':
    case '&':
    case '#':
    case '~':
      csound->Message(csound,"name=\"%c\"", l->type); break;
    case NEWLINE:
      csound->Message(csound,"name=\"NEWLINE\""); break;
    case S_NEQ:
      csound->Message(csound,"name=\"S_NEQ\""); break;
    case S_AND:
      csound->Message(csound,"name=\"S_AND\""); break;
    case S_OR:
      csound->Message(csound,"name=\"S_OR\""); break;
    case S_LT:
      csound->Message(csound,"name=\"S_LT\""); break;
    case S_LE:
      csound->Message(csound,"name=\"S_LE\""); break;
    case S_EQ:
      csound->Message(csound,"name=\"S_EQ\""); break;
    case S_TASSIGN:
      csound->Message(csound,"name=\"S_TASSIGN\""); break;
    case S_TABRANGE:
      csound->Message(csound,"name=\"S_TABRANGE\""); break;
    case S_TABREF:
      csound->Message(csound,"name=\"S_TABREF\""); break;
    case S_A2K:
      csound->Message(csound,"name=\"S_A2K\""); break;
    case S_GT:
      csound->Message(csound,"name=\"S_GT\""); break;
    case S_GE:
      csound->Message(csound,"name=\"S_GE\""); break;
    case S_BITSHIFT_RIGHT:
      csound->Message(csound,"name=\"S_BITSHIFT_RIGHT\""); break;
    case S_BITSHIFT_LEFT:
      csound->Message(csound,"name=\"S_BITSHIFT_LEFT\""); break;
    case LABEL_TOKEN:
      csound->Message(csound,"name=\"LABEL_TOKEN\" label=\"%s\"",
                      l->value->lexeme); break;
    case IF_TOKEN:
      csound->Message(csound,"name=\"IF_TOKEN\""); break;
    case THEN_TOKEN:
          csound->Message(csound,"name=\"THEN_TOKEN\""); break;
    case ITHEN_TOKEN:
          csound->Message(csound,"name=\"ITHEN_TOKEN\""); break;
    case KTHEN_TOKEN:
          csound->Message(csound,"name=\"KTHEN_TOKEN\""); break;
    case ELSEIF_TOKEN:
          csound->Message(csound,"name=\"ELSEIF_TOKEN\""); break;
    case ELSE_TOKEN:
          csound->Message(csound,"name=\"ELSE_TOKEN\""); break;
    case UNTIL_TOKEN:
          csound->Message(csound,"name=\"UNTIL_TOKEN\""); break;
    case DO_TOKEN:
          csound->Message(csound,"name=\"DO_TOKEN\""); break;
    case OD_TOKEN:
          csound->Message(csound,"name=\"OD_TOKEN\""); break;
    case GOTO_TOKEN:
      csound->Message(csound,"name=\"GOTO_TOKEN\""); break;
    case IGOTO_TOKEN:
      csound->Message(csound,"name=\"IGOTO_TOKEN\""); break;
    case KGOTO_TOKEN:
      csound->Message(csound,"name=\"KGOTO_TOKEN\""); break;
    case SRATE_TOKEN:
      csound->Message(csound,"name=\"SRATE_TOKEN\""); break;
    case KRATE_TOKEN:
      csound->Message(csound,"name=\"KRATE_TOKEN\""); break;
    case ZERODBFS_TOKEN:
      csound->Message(csound,"name=\"ZERODBFS_TOKEN\""); break;
    case KSMPS_TOKEN:
      csound->Message(csound,"name=\"KSMPS_TOKEN\""); break;
    case NCHNLS_TOKEN:
      csound->Message(csound,"name=\"NCHNLS_TOKEN\""); break;
    case NCHNLSI_TOKEN:
      csound->Message(csound,"name=\"NCHNLSI_TOKEN\""); break;
    case INSTR_TOKEN:
      csound->Message(csound,"name=\"INSTR_TOKEN\""); break;
    case STRING_TOKEN:
      csound->Message(csound,"name=\"T_STRCONST\" str=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT:
      csound->Message(csound,"name=\"T_IDENT\" varname=\"%s\"",
                      l->value->lexeme); break;
    
    case T_ARRAY:
      csound->Message(csound,"name=\"T_ARRAY\""); break;
            
    case T_ARRAY_IDENT:
      csound->Message(csound,"name=\"T_ARRAY_IDENT\" varname=\"%s\"",
                      l->value->lexeme); break;
    
    case INTEGER_TOKEN:
      csound->Message(csound,"name=\"INTEGER_TOKEN\" value=\"%d\"",
                      l->value->value); break;
    case NUMBER_TOKEN:
      csound->Message(csound,"name=\"NUMBER_TOKEN\" value=\"%f\"",
                      l->value->fvalue); break;
    case S_ANDTHEN:
      csound->Message(csound,"name=\"S_ANDTHEN\""); break;
    case S_APPLY:
      csound->Message(csound,"name=\"S_APPLY\""); break;
    case T_OPCODE0:
      csound->Message(csound,"name=\"T_OPCODE0\" opname0=\"%s\"",
                      l->value->lexeme); break;
    case T_OPCODE:
      csound->Message(csound,"name=\"T_OPCODE\" opname=\"%s\"",
                      l->value->lexeme); break;
    case T_FUNCTION:
      csound->Message(csound,"name=\"T_FUNCTION\" fname=\"%s\"",
                      l->value->lexeme); break;
    case S_UMINUS:
      csound->Message(csound,"name=\"S_UMINUS\""); break;
    case T_INSTLIST:
      csound->Message(csound,"name=\"T_INSTLIST\""); break;
    case UDO_TOKEN:
      csound->Message(csound,"name=\"UDO_TOKEN\""); break;
    case UDO_ANS_TOKEN:
      csound->Message(csound,"name=\"UDO_ANS_TOKEN\" signature=\"%s\"",
                      l->value->lexeme); break;
    case UDO_ARGS_TOKEN:
      csound->Message(csound,"name=\"UDO_ARGS_TOKEN\" signature=\"%s\"",
                      l->value->lexeme); break;
    case S_ELIPSIS:
      csound->Message(csound,"name=\"S_ELIPSIS\""); break;
    case T_MAPI:
      csound->Message(csound,"name=\"T_MAPI\""); break;
    case T_MAPK:
      csound->Message(csound,"name=\"T_MAPK\""); break;
    case T_TADD:
      csound->Message(csound,"name=\"T_TADD\""); break;
    case T_SUB:
      csound->Message(csound,"name=\"T_SUB\""); break;
    case S_TUMINUS:
      csound->Message(csound,"name=\"S_TUMINUS\""); break;
    case T_TMUL:
      csound->Message(csound,"name=\"T_TMUL\""); break;
    case T_TDIV:
      csound->Message(csound,"name=\"T_TDIV\""); break;
    case T_TREM:
      csound->Message(csound,"name=\"T_TREM\""); break;
    default:
      csound->Message(csound,"name=\"unknown\"(%d)", l->type);
    }

    csound->Message(csound, " loc=\"%d:%d\">\n", l->line, l->locn);

    print_tree_xml(csound, l->left,n+1, TREE_LEFT);
    print_tree_xml(csound, l->right,n+1, TREE_RIGHT);

    for (i=0; i<n; i++) {
      csound->Message(csound, " ");
    }

    csound->Message(csound, "</tree%s>\n", child[which]);

    if (l->next != NULL) {
      print_tree_xml(csound, l->next, n, TREE_NEXT);
    }
}

void print_tree(CSOUND * csound, char* msg, TREE *l)
{
    /*if (PARSER_DEBUG)*/ {
      if (msg)
        csound->Message(csound, msg);
      else
        csound->Message(csound, "Printing Tree\n");
      csound->Message(csound, "<ast>\n");
      print_tree_xml(csound, l, 0, TREE_NONE);
      csound->Message(csound, "</ast>\n");
    }
}

void handle_optional_args(CSOUND *csound, TREE *l)
{
    if (l == NULL || l->type == LABEL_TOKEN) return;

    int opnum = find_opcode(csound, l->value->lexeme);
    OENTRY *ep = csound->opcodlst + opnum;
    int nreqd = 0;
    int incnt = tree_arg_list_count(l->right);
    TREE * temp;
    char** inArgParts;

    if (ep->intypes != NULL) {
      nreqd = argsRequired(ep->intypes);
      inArgParts = splitArgs(csound, ep->intypes);
    }

    if (PARSER_DEBUG) {
      csound->Message(csound, "Handling Optional Args for opcode %s, %d, %d",
                      ep->opname, incnt, nreqd);
      csound->Message(csound, "ep->intypes = >%s<\n", ep->intypes);
    }
    if (incnt < nreqd) {         /*  or set defaults: */
      do {
        switch (*inArgParts[incnt]) {
        case 'O':             /* Will this work?  Doubtful code.... */
        case 'o':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "0"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'P':
        case 'p':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "1"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'q':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN, 
                           make_int(csound, "10"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;

        case 'V':
        case 'v':
          temp = make_leaf(csound, l->line, l->locn, NUMBER_TOKEN,
                           make_num(csound, ".5"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'h':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "127"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'J':
        case 'j':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "-1"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'M':
        case 'N':
        case 'm':
          nreqd--;
          break;
        default:
          synterr(csound,
                  Str("insufficient required arguments for opcode %s on line %d\n"),
                  ep->opname, l->line, l->locn);
        }
        incnt++;
      } while (incnt < nreqd);
    }
}

char tree_argtyp(CSOUND *csound, TREE *tree) {
    if (tree->type == INTEGER_TOKEN || tree->type == NUMBER_TOKEN) {
      return 'i';
    }

    return argtyp2( tree->value->lexeme);
}

void handle_polymorphic_opcode(CSOUND* csound, TREE * tree) {
    if (tree->type == '=') {
      /* BUG: tree->right->value may be NULL */
      /* if (tree->right->value) */
      tree->value->lexeme =
        get_assignment_type(csound,
                            tree->left->value->lexeme,
                            tree->right/*->value->lexeme*/);
      return;
    }
    else if (tree->type==0) {
      csound->Message(csound, "Null type in tree -- aborting\n");
      exit(2);
    }
    else if (strcmp(tree->value->lexeme, "init") == 0 && 
            tree->left->type == T_ARRAY_IDENT) {
        // rewrite init as array_init
        tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                               strlen("array_init") + 1);
        strcpy(tree->value->lexeme, "array_init");
    }
    else {
      int opnum = find_opcode(csound, tree->value->lexeme);
      OENTRY *ep = csound->opcodlst + opnum;

/*     int incnt = tree_arg_list_count(tree->right); */

      char * str = (char *)mcalloc(csound, strlen(ep->opname) + 4);
      char c, d;

      if (ep->dsblksiz >= 0xfffb) {

        switch (ep->dsblksiz) {

        case 0xffff:
          /* use outype to modify some opcodes flagged as translating */
          if (PARSER_DEBUG)
            csound->Message(csound, "[%s]\n", tree->left->value->lexeme);

          c = tree_argtyp(csound, tree->left);
          if (c == 'p')   c = 'i';
          if (c == '?')   c = 'a';                   /* tmp */
          sprintf(str, "%s.%c", ep->opname, c);

          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);

     /*if (find_opcode(csound, str) == 0) {*/
     /* synterr(csound, Str("failed to find %s, output arg '%s' illegal type"),
        str, ST(group)[ST(nxtest)]);*/    /* report syntax error     */
     /*ST(nxtest) = 100; */                    /* step way over this line */
     /*goto tstnxt;*/                          /* & go to next            */
     /*break;*/
     /*}*/
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          csound->DebugMsg(csound, Str("modified opcod: %s"), str);
          break;
        case 0xfffe:                              /* Two tags for OSCIL's    */
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffe\n");
          c = tree_argtyp(csound, tree->right);
          if (c != 'a') c = 'k';
          if ((d = tree_argtyp(csound, tree->right->next)) != 'a') d = 'k';
          sprintf(str, "%s.%c%c", ep->opname, c, d);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        case 0xfffd:                              /* For peak, etc.          */
          c = tree_argtyp(csound, tree->right);
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffd\n");
          if (c != 'a') c = 'k';
          sprintf(str, "%s.%c", ep->opname, c);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        case 0xfffc:                              /* For divz types          */
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffc\n");
          c = tree_argtyp(csound, tree->right);
          d = tree_argtyp(csound, tree->right->next);
          if ((c=='i' || c=='c') && (d=='i' || d=='c'))
            c = 'i', d = 'i';
          else {
            if (c != 'a') c = 'k';
            if (d != 'a') d = 'k';
          }
          sprintf(str, "%s.%c%c", ep->opname, c, d);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        case 0xfffb:          /* determine opcode by type of first input arg */
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffb\n");
          c = tree_argtyp(csound, tree->right);
           /* allows a, k, and i types (e.g. Inc, Dec), but not constants */
          if (c=='p') c = 'i';
          /*if (ST(typemask_tabl)[(unsigned char) c] & (ARGTYP_i | ARGTYP_p))
            c = 'i';
            sprintf(str, "%s.%c", ST(linopcod), c);*/
          sprintf(str, "%s.%c", ep->opname, c);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        default:
          csound->Message(csound, "Impossible case\n");
          break;
        }

   /*if (!(isopcod(csound, str))) {*/
   /* if opcode is not found: report syntax error     */
   /*synterr(csound, Str("failed to find %s, input arg illegal type"), str);*/
   /*ST(nxtest) = 100;*/                       /* step way over this line */
   /*goto tstnxt;*/                            /* & go to next            */
   /*}
          ST(linopnum) = ST(opnum);
          ST(linopcod) = ST(opcod);
          csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));*/
      }

      /* free(str); */
    }
}
