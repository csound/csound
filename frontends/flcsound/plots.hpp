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

#ifndef _PLOTS_HPP
#define _PLOTS_HPP

class Plots : public Fl_Group
{
public:
  Plots(int x, int y, int w, int h, const char *label = 0);
  void append(Curve *);
  void clear();
private:
  static const int menu_size = 50;
  Fl_Menu_Item m_menu_items[menu_size + 1];
  Curve *m_curves[menu_size];
  Fl_Choice *m_choice;
  Canvas *m_canvas;
  int m_selection;
  static void choice_callback(Fl_Widget *, void *);
  void choice(Fl_Widget *);
};

#endif
