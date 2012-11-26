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

/* fetch a weight for opcode name */
uint32_t csp_opcode_weight_fetch(CSOUND *csound, char *name);
/* set the time for opcode name */
void csp_opcode_weight_set(CSOUND *csound, char *name, double play_time);
/* print opcode weights */
void csp_weights_dump(CSOUND *csound);
/* print opcode weights normalised to 1-100 (inclusive) scale */
void csp_weights_dump_normalised(CSOUND *csound);
/* dump opcode weights normalised to a file with 1-100 (inclusive) scale
 * also write out the times associated with the weights */
void csp_weights_dump_file(CSOUND *csound);
/* load opcode weights from a file */
void csp_weights_load(CSOUND *csound);
/* calculate the weight for each instrument in the AST
 * put the weight in the instr_semantics_t structure stored in
 * cs_par_orc_semantic_analysis */
void csp_weights_calculate(CSOUND *csound, TREE *root);

/* dag2 prototypes */
enum dag_node_type_t {
    DAG_NODE_INDV,
    DAG_NODE_LIST,
    DAG_NODE_DAG
};

struct dag_base_t {
    char                        hdr[HDR_LEN];
    enum dag_node_type_t        type;
};

typedef struct dag_t {
    struct dag_base_t       hdr;

    int                     count;
    LOCK_TYPE               spinlock;
    LOCK_TYPE               table_spinlock;
    LOCK_TYPE               consume_spinlock;
    struct dag_node_t       **all;
    struct dag_node_t       *insds_chain_start;
    struct dag_node_t       **roots_ori;
    struct dag_node_t       **roots;
    uint8_t                 *root_seen_ori;
    uint8_t                 *root_seen;
    int                     *remaining_count_ori;
    int                     *remaining_count;
    int                     remaining;
    int                     first_root_ori;
    int                     first_root;
    uint8_t                 **table_ori;
    uint8_t                 **table;

    /* used for deciding whether to run this dag in parallel */
    int                     max_roots;
    uint32_t                weight;
} DAG;

/* load the parallel decision infomation from specified file */
void csp_parallel_compute_spec_setup(CSOUND *csound);
/* decide based in parallel decision info whether to do this dag in parallel */
#if 0
int inline csp_parallel_compute_should(CSOUND *csound, DAG *dag);
#endif

typedef struct dag_node_t {
    struct dag_base_t           hdr;

    union {
        struct {
            struct instr_semantics_t    *instr;
            INSDS                       *insds;
            struct dag_node_t           *insds_chain_next;
        };
        struct {
            int count;
            struct dag_node_t **nodes;
        };
    };
} DAG_NODE;

void csp_dag_alloc(CSOUND *csound, DAG **dag);
void csp_dag_dealloc(CSOUND *csound, DAG **dag);
/* add a node to the dag with instrument info */
void csp_dag_add(CSOUND *csound, DAG *dag,
                 struct instr_semantics_t *instr, INSDS *insds);

/* once a DAG has been created and had all its instruments added call this
 * prepares a DAG for use
 * builds edges, roots, root countdowns, finds weight, etc */
void csp_dag_build(CSOUND *csound, DAG **dag, INSDS *chain);
void csp_dag_print(CSOUND *csound, DAG *dag);

/* return 1 if the DAG is completely consume */
int csp_dag_is_finished(DAG *dag);
/* get a node from the dag
 * update_hdl should be passed into consume_update when the node has
 * been performed */
void csp_dag_consume(DAG *dag,
                     struct dag_node_t **node, int *update_hdl);
/* update the dag having consumed a node previously */
void csp_dag_consume_update(DAG *dag, int update_hdl);

/* get a dag from the cache
 * if it exists it is retuned
 * if not builds a new one and stores in the cache, then returns */
void csp_dag_cache_fetch(CSOUND *csound, DAG **dag, INSDS *chain);
void csp_dag_cache_print(CSOUND *csound);

#endif /* end of include guard: __CS_PAR_DISPATCH_H__ */
