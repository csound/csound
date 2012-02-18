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
#ifndef SHELL_H
#define SHELL_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%include "std_string.i"
%{
#include <string>
  %}
#else
#include <string>
#endif

namespace csound
{
  typedef void PyObject_;
  extern void        (*Py_Initialize_)(void);
  extern void        (*Py_Finalize_)(void);
  extern void        (*PySys_SetArgv_)(int, char **);
  extern PyObject_   *(*PyImport_ImportModule_)(char *);
  extern void        (*PyErr_Print_)(void);
  extern PyObject_   *(*PyObject_GetAttrString_)(PyObject_ *, char *);
  extern int         (*PyRun_SimpleFileEx_)(FILE *, const char *, int);
  extern int         (*PyRun_SimpleString_)(const char *);
  extern PyObject_   *(*PyObject_CallMethod_)(PyObject_ *,
                                       char *, char *, ...);
  extern long        (*PyLong_AsLong_)(PyObject_ *);

  /**
   * Provide a shell in which Python scripts
   * can be loaded, saved, and executed.
   * The Python library and API are dynamically
   * loaded and do not reference Python.h,
   * so if Python is not present, this module
   * will still link and load, but not function.
   */
  class SILENCE_PUBLIC Shell
  {
  protected:
    static void *pythonLibrary;
    static const char *pythonLibraryPathList[];
    std::string filename;
    std::string script;
  public:
    Shell();
    virtual ~Shell();
    virtual void open();
    virtual void close();
    virtual void main(int argc, char **argv);
    virtual void initialize();
    virtual void clear();
    static std::string generateFilename();
    virtual void setFilename(std::string filename);
    virtual std::string getFilename() const;
    virtual std::string getOutputSoundfileName() const;
    virtual std::string getMidiFilename() const;
    virtual std::string getScript() const;
    virtual void setScript(std::string text);
    virtual void load(std::string filename);
    virtual void loadAppend(std::string filename);
    virtual void save(std::string filename) const;
    virtual void save() const;
    virtual int runScript();
    virtual int runScript(std::string script);
    virtual void stop();
  };
}
#endif
