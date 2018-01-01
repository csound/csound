#!/usr/bin/env python

# Copyright (C) 2002 Maurizio Umberto Puxeddu

#    This file is part of Csound.

#    The Csound Library is free software; you can redistribute it
#    and/or modify it under the terms of the GNU Lesser General Public
#    License as published by the Free Software Foundation; either
#    version 2.1 of the License, or (at your option) any later version.

#    Csound is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.

#    You should have received a copy of the GNU Lesser General Public
#    License along with Csound; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
#    02110-1301 USA

'''Automatically generate opcode structures, methods [and table entries]. Just run it.'''

def generate_pycall_common_init_code(f, n, pre, post, rate, triggered=0):
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    name = 'py%scall%d%s%s_%srate' % (pre, n, post, t, rate)
    print >> f, 'static int %s(CSOUND *csound, PYCALL%d%s *p)' % (name, n, T)
    print >> f, '{'
    print >> f, '    char      command[1024];'
    print >> f, '    PyObject  *result;'
    print >> f, '   int *py_initialize_done;'
    print >> f, '   if((py_initialize_done = csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||*py_initialize_done == 0)' 
    print >> f, '   return NOTOK;'
    print >> f
    if triggered:
        print >> f, '    if (!*p->trigger) {'
        if n == 0:
            pass
        if n == 1:
            print >> f, '      *p->result = p->oresult;'
        elif n > 1:
            for i in range(n):
                print >> f, '      *p->result%d = p->oresult%d;' % (i+1, i+1)
        print >> f, '      return OK;'
        print >> f, '    }'
        print >> f

def generate_pycall_common_call_code(f, context, withinit, triggered):
    if triggered:
        skip = 2
    else:
        skip = 1
    print >> f, '    format_call_statement(command, (char*) p->function->data,'
    print >> f, '                          p->INOCOUNT, p->args, %d);' % skip
    print >> f
    if context == 'private':
        print >> f, '    result = eval_string_in_given_context(command, 0);'
    else:
        if withinit:
            print >> f, '    create_private_namespace_if_needed(&p->h);'
            print >> f
        print >> f, '    result = eval_string_in_given_context(command, GETPYLOCAL(p->h.insdshead));'
    print >> f

def generate_pycall_exception_handling_code(f, n, pre, post, rate, triggered=0):
    print >> f, '    if (result == NULL)'
    print >> f, '      return pyErrMsg(p, "python exception");'
    print >> f

def generate_pycall_result_conversion_code(f, n, pre, post, rate, triggered=0):
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    if n == 0:
        print >> f, '    if (result != Py_None)'
        print >> f, '      return errMsg(p, "callable must return None");'
    elif n == 1:
        print >> f, '    if (!PyFloat_Check(result)) {'
        print >> f, '      return errMsg(p, "callable must return a float");'
        print >> f, '    }'
        print >> f, '    else {'
        print >> f, '      *p->result = PyFloat_AsDouble(result);'
        if triggered:
            print >> f, '      p->oresult = *p->result;'
        print >> f, '    }'
        print >> f, '    return OK;'
    else:
        name = 'py%scall%d%s%s_%srate' % (pre, n, post, t, rate)
        print >> f, '    if (!PyTuple_Check(result) || PyTuple_Size(result) != %d) {' % n
        print >> f, '      return errMsg(p, "callable must return %d values");'  % n
        print >> f, '    }'
        print >> f, '    else {'
        for i in range(n):
            print >> f, '      *p->result%d = PyFloat_AsDouble(PyTuple_GET_ITEM(result, %d));' % (i+1, i)
            if triggered:
                print >> f, '      p->oresult%d = *p->result%d;' % (i+1, i+1)
        print >> f, '    }'
    print >> f
    print >> f, '    Py_DECREF(result);'
    print >> f, '    return OK;'
    print >> f, '}'
    print >> f

def generate_pycall_krate_method(f, n, triggered=0):
    generate_pycall_common_init_code(f, n, '', '', 'k', triggered)
    generate_pycall_common_call_code(f, 'private', 1, triggered)
    generate_pycall_exception_handling_code(f, n, '', '', 'k', triggered)
    generate_pycall_result_conversion_code(f, n, '', '', 'k', triggered)

def generate_pylcall_irate_method(f, n, triggered=0):
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    name = 'pylcall%d%s_irate' % (n, t)
    print >> f, 'static int %s(CSOUND *csound, PYCALL%d%s *p)' % (name, n, T)
    print >> f, '{'
    print >> f, '   int *py_initialize_done;'
    print >> f, '   if((py_initialize_done = csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||*py_initialize_done == 0)' 
    print >> f, '   return NOTOK;'
    print >> f, '    create_private_namespace_if_needed(&p->h);'
    print >> f, '    return OK;'
    print >> f, '}'
    print >> f

def generate_pylcall_krate_method(f, n, triggered=0):
    generate_pycall_common_init_code(f, n, 'l', '', 'k', triggered)
    generate_pycall_common_call_code(f, 'global', 0, triggered)
    generate_pycall_exception_handling_code(f, n, 'l', '', 'k', triggered)
    generate_pycall_result_conversion_code(f, n, 'l', '', 'k', triggered)

def generate_pylcalli_irate_method(f, n):
    generate_pycall_common_init_code(f, n, 'l', 'i', 'i')
    generate_pycall_common_call_code(f, 'global', 1, 0)
    generate_pycall_exception_handling_code(f, n, 'l', 'i', 'i')
    generate_pycall_result_conversion_code(f, n, 'l', 'i', 'i')

# ----------

def generate_pycall_opcode_struct(f, n, triggered=0):
    if triggered:
        T = 'T'
    else:
        T = ''
    print >> f, 'typedef struct {'
    print >> f, '    OPDS    h;'
    if n == 1:
        print >> f, '    MYFLT   *result;'
    else:
        for i in range(n):
            print >> f, '    MYFLT   *result%d;' % (i+1)
    if triggered:
        print >> f, '    MYFLT   *trigger;'
    print >> f, '    STRINGDAT   *function;'
    print >> f, '    MYFLT   *args[VARGMAX-3];'
    if triggered:
        if n == 1:
            print >> f, '    MYFLT   oresult;'
        else:
            for i in range(n):
                print >> f, '    MYFLT   oresult%d;' % (i+1)
    print >> f, '} PYCALL%d%s;' % (n, T)
    print >> f

# --------

f = open('pycall.auto.c', 'w')
print >> f
for n in range(9):
    generate_pycall_krate_method(f, n)
    generate_pylcall_irate_method(f, n)
    generate_pylcall_krate_method(f, n)
    generate_pylcalli_irate_method(f, n)

    generate_pycall_krate_method(f, n, 1)
    generate_pylcall_irate_method(f, n, 1)
    generate_pylcall_krate_method(f, n, 1)
f.close()

f = open('pycall.auto.h', 'w')
print >> f
for n in range(9):
    generate_pycall_opcode_struct(f, n)
    generate_pycall_opcode_struct(f, n, 1)
f.close()

