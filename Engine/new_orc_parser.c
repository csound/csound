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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include "csound_orc.h"
#include "corfile.h"
#include "score_param.h"

#if defined(HAVE_DIRENT_H)
#  include <dirent.h>
#  if 0 && defined(__MACH__)
typedef void*   DIR;
DIR             opendir(const char *);
struct dirent   *readdir(DIR*);
int             closedir(DIR*);
#  endif
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <io.h>
#  include <direct.h>
#endif

extern void csound_orcrestart(FILE*, void *);

extern int csound_orcdebug;

extern void print_csound_predata(void *);
extern int csound_prelex_init(void *);
extern void csound_preset_extra(void *, void *);

extern int csound_prelex(CSOUND*, void*);
extern int csound_prelex_destroy(void *);

extern int csound_orc_scan_buffer (const char *, size_t, void*);
extern int csound_orcparse(PARSE_PARM *, void *, CSOUND*, TREE**);
extern int csound_orclex_init(void *);
extern void csound_orcset_extra(void *, void *);
extern void csound_orcset_lineno(int, void*);
extern int csound_orclex_destroy(void *);
extern void init_symbtab(CSOUND*);
extern void print_tree(CSOUND *, char *, TREE *);
extern TREE* verify_tree(CSOUND *, TREE *, TYPE_TABLE*);
extern TREE *csound_orc_expand_expressions(CSOUND *, TREE *);
extern TREE* csound_orc_optimize(CSOUND *, TREE *);
//extern void csp_orc_analyze_tree(CSOUND* csound, TREE* root);
extern void csp_orc_sa_print_list(CSOUND*);

#if 0
static void csound_print_preextra(CSOUND *csound, PRE_PARM  *x)
{
    csound->DebugMsg(csound,"********* Extra Pre Data %p *********\n", x);
    csound->DebugMsg(csound,"macros = %p, macro_stack_ptr = %u, ifdefStack=%p,\n"
           "isIfndef=%d\n, line=%d\n",
           x->macros, x->macro_stack_ptr, x->ifdefStack, x->isIfndef, x->line);
    csound->DebugMsg(csound,"******************\n");
}
#endif

uint64_t make_location(PRE_PARM *qq)
{
    int d = qq->depth;
    uint64_t loc = 0;
    int n = (d>8?d-7:0);
    for (; n<=d; n++) {
      loc = (loc<<8)+(qq->lstack[n]);
    }
    return loc;
}

uint64_t make_slocation(PRS_PARM *qq)
{
    int d = qq->depth;
    uint64_t loc = 0;
    int n = (d>8?d-7:0);
    for (; n<=d; n++) {
      loc = (loc<<8)+(qq->lstack[n]);
    }
    return loc;
}

// Code to add #includes of UDOs
static void add_include_udo_dir(CSOUND *csound, CORFIL *xx)
{
#if defined(HAVE_DIRENT_H)
    char *dir = getenv("CS_UDO_DIR");
    char buff[1024];
    if (dir) {
      DIR *udo = opendir(dir);
      printf(Str("** found CS_UDO_DIR=%s\n"), dir);
      if (udo) {
        struct dirent *f;
        //printf("**and it opens\n");
        strcpy(buff, "#line 0\n");
        while ((f = readdir(udo)) != NULL) {
          char *fname = &(f->d_name[0]);
          int n = (int)strlen(fname);
          //printf("**  name=%s n=%d\n", fname, n);
          if (n>4 && (strcmp(&fname[n-4], ".udo")==0)) {
            strlcat(buff, "#include \"", 1024);
            strlcat(buff, dir, 1024);
            strlcat(buff, "/", 1024);
            strlcat(buff, fname, 1024);
            strlcat(buff, "\"\n", 1024);
            if (strlen(buff)>768) {
              corfile_preputs(csound, buff, xx);
              buff[0] ='\0';
            }
          }
        }
        closedir(udo);
        strlcat(buff, "###\n", 1024);
        corfile_preputs(csound, buff, xx);
      }
    }
    //printf("Giving\n%s", corfile_body(xx));
#endif
}

TREE *csoundParseOrc(CSOUND *csound, const char *str)
{
    int err;
    OPARMS *O = csound->oparms;
    csound->parserNamedInstrFlag = 2;
    {
      PRE_PARM    qq;
      /* Preprocess */
      memset(&qq, 0, sizeof(PRE_PARM));
      //csp_orc_sa_print_list(csound);
      csound_prelex_init(&qq.yyscanner);
      csound_preset_extra(&qq, qq.yyscanner);
      qq.line = csound->orcLineOffset;
      csound->expanded_orc = corfile_create_w(csound);
      file_to_int(csound, "**unknown**");
      if (str==NULL) {
        char bb[80];
        if (UNLIKELY(csound->orchstr==NULL && !csound->oparms->daemon))
          csound->Die(csound,
                      Str("parser: failed to open input file %s\n"),
                      csound->orchname);
        else if (csound->orchstr==NULL && csound->oparms->daemon)  return NULL;

        add_include_udo_dir(csound, csound->orchstr);
        if (csound->orchname==NULL ||
            csound->orchname[0]=='\0') csound->orchname = csound->csdname;
        /* We know this is the start so stack is empty so far */
        snprintf(bb, 80, "#source %d\n",
                qq.lstack[0] = file_to_int(csound, csound->orchname));
        corfile_puts(csound, bb, csound->expanded_orc);
        snprintf(bb, 80, "#line %d\n", csound->orcLineOffset);
        corfile_puts(csound, bb, csound->expanded_orc);
      }
      else {
        char bb[80];
        if (csound->orchstr == NULL ||
            corfile_body(csound->orchstr) == NULL)
          csound->orchstr = corfile_create_w(csound);
        else
          corfile_reset(csound->orchstr);
        snprintf(bb, 80, "#line %d\n", csound->orcLineOffset);
        corfile_puts(csound, bb, csound->orchstr);
        corfile_puts(csound, str, csound->orchstr);
        corfile_puts(csound, "\n#exit\n", csound->orchstr);
        corfile_putc(csound, '\0', csound->orchstr);
        corfile_putc(csound, '\0', csound->orchstr);
      }

      csound->DebugMsg(csound, "Calling preprocess on >>%s<<\n",
              corfile_body(csound->orchstr));
      //csound->DebugMsg(csound,"FILE: %s\n", csound->orchstr->body);
      //    csound_print_preextra(&qq);
      cs_init_math_constants_macros(csound);
      cs_init_omacros(csound, csound->omacros);
      //    csound_print_preextra(&qq);
      csound_prelex(csound, qq.yyscanner);
      if (UNLIKELY(qq.ifdefStack != NULL)) {
        csound->Message(csound, Str("Unmatched #ifdef or #ifndef\n"));
        csound->LongJmp(csound, 1);
      }
      csound_prelex_destroy(qq.yyscanner);
      csound->DebugMsg(csound, "yielding >>%s<<\n",
                       corfile_body(csound->expanded_orc));
      corfile_rm(csound, &csound->orchstr);

    }
    {
      /* VL 15.3.2015 allocating memory here will cause
         unwanted growth.
         We just pass a pointer, which will be allocated
         by make leaf */
      TREE* astTree = NULL;
      TREE* newRoot;
      PARSE_PARM  pp;
      TYPE_TABLE* typeTable = NULL;

      /* Parse */
      memset(&pp, '\0', sizeof(PARSE_PARM));
      init_symbtab(csound);

      csound_orcdebug = O->odebug;
      csound_orclex_init(&pp.yyscanner);


      csound_orcset_extra(&pp, pp.yyscanner);
      csound_orc_scan_buffer(corfile_body(csound->expanded_orc),
                             corfile_tell(csound->expanded_orc), pp.yyscanner);

      //csound_orcset_lineno(csound->orcLineOffset, pp.yyscanner);
      //printf("%p\n", astTree);
      err = csound_orcparse(&pp, pp.yyscanner, csound, &astTree);
      //printf("%p\n", astTree);
      //print_tree(csound, "AST - AFTER csound_orcparse()\n", astTree);
      //csp_orc_sa_cleanup(csound);
      corfile_rm(csound, &csound->expanded_orc);
#ifdef PARCS
      if (UNLIKELY(csound->oparms->odebug)) csp_orc_sa_print_list(csound);
#endif
      if (UNLIKELY(csound->synterrcnt)) err = 3;
      if (LIKELY(err == 0)) {
        if (csound->oparms->odebug) csound->Message(csound,
                                                    Str("Parsing successful!\n"));
      }
      else {
        if (err == 1){
          csoundErrorMsg(csound, Str("Parsing failed due to invalid input!\n"));
        }
        else if (err == 2){
          csoundErrorMsg(csound,
                          Str("Parsing failed due to memory exhaustion!\n"));
        }
        else if (err == 3){
          csoundErrorMsg(csound, Str("Parsing failed due to %d syntax error%s!\n"),
                          csound->synterrcnt, csound->synterrcnt==1?"":"s");
        }
        goto ending;
      }
      if (UNLIKELY(PARSER_DEBUG)) {
        print_tree(csound, "AST - INITIAL\n", astTree);
      }

      typeTable = csound->Malloc(csound, sizeof(TYPE_TABLE));
      typeTable->udos = NULL;

      typeTable->globalPool = csoundCreateVarPool(csound);
      typeTable->instr0LocalPool = csoundCreateVarPool(csound);

      typeTable->localPool = typeTable->instr0LocalPool;
      typeTable->labelList = NULL;

      astTree = verify_tree(csound, astTree, typeTable);
//      csound->Free(csound, typeTable->instr0LocalPool);
//      csound->Free(csound, typeTable->globalPool);
//      csound->Free(csound, typeTable);
      //print_tree(csound, "AST - FOLDED\n", astTree);

      if (UNLIKELY(astTree == NULL || csound->synterrcnt)) {
        err = 3;
        if (astTree)
          csound->Message(csound,
                          Str("Parsing failed due to %d semantic error%s!\n"),
                          csound->synterrcnt, csound->synterrcnt==1?"":"s");
        else if (csound->synterrcnt)
          csoundErrorMsg(csound, Str("Parsing failed due to syntax errors\n"));
        else
          csoundErrorMsg(csound, Str("Parsing failed due to no input!\n"));
        goto ending;
      }
      err = 0;

      //csp_orc_analyze_tree(csound, astTree);

//      astTree = csound_orc_expand_expressions(csound, astTree);
//
      if (UNLIKELY(PARSER_DEBUG)) {
        print_tree(csound, "AST - AFTER VERIFICATION/EXPANSION\n", astTree);
      }

    ending:
      csound_orclex_destroy(pp.yyscanner);
      if (UNLIKELY(err)) {
        csound->ErrorMsg(csound, "%s", Str("Stopping on parser failure\n"));
        csoundDeleteTree(csound, astTree);
        if (typeTable != NULL) {
          csoundFreeVarPool(csound, typeTable->globalPool);
          if (typeTable->instr0LocalPool != NULL) {
            csoundFreeVarPool(csound, typeTable->instr0LocalPool);
          }
          if (typeTable->localPool != typeTable->instr0LocalPool) {
            csoundFreeVarPool(csound, typeTable->localPool);
          }
          csound->Free(csound, typeTable);
        }
        return NULL;
      }

      astTree = csound_orc_optimize(csound, astTree);
      //print_tree(csound, "AST after optmize", astTree);
      // small hack: use an extra node as head of tree list to hold the
      // typeTable, to be used during compilation
      newRoot = make_leaf(csound, 0, 0, 0, NULL);
      newRoot->markup = typeTable;
      newRoot->next = astTree;

      /* if (str!=NULL){ */
      /*        if (typeTable != NULL) { */
      /*     csoundFreeVarPool(csound, typeTable->globalPool); */
      /*     if (typeTable->instr0LocalPool != NULL) { */
      /*       csoundFreeVarPool(csound, typeTable->instr0LocalPool); */
      /*     } */
      /*     if (typeTable->localPool != typeTable->instr0LocalPool) { */
      /*       csoundFreeVarPool(csound, typeTable->localPool); */
      /*     } */
      /*     csound->Free(csound, typeTable); */
      /*   } */
      /* } */

      return newRoot;
    }
}
