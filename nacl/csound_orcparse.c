/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse csound_orcparse
#define yylex   csound_orclex
#define yyerror csound_orcerror
#define yylval  csound_orclval
#define yychar  csound_orcchar
#define yydebug csound_orcdebug
#define yynerrs csound_orcnerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEWLINE = 258,
     S_NEQ = 259,
     S_AND = 260,
     S_OR = 261,
     S_LT = 262,
     S_LE = 263,
     S_EQ = 264,
     S_ADDIN = 265,
     S_SUBIN = 266,
     S_MULIN = 267,
     S_DIVIN = 268,
     S_GT = 269,
     S_GE = 270,
     S_BITSHIFT_LEFT = 271,
     S_BITSHIFT_RRIGHT = 272,
     LABEL_TOKEN = 273,
     IF_TOKEN = 274,
     T_OPCODE0 = 275,
     T_OPCODE0B = 276,
     T_OPCODE = 277,
     T_OPCODEB = 278,
     UDO_TOKEN = 279,
     UDOSTART_DEFINITION = 280,
     UDOEND_TOKEN = 281,
     UDO_ANS_TOKEN = 282,
     UDO_ARGS_TOKEN = 283,
     T_ERROR = 284,
     T_FUNCTION = 285,
     T_FUNCTIONB = 286,
     INSTR_TOKEN = 287,
     ENDIN_TOKEN = 288,
     GOTO_TOKEN = 289,
     KGOTO_TOKEN = 290,
     IGOTO_TOKEN = 291,
     SRATE_TOKEN = 292,
     KRATE_TOKEN = 293,
     KSMPS_TOKEN = 294,
     NCHNLS_TOKEN = 295,
     NCHNLSI_TOKEN = 296,
     ZERODBFS_TOKEN = 297,
     STRING_TOKEN = 298,
     T_IDENT = 299,
     T_IDENTB = 300,
     INTEGER_TOKEN = 301,
     NUMBER_TOKEN = 302,
     THEN_TOKEN = 303,
     ITHEN_TOKEN = 304,
     KTHEN_TOKEN = 305,
     ELSEIF_TOKEN = 306,
     ELSE_TOKEN = 307,
     ENDIF_TOKEN = 308,
     UNTIL_TOKEN = 309,
     DO_TOKEN = 310,
     OD_TOKEN = 311,
     T_INSTLIST = 312,
     S_ELIPSIS = 313,
     T_ARRAY = 314,
     T_ARRAY_IDENT = 315,
     T_MAPI = 316,
     T_MAPK = 317,
     S_GEQ = 318,
     S_LEQ = 319,
     S_BITSHIFT_RIGHT = 320,
     S_UNOT = 321,
     S_UMINUS = 322,
     S_ATAT = 323,
     S_AT = 324,
     S_GOTO = 325,
     T_HIGHEST = 326
   };
#endif
/* Tokens.  */
#define NEWLINE 258
#define S_NEQ 259
#define S_AND 260
#define S_OR 261
#define S_LT 262
#define S_LE 263
#define S_EQ 264
#define S_ADDIN 265
#define S_SUBIN 266
#define S_MULIN 267
#define S_DIVIN 268
#define S_GT 269
#define S_GE 270
#define S_BITSHIFT_LEFT 271
#define S_BITSHIFT_RRIGHT 272
#define LABEL_TOKEN 273
#define IF_TOKEN 274
#define T_OPCODE0 275
#define T_OPCODE0B 276
#define T_OPCODE 277
#define T_OPCODEB 278
#define UDO_TOKEN 279
#define UDOSTART_DEFINITION 280
#define UDOEND_TOKEN 281
#define UDO_ANS_TOKEN 282
#define UDO_ARGS_TOKEN 283
#define T_ERROR 284
#define T_FUNCTION 285
#define T_FUNCTIONB 286
#define INSTR_TOKEN 287
#define ENDIN_TOKEN 288
#define GOTO_TOKEN 289
#define KGOTO_TOKEN 290
#define IGOTO_TOKEN 291
#define SRATE_TOKEN 292
#define KRATE_TOKEN 293
#define KSMPS_TOKEN 294
#define NCHNLS_TOKEN 295
#define NCHNLSI_TOKEN 296
#define ZERODBFS_TOKEN 297
#define STRING_TOKEN 298
#define T_IDENT 299
#define T_IDENTB 300
#define INTEGER_TOKEN 301
#define NUMBER_TOKEN 302
#define THEN_TOKEN 303
#define ITHEN_TOKEN 304
#define KTHEN_TOKEN 305
#define ELSEIF_TOKEN 306
#define ELSE_TOKEN 307
#define ENDIF_TOKEN 308
#define UNTIL_TOKEN 309
#define DO_TOKEN 310
#define OD_TOKEN 311
#define T_INSTLIST 312
#define S_ELIPSIS 313
#define T_ARRAY 314
#define T_ARRAY_IDENT 315
#define T_MAPI 316
#define T_MAPK 317
#define S_GEQ 318
#define S_LEQ 319
#define S_BITSHIFT_RIGHT 320
#define S_UNOT 321
#define S_UMINUS 322
#define S_ATAT 323
#define S_AT 324
#define S_GOTO 325
#define T_HIGHEST 326




/* Copy the first part of user declarations.  */
#line 126 "../Engine/csound_orc.y"

/* #define YYSTYPE ORCTOKEN* */
/* JPff thinks that line must be wrong and is trying this! */
#define YYSTYPE TREE*

#ifndef NULL
#define NULL 0L
#endif
#include "csoundCore.h"
#include <ctype.h>
#include <string.h>
#include "namedins.h"

#include "csound_orc.h"
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#include "parse_param.h"

#define udoflag csound->parserUdoflag

#define namedInstrFlag csound->parserNamedInstrFlag

    extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
    extern int csound_orclex(TREE**, CSOUND *, void *);
    extern void print_tree(CSOUND *, char *msg, TREE *);
    extern void csound_orcerror(PARSE_PARM *, void *, CSOUND *, TREE*, const char*);
    extern void add_udo_definition(CSOUND*, char *, char *, char *);
    extern ORCTOKEN *lookup_token(CSOUND*,char*,void*);
#define LINE csound_orcget_lineno(scanner)
#define LOCN csound_orcget_locn(scanner)
    extern int csound_orcget_locn(void *);
    extern int csound_orcget_lineno(void *);
    extern ORCTOKEN *make_string(CSOUND *, char *);


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 291 "csound_orcparse.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  70
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2258

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  91
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  187
/* YYNRULES -- Number of states.  */
#define YYNSTATES  314

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   326

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    89,     2,    75,     2,    73,    65,     2,
      88,    85,    71,    69,    83,    70,     2,    72,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    90,     2,
       2,    84,     2,    63,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    86,     2,    87,    74,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    64,     2,    76,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    66,    67,
      68,    77,    78,    79,    80,    81,    82
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,    11,    14,    16,    18,    20,
      24,    28,    33,    36,    38,    40,    41,    42,    51,    55,
      57,    59,    61,    62,    63,    64,    65,    66,    82,    85,
      86,    91,    93,    98,   103,   108,   113,   118,   123,   128,
     133,   137,   142,   144,   148,   154,   156,   162,   164,   166,
     168,   170,   173,   177,   182,   186,   190,   195,   200,   208,
     218,   227,   238,   241,   243,   249,   251,   253,   255,   257,
     259,   261,   263,   265,   267,   269,   271,   273,   275,   277,
     279,   281,   283,   285,   287,   289,   291,   295,   299,   303,
     305,   307,   309,   311,   313,   314,   318,   322,   326,   330,
     334,   338,   342,   346,   350,   354,   358,   362,   366,   370,
     374,   378,   382,   386,   390,   393,   396,   402,   408,   413,
     417,   419,   423,   427,   431,   435,   438,   441,   444,   447,
     449,   453,   457,   461,   465,   469,   473,   477,   481,   483,
     485,   487,   489,   493,   497,   501,   505,   509,   513,   517,
     521,   525,   529,   532,   535,   539,   543,   546,   550,   556,
     562,   566,   569,   572,   574,   576,   578,   580,   582,   584,
     588,   592,   594,   596,   598,   600,   602,   604,   606,   608,
     610,   612,   614,   616,   618,   620,   622,   624
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      92,     0,    -1,    93,    -1,    93,   106,    -1,    93,    95,
      -1,    93,    99,    -1,   106,    -1,    95,    -1,    99,    -1,
      46,    83,    94,    -1,   115,    83,    94,    -1,    69,   115,
      83,    94,    -1,    69,   115,    -1,    46,    -1,   115,    -1,
      -1,    -1,    32,    96,    94,     3,    97,   105,    33,     3,
      -1,    32,     3,     1,    -1,    44,    -1,    22,    -1,    20,
      -1,    -1,    -1,    -1,    -1,    -1,    25,   100,    98,   101,
      83,   102,    27,   103,    83,    28,     3,   104,   105,    26,
       3,    -1,   105,   107,    -1,    -1,   122,    84,   118,     3,
      -1,   107,    -1,   124,    84,   118,     3,    -1,   124,    10,
     118,     3,    -1,   124,    11,   118,     3,    -1,   124,    12,
     118,     3,    -1,   124,    13,   118,     3,    -1,   109,    84,
     118,     3,    -1,   123,    84,   118,     3,    -1,   108,   129,
     116,     3,    -1,   127,   116,     3,    -1,   128,   116,    85,
       3,    -1,    18,    -1,   114,   115,     3,    -1,    19,   117,
     114,   115,     3,    -1,   110,    -1,    54,   117,    55,   105,
      56,    -1,     3,    -1,   124,    -1,   123,    -1,   109,    -1,
      44,     1,    -1,   108,    83,   124,    -1,   108,    83,    44,
       1,    -1,   108,    83,   123,    -1,   108,    83,   109,    -1,
     109,    86,   119,    87,    -1,   124,    86,   119,    87,    -1,
      19,   117,   113,     3,   105,    53,     3,    -1,    19,   117,
     113,     3,   105,    52,   105,    53,     3,    -1,    19,   117,
     113,     3,   105,   111,    53,     3,    -1,    19,   117,   113,
       3,   105,   111,    52,   105,    53,     3,    -1,   111,   112,
      -1,   112,    -1,    51,   117,   113,     3,   105,    -1,    48,
      -1,    50,    -1,    49,    -1,    34,    -1,    35,    -1,    36,
      -1,    22,    -1,    20,    -1,    30,    -1,    44,    -1,    19,
      -1,    48,    -1,    49,    -1,    50,    -1,    51,    -1,    53,
      -1,    54,    -1,    55,    -1,    56,    -1,    46,    -1,    33,
      -1,   116,    83,   118,    -1,   116,    83,   115,    -1,   116,
      83,     1,    -1,   118,    -1,   117,    -1,    44,    -1,    22,
      -1,    30,    -1,    -1,    88,   117,    85,    -1,   118,     8,
     118,    -1,   118,     8,     1,    -1,   118,    15,   118,    -1,
     118,    15,     1,    -1,   118,     4,   118,    -1,   118,     4,
       1,    -1,   118,     9,   118,    -1,   118,     9,     1,    -1,
     118,    84,   118,    -1,   118,    84,     1,    -1,   118,    14,
     118,    -1,   118,    14,     1,    -1,   118,     7,   118,    -1,
     118,     7,     1,    -1,   117,     5,   117,    -1,   117,     5,
       1,    -1,   117,     6,   117,    -1,   117,     6,     1,    -1,
      89,   117,    -1,    89,     1,    -1,   117,    63,   118,    90,
     118,    -1,   117,    63,   118,    90,     1,    -1,   117,    63,
     118,     1,    -1,   117,    63,     1,    -1,   119,    -1,   119,
      69,   119,    -1,   119,    69,     1,    -1,   119,    70,   119,
      -1,   119,    70,     1,    -1,    70,   119,    -1,    70,     1,
      -1,    69,   119,    -1,    69,     1,    -1,   120,    -1,   119,
      71,   119,    -1,   119,    71,     1,    -1,   119,    72,   119,
      -1,   119,    72,     1,    -1,   119,    74,   119,    -1,   119,
      74,     1,    -1,   119,    73,   119,    -1,   119,    73,     1,
      -1,   121,    -1,   124,    -1,   126,    -1,   109,    -1,   119,
      64,   119,    -1,   119,    64,     1,    -1,   119,    65,   119,
      -1,   119,    65,     1,    -1,   119,    75,   119,    -1,   119,
      75,     1,    -1,   119,    16,   119,    -1,   119,    16,     1,
      -1,   119,    68,   119,    -1,   119,    68,     1,    -1,    76,
     119,    -1,    76,     1,    -1,    88,   118,    85,    -1,    88,
     118,     1,    -1,    88,     1,    -1,   125,   116,    85,    -1,
     129,    90,   125,   116,    85,    -1,   129,    90,   130,   116,
      85,    -1,   130,   116,    85,    -1,   125,     1,    -1,   130,
       1,    -1,    37,    -1,    38,    -1,    39,    -1,    40,    -1,
      41,    -1,    42,    -1,   123,    86,    87,    -1,   124,    86,
      87,    -1,    44,    -1,    45,    -1,    46,    -1,    47,    -1,
      43,    -1,    37,    -1,    38,    -1,    39,    -1,    40,    -1,
      41,    -1,    42,    -1,    20,    -1,    21,    -1,    22,    -1,
      30,    -1,    23,    -1,    31,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   162,   162,   171,   175,   179,   183,   184,   185,   188,
     192,   198,   205,   211,   213,   217,   219,   216,   227,   235,
     236,   237,   240,   242,   244,   246,   248,   239,   284,   288,
     291,   299,   303,   313,   329,   345,   361,   377,   385,   394,
     408,   420,   435,   440,   447,   454,   455,   461,   463,   464,
     465,   466,   472,   473,   479,   480,   483,   488,   497,   503,
     512,   521,   543,   555,   558,   568,   570,   572,   576,   578,
     580,   585,   586,   587,   588,   589,   590,   591,   592,   593,
     594,   595,   596,   597,   598,   599,   603,   609,   616,   617,
     618,   619,   620,   621,   622,   626,   627,   628,   629,   630,
     631,   632,   633,   634,   635,   636,   637,   638,   639,   640,
     641,   642,   643,   644,   645,   647,   650,   653,   654,   655,
     656,   659,   660,   661,   662,   663,   667,   668,   672,   673,
     676,   677,   678,   679,   680,   681,   682,   683,   684,   687,
     688,   689,   690,   691,   692,   693,   694,   695,   696,   698,
     699,   701,   702,   704,   705,   706,   707,   708,   717,   726,
     735,   746,   747,   750,   752,   754,   756,   758,   760,   765,
     770,   775,   776,   778,   780,   782,   784,   786,   788,   790,
     792,   794,   798,   809,   820,   822,   826,   828
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NEWLINE", "S_NEQ", "S_AND", "S_OR",
  "S_LT", "S_LE", "S_EQ", "S_ADDIN", "S_SUBIN", "S_MULIN", "S_DIVIN",
  "S_GT", "S_GE", "S_BITSHIFT_LEFT", "S_BITSHIFT_RRIGHT", "LABEL_TOKEN",
  "IF_TOKEN", "T_OPCODE0", "T_OPCODE0B", "T_OPCODE", "T_OPCODEB",
  "UDO_TOKEN", "UDOSTART_DEFINITION", "UDOEND_TOKEN", "UDO_ANS_TOKEN",
  "UDO_ARGS_TOKEN", "T_ERROR", "T_FUNCTION", "T_FUNCTIONB", "INSTR_TOKEN",
  "ENDIN_TOKEN", "GOTO_TOKEN", "KGOTO_TOKEN", "IGOTO_TOKEN", "SRATE_TOKEN",
  "KRATE_TOKEN", "KSMPS_TOKEN", "NCHNLS_TOKEN", "NCHNLSI_TOKEN",
  "ZERODBFS_TOKEN", "STRING_TOKEN", "T_IDENT", "T_IDENTB", "INTEGER_TOKEN",
  "NUMBER_TOKEN", "THEN_TOKEN", "ITHEN_TOKEN", "KTHEN_TOKEN",
  "ELSEIF_TOKEN", "ELSE_TOKEN", "ENDIF_TOKEN", "UNTIL_TOKEN", "DO_TOKEN",
  "OD_TOKEN", "T_INSTLIST", "S_ELIPSIS", "T_ARRAY", "T_ARRAY_IDENT",
  "T_MAPI", "T_MAPK", "'?'", "'|'", "'&'", "S_GEQ", "S_LEQ",
  "S_BITSHIFT_RIGHT", "'+'", "'-'", "'*'", "'/'", "'%'", "'^'", "'#'",
  "'~'", "S_UNOT", "S_UMINUS", "S_ATAT", "S_AT", "S_GOTO", "T_HIGHEST",
  "','", "'='", "')'", "'['", "']'", "'('", "'!'", "':'", "$accept",
  "orcfile", "rootstatement", "instlist", "instrdecl", "@1", "@2",
  "udoname", "udodecl", "@3", "@4", "@5", "@6", "@7", "statementlist",
  "topstatement", "statement", "ans", "arrayexpr", "ifthen", "elseiflist",
  "elseif", "then", "goto", "label", "exprlist", "bexpr", "expr", "iexp",
  "iterm", "ifac", "rident", "arrayident", "ident", "identb", "constant",
  "opcode0", "opcode0b", "opcode", "opcodeb", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,    63,   124,    38,   318,   319,   320,    43,
      45,    42,    47,    37,    94,    35,   126,   321,   322,   323,
     324,   325,   326,    44,    61,    41,    91,    93,    40,    33,
      58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    91,    92,    93,    93,    93,    93,    93,    93,    94,
      94,    94,    94,    94,    94,    96,    97,    95,    95,    98,
      98,    98,   100,   101,   102,   103,   104,    99,   105,   105,
     106,   106,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   108,   108,
     108,   108,   108,   108,   108,   108,   109,   109,   110,   110,
     110,   110,   111,   111,   112,   113,   113,   113,   114,   114,
     114,   115,   115,   115,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   115,   115,   116,   116,   116,   116,
     116,   116,   116,   116,   116,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   118,   118,   118,   118,
     118,   119,   119,   119,   119,   119,   119,   119,   119,   119,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   122,   122,   122,   122,   122,   122,   123,
     123,   124,   125,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   127,   128,   129,   129,   130,   130
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     2,     1,     1,     1,     3,
       3,     4,     2,     1,     1,     0,     0,     8,     3,     1,
       1,     1,     0,     0,     0,     0,     0,    15,     2,     0,
       4,     1,     4,     4,     4,     4,     4,     4,     4,     4,
       3,     4,     1,     3,     5,     1,     5,     1,     1,     1,
       1,     2,     3,     4,     3,     3,     4,     4,     7,     9,
       8,    10,     2,     1,     5,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     1,
       1,     1,     1,     1,     0,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     2,     5,     5,     4,     3,
       1,     3,     3,     3,     3,     2,     2,     2,     2,     1,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       1,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     3,     3,     2,     3,     5,     5,
       3,     2,     2,     1,     1,     1,     1,     1,     1,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    47,    42,     0,   182,   183,    22,    15,    68,    69,
      70,   163,   164,   165,   166,   167,   168,     0,     0,     0,
       2,     7,     8,     6,    31,     0,    50,    45,     0,     0,
      49,    48,    94,    94,   184,   186,   185,   187,   176,   177,
     178,   179,   180,   181,   175,   171,   172,   173,   174,     0,
       0,     0,     0,     0,   141,     0,     0,   120,   129,   138,
     139,     0,   140,     0,     0,     0,     0,     0,    51,     0,
       1,     4,     5,     3,     0,    94,     0,     0,    75,    72,
      71,    73,    85,    74,    84,    76,    77,    78,    79,    80,
      81,    82,    83,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    92,    93,   171,     0,    90,    89,     0,
     128,     0,   127,   126,   125,   153,   152,   156,     0,     0,
     115,   114,     0,     0,    65,    67,    66,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   161,
       0,     0,   162,     0,    21,    20,    19,    23,    18,    13,
       0,     0,    14,    29,     0,    55,    54,    52,     0,     0,
       0,     0,    43,     0,     0,   169,     0,     0,     0,     0,
       0,   170,     0,    40,     0,     0,    95,   155,   154,   111,
     110,   113,   112,   119,     0,    29,     0,   101,   100,   109,
     108,    97,    96,   103,   102,   107,   106,    99,    98,   105,
     104,   149,   148,   143,   142,   145,   144,   151,   150,   122,
     121,   124,   123,   131,   130,   133,   132,   137,   136,   135,
     134,   147,   146,   157,    94,    94,   160,     0,     0,    12,
      16,     0,     0,    53,    39,    37,    56,    30,    38,    33,
      34,    35,    36,    32,    57,    88,    71,    73,   171,   173,
      87,    86,    41,   118,     0,     0,    44,     0,     0,    24,
       9,     0,    29,    10,    46,    28,   117,   116,     0,    29,
       0,     0,    63,   158,   159,     0,    11,     0,     0,     0,
      58,    29,     0,    62,    25,     0,     0,     0,     0,    60,
       0,    17,    29,    59,     0,     0,    64,    61,     0,    26,
      29,     0,     0,    27
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    19,    20,   161,    21,    67,   272,   157,    22,    65,
     237,   285,   300,   310,   242,    23,   275,    25,    54,    27,
     281,   282,   128,    28,   162,   106,   169,   108,    57,    58,
      59,    29,    30,    60,    61,    62,    32,    33,    63,    64
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -160
static const yytype_int16 yypact[] =
{
    2055,  -160,  -160,  1833,  -160,  -160,  -160,     5,  -160,  -160,
    -160,  -160,  -160,  -160,  -160,  -160,  -160,    39,  1833,    28,
    2055,  -160,  -160,  -160,  -160,    -7,   -68,  -160,  2202,   -81,
     -55,   284,  1888,  1888,  -160,  -160,  -160,  -160,  -160,  -160,
    -160,  -160,  -160,  -160,  -160,  -160,  -160,  -160,  -160,  1105,
    1157,  1209,   390,   445,   -52,  1119,    64,   599,  -160,  -160,
     -41,   278,  -160,   -42,   334,    -3,    52,  2164,  -160,   144,
    -160,  -160,  -160,  -160,    16,  1888,  1833,  1995,  -160,  -160,
    -160,  -160,  -160,  -160,  -160,  -160,  -160,  -160,  -160,  -160,
    -160,  -160,  -160,    80,  1833,  1833,    10,  1833,  1833,  1833,
    1833,  1833,  1943,    19,    34,     8,     3,    37,    64,   -48,
    -160,   390,  -160,  -160,  -160,  -160,  -160,  -160,    -1,   171,
    -160,  -160,   500,   555,  -160,  -160,  -160,   610,   127,  2202,
     665,   720,   775,   830,   885,   940,   995,  1261,  1313,  1365,
    1417,  1469,  1521,  1573,  1625,  1677,  1729,  1781,  1995,  -160,
      33,    32,  -160,    36,  -160,  -160,  -160,  -160,  -160,    50,
    2202,   161,    90,  -160,    24,   -52,    91,    95,    11,    37,
     245,   383,  -160,   266,   323,  -160,   379,   435,   490,   503,
     545,  -160,   493,  -160,   175,   179,  -160,  -160,  -160,  -160,
    -160,  -160,  -160,  -160,    81,  -160,   180,  -160,    64,  -160,
      64,  -160,    64,  -160,    64,  -160,    64,  -160,    64,  -160,
      64,  -160,   270,  -160,   709,  -160,   535,  -160,   270,  -160,
     164,  -160,   164,  -160,   -62,  -160,   -62,  -160,   -62,  -160,
      12,  -160,  -160,  -160,  1888,  1888,  -160,   101,  2164,   104,
    -160,  2164,  2025,  -160,  -160,  -160,  -160,  -160,  -160,  -160,
    -160,  -160,  -160,  -160,  -160,  -160,    19,    34,    23,    29,
    -160,    64,  -160,  -160,  1050,  1863,  -160,    84,    85,  -160,
    -160,  2164,  -160,  -160,  -160,  -160,  -160,    64,  1833,  -160,
     188,    94,  -160,  -160,  -160,   165,  -160,  2082,   184,  2103,
    -160,  -160,   190,  -160,  -160,   193,   197,   198,  2109,  -160,
     119,  -160,  -160,  -160,   206,   182,  2157,  -160,   208,  -160,
    -160,  2146,   224,  -160
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -160,  -160,  -160,   -97,   226,  -160,  -160,  -160,   230,  -160,
    -160,  -160,  -160,  -160,  -159,   237,    27,  -160,     0,  -160,
    -160,   -23,   -27,   207,   -18,   -31,     6,     4,    15,  -160,
    -160,  -160,   202,     1,   116,  -160,  -160,  -160,   246,   121
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -186
static const yytype_int16 yytable[] =
{
      26,    31,   109,    94,   122,   123,   183,    56,    66,    55,
      93,   -91,   146,   147,   244,    34,    76,   154,    77,   155,
      26,    31,    56,    36,    69,   243,   -74,    24,    70,    95,
     150,    96,   -84,   153,    77,   184,   265,   185,   107,   107,
      68,   156,   122,   123,   168,   148,  -171,    24,   151,  -171,
    -171,  -171,  -171,   158,  -171,    35,   119,    56,   118,   121,
     164,  -171,   127,    37,   112,   114,   116,   107,   130,  -171,
     107,   131,   132,   133,   165,   167,    74,    46,   134,   135,
     170,   107,   263,   172,   186,   130,   184,   147,   131,   132,
     133,   -91,   171,   -91,   184,   134,   135,   175,   173,   174,
     127,   176,   177,   178,   179,   180,   -74,  -171,   -74,  -184,
    -171,   196,   -84,   287,   -84,   119,   184,   182,   233,   184,
     289,   236,  -171,  -171,  -185,  -171,    56,    56,   190,   192,
     195,   194,   298,   238,   198,   200,   202,   204,   206,   208,
     210,   270,   239,   306,   273,   278,   291,   292,   136,   122,
     123,   311,   212,   214,   216,   218,   220,   222,   224,   226,
     228,   230,   232,   182,   240,   136,   260,   184,   184,   283,
     284,   264,   187,   241,   286,   130,   255,    96,   131,   132,
     133,   102,   262,   266,   269,   134,   135,   271,   261,   122,
     123,   290,   294,   299,    78,    79,   301,   256,    35,   163,
     302,   303,   305,   267,   268,   257,    37,   127,    82,   307,
     308,   309,    38,    39,    40,    41,    42,    43,    44,   258,
      46,   259,    48,    85,    86,    87,    88,   313,    89,    90,
      91,    92,   124,   125,   126,   143,   144,   145,   146,   147,
     107,   107,    26,    31,    49,    50,    71,   127,   245,   130,
      72,    51,   131,   132,   133,   136,   188,    73,   293,   134,
     135,   296,   129,    52,    53,    26,    31,   234,   277,   247,
     130,    75,   235,   131,   132,   133,   166,     0,     0,   149,
     134,   135,    56,     0,   288,     0,     0,    26,    31,    26,
      31,     0,     0,     0,    97,    98,    99,   100,    26,    31,
     103,    35,     0,     0,     0,     0,    26,    31,   104,    37,
       0,    26,    31,     0,     0,    38,    39,    40,    41,    42,
      43,    44,   105,    46,    47,    48,   248,   130,     0,   136,
     131,   132,   133,     0,     0,   152,     0,   134,   135,   141,
     142,   143,   144,   145,   146,   147,     0,    49,    50,     0,
     136,     0,     0,     0,    51,     0,   103,    35,     0,     0,
       0,   -94,     0,   -94,   104,    37,    52,    53,   101,     0,
     102,    38,    39,    40,    41,    42,    43,    44,   105,    46,
      47,    48,   249,   130,     0,     0,   131,   132,   133,     0,
       0,   117,     0,   134,   135,     0,     0,     0,     0,   137,
       0,     0,     0,    49,    50,     0,     0,   136,     0,     0,
      51,     0,    34,    35,     0,     0,     0,   -94,     0,   -94,
      36,    37,    52,    53,     0,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,   250,   130,
       0,     0,   131,   132,   133,     0,   120,   138,   139,   134,
     135,   140,   141,   142,   143,   144,   145,   146,   147,    49,
      50,     0,     0,   136,     0,     0,    51,    34,    35,     0,
     246,     0,     0,     0,     0,    36,    37,     0,    52,    53,
       0,     0,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,   251,   130,     0,     0,   131,   132,   133,
       0,   189,     0,     0,   134,   135,   252,   130,     0,   137,
     131,   132,   133,     0,    49,    50,     0,   134,   135,   136,
       0,    51,    34,    35,     0,     0,     0,     0,     0,     0,
      36,    37,     0,    52,    53,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,   253,   130,
       0,   137,   131,   132,   133,     0,   191,   138,   139,   134,
     135,   140,   141,   142,   143,   144,   145,   146,   147,    49,
      50,     0,     0,     0,   136,     0,    51,    34,    35,     0,
     254,     0,     0,     0,     0,    36,    37,   136,    52,    53,
       0,     0,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,   140,   141,   142,   143,   144,   145,   146,
     147,   193,     0,     0,     0,   137,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     0,   136,
       0,    51,    34,    35,     0,     0,     0,     0,     0,     0,
      36,    37,     0,    52,    53,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,     0,     0,
       0,     0,     0,   138,   139,     0,   197,   140,   141,   142,
     143,   144,   145,   146,   147,     0,     0,     0,     0,    49,
      50,     0,     0,     0,     0,     0,    51,    34,    35,     0,
       0,     0,     0,     0,     0,    36,    37,     0,    52,    53,
       0,     0,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     0,     0,     0,     0,     0,     0,     0,
       0,   199,     0,     0,     0,   137,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     0,     0,
       0,    51,    34,    35,     0,     0,     0,     0,     0,     0,
      36,    37,     0,    52,    53,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,     0,     0,
       0,     0,     0,     0,   139,     0,   201,   140,   141,   142,
     143,   144,   145,   146,   147,     0,     0,     0,     0,    49,
      50,     0,     0,     0,     0,     0,    51,    34,    35,     0,
       0,     0,     0,     0,     0,    36,    37,     0,    52,    53,
       0,     0,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     0,     0,     0,     0,     0,     0,     0,
       0,   203,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     0,     0,
       0,    51,    34,    35,     0,     0,     0,     0,     0,     0,
      36,    37,     0,    52,    53,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,     0,     0,
       0,     0,     0,     0,     0,     0,   205,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    49,
      50,     0,     0,     0,     0,     0,    51,    34,    35,     0,
       0,     0,     0,     0,     0,    36,    37,     0,    52,    53,
       0,     0,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     0,     0,     0,     0,     0,     0,     0,
       0,   207,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     0,     0,
       0,    51,    34,    35,     0,     0,     0,     0,     0,     0,
      36,    37,     0,    52,    53,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,     0,     0,
       0,     0,     0,     0,     0,     0,   209,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    49,
      50,     0,     0,     0,     0,     0,    51,    34,    35,     0,
       0,     0,     0,     0,     0,    36,    37,     0,    52,    53,
       0,     0,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     0,     0,     0,     0,     0,     0,     0,
       0,   276,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     0,     0,
       0,    51,    34,    35,     0,     0,     0,     0,     0,     0,
      36,    37,     0,    52,    53,     0,     0,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,     0,     0,
       0,     0,     0,     0,     0,     0,   110,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    49,
      50,     0,     0,     0,   122,   123,    51,    34,    35,     0,
       0,     0,     0,     0,     0,    36,    37,     0,    52,    53,
       0,     0,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     8,     9,    10,     0,     0,   113,     0,
       0,     0,     0,     0,     0,     0,     0,   124,   125,   126,
       0,     0,     0,     0,    49,    50,     0,     0,     0,    34,
      35,    51,   127,     0,     0,     0,     0,    36,    37,     0,
       0,     0,     0,   111,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,     0,     0,     0,     0,     0,
     115,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    49,    50,     0,     0,
       0,    34,    35,    51,     0,     0,     0,     0,     0,    36,
      37,     0,     0,     0,     0,   111,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,     0,     0,     0,
       0,     0,   211,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    49,    50,
       0,     0,     0,    34,    35,    51,     0,     0,     0,     0,
       0,    36,    37,     0,     0,     0,     0,   111,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,     0,
       0,     0,     0,     0,   213,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,     0,     0,     0,    34,    35,    51,     0,     0,
       0,     0,     0,    36,    37,     0,     0,     0,     0,   111,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,     0,     0,     0,     0,     0,   215,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,     0,     0,     0,    34,    35,    51,
       0,     0,     0,     0,     0,    36,    37,     0,     0,     0,
       0,   111,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     0,     0,     0,     0,     0,   217,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     0,    34,
      35,    51,     0,     0,     0,     0,     0,    36,    37,     0,
       0,     0,     0,   111,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,     0,     0,     0,     0,     0,
     219,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    49,    50,     0,     0,
       0,    34,    35,    51,     0,     0,     0,     0,     0,    36,
      37,     0,     0,     0,     0,   111,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,     0,     0,     0,
       0,     0,   221,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    49,    50,
       0,     0,     0,    34,    35,    51,     0,     0,     0,     0,
       0,    36,    37,     0,     0,     0,     0,   111,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,     0,
       0,     0,     0,     0,   223,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,     0,     0,     0,    34,    35,    51,     0,     0,
       0,     0,     0,    36,    37,     0,     0,     0,     0,   111,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,     0,     0,     0,     0,     0,   225,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,     0,     0,     0,    34,    35,    51,
       0,     0,     0,     0,     0,    36,    37,     0,     0,     0,
       0,   111,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     0,     0,     0,     0,     0,   227,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    49,    50,     0,     0,     0,    34,
      35,    51,     0,     0,     0,     0,     0,    36,    37,     0,
       0,     0,     0,   111,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,     0,     0,     0,     0,     0,
     229,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    49,    50,     0,     0,
       0,    34,    35,    51,     0,     0,     0,     0,     0,    36,
      37,     0,     0,     0,     0,   111,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,     0,     0,     0,
       0,     0,   231,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    49,    50,
       0,     0,     0,    34,    35,    51,     0,     0,     0,     0,
       0,    36,    37,     0,     0,     0,     0,   111,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      49,    50,     0,     0,     0,    34,    35,    51,     0,     0,
       0,     0,     0,    36,    37,     0,     1,     0,     0,   111,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,     2,     3,     4,     5,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     9,    10,
       0,     0,    49,    50,     0,     0,     0,    17,     0,    51,
     103,    35,     0,     0,   278,   279,   280,    18,   104,    37,
       0,    52,    53,     0,     0,    38,    39,    40,    41,    42,
      43,    44,   105,    46,    47,    48,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    49,    50,     0,
       0,     0,     0,     0,    51,    34,    35,     0,     0,     0,
       0,     0,     0,    36,    37,     0,    52,    53,     0,     0,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    49,    50,     0,     0,     0,    34,    35,    51,
       0,     0,     0,     0,     0,    36,    37,     0,     1,     0,
     181,   111,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,     2,     3,     4,     5,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     1,     8,
       9,    10,     0,     0,    49,    50,     0,     0,     0,    17,
       0,    51,     0,     2,     3,     4,     5,     0,     0,    18,
       6,   274,     0,   111,     0,     1,     0,     7,     0,     8,
       9,    10,    11,    12,    13,    14,    15,    16,     0,    17,
       2,     3,     4,     5,     0,     0,     1,     0,     0,    18,
       0,     0,     1,     0,     0,   295,     8,     9,    10,     0,
       0,     2,     3,     4,     5,     0,    17,     2,     3,     4,
       5,     0,     0,     0,     0,     0,    18,     8,     9,    10,
       0,     0,     0,     8,     9,    10,     0,    17,     0,     1,
       0,     0,     0,    17,     0,     0,   297,    18,     0,     0,
       1,     0,   304,    18,     2,     3,     4,     5,     0,     0,
       0,     0,   312,     0,     0,     2,     3,     4,     5,     0,
       8,     9,    10,    78,    79,     0,    80,     0,     0,     0,
      17,     8,     9,    10,    81,     0,     0,    82,     0,     0,
      18,    17,     0,     0,     0,     0,     0,     0,    83,     0,
     159,    18,    85,    86,    87,    88,     0,    89,    90,    91,
      92,    78,    79,     0,    80,     0,     0,     0,     0,     0,
       0,     0,    81,   160,     0,    82,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    83,     0,    84,     0,
      85,    86,    87,    88,     0,    89,    90,    91,    92
};

static const yytype_int16 yycheck[] =
{
       0,     0,    33,    84,     5,     6,     3,     3,     3,     3,
      28,     3,    74,    75,     3,    22,    84,    20,    86,    22,
      20,    20,    18,    30,    18,     1,     3,     0,     0,    84,
      61,    86,     3,    64,    86,    83,   195,    85,    32,    33,
       1,    44,     5,     6,    75,    86,    22,    20,    90,    10,
      11,    12,    13,     1,    30,    23,    52,    53,    52,    53,
      44,    22,    63,    31,    49,    50,    51,    61,     4,    30,
      64,     7,     8,     9,    74,    74,    83,    45,    14,    15,
      76,    75,     1,     3,    85,     4,    83,    75,     7,     8,
       9,    83,    77,    85,    83,    14,    15,    87,    94,    95,
      63,    97,    98,    99,   100,   101,    83,    83,    85,    90,
      86,   129,    83,   272,    85,   111,    83,   102,    85,    83,
     279,    85,    83,    84,    90,    86,   122,   123,   122,   123,
       3,   127,   291,    83,   130,   131,   132,   133,   134,   135,
     136,   238,   160,   302,   241,    51,    52,    53,    84,     5,
       6,   310,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,     3,    84,   184,    83,    83,    85,
      85,    90,     1,    83,   271,     4,     1,    86,     7,     8,
       9,    86,     3,     3,    83,    14,    15,    83,   184,     5,
       6,     3,    27,     3,    19,    20,     3,    22,    23,    55,
       3,     3,    83,   234,   235,    30,    31,    63,    33,     3,
      28,     3,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,     3,    53,    54,
      55,    56,    48,    49,    50,    71,    72,    73,    74,    75,
     234,   235,   242,   242,    69,    70,    20,    63,     3,     4,
      20,    76,     7,     8,     9,    84,    85,    20,   281,    14,
      15,   288,    55,    88,    89,   265,   265,   151,   264,     3,
       4,    25,   151,     7,     8,     9,    74,    -1,    -1,     1,
      14,    15,   278,    -1,   278,    -1,    -1,   287,   287,   289,
     289,    -1,    -1,    -1,    10,    11,    12,    13,   298,   298,
      22,    23,    -1,    -1,    -1,    -1,   306,   306,    30,    31,
      -1,   311,   311,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,     3,     4,    -1,    84,
       7,     8,     9,    -1,    -1,     1,    -1,    14,    15,    69,
      70,    71,    72,    73,    74,    75,    -1,    69,    70,    -1,
      84,    -1,    -1,    -1,    76,    -1,    22,    23,    -1,    -1,
      -1,    83,    -1,    85,    30,    31,    88,    89,    84,    -1,
      86,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,     3,     4,    -1,    -1,     7,     8,     9,    -1,
      -1,     1,    -1,    14,    15,    -1,    -1,    -1,    -1,    16,
      -1,    -1,    -1,    69,    70,    -1,    -1,    84,    -1,    -1,
      76,    -1,    22,    23,    -1,    -1,    -1,    83,    -1,    85,
      30,    31,    88,    89,    -1,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,     3,     4,
      -1,    -1,     7,     8,     9,    -1,     1,    64,    65,    14,
      15,    68,    69,    70,    71,    72,    73,    74,    75,    69,
      70,    -1,    -1,    84,    -1,    -1,    76,    22,    23,    -1,
      87,    -1,    -1,    -1,    -1,    30,    31,    -1,    88,    89,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,     3,     4,    -1,    -1,     7,     8,     9,
      -1,     1,    -1,    -1,    14,    15,     3,     4,    -1,    16,
       7,     8,     9,    -1,    69,    70,    -1,    14,    15,    84,
      -1,    76,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    31,    -1,    88,    89,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,     3,     4,
      -1,    16,     7,     8,     9,    -1,     1,    64,    65,    14,
      15,    68,    69,    70,    71,    72,    73,    74,    75,    69,
      70,    -1,    -1,    -1,    84,    -1,    76,    22,    23,    -1,
      87,    -1,    -1,    -1,    -1,    30,    31,    84,    88,    89,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    68,    69,    70,    71,    72,    73,    74,
      75,     1,    -1,    -1,    -1,    16,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    84,
      -1,    76,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    31,    -1,    88,    89,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    -1,
      -1,    -1,    -1,    64,    65,    -1,     1,    68,    69,    70,
      71,    72,    73,    74,    75,    -1,    -1,    -1,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    76,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    88,    89,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    16,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    76,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    31,    -1,    88,    89,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    65,    -1,     1,    68,    69,    70,
      71,    72,    73,    74,    75,    -1,    -1,    -1,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    76,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    88,    89,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    76,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    31,    -1,    88,    89,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    76,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    88,    89,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    76,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    31,    -1,    88,    89,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      70,    -1,    -1,    -1,    -1,    -1,    76,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    88,    89,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    -1,
      -1,    76,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    31,    -1,    88,    89,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,
      70,    -1,    -1,    -1,     5,     6,    76,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    88,    89,
      -1,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    34,    35,    36,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    50,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    22,
      23,    76,    63,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,    88,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    -1,
      -1,    22,    23,    76,    -1,    -1,    -1,    -1,    -1,    30,
      31,    -1,    -1,    -1,    -1,    88,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    70,
      -1,    -1,    -1,    22,    23,    76,    -1,    -1,    -1,    -1,
      -1,    30,    31,    -1,    -1,    -1,    -1,    88,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    70,    -1,    -1,    -1,    22,    23,    76,    -1,    -1,
      -1,    -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    88,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    69,    70,    -1,    -1,    -1,    22,    23,    76,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    -1,    -1,
      -1,    88,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    22,
      23,    76,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,    88,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    -1,
      -1,    22,    23,    76,    -1,    -1,    -1,    -1,    -1,    30,
      31,    -1,    -1,    -1,    -1,    88,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    70,
      -1,    -1,    -1,    22,    23,    76,    -1,    -1,    -1,    -1,
      -1,    30,    31,    -1,    -1,    -1,    -1,    88,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    70,    -1,    -1,    -1,    22,    23,    76,    -1,    -1,
      -1,    -1,    -1,    30,    31,    -1,    -1,    -1,    -1,    88,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    69,    70,    -1,    -1,    -1,    22,    23,    76,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,    -1,    -1,
      -1,    88,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    -1,    -1,    -1,    22,
      23,    76,    -1,    -1,    -1,    -1,    -1,    30,    31,    -1,
      -1,    -1,    -1,    88,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,    -1,
      -1,    22,    23,    76,    -1,    -1,    -1,    -1,    -1,    30,
      31,    -1,    -1,    -1,    -1,    88,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    70,
      -1,    -1,    -1,    22,    23,    76,    -1,    -1,    -1,    -1,
      -1,    30,    31,    -1,    -1,    -1,    -1,    88,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    70,    -1,    -1,    -1,    22,    23,    76,    -1,    -1,
      -1,    -1,    -1,    30,    31,    -1,     3,    -1,    -1,    88,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    18,    19,    20,    21,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      -1,    -1,    69,    70,    -1,    -1,    -1,    44,    -1,    76,
      22,    23,    -1,    -1,    51,    52,    53,    54,    30,    31,
      -1,    88,    89,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    -1,
      -1,    -1,    -1,    -1,    76,    22,    23,    -1,    -1,    -1,
      -1,    -1,    -1,    30,    31,    -1,    88,    89,    -1,    -1,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    69,    70,    -1,    -1,    -1,    22,    23,    76,
      -1,    -1,    -1,    -1,    -1,    30,    31,    -1,     3,    -1,
      87,    88,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    18,    19,    20,    21,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,    34,
      35,    36,    -1,    -1,    69,    70,    -1,    -1,    -1,    44,
      -1,    76,    -1,    18,    19,    20,    21,    -1,    -1,    54,
      25,    56,    -1,    88,    -1,     3,    -1,    32,    -1,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    -1,    44,
      18,    19,    20,    21,    -1,    -1,     3,    -1,    -1,    54,
      -1,    -1,     3,    -1,    -1,    33,    34,    35,    36,    -1,
      -1,    18,    19,    20,    21,    -1,    44,    18,    19,    20,
      21,    -1,    -1,    -1,    -1,    -1,    54,    34,    35,    36,
      -1,    -1,    -1,    34,    35,    36,    -1,    44,    -1,     3,
      -1,    -1,    -1,    44,    -1,    -1,    53,    54,    -1,    -1,
       3,    -1,    53,    54,    18,    19,    20,    21,    -1,    -1,
      -1,    -1,    26,    -1,    -1,    18,    19,    20,    21,    -1,
      34,    35,    36,    19,    20,    -1,    22,    -1,    -1,    -1,
      44,    34,    35,    36,    30,    -1,    -1,    33,    -1,    -1,
      54,    44,    -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,
      46,    54,    48,    49,    50,    51,    -1,    53,    54,    55,
      56,    19,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    30,    69,    -1,    33,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    -1,    46,    -1,
      48,    49,    50,    51,    -1,    53,    54,    55,    56
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,    18,    19,    20,    21,    25,    32,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    44,    54,    92,
      93,    95,    99,   106,   107,   108,   109,   110,   114,   122,
     123,   124,   127,   128,    22,    23,    30,    31,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    69,
      70,    76,    88,    89,   109,   117,   118,   119,   120,   121,
     124,   125,   126,   129,   130,   100,     3,    96,     1,   117,
       0,    95,    99,   106,    83,   129,    84,    86,    19,    20,
      22,    30,    33,    44,    46,    48,    49,    50,    51,    53,
      54,    55,    56,   115,    84,    84,    86,    10,    11,    12,
      13,    84,    86,    22,    30,    44,   116,   117,   118,   116,
       1,    88,   119,     1,   119,     1,   119,     1,   117,   118,
       1,   117,     5,     6,    48,    49,    50,    63,   113,   114,
       4,     7,     8,     9,    14,    15,    84,    16,    64,    65,
      68,    69,    70,    71,    72,    73,    74,    75,    86,     1,
     116,    90,     1,   116,    20,    22,    44,    98,     1,    46,
      69,    94,   115,    55,    44,   109,   123,   124,   116,   117,
     118,   119,     3,   118,   118,    87,   118,   118,   118,   118,
     118,    87,   119,     3,    83,    85,    85,     1,    85,     1,
     117,     1,   117,     1,   118,     3,   115,     1,   118,     1,
     118,     1,   118,     1,   118,     1,   118,     1,   118,     1,
     118,     1,   119,     1,   119,     1,   119,     1,   119,     1,
     119,     1,   119,     1,   119,     1,   119,     1,   119,     1,
     119,     1,   119,    85,   125,   130,    85,   101,    83,   115,
       3,    83,   105,     1,     3,     3,    87,     3,     3,     3,
       3,     3,     3,     3,    87,     1,    22,    30,    44,    46,
     115,   118,     3,     1,    90,   105,     3,   116,   116,    83,
      94,    83,    97,    94,    56,   107,     1,   118,    51,    52,
      53,   111,   112,    85,    85,   102,    94,   105,   117,   105,
       3,    52,    53,   112,    27,    33,   113,    53,   105,     3,
     103,     3,     3,     3,    53,    83,   105,     3,    28,     3,
     104,   105,    26,     3
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (parm, scanner, csound, astTree, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, csound, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, parm, scanner, csound, astTree); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, PARSE_PARM *parm, void *scanner, CSOUND * csound, TREE * astTree)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, parm, scanner, csound, astTree)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    PARSE_PARM *parm;
    void *scanner;
    CSOUND * csound;
    TREE * astTree;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (parm);
  YYUSE (scanner);
  YYUSE (csound);
  YYUSE (astTree);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, PARSE_PARM *parm, void *scanner, CSOUND * csound, TREE * astTree)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, parm, scanner, csound, astTree)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    PARSE_PARM *parm;
    void *scanner;
    CSOUND * csound;
    TREE * astTree;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, parm, scanner, csound, astTree);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, PARSE_PARM *parm, void *scanner, CSOUND * csound, TREE * astTree)
#else
static void
yy_reduce_print (yyvsp, yyrule, parm, scanner, csound, astTree)
    YYSTYPE *yyvsp;
    int yyrule;
    PARSE_PARM *parm;
    void *scanner;
    CSOUND * csound;
    TREE * astTree;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , parm, scanner, csound, astTree);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, parm, scanner, csound, astTree); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, PARSE_PARM *parm, void *scanner, CSOUND * csound, TREE * astTree)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, parm, scanner, csound, astTree)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    PARSE_PARM *parm;
    void *scanner;
    CSOUND * csound;
    TREE * astTree;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (parm);
  YYUSE (scanner);
  YYUSE (csound);
  YYUSE (astTree);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (PARSE_PARM *parm, void *scanner, CSOUND * csound, TREE * astTree);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (PARSE_PARM *parm, void *scanner, CSOUND * csound, TREE * astTree)
#else
int
yyparse (parm, scanner, csound, astTree)
    PARSE_PARM *parm;
    void *scanner;
    CSOUND * csound;
    TREE * astTree;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 163 "../Engine/csound_orc.y"
    {
                            if ((yyvsp[(1) - (1)]) != NULL)
                              *astTree = *((TREE *)(yyvsp[(1) - (1)]));
                            csound->synterrcnt = csound_orcnerrs;
                            //print_tree(csound, "ALL", $1);
                        ;}
    break;

  case 3:
#line 172 "../Engine/csound_orc.y"
    {
                        (yyval) = appendToTree(csound, (yyvsp[(1) - (2)]), (yyvsp[(2) - (2)]));
                        ;}
    break;

  case 4:
#line 176 "../Engine/csound_orc.y"
    {
                        (yyval) = appendToTree(csound, (yyvsp[(1) - (2)]), (yyvsp[(2) - (2)]));
                        ;}
    break;

  case 5:
#line 180 "../Engine/csound_orc.y"
    {
                        (yyval) = appendToTree(csound, (yyvsp[(1) - (2)]), (yyvsp[(2) - (2)]));
                        ;}
    break;

  case 9:
#line 189 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE, LOCN, T_INSTLIST,
                               make_leaf(csound, LINE,LOCN,
                                         INTEGER_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (3)])), (yyvsp[(3) - (3)])); ;}
    break;

  case 10:
#line 193 "../Engine/csound_orc.y"
    {
                  csp_orc_sa_instr_add(csound, strdup(((ORCTOKEN *)(yyvsp[(1) - (3)]))->lexeme));
                  (yyval) = make_node(csound,LINE,LOCN, T_INSTLIST,
                               make_leaf(csound, LINE,LOCN,
                                         T_IDENT, (ORCTOKEN *)(yyvsp[(1) - (3)])), (yyvsp[(3) - (3)])); ;}
    break;

  case 11:
#line 199 "../Engine/csound_orc.y"
    {
                  TREE *ans;
                  ans = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)(yyvsp[(2) - (4)]));
                  ans->rate = (int) '+';
                  csp_orc_sa_instr_add(csound, strdup(((ORCTOKEN *)(yyvsp[(2) - (4)]))->lexeme));
                  (yyval) = make_node(csound,LINE,LOCN, T_INSTLIST, ans, (yyvsp[(4) - (4)])); ;}
    break;

  case 12:
#line 206 "../Engine/csound_orc.y"
    {
                  TREE *ans;
                  ans = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)(yyvsp[(2) - (2)]));
                  ans->rate = (int) '+';
                  (yyval) = ans; ;}
    break;

  case 13:
#line 211 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           INTEGER_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 14:
#line 213 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 15:
#line 217 "../Engine/csound_orc.y"
    { namedInstrFlag = 1; ;}
    break;

  case 16:
#line 219 "../Engine/csound_orc.y"
    { namedInstrFlag = 0;
                  csp_orc_sa_instr_add_tree(csound, (yyvsp[(3) - (4)]));
                ;}
    break;

  case 17:
#line 223 "../Engine/csound_orc.y"
    {
                    (yyval) = make_node(csound, LINE,LOCN, INSTR_TOKEN, (yyvsp[(3) - (8)]), (yyvsp[(6) - (8)]));
                    csp_orc_sa_instr_finalize(csound);
                ;}
    break;

  case 18:
#line 228 "../Engine/csound_orc.y"
    {
                    namedInstrFlag = 0;
                    csound->Message(csound, Str("No number following instr\n"));
                    csp_orc_sa_instr_finalize(csound);
                ;}
    break;

  case 19:
#line 235 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 20:
#line 236 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 21:
#line 237 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 22:
#line 240 "../Engine/csound_orc.y"
    { udoflag = -2; ;}
    break;

  case 23:
#line 242 "../Engine/csound_orc.y"
    { udoflag = -1; ;}
    break;

  case 24:
#line 244 "../Engine/csound_orc.y"
    { udoflag = 0;;}
    break;

  case 25:
#line 246 "../Engine/csound_orc.y"
    { udoflag = 1; ;}
    break;

  case 26:
#line 248 "../Engine/csound_orc.y"
    {
                udoflag = 2;
                add_udo_definition(csound,
                        ((ORCTOKEN *)(yyvsp[(3) - (11)]))->lexeme,
                        ((ORCTOKEN *)(yyvsp[(7) - (11)]))->lexeme,
                        ((ORCTOKEN *)(yyvsp[(10) - (11)]))->lexeme);
              ;}
    break;

  case 27:
#line 256 "../Engine/csound_orc.y"
    {
                TREE *udoTop = make_leaf(csound, LINE,LOCN, UDO_TOKEN,
                                         (ORCTOKEN *)NULL);
                TREE *ident = make_leaf(csound, LINE,LOCN, T_IDENT,
                                        (ORCTOKEN *)(yyvsp[(3) - (15)]));
                TREE *udoAns = make_leaf(csound, LINE,LOCN, UDO_ANS_TOKEN,
                                         (ORCTOKEN *)(yyvsp[(7) - (15)]));
                TREE *udoArgs = make_leaf(csound, LINE,LOCN, UDO_ARGS_TOKEN,
                                          (ORCTOKEN *)(yyvsp[(10) - (15)]));
                udoflag = -1;
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "UDO COMPLETE\n");

                udoTop->left = ident;
                ident->left = udoAns;
                ident->right = udoArgs;

                udoTop->right = (TREE *)(yyvsp[(13) - (15)]);

                (yyval) = udoTop;

                if (UNLIKELY(PARSER_DEBUG))
                  print_tree(csound, "UDO\n", (TREE *)(yyval));

              ;}
    break;

  case 28:
#line 285 "../Engine/csound_orc.y"
    {
                    (yyval) = appendToTree(csound, (TREE *)(yyvsp[(1) - (2)]), (TREE *)(yyvsp[(2) - (2)]));
                ;}
    break;

  case 29:
#line 288 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 30:
#line 292 "../Engine/csound_orc.y"
    {

                  TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)(yyvsp[(2) - (4)]));
                  ans->left = (TREE *)(yyvsp[(1) - (4)]);
                  ans->right = (TREE *)(yyvsp[(3) - (4)]);
                  (yyval) = ans;
                ;}
    break;

  case 31:
#line 299 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 32:
#line 304 "../Engine/csound_orc.y"
    {
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)(yyvsp[(2) - (4)]));
                  ans->left = (TREE *)(yyvsp[(1) - (4)]);
                  ans->right = (TREE *)(yyvsp[(3) - (4)]);
                  (yyval) = ans;
                  csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                ;}
    break;

  case 33:
#line 314 "../Engine/csound_orc.y"
    { 
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                        make_token(csound, "="));
                  ORCTOKEN *repeat = make_token(csound, (yyvsp[(1) - (4)])->value->lexeme);
                  ans->left = (TREE *)(yyvsp[(1) - (4)]);
                  ans->right = make_node(csound,LINE,LOCN, '+', 
                                         make_leaf(csound,LINE,LOCN, 
                                                   (yyvsp[(1) - (4)])->value->type, repeat),
                                         (TREE *)(yyvsp[(3) - (4)]));
                  //print_tree(csound, "+=", ans);
                  (yyval) = ans;
                  csp_orc_sa_global_read_write_add_list1(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                ;}
    break;

  case 34:
#line 330 "../Engine/csound_orc.y"
    { 
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                        make_token(csound, "="));
                  ORCTOKEN *repeat = make_token(csound, (yyvsp[(1) - (4)])->value->lexeme);
                  ans->left = (TREE *)(yyvsp[(1) - (4)]);
                  ans->right = make_node(csound,LINE,LOCN, '-', 
                                         make_leaf(csound,LINE,LOCN, 
                                                   (yyvsp[(1) - (4)])->value->type, repeat),
                                         (TREE *)(yyvsp[(3) - (4)]));
                  //print_tree(csound, "-=", ans);
                  (yyval) = ans;
                  csp_orc_sa_global_read_write_add_list1(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                ;}
    break;

  case 35:
#line 346 "../Engine/csound_orc.y"
    { 
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                        make_token(csound, "="));
                  ORCTOKEN *repeat = make_token(csound, (yyvsp[(1) - (4)])->value->lexeme);
                  ans->left = (TREE *)(yyvsp[(1) - (4)]);
                  ans->right = make_node(csound,LINE,LOCN, '*', 
                                         make_leaf(csound,LINE,LOCN, 
                                                   (yyvsp[(1) - (4)])->value->type, repeat),
                                         (TREE *)(yyvsp[(3) - (4)]));
                  //print_tree(csound, "-=", ans);
                  (yyval) = ans;
                  csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                ;}
    break;

  case 36:
#line 362 "../Engine/csound_orc.y"
    { 
                  TREE *ans = make_leaf(csound,LINE,LOCN, '=',
                                        make_token(csound, "="));
                  ORCTOKEN *repeat = make_token(csound, (yyvsp[(1) - (4)])->value->lexeme);
                  ans->left = (TREE *)(yyvsp[(1) - (4)]);
                  ans->right = make_node(csound,LINE,LOCN, '/', 
                                         make_leaf(csound,LINE,LOCN, 
                                                   (yyvsp[(1) - (4)])->value->type, repeat),
                                         (TREE *)(yyvsp[(3) - (4)]));
                  //print_tree(csound, "-=", ans);
                  (yyval) = ans;
                  csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, ans->left),
                                    csp_orc_sa_globals_find(csound, ans->right));
                ;}
    break;

  case 37:
#line 378 "../Engine/csound_orc.y"
    {
              TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)(yyvsp[(2) - (4)]));
              ans->left = (TREE *)(yyvsp[(1) - (4)]);
              ans->right = (TREE *)(yyvsp[(3) - (4)]);
              (yyval) = ans; 

          ;}
    break;

  case 38:
#line 386 "../Engine/csound_orc.y"
    {
              TREE *ans = make_leaf(csound,LINE,LOCN, '=', (ORCTOKEN *)(yyvsp[(2) - (4)]));
              ans->left = (TREE *)(yyvsp[(1) - (4)]);
              ans->right = (TREE *)(yyvsp[(3) - (4)]);
              (yyval) = ans; 

          ;}
    break;

  case 39:
#line 395 "../Engine/csound_orc.y"
    {
                  (yyvsp[(2) - (4)])->left = (yyvsp[(1) - (4)]);
                  (yyvsp[(2) - (4)])->right = (yyvsp[(3) - (4)]);
                  (yyvsp[(2) - (4)])->value->optype = NULL;
                  (yyval) = (yyvsp[(2) - (4)]);
                  
                  csp_orc_sa_global_read_write_add_list(csound,
                                    csp_orc_sa_globals_find(csound, (yyvsp[(2) - (4)])->left),
                                    csp_orc_sa_globals_find(csound, (yyvsp[(2) - (4)])->right));
                  csp_orc_sa_interlocks(csound, (yyvsp[(2) - (4)])->value);
                  query_deprecated_opcode(csound, (yyvsp[(2) - (4)])->value);
                ;}
    break;

  case 40:
#line 409 "../Engine/csound_orc.y"
    {
                  ((TREE *)(yyvsp[(1) - (3)]))->left = NULL;
                  ((TREE *)(yyvsp[(1) - (3)]))->right = (TREE *)(yyvsp[(2) - (3)]);
                  (yyvsp[(1) - (3)])->value->optype = NULL;
                  (yyval) = (yyvsp[(1) - (3)]);
                  csp_orc_sa_global_read_add_list(csound,
                                  csp_orc_sa_globals_find(csound,
                                                          (yyvsp[(1) - (3)])->right));
                  csp_orc_sa_interlocks(csound, (yyvsp[(1) - (3)])->value);
                  query_deprecated_opcode(csound, (yyvsp[(1) - (3)])->value);
                ;}
    break;

  case 41:
#line 421 "../Engine/csound_orc.y"
    {
                  ((TREE *)(yyvsp[(1) - (4)]))->left = NULL;
                  ((TREE *)(yyvsp[(1) - (4)]))->right = (TREE *)(yyvsp[(2) - (4)]);
                  (yyvsp[(1) - (4)])->value->optype = NULL;
                  (yyval) = (yyvsp[(1) - (4)]);
                  
                  csp_orc_sa_global_read_add_list(csound,
                                  csp_orc_sa_globals_find(csound,
                                                          (yyvsp[(1) - (4)])->right));
                  
                  csp_orc_sa_interlocks(csound, (yyvsp[(1) - (4)])->value);
                  query_deprecated_opcode(csound, (yyvsp[(1) - (4)])->value);
                 
                ;}
    break;

  case 42:
#line 436 "../Engine/csound_orc.y"
    {
		  //printf("label %s\n", ((ORCTOKEN *)$1)->lexeme);
                    (yyval) = make_leaf(csound,LINE,LOCN, LABEL_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)]));
                ;}
    break;

  case 43:
#line 441 "../Engine/csound_orc.y"
    {
                    (yyvsp[(1) - (3)])->left = NULL;
                    (yyvsp[(1) - (3)])->right = make_leaf(csound, LINE,LOCN,
                                          LABEL_TOKEN, (ORCTOKEN *)(yyvsp[(2) - (3)]));
                    (yyval) = (yyvsp[(1) - (3)]);
                ;}
    break;

  case 44:
#line 448 "../Engine/csound_orc.y"
    {
                    (yyvsp[(3) - (5)])->left = NULL;
                    (yyvsp[(3) - (5)])->right = make_leaf(csound, LINE,LOCN,
                                          LABEL_TOKEN, (ORCTOKEN *)(yyvsp[(4) - (5)]));
                    (yyval) = make_node(csound,LINE,LOCN, IF_TOKEN, (yyvsp[(2) - (5)]), (yyvsp[(3) - (5)]));
                ;}
    break;

  case 46:
#line 456 "../Engine/csound_orc.y"
    {
                  (yyval) = make_leaf(csound,LINE,LOCN, UNTIL_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (5)]));
                  (yyval)->left = (yyvsp[(2) - (5)]);
                  (yyval)->right = (yyvsp[(4) - (5)]);
              ;}
    break;

  case 47:
#line 461 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 48:
#line 463 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 49:
#line 464 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 50:
#line 465 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 51:
#line 467 "../Engine/csound_orc.y"
    { csound->Message(csound,
                      "Unexpected untyped word %s when expecting a variable\n", 
                      ((ORCTOKEN*)(yyvsp[(1) - (2)]))->lexeme);
                (yyval) = NULL;
              ;}
    break;

  case 52:
#line 472 "../Engine/csound_orc.y"
    { (yyval) = appendToTree(csound, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 53:
#line 474 "../Engine/csound_orc.y"
    { csound->Message(csound,
                      "Unexpected untyped word %s when expecting a variable\n", 
                               ((ORCTOKEN*)(yyvsp[(3) - (4)]))->lexeme);
                (yyval) = appendToTree(csound, (yyvsp[(1) - (4)]), NULL);
              ;}
    break;

  case 54:
#line 479 "../Engine/csound_orc.y"
    { (yyval) = appendToTree(csound, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 55:
#line 480 "../Engine/csound_orc.y"
    { (yyval) = appendToTree(csound, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 56:
#line 484 "../Engine/csound_orc.y"
    {
            appendToTree(csound, (yyvsp[(1) - (4)])->right, (yyvsp[(3) - (4)]));
            (yyval) = (yyvsp[(1) - (4)]);
          ;}
    break;

  case 57:
#line 489 "../Engine/csound_orc.y"
    { 
           char* arrayName = (yyvsp[(1) - (4)])->value->lexeme;
            (yyval) = make_node(csound, LINE, LOCN, T_ARRAY, 
	   make_leaf(csound, LINE, LOCN, T_IDENT, make_token(csound, arrayName)), (yyvsp[(3) - (4)])); 

          ;}
    break;

  case 58:
#line 498 "../Engine/csound_orc.y"
    {
            (yyvsp[(3) - (7)])->right = (yyvsp[(5) - (7)]);
            (yyval) = make_node(csound,LINE,LOCN, IF_TOKEN, (yyvsp[(2) - (7)]), (yyvsp[(3) - (7)]));
            //print_tree(csound, "if-endif", $$);
          ;}
    break;

  case 59:
#line 505 "../Engine/csound_orc.y"
    {
            (yyvsp[(3) - (9)])->right = (yyvsp[(5) - (9)]);
            (yyvsp[(3) - (9)])->next = make_node(csound,LINE,LOCN, ELSE_TOKEN, NULL, (yyvsp[(7) - (9)]));
            (yyval) = make_node(csound,LINE,LOCN, IF_TOKEN, (yyvsp[(2) - (9)]), (yyvsp[(3) - (9)]));
            //print_tree(csound, "if-else", $$);

          ;}
    break;

  case 60:
#line 513 "../Engine/csound_orc.y"
    {
            if (UNLIKELY(PARSER_DEBUG))
                csound->Message(csound, "IF-ELSEIF FOUND!\n");
            (yyvsp[(3) - (8)])->right = (yyvsp[(5) - (8)]);
            (yyvsp[(3) - (8)])->next = (yyvsp[(6) - (8)]);
            (yyval) = make_node(csound, LINE,LOCN, IF_TOKEN, (yyvsp[(2) - (8)]), (yyvsp[(3) - (8)]));
            //print_tree(csound, "if-elseif\n", $$);
          ;}
    break;

  case 61:
#line 523 "../Engine/csound_orc.y"
    {
            TREE * tempLastNode;

            (yyvsp[(3) - (10)])->right = (yyvsp[(5) - (10)]);
            (yyvsp[(3) - (10)])->next = (yyvsp[(6) - (10)]);

            (yyval) = make_node(csound, LINE,LOCN, IF_TOKEN, (yyvsp[(2) - (10)]), (yyvsp[(3) - (10)]));

            tempLastNode = (yyval);

            while (tempLastNode->right!=NULL && tempLastNode->right->next!=NULL) {
                tempLastNode = tempLastNode->right->next;
            }

            tempLastNode->right->next = make_node(csound, LINE,LOCN,
                                                  ELSE_TOKEN, NULL, (yyvsp[(8) - (10)]));
            //print_tree(csound, "IF TREE", $$);
          ;}
    break;

  case 62:
#line 544 "../Engine/csound_orc.y"
    {
                TREE * tempLastNode = (yyvsp[(1) - (2)]);

                while (tempLastNode->right!=NULL &&
                       tempLastNode->right->next!=NULL) {
                    tempLastNode = tempLastNode->right->next;
                }

                tempLastNode->right->next = (yyvsp[(2) - (2)]);
                (yyval) = (yyvsp[(1) - (2)]);
            ;}
    break;

  case 63:
#line 555 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 64:
#line 559 "../Engine/csound_orc.y"
    {
                if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "ELSEIF FOUND!\n");
                (yyvsp[(3) - (5)])->right = (yyvsp[(5) - (5)]);
                (yyval) = make_node(csound,LINE,LOCN, ELSEIF_TOKEN, (yyvsp[(2) - (5)]), (yyvsp[(3) - (5)]));
                //print_tree(csound, "ELSEIF", $$);
            ;}
    break;

  case 65:
#line 569 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, THEN_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 66:
#line 571 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, KTHEN_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 67:
#line 573 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, ITHEN_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 68:
#line 577 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, GOTO_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 69:
#line 579 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, KGOTO_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 70:
#line 581 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, IGOTO_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 71:
#line 585 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 72:
#line 586 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 73:
#line 587 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 74:
#line 588 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 75:
#line 589 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 76:
#line 590 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 77:
#line 591 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 78:
#line 592 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 79:
#line 593 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 80:
#line 594 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 81:
#line 595 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 82:
#line 596 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 83:
#line 597 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 84:
#line 598 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 85:
#line 599 "../Engine/csound_orc.y"
    { (yyval) = (TREE *)(yyvsp[(1) - (1)]); ;}
    break;

  case 86:
#line 604 "../Engine/csound_orc.y"
    {
                    /* $$ = make_node(',', $1, $3); */
                    (yyval) = appendToTree(csound, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)]));
		    
                ;}
    break;

  case 87:
#line 610 "../Engine/csound_orc.y"
    {
                    /* $$ = make_node(',', $1, $3); */
                    (yyval) = appendToTree(csound, (yyvsp[(1) - (3)]),
                                      make_leaf(csound, LINE,LOCN,
                                                LABEL_TOKEN, (ORCTOKEN *)(yyvsp[(3) - (3)])));
                ;}
    break;

  case 89:
#line 617 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 90:
#line 618 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 91:
#line 619 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN, LABEL_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)]));  ;}
    break;

  case 92:
#line 620 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN, LABEL_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 93:
#line 621 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN, LABEL_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 94:
#line 622 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 95:
#line 626 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(2) - (3)]); ;}
    break;

  case 96:
#line 627 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_LE, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 98:
#line 629 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_GE, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 100:
#line 631 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_NEQ, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 102:
#line 633 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_EQ, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 104:
#line 635 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_EQ, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 106:
#line 637 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_GT, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 108:
#line 639 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_LT, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 110:
#line 641 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_AND, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 112:
#line 643 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_OR, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 114:
#line 645 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN,
                                                    S_UNOT, (yyvsp[(2) - (2)]), NULL); ;}
    break;

  case 115:
#line 647 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 116:
#line 651 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound,LINE,LOCN, '?', (yyvsp[(1) - (5)]),
                             make_node(csound, LINE,LOCN, ':', (yyvsp[(3) - (5)]), (yyvsp[(5) - (5)]))); ;}
    break;

  case 120:
#line 656 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 121:
#line 659 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '+', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 123:
#line 661 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound ,LINE,LOCN, '-', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 125:
#line 664 "../Engine/csound_orc.y"
    {
                (yyval) = make_node(csound,LINE,LOCN, S_UMINUS, NULL, (yyvsp[(2) - (2)]));
            ;}
    break;

  case 126:
#line 667 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 127:
#line 669 "../Engine/csound_orc.y"
    {
                (yyval) = (yyvsp[(2) - (2)]);
            ;}
    break;

  case 128:
#line 672 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 129:
#line 673 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 130:
#line 676 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '*', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 132:
#line 678 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '/', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 134:
#line 680 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '^', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 136:
#line 682 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '%', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 138:
#line 684 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]);  ;}
    break;

  case 139:
#line 687 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 140:
#line 688 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 141:
#line 689 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(1) - (1)]); ;}
    break;

  case 142:
#line 690 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '|', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 144:
#line 692 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '&', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 146:
#line 694 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '#', (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 148:
#line 697 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_BITSHIFT_LEFT, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 150:
#line 700 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, S_BITSHIFT_RIGHT, (yyvsp[(1) - (3)]), (yyvsp[(3) - (3)])); ;}
    break;

  case 152:
#line 703 "../Engine/csound_orc.y"
    { (yyval) = make_node(csound, LINE,LOCN, '~', NULL, (yyvsp[(2) - (2)]));;}
    break;

  case 153:
#line 704 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 154:
#line 705 "../Engine/csound_orc.y"
    { (yyval) = (yyvsp[(2) - (3)]);  ;}
    break;

  case 155:
#line 706 "../Engine/csound_orc.y"
    { (yyval) = NULL;  ;}
    break;

  case 156:
#line 707 "../Engine/csound_orc.y"
    { (yyval) = NULL; ;}
    break;

  case 157:
#line 709 "../Engine/csound_orc.y"
    {
                
                (yyvsp[(1) - (3)])->left = NULL;
                (yyvsp[(1) - (3)])->right = (yyvsp[(2) - (3)]);
		(yyvsp[(1) - (3)])->type = T_FUNCTION;
                
                (yyval) = (yyvsp[(1) - (3)]);
            ;}
    break;

  case 158:
#line 718 "../Engine/csound_orc.y"
    {
                (yyvsp[(1) - (5)])->left = NULL;
                (yyvsp[(1) - (5)])->right = (yyvsp[(4) - (5)]);
		(yyvsp[(1) - (5)])->type = T_FUNCTION;
                (yyvsp[(1) - (5)])->value->optype = (yyvsp[(3) - (5)])->value->lexeme;
		
                (yyval) = (yyvsp[(1) - (5)]);
            ;}
    break;

  case 159:
#line 727 "../Engine/csound_orc.y"
    {
                (yyvsp[(1) - (5)])->left = NULL;
                (yyvsp[(1) - (5)])->right = (yyvsp[(4) - (5)]);
		(yyvsp[(1) - (5)])->type = T_FUNCTION;
                (yyvsp[(1) - (5)])->value->optype = (yyvsp[(3) - (5)])->value->lexeme;
		
                (yyval) = (yyvsp[(1) - (5)]);
            ;}
    break;

  case 160:
#line 736 "../Engine/csound_orc.y"
    {
                (yyvsp[(1) - (3)])->left = NULL;
                (yyvsp[(1) - (3)])->right = (yyvsp[(2) - (3)]);
		(yyvsp[(1) - (3)])->type = T_FUNCTION;
                (yyvsp[(1) - (3)])->value->optype = NULL;
       
                (yyval) = (yyvsp[(1) - (3)]);
                //print_tree(csound, "FUNCTION CALL", $$);
            ;}
    break;

  case 163:
#line 750 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                             SRATE_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 164:
#line 752 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                             KRATE_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 165:
#line 754 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                             KSMPS_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 166:
#line 756 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                             NCHNLS_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 167:
#line 758 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                             NCHNLSI_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 168:
#line 760 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                             ZERODBFS_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 169:
#line 765 "../Engine/csound_orc.y"
    {          
            appendToTree(csound, (yyvsp[(1) - (3)])->right, 
	         make_leaf(csound, LINE, LOCN, '[', make_token(csound, "[")));
            (yyval) = (yyvsp[(1) - (3)]);
          ;}
    break;

  case 170:
#line 770 "../Engine/csound_orc.y"
    {
            (yyval) = make_leaf(csound, LINE, LOCN, T_ARRAY_IDENT, make_token(csound, (yyvsp[(1) - (3)])->value->lexeme)); 
	    (yyval)->right = make_leaf(csound, LINE, LOCN, '[', make_token(csound, "["));
          ;}
    break;

  case 171:
#line 775 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 172:
#line 776 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN, T_IDENT, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 173:
#line 778 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           INTEGER_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 174:
#line 780 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           NUMBER_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 175:
#line 782 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           STRING_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 176:
#line 784 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           SRATE_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 177:
#line 786 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           KRATE_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 178:
#line 788 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           KSMPS_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 179:
#line 790 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           NCHNLS_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 180:
#line 792 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                           NCHNLSI_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 181:
#line 794 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound, LINE,LOCN,
                                            ZERODBFS_TOKEN, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 182:
#line 799 "../Engine/csound_orc.y"
    {
	      if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "opcode0 $1=%p (%s)\n",
                                  (yyvsp[(1) - (1)]),((ORCTOKEN *)(yyvsp[(1) - (1)]))->lexeme );
                (yyval) = make_leaf(csound,LINE,LOCN, T_OPCODE0, (ORCTOKEN *)(yyvsp[(1) - (1)]));
                

            ;}
    break;

  case 183:
#line 810 "../Engine/csound_orc.y"
    {
	      if (UNLIKELY(PARSER_DEBUG))
                  csound->Message(csound, "opcode0b $1=%p (%s)\n",
                                  (yyvsp[(1) - (1)]),((ORCTOKEN *)(yyvsp[(1) - (1)]))->lexeme );
                (yyval) = make_leaf(csound,LINE,LOCN, T_OPCODE0, (ORCTOKEN *)(yyvsp[(1) - (1)]));
                

            ;}
    break;

  case 184:
#line 821 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 185:
#line 823 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 186:
#line 827 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;

  case 187:
#line 829 "../Engine/csound_orc.y"
    { (yyval) = make_leaf(csound,LINE,LOCN, T_OPCODE, (ORCTOKEN *)(yyvsp[(1) - (1)])); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 3343 "csound_orcparse.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (parm, scanner, csound, astTree, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (parm, scanner, csound, astTree, yymsg);
	  }
	else
	  {
	    yyerror (parm, scanner, csound, astTree, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, parm, scanner, csound, astTree);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, parm, scanner, csound, astTree);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (parm, scanner, csound, astTree, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, parm, scanner, csound, astTree);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, parm, scanner, csound, astTree);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 832 "../Engine/csound_orc.y"


#ifdef SOME_FINE_DAY
void
yyerror(char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if(yylloc.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ",
            yylloc.first_line, yylloc.first_column,
	    yylloc.last_line, yylloc.last_column);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");

}

void
lyyerror(YYLTYPE t, char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  if(t.first_line)
    fprintf(stderr, "%d.%d-%d.%d: error: ", t.first_line, t.first_column,
	    t.last_line, t.last_column);
  vfprintf(stderr, s, ap);
  fprintf(stderr, "\n");
}

#endif


