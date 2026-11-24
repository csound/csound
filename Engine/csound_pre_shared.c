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

enum {
    ORC_SCAN_CODE,
    ORC_SCAN_QUOTED,
    ORC_SCAN_EXTENDED,
    ORC_SCAN_RAW,
    ORC_SCAN_LINE_COMMENT,
    ORC_SCAN_BLOCK_COMMENT,
    ORC_SCAN_DIRECTIVE,
    ORC_SCAN_MACRO_HEADER,
    ORC_SCAN_MACRO_HEADER_LCOM,
    ORC_SCAN_MACRO_HEADER_BCOM,
    ORC_SCAN_MACRO_BODY
};

int32_t csound_orc_scan_in_code(const ORC_DELIMITER_SCAN *scan)
{
    return scan->mode == ORC_SCAN_CODE || scan->mode == ORC_SCAN_DIRECTIVE;
}

/* The directives the scanner tells apart, matched one character at a time
   after their '#', as the lexer's patterns #[ \t]*define, #ifn?def, #else
   and #end(if)? accept them: none needs a blank after its keyword. */
enum {
    DK_NO_MATCH = 0,
    DK_START,                /* after '#', blanks may still come */
    DK_D, DK_DE, DK_DEF, DK_DEFI, DK_DEFIN,
    DK_I, DK_IF, DK_IFD, DK_IFDE, DK_IFN, DK_IFND, DK_IFNDE,
    DK_E, DK_EL, DK_ELS, DK_EN,
    DK_DEFINE, DK_IFDEF, DK_IFNDEF, DK_ELSE, DK_END   /* a whole keyword */
};

static int dk_step(int step, char c)
{
    switch (step) {
    case DK_START:
        if (c == ' ' || c == '\t') return DK_START;
        if (c == 'd') return DK_D;
        if (c == 'i') return DK_I;
        if (c == 'e') return DK_E;
        return DK_NO_MATCH;
    case DK_D:     return c == 'e' ? DK_DE : DK_NO_MATCH;
    case DK_DE:    return c == 'f' ? DK_DEF : DK_NO_MATCH;
    case DK_DEF:   return c == 'i' ? DK_DEFI : DK_NO_MATCH;
    case DK_DEFI:  return c == 'n' ? DK_DEFIN : DK_NO_MATCH;
    case DK_DEFIN: return c == 'e' ? DK_DEFINE : DK_NO_MATCH;
    case DK_I:     return c == 'f' ? DK_IF : DK_NO_MATCH;
    case DK_IF:    return c == 'd' ? DK_IFD : c == 'n' ? DK_IFN : DK_NO_MATCH;
    case DK_IFD:   return c == 'e' ? DK_IFDE : DK_NO_MATCH;
    case DK_IFDE:  return c == 'f' ? DK_IFDEF : DK_NO_MATCH;
    case DK_IFN:   return c == 'd' ? DK_IFND : DK_NO_MATCH;
    case DK_IFND:  return c == 'e' ? DK_IFNDE : DK_NO_MATCH;
    case DK_IFNDE: return c == 'f' ? DK_IFNDEF : DK_NO_MATCH;
    case DK_E:     return c == 'l' ? DK_EL : c == 'n' ? DK_EN : DK_NO_MATCH;
    case DK_EL:    return c == 's' ? DK_ELS : DK_NO_MATCH;
    case DK_ELS:   return c == 'e' ? DK_ELSE : DK_NO_MATCH;
    case DK_EN:    return c == 'd' ? DK_END : DK_NO_MATCH;
    }
    return DK_NO_MATCH;
}

/* What a whole directive does to the count of open groups: #else, in a
   group that is open either way, nothing. */
static int dk_depth_change(int directive)
{
    if (directive == DK_IFDEF || directive == DK_IFNDEF) return 1;
    return directive == DK_END ? -1 : 0;
}

static void scan_count_conditional(ORC_DELIMITER_SCAN *scan, int change)
{
    int depth = scan->condDepth + change;
    scan->condDepth = (uint16_t)(depth < 0 ? 0 :
                                 depth > UINT16_MAX ? UINT16_MAX : depth);
}

/* Where the look for a conditional directive line is, inside a string,
   comment or macro body. */
enum { CM_IDLE, CM_AT_HASH, CM_KEYWORD, CM_REST };

/* A branch of an #ifdef group that the preprocessor skips is read as plain
   lines: an R{, a block comment or a {{ in it opens nothing. The scanner
   cannot tell which branches are skipped and follows them all, so one of
   those may be left open into the next directive line. There it is taken to
   be what it is in a skipped branch, nothing, and closed, and the directive
   counted: in a branch that is compiled that line would be inside what was
   opened. */
static void csound_orc_scan_open_conditional(ORC_DELIMITER_SCAN *scan,
                                             char c, int32_t atLineStart)
{
    int step;
    switch (scan->condMatch) {
    case CM_IDLE:
      if (scan->resync && scan->condDepth > 0 && atLineStart && c == '#')
        scan->condMatch = CM_AT_HASH;
      return;
    case CM_AT_HASH:
      if (c == '\n') {
        scan->condMatch = CM_IDLE;
        return;
      }
      scan->condPending = DK_START;
      scan->condMatch = CM_KEYWORD;
      break;
    case CM_KEYWORD:
      break;
    default:                    /* CM_REST: the rest of the directive's line */
      if (c != '\n')
        return;
      scan_count_conditional(scan, scan->condPending);
      scan->condMatch = CM_IDLE;
      scan->mode = ORC_SCAN_CODE;
      scan->rawDepth = 0;
      scan->previous = '\0';
      return;
    }
    /* The keyword's step is kept in condPending until it is whole: in a
       macro header macroStep is the depth of its argument brackets. */
    step = dk_step(scan->condPending, c);
    if (step == DK_NO_MATCH) {
      scan->condMatch = CM_IDLE;
      return;
    }
    if (step < DK_DEFINE) {
      scan->condPending = (signed char) step;
      return;
    }
    scan->condPending = (signed char) dk_depth_change(step);
    scan->condMatch = CM_REST;
}

/* Whether a string, comment or macro body is open: what a skipped branch
   may have left open. A quoted string and a ; or // comment end with their
   line. */
static int32_t csound_orc_scan_open_construct(const ORC_DELIMITER_SCAN *scan)
{
    switch (scan->mode) {
    case ORC_SCAN_CODE:
    case ORC_SCAN_DIRECTIVE:
    case ORC_SCAN_QUOTED:
    case ORC_SCAN_LINE_COMMENT:
      return 0;
    default:
      return 1;
    }
}

static int32_t csound_orc_scan_code_char(ORC_DELIMITER_SCAN *scan,
                                         char previous, char c)
{
    if (c == '"')
      scan->mode = ORC_SCAN_QUOTED;
    else if (c == ';' || (previous == '/' && c == '/'))
      scan->mode = ORC_SCAN_LINE_COMMENT;
    else if (previous == '/' && c == '*')
      scan->mode = ORC_SCAN_BLOCK_COMMENT;
    else if (previous == '{' && c == '{')
      scan->mode = ORC_SCAN_EXTENDED;
    else if (previous == 'R' && c == '{') {
      scan->mode = ORC_SCAN_RAW;
      scan->rawDepth = 1;
    } else if (c == '#') {
      scan->mode = ORC_SCAN_DIRECTIVE;
      scan->macroStep = DK_START;
    } else {
      scan->previous = c;
      return c == '}' && scan->condDepth == 0;
    }
    return 0;
}

int32_t csound_orc_scan_char(ORC_DELIMITER_SCAN *scan, char c)
{
    char previous = scan->previous;
    int32_t atLineStart = scan->lineStart;
    scan->previous = '\0';
    scan->lineStart = (unsigned char)(c == '\n' ||
                                      (atLineStart && (c == ' ' || c == '\t')));
    switch (scan->mode) {
    case ORC_SCAN_CODE:
      return csound_orc_scan_code_char(scan, previous, c);
    case ORC_SCAN_QUOTED:
      if (previous != '\\') {
        if (c == '"' || c == '\n')
          scan->mode = ORC_SCAN_CODE;
        else
          scan->previous = c;
      }
      break;
    case ORC_SCAN_EXTENDED:
      if (previous == '}' && c == '}')
        scan->mode = ORC_SCAN_CODE;
      else
        scan->previous = c;
      break;
    case ORC_SCAN_RAW:
      if (previous == 'R' && c == '{')
        scan->rawDepth++;
      else if (previous == '}' && c == 'R') {
        if (--scan->rawDepth == 0)
          scan->mode = ORC_SCAN_CODE;
      } else
        scan->previous = c;
      break;
    case ORC_SCAN_LINE_COMMENT:
      if (c == '\n')
        scan->mode = ORC_SCAN_CODE;
      break;
    case ORC_SCAN_BLOCK_COMMENT:
      if (previous == '*' && c == '/')
        scan->mode = ORC_SCAN_CODE;
      else
        scan->previous = c;
      break;
    case ORC_SCAN_DIRECTIVE: {
      int step = dk_step(scan->macroStep, c);
      if (step == DK_NO_MATCH) {
        /* Not a directive told apart here, as #undef or #include: the lexer
           passes it on, and the rest is scanned as the code that follows. */
        scan->mode = ORC_SCAN_CODE;
        scan->macroStep = 0;
        return csound_orc_scan_code_char(scan, '\0', c);
      }
      scan->macroStep = step;
      if (step >= DK_DEFINE) {
        scan_count_conditional(scan, dk_depth_change(step));
        scan->mode = step == DK_DEFINE ? ORC_SCAN_MACRO_HEADER : ORC_SCAN_CODE;
        scan->macroStep = 0;
      }
      break;
    }
    case ORC_SCAN_MACRO_HEADER:
      if (c == ';')
        scan->mode = ORC_SCAN_MACRO_HEADER_LCOM;
      else if (previous == '/' && c == '/')
        scan->mode = ORC_SCAN_MACRO_HEADER_LCOM;
      else if (previous == '/' && c == '*')
        scan->mode = ORC_SCAN_MACRO_HEADER_BCOM;
      else if (c == '(')
        scan->macroStep++;
      else if (c == ')' && scan->macroStep > 0)
        scan->macroStep--;
      else if (c == '#' && scan->macroStep == 0) {
        scan->mode = ORC_SCAN_MACRO_BODY;
        scan->macroStep = 0;
      } else
        scan->previous = c;
      break;
    case ORC_SCAN_MACRO_HEADER_LCOM:
      if (c == '\n')
        scan->mode = ORC_SCAN_MACRO_HEADER;
      break;
    case ORC_SCAN_MACRO_HEADER_BCOM:
      if (previous == '*' && c == '/')
        scan->mode = ORC_SCAN_MACRO_HEADER;
      else
        scan->previous = c;
      break;
    case ORC_SCAN_MACRO_BODY:
      if (previous == '\\')
        scan->previous = '\0';
      else if (c == '\\')
        scan->previous = '\\';
      else if (c == '#')
        scan->mode = ORC_SCAN_CODE;
      break;
    }
    /* In what may be a skipped branch's open string, comment or macro body,
       watch for the conditional directive line that ends it. */
    if (csound_orc_scan_open_construct(scan))
      csound_orc_scan_open_conditional(scan, c, atLineStart);
    else
      scan->condMatch = CM_IDLE;
    return 0;
}
