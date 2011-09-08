<CsoundSynthesizer>

<CsInstruments>
lua_opdef "luatest", {{
local ffi = require("ffi")
local string = require("string")
local csoundLibrary = ffi.load('csound64.dll.5.2')
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
