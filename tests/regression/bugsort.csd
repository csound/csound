<CsoundSynthesizer>
<CsOptions>
;;; Valgrind is pointing to a memory error in sort.c

;;; ==77802== Invalid read of size 8
;;; ==77802==    at 0x361AD: sort (in
;;; /Users/victor/src/csound6/debug/CsoundLib64.framework/Versions/6.0/CsoundLib
;;; 64)
;;; ==77802==  Address 0x1005170c8 is 0 bytes after a block of size 8 alloc'd
;;; ==77802==    at 0xD6D9: malloc (vg_replace_malloc.c:266)
;;; ==77802==    by 0x3614E: sort (in
;;; /Users/victor/src/csound6/debug/CsoundLib64.framework/Versions/6.0/CsoundLib
;;; 64)
;;; ==77802== 
;;; ==77802== Invalid read of size 8
;;; ==77802==    at 0x361FB: sort (in
;;; /Users/victor/src/csound6/debug/CsoundLib64.framework/Versions/6.0/CsoundLib
;;; 64)
;;; ==77802==  Address 0x1005170b8 is 8 bytes before a block of size 8 alloc'd
;;; ==77802==    at 0xD6D9: malloc (vg_replace_malloc.c:266)
;;; ==77802==    by 0x3614E: sort (in
;;; /Users/victor/src/csound6/debug/CsoundLib64.framework/Versions/6.0/CsoundLib
;;; 64)
;;; ==77802== 

;;; do you have any ideas?
</CsOptions>
<CsInstruments>
0dbfs=1
nchnls = 2

instr 1

endin

</CsInstruments>
<CsScore>

i1 0 1
</CsScore>
</CsoundSynthesizer>

