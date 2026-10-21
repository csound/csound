/*
  plugin_example_cpp.cpp: simple C++ oscillator opcode example

  Copyright (C) 2026
  This file is part of Csound.
*/

#include <modload.h>
#include <cmath>
#ifdef CSOUND_WASI_SHARED_PLUGIN
#include <atomic>
static std::atomic<int> plugin_constructor_count;
static thread_local int plugin_thread_value = 42;
struct SharedPluginInit {
  SharedPluginInit() { ++plugin_constructor_count; }
};
static SharedPluginInit plugin_shared_init;
#endif

struct Hello440 : csnd::Plugin<1, 0> {
  cs_double phase = 0.0;
  cs_double phase_inc = 0.0;

  int init() {
#ifdef CSOUND_WASI_SHARED_PLUGIN
    if (plugin_constructor_count.load() != 1 || plugin_thread_value != 42)
      return NOTOK;
    CSOUND *engine = csound->get_csound();
    engine->Message(engine, "shared C++ plugin: ctor=1 tls=42\n");
#endif
    const cs_double two_pi = 6.283185307179586;
    phase_inc = (two_pi * 440.0) / sr();
    return OK;
  }

  int aperf() {
#ifdef CSOUND_WASI_SHARED_PLUGIN
    if (plugin_constructor_count.load() != 1 || plugin_thread_value != 42)
      return NOTOK;
#endif
    const cs_double two_pi = 6.283185307179586;
    csnd::AudioSig out(this, outargs(0));
    for (auto &sample : out) {
      sample = static_cast<cs_float>(0.2 * std::sin(phase));
      phase += phase_inc;
      if (phase >= two_pi) {
        phase -= two_pi;
      }
    }
    return OK;
  }
};

void csnd::on_load(Csound *csound) {
  csnd::plugin<Hello440>(csound, "hello440", "a", "", csnd::thread::ia);
}
