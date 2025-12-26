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
#include "csound_module.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"

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

TREE *csoundParseOrc(CSOUND *csound, const char *str)
{
    int32_t err;
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
            csound->orchname[0]=='\0') {
          /* Duplicate csdname instead of just pointing to it, to avoid
           * dangling pointer if csdname gets freed during recompilation */
          if (csound->csdname != NULL) {
            csound->orchname = cs_strdup(csound, csound->csdname);
          }
        }
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
      if(csoundGetDebug(csound) & DEBUG_PARSER)
	csoundMessage(csound, "Calling preprocess on:\n %s \n",
              corfile_body(csound->orchstr));
      cs_init_math_constants_macros(csound);
      cs_init_omacros(csound, csound->omacros);
      csound_prelex(csound, qq.yyscanner);
      if (UNLIKELY(qq.ifdefStack != NULL)) {
        csound->Message(csound, Str("Unmatched #ifdef or #ifndef\n"));
        csound->LongJmp(csound, 1);
      }
      csound_prelex_destroy(qq.yyscanner);
      if(csoundGetDebug(csound) & DEBUG_PARSER)
	csoundMessage(csound, "preprocessing result: \n %s\n",
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
          csoundErrorMsg(csound, Str("Parsing failed due to %d syntax error%s!, line %d\n"),
                          csound->synterrcnt, csound->synterrcnt==1?"":"s", astTree->line);
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
              csound->ErrorMsg(csound, "Memory allocation failed for struct wrapper node\n");
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
            csound->ErrorMsg(csound, "Error in early two-phase struct processing\n");
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

      /* Get current module's varPool for g-variables and @global variables
       * For main CSD, this will be root_module->varPool
       * For imported modules, this will be the module's varPool */
      MODULE_STATE *module_state = csoundGetModuleState(csound);

      if (module_state->current_module && module_state->current_module->varPool) {
        typeTable->globalPool = module_state->current_module->varPool;
      } else {
        /* Fallback: create a new pool if no current module (shouldn't happen) */
        typeTable->globalPool = csoundCreateVarPool(csound);
      }

      if (typeTable->globalPool == NULL) {
        csound->ErrorMsg(csound, "Failed to create globalPool in parser\n");
        csound->Free(csound, typeTable);
        err = 3;
        goto ending;
      }

      typeTable->instr0LocalPool = csoundCreateVarPool(csound);
      if (typeTable->instr0LocalPool == NULL) {
        csound->ErrorMsg(csound, "Failed to create instr0LocalPool in parser\n");
        csoundFreeVarPool(csound, typeTable->globalPool);
        csound->Free(csound, typeTable);
        err = 3;
        goto ending;
      }

      typeTable->localPool = typeTable->instr0LocalPool;
      typeTable->labelList = NULL;

      /* Process import statements before semantic analysis
       * In module files, the root node (astTree) might be an import statement.
       * In main files, imports are in astTree->next chain.
       * We need to check both. */
      TREE *current;
      if (astTree && (astTree->type == IMPORT_TOKEN || astTree->type == FROM_TOKEN)) {
        // Root is an import - start from root
        current = astTree;
      } else {
        // Root is not an import - start from next
        current = astTree ? astTree->next : NULL;
      }
      while (current != NULL) {
        if (current->type == IMPORT_TOKEN || current->type == FROM_TOKEN) {
          /* Extract module path from the left child node */
          if (current->left && current->left->value && current->left->value->lexeme) {
            char *module_path = current->left->value->lexeme;

            CS_MODULE *module = csoundGetOrLoadModule(csound, module_path);

            if (module) {
              /* Process the import statement to register items from the loaded module */
              csoundProcessImportStatements(csound, module, current);

              /* Register placeholder globals from imported module so semantic
               * analysis of this file can resolve names (e.g., giTestValue).
               *
               * For selective imports (from X import a,b,c), only add the
               * explicitly listed items. For wildcard or plain imports, add all. */
              {
                int is_selective = 0;
                CS_HASH_TABLE *allowed_names = NULL;

                /* Check if this is a selective import (FROM_TOKEN with non-wildcard list) */
                if (current->type == FROM_TOKEN && current->right && current->right->type != '*') {
                  is_selective = 1;
                  allowed_names = cs_hash_table_create(csound);

                  /* Build a hash set of allowed names */
                  TREE *item = current->right;
                  while (item != NULL) {
                    if (item->value && item->value->lexeme) {
                      cs_hash_table_put(csound, allowed_names, item->value->lexeme, (void*)1);
                    }
                    item = item->next;
                  }
                }

                /* Walk imported module AST and add placeholders for g* identifiers */
                TREE *it = module->ast;
                while (it != NULL) {
                  if (it->type == T_ASSIGNMENT && it->left && it->left->value && it->left->value->lexeme) {
                    char *name = it->left->value->lexeme;
                    if (name && name[0] == 'g' && name[1] != '\0') {
                      /* For selective imports, only add if name is in allowed list */
                      int should_add = 1;
                      if (is_selective && allowed_names != NULL) {
                        should_add = (cs_hash_table_get(csound, allowed_names, name) != NULL);
                      }

                      if (should_add && csoundFindVariableWithName(csound, typeTable->globalPool, name) == NULL) {
                        const CS_TYPE *tp = NULL;
                        switch (name[1]) {
                          case 'i': tp = &CS_VAR_TYPE_I; break;
                          case 'k': tp = &CS_VAR_TYPE_K; break;
                          case 'a': tp = &CS_VAR_TYPE_A; break;
                          case 'S': tp = &CS_VAR_TYPE_S; break;
                          default:  tp = &CS_VAR_TYPE_I; break;
                        }
                        CS_VARIABLE *v = csoundCreateVariable(csound, csound->typePool, tp, name, NULL);
                        csoundAddVariable(csound, typeTable->globalPool, v);
                      }
                    }
                  }
                  it = it->next;
                }

                /* Free the allowed_names hash table if created */
                if (allowed_names != NULL) {
                  cs_hash_table_free(csound, allowed_names);
                }
              }
            } else {
              synterr(csound, Str("Failed to load module: %s"), module_path);
            }
          }
        }
        current = current->next;
      }

      astTree = verify_tree(csound, astTree, typeTable);

      if (UNLIKELY(astTree == NULL || csound->synterrcnt)) {
        err = 3;
        if (astTree)
          csound->Message(csound,
                          Str("Parsing failed due to %d semantic error%s!, line %d\n"),
                          csound->synterrcnt, csound->synterrcnt==1?"":"s", astTree->line);
        else if (csound->synterrcnt)
          csoundErrorMsg(csound, Str("Parsing failed due to syntax errors\n"));
        else
          csoundErrorMsg(csound, Str("Parsing failed due to no input!\n"));
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
          /* Only free globalPool if it was created as a fallback
           * (not pointing to a module's varPool) */
          MODULE_STATE *module_state = csoundGetModuleState(csound);
          if (module_state->current_module == NULL ||
              typeTable->globalPool != module_state->current_module->varPool) {
            csoundFreeVarPool(csound, typeTable->globalPool);
          }
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
      // small hack: use an extra node as head of tree list to hold the
      // typeTable, to be used during compilation
      newRoot = make_leaf(csound, 0, 0, 0, NULL);
      newRoot->markup = typeTable;
      newRoot->next = astTree;

      return newRoot;
    }
}

