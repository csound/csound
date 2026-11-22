/*
    csound_pre_shared.c:

    Shared preprocessor helpers for the orchestra and score macro preprocessors.

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

#include <string.h>
#ifndef WIN32
#include <sys/stat.h>
#endif

#include "csound_pre_shared.h"

int csound_pre_is_dir(const char *path)
{
#ifndef WIN32
    struct stat statbuf;
    if (path == NULL || stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
#else
    (void)path;
    return 0;
#endif
}

void csound_pre_free_macro(CSOUND *csound, MACRO *mm)
{
    int i;
    if (mm == NULL) {
        return;
    }
    for (i = 0; i < mm->acnt; i++) {
        csound->Free(csound, mm->arg[i]);
    }
    csound->Free(csound, mm->body);
    csound->Free(csound, mm->name);
    csound->Free(csound, mm);
}

void csound_pre_delete_macro_list(CSOUND *csound, MACRO **head)
{
    if (head == NULL) {
        return;
    }
    while (*head != NULL) {
        MACRO *next = (*head)->next;
        csound_pre_free_macro(csound, *head);
        *head = next;
    }
}

MACRO *csound_pre_find_definition(MACRO *mmo, char *s)
{
    MACRO *mm = mmo;
    size_t len;

    if (s == NULL) {
        return NULL;
    }
    len = strlen(s);
    if (len == 0) {
        return NULL;
    }
    if (s[len - 1] == '.') {
        s[len - 1] = '\0';
    } else if (len > 1 && s[len - 2] == '.' && s[len - 1] == '(') {
        s[len - 2] = '(';
        s[len - 1] = '\0';
    }

    while (mm != NULL) {
        if (strcmp(s, mm->name) == 0) {
            return mm;
        }
        mm = mm->next;
    }

    /* Fall back to stripped macro-argument prefix after "`...`" if any. */
    mm = mmo;
    if (*s++ == '\0') {
        return NULL;
    }
looking:
    while (*s != '`') {
        if (*s++ == '\0') {
            return NULL;
        }
    }
    s++;
    if (*s++ != '`') {
        s--;
        goto looking;
    }
    while (mm != NULL) {
        if (strcmp(s, mm->name) == 0) {
            return mm;
        }
        mm = mm->next;
    }
    return NULL;
}

int csound_pre_undef_macro_quiet(CSOUND *csound, MACRO **head,
                                 const char *name)
{
    MACRO **link = head;
    if (head == NULL || name == NULL) {
        return 0;
    }
    while (*link != NULL) {
        MACRO *mm = *link;
        if (strcmp(name, mm->name) == 0) {
            *link = mm->next;
            csound_pre_free_macro(csound, mm);
            return 1;
        }
        link = &(*link)->next;
    }
    return 0;
}

void csound_pre_init_cli_macros(CSOUND *csound, MACRO **head, NAMES *nn,
                                const char *opt_name)
{
    while (nn != NULL) {
        char *s = nn->mac;
        char *p = strchr(s, '=');
        const char *colon;
        char *mname;
        MACRO *mm;

        if (p == NULL) {
            p = s + strlen(s);
        }
        if (UNLIKELY(csound->oparms->msglevel & 7)) {
            csound->Message(csound, Str("Macro definition for %*s\n"),
                            (int)(p - s), s);
        }
        colon = strchr(s, ':');
        if (UNLIKELY(colon == NULL || colon + 1 >= p)) {
            csound->Die(csound, Str("Invalid macro name for %s\n"), opt_name);
        }
        s = (char *)(colon + 1);
        mname = (char *)csound->Malloc(csound, (size_t)(p - s) + 1);
        if (UNLIKELY(mname == NULL)) {
            csound->Message(csound, Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
        }
        memcpy(mname, s, (size_t)(p - s));
        mname[p - s] = '\0';

        for (mm = *head; mm != NULL; mm = mm->next) {
            if (strcmp(mm->name, mname) == 0) {
                break;
            }
        }
        if (mm == NULL) {
            mm = (MACRO *)csound->Calloc(csound, sizeof(MACRO));
            if (UNLIKELY(mm == NULL)) {
                csound->Free(csound, mname);
                csound->Message(csound, Str("Memory exhausted"));
                csound->LongJmp(csound, 1);
            }
            mm->name = mname;
            mm->next = *head;
            *head = mm;
        } else {
            int i;
            csound->Free(csound, mname);
            for (i = 0; i < mm->acnt; i++) {
                csound->Free(csound, mm->arg[i]);
                mm->arg[i] = NULL;
            }
            csound->Free(csound, mm->body);
            mm->body = NULL;
        }
        mm->margs = MARGS;
        mm->acnt = 0;
        if (*p != '\0') {
            p++;
        }
        mm->body = (char *)csound->Malloc(csound, strlen(p) + 1);
        if (UNLIKELY(mm->body == NULL)) {
            csound->Message(csound, Str("Memory exhausted"));
            csound->LongJmp(csound, 1);
        }
        strcpy(mm->body, p);
        nn = nn->next;
    }
}

IFDEFSTACK *csound_pre_push_ifdef(CSOUND *csound, IFDEFSTACK **stack,
                                  MACRO *macros, const char *name,
                                  unsigned char isIfndef)
{
    IFDEFSTACK *pp = (IFDEFSTACK *)csound->Calloc(csound, sizeof(IFDEFSTACK));
    MACRO *mm;

    if (UNLIKELY(pp == NULL)) {
        csound->Message(csound, Str("Memory exhausted"));
        csound->LongJmp(csound, 1);
    }
    pp->prv = *stack;
    pp->isDef = isIfndef;
    for (mm = macros; mm != NULL; mm = mm->next) {
        if (strcmp(name, mm->name) == 0) {
            pp->isDef ^= (unsigned char)1;
            break;
        }
    }
    *stack = pp;
    pp->isSkip = pp->isDef ^ (unsigned char)1;
    return pp;
}

void csound_pre_free_ifdef_stack(CSOUND *csound, IFDEFSTACK **stack)
{
    if (stack == NULL) {
        return;
    }
    while (*stack != NULL) {
        IFDEFSTACK *prv = (*stack)->prv;
        csound->Free(csound, *stack);
        *stack = prv;
    }
}
