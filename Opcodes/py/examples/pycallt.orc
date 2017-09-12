sr=44100
kr=4410
ksmps=10
nchnls=1

pyinit

pyruni  {{
def f0(a, b, c, d, e, f, g, h):
    print 'f0 got', a, b, c, d, e, f, g, h
    print 'f0 returning None'

def f1(a, b, c, d, e, f, g, h):
    print 'f1 got', a, b, c, d, e, f, g, h
    print 'f1 returning', a
    return a

def f2(a, b, c, d, e, f, g, h):
    print 'f2 got', a, b, c, d, e, f, g, h
    print 'f2 returning', a, b
    return a, b

def f3(a, b, c, d, e, f, g, h):
    print 'f3 got', a, b, c, d, e, f, g, h
    print 'f3 returning', a, b, c
    return a, b, c

def f4(a, b, c, d, e, f, g, h):
    print 'f4 got', a, b, c, d, e, f, g, h
    print 'f4 returning', a, b, c, d
    return a, b, c, d

def f5(a, b, c, d, e, f, g, h):
    print 'f5 got', a, b, c, d, e, f, g, h
    print 'f5 returning', a, b, c, d, e
    return a, b, c, d, e

def f6(a, b, c, d, e, f, g, h):
    print 'f6 got', a, b, c, d, e, f, g, h
    print 'f6 returning', a, b, c, d, e, f
    return a, b, c, d, e, f

def f7(a, b, c, d, e, f, g, h):
    print 'f7 got', a, b, c, d, e, f, g, h
    print 'f7 returning', a, b, c, d, e, f, g
    return a, b, c, d, e, f, g

def f8(a, b, c, d, e, f, g, h):
    print 'f8 got', a, b, c, d, e, f, g, h
    print 'f8 returning', a, b, c, d, e, f, g, h
    return a, b, c, d, e, f, g, h
}}

instr 1

        kt      metro   1.5

                                 pycallt  kt, "f0", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1                             pycall1t kt, "f1", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2                         pycall2t kt, "f2", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3                     pycall3t kt, "f3", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4                 pycall4t kt, "f4", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5             pycall5t kt, "f5", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5, k6         pycall6t kt, "f6", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5, k6, k7     pycall7t kt, "f7", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5, k6, k7, k8 pycall8t kt, "f8", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
endin
