print [[

U S I N G   C S O U N D   6   V I A   L U A J I T   F F I

Copyright (C) 2013 by Michael Gogins.

This software is licensed under the terms of the
GNU Lesser General Public License.

This script requires LuaJIT from luajit.org.
This script demonstrates how to generate a Csound score in 
Lua, and then realize the score using Csound 6 via the 
Csound API accessed through LuaJIT's foreign function 
interface (FFI). 

The code prints comments to explain every step.

]]
print 'Local variables should be used whenever possible with LuaJIT.'
local string = require('string')
print 'Importing the foreign function interface namespace...'
local ffi = require('ffi')
print 'Declaring the Csound API functions we need to LuaJIT\'s FFI...'
print 'MYFLT must be replaced with its actual type.'
print 'CSOUND * would be tough to do and is replaced by void *.'
ffi.cdef[[
int csoundGetVersion();
int csoundGetAPIVersion();
int csoundInitialize (int *argc, char ***argv, int flags);
void *csoundCreate(void *hostdata);
void csoundDestroy(void *csound);
int csoundSetOption(void *csound, const char *option);
int csoundCompileOrc(void *csound, const char *orc);
int csoundReadScore(void *csound, const char *message);
int csoundScoreEvent (void *csound, char type, const double *pFields, long numFields);
int csoundStart(void *csound);
int csoundPerformKsmps(void *csound);
int csoundPerform(void *csound);
void csoundSetControlChannel(void *csound, const char *name, double value);
int csoundStop(void *csound);
int csoundReset(void *csound);
]]
print 'Loading the Csound shared library...'
local csoundApi = ffi.load('csound64')
print(string.format('Csound version: %5.2f', csoundApi.csoundGetVersion() / 1000.0))
print(string.format('Csound API version: %5.2f', csoundApi.csoundGetAPIVersion() / 100.0))
print 'Calling csoundInitialize...'
local argc = ffi.new('int *')
local argv = ffi.new('char ***')
result = csoundApi.csoundInitialize(argc, argv, 0);
local voidptr = ffi.new('void *')
print 'Creating an instance of Csound...'
local csound = csoundApi.csoundCreate(voidptr)
print 'Configuring Csound for performance (change output filename as required)...'
csoundApi.csoundSetOption(csound, '--sample-accurate')
csoundApi.csoundSetOption(csound, '--output=dac7')
csoundApi.csoundSetOption(csound, '--format=float')
csoundApi.csoundSetOption(csound, '--nodisplays')
print 'Compiling the Csound orchestra (note multi-line text in double brackets)...'
result = csoundApi.csoundCompileOrc(csound, [[
sr = 48000
ksmps = 100
nchnls = 2
            ; Define a control channel for the FM carrier.
            chn_k       "kcarrier", 1
            ; Define a simple FM instrument using built-in sine table.
            instr       1
khz         =           cpsmidinn(p4)
kamplitude  =           ampdb(p5) * 0.1
kmodulator  =           2
kcarrier    chnget      "kcarrier"
            ; Intensity of sidebands.
kindex      line        0, p3, 10
asignal     foscili     kamplitude, khz, kcarrier, kmodulator, kindex
            outs        asignal, asignal
            endin
]])
print 'Defining a difference equation for the logistic map...'
local c = .93847
local y = 0.5
local y1 = 0.5
local interval = 0.125
local duration = 0.5
local insno = 1
local scoretime = 0.5
-- local pfields = ffi.new('double[?]', 5)
print 'Iterating a loop to generate score events and send them to Csound...'
for i = 1, 200 do
    scoretime = scoretime + interval
    y1 = c * y * (1 - y) * 4
    y = y1
    local key = math.floor(36 + y * 60)
    local velocity = 80
    local message = string.format('i %d %9.4f %9.4f %9.4f %9.4f', insno, scoretime, duration, key, velocity)
    -- print(message)
    csoundApi.csoundReadScore(csound, message)
    -- Note that it would be possible to use csoundScoreEvent, but then performance will not terminate
    -- at the end of the score.
    -- pfields[0] = insno
    -- pfields[1] = scoretime
    -- pfields[2] = duration
    -- pfields[3] = key
    -- pfields[4] = velocity
    -- csoundApi.csoundScoreEvent(csound, string.byte('i', 1), pfields, 5)
end
print 'Having sent the entire score, starting the Csound performance...'
result = csoundApi.csoundStart(csound)
print [[Print iterating the performance loop until it returns "completed"...
This enables the Lua code to send data to Csound during performance 
via control channels. And Csound could also send data back to Lua this way.]]
local kcarrier = 1.0
while csoundApi.csoundPerformKsmps(csound)== 0 do   
    kcarrier = kcarrier + 0.0001
    csoundApi.csoundSetControlChannel(csound, 'kcarrier', kcarrier)
end
-- Not sure resetting is a good idea in this context.
-- print 'Resetting Csound to clean up after performance...'
-- csoundApi.csoundReset(csound)
