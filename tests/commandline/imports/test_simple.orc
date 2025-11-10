/* test_simple.orc: A very simple test module for verification
 * Copyright (C) 2025 Csound Developers
 * This file is part of Csound.
 */

/* Simple test UDO that returns a constant value */
opcode TestConst, i, i
    iVal xin
    xout iVal
endop

/* Module-level variable for testing */
giTestVar = 42
gSTestString = "Hello from module"