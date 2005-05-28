#include <stdio.h>
#include "jsnd5.h"
#include "jsnd5.tab.h"

extern int debug;
extern FILE* csfile;
extern TOKEN *lookup_token(char *);

char* type2string(int);
void generate_list(TREE*);
void generate_expression(TREE*);
void generate_code(TREE *);
TREE *simplify(TREE*);
TREE* simplify_list(TREE *);

static int tmpbase = 0;

void lose(TREE *l)              /* Return tree to free space */
{
    while (l!=NULL) {
      TREE *tmp;
      lose(l->left);
      tmp = l->right;
      free(l);
      l = tmp;
    }
}

TREE *simplify(TREE* l)         /* Walk tree, compiling complex bits to temps */
{
    if (!complex_expression(l)) {
      return l;
    }
    switch (l->type) {
    case S_APPLY:
      {
        TREE *na;
        char buf[100];
        l->right = simplify_list(l->right); /* waste space?? */
        sprintf(buf, "t%d", tmpbase++);
        na = make_leaf(T_IDENT, lookup_token(buf)); /* Should be typed */
        fprintf(csfile, "\t%s\t%s\t",
                na->value->lexeme,
                type2string(l->left->type));
        generate_list(l->right);
        fprintf(csfile, "\n");
        return na;
      }
    case S_PLUS:
    case S_MINUS:
    case S_TIMES:
    case S_DIV:
      {
        TREE *la, *ra, *na;
        TREE *ans;
        int type = l->type;
        char buf[100];
        la = simplify(l->left);
        ra = simplify(l->right);
        free(l);
        sprintf(buf, "t%d", tmpbase++);
        na = make_leaf(T_IDENT, lookup_token(buf)); /* Should be typed */
        fprintf(csfile, "\t%s\t%s\t", na->value->lexeme, type2string(type));
        ans = make_node(S_COM, la, ra);
        generate_list(ans);
        fprintf(csfile, "\n");
        return na;
      }
    case S_UMINUS:
      {
        TREE *la, *na;
        int type = l->type;
        char buf[100];
        la = simplify(l->left);
        lose(l);
        sprintf(buf, "t%d", tmpbase++);
        na = make_leaf(T_IDENT, lookup_token(buf));
        fprintf(csfile, "\t%s\t=\t", na->value->lexeme);
        generate_expression(make_node(type, la, NULL));
        fprintf(csfile, "\n");
        return na;
      }
    default:
      fprintf(csfile, "???");
    }
    return l;
}

TREE* simplify_list(TREE* l)
{
    if (l==NULL) return l;
    if (l->type==S_COM) {
      l->left = simplify_list(l->left);
      l->right = simplify(l->right);
      return l;
    }
    return simplify(l);
}

int complex_expression(TREE* l)
{
    if (l==NULL) return 0;
    if (l->type==T_IDENT    || l->type==T_IDENT_I   || l->type==T_IDENT_GI ||
        l->type==T_IDENT_K  || l->type== T_IDENT_GK || l->type==T_IDENT_A  ||
        l->type==T_IDENT_GA || l->type==T_IDENT_W   || l->type==T_IDENT_GW ||
        l->type==T_IDENT_F  || l->type==T_IDENT_GF  || l->type==T_IDENT_P  ||
        l->type==T_IDENT_S  || l->type==T_IDENT_GS  ||
        l->type==T_NUMBER   || l->type==T_INTGR     || l->type==T_STRCONST )
      return 0;
    return 1;
}

int complex_expression_list(TREE* l)
{
    if (l==NULL) return 0;
    if (l->type==S_COM) {
      if (complex_expression_list(l->left)) return 1;
      else return complex_expression(l->right);
    }
    return complex_expression(l);
}

void generate_expression(TREE* l)
{
    if (l==NULL) return;
    if (l->type==T_INTGR) {
      fprintf(csfile, "%d", l->value->value);
    }
    else if (l->type==T_NUMBER) {
      fprintf(csfile, "%f", l->value->fvalue);
    }
    else if (l->type==T_STRCONST) {
      fprintf(csfile, "\"%s\"", l->value->lexeme);
    }
    else if (l->type==T_IDENT    || l->type==T_IDENT_I   || l->type==T_IDENT_GI ||
             l->type==T_IDENT_K  || l->type== T_IDENT_GK || l->type==T_IDENT_A  ||
             l->type==T_IDENT_GA || l->type==T_IDENT_W   || l->type==T_IDENT_GW ||
             l->type==T_IDENT_S  || l->type== T_IDENT_GS ||
             l->type==T_IDENT_F  || l->type==T_IDENT_GF  || l->type==T_IDENT_P) {
      fprintf(csfile, "%s", l->value->lexeme);
    }
    else if (l->type==S_APPLY) {
      fprintf(csfile, "%s(", type2string(l->left->type));
      generate_list(l->right);
      fprintf(csfile, ")");
    }
    /* This is really not needed but for now... */
    else if (l->type==S_PLUS || l->type==S_MINUS ||
             l->type==S_TIMES || l->type==S_DIV) {
      fprintf(csfile, "(");
      generate_expression(l->left);
      fprintf(csfile, "%s", type2string(l->type));
      generate_expression(l->right);
      fprintf(csfile, ")");
    }
    else if (l->type==S_UMINUS) {
      fprintf(csfile, "(-");
      generate_expression(l->left);
      fprintf(csfile, ")");
    }
    else {
      fprintf(csfile, "???");
    }
}

void generate_list(TREE* l)
{
    if (l==NULL) return;
    if (l->type==S_COM) {
      generate_list(l->left);
      fprintf(csfile, ", ");
      generate_expression(l->right);
    }
    else generate_expression(l);
}

char* type2string(int n)
{
    static char buffer[100];
    switch(n) {
    case S_COM:
      return ", ";
    case S_Q:
      return "?";
    case S_COL:
      return ":";
    case S_NOT:
      return "~";
    case S_PLUS:
      return "add";
    case S_MINUS:
      return "sub";
    case S_TIMES:
      return "mult";
    case S_DIV:
      return "div";
    case S_NL:
      return "\n";
    case S_LB:
      return "S_LB";
    case S_RB:
      return "S_RB";
    case S_NEQ:
      return "!=";
    case S_AND:
      return "&&";
    case S_OR:
      return "||";
    case S_LT:
      return "<";
    case S_LE:
      return "<=";
    case S_EQ:
      return "==";
    case S_ASSIGN:
      return "=";
    case S_GT:
      return ">";
    case S_GE:
      return ">=";
    case T_IF:
      return " if";
    case T_ABS:
      return "abs";
    case T_AMPDB:
      return "ampdb";
    case T_AMPDBFS:
      return "ampdbfs";
    case T_BIRND:
      return "birnd";
    case T_COS:
      return "cos";
    case T_COSH:
      return "cosh";
    case T_COSINV:
      return "cosinv";
    case T_CPS2PCH:
      return "cps2pch";
    case T_CPSOCT:
      return "cpsoct";
    case T_CPSPCH:
      return "cpspch";
    case T_DB:
      return "db";
    case T_DBAMP:
      return "dbamp";
    case T_DBFSAMP:
      return "dbfsamp";
    case T_EXP:
      return "exp";
    case T_FILELEN:
      return "filelen";
    case T_FILENCHNLS:
      return "filenchnls";
    case T_FILESR:
      return "filesr";
    case T_I:
      return "i";
    case T_INT:
      return "int";
    case T_LOG:
      return "log";
    case T_LOG10:
      return "log10";
    case T_OCTCPS:
      return "octcps";
    case T_OCTPCH:
      return "octpch";
    case T_P:
      return "p";
    case T_RND:
      return "rnd";
    case T_RND31:
      return "rnd31";
    case T_SIN:
      return "sin";
    case T_SINH:
      return "sinh";
    case T_SININV:
      return "sininv";
    case T_SQRT:
      return "sqrt";
    case T_TABLENG:
      return "tableng";
    case T_TAN:
      return "tan";
    case T_TANH:
      return "tanh";
    case T_TANINV:
      return "taninv";
    case T_SRATE:
      return "sr";
    case T_KRATE:
      return "kr";
    case T_KSMPS:
      return "ksmps";
    case T_NCHNLS:
      return "nchnls";
    case T_OPCODE0:
      return "OP0";
    case T_OPCODE:
      return "OP";
    case T_STRSET:
      return "strset";
    case T_PSET:
      return "pset";
    case T_CTRLINIT:
      return "ctrlinit";
    case T_MASSIGN:
      return "massign";
    case T_TURNON:
      return "turnon";
    case T_PREALLOC:
      return "prealloc";
    case T_ZAKINIT:
      return "zakinit";
    case T_FTGEN:
      return "ftgen";
    case T_INIT:
      return "init";
    }
    return "???";
}

void generate_code(TREE *l)
{
    if (l==NULL) return;
    if (l->type==S_ANDTHEN) {
      generate_code(l->left);
      generate_code(l->right);
      return;
    }
    if (l->type==S_ASSIGN) {
      if (complex_expression(l->right)) {
        TREE *mod = NULL;
        tmpbase = 0;
        mod = simplify(l->right);
        fprintf(csfile, "\t%s\t=\t", l->left->value->lexeme);
        generate_expression(mod);
      }
      else {
        fprintf(csfile, "\t%s\t=\t", l->left->value->lexeme);
        generate_expression(l->right);
      }
      fprintf(csfile, "\n");
      return;
    }
    /* for now assume an opcode */
    if (complex_expression_list(l->right)) {
      TREE *mod;
      tmpbase = 0;
      mod = simplify_list(l->right);
      fprintf(csfile, "\t");
      if (l->left) generate_list(l->left);
      fprintf(csfile, "\t%s\t", l->value->lexeme);
      generate_list(mod);
      fprintf(csfile, "\n");
    }
    else {
      fprintf(csfile, "\t");
      if (l->left) generate_list(l->left);
      fprintf(csfile, "\t%s\t", l->value->lexeme);
      generate_list(l->right);
      fprintf(csfile, "\n");
    }
}
