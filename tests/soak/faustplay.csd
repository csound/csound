<CsoundSynthesizer>
<CsOptions>
-d -odac
</CsOptions>
<CsInstruments>
nchnls= 1

giph faustcompile {{
SR = 44100;
decimal(a) = a - floor(a);
freq = hslider("freq", 0, -20000, 20000, 1);
incr(fr) =  fr / float(SR);
phasor(fr,ph) =  incr(fr) : (+ : decimal) ~ _ :
+(ph) : decimal;
process = phasor(freq,0);
}}, "-vec -lv 1"

instr 1
kb1 = p5
ib faustdsp giph
faustctl ib,"freq",kb1
asig faustplay ib
out sin(2*$M_PI*asig)*p4*0dbfs
endin


</CsInstruments>
<CsScore>
i1 0 1 0.5 150
</CsScore>
</CsoundSynthesizer>


