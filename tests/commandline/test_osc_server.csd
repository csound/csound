<CsoundSynthesizer>
<CsOptions>
-n --port=7000
</CsOptions>
<CsInstruments>
0dbfs=1

chn_k "test",3

instr Exit
exitnow(-1)
endin

instr 1
 status:k,f:k,mess:S,n:k = OSClisten("/in", "fsi")
 if status:k > 0 then
  test1:k strcmpk mess, "hello"
  if test1 != 0 || f != 1 || n != 2 then
   printks2("exiting %f\n", -1)
   schedulek(Exit,0,0)
   else
    printk2 f
  endif
 endif
endin


instr 2
 host:S = "localhost"
 port:i = 7000
 OSCsend(2,  host, port, "/in", 
 	"fsi", p4, "hello", p5)
 OSCsend(3,  host, port, "/ina", 
 	"fi", 3, 4)
 OSCsend(1, host, port, "/csound/channel/test", 
 	"f", 1)
endin

instr 3
 host:S = "localhost"
 port:i = 7000
 OSCsend(0, host, port, "/csound/event/instr", 
 	"fffff", 2, 0, 1, 1, 2)
endin

instr 4
 host:S = "localhost"
 port:i = 7000
 OSCsend(0, host, port, "/csound/event", "s",
 	{{ i2 0 1 1 2}})
endin

instr 5
 host:S = "localhost"
 port:i = 7000
 test:k = chnget("test")
 if test != 1 then
  schedulek(Exit,0,0)
 endif
 printk2(test)
 OSCsend(0, host, port, "/csound/compile", "s",
 	{{ event_i "e", 0, 0}})
endin

instr 6
 event_i "e", 0, 0
endin

schedule(1,0,4)
schedule(3,1,1)
schedule(4,2,1)
schedule(5,3,1)
//schedule(6,4,1)

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>

