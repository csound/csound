print [[
U S I N G   C S O U N D   6   V I A   L U A J I T   F F I

Copyright (C) 2013 by Michael Gogins.

This software is licensed under the terms of the
GNU Lesser General Public License.
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
void csoundSetControlChannel(void *csound, const char *name, double value);
int csoundStop(void *csound);
int csoundReset(void *csound);
]]
local csoundApi = ffi.load('csound64')
print(string.format('Csound API version: %5.2f', csoundApi.csoundGetAPIVersion() / 100.0))
local argc = ffi.new('int *')
local argv = ffi.new('char ***')
result = csoundApi.csoundInitialize(argc, argv, 0);
local voidptr = ffi.new('void *')
local csound = csoundApi.csoundCreate(voidptr)
csoundApi.csoundSetOption(csound, '--output=dac14')
csoundApi.csoundSetOption(csound, '--format=float')
csoundApi.csoundSetOption(csound, '--nodisplays')
result = csoundApi.csoundCompileOrc(csound, [[
sr = 48000
ksmps = 100
nchnls = 2
            chn_k       "kcarrier", 1
            instr       1
khz         =           cpsmidinn(p4)
kamplitude  =           ampdb(p5) * 0.1
kmodulator  =           2
gkcarrier   chnget      "kcarrier"
            ; Intensity of sidebands.
kindex      line        0, p3, 10
asignal     foscili     kamplitude, khz, gkcarrier, kmodulator, kindex
            outs        asignal, asignal
            endin
]])
local c = .93847
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
    local message = string.format('i %d %9.4f %9.4f %9.4f %9.4f', insno, scoretime, duration, key, velocity)
    print(message)
    csoundApi.csoundReadScore(csound, message)
end
result = csoundApi.csoundStart(csound)
local kcarrier = 1.0
while csoundApi.csoundPerformKsmps(csound)== 0 do   
    local kcarrier = kcarrier + 0.01
    csoundApi.csoundSetControlChannel(csound, 'kcarrier', kcarrier)
end
-- csoundApi.csoundReset(csound)
