/*
    getstring.c:

    Copyright (C) 1999 John ffitch
    Jan 27 2005: replaced with new implementation by Istvan Varga

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

#include <cs.h>
#ifdef HAVE_DIRENT_H
#include <sys/types.h>
#include <dirent.h>
#ifdef __MACH__
#ifdef DIRENT_FIX
typedef void* DIR;
DIR opendir(const char *);
struct dirent *readdir(DIR*);
int closedir(DIR*);
#endif
#endif
#endif
#include "csound.h"

#define name_hash_10(x,y)       (hash_table_10[(x) ^ (y)])

const unsigned short hash_table_10[1024] = {
         834u,  379u,  434u,  783u,  766u,  961u,   52u,  121u,  827u,  923u,
         200u,  756u,  174u,  607u,  864u,  704u,  242u,  939u,  979u,  983u,
         252u,  554u,  150u,  902u,  243u,  424u,  697u,  487u,  343u,  584u,
         476u,  721u,  692u,   48u,  677u,  560u,  575u,  966u,  832u,  425u,
         177u,  260u,  446u,  947u,  160u,   90u,  833u,  849u,  826u,  771u,
         194u,  325u,  919u,  431u,  843u,  189u,  565u,  759u,  625u,  232u,
          83u,  926u,  691u,   43u,  501u,  835u,  989u,  443u,  904u,  995u,
         568u,   95u,  892u,  126u,  456u,  235u,  997u,  755u,   63u,  970u,
         732u,  104u,  866u,  587u,  543u,  632u,  342u,  315u,  144u,  791u,
         684u,  731u,  580u,  753u,    6u,  360u,  386u,  916u,   61u,  736u,
         346u,  387u,   91u,  822u,  135u,  385u,  354u,  611u,  946u,   47u,
         889u,  202u,  396u,  184u,  403u,  964u,  199u,  556u,  514u,  943u,
         122u,  370u,  520u,  435u,  976u,  679u,  335u,  954u,  624u,  428u,
         171u,    2u,  836u,  317u,  748u,  804u,  119u,  314u,  166u,  304u,
         369u,  635u,  817u,  593u,  186u,  838u,   98u,  792u,  151u,  303u,
         974u,  449u,  274u,  602u,  466u,  308u,  590u,  156u,  328u,  110u,
         546u,  400u,  240u,  663u,  811u,  917u,   54u, 1003u,  675u,  388u,
         549u,  813u,  971u,  492u,  505u,  960u,  180u,  915u,  657u,   66u,
         795u,   30u,  333u,  770u,  816u,  653u,  723u,   59u,  488u,  963u,
          49u,  931u,  493u,  853u,  111u,   56u,  787u,  382u,  516u,  875u,
         309u,  550u,   40u,  906u,  490u,  406u,  638u,  361u,  659u,  648u,
         495u,   87u,  893u,  204u,  752u,  871u,  905u,   20u,  557u,  886u,
         442u,  485u,  499u,  352u,  350u,  300u,  725u,  777u,  391u,  944u,
         127u,  357u,  981u,   39u,  404u,  998u,  987u,  330u,  306u,  427u,
          24u,  461u,  877u,  671u,  539u,  484u,  464u,  769u,  941u,  413u,
         922u,  445u,  899u,  236u,  809u,  534u,  164u,  187u,  741u,  380u,
         612u,  595u,  351u,   60u,  993u,  118u,   94u,  950u,  664u,  264u,
         145u,  714u,  159u,  368u,  859u,  473u,  918u,  332u,  619u,   29u,
         448u,  176u,  109u,  879u,   55u,  526u,  790u,  540u,  701u,  389u,
         148u,  138u,  452u,  512u,  393u,  378u,  952u,   42u,  102u,  798u,
         688u,   31u,  585u,  178u,  689u,  277u,  685u,  437u,  509u,  140u,
         589u,  967u,  299u,  789u,  564u,   57u,  720u,  654u,  414u,   28u,
         758u,  764u,  932u,   82u,   23u,  365u,  402u,  782u,  676u,  282u,
         660u,  767u,  276u,  136u,  415u,  718u,  936u,  139u,  507u,  255u,
         537u,   85u,  296u,  942u,  100u,    7u,  338u,  768u,  738u,  228u,
         185u,  772u,  266u,  498u,  496u,  258u,  925u,  429u,  586u,  281u,
         278u,  147u,  869u,  517u,  695u,  478u,  459u,  453u,   58u,   18u,
         246u,  729u,  572u,  992u,  775u,  513u,  778u,  508u,  610u,  465u,
         984u,  803u,  631u,  927u,  518u,  555u,  439u,  412u,  933u,   89u,
         191u,  603u,  711u,  173u,  698u,  897u, 1021u,  146u,  739u,  440u,
         544u,  197u,  221u,  491u,  525u,  728u,  366u,  599u,  920u,  420u,
         870u,  815u,  161u,   78u,  153u,  529u,  213u,  377u,  212u, 1010u,
         390u,  124u,  327u,  120u,   19u,  137u,  149u,    0u,  794u,  515u,
         839u,  957u,  569u,  337u,  940u,  419u,  470u,  682u,  225u,  700u,
         143u,   93u, 1014u,  990u,  198u,  938u,  642u,  114u, 1013u,  884u,
         793u,  608u,  503u,   21u,  761u,  819u,  617u,  605u,  155u,  551u,
         596u,   36u,  364u,  280u,  618u,  716u,  965u,  734u,  844u,  880u,
         457u,  272u,  667u,  951u,  604u,  614u,  205u,  279u,  230u,  656u,
         661u,   38u,  571u,   80u,    1u,  312u,  545u,  678u,   72u,  458u,
         955u,  592u,  862u, 1009u,  911u, 1005u,   86u,  349u,  903u,  268u,
         297u,  441u,   99u,  730u,  219u,  672u,  854u,  322u,  214u,  218u,
         408u,  760u,  609u,  409u,  852u, 1012u,  326u,  348u,  324u,  928u,
         935u,  216u,  740u,  737u,  873u,  841u,  668u,  334u,  310u,  810u,
         482u,  644u,  339u,  295u,  363u,  383u, 1001u,  188u,  629u,  376u,
         433u,  717u,  157u,  765u,  271u,   27u,  249u,  237u,  699u,  882u,
          15u,  598u,  937u,  750u,  807u,  900u,  397u,  253u,  662u,  371u,
         888u,  818u,  781u,   35u,  800u,  850u,  805u,   50u, 1020u,   71u,
         494u,  367u,  536u,  347u,  345u,  210u,   11u,  479u,  203u,  825u,
         447u, 1015u,  594u,  533u,  432u,  250u,  356u,  703u,  395u,  840u,
         601u,  208u,  132u,    5u,  234u,   79u,  860u,  754u,  562u,  472u,
         868u,  831u,  788u,  999u,  830u,  845u,  776u,  634u,  578u,  681u,
         344u,  934u,  724u,  532u,   33u,  244u,  640u,  797u,  894u, 1022u,
         239u,  384u,  745u,  690u,  167u,  786u,  953u,  615u,  542u,  170u,
          46u,  193u,  256u,  669u,   25u,  112u,    8u, 1017u,  847u,  821u,
         719u,  245u,  541u,  142u,  988u,  857u,  489u,  949u,  907u,  842u,
        1000u,  867u,  374u,  901u,  895u,  658u,  883u,   68u, 1006u,  912u,
         353u,  500u,  630u,  757u,  986u,  421u,  436u,  958u,  994u,  480u,
         502u,  646u,  426u,  913u,  865u,  181u,  779u,  373u,  801u,  101u,
         715u,  430u,  172u,  747u,  973u,  968u,  665u,   67u,  637u,  226u,
         929u, 1004u,  269u,  392u,  527u,  975u,  462u,  746u,  908u,  469u,
         375u,  321u,  959u,  547u,  874u,   16u,  463u,  307u,  358u,  423u,
          37u,  152u,  528u,  647u,  163u,  574u,  561u,  399u,  123u,  626u,
         263u,  559u,  359u,  248u,  323u,  506u,  693u, 1019u,   10u,  116u,
         183u,  991u,  222u,  292u,  673u,  597u,  573u,    3u,  450u,  707u,
           4u,  530u,  318u,  591u,  709u,  696u,  175u,  722u,   22u,  320u,
          41u,   65u,  914u,  259u,  261u,  362u,  713u,   14u,  972u,  969u,
         846u,  985u, 1023u,  394u,  548u,  566u,  674u,  316u,  622u,  820u,
         262u,  824u,  291u,  372u,  878u,  162u,  799u,  945u,  133u,  762u,
         680u,  633u,  483u,  639u,  563u,  552u,  694u,  331u,  141u,  524u,
         336u,  283u,  727u,  207u,  254u,  796u,  407u,   81u,   74u,  417u,
          69u,  523u,  891u,  708u,  621u,  209u,  535u,  293u,  606u,  206u,
         848u,  910u,  340u,  294u, 1008u,  613u,  583u,  652u,  712u,  451u,
         224u,  812u, 1011u,  105u,  982u,  511u,  165u,  267u,    9u,   70u,
         802u,  265u,  702u,  103u,  636u,  298u,  179u,  270u,   12u, 1002u,
         924u,  921u, 1016u,  978u,  890u,  858u,  134u,  948u,  190u,  301u,
         319u,   53u,  257u,  290u,  751u,  837u,  774u,  735u,  285u,  289u,
         650u,  287u,  930u,  623u,  616u,  233u,  286u,  398u,  733u,  302u,
         855u,  651u,   97u,  588u,  686u,  223u,  531u,  117u,  627u,  168u,
         710u,  284u,   96u,  128u,  131u,  460u,  510u,  504u,  863u,  247u,
         355u,  628u,  201u,  192u,   77u,  486u,  381u,  275u,  311u,  238u,
         851u,  329u,   13u,  962u,  683u,  169u,  898u,  620u,  643u,  288u,
         125u,  305u,  251u,  687u,  454u,  577u,  784u,  705u,  405u,  896u,
         742u,  422u,  474u,  229u,  726u,  215u,  909u,  471u,  401u,   44u,
         872u,   92u,  885u,  475u,  411u,  444u,  438u,  273u,  666u,  196u,
         113u,   84u,   17u,  814u,  481u,   45u,   34u,  416u,   88u,  538u,
         773u,  108u,  522u,  977u,  829u,  129u,  823u,  467u,  521u,  468u,
         341u,  154u,  313u,  582u,  706u,  749u,  856u,  558u,   73u,   76u,
         217u,  519u,  455u,   64u,  670u,  763u,  980u,  581u,  418u,  106u,
         806u,  570u,  645u,  130u,   51u,  655u, 1018u,   26u,   62u,  744u,
         497u,  641u,  956u,  182u,  887u,  477u, 1007u,  241u,  828u,  785u,
         808u,  227u,  743u,  876u,  231u,  576u,  115u,  211u,  780u,   75u,
         553u,  579u,  861u,   32u,  996u,  195u,  220u,  881u,  600u,  158u,
         567u,  410u,  107u,  649u
};

/* File format of string database files:                                    */
/*                                                                          */
/* Bytes 0 to 3:    magic number (the string "cStr")                        */
/* Bytes 4, 5:      file version (two bytes, big-endian, should be 0x1000)  */
/* Bytes 6, 7:      language code (two bytes, big-endian; see n_getstr.h)   */
/* Bytes 8 to 11:   number of strings (four bytes, big-endian, must be > 0) */
/* From byte 12:                                                            */
/*   list of strings, in the following format:                              */
/*     0x81     (1 byte)                                                    */
/*     original string, terminated with '\0' (maximum length is 16383)      */
/*     0x82     (1 byte)                                                    */
/*     translated string, terminated with '\0' (maximum length is 16383)    */
/*   there should be as many string pairs as defined by bytes 8 to 11.      */

#define CSSTRNGS_VERSION 0x1000

static const char *language_names[] = {
    "(default)",
    "Afrikaans",
    "Albanian",
    "Arabic",
    "Armenian",
    "Assamese",
    "Azeri",
    "Basque",
    "Belarusian",
    "Bengali",
    "Bulgarian",
    "Catalan",
    "Chinese",
    "Croatian",
    "Czech",
    "Danish",
    "Dutch",
    "English (UK)",
    "English (US)",
    "Estonian",
    "Faeroese",
    "Farsi",
    "Finnish",
    "French",
    "Georgian",
    "German",
    "Greek",
    "Gujarati",
    "Hebrew",
    "Hindi",
    "Hungarian",
    "Icelandic",
    "Indonesian",
    "Italian",
    "Japanese",
    "Kannada",
    "Kashmiri",
    "Kazak",
    "Konkani",
    "Korean",
    "Latvian",
    "Lithuanian",
    "Macedonian",
    "Malay",
    "Malayalam",
    "Manipuri",
    "Marathi",
    "Nepali",
    "Norwegian",
    "Oriya",
    "Polish",
    "Portuguese",
    "Punjabi",
    "Romanian",
    "Russian",
    "Sanskrit",
    "Serbian",
    "Sindhi",
    "Slovak",
    "Slovenian",
    "Spanish",
    "Swahili",
    "Swedish",
    "Tamil",
    "Tatar",
    "Telugu",
    "Thai",
    "Turkish",
    "Ukrainian",
    "Urdu",
    "Uzbek",
    "Vietnamese"
};

typedef struct lclstr_s {
    char    *str;                       /* original string      */
    char    *str_tran;                  /* translated string    */
    struct lclstr_s                     /* pointer to previous  */
            *prv;                       /*   structure in chain */
} lclstr_t;

typedef struct l10ndb_s {
    cslanguage_t    lang_code;          /* language code        */
    lclstr_t        *h_tabl[1024];      /* 10 bit hash table    */
} l10ndb_t;

static  l10ndb_t    getstr_db = { CSLANGUAGE_DEFAULT, { NULL } };

/* Set default language and free all memory used by string database. */

static  void    free_string_database(void)
{
    int         i;
    lclstr_t    *p, *q;

    getstr_db.lang_code = CSLANGUAGE_DEFAULT;
    for (i = 0; i < 1024; i++) {
      p = getstr_db.h_tabl[i];
      while (p != NULL) {
        if (p->str != NULL)
          free(p->str);
        if (p->str_tran != NULL)
          free(p->str_tran);
        q = p->prv;
        free(p);
        p = q;
      }
      getstr_db.h_tabl[i] = NULL;
    }
}

/* Translate string 's' to the current language, and return pointer to      */
/* the translated message.                                                  */
/* This may be the same as 's' if language was set to CSLANGUAGE_DEFAULT,   */
/* or the string is not found in the database.                              */

PUBLIC char *csoundLocalizeString(const char *s)
{
    /* if default language, */
    if (getstr_db.lang_code == CSLANGUAGE_DEFAULT)
      return (char*) s;         /* return original string */
    else {
      int           h = 0;
      unsigned char *c = (unsigned char*) s - 1;
      lclstr_t      *p;
      /* calculate hash value */
      while (*++c) h = (int) name_hash_10(h, (int) *c);
      /* check table */
      p = getstr_db.h_tabl[h];
      while (p != NULL && strcmp(s, p->str) != 0) p = p->prv;
      if (p == NULL) {
        return (char*) s;       /* no entry, return original string */
      }
      else {
        return (char*) p->str_tran;     /* otherwise translate */
      }
    }
}

#define   READ_HDR_BYTE                 \
{                                       \
    i = (getc(f));                      \
    if (i == EOF) goto end_of_file;     \
    j = (j << 8) | i;                   \
}

#define   READ_DATA_BYTE                                        \
{                                                               \
    i = (getc(f));                                              \
    if (i == EOF) {                                             \
      fprintf(stderr, "%s: unexpected end of file\n", fname);   \
      goto end_of_file;                                         \
    }                                                           \
}

/* Load a single string database file.   */
/* Returns the number of strings loaded. */

static int load_language_file(const char *fname, cslanguage_t lang_code)
{
    int     i, j, nr_strings = 0, nr_reqd, h, max_length = 16384;
    char    *buf1, *buf2;
    unsigned char   *c;
    lclstr_t        *p;
    FILE    *f;

    /* allocate memory for string buffers */
    buf1 = (char*) malloc((size_t) max_length * sizeof(char));
    if (buf1 == NULL) {
      fprintf(stderr, "csoundSetLanguage: not enough memory\n");
      return 0;
    }
    buf2 = (char*) malloc((size_t) max_length * sizeof(char));
    if (buf2 == NULL) {
      fprintf(stderr, "csoundSetLanguage: not enough memory\n");
      return 0;
    }
    /* open file */
    f = fopen(fname, "rb");
    if (f == NULL)
      return 0;
    /* check magic number (4 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j != 0x63537472)                /* "cStr" */
      goto end_of_file;
    /* check version (2 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j != CSSTRNGS_VERSION) {
      goto end_of_file;
    }
    /* language code (2 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j != (int) lang_code)
      goto end_of_file;
    /* number of strings (4 bytes, big-endian) */
    j = 0; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE; READ_HDR_BYTE;
    if (j < 1) {
      fprintf(stderr, "csoundSetLanguage: %s: warning: no strings in file\n",
                      fname);
      goto end_of_file;
    }
    nr_reqd = j;
    /* read all strings from file */
    do {
      /* original string: */
      READ_DATA_BYTE;
      if (i != 0x81) {
        fprintf(stderr, "csoundSetLanguage: %s: corrupted file\n", fname);
        goto end_of_file;
      }
      c = (unsigned char*) buf1;
      h = 0;
      j = 0;
      /* read, and calculate hash value */
      while (1) {
        READ_DATA_BYTE;
        *c++ = (unsigned char) i;
        if (!i) break;
        h = (int) name_hash_10(h, i);
        if (++j >= max_length) {
          fprintf(stderr, "csoundSetLanguage: %s: error: string length > %d\n",
                          fname, max_length);
          goto end_of_file;
        }
      }
      /* translated string: */
      READ_DATA_BYTE;
      if (i != 0x82) {
        fprintf(stderr, "csoundSetLanguage: %s: corrupted file\n", fname);
        goto end_of_file;
      }
      c = (unsigned char*) buf2;
      j = 0;
      /* read */
      do {
        if (j >= max_length) {
          fprintf(stderr, "csoundSetLanguage: %s: error: string length > %d\n",
                          fname, max_length);
          goto end_of_file;
        }
        READ_DATA_BYTE;
        *c++ = (unsigned char) i;
        j++;
      } while (i);
      /* check database: if string is already defined, ignore new definition */
      p = getstr_db.h_tabl[h];
      while (p != NULL && strcmp(p->str, buf1) != 0) p = p->prv;
      if (p != NULL) continue;
      /* store in database */
      p = (lclstr_t*) malloc(sizeof(lclstr_t));
      if (p == NULL) {
        fprintf(stderr, "csoundSetLanguage: not enough memory\n");
        goto end_of_file;
      }
      p->str = (char*) malloc((size_t) strlen(buf1) + (size_t) 1);
      if (p->str == NULL) {
        free(p);
        fprintf(stderr, "csoundSetLanguage: not enough memory\n");
        goto end_of_file;
      }
      p->str_tran = (char*) malloc((size_t) strlen(buf2) + (size_t) 1);
      if (p->str_tran == NULL) {
        free(p->str); free(p);
        fprintf(stderr, "csoundSetLanguage: not enough memory\n");
        goto end_of_file;
      }
      strcpy(p->str, buf1);
      strcpy(p->str_tran, buf2);
      p->prv = getstr_db.h_tabl[h];
      getstr_db.h_tabl[h] = p;
      nr_strings++;             /* successfully loaded */
    } while (--nr_reqd);

 end_of_file:
    fclose(f);
    free(buf2);
    free(buf1);
    return nr_strings;
}

/* Set language to 'lang_code' (lang_code can be for example            */
/* CSLANGUAGE_ENGLISH_UK or CSLANGUAGE_FRENCH or many others, see       */
/* n_getstr.h for the list of languages).                               */
/* This affects all Csound instances running in the address space       */
/* of the current process.                                              */
/* The special language code CSLANGUAGE_DEFAULT can be used to disable  */
/* translation of messages and free all memory allocated by a previous  */
/* call to csoundSetLanguage().                                         */
/* csoundSetLanguage() loads all files for the selected language from   */
/* the directory specified by the CSSTRNGS environment variable.        */

PUBLIC void csoundSetLanguage(cslanguage_t lang_code)
{
    int     nr_strings = 0;
    char    *dirnam;
#ifdef HAVE_DIRENT_H
    int     i;
    char    *s, dir_name[1024], file_name[1024];
    DIR     *dirptr;
    struct dirent   *ent;
#endif

    if (lang_code == CSLANGUAGE_DEFAULT)
      fprintf(stderr, "Localisation of messages is disabled, using "
                      "default language.\n");
    else
      fprintf(stderr, "Setting language of messages to %s ...\n",
                      language_names[(int) lang_code]);
    if (getstr_db.lang_code == lang_code)
      return;

    free_string_database();
    /* set language code */
    getstr_db.lang_code = lang_code;
    if (lang_code == CSLANGUAGE_DEFAULT)
      return;

    /* load all files from CSSTRNGS directory */
    dirnam = getenv("CSSTRNGS");
    if (dirnam == NULL)
      return;

#ifdef HAVE_DIRENT_H
    /* directory name */
    strcpy((char*) dir_name, dirnam);
    i = strlen(dir_name);
    if (i > 0) {
      /* strip any trailing pathname delimiter */
      if (dir_name[i - 1] == DIRSEP || dir_name[i - 1] == '/') i--;
      /* and now add one to make sure there is exactly one */
      dir_name[i++] = DIRSEP;
      dir_name[i] = '\0';
    }
    dirptr = opendir(dirnam);
    if (dirptr == NULL)
      return;
    while ((ent = readdir(dirptr)) != NULL) {
      s = ent->d_name;                  /* file name        */
      strcpy(file_name, dir_name);      /* with full path   */
      strcat(file_name, s);
      nr_strings += load_language_file(file_name, lang_code);
    }
    closedir(dirptr);
#else
    /* if dirent.h is not available, CSSTRNGS is expected to */
    /* point to a single file */
    nr_strings += load_language_file(dirnam, lang_code);
#endif
    fprintf(stderr, " ... done, %d strings loaded.\n", nr_strings);
}

/* ------------------------------------------------------------------------- */

typedef struct LanguageSpec_s {
    char    *langname;
    int     langcode;
} LanguageSpec_t;

static LanguageSpec_t lang_list[] = {
    { "English_UK",     CSLANGUAGE_ENGLISH_UK   },
    { "uk",             CSLANGUAGE_ENGLISH_UK   },
    { "English",        CSLANGUAGE_ENGLISH_UK   },
    { "English_US",     CSLANGUAGE_ENGLISH_US   },
    { "us",             CSLANGUAGE_ENGLISH_US   },
    { "French",         CSLANGUAGE_FRENCH       },
    { "fr",             CSLANGUAGE_FRENCH       },
    { "German",         CSLANGUAGE_GERMAN       },
    { "de",             CSLANGUAGE_GERMAN       },
    { "Italian",        CSLANGUAGE_ITALIAN      },
    { "it",             CSLANGUAGE_ITALIAN      },
    { "Spanish",        CSLANGUAGE_SPANISH      },
    { "es",             CSLANGUAGE_SPANISH      },
    { NULL, -1 },
};

void init_getstring(int argc, char **argv)
{
    char    *s;
    int     n;

    argc = argc;
    argv = argv;

    s = getenv("CS_LANG");
    if (s == NULL)
      csoundSetLanguage(CSLANGUAGE_DEFAULT);
    else {
      n = -1;
      while (lang_list[++n].langname != NULL) {
        if (strcmp(s, lang_list[n].langname) == 0)
          break;
      }
      if (lang_list[n].langname != NULL)
        n = lang_list[n].langcode;
      else
        n = CSLANGUAGE_DEFAULT;
      csoundSetLanguage(n);
    }
}

