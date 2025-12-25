/* alias_module_b.orc: Test module B for alias testing */

giModuleB_Value = 200

opcode ModuleB_Triple, i, i
    ival xin
    xout ival * 3
endop
