 /*
    csound_orc_expressions.c:

    Copyright (C) 2006
    Steven Yi

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

#include "csoundCore.h"
#include "csound_orc.h"

extern char argtyp2(CSOUND *, char *);
extern void print_tree(CSOUND *, TREE *);
extern void handle_polymorphic_opcode(CSOUND*, TREE *);
extern void handle_optional_args(CSOUND *, TREE *);
extern ORCTOKEN *make_label(CSOUND *, char *);

char *create_out_arg(CSOUND *csound, char outype) {
    char* s = (char *)csound->Malloc(csound, 8);

    switch(outype) {
        case 'a': sprintf(s, "#a%d", csound->acount++); break;
        case 'K':
        case 'k': sprintf(s, "#k%d", csound->kcount++); break;
        case 'B': sprintf(s, "#B%d", csound->Bcount++); break;
        case 'b': sprintf(s, "#b%d", csound->bcount++); break;
        default:  sprintf(s, "#i%d", csound->icount++); break;
    }

    return s;
}

/**
 * Handles expression opcode type, appending to passed in opname
 * returns outarg type
 */
char *set_expression_type(CSOUND *csound, char * op, char arg1, char arg2) {
    char outype, *s;

    if (arg1 == 'a') {
        if (arg2 == 'a') {
            strcat(op,".aa");
        } else {
            strcat(op,".ak");
        }
        outype = 'a';
    } else if (arg2 == 'a') {
        strcat(op,".ka");
        outype = 'a';
    } else if (arg1 == 'k' || arg2 == 'k') {
        strcat(op,".kk");
        outype = 'k';
    } else {
        strcat(op,".ii");
        outype = 'i';
    }

    s = create_out_arg(csound, outype);

    csound->Message(csound, "SET_EXPRESSION_TYPE: %s : %s\n", op, s);

    return s;
}

char * get_boolean_arg(CSOUND *csound) {
    char* s = (char *)csound->Malloc(csound, 8);

    sprintf(s, "#B%d", csound->Bcount++);

    return s;
}

int get_expression_ans_type(CSOUND * csound, char * ans) {
    char * t = ans;
    t++;

    switch(*t) {
        case 'a':
            return T_IDENT_A;
        case 'k':
            return T_IDENT_K;
        case 'B':
            return T_IDENT_B;
        case 'b':
            return T_IDENT_b;
        default:
            return T_IDENT_I;
    }
}

TREE *create_empty_token(CSOUND *csound) {
  TREE *ans;
  ans = (TREE*)mmalloc(csound, sizeof(TREE));
  if (ans==NULL) {
    /* fprintf(stderr, "Out of memory\n"); */
    exit(1);
  }
  ans->type = -1;
  ans->left = NULL;
  ans->right = NULL;
  ans->next = NULL;
  ans->len = 0;
  ans->rate = -1;
  ans->value = NULL;
  return ans;
}

TREE *create_minus_token(CSOUND *csound) {
  TREE *ans;
  ans = (TREE*)mmalloc(csound, sizeof(TREE));
  if (ans==NULL) {
    /* fprintf(stderr, "Out of memory\n"); */
    exit(1);
  }
  ans->type = T_INTGR;
  ans->left = NULL;
  ans->right = NULL;
  ans->next = NULL;
  ans->len = 0;
  ans->rate = -1;
  ans->value = make_int(csound, "-1");
  return ans;
}

TREE * create_opcode_token(CSOUND *csound, char* op) {
  TREE *ans = create_empty_token(csound);

  ans->type = T_OPCODE;

  ans->value = make_token(csound, op);
  ans->value->type = T_OPCODE;

  return ans;

}

TREE * create_ans_token(CSOUND *csound, char* var) {
  TREE *ans = create_empty_token(csound);

  ans->type = get_expression_ans_type(csound, var);
  ans->value = make_token(csound, var);
  ans->value->type = ans->type;

  return ans;

}

TREE * create_goto_token(CSOUND *csound, char * booleanVar, TREE * gotoNode) {
/*     TREE *ans = create_empty_token(csound); */

    char* op = (char *)csound->Malloc(csound, 6);

    switch(gotoNode->type) {
        case T_KGOTO:
            sprintf(op, "ckgoto");
            break;
        case T_IGOTO:
            sprintf(op, "cigoto");
            break;
        case T_ITHEN:
	        sprintf(op, "cngoto");
            break;            
        case T_THEN:
        case T_KTHEN:
            sprintf(op, "cngoto");
            break;            
        default:
            sprintf(op, "cggoto");
    }

    TREE * opTree = create_opcode_token(csound, op);

    TREE * bVar = create_empty_token(csound);
    bVar->type = T_IDENT_B;
    bVar->value = make_token(csound, booleanVar);
    bVar->value->type = bVar->type;

    opTree->left = NULL;
    opTree->right = bVar;
    opTree->right->next = gotoNode->right;

    return opTree;
}

/* THIS PROBABLY NEEDS TO CHANGE TO RETURN DIFFERENT GOTO TYPES LIKE IGOTO, ETC */
TREE *create_simple_goto_token(CSOUND *csound, TREE *label) {
	TREE * opTree = create_opcode_token(csound, "goto");

	opTree->left = NULL;
	opTree->right = label;
	
    return opTree;
}

/* Returns if passed in TREE node is a numerical expression */
int is_expression_node(TREE *node) {
    if(node == NULL) {
        return 0;
    }

    switch(node->type) {
        case S_PLUS:
        case S_MINUS:
        case S_TIMES:
        case S_DIV:
        case T_FUNCTION:
        case S_UMINUS:
            return 1;
    }
    return 0;
}

/* Returns if passed in TREE node is a boolean expression */
int is_boolean_expression_node(TREE *node) {
    if(node == NULL) {
        return 0;
    }

    switch(node->type) {
        case S_EQ:
        case S_NEQ:
        case S_GE:
        case S_LE:
        case S_GT:
        case S_LT:
        case S_AND:
        case S_OR:
            return 1;
    }
    return 0;
}


/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
TREE * create_expression(CSOUND *csound, TREE *root) {
    char *op, arg1, arg2, c, *outarg = NULL;

    TREE *anchor = NULL, *last;

    /* HANDLE SUB EXPRESSIONS */

    if(is_expression_node(root->left)) {
        anchor = create_expression(csound, root->left);

        /* TODO - Free memory of old left node
           freetree */

        last = anchor;

        while(last->next != NULL) {
            last = last->next;
        }

        root->left = create_ans_token(csound, last->left->value->lexeme);

    }

    if(is_expression_node(root->right)) {
        TREE * newRight = create_expression(csound, root->right);


        if(anchor == NULL) {
            anchor = newRight;
        } else {
            last = anchor;

            while(last->next != NULL) {
                last = last->next;
            }

            last->next = newRight;
        }

        last = newRight;

        while(last->next != NULL) {
            last = last->next;
        }

        /* TODO - Free memory of old right node
           freetree */

        root->right = create_ans_token(csound, last->left->value->lexeme);
    }


    op = mcalloc(csound, 80);

    arg1 = '\0';
    if(root->left != NULL) {
        arg1 = argtyp2(csound, root->left->value->lexeme);
    }
    arg2 = argtyp2(csound, root->right->value->lexeme);

    switch(root->type) {
        case S_PLUS:
            strcpy(op, "add");
            outarg = set_expression_type(csound, op, arg1, arg2);
            break;
        case S_MINUS:
            strcpy(op, "sub");
            outarg = set_expression_type(csound, op, arg1, arg2);
            break;
        case S_TIMES:
            strcpy(op, "mul");
            outarg = set_expression_type(csound, op, arg1, arg2);
            break;
        case S_DIV:
            strcpy(op, "div");
            outarg = set_expression_type(csound, op, arg1, arg2);
            break;
        case T_FUNCTION: /* assumes on single arg input */
            c = arg2;

            if (c == 'p' || c == 'c')   c = 'i';

            sprintf(op, "%s.%c", root->value->lexeme, c);

            csound->Message(csound, "Found OP: %s\n", op);

            outarg = create_out_arg(csound, c);
            break;
        case S_UMINUS:
            csound->Message(csound, "HANDLING UNARY MINUS!");
            root->left = create_minus_token(csound);
            arg1 = 'k';
            strcpy(op, "mul");
            outarg = set_expression_type(csound, op, arg1, arg2);
            break;
    }

    TREE * opTree = create_opcode_token(csound, op);

    if(root->left != NULL) {
        opTree->right = root->left;
        opTree->right->next = root->right;
        opTree->left = create_ans_token(csound, outarg);
    } else {
        opTree->right = root->right;
        opTree->left = create_ans_token(csound, outarg);
    }

    if(anchor == NULL) {
        anchor = opTree;
    } else {
        last = anchor;

        while(last->next != NULL) {
            last = last->next;
        }

        last->next = opTree;
    }

    mfree(csound, op);

    return anchor;
}

/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
TREE * create_boolean_expression(CSOUND *csound, TREE *root) {
    /* csound->Message(csound, "Creating boolean expression\n"); */
    char *op, *outarg;

    TREE *anchor = NULL, *last;

    /* HANDLE SUB EXPRESSIONS */

    if(is_boolean_expression_node(root->left)) {
        anchor = create_boolean_expression(csound, root->left);

        /* TODO - Free memory of old left node
           freetree */

        root->left = create_ans_token(csound, anchor->left->value->lexeme);

    }

    if(is_boolean_expression_node(root->right)) {
        TREE * newRight = create_boolean_expression(csound, root->right);

        if(anchor == NULL) {
            anchor = newRight;
        } else {
            last = anchor;

            while(last->next != NULL) {
                last = last->next;
            }

            last->next = newRight;
        }

        /* TODO - Free memory of old right node
           freetree */

        root->right = create_ans_token(csound, newRight->left->value->lexeme);
    }


    op = mcalloc(csound, 80);

    switch(root->type) {
        case S_EQ:
            strcpy(op, "==");
            break;
        case S_NEQ:
            strcpy(op, "!=");
            break;
        case S_GE:
            strcpy(op, ">=");
            break;
        case S_LE:
            strcpy(op, "<=");
            break;
        case S_GT:
            strcpy(op, ">");
            break;
        case S_LT:
            strcpy(op, "<");
            break;
        case S_AND:
            strcpy(op, "&&");
            break;
        case S_OR:
            strcpy(op, "||");
            break;
    }

    csound->Message(csound, "Operator Found: %s\n", op);

    outarg = get_boolean_arg(csound);

    TREE * opTree = create_opcode_token(csound, op);

    opTree->right = root->left;
    opTree->right->next = root->right;
    opTree->left = create_ans_token(csound, outarg);

    if(anchor == NULL) {
        anchor = opTree;
    } else {
        last = anchor;

        while(last->next != NULL) {
            last = last->next;
        }

        last->next = opTree;
    }



    mfree(csound, op);

    return anchor;
}

static TREE *create_synthetic_label(CSOUND *csound, long count) {
	char *label = (char *)csound->Calloc(csound, 20);
	
	sprintf(label, "__synthetic_%ld", count);
	
	return make_leaf(csound, T_LABEL, make_label(csound, label));
}

/* Expands expression nodes into opcode calls */
TREE *csound_orc_expand_expressions(CSOUND * csound, TREE *root)
{
	long labelCounter = 300L; 
    
    TREE *anchor = NULL;
    TREE * expressionNodes = NULL;

    TREE *current = root;
    TREE *previous = NULL;

    csound->Message(csound, "[Begin Expanding Expressions in AST]\n");
    
    while(current != NULL) {
        switch(current->type) {
            case T_INSTR:
                csound->Message(csound, "Instrument found\n");

                current->right = csound_orc_expand_expressions(csound, current->right);

                break;
            case T_UDO:
                csound->Message(csound, "UDO found\n");

                break;

            case T_IF:

                csound->Message(csound, "Found IF statement\n");

                TREE * left = current->left;
                TREE * right = current->right;

                if(right->type == T_IGOTO ||
                   right->type == T_KGOTO ||
                   right->type == T_GOTO) {
                    csound->Message(csound, "Found if-goto\n");

                    expressionNodes = create_boolean_expression(csound, left);

                    print_tree(csound, expressionNodes);

                    /* Set as anchor if necessary */
                    if(anchor == NULL) {
                        anchor = expressionNodes;
                    }

                    /* reconnect into chain */
                    TREE* last = expressionNodes;

                    while(last->next != NULL) {
                        last = last->next;
                    }


                    if(previous != NULL) {
                        previous->next = expressionNodes;
                    }

                    TREE * gotoToken = create_goto_token(csound,
                        expressionNodes->left->value->lexeme, right);

                    gotoToken->next = current->next;

                    last->next = gotoToken;
                    current = gotoToken;
                    previous = last;
                } else if(right->type == T_THEN ||
                   right->type == T_ITHEN ||
                   right->type == T_KTHEN) {
                    csound->Message(csound, "Found if-then\n");

                    int endLabelCounter = -1;
                    
                    if(right->next != NULL) {
                    	endLabelCounter = labelCounter++; 
                    }
                    
                    TREE *newCurrent = NULL;
                    TREE *currentIfTree = current;
                    TREE *tempLeft;
                    TREE *tempRight;
                    
                    while(currentIfTree != NULL) {
                    
                    	tempLeft = currentIfTree->left;
                    	tempRight = currentIfTree->right;
                    	
                    	if(currentIfTree->type != T_ELSE) {
                    		expressionNodes = create_boolean_expression(csound, tempLeft);
                    	}
	
	                    print_tree(csound, expressionNodes);
	
	                    /* Set as anchor if necessary */
	                    if(anchor == NULL) {
	                        anchor = expressionNodes;
	                    }
	
	                    /* reconnect into chain */
	                    TREE* last = expressionNodes;
	
	                    while(last->next != NULL) {
	                        last = last->next;
	                    }
	
	
	                    if(previous != NULL) {
	                        previous->next = expressionNodes;
	                    }
	
	                    TREE *statements = tempRight->right;
	                    
	                    TREE *label = create_synthetic_label(csound, labelCounter);
	                    TREE *labelEnd = create_synthetic_label(csound, labelCounter++);
	                    
	                    tempRight->right = label; 
	                    
	                    TREE * gotoToken = create_goto_token(csound,
	                        expressionNodes->left->value->lexeme, tempRight);
	
	                    
	                    /* relinking */
	                    gotoToken->next = statements;
	                    
	                    while(statements->next != NULL) {
	                    	statements = statements->next;
	                    }
	                    
	                    if(endLabelCounter > 0) {
	                    	TREE *endLabel = create_synthetic_label(csound, 
	                    			endLabelCounter);
	                    	TREE *gotoEndLabelToken = create_simple_goto_token(csound,
	    	                        endLabel);
	                    	
	                    	statements->next = gotoEndLabelToken;
	                    	gotoEndLabelToken->next = labelEnd;
	                    	labelEnd->next = current->next;
	                    } else {
	                    	statements->next = labelEnd;
	                    	labelEnd->next = current->next;
	                    }
	
	                    last->next = gotoToken;
	                    
	                    
	                    if(newCurrent == NULL) {
	                    	newCurrent = gotoToken;
	                    }
	                    
	                    previous = last;
                                       
	                    currentIfTree = right->next;
                    }
                    
                    current = newCurrent;
                    
                    if(endLabelCounter > 0) {
                    	TREE *endLabel = create_synthetic_label(csound, 
                    		                    			endLabelCounter);
                    	
                    }
                    
                } else {
                    csound->Message(csound, "ERROR: Neither if-goto or if-then found!!!");
                }

                break;
            default:

                if(current->right != NULL) {
                    csound->Message(csound, "Found Statement.\n");

                    if(current->type == S_ASSIGN) {
                        // perhaps do optimization here?
                        //csound->Message(csound, "Assignment Statement.\n");
                    }


                    TREE* previousArg = NULL;
                    TREE* currentArg = current->right;

                    while(currentArg != NULL) {
                        if(is_expression_node(currentArg)) {

                            csound->Message(csound, "Found Expression.\n");

                            expressionNodes = create_expression(csound, currentArg);

                            /* Set as anchor if necessary */
                            if(anchor == NULL) {
                                anchor = expressionNodes;
                            }

                            /* reconnect into chain */
                            TREE* last = expressionNodes;

                            while(last->next != NULL) {
                                last = last->next;
                            }

                            last->next = current;

                            if(previous == NULL) {
                                previous = last;
                            } else {
                                previous->next = expressionNodes;
                                previous = last;
                            }

                            char * newArg = last->left->value->lexeme;

                            csound->Message(csound, "New Arg: %s\n", newArg);

                            /* handle arg replacement of currentArg here */

                            TREE *nextArg = currentArg->next;
                            TREE *newArgTree = create_ans_token(csound, newArg);

                            /* print_tree(csound, newArgTree); */

                            if(previousArg == NULL) {
                                current->right = newArgTree;
                            } else {
                                previousArg->next = newArgTree;
                            }

                            newArgTree->next = nextArg;
                            currentArg = newArgTree;

                            /* TODO - Delete the expression nodes here */

                        }

                        previousArg = currentArg;
                        currentArg = currentArg->next;
                    }

                    handle_polymorphic_opcode(csound, current);
                    handle_optional_args(csound, current);
                }
        }

        if(anchor == NULL) {
            anchor = current;
        }

        previous = current;
        current = current->next;

    }

    csound->Message(csound, "[End Expanding Expressions in AST]\n");

    return anchor;
}

