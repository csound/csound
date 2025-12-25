/* alias_module_a.orc: Test module A for alias testing */

giModuleA_Value = 100

opcode ModuleA_Double, i, i
    ival xin
    xout ival * 2
endop
