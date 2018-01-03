/*
    cs_par_dispatch.h:

    Copyright (C) 2011 by John ffitch

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

#ifndef __CS_PAR_DISPATCH_H__
#define __CS_PAR_DISPATCH_H__

/*
 * locks must first be inserted and then the cache built
 * following this globals can be locked and unlocked with
 * the appropriate functions
 */
/* add global locks into AST root */
static TREE *csp_locks_insert(CSOUND * csound, TREE *root);
/* build the cache of global locks */
void csp_locks_cache_build(CSOUND *csound);
/* lock global with index */
void csp_locks_lock(CSOUND * csound, int global_index);
/* unlock global with index */
void csp_locks_unlock(CSOUND * csound, int global_index);

#endif /* end of include guard: __CS_PAR_DISPATCH_H__ */
