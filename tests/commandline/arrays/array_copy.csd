<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

ksmps = 32

instr copy_one_dim_i2i

 iArrSrc[] fillarray 1, 2, 3, 4, 5
 iArrCpy[] init 5
 iArrCpy = iArrSrc
 print iArrCpy[0], iArrCpy[4]

endin

instr copy_one_dim_k2k

 kArrSrc[] fillarray 1, 2, 3, 4, 5
 kArrCpy[] init 5
 kArrCpy = kArrSrc
 printk 0, kArrCpy[0]
 printk 0, kArrCpy[4]
 turnoff

endin

instr copy_one_dim_i2k

 iArrSrc[] fillarray 1, 2, 3, 4, 5
 kArrCpy[] init 5
 kArrCpy = iArrSrc
 printk 0, kArrCpy[0]
 printk 0, kArrCpy[4]
 turnoff

endin

instr copy_one_dim_k2i

 kArrSrc[] fillarray 1, 2, 3, 4, 5
 iArrCpy[] init 5
 iArrCpy = kArrSrc
 print iArrCpy[0]
 print iArrCpy[4]

endin

instr copy_two_dims_i2i

 iArrSrc[][] init 2, 3
 iArrSrc fillarray 1, 2, 3, 4, 5, 6
 iArrCpy[][] init 2, 3
 iArrCpy = iArrSrc
 print iArrCpy[0][0], iArrCpy[1][2]

endin

instr copy_two_dims_k2k

 kArrSrc[][] init 2, 3
 kArrSrc fillarray 1, 2, 3, 4, 5, 6
 kArrCpy[][] init 2, 3
 kArrCpy = kArrSrc
 printk 0, kArrCpy[0][0]
 printk 0, kArrCpy[1][2]
 turnoff

endin

instr copy_two_dims_i2k

 iArrSrc[][] init 2, 3 
 iArrSrc fillarray 1, 2, 3, 4, 5, 6
 kArrCpy[][] init 2, 3
 kArrCpy = iArrSrc
 printk 0, kArrCpy[0][0]
 printk 0, kArrCpy[1][2]
 turnoff

endin

instr copy_two_dims_k2i

 kArrSrc[][] init 2, 3 
 kArrSrc fillarray 1, 2, 3, 4, 5, 6
 iArrCpy[][] init 2, 3
 iArrCpy = kArrSrc
 print iArrCpy[0][0], iArrCpy[1][2]

endin


</CsInstruments>
<CsScore>
i "copy_one_dim_i2i" 0 0
i "copy_one_dim_k2k" .1 .1
i "copy_one_dim_i2k" .2 .1
i "copy_one_dim_k2i" .3 0
i "copy_two_dims_i2i" .4 0
i "copy_two_dims_k2k" .5 .1
i "copy_two_dims_i2k" .6 .1
i "copy_two_dims_k2i" .7 0
</CsScore>
</CsoundSynthesizer>

