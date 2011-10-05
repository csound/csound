<CsoundSynthesizer>

<CsInstruments>
lua_opdef "luatest", {{
local ffi = require("ffi")
local string = require("string")
local csoundLibrary = ffi.load('/Library/Frameworks/CsoundLib.framework/CsoundLib')
ffi.cdef[[
    int csoundGetKsmps(void *);
    double csoundGetSr(void *);
    struct luatest_arguments {double *out; double *stringout; char *stringin; double *in1; double *in2; double sr; int ksmps; };
]]
function luatest_init(csound, opcode, carguments)
    local arguments = ffi.cast("struct luatest_arguments *", carguments)
    arguments.sr = csoundLibrary.csoundGetSr(csound)
    print(string.format('stringin: %s', ffi.string(arguments.stringin)))
    print(string.format('sr: %f', arguments.sr))
    arguments.ksmps = csoundLibrary.csoundGetKsmps(csound)
    print(string.format('ksmps: %d', arguments.ksmps))
    arguments.out[0] = arguments.in1[0] * arguments.in2[0]
    ffi.copy(arguments.stringout, 'Hello, world!')
    return 0
end
function luatest_kontrol(csound, opcode, carguments)
    local arguments = ffi.cast("struct luatest_arguments *", carguments)
    arguments.out[0] = arguments.in1[0] * arguments.in2[0]
    return 0
end
function luatest_noteoff(csound, opcode, carguments)
    local arguments = ffi.cast("struct luatest_arguments *", carguments)
    arguments.out[0] = arguments.in1[0] * arguments.in2[0]
    print('off')
    return 0
end
}}

instr 1
    iresult = 0
    Stringin = "stringin"
    Stringout = "stringout"
    lua_iopcall "luatest", iresult, Stringout, Stringin, p2, p3
    prints Stringout
endin
instr 2
    iresult = 0
    Stringin = "stringin"
    Stringout = "initial value"
    lua_iopcall_off "luatest", iresult, Stringout, Stringin, p2, p3
    print iresult
    prints Stringout
endin
</CsInstruments>

<CsScore>
i 1 1 2
i 2 2 2
i 1 3 2
e
</CsScore>

</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>72</x>
 <y>179</y>
 <width>400</width>
 <height>200</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>231</r>
  <g>46</g>
  <b>255</b>
 </bgcolor>
 <bsbObject version="2" type="BSBVSlider">
  <objectName>slider1</objectName>
  <x>5</x>
  <y>5</y>
  <width>20</width>
  <height>100</height>
  <uuid>{68aef6e9-fc92-45d7-9c08-0ba0f7162849}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.00000000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
<MacOptions>
Version: 3
Render: Real
Ask: Yes
Functions: ioObject
Listing: Window
WindowBounds: -900 -700 400 200
CurrentView: io
IOViewEdit: On
Options:
</MacOptions>

<MacGUI>
ioView nobackground {59367, 11822, 65535}
ioSlider {5, 5} {20, 100} 0.000000 1.000000 0.000000 slider1
</MacGUI>
