/*
  csound_misc.h: miscellaneous utility functions

  Copyright (C) 2024

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

#ifndef CS_MISC_H
#define CS_MISC_H

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct RTCLOCK_S {
    int_least64_t   starttime_real;
    int_least64_t   starttime_CPU;
  } RTCLOCK;

  typedef struct CsoundRandMTState_ {
    int32_t         mti;
    uint32_t    mt[624];
  } CsoundRandMTState;

  typedef struct {
    char        *opname;
    char        *outypes;
    char        *intypes;
    int32_t     flags;
  } opcodeListEntry;

  
  /** @defgroup MISCELLANEOUS Miscellaneous functions
   *
   *  @{ */

  /**
   * Gets an alphabetically sorted list of all opcodes.
   * Should be called after externals are loaded by csoundCompile().
   * Returns the number of opcodes, or a negative error code on failure.
   * Make sure to call csoundDisposeOpcodeList() when done with the list.
   */
  PUBLIC int32_t csoundNewOpcodeList(CSOUND *csound, opcodeListEntry **lstp);

  /**
   * Frees an opcodeListEntry lst created by csoundNewOpcodeList()
   */
   PUBLIC void csoundDisposeOpcodeList(CSOUND *csound, opcodeListEntry *lst);  
  
  /**
   * Runs an external command with the arguments specified in 'argv'.
   * argv[0] is the name of the program to execute (if not a full path
   * file name, it is searched in the directories defined by the PATH
   * environment variable). The list of arguments should be terminated
   * by a NULL pointer.
   * If 'noWait' is zero, the function waits until the external program
   * finishes, otherwise it returns immediately. In the first case, a
   * non-negative return value is the exit status of the command (0 to
   * 255), otherwise it is the PID of the newly created process.
   * On error, a negative value is returned.
   */
  PUBLIC long csoundRunCommand(const char * const *argv, int32_t noWait);

  /**
   * Initialise a timer structure.
   */
  PUBLIC void csoundInitTimerStruct(RTCLOCK *);

  /**
   * Return the elapsed real time (in seconds) since the specified timer
   * structure was initialised.
   */
  PUBLIC double csoundGetRealTime(RTCLOCK *);

  /**
   * Return the elapsed CPU time (in seconds) since the specified timer
   * structure was initialised.
   */
  PUBLIC double csoundGetCPUTime(RTCLOCK *);

  /**
   * Return a 32-bit unsigned integer to be used as seed from current time.
   */
  PUBLIC uint32_t csoundGetRandomSeedFromTime(void);

  /**
   * Set language to 'lang_code' (lang_code can be for example
   * CSLANGUAGE_ENGLISH_UK or CSLANGUAGE_FRENCH or many others,
   * see n_getstr.h for the list of languages). This affects all
   * Csound instances running in the address space of the current
   * process. The special language code CSLANGUAGE_DEFAULT can be
   * used to disable translation of messages and free all memory
   * allocated by a previous call to csoundSetLanguage().
   * csoundSetLanguage() loads all files for the selected language
   * from the directory specified by the CSSTRNGS environment
   * variable.
   */
  PUBLIC void csoundSetLanguage(cslanguage_t lang_code);

  /**
   * Get pointer to the value of environment variable 'name', searching
   * in this order: local environment of 'csound' (if not NULL), variables
   * set with csoundSetGlobalEnv(), and system environment variables.
   * If 'csound' is not NULL, should be called after csoundCompile().
   * Return value is NULL if the variable is not set.
   */
  PUBLIC const char *csoundGetEnv(CSOUND *csound, const char *name);

  /**
   * Set the global value of environment variable 'name' to 'value',
   * or delete variable if 'value' is NULL.
   * It is not safe to call this function while any Csound instances
   * are active.
   * Returns zero on success.
   */
  PUBLIC int32_t csoundSetGlobalEnv(const char *name, const char *value);

  /**
   * Allocate nbytes bytes of memory that can be accessed later by calling
   * csoundQueryGlobalVariable() with the specified name; the space is
   * cleared to zero.
   * Returns CSOUND_SUCCESS on success, CSOUND_ERROR in case of invalid
   * parameters (zero nbytes, invalid or already used name), or
   * CSOUND_MEMORY if there is not enough memory.
   */
  PUBLIC int32_t csoundCreateGlobalVariable(CSOUND *,
                                        const char *name, size_t nbytes);

  /**
   * Get pointer to space allocated with the name "name".
   * Returns NULL if the specified name is not defined.
   */
  PUBLIC void *csoundQueryGlobalVariable(CSOUND *, const char *name);

  /**
   * This function is the same as csoundQueryGlobalVariable(), except the
   * variable is assumed to exist and no error checking is done.
   * Faster, but may crash or return an invalid pointer if 'name' is
   * not defined.
   */
  PUBLIC void *csoundQueryGlobalVariableNoCheck(CSOUND *, const char *name);

  /**
   * Free memory allocated for "name" and remove "name" from the database.
   * Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the name is
   * not defined.
   */
  PUBLIC int32_t csoundDestroyGlobalVariable(CSOUND *, const char *name);

  /**
   * Run utility with the specified name and command line arguments.
   * Should be called after loading utility plugins.
   * Use csoundReset() to clean up after calling this function.
   * Returns zero if the utility was run successfully.
   */
  PUBLIC int32_t csoundRunUtility(CSOUND *, const char *name,
                              int32_t argc, char **argv);

  /**
   * Returns a NULL terminated list of registered utility names.
   * The caller is responsible for freeing the returned array with
   * csoundDeleteUtilityList(), however, the names should not be
   * changed or freed.
   * The return value may be NULL in case of an error.
   */
  PUBLIC char **csoundListUtilities(CSOUND *);

  /**
   * Releases an utility list previously returned by csoundListUtilities().
   */
  PUBLIC void csoundDeleteUtilityList(CSOUND *, char **lst);

  /**
   * Get utility description.
   * Returns NULL if the utility was not found, or it has no description,
   * or an error occured.
   */
  PUBLIC const char *csoundGetUtilityDescription(CSOUND *,
                                                 const char *utilName);

  /**
   * Simple linear congruential random number generator:
   *   (*seedVal) = (*seedVal) * 742938285 % 2147483647
   * the initial value of *seedVal must be in the range 1 to 2147483646.
   * Returns the next number from the pseudo-random sequence,
   * in the range 1 to 2147483646.
   */
  PUBLIC int32_t csoundRand31(int32_t *seedVal);

  /**
   * Initialise Mersenne Twister (MT19937) random number generator,
   * using 'keyLength' unsigned 32 bit values from 'initKey' as seed.
   * If the array is NULL, the length parameter is used for seeding.
   */
  PUBLIC void csoundSeedRandMT(CsoundRandMTState *p,
                               const uint32_t *initKey, uint32_t keyLength);

  /**
   * Returns next random number from MT19937 generator.
   * The PRNG must be initialised first by calling csoundSeedRandMT().
   */
  PUBLIC uint32_t csoundRandMT(CsoundRandMTState *p);
  /** @}*/
#ifdef __cplusplus
}
#endif
 
#endif
