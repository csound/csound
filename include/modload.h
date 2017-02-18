#ifndef __MODLOAD__H
#define __MODLOAD__H

#include <plugin.h>
namespace csnd {
  /** Plugin library entry point
   */
  void on_load(Csound *);
}

extern "C" {
PUBLIC int csoundModuleCreate(CSOUND *csound) { return 0; }
PUBLIC int csoundModuleDestroy(CSOUND *csound) { return 0; }
PUBLIC int csoundModuleInit(CSOUND *csound) {
  csnd::on_load((csnd::Csound *)csound);
  return 0;
  }
}
#endif
