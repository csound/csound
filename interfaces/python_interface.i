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
%module(directors="1") csnd6
%feature("director") CsoundCallbackWrapper;
%feature("nodirector") Csound;
#else /* fix for OSX */
/* MODULE name is now csnd6 */
%module csnd6
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
    #include <cmath>
    #include <cstddef>
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
%typemap(in) PyObject

*pyfunc {
  if($input != Py_None)
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
    Csound *p = (Csound *) csoundGetHostData(in);
    pycbdata *pydata = (pycbdata *)p->pydata;
    PyObject *pyfunc = pydata->mfunc, *arg;
    char *mbuf = new char[sizeof(format)*10 + 256];
    vsprintf(mbuf, format, valist);
    //if(ch = strrchr(mbuf, '\n')) *ch = '\0';
   if (strlen(mbuf) > 1){
#ifndef PYTHON_23_or_older
       if(!PyEval_ThreadsInitialized())
#endif
	PyEval_InitThreads();
    PyGILState_STATE gst;
    // printf("MESS BEFORE \n");
    gst = PyGILState_Ensure();
    arg = Py_BuildValue("(s)", mbuf);
    res =  PyEval_CallObject(pyfunc, arg);
    if (res == NULL){
       PyErr_SetString(PyExc_TypeError, "Exception in callback");
    }else Py_DECREF(res);
   // printf("Mes: %s \n", mbuf);
   PyGILState_Release(gst);
   // printf("MESS OVER \n");
}
    delete[] mbuf;
}
static void VoidMessageCallback(CSOUND *in, int attr,
                                     const char *format, va_list valist){
//printf("void message callback\n");
}


 static void PythonInChannelCallback(CSOUND *in, const char *chan, void *v,
                                       const void *channelType){

    PyObject *res;
    Csound *p = (Csound *) csoundGetHostData(in);
    pycbdata *pydata = (pycbdata *)p->pydata;
    PyObject *pyfunc = pydata->invalfunc, *arg;
    PyGILState_STATE gst;
    MYFLT *val = (MYFLT *) v;
    gst = PyGILState_Ensure();
    arg = Py_BuildValue("(s)", chan);
    res =  PyEval_CallObject(pyfunc, arg);
    if (res == NULL){
       PyErr_SetString(PyExc_TypeError, "Exception in callback");
    }else{
      if(PyFloat_Check(res)) *val = (MYFLT) PyFloat_AsDouble(res);
     else *val = 0.0;
     Py_DECREF(res);
    }
   PyGILState_Release(gst);


}

 static void PythonOutChannelCallback(CSOUND *in, const char *chan, void *v,
                                                       const void *channelType) {

    PyObject *res;
    Csound *p = (Csound *) csoundGetHostData(in);
    pycbdata *pydata = (pycbdata *)p->pydata;
    PyObject *pyfunc = pydata->outvalfunc, *arg;
    PyGILState_STATE gst;
    MYFLT *val = (MYFLT *) v;
    gst = PyGILState_Ensure();
    arg = Py_BuildValue("(s,d)", chan, (double) *val);
    res =  PyEval_CallObject(pyfunc, arg);
    if (res == NULL){
       PyErr_SetString(PyExc_TypeError, "Exception in callback");
    }else Py_DECREF(res);
   PyGILState_Release(gst);

}

static int PythonMidiInOpen(CSOUND *in, void **udata, const char *name){

    PyObject *res;
    Csound *p = (Csound *) csoundGetHostData(in);
    pycbdata *pydata = (pycbdata *)p->pydata;
    PyObject *pyfunc = pydata->midiinopenfunc, *arg;
    PyGILState_STATE gst;
    gst = PyGILState_Ensure();
    arg = Py_BuildValue("(s)", name);
    res =  PyEval_CallObject(pyfunc, arg);
    if (res == NULL){
       PyErr_SetString(PyExc_TypeError, "Exception in callback");
    }else {
       *udata = (void *) res;
    }
    PyGILState_Release(gst);
    return 0;
}

static int PythonMidiInClose(CSOUND *in, void *udata){
    PyObject *res;
    Csound *p = (Csound *) csoundGetHostData(in);
    pycbdata *pydata = (pycbdata *)p->pydata;
    PyObject *pyfunc = pydata->midiinclosefunc, *arg;
    PyGILState_STATE gst;
    gst = PyGILState_Ensure();
    arg = Py_BuildValue("(O)", (PyObject *) udata);
    res =  PyEval_CallObject(pyfunc, arg);
    if (res == NULL){
       PyErr_SetString(PyExc_TypeError, "Exception in callback");
    }else  Py_DECREF(res);
    Py_DECREF((PyObject *) udata);
    PyGILState_Release(gst);
    return 0;
}

static int PythonMidiRead(CSOUND *in, void *udata,
                                  unsigned char *buf, int nBytes){
    PyObject *res;
    int i;
    Csound *p = (Csound *) csoundGetHostData(in);
    pycbdata *pydata = (pycbdata *)p->pydata;
    PyObject *pyfunc = pydata->midireadfunc, *arg;
    PyGILState_STATE gst;
    gst = PyGILState_Ensure();
    arg = Py_BuildValue("(O,i)", (PyObject *) udata, nBytes);
    res =  PyEval_CallObject(pyfunc, arg);
    if (res == NULL){
       PyErr_SetString(PyExc_TypeError, "Exception in callback");
    }else {
    if(PyList_Check(res))
       for(i=0; i < nBytes; i++)
          buf[i] = (char) PyInt_AsLong(PyList_GetItem(res,i));
     else for(i=0; i < nBytes; i++)  buf[i] = 0;
     Py_DECREF(res);
     }
    PyGILState_Release(gst);
    return 0;
}

static void pythonMessageCallback(CSOUND *csound,
                                  int attr, const char *format, va_list valist)
{
  char          buffer[8192];
  static std::string  lineBuffer = "print '''";     // FIXME
  unsigned int  i, len;
#ifdef HAVE_C99
  len = (unsigned int) vsnprintf(&(buffer[0]), (size_t) 8192, format, valist);
  if (len >= 8192U)
    {
      PyRun_SimpleString("print '''Error: message buffer overflow'''");
      return;
    }
#else
  len = (unsigned int) vsprintf(&(buffer[0]), format, valist);
  if (len >= 8192U)
    {
      PyRun_SimpleString("print '''Error: message buffer overflow'''");
      exit(-1);
    }
#endif
  for (i = 0; i < len; i++) {
    if (buffer[i] == '\n') {
      lineBuffer += "'''";
      PyRun_SimpleString(lineBuffer.c_str());
      lineBuffer = "print '''";
      continue;
    }
    if (buffer[i] == '\'' || buffer[i] == '\\')
      lineBuffer += '\\';
    lineBuffer += buffer[i];
  }
}

%}

%ignore csoundSetHostData(CSOUND *, void *);
%ignore csoundGetHostData(CSOUND *);
%include "csound.h"
%include "cfgvar.h"
%apply MYFLT &OUTPUT { MYFLT &dflt, MYFLT &min, MYFLT &max };
%apply MYFLT &OUTPUT { MYFLT &value };


%ignore Csound::SetCscoreCallback(void (*cscoreCallback_)(CSOUND *));
%ignore Csound::SetOutputChannelCallback(channelCallback_t inputChannelCalback);
%ignore Csound::SetInputChannelCallback(channelCallback_t inputChannelCalback);
%ignore Csound::SetExternalMidiInOpenCallback(int (*)(CSOUND *, void *, const char*));
%ignore Csound::SetExternalMidiReadCallback(int (*)(CSOUND *, void *, unsigned char *, int));
%ignore Csound::SetExternalMidiInCloseCallback(int (*)(CSOUND *, void *));

%ignore Csound::SetHostData(void *);
%ignore Csound::GetHostData();
%ignore Csound::SetMessageCallback(void (*)(CSOUND *, int attr,const char *format, va_list valist));
%include "csound.hpp"

%extend Csound {
  void SetHostData(PyObject *data){
   ((pycbdata *)self->pydata)->hostdata = data;
}
  PyObject *GetHostData() {
   return ((pycbdata *)self->pydata)->hostdata;
}
  //#ifndef PYTHON_23_or_older
  void SetMessageCallback(PyObject *pyfunc){
     // thread safety mechanism
    if (pyfunc == Py_None){
       Py_XINCREF(pyfunc);
       self->SetMessageCallback(VoidMessageCallback);
       return;
      }

    pycbdata *pydata = (pycbdata *) self->pydata;
    if(pydata->mfunc == NULL) {
        if(!PyEval_ThreadsInitialized())  PyEval_InitThreads();
    }
    else Py_XDECREF(pydata->mfunc);
        pydata->mfunc = pyfunc;
        self->SetMessageCallback(PythonMessageCallback);
        Py_XINCREF(pyfunc);
}

   void SetInputChannelCallback(PyObject *pyfunc){
     // thread safety mechanism
    pycbdata *pydata = (pycbdata *) self->pydata;
    if(pydata->invalfunc == NULL) {
       if(!PyEval_ThreadsInitialized()) PyEval_InitThreads();
    }
    else Py_XDECREF(pydata->invalfunc);
        pydata->invalfunc = pyfunc;
        self->SetInputChannelCallback(PythonInChannelCallback);
        Py_XINCREF(pyfunc);
	}

   void SetOutputChannelCallback(PyObject *pyfunc){
     // thread safety mechanism
    pycbdata *pydata = (pycbdata *) self->pydata;
    if(pydata->outvalfunc == NULL){
      if(!PyEval_ThreadsInitialized()) PyEval_InitThreads();
    }
    else Py_XDECREF(pydata->outvalfunc);

        pydata->outvalfunc = pyfunc;
        self->SetOutputChannelCallback(PythonOutChannelCallback);
        Py_XINCREF(pyfunc);
}
void SetExternalMidiInOpenCallback(PyObject *pyfunc){
     // thread safety mechanism
    pycbdata *pydata = (pycbdata *) self->pydata;
    if(pydata->midiinopenfunc == NULL) {
        if(!PyEval_ThreadsInitialized()) PyEval_InitThreads();
    }
    else Py_XDECREF(pydata->midiinopenfunc);
        pydata->midiinopenfunc = pyfunc;
        self->SetExternalMidiInOpenCallback(PythonMidiInOpen);
        Py_XINCREF(pyfunc);
}

void SetExternalMidiInCloseCallback(PyObject *pyfunc){
     // thread safety mechanism
    pycbdata *pydata = (pycbdata *) self->pydata;
    if(pydata->midiinclosefunc == NULL) {
        if(!PyEval_ThreadsInitialized()) PyEval_InitThreads();
    }
    else Py_XDECREF(pydata->midiinclosefunc);
        pydata->midiinopenfunc = pyfunc;
        self->SetExternalMidiInCloseCallback(PythonMidiInClose);
        Py_XINCREF(pyfunc);
}


void SetExternalMidiReadCallback(PyObject *pyfunc){
     // thread safety mechanism
    pycbdata *pydata = (pycbdata *) self->pydata;
    if(pydata->midireadfunc == NULL) {
        if(!PyEval_ThreadsInitialized()) PyEval_InitThreads();
    }
    else Py_XDECREF(pydata->midireadfunc);
        pydata->midiinopenfunc = pyfunc;
        self->SetExternalMidiReadCallback(PythonMidiRead);
        Py_XINCREF(pyfunc);
}
//#endif
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
    PyGILState_STATE stat;
    CsoundPerformanceThread *t = (CsoundPerformanceThread *) p;
    stat = PyGILState_Ensure();
    res = PyEval_CallObject(t->pydata.func, t->pydata.data);
    if (res == NULL){
    PyErr_SetString(PyExc_TypeError, "Exception in callback");
     }
    else Py_DECREF(res);
    PyGILState_Release(stat);
}
%}


%ignore CsoundPerformanceThread::SetProcessCallback(void (*Callback)(void *), void *cbdata);

%include "cs_glue.hpp"
%include "csPerfThread.hpp"

%extend CsoundPerformanceThread {
   // Set the Python callback
   void SetProcessCallback(PyObject *pyfunc, PyObject *p){
    if(self->GetProcessCallback() == NULL) {
#ifndef PYTHON_23_or_older
       if(!PyEval_ThreadsInitialized())
#endif
                          PyEval_InitThreads();
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


%extend CppSound {
  void setPythonMessageCallback()
  {
    self->SetMessageCallback(pythonMessageCallback);
  }

 
};
