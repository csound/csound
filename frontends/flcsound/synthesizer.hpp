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

#ifndef _SYNTHESIZER_HPP
#define _SYNTHESIZER_HPP

extern "C" {
  typedef int (*yield_callback_t)(void *);
  typedef void (*message_callback_t)(void *, const char *, va_list);
  typedef void (*draw_graph_callback_t)(void *, Curve *);
}

class Synthesizer
{
public:
  Synthesizer();
  ~Synthesizer();
  void set_yield_callback(void *, yield_callback_t);
  void set_message_callback(void *, message_callback_t);
  void set_throw_message_callback(void *, message_callback_t);
  void set_draw_graph_callback(void *, draw_graph_callback_t);
  int perform(int, char **);
  int perform(std::vector<std::string>);
  static std::string get_version();
  static void s_draw_graph(void *, Curve *); // This really should be private.
private:
  void *m_csound;
  void *m_yield_data;
  yield_callback_t m_yield_callback;
  int yield();
  static int s_yield(void *);
  void *m_message_data;
  message_callback_t m_message_callback;
  void message(const char *, va_list);
  static void s_message(void *, const char *, va_list);
  void *m_throw_message_data;
  message_callback_t m_throw_message_callback;
  void throw_message(const char *, va_list);
  static void s_throw_message(void *, const char *, va_list);
  void *m_draw_graph_data;
  draw_graph_callback_t m_draw_graph_callback;
  void draw_graph(Curve *);
};

#endif
