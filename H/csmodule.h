/*
    csmodule.h:

    Copyright (C) 2005 Istvan Varga
    based on dl_opcodes.c, Copyright (C) 2002 John ffitch

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

#ifndef CSOUND_CSMODULE_H
#define CSOUND_CSMODULE_H

/******************************************************************************
 * NEW PLUGIN INTERFACE                                                       *
 * ====================                                                       *
 *                                                                            *
 * Plugin libraries are loaded from the directory defined by the environment  *
 * variable OPCODEDIR (or the current directory if OPCODEDIR is unset) by     *
 * csoundCreate() after the creation of a Csound instance, and are unloaded   *
 * at the end of performance.                                                 *
 * A library may export any of the following four interface functions,        *
 * however, the presence of csoundModuleCreate() is required for identifying  *
 * the file as a Csound plugin module.                                        *
 *                                                                            *
 * int csoundModuleCreate(void *csound)         (required)                    *
 * ------------------------------------                                       *
 *                                                                            *
 * Pre-initialisation function that is called by csoundCreate() after the     *
 * creation of Csound instance 'csound'.                                      *
 *                                                                            *
 * int csoundModuleInit(void *csound)           (optional)                    *
 * ----------------------------------                                         *
 *                                                                            *
 * Called by Csound instances before orchestra translation. One possible use  *
 * of csoundModuleInit() is adding new opcodes with csoundAppendOpcode().     *
 *                                                                            *
 * int csoundModuleDestroy(void *csound)        (optional)                    *
 * -------------------------------------                                      *
 *                                                                            *
 * Destructor function for Csound instance 'csound', called at the end of     *
 * performance, after closing audio output.                                   *
 *                                                                            *
 * char *csoundModuleErrorCodeToString(int errcode)     (optional)            *
 * ------------------------------------------------                           *
 *                                                                            *
 * Converts error codes returned by any of the initialisation or destructor   *
 * functions to a string message.                                             *
 *                                                                            *
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------ INTERNAL API FUNCTIONS ------------------------ */

/**
 * Load plugin libraries for Csound instance 'csound', and call
 * pre-initialisation functions.
 * Return value is CSOUND_SUCCESS if there was no error, CSOUND_ERROR if
 * some modules could not be loaded or initialised, and CSOUND_MEMORY
 * if a memory allocation failure has occured.
 */
int csoundLoadModules(void *csound);

/**
 * Call initialisation functions of all loaded modules that have a
 * csoundModuleInit symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and CSOUND_ERROR if
 * some modules could not be initialised.
 */
int csoundInitModules(void *csound);

/**
 * Call destructor functions of all loaded modules that have a
 * csoundModuleDestroy symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and
 * CSOUND_ERROR if some modules could not be de-initialised.
 */
int csoundDestroyModules(void *csound);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif  /* CSOUND_CSMODULE_H */

