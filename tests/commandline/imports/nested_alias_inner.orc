/* nested_alias_inner.orc: Inner module that will be imported with alias */

giInnerValue = 500

opcode InnerDouble, i, i
    ival xin
    xout ival * 2
endop
