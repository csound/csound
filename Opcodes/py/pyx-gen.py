#!/usr/bin/env python

def generate_x_method(f, action, context, rate0, triggered):
    if rate0 != 'k':
        rate = rate0
    else:
        rate = ''

    if action in ('exec', 'assign', 'eval'):
        size = 1024
    elif action == 'run':
        size = 40960
    else:
        raise 'undefined action %s' % action

    if action in ('run', 'assign'):
        helper =  'run_statement'
    elif action == 'exec':
        helper = 'exec_file'
    elif action == 'eval':
        helper = 'eval_string'
    else:
        raise 'undefined action %s' % action

    if context == 'private':
        prefix = 'l'
        ns = 'GETPYLOCAL(p->h.insdshead)'
    elif context == 'global':
        prefix = ''
        ns = '0'
        prepare = ''
    else:
        raise 'undefined context %s' % context

    if triggered:
        if rate == 'i': raise 'cannot be triggered at i-rate'
        t, T = 't', 'T'
    else:
        t, T = '', ''

    ACTION = action.upper()
    RATE = rate.upper()

    name = 'py%(prefix)s%(action)s%(rate)s%(t)s_%(rate0)srate' % locals()

    print >> f, 'static int %(name)s(CSOUND *csound, PY%(ACTION)s%(T)s *p)' % locals()
    print >> f, '{'
    print >> f, '    char      source[%d];' % size
    print >> f, '    PyObject  *result;'
    print >> f, '   int *py_initialize_done;'
    print >> f, '   if((py_initialize_done = csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||*py_initialize_done == 0)' 
    print >> f, '   return NOTOK;'
    print >> f

    if triggered:
        if action == 'eval':
            print >> f, '    if (!*p->trigger) {'
            print >> f, '      *p->result = p->oresult;'
            print >> f, '      return OK;'
            print >> f, '    }'
        else:
            print >> f, '    if (!*p->trigger)'
            print >> f, '      return OK;'
        print >> f

    if context == 'private' and rate0 == 'i':
        print >> f, '    create_private_namespace_if_needed(&p->h);'
        print >> f

    if action == 'assign':
        print >> f, '    sprintf(source, "%s = %f", (char*) p->string->data, *p->value);'
    else:
        print >> f, '    strcpy(source, (char*) p->string->data);'
    print >> f

    if action == 'exec':
        print >> f, '    result = %(helper)s_in_given_context(csound, source, %(ns)s);' % locals()
    else:
        print >> f, '    result = %(helper)s_in_given_context(source, %(ns)s);' % locals()
    print >> f, '    if (result == NULL) {'
    print >> f, '      return pyErrMsg(p, "python exception");'
    print >> f, '    }'
    if action == 'eval':
        print >> f, '    else if (!PyFloat_Check(result)) {'
        print >> f, '      errMsg(p, "expression must evaluate in a float");'
        print >> f, '    }'
        print >> f, '    else {'
        print >> f, '      *p->result = PyFloat_AsDouble(result);'
        if triggered:
            print >> f, '      p->oresult = *p->result;'
        print >> f, '    }'

    print >> f, '    Py_DECREF(result);'
    print >> f, '    return OK;'
    print >> f, '}'
    print >> f

def generate_init_method(f, action, triggered):
    ACTION = action.upper()
    if triggered:
        t, T = 't', 'T'
    else:
        t, T = '', ''
    print >> f, 'static int pyl%(action)s%(t)s_irate(CSOUND *csound, PY%(ACTION)s%(T)s *p)' % locals()
    print >> f, '{'
    print >> f, '   int *py_initialize_done;'
    print >> f, '   if((py_initialize_done = csound->QueryGlobalVariable(csound,"PY_INITIALIZE")) == NULL ||*py_initialize_done == 0)' 
    print >> f, '   return NOTOK;'
    print >> f, '    create_private_namespace_if_needed(&p->h);'
    print >> f, '    return OK;'
    print >> f, '}'
    print >> f

# ----------------

def generate_pycall_opcode_struct(f, action, triggered):
    ACTION = action.upper()
    if triggered:
        T = 'T'
    else:
        T = ''
    print >> f, 'typedef struct {'
    print >> f, '    OPDS    h;'
    if action == 'eval':
        print >> f, '    MYFLT   *result;'
    if triggered:
        print >> f, '    MYFLT   *trigger;'
    print >> f, '    MYFLT   *string;'
    if action == 'assign':
        print >> f, '    MYFLT   *value;'
    if action == 'eval' and triggered:
        print >> f, '    MYFLT   oresult;'
    print >> f, '} PY%(ACTION)s%(T)s;' % locals()
    print >> f

f = open('pyx.auto.c', 'w')
print >> f
for action in ['exec', 'run', 'eval', 'assign']:
    generate_x_method(f, action, 'global', 'k', 0)
    generate_x_method(f, action, 'global', 'i', 0)

    generate_init_method(f, action, 0)
    generate_x_method(f, action, 'private', 'k', 0)
    generate_x_method(f, action, 'private', 'i', 0)

    generate_x_method(f, action, 'global', 'k', 1)
    generate_init_method(f, action, 1)
    generate_x_method(f, action, 'private', 'k', 1)
f.close()

f = open('pyx.auto.h', 'w')
print >> f
for action in ['exec', 'run', 'eval', 'assign']:
    for triggered in [0, 1]:
        generate_pycall_opcode_struct(f, action, triggered)
f.close()

