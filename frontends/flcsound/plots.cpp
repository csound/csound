/*
 * Display a choice of curves
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

#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>
#include "curve.hpp"
#include "canvas.hpp"
#include "plots.hpp"

Plots::Plots(int x, int y, int w, int h, const char *label)
  : Fl_Group(x, y, w, h, label)
{
  int s = FL_NORMAL_SIZE * 2;
  if (s >= h)
    s = h/2;
  m_choice = new Fl_Choice(x, y, w, s, "Choice");
  m_canvas = new Canvas(x, y + s, w, h - s, "Canvas");
  m_canvas->labeltype(FL_NO_LABEL);
  end();
  resizable(m_canvas);
  m_selection = m_choice->value();
  for (int i = 0; i < menu_size; i++) {
    m_menu_items[i].text = 0;
    m_menu_items[i].shortcut_ = 0;
    m_menu_items[i].callback_ = choice_callback;
    m_menu_items[i].user_data_ = this;
    m_menu_items[i].flags = 0;
    m_menu_items[i].labeltype_ = 0;
    m_menu_items[i].labelfont_ = 0;
    m_menu_items[i].labelsize_ = 0;
    m_menu_items[i].labelcolor_ = 0;
    m_curves[i] = 0;
  }
  m_menu_items[menu_size].text = 0;
}

void Plots::append(Curve *curve)
{
  int i;
  std::string caption = curve->get_caption();
  for (i = 0; m_menu_items[i].text; i++) {
    if (caption == m_menu_items[i].text) {
      if (m_selection == i)
	m_canvas->set_curve(curve);
      delete m_curves[i];
      m_curves[i] = curve;
      return;
    }
  }
  if (i >= menu_size) {		// Just drop it.
    delete curve;
    return;
  }
  m_curves[i] = curve;
  size_t n = caption.length() + 1;
  char *p = new char(n + 1);
  p[caption.copy(p, n, 0)] = 0;
  m_menu_items[i].text = p;
  m_choice->menu(m_menu_items);
  if (i == 0)
    m_canvas->set_curve(curve);
}

void Plots::clear()
{
  m_canvas->set_curve(0);
  m_choice->clear();
  m_selection = m_choice->value();
  for (int i = 0; m_menu_items[i].text; i++) {
    delete m_menu_items[i].text;
    m_menu_items[i].text = 0;
    delete m_curves[i];
    m_curves[i] = 0;
  }
}

void Plots::choice_callback(Fl_Widget *w, void *data)
{
  ((Plots *)data)->choice(w);
}

void Plots::choice(Fl_Widget *)
{
  m_selection = m_choice->value();
  m_canvas->set_curve(m_curves[m_selection]);
}
