/* alias_module_b.orc: Test module B for alias testing */

giModuleB_Value = 200

opcode ModuleB_Triple(ival):i
    iout = ival * 3
    xout iout
endop
