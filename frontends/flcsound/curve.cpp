/*
 * Curve to be drawn
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
#include "curve.hpp"

// Curve is a straightforward abstract data type for a curve

float *Curve::copy(size_t size, float *data)
{
  float *result = new float[size];
  for (size_t i = 0; i < size; i++)
    result[i] = data[i];
  return result;
}

float *Curve::copy(size_t size, double *data)
{
  float *result = new float[size];
  for (size_t i = 0; i < size; i++)
    result[i] = data[i];
  return result;
}

void Curve::destroy()
{
  delete [] m_data;
}

Curve::Curve(float *data, size_t size, const std::string& caption,
             Polarity polarity, float max, float min, float absmax,
             float y_scale, bool dotted_divider)
  : m_caption(caption)
{
  m_data = copy(size, data);
  m_size = size;
  m_polarity = polarity;
  m_max = max;
  m_min = min;
  m_absmax = absmax;
  m_y_scale = y_scale;
  m_dotted_divider = dotted_divider;
}

Curve::Curve(double *data, size_t size, const std::string& caption,
             Polarity polarity, double max, double min, double absmax,
             double y_scale, bool dotted_divider)
  : m_caption(caption)
{
  m_data = copy(size, data);
  m_size = size;
  m_polarity = polarity;
  m_max = max;
  m_min = min;
  m_absmax = absmax;
  m_y_scale = y_scale;
  m_dotted_divider = dotted_divider;
}

Curve::Curve(const Curve& curve)
  : m_caption(curve.m_caption)
{
  m_data = copy(curve.m_size, curve.m_data);
  m_size = curve.m_size;
  m_polarity = curve.m_polarity;
  m_max = curve.m_max;
  m_min = curve.m_min;
  m_absmax = curve.m_absmax;
  m_y_scale = curve.m_y_scale;
  m_dotted_divider = curve.m_dotted_divider;
}

Curve& Curve::operator=(const Curve& curve)
{
  if (this != &curve) {
    destroy();
    m_data = copy(curve.m_size, curve.m_data);
    m_size = curve.m_size;
    m_caption = curve.m_caption;
    m_polarity = curve.m_polarity;
    m_max = curve.m_max;
    m_min = curve.m_min;
    m_absmax = curve.m_absmax;
    m_y_scale = curve.m_y_scale;
    m_dotted_divider = curve.m_dotted_divider;
  }
  return *this;
}

Curve::~Curve()
{
  destroy();
}

size_t Curve::get_size() const
{
  return m_size;
}

float *Curve::get_data() const
{
  return m_data;
}

std::string Curve::get_caption() const
{
  return m_caption;
}

Polarity Curve::get_polarity() const
{
  return m_polarity;
}

float Curve::get_max() const
{
  return m_max;
}

float Curve::get_min() const
{
  return m_min;
}

float Curve::get_absmax() const
{
  return m_absmax;
}

float Curve::get_y_scale() const
{
  return m_y_scale;
}

bool Curve::is_divider_dotted() const
{
  return m_dotted_divider;
}

bool Curve::has_same_caption(Curve *curve) const
{
  return curve && m_caption == curve->m_caption;
}
