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
 * NOTE: SDFT must be #defined in order to build and use these opcodes.
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
 * of the array object, which is actually the llocator opcode instance.
 * Although array objects are i-rate, of course
 * their values and even shapes may change at i-rate or k-rate.
 *
 * All operands must be pre-allocated; except for the creation
 * opcodes, no opcode ever allocates any arrays.
 * This is true even if the array appears on the 
 * left-hand side of an opcode! However, some operations 
 * may reshape arrays to hold results.
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
 * 4. Operation name: common mathematical name (preferred) or abbreviation.
 * 5. Type code(s) for input values, if not implicit.
 *
 * Array Creation
 * --------------
 *
 * ivr                        la_i_vr_create      irows
 * ivc                        la_i_vc_create      irows
 * imr                        la_i_mr_create      irows, icolumns  [, odiagonal]
 * imc                        la_i_mc_create      irows, icolumns  [, odiagonal_r, odiagonal_i]
 *
 * Array Introspection
 * -------------------
 *
 * irows                      la_i_size_vr        ivr
 * irows                      la_i_size_vc        ivc
 * irows, icolumns            la_i_size_mr        imr
 * irows, icolumns            la_i_size_mc        imc
 *
 *                            la_i_print_vr       ivr
 *                            la_i_print_vc       ivc
 *                            la_i_print_mr       imr
 *                            la_i_print_mc       imc
 *
 * Array Assignment and Conversion
 * -------------------------------
 *
 * ivr                        la_i_assign_vr      ivr
 * ivr                        la_k_assign_vr      ivr
 * ivc                        la_i_assign_vc      ivc
 * ivc                        la_k_assign_vc      ivr
 * imr                        la_i_assign_mr      imr
 * imr                        la_k_assign_mr      imr
 * imc                        la_i_assign_mc      imc
 * imc                        la_k_assign_mc      imr
 *
 * ivr                        la_k_assign_a       asig
 * ivr                        la_i_assign_t       itablenumber
 * ivr                        la_k_assign_t       itablenumber
 * ivc                        la_k_assign_f       fsig
 * 
 * asig                       la_k_a_assign       ivr
 * itablenum                  la_i_t_assign       ivr
 * itablenum                  la_k_t_assign       ivr
 * fsig                       la_k_f_assign       ivc
 *
 * Array Element Access
 * --------------------
 *
 * ivr                        la_i_vr_set         irow, ivalue
 * kvr                        la_k_vr_set         krow, kvalue
 * ivc                        la_i_vc_set         irow, ivalue_r, ivalue_i
 * kvc                        la_k_vc_set         krow, kvalue_r, kvalue_i
 * imr                        la_i mr_set         irow, icolumn, ivalue
 * kmr                        la_k mr_set         krow, kcolumn, ivalue
 * imc                        la_i_mc_set         irow, icolumn, ivalue_r, ivalue_i
 * kmc                        la_k_mc_set         krow, kcolumn, kvalue_r, kvalue_i
 * 
 * ivalue                     la_i_get_vr         ivr, irow      
 * kvalue                     la_k_get_vr         ivr, krow,     
 * ivalue_r, ivalue_i         la_i_get_vc         ivc, irow
 * kvalue_r, kvalue_i         la_k_get_vc         ivc, krow
 * ivalue                     la_i_get_mr         imr, irow, icolumn
 * kvalue                     la_k_get_mr         imr, krow, kcolumn
 * ivalue_r, ivalue_i         la_i_get_mc         imc, irow, icolumn
 * kvalue_r, kvalue_i         la_k_get_mc         imc, krow, kcolumn
 *
 * Single Array Operations
 * -----------------------
 *
 * imr                        la_i_transpose_mr   imr
 * imr                        la_k_transpose_mr   imr
 * imc                        la_i_transpose_mc   imc
 * imc                        la_k_transpose_mc   imc

 * ivr                        la_i_conjugate_vr   ivr
 * ivr                        la_k_conjugate_vr   ivr
 * ivc                        la_i_conjugate_vc   ivc
 * ivc                        la_k_conjugate_vc   ivc
 * imr                        la_i_conjugate_mr   imr
 * imr                        la_k_conjugate_mr   imr
 * imc                        la_i_conjugate_mc   imc
 * imc                        la_k_conjugate_mc   imc
 *
 * Scalar Operations
 * -----------------
 *
 * ir                         la_i_norm1_vr       ivr
 * kr                         la_k_norm1_vc       ivc
 * ir                         la_i_norm1_mr       imr
 * kr                         la_k_norm1_mc       imc
 *
 * ir                         la_i_norm2_vr       ivr
 * kr                         la_k_norm2_vc       ivc
 *
 * ir                         la_i_norm_max       imr
 * kr                         la_k_norm_max       imc
 *
 * ir                         la_i_norm_inf_vr    ivr
 * kr                         la_k_norm_inf_vc    ivc
 * ir                         la_i_norm_inf_mr    imr
 * kr                         la_k_norm_inf_mc    imc
 *
 * ir                         la_i_trace_mr       imr
 * kr                         la_k_trace_mr       imr
 * ir                         la_i_trace_mc       imc
 * kr                         la_k_trace_mc       imc
 *
 * ir                         la_i_lu_det         imr
 * kr                         la_k_lu_det         imr
 * ir                         la_i_lu_det         imc
 * kr                         la_k_lu_det         imc
 *
 * ivr                        la_i_scale_vr       ivr, ir
 * ivr                        la_k_scale_vr       ivr, ir
 * imr                        la_i_scale_mr       imr, ir
 * imr                        la_k_scale_mr       imr, ir
 *
 * Elementwise Array-Array Operations
 * ----------------------------------
 *
 * ivr                        la_i_add_vr         ivr_a, ivr_b
 * ivc                        la_k_add_vc         ivc_a, ivc_b
 * imr                        la_i_add_mr         imr_a, imr_b
 * imc                        la_k_add_mc         imc_a, imc_b
 *
 * ivr                        la_i_subtract_vr    ivr_a, ivr_b
 * ivc                        la_k_subtract_vc    ivc_a, ivc_b
 * imr                        la_i_subtract_mr    imr_a, imr_b
 * imc                        la_k_subtract_mc    imc_a, imc_b
 *
 * ivr                        la_i_multiply_vr    ivr_a, ivr_b
 * ivc                        la_k_multiply_vc    ivc_a, ivc_b
 * imr                        la_i_multiply_mr    imr_a, imr_b
 * imc                        la_k_multiply_mc    imc_a, imc_b
 *
 * ivr                        la_i_divide_vr      ivr_a, ivr_b
 * ivc                        la_k_divide_vc      ivc_a, ivc_b
 * imr                        la_i_divide_mr      imr_a, imr_b
 * imc                        la_k_divide_mc      imc_a, imc_b
 *
 * Inner Products
 * --------------
 * 
 * ir                         la_i_dot_vr_vr      ivr_a, ivr_b
 * kr                         la_k_dot_vr_vr      ivr_a, ivr_b
 * ir, ii                     la_i_dot_vc_vc      ivc_a, ivc_b
 * kr, ki                     la_k_dot_vc_vc      ivc_a, ivc_b
 *
 * ivr                        la_i_dot_mr_vr      imr_a, ivr_b
 * ivr                        la_k_dot_mr_vr      imr_a, ivr_b
 * ivc                        la_i_dot_mc_vc      imc_a, ivc_b
 * ivc                        la_k_dot_mc_vc      imc_a, ivc_b
 *
 * imr                        la_i_dot_mr_mr      imr_a, imr_b
 * imr                        la_k_dot_mr_mr      imr_a, imr_b
 * imc                        la_i_dot_mc_mc      imc_a, imc_b
 * imc                        la_k_dot_mc_mc      imc_a, imc_b
 *
 * Matrix Inversion
 * ----------------
 * 
 * imr                        la_i_invert_mr      imr
 * imr                        la_k_invert_mr      imr
 * imc                        la_i_invert_mc      imc
 * imc                        la_k_invert_mc      imc
 *
 * Matrix Decompositions
 * ---------------------
 *
 * imr                        la_i_upper_solve_mr   imr [, j_1_diagonal]
 * imr                        la_k_upper_solve_mr   imr [, j_1_diagonal]
 * imc                        la_i_upper_solve_mc   imc [, j_1_diagonal]
 * imc                        la_k_upper_solve_mc   imc [, j_1_diagonal]
 *
 * imr                        la_i_lower_solve_mr   imr [, j_1_diagonal]
 * imr                        la_k_lower_solve_mr   imr [, j_1_diagonal]
 * imc                        la_i_lower_solve_mc   imc [, j_1_diagonal]
 * imc                        la_k_lower_solve_mc   imc [, j_1_diagonal]
 *
 * imr                        la_i_lu_factor_mr     imr, ivr_pivot
 * imr                        la_k_lu_factor_mr     imr, kvr_pivot
 * imc                        la_i_lu_factor_mc     imc, ivc_pivot
 * imc                        la_k_lu_factor_mc     imc, ivc_pivot
 *
 * imr                        la_i_lu_solve_mr      imr, ivr_x, ivr_pivot
 * imr                        la_k_lu_solve_mr      imr, ivr_x, ivr_pivot
 * imc                        la_i_lu_solve_mc      imc, ivc_x, ivc_pivot
 * imc                        la_k_lu_solve_mc      imc, ivc_x, ivc_pivot
 *
 * imr_q, imr_r               la_i_qr_factor_mr     imr
 * imr_q, imr_r               la_k_qr_factor_mr     imr
 * imc_q, imc_r               la_i_qr_factor_mc     imc
 * imc_q, imc_r               la_k_qr_factor_mc     imc
 *
 * ivr_eig_vals               la_i_qr_eigen_mr      imr, i_tolerance
 * ivr_eig_vals               la_k_qr_eigen_mr      imr, k_tolerance
 * ivr_eig_vals               la_i_qr_eigen_mc      imc, i_tolerance
 * ivr_eig_vals               la_k_qr_eigen_mc      imc, k_tolerance
 *
 * ivr_eig_vals, imr_eig_vecs la_i_qr_sym_eigen_mr  imr, i_tolerance
 * ivr_eig_vals, imr_eig_vecs la_k_qr_sym_eigen_mr  imr, k_tolerance
 * ivc_eig_vals, imc_eig_vecs la_i_qr_sym_eigen_mc  imc, i_tolerance
 * ivc_eig_vals, imc_eig_vecs la_k_qr_sym_eigen_mc  imc, k_tolerance
 * 
 */

extern "C" 
{
  // a-rate, k-rate, FUNC, SPECDAT
#include <csdl.h> 
  // PVSDAT
#include <pstream.h>
}

#include <OpcodeBase.hpp>
#include <complex>
#include <sstream>
#include <vector>
#include <gmm/gmm.h>


template<typename A, typename F>
struct ArrayCaster
{
  union {
    A* a;
    F f;
  };
};

template<typename A, typename F> void tof(A *a, F *f)
{
  ArrayCaster<A, F> arrayCaster;
  arrayCaster.a = a;
  *f = arrayCaster.f;
};

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
  int init(CSOUND *csound)
  {
    toa(i_vr, lhs);
    ksmps = csound->GetKsmps(csound);
    gmm::resize(lhs->vr, ksmps);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    for (size_t i = 0; i < ksmps; ++i) {
      lhs->vr[i] = a_a[i];
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
    ksmps = csound->GetKsmps(csound);
    gmm::resize(rhs->vr, ksmps);
    return OK;
  }
  int kontrol(CSOUND *csound)
  {
    for (size_t i = 0; i < ksmps; ++i) {
      a_a[i] = rhs->vr[i];
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
    mc->mc(size_t(*i_row), size_t(*i_column)) = std::complex<MYFLT>(*i_value_r, *i_value_i);
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
    mc->mc(size_t(*k_row), size_t(*k_column)) = std::complex<MYFLT>(*k_value_r, *k_value_i);
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
    *i_value_r = vc->vc[size_t(*i_row)].real();
    *i_value_i = vc->vc[size_t(*i_row)].imag();
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
    *k_value_r = vc->vc[size_t(*k_row)].real();
    *k_value_i = vc->vc[size_t(*k_row)].imag();
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
    *i_value_r = mc->mc(size_t(*i_row), size_t(*i_column)).real();
    *i_value_i = mc->mc(size_t(*i_row), size_t(*i_column)).imag();
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
    *k_value_r = mc->mc(size_t(*k_row), size_t(*k_column)).real();
    *k_value_i = mc->mc(size_t(*k_row), size_t(*k_column)).imag();
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

extern "C" 
{

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    int status = 0;
    status |= csound->AppendOpcode(csound, 
				   "la_i_vr_create", 
				   sizeof(la_i_vr_create_t), 
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_vr_create_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_vc_create", 
				   sizeof(la_i_vr_create_t), 
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_vc_create_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_mr_create", 
				   sizeof(la_i_mr_create_t), 
				   1, 
				   "i", 
				   "iio", 
				   (int (*)(CSOUND*,void*)) &la_i_mr_create_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_mc_create", 
				   sizeof(la_i_mc_create_t), 
				   1, 
				   "i", 
				   "iioo", 
				   (int (*)(CSOUND*,void*)) &la_i_mc_create_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_size_vr", 
				   sizeof(la_i_size_vr_t), 
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_size_vr_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_size_vc", 
				   sizeof(la_i_size_vc_t), 
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_size_vc_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_size_mr", 
				   sizeof(la_i_size_mr_t), 
				   1, 
				   "ii", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_size_mr_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_size_mc", 
				   sizeof(la_i_size_mc_t), 
				   1, 
				   "ii", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_size_mc_t::init_, 
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_print_vr",
				   sizeof(la_i_print_vr_t),
				   1, 
				   "", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_print_vr_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_print_vc",
				   sizeof(la_i_print_vc_t),
				   1, 
				   "", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_print_vc_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_print_mr",
				   sizeof(la_i_print_mr_t),
				   1, 
				   "", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_print_mr_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_print_mc",
				   sizeof(la_i_print_mc_t),
				   1, 
				   "", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_print_mc_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_assign_vr",
				   sizeof(la_i_assign_vr_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_assign_vr_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_assign_vr",
				   sizeof(la_k_assign_vr_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_assign_vr_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_assign_vr_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_assign_vc",
				   sizeof(la_i_assign_vc_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_assign_vc_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_assign_vc",
				   sizeof(la_k_assign_vc_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_assign_vc_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_assign_vc_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_assign_mr",
				   sizeof(la_i_assign_mr_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_assign_mr_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_assign_mr",
				   sizeof(la_k_assign_mr_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_assign_mr_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_assign_mr_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_assign_mc",
				   sizeof(la_i_assign_mc_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_assign_mc_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_assign_mc",
				   sizeof(la_k_assign_mc_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_assign_mc_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_assign_mc_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_assign_a",
				   sizeof(la_k_assign_a_t),
				   2, 
				   "i", 
				   "a", 
				   (int (*)(CSOUND*,void*)) &la_k_assign_a_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_assign_a_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_assign_t",
				   sizeof(la_i_assign_t_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_assign_t_t::init_,
				   (int (*)(CSOUND*,void*)) &la_i_assign_t_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_assign_t",
				   sizeof(la_k_assign_t_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_assign_t_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_assign_t_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_assign_f",
				   sizeof(la_k_assign_f_t),
				   2, 
				   "i", 
				   "f", 
				   (int (*)(CSOUND*,void*)) &la_k_assign_f_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_assign_f_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_a_assign",
				   sizeof(la_k_a_assign_t),
				   2, 
				   "a", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_a_assign_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_a_assign_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0); 
    status |= csound->AppendOpcode(csound, 
				   "la_i_t_assign",
				   sizeof(la_i_t_assign_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_t_assign_t::init_,
				   (int (*)(CSOUND*,void*)) 0,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_t_assign",
				   sizeof(la_k_t_assign_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_t_assign_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_t_assign_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_f_assign",
				   sizeof(la_k_f_assign_t),
				   2, 
				   "f", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_f_assign_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_f_assign_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_vr_set",
				   sizeof(la_i_vr_set_t),
				   1, 
				   "i", 
				   "ii", 
				   (int (*)(CSOUND*,void*)) &la_i_vr_set_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_vr_set",
				   sizeof(la_k_vr_set_t),
				   2, 
				   "i", 
				   "kk", 
				   (int (*)(CSOUND*,void*)) &la_k_vr_set_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_vr_set_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_vc_set",
				   sizeof(la_i_vc_set_t),
				   1, 
				   "i", 
				   "iii", 
				   (int (*)(CSOUND*,void*)) &la_i_vc_set_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_vc_set",
				   sizeof(la_k_vc_set_t),
				   2, 
				   "i", 
				   "kkk",
				   (int (*)(CSOUND*,void*)) &la_k_vc_set_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_vc_set_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_mr_set",
				   sizeof(la_i_mr_set_t),
				   1, 
				   "i", 
				   "iii", 
				   (int (*)(CSOUND*,void*)) &la_i_mr_set_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
   status |= csound->AppendOpcode(csound, 
				   "la_k_mr_set",
				   sizeof(la_k_mr_set_t),
				   2, 
				   "i", 
				   "kkk", 
				   (int (*)(CSOUND*,void*)) &la_k_mr_set_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_mr_set_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_mc_set",
				   sizeof(la_i_mc_set_t),
				   1, 
				   "i", 
				   "iiii", 
				   (int (*)(CSOUND*,void*)) &la_i_mc_set_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_mc_set",
				   sizeof(la_k_mc_set_t),
				   2, 
				   "i", 
				   "kkkk", 
				   (int (*)(CSOUND*,void*)) &la_k_mc_set_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_mc_set_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);

    status |= csound->AppendOpcode(csound, 
				   "la_i_get_vr",
				   sizeof(la_i_get_vr_t),
				   1, 
				   "i", 
				   "ii", 
				   (int (*)(CSOUND*,void*)) &la_i_get_vr_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_get_vr",
				   sizeof(la_k_get_vr_t),
				   2, 
				   "k", 
				   "ik", 
				   (int (*)(CSOUND*,void*)) &la_k_get_vr_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_get_vr_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_get_vc",
				   sizeof(la_i_get_vc_t),
				   1, 
				   "ii",
				   "ii", 
				   (int (*)(CSOUND*,void*)) &la_i_get_vc_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_get_vc",
				   sizeof(la_k_get_vc_t),
				   2, 
				   "kk", 
				   "ik", 
				   (int (*)(CSOUND*,void*)) &la_k_get_vc_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_get_vc_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_get_mr",
				   sizeof(la_i_get_mr_t),
				   1, 
				   "i", 
				   "iii", 
				   (int (*)(CSOUND*,void*)) &la_i_get_mr_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_get_mr",
				   sizeof(la_k_get_mr_t),
				   2, 
				   "k", 
				   "ikk", 
				   (int (*)(CSOUND*,void*)) &la_k_get_mr_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_get_mr_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_get_mc",
				   sizeof(la_i_get_mc_t),
				   1, 
				   "ii",
				   "iii", 
				   (int (*)(CSOUND*,void*)) &la_i_get_mc_t::init_,
				   (int (*)(CSOUND*,void*)) 0, 
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_get_mc",
				   sizeof(la_k_get_mc_t),
				   2, 
				   "kk", 
				   "ikk", 
				   (int (*)(CSOUND*,void*)) &la_k_get_mc_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_get_mc_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_transpose_mr",
				   sizeof(la_i_transpose_mr_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_transpose_mr_t::init_,
				   (int (*)(CSOUND*,void*)) 0,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_transpose_mr",
				   sizeof(la_k_transpose_mr_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_transpose_mr_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_transpose_mr_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_transpose_mc",
				   sizeof(la_i_transpose_mc_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_transpose_mc_t::init_,
				   (int (*)(CSOUND*,void*)) 0,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_transpose_mc",
				   sizeof(la_k_transpose_mc_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_transpose_mc_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_transpose_mc_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_conjugate_vr",
				   sizeof(la_i_conjugate_vr_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_conjugate_vr_t::init_,
				   (int (*)(CSOUND*,void*)) 0,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_conjugate_vr",
				   sizeof(la_k_conjugate_vr_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_vr_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_vr_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_conjugate_vc",
				   sizeof(la_i_conjugate_vc_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_conjugate_vc_t::init_,
				   (int (*)(CSOUND*,void*)) 0,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_conjugate_vc",
				   sizeof(la_k_conjugate_vc_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_vc_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_vc_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_conjugate_mr",
				   sizeof(la_i_conjugate_mr_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_conjugate_mr_t::init_,
				   (int (*)(CSOUND*,void*)) 0,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_conjugate_mr",
				   sizeof(la_k_conjugate_mr_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_mr_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_mr_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_i_conjugate_mc",
				   sizeof(la_i_conjugate_mc_t),
				   1, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_i_conjugate_mc_t::init_,
				   (int (*)(CSOUND*,void*)) 0,
				   (int (*)(CSOUND*,void*)) 0);
    status |= csound->AppendOpcode(csound, 
				   "la_k_conjugate_mc",
				   sizeof(la_k_conjugate_mc_t),
				   2, 
				   "i", 
				   "i", 
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_mc_t::init_,
				   (int (*)(CSOUND*,void*)) &la_k_conjugate_mc_t::kontrol_,
				   (int (*)(CSOUND*,void*)) 0);
    return status;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    return 0;
  }
}
