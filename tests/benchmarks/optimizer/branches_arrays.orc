instr 1
 kvalues[] fillarray 1,2,3,4,5,6,7,8
 ksum = 0
 kindx = 0
 while kindx < 8 do
  if kvalues[kindx] > 4 then
   ksum += kvalues[kindx]*0.5
  else
   ksum += kvalues[kindx]*0.25
  endif
  kindx += 1
 od
 aout = ksum*0.001
 out aout
endin
