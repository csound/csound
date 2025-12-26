/* nested_module.orc: Module that imports another module */

import "simple_test.orc"

opcode NestedOsc(kfreq):a
    /* Use the UDO from simple_test */
    aout = TestOsc(kfreq * 2)
    xout aout
endop

giNestedValue = 100
