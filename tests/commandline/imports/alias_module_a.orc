/* alias_module_a.orc: Test module A for alias testing */

giModuleA_Value = 100

opcode ModuleA_Double(ival):i
    iout = ival * 2
    xout iout
endop
