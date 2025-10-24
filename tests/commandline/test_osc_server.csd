<CsoundSynthesizer>
<CsOptions>
-odac --port=7001
</CsOptions>
<CsInstruments>
0dbfs=1

gih OSCinit 7000

instr 1
OSCsend 0, "localhost", 7001, "/csound/compile", "s",
{{
instr 2
kans, k1 OSClisten "/test", "f"
if kans > 0 then
 printk2 k1
endif
endin

instr 3
 exitnow(0)
endin
}}

OSCsend 0, "localhost", 7001, "/csound/channel/test", "f", 1
if timeinsts:k() > 0.5 then
 OSCsend 0,"localhost", 7001, "/csound/event/instr", "fff",2,0,2
 OSCsend 0,"localhost", 7001, "/test", "f", chnget:k("test")
 OSCsend 0,"localhost", 7001, "/csound/event", "s", {{i 3 1 0 }}
endif
endin
schedule(1,0,1)

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>

