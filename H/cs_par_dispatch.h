#ifndef __CS_PAR_DISPATCH_H__
#define __CS_PAR_DISPATCH_H__

/*
 * locks must first be inserted and then the cache built
 * following this globals can be locked and unlocked with
 * the appropriate functions
 */
/* add global locks into AST root */
TREE *csp_locks_insert(CSOUND * csound, TREE *root);
/* build the cache of global locks */
void csp_locks_cache_build(CSOUND *csound);
/* lock global with index */
void csp_locks_lock(CSOUND * csound, int global_index);
/* unlock global with index */
void csp_locks_unlock(CSOUND * csound, int global_index);

#endif /* end of include guard: __CS_PAR_DISPATCH_H__ */
