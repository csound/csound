/*
 * pycall.auto.c
 *
 * Copyright (C) 2002 Maurizio Umberto Puxeddu, Michael Gogins
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

static int pycall0_krate(CSOUND *csound, PYCALL0 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (result == NULL)
      return pyErrMsg(p, "python exception");

    if (result != Py_None)
      return errMsg(p, "callable must return None");

    Py_DECREF(result);
    return OK;
}

static int pylcall0_irate(CSOUND *csound, PYCALL0 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall0_krate(CSOUND *csound, PYCALL0 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(result != Py_None))
      return errMsg(p, "callable must return None");

    Py_DECREF(result);
    return OK;
}

static int pylcall0i_irate(CSOUND *csound, PYCALL0 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(result != Py_None))
      return errMsg(p, "callable must return None");

    Py_DECREF(result);
    return OK;
}

static int pycall0t_krate(CSOUND *csound, PYCALL0T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(result != Py_None))
      return errMsg(p, "callable must return None");

    Py_DECREF(result);
    return OK;
}

static int pylcall0t_irate(CSOUND *csound, PYCALL0T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall0t_krate(CSOUND *csound, PYCALL0T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(result != Py_None))
      return errMsg(p, "callable must return None");

    Py_DECREF(result);
    return OK;
}

static int pycall1_krate(CSOUND *csound, PYCALL1 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyFloat_Check(result))) {
      return errMsg(p, "callable must return a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
    }
    return OK;

    Py_DECREF(result);
    return OK;
}

static int pylcall1_irate(CSOUND *csound, PYCALL1 *p)
{
     IGN(csound);
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall1_krate(CSOUND *csound, PYCALL1 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyFloat_Check(result))) {
      return errMsg(p, "callable must return a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
    }
    return OK;

    Py_DECREF(result);
    return OK;
}

static int pylcall1i_irate(CSOUND *csound, PYCALL1 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyFloat_Check(result))) {
      return errMsg(p, "callable must return a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
    }
    return OK;

    Py_DECREF(result);
    return OK;
}

static int pycall1t_krate(CSOUND *csound, PYCALL1T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result = p->oresult;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyFloat_Check(result))) {
      return errMsg(p, "callable must return a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
      p->oresult = *p->result;
    }
    return OK;

    Py_DECREF(result);
    return OK;
}

static int pylcall1t_irate(CSOUND *csound, PYCALL1T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall1t_krate(CSOUND *csound, PYCALL1T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result = p->oresult;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyFloat_Check(result))) {
      return errMsg(p, "callable must return a float");
    }
    else {
      *p->result = PyFloat_AsDouble(result);
      p->oresult = *p->result;
    }
    return OK;

    Py_DECREF(result);
    return OK;
}

static int pycall2_krate(CSOUND *csound, PYCALL2 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 2)) {
      return errMsg(p, "callable must return 2 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall2_irate(CSOUND *csound, PYCALL2 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall2_krate(CSOUND *csound, PYCALL2 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 2)) {
      return errMsg(p, "callable must return 2 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall2i_irate(CSOUND *csound, PYCALL2 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 2)) {
      return errMsg(p, "callable must return 2 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
    }

    Py_DECREF(result);
    return OK;
}

static int pycall2t_krate(CSOUND *csound, PYCALL2T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 2)) {
      return errMsg(p, "callable must return 2 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall2t_irate(CSOUND *csound, PYCALL2T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
   create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall2t_krate(CSOUND *csound, PYCALL2T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 2)) {
      return errMsg(p, "callable must return 2 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
    }

    Py_DECREF(result);
    return OK;
}

static int pycall3_krate(CSOUND *csound, PYCALL3 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 3)) {
      return errMsg(p, "callable must return 3 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall3_irate(CSOUND *csound, PYCALL3 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall3_krate(CSOUND *csound, PYCALL3 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 3)) {
      return errMsg(p, "callable must return 3 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall3i_irate(CSOUND *csound, PYCALL3 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 3)) {
      return errMsg(p, "callable must return 3 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
    }

    Py_DECREF(result);
    return OK;
}

static int pycall3t_krate(CSOUND *csound, PYCALL3T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 3)) {
      return errMsg(p, "callable must return 3 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall3t_irate(CSOUND *csound, PYCALL3T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall3t_krate(CSOUND *csound, PYCALL3T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 3)) {
      return errMsg(p, "callable must return 3 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
    }

    Py_DECREF(result);
    return OK;
}

static int pycall4_krate(CSOUND *csound, PYCALL4 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 4)) {
      return errMsg(p, "callable must return 4 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall4_irate(CSOUND *csound, PYCALL4 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall4_krate(CSOUND *csound, PYCALL4 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 4)) {
      return errMsg(p, "callable must return 4 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall4i_irate(CSOUND *csound, PYCALL4 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 4)) {
      return errMsg(p, "callable must return 4 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
    }

    Py_DECREF(result);
    return OK;
}

static int pycall4t_krate(CSOUND *csound, PYCALL4T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 4)) {
      return errMsg(p, "callable must return 4 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall4t_irate(CSOUND *csound, PYCALL4T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall4t_krate(CSOUND *csound, PYCALL4T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 4)) {
      return errMsg(p, "callable must return 4 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
    }

    Py_DECREF(result);
    return OK;
}

static int pycall5_krate(CSOUND *csound, PYCALL5 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 5)) {
      return errMsg(p, "callable must return 5 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall5_irate(CSOUND *csound, PYCALL5 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall5_krate(CSOUND *csound, PYCALL5 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 5)) {
      return errMsg(p, "callable must return 5 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall5i_irate(CSOUND *csound, PYCALL5 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 5)) {
      return errMsg(p, "callable must return 5 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
    }

    Py_DECREF(result);
    return OK;
}

static int pycall5t_krate(CSOUND *csound, PYCALL5T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 5)) {
      return errMsg(p, "callable must return 5 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall5t_irate(CSOUND *csound, PYCALL5T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall5t_krate(CSOUND *csound, PYCALL5T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 5)) {
      return errMsg(p, "callable must return 5 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
    }

    Py_DECREF(result);
    return OK;
}

static int pycall6_krate(CSOUND *csound, PYCALL6 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 6)) {
      return errMsg(p, "callable must return 6 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall6_irate(CSOUND *csound, PYCALL6 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall6_krate(CSOUND *csound, PYCALL6 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 6)) {
      return errMsg(p, "callable must return 6 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall6i_irate(CSOUND *csound, PYCALL6 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 6)) {
      return errMsg(p, "callable must return 6 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
    }

    Py_DECREF(result);
    return OK;
}

static int pycall6t_krate(CSOUND *csound, PYCALL6T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      *p->result6 = p->oresult6;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 6)) {
      return errMsg(p, "callable must return 6 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      p->oresult6 = *p->result6;
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall6t_irate(CSOUND *csound, PYCALL6T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall6t_krate(CSOUND *csound, PYCALL6T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      *p->result6 = p->oresult6;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 6)) {
      return errMsg(p, "callable must return 6 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      p->oresult6 = *p->result6;
    }

    Py_DECREF(result);
    return OK;
}

static int pycall7_krate(CSOUND *csound, PYCALL7 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 7)) {
      return errMsg(p, "callable must return 7 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall7_irate(CSOUND *csound, PYCALL7 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall7_krate(CSOUND *csound, PYCALL7 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 7)) {
      return errMsg(p, "callable must return 7 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall7i_irate(CSOUND *csound, PYCALL7 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 7)) {
      return errMsg(p, "callable must return 7 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
    }

    Py_DECREF(result);
    return OK;
}

static int pycall7t_krate(CSOUND *csound, PYCALL7T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      *p->result6 = p->oresult6;
      *p->result7 = p->oresult7;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 7)) {
      return errMsg(p, "callable must return 7 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      p->oresult6 = *p->result6;
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
      p->oresult7 = *p->result7;
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall7t_irate(CSOUND *csound, PYCALL7T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall7t_krate(CSOUND *csound, PYCALL7T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      *p->result6 = p->oresult6;
      *p->result7 = p->oresult7;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 7)) {
      return errMsg(p, "callable must return 7 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      p->oresult6 = *p->result6;
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
      p->oresult7 = *p->result7;
    }

    Py_DECREF(result);
    return OK;
}

static int pycall8_krate(CSOUND *csound, PYCALL8 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 8)) {
      return errMsg(p, "callable must return 8 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
      *p->result8 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 7));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall8_irate(CSOUND *csound, PYCALL8 *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall8_krate(CSOUND *csound, PYCALL8 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 8)) {
      return errMsg(p, "callable must return 8 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
      *p->result8 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 7));
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall8i_irate(CSOUND *csound, PYCALL8 *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 1);

    create_private_namespace_if_needed(&p->h);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 8)) {
      return errMsg(p, "callable must return 8 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
      *p->result8 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 7));
    }

    Py_DECREF(result);
    return OK;
}

static int pycall8t_krate(CSOUND *csound, PYCALL8T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      *p->result6 = p->oresult6;
      *p->result7 = p->oresult7;
      *p->result8 = p->oresult8;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, 0);

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 8)) {
      return errMsg(p, "callable must return 8 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      p->oresult6 = *p->result6;
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
      p->oresult7 = *p->result7;
      *p->result8 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 7));
      p->oresult8 = *p->result8;
    }

    Py_DECREF(result);
    return OK;
}

static int pylcall8t_irate(CSOUND *csound, PYCALL8T *p)
{
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    create_private_namespace_if_needed(&p->h);
    return OK;
}

static int pylcall8t_krate(CSOUND *csound, PYCALL8T *p)
{
    char      command[1024];
    PyObject  *result;
    int *py_initialize_done;
    if (UNLIKELY((py_initialize_done =
                  csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||
                 *py_initialize_done == 0))
      return NOTOK;
    if (!*p->trigger) {
      *p->result1 = p->oresult1;
      *p->result2 = p->oresult2;
      *p->result3 = p->oresult3;
      *p->result4 = p->oresult4;
      *p->result5 = p->oresult5;
      *p->result6 = p->oresult6;
      *p->result7 = p->oresult7;
      *p->result8 = p->oresult8;
      return OK;
    }

    format_call_statement(command, (char*) p->function->data,
                          p->INOCOUNT, p->args, 2);

    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));

    if (UNLIKELY(result == NULL))
      return pyErrMsg(p, "python exception");

    if (UNLIKELY(!PyTuple_Check(result) || PyTuple_Size(result) != 8)) {
      return errMsg(p, "callable must return 8 values");
    }
    else {
      *p->result1 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 0));
      p->oresult1 = *p->result1;
      *p->result2 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 1));
      p->oresult2 = *p->result2;
      *p->result3 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 2));
      p->oresult3 = *p->result3;
      *p->result4 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 3));
      p->oresult4 = *p->result4;
      *p->result5 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 4));
      p->oresult5 = *p->result5;
      *p->result6 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 5));
      p->oresult6 = *p->result6;
      *p->result7 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 6));
      p->oresult7 = *p->result7;
      *p->result8 = PyFloat_AsDouble(PyTuple_GET_ITEM(result, 7));
      p->oresult8 = *p->result8;
    }

    Py_DECREF(result);
    return OK;
}
