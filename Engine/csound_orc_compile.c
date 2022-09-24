/*
  csound_orc_compile.c:
  (Based on otran.c)

  Copyright (C) 1991, 1997, 2003, 2006, 2012
  Barry Vercoe, John ffitch, Steven Yi, Victor Lazzarini

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include "csoundCore.h"
#include "csound_orc.h"
#include "parse_param.h"
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "insert.h"
#include "oload.h"
#include "pstream.h"
//#include "typetabl.h"
#include "csound_orc_semantics.h"
#include "csound_standard_types.h"

MYFLT csoundInitialiseIO(CSOUND *csound);
void    iotranset(CSOUND *), sfclosein(CSOUND*), sfcloseout(CSOUND*);
static const char *INSTR_NAME_FIRST = "::^inm_first^::";
static ARG *createArg(CSOUND *csound, INSTRTXT *ip, char *s,
                      ENGINE_STATE *engineState);
static void insprep(CSOUND *, INSTRTXT *, ENGINE_STATE *engineState);
static void lgbuild(CSOUND *, INSTRTXT *, char *, int inarg,
                    ENGINE_STATE *engineState);
int pnum(char *s);
static void unquote_string(char *, const char *);
void print_tree(CSOUND *, char *, TREE *);
void close_instrument(CSOUND *csound, ENGINE_STATE *engineState, INSTRTXT *ip);
char argtyp2(char *s);
void debugPrintCsound(CSOUND *csound);

void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState);
int named_instr_alloc(CSOUND *csound, char *s, INSTRTXT *ip, int32 insno,
                      ENGINE_STATE *engineState, int merge);
int check_instr_name(char *s);
void free_instr_var_memory(CSOUND *, INSDS *);
void mergeState_enqueue(CSOUND *csound, ENGINE_STATE *e, TYPE_TABLE *t,
                        OPDS *ids);

extern const char *SYNTHESIZED_ARG;

#ifdef FLOAT_COMPARE
#undef FLOAT_COMPARE
#endif
#ifdef USE_DOUBLE
#define FLOAT_COMPARE(x, y) (fabs((double)(x) / (double)(y)-1.0) > 1.0e-12)
#else
#define FLOAT_COMPARE(x, y) (fabs((double)(x) / (double)(y)-1.0) > 5.0e-7)
#endif
/* ------------------------------------------------------------------------ */

char *strsav_string(CSOUND *csound, ENGINE_STATE *engineState, char *key) {
    char *retVal =
      cs_hash_table_get_key(csound, csound->engineState.stringPool, key);

    if (retVal == NULL) {
      // printf("strsav_string: %s\n", key);
      retVal = cs_hash_table_put_key(csound, engineState->stringPool, key);
    }
    return retVal;
}

int pnum(char *s) /* check a char string for pnum format  */
/*   and return the pnum ( >= 0 )       */
{ /* else return -1                       */
    int n;

    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return (n);
    return (-1);
}

#if 0
static int argCount(ARG* arg)
{
    int retVal = -1;
    if (arg != NULL) {
      retVal = 0;
      while (arg != NULL) {
        arg = arg->next;
        retVal++;
      }
    }
    return retVal;
}
#endif

/* get size of string in MYFLT units */
/*static inline int strlen_to_samples(const char *s)
  {
  int n = (int) strlen(s);
  n = (n + (int) sizeof(MYFLT)) / (int) sizeof(MYFLT);
  return n;
  }
*/

/* convert string constant */
void unquote_string(char *dst, const char *src) {
  int i, j, n = (int)strlen(src) - 1;
  for (i = 1, j = 0; i < n; i++) {
    // printf("char - %c\n", src[i]);
    if (src[i] != '\\')
      dst[j++] = src[i];
    else {
      // printf("char-- - %c\n", src[i]);
      switch (src[++i]) {

      case 'a':
        dst[j++] = '\a';
        break;
      case 'b':
        dst[j++] = '\b';
        break;
      case 'f':
        dst[j++] = '\f';
        break;
      case 'n':
        dst[j++] = '\n';
        break;
      case 'r':
        dst[j++] = '\r';
        break;
      case 't':
        dst[j++] = '\t';
        break;
      case 'v':
        dst[j++] = '\v';
        break;
      case '"':
        dst[j++] = '"';
        break;
      case '\\':
        dst[j++] = '\\'; /*printf("char-- + %c\n", src[i]);*/
        break;

      default:
        // printf("char-- ++ %c\n", src[i]);
        if (src[i] >= '0' && src[i] <= '7') {
          int k = 0, l = (int)src[i] - '0';
          while (++k < 3 && src[i + 1] >= '0' && src[i + 1] <= '7')
            l = (l << 3) | ((int)src[++i] - '0');
          dst[j++] = (char)l;
        } else {
          dst[j++] = '\\';
          i--;
        }
      }
    }
  }
  dst[j] = '\0';
}

int tree_arg_list_count(TREE *root) {
  int count = 0;
  TREE *current = root;

  while (current != NULL) {
    current = current->next;
    count++;
  }
  return count;
}

/**
 * Returns last OPTXT of OPTXT chain optxt
 */
static OPTXT *last_optxt(OPTXT *optxt) {
  OPTXT *current = optxt;

  while (current->nxtop != NULL) {
    current = current->nxtop;
  }
  return current;
}

/**
 * Append OPTXT op2 to end of OPTXT chain op1
 */
/*
  static inline void append_optxt(OPTXT *op1, OPTXT *op2)
  {
  last_optxt(op1)->nxtop = op2;
  }
*/

/** Counts number of args in argString, taking into account array identifiers */
int argsRequired(char *argString) {
  int retVal = 0;
  char *t = argString;

  if (t != NULL) {
    while (*t != '\0') {
      retVal++;
      t++;
      while (*t == '[') {
        t++;
        if (*t != ']') {
          // ERROR HERE, unmatched array identifier, perhaps should report...
          return -1;
        }
        t++;
      }
    }
  }
  return retVal;
}

/** Splits args in argString into char**, taking into account array identifiers
 */
char **splitArgs(CSOUND *csound, char *argString) {
  int argCount = argsRequired(argString);
  char **args = csound->Malloc(csound, sizeof(char *) * (argCount + 1));
  // printf("alloc %p\n", args);
  char *t = argString;
  int i = 0;

  if (t != NULL) {
    while (*t != '\0') {
      char *part;
      int dimensions = 0;

      if (*(t + 1) == '[') {
        char *start = t;
        int len = 1;
        int j;
        t++;

        while (*t == '[') {
          t++;
          len++;

          if (UNLIKELY(*t != ']')) {
            // FIXME: needs more precise error information
            csound->Message(csound,
                            Str("ERROR: Unmatched bracket found in array"
                                "argument type specification\n"));
            return NULL;
          }

          t++;
          len++;
          dimensions++;
        }
        part = csound->Malloc(csound, sizeof(char) * (dimensions + 3));
        // printf("alloc %p\n", part);
        part[dimensions + 2] = '\0';
        part[dimensions + 1] = ']';
        part[dimensions] = *start;
        for (j = 0; j < dimensions; j++) {
          part[j] = '[';
        }

      } else {
        part = csound->Malloc(csound, sizeof(char) * 2);
        // printf("alloc %p\n", part);
        part[0] = *t;
        part[1] = '\0';
        t++;
      }
      args[i] = part;
      i++;
    }
  }

  args[argCount] = NULL;

  return args;
}

OENTRY *find_opcode(CSOUND *, char *);
/**
 * Create an Opcode (OPTXT) from the AST node given for a given engineState
 */
OPTXT *create_opcode(CSOUND *csound, TREE *root, INSTRTXT *ip,
                     ENGINE_STATE *engineState) {
  TEXT *tp;
  TREE *inargs, *outargs;
  OPTXT *optxt;
  char *arg;
  int n; // nreqd;
  optxt = (OPTXT *)csound->Calloc(csound, (int32)sizeof(OPTXT));
  tp = &(optxt->t);
  tp->linenum = root->line; tp->locn = root->locn;
  OENTRY *labelOpcode;

  switch (root->type) {
  case LABEL_TOKEN:
    labelOpcode = find_opcode(csound, "$label");
    /* TODO - Need to verify here or elsewhere that this label is not
       already defined */
    tp->oentry = labelOpcode;
    tp->opcod = strsav_string(csound, engineState, root->value->lexeme);

    tp->outlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
    tp->outlist->count = 0;
    tp->inlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
    tp->inlist->count = 0;

    // ip->mdepends |= labelOpcode->flags;
    ip->opdstot += labelOpcode->dsblksiz;

    break;
  case '=':
  case GOTO_TOKEN:
  case IGOTO_TOKEN:
  case KGOTO_TOKEN:
  case T_OPCODE:
  case T_OPCODE0:
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "create_opcode: Found node for opcode %s\n",
                      root->value->lexeme);

    // FIXME THIS RESULT IS NOT USED -- VL I don't think it's needed
    // nreqd = tree_arg_list_count(root->left);   /* outcount */
    /* replace opcode if needed */

    /* INITIAL SETUP */
    tp->oentry = (OENTRY *)root->markup;
    tp->opcod = strsav_string(csound, engineState, tp->oentry->opname);
    //tp->linenum = root->line; tp->locn = root->locn;
    // ip->mdepends |= tp->oentry->flags;
    ip->opdstot += tp->oentry->dsblksiz;

    /* BUILD ARG LISTS */
    {
      int incount = tree_arg_list_count(root->right);
      int outcount = tree_arg_list_count(root->left);
      int argcount = 0;
      size_t m = sizeof(ARGLST) + (incount - 1) * sizeof(char *);
      tp->inlist = (ARGLST *)csound->ReAlloc(csound, tp->inlist, m);
      tp->inlist->count = incount;

      m = sizeof(ARGLST) + (outcount - 1) * sizeof(char *);
      tp->outlist = (ARGLST *)csound->ReAlloc(csound, tp->outlist, m);
      tp->outlist->count = outcount;

      tp->inArgCount = 0;

      for (inargs = root->right; inargs != NULL; inargs = inargs->next) {
        /* INARGS */
        arg = inargs->value->lexeme;
        // printf("arg: %s\n", arg);
        tp->inlist->arg[argcount++] = strsav_string(csound, engineState, arg);

        if ((n = pnum(arg)) >= 0) {
          if (n > ip->pmax)
            ip->pmax = n;
        }
        /* VL 14/12/11 : calling lgbuild here seems to be problematic for
           undef arg checks */
        else {
          lgbuild(csound, ip, arg, 1, engineState);
        }
        if (inargs->markup != &SYNTHESIZED_ARG) {
          tp->inArgCount++;
        }
      }
    }
    /* VERIFY ARG LISTS MATCH OPCODE EXPECTED TYPES */
    {

      OENTRY *ep = tp->oentry;
      int argcount = 0;
      for (outargs = root->left; outargs != NULL; outargs = outargs->next) {
        arg = outargs->value->lexeme;
        tp->outlist->arg[argcount++] = strsav_string(csound, engineState, arg);
      }

      tp->outArgCount = 0;

      /* OUTARGS */
      for (outargs = root->left; outargs != NULL; outargs = outargs->next) {

        arg = outargs->value->lexeme;

        if ((n = pnum(arg)) >= 0) {
          if (n > ip->pmax)
            ip->pmax = n;
        } else {
          csound->DebugMsg(csound, "Arg: %s\n", arg);
          lgbuild(csound, ip, arg, 0, engineState);
        }
        tp->outArgCount++;
      }

      if (root->right != NULL) {
        if (ep->intypes[0] != 'l') { /* intype defined by 1st inarg */
          tp->intype = argtyp2(tp->inlist->arg[0]);
        } else {
          tp->intype = 'l'; /*   (unless label)  */
        }
      }

      if (root->left != NULL) { /* pftype defined by outarg */
        tp->pftype = argtyp2(root->left->value->lexeme);
      } else { /*    else by 1st inarg     */
        tp->pftype = tp->intype;
      }
    }
    break;
  default:
    csound->Message(csound, Str("create_opcode: No rule to handle statement of "
                                "type %d\n"),
                    root->type);
    if (PARSER_DEBUG)
      print_tree(csound, NULL, root);
  }

  return optxt;
}

/**
 * Add a global variable and allocate memory
 * Globals, unlike locals, keep their memory space
 * in separate blocks, pointed by var->memBlock
 */
void addGlobalVariable(CSOUND *csound, ENGINE_STATE *engineState, CS_TYPE *type,
                       char *name, void *typeArg) {
  CS_VARIABLE *var =
      csoundCreateVariable(csound, csound->typePool, type, name, typeArg);
  size_t memSize = CS_VAR_TYPE_OFFSET + var->memBlockSize;
  CS_VAR_MEM *varMem = csound->Malloc(csound, memSize);
  csoundAddVariable(csound, engineState->varPool, var);

  varMem->varType = var->varType;
  var->memBlock = varMem;
  if (var->initializeVariableMemory != NULL) {
    var->initializeVariableMemory((void *)csound, var, &varMem->value);
  }
}

void *find_or_add_constant(CSOUND *csound, CS_HASH_TABLE *constantsPool,
                           const char *name, MYFLT value) {
  void *retVal = cs_hash_table_get(csound, constantsPool, (char *)name);
  if (retVal == NULL) {
    CS_VAR_MEM *memValue = csound->Calloc(csound, sizeof(CS_VAR_MEM));
    memValue->varType = (CS_TYPE *)&CS_VAR_TYPE_C;
    memValue->value = value;
    cs_hash_table_put(csound, constantsPool, (char *)name, memValue);
    retVal = cs_hash_table_get(csound, constantsPool, (char *)name);
  }
  return retVal;
}

/**
 * NB - instr0 to be created only once, in the first compilation
 *  and stored in csound->instr0
 * Create an Instrument (INSTRTXT) from the AST node given for use as
 * Instrument0. Called from csound_orc_compile.
 */
INSTRTXT *create_instrument0(CSOUND *csound, TREE *root,
                             ENGINE_STATE *engineState, CS_VAR_POOL *varPool) {
  INSTRTXT *ip;
  OPTXT *op;
  TREE *current;
  MYFLT sr = FL(-1.0), kr = FL(-1.0), ksmps = FL(-1.0), nchnls = DFLT_NCHNLS,
        inchnls = -FL(1.0), _0dbfs = FL(-1.0);
  int krdef = 0; //, ksmpsdef = 0, srdef = 0;
  double A4 = 0.0;
  CS_TYPE *rType = (CS_TYPE *)&CS_VAR_TYPE_R;
  OPARMS *O = csound->oparms;

  addGlobalVariable(csound, engineState, rType, "sr", NULL);
  addGlobalVariable(csound, engineState, rType, "kr", NULL);
  addGlobalVariable(csound, engineState, rType, "ksmps", NULL);
  addGlobalVariable(csound, engineState, rType, "nchnls", NULL);
  addGlobalVariable(csound, engineState, rType, "nchnls_i", NULL);
  addGlobalVariable(csound, engineState, rType, "0dbfs", NULL);
  addGlobalVariable(csound, engineState, rType, "A4", NULL);
  addGlobalVariable(csound, engineState, rType, "$sr", NULL);
  addGlobalVariable(csound, engineState, rType, "$kr", NULL);
  addGlobalVariable(csound, engineState, rType, "$ksmps", NULL);

  find_or_add_constant(csound, engineState->constantsPool, "0", 0.0);

  ip = (INSTRTXT *)csound->Calloc(csound, sizeof(INSTRTXT));
  ip->varPool = varPool;
  op = (OPTXT *)ip;

  current = root;

  /* initialize */

  // ip->mdepends = 0;
  ip->opdstot = 0;
  ip->nocheckpcnt = 0;

  ip->pmax = 3L;

  /* start chain */
  ip->t.oentry = find_opcode(csound, "instr");
  /*  to hold global assigns */
  ip->t.opcod = strsav_string(csound, engineState, "instr");

  /* The following differs from otran and needs review.  otran keeps a
   * nulllist to point to for empty lists, while this is creating a new list
   * regardless
   */
  ip->t.outlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
  ip->t.outlist->count = 0;
  ip->t.inlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
  ip->t.inlist->count = 1;

  ip->t.inlist->arg[0] = strsav_string(csound, engineState, "0");

  while (current != NULL) {
    unsigned int uval;
    if (current->type != INSTR_TOKEN && current->type != UDO_TOKEN) {
      OENTRY *oentry = (OENTRY *)current->markup;
      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound, "In INSTR 0: %s\n", current->value->lexeme);

      if (current->type == '=' && strcmp(oentry->opname, "=.r") == 0) {

        // FIXME - perhaps should add check as it was in
        // constndx?  Not sure if necessary due to assumption
        // that tree will be verified
        MYFLT val = (MYFLT)cs_strtod(current->right->value->lexeme, NULL);
        // systems constants get set here and are not
        // compiled into i-time code
        find_or_add_constant(csound, csound->engineState.constantsPool,
                             (const char *)current->right->value->lexeme, val);

        /* modify otran defaults*/
        /* removed assignments to csound->tran_* */
        if (current->left->type == SRATE_TOKEN) {
          sr = val;
          // srdef = 1;
        } else if (current->left->type == KRATE_TOKEN) {
          kr = val;
          krdef = 1;
        } else if (current->left->type == KSMPS_TOKEN) {
          uval = (val <= 0 ? 1u : (unsigned int)val);
          ksmps = uval;
          // ksmpsdef = 1;
        } else if (current->left->type == NCHNLS_TOKEN) {
          uval = (val <= 0 ? 1u : (unsigned int)val);
          nchnls = uval;
        } else if (current->left->type == NCHNLSI_TOKEN) {
          uval = (val < 0 ? 1u : (unsigned int)val);
          inchnls = uval;
        } else if (current->left->type == ZERODBFS_TOKEN) {
          _0dbfs = val;
        } else if (current->left->type == A4_TOKEN) {
          A4 = val;
        }
      } else {
        op->nxtop = create_opcode(csound, current, ip, engineState);
        op = last_optxt(op);
      }
    }
    current = current->next;
  }

  /* Deal with defaults and consistency */
  if (ksmps == FL(-1.0)) {
    if (sr == FL(-1.0))
      sr = csound->oparms->sr_default;//   DFLT_SR;
    if (kr == FL(-1.0))
      kr = csound->oparms->kr_default;//  DFLT_KR;
    ksmps = (MYFLT)((int)(sr / kr + FL(0.5)));
    kr = sr / ksmps; /* VL - avoid inconsistency */
  } else if (kr == FL(-1.0)) {
    if (sr == FL(-1.0))
      sr = csound->oparms->sr_default;//  DFLT_SR;
    kr = sr / ksmps;
  } else if (sr == FL(-1.0)) {
    sr = kr * ksmps;
  }
  /* That deals with missing values, however we do need ksmps to be integer */
  {
    CSOUND *p = (CSOUND *)csound;
    char err_msg[128];
    CS_SPRINTF(err_msg, "sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:", sr, kr,
               ksmps);
    if (UNLIKELY(sr <= FL(0.0)))
      synterr(p, Str("%s invalid sample rate"), err_msg);
    if (UNLIKELY(kr <= FL(0.0)))
      synterr(p, Str("%s invalid control rate"), err_msg);
    if (UNLIKELY(ksmps <= FL(0.0)))
      synterr(p, Str("%s invalid number of samples"), err_msg);
    else if (UNLIKELY(ksmps < FL(0.75) ||
                      FLOAT_COMPARE(ksmps, MYFLT2LRND(ksmps)))) {
      /* VL 14/11/18: won't fail but correct values to make ksmps integral */
      csound->Warning(p, Str("%s invalid ksmps value, needs to be integral."),
                      err_msg);
      ksmps = floor(ksmps);
      kr = sr/ksmps;
      csound->Warning(p, "resetting orc parameters to: "
                 "sr = %.7g, kr = %.7g, ksmps = %.7g", sr, kr,
                 ksmps);
    }
    else if (UNLIKELY(FLOAT_COMPARE(sr, (double)kr * ksmps)
                      && !(O->ksmps_override || O->sr_override ||
                            O->kr_override)))
      synterr(p, Str("%s inconsistent sr, kr, ksmps\n"), err_msg);
    else if (UNLIKELY((ksmps > sr) && !(O->ksmps_override || O->sr_override)))
      synterr(p, Str("%s inconsistent sr, kr, ksmps\n"), err_msg);
  }

  csound->ksmps = ksmps;
  csound->nchnls = nchnls;
  if (inchnls < 0)
    csound->inchnls = nchnls;
  else
    csound->inchnls = inchnls;
  csound->esr = sr;
  csound->ekr = kr;
  if (_0dbfs < 0)
    csound->e0dbfs = DFLT_DBFS;
  else
    csound->e0dbfs = _0dbfs;
  if (A4 == 0)
    csound->A4 = 440.0;
  else {
    extern void csound_aops_init_tables(CSOUND *);
    csound->A4 = A4;
    csound_aops_init_tables(csound);
  }
  /* if (UNLIKELY(csound->e0dbfs <= FL(0.0))) {
    csound->Warning(csound, Str("bad value for 0dbfs: must be positive. "
                                "Setting default value."));
    csound->e0dbfs = DFLT_DBFS;
    }*/

  if (O->nchnls_override > 0)
    csound->nchnls = csound->inchnls = O->nchnls_override;
  if (O->nchnls_i_override > 0)
    csound->inchnls = O->nchnls_i_override;
  if (O->e0dbfs_override > 0)
    csound->e0dbfs = O->e0dbfs_override;

  // VL 01-05-2019
  // if --use-system-sr is applied, then we need to
  // initialise IO early to get the sampling rate
  // at this stage we have enough data on channels
  // to do this. Only applies to audio device output
  if(O->sr_override == -1.0 &&
     !strncmp(O->outfilename, "dac",3)) {
    MYFLT tmp_sr = csound->esr;
    csound->esr = -1.0;
    O->sr_override = csoundInitialiseIO(csound);

    if(O->sr_override > 0) {
      if(O->msglevel || O->odebug)
        csound->Message(csound,
                        Str("Using system sampling rate %.1f\n"), O->sr_override);
    }
    else {
      if(O->msglevel || O->odebug)
        csound->Message(csound, Str("System sr not available\n"));
      O->sr_override = FL(0.0);
    }
    csound->esr = tmp_sr;
  }

  if (UNLIKELY(O->odebug))
    csound->Message(csound, "esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d "
                            "0dbfs = %.1f\n",
                    csound->esr, csound->ekr, csound->ksmps, csound->nchnls,
                    csound->e0dbfs);

  if (O->sr_override || O->kr_override ||
      O->ksmps_override) { /* if command-line overrides, apply now */
    MYFLT ensmps;

    if (!O->ksmps_override) {
      csound->esr = (MYFLT)(O->sr_override ? O->sr_override : csound->esr);
      if (krdef) {
        csound->ekr = (MYFLT)(O->kr_override ? O->kr_override : csound->ekr);
        csound->ksmps =
            (int)((ensmps = ((MYFLT)csound->esr / (MYFLT)csound->ekr)) +
                  FL(0.5));
      } else {
        csound->ekr = csound->esr / csound->ksmps;
        ensmps = csound->ksmps;
      }
    } else {
      csound->ksmps = (ensmps = O->ksmps_override);
      if (O->sr_override) {
        csound->ekr = O->sr_override / csound->ksmps;
        csound->esr = csound->ekr * ensmps; // force SR
      } else if (O->kr_override) {
        csound->esr = O->kr_override * csound->ksmps;
        csound->ekr = O->kr_override;
      } else {
        csound->ekr = csound->esr / csound->ksmps;
      }
    }

    /* chk consistency one more time */
    {
      char s[256];
      CS_SPRINTF(s, Str("sr = %.7g, kr = %.7g, ksmps = %.7g\n"),
                 csound->esr, csound->ekr, ensmps);
      if (UNLIKELY(csound->ksmps < 1 || FLOAT_COMPARE(ensmps, csound->ksmps))) {
        /* VL 14/11/18: won't fail but correct values to make ksmps integral */
        csound->Warning(csound,
                        Str("%s invalid ksmps value, needs to be integral."), s);
        ensmps = csound->ksmps = floor(ensmps);
        csound->ekr  = csound->esr/csound->ksmps;
        csound->Warning(csound, Str("resetting orc parameters to: "
                                    "sr = %.7g, kr = %.7g, ksmps = %u"),
                        csound->esr, csound->ekr, csound->ksmps);
      }
      if (UNLIKELY(csound->esr <= FL(0.0)))
        csoundDie(csound, Str("%s invalid sample rate"), s);
      if (UNLIKELY(csound->ekr <= FL(0.0)))
        csoundDie(csound, Str("%s invalid control rate"), s);
      if (UNLIKELY(FLOAT_COMPARE(csound->esr, (double)csound->ekr * ensmps)))
        csoundDie(csound, Str("%s inconsistent sr, kr, ksmps"), s);
    }
    if(O->odebug)
    csound->Message(csound, Str("sample rate overrides: "
                                "esr = %7.4f, ekr = %7.4f, ksmps = %d\n"),
                    csound->esr, csound->ekr, csound->ksmps);
  }

  csound->tpidsr = TWOPI_F / csound->esr; /* now set internal  */
  csound->mtpdsr = -(csound->tpidsr);     /*    consts         */
  csound->pidsr = PI_F / csound->esr;
  csound->mpidsr = -(csound->pidsr);
  csound->onedksmps = FL(1.0) / (MYFLT)csound->ksmps;
  csound->sicvt = FMAXLEN / csound->esr;
  csound->kicvt = FMAXLEN / csound->ekr;
  csound->onedsr = FL(1.0) / csound->esr;
  csound->onedkr = FL(1.0) / csound->ekr;
  csound->global_kcounter = csound->kcounter;

  if (csound->ksmps != DFLT_KSMPS) {
    reallocateVarPoolMemory(csound, engineState->varPool);
    // csound->Message(csound, "recalculate varpool\n");
  }
  close_instrument(csound, engineState, ip);

  csound->inZero = 0;
  return ip;
}

/**
   This global instrument replaces instr 0 in
   subsequent compilations. It does not allow the
   setting of system parameters such as ksmps etc,
   but it allows i-time code to be compiled and run.
**/
INSTRTXT *create_global_instrument(CSOUND *csound, TREE *root,
                                   ENGINE_STATE *engineState,
                                   CS_VAR_POOL *varPool) {
  INSTRTXT *ip;
  OPTXT *op;
  TREE *current;

  //csound->inZero = 1;
  find_or_add_constant(csound, engineState->constantsPool, "0", 0);

  ip = (INSTRTXT *)csound->Calloc(csound, sizeof(INSTRTXT));
  ip->varPool = varPool;
  op = (OPTXT *)ip;

  current = root;

  /* initialize */
  // ip->mdepends = 0;
  ip->opdstot = 0;
  ip->pmax = 3L;

  /* start chain */
  ip->t.oentry = find_opcode(csound, "instr");
  /*  to hold global assigns */
  ip->t.opcod = strsav_string(csound, engineState, "instr");

  /* The following differs from otran and needs review.  otran keeps a
   * nulllist to point to for empty lists, while this is creating a new list
   * regardless
   */
  ip->t.outlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
  ip->t.outlist->count = 0;
  ip->t.inlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
  ip->t.inlist->count = 1;

  ip->t.inlist->arg[0] = strsav_string(csound, engineState, "0");

  while (current != NULL) {
    if (current->type != INSTR_TOKEN && current->type != UDO_TOKEN) {
      OENTRY *oentry = (OENTRY *)current->markup;
      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound, "In INSTR GLOBAL: %s\n",
                        current->value->lexeme);
      if (UNLIKELY(current->type == '=' && strcmp(oentry->opname, "=.r") == 0))
        csound->Warning(csound, Str("system constants can only be set once\n"));
      else {
        op->nxtop = create_opcode(csound, current, ip, engineState);
        op = last_optxt(op);
      }
    }
    current = current->next;
  }

  close_instrument(csound, engineState, ip);
  //csound->inZero = 0;
  return ip;
}

int tree_contains_fn_p(CSOUND *csound, TREE *t) {
  // print_tree(csound, "\ntree_contains_fn_p", t);
  while (t != NULL) {
    if (t->type == T_OPCODE && strcmp(t->value->lexeme, "p") == 0)
      return 1;
    if (t->left && tree_contains_fn_p(csound, t->left))
      return 1;
    if (t->right && tree_contains_fn_p(csound, t->right))
      return 1;
    t = t->next;
  }
  return 0;
}

/**
 * Create an Instrument (INSTRTXT) from the AST node given. Called from
 * csound_orc_compile.
 */
INSTRTXT *create_instrument(CSOUND *csound, TREE *root,
                            ENGINE_STATE *engineState) {
  INSTRTXT *ip;
  OPTXT *op;
  char *c;
  TREE *statements, *current;

  ip = (INSTRTXT *)csound->Calloc(csound, sizeof(INSTRTXT));
  ip->varPool = (CS_VAR_POOL *)root->markup;
  op = (OPTXT *)ip;
  statements = root->right;
  // ip->mdepends = 0;
  ip->opdstot = 0;
  ip->nocheckpcnt = tree_contains_fn_p(csound, root->right);
  ip->pmax = 3L;

  /* Initialize */
  ip->t.oentry = find_opcode(csound, "instr");
  /*  to hold global assigns */
  ip->t.opcod = strsav_string(csound, engineState, "instr");

  /* The following differs from otran and needs review.  otran keeps a
   * nulllist to point to for empty lists, while this is creating a new list
   * regardless
   */
  ip->t.outlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
  ip->t.outlist->count = 0;
  ip->t.inlist = (ARGLST *)csound->Malloc(csound, sizeof(ARGLST));
  ip->t.inlist->count = 1;

  /* create local ksmps variable */
  CS_TYPE *rType = (CS_TYPE *)&CS_VAR_TYPE_R;
  CS_VARIABLE *var =
      csoundCreateVariable(csound, csound->typePool, rType, "ksmps", NULL);
  csoundAddVariable(csound, ip->varPool, var);
  /* same for kr */
  var = csoundCreateVariable(csound, csound->typePool, rType, "kr", NULL);
  csoundAddVariable(csound, ip->varPool, var);

  /* Maybe should do this assignment at end when instr is setup?
   * Note: look into how "instr 4,5,6,8" is handled, i.e. if copies
   * are made or if they are all set to point to the same INSTRTXT
   *
   * Note2: For now am not checking if root->left is a list (i.e. checking
   * root->left->next is NULL or not to indicate list)
   */
  if (root->left->type == INTEGER_TOKEN) {            /* numbered instrument */
    int32 instrNum = (int32)root->left->value->value; /* Not used! */

    c = csound->Malloc(csound, 10); /* arbritrarily chosen number of digits */
    snprintf(c, 10, "%" PRIi32, instrNum);

    if (PARSER_DEBUG)
      csound->Message(csound, Str("create_instrument: instr num %" PRIi32 "\n"),
                      instrNum);

    ip->t.inlist->arg[0] = strsav_string(csound, engineState, c);

    csound->Free(csound, c);
  } else if (root->left->type == T_IDENT &&
             !(root->left->left != NULL &&
               root->left->left->type ==
                   UDO_ANS_TOKEN)) { /* named instrument */
    int32 insno_priority = -1L;
    c = root->left->value->lexeme;

    if (PARSER_DEBUG)
      csound->Message(csound, "create_instrument: instr name %s\n", c);

    if (UNLIKELY(root->left->rate == (int)'+')) {
      insno_priority--;
    }

    ip->insname = csound->Malloc(csound, strlen(c) + 1);
    strcpy(ip->insname, c);
  }
  current = statements;
  while (current != NULL) {
    OPTXT *optxt = create_opcode(csound, current, ip, engineState);
    op->nxtop = optxt;
    op = last_optxt(op);
    current = current->next;
  }
  close_instrument(csound, engineState, ip);
  return ip;
}

void close_instrument(CSOUND *csound, ENGINE_STATE *engineState, INSTRTXT *ip) {
  OPTXT *bp, *current;
  int n;
  bp = (OPTXT *)csound->Calloc(csound, (int32)sizeof(OPTXT));

  bp->t.oentry = find_opcode(csound, "endin"); /*  send an endin to */
  bp->t.opcod =
      strsav_string(csound, engineState, "endin"); /*  term instr 0 blk */
  bp->t.outlist = bp->t.inlist = NULL;

  bp->nxtop = NULL; /* terminate the optxt chain */

  current = (OPTXT *)ip;

  while (current->nxtop != NULL) {
    current = current->nxtop;
  }

  current->nxtop = bp;
  ip->pextrab = ((n = ip->pmax - 3L) > 0 ? (int)n * sizeof(MYFLT) : 0);
  ip->pextrab = ((int)ip->pextrab + 7) & (~7);
  ip->muted = 1;
}

void deleteVarPoolMemory(void *csound, CS_VAR_POOL *pool);

/**
   This function deletes an inactive instrument which has been replaced
*/
void free_instrtxt(CSOUND *csound, INSTRTXT *instrtxt) {
  INSTRTXT *ip = instrtxt;
  INSDS *active = ip->instance;
  while (active != NULL) { /* remove instance memory */
    INSDS *nxt = active->nxtinstance;
    if (active->fdchp != NULL)
      fdchclose(csound, active);
    if (active->auxchp != NULL)
      auxchfree(csound, active);
    free_instr_var_memory(csound, active);
    if (active->opcod_iobufs != NULL)
      csound->Free(csound, active->opcod_iobufs);
    csound->Free(csound, active);
    active = nxt;
  }
  OPTXT *t = ip->nxtop;
  while (t) {
    OPTXT *s = t->nxtop;
    TEXT *ttp = &t->t;
    // printf("%s\n",  ttp->opcod);
    ARG *current = ttp->outArgs;
    while (current != NULL) {
      ARG *tmp = current;
      // printf("delete %p\n", tmp);
      current = current->next;
      csound->Free(csound, tmp);
    }
    csound->Free(csound, t->t.outlist);
    current = ttp->inArgs;
    while (current != NULL) {
      ARG *tmp = current;
      // printf("delete %p\n", tmp);
      current = current->next;
      csound->Free(csound, tmp);
    }
    csound->Free(csound, t->t.inlist);
    csound->Free(csound, t);
    t = s;
  }

  csound->Free(csound, ip->t.outlist);
  csound->Free(csound, ip->t.inlist);
  CS_VARIABLE *var = ip->varPool->head;
  while (var != NULL) {
    CS_VARIABLE *tmp = var;
    var = var->next;
    csound->Free(csound, tmp->varName);
  }

  csoundFreeVarPool(csound, ip->varPool);
  csound->Free(csound, ip);
  if (UNLIKELY(csound->oparms->odebug))
    csound->Message(csound, Str("-- deleted instr from deadpool\n"));
}

/**
 * This function has two purposes:
 * 1) check deadpool for active instances, and
 * if none is active, send it to be deleted
 * 2) add a dead instr to deadpool (because it still has active instances)
 */
void add_to_deadpool(CSOUND *csound, INSTRTXT *instrtxt) {
  /* check current items in deadpool to see if they need deleting */
  int i;
  for (i = 0; i < csound->dead_instr_no; i++) {
    if (csound->dead_instr_pool[i] != NULL) {
      INSDS *active = csound->dead_instr_pool[i]->instance;
      while (active != NULL) {
        if (active->actflg) {
          // add_to_deadpool(csound,csound->dead_instr_pool[i]);
          break;
        }
        active = active->nxtinstance;
      }
      /* no active instances */
      if (active == NULL) {
        if (UNLIKELY(csound->oparms->odebug))
          csound->Message(csound, Str(" -- free instr def %p %p\n"),
                          csound->dead_instr_pool[i]->instance,
                          csound->dead_instr_pool[i]);
        free_instrtxt(csound, csound->dead_instr_pool[i]);
        csound->dead_instr_pool[i] = NULL;
      }
    }
  }
  /* add latest instr to deadpool */
  /* check for free slots */
  for (i = 0; i < csound->dead_instr_no; i++) {
    if (csound->dead_instr_pool[i] == NULL) {
      csound->dead_instr_pool[i] = instrtxt;
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound, Str(" -- added to deadpool slot %d\n"), i);
      return;
    }
  }
  /* no free slots, expand pool */
  csound->dead_instr_pool = (INSTRTXT **)csound->ReAlloc(
      csound, csound->dead_instr_pool,
      ++csound->dead_instr_no * sizeof(INSTRTXT *));
  csound->dead_instr_pool[csound->dead_instr_no - 1] = instrtxt;
  if (UNLIKELY(csound->oparms->odebug))
    csound->Message(csound, Str(" -- added to deadpool slot %d\n"),
                    csound->dead_instr_no - 1);
}

MYFLT named_instr_find(CSOUND *csound, char *s);
MYFLT named_instr_find_in_engine(CSOUND *csound, char *s,
                                  ENGINE_STATE *engineState);
/**
   allocate entry for named instrument ip with name s
   instrument number is set to insno
   If named instr exists, it is replaced.
*/
int named_instr_alloc(CSOUND *csound, char *s, INSTRTXT *ip, int32 insno,
                      ENGINE_STATE *engineState, int merge) {
  INSTRNAME *inm, *inm2, *inm_head;
  int ret = 1, no = insno;

  if (UNLIKELY(!engineState->instrumentNames))
    engineState->instrumentNames = cs_hash_table_create(csound);

  /* now check if instrument is already defined */
  inm = cs_hash_table_get(csound, engineState->instrumentNames, s);
  if (inm != NULL) {
    int i;
    ret = 0;
    if (!merge)
      return ret;
    inm->ip->isNew = 1;
    /* redefinition does not raise an error now, just a warning */
    if (UNLIKELY(csound->oparms->odebug))
      csound->Warning(csound, Str("named instr %" PRIi32 " redefined, "
                                  "replacing previous definition"),
                      inm->instno);
    // VL 26.05.2018 get the existing instr allocated number
    no = inm->instno;
    /* here we should move the old instrument definition into a deadpool
       which will be checked for active instances and freed when there are no
       further ones
    */
    for (i = 0; i < engineState->maxinsno; i++) {
      /* check for duplicate numbers and do nothing */
      if (i != inm->instno &&
          engineState->instrtxtp[i] == engineState->instrtxtp[inm->instno]) {
        // csound->Message(csound, "duplicate %d %d\n", i, inm->instno);
        // so fill this with the new instrument pointer
        engineState->instrtxtp[i] = ip;
        goto cont;
      }
    }
    INSDS *active = engineState->instrtxtp[inm->instno]->instance;
    while (active != NULL) {
      if (active->actflg) {
        /* FIXME:  */
        /* this seems to be wiping memory that is still being used */
        // add_to_deadpool(csound, engineState->instrtxtp[inm->instno]);
        /* this marks the instrument number ready for replacement */
        engineState->instrtxtp[inm->instno] = NULL;
        break;
      }
      active = active->nxtinstance;
    }
    /* no active instances */
    if (active == NULL) {
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound, Str("no active instances\n"));
      free_instrtxt(csound, engineState->instrtxtp[inm->instno]);
      engineState->instrtxtp[inm->instno] = NULL;
    }
    inm->ip->instance = inm->ip->act_instance = inm->ip->lst_instance = NULL;
  }
cont:

  /* allocate entry, */
  inm = (INSTRNAME *)csound->Calloc(csound, sizeof(INSTRNAME));

  /* and store parameters */
  inm->name = cs_strdup(csound, s);
  inm->ip = ip;
  // VL 26.05.2018 copy existing number
  if(no > 0)
    inm->instno = no;

  //printf("insno %d %s\n", insno, s);
  /* link into chain */
  cs_hash_table_put(csound, engineState->instrumentNames, s, inm);

  if(!merge) {
  /* temporary chain for use by named_instr_assign_numbers()
     this is not needed at merge stage
  */
    inm2 = (INSTRNAME *)csound->Calloc(csound, sizeof(INSTRNAME));
    inm2->instno = insno;
    inm2->name = (char *) inm; /* hack */
    inm_head = cs_hash_table_get(csound, engineState->instrumentNames,
                                 (char *)INSTR_NAME_FIRST);

    if (inm_head == NULL) {
      cs_hash_table_put(csound, engineState->instrumentNames,
                        (char *)INSTR_NAME_FIRST, inm2);
    } else {
      while (inm_head->next != NULL) {
        inm_head = inm_head->next;
      }
      inm_head->next = inm2;
    }
  }

  if (UNLIKELY(csound->oparms->odebug) && engineState == &csound->engineState)
    csound->Message(csound, "named instr name = \"%s\", txtp = %p,\n", s,
                    (void *)ip);
  return ret;
}


/**
   assign instrument numbers to all named instruments
*/
void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState) {
  INSTRNAME *inm, *inm2, *inm_first;
  int num = 0, inum, insno_priority = 0;

  if (!engineState->instrumentNames)
    return; /* no named instruments */
  inm_first = cs_hash_table_get(csound, engineState->instrumentNames,
                                (char *)INSTR_NAME_FIRST);

  while (--insno_priority > -3) {
    if (insno_priority == -2) {
      int found = 0;
      /* check both this state & current state */
      if(engineState->maxinsno > csound->engineState.maxinsno) {
        for(num=engineState->maxinsno; num > csound->engineState.maxinsno; num--) {
          if(engineState->instrtxtp[num]) {
            found = 1;
            break;
          }
        }
      } else {
        for(num=csound->engineState.maxinsno; num > engineState->maxinsno; num--) {
          if(csound->engineState.instrtxtp[num]) {
            found = 1;
            break;
          }
        }
      }
      if(!found) {
        while(!engineState->instrtxtp[num] && !csound->engineState.instrtxtp[num] && --num);
      }
    }
    for (inm = inm_first; inm; inm = inm->next) {
      INSTRNAME *temp = (INSTRNAME *)inm->name;
      int no = 0;
      if ((int)inm->instno != insno_priority)
        continue;
      no = named_instr_find(csound, temp->name);

      if (no == 0) { // if there is no allocated number
        /* find an unused number and use it */
        /* VL, start from instr 1 */
        num = 1;
        /* check both this state & current state */
        while (num <= engineState->maxinsno
               && (engineState->instrtxtp[num]
              || csound->engineState.instrtxtp[num])) num++;

        /* we may need to expand the instrument array */
        if (num > engineState->maxinsno) {
          int m = engineState->maxinsno;
          engineState->maxinsno += MAXINSNO; /* Expand */
          engineState->instrtxtp = (INSTRTXT **)csound->ReAlloc(
              csound, engineState->instrtxtp,
              (1 + engineState->maxinsno) * sizeof(INSTRTXT *));
          /* Array expected to be nulled so.... */
          while (++m <= engineState->maxinsno)
            engineState->instrtxtp[m] = NULL;
        }
        inum = num;

      } else {
        inum = no; // else use existing number
      }
      /* hack: "name" actually points to the corresponding INSTRNAME */
      inm2 = (INSTRNAME *)(inm->name); /* entry in the table */

      inm2->instno = (int32)inum;
      engineState->instrtxtp[inum] = inm2->ip;

      if (UNLIKELY((csound->oparms->odebug) || (csound->oparms->msglevel > 0)))
        csound->Message(csound, Str("instr %s uses instrument number %d\n"),
                        inm2->name, inum);
    }
  }
  /* clear temporary chains */

  inm = inm_first;
  while (inm) {
    INSTRNAME *nxtinm = inm->next;
    csound->Free(csound, inm);
    inm = nxtinm;
  }
  cs_hash_table_remove(csound, engineState->instrumentNames,
                       (char *)INSTR_NAME_FIRST);
}

/**
   Insert INSTRTXT into an engineState list of INSTRTXT's,
   checking to see if number is greater than number of pointers currently
   allocated and if so expand pool of instruments
*/
void insert_instrtxt(CSOUND *csound, INSTRTXT *instrtxt, int32 instrNum,
                     ENGINE_STATE *engineState, int merge) {
  int i;

  if (UNLIKELY(instrNum >= engineState->maxinsno)) {
    int old_maxinsno = engineState->maxinsno;

    /* expand */
    while (instrNum >= engineState->maxinsno) {
      engineState->maxinsno += MAXINSNO;
    }

    engineState->instrtxtp = (INSTRTXT **)csound->ReAlloc(
        csound, engineState->instrtxtp,
        (1 + engineState->maxinsno) * sizeof(INSTRTXT *));

    /* Array expected to be nulled so.... */
    for (i = old_maxinsno + 1; i <= engineState->maxinsno; i++) {
      engineState->instrtxtp[i] = NULL;
    }
  }

  if (UNLIKELY(engineState->instrtxtp[instrNum] != NULL)) {
    instrtxt->isNew = 1;

    /* redefinition does not raise an error now, just a warning */
    /* unless we are not merging */
    if (!merge)
      synterr(csound, Str("instr %d redefined\n"), instrNum);
    if (UNLIKELY(instrNum && csound->oparms->odebug))
      csound->Warning(csound, Str("instr %" PRIi32 " redefined, "
                                  "replacing previous definition"),
                      instrNum);
    /* inherit active & maxalloc flags */
    instrtxt->active = engineState->instrtxtp[instrNum]->active;
    instrtxt->maxalloc = engineState->instrtxtp[instrNum]->maxalloc;

    /* here we should move the old instrument definition into a deadpool
       which will be checked for active instances and freed when there are no
       further ones
    */
    for (i = 0; i < engineState->maxinsno; i++) {
      /* check for duplicate numbers and do nothing */
      if (i != instrNum &&
          engineState->instrtxtp[i] == engineState->instrtxtp[instrNum]) {
        csound->Message(csound, "duplicate %d %d\n", i, instrNum);
        // VL 26.05.2018
        // so fill this with the new instrument pointer
        engineState->instrtxtp[i] = instrtxt;
        goto end;
      }
    }
    INSDS *active = engineState->instrtxtp[instrNum]->instance;
    while (active != NULL && instrNum != 0) {
      if (active->actflg) {
        add_to_deadpool(csound, engineState->instrtxtp[instrNum]);
        break;
      }
      active = active->nxtinstance;
    }

    /* no active instances */
    /* instr0 is freed elsewhere */
    if (active == NULL && instrNum != 0) {
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound, Str("no active instances of instr %d\n"),
                        instrNum);
      free_instrtxt(csound, engineState->instrtxtp[instrNum]);
    }

    /* err++; continue; */
  }
end:

  instrtxt->instance = instrtxt->act_instance = instrtxt->lst_instance = NULL;
  engineState->instrtxtp[instrNum] = instrtxt;
  //csound->Message(csound, "instrument %d of %d: %p \n",
  //                instrNum, engineState->maxinsno, instrtxt);
}

void insert_opcodes(CSOUND *csound, OPCODINFO *opcodeInfo,
                    ENGINE_STATE *engineState) {
  if (opcodeInfo) {
    int num = engineState->maxinsno; /* store after any other instruments */
    OPCODINFO *inm = opcodeInfo;
    while (inm) {
      /* we may need to expand the instrument array */
      if (UNLIKELY(++num > engineState->maxopcno)) {
        int i;
        i = (engineState->maxopcno > 0 ? engineState->maxopcno
                                       : engineState->maxinsno);
        engineState->maxopcno = i + MAXINSNO;
        engineState->instrtxtp = (INSTRTXT **)csound->ReAlloc(
            csound, engineState->instrtxtp,
            (1 + engineState->maxopcno) * sizeof(INSTRTXT *));
        /* Array expected to be nulled so.... */
        while (++i <= engineState->maxopcno)
          engineState->instrtxtp[i] = NULL;
      }
      inm->instno = num;
      // csound->Message(csound, Str("UDO INSTR NUM: %d\n"), num);
      engineState->instrtxtp[num] = inm->ip;
      inm = inm->prv;
    }
  }
}

OPCODINFO *find_opcode_info(CSOUND *csound, char *opname, char *outargs,
                            char *inargs) {
  OPCODINFO *opinfo = csound->opcodeInfo;
  if (UNLIKELY(opinfo == NULL)) {
    csound->Message(csound, Str("!!! csound->opcodeInfo is NULL !!!\n"));
    return NULL;
  }

  while (opinfo != NULL) {
    if (UNLIKELY(strcmp(opinfo->name, opname) == 0 &&
                 strcmp(opinfo->intypes, inargs) == 0 &&
                 strcmp(opinfo->outtypes, outargs) == 0)) {
      return opinfo;
    }
    opinfo = opinfo->prv; /* Move on: JPff suggestion */
  }

  return NULL;
}

/**
   Merge a new engineState into csound->engineState
   1) Add to stringPool, constantsPool and varPool (globals)
   2) Add to opinfo and UDOs
   3) Call insert_instrtxt() on csound->engineState for each new instrument
   4) Call insprep() and recalculateVarPoolMemory() for each new instrument
   5) patch up nxtinstxt order
*/
int engineState_merge(CSOUND *csound, ENGINE_STATE *engineState) {
  int i, end = engineState->maxinsno;
  ENGINE_STATE *current_state = &csound->engineState;
  INSTRTXT *current;//, *old_instr0;
  int count = 0;

  // cs_hash_table_merge(csound,
  //                current_state->stringPool, engineState->stringPool);

  cs_hash_table_merge(csound, current_state->constantsPool,
                      engineState->constantsPool);

  /* for (count = 0; count < engineState->constantsPool->count; count++) {
     if (UNLIKELY(csound->oparms->odebug))
     csound->Message(csound, Str(" merging constants %d) %f\n"),
     count, engineState->constantsPool->values[count].value);
     myflt_pool_find_or_add(csound, current_state->constantsPool,
     engineState->constantsPool->values[count].value);
     }*/

  CS_VARIABLE *gVar = engineState->varPool->head;
  while (gVar != NULL) {
    CS_VARIABLE *var;
    if (UNLIKELY(csound->oparms->odebug))
      csound->Message(csound, Str(" merging %p %d) %s:%s\n"), gVar, count,
                      gVar->varName, gVar->varType->varTypeName);
    var = csoundFindVariableWithName(csound, current_state->varPool,
                                     gVar->varName);
    if (var == NULL) {
      ARRAY_VAR_INIT varInit;
      varInit.dimensions = gVar->dimensions;
      varInit.type = gVar->subType;
      var = csoundCreateVariable(csound, csound->typePool, gVar->varType,
                                 gVar->varName, &varInit);
      csoundAddVariable(csound, current_state->varPool, var);
      /* memory has already been allocated, so we just point to it */
      /* when disposing of the engineState global vars, we do not
         delete the memBlock */
      var->memBlock = gVar->memBlock;
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound, Str(" adding %p %d) %s:%s\n"), var, count,
                        gVar->varName, gVar->varType->varTypeName);
      gVar = gVar->next;
    } else {
      // if variable exists
      // free variable mem block
      // printf("free %p\n", gVar->memBlock);
      // the CS_VARIABLE itself will be freed on engine_free()
      csound->Free(csound, gVar->memBlock);
      csound->Free(csound, gVar->varName);
      gVar = gVar->next;
    }
  }

  /* merge opcodinfo */

  /* VL probably not the right place, since instr list
     might grow
     insert_opcodes(csound, csound->opcodeInfo, current_state);
  */

  // VL: don't want to free instr0
  // old_instr0 = current_state->instrtxtp[0];
  insert_instrtxt(csound, engineState->instrtxtp[0], 0, current_state, 1);
  for (i = 1; i < end; i++) {
    current = engineState->instrtxtp[i];

    if (current != NULL) {
      // csound->Message(csound, "INSTR %d \n", i);
      if (current->insname == NULL) {
        if (csound->oparms->odebug)
          csound->Message(csound, Str("merging instr %d\n"), i);
        /* a first attempt at this merge is to make it use
           insert_instrtxt again */
        /* insert instrument in current engine */
        insert_instrtxt(csound, current, i, current_state, 1);
      } else {
       if (UNLIKELY(csound->oparms->odebug))
          csound->Message(csound, Str("merging named instr %s\n"),
                          current->insname);
        /* allocate a named_instr string in the current engine */
        /* find the assigned number in the engineState and use it for
           the current engine */
        int32 nnum =
          named_instr_find_in_engine(csound, current->insname, engineState);
        named_instr_alloc(csound, current->insname, current, nnum, current_state,
                         1);
        /* place it in the corresponding slot */
        current_state->instrtxtp[i] = current;
      }
    }
  }
  /* VL 30.6.2018 commented this out so all the assignment
     occurs earlier on before merge
  */
  // csound->Message(csound, "assign numbers; %p\n", current_state);
  //named_instr_assign_numbers(csound, current_state);

  /* VL MOVED here after all instruments are merged so
     that we get the correct number */
  insert_opcodes(csound, csound->opcodeInfo, current_state);
  /* this needs to be called in a separate loop
     in case of multiple instr numbers, so insprep() is called only once */
  current = (&(engineState->instxtanchor)); //->nxtinstxt;
  while ((current = current->nxtinstxt) != NULL) {
    if (UNLIKELY(csound->oparms->odebug))
      csound->Message(csound, "insprep %p\n", current);
    insprep(csound, current, current_state); /* run insprep() to connect ARGS */
    recalculateVarPoolMemory(csound,
                             current->varPool); /* recalculate var pool */
  }
  /* now we need to patch up instr order */
  end = current_state->maxinsno;
  end = end < current_state->maxopcno ? current_state->maxopcno : end;
  for (i = 0; i < end; i++) {
    int j;
    current = current_state->instrtxtp[i];
    if (current != NULL) {
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound, "instr %d:%p\n", i, current);
      current->nxtinstxt = NULL;
      j = i;
      while (++j < end - 1) {
        if (current_state->instrtxtp[j] != NULL) {
          current->nxtinstxt = current_state->instrtxtp[j];
          break;
        }
      }
    }
  }
  (&(current_state->instxtanchor))->nxtinstxt = csound->instr0;
  /* now free old instr 0 */
  // VL 23-09-2022 this can't be done simply because we don't know if the
  // instr0 has finished executing.
  // At the moment, instr0 can't be freed anywhere because it may hold state
  // that is referred to elsewhere by instrument instances.
  // something to consider in 7.*
  //  free_instrtxt(csound, old_instr0);
  return 0;
}

int engineState_free(CSOUND *csound, ENGINE_STATE *engineState) {

  // csound->Free(csound, engineState->instrumentNames);
  cs_hash_table_free(csound, engineState->constantsPool);
  // cs_hash_table_free(csound, engineState->stringPool);
  csoundFreeVarPool(csound, engineState->varPool);
  csound->Free(csound, engineState->instrtxtp);
  csound->Free(csound, engineState);
  return 0;
}

void free_typetable(CSOUND *csound, TYPE_TABLE *typeTable) {
  cs_cons_free_complete(csound, typeTable->labelList);
  csound->Free(csound, typeTable);
}

static char *node2string(int type) {
  /* Add new nodes here as necessary -- JPff */
  switch (type) {
  /* case LABEL_TOKEN: */
  /*   return "label"; */
  default:
    return "??";
  }
}

/** Merge and Dispose of engine state and type table,
    and run global i-time code
*/
void merge_state(CSOUND *csound, ENGINE_STATE *engineState,
                 TYPE_TABLE *typetable, OPDS *ids) {
  if (csound->init_pass_threadlock)
    csoundLockMutex(csound->init_pass_threadlock);
  engineState_merge(csound, engineState);
  engineState_free(csound, engineState);
  free_typetable(csound, typetable);
  /* run global i-time code */
  init0(csound);
  csound->ids = ids;
  if (csound->init_pass_threadlock)
    csoundUnlockMutex(csound->init_pass_threadlock);
}

/**
 * Compile the given TREE node into structs

 In the the first compilation run, it:
 1) Uses the empty csound->engineState
 2) Creates instrument 0
 3) Creates other instruments and UDOs
 4) Runs insprep() and recalculateVarpool() for each instrument

 In any subsequent compilation run, it:
 1) Creates a new engineState
 2) instrument 0 is treated as a global i-time instrument, header constants
 are ignored.
 3) Creates other instruments
 4) Calls engineState_merge() and engineState_free()

 async determines asynchronous operation of the
 merge stage.

 VL 20-12-12

 * ASSUMES: TREE has been validated prior to compilation
 *
 *
 */
int csoundCompileTreeInternal(CSOUND *csound, TREE *root, int async) {
  INSTRTXT *instrtxt = NULL;
  INSTRTXT *ip = NULL;
  INSTRTXT *prvinstxt;
  OPTXT *bp;
  char *opname;
  TREE *current = root;
  ENGINE_STATE *engineState;
  CS_VARIABLE *var;
  TYPE_TABLE *typeTable = (TYPE_TABLE *)current->markup;

  current = current->next;
  if (csound->instr0 == NULL) {
    engineState = &csound->engineState;
    engineState->varPool = typeTable->globalPool;

    csound->instr0 = create_instrument0(csound, current, engineState,
                                        typeTable->instr0LocalPool);
    cs_hash_table_put_key(csound, engineState->stringPool, "\"\"");
    prvinstxt = &(engineState->instxtanchor);
    engineState->instrtxtp = (INSTRTXT **)csound->Calloc(
        csound, (1 + engineState->maxinsno) * sizeof(INSTRTXT *));
    prvinstxt = prvinstxt->nxtinstxt = csound->instr0;
    insert_instrtxt(csound, csound->instr0, 0, engineState, 0);
  } else {
    engineState = (ENGINE_STATE *)csound->Calloc(csound, sizeof(ENGINE_STATE));
    engineState->stringPool = csound->engineState.stringPool;
    // cs_hash_table_create(csound);
    engineState->constantsPool = cs_hash_table_create(csound);
    engineState->varPool = typeTable->globalPool;
    prvinstxt = &(engineState->instxtanchor);
    engineState->instrtxtp = (INSTRTXT **)csound->Calloc(
        csound, (1 + engineState->maxinsno) * sizeof(INSTRTXT *));
    /* VL: allowing global code to be evaluated in
       subsequent compilations */
    //csound->inZero = 1;
    csound->instr0 = create_global_instrument(csound, current, engineState,
                                              typeTable->instr0LocalPool);
    //csound->inZero = 0;

    insert_instrtxt(csound, csound->instr0, 0, engineState, 1);

    prvinstxt = prvinstxt->nxtinstxt = csound->instr0;
    // engineState->maxinsno = 1;
  }

  // allocate memory for global vars
  // if this variable already exists,
  // memory will be freed on merge.
  var = typeTable->globalPool->head;
  while (var != NULL) {
    size_t memSize = CS_VAR_TYPE_OFFSET + var->memBlockSize;
    CS_VAR_MEM *varMem = (CS_VAR_MEM *)csound->Calloc(csound, memSize);
    // printf("alloc %p -- %s\n", varMem, var->varName);
    varMem->varType = var->varType;
    var->memBlock = varMem;
    if (var->initializeVariableMemory != NULL) {
      var->initializeVariableMemory((void *)csound, var, &varMem->value);
    } else
      memset(&varMem->value, 0, var->memBlockSize);
    var = var->next;
  }

  while (current != NULL) {

    switch (current->type) {
    case '=':
      /* csound->Message(csound, "Assignment found\n"); */
      break;
    case INSTR_TOKEN:
      // print_tree(csound, "Instrument found\n", current);
      instrtxt = create_instrument(csound, current, engineState);

      prvinstxt = prvinstxt->nxtinstxt = instrtxt;

      /* Handle Inserting into CSOUND here by checking ids (name or
       * numbered) and using new insert_instrtxt?
       */
      /* Temporarily using the following code */
      if (current->left->type == INTEGER_TOKEN) { /* numbered instrument, eg.:
                                                     instr 1
                                                  */
        int32 instrNum = (int32)current->left->value->value;
        insert_instrtxt(csound, instrtxt, instrNum, engineState, 0);

      } else if (current->left->type == T_IDENT) { /* named instrument, eg.:
                                                      instr Hello
                                                   */
        int32 insno_priority = -1L;
        char *c;
        c = current->left->value->lexeme;

        if (UNLIKELY(current->left->rate == (int)'+')) {
          insno_priority--;
        }
        if (UNLIKELY(!check_instr_name(c))) {
          synterr(csound, Str("invalid name for instrument"));
        }

        named_instr_alloc(csound, c, instrtxt, insno_priority, engineState, 0);
        if(engineState != &csound->engineState)
          named_instr_assign_numbers(csound, engineState);
        /* VL 10.10.14: check for redefinition */
        // if (UNLIKELY(!named_instr_alloc(csound, c,
        //  instrtxt, insno_priority,
        //                              engineState, 0))) {
        // synterr(csound, Str("instr %s redefined\n"), c);
        //}

        instrtxt->insname = csound->Malloc(csound, strlen(c) + 1);
        strcpy(instrtxt->insname, c);
      } else if (current->left->type == T_INSTLIST) {
        /* list of instr names, eg:
           instr Hello, 1, 2
        */

        TREE *p = current->left;
        while (p) {
          if (PARSER_DEBUG)
            print_tree(csound, "Top of loop\n", p);
          if (p->left) {

            if (p->left->type == INTEGER_TOKEN) {
              //csound->Message(csound, "instrument %d \n",
              //                (int) p->left->value->value);
              insert_instrtxt(csound, instrtxt, p->left->value->value,
                              engineState, 0);
            } else if (p->left->type == T_IDENT) {
              int32 insno_priority = -1L;
              char *c;
              c = p->left->value->lexeme;
              if (UNLIKELY(p->left->rate == (int)'+')) {
                insno_priority--;
              }
              if (UNLIKELY(!check_instr_name(c))) {
                synterr(csound, Str("invalid name for instrument"));
              }
              // VL 25.05.2018
              // this should only be run here in the
              // first compilation
              if(engineState == &csound->engineState)
                named_instr_alloc(csound, c, instrtxt, insno_priority,
                                  engineState, 0);
              /* if (UNLIKELY(!named_instr_alloc(csound, c, */
              /*                                 instrtxt, insno_priority, */
              /*                                 engineState,0))) { */
              /*   synterr(csound, Str("instr %s redefined"), c); */
              /* } */

              instrtxt->insname = csound->Malloc(csound, strlen(c) + 1);
              strcpy(instrtxt->insname, c);
            }
          } else {
            if (p->type == INTEGER_TOKEN) {

              insert_instrtxt(csound, instrtxt, p->value->value, engineState,
                              0);
            } else if (p->type == T_IDENT) {

              int32 insno_priority = -1L;
              char *c;
              c = p->value->lexeme;

              if (UNLIKELY(p->rate == (int)'+')) {
                insno_priority--;
              }
              if (UNLIKELY(!check_instr_name(c))) {
                synterr(csound, Str("invalid name for instrument"));
              }
              // VL 25.05.2018
              // this should only be run here in the
              // first compilation
              // if(engineState == &csound->engineState)
              named_instr_alloc(csound, c, instrtxt, insno_priority,
                                engineState, 0);

              /* if (UNLIKELY(!named_instr_alloc(csound, c, */
              /*                                 instrtxt, insno_priority, */
              /*                                 engineState,0))) { */
              /*   synterr(csound, Str("instr %s redefined"), c); */
              /* } */
              instrtxt->insname = csound->Malloc(csound, strlen(c) + 1);
              strcpy(instrtxt->insname, c);
            }
            break;
          }
          p = p->right;
        }
      }
      break;
    case UDO_TOKEN:
      /* csound->Message(csound, "UDO found\n"); */
      instrtxt = create_instrument(csound, current, engineState);
      prvinstxt = prvinstxt->nxtinstxt = instrtxt;
      opname = current->left->value->lexeme;
      OPCODINFO *opinfo =
          find_opcode_info(csound, opname, current->left->left->value->lexeme,
                           current->left->right->value->lexeme);

      if (UNLIKELY(opinfo == NULL)) {
        csound->Message(csound,
                        Str("ERROR: Could not find OPCODINFO for opname: %s\n"),
                        opname);
      } else {
        opinfo->ip = instrtxt;
        instrtxt->insname = cs_strdup(csound, opname);
        instrtxt->opcode_info = opinfo;
      }

      /* Handle Inserting into CSOUND here by checking id's (name or
       * numbered) and using new insert_instrtxt?
       */

      break;
    case T_OPCODE:
    case T_OPCODE0:
    case LABEL:
    case LABEL_TOKEN:
      break;

    default:
      csound->Message(csound,
                      Str("Unknown TREE node of type %d (%s) found in root.\n"),
                      current->type, node2string(current->type));
      if (PARSER_DEBUG)
        print_tree(csound, NULL, current);
    }
    current = current->next;
  }

  if (UNLIKELY(csound->synterrcnt)) {
    print_opcodedir_warning(csound);
    csound->Warning(csound, Str("%d syntax errors in orchestra.  "
                                "compilation invalid\n"),
                    csound->synterrcnt);
    free_typetable(csound, typeTable);
    return CSOUND_ERROR;
  }


  if (engineState != &csound->engineState) {
    OPDS *ids = csound->ids;
    /* any compilation other than the first one */
    /* merge ENGINE_STATE */
    /* lock to ensure thread-safety */
    if (!async) {
      /* now add the instruments with names, assigning them fake instr numbers */
      named_instr_assign_numbers(csound, engineState);
      if (!csound->oparms->realtime)
        csoundLockMutex(csound->API_lock);
      merge_state(csound, engineState, typeTable, ids);
      if (!csound->oparms->realtime)
        csoundUnlockMutex(csound->API_lock);
    } else {
      if (csound->oparms->realtime)
        csoundSpinLock(&csound->alloc_spinlock);
      mergeState_enqueue(csound, engineState, typeTable, ids);
      if (csound->oparms->realtime)
        csoundSpinUnLock(&csound->alloc_spinlock);
    }
  } else {
    /* now add the instruments with names, assigning them fake instr numbers */
    named_instr_assign_numbers(csound, engineState);
    /* first compilation */
    insert_opcodes(csound, csound->opcodeInfo, engineState);
    ip = engineState->instxtanchor.nxtinstxt;
    bp = (OPTXT *)ip;
    while (bp != (OPTXT *)NULL && (bp = bp->nxtop) != NULL) {
      /* chk instr 0 for illegal perfs */
      int thread;
      OENTRY *oentry = bp->t.oentry;
      if (strcmp(oentry->opname, "endin") == 0)
        break;
      if (strcmp(oentry->opname, "$label") == 0)
        continue;
      if (PARSER_DEBUG)
        csound->DebugMsg(csound, "Instr 0 check on opcode=%s\n", bp->t.opcod);
      if (UNLIKELY((thread = oentry->thread) & 06 ||
                   (!thread && bp->t.pftype != 'b'))) {
        csound->DebugMsg(csound, "***opcode=%s thread=%d pftype=%c\n",
                         bp->t.opcod, thread, bp->t.pftype);
        /* synterr(csound,
           Str("perf-pass statements illegal in header blk (%s)\n"),
           oentry->opname);*/
        csound->Warning(csound,
                        Str("%s: perf-time code in global space, ignored"),
                        oentry->opname);
      }
    }

    ip = &(engineState->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL) { /* add all other entries */
      insprep(csound, ip, engineState);    /*   as combined offsets */
      recalculateVarPoolMemory(csound, ip->varPool);
    }

    CS_VARIABLE *var;
    var = csoundFindVariableWithName(csound, engineState->varPool, "sr");
    var->memBlock->value = csound->esr;
    var = csoundFindVariableWithName(csound, engineState->varPool, "kr");
    var->memBlock->value = csound->ekr;
    var = csoundFindVariableWithName(csound, engineState->varPool, "ksmps");
    var->memBlock->value = csound->ksmps;
    var = csoundFindVariableWithName(csound, engineState->varPool, "nchnls");
    var->memBlock->value = csound->nchnls;
    if (csound->inchnls < 0)
      csound->inchnls = csound->nchnls;
    var = csoundFindVariableWithName(csound, engineState->varPool, "nchnls_i");
    var->memBlock->value = csound->inchnls;
    var = csoundFindVariableWithName(csound, engineState->varPool, "0dbfs");
    var->memBlock->value = csound->e0dbfs;
    var = csoundFindVariableWithName(csound, engineState->varPool, "A4");
    var->memBlock->value = csound->A4;
  }

  return CSOUND_SUCCESS;
}

#ifdef EMSCRIPTEN
void sanitize(CSOUND *csound) {}
#else
extern void sanitize(CSOUND *csound);
#endif

/**
   Parse and compile an orchestra given on an string (OPTIONAL)
   if str is NULL the string is taken from the internal corfile
   containing the initial orchestra file passed to Csound.
   Also evaluates any global space code.
   async determines asynchronous operation of the
   merge stage.
*/
int csoundCompileOrcInternal(CSOUND *csound, const char *str, int async) {
  TREE *root;
  int retVal = 1;
  volatile jmp_buf tmpExitJmp;

  memcpy((void *)&tmpExitJmp, (void *)&csound->exitjmp, sizeof(jmp_buf));
  if ((retVal = setjmp(csound->exitjmp))) {
    memcpy((void *)&csound->exitjmp, (void *)&tmpExitJmp, sizeof(jmp_buf));
    return retVal;
  }
  // retVal = 1;
  root = csoundParseOrc(csound, str);
  if (LIKELY(root != NULL)) {
    retVal = csoundCompileTreeInternal(csound, root, async);
#ifdef PARCS
    // Sanitise semantic sets here
    sanitize(csound);
#endif
    csoundDeleteTree(csound, root);
  } else {
    // csoundDeleteTree(csound, root);
    memcpy((void *)&csound->exitjmp, (void *)&tmpExitJmp, sizeof(jmp_buf));
    return CSOUND_ERROR;
  }

  if (UNLIKELY(csound->oparms->odebug))
    debugPrintCsound(csound);
  memcpy((void *)&csound->exitjmp, (void *)&tmpExitJmp, sizeof(jmp_buf));
  return retVal;
}

/* prep an instr template for efficient allocs  */
/* repl arg refs by offset ndx to lcl/gbl space */
static void insprep(CSOUND *csound, INSTRTXT *tp, ENGINE_STATE *engineState)
{
  OPARMS *O = csound->oparms;
  OPTXT *optxt;
  OENTRY *ep;
  char **argp;

  int n, inreqd;
  char **argStringParts;
  ARGLST *outlist, *inlist;

  OENTRY *pset = find_opcode(csound, "pset");

  optxt = (OPTXT *)tp;
  while ((optxt = optxt->nxtop) != NULL) { /* for each op in instr */
    TEXT *ttp = &optxt->t;
    ep = ttp->oentry;

    if (strcmp(ep->opname, "endin") == 0 /*    (until ENDIN)     */
        || strcmp(ep->opname, "endop") == 0)
      break;
    if (strcmp(ep->opname, "$label") == 0) {
      continue;
    }

    if (UNLIKELY(O->odebug))
      csound->Message(csound, "%s args:", ep->opname);
    if ((outlist = ttp->outlist) == NULL || !outlist->count)
      ttp->outArgs = NULL;
    else {
      n = outlist->count;
      argp = outlist->arg; /* get outarg indices */
      while (n--) {
        ARG *arg = createArg(csound, tp, *argp++, engineState);
        if (ttp->outArgs == NULL) {
          ttp->outArgs = arg;
        } else {
          ARG *current = ttp->outArgs;
          while (current->next != NULL) {
            current = current->next;
          }
          current->next = arg;
          arg->next = NULL;
        }
      }
    }
    if ((inlist = ttp->inlist) == NULL || !inlist->count)
      ttp->inArgs = NULL;
    else {
      inreqd = argsRequired(ep->intypes);
      argStringParts = splitArgs(csound, ep->intypes);
      argp = inlist->arg; /* get inarg indices */
      for (n = 0; n < inlist->count; n++, argp++) {
        ARG *arg = NULL;
        if (n < inreqd && *argStringParts[n] == 'l') {
          arg = csound->Calloc(csound, sizeof(ARG));
          arg->type = ARG_LABEL;
          arg->argPtr = csound->Malloc(csound, strlen(*argp) + 1);
          strcpy(arg->argPtr, *argp);
          if (UNLIKELY(O->odebug))
            csound->Message(csound, "\t%s:", *argp); /* if arg is label,  */
        } else {
          char *s = *argp;
          arg = createArg(csound, tp, s, engineState);
        }

        if (ttp->inArgs == NULL) {
          ttp->inArgs = arg;
          // printf("yinarg %p -- opcode %s\n", arg, ttp->opcod);
        } else {
          ARG *current = ttp->inArgs;
          // printf("xinarg %p %p -- opcode %s\n", current, arg, ttp->opcod);
          while (current->next != NULL) {
            // printf("inarg %p %p -- opcode %s\n", current, arg, ttp->opcod);
            current = current->next;
          }
          current->next = arg;

          arg->next = NULL;
        }
      }

      if (ttp->oentry == pset) {
        MYFLT *fp1;
        int n;
        ARG *inArgs = ttp->inArgs;
        // CS_VARIABLE* var;

        if (tp->insname) {
          if(csound->oparms_.msglevel || csound->oparms_.odebug)
              csound->Message(csound, "PSET: isname=\"%s\", pmax=%d\n", tp->insname,
                              tp->pmax);
        } else {
          if(csound->oparms_.msglevel || csound->oparms_.odebug)
            csound->Message(csound, "PSET: isno=??, pmax=%d\n", tp->pmax);
        }
        if (UNLIKELY((n = ttp->inArgCount) != tp->pmax)) {
          // csound->Warning(csound, Str("i%d pset args != pmax"), (int) insno);
          csound->Warning(csound, Str("i[fixme] pset args != pmax"));
          if (n < tp->pmax)
            n = tp->pmax; /* cf pset, pmax    */
        }
        tp->psetdata = (MYFLT *)csound->Calloc(csound, n * sizeof(MYFLT));

        for (n = 0, fp1 = tp->psetdata; n < (int)ttp->inArgCount;
             n++, inArgs = inArgs->next) {
          switch (inArgs->type) {
          case ARG_CONSTANT:

            *fp1++ = ((CS_VAR_MEM *)inArgs->argPtr)->value;
            break;

          //                      case ARG_LOCAL:
          //                          *fp1++ = 44.0;
          //                          break;
          //
          //                      case ARG_GLOBAL:
          //                          var = (CS_VARIABLE*)inArgs->argPtr;
          //                          *fp1++ = *((MYFLT*)var->memBlock);
          //                          break;

          /* FIXME - to note, because this is done during
             compile time, pset does not work with local and
             global variables as they have not been initialized
             yet.  Csound5 also did not work with local/global
             variables.  In the future, use the test in
             tests/commandline/contrib/test_pset.csd for testing.
          */
          default:
            *fp1++ = 0.0;
            break;
          }

          //            csound->Message(csound, "..%f..", *(fp1-1));
        }

        csound->Message(csound, "\n");
      }
      // printf("delete %p\n", argStringParts);
      for (n = 0; argStringParts[n] != NULL; n++) {
        // printf("delete %p\n", argStringParts[n]);
        csound->Free(csound, argStringParts[n]);
      }
      csound->Free(csound, argStringParts);
    }

    if (UNLIKELY(O->odebug))
      csound->Message(csound, "\n");
  }
}

/* build pool of floating const values  */
/* build lcl/gbl list of ds names, offsets */
/* (no need to save the returned values) */
static void lgbuild(CSOUND *csound, INSTRTXT *ip, char *s, int inarg,
                    ENGINE_STATE *engineState) {
  IGN(ip);
  IGN(inarg);
  char c;
  char *temp;

  c = *s;
  /* must trap 0dbfs as name starts with a digit! */
  if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
      (c == '0' && strcmp(s, "0dbfs") != 0)) {
    if (cs_hash_table_get(csound, csound->engineState.constantsPool, s) ==
        NULL) {
      find_or_add_constant(csound, engineState->constantsPool, s,
                           cs_strtod(s, NULL));
    }
  } else if (c == '"') {
    temp = csound->Calloc(csound, strlen(s) + 1);
    // csound->Message(csound, "%c\n", s[1]);
    unquote_string(temp, s);
    cs_hash_table_put_key(csound, engineState->stringPool, temp);
    csound->Free(csound, temp);
  }
}

/* get storage ndx of const, pnum, lcl or gbl */
/* argument const/gbl indexes are positiv+1, */
/* pnum/lcl negativ-1 called only after      */
/* poolcount & lclpmax are finalised */
static ARG *createArg(CSOUND *csound, INSTRTXT *ip, char *s,
                      ENGINE_STATE *engineState) {
  char c;
  char *temp;
  int n;

  c = *s;

  ARG *arg = csound->Calloc(csound, sizeof(ARG));

  if (UNLIKELY(csound->oparms->odebug))
    csound->Message(csound, "\t%s", s); /* if arg is label,  */

  /* must trap 0dbfs as name starts with a digit! */
  if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
      (c == '0' && strcmp(s, "0dbfs") != 0)) {
    arg->type = ARG_CONSTANT;
    // printf("create constant %p: %c\n", arg, c);

    if ((arg->argPtr = cs_hash_table_get(
             csound, csound->engineState.constantsPool, s)) != NULL) {
      arg->argPtr = find_or_add_constant(csound, engineState->constantsPool, s,
                                         cs_strtod(s, NULL));
    }
  } else if (c == '"') {
    size_t memSize = CS_VAR_TYPE_OFFSET + sizeof(STRINGDAT);
    CS_VAR_MEM *varMem = csound->Calloc(csound, memSize);
    STRINGDAT *str = (STRINGDAT *)&varMem->value;
    // printf("create string %p: %s\n", arg, str->data);
    varMem->varType = (CS_TYPE *)&CS_VAR_TYPE_S;
    arg->type = ARG_STRING;
    temp = csound->Calloc(csound, strlen(s) + 1);
    unquote_string(temp, s);
    str->data =
        cs_hash_table_get_key(csound, csound->engineState.stringPool, temp);
    str->size = strlen(temp) + 1;
    csound->Free(csound, temp);
    arg->argPtr = str;
    if (str->data == NULL) {
      str->data = cs_hash_table_put_key(csound, engineState->stringPool, temp);
    }
  } else if ((n = pnum(s)) >= 0) {
    arg->type = ARG_PFIELD;
    arg->index = n;
  }
  /* trap local ksmps and kr  */
  else if ((strcmp(s, "ksmps") == 0 &&
            csoundFindVariableWithName(csound, ip->varPool, s)) ||
           (strcmp(s, "kr") == 0 &&
            csoundFindVariableWithName(csound, ip->varPool, s))) {
    arg->type = ARG_LOCAL;
    arg->argPtr = csoundFindVariableWithName(csound, ip->varPool, s);
  } else if (c == 'g' || (c == '#' && *(s + 1) == 'g') ||
             csoundFindVariableWithName(csound, csound->engineState.varPool,
                                        s) != NULL) {
    // FIXME - figure out why string pool searched with gexist
    //|| string_pool_indexof(csound->engineState.stringPool, s) > 0) {
    arg->type = ARG_GLOBAL;
    arg->argPtr = csoundFindVariableWithName(csound, engineState->varPool, s);
    // printf("create global %p: %s\n", arg->argPtr, s);
  } else {
    arg->type = ARG_LOCAL;
    arg->argPtr = csoundFindVariableWithName(csound, ip->varPool, s);
    // printf("create local %p: %s\n", arg, s);
    if (arg->argPtr == NULL) {
      csound->Message(csound, Str("Missing local arg: %s\n"), s);
    }
  }
  /*    csound->Message(csound, " [%s -> %d (%x)]\n", s, indx, indx); */

  return arg;
}

char argtyp2(char *s) { /* find arg type:  d, w, a, k, i, c, p, r, S, B, b, t */
  char c = *s;          /*   also set lgprevdef if !c && !p && !S */

  /* trap this before parsing for a number! */
  /* two situations: defined at header level: 0dbfs = 1.0
   *  and returned as a value:  idb = 0dbfs
   */

  if ((c >= '1' && c <= '9') || c == '.' || c == '-' || c == '+' ||
      (c == '0' && strcmp(s, "0dbfs") != 0))
    return ('c'); /* const */
  if (pnum(s) >= 0)
    return ('p'); /* pnum */
  if (c == '"')
    return ('S'); /* quoted String */
  if (strcmp(s, "sr") == 0 || strcmp(s, "kr") == 0 || strcmp(s, "0dbfs") == 0 ||
      strcmp(s, "nchnls_i") == 0 || strcmp(s, "ksmps") == 0 ||
      strcmp(s, "nchnls") == 0)
    return ('r'); /* rsvd */
  if (c == 'w')   /* N.B. w NOT YET #TYPE OR GLOBAL */
    return (c);
  if (c == '#')
    c = *(++s);
  if (c == 'g')
    c = *(++s);
  if (c == '[') {
    while (c == '[') {
      c = *(++s);
    }
  }
  if (c == 't') { /* Support legacy t-vars by mapping to k subtypes */
    return 'k';
  }
  if (strchr("akiBbfSt", c) != NULL)
    return (c);
  else
    return ('?');
}

/* For diagnostics map file name or macro name to an index */
uint8_t file_to_int(CSOUND *csound, const char *name) {
  // extern char *strdup(const char *);
  uint8_t n = 0;
  char **filedir = csound->filedir;
  while (n < 255 && filedir[n] && n < 255) { /* Do we have it already? */
    if (strcmp(filedir[n], name) == 0)
      return n; /* yes */
    n++;        /* look again */
  }
  // Not there so add
  // ensure long enough?
  if (UNLIKELY(n == 255)) {
    filedir[n] = cs_strdup(csound, Str("**unrecorded**"));
  } else {
    filedir[n] = cs_strdup(csound, (char *)name);
    filedir[n + 1] = NULL;
  }
  return n;
}

void debugPrintCsound(CSOUND *csound) {
  INSTRTXT *current;
  CONS_CELL *val = cs_hash_table_keys(csound, csound->engineState.stringPool);
  CONS_CELL *const_val =
      cs_hash_table_values(csound, csound->engineState.constantsPool);
  int count = 0;
  csound->Message(csound, "Compile State:\n");
  csound->Message(csound, "String Pool:\n");

  while (val != NULL) {
    csound->Message(csound, "    %d) %s\n", count++, (char *)val->value);
    val = val->next;
  }

  csound->Message(csound, "Constants Pool:\n");
  while (const_val != NULL) {
    CS_VAR_MEM *mem = (CS_VAR_MEM *)const_val->value;
    csound->Message(csound, "  %d) %f\n", count++, mem->value);
    const_val = const_val->next;
  }

  csound->Message(csound, "Global Variables:\n");
  CS_VARIABLE *gVar = csound->engineState.varPool->head;
  count = 0;
  while (gVar != NULL) {
    csound->Message(csound, "  %d) %s:%s\n", count++, gVar->varName,
                    gVar->varType->varTypeName);
    gVar = gVar->next;
  }

  /* bad practice to declare variables in middle of block */
  current = &(csound->engineState.instxtanchor);
  current = current->nxtinstxt;
  count = 0;
  while (current != NULL) {
    csound->Message(csound, "Instrument %d %p %p\n", count, current,
                    current->nxtinstxt);
    csound->Message(csound, "Variables\n");

    if (current->varPool != NULL) {
      CS_VARIABLE *var = current->varPool->head;
      int index = 0;
      while (var != NULL) {
        if (var->varType == &CS_VAR_TYPE_ARRAY) {
          csound->Message(csound, "  %d) %s:[%s]\n", index++, var->varName,
                          var->subType->varTypeName);
        } else {
          csound->Message(csound, "  %d) %s:%s\n", index++, var->varName,
                          var->varType->varTypeName);
        }

        var = var->next;
      }
    }
    count++;
    current = current->nxtinstxt;
  }
}

#include "interlocks.h"
void query_deprecated_opcode(CSOUND *csound, ORCTOKEN *o) {
    char *name = o->lexeme;
    OENTRY *ep = find_opcode(csound, name);
    if (UNLIKELY((ep->flags &_QQ) &&
                 (csound->oparms_.msglevel&CS_NOQQ)==CS_NOQQ))
      csound->Warning(csound, Str("Opcode \"%s\" is deprecated\n"), name);
}

int query_reversewrite_opcode(CSOUND *csound, ORCTOKEN *o) {
    char *name = o->lexeme;
    OENTRY *ep = find_opcode(csound, name);
    return (ep->flags & WI);
}
