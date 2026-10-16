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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "csoundCore.h"
#include "csound_orc.h"
#include "corfile.h"
#include "score_param.h"
#include "csound_orc_semantics.h"
#include "new_orc_parser.h"
#include "csmodule.h"
void add_opcode_defs(CSOUND *csound);

#if defined(HAVE_DIRENT_H)
#  include <dirent.h>
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <io.h>
#  include <direct.h>
#endif

uint64_t make_location(PRE_PARM *qq)
{
    int32_t d = qq->depth;
    uint64_t loc = 0;
    int32_t n = (d>8?d-7:0);
    for (; n<=d; n++) {
      loc = (loc<<8)+(qq->lstack[n]);
    }
    return loc;
}

uint64_t make_slocation(PRS_PARM *qq)
{
    int32_t d = qq->depth;
    uint64_t loc = 0;
    int32_t n = (d>8?d-7:0);
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
      //printf(Str("** found CS_UDO_DIR=%s\n"), dir);
      if (udo) {
        struct dirent *f;
        //printf("**and it opens\n");
        strcpy(buff, "#line 0\n");
        while ((f = readdir(udo)) != NULL) {
          char *fname = &(f->d_name[0]);
          int32_t n = (int)strlen(fname);
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

static void free_preprocessor(CSOUND *csound, PRE_PARM *qq)
{
    if (qq->yyscanner != NULL) {
      csound_pre_delete_buffers(qq->yyscanner);
      csound_prelex_destroy(qq->yyscanner);
    }
    while (qq->ifdefStack != NULL) {
      IFDEFSTACK *entry = qq->ifdefStack;
      qq->ifdefStack = entry->prv;
      csound->Free(csound, entry);
    }
    if (qq->pending_macro != NULL) {
      MACRO *macro = qq->pending_macro;
      csound->Free(csound, macro->name);
      csound->Free(csound, macro->body);
      for (int32_t i = 0; i < macro->acnt; ++i)
        csound->Free(csound, macro->arg[i]);
      csound->Free(csound, macro);
    }
    csound->Free(csound, qq->macro_name_buffer);
    csound_pre_free_macro_args(csound, qq->pending_arguments,
                              qq->pending_argument_count);
    for (uint32_t i = qq->macro_stack_ptr; i > 0; --i)
      csound_pre_free_macro_args(csound, qq->alt_stack[i - 1].s,
                                qq->alt_stack[i - 1].n);
    /* Includes save their previous path in the macro stack. Compare the
       borrowed root by identity: an include may also allocate ".". */
    if (qq->path != NULL && qq->path != qq->initial_path)
      free(qq->path);
    for (uint32_t i = 0; i < qq->macro_stack_ptr; ++i) {
      char *path = qq->alt_stack[i].path;
      if (path != NULL && path != qq->initial_path)
        free(path);
    }
    csound->Free(csound, qq->alt_stack);
    corfile_rm(csound, &csound->orchstr);
}

static int32_t preprocess_orc(CSOUND *csound, const char *str, PRE_PARM *qq)
{
      /* A failed live edit must not carry errors into the next request. */
      csound->synterrcnt = 0;
      //csp_orc_sa_print_list(csound);
      csound_prelex_init(&qq->yyscanner);
      csound_preset_extra(qq, qq->yyscanner);
      qq->line = csound->orcLineOffset;
      csound->expanded_orc = corfile_create_w(csound);
      file_to_int(csound, "**unknown**");
      if (str==NULL) {
        char bb[80];
        if (UNLIKELY(csound->orchstr==NULL && !csound->oparms->daemon))
          csound->Die(csound,
                      Str("parser: failed to open input file %s\n"),
                      csound->orchname);
        else if (csound->orchstr==NULL && csound->oparms->daemon)  return CSOUND_ERROR;

        add_include_udo_dir(csound, csound->orchstr);
        if (csound->orchname==NULL ||
            csound->orchname[0]=='\0') csound->orchname = csound->csdname;
        /* We know this is the start so stack is empty so far */
        snprintf(bb, 80, "#source %d\n",
                qq->lstack[0] = file_to_int(csound, csound->orchname));
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
      if(csoundGetDebug(csound) & DEBUG_PARSER)
	csoundMessage(csound, "Calling preprocess on:\n %s \n",
              corfile_body(csound->orchstr));
      cs_init_math_constants_macros(csound);
      cs_init_omacros(csound, csound->omacros);
      csound_prelex(csound, qq->yyscanner);
      if (UNLIKELY(qq->ifdefStack != NULL)) {
        csound->Message(csound, Str("Unmatched #ifdef or #ifndef\n"));
        csound->LongJmp(csound, 1);
      }
      if(csoundGetDebug(csound) & DEBUG_PARSER)
	csoundMessage(csound, "preprocessing result: \n %s\n",
                       corfile_body(csound->expanded_orc));
    /* Some WASI file errors return instead of jumping. Reject them before
       the parser replaces synterrcnt with its own error count. */
    return csound->synterrcnt ? CSOUND_ERROR : CSOUND_SUCCESS;
}

static int32_t preprocess_orc_scoped(CSOUND *csound, const char *str)
{
#ifdef CSOUND_WASI_REALTIME
    CSOUND_WASI_COMPILE_JUMP *parent = csoundWasiCompileJumpContext(csound);
    if (parent != NULL && parent->enabled) {
      /* Keep resources in heap storage: automatic variables modified after
         setjmp have indeterminate values after a jump. Recover while the
         scanner and partial macro definition still have an owner. */
      PRE_PARM *qq = csound->Calloc(csound, sizeof(PRE_PARM));
      CSOUND_WASI_COMPILE_JUMP context;
      int32_t result;
      context.csound = csound;
      context.enabled = 1;
      csoundWasiSwapCompileJumpContext(&context);
      if (setjmp(context.target)) {
        result = CSOUND_ERROR;
        csound->synterrcnt = 0;
      } else {
        result = preprocess_orc(csound, str, qq);
      }
      csoundWasiSwapCompileJumpContext(parent);
      free_preprocessor(csound, qq);
      csound->Free(csound, qq);
      if (result != CSOUND_SUCCESS)
        corfile_rm(csound, &csound->expanded_orc);
      return result;
    }
#endif
    PRE_PARM qq = {0};
    int32_t result = preprocess_orc(csound, str, &qq);
    free_preprocessor(csound, &qq);
    if (result != CSOUND_SUCCESS)
      corfile_rm(csound, &csound->expanded_orc);
    return result;
}

TREE *csoundParseOrc(CSOUND *csound, const char *str)
{
    int32_t err;
    if (UNLIKELY(csoundLoadRequestedPlugins(csound) != CSOUND_SUCCESS))
      return NULL;
    add_opcode_defs(csound);  // add global OpcodeDef variables
    if (preprocess_orc_scoped(csound, str) != CSOUND_SUCCESS)
      return NULL;
    {
      /* VL 15.3.2015 allocating memory here will cause
         unwanted growth.
         We just pass a pointer, which will be allocated
         by make leaf */
      TREE* astTree = NULL;
      TREE* newRoot;
      PARSE_PARM  pp;
      TYPE_TABLE* typeTable = NULL;
      int32_t hadStatements = 0;

      /* Parse */
      memset(&pp, '\0', sizeof(PARSE_PARM));

      csound_orclex_init(&pp.yyscanner);

      csound_orcset_extra(&pp, pp.yyscanner);
      csound_orc_scan_buffer(corfile_body(csound->expanded_orc),
                             corfile_tell(csound->expanded_orc), pp.yyscanner);
      err = csound_orcparse(&pp, pp.yyscanner, csound, &astTree);
      corfile_rm(csound, &csound->expanded_orc);
#ifdef PARCS
      if (UNLIKELY(csoundGetDebug(csound) > 99)) csp_orc_sa_print_list(csound);
#endif
      if (UNLIKELY(csound->synterrcnt)) err = 3;
      if (LIKELY(err == 0)) {
        if (csoundGetDebug(csound) & DEBUG_PARSER)
	  csound->Message(csound,Str("Parsing successful!\n"));
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
          csoundErrorMsg(csound, Str("Parsing failed due to %d syntax error%s\n"),
                         csound->synterrcnt, csound->synterrcnt==1?"":"s");
        }
        goto ending;
      }
      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_PARSER)) {
        print_tree(csound, "AST - INITIAL\n", astTree);
      }

      // EARLY two-phase struct processing: must happen before any variable declarations
      // This ensures struct types are available when parsing variable declarations like john:Person
      {
        extern int32_t process_struct_definitions_two_phase(CSOUND* csound, TREE* structDefList);

        // Create a separate list of wrapper nodes to avoid mutating the original AST
        TREE* structList = NULL;
        TREE* structTail = NULL;
        TREE* scan = astTree;

        while (scan != NULL) {
          if (scan->type == STRUCT_TOKEN) {
            // Create a wrapper node that references the struct node without modifying its next pointer
            TREE* wrapper = (TREE*)csound->Malloc(csound, sizeof(TREE));
            if (UNLIKELY(wrapper == NULL)) {
              csound->ErrorMsg(csound, Str("Memory allocation failed for struct wrapper node\n"));
              err = 3;

              // Clean up any already allocated wrapper nodes before jumping to ending
              TREE* current = structList;
              while (current != NULL) {
                TREE* next = current->next;
                csound->Free(csound, current);
                current = next;
              }
              structList = NULL;

              goto ending;
            }

            // Initialize wrapper with minimal information needed for processing
            wrapper->type = scan->type;
            wrapper->value = scan->value;
            wrapper->rate = scan->rate;
            wrapper->len = scan->len;
            wrapper->line = scan->line;
            wrapper->locn = scan->locn;
            wrapper->left = scan->left;
            wrapper->right = scan->right;
            wrapper->markup = scan->markup;
            wrapper->next = NULL;  // Initialize next to NULL

            // Add wrapper to struct list
            if (structList == NULL) {
              structList = wrapper;
              structTail = wrapper;
            } else {
              structTail->next = wrapper;
              structTail = wrapper;
            }
          }
          scan = scan->next;
        }

        // Process all struct definitions in two phases
        if (structList != NULL) {
          if (!process_struct_definitions_two_phase(csound, structList)) {
            csound->ErrorMsg(csound, Str("Error in early two-phase struct processing\n"));
            err = 3;

            // Clean up wrapper nodes before exiting
            TREE* current = structList;
            while (current != NULL) {
              TREE* next = current->next;
              csound->Free(csound, current);
              current = next;
            }
            goto ending;
          }

          // Clean up wrapper nodes after successful processing
          TREE* current = structList;
          while (current != NULL) {
            TREE* next = current->next;
            csound->Free(csound, current);
            current = next;
          }
        }
      }

      typeTable = csound->Malloc(csound, sizeof(TYPE_TABLE));
      typeTable->udos = NULL;

      typeTable->globalPool = csoundCreateVarPool(csound);
      if (typeTable->globalPool == NULL) {
        csound->ErrorMsg(csound, Str("Failed to create globalPool in parser\n"));
        csound->Free(csound, typeTable);
        err = 3;
        goto ending;
      }


      typeTable->instr0LocalPool = csoundCreateVarPool(csound);
      if (typeTable->instr0LocalPool == NULL) {
        csound->ErrorMsg(csound, Str("Failed to create instr0LocalPool in parser\n"));
        csoundFreeVarPool(csound, typeTable->globalPool);
        csound->Free(csound, typeTable);
        err = 3;
        goto ending;
      }

      typeTable->localPool = typeTable->instr0LocalPool;
      typeTable->labelList = NULL;

      /* Empty input parses to no statements, which is valid: the compiler is
         then given just the sentinel root carrying the TYPE_TABLE and can
         still create instr0 and initialise the system constants. Verification
         returns NULL both for that case and for failure, so remember whether
         there was anything to verify rather than inferring it from the result.
         synterrcnt alone is not sufficient: some verify_tree failures, such
         as a rejected struct definition, report through ErrorMsg only and
         leave synterrcnt unchanged. */
      hadStatements = (astTree != NULL);
      astTree = verify_tree(csound, astTree, typeTable);

      if (UNLIKELY(csound->synterrcnt || (hadStatements && astTree == NULL))) {
        err = 3;
        if (astTree)
          csound->Message(csound,
                          Str("Parsing failed due to %d semantic error%s!, line %d\n"),
                          csound->synterrcnt, csound->synterrcnt==1?"":"s", astTree->line);
        else if (csound->synterrcnt)
          csoundErrorMsg(csound, Str("Parsing failed due to syntax errors\n"));
        else
          csoundErrorMsg(csound, Str("Parsing failed due to semantic errors\n"));
        goto ending;
      }
      err = 0;

      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_PARSER)) {
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
      /* Prepend a sentinel node to the statement list to carry the
         TYPE_TABLE across to the compilation step, which receives only the
         tree root and reads the table back from root->markup. The node is
         type 0 with no value, which is how csound_compile_tree recognises and
         skips it. For empty input astTree is NULL, so this node is the only
         one in the list and carries the table by itself. */
      newRoot = make_leaf(csound, 0, 0, 0, NULL);
      newRoot->markup = typeTable;
      newRoot->next = astTree;

      return newRoot;
    }
}
