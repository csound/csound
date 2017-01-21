/**
 * L I N E A R   A L G E B R A   O P C O D E S   F O R   C S O U N D
 * Michael Gogins
 *
 * These opcodes implement many linear algebra operations,
 * from scalar, vector, and matrix arithmetic up to
 * and including QR based eigenvalue decompositions.
 * The opcodes are designed for digital signal processing,
 * and of course other mathematical operations,
 * in the Csound orchestra language.
 *
 * The numerical implementation uses the gmm++ library
 * from http://home.gna.org/getfem/gmm_intro.
 *
 * NOTE: SDFT must be #defined in order to build and use these opcodes.
 *
 *       For applications with f-sig variables, array arithmetic must be
 *       performed only when the f-sig is "current," because f-rate
 *       is some fraction of k-rate; currency can be determined with the
 *       la_k_current_f opcode.
 *
 *       For applications using assignments between real vectors and
 *       a-rate variables, array arithmetic must be performed only when the
 *       vectors are "current", because the size of the vector may be
 *       some integral multiple of ksmps; currency can be determined
 *       by means of the la_k_current_vr opcode.
 *
 * Linear Algebra Data Types
 * -------------------------
 *
 * Mathematical Type  Code  Corresponding Csound Type or Types
 * -----------------  ----  -------------------------------------------------
 * real scalar        r     i-rate or k-rate variable
 * complex scalar     c     pair of i-rate or k-rate variables, e.g. "kr, ki"
 * real vector        vr    i-rate variable holding address of array
 *                    a     a-rate variable
 *                    t     function table number
 * complex vector     vc    i-rate variable holding address of array
 *                    f     fsig variable
 * real matrix        mr    i-rate variable holding address of array
 * complex matrix     mc    i-rate variable holding address of array
 *
 * All arrays are 0-based; the first index iterates rows to give columns,
 * the second index iterates columns to give elements.
 *
 * All arrays are general and dense; banded, Hermitian, symmetric
 * and sparse routines are not implemented.
 *
 * An array can be of type code vr, vc, mr, or mc
 * and is stored in an i-rate object.
 * In orchestra code, an array is passed as a
 * MYFLT i-rate variable that contains the address
 * of the array object, which is actually stored
 * in the allocator opcode instance.
 * Although array variables are i-rate, of course
 * their values and even shapes may change at i-rate or k-rate.
 *
 * All operands must be pre-allocated; except for the creation
 * opcodes, no opcode ever allocates any arrays.
 * This is true even if the array appears on the
 * left-hand side of an opcode! However, some operations
 * may reshape arrays to hold results.
 *
 * Arrays are automatically deallocated when their instrument
 * is deallocated.
 *
 * Not only for more efficient performance,
 * but also to make it easier to remember opcode names,
 * the performance rate, output value types, operation names,
 * and input value types are deterministically
 * encoded into the opcode name:
 * 1. "la" for "linear algebra opcode family".
 * 2. "i" or "k" for performance rate.
 * 3. Type code(s) (see above table) for output value(s),
 *    but only if the type is not implicit from the input values.
 * 4. Operation name: common mathematical name
 *    (preferred) or abbreviation.
 * 5. Type code(s) for input values, if not implicit.
 *
 * For details, see the gmm++ documentation at
 * http://download.gna.org/getfem/doc/gmmuser.pdf.
 *
 * Array Creation
 * --------------
 *
 * ivr                         la_i_vr_create        irows
 * ivc                         la_i_vc_create        irows
 * imr                         la_i_mr_create        irows, icolumns  [, odiagonal]
 * imc                         la_i_mc_create        irows, icolumns
                                                     [, odiagonal_r, odiagonal_i]
 *
 * Array Introspection
 * -------------------
 *
 * irows                       la_i_size_vr          ivr
 * irows                       la_i_size_vc          ivc
 * irows, icolumns             la_i_size_mr          imr
 * irows, icolumns             la_i_size_mc          imc
 *
 * kfiscurrent                 la_k_current_f        fsig
 * kvriscurrent                la_k_current_vr       ivr
 *
 *                             la_i_print_vr         ivr
 *                             la_i_print_vc         ivc
 *                             la_i_print_mr         imr
 *                             la_i_print_mc         imc
 *
 * Array Assignment and Conversion
 * -------------------------------
 *
 * ivr                         la_i_assign_vr        ivr
 * ivr                         la_k_assign_vr        ivr
 * ivc                         la_i_assign_vc        ivc
 * ivc                         la_k_assign_vc        ivr
 * imr                         la_i_assign_mr        imr
 * imr                         la_k_assign_mr        imr
 * imc                         la_i_assign_mc        imc
 * imc                         la_k_assign_mc        imr
 *
 * NOTE: Assignments from tables or fsigs will resize vectors.
 *       Assignments to or from asigs are incremental -- ksmps
 *       frames are copied each kperiod and the array index
 *       wraps around as required.
 *
 * ivr                         la_k_assign_a         asig
 * ivr                         la_i_assign_t         itablenumber
 * ivr                         la_k_assign_t         itablenumber
 * ivc                         la_k_assign_f         fsig
 *
 * asig                        la_k_a_assign         ivr
 * itablenum                   la_i_t_assign         ivr
 * itablenum                   la_k_t_assign         ivr
 * fsig                        la_k_f_assign         ivc
 *
 * Fill Arrays with Random Elements
 * --------------------------------
 *
 * ivr                         la_i_random_vr        [ifill_fraction]
 * ivr                         la_k_random_vr        [kfill_fraction]
 * ivc                         la_i_random_vc        [ifill_fraction]
 * ivc                         la_k_random_vc        [kfill_fraction]
 * imr                         la_i_random_mr        [ifill_fraction]
 * imr                         la_k_random_mr        [kfill_fraction]
 * imc                         la_i_random_mc        [ifill_fraction]
 * imc                         la_k_random_mc        [kfill_fraction]
 *
 * Array Element Access
 * --------------------
 *
 * ivr                         la_i_vr_set           irow, ivalue
 * kvr                         la_k_vr_set           krow, kvalue
 * ivc                         la_i_vc_set           irow, ivalue_r, ivalue_i
 * kvc                         la_k_vc_set           krow, kvalue_r, kvalue_i
 * imr                         la_i mr_set           irow, icolumn, ivalue
 * kmr                         la_k mr_set           krow, kcolumn, ivalue
 * imc                         la_i_mc_set           irow, icolumn, ivalue_r,
                                                                    ivalue_i
 * kmc                         la_k_mc_set           krow, kcolumn, kvalue_r,
                                                                    kvalue_i
 *
 * ivalue                      la_i_get_vr           ivr, irow
 * kvalue                      la_k_get_vr           ivr, krow,
 * ivalue_r, ivalue_i          la_i_get_vc           ivc, irow
 * kvalue_r, kvalue_i          la_k_get_vc           ivc, krow
 * ivalue                      la_i_get_mr           imr, irow, icolumn
 * kvalue                      la_k_get_mr           imr, krow, kcolumn
 * ivalue_r, ivalue_i          la_i_get_mc           imc, irow, icolumn
 * kvalue_r, kvalue_i          la_k_get_mc           imc, krow, kcolumn
 *
 * Single Array Operations
 * -----------------------
 *
 * imr                         la_i_transpose_mr     imr
 * imr                         la_k_transpose_mr     imr
 * imc                         la_i_transpose_mc     imc
 * imc                         la_k_transpose_mc     imc

 * ivr                         la_i_conjugate_vr     ivr
 * ivr                         la_k_conjugate_vr     ivr
 * ivc                         la_i_conjugate_vc     ivc
 * ivc                         la_k_conjugate_vc     ivc
 * imr                         la_i_conjugate_mr     imr
 * imr                         la_k_conjugate_mr     imr
 * imc                         la_i_conjugate_mc     imc
 * imc                         la_k_conjugate_mc     imc
 *
 * Scalar Operations
 * -----------------
 *
 * ir                          la_i_norm1_vr         ivr
 * kr                          la_k_norm1_vr         ivc
 * ir                          la_i_norm1_vc         ivc
 * kr                          la_k_norm1_vc         ivc
 * ir                          la_i_norm1_mr         imr
 * kr                          la_k_norm1_mr         imr
 * ir                          la_i_norm1_mc         imc
 * kr                          la_k_norm1_mc         imc
 *
 * ir                          la_i_norm_euclid_vr   ivr
 * kr                          la_k_norm_euclid_vr   ivr
 * ir                          la_i_norm_euclid_vc   ivc
 * kr                          la_k_norm_euclid_vc   ivc
 * ir                          la_i_norm_euclid_mr   mvr
 * kr                          la_k_norm_euclid_mr   mvr
 * ir                          la_i_norm_euclid_mc   mvc
 * kr                          la_k_norm_euclid_mc   mvc
 *
 * ir                          la_i_distance_vr      ivr
 * kr                          la_k_distance_vr      ivr
 * ir                          la_i_distance_vc      ivc
 * kr                          la_k_distance_vc      ivc
 *
 * ir                          la_i_norm_max         imr
 * kr                          la_k_norm_max         imc
 * ir                          la_i_norm_max         imr
 * kr                          la_k_norm_max         imc
 *
 * ir                          la_i_norm_inf_vr      ivr
 * kr                          la_k_norm_inf_vr      ivr
 * ir                          la_i_norm_inf_vc      ivc
 * kr                          la_k_norm_inf_vc      ivc
 * ir                          la_i_norm_inf_mr      imr
 * kr                          la_k_norm_inf_mr      imr
 * ir                          la_i_norm_inf_mc      imc
 * kr                          la_k_norm_inf_mc      imc
 *
 * ir                          la_i_trace_mr         imr
 * kr                          la_k_trace_mr         imr
 * ir, ii                      la_i_trace_mc         imc
 * kr, ki                      la_k_trace_mc         imc
 *
 * ir                          la_i_lu_det           imr
 * kr                          la_k_lu_det           imr
 * ir                          la_i_lu_det           imc
 * kr                          la_k_lu_det           imc
 *
 * Elementwise Array-Array Operations
 * ----------------------------------
 *
 * ivr                         la_i_add_vr           ivr_a, ivr_b
 * ivc                         la_k_add_vc           ivc_a, ivc_b
 * imr                         la_i_add_mr           imr_a, imr_b
 * imc                         la_k_add_mc           imc_a, imc_b
 *
 * ivr                         la_i_subtract_vr      ivr_a, ivr_b
 * ivc                         la_k_subtract_vc      ivc_a, ivc_b
 * imr                         la_i_subtract_mr      imr_a, imr_b
 * imc                         la_k_subtract_mc      imc_a, imc_b
 *
 * ivr                         la_i_multiply_vr      ivr_a, ivr_b
 * ivc                         la_k_multiply_vc      ivc_a, ivc_b
 * imr                         la_i_multiply_mr      imr_a, imr_b
 * imc                         la_k_multiply_mc      imc_a, imc_b
 *
 * ivr                         la_i_divide_vr        ivr_a, ivr_b
 * ivc                         la_k_divide_vc        ivc_a, ivc_b
 * imr                         la_i_divide_mr        imr_a, imr_b
 * imc                         la_k_divide_mc        imc_a, imc_b
 *
 * Inner Products
 * --------------
 *
 * ir                          la_i_dot_vr           ivr_a, ivr_b
 * kr                          la_k_dot_vr           ivr_a, ivr_b
 * ir, ii                      la_i_dot_vc           ivc_a, ivc_b
 * kr, ki                      la_k_dot_vc           ivc_a, ivc_b
 *
 * imr                         la_i_dot_mr           imr_a, imr_b
 * imr                         la_k_dot_mr           imr_a, imr_b
 * imc                         la_i_dot_mc           imc_a, imc_b
 * imc                         la_k_dot_mc           imc_a, imc_b
 *
 * ivr                         la_i_dot_mr_vr        imr_a, ivr_b
 * ivr                         la_k_dot_mr_vr        imr_a, ivr_b
 * ivc                         la_i_dot_mc_vc        imc_a, ivc_b
 * ivc                         la_k_dot_mc_vc        imc_a, ivc_b
 *
 * Matrix Inversion
 * ----------------
 *
 * imr, icondition             la_i_invert_mr        imr
 * imr, kcondition             la_k_invert_mr        imr
 * imc, icondition             la_i_invert_mc        imc
 * imc, kcondition             la_k_invert_mc        imc
 *
 * Matrix Decompositions and Solvers
 * ---------------------------------
 *
 * ivr                         la_i_upper_solve_mr   imr [, j_1_diagonal]
 * ivr                         la_k_upper_solve_mr   imr [, j_1_diagonal]
 * ivc                         la_i_upper_solve_mc   imc [, j_1_diagonal]
 * ivc                         la_k_upper_solve_mc   imc [, j_1_diagonal]
 *
 * ivr                         la_i_lower_solve_mr   imr [, j_1_diagonal]
 * ivr                         la_k_lower_solve_mr   imr [, j_1_diagonal]
 * ivc                         la_i_lower_solve_mc   imc [, j_1_diagonal]
 * ivc                         la_k_lower_solve_mc   imc [, j_1_diagonal]
 *
 * imr, ivr_pivot, isize       la_i_lu_factor_mr     imr
 * imr, ivr_pivot, ksize       la_k_lu_factor_mr     imr
 * imc, ivr_pivot, isize       la_i_lu_factor_mc     imc
 * imc, ivr_pivot, ksize       la_k_lu_factor_mc     imc
 *
 * ivr_x                       la_i_lu_solve_mr      imr, ivr_b
 * ivr_x                       la_k_lu_solve_mr      imr, ivr_b
 * ivc_x                       la_i_lu_solve_mc      imc, ivc_b
 * ivc_x                       la_k_lu_solve_mc      imc, ivc_b
 *
 * imr_q, imr_r                la_i_qr_factor_mr     imr
 * imr_q, imr_r                la_k_qr_factor_mr     imr
 * imc_q, imc_r                la_i_qr_factor_mc     imc
 * imc_q, imc_r                la_k_qr_factor_mc     imc
 *
 * ivr_eig_vals                la_i_qr_eigen_mr      imr, i_tolerance
 * ivr_eig_vals                la_k_qr_eigen_mr      imr, k_tolerance
 * ivr_eig_vals                la_i_qr_eigen_mc      imc, i_tolerance
 * ivr_eig_vals                la_k_qr_eigen_mc      imc, k_tolerance
 *
 * NOTE: Matrix must be Hermitian in order to compute eigenvectors.
 *
 * ivr_eig_vals, imr_eig_vecs  la_i_qr_sym_eigen_mr  imr, i_tolerance
 * ivr_eig_vals, imr_eig_vecs  la_k_qr_sym_eigen_mr  imr, k_tolerance
 * ivc_eig_vals, imc_eig_vecs  la_i_qr_sym_eigen_mc  imc, i_tolerance
 * ivc_eig_vals, imc_eig_vecs  la_k_qr_sym_eigen_mc  imc, k_tolerance
 *
 */

extern "C"
{
  // a-rate, k-rate, FUNC, SPECDAT
#include <csdl.h>
  // PVSDAT
#include <pstream.h>
}

#ifdef ST
#undef ST
#endif

#ifdef WR
#undef WR
#endif

#include <OpcodeBase.hpp>
#include <complex>
#include <sstream>
#include <vector>
#include <gmm/gmm.h>

using namespace csound;

/**
 * Template union for safely and efficiently
 * typecasting the value of a MYFLT variable
 * to the address of an array, and vice versa.
 */
template<typename A, typename F>
struct ArrayCaster
{
  union {
    A* a;
    F f;
  };
};

/**
 * Safely and efficiently typecast the address
 * of an array to the value of a MYFLT variable.
 */
template<typename A, typename F> void tof(A *a, F *f)
{
  ArrayCaster<A, F> arrayCaster;
  arrayCaster.a = a;
  *f = arrayCaster.f;
};

/**
 * Safely and efficiently typecast the value of a
 * a MYFLT variable to the address of an array.
 */
template<typename A, typename F> void toa(F *f, A *&a)
{
  ArrayCaster<A, F> arrayCaster;
  arrayCaster.f = *f;
  a = arrayCaster.a;
};

class la_i_vr_create_t : public OpcodeNoteoffBase<la_i_vr_create_t>
{
public:
  MYFLT *i_vr;
  MYFLT *i_rows;
  std::vector<MYFLT> vr;
  int init(CSOUND *)
  {
    vr.resize(size_t(*i_rows));
    tof(this, i_vr);
    return OK;
  }
  int noteoff(CSOUND *)
  {
    vr.resize(0);
    return OK;
  }
};

class la_i_vc_create_t : public OpcodeNoteoffBase<la_i_vc_create_t>
{
public:
  MYFLT *i_vc;
  MYFLT *i_rows;
  std::vector< std::complex<MYFLT> > vc;
  int init(CSOUND *)
  {
    vc.resize(size_t(*i_rows));
    tof(this, i_vc);
    return OK;
  }
  int noteoff(CSOUND *)
  {
    vc.resize(0);
    return OK;
  }
};

class la_i_mr_create_t : public OpcodeNoteoffBase<la_i_mr_create_t>
{
public:
  MYFLT *i_mr;
  MYFLT *i_rows;
  MYFLT *i_columns;
  MYFLT *o_diagonal;
  gmm::dense_matrix<MYFLT> mr;
  int init(CSOUND *)
  {
    gmm::resize(mr, size_t(*i_rows), size_t(*i_columns));
    if (*o_diagonal) {
      for (size_t i = 0, n = size_t(*i_rows); i < n; ++i) {
        mr(i, i) = *o_diagonal;
      }
    }
    tof(this, i_mr);
    return OK;
  }
  int noteoff(CSOUND *)
  {
    mr.resize(0, 0);
    return OK;
  }
};

class la_i_mc_create_t : public OpcodeNoteoffBase<la_i_mc_create_t>
{
public:
  MYFLT *i_mc;
  MYFLT *i_rows;
  MYFLT *i_columns;
  MYFLT *o_diagonal_r;
  MYFLT *o_diagonal_i;
  gmm::dense_matrix< std::complex<MYFLT> > mc;
  int init(CSOUND *)
  {
    gmm::resize(mc, size_t(*i_rows), size_t(*i_columns));
    if (*o_diagonal_r || *o_diagonal_i) {
      for (size_t i = 0, n = size_t(*i_rows); i < n; ++i) {
        mc(i, i) = std::complex<MYFLT>(*o_diagonal_r, *o_diagonal_i);
      }
    }
    tof(this, i_mc);
    return OK;
  }
  int noteoff(CSOUND *)
  {
    mc.resize(0, 0);
    return OK;
  }
};

class la_i_size_vr_t : public OpcodeBase<la_i_size_vr_t>
{
public:
  MYFLT *i_size;
  MYFLT *i_vr;
  int init(CSOUND *)
  {
    la_i_vr_create_t *array = 0;
    toa(i_vr, array);
    *i_size = (MYFLT) gmm::vect_size(array->vr);
    return OK;
  }
};

class la_i_size_vc_t : public OpcodeBase<la_i_size_vc_t>
{
public:
  MYFLT *i_size;
  MYFLT *i_vc;
  int init(CSOUND *)
  {
    la_i_vc_create_t *array = 0;
    toa(i_vc, array);
    *i_size = (MYFLT) gmm::vect_size(array->vc);
    return OK;
  }
};

class la_i_size_mr_t : public OpcodeBase<la_i_size_mr_t>
{
public:
  MYFLT *i_rows;
  MYFLT *i_columns;
  MYFLT *i_mr;
  int init(CSOUND *)
  {
    la_i_mr_create_t *array = 0;
    toa(i_mr, array);
    *i_rows = (MYFLT) gmm::mat_nrows(array->mr);
    *i_columns  = (MYFLT) gmm::mat_ncols(array->mr);
    return OK;
  }
};

class la_i_size_mc_t : public OpcodeBase<la_i_size_mc_t>
{
public:
  MYFLT *i_rows;
  MYFLT *i_columns;
  MYFLT *i_mc;
  int init(CSOUND *)
  {
    la_i_mc_create_t *array = 0;
    toa(i_mc, array);
    *i_rows = (MYFLT) gmm::mat_nrows(array->mc);
    *i_columns  = (MYFLT) gmm::mat_ncols(array->mc);
    return OK;
  }
};

/**
 * Return 1 if the input f-sig is current
 * (i.e., if its value might change in the next kperiod);
 * return 0 if the input f-sig is not current.
 *
 * Example:
 *
 * kcurrent la_k_current_f fsig
 * if (kcurrent == 1.0) then
 * ; arithmetic on fsig here...
 * endif
 */
class la_k_current_f_t : public OpcodeBase<la_k_current_f_t>
{
public:
  MYFLT *k_current;
  PVSDAT *f_sig;
  size_t last_frame;
  int init(CSOUND *)
  {
    last_frame = f_sig->framecount;
    return OK;
  }
  int kontrol(CSOUND *)
  {
    if (last_frame < f_sig->framecount) {
      last_frame = f_sig->framecount;
      *k_current = 1.0;
    } else {
      *k_current = 0.0;
    }
    return OK;
  }
};

/**
 * Return 1 if the real vector is current (i.e., its
 * current index equals 0); return 0 if the input
 * real vector is not current. A real vector is current
 * in the very first kperiod of performance,
 * and at each subsequent kperiod where ksmps has
 * accumulated past the end of the vector and
 * has wrapped back to index 0. For this to be
 * possible, the size of the vector must be an integral
 * multiple of ksmps.
 *
 * Example:
 *
 * ; FIRST, assignments from a-rate variables to vectors.
 * kcurrent la_k_current_vr ivr
 * if (kcurrent == 1.0) then
 * ; SECOND, arithmetic with current vectors.
 * endif
 * ; THIRD, assignments from vectors to a-rate variables.
 */

class la_k_current_vr_t : public OpcodeBase<la_k_current_vr_t>
{
public:
  MYFLT *k_current;
  MYFLT *rhs_ivr;
  la_i_vr_create_t *rhs;
  size_t ksmps;
  size_t vector_size;
  int init(CSOUND *csound)
  {
    rhs = 0;
    toa(rhs_ivr, rhs);
    ksmps = opds.insdshead->ksmps;
    vector_size = gmm::vect_size(rhs->vr);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    size_t frame_count = opds.insdshead->kcounter * ksmps;
    size_t index = frame_count % vector_size;
    if (index == 0) {
      *k_current = 1.0;
    } else {
      *k_current = 0.0;
    }
    return OK;
  }
};

class la_i_print_vr_t : public OpcodeBase<la_i_print_vr_t>
{
public:
  MYFLT *i_vr;
  int init(CSOUND *csound)
  {
    la_i_vr_create_t *array = 0;
    toa(i_vr, array);
    std::ostringstream stream;
    stream << array->vr << std::endl;
    csound->Message(csound, stream.str().c_str());
    return OK;
  }
};

class la_i_print_vc_t : public OpcodeBase<la_i_print_vc_t>
{
public:
  MYFLT *i_vc;
  int init(CSOUND *csound)
  {
    la_i_vc_create_t *array = 0;
    toa(i_vc, array);
    std::ostringstream stream;
    stream << array->vc << std::endl;
    csound->Message(csound, stream.str().c_str());
    return OK;
  }
};

class la_i_print_mr_t : public OpcodeBase<la_i_print_mr_t>
{
public:
  MYFLT *i_mr;
  int init(CSOUND *csound)
  {
    la_i_mr_create_t *array = 0;
    toa(i_mr, array);
    std::ostringstream stream;
    stream << array->mr << std::endl;
    csound->Message(csound, stream.str().c_str());
    return OK;
  }
};

class la_i_print_mc_t : public OpcodeBase<la_i_print_mc_t>
{
public:
  MYFLT *i_mc;
  int init(CSOUND *csound)
  {
    la_i_mc_create_t *array = 0;
    toa(i_mc, array);
    std::ostringstream stream;
    stream << array->mc << std::endl;
    csound->Message(csound, stream.str().c_str());
    return OK;
  }
};

class la_i_assign_vr_t : public OpcodeBase<la_i_assign_vr_t>
{
public:
  MYFLT *i_vr_lhs;
  MYFLT *i_vr_rhs;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_vr_lhs, lhs);
    toa(i_vr_rhs, rhs);
    gmm::copy(rhs->vr, lhs->vr);
    return OK;
  }
};

class la_k_assign_vr_t : public OpcodeBase<la_k_assign_vr_t>
{
public:
  MYFLT *i_vr_lhs;
  MYFLT *i_vr_rhs;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_vr_lhs, lhs);
    toa(i_vr_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::copy(rhs->vr, lhs->vr);
    return OK;
  }
};

class la_i_assign_vc_t : public OpcodeBase<la_i_assign_vc_t>
{
public:
  MYFLT *i_vc_lhs;
  MYFLT *i_vc_rhs;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_vc_lhs, lhs);
    toa(i_vc_rhs, rhs);
    gmm::copy(rhs->vc, lhs->vc);
    return OK;
  }
};

class la_k_assign_vc_t : public OpcodeBase<la_k_assign_vc_t>
{
public:
  MYFLT *i_vc_lhs;
  MYFLT *i_vc_rhs;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_vc_lhs, lhs);
    toa(i_vc_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::copy(rhs->vc, lhs->vc);
    return OK;
  }
};

class la_i_assign_mr_t : public OpcodeBase<la_i_assign_mr_t>
{
public:
  MYFLT *i_mr_lhs;
  MYFLT *i_mr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_mr_lhs, lhs);
    toa(i_mr_rhs, rhs);
    gmm::copy(rhs->mr, lhs->mr);
    return OK;
  }
};

class la_k_assign_mr_t : public OpcodeBase<la_k_assign_mr_t>
{
public:
  MYFLT *i_mr_lhs;
  MYFLT *i_mr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_mr_lhs, lhs);
    toa(i_mr_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::copy(rhs->mr, lhs->mr);
    return OK;
  }
};

class la_i_assign_mc_t : public OpcodeBase<la_i_assign_mc_t>
{
public:
  MYFLT *i_mc_lhs;
  MYFLT *i_mc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_mc_lhs, lhs);
    toa(i_mc_rhs, rhs);
    gmm::copy(rhs->mc, lhs->mc);
    return OK;
  }
};

class la_k_assign_mc_t : public OpcodeBase<la_k_assign_mc_t>
{
public:
  MYFLT *i_mc_lhs;
  MYFLT *i_mc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *csound)
  {
    toa(i_mc_lhs, lhs);
    toa(i_mc_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::copy(rhs->mc, lhs->mc);
    return OK;
  }
};

class la_k_assign_a_t : public OpcodeBase<la_k_assign_a_t>
{
public:
  MYFLT *i_vr;
  MYFLT *a_a;
  la_i_vr_create_t *lhs;
  size_t ksmps;
  size_t vector_size;
  int init(CSOUND *csound)
  {
    toa(i_vr, lhs);
    ksmps = opds.insdshead->ksmps;
    vector_size = gmm::vect_size(lhs->vr);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    uint32_t offset = opds.insdshead->ksmps_offset;
    uint32_t early  = opds.insdshead->ksmps_no_end;
    size_t frame_count = opds.insdshead->ksmps * ksmps;
    size_t array_i = frame_count % vector_size;
    if (UNLIKELY(early)) ksmps -= early;
    for (size_t i = offset; i < ksmps; ++i, ++array_i) {
      lhs->vr[array_i] = a_a[i];
    }
    return OK;
  }
};

class la_i_assign_t_t : public OpcodeBase<la_i_assign_t_t>
{
public:
  MYFLT *i_vr;
  MYFLT *i_tablenumber;
  la_i_vr_create_t *lhs;
  int tablenumber;
  int n;
  int init(CSOUND *csound)
  {
    toa(i_vr, lhs);
    tablenumber = int(std::floor(*i_tablenumber));
    n = csound->TableLength(csound, tablenumber);
    gmm::resize(lhs->vr, n);
    for (int i = 0; i < n; ++i) {
      lhs->vr[i] = csound->TableGet(csound, tablenumber, i);
    }
    return OK;
  }
};

class la_k_assign_t_t : public OpcodeBase<la_k_assign_t_t>
{
public:
  MYFLT *i_vr;
  MYFLT *i_tablenumber;
  la_i_vr_create_t *lhs;
  int tablenumber;
  int n;
  int init(CSOUND *csound)
  {
    toa(i_vr, lhs);
    tablenumber = int(std::floor(*i_tablenumber));
    n = csound->TableLength(csound, tablenumber);
    gmm::resize(lhs->vr, n);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    for (int i = 0; i < n; ++i) {
      lhs->vr[i] = csound->TableGet(csound, tablenumber, i);
    }
    return OK;
  }
};

class la_k_assign_f_t : public OpcodeBase<la_k_assign_f_t>
{
public:
  MYFLT *i_vc;
  PVSDAT *f_fsig;
  la_i_vc_create_t *lhs;
  int n;
  std::complex<MYFLT> *f;
  int init(CSOUND *csound)
  {
    toa(i_vc, lhs);
    n = f_fsig->N;
    // std::complex<MYFLT> has the same layout as CMPLX.
    f = (std::complex<MYFLT> *)f_fsig->frame.auxp;
    gmm::resize(lhs->vc, n);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    for (int i = 0; i < n; ++i) {
      lhs->vc[i] = f[i];
    }
    return OK;
  }
};

class la_k_a_assign_t : public OpcodeBase<la_k_a_assign_t>
{
public:
  MYFLT *a_a;
  MYFLT *i_vr;
  la_i_vr_create_t *rhs;
  size_t ksmps;
  int init(CSOUND *csound)
  {
    toa(i_vr, rhs);
    ksmps = opds.insdshead->ksmps;
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    uint32_t offset = opds.insdshead->ksmps_offset;
    uint32_t early  = opds.insdshead->ksmps_no_end;
    memset(a_a, '\0', offset*sizeof(MYFLT));
    size_t frameCount = opds.insdshead->kcounter * opds.insdshead->ksmps;
    size_t vectorSize = gmm::vect_size(rhs->vr);
    size_t array_i = frameCount % vectorSize;
    if (UNLIKELY(early)) ksmps -= early;
    for (size_t i = 0; i < ksmps; ++i, ++array_i) {
      a_a[i] = rhs->vr[array_i];
    }
    return OK;
  }
};

class la_i_t_assign_t : public OpcodeBase<la_i_t_assign_t>
{
public:
  MYFLT *i_tablenumber;
  MYFLT *i_vr;
  la_i_vr_create_t *rhs;
  int tablenumber;
  int n;
  int init(CSOUND *csound)
  {
    toa(i_vr, rhs);
    tablenumber = int(std::floor(*i_tablenumber));
    n = csound->TableLength(csound, tablenumber);
    gmm::resize(rhs->vr, n);
    for (int i = 0; i < n; ++i) {
      csound->TableSet(csound, tablenumber, i, rhs->vr[i]);
    }
    return OK;
  }
};

class la_k_t_assign_t : public OpcodeBase<la_k_t_assign_t>
{
public:
  MYFLT *i_tablenumber;
  MYFLT *i_vr;
  la_i_vr_create_t *rhs;
  int tablenumber;
  int n;
  int init(CSOUND *csound)
  {
    toa(i_vr, rhs);
    tablenumber = int(std::floor(*i_tablenumber));
    n = csound->TableLength(csound, tablenumber);
    gmm::resize(rhs->vr, n);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    for (int i = 0; i < n; ++i) {
      csound->TableSet(csound, tablenumber, i, rhs->vr[i]);
    }
    return OK;
  }
};

class la_k_f_assign_t : public OpcodeBase<la_k_f_assign_t>
{
public:
  PVSDAT *f_fsig;
  MYFLT *i_vc;
  la_i_vc_create_t *rhs;
  int n;
  std::complex<MYFLT> *f;
  int init(CSOUND *csound)
  {
    toa(i_vc, rhs);
    n = f_fsig->N;
    // std::complex<MYFLT> has the same layout as CMPLX.
    f = (std::complex<MYFLT> *)f_fsig->frame.auxp;
    gmm::resize(rhs->vc, n);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    for (int i = 0; i < n; ++i) {
      f[i] = rhs->vc[i];
    }
    return OK;
  }
};

class la_i_random_vr_t : public OpcodeBase<la_i_random_vr_t>
{
public:
  MYFLT *i_vr_lhs;
  MYFLT *i_fraction;
  la_i_vr_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_vr_lhs, lhs);
    gmm::fill_random(lhs->vr, *i_fraction);
    return OK;
  }
};

class la_k_random_vr_t : public OpcodeBase<la_k_random_vr_t>
{
public:
  MYFLT *i_vr_lhs;
  MYFLT *i_fraction;
  la_i_vr_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_vr_lhs, lhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::fill_random(lhs->vr, *i_fraction);
    return OK;
  }
};

class la_i_random_vc_t : public OpcodeBase<la_i_random_vc_t>
{
public:
  MYFLT *i_vc_lhs;
  MYFLT *i_fraction;
  la_i_vc_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_vc_lhs, lhs);
    gmm::fill_random(lhs->vc, *i_fraction);
    return OK;
  }
};

class la_k_random_vc_t : public OpcodeBase<la_k_random_vc_t>
{
public:
  MYFLT *i_vc_lhs;
  MYFLT *i_fraction;
  la_i_vc_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_vc_lhs, lhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::fill_random(lhs->vc, *i_fraction);
    return OK;
  }
};

class la_i_random_mr_t : public OpcodeBase<la_i_random_mr_t>
{
public:
  MYFLT *i_mr_lhs;
  MYFLT *i_fraction;
  la_i_mr_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_mr_lhs, lhs);
    gmm::fill_random(lhs->mr, *i_fraction);
    return OK;
  }
};

class la_k_random_mr_t : public OpcodeBase<la_k_random_mr_t>
{
public:
  MYFLT *i_mr_lhs;
  MYFLT *i_fraction;
  la_i_mr_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_mr_lhs, lhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::fill_random(lhs->mr, *i_fraction);
    return OK;
  }
};

class la_i_random_mc_t : public OpcodeBase<la_i_random_mc_t>
{
public:
  MYFLT *i_mc_lhs;
  MYFLT *i_fraction;
  la_i_mc_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_mc_lhs, lhs);
    gmm::fill_random(lhs->mc, *i_fraction);
    return OK;
  }
};

class la_k_random_mc_t : public OpcodeBase<la_k_random_mc_t>
{
public:
  MYFLT *i_mc_lhs;
  MYFLT *i_fraction;
  la_i_mc_create_t *lhs;
  int init(CSOUND *csound)
  {
    toa(i_mc_lhs, lhs);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    gmm::fill_random(lhs->mc, *i_fraction);
    return OK;
  }
};

class la_i_vr_set_t : public OpcodeBase<la_i_vr_set_t>
{
public:
  MYFLT *i_vr;
  MYFLT *i_row;
  MYFLT *i_value;
  la_i_vr_create_t *vr;
  int init(CSOUND *)
  {
    toa(i_vr, vr);
    vr->vr[size_t(*i_row)] = *i_value;
    return OK;
  }
};

class la_k_vr_set_t : public OpcodeBase<la_k_vr_set_t>
{
public:
  MYFLT *i_vr;
  MYFLT *k_row;
  MYFLT *k_value;
  la_i_vr_create_t *vr;
  int init(CSOUND *)
  {
    toa(i_vr, vr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    vr->vr[size_t(*k_row)] = *k_value;
    return OK;
  }
};

class la_i_vc_set_t : public OpcodeBase<la_i_vc_set_t>
{
public:
  MYFLT *i_vc;
  MYFLT *i_row;
  MYFLT *i_value_r;
  MYFLT *i_value_i;
  la_i_vc_create_t *vc;
  int init(CSOUND *)
  {
    toa(i_vc, vc);
    vc->vc[size_t(*i_row)] = std::complex<MYFLT>(*i_value_r, *i_value_i);
    return OK;
  }
};

class la_k_vc_set_t : public OpcodeBase<la_k_vc_set_t>
{
public:
  MYFLT *i_vc;
  MYFLT *k_row;
  MYFLT *k_value_r;
  MYFLT *k_value_i;
  la_i_vc_create_t *vc;
  int init(CSOUND *)
  {
    toa(i_vc, vc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    vc->vc[size_t(*k_row)] = std::complex<MYFLT>(*k_value_r, *k_value_i);
    return OK;
  }
};

class la_i_mr_set_t : public OpcodeBase<la_i_mr_set_t>
{
public:
  MYFLT *i_mr;
  MYFLT *i_row;
  MYFLT *i_column;
  MYFLT *i_value;
  la_i_mr_create_t *mr;
  int init(CSOUND *)
  {
    toa(i_mr, mr);
    mr->mr(size_t(*i_row), size_t(*i_column)) = *i_value;
    return OK;
  }
};

class la_k_mr_set_t : public OpcodeBase<la_k_mr_set_t>
{
public:
  MYFLT *i_mr;
  MYFLT *k_row;
  MYFLT *k_column;
  MYFLT *k_value;
  la_i_mr_create_t *mr;
  int init(CSOUND *)
  {
    toa(i_mr, mr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    mr->mr(size_t(*k_row), size_t(*k_column)) = *k_value;
    return OK;
  }
};

class la_i_mc_set_t : public OpcodeBase<la_i_mc_set_t>
{
public:
  MYFLT *i_mc;
  MYFLT *i_row;
  MYFLT *i_column;
  MYFLT *i_value_r;
  MYFLT *i_value_i;
  la_i_mc_create_t *mc;
  int init(CSOUND *)
  {
    toa(i_mc, mc);
    mc->mc(size_t(*i_row), size_t(*i_column)) =
      std::complex<MYFLT>(*i_value_r, *i_value_i);
    return OK;
  }
};

class la_k_mc_set_t : public OpcodeBase<la_k_mc_set_t>
{
public:
  MYFLT *i_mc;
  MYFLT *k_row;
  MYFLT *k_column;
  MYFLT *k_value_r;
  MYFLT *k_value_i;
  la_i_mc_create_t *mc;
  int init(CSOUND *)
  {
    toa(i_mc, mc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    mc->mc(size_t(*k_row), size_t(*k_column)) =
      std::complex<MYFLT>(*k_value_r, *k_value_i);
    return OK;
  }
};

class la_i_get_vr_t : public OpcodeBase<la_i_get_vr_t>
{
public:
  MYFLT *i_value;
  MYFLT *i_vr;
  MYFLT *i_row;
  la_i_vr_create_t *vr;
  int init(CSOUND *)
  {
    toa(i_vr, vr);
    *i_value = vr->vr[size_t(*i_row)];
    return OK;
  }
};

class la_k_get_vr_t : public OpcodeBase<la_k_get_vr_t>
{
public:
  MYFLT *k_value;
  MYFLT *i_vr;
  MYFLT *k_row;
  la_i_vr_create_t *vr;
  int init(CSOUND *)
  {
    toa(i_vr, vr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    *k_value = vr->vr[size_t(*k_row)];
    return OK;
  }
};

class la_i_get_vc_t : public OpcodeBase<la_i_get_vc_t>
{
public:
  MYFLT *i_value_r;
  MYFLT *i_value_i;
  MYFLT *i_vc;
  MYFLT *i_row;
  la_i_vc_create_t *vc;
  int init(CSOUND *)
  {
    toa(i_vc, vc);
    const std::complex<MYFLT> &lhs = vc->vc[size_t(*i_row)];
    *i_value_r = lhs.real();
    *i_value_i = lhs.imag();
    return OK;
  }
};

class la_k_get_vc_t : public OpcodeBase<la_k_get_vc_t>
{
public:
  MYFLT *k_value_r;
  MYFLT *k_value_i;
  MYFLT *i_vc;
  MYFLT *k_row;
  la_i_vc_create_t *vc;
  int init(CSOUND *)
  {
    toa(i_vc, vc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    const std::complex<MYFLT> &lhs = vc->vc[size_t(*k_row)];
    *k_value_r = lhs.real();
    *k_value_i = lhs.imag();
    return OK;
  }
};


class la_i_get_mr_t : public OpcodeBase<la_i_get_mr_t>
{
public:
  MYFLT *i_value;
  MYFLT *i_mr;
  MYFLT *i_row;
  MYFLT *i_column;
  la_i_mr_create_t *mr;
  int init(CSOUND *)
  {
    toa(i_mr, mr);
    *i_value = mr->mr(size_t(*i_row), size_t(*i_column));
    return OK;
  }
};

class la_k_get_mr_t : public OpcodeBase<la_k_get_mr_t>
{
public:
  MYFLT *k_value;
  MYFLT *i_mr;
  MYFLT *k_row;
  MYFLT *k_column;
  la_i_mr_create_t *mr;
  int init(CSOUND *)
  {
    toa(i_mr, mr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    *k_value = mr->mr(size_t(*k_row), size_t(*k_column));
    return OK;
  }
};

class la_i_get_mc_t : public OpcodeBase<la_i_get_mc_t>
{
public:
  MYFLT *i_value_r;
  MYFLT *i_value_i;
  MYFLT *i_mc;
  MYFLT *i_row;
  MYFLT *i_column;
  la_i_mc_create_t *mc;
  int init(CSOUND *)
  {
    toa(i_mc, mc);
    const std::complex<MYFLT> &lhs = mc->mc(size_t(*i_row), size_t(*i_column));
    *i_value_r = lhs.real();
    *i_value_i = lhs.imag();
    return OK;
  }
};

class la_k_get_mc_t : public OpcodeBase<la_k_get_mc_t>
{
public:
  MYFLT *k_value_r;
  MYFLT *k_value_i;
  MYFLT *i_mc;
  MYFLT *k_row;
  MYFLT *k_column;
  la_i_mc_create_t *mc;
  int init(CSOUND *)
  {
    toa(i_mc, mc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    const std::complex<MYFLT> &lhs = mc->mc(size_t(*k_row), size_t(*k_column));
    *k_value_r = lhs.real();
    *k_value_i = lhs.imag();
     return OK;
  }
};

class la_i_transpose_mr_t : public OpcodeBase<la_i_transpose_mr_t>
{
public:
  MYFLT *imr_lhs;
  MYFLT *imr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imr_lhs, lhs);
    toa(imr_rhs, rhs);
    gmm::copy(gmm::transposed(rhs->mr), lhs->mr);
    return OK;
  }
};

class la_k_transpose_mr_t : public OpcodeBase<la_k_transpose_mr_t>
{
public:
  MYFLT *imr_lhs;
  MYFLT *imr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imr_lhs, lhs);
    toa(imr_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(gmm::transposed(rhs->mr), lhs->mr);
    return OK;
  }
};

class la_i_transpose_mc_t : public OpcodeBase<la_i_transpose_mc_t>
{
public:
  MYFLT *imc_lhs;
  MYFLT *imc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imc_lhs, lhs);
    toa(imc_rhs, rhs);
    gmm::copy(gmm::transposed(rhs->mc), lhs->mc);
    return OK;
  }
};

class la_k_transpose_mc_t : public OpcodeBase<la_k_transpose_mc_t>
{
public:
  MYFLT *imc_lhs;
  MYFLT *imc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imc_lhs, lhs);
    toa(imc_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(gmm::transposed(rhs->mc), lhs->mc);
    return OK;
  }
};

class la_i_conjugate_vr_t : public OpcodeBase<la_i_conjugate_vr_t>
{
public:
  MYFLT *ivr_lhs;
  MYFLT *ivr_rhs;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(ivr_lhs, lhs);
    toa(ivr_rhs, rhs);
    gmm::copy(gmm::conjugated(rhs->vr), lhs->vr);
    return OK;
  }
};

class la_k_conjugate_vr_t : public OpcodeBase<la_k_conjugate_vr_t>
{
public:
  MYFLT *ivr_lhs;
  MYFLT *ivr_rhs;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(ivr_lhs, lhs);
    toa(ivr_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(gmm::conjugated(rhs->vr), lhs->vr);
    return OK;
  }
};

class la_i_conjugate_vc_t : public OpcodeBase<la_i_conjugate_vc_t>
{
public:
  MYFLT *ivc_lhs;
  MYFLT *ivc_rhs;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(ivc_lhs, lhs);
    toa(ivc_rhs, rhs);
    gmm::copy(gmm::conjugated(rhs->vc), lhs->vc);
    return OK;
  }
};

class la_k_conjugate_vc_t : public OpcodeBase<la_k_conjugate_vc_t>
{
public:
  MYFLT *ivc_lhs;
  MYFLT *ivc_rhs;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(ivc_lhs, lhs);
    toa(ivc_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(gmm::conjugated(rhs->vc), lhs->vc);
    return OK;
  }
};

class la_i_conjugate_mr_t : public OpcodeBase<la_i_conjugate_mr_t>
{
public:
  MYFLT *imr_lhs;
  MYFLT *imr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imr_lhs, lhs);
    toa(imr_rhs, rhs);
    gmm::copy(gmm::conjugated(rhs->mr), lhs->mr);
    return OK;
  }
};

class la_k_conjugate_mr_t : public OpcodeBase<la_k_conjugate_mr_t>
{
public:
  MYFLT *imr_lhs;
  MYFLT *imr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imr_lhs, lhs);
    toa(imr_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(gmm::conjugated(rhs->mr), lhs->mr);
    return OK;
  }
};

class la_i_conjugate_mc_t : public OpcodeBase<la_i_conjugate_mc_t>
{
public:
  MYFLT *imc_lhs;
  MYFLT *imc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imc_lhs, lhs);
    toa(imc_rhs, rhs);
    gmm::copy(gmm::conjugated(rhs->mc), lhs->mc);
    return OK;
  }
};

class la_k_conjugate_mc_t : public OpcodeBase<la_k_conjugate_mc_t>
{
public:
  MYFLT *imc_lhs;
  MYFLT *imc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imc_lhs, lhs);
    toa(imc_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(gmm::conjugated(rhs->mc), lhs->mc);
    return OK;
  }
};

class la_i_norm1_vr_t : public OpcodeBase<la_i_norm1_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm1(rhs->vr);
    return OK;
  }
};

class la_k_norm1_vr_t : public OpcodeBase<la_k_norm1_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm1(rhs->vr);
    return OK;
  }
};

class la_i_norm1_vc_t : public OpcodeBase<la_i_norm1_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm1(rhs->vc);
    return OK;
  }
};

class la_k_norm1_vc_t : public OpcodeBase<la_k_norm1_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm1(rhs->vc);
    return OK;
  }
};

class la_i_norm1_mr_t : public OpcodeBase<la_i_norm1_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norm1(rhs->mr);
    return OK;
  }
};

class la_k_norm1_mr_t : public OpcodeBase<la_k_norm1_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norm1(rhs->mr);
    return OK;
  }
};

class la_i_norm1_mc_t : public OpcodeBase<la_i_norm1_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norm1(rhs->mc);
    return OK;
  }
};

class la_k_norm1_mc_t : public OpcodeBase<la_k_norm1_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norm1(rhs->mc);
    return OK;
  }
};

class la_i_norm_euclid_vr_t : public OpcodeBase<la_i_norm_euclid_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm2(rhs->vr);
    return OK;
  }
};

class la_k_norm_euclid_vr_t : public OpcodeBase<la_k_norm_euclid_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm2(rhs->vr);
    return OK;
  }
};

class la_i_norm_euclid_vc_t : public OpcodeBase<la_i_norm_euclid_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm2(rhs->vc);
    return OK;
  }
};

class la_k_norm_euclid_vc_t : public OpcodeBase<la_k_norm_euclid_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norm2(rhs->vc);
    return OK;
  }
};

class la_i_norm_euclid_mr_t : public OpcodeBase<la_i_norm_euclid_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_euclidean_norm(rhs->mr);
    return OK;
  }
};

class la_k_norm_euclid_mr_t : public OpcodeBase<la_k_norm_euclid_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_euclidean_norm(rhs->mr);
    return OK;
  }
};

class la_i_norm_euclid_mc_t : public OpcodeBase<la_i_norm_euclid_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_euclidean_norm(rhs->mc);
    return OK;
  }
};

class la_k_norm_euclid_mc_t : public OpcodeBase<la_k_norm_euclid_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_euclidean_norm(rhs->mc);
    return OK;
  }
};

class la_i_distance_vr_t : public OpcodeBase<la_i_distance_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_1_;
  MYFLT *rhs_2_;
  la_i_vr_create_t *rhs_1;
  la_i_vr_create_t *rhs_2;
  int init(CSOUND *)
  {
    toa(rhs_1_, rhs_1);
    toa(rhs_2_, rhs_2);
    *lhs = gmm::vect_dist2(rhs_1->vr, rhs_2->vr);
    return OK;
  }
};

class la_k_distance_vr_t : public OpcodeBase<la_k_distance_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_1_;
  MYFLT *rhs_2_;
  la_i_vr_create_t *rhs_1;
  la_i_vr_create_t *rhs_2;
  int init(CSOUND *)
  {
    toa(rhs_1_, rhs_1);
    toa(rhs_2_, rhs_2);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    *lhs = gmm::vect_dist2(rhs_1->vr, rhs_2->vr);
    return OK;
  }
};

class la_i_distance_vc_t : public OpcodeBase<la_i_distance_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_1_;
  MYFLT *rhs_2_;
  la_i_vc_create_t *rhs_1;
  la_i_vc_create_t *rhs_2;
  int init(CSOUND *)
  {
    toa(rhs_1_, rhs_1);
    toa(rhs_2_, rhs_2);
    *lhs = gmm::vect_dist2(rhs_1->vc, rhs_2->vc);
    return OK;
  }
};

class la_k_distance_vc_t : public OpcodeBase<la_k_distance_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_1_;
  MYFLT *rhs_2_;
  la_i_vc_create_t *rhs_1;
  la_i_vc_create_t *rhs_2;
  int init(CSOUND *)
  {
    toa(rhs_1_, rhs_1);
    toa(rhs_2_, rhs_2);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    *lhs = gmm::vect_dist2(rhs_1->vc, rhs_2->vc);
    return OK;
  }
};

class la_i_norm_max_mr_t : public OpcodeBase<la_i_norm_max_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_maxnorm(rhs->mr);
    return OK;
  }
};

class la_k_norm_max_mr_t : public OpcodeBase<la_k_norm_max_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_maxnorm(rhs->mr);
    return OK;
  }
};

class la_i_norm_max_mc_t : public OpcodeBase<la_i_norm_max_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_maxnorm(rhs->mc);
    return OK;
  }
};

class la_k_norm_max_mc_t : public OpcodeBase<la_k_norm_max_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_maxnorm(rhs->mc);
    return OK;
  }
};

class la_i_norm_inf_vr_t : public OpcodeBase<la_i_norm_inf_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norminf(rhs->vr);
    return OK;
  }
};

class la_k_norm_inf_vr_t : public OpcodeBase<la_k_norm_inf_vr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norminf(rhs->vr);
    return OK;
  }
};

class la_i_norm_inf_vc_t : public OpcodeBase<la_i_norm_inf_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norminf(rhs->vc);
    return OK;
  }
};

class la_k_norm_inf_vc_t : public OpcodeBase<la_k_norm_inf_vc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_vc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::vect_norminf(rhs->vc);
    return OK;
  }
};

class la_i_norm_inf_mr_t : public OpcodeBase<la_i_norm_inf_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norminf(rhs->mr);
    return OK;
  }
};

class la_k_norm_inf_mr_t : public OpcodeBase<la_k_norm_inf_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norminf(rhs->mr);
    return OK;
  }
};

class la_i_norm_inf_mc_t : public OpcodeBase<la_i_norm_inf_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norminf(rhs->mc);
    return OK;
  }
};

class la_k_norm_inf_mc_t : public OpcodeBase<la_k_norm_inf_mc_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_norminf(rhs->mc);
    return OK;
  }
};

class la_i_trace_mr_t : public OpcodeBase<la_i_trace_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_trace(rhs->mr);
    return OK;
  }
};

class la_k_trace_mr_t : public OpcodeBase<la_k_trace_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::mat_trace(rhs->mr);
    return OK;
  }
};

class la_i_trace_mc_t : public OpcodeBase<la_i_trace_mc_t>
{
public:
  MYFLT *lhs_r;
  MYFLT *lhs_i;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    const std::complex<MYFLT> &lhs = gmm::mat_trace(rhs->mc);
    *lhs_r = lhs.real();
    *lhs_i = lhs.imag();
    return OK;
  }
};

class la_k_trace_mc_t : public OpcodeBase<la_k_trace_mc_t>
{
public:
  MYFLT *lhs_r;
  MYFLT *lhs_i;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    const std::complex<MYFLT> &lhs = gmm::mat_trace(rhs->mc);
    *lhs_r = lhs.real();
    *lhs_i = lhs.imag();
    return OK;
  }
};

class la_i_lu_det_mr_t : public OpcodeBase<la_i_lu_det_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::lu_det(rhs->mr);
    return OK;
  }
};

class la_k_lu_det_mr_t : public OpcodeBase<la_k_lu_det_mr_t>
{
public:
  MYFLT *lhs;
  MYFLT *rhs_;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    *lhs = gmm::lu_det(rhs->mr);
    return OK;
  }
};

class la_i_lu_det_mc_t : public OpcodeBase<la_i_lu_det_mc_t>
{
public:
  MYFLT *lhs_r;
  MYFLT *lhs_i;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    const std::complex<MYFLT> &lhs = gmm::lu_det(rhs->mc);
    *lhs_r = lhs.real();
    *lhs_i = lhs.imag();
    return OK;
  }
};

class la_k_lu_det_mc_t : public OpcodeBase<la_k_lu_det_mc_t>
{
public:
  MYFLT *lhs_r;
  MYFLT *lhs_i;
  MYFLT *rhs_;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    toa(rhs_, rhs);
    const std::complex<MYFLT> &lhs = gmm::lu_det(rhs->mc);
    *lhs_r = lhs.real();
    *lhs_i = lhs.imag();
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a += b.
 */
class la_i_add_vr_t : public OpcodeBase<la_i_add_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] + rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a += b.
 */
class la_k_add_vr_t : public OpcodeBase<la_k_add_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] + rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a += b.
 */
class la_i_add_vc_t : public OpcodeBase<la_i_add_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] + rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a += b.
 */
class la_k_add_vc_t : public OpcodeBase<la_k_add_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] + rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A += B.
 */
class la_i_add_mr_t : public OpcodeBase<la_i_add_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mr);
    size_t columnN = gmm::mat_ncols(rhs_a->mr);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) + rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A += B.
 */
class la_k_add_mr_t : public OpcodeBase<la_k_add_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mr);
    columnN = gmm::mat_ncols(rhs_a->mr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) + rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A += B.
 */
class la_i_add_mc_t : public OpcodeBase<la_i_add_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mc);
    size_t columnN = gmm::mat_ncols(rhs_a->mc);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) + rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise addition.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A += B.
 */
class la_k_add_mc_t : public OpcodeBase<la_k_add_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mc);
    columnN = gmm::mat_ncols(rhs_a->mc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) + rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a -= b.
 */
class la_i_subtract_vr_t : public OpcodeBase<la_i_subtract_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] - rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a -= b.
 */
class la_k_subtract_vr_t : public OpcodeBase<la_k_subtract_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] - rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a -= b.
 */
class la_i_subtract_vc_t : public OpcodeBase<la_i_subtract_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] - rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a -= b.
 */
class la_k_subtract_vc_t : public OpcodeBase<la_k_subtract_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] - rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A -= B.
 */
class la_i_subtract_mr_t : public OpcodeBase<la_i_subtract_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mr);
    size_t columnN = gmm::mat_ncols(rhs_a->mr);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) - rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A -= B.
 */
class la_k_subtract_mr_t : public OpcodeBase<la_k_subtract_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mr);
    columnN = gmm::mat_ncols(rhs_a->mr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) - rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A -= B.
 */
class la_i_subtract_mc_t : public OpcodeBase<la_i_subtract_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mc);
    size_t columnN = gmm::mat_ncols(rhs_a->mc);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) - rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise subtraction.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A -= B.
 */
class la_k_subtract_mc_t : public OpcodeBase<la_k_subtract_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mc);
    columnN = gmm::mat_ncols(rhs_a->mc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) - rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_i_multiply_vr_t : public OpcodeBase<la_i_multiply_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] * rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_k_multiply_vr_t : public OpcodeBase<la_k_multiply_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] * rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_i_multiply_vc_t : public OpcodeBase<la_i_multiply_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] * rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_k_multiply_vc_t : public OpcodeBase<la_k_multiply_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] * rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A *= B.
 */
class la_i_multiply_mr_t : public OpcodeBase<la_i_multiply_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mr);
    size_t columnN = gmm::mat_ncols(rhs_a->mr);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) * rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A *= B.
 */
class la_k_multiply_mr_t : public OpcodeBase<la_k_multiply_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mr);
    columnN = gmm::mat_ncols(rhs_a->mr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) * rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A *= B.
 */
class la_i_multiply_mc_t : public OpcodeBase<la_i_multiply_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mc);
    size_t columnN = gmm::mat_ncols(rhs_a->mc);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) * rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A *= B.
 */
class la_k_multiply_mc_t : public OpcodeBase<la_k_multiply_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mc);
    columnN = gmm::mat_ncols(rhs_a->mc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) * rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a /= b.
 */
class la_i_divide_vr_t : public OpcodeBase<la_i_divide_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] / rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a /= b.
 */
class la_k_divide_vr_t : public OpcodeBase<la_k_divide_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vr.size(); i < n; ++i) {
      lhs->vr[i] = rhs_a->vr[i] / rhs_b->vr[i];
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a /= b.
 */
class la_i_divide_vc_t : public OpcodeBase<la_i_divide_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] / rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a /= b.
 */
class la_k_divide_vc_t : public OpcodeBase<la_k_divide_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t i = 0, n = rhs_a->vc.size(); i < n; ++i) {
      lhs->vc[i] = rhs_a->vc[i] / rhs_b->vc[i];
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A /= B.
 */
class la_i_divide_mr_t : public OpcodeBase<la_i_divide_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mr);
    size_t columnN = gmm::mat_ncols(rhs_a->mr);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) /
                                 rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A /= B.
 */
class la_k_divide_mr_t : public OpcodeBase<la_k_divide_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mr);
    columnN = gmm::mat_ncols(rhs_a->mr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mr(rowI, columnI) = rhs_a->mr(rowI, columnI) /
                                 rhs_b->mr(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A /= B.
 */
class la_i_divide_mc_t : public OpcodeBase<la_i_divide_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    size_t rowN = gmm::mat_nrows(rhs_a->mc);
    size_t columnN = gmm::mat_ncols(rhs_a->mc);
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) /
                                 rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise division.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform A /= B.
 */
class la_k_divide_mc_t : public OpcodeBase<la_k_divide_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  size_t rowN;
  size_t columnN;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    rowN = gmm::mat_nrows(rhs_a->mc);
    columnN = gmm::mat_ncols(rhs_a->mc);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    for (size_t rowI = 0; rowI < rowN; ++rowI) {
      for (size_t columnI = 0; columnI < columnN; ++columnI) {
        lhs->mc(rowI, columnI) = rhs_a->mc(rowI, columnI) /
                                 rhs_b->mc(rowI, columnI);
      }
    }
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_i_dot_vr_t : public OpcodeBase<la_i_dot_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    *lhs_ = gmm::vect_sp(rhs_a->vr, rhs_b->vr);
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_k_dot_vr_t : public OpcodeBase<la_k_dot_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    *lhs_ = gmm::vect_sp(rhs_a->vr, rhs_b->vr);
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_i_dot_vc_t : public OpcodeBase<la_i_dot_vc_t>
{
public:
  MYFLT *lhs_r;
  MYFLT *lhs_i;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    std::complex<MYFLT> lhs = gmm::vect_sp(rhs_a->vc, rhs_b->vc);
    *lhs_r = lhs.real();
    *lhs_i = lhs.imag();
    return OK;
  }
};

/**
 * Elementwise multiplication.
 * The array on the left-hand side can also appear
 * on the right-hand side in order to perform a *= b.
 */
class la_k_dot_vc_t : public OpcodeBase<la_k_dot_vc_t>
{
public:
  MYFLT *lhs_r;
  MYFLT *lhs_i;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    std::complex<MYFLT> lhs = gmm::vect_sp(rhs_a->vc, rhs_b->vc);
    *lhs_r = lhs.real();
    *lhs_i = lhs.imag();
    return OK;
  }
};

class la_i_dot_mr_t : public OpcodeBase<la_i_dot_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    gmm::mult(rhs_a->mr, rhs_b->mr, lhs->mr);
    return OK;
  }
};

class la_k_dot_mr_t : public OpcodeBase<la_k_dot_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_mr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    gmm::mult(rhs_a->mr, rhs_b->mr, lhs->mr);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::mult(rhs_a->mr, rhs_b->mr, lhs->mr);
    return OK;
  }
};

class la_i_dot_mc_t : public OpcodeBase<la_i_dot_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    gmm::mult(rhs_a->mc, rhs_b->mc, lhs->mc);
    return OK;
  }
};

class la_k_dot_mc_t : public OpcodeBase<la_k_dot_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_mc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::mult(rhs_a->mc, rhs_b->mc, lhs->mc);
    return OK;
  }
};

class la_i_dot_mr_vr_t : public OpcodeBase<la_i_dot_mr_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    gmm::mult(rhs_a->mr, rhs_b->vr, lhs->vr);
    return OK;
  }
};

class la_k_dot_mr_vr_t : public OpcodeBase<la_k_dot_mr_vr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs;
  la_i_mr_create_t *rhs_a;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::mult(rhs_a->mr, rhs_b->vr, lhs->vr);
    return OK;
  }
};

class la_i_dot_mc_vc_t : public OpcodeBase<la_i_dot_mc_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    gmm::mult(rhs_a->mc, rhs_b->vc, lhs->vc);
    return OK;
  }
};

class la_k_dot_mc_vc_t : public OpcodeBase<la_k_dot_mc_vc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_a_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs;
  la_i_mc_create_t *rhs_a;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_a_, rhs_a);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::mult(rhs_a->mc, rhs_b->vc, lhs->vc);
    return OK;
  }
};

class la_i_invert_mr_t : public OpcodeBase<la_i_invert_mr_t>
{
public:
  MYFLT *imr_lhs;
  MYFLT *icondition;
  MYFLT *imr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imr_lhs, lhs);
    toa(imr_rhs, rhs);
    gmm::copy(rhs->mr, lhs->mr);
    *icondition = gmm::lu_inverse(lhs->mr);
    return OK;
  }
};

class la_k_invert_mr_t : public OpcodeBase<la_k_invert_mr_t>
{
public:
  MYFLT *imr_lhs;
  MYFLT *kcondition;
  MYFLT *imr_rhs;
  la_i_mr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imr_lhs, lhs);
    toa(imr_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(rhs->mr, lhs->mr);
    *kcondition = gmm::lu_inverse(lhs->mr);
    return OK;
  }
};

class la_i_invert_mc_t : public OpcodeBase<la_i_invert_mc_t>
{
public:
  MYFLT *imc_lhs;
  MYFLT *icondition_r;
  MYFLT *icondition_i;
  MYFLT *imc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imc_lhs, lhs);
    toa(imc_rhs, rhs);
    gmm::copy(rhs->mc, lhs->mc);
    std::complex<MYFLT> condition = gmm::lu_inverse(lhs->mc);
    *icondition_r = condition.real();
    *icondition_i = condition.imag();
    return OK;
  }
};

class la_k_invert_mc_t : public OpcodeBase<la_k_invert_mc_t>
{
public:
  MYFLT *imc_lhs;
  MYFLT *kcondition_r;
  MYFLT *kcondition_i;
  MYFLT *imc_rhs;
  la_i_mc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(imc_lhs, lhs);
    toa(imc_rhs, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::copy(rhs->mc, lhs->mc);
    std::complex<MYFLT> condition = gmm::lu_inverse(lhs->mc);
    *kcondition_r = condition.real();
    *kcondition_i = condition.imag();
    return OK;
  }
};

class la_i_upper_solve_mr_t : public OpcodeBase<la_i_upper_solve_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    gmm::upper_tri_solve(rhs->mr, lhs->vr, bool(*is_unit));
    return OK;
  }
};

class la_k_upper_solve_mr_t : public OpcodeBase<la_k_upper_solve_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::upper_tri_solve(rhs->mr, lhs->vr, bool(*is_unit));
    return OK;
  }
};

class la_i_upper_solve_mc_t : public OpcodeBase<la_i_upper_solve_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    gmm::upper_tri_solve(rhs->mc, lhs->vc, bool(*is_unit));
    return OK;
  }
};

class la_k_upper_solve_mc_t : public OpcodeBase<la_k_upper_solve_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::upper_tri_solve(rhs->mc, lhs->vc, bool(*is_unit));
    return OK;
  }
};

class la_i_lower_solve_mr_t : public OpcodeBase<la_i_lower_solve_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    gmm::lower_tri_solve(rhs->mr, lhs->vr, bool(*is_unit));
    return OK;
  }
};

class la_k_lower_solve_mr_t : public OpcodeBase<la_k_lower_solve_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vr_create_t *lhs;
  la_i_mr_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::lower_tri_solve(rhs->mr, lhs->vr, bool(*is_unit));
    return OK;
  }
};

class la_i_lower_solve_mc_t : public OpcodeBase<la_i_lower_solve_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    gmm::lower_tri_solve(rhs->mc, lhs->vc, bool(*is_unit));
    return OK;
  }
};

class la_k_lower_solve_mc_t : public OpcodeBase<la_k_lower_solve_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *rhs_;
  MYFLT *is_unit;
  la_i_vc_create_t *lhs;
  la_i_mc_create_t *rhs;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::lower_tri_solve(rhs->mc, lhs->vc, bool(*is_unit));
    return OK;
  }
};

class la_i_lu_factor_mr_t : public OpcodeBase<la_i_lu_factor_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *pivot_;
  MYFLT *isize;
  MYFLT *rhs_;
  la_i_mr_create_t *lhs;
  la_i_vr_create_t *pivot;
  la_i_mr_create_t *rhs;
  std::vector<size_t> pivot__;
  size_t pivot_size;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, pivot);
    toa(rhs_, rhs);
    pivot_size = gmm::mat_nrows(rhs->mr);
    pivot__.resize(pivot_size);
    gmm::copy(rhs->mr, lhs->mr);
    *isize = gmm::lu_factor(lhs->mr, pivot__);
    for (size_t i = 0; i < pivot_size; ++i) {
      pivot->vr[i] = pivot__[i];
    }
    return OK;
  }
};

class la_k_lu_factor_mr_t : public OpcodeBase<la_k_lu_factor_mr_t>
{
public:
  MYFLT *lhs_;
  MYFLT *pivot_;
  MYFLT *ksize;
  MYFLT *rhs_;
  la_i_mr_create_t *lhs;
  la_i_vr_create_t *pivot;
  la_i_mr_create_t *rhs;
  std::vector<size_t> pivot__;
  size_t pivot_size;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, pivot);
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    pivot_size = gmm::mat_nrows(rhs->mr);
    pivot__.resize(pivot_size);
    gmm::copy(rhs->mr, lhs->mr);
    *ksize = gmm::lu_factor(lhs->mr, pivot__);
    for (size_t i = 0; i < pivot_size; ++i) {
      pivot->vr[i] = pivot__[i];
    }
    return OK;
  }
};

class la_i_lu_factor_mc_t : public OpcodeBase<la_i_lu_factor_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *pivot_;
  MYFLT *isize;
  MYFLT *rhs_;
  la_i_mc_create_t *lhs;
  la_i_vr_create_t *pivot;
  la_i_mc_create_t *rhs;
  std::vector<size_t> pivot__;
  size_t pivot_size;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, pivot);
    toa(rhs_, rhs);
    pivot_size = gmm::mat_nrows(rhs->mc);
    pivot__.resize(pivot_size);
    gmm::copy(rhs->mc, lhs->mc);
    *isize = gmm::lu_factor(lhs->mc, pivot__);
    for (size_t i = 0; i < pivot_size; ++i) {
      pivot->vr[i] = pivot__[i];
    }
    return OK;
  }
};

class la_k_lu_factor_mc_t : public OpcodeBase<la_k_lu_factor_mc_t>
{
public:
  MYFLT *lhs_;
  MYFLT *pivot_;
  MYFLT *ksize;
  MYFLT *rhs_;
  la_i_mc_create_t *lhs;
  la_i_vr_create_t *pivot;
  la_i_mc_create_t *rhs;
  std::vector<size_t> pivot__;
  size_t pivot_size;
  int init(CSOUND *)
  {
    toa(lhs_, lhs);
    toa(rhs_, pivot);
    toa(rhs_, rhs);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    pivot_size = gmm::mat_nrows(rhs->mc);
    pivot__.resize(pivot_size);
    gmm::copy(rhs->mc, lhs->mc);
    *ksize = gmm::lu_factor(lhs->mc, pivot__);
    for (size_t i = 0; i < pivot_size; ++i) {
      pivot->vr[i] = pivot__[i];
    }
    return OK;
  }
};

class la_i_lu_solve_mr_t : public OpcodeBase<la_i_lu_solve_mr_t>
{
public:
  MYFLT *lhs_x_;
  MYFLT *rhs_A_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs_x;
  la_i_mr_create_t *rhs_A;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_x_, lhs_x);
    toa(rhs_A_, rhs_A);
    toa(rhs_b_, rhs_b);
    gmm::lu_solve(rhs_A->mr, lhs_x->vr, rhs_b->vr);
    return OK;
  }
};

class la_k_lu_solve_mr_t : public OpcodeBase<la_k_lu_solve_mr_t>
{
public:
  MYFLT *lhs_x_;
  MYFLT *rhs_A_;
  MYFLT *rhs_b_;
  la_i_vr_create_t *lhs_x;
  la_i_mr_create_t *rhs_A;
  la_i_vr_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_x_, lhs_x);
    toa(rhs_A_, rhs_A);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::lu_solve(rhs_A->mr, lhs_x->vr, rhs_b->vr);
    return OK;
  }
};

class la_i_lu_solve_mc_t : public OpcodeBase<la_i_lu_solve_mc_t>
{
public:
  MYFLT *lhs_x_;
  MYFLT *rhs_A_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs_x;
  la_i_mc_create_t *rhs_A;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_x_, lhs_x);
    toa(rhs_A_, rhs_A);
    toa(rhs_b_, rhs_b);
    gmm::lu_solve(rhs_A->mc, lhs_x->vc, rhs_b->vc);
    return OK;
  }
};

class la_k_lu_solve_mc_t : public OpcodeBase<la_k_lu_solve_mc_t>
{
public:
  MYFLT *lhs_x_;
  MYFLT *rhs_A_;
  MYFLT *rhs_b_;
  la_i_vc_create_t *lhs_x;
  la_i_mc_create_t *rhs_A;
  la_i_vc_create_t *rhs_b;
  int init(CSOUND *)
  {
    toa(lhs_x_, lhs_x);
    toa(rhs_A_, rhs_A);
    toa(rhs_b_, rhs_b);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::lu_solve(rhs_A->mc, lhs_x->vc, rhs_b->vc);
    return OK;
  }
};

class la_i_qr_factor_mr_t : public OpcodeBase<la_i_qr_factor_mr_t>
{
public:
  MYFLT *lhs_Q_;
  MYFLT *lhs_R_;
  MYFLT *rhs_A_;
  la_i_mr_create_t *lhs_Q;
  la_i_mr_create_t *lhs_R;
  la_i_mr_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_Q_, lhs_Q);
    toa(lhs_R_, lhs_R);
    toa(rhs_A_, rhs_A);
    gmm::qr_factor(rhs_A->mr, lhs_Q->mr, lhs_R->mr);
    return OK;
  }
};

class la_k_qr_factor_mr_t : public OpcodeBase<la_k_qr_factor_mr_t>
{
public:
  MYFLT *lhs_Q_;
  MYFLT *lhs_R_;
  MYFLT *rhs_A_;
  la_i_mr_create_t *lhs_Q;
  la_i_mr_create_t *lhs_R;
  la_i_mr_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_Q_, lhs_Q);
    toa(lhs_R_, lhs_R);
    toa(rhs_A_, rhs_A);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::qr_factor(rhs_A->mr, lhs_Q->mr, lhs_R->mr);
    return OK;
  }
};

class la_i_qr_factor_mc_t : public OpcodeBase<la_i_qr_factor_mc_t>
{
public:
  MYFLT *lhs_Q_;
  MYFLT *lhs_R_;
  MYFLT *rhs_A_;
  la_i_mc_create_t *lhs_Q;
  la_i_mc_create_t *lhs_R;
  la_i_mc_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_Q_, lhs_Q);
    toa(lhs_R_, lhs_R);
    toa(rhs_A_, rhs_A);
    gmm::qr_factor(rhs_A->mc, lhs_Q->mc, lhs_R->mc);
    return OK;
  }
};

class la_k_qr_factor_mc_t : public OpcodeBase<la_k_qr_factor_mc_t>
{
public:
  MYFLT *lhs_Q_;
  MYFLT *lhs_R_;
  MYFLT *rhs_A_;
  la_i_mc_create_t *lhs_Q;
  la_i_mc_create_t *lhs_R;
  la_i_mc_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_Q_, lhs_Q);
    toa(lhs_R_, lhs_R);
    toa(rhs_A_, rhs_A);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::qr_factor(rhs_A->mc, lhs_Q->mc, lhs_R->mc);
    return OK;
  }
};

class la_i_qr_eigen_mr_t : public OpcodeBase<la_i_qr_eigen_mr_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *rhs_A_;
  MYFLT *itolerance;
  la_i_vr_create_t *lhs_eigenvalues;
  la_i_mr_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(rhs_A_, rhs_A);
    gmm::implicit_qr_algorithm(rhs_A->mr, lhs_eigenvalues->vr, double(*itolerance));
    return OK;
  }
};

class la_k_qr_eigen_mr_t : public OpcodeBase<la_k_qr_eigen_mr_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *rhs_A_;
  MYFLT *ktolerance;
  la_i_vr_create_t *lhs_eigenvalues;
  la_i_mr_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(rhs_A_, rhs_A);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::implicit_qr_algorithm(rhs_A->mr, lhs_eigenvalues->vr, double(*ktolerance));
    return OK;
  }
};

class la_i_qr_eigen_mc_t : public OpcodeBase<la_i_qr_eigen_mc_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *rhs_A_;
  MYFLT *itolerance;
  la_i_vc_create_t *lhs_eigenvalues;
  la_i_mc_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(rhs_A_, rhs_A);
    gmm::implicit_qr_algorithm(rhs_A->mc, lhs_eigenvalues->vc, double(*itolerance));
    return OK;
  }
};

class la_k_qr_eigen_mc_t : public OpcodeBase<la_k_qr_eigen_mc_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *rhs_A_;
  MYFLT *ktolerance;
  la_i_vc_create_t *lhs_eigenvalues;
  la_i_mc_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(rhs_A_, rhs_A);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::implicit_qr_algorithm(rhs_A->mc, lhs_eigenvalues->vc, double(*ktolerance));
    return OK;
  }
};

class la_i_qr_sym_eigen_mr_t : public OpcodeBase<la_i_qr_sym_eigen_mr_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *lhs_eigenvectors_;
  MYFLT *rhs_A_;
  MYFLT *itolerance;
  la_i_vr_create_t *lhs_eigenvalues;
  la_i_mr_create_t *lhs_eigenvectors;
  la_i_mr_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(lhs_eigenvectors_, lhs_eigenvectors);
    toa(rhs_A_, rhs_A);
    gmm::implicit_qr_algorithm(rhs_A->mr, lhs_eigenvalues->vr,
                               lhs_eigenvectors->mr, double(*itolerance));
    return OK;
  }
};

class la_k_qr_sym_eigen_mr_t : public OpcodeBase<la_k_qr_sym_eigen_mr_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *lhs_eigenvectors_;
  MYFLT *rhs_A_;
  MYFLT *ktolerance;
  la_i_vr_create_t *lhs_eigenvalues;
  la_i_mr_create_t *lhs_eigenvectors;
  la_i_mr_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(lhs_eigenvectors_, lhs_eigenvectors);
    toa(rhs_A_, rhs_A);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::implicit_qr_algorithm(rhs_A->mr, lhs_eigenvalues->vr,
                               lhs_eigenvectors->mr, double(*ktolerance));
    return OK;
  }
};

class la_i_qr_sym_eigen_mc_t : public OpcodeBase<la_i_qr_sym_eigen_mc_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *lhs_eigenvectors_;
  MYFLT *rhs_A_;
  MYFLT *itolerance;
  la_i_vc_create_t *lhs_eigenvalues;
  la_i_mc_create_t *lhs_eigenvectors;
  la_i_mc_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(lhs_eigenvectors_, lhs_eigenvectors);
    toa(rhs_A_, rhs_A);
    gmm::implicit_qr_algorithm(rhs_A->mc, lhs_eigenvalues->vc,
                               lhs_eigenvectors->mc, double(*itolerance));
    return OK;
  }
};

class la_k_qr_sym_eigen_mc_t : public OpcodeBase<la_k_qr_sym_eigen_mc_t>
{
public:
  MYFLT *lhs_eigenvalues_;
  MYFLT *lhs_eigenvectors_;
  MYFLT *rhs_A_;
  MYFLT *ktolerance;
  la_i_vc_create_t *lhs_eigenvalues;
  la_i_mc_create_t *lhs_eigenvectors;
  la_i_mc_create_t *rhs_A;
  int init(CSOUND *)
  {
    toa(lhs_eigenvalues_, lhs_eigenvalues);
    toa(lhs_eigenvectors_, lhs_eigenvectors);
    toa(rhs_A_, rhs_A);
    return OK;
  }
  int kontrol(CSOUND *)
  {
    gmm::implicit_qr_algorithm(rhs_A->mc, lhs_eigenvalues->vc,
                               lhs_eigenvectors->mc, double(*ktolerance));
    return OK;
  }
};

extern "C"
{

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    int status = 0;
    status |=
      csound->AppendOpcode(csound,
                           "la_i_vr_create",
                           sizeof(la_i_vr_create_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_vr_create_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_vc_create",
                           sizeof(la_i_vr_create_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_vc_create_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_mr_create",
                           sizeof(la_i_mr_create_t),
                           0,
                           1,
                           "i",
                           "iio",
                           (int (*)(CSOUND*,void*)) &la_i_mr_create_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_mc_create",
                           sizeof(la_i_mc_create_t),
                           0,
                           1,
                           "i",
                           "iioo",
                           (int (*)(CSOUND*,void*)) &la_i_mc_create_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_size_vr",
                           sizeof(la_i_size_vr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_size_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_size_vc",
                           sizeof(la_i_size_vc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_size_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_size_mr",
                           sizeof(la_i_size_mr_t),
                           0,
                           1,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_size_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_size_mc",
                           sizeof(la_i_size_mc_t),
                           0,
                           1,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_size_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_current_f",
                           sizeof(la_k_current_f_t),
                           0,
                           3,
                           "k",
                           "f",
                           (int (*)(CSOUND*,void*)) &la_k_current_f_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_current_f_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_current_vr",
                           sizeof(la_k_current_vr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_current_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_current_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_print_vr",
                           sizeof(la_i_print_vr_t),
                           0,
                           1,
                           "",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_print_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_print_vc",
                           sizeof(la_i_print_vc_t),
                           0,
                           1,
                           "",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_print_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_print_mr",
                           sizeof(la_i_print_mr_t),
                           0,
                           1,
                           "",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_print_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_print_mc",
                           sizeof(la_i_print_mc_t),
                           0,
                           1,
                           "",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_print_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_assign_vr",
                           sizeof(la_i_assign_vr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_assign_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_assign_vr",
                           sizeof(la_k_assign_vr_t),
                           0,
                           3,
                           "i",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_assign_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_assign_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_assign_vc",
                           sizeof(la_i_assign_vc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_assign_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_assign_vc",
                           sizeof(la_k_assign_vc_t),
                           0,
                           3,
                           "i",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_assign_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_assign_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_assign_mr",
                           sizeof(la_i_assign_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_assign_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_assign_mr",
                           sizeof(la_k_assign_mr_t),
                           0,
                           3,
                           "i",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_assign_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_assign_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_assign_mc",
                           sizeof(la_i_assign_mc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_assign_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_assign_mc",
                           sizeof(la_k_assign_mc_t),
                           0,
                           3,
                           "i",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_assign_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_assign_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_assign_a",
                           sizeof(la_k_assign_a_t),
                           0,
                           3,
                           "i",
                           "a",
                           (int (*)(CSOUND*,void*)) &la_k_assign_a_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_assign_a_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_assign_t",
                           sizeof(la_i_assign_t_t),
                           TR,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_assign_t_t::init_,
                           (int (*)(CSOUND*,void*)) &la_i_assign_t_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_assign_t",
                           sizeof(la_k_assign_t_t),
                           TR,
                           3,
                           "i",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_assign_t_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_assign_t_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_assign_f",
                           sizeof(la_k_assign_f_t),
                           0,
                           3,
                           "i",
                           "f",
                           (int (*)(CSOUND*,void*)) &la_k_assign_f_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_assign_f_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_a_assign",
                           sizeof(la_k_a_assign_t),
                           0,
                           3,
                           "a",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_a_assign_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_a_assign_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_t_assign",
                           sizeof(la_i_t_assign_t),
                           TW,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_t_assign_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_t_assign",
                           sizeof(la_k_t_assign_t),
                           TW,
                           3,
                           "i",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_t_assign_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_t_assign_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_f_assign",
                           sizeof(la_k_f_assign_t),
                           0,
                           3,
                           "f",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_k_f_assign_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_f_assign_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_random_vr",
                           sizeof(la_i_random_vr_t),
                           0,
                           1,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_i_random_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_random_vr",
                           sizeof(la_k_random_vr_t),
                           0,
                           3,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_k_random_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_random_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_random_vc",
                           sizeof(la_i_random_vc_t),
                           0,
                           1,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_i_random_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_random_vc",
                           sizeof(la_k_random_vc_t),
                           0,
                           3,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_k_random_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_random_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_random_mr",
                           sizeof(la_i_random_mr_t),
                           0,
                           1,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_i_random_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_random_mr",
                           sizeof(la_k_random_mr_t),
                           0,
                           3,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_k_random_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_random_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_random_mc",
                           sizeof(la_i_random_mc_t),
                           0,
                           1,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_i_random_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_random_mc",
                           sizeof(la_k_random_mc_t),
                           0,
                           3,
                           "i",
                           "p",
                           (int (*)(CSOUND*,void*)) &la_k_random_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_random_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_vr_set",
                           sizeof(la_i_vr_set_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_vr_set_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_vr_set",
                           sizeof(la_k_vr_set_t),
                           0,
                           3,
                           "i",
                           "kk",
                           (int (*)(CSOUND*,void*)) &la_k_vr_set_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_vr_set_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_vc_set",
                           sizeof(la_i_vc_set_t),
                           0,
                           1,
                           "i",
                           "iii",
                           (int (*)(CSOUND*,void*)) &la_i_vc_set_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_vc_set",
                           sizeof(la_k_vc_set_t),
                           0,
                           3,
                           "i",
                           "kkk",
                           (int (*)(CSOUND*,void*)) &la_k_vc_set_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_vc_set_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_mr_set",
                           sizeof(la_i_mr_set_t),
                           0,
                           1,
                           "i",
                           "iii",
                           (int (*)(CSOUND*,void*)) &la_i_mr_set_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
   status |=
     csound->AppendOpcode(csound,
                          "la_k_mr_set",
                          sizeof(la_k_mr_set_t),
                          0,
                          3,
                          "i",
                          "kkk",
                          (int (*)(CSOUND*,void*)) &la_k_mr_set_t::init_,
                          (int (*)(CSOUND*,void*)) &la_k_mr_set_t::kontrol_,
                          (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_mc_set",
                           sizeof(la_i_mc_set_t),
                           0,
                           1,
                           "i",
                           "iiii",
                           (int (*)(CSOUND*,void*)) &la_i_mc_set_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_mc_set",
                           sizeof(la_k_mc_set_t),
                           0,
                           3,
                           "i",
                           "kkkk",
                           (int (*)(CSOUND*,void*)) &la_k_mc_set_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_mc_set_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);

    status |=
      csound->AppendOpcode(csound,
                           "la_i_get_vr",
                           sizeof(la_i_get_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_get_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_get_vr",
                           sizeof(la_k_get_vr_t),
                           0,
                           3,
                           "k",
                           "ik",
                           (int (*)(CSOUND*,void*)) &la_k_get_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_get_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_get_vc",
                           sizeof(la_i_get_vc_t),
                           0,
                           1,
                           "ii",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_get_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_get_vc",
                           sizeof(la_k_get_vc_t),
                           0,
                           3,
                           "kk",
                           "ik",
                           (int (*)(CSOUND*,void*)) &la_k_get_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_get_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_get_mr",
                           sizeof(la_i_get_mr_t),
                           0,
                           1,
                           "i",
                           "iii",
                           (int (*)(CSOUND*,void*)) &la_i_get_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_get_mr",
                           sizeof(la_k_get_mr_t),
                           0,
                           3,
                           "k",
                           "ikk",
                           (int (*)(CSOUND*,void*)) &la_k_get_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_get_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_get_mc",
                           sizeof(la_i_get_mc_t),
                           0,
                           1,
                           "ii",
                           "iii",
                           (int (*)(CSOUND*,void*)) &la_i_get_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_get_mc",
                           sizeof(la_k_get_mc_t),
                           0,
                           3,
                           "kk",
                           "ikk",
                           (int (*)(CSOUND*,void*)) &la_k_get_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_get_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_transpose_mr",
                           sizeof(la_i_transpose_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_transpose_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_transpose_mr",
                           sizeof(la_k_transpose_mr_t),
                           0,
                           3,
                           "i",
                           "k",
                           (int (*)(CSOUND*,void*)) &la_k_transpose_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_transpose_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_transpose_mc",
                           sizeof(la_i_transpose_mc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_transpose_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_transpose_mc",
                           sizeof(la_k_transpose_mc_t),
                           0,
                           2,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_transpose_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_transpose_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_conjugate_vr",
                           sizeof(la_i_conjugate_vr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_conjugate_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_conjugate_vr",
                           sizeof(la_k_conjugate_vr_t),
                           0,
                           3,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_conjugate_vc",
                           sizeof(la_i_conjugate_vc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_conjugate_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_conjugate_vc",
                           sizeof(la_k_conjugate_vc_t),
                           0,
                           3,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_conjugate_mr",
                           sizeof(la_i_conjugate_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_conjugate_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_conjugate_mr",
                           sizeof(la_k_conjugate_mr_t),
                           0,
                           3,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_conjugate_mc",
                           sizeof(la_i_conjugate_mc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_conjugate_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_conjugate_mc",
                           sizeof(la_k_conjugate_mc_t),
                           0,
                           3,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_conjugate_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm1_vr",
                           sizeof(la_i_norm1_vr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm1_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm1_vr",
                           sizeof(la_k_norm1_vr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm1_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm1_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm1_vc",
                           sizeof(la_i_norm1_vc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm1_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm1_vc",
                           sizeof(la_k_norm1_vc_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm1_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm1_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm1_mr",
                           sizeof(la_i_norm1_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm1_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm1_mr",
                           sizeof(la_k_norm1_mr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm1_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm1_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm1_mc",
                           sizeof(la_i_norm1_mc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm1_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm1_mc",
                           sizeof(la_k_norm1_mc_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm1_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm1_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_euclid_vr",
                           sizeof(la_i_norm_euclid_vr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_euclid_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_euclid_vr",
                           sizeof(la_k_norm_euclid_vr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_euclid_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_euclid_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_euclid_vc",
                           sizeof(la_i_norm_euclid_vc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_euclid_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_euclid_vc",
                           sizeof(la_k_norm_euclid_vc_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_euclid_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_euclid_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_euclid_mr",
                           sizeof(la_i_norm_euclid_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_euclid_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_euclid_mr",
                           sizeof(la_k_norm_euclid_mr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_euclid_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_euclid_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_euclid_mc",
                           sizeof(la_i_norm_euclid_mc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_euclid_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_euclid_mc",
                           sizeof(la_k_norm_euclid_mc_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_euclid_mc_t::init_,
                           (int (*)(CSOUND*,void*))&la_k_norm_euclid_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_distance_vr",
                           sizeof(la_i_distance_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_distance_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_distance_vr",
                           sizeof(la_k_distance_vr_t),
                           0,
                           3,
                           "k",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_distance_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_distance_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_distance_vc",
                           sizeof(la_i_distance_vc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_distance_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_distance_vc",
                           sizeof(la_k_distance_vc_t),
                           0,
                           3,
                           "k",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_distance_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_distance_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_max_mr",
                           sizeof(la_i_norm_max_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_max_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_max_mr",
                           sizeof(la_k_norm_max_mr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_max_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_max_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_max_mc",
                           sizeof(la_i_norm_max_mc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_max_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_max_mc",
                           sizeof(la_k_norm_max_mc_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_max_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_max_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_inf_vr",
                           sizeof(la_i_norm_inf_vr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_inf_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_inf_vr",
                           sizeof(la_k_norm_inf_vr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_inf_vc",
                           sizeof(la_i_norm_inf_vc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_inf_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_inf_vc",
                           sizeof(la_k_norm_inf_vc_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_inf_mr",
                           sizeof(la_i_norm_inf_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_inf_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_inf_mr",
                           sizeof(la_k_norm_inf_mr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_norm_inf_mc",
                           sizeof(la_i_norm_inf_mc_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_norm_inf_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_norm_inf_mc",
                           sizeof(la_k_norm_inf_mc_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_norm_inf_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_trace_mr",
                           sizeof(la_i_trace_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_trace_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_trace_mr",
                           sizeof(la_k_trace_mr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_trace_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_trace_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_trace_mc",
                           sizeof(la_i_trace_mc_t),
                           0,
                           1,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_trace_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_trace_mc",
                           sizeof(la_k_trace_mc_t),
                           0,
                           3,
                           "kk",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_trace_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_trace_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lu_det_mr",
                           sizeof(la_i_lu_det_mr_t),
                           0,
                           1,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_lu_det_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lu_det_mr",
                           sizeof(la_k_lu_det_mr_t),
                           0,
                           3,
                           "k",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_lu_det_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_lu_det_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lu_det_mc",
                           sizeof(la_i_lu_det_mc_t),
                           0,
                           1,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_lu_det_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lu_det_mc",
                           sizeof(la_k_lu_det_mc_t),
                           0,
                           3,
                           "kk",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_lu_det_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_lu_det_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_add_vr",
                           sizeof(la_i_add_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_add_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_add_vr",
                           sizeof(la_k_add_vr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_add_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_add_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_add_vc",
                           sizeof(la_i_add_vc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_add_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_add_vc",
                           sizeof(la_k_add_vc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_add_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_add_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_add_mr",
                           sizeof(la_i_add_mr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_add_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_add_mr",
                           sizeof(la_k_add_mr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_add_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_add_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_add_mc",
                           sizeof(la_i_add_mc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_add_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_add_mc",
                           sizeof(la_k_add_mc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_add_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_add_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_subtract_vr",
                           sizeof(la_i_subtract_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_subtract_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_subtract_vr",
                           sizeof(la_k_subtract_vr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_subtract_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_subtract_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_subtract_vc",
                           sizeof(la_i_subtract_vc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_subtract_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_subtract_vc",
                           sizeof(la_k_subtract_vc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_subtract_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_subtract_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_subtract_mr",
                           sizeof(la_i_subtract_mr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_subtract_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_subtract_mr",
                           sizeof(la_k_subtract_mr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_subtract_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_subtract_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_subtract_mc",
                           sizeof(la_i_subtract_mc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_subtract_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_subtract_mc",
                           sizeof(la_k_subtract_mc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_subtract_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_subtract_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_multiply_vr",
                           sizeof(la_i_multiply_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_multiply_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_multiply_vr",
                           sizeof(la_k_multiply_vr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_multiply_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_multiply_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_multiply_vc",
                           sizeof(la_i_multiply_vc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_multiply_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_multiply_vc",
                           sizeof(la_k_multiply_vc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_multiply_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_multiply_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_multiply_mr",
                           sizeof(la_i_multiply_mr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_multiply_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_multiply_mr",
                           sizeof(la_k_multiply_mr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_multiply_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_multiply_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_multiply_mc",
                           sizeof(la_i_multiply_mc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_multiply_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_multiply_mc",
                           sizeof(la_k_multiply_mc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_multiply_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_multiply_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);


    status |=
      csound->AppendOpcode(csound,
                           "la_i_divide_vr",
                           sizeof(la_i_divide_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_divide_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_divide_vr",
                           sizeof(la_k_divide_vr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_divide_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_divide_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_divide_vc",
                           sizeof(la_i_divide_vc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_divide_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_divide_vc",
                           sizeof(la_k_divide_vc_t),
                           0,
                           3,
                           "i",
                           "kk",
                           (int (*)(CSOUND*,void*)) &la_k_divide_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_divide_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_divide_mr",
                           sizeof(la_i_divide_mr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_divide_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_divide_mr",
                           sizeof(la_k_divide_mr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_divide_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_divide_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_divide_mc",
                           sizeof(la_i_divide_mc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_divide_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_divide_mc",
                           sizeof(la_k_divide_mc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_divide_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_divide_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_dot_vr",
                           sizeof(la_i_dot_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_dot_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_dot_vr",
                           sizeof(la_k_dot_vr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_dot_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_dot_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_dot_vc",
                           sizeof(la_i_dot_vc_t),
                           0,
                           1,
                           "ii",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_dot_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_dot_vc",
                           sizeof(la_k_dot_vc_t),
                           0,
                           3,
                           "ii",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_dot_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_dot_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_dot_mr",
                           sizeof(la_i_dot_mr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_dot_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_dot_mr",
                           sizeof(la_k_dot_mr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_dot_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_dot_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_dot_mc",
                           sizeof(la_i_dot_mc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_dot_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_dot_mc",
                           sizeof(la_k_dot_mc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_dot_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_dot_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_dot_mr_vr",
                           sizeof(la_i_dot_mr_vr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_dot_mr_vr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_dot_mr_vr",
                           sizeof(la_k_dot_mr_vr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_dot_mr_vr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_dot_mr_vr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_dot_mc_vc",
                           sizeof(la_i_dot_mc_vc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_dot_mc_vc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_dot_mc_vc",
                           sizeof(la_k_dot_mc_vc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_dot_mc_vc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_dot_mc_vc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_invert_mr",
                           sizeof(la_i_invert_mr_t),
                           0,
                           1,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_invert_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_invert_mr",
                           sizeof(la_k_invert_mr_t),
                           0,
                           3,
                           "ik",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_invert_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_invert_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_invert_mc",
                           sizeof(la_i_invert_mc_t),
                           0,
                           1,
                           "iii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_invert_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_invert_mc",
                           sizeof(la_k_invert_mc_t),
                           0,
                           3,
                           "ikk",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_invert_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_invert_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_upper_solve_mr",
                           sizeof(la_i_upper_solve_mr_t),
                           0,
                           1,
                           "i",
                           "io",
                           (int (*)(CSOUND*,void*)) &la_i_upper_solve_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_upper_solve_mr",
                           sizeof(la_k_upper_solve_mr_t),
                           0,
                           3,
                           "i",
                           "iO",
                           (int (*)(CSOUND*,void*)) &la_k_upper_solve_mr_t::init_,
                           (int (*)(CSOUND*,void*))
                                   &la_k_upper_solve_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_upper_solve_mc",
                           sizeof(la_i_upper_solve_mc_t),
                           0,
                           1,
                           "i",
                           "io",
                           (int (*)(CSOUND*,void*)) &la_i_upper_solve_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_upper_solve_mc",
                           sizeof(la_k_upper_solve_mc_t),
                           0,
                           3,
                           "i",
                           "iO",
                           (int (*)(CSOUND*,void*)) &la_k_upper_solve_mc_t::init_,
                           (int (*)(CSOUND*,void*))
                                    &la_k_upper_solve_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lower_solve_mr",
                           sizeof(la_i_lower_solve_mr_t),
                           0,
                           1,
                           "i",
                           "io",
                           (int (*)(CSOUND*,void*)) &la_i_lower_solve_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lower_solve_mr",
                           sizeof(la_k_lower_solve_mr_t),
                           0,
                           3,
                           "i",
                           "iO",
                           (int (*)(CSOUND*,void*)) &la_k_lower_solve_mr_t::init_,
                           (int (*)(CSOUND*,void*))
                                    &la_k_lower_solve_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lower_solve_mc",
                           sizeof(la_i_lower_solve_mc_t),
                           0,
                           1,
                           "i",
                           "io",
                           (int (*)(CSOUND*,void*)) &la_i_lower_solve_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lower_solve_mc",
                           sizeof(la_k_lower_solve_mc_t),
                           0,
                           3,
                           "i",
                           "iO",
                           (int (*)(CSOUND*,void*)) &la_k_lower_solve_mc_t::init_,
                           (int (*)(CSOUND*,void*))
                                    &la_k_lower_solve_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lu_factor_mr",
                           sizeof(la_i_lu_factor_mr_t),
                           0,
                           1,
                           "iii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_lu_factor_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lu_factor_mr",
                           sizeof(la_k_lu_factor_mr_t),
                           0,
                           3,
                           "iik",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_lu_factor_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_lu_factor_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lu_factor_mc",
                           sizeof(la_i_lu_factor_mc_t),
                           0,
                           1,
                           "iii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_lu_factor_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lu_factor_mc",
                           sizeof(la_k_lu_factor_mc_t),
                           0,
                           3,
                           "i",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_lu_factor_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_lu_factor_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lu_solve_mr",
                           sizeof(la_i_lu_solve_mr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_lu_solve_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lu_solve_mr",
                           sizeof(la_k_lu_solve_mr_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_lu_solve_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_lu_solve_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_lu_solve_mc",
                           sizeof(la_i_lu_solve_mc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_lu_solve_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_lu_solve_mc",
                           sizeof(la_k_lu_solve_mc_t),
                           0,
                           3,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_k_lu_solve_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_lu_solve_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_qr_factor_mr",
                           sizeof(la_i_qr_factor_mr_t),
                           0,
                           1,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_qr_factor_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_qr_factor_mr",
                           sizeof(la_k_qr_factor_mr_t),
                           0,
                           3,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_qr_factor_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_qr_factor_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_qr_factor_mc",
                           sizeof(la_i_qr_factor_mc_t),
                           0,
                           1,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_i_qr_factor_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_qr_factor_mc",
                           sizeof(la_k_qr_factor_mc_t),
                           0,
                           3,
                           "ii",
                           "i",
                           (int (*)(CSOUND*,void*)) &la_k_qr_factor_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_qr_factor_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_qr_eigen_mr",
                           sizeof(la_i_qr_eigen_mr_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_qr_eigen_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_qr_eigen_mr",
                           sizeof(la_k_qr_eigen_mr_t),
                           0,
                           3,
                           "i",
                           "ik",
                           (int (*)(CSOUND*,void*)) &la_k_qr_eigen_mr_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_qr_eigen_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_qr_eigen_mc",
                           sizeof(la_i_qr_eigen_mc_t),
                           0,
                           1,
                           "i",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_qr_eigen_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_qr_eigen_mc",
                           sizeof(la_k_qr_eigen_mc_t),
                           0,
                           3,
                           "i",
                           "ik",
                           (int (*)(CSOUND*,void*)) &la_k_qr_eigen_mc_t::init_,
                           (int (*)(CSOUND*,void*)) &la_k_qr_eigen_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_qr_sym_eigen_mr",
                           sizeof(la_i_qr_sym_eigen_mr_t),
                           0,
                           1,
                           "ii",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_qr_sym_eigen_mr_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_qr_sym_eigen_mr",
                           sizeof(la_k_qr_sym_eigen_mr_t),
                           0,
                           3,
                           "ii",
                           "ik",
                           (int (*)(CSOUND*,void*)) &la_k_qr_sym_eigen_mr_t::init_,
                           (int (*)(CSOUND*,void*))
                                    &la_k_qr_sym_eigen_mr_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_i_qr_sym_eigen_mc",
                           sizeof(la_i_qr_sym_eigen_mc_t),
                           0,
                           1,
                           "ii",
                           "ii",
                           (int (*)(CSOUND*,void*)) &la_i_qr_sym_eigen_mc_t::init_,
                           (int (*)(CSOUND*,void*)) 0,
                           (int (*)(CSOUND*,void*)) 0);
    status |=
      csound->AppendOpcode(csound,
                           "la_k_qr_sym_eigen_mc",
                           sizeof(la_k_qr_sym_eigen_mc_t),
                           0,
                           3,
                           "ii",
                           "ik",
                           (int (*)(CSOUND*,void*)) &la_k_qr_sym_eigen_mc_t::init_,
                           (int (*)(CSOUND*,void*))
                                    &la_k_qr_sym_eigen_mc_t::kontrol_,
                           (int (*)(CSOUND*,void*)) 0);
    return status;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    return 0;
  }
}
