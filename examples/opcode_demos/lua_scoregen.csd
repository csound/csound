<CsoundSynthesizer>

<CsInstruments>

sr      = 48000
ksmps   = 100
nchnls  = 2

; Lua code to generate a score in the orchestra header.

lua_exec {{
local ffi = require("ffi")
local math = require("math")
local string = require("string")
local csoundApi = ffi.load('csound64.dll.5.2')
-- Declare the parts of the Csound API that we need.
ffi.cdef[[
    int csoundGetKsmps(void *);
    double csoundGetSr(void *);
    int csoundInputMessage(void *, const char *message);
]]
-- Print that we have acquired the global Csound object.
print('csound:', csound)
-- Prove that the Csound API and object are working.
print('sr:', csoundApi.csoundGetSr(csound))

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
    csoundApi.csoundInputMessage(csound, message)
end

}}

            instr       1
            ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
            ; Simple FM instrument.
            ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
khz         =           cpsmidinn(p4)
kamplitude  =           ampdb(p5) * 0.1
kcarrier    =           1
kmodulator  =           1.44
            ; Intensity sidebands.
kindex      line        0, p3, 20	
isine       ftgenonce   1, 0, 16384, 10, 1
asignal     foscili     kamplitude, khz, kcarrier, kmodulator, kindex, isine
            outs        asignal, asignal
            endin
            
</CsInstruments>

<CsScore>
e 4.0
</CsScore>

</CsoundSynthesizer>
