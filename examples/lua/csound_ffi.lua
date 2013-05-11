print [[
U S I N G   C S O U N D   6   V I A   L U A J I T   F F I

Copyright (C) 2013 by Michael Gogins.

This software is licensed under the terms of the
GNU Lesser General Public License.

This program demonstrates the use of Csound
for score generation and peformance through its C API
and LuaJIT's FFI facilities.
]]
local ffi = require('ffi')
ffi.cdef[[
int csoundGetAPIVersion();
int csoundInitialize (int *argc, char ***argv, int flags);
void *csoundCreate(void *hostdata);
void csoundDestroy(void *csound);
int csoundSetOption(void *csound, const char *option);
int csoundCompileOrc(void *csound, const char *orc);
int csoundReadScore(void *csound, const char *message);
int csoundStart(void *csound);
int csoundPerformKsmps(void *csound);
int csoundPerform(void *csound);
]]
local csoundApi = ffi.load('csound64')
apiversion = csoundApi.csoundGetAPIVersion() / 100.0
print(string.format('Csound API version: %5.2f', apiversion))
local argc = ffi.new('int *')
local argv = ffi.new('char ***')
result = csoundApi.csoundInitialize(argc, argv, 0);
local voidptr = ffi.new('void *')
local csound = csoundApi.csoundCreate(voidptr)
local orc = [[
sr = 48000
ksmps = 100
nchnls = 2
            instr       1
            ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
            ; Simple FM instrument.
            ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
khz         =           cpsmidinn(p4)
kamplitude  =           ampdb(p5) * 0.1
kcarrier    =           1
kmodulator  =           1.44
            ; Intensity sidebands.
kindex      line        0, p3, 1
isine       ftgenonce   1, 0, 16384, 10, 1
asignal     foscili     kamplitude, khz, kcarrier, kmodulator, kindex, isine
            outs        asignal, asignal
            endin
]]
csoundApi.csoundSetOption(csound, '-odac')
result = csoundApi.csoundCompileOrc(csound, orc)
print(string.format('result: %d', result))
result = csoundApi.csoundStart(csound)
print(string.format('result: %d', result))

-- Compute a score using the logistic equation.

local c = .93849
local y = 0.5
local y1 = 0.5
local interval = 0.125
local duration = 0.5
local insno = 1
local scoretime = 0.5

for i = 1, 200 do
    scoretime = scoretime + interval
    y1 = c * y * (1 - y) * 4
    y = y1
    local key = math.floor(36 + y * 60)
    local velocity = 80

    -- Format each iteration of the logistic equation as a Csound score event and schedule it.

    local message = string.format('i %d %9.4f %9.4f %9.4f %9.4f', insno, scoretime, duration, key, velocity)
    print(message)
    csoundApi.csoundReadScore(csound, message)
end
message = string.format('e %9.4f', scoretime + 5.0)
print(message)
csoundApi.csoundReadScore(csound, message)
csoundApi.csoundPerform(csound)
print 'Finished....'
