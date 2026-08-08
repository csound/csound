/*
    csdebug.h:

    Copyright (C) 2014 Andres Cabrera

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

#ifndef CSDEBUG_H
#define CSDEBUG_H

/**
* \file csdebug.h
*
* This header provides the debugger API which is part of libcsound.
*
* Basic usage of the debugger is this:
*
* \code
    CSOUND* csound = csoundCreate(NULL);
    csoundCompileOrc(csound, " ");
    csoundStart(csound);
    csoundDebuggerInit(csound);
    csoundSetBreakpointCallback(csound, brkpt_cb, &userdata);
    csoundSetInstrumentBreakpoint(csound, 1, 5);

    // Run Csound Performance here
    // The breakpoint callback will be called when instrument 1 has been
    // instantiated and 5 control blocks have been processed.

    csoundDebuggerClean(csound);
    csoundDestroy(csound);
* \endcode
*
*/

#ifdef __BUILDING_LIBCSOUND
#include "csoundCore.h"
#else
#include "csound.h"
#endif

#include "csound_type_system.h"


/** @defgroup DEBUGGER Debugger
 *
 *  @{ */

typedef struct debug_instr_s {
    CS_VARIABLE *varPoolHead;
    MYFLT *lclbas;
    void *instrptr;
    MYFLT p1, p2, p3;
    uint64_t kcounter;
    int32_t line;
    struct debug_instr_s *next;
} debug_instr_t;

typedef struct debug_opcode_s {
    char opname[16];
    int32_t line;
    // TODO: Fill opcode linked list
    struct debug_opcode_s *next;
    struct debug_opcode_s *prev;
} debug_opcode_t;

typedef struct debug_variable_s {
    const char *name;
    const char *typeName;
    void *data;
    struct debug_variable_s *next;
} debug_variable_t;

/** Active UDO invocation frame (sub-instrument instance behind a UOPCODE call). */
typedef struct debug_udo_frame_s {
    const char *udoName;
    int32_t callLine;
    int32_t depth;
    int32_t frameIndex;
    debug_variable_t *varList;
    struct debug_udo_frame_s *next;
} debug_udo_frame_t;

/** f-signal (PVSDAT) metadata, filled by csoundDebugSerializeFsig(). */
typedef struct debug_fsig_info_s {
    int32_t N;             /* FFT size */
    int32_t NB;            /* number of bins = N/2 + 1 */
    int32_t overlap;       /* hop size */
    int32_t winsize;       /* analysis window size */
    int32_t wintype;       /* window type */
    int32_t format;        /* PVS analysis format (0 = PVS_AMP_FREQ) */
    uint32_t framecount;   /* increments when a new analysis frame is ready */
    int32_t sliding;       /* 1 = source frame is MYFLT (sliding), 0 = float32 */
} debug_fsig_info_t;

/** Numeric array (ARRAYDAT) metadata, filled by csoundDebugSerializeArray(). */
typedef struct debug_array_info_s {
    int32_t dimensions;        /* number of array dimensions */
    int32_t arrayMemberSize;   /* bytes per element */
    int32_t totalElements;     /* total MYFLT values in the flat data */
    char elementTypeName[16];  /* element type name, e.g. "k", "a", "i" */
} debug_array_info_t;

typedef struct {
    debug_instr_t *breakpointInstr;
    debug_variable_t *instrVarList;
    debug_instr_t *instrListHead;
    debug_opcode_t *currentOpcode;
} debug_bkpt_info_t;


/** \cond DOXYGEN_HIDDEN
 * These types should not appear in the Doxygen docs */


typedef enum {
    CSDEBUG_BKPT_LINE,
    CSDEBUG_BKPT_INSTR,
    CSDEBUG_BKPT_DELETE,
    CSDEBUG_BKPT_CLEAR_ALL
} bkpt_mode_t;

typedef enum {
    CSDEBUG_STATUS_RUNNING,
    CSDEBUG_STATUS_STOPPED,
    CSDEBUG_STATUS_NEXT
} debug_status_t;

typedef struct bkpt_node_s {
    int32_t line; /* if line is < 0 breakpoint is for instrument instances */
    MYFLT instr; /* instrument number (including fractional part */
    int32_t skip; /* number of times to skip when arriving at the breakpoint */
    int32_t count; /* current backwards count for skip, when 0 break */
    bkpt_mode_t mode;
    struct bkpt_node_s *next;
} bkpt_node_t;

typedef enum {
    CSDEBUG_CMD_NONE,
    CSDEBUG_CMD_STEPOVER,
    CSDEBUG_CMD_STEPINTO,
    CSDEBUG_CMD_NEXT,
    CSDEBUG_CMD_CONTINUE,
    CSDEBUG_CMD_STOP
} debug_command_t;

typedef enum {
    CSDEBUG_OFF = 0x0,
    CSDEBUG_K = 0x01,
    CSDEBUG_INIT = 0x02
} debug_mode_t;

#ifdef __BUILD_LIBCSOUND

void csoundDebuggerBreakpointReached(CSOUND *csound);

#endif

/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif


/** Breakpoint callback function type
 *
 * When a breakpoint is reached, the debugger will call a function of this type
 * see csoundSetBreakpointCallback() */
typedef void (*breakpoint_cb_t) (CSOUND *, debug_bkpt_info_t *, void *userdata);

/** Debug k-cycle callback function type
 *
 * Called after every k-cycle when the debugger is active (kperf_debug),
 * after all instruments have run, before audio output is sent.
 * Use csoundDebugGetInstrInstances() and csoundDebugGetVariables() inside
 * the callback to inspect active instrument variables in real time.
 * The callback must return quickly — it fires from within the performance
 * loop.
 *
 * Unlike the breakpoint callback, this callback is non-stopping: Csound
 * continues performance normally after the callback returns.
 *
 * Requires csoundDebuggerInit() to have been called first.
 */
typedef void (*debug_cb_t)(CSOUND *csound, void *userdata);

typedef struct csdebug_data_s {
    void *bkpt_buffer; /* for passing breakpoints to the running engine */
    void *cmd_buffer;     /* for passing commands to the running engine */
    debug_status_t status;
    bkpt_node_t *bkpt_anchor;            /* linked list for breakpoints */
    bkpt_node_t *cur_bkpt;   /* current breakpoint where we are stopped */
    breakpoint_cb_t bkpt_cb;
    void *cb_data;
    void *debug_instr_ptr;      /* != NULL when stopped at a breakpoint.
                                   Holds INSDS * */
    void *debug_opcode_ptr; /* != NULL when stopped at a line breakpoint.
                               Holds OPDS * */
} csdebug_data_t;

/** Intialize debugger facilities
 *
 * This function allocates debugger structures, and enables its usage.
 * There is a small performance penalty when using the debugger, so
 * be sure to call csoundDebuggerClean() after use.
 *
 * This call is not thread safe and must be called before performance starts.
 *
 * @param csound A Csound instance
 *
 * Returns CSOUND_ERROR on failure, CSOUND_SUCCESS on initialisation completed.
*/
PUBLIC int32_t csoundDebuggerInit(CSOUND *csound);

/** Cleanup debugger facilities
 *
 * @param csound A Csound instance
*/
PUBLIC void csoundDebuggerClean(CSOUND *csound);

/** Set a breakpoint on a particular line
 *
 * @param csound A Csound instance @param line The line on which to
 * set a breakpoint
 * @param instr When set to 0, the line number refers
 * to the line number in the score. When a number is given, it should
 * refer to an instrument that has been compiled on the fly using
 * csoundParseOrc().
 * @param skip number of control blocks to skip
 *
*/
PUBLIC void csoundSetBreakpoint(CSOUND *csound, int32_t line, int32_t instr, int32_t skip);

/** Remove a previously set line breakpoint
 *
*/
PUBLIC void csoundRemoveBreakpoint(CSOUND *csound, int32_t line, int32_t instr);

/** Set a breakpoint for an instrument number
 *
 * Sets a breakpoint for an instrument number with optional number of skip
 * control blocks. You can specify a fractional instrument number to identify
 * particular instances. Specifying a value greater than 1 will result in that
 * number of control blocks being skipped before this breakpoint is called
 * again. A value of 0 and 1 has the same effect.
 *
 * This call is thread safe, as the breakpoint will be put in a lock free queue
 * that is processed as soon as possible in the kperf function.
 *
 * @param csound a Csound instance
 * @param instr instrument number
 * @param skip number of control blocks to skip
 */
PUBLIC void csoundSetInstrumentBreakpoint(CSOUND *csound, MYFLT instr, int32_t skip);

/** Remove instrument breakpoint
 *
 * Removes an instrument breakpoint from the breakpoint list. Csound will no
 * longer break at that instrument
 *
 * This call is thread safe, as the breakpoint will be put in a lock free queue
 * that is processed as soon as possible in the kperf function.
 */
PUBLIC void csoundRemoveInstrumentBreakpoint(CSOUND *csound, MYFLT instr);

/** Clear all breakpoints
 *
 * Removes all breakpoints. This call is thread safe, as it will be processed
 *  as soon as possible in the kperf function.
 */
PUBLIC void csoundClearBreakpoints(CSOUND *csound);

/** Sets the breakpoint callback function
 *
 * Sets the function that will be called when a breakpoint is reached.
 *
 * @param csound Csound instance pointer
 * @param bkpt_cb pointer to breakpoint callback function
 * @param userdata pointer to user data that will be passed to the callback
 * function
 */
PUBLIC void csoundSetBreakpointCallback(CSOUND *csound,
                                        breakpoint_cb_t bkpt_cb, void *userdata);

/* Not implemented yet, so not exposed in the API
PUBLIC void csoundDebugStepOver(CSOUND *csound);
PUBLIC void csoundDebugStepInto(CSOUND *csound);
*/

/** Continue execution and break at next instrument instance
 *
 * Call this function to continue execution but automatically stop at
 * next instrument instance.
 */
PUBLIC void csoundDebugNext(CSOUND *csound);

/** Continue execution from breakpoint
 *
 * Call this function to continue execution of a Csound instance which is
 * stopped because a breakpoint has been reached. This function will continue
 * traversing the instrument chain from the instrument instance that
 * triggered the break.
 */
PUBLIC void csoundDebugContinue(CSOUND *csound);

/** Stop Csound rendering and enter the debugger
 *
 * Calling this function will enter the debugger at the soonest possible point
 * as if a breakpoint had been reached.
 */
PUBLIC void csoundDebugStop(CSOUND *csound);

/** Get a list of active instrument instances
 * Returns a linked list of allocated instrument instances
 * csoundDebugFreeInstrInstances() must be called on the list once it is no
 * longer needed.
 * This function is not thread safe and should only be called while the csound
 * engine is stopped at a breakpoint.
 */
PUBLIC debug_instr_t *csoundDebugGetInstrInstances(CSOUND *csound);

/** Free list created by csoundDebugGetCurrentInstrInstance() or
 * csoundDebugGetInstrInstances()
 */
PUBLIC void csoundDebugFreeInstrInstances(CSOUND *csound, debug_instr_t *instr);

/** Get list of variables for instrument */
PUBLIC debug_variable_t *csoundDebugGetVariables(CSOUND *csound,
                                                 debug_instr_t *instr);

/** Free variable list generated by csoundDebugGetVariables() */
PUBLIC void csoundDebugFreeVariables(CSOUND *csound,
                                     debug_variable_t *varHead);

/** Get active UDO frames for one top-level instrument instance
 *
 * Walks the UOPCODE chain hung off the instrument's opcod_deact list and
 * returns one entry per active UDO sub-instance (including nested/recursive
 * frames at greater depth values). Each frame includes a variable list for
 * that UDO body, read the same way as csoundDebugGetVariables().
 *
 * callLine is the source line of the UDO call (from the call-site opcode).
 * frameIndex orders sibling calls on the same parent (0 = head / most recent
 * on the parent's opcod_deact chain).
 *
 * csoundDebugFreeUdoFrames() must be called when the list is no longer needed.
 * Not thread-safe; call from the k-cycle callback or between k-cycles.
 *
 * truncatedOut (may be NULL) is set to 1 when the walk could not enumerate all
 * active UDO frames (currently: saved-chain depth safety limit). When
 * non-zero, the returned list may be incomplete.
 */
PUBLIC debug_udo_frame_t *csoundDebugGetUdoFrames(CSOUND *csound,
                                                  debug_instr_t *instr,
                                                  int32_t *truncatedOut);

/** Free list from csoundDebugGetUdoFrames() */
PUBLIC void csoundDebugFreeUdoFrames(CSOUND *csound,
                                     debug_udo_frame_t *frameHead);


/** Get the list of global variables (orchestra-wide symbols)
 *
 * Enumerates every variable in csound->engineState.varPool — the global pool
 * holding gk*, ga*, gi*, gS*, gf*, global arrays, and Csound's internal
 * globals (sr, kr, ksmps, ...). Returns a debug_variable_t linked list in the
 * same format as csoundDebugGetVariables(): for scalar/audio/string types the
 * data pointer is ready to read; for "f" it points to a PVSDAT and for "["
 * to an ARRAYDAT (decode these with csoundDebugSerializeFsig() /
 * csoundDebugSerializeArray()).
 *
 * Unlike instrument-local variables, each global has its own storage block,
 * so the data pointers are taken from var->memBlock (not a shared lclbas).
 *
 * Returns NULL if the global pool is not available (before compilation).
 * data pointers are borrowed; free the list with csoundDebugFreeVariables().
 * Not thread-safe; call from the k-cycle callback or between k-cycles.
 */
PUBLIC debug_variable_t *csoundDebugGetGlobalVariables(CSOUND *csound);

/** Serialize an f-signal (PVSDAT) analysis frame into a flat float buffer
 *
 * varData must point to a PVSDAT, as provided by csoundDebugGetVariables() or
 * csoundDebugGetGlobalVariables() for a variable of type "f". The current
 * analysis frame is written to outBuf as 2*NB interleaved float32 values
 * (amp0, freq0, amp1, freq1, ...), regardless of whether the source frame is
 * float32 (normal) or MYFLT (sliding). For sliding analysis the most recent
 * active sub-frame in the current ksmps block is used.
 *
 * localKsmps is the producer's current local ksmps (from the instrument or UDO
 * instance that owns the f-signal). Pass 0 only when that producer is actually
 * using the engine-global ksmps; otherwise pass the producer's current local
 * ksmps, whether the producer is a UDO or a top-level instrument.
 *
 * infoOut (may be NULL) receives the frame metadata.
 *
 * Returns the total number of float values available (2*NB) and copies
 * min(2*NB, bufMax) of them. Returns 0 (and sets NB=0) when the frame has not
 * been allocated yet (frame.auxp == NULL, e.g. before the first analysis run)
 * or on invalid input.
 */
PUBLIC int32_t csoundDebugSerializeFsig(CSOUND *csound, void *varData,
                                        float *outBuf, int32_t bufMax,
                                        debug_fsig_info_t *infoOut,
                                        int32_t localKsmps);

/** Serialize a numeric array (ARRAYDAT) into a flat MYFLT buffer
 *
 * varData must point to an ARRAYDAT, as provided by csoundDebugGetVariables()
 * or csoundDebugGetGlobalVariables() for a variable of type "[". The flat
 * element data is copied to outBuf. Non-numeric arrays (e.g. S[] or f[])
 * return 0 with elementTypeName set so the caller can skip them.
 *
 * infoOut (may be NULL) receives the array shape and element type.
 *
 * Returns the total number of MYFLT values available and copies
 * min(total, bufMax) of them. Returns 0 on invalid/empty input.
 */
PUBLIC int32_t csoundDebugSerializeArray(CSOUND *csound, void *varData,
                                         MYFLT *outBuf, int32_t bufMax,
                                         debug_array_info_t *infoOut);


/** Set a per-k-cycle debug callback
 *
 * Registers a function that will be called after every k-cycle when the
 * debugger is active, once all active instrument instances have been
 * processed and before the audio output buffer is sent. This provides a
 * non-stopping hook into the debug performance loop suitable for real-time
 * variable inspection.
 *
 * Requires csoundDebuggerInit() to have been called first — the callback
 * only fires inside kperf_debug().
 *
 * Inside the callback, csoundDebugGetInstrInstances() and
 * csoundDebugGetVariables() can be used to read the current state of all
 * active instruments.
 *
 * Pass NULL for cb to remove a previously set callback.
 *
 * @param csound   Csound instance pointer
 * @param cb       pointer to callback function (NULL to remove)
 * @param userdata pointer to user data passed back to the callback
 */
PUBLIC void csoundSetDebugCallback(CSOUND *csound,
                                   debug_cb_t cb, void *userdata);

/** Remove the per-k-cycle debug callback
 *
 * Equivalent to calling csoundSetDebugCallback(csound, NULL, NULL).
 *
 * @param csound Csound instance pointer
 */
PUBLIC void csoundRemoveDebugCallback(CSOUND *csound);

/**  @} */

#ifdef __cplusplus
}
#endif

#endif
