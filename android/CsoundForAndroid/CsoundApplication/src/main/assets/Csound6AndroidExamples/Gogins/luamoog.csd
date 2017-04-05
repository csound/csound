<CsoundSynthesizer>
<CsOptions>
-odac -m3
</CsOptions>
<CsInstruments>
sr =    48000
ksmps =   100
nchnls =    1

    gibegan     rtclock

    lua_opdef   "moogladder", {{
local ffi = require("ffi")
local math = require("math")
local string = require("string")
local csoundApi = ffi.load("csoundandroid")
ffi.cdef[[
    int csoundGetKsmps(void *);
    float csoundGetSr(void *);
    struct moogladder_t {
      float *out;
      float *inp;
      float *freq;
      float *res;
      float *istor;
      float sr;
      float ksmps;
      float thermal;
      float f;
      float fc;
      float fc2;
      float fc3;
      float fcr;
      float acr;
      float tune;
      float res4;
      float input;
      float i;
      float j;
      float k;
      float kk;
      float stg[6];
      float delay[6];
      float tanhstg[6];
    };
]]

local moogladder_ct = ffi.typeof('struct moogladder_t *')

function moogladder_init(csound, opcode, carguments)
    local p = ffi.cast(moogladder_ct, carguments)
    p.sr = csoundApi.csoundGetSr(csound)
    p.ksmps = csoundApi.csoundGetKsmps(csound)
    if p.istor[0] == 0 then
        for i = 0, 5 do
            p.delay[i] = 0.0
        end
        for i = 0, 3 do
            p.tanhstg[i] = 0.0
        end
    end
    return 0
end

function moogladder_kontrol(csound, opcode, carguments)
    local p = ffi.cast(moogladder_ct, carguments)
    -- transistor thermal voltage
    p.thermal = 1.0 / 40000.0
    if p.res[0] < 0.0 then
        p.res[0] = 0.0
    end
    -- sr is half the actual filter sampling rate
    p.fc = p.freq[0] / p.sr
    p.f = p.fc / 2.0
    p.fc2 = p.fc * p.fc
    p.fc3 = p.fc2 * p.fc
    -- frequency & amplitude correction
    p.fcr = 1.873 * p.fc3 + 0.4955 * p.fc2 - 0.6490 * p.fc + 0.9988
    p.acr = -3.9364 * p.fc2 + 1.8409 * p.fc + 0.9968
    -- filter tuning
    p.tune = (1.0 - math.exp(-(2.0 * math.pi * p.f * p.fcr))) / p.thermal
    p.res4 = 4.0 * p.res[0] * p.acr
    -- Nested 'for' loops crash, not sure why.
    -- Local loop variables also are problematic.
    -- Lower-level loop constructs don't crash.
    p.i = 0
    while p.i < p.ksmps do
        p.j = 0
        while p.j < 2 do
            p.k = 0
            while p.k < 4 do
                if p.k == 0 then
                    p.input = p.inp[p.i] - p.res4 * p.delay[5]
                    p.stg[p.k] = p.delay[p.k] + p.tune * (math.tanh(p.input * p.thermal) - p.tanhstg[p.k])
                else
                    p.input = p.stg[p.k - 1]
                    p.tanhstg[p.k - 1] = math.tanh(p.input * p.thermal)
                    if p.k < 3 then
                        p.kk = p.tanhstg[p.k]
                    else
                        p.kk = math.tanh(p.delay[p.k] * p.thermal)
                    end
                    p.stg[p.k] = p.delay[p.k] + p.tune * (p.tanhstg[p.k - 1] - p.kk)
                end
                p.delay[p.k] = p.stg[p.k]
                p.k = p.k + 1
            end
            -- 1/2-sample delay for phase compensation
            p.delay[5] = (p.stg[3] + p.delay[4]) * 0.5
            p.delay[4] = p.stg[3]
            p.j = p.j + 1
        end
        p.out[p.i] = p.delay[5]
        p.i = p.i + 1
    end
    return 0
end
}}

/*
Moogladder - An improved implementation of the Moog ladder filter

DESCRIPTION
This is an new digital implementation of the Moog ladder filter based on the work of Antti Huovilainen,
described in the paper \"Non-Linear Digital Implementation of the Moog Ladder Filter\" (Proceedings of DaFX04, Univ of Napoli).
This implementation is probably a more accurate digital representation of the original analogue filter.
This is version 2 (revised 14/DEC/04), with improved amplitude/resonance scaling and frequency correction using a couple of polynomials,as suggested by Antti.

SYNTAX
ar  Moogladder  asig, kcf, kres

PERFORMANCE
asig - input signal
kcf - cutoff frequency (Hz)
kres - resonance (0 - 1).

CREDITS
Victor Lazzarini
*/

                    opcode  moogladderu, a, akk
asig, kcf, kres     xin
                    setksmps    1
ipi                 =           4 * taninv(1)
/* filter delays */
az1                 init        0
az2                 init        0
az3                 init        0
az4                 init        0
az5                 init        0
ay4                 init        0
amf                 init        0
                    if          kres > 1 then
kres                =           1
                    elseif      kres < 0 then
kres                =           0
                    endif
/* twice the \'thermal voltage of a transistor\' */
i2v                 =           40000
/* sr is half the actual filter sampling rate  */
kfc                 =           kcf/sr
kf                  =           kcf/(sr*2)
/* frequency & amplitude correction  */
kfcr                =           1.8730 * (kfc^3) + 0.4955 * (kfc^2) - 0.6490 * kfc + 0.9988
kacr                =           -3.9364 * (kfc^2) + 1.8409 * kfc + 0.9968;
/* filter tuning  */
k2vg                =           i2v * (1 - exp(-2 * ipi * kfcr * kf))
/* cascade of 4 1st order sections         */
ay1                 =           az1 + k2vg * (tanh((asig - 4 * kres * amf * kacr) / i2v) - tanh(az1 / i2v))
az1                 =           ay1
ay2                 =           az2 + k2vg * (tanh(ay1 / i2v) - tanh(az2 / i2v ))
az2                 =           ay2
ay3                 =           az3 + k2vg * (tanh(ay2 / i2v) - tanh(az3 / i2v))
az3                 =           ay3
ay4                 =           az4 + k2vg * (tanh(ay3 / i2v) - tanh(az4 / i2v))
az4                 =           ay4
/* 1/2-sample delay for phase compensation  */
amf                 =           (ay4 + az5) *0.5
az5                 =           ay4
/* oversampling  */
ay1                 =           az1 + k2vg * (tanh((asig - 4 * kres * amf * kacr) / i2v) - tanh(az1 / i2v))
az1                 =           ay1
ay2                 =           az2 + k2vg * (tanh(ay1 / i2v) - tanh(az2 / i2v ))
az2                 =           ay2
ay3                 =           az3 + k2vg * (tanh(ay2 / i2v) - tanh(az3 / i2v))
az3                 =           ay3
ay4                 =           az4 + k2vg * (tanh(ay3 / i2v) - tanh(az4 / i2v))
az4                 =           ay4
amf                 =           (ay4 + az5) * 0.5
az5                 =           ay4
                    xout        amf
                    endop

instr 1
                prints      "No filter.\n"
	kfe         expseg      500, p3*0.9, 1800, p3*0.1, 3000
    kenv        linen       10000, 0.05, p3, 0.05
    asig        buzz        kenv, 100, sr/(200), 1
    ; afil      moogladder  asig, kfe, 1
                out         asig
endin

instr 2
                prints      "Native moogladder.\n"
	kfe         expseg      500, p3*0.9, 1800, p3*0.1, 3000
    kenv        linen       10000, 0.05, p3, 0.05
    asig        buzz        kenv, 100, sr/(200), 1
    afil        moogladder  asig, kfe, 1
                out         afil
endin

instr 3
                prints      "UDO moogladder.\n"
	kfe         expseg      500, p3*0.9, 1800, p3*0.1, 3000
    kenv        linen       10000, 0.05, p3, 0.05
    asig        buzz        kenv, 100, sr/(200), 1
    afil        moogladderu asig, kfe, 1
                out         afil
endin

instr 4
                prints      "Lua moogladder.\n"
    kres        init        1
    istor       init        0
	kfe         expseg      500, p3*0.9, 1800, p3*0.1, 3000
    kenv        linen       10000, 0.05, p3, 0.05
    asig        buzz        kenv, 100, sr/(200), 1
    afil        init        0
                lua_ikopcall    "moogladder", afil, asig, kfe, kres, istor
                out         afil
endin

instr 5
    giended     rtclock
    ielapsed    =           giended - gibegan
                print       ielapsed
    gibegan     rtclock
endin

</CsInstruments>

<CsScore>
f 1     0 65536 10 1
i 5.1   0   1
i 4     1   20
i 5.2   21  1
i 4     22  20
i 5.3   42  1
i 2     43  20
i 5.4   63  1
i 2     64  20
i 5.5   84  1
i 3     85  20
i 5.6   105 1
i 3     106 20
i 5.7   126 1
i 1     127 20
i 5.8   147 1
i 1     148 20
i 5.9   168 1
i 4     169 20
i 4     170 20
i 4     171 20
e
</CsScore>

</CsoundSynthesizer>
