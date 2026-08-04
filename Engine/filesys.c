/*
    files.c: filesystem and environment functions

    Copyright (C) 2005-25 Istvan Varga, Victor Lazzarini

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "csoundCore.h"
#include "soundfile.h"
#include "soundio.h"
#include "filesys.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#if defined(MSVC) || defined(__wasi__)
#include <fcntl.h>
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  define getcwd(x,y) _getcwd(x,y)
#endif

#include "namedins.h"

#if defined(MSVC)
#define RD_OPTS  _O_RDONLY | _O_BINARY
#define WR_OPTS  _O_TRUNC | _O_CREAT | _O_WRONLY | _O_BINARY,_S_IWRITE
#elif defined(WIN32)
#define RD_OPTS  O_RDONLY | O_BINARY
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#elif defined DOSGCC
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#else
#ifndef O_BINARY
# define O_BINARY (0)
#endif
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#endif


/** Check if file name is valid, and copy with converting pathname delimiters */
static char *convert_path_name(CSOUND *csound, const char *filename) {
    char  *name;
    int32_t   i = 0;

/* FIXMEs:  need to convert ':' from Mac pathnames (but be careful of not
   messing up Windows drive names!); need to be careful of
   relative paths containing "./", "../", or multiple colons "::"; need to
   collapse multiple slashes "//" or "\\\\" ??  */
    if (filename == NULL || filename[0] == '\0')
      return NULL;
    name = (char*) csound->Malloc(csound, (size_t) strlen(filename) + (size_t) 1);
    do {
      if (filename[i] != '/' && filename[i] != '\\')
        name[i] = filename[i];
      else
        name[i] = DIRSEP;
    } while (filename[i++] != '\0');
    if (name[i - 2] == DIRSEP
#ifdef WIN32
        || (isalpha(name[0]) && name[1] == ':' && name[2] == '\0')
#endif
        ) {
      csound->Free(csound, name);
      return NULL;
    }
    return name;
}

/**  Check if name is a full pathname for the platform we are running on. */
static int32_t is_name_full_path(const char *name)
{
#ifdef WIN32
    if (isalpha(name[0]) && name[1] == ':') return 1;
#endif
    if (name[0] == DIRSEP) /* ||
        (name[0] == '.' && (name[1] == DIRSEP ||
                            (name[1] == '.' && name[2] == DIRSEP)))) */
      return 1;
    return 0;
}


/** Properly concatenates the full or relative pathname in path1 with
 *  the relative pathname or filename in path2 according to the rules
 *  for the platform we are running on.  path1 is assumed to be
 *  a directory whether it ends with DIRSEP or not.  Relative paths must
 *  conform to the conventions for the current platform (begin with ':'
 *  on MacOS 9 and not begin with DIRSEP on others).
 */
char* csound_concatenate_paths(CSOUND* csound, const char *path1,
                             const char *path2)
{
    char *result;
    const char *start2;
    char separator[2];
    int32_t  len1 = (int32_t) strlen(path1);
    int32_t  len2 = (int32_t) strlen(path2);

    /* cannot join two full pathnames -- so just return path2 ? */
    if (is_name_full_path(path2)) {
        result = (char*) csound->Malloc(csound, (size_t)len2+1);
        strcpy(result, path2);
        return result;
    }

    start2 = path2;
    /* ignore "./" at the beginning */
    if (path2[0] == '.' && path2[1] == DIRSEP) start2 = path2 + 2;

    result = (char*) csound->Malloc(csound, (size_t)len1+(size_t)len2+2);
    strcpy(result, path1);
    /* check for final DIRSEP in path1 */
    if (path1[len1-1] != DIRSEP) {
        separator[0] = DIRSEP; separator[1] = '\0';
        strcat(result, separator);
    }
    strcat(result, start2);

    return result;
}

/** Converts a pathname to native format and returns just the part of
 *  the path that specifies the directory.  Does not return the final
 *  DIRSEP.  Returns an empty string if no path components occur before
 *  the filename.  Returns NULL if unable to carry out the operation
 *  for some reason.
 */
static char *split_directory_from_path(CSOUND* csound, const char * path)
{
    char *convPath;
    char *lastIndex;
    char *partialPath;
    int32_t  len;

    if ((convPath = convert_path_name(csound, path)) == NULL)
        return NULL;
    lastIndex = strrchr(convPath, DIRSEP);

    if (lastIndex == NULL) {  /* no DIRSEP before filename */
#ifdef WIN32  /* e.g. C:filename */
        if (isalpha(convPath[0]) && convPath[1] == ':') {
            partialPath = (char*) csound->Malloc(csound, (size_t) 3);
            partialPath[0] = convPath[0];
            partialPath[1] = convPath[1];
            partialPath[2] = '\0';
            csound->Free(csound, convPath);
            return partialPath;
        }
#endif
        partialPath = (char*) csound->Malloc(csound, (size_t) 1);
        partialPath[0] = '\0';
    }
    else {
      len = (int32_t) (lastIndex - convPath);
        partialPath = (char*) csound->Malloc(csound, len+1);
        strNcpy(partialPath, convPath, len+1);
        //partialPath[len] = '\0';
   }
   csound->Free(csound, convPath);
   return partialPath;
}

/* given a file name as string, return full path of directory of file;
 * Note: does not check if file exists
 */
char *csound_get_directory_for_path(CSOUND* csound, const char * path) {
#ifdef BARE_METAL
  (void) csound;
  (void) path;
  return NULL;
#else
    char *partialPath, *tempPath, *lastIndex;
    char *retval;
    char *cwd;
    int32_t  len;

    if (path == NULL) return NULL;

    tempPath = convert_path_name(csound, path);
    if (tempPath == NULL) return NULL;
    lastIndex = strrchr(tempPath, DIRSEP);

    if (tempPath && is_name_full_path(tempPath)) {
      /* check if root directory */
      if (lastIndex == tempPath) {
        partialPath = (char *)csound->Malloc(csound, 2);
        partialPath[0] = DIRSEP;
        partialPath[1] = '\0';

        csound->Free(csound, tempPath);

        return partialPath;
      }

#  ifdef WIN32
      /* check if root directory of Windows drive */
      if ((lastIndex - tempPath) == 2 && tempPath[1] == ':') {
        partialPath = (char *)csound->Malloc(csound, 4);
        partialPath[0] = tempPath[0];
        partialPath[1] = tempPath[1];
        partialPath[2] = tempPath[2];
        partialPath[3] = '\0';

        csound->Free(csound, tempPath);

        return partialPath;
      }
#  endif
      len = (int32_t)(lastIndex - tempPath);

      partialPath = (char *)csound->Calloc(csound, len + 1);
      strNcpy(partialPath, tempPath, len+1);

      csound->Free(csound, tempPath);

      return partialPath;
    }

    /* do we need to worry about ~/ on *nix systems ? */
    /* we have a relative path or just a filename */
    len = 32;
    cwd = csound->Malloc(csound, len);
 again:
    if (UNLIKELY(getcwd(cwd, len)==NULL)) {
      // Should check ERANGE==errno
      //csoundDie(csound, Str("Current directory path name too long\n"));
      len =len+len; cwd = csound->ReAlloc(csound, cwd, len);
      if (UNLIKELY(len>1024*1024))
        csoundDie(csound, Str("Current directory path name too long\n"));
      goto again;
    }

    if (lastIndex == NULL) {
      return cwd;
    }

    len = (int32_t)(lastIndex - tempPath);  /* could be 0 on OS 9 */

    partialPath = (char *)csound->Calloc(csound, len + 1);
    strNcpy(partialPath, tempPath, len+1);

    retval = csound_concatenate_paths(csound, cwd, partialPath);

    csound->Free(csound, cwd);
    csound->Free(csound, partialPath);
    csound->Free(csound, tempPath);

    return retval;
#endif
}

static SNDFILE *open_file_snd(CSOUND *csound, const char *path, int32_t mode, SFLIB_INFO *sfinfo){
    SNDFILE *sf = NULL;

    if(csound->OpenSoundFileCallback_ != NULL) {
        sf = csound->OpenSoundFileCallback_(csound, path, mode, sfinfo);
    }

    return sf;
}

static FILE *open_file_std(CSOUND *csound, char **fullName,
                            const char *filename, const char *mode)
{
    FILE *f = NULL;

    /* try to open file if callback is specified */
    if(csound->OpenFileCallback_ != NULL) {
        f = csound->OpenFileCallback_(csound, filename, (char*) mode);
        if (f != NULL) {
            *fullName = (char*) filename;
        }
    }

    /* fallback to fopen */
    if (f == NULL) {
#if defined(WIN32)
      /* To handle Widows errors in file name characters. */
      size_t sz = 2 * MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
      wchar_t *wfname = malloc(sz);
      wchar_t *wmode = 0;

      MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfname, sz);
      sz = 2 * MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);
      wmode = malloc(sz);
      MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, sz);
      f = _wfopen(wfname, wmode);
      if (UNLIKELY(f == NULL)) {
        /* csoundErrorMsg(csound, Str("csound->FileOpen2(\"%s\") failed: %s."), */
        /*                filename, strerror(errno)); */
        free(wfname);
        free(wmode);
      } else {
        *fullName = (char*) filename;
        free(wfname);
        free(wmode);
      }
#else
      f = fopen(filename, (char*) mode);
      if (LIKELY(f != NULL)) {
        *fullName = (char*) filename;
      }
#endif
    }

    return f;
}

static FILE *find_file_std(CSOUND *csound, char **fullName,
                                const char *filename, const char *mode,
                                const char *envList)
{
    FILE  *f;
    char  *name, *name2, **searchPath;

    *fullName = NULL;
    if ((name = convert_path_name(csound, filename)) == NULL)
      return (FILE*) NULL;
    if (mode[0] != 'w') {
      /* read: try the specified name first */
      f = open_file_std(csound, fullName, name, mode);
      if (f != NULL) {
        *fullName = name;
        return f;
      }
      /* if full path, and not found: */
      if (is_name_full_path(name)) {
        csound->Free(csound, name);
        return (FILE*) NULL;
      }
    }
    else if (is_name_full_path(name)) {
      /* if write and full path: */
      f = open_file_std(csound, fullName, name, mode);
      if (f != NULL)
        *fullName = name;
      else
        csound->Free(csound, name);
      return f;
    }
    /* search paths defined by environment variable list */
    if (envList != NULL && envList[0] != '\0' &&
        (searchPath = csoundGetSearchPathFromEnv((CSOUND*) csound, envList))
        != NULL) {
      //len = (int32_t) strlen(name) + 1;
      while (*searchPath != NULL) {
        name2 = csound_concatenate_paths(csound, *searchPath, name);
        f = open_file_std(csound, fullName, name2, mode);
        if (f != NULL) {
          csound->Free(csound, name);
          *fullName = name2;
          return f;
        }
        csound->Free(csound, name2);
        searchPath++;
      }
    }
    /* if write mode, try current directory last */
    if (mode[0] == 'w') {
      f = open_file_std(csound, fullName, name, mode);
      if (f != NULL) {
        *fullName = name;
        return f;
      }
    }
    /* not found */
    csound->Free(csound, name);
    return (FILE*) NULL;
}

static void overwrite_warning(CSOUND *csound, const char *name) {
  int32_t fd;
  fd = open(name, WR_OPTS | O_EXCL);
  if(fd == -1)
    csoundWarning(csound, "file %s exists...\n...will be overwritten", name);
  else 
    close(fd);
}

static int32_t find_file_fd(CSOUND *csound, char **fullName,
                             const char *filename, int32_t write_mode,
                             const char *envList)
{
    char  *name, *name2, **searchPath;
    int32_t   fd;

    *fullName = NULL;
    if ((name = convert_path_name(csound, filename)) == NULL)
      return -1;
    if (!write_mode) {
      /* read: try the specified name first */
      fd = open(name, RD_OPTS);
      if (fd >= 0) {
        *fullName = name;
        return fd;
      }
      /* if full path, and not found: */
      if (is_name_full_path(name)) {
        csound->Free(csound, name);
        return -1;
      }
    }
    else if (is_name_full_path(name)) {
      /* if write and full path: */
      overwrite_warning(csound, name);
      fd = open(name, WR_OPTS);
      if (fd >= 0)
        *fullName = name;
      else
        csound->Free(csound, name);
      return fd;
    }
    /* search paths defined by environment variable list */
    if (envList != NULL && envList[0] != '\0' &&
        (searchPath = csoundGetSearchPathFromEnv((CSOUND*) csound, envList))
        != NULL) {
      //len = (int32_t) strlen(name) + 1;
      while (*searchPath != NULL) {
        name2 = csound_concatenate_paths(csound, *searchPath, name);
        if (!write_mode)
          fd = open(name2, RD_OPTS);
        else
          fd = open(name2, WR_OPTS);
        if (fd >= 0) {
          csound->Free(csound, name);
          *fullName = name2;
          return fd;
        }
        csound->Free(csound, name2);
        searchPath++;
      }
    }
    /* if write mode, try current directory last */
    if (write_mode) {
      overwrite_warning(csound, name);
      fd = open(name, WR_OPTS);
      if (fd >= 0) {
        *fullName = name;
        return fd;
      }
    }
    /* not found */
    csound->Free(csound, name);
    return -1;
}

static SNDFILE *find_file_snd(CSOUND *csound, char **fullName,
                             const char *filename, int32_t write_mode,
                             SFLIB_INFO *sfinfo, const char *envList)
{
    char  *name, *name2, **searchPath;
    SNDFILE *sf;

    *fullName = NULL;
    if ((name = convert_path_name(csound, filename)) == NULL)
      return NULL;
    if (!write_mode) {
      /* read: try the specified name first */
      sf = open_file_snd(csound, name, SFM_READ, sfinfo);
      if (sf != NULL) {
        *fullName = name;
        return sf;
      }
      /* if full path, and not found: */
      if (is_name_full_path(name)) {
        csound->Free(csound, name);
        return NULL;
      }
    }
    else if (is_name_full_path(name)) {
      /* if write and full path: */
      sf = open_file_snd(csound, name, SFM_WRITE, sfinfo);
      if (sf != NULL)
        *fullName = name;
      else
        csound->Free(csound, name);
      return sf;
    }
    /* search paths defined by environment variable list */
    if (envList != NULL && envList[0] != '\0' &&
        (searchPath = csoundGetSearchPathFromEnv((CSOUND*) csound, envList))
        != NULL) {
      //len = (int32_t) strlen(name) + 1;
      while (*searchPath != NULL) {
        name2 = csound_concatenate_paths(csound, *searchPath, name);
        if (!write_mode)
          sf = open_file_snd(csound, name2, SFM_READ, sfinfo);
        else
          sf = open_file_snd(csound, name2, SFM_WRITE, sfinfo);
        if (sf != NULL) {
          csound->Free(csound, name);
          *fullName = name2;
          return sf;
        }
        csound->Free(csound, name2);
        searchPath++;
      }
    }
    /* if write mode, try current directory last */
    if (write_mode) {
      sf = open_file_snd(csound, name, SFM_WRITE, sfinfo);
      if (sf != NULL) {
        *fullName = name;
        return sf;
      }
    }
    /* not found */
    csound->Free(csound, name);
    return NULL;
}

/**
 * Search for input file 'filename'.
 * If the file name specifies full path (it begins with '.', the pathname
 * delimiter character, or a drive letter and ':' on Windows), that exact
 * file name is tried without searching.
 * Otherwise, the file is searched relative to the current directory first,
 * and if it is still not found, a pathname list that is created the
 * following way is searched:
 *   1. if envList is NULL or empty, no directories are searched
 *   2. envList is parsed as a ';' or ':' separated list of environment
 *      variable names, and all environment variables are expanded and
 *      expected to contain a ';' or ':' separated list of directory names
 *   2. all directories in the resulting pathname list are searched, starting
 *      from the last and towards the first one, and the directory where the
 *      file is found first will be used
 * The function returns a pointer to the full name of the file if it is
 * found, and NULL if the file could not be found in any of the search paths,
 * or an error has occured. The caller is responsible for freeing the memory
 * pointed to by the return value, by calling csound->Free().
 */
char *csoundFindInputFile(CSOUND *csound,
                          const char *filename, const char *envList)
{
    char  *name_found;
    int32_t   fd;

    if (csound == NULL)
      return NULL;
    fd = find_file_fd(csound, &name_found, filename, 0, envList);
    if (fd >= 0)
      close(fd);
    return name_found;
}

/**
 * Search for a location to write file 'filename'.
 * If the file name specifies full path (it begins with '.', the pathname
 * delimiter character, or a drive letter and ':' on Windows), that exact
 * file name is tried without searching.
 * Otherwise, a pathname list that is created the following way is searched:
 *   1. if envList is NULL or empty, no directories are searched
 *   2. envList is parsed as a ';' separated list of environment variable
 *      names, and all environment variables are expanded and expected to
 *      contain a ';' separated list of directory names
 *   2. all directories in the resulting pathname list are searched, starting
 *      from the last and towards the first one, and the directory that is
 *      found first where the file can be written to will be used
 * Finally, if the file cannot be written to any of the directories in the
 * search paths, writing relative to the current directory is tried.
 * The function returns a pointer to the full name of the file if a location
 * suitable for writing the file is found, and NULL if the file cannot not be
 * written anywhere in the search paths, or an error has occured.
 * The caller is responsible for freeing the memory pointed to by the return
 * value, by calling csound->Free().
 */
char *csoundFindOutputFile(CSOUND *csound,
                           const char *filename,
                           const char *envList) {
    char  *name_found;
    int32_t   fd;

    if (csound == NULL)
      return NULL;
    fd = find_file_fd(csound, &name_found, filename, 1, envList);
    if (fd >= 0) {
      close(fd);
      /* since find_file_fd() creates an empty file in
         the process of checking if it can be written
         we remove it here so the filesystem is as before.
      */
      if(remove(name_found)<0)
        csound->DebugMsg(csound, Str("Remove failed\n"));
    }
    return name_found;
}

/**
 * Open a file and return handle.
 *
 * CSOUND *csound:
 *   Csound instance pointer
 * void *fd:
 *   pointer a variable of type int32_t,  FILE*, or SNDFILE*, depending on 'type',
 *   for storing handle to be passed to file read/write functions
 * int32_t type:
 *   file type, one of the following:
 *     CSFILE_FD_R:     read file using low level interface (open())
 *     CSFILE_FD_W:     write file using low level interface (open())
 *     CSFILE_STD:      use ANSI C interface (fopen())
 *     CSFILE_SND_R:    read sound file
 *     CSFILE_SND_W:    write sound file
 * const char *name:
 *   file name
 * void *param:
 *   parameters, depending on type:
 *     CSFILE_FD_R:     unused (should be NULL)
 *     CSFILE_FD_W:     unused (should be NULL)
 *     CSFILE_STD:      mode parameter (of type char*) to be passed to fopen()
 *     CSFILE_SND_R:    SFLIB_INFO* parameter for csound->SndfileOpen(csound,), with defaults for
 *                      raw file; the actual format paramaters of the opened
 *                      file will be stored in this structure
 *     CSFILE_SND_W:    SFLIB_INFO* parameter for csound->SndfileOpen(csound,), output file format
 * const char *env:
 *   list of environment variables for search path (see csoundFindInputFile()
 *   for details); if NULL, the specified name is used as it is, without any
 *   conversion or search.
 * int32_t csFileType:
 *   A value from the enumeration CSOUND_FILETYPES (see soundCore.h)
 * int32_t isTemporary:
 *   1 if this file will be deleted when Csound is finished.
 *   Otherwise, 0.
 * return value:
 *   opaque handle to the opened file, for use with csoundGetFileName() or
 *   csoundFileClose(), or storing in FDCH.fd.
 *   On failure, NULL is returned.
 */

/* Realtime init and note deactivation may update this registry concurrently.
   Pointer updates use the spinlock. When the file worker is running, removed
   nodes are reclaimed only after its current traversal has completed. */
enum {
  FILE_IO_STARTING = -1,
  FILE_IO_STOPPED,
  FILE_IO_RUNNING
};

enum {
  FILE_ASYNC_NONE = 0,
  FILE_ASYNC_STARTING = -1,
  FILE_ASYNC_RUNNING = ASYNC_GLOBAL
};

static void link_open_file(CSOUND *csound, CSFILE *file,
                           int32_t setupReaders)
{
    csoundSpinLock(&csound->open_files_lock);
    file->retired_nxt = NULL;
    file->io_pass = 0;
    file->io_readers = setupReaders;
    file->retired = 0;
    file->prv = NULL;
    file->nxt = (CSFILE*) csound->open_files;
    if (file->nxt != NULL)
      file->nxt->prv = file;
    csound->open_files = (void*) file;
    csoundSpinUnLock(&csound->open_files_lock);
}

static int32_t unlink_open_file(CSOUND *csound, CSFILE *file,
                                int32_t holdForClose)
{
    int32_t deferred;

    csoundSpinLock(&csound->open_files_lock);
    if (file->retired) {
      csoundSpinUnLock(&csound->open_files_lock);
      return 1;
    }
    if (file->prv == NULL)
      csound->open_files = (void*) file->nxt;
    else
      file->prv->nxt = file->nxt;
    if (file->nxt != NULL)
      file->nxt->prv = file->prv;
    /* Only the node currently borrowed by the file worker needs deferred
       reclamation. Ordinary files retain synchronous close semantics even
       while an unrelated asynchronous file is open. */
    deferred = file->io_readers != 0;
    if (deferred) {
      /* A synchronous closer keeps one claim so the worker cannot reclaim the
         node before the closer can report the real close result. */
      if (holdForClose)
        file->io_readers++;
      file->retired = 1;
      file->retired_nxt = (CSFILE *) csound->retired_files;
      csound->retired_files = file;
    }
    else {
      file->nxt = file->prv = NULL;
    }
    csoundSpinUnLock(&csound->open_files_lock);
    return deferred;
}

/* An asynchronous open starts with one setup reader so shutdown can retire,
   but cannot reclaim, the node before its buffers and worker state are ready. */
static void *csoundFileOpenInternal(
  CSOUND *csound, void *fd, int32_t type, const char *name, void *param,
  const char *env, int32_t csFileType, int32_t isTemporary,
  int32_t setupReaders)
{
    CSFILE  *p = NULL;
    char    *fullName = NULL;
    FILE    *tmp_f = NULL;
    SNDFILE *tmp_sf = NULL;
    SFLIB_INFO sfinfo;
    int32_t     tmp_fd = -1, nbytes = (int32_t) sizeof(CSFILE);


    /* check file type */
    if (UNLIKELY((uint32_t) (type - 1) >= (uint32_t) CSFILE_SND_W)) {
      csoundErrorMsg(csound, Str("internal error: csoundFileOpen(): "
                                 "invalid type: %d"), type);
      return NULL;
    }
    /* get full name and open file */
    if (env == NULL) {
      if (type == CSFILE_STD) {
        tmp_f = open_file_std(csound, &fullName, name, param);
        if (UNLIKELY(tmp_f == NULL)) {
          /* csoundErrorMsg(csound, Str("csound->FileOpen(\"%s\") failed: %s."), */
          /*                name, strerror(errno)); */
          goto err_return;
        }
      }
      else {
        /* try to open sound file if callback is specified */
        if (csound->OpenSoundFileCallback_ != NULL) {
          memcpy(&sfinfo, param, sizeof(SFLIB_INFO));
          if (type == CSFILE_SND_R)
            tmp_sf = open_file_snd(csound, name, SFM_READ, &sfinfo);
          if (type == CSFILE_SND_W)
            tmp_sf = open_file_snd(csound, name, SFM_WRITE, &sfinfo);
        }
        /* fallback to open with fd */
        if (tmp_sf == (SNDFILE*) NULL) {
          fullName = (char*) name;
          if (type == CSFILE_SND_R || type == CSFILE_FD_R)
            tmp_fd = open(fullName, RD_OPTS);
          else
            tmp_fd = open(fullName, WR_OPTS);
          if (tmp_fd < 0)
            goto err_return;
        }
      }
    }
    else {
      if (type == CSFILE_STD) {
        tmp_f = find_file_std(csound, &fullName, name, (char*) param, env);
        if (UNLIKELY(tmp_f == NULL))
          goto err_return;
      }
      else {
        /* try to open sound file if callback is specified */
        if(csound->OpenSoundFileCallback_ != NULL) {
          memcpy(&sfinfo, param, sizeof(SFLIB_INFO));
          if (type == CSFILE_SND_R)
            tmp_sf = find_file_snd(csound, &fullName, name, 0, &sfinfo, env);
          if (type == CSFILE_SND_W)
            tmp_sf = find_file_snd(csound, &fullName, name, 1, &sfinfo, env);
        }
        /* fallback to open with fd */
        if (tmp_sf == (SNDFILE*) NULL) {
          if (type == CSFILE_SND_R || type == CSFILE_FD_R)
            tmp_fd = find_file_fd(csound, &fullName, name, 0, env);
          else
            tmp_fd = find_file_fd(csound, &fullName, name, 1, env);
          if (UNLIKELY(tmp_fd < 0))
            goto err_return;
        }
      }
    }
    nbytes += (int32_t) strlen(fullName);
    /* allocate file structure */
    p = (CSFILE*) csound->Malloc(csound, (size_t) nbytes);
    if (UNLIKELY(p == NULL))
      goto err_return;
    p->nxt = (CSFILE*) NULL;
    p->prv = (CSFILE*) NULL;
    p->type = type;
    p->fd = tmp_fd;
    p->f = tmp_f;
    p->sf = tmp_sf;
    p->cb = NULL;
    p->async_flag = FILE_ASYNC_NONE;
    p->items = 0;
    p->pos = 0;
    p->buf = NULL;
    p->bufsize = 0;
    strcpy(&(p->fullName[0]), fullName);
    if (env != NULL) {
      csound->Free(csound, fullName);
      env = NULL;
    }

    /* if sound file, re-open file descriptor with libsndfile if tmp_sf is null */
    switch (type) {
    case CSFILE_STD:                          /* stdio */
      *((FILE**) fd) = tmp_f;
      break;
    case CSFILE_SND_R:                        /* sound file read */
      if (p->sf != (SNDFILE*) NULL)
          goto doneSFOpen;
      memcpy(&sfinfo, param, sizeof(SFLIB_INFO));
      p->sf = csound->SndfileOpenFd(csound,tmp_fd, SFM_READ, &sfinfo, 0);
      if (p->sf == (SNDFILE*) NULL) {
        int32_t   extPos;
        /* open failed: */
        extPos = (nbytes - (int32_t) sizeof(CSFILE)) - 4;
        /* check for .sd2 file first */
        if (extPos > 0 &&
            p->fullName[extPos] == (char) '.' &&
            tolower(p->fullName[extPos + 1]) == (char) 's' &&
            tolower(p->fullName[extPos + 2]) == (char) 'd' &&
            p->fullName[extPos + 3] == (char) '2') {
          //memset(&sfinfo, 0, sizeof(SFLIB_INFO));
          p->sf = csound->SndfileOpen(csound,&(p->fullName[0]), SFM_READ, &sfinfo);
          if (p->sf != (SNDFILE*) NULL) {
            /* if successfully opened as .sd2, */
            /* the integer file descriptor is no longer needed */
            close(tmp_fd);
            p->fd = tmp_fd = -1;
            csound->SndfileCommand(csound,p->sf, SFC_SET_VBR_ENCODING_QUALITY,
                       &csound->oparms->quality, sizeof(double));
            goto doneSFOpen;
          }
        }
#if 0
        /* maybe raw file ? rewind and try again */
        if (lseek(tmp_fd, (off_t) 0, SEEK_SET) == (off_t) 0) {
          SFLIB_INFO *sf = (SFLIB_INFO*)param;
          sf->format = TYPE2SF(TYP_RAW) |  AE_SHORT ;
          sf->samplerate = csound->esr;
          //sf->channels = 1;//csound->inchnls;
          csound->Warning(csound,
                          Str("After open failure(%s)\n"
                              "will try to open %s as raw\n"),
                          csound->SndfileStrError(csound,NULL), fullName);
          p->sf = csound->SndfileOpenFd(csound,tmp_fd, SFM_READ, sf, 0);
        }
#endif
        if (UNLIKELY(p->sf == (SNDFILE*) NULL)) {
          /* csound->Warning(csound, Str("Failed to open %s: %s\n"), */
          /*                 fullName, csound->SndfileStrError(csound,NULL)); */
          goto err_return;
        }
      }
      else {
      doneSFOpen:
        memcpy((SFLIB_INFO*) param, &sfinfo, sizeof(SFLIB_INFO));
      }
      *((SNDFILE**) fd) = p->sf;
      break;
    case CSFILE_SND_W:                        /* sound file write */
      if (p->sf == (SNDFILE*) NULL) {
        p->sf = csound->SndfileOpenFd(csound,tmp_fd, SFM_WRITE, (SFLIB_INFO*) param, 0);
        if (UNLIKELY(p->sf == (SNDFILE*) NULL)) {
            csound->Warning(csound, Str("Failed to open %s: %s\n"),
                            fullName, csound->SndfileStrError(csound,NULL));
          goto err_return;
        }
      }
      csound->SndfileCommand(csound,p->sf, SFC_SET_CLIPPING, NULL, SFLIB_TRUE);
      csound->SndfileCommand(csound,p->sf, SFC_SET_VBR_ENCODING_QUALITY,
                 &csound->oparms->quality, sizeof(double));
      *((SNDFILE**) fd) = p->sf;
      break;
    default:                                  /* low level I/O */
      *((int*) fd) = tmp_fd;
    }
    /* link into chain of open files */
    link_open_file(csound, p, setupReaders);
    /* notify the host if it asked */
    if (csound->FileOpenCallback_ != NULL) {
      int32_t writing = (type == CSFILE_SND_W || type == CSFILE_FD_W ||
                     (type == CSFILE_STD && ((char*)param)[0] == 'w'));
      if (csFileType == CSFTYPE_UNKNOWN_AUDIO && type == CSFILE_SND_R)
        csFileType = csoundSndfileType2CsfileType(((SFLIB_INFO*)param)->format);
      csound->FileOpenCallback_(csound, p->fullName, csFileType,
                                writing, isTemporary);
    }
    /* return with opaque file handle */
    return (void*) p;

 err_return:
    /* clean up on error */
    if (p != NULL)
      csound->Free(csound, p);
    if (fullName != NULL && env != NULL)
      csound->Free(csound, fullName);
    if (tmp_fd >= 0)
      close(tmp_fd);
    else if (tmp_f != NULL)
      fclose(tmp_f);
    if (type > CSFILE_STD)
      *((SNDFILE**) fd) = (SNDFILE*) NULL;
    else if (type == CSFILE_STD)
      *((FILE**) fd) = (FILE*) NULL;
    else
      *((int*) fd) = -1;
    return NULL;
}

void *csoundFileOpen(CSOUND *csound, void *fd, int32_t type,
                     const char *name, void *param, const char *env,
                     int32_t csFileType, int32_t isTemporary)
{
    return csoundFileOpenInternal(csound, fd, type, name, param, env,
                                  csFileType, isTemporary, 0);
}


/**
 * Allocate a file handle for an existing file already opened with open(),
 * fopen(), or csound->SndfileOpen(csound,), for later use with csoundFileClose() or
 * csoundGetFileName(), or storing in an FDCH structure.
 * Files registered this way (or opened with csoundFileOpen()) are also
 * automatically closed by csoundReset().
 * Parameters and return value are similar to csoundFileOpen(), except
 * fullName is the name that will be returned by a later call to
 * csoundGetFileName().
 */

void *csoundCreateFileHandle(CSOUND *csound,
                             void *fd, int32_t type, const char *fullName)
{
    CSFILE  *p = NULL;
    int32_t     nbytes = (int32_t) sizeof(CSFILE);

    /* name should not be empty */
    if (fullName == NULL || fullName[0] == '\0')
      return NULL;
    nbytes += (int32_t) strlen(fullName);
    /* allocate file structure */
    p = (CSFILE*) csound->Calloc(csound, (size_t) nbytes);
    if (p == NULL)
      return NULL;
    p->nxt = (CSFILE*) NULL;
    p->prv = (CSFILE*) NULL;
    p->type = type;
    p->fd = -1;
    p->f = (FILE*) NULL;
    p->sf = (SNDFILE*) NULL;
    p->cb = NULL;
    strcpy(&(p->fullName[0]), fullName);
    /* open file */
    switch (type) {
    case CSFILE_FD_R:
    case CSFILE_FD_W:
      p->fd = *((int*) fd);
      break;
    case CSFILE_STD:
      p->f = *((FILE**) fd);
      break;
    case CSFILE_SND_R:
    case CSFILE_SND_W:
      p->sf = *((SNDFILE**) fd);
      break;
    default:
      csoundErrorMsg(csound, Str("internal error: csoundCreateFileHandle(): "
                                 "invalid type: %d"), type);
      csound->Free(csound, p);
      return NULL;
    }
    /* link into chain of open files */
    link_open_file(csound, p, 0);
    /* return with opaque file handle */
    p->cb = NULL;
    return (void*) p;
}

/**
 * Get the full name of a file previously opened with csoundFileOpen().
 */

char *csoundGetFileName(void *fd)
{
    return &(((CSFILE*) fd)->fullName[0]);
}

/* Close and release an unlinked file that has no outstanding worker borrow. */
static int32_t close_file_now(CSOUND *csound, CSFILE *p)
{
    int32_t     retval = -1;

    switch (p->type) {
    case CSFILE_FD_R:
    case CSFILE_FD_W:
      retval = close(p->fd);
      break;
    case CSFILE_STD:
      retval = fclose(p->f);
      break;
    case CSFILE_SND_R:
    case CSFILE_SND_W:
      if (p->sf != NULL)
        retval = csound->SndfileClose(csound, p->sf);
      p->sf = NULL;
      if (p->fd >= 0)
        retval |= close(p->fd);
      break;
    }
    if (p->buf != NULL)
      csound->Free(csound, p->buf);
    p->bufsize = 0;
    if (p->cb != NULL)
      csound->DestroyCircularBuffer(csound, p->cb);
    csound->Free(csound, p);
    return retval;
}

static int32_t reclaim_retired_files(CSOUND *csound, int32_t workerShutdown)
{
    CSFILE *current, *previous, *reclaim = NULL;
    int32_t keepRunning;

    /* Only the worker may publish STOPPED. Setup and cancellation paths can
       reclaim nodes while the current worker is still alive. */
    csoundSpinLock(&csound->open_files_lock);
    previous = NULL;
    current = (CSFILE *) csound->retired_files;
    while (current != NULL) {
      CSFILE *next = current->retired_nxt;

      if (current->io_readers == 0) {
        if (previous == NULL)
          csound->retired_files = next;
        else
          previous->retired_nxt = next;
        current->retired_nxt = reclaim;
        reclaim = current;
      }
      else
        previous = current;
      current = next;
    }
    keepRunning = ATOMIC_GET(csound->file_io_start) == FILE_IO_STARTING;
    for (current = (CSFILE *) csound->open_files;
         current != NULL && !keepRunning; current = current->nxt) {
      keepRunning = current->async_flag != FILE_ASYNC_NONE;
    }
    if (csound->retired_files != NULL)
      keepRunning = 1;
    if (!keepRunning && workerShutdown)
      ATOMIC_SET(csound->file_io_start, FILE_IO_STOPPED);
    csoundSpinUnLock(&csound->open_files_lock);

    while (reclaim != NULL) {
      CSFILE *next = reclaim->retired_nxt;

      reclaim->nxt = reclaim->prv = reclaim->retired_nxt = NULL;
      close_file_now(csound, reclaim);
      reclaim = next;
    }
    return keepRunning;
}

static int32_t claim_retired_file_for_close(CSOUND *csound, CSFILE *file)
{
    CSFILE *current, *previous;
    int32_t claimed = 0;

    csoundSpinLock(&csound->open_files_lock);
    if (file->io_readers == 1) {
      previous = NULL;
      current = (CSFILE *) csound->retired_files;
      while (current != NULL && current != file) {
        previous = current;
        current = current->retired_nxt;
      }
      if (current != NULL) {
        if (previous == NULL)
          csound->retired_files = current->retired_nxt;
        else
          previous->retired_nxt = current->retired_nxt;
        file->nxt = file->prv = file->retired_nxt = NULL;
        file->io_readers = 0;
        file->retired = 0;
        claimed = 1;
      }
    }
    csoundSpinUnLock(&csound->open_files_lock);
    return claimed;
}

/**
 * Close a file previously opened with csoundFileOpen(). Synchronous mode
 * waits for current file-worker borrowers and returns the underlying close
 * result. Deferred mode transfers ownership without waiting; it returns
 * CSOUND_SUCCESS once accepted and cannot report a later close error.
 */
int32_t csoundFileClose(CSOUND *csound, void *fd, uint32_t closeFlags)
{
    CSFILE *p = (CSFILE *) fd;

    if (UNLIKELY(closeFlags & ~CSFILE_CLOSE_DEFER)) {
      csoundErrorMsg(csound, Str("csoundFileClose: invalid close flags: %u"),
                     closeFlags);
      return CSOUND_ERROR;
    }

    if (closeFlags & CSFILE_CLOSE_DEFER) {
      if (!unlink_open_file(csound, p, 0))
        (void) close_file_now(csound, p);
      return CSOUND_SUCCESS;
    }

    if (unlink_open_file(csound, p, 1)) {
      while (!claim_retired_file_for_close(csound, p))
        csoundSleep(1);
    }

    return close_file_now(csound, p);
}

/* Close all open files; called by csoundReset(). */

void close_all_files(CSOUND *csound)
{
    CSFILE *file;

    /* A creator publishes the thread handle after CreateThread returns. Do
       not tear down its registry while that publication is in flight. */
    while (ATOMIC_GET(csound->file_io_start) == FILE_IO_STARTING)
      csoundSleep(1);
    for (;;) {
      csoundSpinLock(&csound->open_files_lock);
      file = (CSFILE *) csound->open_files;
      csoundSpinUnLock(&csound->open_files_lock);
      if (file == NULL)
        break;
      csoundFileClose(csound, file, CSFILE_CLOSE_SYNC);
    }
    if (csound->file_io_thread != NULL) {
#ifndef __EMSCRIPTEN__
      csound->JoinThread(csound->file_io_thread);
#endif
      if (csound->file_io_threadlock != NULL)
        csound->DestroyThreadLock(csound->file_io_threadlock);
    }
    reclaim_retired_files(csound, 0);
    csound->file_io_thread = NULL;
    csound->file_io_threadlock = NULL;
    ATOMIC_SET(csound->file_io_start, FILE_IO_STOPPED);
}

/* The fromScore parameter should be 1 if opening a score include file,
   0 if opening an orchestra include file */
void *fopen_path(CSOUND *csound, FILE **fp, const char *name, const char *basename,
                 char *env, int32_t fromScore)
{
    void *fd;
    int32_t  csftype = (fromScore ? CSFTYPE_SCO_INCLUDE : CSFTYPE_ORC_INCLUDE);

    /* First try to open name given */
    fd = csound->FileOpen(csound, fp, CSFILE_STD, name, "r", NULL,
                           csftype, 0);
    if (fd != NULL)
      return fd;
    /* if that fails try in base directory */
    if (basename != NULL) {
      char *dir, *name_full;
      if ((dir = split_directory_from_path(csound, basename)) != NULL) {
        name_full = csound_concatenate_paths(csound, dir, name);
        fd = csound->FileOpen(csound, fp, CSFILE_STD, name_full, "r", NULL,
                               csftype, 0);
        csound->Free(csound, dir);
        csound->Free(csound, name_full);
        if (fd != NULL)
          return fd;
      }
    }
    /* or use env argument */
    fd = csound->FileOpen(csound, fp, CSFILE_STD, name, "r", env,
                           csftype, 0);
    return fd;
}

static uintptr_t file_iothread(void *p);

static int32_t start_file_io_thread(CSOUND *csound)
{
    int32_t lifecycle;
    int32_t creator = 0;

    for (;;) {
      csoundSpinLock(&csound->open_files_lock);
      lifecycle = ATOMIC_GET(csound->file_io_start);
      if (lifecycle == FILE_IO_STOPPED) {
        ATOMIC_SET(csound->file_io_start, FILE_IO_STARTING);
        creator = 1;
      }
      csoundSpinUnLock(&csound->open_files_lock);
      if (lifecycle == FILE_IO_RUNNING)
        return OK;
      if (creator)
        break;
      csoundSleep(1);
    }

    if (csound->file_io_thread != NULL) {
      csound->JoinThread(csound->file_io_thread);
      csound->file_io_thread = NULL;
      if (csound->file_io_threadlock != NULL) {
        csound->DestroyThreadLock(csound->file_io_threadlock);
        csound->file_io_threadlock = NULL;
      }
    }
    /* STARTING serializes creators while thread creation remains outside the
       registry spinlock. read_files keeps the new worker alive until this
       function publishes RUNNING. */
    csound->file_io_threadlock = csound->CreateThreadLock();
    if (LIKELY(csound->file_io_threadlock != NULL)) {
      csound->NotifyThreadLock(csound->file_io_threadlock);
      csound->file_io_thread =
        csound->CreateThread(file_iothread, (void *) csound);
    }

    csoundSpinLock(&csound->open_files_lock);
    lifecycle = csound->file_io_thread != NULL ? FILE_IO_RUNNING :
                                                  FILE_IO_STOPPED;
    ATOMIC_SET(csound->file_io_start, lifecycle);
    csoundSpinUnLock(&csound->open_files_lock);
    if (lifecycle == FILE_IO_RUNNING)
      return OK;

    if (csound->file_io_threadlock != NULL) {
      csound->DestroyThreadLock(csound->file_io_threadlock);
      csound->file_io_threadlock = NULL;
    }
    reclaim_retired_files(csound, 0);
    return NOTOK;
}

static int32_t finish_async_file_setup(CSOUND *csound, CSFILE *file)
{
    int32_t retired;

    csoundSpinLock(&csound->open_files_lock);
    file->io_readers--;
    retired = file->retired;
    csoundSpinUnLock(&csound->open_files_lock);
    if (retired)
      reclaim_retired_files(csound, 0);
    return retired;
}

void *csoundFileOpenAsync(CSOUND *csound, void *fd, int32_t type,
                           const char *name, void *param, const char *env,
                           int32_t csFileType, int32_t buffsize, int32_t isTemporary)
{
#ifndef __EMSCRIPTEN__
    CSFILE *p;
    int32_t cancelled;
    if ((p = (CSFILE *) csoundFileOpenInternal(
           csound, fd, type, name, param, env, csFileType, isTemporary, 1)) ==
        NULL)
      return NULL;

    /* Keep the worker alive while this file's circular buffers are prepared,
       but do not let it process the file before setup is complete. */
    csoundSpinLock(&csound->open_files_lock);
    cancelled = p->retired;
    if (!cancelled)
      p->async_flag = FILE_ASYNC_STARTING;
    csoundSpinUnLock(&csound->open_files_lock);
    if (cancelled) {
      finish_async_file_setup(csound, p);
      return NULL;
    }

    if (UNLIKELY(start_file_io_thread(csound) != OK)) {
      csoundSpinLock(&csound->open_files_lock);
      p->async_flag = FILE_ASYNC_NONE;
      csoundSpinUnLock(&csound->open_files_lock);
      csoundFileClose(csound, p, CSFILE_CLOSE_DEFER);
      finish_async_file_setup(csound, p);
      return NULL;
    }
    csound->WaitThreadLockNoTimeout(csound->file_io_threadlock);

    p->cb = csound->CreateCircularBuffer(csound, buffsize*4, sizeof(MYFLT));
    p->items = 0;
    p->pos = 0;
    p->bufsize = buffsize;
    p->buf = (MYFLT *) csound->Calloc(csound, sizeof(MYFLT)*buffsize);
    csoundSpinLock(&csound->open_files_lock);
    cancelled = p->retired || p->cb == NULL || p->buf == NULL;
    if (!cancelled)
      p->async_flag = FILE_ASYNC_RUNNING;
    else
      p->async_flag = FILE_ASYNC_NONE;
    csoundSpinUnLock(&csound->open_files_lock);
    csound->NotifyThreadLock(csound->file_io_threadlock);

    if (cancelled) {
      /* Defer close without waiting for the setup reader below. */
      csoundFileClose(csound, (void *) p, CSFILE_CLOSE_DEFER);
      finish_async_file_setup(csound, p);
      return NULL;
    }
    if (finish_async_file_setup(csound, p))
      return NULL;
    return (void *) p;
#else
    return NULL;
#endif
}

uint32_t csoundReadAsync(CSOUND *csound, void *handle,
                             MYFLT *buf, int32_t items)
{
    CSFILE *p = handle;
    if (p != NULL &&  p->cb != NULL)
      return csound->ReadCircularBuffer(csound, p->cb, buf, items);
    else return 0;
}

uint32_t csoundWriteAsync(CSOUND *csound, void *handle,
                              MYFLT *buf, int32_t items)
{
    CSFILE *p = handle;
    if (p != NULL &&  p->cb != NULL)
      return csound->WriteCircularBuffer(csound, p->cb, buf, items);
    else return 0;
}

int32_t csoundFSeekAsync(CSOUND *csound, void *handle, int32_t pos, int32_t whence){
    CSFILE *p = handle;
    int32_t ret = 0;
    csound->WaitThreadLockNoTimeout(csound->file_io_threadlock);
    switch (p->type) {
    case CSFILE_FD_R:
      break;
    case CSFILE_FD_W:
      break;
    case CSFILE_STD:
      break;
    case CSFILE_SND_R:
    case CSFILE_SND_W:
      ret = (int32_t) csound->SndfileSeek(csound,p->sf,pos,whence);
      //csoundMessage(csound, "seek set %d\n", pos);
      csound->FlushCircularBuffer(csound, p->cb);
      p->items = 0;
      break;
    }
    csound->NotifyThreadLock(csound->file_io_threadlock);
    return ret;
}


static int32_t read_files(CSOUND *csound){
    CSFILE *current;
    uint64_t pass;

    csoundSpinLock(&csound->open_files_lock);
    pass = ++csound->file_io_pass;
    csoundSpinUnLock(&csound->open_files_lock);
    /* Restart at the head after each unpinned read. A pass stamp prevents
       duplicate work while avoiding a cached next pointer that a concurrent
       close could retire and reclaim. */
    for (;;) {
      csoundSpinLock(&csound->open_files_lock);
      current = (CSFILE *) csound->open_files;
      while (current != NULL &&
             (current->async_flag != FILE_ASYNC_RUNNING ||
              current->io_pass == pass))
        current = current->nxt;
      if (current != NULL) {
        current->io_pass = pass;
        current->io_readers++;
      }
      csoundSpinUnLock(&csound->open_files_lock);
      if (current == NULL)
        break;
      {
        int32_t m = current->pos, l, n = current->items;
        int32_t items = current->bufsize;
        MYFLT *buf = current->buf;
        switch (current->type) {
        case CSFILE_FD_R:
          break;
        case CSFILE_FD_W:
          break;
        case CSFILE_STD:
          break;
        case CSFILE_SND_R:
          if (n == 0) {
            n = (int32_t) csound->SndfileReadSamples(csound, current->sf,
                                                     buf, items);
            m = 0;
          }
          l = csound->WriteCircularBuffer(csound,current->cb,&buf[m],n);
          m += l;
          n -= l;
          current->items = n;
          current->pos = m;
          break;
        case CSFILE_SND_W:
          items = csound->ReadCircularBuffer(csound, current->cb, buf, items);
          if (items == 0) { csoundSleep(10); break;}
          csound->SndfileWriteSamples(csound, current->sf, buf, items);
          break;
        }
      }
      csoundSpinLock(&csound->open_files_lock);
      current->io_readers--;
      csoundSpinUnLock(&csound->open_files_lock);
    }
    return reclaim_retired_files(csound, 1);
}


static uintptr_t file_iothread(void *p){
    int32_t res = 1;
    CSOUND *csound = p;
    int32_t wakeup = (int32_t) (1000*csound->ksmps/csound->esr);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    if (wakeup == 0) wakeup = 1;
    while (res){
      csoundSleep(wakeup);
      csound->WaitThreadLockNoTimeout(csound->file_io_threadlock);
      res = read_files(csound);
      csound->NotifyThreadLock(csound->file_io_threadlock);
    }
    /* read_files publishes STOPPED under open_files_lock. A new creator may
       already have advanced the lifecycle to STARTING by the time this old
       worker returns, so it must not overwrite that newer generation. */
    return (uintptr_t)NULL;
}
