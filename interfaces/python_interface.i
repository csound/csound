/*
* C S O U N D
*
* External language interfaces for the "C" and "C++" Csound API.
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
 
#ifndef MACOSX
%module(directors="1") csnd
%feature("director") CsoundCallbackWrapper;
%feature("nodirector") Csound;
#else /* fix for OSX */
%module csnd
#endif

%include "typemaps.i"

%include "std_string.i"
%include "std_vector.i"
%include "carrays.i"
%array_functions(int, intp);
%array_functions(float, floatp);
%array_functions(double, doublep);

%array_class(int, intArray);
%array_class(float, floatArray);
%array_class(double, doubleArray)

%feature("autodoc", "1");
%{
    #include "csound.h"
    #include "cfgvar.h"
    #include "csound.hpp"
    #include "cs_glue.hpp"
    #include "csPerfThread.hpp"
    #include "CsoundFile.hpp"
    #include "CppSound.hpp"
    #include "filebuilding.h"
    #include "Soundfile.hpp"
%}

%apply int { size_t };
typedef unsigned int uint32_t;
#ifndef MSVC
%apply long long { uint32_t };
#endif

/* %typemap(in) char ** { */
  /* Check if is a list */
/* if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
        $1[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
        PyErr_SetString(PyExc_TypeError,"list must contain strings");
        free($1);
        return NULL;
      }
    }
    $1[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(freearg) char ** {
  free((char *) $1);
} */

%include "exclusions.i"
%ignore  csoundSetCscoreCallback(CSOUND *,void (*cscoreCallback_)(CSOUND *));

// typemap for callbacks
%typemap(in) PyObject *pyfunc {
  if(!PyCallable_Check($input)){
    PyErr_SetString(PyExc_TypeError, "Not a callable object!");
    return NULL;
}
$1 = $input;
}

%{
// this will be used as an interface to the
// message callback
static void PythonMessageCallback(CSOUND *in, int attr,
                                     const char *format, va_list valist){

    PyObject *res;
    int i=0;
    CSOUND *p = in; 
    PyThreadState **tstate = (PyThreadState **) csoundQueryGlobalVariable(p,"::tstate");
    PyObject **pyfunc = (PyObject **) csoundQueryGlobalVariable(p,"::pyfunc"), *arg;
    
    char *mbuf = new char[strlen(format)*10];
    if(*tstate == NULL)
        *tstate = PyThreadState_New(PyInterpreterState_New()); 
    PyEval_AcquireThread(*tstate); 
    vsprintf(mbuf, format, valist);
    // attempt to remove some of the newlines
    while(mbuf[i] != '\0') i++; mbuf[i-1] = '\0';
    arg = Py_BuildValue("(s)", mbuf); 
    res =  PyEval_CallObject(*pyfunc, arg);
    if (res == NULL){
     PyErr_SetString(PyExc_TypeError, "Exception in callback");
    }
    else Py_DECREF(res);    

   PyEval_ReleaseThread(*tstate);
   delete[] mbuf;
}
%}

%include "csound.h"
%include "cfgvar.h"

%apply MYFLT &OUTPUT { MYFLT &dflt, MYFLT &min, MYFLT &max };
%apply MYFLT &OUTPUT { MYFLT &value };

%ignore Csound::SetCscoreCallback(void (*cscoreCallback_)(CSOUND *));
%include "csound.hpp"
%extend Csound {
   void SetPythonMessageCallback(PyObject *pyfunc){
     // thread safety mechanism   
     PyEval_InitThreads();
     PyObject **p, *arg;
     PyThreadState **tstate;
     // Py_XDECREF(pyfunc);  
     self->CreateGlobalVariable("::pyfunc", sizeof(PyObject *));
     self->CreateGlobalVariable("::tstate", sizeof(PyThreadState *));
     p = (PyObject **) self->QueryGlobalVariable("::pyfunc");
     *p = pyfunc;
     arg = Py_BuildValue("(s)", "Message Callback set\n"); 
     PyEval_CallObject(pyfunc, arg);
     tstate = (PyThreadState **) self->QueryGlobalVariable("::tstate");
     *tstate = NULL;     
     self->SetMessageCallback(PythonMessageCallback);
     Py_XINCREF(pyfunc); 
}
}

%clear MYFLT &dflt;
%clear MYFLT &min;
%clear MYFLT &max;
%clear MYFLT &value;


%{
// this will be used as an interface to the
// callback
static void PythonCallback(void *p){

    PyObject *res;
    CsoundPerformanceThread *t = (CsoundPerformanceThread *) p;
    if(t->_tstate == NULL)
        t->_tstate = PyThreadState_New(PyInterpreterState_New()); 
    PyEval_AcquireThread(t->_tstate);    
    res = PyEval_CallObject(t->pydata.func, t->pydata.data);
    if (res == NULL){
    PyErr_SetString(PyExc_TypeError, "Exception in callback");
     }
    else Py_DECREF(res);    
   PyEval_ReleaseThread(t->_tstate);
}
%}


%ignore CsoundPerformanceThread::SetProcessCallback(void (*Callback)(void *), void *cbdata);

%include "cs_glue.hpp"
%include "csPerfThread.hpp"

%extend CsoundPerformanceThread {
   // Set the Python callback
   void SetProcessCallback(PyObject *pyfunc, PyObject *p){
    if(self->GetProcessCallback() == NULL) {
       PyEval_InitThreads();
       self->_tstate = NULL;
     }
     else Py_XDECREF(self->pydata.func);  
    self->pydata.func = pyfunc;
    self->pydata.data = Py_BuildValue("(O)", p);
    self->SetProcessCallback(PythonCallback, (void *)self);
    Py_XINCREF(pyfunc);

  }
}

//#ifndef MACOSX
%include "CsoundFile.hpp"
//#endif
%include "CppSound.hpp"
/*
%include "filebuilding.h"
%include "Soundfile.hpp"
*/
