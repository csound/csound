/**
 * L I N E A R   A L G E B R A   O P C O D E S   F O R   C S O U N D
 * Michael Gogins
 *
 * These opcodes implement many linear algebra operations,
 * from scalar, vector, and matrix arithmetic up to 
 * and including eigenvalue decompositions.
 * The opcodes are designed to facilitate signal processing, 
 * and of course other mathematical operations, 
 * in the Csound orchestra language.
 *
 * Linear Algebra Data Types
 *
 * Mathematical    Code   Corresponding Csound Type or Types
 * --------------  -----  --------------------------------------------------------
 * real scalar     r      Native i-rate or k-rate variable
 * complex scalar  c      Pair of native i-rate or k-rate variables, e.g. "kr, ki"
 * real vector     rv     Reference stored in native i-rate or k-rate variable
 *                 a      Native a-rate variable
 *                 t      Native function table number
 * complex vector  cv     Reference stored in native i-rate or k-rate 
 *                 f      Native fsig variable
 * real matrix     rm     Reference stored in native i-rate or k-rate variable
 * complex matrix  cm     Reference stored in native i-rate or k-rate variable
 *
 * All arrays are 0-based.
 *
 * All arrays are considered to be column-major 
 * (x or i goes along columnns, y or j goes along rows).
 *
 * All arrays are general and dense; banded, Hermitian, symmetric 
 * and sparse routines are not implemented.
 *
 * All operands must be pre-allocated; no operation allocates any arrays.
 * However, some operations may resize or reshape arrays to hold results.
 *
 * Data is stored in i-rate and k-rate "array" objects, 
 * which can be of type code vr, vc, mr, or mc, and 
 * which internally store type code and shape. 
 * In orchestra code the "array" variable is passed in the same way 
 * as a MYFLT i-rate or i-rate variable, but it is in reality
 * a pointer to the creator opcode instance.
 *
 * The processing rate and operation name 
 * are deterministically and exhaustively encoded into the opcode name as follows.
 * Each part below is separated by an underscore.
 * 1. Opcode family: "la" for "linear algebra".
 * 2. Processing rate: "i" for i-rate or "k" for k-rate.
 * 3. For creators only: type code of the created array.
 * 4. Operation: the common mathematical term or abbreviation, e.g.
 *    "plus", "minus", "prod", "div", "dot", "abs", "conj", "norm1", "det", "hermite", and so on.
 * 5. Where not array types: type code of parameter(s).
 *
 * Array Creators
 * --------------
 *
 * iarray                  la_i_vr_create    irows
 * iarray                  la_i_vc_create    irows
 * iarray                  la_i_mr_create    irows, icolumns  [, jdiagonal]
 * iarray                  la_i_mc_create    irows, icolumns  [, jdiagonal]
 * iarray                  la_i_copy         iarray
 * iarray                  la_i_copy_a       asig
 * iarray                  la_i_copy_f       fsig
 * iarray                  la_i_copy_t       itable
 *
 * Array Introspection
 * -------------------
 *
 * itype, irows, icolumns  la_i_info         iarray
 *
 * Array Element Access
 * --------------------
 *
 *                         la_i_vr_set       iarray, icolumn, ivalue
 *                         la_i_vc_set       iarray, icolumn, irvalue, iivalue
 *                         la_i mr_set       iarray, icolumn, irow, ivalue
 *                         la_i_mc_set       iarray, icolumn, irow, irvalue, iivalue
 * ivalue                  la_i_vr_get       iarray, icolumn
 * irvalue, iivalue        la_i_vc_get       iarray, icolumn, irow
 * ivalue                  la_i_mr_get       iarray, icolumn
 * irvalue, iivalue        la_i_mc_get       iarray, icolumn, irow
 *
 * Single Array Operations
 * -----------------------
 *
 * iarray                  la_i_conjugate    iarray
 * iarray                  la_i_transpose    iarray
 *
 * Scalar Operations
 * -----------------
 *
 * ir                      la_i_norm1        iarray
 * ir                      la_i_norm2        iarray
 * ir                      la_i_norm_inf     iarray
 * ir                      la_i_trace        iarray
 * ir                      la_i_determinant  imatrix
 * iarray                  la_i_scale_r      iarray, ir
 * iarray                  la_i_scale_c      iarray, ireal, iimaginary
 *
 * Elementwise Array-Array Operations
 * ----------------------------------
 *
 * iarray                 la_i_add           iarray_a, iarray_b
 * iarray                 la_i_subtract      iarray_a, iarray_b
 * iarray                 la_i_multiply      iarray_a, iarray_b
 * iarray                 la_i_divide        iarray_a, iarray-b
 *
 * Inner Products
 * --------------
 * 
 * iarray                 la_i_dot           iarray_a, iarray_b
 * iarray                 la_i_invert        iarray
 *
 * Array Decompositions
 * --------------------
 *
 * iarray                 la_i_upper_solve   imatrix [, j_1_diagonal]
 * iarray                 la_i_lower_solve   imatrix [, j_1_diagonal]
 * iarray                 la_i_lu_factor     imatrix, ivector_pivot
 * iarray                 la_i_lu_solve      imatrix, ivector, ivector_pivot
 * imatrix_q, imatrix_r   la_i_qr_factor     imatrix
 * 
 */

extern "C" 
{
  // a-rate, k-rate, FUNC, SPECDAT
#include <csoundCore.h> 
  // PVSDAT
#include <pstream.h>
}


#include <OpcodeBase.hpp>
#include <gmm/gmm.h>
#include <vector>

class LA_I_RV_CREATE : public NoteoffOpcodeBase
{
public:
  // Return value.
  MYFLT *value;
  // Parameters.
  MYFLT *columns;
  MYFLT *;
  // State.
  std::vector<MYFLT> vector;
  int init(CSOUND *)
  {
    vector.resize(size_t(*size));
    value = &vector.front();
  }
  int noteoff(CSOUND *csound)
  {
    vector.resize(0);
  }
};

class LA_I_CV_CREATE : public NoteoffOpcodeBase
{
public:
  // Return value.
  MYFLT *value;
  // Parameters.
  MYFLT *size;
  MYFLT *diagonal;
  // State.
  std::vector< std::complex<MYFLT> > vector;
  int init(CSOUND *)
  {
    vector.resize(size_t(*size));
    value = &vector.front();
  }
  int noteoff(CSOUND *csound)
  {
    vector.resize(0);
  }
};

/*







*/

extern "C" 
{
  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    int status = 0;
    return status;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    return 0;
  }
}
