/* nested_alias_inner.orc: Inner module that will be imported with alias */

giInnerValue = 500

opcode InnerDouble(ival):i
    iout = ival * 2
    xout iout
endop
