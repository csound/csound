/*
 * C S O U N D
 *
 * L I C E N S E
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
#ifndef EXCEPTION_H
#define EXCEPTION_H
#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include <string>
%}
#else
#include <string>
#endif

namespace csound
{
  /**
   * Base class for C++ exceptions in the Silence system.
   */
  class SILENCE_PUBLIC Exception
  {
    std::string message;
  public:
    Exception(std::string message_) :
      message(message_)
      {
      }
    virtual ~Exception()
    {
    }
    std::string getMessage() const
    {
      return message;
    }
  };
}

#endif
