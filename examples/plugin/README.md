
Csound Plugin Opcode Framework (CPOF)
===========================


Plugins in Csound are usually written in C, which provides a low-level access to
the system interface, allowing an uncompromised and complete scope for
new opcode development. For most sys developers, this will continue to be the
best working environment. However, for many of the more common forms of
plugins, this interface can be complex and cumbersome. In particular, we
might like to take advantage of an object-oriented approach so that
we can, for instance, re-use code more extensively and take advantage of
existing algorithms and libraries. For this, the ideal language is C++, and
so we have provided a lightweight framework to facilitate programming in
this environment. While there is an existing interface for plugin writing in
the Csound codebase, we would like to provide here an alternative to it
attempts to be thin, simple, complete and that handles internal
Csound resources in a safe way.

The Base Classes
---------------------------

The framework base classes are actually templates which need to be
derived and instantiated by the user code. The most general of these is
Plugin. To use we program our own class by subclassing it and
passing the number of output and inputs our opcode needs as its
template arguments

```
#include <plugin.h>

Struct MyPlug : csnd::Plugin<1,1> {

};
```

The above lines will create a plugin opcode with one output (first
template argument) and one input (second template argument). All CPOF
code is declared in the namespace csnd.
This class will create a complete opcode, although it will also be fully non-op.

To make it do something, we will need to reimplement one, two or three
of its methods. This base class is derived from the Csound structure
OPDS and has the following members:

* outargs: output arguments.
* inargs: input arguments.
* csound: a pointer to the Csound object running this opcode.
* offset: the starting position of an audio vector (for audio opcodes).
* nsmps: the size of an audio vector (also for audio opcodes only).
* init(), kperf() and aperf() non-op methods, to be reimplemented as needed.
* sa_offset() method to be used in audio processsing to calculate offset and
	nsmps for sample-accurate behaviour.

The other base class in the CPOF is FPlugin, derived from Plugin, which
provides some extra facilities for fsig (streaming frequency-domain) plugins:

* framecount: a member to hold a running count of fsig frames.
* check_sliding() and check_format() methods to check an input fsig format.

Init-time opcodes
-------------------------------------------

For init-time opcodes, all we need to do is provide an implementation of
the init() method:

```
#include <plugin.h>

/** i-time plugin opcode example
    with 1 output and 1 input \n
    iout simple iin
 */
struct Simplei : csnd::Plugin<1,1> {
  int init() {
    outargs[0] = inargs[0];
    return OK;
  }
};
```
In this simple example, we just copy the input arguments to the output once, at
init-time. Each scalar input can be accessed using array indexing.


K-rate opcodes
-------------------------------------------

For opcodes running only at k-rate (no init-time operation), all we need to do
is provide an implementation of the kperf() method:

```
#include <plugin.h>

/** k-rate plugin opcode example
    with 1 output and 1 input \n
    kout simple kin
 */
struct Simplek : csnd::Plugin<1,1> {
  int kperf() {
    outargs[0] = inargs[0];
    return OK;
  }
};
```
Similarly, in this simple example, we just copy the input arguments to the output
at each k-period.

A-rate opcodes
-------------------------------------------

For opcodes running only at a-rate (and with no init-time operation), all we need to do
is provide an implementation of the aperf() method:

```
#include <plugin.h>

/** a-rate plugin opcode example
    with 1 output and 1 input \n
    aout simple ain
 */
struct Simplea : csnd::Plugin<1,1> {
  int aperf() {
    std::copy(inargs.data(0),inargs.data(0)+insdshead->ksmps, outargs.data(0));
    return OK;
  }
};
  ```

Because audio arguments are vectors, we get these using the data() method
for the inargs and outargs objects, which takes the argument number as
input and returns a MYFLT pointer to the vector. MYFLT is the internal
floating-point data type used by Csound.

Note that the OPDS member insdshead holds the value of the instrument
vector size (ksmps), so we can get it from there. More normally, we
will just access the nsmps variable after calling sa_offset() to get this
value. We will demonstrate this in later examples.

Registering opcodes with Csound
---------------------------------------

Once we have written our opcode classes, we need to tell Csound about
them, so that they can be used, for this we use the CPOF function template
plugin():

```
template <typename T>
int plugin(CSOUND *csound, const char *name, const char *oargs,
           const char *iargs, uint32_t thread)

```

Its parameters are:

* csound: a pointer to the Csound object to which we want to register our opcode.
* name: the opcode name as it will be used in Csound code.
* oargs: a string containing the opcode output types, one identifier per argument
* iargs: a string containintg the opcode input types, one identifier per argument
* thread: a code to tell Csound when the opcode shoulld be active.

For opcode type identifiers, the most common types are: a (audio), k (control), i (i-time),
S (string) and f (fsig). For the thread argument, we have the following options, which
depend on the processing methods implemented in our plugin class:

* csnd::thread::i : init().
* csnd::thread::k : kperf(). 
* csnd::thread::ik : init() and kperf().
* csnd:thread::a : aperf().
* csnd::thread::ia : init() and aperf().
* csnd::thread::ika : init(), kperf() and aperf().

We instantiate and call these template functions inside the csoundModuleInit() function
of our opcode dynamic library. This is one of three functions we need to supply in order
for Csound to load the plugin (the other two are generally left as non-ops):

```
/** Module creation, initalisation and destruction
 */
extern "C" {
PUBLIC int csoundModuleInit(CSOUND *csound) {
  csnd::plugin<Simplei>(csound, "simple", "i", "i",  csnd::thread::i);
  csnd::plugin<Simplek>(csound, "simple", "k", "k",  csnd::thread::k);
  csnd::plugin<Simplea>(csound, "simple", "a", "a",  csnd::thread::a);
  return 0;
}
PUBLIC int csoundModuleCreate(CSOUND *csound) { return 0; }
PUBLIC int csoundModuleDestroy(CSOUND *csound) { return 0; }
}
```

Note how the class name is passed as an argument to the function template,
followed by the function call.

Memory allocation
---------------------------------------------------

For efficiency and to prevent leaks and undefined behaviour we need to
leave all memory allocation to Csound and refrain from using C++
allocators or standard library containers that use dynamic allocation
behind the scenes (e.g. std::vector). If we follow these rules, our code
will work as intended and cause no problems for users.

This requires us to use the AuxAlloc mechanism implemented by
Csound for opcodes. To allow for ease of use, CPOF provides a wrapper
template class (which is not too dissimilar to std::vector) for us to
allocate and use as much memory as we need. This functionality
is given by the AuxMem class, which has the following methods and
members:

* allocate(): allocates memory (if required).
* operator[] : array-subscript access to the allocated memory.
* data(): returns a pointer to the data.
* len(): returns the length of the vector.
* begin() and end(): return iterators to the beginning and end of
data
* iterator and const_iterator: iterator types for this class.

As an example of use, we can implement a simple delay line
opcode, whose delay time is set at i-time, providing a slap-back
echo effect:

```
/** a-rate plugin opcode example: delay line
    with 1 output and 2 inputs (a,i) \n
    asig delayline ain,idel
 */
struct DelayLine : csnd::Plugin<1,2> {
  csnd::AuxMem<MYFLT> delay;
  csnd::AuxMem<MYFLT>::iterator iter;

  int init() {
    delay.allocate(csound, csound->GetSr(csound)*inargs[1]);
    iter = delay.begin();
    return OK;
  }
  
  int aperf() {
    MYFLT *out = outargs.data(0);
    MYFLT *in = inargs.data(0);
    
    sa_offset(out);
    for(uint32_t i=offset; i < nsmps; i++, iter++) {
      if(iter == delay.end()) iter = delay.begin();
      out[i] = *iter;
      *iter = in[i]; 
    }
    return OK;
  }
};
```

In this example, we use an AuxMem interator to access the
delay vector. It is equally possible to access each element with
an array-style subscript. The memory allocated by this class is
managed by Csound, so we do not need to be concerned about
disposing of it. To register this opcode, we do

```
csnd::plugin<DelayLine>(csound, "delayline", "a", "ai", csnd::thread::ia);
```

Table Access
-----------------------------------------------

Access to function tables has also been facilitated by a thin
wrapper class that allows us to treat it as a vector object.
This is provided by the Table class, which has the
following members:

* init(): initialises a table object from an opcode argument pointer.
* operator[] : array-subscript access to the function table.
* data(): returns a pointer to the function table data.
* len(): returns the length of the table (excluding guard point).
* begin() and end(): return iterators to the beginning and end of
the function table
* iterator and const_iterator: iterator types for this class.


An example of table access is given by an oscillator opcode,
which is implemented in the following class:

```
/** a-rate plugin opcode example: oscillator
    with 1 output and 3 inputs (k,k,i) \n
    aout oscillator kamp,kcps,ifn
 */
struct Oscillator : csnd::Plugin<1,3> {
  csnd::Table table;
  double scl;
  double ndx;

  int init() {
    table.init(csound,inargs.data(2));
    scl = table.len()/csound->GetSr(csound);
    ndx = 0;
    return OK;
  }
  
  int aperf() {
    MYFLT *out = outargs.data(0);
    MYFLT amp = inargs[0];
    MYFLT si = inargs[1]*scl;
    
    sa_offset(out);
    for(uint32_t i=offset; i < nsmps; i++) {
      out[i] = amp*table[(uint32_t)ndx];
      ndx += si;
      while(ndx < 0)
	ndx += table.len();
      while(ndx >= table.len())
	ndx -= table.len();
    }
    return OK;
  }
};
```
The table is initialised by passing the relevant argument
pointer to it (using its data() method). Also note that,
as we need a precise phase index value,
we cannot use iterators in this case (without making it
very awkward), so we employ straightforward
array subscripting. The opcode is registered by

```
csnd::plugin<Oscillator>(csound, "oscillator", "a", "kki",csnd::thread::ia);
```

Strings
---------------------------------------------

String variables in Csound are held in a STRINGDAT data structure,
containing a data member that holds the actual string and a size
member with the allocated memory size. While CPOF does not
wrap strings, it provides a translated access to string arguments
through the argument objects str_data() function. This takes a
an argument index (similarly to data()) and returns a reference to
the string variable, as demonstrated in this example:

```
/** i-time string plugin opcode example
    with only 1 input \n
    tprint Sin
 */
struct Tprint : csnd::Plugin<0,1> {
  int init() {
    csound->Message(csound, "%s",
		    inargs.str_data(0).data);
    return OK;
  }
};
```

This opcode will print the string to the console. Note that we have
no output arguments, so we set the first template parameter to 0.
We register it using

```
csnd::plugin<Tprint>(csound, "tprint", "", "S",  csnd::thread::i);

```

Fsigs
------------------------------------------------

For streaming spectral processing opcodes, we have a
different base class with extra facilities needed for their operation (FPlugin).
Fsig variables are held in a PVSDAT data structure. To facilitate
their manipulation, CPOF provides a wrapper class Fsig
with the following members:

* init(): initialisation from individual parameters or from an
existing fsig. Also allocates frame memory as needed.
* operator[] : array-subscript access to the spectral frame
  (csnd::pvscmplx, same as std::complex\<float\>).
* data(): returns a pointer to the spectral frame data (
  csnd::pvscmpl \*).
* len(): returns the length of the frame.
* begin() and end(): return iterators to the beginning and end of
the data frame (undefined behaviour for sliding mode).
* iterator and const_iterator: iterator types for this class.
* data_sliding(): returns a pointer to the spectral frame data for
sliding analysis mode (as a csnd::sldcmplx \*, same as std::complex\<MYFLT\>\*).
* count(): get and set fsig framecount.
* isSliding(): checks for sliding mode.
* fsig_format(): returns the fsig data format (csnd::fsig_format::pvs,
csnd::fsig_format::polar, csnd::fsig_format::complex, or
csnd::fsig_format::tracks).

To facilitate the handling of pvs bin data, we have a translation
type pvsbin, to which we can assign a pvscmplx variable. This has the
following methods:

* amp(): returns the bin amplitude.
* freq(): returns the bin frequency.
* amp(float a): sets the bin amplitude to a.
* freq(float f): sets the bin frequency to f.

Fsig opcodes run at k-rate but will internally use an update rate based
on the analysis hopsize. For this to work, a framecount is kept and
checked to make sure we only process the input when new data is
available. The following example class implements a simple gain
scaler for fsigs:

```
/** f-sig plugin opcode example: pv gain change
    with 1 output and 2 inputs (f,k) \n
    fsig pvg fsin, kgain
 */
struct PVGain : csnd::FPlugin<1, 2> {

  int init() {
    if (check_sliding(inargs.fsig_data(0)) != OK &&
        (check_format(inargs.fsig_data(0)) != OK ||
         check_format(inargs.fsig_data(0), csnd::fsig_format::polar)))
      return NOTOK;

    csnd::Fsig &fout = outargs.fsig_data(0);
    fout.init(csound, inargs.fsig_data(0));
    framecount = 0;
    return OK;
  }

  int kperf() {
    csnd::Fsig &fin = inargs.fsig_data(0);
    csnd::Fsig &fout = outargs.fsig_data(0);
    uint32_t i;

    if (framecount < fin.count()) {
      MYFLT g = inargs[1];
      std::transform(fin.begin(), fin.end(), fout.begin(),
		     [g](csnd::pvsbin f)
		     { f.amp(g*f.amp()); return f;});
      framecount = fout.count(fin.count());
    }
    return OK;
  }
};
```

Note that, as with strings, there is a dedicated method in the arguments
object that returns a ref to an Fsig class. This is used to initialise the
output object at i-time and then to obtain the input and output
variable data Csound processing. The framecount member is provided
by the base class, as well as the format check methods. This opcode
is registered using

```
csnd::plugin<PVGain>(csound, "pvg", "f", "fk", csnd::thread::ik);
```

Arrays
----------------------------------------------

Opcodes with array inputs or outputs use the data structure ARRAYDAT
for parameters. Again, in order to facilitate access to these argument
types, CPOF provides a wrapper class. The framework currently supports
only one-dimensional arrays (for multidimensional arrays a raw pointer
to ARRAYDAT should be used), which covers the most common uses.

The template container class Vector, derived from ARRAYDAT, holds the
argument data. It has the following members:

* init(): initialises an output variable.
* operator[] : array-subscript access to the vector data.
* data(): returns a pointer to the vector data.
* len(): returns the length of the vector.
* begin() and end(): return iterators to the beginning and end of
the vector.
* iterator and const_iterator: iterator types for this class.
* data_array(): returns a pointer to the vector data.

In addition to this, the inargs and outargs objects in the Plugin
class have a template method that can be used to get a Vector
class reference. A trivial example is shown below:

```
/** k-rate numeric array example
    with 1 output and 1 input \n
    kout[] simple kin[]
 */
struct SimpleArray : csnd::Plugin<1, 1> {
  int init() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    out.init(csound, in.len());
    return OK;
  }

  int kperf() {
    csnd::Vector<MYFLT> &out = outargs.vector_data<MYFLT>(0);
    csnd::Vector<MYFLT> &in = inargs.vector_data<MYFLT>(0);
    std::copy(in.begin(), in.end(), out.begin());
    return OK;
  }
  };
  ```

This opcode is registered using the following line:

```
csnd::plugin<SimpleArray>(csound, "simple", "k[]", "k[]", csnd::thread::ik);
```

Building the opcodes
-------------------------------------------

The code discussed here is provided in the opcodes.cpp source file in the
examples directory. In order to build these opcodes, we require a c++
compiler using c++11 mode (-std=c++11), and the Csound public
headers (including plugin.h). The opcodes should be built as a dynamic/shared
library (e.g so in Linux and dylib in OSX), but CPOF does not impose any
link-time dependencies (not even to Csound).

Victor Lazzarini, 01/2017

