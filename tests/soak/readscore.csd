<CsoundSynthesizer>  
<CsOptions>
-d -o dac
</CsOptions> 
<CsInstruments>   
 
instr 1
a1 flooper2 1000,p4,2,5,0.1,1 
   out a1
endin

instr 2
ires readscore {{
{12  COUNT 
i1 $COUNT 1 [1 + $COUNT/12]
}
}}
endin
 
</CsInstruments>
<CsScore>
f0 12
f 1 0 0 1 "fox.wav" 0 0 1

i2 0 1



</CsScore>
</CsoundSynthesizer> 
 
