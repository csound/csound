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

#ifndef _CURVE_HPP
#define _CURVE_HPP

enum Polarity {
  POLARITY_NOPOL,
  POLARITY_NEGPOL,
  POLARITY_POSPOL,
  POLARITY_BIPOL,
};

class Curve
{
public:
  Curve(float *, size_t, const std::string&, Polarity,
	float, float, float, float, bool);
  Curve(double *, size_t, const std::string&, Polarity,
	double, double, double, double, bool);
  Curve(const Curve&);
  Curve &operator=(const Curve&);
  ~Curve();
  float *get_data() const;
  size_t get_size() const;	// number of points
  std::string get_caption() const; // title of curve
  Polarity get_polarity() const; // polarity
  float get_max() const;	// curve max
  float get_min() const;	// curve min
  float get_absmax() const;	// abs max of above
  float get_y_scale() const;	// Y axis scaling factor
  bool is_divider_dotted() const; // Add dotted divider when true
  bool has_same_caption(Curve *) const;
private:
  float *m_data;
  size_t m_size;
  std::string m_caption;
  Polarity m_polarity;
  float m_max, m_min, m_absmax, m_y_scale;
  bool m_dotted_divider;
  static float *copy(size_t, float *);
  static float *copy(size_t, double *);
  void destroy();
};

#endif
