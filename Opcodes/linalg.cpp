/**
 * L I N E A R   A L G E B R A   O P C O D E S
 * Michael Gogins
 *
 * These opcodes implement many BLAS and LAPACK 
 * linear algebra operations, up to and including eigenvalue decompositions.
 * They are designed to facilitate MATLAB style signal processing
 * in the Csound orchestra language, and of course other mathematical
 * operations.
 *
 * The opcodes work with any combination of the the following data types:
 * 1. Csound i-rate and k-rate variables, as real (r) or complex (cr, ci) scalars.
 * 2. Csound a-rate signals and variables, as real vectors (rv) of size ksmps.
 * 3. Csound function tables (FUNC), as real vectors (rv).
 * 4. Csound fsigs (PVS signals, PVSDAT), as complex vectors (cv).
 * 5. Csound wsigs (spectral signals, SPECDAT), as complex vectors (cv).
 * 6. Allocated vectors, real and complex, considered to be 
 *    column vectors (rv, rc).
 * 7. Allocated matrices, real and complex (mr, mc), considered to be
 *    row-major (x or i goes along rows, y or j goes along columns).
 * 8. In the opcode signatures, the type codes are prefixed first 
 *    with the Csound rate, then with the data type, then with an
 *    underscore. Thus, the i-rate BLAS complex vector x is ivc_x;
 *    the i-rate or k-rate BLAS real vector y is xvr_y.
 * 9. As in BLAS, stride arguments can be negative.
 *
 * Note the following limitations and caveats:
 * 1. All opcodes in this library are prefixed "la_" for "Linear Algebra".
 * 2. The body of the opcode name is taken from the BLAS or LAPACK name.
 *    The BLAS type codes (S, D, C, Z) are omitted from the name, because
 *    Csound infers the BLAS types from the Csound types of the arguments
 *    and return values.
 * 3. Operations that return scalars return i-rate or k-rate values
 *    on the left-hand side of the opcode.
 * 4. Operations that return vectors or matrices copy the return value 
 *    into a preallocated argument on the right-hand side of the opcode.
 * 5. Because all Csound data types know their own size,
 *    the BLAS and LAPACK size arguments are omitted,
 *    except from creator functions.
 * 6. For double-precision Csound, all array elements are doubles;
 *    for single-precision Csound, all array elements are floats.
 * 7. All arrays are row-major.
 * 8. All arrays are 0-based.
 * 9. All arrays are dense.
 *10. The rate (i-rate versus k-rate or a-rate) of the result (return value)
 *    determines the rate of the operation. The exact same function 
 *    is called for i-rate, k-rate, and a-rate; the only difference is that
 *    a function with an i-rate result is evaluated only in the init pass,
 *    while the same function with a k-rate or a-rate result is evaluated 
 *    for each kperiod pass.
 *
 * To understand how to use the opcodes, what the arguments are, 
 * which arguments are inputs, which arguments are outputs, and so on, 
 * consult the BLAS and LAPACK documentation. The operations are:
 *
 * ALLOCATORS
 *
 * Allocate real vector.
 * ivr la_rv ir_size
 *
 * Allocate complex vector.
 * ivc la_cv ir_size
 *
 * Allocate real matrix, with optional diagonal.
 * imr la_rm ir_rows, ir_columns [, jr_diagonal]
 * 
 * Allocate complex matrix, with optional diagonal.
 * imc la_cm irows, icolumns [, jr_diagonal, ji_diagonal]
 *
 * LEVEL 1 BLAS -- VECTOR-VECTOR OPERATIONS
 *
 * Generate a Givens plane rotation.
 * la_rotg xvr_a, xvr_b, xvr_c, xvr_s
 * la_rotg xvc_a, xvc_b, xvc_c, xvc_s
 *
 * Apply a Givens plane rotation to a set of points.
 * la_rot xvr_x, xr_xstride, xvr_y, xr_ystride, xr_c, xr_s
 * la_rot xvc_x, xr_xstride, xvc_y, xr_ystride, xr_c, xr_s
 *
 * Scale a vector by a scalar.
 * la_scal xr_alpha, xvr, xvr_stride
 * la_scal xr_alpha, xcr, xvr_stride
 *
 * Copy vector x into vector y.
 * la_copy xvr_x, xr_xstride, xvr_y, xr_ystride
 * la_copy xcr_x, xr_xstride, xvc_y, xr_ystride
 *
 * Add a scalar multiple of vector x to to another vector y.
 * la_axpy xr_a, xvr_x, xr_xstride, xvr_y, xr_ystride
 * la_axpy xr_a, xcr_x, xr_xstride, xcr_y, xr_ystride
 *
 * Return the dot product of vectors x and y.
 * xr la_dot xvr_x, xr_xstride, xvr_y, xr_ystride
 * xcr, xci la_dotu xvc_x, xr_xstride, xvc_y, xr_ystride
 *
 * Return the dot product of the conjugate of vector x with vector y.
 * xcr, xci la_dotc xvc_x, xr_xstride, xvc_y, xr_ystride
 *
 * Return the Euclidean norm of the vector.
 * xr nrm2 xvr_x, xr_xstride
 * xcr, xci nrm2 xcr_x, xr_xstride
 *
 * Return the real sum of the absolute values of the vector elements.
 * xr asum xvr_x, xr_xstride
 * xr axum xcr_x, xr_xstride

CBLAS INDEX
cblas i♦amax
 const TYPE *X, const int incX) amax ← 1stk ∋ |re(xk)| + |im(xk)| S,D,C,Z
 * Level 2 BLAS
la_gemv ( const enum CBLAS ORDER Order, const enum CBLAS TRANSPOSE TransA, const int M, const int N, SCALAR
alpha, const TYPE *A, const int lda, const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y, y ← AT x + y,
y ← AHx + y, A − m × n
S,D,C,Z
la_gbmv ( const enum CBLAS ORDER Order, const enum CBLAS TRANSPOSE TransA, const int M, const int N, const int
KL, const int KU, SCALAR alpha, const TYPE *A, const int lda, const TYPE *X, const int incX, SCALAR beta,
TYPE *Y, const int incY )
y ← Ax + y, y ← AT x + y,
y ← AHx + y, A − m × n
S,D,C,Z
la_hemv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *A,
const int lda, const void *X, const int incX, const void *beta, void *Y, const int incY )
y ← Ax + y C,Z
void la_hbmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const int K, const void *alpha,
const void *A, const int lda, const void *X, const int incX, const void *beta, void *Y, const int incY )
y ← Ax + y C,Z
void la_hpmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *Ap,
const void *X, const int incX, const void *beta, void *Y, const int incY )
y ← Ax + y C,Z
void la_symv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *A,
const int lda, const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y S,D
void la_sbmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const int K, SCALAR alpha, const
TYPE *A, const int lda, const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y S,D
void la_spmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *Ap,
const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y S,D
void la_trmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← Ax, x ← AT x, x ← AHx S,D,C,Z
void la_tbmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const int K, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← Ax, x ← AT x, x ← AHx S,D,C,Z
void la_tpmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *Ap, TYPE *X, const int incX )
x ← Ax, x ← AT x, x ← AHx S,D,C,Z
void la_trsv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← A−1x, x ← A−T x, x ← A−Hx S,D,C,Z
void la_tbsv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const int K, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← A−1x, x ← A−T x, x ← A−Hx S,D,C,Z
void la_tpsv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *Ap, TYPE *X, const int incX )
x ← A−1x, x ← A−T x, x ← A−Hx S,D,C,Z
void la_ger ( const enum CBLAS ORDER Order, const int M, const int N, SCALAR alpha, const TYPE *X, const int incX, const
TYPE *Y, const int incY, TYPE *A, const int lda )
A ← xyT + A, A − m × n S,D
void la_geru ( const enum CBLAS ORDER Order, const int M, const int N, const void *alpha, const void *X, const int incX, const
void *Y, const int incY, void *A, const int lda )
A ← xyT + A, A − m × n C,Z
void la_gerc ( const enum CBLAS ORDER Order, const int M, const int N, const void *alpha, const void *X, const int incX, const
void *Y, const int incY, void *A, const int lda )
A ← xyH + A, A − m × n C,Z
void la_her ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const UTYPE alpha, const void
*X, const int incX, void *A, const int lda )
A ← xxH + A C,Z
void la_hpr ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const UTYPE alpha, const void
*X, const int incX, void *A )
A ← xxH + A C,Z
void la_her2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *X,
const int incX, const void *Y, const int incY, void *A, const int lda )
A ← xyH + y(x)H + A C,Z
void la_hpr2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *X,
const int incX, const void *Y, const int incY, void *Ap )
A ← xyH + y(x)H + A C,Z
void la_syr ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, TYPE *A, const int lda )
A ← xxT + A S,D
void la_spr ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, TYPE *Ap )
A ← xxT + A S,D
void la_syr2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, const TYPE *Y, const int incY, TYPE *A, const int lda )
A ← xyT + yxT + A S,D
void la_spr2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, const TYPE *Y, const int incY, TYPE *A )
A ← xyT + yxT + A S,
 * Level 3 BLAS
void la_gemm ( const enum CBLAS ORDER Order, const enum CBLAS TRANSPOSE TransA, const enum CBLAS TRANSPOSE
TransB, const int M, const int N, const int K, const SCALAR alpha, const TYPE *A, const int lda, const TYPE *B,
const int ldb, const SCALAR beta, TYPE *C, const int ldc )
C ← op(A)op(B) + C,
op(X) = X,XT ,XH, C − m × n
S,D,C,Z
void la_symm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const int M,
const int N, SCALAR alpha, const TYPE *A, const int lda, const TYPE *B, const int ldb, SCALAR beta, TYPE *C,
const int ldc )
C ← AB + C, C ← BA + C, C −
m × n, A = AT
S,D,C,Z
void la_hemm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const int M,
const int N, const void *alpha, const void *A, const int lda, const void *B, const int ldb, const void *beta, void *C,
const int ldc )
C ← AB + C, C ← BA + C, C −
m × n,A = AH
C,Z
void la_syrk ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, SCALAR alpha, const TYPE *A, const int lda, SCALAR beta, TYPE *C, const int ldc )
C ← AAT + C, C ← ATA + C,
C − n × n
S,D,C,Z
void la_herk ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, const UTYPE alpha, const void *A, const int lda, const UTYPE beta, void *C, const int ldc )
C ← AAH + C, C ← AHA + C,
C − n × n
C,Z
void la_syr2k ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, SCALAR alpha, const TYPE *A, const int lda, const TYPE *B, const int ldb, SCALAR beta, TYPE
*C, const int ldc )
C ← ABT + ¯BAT +C, C ← ATB+
¯BTA + C, C − n × n
S,D,C,Z
void la_her2k ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, const void *alpha, const void *A, const int lda, const void *B, const int ldb, const UTYPE beta,
void *C, const int ldc )
C ← ABH+¯BAH+C, C ← AHB+
¯BHA + C, C − n × n
C,Z
void la_trmm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const enum
CBLAS TRANSPOSE TransA, const enum CBLAS DIAG Diag, const int M, const int N, SCALAR alpha, const TYPE
*A, const int lda, TYPE *B, const int ldb )
B ← op(A)B, B ← Bop(A), op(A) =
A,AT ,AH, B − m × n
S,D,C,Z
void la_trsm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const enum
CBLAS TRANSPOSE TransA, const enum CBLAS DIAG Diag, const int M, const int N, SCALAR alpha, const TYPE
*A, const int lda, TYPE *B, const int ldb )
B ← op(A−1)B, B ← Bop(A−1),
op(A) = A,AT ,AH, B − m × n
S,D,C,Z
 * LAPACK
Simple Drivers 
Simple Driver Routines for Linear Equations 
Matrix Type Routine 
General SGESV( N, NRHS, A, LDA, IPIV, B, LDB, INFO ) 
CGESV( N, NRHS, A, LDA, IPIV, B, LDB, INFO ) 
General Band SGBSV( N, KL, KU, NRHS, AB, LDAB, IPIV, B, LDB, INFO ) 
CGBSV( N, KL, KU, NRHS, AB, LDAB, IPIV, B, LDB, INFO ) 
General Tridiagonal SGTSV( N, NRHS, DL, D, DU, B, LDB, INFO ) 
CGTSV( N, NRHS, DL, D, DU, B, LDB, INFO ) 
Symmetric/Hermitian SPOSV( UPLO, N, NRHS, A, LDA, B, LDB, INFO ) 
Positive Definite CPOSV( UPLO, N, NRHS, A, LDA, B, LDB, INFO ) 
Symmetric/Hermitian SPPSV( UPLO, N, NRHS, AP, B, LDB, INFO ) 
Positive Definite (Packed Storage) CPPSV( UPLO, N, NRHS, AP, B, LDB, INFO ) 
Symmetric/Hermitian SPBSV( UPLO, N, KD, NRHS, AB, LDAB, B, LDB, INFO ) 
Positive Definite Band CPBSV( UPLO, N, KD, NRHS, AB, LDAB, B, LDB, INFO ) 
Symmetric/Hermitian SPTSV( N, NRHS, D, E, B, LDB, INFO ) 
Positive Definite Tridiagonal CPTSV( N, NRHS, D, E, B, LDB, INFO ) 
Symmetric/Hermitian SSYSV( UPLO, N, NRHS, A, LDA, IPIV, B, LDB, WORK, LWORK, INFO ) 
Indefinite CSYSV( UPLO, N, NRHS, A, LDA, IPIV, B, LDB, WORK, LWORK, INFO ) 
CHESV( UPLO, N, NRHS, A, LDA, IPIV, B, LDB, WORK, LWORK, INFO ) 
Symmetric/Hermitian SSPSV( UPLO, N, NRHS, AP, IPIV, B, LDB, INFO ) 
Indefinite (Packed Storage) CSPSV( UPLO, N, NRHS, AP, IPIV, B, LDB, INFO ) 
CHPSV( UPLO, N, NRHS, AP, IPIV, B, LDB, INFO ) 
Simple Driver Routines for Standard and Generalized Linear Least Squares Problems 
Problem Type Routine 
Solve Using Orthogonal Factor, SGELS( TRANS, M, N, NRHS, A, LDA, B, LDB, WORK, LWORK, INFO ) 
Assuming Full Rank CGELS( TRANS, M, N, NRHS, A, LDA, B, LDB, WORK, LWORK, INFO ) 
Solve LSE Problem Using GRQ SGGLSE( M, N, P, A, LDA, B, LDB, C, D, X, WORK, LWORK, INFO ) 
CGGLSE( M, N, P, A, LDA, B, LDB, C, D, X, WORK, LWORK, INFO ) 
Solve GLM Problem Using GQR SGGGLM( N, M, P, A, LDA, B, LDB, D, X, Y, WORK, LWORK, INFO ) 
CGGGLM( N, M, P, A, LDA, B, LDB, D, X, Y, WORK, LWORK, INFO ) 

Simple and Divide and Conquer Driver Routines for Standard Eigenvalue and Singular Value Problems 
Matrix/Problem Type Routine 
Symmetric/Hermitian SSYEV( JOBZ, UPLO, N, A, LDA, W, WORK, LWORK, INFO ) 
Eigenvalues/vectors CHEEV( JOBZ, UPLO, N, A, LDA, W, WORK, LWORK, RWORK, INFO ) 
Divide and Conquer SSYEVD( JOBZ, UPLO, N, A, LDA, W, WORK, LWORK, IWORK, LIWORK, INFO ) 
CHEEVD( JOBZ, UPLO, N, A, LDA, W, WORK, LWORK, RWORK, LRWORK, IWORK, LIWORK, INFO ) 
Symmetric/Hermitian SSPEV( JOBZ, UPLO, N, AP, W, Z, LDZ, WORK, INFO ) 
(Packed Storage) CHPEV( JOBZ, UPLO, N, AP, W, Z, LDZ, WORK, RWORK, INFO ) 
Eigenvalues/vectors 
Divide and Conquer SSPEVD( JOBZ, UPLO, N, AP, W, Z, LDZ, WORK, LWORK, IWORK, LIWORK, INFO ) 
CHPEVD( JOBZ, UPLO, N, AP, W, Z, LDZ, WORK, LWORK, RWORK, LRWORK, IWORK, LIWORK, INFO ) 
Symmetric/Hermitian Band SSBEV( JOBZ, UPLO, N, KD, AB, LDAB, W, Z, LDZ, WORK, INFO ) 
Eigenvalues/vectors CHBEV( JOBZ, UPLO, N, KD, AB, LDAB, W, Z, LDZ, WORK, RWORK, INFO ) 
Divide and Conquer SSBEVD( JOBZ, UPLO, N, KD, AB, LDAB, W, Z, LDZ, WORK, LWORK, IWORK, LIWORK, INFO ) 
CHBEVD( JOBZ, UPLO, N, KD, AB, LDAB, W, Z, LDZ, WORK, LWORK, RWORK, LRWORK, IWORK, LIWORK, INFO ) 
Symmetric Tridiagonal SSTEV( JOBZ, N, D, E, Z, LDZ, WORK, INFO ) 
Eigenvalues/vectors 
Divide and Conquer SSTEVD( JOBZ, N, D, E, Z, LDZ, WORK, LWORK, IWORK, LIWORK, INFO ) 
General SGEES( JOBVS, SORT, SELECT, N, A, LDA, SDIM, WR, WI, VS, LDVS, WORK, LWORK, BWORK, INFO ) 
Schur Factorization CGEES( JOBVS, SORT, SELECT, N, A, LDA, SDIM, W, VS, LDVS, WORK, LWORK, RWORK, BWORK, INFO ) 
General SGEEV( JOBVL, JOBVR, N, A, LDA, WR, WI, VL, LDVL, VR, LDVR, WORK, LWORK, INFO ) 
Eigenvalues/vectors CGEEV( JOBVL, JOBVR, N, A, LDA, W, VL, LDVL, VR, LDVR, WORK, LWORK, RWORK, INFO ) 
General SGESVD( JOBU, JOBVT, M, N, A, LDA, S, U, LDU, VT, LDVT, WORK, LWORK, INFO ) 
Singular Values/Vectors CGESVD( JOBU, JOBVT, M, N, A, LDA, S, U, LDU, VT, LDVT, WORK, LWORK, RWORK, INFO ) 
Divide and Conquer SGESDD( JOBZ, M, N, A, LDA, S, U, LDU, VT, LDVT, WORK, LWORK, IWORK, INFO ) 
CGESDD( JOBZ, M, N, A, LDA, S, U, LDU, VT, LDVT, WORK, LWORK, RWORK, IWORK, INFO ) 
Simple and Divide and Conquer Driver Routines for Generalized Eigenvalue and Singular Value Problems 
Matrix/Problem Type Routine 
Symmetric­definite SSYGV( ITYPE, JOBZ, UPLO, N, A, LDA, B, LDB, W, WORK, LWORK, INFO ) 
Eigenvalues/vectors CHEGV( ITYPE, JOBZ, UPLO, N, A, LDA, B, LDB, W, WORK, LWORK, RWORK, INFO ) 
Divide and Conquer SSYGVD( ITYPE, JOBZ, UPLO, N, A, LDA, B, LDB, W, WORK, LWORK, IWORK, LIWORK, INFO ) 
CHEGVD( ITYPE, JOBZ, UPLO, N, A, LDA, B, LDB, W, WORK, LWORK, RWORK, LRWORK, IWORK, LIWORK, INFO ) 
Symmetric­definite SSPGV( ITYPE, JOBZ, UPLO, N, AP, BP, W, Z, LDZ, WORK, INFO ) 
(Packed Storage) CHPGV( ITYPE, JOBZ, UPLO, N, AP, BP, W, Z, LDZ, WORK, RWORK, INFO ) 
Eigenvalues/vectors 
Divide and Conquer SSPGVD( ITYPE, JOBZ, UPLO, N, AP, BP, W, Z, LDZ, WORK, LWORK, IWORK, LIWORK, INFO ) 
CHPGVD( ITYPE, JOBZ, UPLO, N, AP, BP, W, Z, LDZ, WORK, LWORK, RWORK, LRWORK, IWORK, LIWORK, INFO ) 
Symmetric­definite SSBGV( JOBZ, UPLO, N, KA, KB, AB, LDAB, BB, LDBB, W, Z, LDZ, WORK, INFO ) 
(Band Storage) CHBGV( JOBZ, UPLO, N, KA, KB, AB, LDAB, BB, LDBB, W, Z, LDZ, WORK, RWORK, INFO ) 
Eigenvalues/vectors 
Divide and Conquer SSBGVD( JOBZ, UPLO, N, KA, KB, AB, LDAB, BB, LDBB, W, Z, LDZ, WORK, LWORK, IWORK, LIWORK, INFO ) 
CHBGVD( JOBZ, UPLO, N, KA, KB, AB, LDAB, BB, LDBB, W, Z, LDZ, WORK, LWORK, RWORK, LRWORK, IWORK, LIWORK, INFO ) 
General SGGES( JOBVSL, JOBVSR, SORT, SELCTG, N, A, LDA, B, LDB, SDIM, ALPHAR, ALPHAI, BETA, VSL, LDVSL, VSR, LDVSR, WORK, LWORK, BWORK, INFO ) 
Schur Factorization CGGES( JOBVSL, JOBVSR, SORT, SELCTG, N, A, LDA, B, LDB, SDIM, ALPHA, BETA, VSL, LDVSL, VSR, LDVSR, WORK, LWORK, RWORK, BWORK, INFO ) 
General SGGEV( JOBVL, JOBVR, N, A, LDA, B, LDB, ALPHAR, ALPHAI, BETA, VL, LDVL, VR, LDVR, WORK, LWORK, INFO ) 
Eigenvalues/vectors CGGEV( JOBVL, JOBVR, N, A, LDA, B, LDB, ALPHA, BETA, VL, LDVL, VR, LDVR, WORK, LWORK, RWORK, INFO ) 
General SGGSVD( JOBU, JOBV, JOBQ, M, N, P, K, L, A, LDA, B, LDB, ALPHA, BETA, U, LDU, V, LDV, Q, LDQ, WORK, IWORK, INFO ) 
Singular Values/Vectors CGGSVD( JOBU, JOBV, JOBQ, M, N, P, K, L, A, LDA, B, LDB, ALPHA, BETA, U, LDU, V, LDV, Q, LDQ, WORK, RWORK, IWORK, INFO ) 

Expert Drivers 
Expert Driver Routines for Linear Equations 
Matrix Type Routine 
General SGESVX( FACT, TRANS, N, NRHS, A, LDA, AF, LDAF, IPIV, EQUED, R, C, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, IWORK, INFO ) 
CGESVX( FACT, TRANS, N, NRHS, A, LDA, AF, LDAF, IPIV, EQUED, R, C, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
General Band SGBSVX( FACT, TRANS, N, KL, KU, NRHS, AB, LDAB, AFB, LDAFB, IPIV, EQUED, R, C, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, IWORK, INFO ) 
CGBSVX( FACT, TRANS, N, KL, KU, NRHS, AB, LDAB, AFB, LDAFB, IPIV, EQUED, R, C, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
General Tridiagonal SGTSVX( FACT, TRANS, N, NRHS, DL, D, DU, DLF, DF, DUF, DU2, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, IWORK, INFO ) 
CGTSVX( FACT, TRANS, N, NRHS, DL, D, DU, DLF, DF, DUF, DU2, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
Symmetric/Hermitian SPOSVX( FACT, UPLO, N, NRHS, A, LDA, AF, LDAF, EQUED, S, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, IWORK, INFO ) 
Positive Definite CPOSVX( FACT, UPLO, N, NRHS, A, LDA, AF, LDAF, EQUED, S, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
Symmetric/Hermitian SPPSVX( FACT, UPLO, N, NRHS, AP, AFP, EQUED, S, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, IWORK, INFO ) 
Positive Definite (Packed Storage) CPPSVX( FACT, UPLO, N, NRHS, AP, AFP, EQUED, S, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
Symmetric/Hermitian SPBSVX( FACT, UPLO, N, KD, NRHS, AB, LDAB, AFB, LDAFB, EQUED, S, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, IWORK, INFO ) 
Positive Definite Band CPBSVX( FACT, UPLO, N, KD, NRHS, AB, LDAB, AFB, LDAFB, EQUED, S, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
Symmetric/Hermitian SPTSVX( FACT, N, NRHS, D, E, DF, EF, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, INFO ) 
Positive Definite Tridiagonal CPTSVX( FACT, N, NRHS, D, E, DF, EF, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
Symmetric/Hermitian SSYSVX( FACT, UPLO, N, NRHS, A, LDA, AF, LDAF, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, LWORK, IWORK, INFO ) 
Indefinite CSYSVX( FACT, UPLO, N, NRHS, A, LDA, AF, LDAF, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, LWORK, RWORK, INFO ) 
CHESVX( FACT, UPLO, N, NRHS, A, LDA, AF, LDAF, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, LWORK, RWORK, INFO ) 
Symmetric/Hermitian SSPSVX( FACT, UPLO, N, NRHS, AP, AFP, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, IWORK, INFO ) 
Indefinite (Packed Storage) CSPSVX( FACT, UPLO, N, NRHS, AP, AFP, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
CHPSVX( FACT, UPLO, N, NRHS, AP, AFP, IPIV, B, LDB, X, LDX, RCOND, FERR, BERR, WORK, RWORK, INFO ) 
Divide and Conquer and Expert Driver Routines for Linear Least Squares Problems 
Problem Type Routine 
Solve Using Orthogonal Factor SGELSY( M, N, NRHS, A, LDA, B, LDB, JPVT, RCOND, RANK, WORK, LWORK, INFO ) 
CGELSY( M, N, NRHS, A, LDA, B, LDB, JPVT, RCOND, RANK, WORK, LWORK, RWORK, INFO ) 
Solve Using SVD, SGELSS( M, N, NRHS, A, LDA, B, LDB, S, RCOND, RANK, WORK, LWORK, INFO ) 
Allowing for Rank­Deficiency CGELSS( M, N, NRHS, A, LDA, B, LDB, S, RCOND, RANK, WORK, LWORK, RWORK, INFO ) 
Solve Using D&C SVD, SGELSD( M, N, NRHS, A, LDA, B, LDB, S, RCOND, RANK, WORK, LWORK, IWORK, INFO ) 
Allowing for Rank­Deficiency CGELSD( M, N, NRHS, A, LDA, B, LDB, S, RCOND, RANK, WORK, LWORK, RWORK, IWORK, INFO ) 

Expert and RRR Driver Routines for Standard and Generalized Symmetric Eigenvalue Problems 
Matrix/Problem Type Routine 
Symmetric/Hermitian SSYEVX( JOBZ, RANGE, UPLO, N, A, LDA, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, LWORK, IWORK, IFAIL, INFO ) 
Eigenvalues/vectors CHEEVX( JOBZ, RANGE, UPLO, N, A, LDA, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, LWORK, RWORK, IWORK, IFAIL, INFO ) 
SSYEVR( JOBZ, RANGE, UPLO, N, A, LDA, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, ISUPPZ, WORK, LWORK, IWORK, LIWORK, INFO ) 
CHEEVR( JOBZ, RANGE, UPLO, N, A, LDA, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, ISUPPZ, WORK, LWORK, RWORK, LRWORK, IWORK, LIWORK, INFO ) 
SSYGVX( ITYPE, JOBZ, RANGE, UPLO, N, A, LDA, B, LDB, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, LWORK, IWORK, IFAIL, INFO ) 
CHEGVX( ITYPE, JOBZ, RANGE, UPLO, N, A, LDA, B, LDB, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, LWORK, RWORK, IWORK, IFAIL, INFO ) 
Symmetric/Hermitian SSPEVX( JOBZ, RANGE, UPLO, N, AP, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, IWORK, IFAIL, INFO ) 
(Packed Storage) CHPEVX( JOBZ, RANGE, UPLO, N, AP, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, RWORK, IWORK, IFAIL, INFO ) 
Eigenvalues/vectors 
SSPGVX( ITYPE, JOBZ, RANGE, UPLO, N, AP, BP, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, IWORK, IFAIL, INFO ) 
CHPGVX( ITYPE, JOBZ, RANGE, UPLO, N, AP, BP, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, RWORK, IWORK, IFAIL, INFO ) 
Symmetric/Hermitian Band SSBEVX( JOBZ, RANGE, UPLO, N, KD, AB, LDAB, Q, LDQ, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, IWORK, IFAIL, INFO ) 
Eigenvalues/vectors CHBEVX( JOBZ, RANGE, UPLO, N, KD, AB, LDAB, Q, LDQ, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, RWORK, IWORK, IFAIL, INFO ) 
SSBGVX( JOBZ, RANGE, UPLO, N, KA, KB, AB, LDAB, BB, LDBB, Q, LDQ, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, IWORK, IFAIL, INFO ) 
CHBGVX( JOBZ, RANGE, UPLO, N, KA, KB, AB, LDAB, BB, LDBB, Q, LDQ, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, RWORK, IWORK, IFAIL, INFO ) 
Symmetric Tridiagonal SSTEVX( JOBZ, RANGE, N, D, E, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, WORK, IWORK, IFAIL, INFO ) 
Eigenvalues/vectors 
SSTEVR( JOBZ, RANGE, N, D, E, VL, VU, IL, IU, ABSTOL, M, W, Z, LDZ, ISUPPZ, WORK, LWORK, IWORK, LIWORK, INFO ) 
Expert Driver Routines for Standard and Generalized Nonsymmetric Eigenvalue Problems 
Problem Type Routine 
Schur SGEESX( JOBVS, SORT, SELECT, SENSE, N, A, LDA, SDIM, WR, WI, VS, LDVS, RCONDE, RCONDV, WORK, LWORK, IWORK, LIWORK, BWORK, INFO ) 
Factorization CGEESX( JOBVS, SORT, SELECT, SENSE, N, A, LDA, SDIM, W, VS, LDVS, RCONDE, RCONDV, WORK, LWORK, RWORK, BWORK, INFO ) 
SGGESX( JOBVSL, JOBVSR, SORT, SELCTG, SENSE, N, A, LDA, B, LDB, SDIM, ALPHAR, ALPHAI, BETA, VSL, LDVSL, VSR, LDVSR, RCONDE, RCONDV, WORK, LWORK, IWORK, LIWORK, BWORK, INFO ) 
CGGESX( JOBVSL, JOBVSR, SORT, SELCTG, SENSE, N, A, LDA, B, LDB, SDIM, ALPHAR, ALPHAI, BETA, VSL, LDVSL, VSR, LDVSR, RCONDE, RCONDV, WORK, LWORK, RWORK, IWORK, LIWORK, BWORK, INFO ) 
Eigenvalues/ SGEEVX( BALANC, JOBVL, JOBVR, SENSE, N, A, LDA, WR, WI, VL, LDVL, VR, LDVR, ILO, IHI, SCALE, ABNRM, RCONDE, RCONDV, WORK, LWORK, IWORK, INFO ) 
vectors CGEEVX( BALANC, JOBVL, JOBVR, SENSE, N, A, LDA, W, VL, LDVL, VR, LDVR, ILO, IHI, SCALE, ABNRM, RCONDE, RCONDV, WORK, LWORK, RWORK, INFO ) 
SGGEVX( BALANC, JOBVL, JOBVR, SENSE, N, A, LDA, B, LDB, ALPHAR, ALPHAI, BETA, VL, LDVL, VR, LDVR, ILO, IHI, LSCALE, RSCALE, ABNRM, BBNRM, RCONDE, RCONDV, WORK, LWORK, IWORK, BWORK, INFO ) 
CGGEVX( BALANC, JOBVL, JOBVR, SENSE, N, A, LDA, B, LDB, ALPHAR, ALPHAI, BETA, VL, LDVL, VR, LDVR, ILO, IHI, LSCALE, RSCALE, ABNRM, BBNRM, RCONDE, RCONDV, WORK, LWORK, RWORK, IWORK, BWORK, INFO ) 
 */

extern "C" 
{
  // a-rate, k-rate, FUNC, SPECDAT
#include <csoundCore.h> 
  // PVSDAT
#include <pstream.h>
}

#include <OpcodeBase.hpp>


