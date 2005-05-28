/*
  * csoundapi.h
  *
  * Copyright (C) 2002 Maurizio Umberto Puxeddu
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
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
#ifdef EMBEDDED_PYTHON
#include <Python.h>
#define FUCK_YOU_AND_YOUR_FUCKING_MACROS
#include "csdl.h"
#include "schedule.h"

//extern GLOBALS glob;

static PyObject *self = 0;

/* DATA TYPES */

staticforward PyTypeObject csound_FTableType;

typedef struct {
  PyObject_HEAD
  int ftnum;
} csound_FTableObject;

csound_FTableObject *
_csound_ftable_create(int ftnum)
{
  csound_FTableObject *ftable;
  ftable = PyObject_New(csound_FTableObject, &csound_FTableType);
  ftable->ftnum = ftnum;
  return ftable;
}

static PyObject*
csound_new_ftable(PyObject *self, PyObject *args)
{
  int ftnum;

  if (!PyArg_ParseTuple(args, "i:new_ftable", &ftnum))
    return NULL;

  return (PyObject*)_csound_ftable_create(ftnum);
}

static PyObject *
csound_ftable_int(csound_FTableObject *self)
{
  return Py_BuildValue("i", self->ftnum);
}

static PyObject *
csound_ftable_float(csound_FTableObject *self)
{
  return Py_BuildValue("f", (double)self->ftnum);
}

static void
csound_ftable_dealloc(PyObject *self)
{
  PyObject_Del(self);
}

static PyMethodDef csound_ftable_methods[] = {
  //{"__int__", (PyCFunction)csound_ftable___int__, METH_VARARGS, "Convert to integer"},
  {NULL, NULL}           /* sentinel */
};

static PyObject *
csound_ftable_getattr(csound_FTableObject *self, char *name)
{
  return Py_FindMethod(csound_ftable_methods, (PyObject *)self, name);
}

static PyNumberMethods csound_FTableType_as_number = {
  0,
  0,
  0,
  0,
  0,

  0,
  0,
  0,
  0,
  0,

  0,
  0,
  0,
  0,
  0,

  0,
  0,
  0,
  (unaryfunc)csound_ftable_int,
  0,
  (unaryfunc)csound_ftable_float,
  0,
  0,

  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,

  0,
  0,
  0,
  0
};

extern FUNC **flist;

static PyObject *
csound_ftable_item(csound_FTableObject *self, int index)
{
  FUNC *ftp = flist[self->ftnum];

  if (ftp == NULL)
    return NULL;

  if (index >= ftp->flen)
    return NULL;

  return Py_BuildValue("f", ftp->ftable[index]);
}

static PyObject *
csound_ftable_slice(csound_FTableObject *self, int begin, int end)
{
  PyObject *l;
  int i, n;
  FUNC *ftp = flist[self->ftnum];

  if (ftp == NULL)
    {
      printf("ftable slice: INVALID TABLE NO\n");
      return NULL;
    }

  if (end > ftp->flen)
    end = ftp->flen;

  if (begin > ftp->flen)
    begin = ftp->flen;

  if (end < 0)
    end = ftp->flen - end;

  if (begin < 0)
    begin = ftp->flen - begin;

  if (end < begin)
    {
      printf("ftable slice: INVALID RANGE %d %d\n", begin, end);
      return NULL;
    }

  n = end - begin;
  l = PyList_New(n);
  for (i = 0; i < n; ++i)
    PyList_SetItem(l, i, Py_BuildValue("f", ftp->ftable[begin + i]));

  return l;
}

static int
csound_ftable_item_assign(csound_FTableObject *self, int index, PyObject *value)
{
  FUNC *ftp = flist[self->ftnum];

  if (ftp == NULL)
    return 1;

  if (index >= ftp->flen)
    return 1;

  if (!PyFloat_Check(value))
    return 1;

  ftp->ftable[index] = (MYFLT)PyFloat_AsDouble(value);

  return 0;
}

static int
csound_ftable_slice_assign(csound_FTableObject *self, int begin, int end, PyObject *value)
{
  int i, n;
  FUNC *ftp = flist[self->ftnum];
  PyObject *s;

  if (ftp == NULL)
    return 1;

  if (end > ftp->flen)
    end = ftp->flen;

  if (begin > ftp->flen)
    begin = ftp->flen;

  if (end < 0)
    end = ftp->flen - end;

  if (begin < 0)
    begin = ftp->flen - begin;

  if (end < begin)
    {
      printf("ftable slice: INVALID RANGE %d %d\n", begin, end);
      return 1;
    }

  n = end - begin;

  if (!PySequence_Check(value))
    return 1;

  s = PySequence_Fast(value, "NOT A SEQUENCE");

  for (i = 0; i < n; ++i)
    ftp->ftable[begin + i] = (MYFLT)PyFloat_AsDouble(PySequence_Fast_GET_ITEM(value, i));

  return 0;
}

static int
csound_ftable_length(csound_FTableObject *self)
{
  extern FUNC    **flist;
  FUNC *ftp = flist[self->ftnum];

  if (ftp)
    return ftp->flen;

  return -1;
}

static PySequenceMethods csound_FTableType_as_sequence = {
  (inquiry)csound_ftable_length,
  0,
  0,
  (intargfunc)csound_ftable_item,
  (intintargfunc)csound_ftable_slice,
  (intobjargproc)csound_ftable_item_assign,
  (intintobjargproc)csound_ftable_slice_assign,
  0,
  0,
  0
};

static PyTypeObject csound_FTableType = {
  PyObject_HEAD_INIT(NULL)
  0,
  "ftable",
  sizeof(csound_FTableObject),
  0,
  (destructor)csound_ftable_dealloc, /*tp_dealloc*/
  0,          /*tp_print*/
  (getattrfunc)csound_ftable_getattr,          /*tp_getattr*/
  0,          /*tp_setattr*/
  0,          /*tp_compare*/
  0,          /*tp_repr*/
  &csound_FTableType_as_number,          /*tp_as_number*/
  &csound_FTableType_as_sequence,          /*tp_as_sequence*/
  0,          /*tp_as_mapping*/
  0,          /*tp_hash */
};

/* MODULE METHODS */

extern char    *orchname;
extern char        *scorename;
extern long    kcounter;

static PyObject *
csound_orchestra(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ":orchestra"))
    return NULL;
  return Py_BuildValue("s", /*glob.*/orchname);
}

static PyObject *
csound_score(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ":score"))
    return NULL;
  return Py_BuildValue("s", /* glob. */ scorename);
}

static PyObject *
csound_kcycle(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ":kcycle"))
    return NULL;
  return Py_BuildValue("i", /* glob. */ kcounter);
}

static PyObject *
csound_time(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ":time"))
    return NULL;
  return Py_BuildValue("f", /* glob. */ kcounter * /* glob. */ onedkr);
}

#define FZERO (FL(0.0))    /* (Shouldn't there be global decl's for these?) */

static PyObject *
csound_ievent(PyObject *self, PyObject *args)
{
  int i, narg = PyTuple_Size(args);
  EVTBLK newevt;

  if (narg < 3)
    return NULL;

  newevt.strarg = NULL;
  newevt.opcod = 'i';
  newevt.pcnt = narg;
  newevt.p[1] = (MYFLT) PyFloat_AsDouble(PyTuple_GET_ITEM(args, 0));
  newevt.p[2] = (MYFLT) PyFloat_AsDouble(PyTuple_GET_ITEM(args, 1));
  newevt.p[3] = (MYFLT) PyFloat_AsDouble(PyTuple_GET_ITEM(args, 2));
  for (i = 0; i < narg - 3; i++)
    newevt.p[i + 4] = (MYFLT) PyFloat_AsDouble(PyTuple_GET_ITEM(args, i + 3));
  /* Set start time from kwhen */
  if (newevt.p[2] < FL(0.0)) {
    newevt.p[2] = FL(0.0);
    csoundWarning(&cenviron,
                  Str("queue_ievent warning: negative onset reset to zero"));
  }
  insert_score_event(&cenviron, &newevt,
                     cenviron.sensEvents_state.curTime, 0);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
csound_eevent(PyObject *self, PyObject *args)
{
  double onset;
  EVTBLK newevt;

  if (!PyArg_ParseTuple(args, "d:eevent", &PyList_Type, &onset))
    return NULL;

  onset = PyFloat_AsDouble(PyTuple_GET_ITEM(args, 0));
  newevt.strarg = NULL;
  newevt.opcod = 'e';
  newevt.pcnt = 0;
  insert_score_event(&cenviron, &newevt,
                     cenviron.sensEvents_state.curTime + onset, 0);
  Py_INCREF(Py_None);
  return Py_None;
}

extern FUNC *hfgens(EVTBLK *evtblkp);

static PyObject *
csound_ftable(PyObject *self, PyObject *args)
{
  MYFLT what, size, fgen;
  int i, narg = PyTuple_Size(args);
  MYFLT starttime;
  EVTBLK newevt;
  FUNC *ftp;

  if (narg < 3)
    {
      PyErr_SetString(PyExc_ValueError, "too few arguments");
      return NULL;
    }

  what = (MYFLT)PyFloat_AsDouble(PyTuple_GET_ITEM(args, 0));
  size = (MYFLT)PyFloat_AsDouble(PyTuple_GET_ITEM(args, 1));
  fgen = (MYFLT)PyFloat_AsDouble(PyTuple_GET_ITEM(args, 2));

  newevt.opcod = 'f';
  /* Set start time from kwhen */
  starttime = FZERO;

  /* Add current time (see note about kadjust in triginset() above) */
  newevt.pcnt = (short)(starttime * /* glob. */ ekr + FL(0.5));
  newevt.p2orig = starttime;
  newevt.p3orig = size;
  /* Copy all arguments to the new event */
  newevt.pcnt = narg + 2;
  for (i = 0; i < narg-3; i++)
    newevt.p[i+5] = (MYFLT)PyFloat_AsDouble(PyTuple_GET_ITEM(args, i + 3));
  newevt.p[4] = (short)fgen;
  newevt.p[3] = (int)size;
  newevt.p[2] = starttime;    /* Set actual start time in p2 */
  newevt.p[1] = what;

  ftp = hfgens(&newevt);

  return (PyObject*)_csound_ftable_create(ftp->fno);
}

static PyObject *
csound_stdout_write(PyObject *self, PyObject *args)
{
  char *data;
  int size, result;

  if (!PyArg_ParseTuple(args, "s#:stdout_write", &data, &size))
    return NULL;

  result = python_stdout_write(data, size);

  return Py_BuildValue("i", result);
}

static PyObject *
csound_stderr_write(PyObject *self, PyObject *args)
{
  char *data;
  int size, result;

  if (!PyArg_ParseTuple(args, "s#:stderr_write", &data, &size))
    return NULL;

  result = python_stderr_write(data, size);

  return Py_BuildValue("i", result);
}

/* CSOUND MODULE */

static PyMethodDef csound_methods[] = {
  {"orchestra", csound_orchestra, METH_VARARGS, "Return the orchestra file name."},
  {"score", csound_score, METH_VARARGS, "Return the score file name."},

  {"ievent", csound_ievent, METH_VARARGS, "Queue a new i-statement."},
  {"eevent", csound_eevent, METH_VARARGS, "Queue a new e-statement."},

  {"ftable", csound_ftable, METH_VARARGS, "Create a new f-statement."},
  {"time", csound_time, METH_VARARGS, "Return Csound time in seconds."},
  {"kcycle", csound_kcycle, METH_VARARGS, "Return Csound time in k-cycle (current k-cycle number)."},

  {"new_ftable", csound_new_ftable, METH_VARARGS, "Create a new ftable object."},

  {"stdout_write", csound_stdout_write, METH_VARARGS, "Write data using the stdout hook."},
  {"stderr_write", csound_stderr_write, METH_VARARGS, "Write data using the stderr hook."},

  {NULL, NULL, 0, NULL}
};

PyObject *
csound_module_init(void)
{
  csound_FTableType.ob_type = &PyType_Type;
  self = Py_InitModule("csound", csound_methods);
  PyDict_SetItemString(PyModule_GetDict(self), "args", PyDict_New());
  return self;
}

PyObject *
csound_module_get_module(void)
{
  return self;
}
#endif
