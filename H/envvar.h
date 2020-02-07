/*
    envvar.h:

    Copyright (C) 2005 Istvan Varga

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

#ifndef CSOUND_ENVVAR_H
#define CSOUND_ENVVAR_H

#if !defined(__BUILDING_LIBCSOUND)
#  error "Csound plugins and host applications should not include envvar.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Set environment variable 'name' to 'value'.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
   * if the environment variable could not be set for some reason.
   */
  int csoundSetEnv(CSOUND *csound, const char *name, const char *value);

  /**
   * Append 'value' to environment variable 'name', using ';' as
   * separator character.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
   * if the environment variable could not be set for some reason.
   */
  int csoundAppendEnv(CSOUND *csound, const char *name, const char *value);

  /**
   * Prepend 'value' to environment variable 'name', using ';' as
   * separator character.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
   * if the environment variable could not be set for some reason.
   */
  int csoundPrependEnv(CSOUND *csound, const char *name, const char *value);

  /**
   * Initialise environment variable database, and copy system
   * environment variables.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY in case of an error.
   */
  int csoundInitEnv(CSOUND *csound);

  /**
   * Parse 's' as an assignment to environment variable, in the format
   * "NAME=VALUE" for replacing the previous value, or "NAME+=VALUE"
   * for appending.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY in case of an error.
   */
  int csoundParseEnv(CSOUND *csound, const char *s);

  /** Check if file name is valid, and copy with converting pathname delimiters */
  char *csoundConvertPathname(CSOUND *csound, const char *filename);

  /**  Check if name is a full pathname for the platform we are running on. */
  int csoundIsNameFullpath(const char *name);

  /** Check if name is a relative pathname for this platform.  Bare
   *  filenames with no path information are not counted.
   */
  int csoundIsNameRelativePath(const char *name);

  /** Check if name is a "leaf" (bare) filename for this platform. */
  int csoundIsNameJustFilename(const char *name);

  /** Properly concatenates the full or relative pathname in path1 with
   *  the relative pathname or filename in path2 according to the rules
   *  for the platform we are running on.  path1 is assumed to be
   *  a directory whether it ends with DIRSEP or not.  Relative paths must
   *  conform to the conventions for the current platform (begin with ':'
   *  on MacOS 9 and not begin with DIRSEP on others).
   */
  char* csoundConcatenatePaths(CSOUND* csound, const char *path1,
                                               const char *path2);

  /** Converts a pathname to native format and returns just the part of
   *  the path that specifies the directory.  Does not return the final
   *  DIRSEP.  Returns an empty string if no path components occur before
   *  the filename.  Returns NULL if unable to carry out the operation
   *  for some reason.
   */
  char *csoundSplitDirectoryFromPath(CSOUND* csound, const char * path);

  /** Return just the final component of a full path */
  char *csoundSplitFilenameFromPath(CSOUND* csound, const char * path);

  /**
   * Search for input file 'filename'.
   * If the file name specifies full path (it begins with '.', the pathname
   * delimiter character, or a drive letter and ':' on Windows), that exact
   * file name is tried without searching.
   * Otherwise, the file is searched relative to the current directory first,
   * and if it is still not found, a pathname list that is created the
   * following way is searched:
   *   1. if envList is NULL or empty, no directories are searched
   *   2. envList is parsed as a ';' separated list of environment variable
   *      names, and all environment variables are expanded and expected to
   *      contain a ';' separated list of directory names
   *   2. all directories in the resulting pathname list are searched, starting
   *      from the last and towards the first one, and the directory where the
   *      file is found first will be used
   * The function returns a pointer to the full name of the file if it is
   * found, and NULL if the file could not be found in any of the search paths,
   * or an error has occured. The caller is responsible for freeing the memory
   * pointed to by the return value, by calling mfree().
   */
  char *csoundFindInputFile(CSOUND *csound,
                            const char *filename, const char *envList);

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
   * value, by calling mfree().
   */
  char *csoundFindOutputFile(CSOUND *csound,
                             const char *filename, const char *envList);

  /**
   * Open a file and return handle.
   *
   * CSOUND *csound:
   *   Csound instance pointer
   * void *fd:
   *   pointer a variable of type int, FILE*, or SNDFILE*, depending on 'type',
   *   for storing handle to be passed to file read/write functions
   * int type:
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
   *     CSFILE_SND_R:    SF_INFO* parameter for sf_open(), with defaults for
   *                      raw file; the actual format paramaters of the opened
   *                      file will be stored in this structure
   *     CSFILE_SND_W:    SF_INFO* parameter for sf_open(), output file format
   * const char *env:
   *   list of environment variables for search path (see csoundFindInputFile()
   *   for details); if NULL, the specified name is used as it is, without any
   *   conversion or search.
   * int csFileType:
   *   A value from the enumeration CSOUND_FILETYPES (see CsoundCore.h)
   * int isTemporary:
   *   1 if this file will be deleted when Csound is finished.
   *   Otherwise, 0.
   * return value:
   *   opaque handle to the opened file, for use with csoundGetFileName() or
   *   csoundFileClose(), or storing in FDCH.fd.
   *   On failure, NULL is returned.
   */
  void *csoundFileOpenWithType(CSOUND *csound, void *fd, int type,
                               const char *name, void *param, const char *env,
                               int csFileType, int isTemporary);

  /**
   * Allocate a file handle for an existing file already opened with open(),
   * fopen(), or sf_open(), for later use with csoundFileClose() or
   * csoundGetFileName(), or storing in an FDCH structure.
   * Files registered this way are also
   * automatically closed by csoundReset().
   * Parameters and return value are similar to csoundFileOpenithType(), except
   * fullName is the name that will be returned by a later call to
   * csoundGetFileName().
   */
  void *csoundCreateFileHandle(CSOUND *,
                               void *fd, int type, const char *fullName);

  /**
   * Get the full name of a file previously opened with csoundFileOpen().
   */
  char *csoundGetFileName(void *fd);

  /**
   * Close a file previously opened with csoundFileOpen().
   */
  int csoundFileClose(CSOUND *, void *fd);

  /** Given a file name as string, return full path of directory of file;
   * Note: does not check if file exists
   */
  char *csoundGetDirectoryForPath(CSOUND* csound, const char * path);


  void *csoundFileOpenWithType_Async(CSOUND *csound, void *fd, int type,
                                     const char *name, void *param,
                                     const char *env,
                                     int csFileType, int buffsize,
                                     int isTemporary);

  unsigned int csoundReadAsync(CSOUND *csound, void *handle,
                               MYFLT *buf, int items);

  unsigned int csoundWriteAsync(CSOUND *csound, void *handle,
                                MYFLT *buf, int items);

  int csoundFSeekAsync(CSOUND *csound, void *handle, int pos, int whence);


#ifdef __cplusplus
}
#endif

#endif  /* CSOUND_ENVVAR_H */
