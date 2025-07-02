
/* Csound launcher utility - written by Istvan Varga, Jan 2003 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(WIN32)
#include <process.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
typedef int int32_t;
#endif

/* default Csound executable */
#ifdef WIN32
char    default_csnd[] = "csound32 -W";
char    default_csnd_r[] = "csound32 -h";
#else
char    default_csnd[] = "csound -W";
char    default_csnd_r[] = "csound32 -h";
#endif

/* default Csound flags */
char    default_csfl[] = "-d -m135 -H1 -s";
char    default_csfl_r[] = "-m128 -H0 -o dac";

char    *orcname = NULL;
char    *sconame = NULL;
char    *midname = NULL;
char    *csdname = NULL;

char    dir_name[256], prefix[256];

int     nr_files = 0, max_files = 0;
char    **file_names = NULL;

/* returns non-zero if the specified file name has .orc extension */

int is_orc(char *s)
{
    int n = (int32_t) strlen(s);
    if (n < 5) return 0;
    --n; if (s[n] != 'C' && s[n] != 'c') return 0;
    --n; if (s[n] != 'R' && s[n] != 'r') return 0;
    --n; if (s[n] != 'O' && s[n] != 'o') return 0;
    --n; if (s[n] != '.') return 0;
    return 1;
}

/* returns non-zero if the specified file name has .sco extension */

int is_sco(char *s)
{
    int n = (int32_t) strlen(s);
    if (n < 5) return 0;
    --n; if (s[n] != 'O' && s[n] != 'o') return 0;
    --n; if (s[n] != 'C' && s[n] != 'c') return 0;
    --n; if (s[n] != 'S' && s[n] != 's') return 0;
    --n; if (s[n] != '.') return 0;
    return 1;
}

/* returns non-zero if the specified file name has .mid extension */

int is_mid(char *s)
{
    int n = (int32_t) strlen(s);
    if (n < 5) return 0;
    --n; if (s[n] != 'D' && s[n] != 'd') return 0;
    --n; if (s[n] != 'I' && s[n] != 'i') return 0;
    --n; if (s[n] != 'M' && s[n] != 'm') return 0;
    --n; if (s[n] != '.') return 0;
    return 1;
}

/* returns non-zero if the specified file name has .csd extension */

int is_csd(char *s)
{
    int n = (int32_t) strlen(s);
    if (n < 5) return 0;
    --n; if (s[n] != 'D' && s[n] != 'd') return 0;
    --n; if (s[n] != 'S' && s[n] != 's') return 0;
    --n; if (s[n] != 'C' && s[n] != 'c') return 0;
    --n; if (s[n] != '.') return 0;
    return 1;
}

/* split filename to directory and base name */

void split_filename(char *fullname, char *dir, char *bas)
{
    int m;
    /* if no filename was given */
    if (fullname == NULL || strlen(fullname) == 0) {
      *dir = '\0';
      *bas = '\0';
      return;
    }
    m = (int32_t) strlen(fullname);
    while (--m >= 0 &&
           fullname[m] != '/' && fullname[m] != '\\' && fullname[m] != ':');
    /* directory name */
    if (m < 0)
#ifdef WIN32
      strcpy(dir, ".\\");
#else
      strcpy(dir, "./");
#endif
    else {
      strncpy(dir, fullname, m + 1);
      dir[m + 1] = '\0';
    }
    /* base name */
    strcpy(bas, fullname + (m + 1));
}

#if 0 && defined(__MACH__)
/* There is something odd on OSX about dirent.h */
typedef void* DIR;
DIR opendir(const char *);
struct dirent *readdir(DIR*);
int closedir(DIR*);
#endif

void create_file_list(void)
{
    DIR             *d;
    struct dirent   *ep;
    char            tmp[256];

    d = opendir(dir_name);
    if (d == NULL) {
      fprintf(stderr, "cs: error opening directory %s: %s\n",
                      dir_name, strerror(errno));
      exit(-1);
    }
    /* find all files in directory */
    while ((ep = readdir(d)) != NULL) {
      if (nr_files >= max_files) {
        /* expand file name array */
        max_files = max_files + (max_files >> 2) + 16;
        file_names = (char**) realloc(file_names, sizeof(char*) * max_files);
        if (file_names == NULL) {
          fprintf(stderr, "cs: not enough memory\n"); exit(-1);
        }
      }
      file_names[nr_files] = (char*) malloc(sizeof(char) * 256);
      if (file_names[nr_files] == NULL) {
        fprintf(stderr, "cs: not enough memory\n"); exit(-1);
      }
      /* store new file name */
      split_filename(ep->d_name, tmp, file_names[nr_files]);
      nr_files++;
    }
    closedir(d);
}

int chr_cmp(char a, char b)
{
    if (a == b) return 2;       /* characters are exactly the same */
    if (isupper(a)) a = tolower(a);
    if (isupper(b)) b = tolower(b);
    if (a == b) return 1;       /* characters are the same if case is ignored */
    return 0;                   /* different characters */
}

int find_best_match(int file_type, char **s)
{
    int chars_match = 0, n = -1, m, i, j;
    *s = NULL;
    while (++n < nr_files) {
      /* first, check if the file is of the right type */
      if (file_type == 0) {
        if (!is_orc(file_names[n])) continue;   /* file type 0: orchestra */
      }
      else if (file_type == 1) {
        if (!is_sco(file_names[n])) continue;   /* file type 1: score */
      }
      else if (file_type == 2) {
        if (!is_mid(file_names[n])) continue;   /* file type 2: MIDI */
      }
      else if (file_type == 3) {
        if (!is_csd(file_names[n])) continue;   /* file type 3: CSD */
      }
      /* now find the number of characters matching the prefix */
      i = 2; j = -1;
      for (m = 0; m < (int) strlen(prefix); m++) {
        j = chr_cmp(file_names[n][m], prefix[m]);
        if (!j) {
          if (m < ((int) strlen(file_names[n]) - 4))
            j = -1;     /* does not match */
          break;
        }
        if (j < i) i = j;               /* if case was ignored */
      }
      if (j < 0) continue;
      /* an exact match is always preferred */
      if (m == (int) strlen(prefix) && m == (int) (strlen(file_names[n]) - 4))
        m++;
      /* the final value is the number of matching characters * 2 */
      /*   +1 if case is also correct */
      /*   +2 for exact match         */
      /*   +3 if both the above       */
      m = (m << 1) + (i - 1);
      /* if this is the best match so far, record it */
      if (m >= chars_match) {
        chars_match = m;
        *s = file_names[n];
      }
    }
    return chars_match;
}

void find_files(char *name)
{
    int   base_match = -1, n;
    int   orcmatch = 0, scomatch = 0, midmatch = 0, csdmatch = 0;
    char  *s = NULL;

    split_filename(name, dir_name, prefix);
    if (dir_name[0] == '\0' || prefix[0] == '\0') {
      fprintf(stderr, "cs: invalid or empty name\n");
      exit(-1);
    }
    create_file_list();
    /* minimum number of characters to match */
    while (++base_match < (int) strlen(prefix) && isalpha(prefix[base_match]));
    base_match <<= 1;           /* because find_best_match() returns *2 */
    /* try to find CSD file first */
    n = find_best_match(3, &s);
    if (n && n >= base_match) {
      csdname = (char*) malloc(sizeof(char) * 256);
      if (csdname == NULL) {
        fprintf(stderr, "cs: not enough memory\n"); exit(-1);
      }
      strcpy(csdname, dir_name);
      strncat(csdname, s, 255); csdname[255] = '\0';
      csdmatch = n;
    }
    /* now find orchestra or score */
    n = find_best_match(0, &s);
    if (n && n >= base_match) {
      orcname = (char*) malloc(sizeof(char) * 256);
      if (orcname == NULL) {
        fprintf(stderr, "cs: not enough memory\n"); exit(-1);
      }
      strcpy(orcname, dir_name);
      strcat(orcname, s);
      orcmatch = n;
    }
    n = find_best_match(1, &s);
    if (n && n >= base_match) {
      sconame = (char*) malloc(sizeof(char) * 256);
      if (sconame == NULL) {
        fprintf(stderr, "cs: not enough memory\n"); exit(-1);
      }
      strcpy(sconame, dir_name);
      strcat(sconame, s);
      scomatch = n;
    }
    /* decide whether to use the CSD file or the orc/sco pair */
    if ((orcmatch >= csdmatch && scomatch >= csdmatch) &&
        (orcmatch > csdmatch || scomatch > csdmatch)) {
      csdmatch = 0;
      if (csdname != NULL) {
        free(csdname);
        csdname = NULL;
      }
    }
    if (csdname == NULL && (orcname == NULL || sconame == NULL)) {
      fprintf(stderr, "cs: cannot find a CSD file or an orc/sco pair with ");
      fprintf(stderr, "the specified name\n");
      exit(-1);
    }
    if (csdname != NULL) {
      fprintf(stderr, "found CSD file:       %s\n", csdname);
      if (orcname != NULL) {            /* do not use both CSD and orc/sco */
        free(orcname);
        orcname = NULL;
      }
      if (sconame != NULL) {
        free(sconame);
        sconame = NULL;
      }
    }
    else {
      fprintf(stderr, "found orchestra file: %s\n", orcname);
      fprintf(stderr, "found score file:     %s\n", sconame);
    }
    /* finally, search for a MIDI file */
    n = find_best_match(2, &s);
    if (n && n >= base_match) {
      midname = (char*) malloc(sizeof(char) * 256);
      if (midname == NULL) {
        fprintf(stderr, "cs: not enough memory\n"); exit(-1);
      }
      strcpy(midname, dir_name);
      strcat(midname, s);
      midmatch = n;
    }
    if (midmatch < csdmatch || midmatch < orcmatch || midmatch < scomatch) {
      midmatch = 0;
      if (midname != NULL) {
        free(midname);
        midname = NULL;
      }
    }
    if (midname != NULL) {
      fprintf(stderr, "found MIDI file:      %s\n", midname);
    }
    /* free file name list */
    for (n = 0; n < nr_files; n++) {
      free(file_names[n]);
    }
    if (file_names != NULL) {
      free(file_names);
      file_names = NULL;
    }
}

void copy_options(char **dst, char *src)
{
    int   i = -1;
    int   j = 0;        /* 1 if copying an option (2 if quoted) */
    char  *s = *dst;

    while (++i < (int) strlen(src)) {
      if (!j) {
        /* skip whitespace between options */
        if (isblank(src[i])) continue;
        /* if quoted option */
        if (src[i] == '"') {
          j = 2; *s++ = ' '; *s++ = '"'; continue;
        }
        /* otherwise simple option */
        *s++ = ' '; *s++ = '"'; j = 1;
      }
      if (j == 1 && isblank(src[i])) {
        /* whitespace can terminate simple option, but not quoted one */
        *s++ = '"'; j = 0; continue;
      }
      if (src[i] == '"') {
        /* quote: terminates option */
        *s++ = '"';
        if (j == 1) {
          /* and if it was a simple option, also begins a new (quoted) one */
          *s++ = ' '; *s++ = '"'; j = 2;
        }
        else
          j = 0;        /* otherwise no longer in an option */
        continue;
      }
      /* copy option text */
      *s++ = src[i];
    }
    /* terminate last option if necessary */
    if (j)
      *s++ = '"';
    /* store new pointer */
    *dst = s;
}

int main(int argc, char **argv)
{
    int     i, j, k, opts[26];
    char    tmp[256], cmdline[1024], optlst[256], extra_opts[1024], c, *s, *s2;
    char    tmp2[256], *cs_argv[256];

    if (argc < 2) {
      fprintf(stderr, "usage: cs [-OPTIONS] <name> [CSOUND OPTIONS ... ]\n");
      exit(-1);
    }
    i = 0; j = argc;
    tmp[0] = '\0';
    cmdline[0] = '\0';
    optlst[0] = '\0';
    extra_opts[0] = '\0';
    memset(opts, 0, sizeof(int) * 26);
    /* parse command line options */
    while (--j) {
      i++;
      if (argv[i][0] != '-' && i < 3 && tmp[0] == '\0') {
        strncpy(tmp, argv[i],255);
        /* strip extension */
        if (is_orc(tmp) || is_sco(tmp) || is_mid(tmp) || is_csd(tmp))
          tmp[strlen(tmp) - 4] = '\0';
        if (tmp[0] == '\0') {
          fprintf(stderr, "cs: invalid or empty name\n");
          exit(-1);
        }
      }
      else if (argv[i][0] == '-' && i == 1) {
        int n = 0;
        /* option list */
        while (++n < (int) strlen(argv[i])) {
          c = argv[i][n];
          if (islower(c)) c = toupper(c);
          if (c < 'A' || c > 'Z') {
            fprintf(stderr, "cs: invalid option: %c\n", argv[i][n]);
            exit(-1);
          }
          opts[(int) c - 'A'] = 1;
          optlst[n - 1] = c;
        }
        optlst[n] = '\0';
      }
      else {
        /* anything else is Csound flags */
        strcat(extra_opts, " \"");
        strncat(extra_opts, argv[i], 1018);
        strcat(extra_opts, "\"");
      }
    }
    if (tmp[0] == '\0') {
      fprintf(stderr, "cs: no name was specified\n");
      exit(-1);
    }
    /* set filenames */
    find_files(tmp);
    /* find csound executable */
    tmp[0] = '\0';
    /*   first, search environment variables, based on option list */
    /*   (the last option has the highest precedence) */
    s = getenv("CSOUND");
    if (s != NULL)      /* get default setting from CSOUND, if available */
      strncpy(tmp, s, 255);
    for (i = (int) strlen(optlst); --i >= 0; ) {
      snprintf(tmp2, 256, "CSOUND_%c", optlst[i]);
      s = getenv(tmp2);
      if (s != NULL) {
        strncpy(tmp, s, 255);
        if (tmp[0] != '\0') break;
      }
    }
    /*   if nothing was found, use default settings */
    if (tmp[0] == '\0') {
      if      (opts[17]) strncpy(tmp, default_csnd_r, 256);
      else               strncpy(tmp, default_csnd, 256);
    }
    /* convert this to a list of quoted options */
    s2 = cmdline;
    copy_options(&s2, tmp);
    /* add default flags, */
    s = getenv("CSFLAGS");              /* from CSFLAGS, if possible */
    if (s == NULL) s = default_csfl;
    copy_options(&s2, s);
    /* any options from the environment, */
    for (i = 0; i < (int) strlen(optlst); i++) {
      snprintf(tmp2, 256, "CSFLAGS_%c", optlst[i]);
      s = getenv(tmp2);
      if (s != NULL)
        copy_options(&s2, s);
      else if (optlst[i] == 'R')                /* built-in defaults for */
        copy_options(&s2, default_csfl_r);      /* realtime use */
    }
    /* and those specified on the command line */
    copy_options(&s2, extra_opts);
    /* add flag for MIDI file if there is any */
    /* IV - Jan 30 2003: this is not done if the user has specified any */
    /* MIDI device or file, so that the automatic use of MIDI file can */
    /* be overridden */
    *s2 = '\0';
    if (midname != NULL &&
        strstr(cmdline, "\"-M\"") == NULL &&
        strstr(cmdline, "\"-F\"") == NULL) {
      copy_options(&s2, "-F");
      copy_options(&s2, midname);
    }
    /* unless there is any user specified output (e.g. stdout or real-time), */
    /* or output is disabled, add output file */
    *s2 = '\0';
    if (strstr(cmdline, "\"-n\"") == NULL &&
        strstr(cmdline, "\"-o\"") == NULL) {    /* IV - Jan 30 2003 */
      char  tmp3[256];
      /* of all the input files, find the one with the longest name */
      /* for determining filename prefix */
      tmp[0] = '\0';
      if (orcname != NULL && strlen(orcname) > strlen(tmp))
        strcpy(tmp, orcname);
      if (sconame != NULL && strlen(sconame) > strlen(tmp))
        strcpy(tmp, sconame);
      if (midname != NULL && strlen(midname) > strlen(tmp))
        strcpy(tmp, midname);
      if (csdname != NULL && strlen(csdname) > strlen(tmp))
        strcpy(tmp, csdname);
      /* set extension depending on file type */
      i = (int32_t) strlen(tmp) - 4;
      tmp[i] = '\0';
      if (strstr(cmdline, "\"-J\"") != NULL)                    /* IRCAM */
        strcat(tmp, ".sf");
      else if (strstr(cmdline, "\"-A\"") != NULL)               /* AIFF */
        strcat(tmp, ".aif");
      else if (strstr(cmdline, "\"-h\"") != NULL)               /* RAW */
        strcat(tmp, ".pcm");
      else                                                      /* WAVE */
        strcat(tmp, ".wav");
      /* add output file to options */
      copy_options(&s2, "-o");
      split_filename(tmp, tmp2, tmp3);  /* always write to current directory */
      copy_options(&s2, tmp3);
    }
    /* finally, add the input files */
    if (csdname != NULL) copy_options(&s2, csdname);
    if (orcname != NULL) copy_options(&s2, orcname);
    if (sconame != NULL) copy_options(&s2, sconame);
    *s2 = '\0';
    /* split command line to separate strings */
    s2 = cmdline;
    j = k = 0;
    while (*s2 != '\0') {
      if (*s2 == '"') {
        if (k == 0) {
          cs_argv[j] = s2 + 1; j++;     /* begin option */
        }
        else
          *s2 = '\0';                   /* end of option */
        k = 1 - k;
      }
      s2++;
    }
    cs_argv[j] = NULL;                  /* terminate list */
    if (j < 1) {        /* assertion only */
      fprintf(stderr, "cs: internal error: no command to execute\n");
      exit(-1);
    }
    /* print the command line with all options */
    fprintf(stderr, "command line:        ");
    for (i = 0; i < j; i++) {
      if (strchr(cs_argv[i], ' ') || strchr(cs_argv[i], '\t') ||
          strchr(cs_argv[i], '\r') || strchr(cs_argv[i], '\n'))
        fprintf(stderr, " \"%s\"", cs_argv[i]);         /* if need to quote */
      else
        fprintf(stderr, " %s", cs_argv[i]);
    }
    fprintf(stderr, "\n");
    /* program name */
    split_filename(cs_argv[0], tmp, tmp2);
    strncpy(tmp, cs_argv[0], 255); tmp[255] = '\0'; /* with path */
    cs_argv[0] = tmp2;                          /* and without it */
    /* free all memory */
    if (orcname != NULL) free(orcname);
    if (sconame != NULL) free(sconame);
    if (midname != NULL) free(midname);
    if (csdname != NULL) free(csdname);
    /* execute command */
#ifndef __wasm__
    if (execvp(tmp, cs_argv)) {
      fprintf(stderr, "cs: error executing Csound command: %s\n",
                      strerror(errno));
      exit(-1);
    }
#endif

    return 0;
}

