 /*
    csound_orc_optimizee.c:

    Copyright (C) 2006
    Steven Yi

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
#include "csound_orc.h"

/* Optimizes tree (expressions, etc.) */
TREE* csound_orc_optimize(CSOUND * csound, TREE *root)
{
    if (PARSER_DEBUG) csound->Message(csound, "Optimizing AST\n");

    TREE *anchor = NULL;

    TREE *current = root;
    TREE *previous = NULL;

    while(current != NULL) {
      switch(current->type) {
      case T_INSTR:
        if (PARSER_DEBUG) csound->Message(csound, "Instrument found\n");
        current->right = csound_orc_optimize(csound, current->right);

        break;
      case T_UDO:
        if (PARSER_DEBUG) csound->Message(csound, "UDO found\n");

        break;

      case T_IF:

        break;

      default:

        if (current->right != NULL) {
          if (PARSER_DEBUG) csound->Message(csound, "Found Statement.\n");

          if (current->type == S_ASSIGN && previous != NULL) {
            /* S_ASSIGN should be guaranteed to have left and right
             * arg by the time it gets here */
            if (previous->left != NULL && previous->left->value != NULL) {
              if (strcmp(previous->left->value->lexeme,
                        current->right->value->lexeme) == 0) {

              }

            }

          }
        }
      }

      if (anchor == NULL) {
        anchor = current;
      }

      previous = current;
      current = current->next;

    }

    if (PARSER_DEBUG) csound->Message(csound, "[End Optimizing AST]\n");

    return anchor;

}
