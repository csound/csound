/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



