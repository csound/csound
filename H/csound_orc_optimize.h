/* Private orchestra optimizer pass selection. */
#pragma once

enum {
    CSOUND_OPT_LEGACY = 1 << 0,
    CSOUND_OPT_CONSTANT_FOLDING = 1 << 1,
    CSOUND_OPT_COPY_REMOVAL = 1 << 2,
    CSOUND_OPT_CSE = 1 << 3
};
