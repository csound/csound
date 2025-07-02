/*
  csound_compiler.h: lower-level parsing and compiling interface

  Copyright (C) 2024

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

#ifndef CSOUND_COMPILER_H
#define CSOUND_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

 
 typedef struct ORCTOKEN {
    int32_t              type;
    char             *lexeme;
    int32_t              value;
    double           fvalue;
    char             *optype;
    struct ORCTOKEN  *next;
  } ORCTOKEN;

  
  typedef struct TREE {
    int32_t           type;
    ORCTOKEN      *value;
    int32_t           rate;
    int32_t           len;
    int32_t           line;
    uint64_t      locn;
    struct TREE   *left;
    struct TREE   *right;
    struct TREE   *next;
    void          *markup;  // TEMPORARY - used by semantic checker to
    // markup node adds OENTRY or synthetic var
    // names to expression nodes should be moved
    // to TYPE_TABLE
  } TREE;

#ifndef CSOUND_CSDL_H
 
  /** @}*/
  /** @defgroup COMPILATION Parsing and compilation
   *
   *  @{ */
  /**
   * Parse the given orchestra from an ASCII string into a TREE.
   * This can be called during performance to parse new code.
   */
  PUBLIC TREE *csoundParseOrc(CSOUND *csound, const char *str);

  /**
   * Compile the given TREE node into structs for Csound to use
   * in synchronous or asynchronous (async = 1) mode.
   */
  PUBLIC int32_t csoundCompileTree(CSOUND *csound, TREE *root, int32_t async);

  /**
   * Free the resources associated with the TREE *tree
   * This function should be called whenever the TREE was
   * created with csoundParseOrc and memory can be deallocated.
   **/
  PUBLIC void csoundDeleteTree(CSOUND *csound, TREE *tree);  


  /** @}*/
  
#endif  /* !CSOUND_CSDL_H */

#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_COMPILER_H */
