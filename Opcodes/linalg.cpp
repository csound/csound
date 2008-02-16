/**
 * L I N E A R   A L G E B R A   O P C O D E S   F O R   C S O U N D
 * Michael Gogins
 *
 * These opcodes implement many linear algebra operations
 * from BLAS and LAPACK, up to and including eigenvalue decompositions.
 * They are designed to facilitate signal processing, 
 * and of course other mathematical operations, 
 * in the Csound orchestra language.
 *
 * The opcodes work with the following data types (and data type codes):
 * 1. Real scalars, which are Csound i-rate (ir) or k-rate scalars (kr), 
 *    where x stands for either i or k (xr).
 * 2. Complex scalars, which are ordered pairs of Csound scalars (xcr, xci).
 * 3. Allocated real vectors (xvr).
 * 4. Allocated complex vectors (xvc).
 * 5. Allocated real matrices (xmr).
 * 6. Allocated complex matrices (xmc).
 * 7. Storage for each allocated array is created using AUXCH,
 *    but it is used as a regular Csound variable,
 *    which is always a pointer to a floating-point number. 
 *    The size and type code of the array are found
 *    in the first 8 bytes of storage:
 *    bytes 0 through 4: number of elements in the array.
 *    bytes 5 through 7: type code (e.g. 'imc'), encoding rate, rank, type.
 *    byte  8:           first byte of the first element of the array.
 *
 * The BLAS vector-vector copy functions are overloaded, 
 * and can also be used for copying BLAS arrays 
 * to and from other Csound data types:
 * 1. Csound a-rate scalars (which are already vectors) 
 *    and function tables (i-rate scalars as function table numbers)
 *    can copied to and from real vectors (xvr)
 *    of the same size.
 * 2. Csound fsigs (PVS signals, PVSDAT) 
 *    and wsigs (spectral signals, SPECDAT) 
 *    can be copied to and from complex vectors (xvc)
 *    of the same size.
 *
 * Opcode names and signatures are determined as follows:
 * 1. All opcode names in this library are prefixed "la_" for "linear algebra".
 * 2. The opcode name is then additionally prefixed with a data type code.
 *    Because Csound variables have a fixed precision 
 *    (always single or always double), 
 *    Csound data types (r for real, c for complex) are used instead of 
 *    BLAS data types (S, D, C, Z). 
 * 3. The body of the opcode name is the same as 
 *    the body of the BLAS or LAPACK name.
 * 4. The return values and parameters are prefixed 
 *    first with the Csound rate, then with v for vector or m for matrix,
 *    then with the data type code, then (if a variable) with an underscore; 
 *    as previously noted complex scalars are pairs of real scalars.
 *    Thus, the i-rate BLAS complex scalar a is icr_a, ici_a;
 *    the k-rate BLAS complex vector x is kvc_x; and
 *    the i-rate or k-rate BLAS real matrix A is xvr_A.
 * 5. Because all Csound data types and allocated arrays 
 *    know their own size, BLAS and LAPACK size arguments are omitted,
 *    except from creator functions; this is similar to the FORTRAN 95
 *    interface to the BLAS routines.
 * 6. Operations that return scalars usually return i-rate or k-rate values
 *    on the left-hand side of the opcode.
 * 7. Operations that return vectors or matrices always copy the return value 
 *    into a preallocated argument on the right-hand side of the opcode.
 * 8. The rate (i-rate versus k-rate or a-rate) of either the return value,
 *    or the first argument if there is no return value, determines 
 *    the rate of the operation.  The exact same function is called 
 *    for i-rate, k-rate, and a-rate opcodes; 
 *    the only difference is that an i-rate opcode is evaluated only in the init pass,
 *    while a k-rate or a-rate opcode is evaluated in each kperiod pass.
 * 9. All arrays are 0-based.
 *10. All matrices are considered to be row-major 
 *    (x or i goes along rows, y or j goes along columns).
 *11. All arrays are considered to be general and dense; therefore, the BLAS 
 *    banded, Hermitian, symmetric, and sparse routines are not implemented.
 *12. At this time, only the general-purpose 'driver' routines 
 *    for LAPACK are implemented.
 *
 * For more complete information on how to use the opcodes, 
 * what the arguments are, which arguments are inputs, which arguments are outputs, 
 * and so on, consult BLAS and LAPACK documentation, 
 * e.g. http://www.intel.com/software/products/mkl/docs/WebHelp/mkl.htm.
 * 
 * The operations are:
 *
 * ALLOCATORS
 *
 * Allocate real vector.
 * xvr la_rv ir_size
 *
 * Allocate complex vector.
 * xvc la_cv ir_size
 *
 * Allocate real matrix, with optional diagonal.
 * xmr la_rm ir_rows, ir_columns [, or_diagonal]
 * 
 * Allocate complex matrix, with optional diagonal.
 * xmc la_cm irows, icolumns [, or_diagonal, oi_diagonal]
 *
 * LEVEL 1 BLAS -- VECTOR-VECTOR OPERATIONS
 *
 * Vector-vector dot product:
 * Return x * y.
 * xr       la_rdot  xvr_x, xvr_y
 * xcr, xci la_cdotu xvc_x, xvc_y
 *
 * Conjugate vector-vector dot product:
 * Return conjg(x) * y.
 * xcr, xci la_cdotc xvc_x, xvc_y
 *
 * Compute y := a * x + y.
 * la_raxpy xr_a, xvr_x, xvr_y
 * la_caxpy xr_a, xcr_x, xcr_y
 *
 * Givens rotation:
 * Given a point in a, b,
 * compute the Givens plane rotation in a, b, c, s
 * that zeros the y-coordinate of the point.
 * la_rrotg xr_a, xr_b, xr_c, xr_s
 * la_crotg xcr_a, xci_a, xcr_b, xci_b, xcr_c, xci_c, xcr_s, xci_s
 *
 * Rotate a vector x by a vector y:
 * For all i, x[i] = c * x[i] + s * y[i];
 * For all i, y[i] = c * y[i] - s * x[i].
 * la_rrot xvr_x, xvr_y, xr_c, xr_s
 * la_crot xvc_x, xvc_y, xr_c, xr_s
 *
 * Vector copy:
 * Compute y := x.
 * Note that x or y can be a-rate, function table number,
 * 
 * la_rcopy xvr_x, xvr_y
 * la_ccopy xcr_x, xvc_y
 *
 * Vector swap:
 * Swap vector x with vector y.
 * la_rswap xvr_x, xvr_y
 * la_cswap xcr_x, xvc_y
 *
 * Vector norm:
 * Return the Euclidean norm of vector x.
 * xr       rnrm2 xvr_x
 * xcr, xci cnrm2 xcr_x
 *
 * Vector absolute value:
 * Return the real sum of the absolute values of the vector elements.
 * xr asum xvr_x
 * xr axum xcr_x
 *
 * Vector scale:
 * Compute x := alpha * x.
 * la_rscal xr_alpha, xvr_x
 * la_cscal xcr_alpha, xci_alpha, xcr_x
 *
 * Vector absolute maximum:
 * Return the index of the maximum absolute value in vector x.
 * xr la_ramax xvr_x
 * xr la_camax xvc_x
 *
 * Vector absolute minimum:
 * Return the index of the minimum absolute value in vector x.
 * xr la_ramin xvr_x
 * xr la_camin xvc_x
 *
 * LEVEL 2 BLAS -- MATRIX-VECTOR OPERATIONS
 *
 * Matrix-vector dot product:
 * Compute y := alpha * A * x + beta * y, if trans is 0.
 * Compute y := alpha * A' * x + beta * y, if trans is 1.
 * Compute y := alpha * conjg(A') * x + beta * y, if trans is 2.
 * la_rgemv xmr_A, xvr_x, xvr_ay [, Pr_alpha][, or_beta][, or_trans]
 * la_cgemv xmc_A, xvc_x, xvc_ay [, Pcr_alpha, oci_alpha][, ocr_beta, oci_beta][, or_trans]
 *
 * Rank-1 update of a matrix:
 * Compute A := alpha * x * y' + A.
 * la_rger xr_alpha, xvr_x, xvr_y, xmr_A
 * la_cger xcr_alpha, xci_alpha, xvc_x, xvc_y, xmc_A
 *
 * Rank-1 update of a conjugated matrix:
 * Compute A := alpha * x * conjg(y') + A
 * la_rgerc xmr_A, xvr_x, xvr_y, [, Pr_alpha]
 * la_cgerc xmc_A, xvc_x, xvc_y, [, Pcr_alpha, oci_alpha]
 *
 * Solve a system of linear equations:
 * Coefficients are in a packed triangular matrix,
 * upper triangular if uplo = 0, lower triangular if uplo = 1.
 * Solve A * x = b for x, if trans = 0.
 * Solve A' * x = b for x, if trans = 1.
 * Solve conjg(A') * x = b for x, if trans = 2.
 * la_rtpsv xmr_A, xvr_x, xvr_b [, or_uplo][, or_trans]
 * la_ctpsv xmc_A, xvc_x, xvc_b [, or_uplo][, or_trans]
 *
 * LEVEL 3 BLAS -- MATRIX-MATRIX OPERATIONS
 * 
 * Matrix-matrix dot product:
 * Compute C := alpha * op(A) * op(B) + beta * C,
 * where transa or transb is 0 if op(X) is X, 1 if op(X) is X', or 2 if op(X) is conjg(X);
 * alpha and beta are 1 by default (set beta to 0 if you don't want to zero C first).
 * la_rgemm xmr_A, xmr_B, xmr_C [, or_transa][, or_transb][, Pr_alpha][, Pr_beta]
 * la_cgemm xmc_A, xmc_B, xmc_C [, or_transa][, or_transb][, Pcr_alpha, oci_alpha][, Pcr_beta, oci_beta]
 *
 * Solve a matrix equation:
 * Solve op(A) * X = alpha * B or X * op(A) = alpha * B for X,
 * where transa is 0 if op(X) is X, 1 if op(X) is X', or 2 if op(X) is conjg(X);
 * side is 0 for op(A) on the left, or 1 for op(A) on the right;
 * uplo is 0 for upper triangular A, or 1 for lower triangular A;
 * and diag is 0 for unit triangular A, or 1 for non-unit triangular A.
 * la_rtrsm xmr_A, xmr_B [, or_side][, or_uplo][, or_transa][, or_diag] [, Pr_alpha]
 * la_ctrsm xmc_A, cmc_B [, or_side][, or_uplo][, or_transa][, or_diag] [, Pcr_alpha, oci_alpha]
 * 
 * LAPACK -- LINEAR SOLUTIONS 
 *
 * Solve a system of linear equations:
 * Solve A * X = B, 
 * where A is a square matrix of coefficients,
 * the columns of B are individual right-hand sides,
 * ipiv is a vector for pivot elements,
 * the columns of B are replaced with the corresponding solutions,
 * and info is replaced with 0 for success, -i for a bad value in the ith parameter,
 * or i if U(i, i) is 0, i.e. U is singular.
 * la_rgesv xmr_A, xmr_B [, ovr_ipiv][, or_info]
 * la_cgesv xmc_A, xmc_B [, ovc_ipiv][, or_info]
 *
 * LAPACK -- LINEAR LEAST SQUARES PROBLEMS
 * 
 * QR or LQ factorization:
 * Uses QR or LQ factorization to solve an overdetermined or underdetermined 
 * linear system with full rank matrix, 
 * where A is the m x n matrix to factor;
 * B is an m x number of right-hand sides,
 * containing B as input and solution X as result;
 * if trans = 0, A is normal;
 * if trans = 1, A is transposed (real matrices only);
 * if trans = 2, A is conjugate-transposed (complex matrices only);
 * and info is replaced with 0 for success, -i for a bad value in the ith parameter,
 * or i if U(i, i) is 0, the i-th diagonal element of the triangular factor 
 * of A is zero, so that A does not have full rank; the least squares solution could not be computed.
 * la_rgels xmr_A, xmr_B [, or_trans] [, or_info]
 * 
 * LAPACK -- SINGULAR VALUE DECOMPOSITION
 *
 * Singular value decomposition of a general rectangular matrix:
 * Singular value decomposition of a general rectangular matrix by divide and conquer:
 *
 * LAPACK -- NONSYMMETRIX EIGENPROBLEMS
 *
 * Computes the eigenvalues and Schur factorization of a general matrix, 
 * and orders the factorization so that selected eigenvalues are at the top left of the Schur form:
 * Computes the eigenvalues and left and right eigenvectors of a general matrix:
 *
 * LAPACK -- GENERALIZED NONSYMMETRIC EIGENPROBLEMS
 *
 * Compute the generalized eigenvalues, Schur form, 
 * and the left and/or right Schur vectors for a pair of nonsymmetric matrices:
 * Computes the generalized eigenvalues, and the left and/or right generalized eigenvectors 
 * for a pair of nonsymmetric matrices:
 * 
 */
extern "C" 
{
  // a-rate, k-rate, FUNC, SPECDAT
#include <csoundCore.h> 
  // PVSDAT
#include <pstream.h>
}

/**
 * Used for all types of arrays
 * (vectors and matrices, real and complex).
 */
struct BLASArray
{
  size_t elements;
  char[4] typecode;
  MYFLT *data;
};

#include <OpcodeBase.hpp>


