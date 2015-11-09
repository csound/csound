/*
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
#include <OpcodeBase.hpp>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <pthread.h>
#include <string>
#include <vector>

using namespace std;

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

/**
 * L U A   O P C O D E S   F O R   C S O U N D
 *
 * Author: Michael Gogins
 * 1 September 2011
 *
 * These opcodes require LuaJIT 2.0 or later with FFI to be installed with
 * Csound. They allow users to define new opcodes in Lua, a modern language
 * with simple sytax, full lexical scoping, support for classes, and support
 * for functional programming. These opcodes are designed to be thread-safe.
 * LuaJIT runs almost as fast as C and faster than Csound orchestra code.
 */

/**
 * Stores Lua references to opcode subroutines for greater efficiency of
 * calling.
 */
struct keys_t
{
    keys_t() : init_key(0), kontrol_key(0), audio_key(0), noteoff_key(0) {}
    int init_key;
    int kontrol_key;
    int audio_key;
    int noteoff_key;
};

static pthread_mutex_t lc_getrefkey = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lc_manageLuaState = PTHREAD_MUTEX_INITIALIZER;

struct CriticalSection
{
    CriticalSection(pthread_mutex_t &mutex_) : mutex(mutex_), status(-1)
    {
        status = pthread_mutex_lock(&mutex);
    }
    ~CriticalSection()
    {
        if (status >= 0) {
            pthread_mutex_unlock(&mutex);
        }
    }
    pthread_mutex_t &mutex;
    int status;
};

/**
 * Thread-safe storage for Lua references to opcode subroutines.
 */
keys_t &manageLuaReferenceKeys(const lua_State *L,
                               const std::string &opcode, char operation)
{
    static std::map<const lua_State *,
                    std::map<std::string, keys_t> > luaReferenceKeys;
    keys_t *keys = 0;
    {
        CriticalSection criticalSection(lc_getrefkey);
        switch(operation)
        {
        case 'O':
        {
            keys = &luaReferenceKeys[L][opcode];
        }
        break;
        case 'C':
        {

            luaReferenceKeys.erase(L);
        }
        break;
        }
    }
    return *keys;
}


struct LuaStateForThread
{
    pthread_t thread;
    lua_State *L;
};

bool operator == (const LuaStateForThread& a, const LuaStateForThread &b)
{
    if (pthread_equal(a.thread, b.thread)) {
        return true;
    } else {
        return false;
    }
}

/**
 * Thread-safe storage for Lua states (virtual machines). There is one Lua state
 * per thread, rather than per instance of Csound, in case one instance of Csound
 * is running multiple threads with multiple instances of a Lua opcode.
 */
lua_State *manageLuaState(char operation)
{
    static std::vector<LuaStateForThread> luaStatesForThreads;
    CriticalSection criticalSection(lc_manageLuaState);
    LuaStateForThread luaStateForThread;
    luaStateForThread.thread = pthread_self();
    std::vector<LuaStateForThread>::iterator it =
      std::find(luaStatesForThreads.begin(),
                luaStatesForThreads.end(),
                luaStateForThread);
    lua_State *L = 0;
    switch(operation)
    {
    case 'O':
    {
        if (it == luaStatesForThreads.end())
        {
            luaStateForThread.L = lua_open();
            luaL_openlibs(luaStateForThread.L);
            luaStatesForThreads.push_back(luaStateForThread);
            L = luaStateForThread.L;
        }
        else
        {
            L = it->L;
        }
    }
    break;
    case 'C':
    {
        if (it != luaStatesForThreads.end()) {
            manageLuaReferenceKeys(it->L, "", 'C');
            luaStatesForThreads.erase(it);
        }
    }
    break;
    }
    return L;
}

/**
 * lua_exec Scode -- Execute an arbitrary block of Lua code at i-rate.
 */
class cslua_exec : public OpcodeBase<cslua_exec>
{
public:
    /**
     * No outputs.
     */
    /**
     * Inputs.
     */
    MYFLT *luacode_;
    int init(CSOUND *csound)
    {
        int result = OK;
        lua_State *L = manageLuaState('O');
        /* Ensure that Csound is available in the global environment. */
        lua_pushlightuserdata(L, csound);
        lua_setfield(L, LUA_GLOBALSINDEX, "csound");
        const char *luacode = ((STRINGDAT *)luacode_)->data;
        log(csound, "Executing (L: 0x%p) Lua code.\n", L);
        warn(csound, "\n%s\n", luacode);
        result = luaL_dostring(L, luacode);
        if (result == 0)
        {
            //log(csound, "Result: %d\n", result);
        }
        else
        {
            log(csound, "luaL_dostring failed with: %d\n%s\n",
                result, lua_tostring(L, -1));
        }
        return result;
    }
};

/**
 * lua_iopcall Sname, ...  Calls a Lua opcode at i-rate only. Any number of
 *                         output and/or input arguments may be passed.
 *                         All arguments must be passed on the right-hand
 *                         side and outputs are returned in the argument.
 *                         Requires opname_init to be defined in Lua.
 *
 * lua_ikopcall Sname, ... Calls a Lua opcode at i-rate and k-rate. Any number of
 *                         output and/or input arguments may be passed.
 *                         All arguments must be passed on the right-hand
 *                         side and outputs are returned in the argument.
 *                         Requires opname_init and opname_kontrol to be
 *                         defined in Lua.
 *
 * lua_iaopcall Sname, ... Calls a Lua opcode at i-rate and a-rate. Any number of
 *                         output and/or input arguments may be passed.
 *                         All arguments must be passed on the right-hand
 *                         side and outputs are returned in the argument.
 *                         Requires opname_init and opname_audio to be defined
 *                         in Lua.
 *
 * Opcode that actually implements arbitrary Lua opcodes
 * by calling from Csound into Lua functions.
 *
 * Lua functions access elements of arguments as follows
 * (pointers to both scalars and arrays are dereferenced by the array
 * access operator):
 * ffi.cdef(' struct arguments_t { double *a_out, double *i_in,
 *                                 double *i_txt, double *f_sig };');
 * local arguments = ffi.cast("struct arguments_t *", carguments_lightuserdata)
 * for i = 0, ksmps -1 do begin carguments.a_out[i] = carguments.i_in[0] * 3
 * end end
 */
class cslua_opcall: public OpcodeBase<cslua_opcall>
{
public:
    MYFLT *opcodename_;
    /**
      * This will hold, slot by slot, first output parameters
      * and then input parameters, exactly as declared by intypes and outtypes.
      * These parameters must also be re-declared in the Lua code as a C struct
      * using FFI (i.e. using 'ffi.cdef("typedef xxx typename;")' and
      * 'ctype = ffi.typeof("typename"))' and accessed using FFI in Lua code
      * after performing a type cast (i.e.using 'cobject = ffi.cast(ctype,
      * lightuserdata)'). Each slot, in turn, will contain a pointer to MYFLT
      * that references a Csound argument type (i.e. string, i-rate or k-rate
      * scalar, a-rate vector, f-sig). Note that the C struct may contain
      * additional data, e.g. for opcode state, after the opcode output and
      * input arguments, as limited by the available space.
      */
    MYFLT *arguments[1000];
    const char *opcodename;
public:
    /**
     * Calls a Lua function with the signature:
     * opcodename_init(csound [lightuserdata],
     *                 opcode [lightyserdata],
     *                 parameters [lightuserdata]) -> result [number].
     */
    int init(CSOUND *csound)
    {
        int result = OK;
        opcodename = ((STRINGDAT *)opcodename_)->data;
        lua_State *L = manageLuaState('O');
        keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
        lua_rawgeti(L, LUA_REGISTRYINDEX, keys.init_key);
        lua_pushlightuserdata(L, csound);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &arguments);
        if (lua_pcall(L, 3, 1, 0) != 0)
        {
            log(csound, "Lua error in \"%s_init\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return OK;
    }

    int kontrol(CSOUND *csound)
    {
        int result = OK;
        lua_State *L = manageLuaState('O');
        keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
        lua_rawgeti(L, LUA_REGISTRYINDEX, keys.kontrol_key);
        lua_pushlightuserdata(L, csound);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &arguments);
        if (lua_pcall(L, 3, 1, 0) != 0)
        {
            log(csound, "Lua error in \"%s_kontrol\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }

    int audio(CSOUND *csound)
    {
        int result = OK;
        lua_State *L = manageLuaState('O');
        keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
        lua_rawgeti(L, LUA_REGISTRYINDEX, keys.audio_key);
        lua_pushlightuserdata(L, csound);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, arguments);
        if (lua_pcall(L, 3, 1, 0) != 0)
        {
            log(csound, "Lua error in \"%s_audio\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }
};

/**
 * lua_iopcall_off Sname, ...  Calls a Lua opcode at i-rate only. Any number of
 *                             output and/or input arguments may be passed.
 *                             All arguments must be passed on the right-hand
 *                             side and outputs are returned in the argument.
 *                             Requires opname_init to be defined in Lua.
 *
 * lua_ikopcall_off Sname, ... Calls a Lua opcode at i-rate and k-rate. Any
 *                             number of output and/or input arguments may be
 *                             passed.
 *                             All arguments must be passed on the right-hand
 *                             side and outputs are returned in the argument.
 *                             Requires opname_init and opname_kontrol to be
 *                             defined in Lua.
 *
 * lua_iaopcall_off Sname, ... Calls a Lua opcode at i-rate and a-rate. Any
 *                             number of output and/or input arguments may be
 *                             passed.
 *                             All arguments must be passed on the right-hand
 *                             side and outputs are returned in the argument.
 *                             Requires opname_init and opname_audio to be
 *                             defined in Lua.
 *
 * Opcode that actually implements arbitrary Lua opcodes
 * by calling from Csound into Lua functions; this variant
 * of the opcode always schedules a "note off" event that is called
 * when the intrument instances is removed from the active list, and
 * which can be used to release unneeded resources, reschedule the
 * instrument with a reverb tail, and so on.
 *
 * Lua functions access elements of arguments as follows
 * (pointers to both scalars and arrays are dereferenced by the array
 *  access operator):
 * ffi.cdef(' struct arguments_t { double *a_out, double *i_in,
 *                                 double *i_txt, double *f_sig };');
 * local arguments = ffi.cast("struct arguments_t *", carguments_lightuserdata)
 * for i = 0, ksmps -1 do begin carguments.a_out[i] = carguments.i_in[0] * 3
 * end end
 */
class cslua_opcall_off: public OpcodeNoteoffBase<cslua_opcall_off>
{
public:
    MYFLT *opcodename_;
    /**
      * This will hold, slot by slot, first output parameters
      * and then input parameters, exactly as declared by intypes and outtypes.
      * These parameters must also be re-declared in the Lua code as a C struct
      * using FFI (i.e. using 'ffi.cdef("typedef xxx typename;")' and
      * 'ctype = ffi.typeof("typename"))' and accessed using FFI in Lua code
      * after performing a type cast (i.e.using 'cobject = ffi.cast(ctype,
      * lightuserdata)'). Each slot, in turn, will contain a pointer to MYFLT
      * that references a Csound argument type (i.e. string, i-rate or k-rate
      * scalar, a-rate vector, f-sig). Note that the C struct may contain
      * additional data, e.g. for opcode state, after the opcode output and
      * input arguments, as limited by the available space.
      */
    MYFLT *arguments[1000];
    const char *opcodename;
public:
    /**
     * Calls a Lua function with the signature:
     * opcodename_init(csound [lightuserdata],
     *                 opcode [lightyserdata],
     *                 parameters [lightuserdata]) -> result [number].
     */
    int init(CSOUND *csound)
    {
        int result = OK;
        opcodename = ((STRINGDAT *)opcodename_)->data;
        lua_State *L = manageLuaState('O');
        keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
        lua_rawgeti(L, LUA_REGISTRYINDEX, keys.init_key);
        lua_pushlightuserdata(L, csound);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &arguments);
        if (lua_pcall(L, 3, 1, 0) != 0)
        {
            log(csound, "Lua error in \"%s_init\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return OK;
    }

    int kontrol(CSOUND *csound)
    {
        int result = OK;
        lua_State *L = manageLuaState('O');
        keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
        lua_rawgeti(L, LUA_REGISTRYINDEX, keys.kontrol_key);
        lua_pushlightuserdata(L, csound);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &arguments);
        if (lua_pcall(L, 3, 1, 0) != 0)
        {
            log(csound, "Lua error in \"%s_kontrol\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }

    int audio(CSOUND *csound)
    {
        int result = OK;
        lua_State *L = manageLuaState('O');
        keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
        lua_rawgeti(L, LUA_REGISTRYINDEX, keys.audio_key);
        lua_pushlightuserdata(L, csound);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, arguments);
        if (lua_pcall(L, 3, 1, 0) != 0)
        {
            log(csound, "Lua error in \"%s_audio\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }

    int noteoff(CSOUND *csound)
    {
        int result = OK;
        lua_State *L = manageLuaState('O');
        keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
        lua_rawgeti(L, LUA_REGISTRYINDEX, keys.noteoff_key);
        lua_pushlightuserdata(L, csound);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, arguments);
        if (lua_pcall(L, 3, 1, 0) != 0)
        {
            log(csound, "Lua error in \"%s_noteoff\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        else
        {
            log(csound, "Lua called \"%s_noteoff\": %s.\n",
                opcodename, lua_tostring(L, -1));
        }
        result = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return result;
    }
};

/**
 * lua_opdef Sname, Scode -- Define an opcode in Lua at i-rate. The opcode can
 *                           take any number of output and/or input arguments
 *                           of any type.
 *
 * Opcode that effectively defines new opcodes in Lua. The Lua code must
 * define all functions that will be called from Csound,
 * using the following naming convention, where opcodename
 * stands for the actual opcode name:
 * <ul>
 * <li> opcodename_init for the i-rate opcode subroutine.
 * <li> opcodename_kontrol for the k-rate opcode subroutine.
 * <li> opcodename_audio for the a-rate opcode subroutine.
 * <li> opcodename_noteoff for the note-off subroutine.
 * </ul>
 * Each of these Lua functions will receive three lightuserdata
 * (i.e. pointer) arguments: the CSOUND object, the opcode instance,
 * and a pointer to the opcode arguments, which the Lua code must type cast
 * to a LuaJIT FFI ctype structure containing the opcode output arguments,
 * input arguments, and state variables. Using LuaJIT FFI, the elements of
 * this structure will be accessible as though they were Lua types.
 *
 * Each of these Lua functions must return 0 for success
 * or 1 for failure.
 *
 * The Lua functions may do absolutely anything, although of
 * course if real-time performance is expected, care must be
 * taken to disable Lua garbage collection and observe other
 * recommendations for real-time code.
 */
class cslua_opdef : public OpcodeBase<cslua_opdef>
{
public:
    /**
     * No outputs.
     */
    /**
     * Inputs.
     */
    MYFLT *opcodename_;
    MYFLT *luacode_;
public:
    int init(CSOUND *csound)
    {
        int result = OK;
        lua_State *L = manageLuaState('O');
        /* Ensure that Csound is available in the global environment. */
        lua_pushlightuserdata(L, csound);
        lua_setfield(L, LUA_GLOBALSINDEX, "csound");
        const char *opcodename = ((STRINGDAT *)opcodename_)->data;
        const char *luacode = ((STRINGDAT *)luacode_)->data;
        //log(csound, "Executing Lua code:\n%s\n", luacode);
        result = luaL_dostring(L, luacode);
        if (result == 0)
        {
            keys_t &keys = manageLuaReferenceKeys(L, opcodename, 'O');
            log(csound, "Opcode: %s\n", opcodename);
            log(csound, "Result: %d\n", result);
            char init_function[0x100];
            snprintf(init_function, 0x100,
                          "%s_init", opcodename); //h.optext->t.opcod);
            lua_getglobal(L, init_function);
            if (!lua_isnil(L, 1))
            {
                keys.init_key = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pop(L, 1);
            }
            char kontrol_function[0x100];
            snprintf(kontrol_function, 0x100,
                          "%s_kontrol", opcodename); //h.optext->t.opcod);
            lua_getglobal(L, kontrol_function);
            if (!lua_isnil(L, 1))
            {
                keys.kontrol_key = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pop(L, 1);
            }
            char audio_function[0x100];
            snprintf(audio_function, 0x100,
                          "%s_audio", opcodename); //h.optext->t.opcod);
            lua_getglobal(L, audio_function);
            if (!lua_isnil(L, 1))
            {
                keys.audio_key = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pop(L, 1);
            }
            char noteoff_function[0x100];
            snprintf(noteoff_function, 0x100,
                          "%s_noteoff", opcodename); //h.optext->t.opcod);
            lua_getglobal(L, noteoff_function);
            if (!lua_isnil(L, 1))
            {
                keys.noteoff_key = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_pop(L, 1);
            }
        }
        else
        {
            log(csound, "luaL_dostring failed with: %d\n", result);
        }
        return result;
    }
};

extern "C"
{
    /**
     * The only difference between the luacall opcodes is the "thread"
     * defining when they are called, which has exactly the same meaning as
     * for a C opcode. The user must take care to define, in Lua, the opcode
     * routines required by that specific "thread."
     */
    OENTRY oentries[] =
    {
        {
            (char*)"lua_exec",
            sizeof(cslua_exec),
            0,
            1,
            (char*)"",
            (char*)"TNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
            (SUBR) cslua_exec::init_,
            (SUBR) 0,
            (SUBR) 0,
        },
        {
            (char*)"lua_iopcall",
            sizeof(cslua_opcall),
            0,
            1,
            (char*)"",
            (char*)"TNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
            (SUBR) cslua_opcall::init_,
            (SUBR) 0,
            (SUBR) 0,
        },
        {
            (char*)"lua_ikopcall",
            sizeof(cslua_opcall),
            0,
            3,
            (char*)"",
            (char*)"TNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
            (SUBR) cslua_opcall::init_,
            (SUBR) cslua_opcall::kontrol_,
            (SUBR) 0,
        },
        {
            (char*)"lua_iaopcall",
            sizeof(cslua_opcall),
            0,
            5,
            (char*)"",
            (char*)"TNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
            (SUBR) cslua_opcall::init_,
            (SUBR) 0,
            (SUBR) cslua_opcall::audio_,
        },
        {
            (char*)"lua_iopcall_off",
            sizeof(cslua_opcall_off),
            0,
            1,
            (char*)"",
            (char*)"TNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
            (SUBR) cslua_opcall_off::init_,
            (SUBR) 0,
            (SUBR) 0,
        },
        {
            (char*)"lua_ikopcall_off",
            sizeof(cslua_opcall_off),
            0,
            3,
            (char*)"",
            (char*)"TNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
            (SUBR) cslua_opcall_off::init_,
            (SUBR) cslua_opcall_off::kontrol_,
            (SUBR) 0,
        },
        {
            (char*)"lua_iaopcall_off",
            sizeof(cslua_opcall_off),
            0,
            5,
            (char*)"",
            (char*)"TNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
            "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN",
            (SUBR) cslua_opcall_off::init_,
            (SUBR) 0,
            (SUBR) cslua_opcall_off::audio_,
        },
        {
            (char*)"lua_opdef",
            sizeof(cslua_opdef),
            0,
            1,
            // No outputs.
            (char*)"",
            // Inputs: name and Lua code.
            // The Lua code will usually be a multi-line string.
            (char*)"TT",
            (SUBR) cslua_opdef::init_,
            0,
            0,
        },
        {
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
        }
    };

    PUBLIC int csoundModuleCreate(CSOUND *csound)
    {
        return 0;
    }

    PUBLIC int csoundModuleInit(CSOUND *csound)
    {
        int status = 0;
        for(OENTRY *oentry = &oentries[0]; oentry->opname; oentry++)
        {
            status |= csound->AppendOpcode(csound,
                                           oentry->opname,
                                           oentry->dsblksiz,
                                           oentry->flags,
                                           oentry->thread,
                                           oentry->outypes,
                                           oentry->intypes,
                                           (int (*)(CSOUND*,void*)) oentry->iopadr,
                                           (int (*)(CSOUND*,void*)) oentry->kopadr,
                                           (int (*)(CSOUND*,void*)) oentry->aopadr);
        }
        manageLuaState('O');
        return status;
    }

    PUBLIC int csoundModuleDestroy(CSOUND *csound)
    {
        manageLuaState('C');
        return OK;
    }
}
