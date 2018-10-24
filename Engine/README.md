Engine: files for internal engine operation
================================

* auxfd.c: auxiliar resources management for opcodes
* cfgvar.c: configuration variable functions
* corfiles.c: core file processing for csound programs and scores
* cs_new_dispatch.c: parallel csound task dependency management and dispatching
* cs_par_base.c: parallel csound functions
* cs_par_orc_semantic_analysis.c: parallel csound semantics
* csound_data_structures.c: useful data structures (lists, cons cells, hash tables etc)
* csound_orc.lex: csound language lexer
* csound_orc.y: csound language parser
* csound_orc_compile.c: csound compiler
* csound_orc_expressions.c: expression translation, argument lists, etc
* csound_orc_optimize.c: expression optimisation
* csound_orc_semantics.c: csound code semantic analysis
* csound_pre.lex: csound language preprocessor lexer
* csound_prs.lex: score preprocessor lexer
* csound_sco.lex: score lexer
* csound_sco.y: score parser (not used)
* csound_standard_types.c : csound language internal types
* csound_type_system.c : type system functions
* entry1.c: opcode database
* envvar.c: environment variable and file opening functions
* extract.c: score extraction
* fgens.c: function table generators
* insert.c: opcode instantiation and insertion, user-defined opcode functions
* linevent.c: realtime event processing
* memalloc.c: memory resources management
* memfiles.c: functions for memory files
* musmon.c: performance control and event scheduling
* namedins.c: functions for named instrument system
* new_orc_parser.c: csound language parser control
* parse_param.h: macro parameters orchestra parsing structures and prototypes
* score_param.h: macro parameters score parsing structures and prototypes
* scsort.c: score sorting
* scxtract.c: score extraction
* sort.c: sorting functions
* sread.c: score reading functions.
* swritestr.c: score corefile writing.
* symbtab.c: compiler symbol table, user-defined opcode parameter setting.
* twarp.c: score time warping.
