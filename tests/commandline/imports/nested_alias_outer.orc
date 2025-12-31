/* nested_alias_outer.orc: Outer module that imports inner module with alias */

import "nested_alias_inner.orc" as inner

giOuterValue = 1000

/* UDO that uses the inner module's variable via alias */
opcode OuterGetInnerValue():i
    iout = inner.giInnerValue
    xout iout
endop

/* UDO that uses inner module's UDO */
opcode OuterTriple(ival):i
    /* Call inner's double, then add ival for triple effect */
    iDoubled = InnerDouble(ival)
    iout = iDoubled + ival
    xout iout
endop

/* UDO that modifies and returns inner value */
opcode OuterModifyInner(inewVal):i
    /* Read current value and return a computed value */
    iCurrent = inner.giInnerValue
    iout = iCurrent + inewVal
    xout iout
endop
