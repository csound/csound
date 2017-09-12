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
#include "CppSound.hpp"
#include "Shell.hpp"
#include "System.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include "csdl.h"

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#endif

namespace csound
{

  void        (*Py_Initialize_)(void) = 0;
  void        (*Py_Finalize_)(void) = 0;
  void        (*PySys_SetArgv_)(int, char **) = 0;
  PyObject_   *(*PyImport_ImportModule_)(char *) = 0;
  void        (*PyErr_Print_)(void) = 0;
  PyObject_   *(*PyObject_GetAttrString_)(PyObject_ *, char *) = 0;
  int         (*PyRun_SimpleFileEx_)(FILE *, const char *, int) = 0;
  int         (*PyRun_SimpleString_)(const char *) = 0;
  PyObject_   *(*PyObject_CallMethod_)(PyObject_ *,
                                       char *, char *, ...) = 0;
  long        (*PyLong_AsLong_)(PyObject_ *) = 0;

  void *Shell::pythonLibrary = 0;
  const char *Shell::pythonLibraryPathList[] = {
#ifdef WIN32
    "python25.dll",
    "python24.dll",
    "python23.dll",
#elif defined(MACOSX)
    // hope that one of these will work
    "/System/Library/Frameworks/Python.Framework/Versions/Current/Python",
    "/System/Library/Frameworks/Python.framework/Versions/Current/Python",
    "/System/Library/Frameworks/Python.Framework/Versions/2.5/Python",
    "/System/Library/Frameworks/Python.framework/Versions/2.5/Python",
    "/System/Library/Frameworks/Python.Framework/Versions/2.4/Python",
    "/System/Library/Frameworks/Python.framework/Versions/2.4/Python",
    "/System/Library/Frameworks/Python.Framework/Versions/2.3/Python",
    "/System/Library/Frameworks/Python.framework/Versions/2.3/Python",
    "/usr/lib/libpython2.5.dylib",
    "/usr/lib/libpython2.4.dylib",
    "/usr/lib/libpython2.3.dylib",
    "/Library/Frameworks/Python.Framework/Versions/Current/Python",
    "/Library/Frameworks/Python.framework/Versions/Current/Python",
    "/Library/Frameworks/Python.Framework/Versions/2.5/Python",
    "/Library/Frameworks/Python.framework/Versions/2.5/Python",
    "/Library/Frameworks/Python.Framework/Versions/2.4/Python",
    "/Library/Frameworks/Python.framework/Versions/2.4/Python",
    "/Library/Frameworks/Python.Framework/Versions/2.3/Python",
    "/Library/Frameworks/Python.framework/Versions/2.3/Python",
    "/usr/local/lib/libpython2.5.dylib",
    "/usr/local/lib/libpython2.4.dylib",
    "/usr/local/lib/libpython2.3.dylib",
#else
    "libpython2.5.so",
    "libpython2.4.so",
    "libpython2.3.so",
#endif
    (char*) 0
  };

  Shell::Shell()
  {
  }

  Shell::~Shell()
  {
  }

  static bool pythonFuncWarning(void **pythonLibrary,
                                const char *funcName)
  {
    csound::System::warn("Failed to find '%s' function. "
                         "Python scripting is not enabled.\n", funcName);
    csoundCloseLibrary(*pythonLibrary);
    *pythonLibrary = (void *) 0;
    return false;
  }

  void Shell::open()
  {
    // nothing to do if already loaded
    if (pythonLibrary) {
      return;
    }
    int     result = CSOUND_ERROR;
#ifdef WIN32
    // avoid pop-up window about missing DLL file
    SetErrorMode((unsigned int) SEM_NOOPENFILEERRORBOX);
#endif
    for (const char **sp = &(pythonLibraryPathList[0]); *sp != (char*) 0; sp++) {
      if ((result = csoundOpenLibrary(&pythonLibrary, *sp)) == CSOUND_SUCCESS) {
        break;
      }
    }
#ifdef WIN32
    SetErrorMode((unsigned int) 0);
#endif
    if (result != CSOUND_SUCCESS) {
      csound::System::warn("Python not found, disabling scripting. "
                           "Check your PATH or Python installation.\n");
      pythonLibrary = (void*) 0;
      return;
    }
    Py_Initialize_ =
      (void (*)(void))
      csoundGetLibrarySymbol(pythonLibrary, "Py_Initialize");
    if (!Py_Initialize_) {
      pythonFuncWarning(&pythonLibrary, "Py_Initialize");
      return;
    }
    Py_Finalize_ =
      (void (*)(void))
      csoundGetLibrarySymbol(pythonLibrary, "Py_Finalize");
    if (!Py_Finalize_) {
      pythonFuncWarning(&pythonLibrary, "Py_Finalize");
      return;
    }
    PySys_SetArgv_ =
      (void (*)(int, char **))
      csoundGetLibrarySymbol(pythonLibrary, "PySys_SetArgv");
    if (!PySys_SetArgv_) {
      pythonFuncWarning(&pythonLibrary, "PySys_SetArgv");
      return;
    }
    PyImport_ImportModule_ =
      (PyObject_* (*)(char *))
      csoundGetLibrarySymbol(pythonLibrary, "PyImport_ImportModule");
    if (!PyImport_ImportModule_) {
      pythonFuncWarning(&pythonLibrary,
                        "PyImport_ImportModule");
      return;
    }
    PyRun_SimpleFileEx_ =
      (int (*)(FILE *, const char *, int))
      csoundGetLibrarySymbol(pythonLibrary, "PyRun_SimpleFileEx");
    if (!PyRun_SimpleFileEx_) {
      pythonFuncWarning(&pythonLibrary, "PyRun_SimpleFileEx");
      return;
    }
    PyRun_SimpleString_ =
      (int (*)(const char *))
      csoundGetLibrarySymbol(pythonLibrary, "PyRun_SimpleString");
    if (!PyRun_SimpleString_) {
      pythonFuncWarning(&pythonLibrary, "PyRun_SimpleString");
      return;
    }
    PyErr_Print_ =
      (void (*)(void))
      csoundGetLibrarySymbol(pythonLibrary, "PyErr_Print");
    if (!PyErr_Print_) {
      pythonFuncWarning(&pythonLibrary, "PyErr_Print");
      return;
    }
    PyObject_GetAttrString_ =
      (PyObject_ *(*)(PyObject_ *, char *))
      csoundGetLibrarySymbol(pythonLibrary, "PyObject_GetAttrString");
    if (!PyObject_GetAttrString_) {
      pythonFuncWarning(&pythonLibrary,
                        "PyObject_GetAttrString");
      return;
    }
    PyObject_CallMethod_ =
      (PyObject_ * (*)(PyObject_ *, char *, char *, ...))
      csoundGetLibrarySymbol(pythonLibrary, "PyObject_CallMethod");
    if (!PyObject_CallMethod_) {
      pythonFuncWarning(&pythonLibrary, "PyObject_CallMethod");
      return;
    }
    PyLong_AsLong_ =
      (long (*)(PyObject_ *))
      csoundGetLibrarySymbol(pythonLibrary, "PyLong_AsLong");
    if (!PyLong_AsLong_) {
      pythonFuncWarning(&pythonLibrary, "PyLong_AsLong");
      return;
    }
    Py_Initialize_();
  }

  void Shell::close()
  {
    // Should be able to Py_Finalize here, but it doesn't work in VST.
  }

  void Shell::main(int argc, char **argv)
  {
    PySys_SetArgv_(argc, argv);
    /* Sanitize sys.path */
    PyRun_SimpleString_("import sys; sys.path = filter(None, sys.path)");
  }

  void Shell::initialize()
  {
    clear();
    setFilename(generateFilename());
  }

  void Shell::clear()
  {
    filename.erase();
    script.erase();
  }

  std::string Shell::generateFilename()
  {
    time_t time_ = 0;
    time(&time_);
    struct tm* tm_ = std::gmtime(&time_);
    char buffer[0x100];
    strftime(buffer, 0x100, "csound.%Y-%m-%d.%H-%M-%S.py", tm_);
    return buffer;
  }

  void Shell::setFilename(std::string filename)
  {
    this->filename = filename;
  }

  std::string Shell::getFilename() const
  {
    return filename;
  }

  std::string Shell::getOutputSoundfileName() const
  {
    std::string outputFilename = getFilename();
    outputFilename.append(".wav");
    return filename;
  }

  std::string Shell::getMidiFilename() const
  {
    std::string midiFilename = getFilename();
    midiFilename.append(".mid");
    return filename;
  }

  std::string Shell::getScript() const
  {
    return script;
  }

  void Shell::setScript(std::string script)
  {
    this->script = script;
  }

  void Shell::load(std::string filename)
  {
    clear();
    loadAppend(filename);
  }

  void Shell::loadAppend(std::string filename)
  {
    std::ifstream stream;
    stream.open(filename.c_str(), std::ios_base::binary);
    char c;
    while(!stream.eof())
      {
        stream.get(c);
        script.push_back(c);
      }
  }

  void Shell::save(std::string filename) const
  {
    std::ofstream stream;
    if(filename.length() > 0)
      {
        stream.open(filename.c_str(), std::ios_base::binary);
        for(std::string::const_iterator it = script.begin(); it != script.end(); ++it)
          {
            stream.put(*it);
          }
      }
  }

  void Shell::save() const
  {
    save(getFilename());
  }

  int Shell::runScript()
  {
    return runScript(script);
  }

  int Shell::runScript(std::string script_)
  {
    csound::System::message("BEGAN Shell::runScript()...\n");
    clock_t began = std::clock();
    int result = 0;
    try
      {
        char *script__ = const_cast<char *>(script_.c_str());
        csound::System::message("==============================================================================================================\n");
        result = PyRun_SimpleString_(script__);
        if(result)
          {
            PyErr_Print_();
          }
      }
    catch(...)
      {
        csound::System::error("Unidentified exception in silence::Shell::run().\n");
      }
    csound::System::message("==============================================================================================================\n");
    clock_t ended = std::clock();
    double elapsed = double(ended - began) / double(CLOCKS_PER_SEC);
    csound::System::message("PyRun_SimpleString returned %d after %.3f seconds.\n", result, elapsed);
    csound::System::message("ENDED Shell::runScript().\n");
    return result;
  }

  void Shell::stop()
  {
    //PyErr_SetString(PyExc_KeyboardInterrupt, "Shell::stop() was called.");
  }
}
