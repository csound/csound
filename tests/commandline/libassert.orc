opcode assert(condition:b, message:S):void
    if(!condition) then
        prints(message)
        exitnow(-1)
    endif
endop

opcode assertEquals(ival, iexpected):void
    if(ival != iexpected) then
        prints("Error: ival was %g, expected %g\n", ival, iexpected)
        exitnow(-1)
    endif
endop
