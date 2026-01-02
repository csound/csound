<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>
; Test: Import global struct variables from a module
; This tests importing struct types, global struct variables, and UDOs that use them

sr = 44100
kr = 4410
nchnls = 2
0dbfs = 1.0

; Import the struct type, global struct variable, and UDO
from "lib_struct_globals.orc" import Vec2, gVec, giSimple, printVec

instr 1
    ; Print values from imported simple global
    prints "giSimple = %d\n", giSimple
    
    ; Print values from imported struct global (member access)
    prints "gVec.x = %d, gVec.y = %d\n", gVec.x, gVec.y
    
    ; Call the imported UDO that uses the global struct
    printVec
    
    ; Verify we can use the imported struct type for local variables
    kLocal:Vec2 init 100, 200
    prints "kLocal.x = %d, kLocal.y = %d\n", kLocal.x, kLocal.y
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
