/*
 * Csound synthesizer
 *
 * Copyright (c) 2003 by John D. Ramsdell. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cstdarg>
#include <string>
#include <vector>
#include <sstream>
#include <csound/csound.h>
#include "curve.hpp"
#include "synthesizer.hpp"

#define SINGLE_INSTANCE_CSOUND

namespace {

#if defined SINGLE_INSTANCE_CSOUND
  Synthesizer *s_synth = 0;

  inline Synthesizer *get_synth_instance(CSOUND *csound)
  {
    return s_synth;
  }
#else
  inline Synthesizer *get_synth_instance(CSOUND *csound)
  {
    return (Synthesizer *)csoundGetHostData(csound);
  }
#endif

  // This namespace is used to hide WINDAT from the synthesizer header.

  extern "C" void make_a_graph(CSOUND *csound, WINDAT *windat, char *name)
  {
  }

  extern "C" void draw_a_graph(CSOUND *csound, WINDAT *windat)
  {
    windat->caption[CAPSIZE - 1] = 0; // Just in case...
    Polarity polarity;
    // translate polarities and hope the definition in Csound doesn't change.
    switch (windat->polarity) {
    case NEGPOL:
      polarity = POLARITY_NEGPOL;
      break;
    case POSPOL:
      polarity = POLARITY_POSPOL;
      break;
    case BIPOL:
      polarity = POLARITY_BIPOL;
      break;
    default:
      polarity = POLARITY_NOPOL;
    }
    Curve *curve
      = new Curve(windat->fdata,
                  windat->npts,
                  windat->caption,
                  polarity,
                  windat->max,
                  windat->min,
                  windat->absmax,
                  windat->oabsmax,
                  windat->danflag);
    Synthesizer::s_draw_graph(csound, curve);
  }

  extern "C" void kill_a_graph(CSOUND *csound, WINDAT *windat)
  {
  }

  extern "C" int exit_a_graph(CSOUND *csound)
  {
    return 0;
  }
};

Synthesizer::Synthesizer()
  : m_csound(csoundCreate(this)),
    m_yield_data(0),
    m_yield_callback(0),
    m_message_data(0),
    m_message_callback(0),
    m_throw_message_data(0),
    m_throw_message_callback(0),
    m_draw_graph_data(0),
    m_draw_graph_callback(0)
{
#if defined SINGLE_INSTANCE_CSOUND
  s_synth = this;
#endif
}

Synthesizer::~Synthesizer()
{
  csoundDestroy(m_csound);
}

void Synthesizer::set_yield_callback(void *data, yield_callback_t callback)
{
  m_yield_data = data;
  m_yield_callback = callback;
  csoundSetYieldCallback(m_csound, s_yield);
}

int Synthesizer::s_yield(CSOUND *csound)
{
  Synthesizer *synth = get_synth_instance(csound);
  return !synth || synth->yield();
}

int Synthesizer::yield()
{
  return !m_yield_callback || (*m_yield_callback)(m_yield_data);
}

void Synthesizer::set_message_callback(void * data,
                                       message_callback_t callback)
{
  m_message_data = data;
  m_message_callback = callback;
  csoundSetMessageCallback(m_csound, s_message);
}

void Synthesizer::s_message(CSOUND *csound, const char *format, va_list args)
{
  Synthesizer *synth = get_synth_instance(csound);
  if (synth)
    synth->message(format, args);
}

void Synthesizer::message(const char *format, va_list args)
{
  if (m_message_callback)
    (*m_message_callback)(m_message_data, format, args);
}

void Synthesizer::set_throw_message_callback(void *data,
                                             message_callback_t callback)
{
  m_throw_message_data = data;
  m_throw_message_callback = callback;
  csoundSetThrowMessageCallback(m_csound, s_throw_message);
}

void Synthesizer::s_throw_message(CSOUND *csound, const char *format,
                                  va_list args)
{
  Synthesizer *synth = get_synth_instance(csound);
  if (synth)
    synth->throw_message(format, args);
}

void Synthesizer::throw_message(const char *format, va_list args)
{
  if (m_throw_message_callback)
    (*m_throw_message_callback)(m_throw_message_data, format, args);
}

void Synthesizer::set_draw_graph_callback(void *data,
                                          draw_graph_callback_t callback)
{
  m_draw_graph_data = data;
  m_draw_graph_callback = callback;
  csoundSetIsGraphable(m_csound, true);
  csoundSetMakeGraphCallback(m_csound, make_a_graph);
  csoundSetDrawGraphCallback(m_csound, draw_a_graph);
  csoundSetKillGraphCallback(m_csound, kill_a_graph);
  csoundSetExitGraphCallback(m_csound, exit_a_graph);
}

void Synthesizer::s_draw_graph(CSOUND *csound, Curve *curve)
{
  Synthesizer *synth = get_synth_instance(csound);
  if (synth)
    synth->draw_graph(curve);
}

void Synthesizer::draw_graph(Curve *curve)
{
  if (m_draw_graph_callback)
    (*m_draw_graph_callback)(m_draw_graph_data, curve);
}

int Synthesizer::perform(int argc, char **argv)
{
  int rc = csoundPerform(m_csound, argc, argv);
  csoundCleanup(m_csound);
  csoundReset(m_csound);
  return rc;
}

int Synthesizer::perform(std::vector<std::string> args)
{
  size_t argc = args.size();
  char *argv[argc + 1];
  for (size_t i = 0; i < argc; i++) {
    size_t n = args[i].length();
    char *cp = new char[n + 1];
    cp[args[i].copy(cp, n)] = 0;
    argv[i] = cp;
  }
  argv[argc] = 0;
  int rc = perform(argc, argv);
  for (size_t i = 0; i < argc; i++)
    delete argv[i];
  return rc;
}

std::string Synthesizer::get_version()
{
  int version = csoundGetVersion();
  int major = version / 1000;
  int minor = (version % 1000) / 10;
  std::ostringstream ost;
  ost << major << '.';
  ost.width(2);
  ost.fill('0');
  ost << minor;
  return ost.str();
}

