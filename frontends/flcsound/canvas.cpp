/*
 * A canvas on which to draw a curve
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
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "curve.hpp"
#include "canvas.hpp"

Canvas::Canvas(int x, int y, int w, int h, const char *label)
  : Fl_Widget(x, y, w, h, label), m_curve(0)
{
}

void Canvas::set_curve(Curve *curve)
{
  m_curve = curve;
  redraw();
}

const int border = 2 * FL_NORMAL_SIZE;
const int min_ratio = 3;

void Canvas::draw()
{
  draw_box();
  if (!m_curve)
    return;
  float width = w() - 2 * border;
  float height = h() - 2 * border;
  if (height * min_ratio  > width)
    height = width / min_ratio;
  float x_origin = x() + border;
  float y_origin;
  Polarity pol = m_curve->get_polarity();
  switch (pol) {
  case POLARITY_BIPOL:
    y_origin = y() + border + height / 2;
    break;
  case POLARITY_NEGPOL:
    y_origin = y() + border;
    break;
  default:
    y_origin = y() + border + height;
    break;
  }

  float *data = m_curve->get_data();
  size_t size = m_curve->get_size();
  float max = m_curve->get_max();
  float min = m_curve->get_min();
  float y_scale = m_curve->get_y_scale();
  if (max > min && width > 0.0 && height > 0.0) {
    fl_color(0,0,0);
    fl_line_style(FL_SOLID);
    fl_push_clip(x(), y(), w(), h());
    fl_push_matrix();
    fl_translate(x_origin, y_origin);
    fl_scale(width * y_scale / size, -height / (max - min));
    fl_begin_line();
    float x = 0;
    for (size_t i = 0; i < size; i++) {
      fl_vertex(x, data[i]);
      x += 1.0;
    }
    fl_end_line();

    // Draw axis
    fl_begin_line();
    fl_vertex(0.0, 0.0);
    fl_vertex(x, 0.0);
    fl_end_line();
    fl_begin_line();
    fl_vertex(0.0, max);
    fl_vertex(0.0, min);
    fl_end_line();
    fl_pop_matrix();
    fl_pop_clip();
  }
}
