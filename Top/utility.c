/*
  utility.c:

  Copyright (C) 2005 Istvan Varga

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
#include "cs_util.h"

typedef struct csUtility_s {
    char                *name;
    struct csUtility_s  *nxt;
    int                 (*UtilFunc)(void*, int, char**);
} csUtility_t;

static const char list_var[] = "utilities::list";

PUBLIC int csoundAddUtility(void *csound_, const char *name,
                            int (*UtilFunc)(void*, int, char**))
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    csUtility_t *p;

    if (csound == NULL || name == NULL || name[0] == '\0' || UtilFunc == NULL)
      return -1;
    p = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
    if (p != NULL) {
      do {
        if (strcmp(p->name, name) == 0)
          return -1;    /* name is already in use */
        p = p->nxt;
      } while (p != NULL);
      p = csound->Malloc(csound, sizeof(csUtility_t));
      p->name = csound->Malloc(csound, strlen(name) + 1);
      strcpy(p->name, name);
      p->nxt = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
      p->UtilFunc = UtilFunc;
      return 0;
    }
    if (csound->CreateGlobalVariable(csound,list_var,sizeof(csUtility_t)) != 0)
      return -1;
    p = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
    p->name = csound->Malloc(csound, strlen(name) + 1);
    strcpy(p->name, name);
    p->nxt = NULL;
    p->UtilFunc = UtilFunc;
    return 0;
}

PUBLIC int csoundRunUtility(void *csound_, const char *name,
                            int argc, char **argv)
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    csUtility_t *p;

    if (csound == NULL)
      return -1;
    if (name == NULL || name[0] == '\0')
      goto notFound;
    p = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
    do {
      if (p == NULL)
        goto notFound;
      if (strcmp(p->name, name) == 0)
        break;
      p = p->nxt;
    } while (1);
    csound->Message(csound, Str("util %s:\n"), name);
    return (p->UtilFunc(csound, argc, argv));

 notFound:
    csound->Message(csound, Str("Error: utility "));
    if (name != NULL && name[0] != '\0')
      csound->Message(csound, "'%s' ", name);
    csound->Message(csound, Str("not found\n"));
    return -1;
}

