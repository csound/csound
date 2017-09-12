sr=44100
kr=4410
ksmps=10
nchnls=1

pyinit

pyruni  {{
def f0(a, b, c, d, e, f, g, h):
    pass

def f1(a, b, c, d, e, f, g, h):
    return a

def f2(a, b, c, d, e, f, g, h):
    return a, b

def f3(a, b, c, d, e, f, g, h):
    return a, b, c

def f4(a, b, c, d, e, f, g, h):
    return a, b, c, d

def f5(a, b, c, d, e, f, g, h):
    return a, b, c, d, e

def f6(a, b, c, d, e, f, g, h):
    return a, b, c, d, e, f

def f7(a, b, c, d, e, f, g, h):
    return a, b, c, d, e, f, g

def f8(a, b, c, d, e, f, g, h):
    return a, b, c, d, e, f, g, h
}}

instr 1

                                 pycall  "f0", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1                             pycall1 "f1", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2                         pycall2 "f2", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3                     pycall3 "f3", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4                 pycall4 "f4", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5             pycall5 "f5", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5, k6         pycall6 "f6", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5, k6, k7     pycall7 "f7", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8
  k1, k2, k3, k4, k5, k6, k7, k8 pycall8 "f8", 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8

        printk  0.01, k1
        printk  0.01, k2
        printk  0.01, k3
        printk  0.01, k4
        printk  0.01, k5
        printk  0.01, k6
        printk  0.01, k7
        printk  0.01, k8
endin
