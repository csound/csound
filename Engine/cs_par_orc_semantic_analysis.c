+/*
+    cs_par_orc_semantic_analysis.c:
+
+    Copyright (C) 2006
+
+    This file is part of Csound.
+
+    The Csound Library is free software; you can redistribute it
+    and/or modify it under the terms of the GNU Lesser General Public
+    License as published by the Free Software Foundation; either
+    version 2.1 of the License, or (at your option) any later version.
+
+    Csound is distributed in the hope that it will be useful,
+    but WITHOUT ANY WARRANTY; without even the implied warranty of
+    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+    GNU Lesser General Public License for more details.
+
+    You should have received a copy of the GNU Lesser General Public
+    License along with Csound; if not, write to the Free Software
+    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
+    02111-1307 USA
+*/
+
+#include <stdio.h>
+#include <stdlib.h>
+/*#include <string.h>
+#include <errno.h>*/
+
+#include "csoundCore.h"
+/*#include "namedins.h"*/
+#include "csound_orc.h"
+#include "tok.h"
+
+#include "cs_par_base.h"
+#include "cs_par_orc_semantic_analysis.h"
+
+
+/**********************************************************************************************
+ * static function prototypes
+ */
+/* static int csp_thread_index_get(CSOUND *csound); */
+static struct instr_semantics_t *instr_semantics_alloc(CSOUND *csound, char *name);
+
+/**********************************************************************************************
+ * helper functions
+ */
+
+static struct instr_semantics_t *instr_semantics_alloc(CSOUND *csound, char *name)
+{
+    struct instr_semantics_t *instr = csound->Malloc(csound, sizeof(struct instr_semantics_t));
+    memset(instr, 0, sizeof(struct instr_semantics_t));
+    strncpy(instr->hdr, INSTR_SEMANTICS_HDR, HDR_LEN);
+    instr->name = name;
+    instr->insno = -1;
+    
+    csp_set_alloc_string(csound, &(instr->read_write));
+    csp_set_alloc_string(csound, &(instr->write));
+    csp_set_alloc_string(csound, &(instr->read));
+    
+    return instr;
+}
+
+#if 0
+static int csp_thread_index_get(CSOUND *csound) {
+    int index = 0;
+    void *threadId = csound->GetCurrentThreadID();
+    THREADINFO *current = csound->multiThreadedThreadInfo;
+
+    /* if(current == NULL) {
+        return -2;
+    }
+
+    while(current != NULL) {
+        if (pthread_equal(*(pthread_t *)threadId, *(pthread_t *)current->threadId)) {
+            return index;
+        }
+        index++;
+        current = current->next;
+    }
+    return -1; */
+    
+    int equal = pthread_equal(*(pthread_t *)threadId, *(pthread_t *)current->threadId);
+    free(threadId);
+    return equal;
+}
+
+/* #define csp_thread_index_get(csound) 1 */
+#endif
+
+/**********************************************************************************************
+ * parse time support
+ */
+
+static struct instr_semantics_t *curr;
+static struct instr_semantics_t *root;
+
+void csp_orc_sa_cleanup(CSOUND *csound)
+{
+    struct instr_semantics_t *current = root, *h = NULL;
+    while (current != NULL) {
+        
+        csp_set_dealloc(csound, &(current->read));
+        csp_set_dealloc(csound, &(current->write));
+        csp_set_dealloc(csound, &(current->read_write));
+        
+        h = current;
+        current = current->next;
+        csound->Free(csound, h);
+    }
+
+    curr = NULL;
+    root = NULL;
+}
+
+void csp_orc_sa_print_list(CSOUND *csound)
+{
+    csound->Message(csound, "Semantic Analysis\n");
+    struct instr_semantics_t *current = root;
+    while (current != NULL) {
+        csound->Message(csound, "Instr: %s\n", current->name);
+        csound->Message(csound, "  read\n");
+        csp_set_print(csound, current->read);
+        
+        csound->Message(csound, "  write\n");
+        csp_set_print(csound, current->write);
+        
+        csound->Message(csound, "  read_write\n");
+        csp_set_print(csound, current->read_write);
+        
+        csound->Message(csound, "  weight: %u\n", current->weight);
+        
+        current = current->next;
+    }
+    csound->Message(csound, "Semantic Analysis Ends\n");
+}
+
+void csp_orc_sa_global_read_write_add_list(CSOUND *csound, struct set_t *write, struct set_t *read)
+{
+    if (curr == NULL) {
+        csound->Message(csound, "Add global read, write lists without any instruments\n");
+    } else if (write == NULL  || read == NULL) {
+        csound->Die(csound, "Invalid NULL parameter set to add to global read, write lists\n");
+    } else {
+        struct set_t *new = NULL;
+        csp_set_union(csound, write, read, &new);
+        if (write->count == 1 && read->count == 1 && new->count == 1) {
+            /* this is a read_write list thing */
+            struct set_t *new_read_write = NULL;
+            csp_set_union(csound, curr->read_write, new, &new_read_write);
+            csp_set_dealloc(csound, &curr->read_write);
+            curr->read_write = new_read_write;
+        } else {
+            csp_orc_sa_global_write_add_list(csound, write);
+            csp_orc_sa_global_read_add_list(csound, read);
+        }
+        
+        csp_set_dealloc(csound, &new);
+    }    
+}
+
+void csp_orc_sa_global_write_add_list(CSOUND *csound, struct set_t *set)
+{
+    if (curr == NULL) {
+        csound->Message(csound, "Add a global write_list without any instruments\n");
+    } else if (set == NULL) {
+        csound->Die(csound, "Invalid NULL parameter set to add to a global write_list\n");
+    } else {
+        struct set_t *new = NULL;
+        csp_set_union(csound, curr->write, set, &new);
+
+        csp_set_dealloc(csound, &curr->write);
+        csp_set_dealloc(csound, &set);
+        
+        curr->write = new;
+    }
+}
+
+void csp_orc_sa_global_read_add_list(CSOUND *csound, struct set_t *set)
+{
+    if (curr == NULL) {
+        csound->Message(csound, "add a global read_list without any instruments\n");
+    } else if (set == NULL) {
+        csound->Die(csound, "Invalid NULL parameter set to add to a global read_list\n");
+    } else {
+        struct set_t *new = NULL;
+        csp_set_union(csound, curr->read, set, &new);
+
+        csp_set_dealloc(csound, &curr->read);
+        csp_set_dealloc(csound, &set);
+        
+        curr->read = new;
+    }
+}
+
+static int inInstr = 0;
+
+void csp_orc_sa_instr_add(CSOUND *csound, char *name)
+{
+    inInstr = 1;
+    if (root == NULL) {
+        root = instr_semantics_alloc(csound, name);
+        curr = root;
+    } else if (curr == NULL) {
+        struct instr_semantics_t *prev = root;
+        curr = prev->next;
+        while (curr != NULL) {
+            prev = curr;
+            curr = curr->next;
+        }
+        prev->next = instr_semantics_alloc(csound, name);
+        curr = prev->next;
+    } else {
+        curr->next = instr_semantics_alloc(csound, name);
+        curr = curr->next;
+    }
+    // curr->insno = named_instr_find(name);
+}
+
+void csp_orc_sa_instr_finalize(CSOUND *csound)
+{
+    curr = NULL;
+    inInstr = 0;
+}
+
+struct set_t *csp_orc_sa_globals_find(CSOUND *csound, TREE *node)
+{
+    if (node == NULL) {
+        struct set_t *set = NULL;
+        csp_set_alloc_string(csound, &set);
+        return set;
+    }
+    
+    struct set_t *left  = csp_orc_sa_globals_find(csound, node->left);
+    struct set_t *right = csp_orc_sa_globals_find(csound, node->right);
+    
+    struct set_t *current_set = NULL;
+    csp_set_union(csound, left, right, &current_set);
+    
+    csp_set_dealloc(csound, &left);
+    csp_set_dealloc(csound, &right);
+    
+    switch (node->type) {
+        case T_IDENT_GI:
+        case T_IDENT_GK:
+        case T_IDENT_GF:
+        case T_IDENT_GW:
+        case T_IDENT_GS:
+        case T_IDENT_GA:
+            csp_set_add(csound, current_set, node->value->lexeme);
+            break;
+        default:
+              /* no globals */
+            break;
+    }
+    
+    if (node->next != NULL) {
+        struct set_t *prev_set = current_set;
+        struct set_t *next = csp_orc_sa_globals_find(csound, node->next);
+        csp_set_union(csound, prev_set, next, &current_set);
+        
+        csp_set_dealloc(csound, &prev_set);
+        csp_set_dealloc(csound, &next);
+    }
+
+    return current_set;
+}
+
+struct instr_semantics_t *csp_orc_sa_instr_get_by_name(char *instr_name)
+{    
+    struct instr_semantics_t *current_instr = root;
+    while (current_instr != NULL) {
+        if (strcmp(current_instr->name, instr_name) == 0) {
+            return current_instr;
+        }
+        current_instr = current_instr->next;
+    }
+    return NULL;
+}
+
+struct instr_semantics_t *csp_orc_sa_instr_get_by_num(int16 insno)
+{
+    struct instr_semantics_t *current_instr = root;
+    while (current_instr != NULL) {
+        if (current_instr->insno != -1 && current_instr->insno == insno) {
+            return current_instr;
+        }
+        current_instr = current_instr->next;
+    }
+    
+    #define BUF_LENGTH 8
+    char buf[BUF_LENGTH];
+    snprintf(buf, BUF_LENGTH, "%i", insno);
+    
+    current_instr = csp_orc_sa_instr_get_by_name(buf);
+    if (current_instr != NULL) {
+        current_instr->insno = insno;
+    }
+    return current_instr;
+}
