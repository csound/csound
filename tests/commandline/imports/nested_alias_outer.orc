/* nested_alias_outer.orc: Outer module that imports inner module with alias */

import "nested_alias_inner.orc" as inner

giOuterValue = 1000

/* UDO that uses the inner module's variable via alias */
opcode OuterGetInnerValue, i, 0
    xout inner.giInnerValue
endop

/* UDO that uses inner module's UDO */
opcode OuterTriple, i, i
    ival xin
    /* Call inner's double, then add ival for triple effect */
    iDoubled InnerDouble ival
    xout iDoubled + ival
endop

/* UDO that modifies and returns inner value */
opcode OuterModifyInner, i, i
    inewVal xin
    /* Note: Direct assignment to module alias var not yet supported,
       so we just read and return a computed value */
    iCurrent = inner.giInnerValue
    xout iCurrent + inewVal
endop
