/*
  modload.h
  CPOF Csound Plugin Opcode Framework
  module load C functions and entry point

  (c) Victor Lazzarini, 2017

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA

*/
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
