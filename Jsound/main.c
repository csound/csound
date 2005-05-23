#include <stdio.h>
#include "jsnd.h"
#include "jsnd.yacc.tab.h"

void yyparse(void);
extern int yydebug;
int debug = 0;
int noheader;

extern char* type2string(int);
static void print_tree_i(TREE*, int);
static void process_init(TREE*);

double sr = -1.0, kr = -1.0;
int ksmps = -1, nchnls = -1;
TREE* statement_list;
FILE* csfile;
TREE* init_list;

int main(int argc, char **argv)
{
    int n;
    char *name = "out.orc";
    noheader = 1;
    init_symbtab();
    if (argc>1) {
      for (n=1; n<argc; n++) {
        if (argv[n][0]=='-') {
          if (argv[n][1]=='d') yydebug = 1;
          else {}
        }
        else name = argv[n];
      }
    }
    init_list = NULL;           /* For instrument 0 stuff */
    printf("JSND....\n");
    if (strcmp(name, "stdout")==0) csfile = stdout;
    else csfile = fopen(name, "w");
    yyparse();
    process_init(init_list);
    printf(".......done\n");
    fclose(csfile);
}

int yywrap(void)
{
    printf("END OF INPUT\n");
    return (1);
}

void yyerror(char *str)
{
    printf("%s\n", str);
}

TREE* make_node(int type, TREE* left, TREE* right)
{
  TREE *ans;
  ans = (TREE*)malloc(sizeof(TREE));
  ans->type = type;
  ans->left = left;
  ans->right = right;
  ans->len = 2;
  ans->rate = -1;
  return ans;
}

TREE* make_leaf(int type, TOKEN *v)
{
  TREE *ans;
  ans = (TREE*)malloc(sizeof(TREE));
  ans->type = type;
  ans->left = NULL;
  ans->right = NULL;
  ans->len = 0;
  ans->rate = -1;
  ans->value = v;
  return ans;
}

/* Add a new operation to  instr 0 */
void instr0(TOKEN *type, TREE* ans, TREE* args)
{
    printf("Instr 0 opcode: %s\n", type2string(type->type));
    print_tree_i(ans, 3);
    print_tree_i(args, 3);
    init_list = make_node(S_ANDTHEN, init_list, make_node(type, ans, args));
    printf("\n");
}

/* Process the instr0 */
static void process_init(TREE *l)
{
}

static void print_tree_i(TREE* l, int n)
{
    int i;
    if (l==NULL) return;
    for (i=0; i<n; i++) putchar(' ');
    switch (l->type) {
    case S_COM:
      printf("S_COM:\n"); break;
    case S_Q:
      printf("S_Q:\n"); break;
    case S_COL:
      printf("S_COL:\n"); break;
    case S_NOT:
      printf("S_NOT:\n"); break;
    case S_PLUS:
      printf("S_PLUS:\n"); break;
    case S_MINUS:
      printf("S_MINUS:\n"); break;
    case S_TIMES:
      printf("S_TIMES:\n"); break;
    case S_DIV:
      printf("S_DIV:\n"); break;
    case S_NL:
      printf("S_NL:\n"); break;
    case S_LB:
      printf("S_LB:\n"); break;
    case S_RB:
      printf("S_RB:\n"); break;
    case S_NEQ:
      printf("S_NEQ:\n"); break;
    case S_AND:
      printf("S_AND:\n"); break;
    case S_OR:
      printf("S_OR:\n"); break;
    case S_LT:
      printf("S_LT:\n"); break;
    case S_LE:
      printf("S_LE:\n"); break;
    case S_EQ:
      printf("S_EQ:\n"); break;
    case S_ASSIGN:
      printf("S_ASSIGN:\n"); break;
    case S_GT:
      printf("S_GT:\n"); break;
    case S_GE:
      printf("S_GE:\n"); break;
    case T_IF:
      printf("T_IF:\n"); break;
    case T_ABS:
      printf("T_ABS:\n"); break;
    case T_AMPDB:
      printf("T_AMPDB:\n"); break;
    case T_AMPDBFS:
      printf("T_AMPDBFS:\n"); break;
    case T_BIRND:
      printf("T_BIRND:\n"); break;
    case T_COS:
      printf("T_COS:\n"); break;
    case T_COSH:
      printf("T_COSH:\n"); break;
    case T_COSINV:
      printf("T_COSINV:\n"); break;
    case T_CPS2PCH:
      printf("T_CPS2PCH:\n"); break;
    case T_CPSOCT:
      printf("T_CPSOCT:\n"); break;
    case T_CPSPCH:
      printf("T_CPSPCH:\n"); break;
    case T_DB:
      printf("T_DB:\n"); break;
    case T_DBAMP:
      printf("T_DBAMP:\n"); break;
    case T_DBFSAMP:
      printf("T_DBFSAMP:\n"); break;
    case T_EXP:
      printf("T_EXP:\n"); break;
    case T_FILELEN:
      printf("T_FILELEN:\n"); break;
    case T_FILENCHNLS:
      printf("T_FILENCHNLS:\n"); break;
    case T_FILESR:
      printf("T_FILESR:\n"); break;
    case T_I:
      printf("T_I:\n"); break;
    case T_INT:
      printf("T_INT:\n"); break;
    case T_LOG:
      printf("T_LOG:\n"); break;
    case T_LOG10:
      printf("T_LOG10:\n"); break;
    case T_OCTCPS:
      printf("T_OCTCPS:\n"); break;
    case T_OCTPCH:
      printf("T_OCTPCH:\n"); break;
    case T_P:
      printf("T_P:\n"); break;
    case T_RND:
      printf("T_RND:\n"); break;
    case T_RND31:
      printf("T_RND31:\n"); break;
    case T_SIN:
      printf("T_SIN:\n"); break;
    case T_SINH:
      printf("T_SINH:\n"); break;
    case T_SININV:
      printf("T_SININV:\n"); break;
    case T_SQRT:
      printf("T_SQRT:\n"); break;
    case T_TABLENG:
      printf("T_TABLENG:\n"); break;
    case T_TAN:
      printf("T_TAN:\n"); break;
    case T_TANH:
      printf("T_TANH:\n"); break;
    case T_TANINV:
      printf("T_TANINV:\n"); break;
    case T_SRATE:
      printf("T_SRATE:\n"); break;
    case T_KRATE:
      printf("T_KRATE:\n"); break;
    case T_KSMPS:
      printf("T_KSMPS:\n"); break;
    case T_NCHNLS:
      printf("T_NCHNLS:\n"); break;
    case T_STRCONST:
      printf("T_STRCONST: %s\n", l->value->lexeme); return;
    case T_IDENT:
      printf("T_IDENT: %s\n", l->value->lexeme); return;
    case T_IDENT_I:
      printf("IDENT_I: %s\n", l->value->lexeme); return;
    case T_IDENT_GI:
      printf("IDENT_GI: %s\n", l->value->lexeme); return;
    case T_IDENT_K:
      printf("IDENT_K: %s\n", l->value->lexeme); return;
    case T_IDENT_GK:
      printf("IDENT_GK: %s\n", l->value->lexeme); return;
    case T_IDENT_A:
      printf("IDENT_A: %s\n", l->value->lexeme); return;
    case T_IDENT_GA:
      printf("IDENT_GA: %s\n", l->value->lexeme); return;
    case T_IDENT_W:
      printf("IDENT_W: %s\n", l->value->lexeme); return;
    case T_IDENT_GW:
      printf("IDENT_GW: %s\n", l->value->lexeme); return;
    case T_IDENT_F:
      printf("IDENT_F: %s\n", l->value->lexeme); return;
    case T_IDENT_GF:
      printf("IDENT_GF: %s\n", l->value->lexeme); return;
    case T_IDENT_P:
      printf("IDENT_P: %s\n", l->value->lexeme); return;
    case T_IDENT_S:
      printf("IDENT_S: %s\n", l->value->lexeme); return;
    case T_IDENT_GS:
      printf("IDENT_GS: %s\n", l->value->lexeme); return;
    case T_INTGR:
      printf("T_INTGR: %d\n", l->value->value); return;
    case T_NUMBER:
      printf("T_NUMBER: %f\n", l->value->fvalue); return;
    case S_ANDTHEN:
      printf("S_ANDTHEN:\n"); break;
    case S_APPLY:
      printf("S_APPLY:\n"); break; 
    case T_OPCODE0:
      printf("T_OPCODE0:%s\n", l->value->lexeme); return;
    case T_OPCODE:
      printf("T_OPCODE:%s\n", l->value->lexeme); return;
    default:
      printf("t:%d\n", l->type);
    }
    print_tree_i(l->left,n+1);
    print_tree_i(l->right,n+1);
}

void print_tree(TREE *l)
{
    printf("Printing Tree\n");
    print_tree_i(l, 0);
}

TREE* optimise_tree(TREE *l)
{
    return l;
}

void generate_header(void)
{
    if (sr==-1.0) {
      if (kr==-1.0) {
        if (ksmps==-1) sr = 44100, kr = 441, ksmps = 100;
        else sr = 44100, kr = sr/ksmps;
      }
      else if (ksmps==-1) {
        ksmps = 100; sr = ksmps*kr;
      }
      else sr = kr*ksmps;
    }
    else if (kr==-1.0) {
      if (ksmps==-1) kr = 441, ksmps = 100;
      else kr = sr/ksmps;
    }
    else if (ksmps==-1) ksmps = (int)(sr/kr +0.5);
    if (nchnls==-1) nchnls = 2;
    fprintf(csfile,
            "\tsr\t=\t%f\n\tkr\t=\t%f\n\tksmps\t=\t%d\n\tnchnls\t=\t%d\n\n",
            sr, kr, ksmps, nchnls);
    noheader = 0;
}

void start_instr(int n)
{
    printf("Start Instrument %d\n", n);
    statement_list = NULL;
    if (noheader) generate_header();
    fprintf(csfile, "\ninstr %d\n", n);
}

void end_instr(void)
{
    printf("End Instr\n");
/*     print_tree(statement_list); */
    statement_list = optimise_tree(statement_list);
    generate_code(statement_list);
    fprintf(csfile, "\nendin\n");
}
