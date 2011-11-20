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

//#include "yyguts.h"

#define ST(x)   (((RDORCH_GLOBALS*) csound->rdorchGlobals)->x)

extern void csound_orcrestart(FILE*, void *);

extern int csound_orcdebug;

extern int csound_orcparse(PARSE_PARM *, void *, CSOUND*, TREE*);
extern void init_symbtab(CSOUND*);
extern void print_tree(CSOUND *, char *, TREE *);
extern TREE* verify_tree(CSOUND *, TREE *);
extern TREE *csound_orc_expand_expressions(CSOUND *, TREE *);
extern TREE* csound_orc_optimize(CSOUND *, TREE *);
extern void csound_orc_compile(CSOUND *, TREE *);
#ifdef PARCS
extern TREE *csp_locks_insert(CSOUND *csound, TREE *root);
#endif


void new_orc_parser(CSOUND *csound)
{
    void *t;
    int retVal;
    TREE* astTree = (TREE *)mcalloc(csound, sizeof(TREE));
    OPARMS *O = csound->oparms;
    PARSE_PARM  pp;
    void *ttt;
    //    struct yyguts_t* yyg;

    memset(&pp, '\0', sizeof(PARSE_PARM));
    init_symbtab(csound);

    pp.buffer = (char*)csound->Malloc(csound, lMaxBuffer);

    if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Testing...\n");

    csound_orcdebug = O->odebug;
    csound_orclex_init(&pp.yyscanner);
    //    yyg = (struct yyguts_t*)pp.yyscanner;

    csound_orcset_extra(&pp, pp.yyscanner);

    csound_orc_scan_string(corfile_body(csound->orchstr), pp.yyscanner);
    /*     These relate to file input only       */
    /*     csound_orcset_in(ttt, pp.yyscanner); */
    /*     csound_orcrestart(ttt, pp.yyscanner); */
    csound_orcset_lineno(csound->orcLineOffset, pp.yyscanner);
    cs_init_math_constants_macros(csound, pp.yyscanner);
    cs_init_omacros(csound, pp.yyscanner, csound->omacros);

    retVal = csound_orcparse(&pp, pp.yyscanner, csound, astTree);

    if (LIKELY(retVal == 0)) {
      csound->Message(csound, "Parsing successful!\n");
    }
    else if (retVal == 1){
      csound->Message(csound, "Parsing failed due to invalid input!\n");
    }
    else if (retVal == 2){
      csound->Message(csound, "Parsing failed due to memory exhaustion!\n");
    }

    if (UNLIKELY(PARSER_DEBUG)) {
      print_tree(csound, "AST - INITIAL\n", astTree);
    }

    astTree = verify_tree(csound, astTree);
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

    csound->Free(csound, pp.buffer);
}

