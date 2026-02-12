/*
  plugin_example_cpp.cpp: simple C++ oscillator opcode example

  Copyright (C) 2026
  This file is part of Csound.
*/

#include <modload.h>
#include <cmath>

struct Hello440 : csnd::Plugin<1, 0> {
  double phase = 0.0;
  double phase_inc = 0.0;

  int init() {
    const double two_pi = 6.283185307179586;
    phase_inc = (two_pi * 440.0) / sr();
    return OK;
  }

  int aperf() {
    const double two_pi = 6.283185307179586;
    csnd::AudioSig out(this, outargs(0));
    for (auto &sample : out) {
      sample = static_cast<MYFLT>(0.2 * std::sin(phase));
      phase += phase_inc;
      if (phase >= two_pi) {
        phase -= two_pi;
      }
    }
    return OK;
  }
};

void csnd::on_load(Csound *csound) {
  csnd::plugin<Hello440>(csound, "hello440", "a", "", csnd::thread::a);
}
