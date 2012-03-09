/*
    new_orc_parser.c:

    Copyright (C) 2006
    Steven Yi
    Modifications 2009 by Christopher Wilson for multicore

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
#include "csound_orcparse.h"
#include "csound_orc.h"
#include "parse_param.h"
#include "corfile.h"

#define ST(x)   (((RDORCH_GLOBALS*) csound->rdorchGlobals)->x)

extern void csound_orcrestart(FILE*, void *);

extern int csound_orcdebug;

extern void print_csound_predata(void *);
extern void csound_prelex_init(void *);
extern void csound_preset_extra(void *, void *);

extern void csound_prelex(CSOUND*, void*);
extern void csound_prelex_destroy(void *);

extern void csound_orc_scan_buffer (const char *, size_t, void*);
extern int csound_orcparse(PARSE_PARM *, void *, CSOUND*, TREE*);
extern void csound_orclex_init(void *);
extern void csound_orcset_extra(void *, void *);
//extern void csound_orc_scan_string(char *, void *);
extern void csound_orcset_lineno(int, void*);
extern void csound_orclex_destroy(void *);
extern void init_symbtab(CSOUND*);
extern void print_tree(CSOUND *, char *, TREE *);
extern TREE* verify_tree(CSOUND *, TREE *);
extern TREE *csound_orc_expand_expressions(CSOUND *, TREE *);
extern TREE* csound_orc_optimize(CSOUND *, TREE *);
extern void csound_orc_compile(CSOUND *, TREE *);
#ifdef PARCS
extern TREE *csp_locks_insert(CSOUND *csound, TREE *root);
void csp_locks_cache_build(CSOUND *);
void csp_weights_calculate(CSOUND *, TREE *);
#endif


void csound_print_preextra(CSOUND *csound, PRE_PARM  *x)
{
    csound->DebugMsg(csound,"********* Extra Pre Data %p *********\n", x);
    csound->DebugMsg(csound,"macros = %p, macro_stack_ptr = %u, ifdefStack=%p, isIfndef=%d\n"
           "isInclude=%d, clearBufferAfterEOF=%d, line=%d\n",
           x->macros, x->macro_stack_ptr, x->ifdefStack, x->isIfndef,
           x->isInclude, x->clearBufferAfterEOF, x->line);
    csound->DebugMsg(csound,"******************\n");
}

uint32_t make_location(PRE_PARM *qq)
{
    int d = qq->depth;
    uint32_t loc = 0;
    int n = (d>6?d-5:0);
    for (; n<=d; n++) {
      loc = (loc<<6)+(qq->lstack[n]);
    }
    return loc;
}

int new_orc_parser(CSOUND *csound)
{
    int retVal;
    OPARMS *O = csound->oparms;
    {
      PRE_PARM    qq;
      /* Preprocess */
      memset(&qq, 0, sizeof(PRE_PARM));
      csound_prelex_init(&qq.yyscanner);
      csound_preset_extra(&qq, qq.yyscanner);
      qq.line = csound->orcLineOffset;
      csound->expanded_orc = corfile_create_w();
      {
        char bb[80];
        file_to_int(csound, "**unknown**");
        if (csound->orchname==NULL ||
            csound->orchname[0]=='\0') csound->orchname = csound->csdname;
        /* We know this is the start so stack is empty so far */
        sprintf(bb, "#source %d\n",
                qq.lstack[0] = file_to_int(csound, csound->orchname));
        corfile_puts(bb, csound->expanded_orc);
        sprintf(bb, "#line %d\n", csound->orcLineOffset);
        corfile_puts(bb, csound->expanded_orc);
      }
      csound->DebugMsg(csound, "Calling preprocess on >>%s<<\n",
              corfile_body(csound->orchstr));
      //csound->DebugMsg(csound,"FILE: %s \n", csound->orchstr->body);
      //    csound_print_preextra(&qq);
      cs_init_math_constants_macros(csound, &qq);
      cs_init_omacros(csound, &qq, csound->omacros);
      //    csound_print_preextra(&qq);
      csound_prelex(csound, qq.yyscanner);
      if (UNLIKELY(qq.ifdefStack != NULL)) {
        csound->Message(csound, Str("Unmatched #ifdef\n"));
        csound->LongJmp(csound, 1);
      }
      csound_prelex_destroy(qq.yyscanner);
      csound->DebugMsg(csound, "yielding >>%s<<\n", corfile_body(csound->expanded_orc));
      corfile_rm(&csound->orchstr);
    }
    {
      TREE* astTree = (TREE *)mcalloc(csound, sizeof(TREE));
      PARSE_PARM  pp;
      /* Parse */
      memset(&pp, '\0', sizeof(PARSE_PARM));
      init_symbtab(csound);

      csound_orcdebug = O->odebug;
      csound_orclex_init(&pp.yyscanner);

      csound_orcset_extra(&pp, pp.yyscanner);
      csound_orc_scan_buffer(corfile_body(csound->expanded_orc),
                             corfile_tell(csound->expanded_orc), pp.yyscanner);
      //csound_orcset_lineno(csound->orcLineOffset, pp.yyscanner);
      retVal = csound_orcparse(&pp, pp.yyscanner, csound, astTree);
      corfile_rm(&csound->expanded_orc);
      if (csound->synterrcnt) retVal = 3;
      if (LIKELY(retVal == 0)) {
        csound->Message(csound, "Parsing successful!\n");
      }
      else {
        if (retVal == 1){
          csound->Message(csound, "Parsing failed due to invalid input!\n");
        }
        else if (retVal == 2){
          csound->Message(csound, "Parsing failed due to memory exhaustion!\n");
        }
        else if (retVal == 3){
          csound->Message(csound, "Parsing failed due to %d syntax error%s!\n",
                          csound->synterrcnt, csound->synterrcnt==1?"":"s");
        }
        goto ending;
      }
      if (UNLIKELY(PARSER_DEBUG)) {
        print_tree(csound, "AST - INITIAL\n", astTree);
      }
      //print_tree(csound, "AST - INITIAL\n", astTree);
      astTree = verify_tree(csound, astTree);
      //print_tree(csound, "AST - FOLDED\n", astTree);
#ifdef PARCS
      if (LIKELY(O->numThreads > 1)) {
        /* insert the locks around global variables before expr expansion */
        astTree = csp_locks_insert(csound, astTree);
        csp_locks_cache_build(csound);
      }
#endif /* PARCS */

      astTree = csound_orc_expand_expressions(csound, astTree);

      if (UNLIKELY(PARSER_DEBUG)) {
        print_tree(csound, "AST - AFTER EXPANSION\n", astTree);
      }
#ifdef PARCS
      if (LIKELY(O->numThreads > 1)) {
        /* calculate the weights for the instruments */
        csp_weights_calculate(csound, astTree);
      }
#endif /* PARCS */

      astTree = csound_orc_optimize(csound, astTree);
      csound_orc_compile(csound, astTree);

    ending:
      csound_orclex_destroy(pp.yyscanner);
    }
    return retVal;
}
