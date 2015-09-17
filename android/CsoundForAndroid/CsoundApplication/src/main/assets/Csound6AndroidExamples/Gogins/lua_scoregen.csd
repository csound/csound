<CsoundSynthesizer>
<CsOptions>
-odac -dm3 --sample-accurate
</CsOptions>
<CsInstruments>

sr      = 44100
ksmps   = 100
nchnls  = 2


; Lua code to generate a score in the orchestra header.
lua_exec {{
print (package.path)
package.path = package.path .. ";/storage/emulated/0/silencio/?.lua"
local ffi = require("ffi")
local math = require("math")
local string = require("string")

local silencio = require ("Silencio")
local csoundApi = ffi.load("csoundandroid")
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

local c = .999938749
local y = 0.57
local y1 = 0.58
local interval = 1 / 5
local duration = 4 * interval
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


gasendL		init		0
gasendR		init		0 

chnset      .5, "slider4"

            instr       1
            ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
            ; Simple FM instrument.
            ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
khz         =           cpsmidinn(p4)
kamplitude  =           ampdb(p5) * 0.1
ktrackpady  chnget      "slider1"
kcarrier    =           1 + 10 * ktrackpady
ktrackpadx  chnget      "slider2"
kmodulator  =           1 + 10 * ktrackpadx
iattack     =           0.03
irelease    =           0.1
isustain    =           p3
p3          =           iattack + isustain + irelease
            ; Intensity sidebands.
intensity   chnget      "slider3"
kindex      line        intensity * 20, p3, 0.25
isine       ftgenonce   1, 0, 16384, 10, 1
asignal     foscili     kamplitude, khz, kcarrier, kmodulator, kindex, isine
adamping    linseg      0, iattack, 1, isustain, 1, irelease, 0
asignal     =           asignal * adamping
gasendL     =           gasendL + asignal
gasendR     =           gasendR + asignal
            endin

		instr		reverb
print p1, p2, p3
kreverbdelay  chnget      "slider4"
aL,aR		reverbsc	gasendL, gasendR,kreverbdelay, 10000
		outs		aL, aR
		clear		gasendL, gasendR
		endin
            
alwayson "reverb"

</CsInstruments>
<CsScore>
f 0 3600
</CsScore>
</CsoundSynthesizer>
