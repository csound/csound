/**
 * L I N E A R   A L G E B R A   O P C O D E S
 * Michael Gogins
 *
 * These opcodes implement many BLAS and LAPACK 
 * linear algebra operations, up to eigenvalue decomposition.
 * They are designed to facilitate MATLAB style signal processing
 * in the Csound orchestra language.
 *
 * Note the following limitations and caveats:
 * 1. The rate (irate versus krate or arate) of the result (return value)
 *    determines the rate of the operation. The exact same function 
 *    is called for irate, krate, and arate; the only difference is that
 *    a function with an irate result is evaluated only once per init,
 *    while the same function with a krate or arate result is evaluated 
 *    for each kperiod.
 * 2. Operations that return scalars return irate or krate values
 *    on the left-hand side of the opcode.
 * 3. Operations that return vectors or matrices copy the return value 
 *    into a preallocated argument on the right-hand side of the opcode.
 * 4. For double-precision Csound, all array elements are doubles;
 *    for single-precision Csound, all array elements are floats.
 * 5. All arrays are row-major.
 * 6. All arrays are 0-based.
 * 7. All arrays are dense.
 *
 * The opcodes work with any combination of the the following data types
 * (S scalar, C complex, V real or complex vector, M real or complex matrix):
 * 1. Csound irate and krate variables, as real scalars [S], 
 *    complex scalars as pairs of S.
 * 2. Csound arate signals and variables, as real vectors of size ksmps [V]
 * 3. Csound function tables (FUNC), as real vectors [V].
 * 4. Csound fsigs (PVS signals, PVSDAT), as complex vectors [V].
 * 5. Csound wsigs (spectral signals, SPECDAT), as complex vectors [V].
 * 6. Allocated complex scalars [C].
 * 7. Allocated vectors, real and complex, considered to be 
 *    column vectors [V].
 * 8. Allocated matrices, real and complex [M], considered to be
 *    row-major.
 *
 * The operations are:
 * Level 1 BLAS
void cblas ♦rotg (TYPE *a, TYPE *b, TYPE *c, TYPE *s) Generate plane rotation S, D
void cblas ♦rotg (TYPE *a, TYPE *b, TYPE *c, TYPE *s) Generate plane rotation C, Z
void cblas ♦rotmg (TYPE *d1, TYPE *d2, TYPE *b1, SCALAR b2, TYPE *P) Generate modified plane rotation S,D
void cblas ♦rot (const int N, TYPE *X, const int incX, TYPE *Y, const int incY, SCALAR c, SCALAR s) Apply plane rotation S,D
void cblas ♦rot (const int N, TYPE *X, const int incX, TYPE *Y, const int incY, const UTYPE c, const UTYPE s) Apply plane rotation CS,ZD
void cblas ♦rotm (const int N, TYPE *X, const int incX, TYPE *Y, const int incY, SCALAR c, TYPE *P) Apply modified plane rotation S,D
void cblas ♦scal (const int N, SCALAR alpha, TYPE *X, const int incX) x ↔ y S,D,C,Z,CS,ZD
void cblas ♦copy (const int N, const TYPE *X, const int incX, TYPE *Y, const int incY) y ← x S,D,C,Z
void cblas ♦axpy (const int N, SCALAR alpha, const TYPE *X, const int incX, TYPE *Y, const int incY) y ← x + y S,D,C,Z
TYPE cblas ♦dot (const int N, const TYPE *X, const int incX, const TYPE *Y, const int incY) cblas dot ← xT y S,D,DS
void
cblas ♦dotu sub
(const int N, const TYPE *X, const int incX, const TYPE *Y, const int incY, TYPE *dotu) dotu ← xT y C,Z
void
cblas ♦dotc sub
(const int N, const TYPE *X, const int incX, const TYPE *Y, const int incY, TYPE *dotc) dotc ← xHy C,Z
float cblas sdsdot (const int N, const float alpha, const float *X, const int incX, const float *Y, const int incY) dot ←  + xT y SDS
UTYPE
cblas ♦nrm2
(const int N, const TYPE *X, const int incX) cblas nrm2 ← ||x||2 S,D,SC,DZ
UTYPE
cblas ♦asum
(const int N, const TYPE *X, const int incX) cblas asum ← ||re(x)||1 + ||im(x)||1 S,D,SC,DZ
CBLAS INDEX
cblas i♦amax
(const int N, const TYPE *X, const int incX) amax ← 1stk ∋ |re(xk)| + |im(xk)| S,D,C,Z
 * Level 2 BLAS
void cblas ♦gemv ( const enum CBLAS ORDER Order, const enum CBLAS TRANSPOSE TransA, const int M, const int N, SCALAR
alpha, const TYPE *A, const int lda, const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y, y ← AT x + y,
y ← AHx + y, A − m × n
S,D,C,Z
void cblas ♦gbmv ( const enum CBLAS ORDER Order, const enum CBLAS TRANSPOSE TransA, const int M, const int N, const int
KL, const int KU, SCALAR alpha, const TYPE *A, const int lda, const TYPE *X, const int incX, SCALAR beta,
TYPE *Y, const int incY )
y ← Ax + y, y ← AT x + y,
y ← AHx + y, A − m × n
S,D,C,Z
void cblas ♦hemv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *A,
const int lda, const void *X, const int incX, const void *beta, void *Y, const int incY )
y ← Ax + y C,Z
void cblas ♦hbmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const int K, const void *alpha,
const void *A, const int lda, const void *X, const int incX, const void *beta, void *Y, const int incY )
y ← Ax + y C,Z
void cblas ♦hpmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *Ap,
const void *X, const int incX, const void *beta, void *Y, const int incY )
y ← Ax + y C,Z
void cblas ♦symv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *A,
const int lda, const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y S,D
void cblas ♦sbmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const int K, SCALAR alpha, const
TYPE *A, const int lda, const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y S,D
void cblas ♦spmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *Ap,
const TYPE *X, const int incX, SCALAR beta, TYPE *Y, const int incY )
y ← Ax + y S,D
void cblas ♦trmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← Ax, x ← AT x, x ← AHx S,D,C,Z
void cblas ♦tbmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const int K, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← Ax, x ← AT x, x ← AHx S,D,C,Z
void cblas ♦tpmv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *Ap, TYPE *X, const int incX )
x ← Ax, x ← AT x, x ← AHx S,D,C,Z
void cblas ♦trsv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← A−1x, x ← A−T x, x ← A−Hx S,D,C,Z
void cblas ♦tbsv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const int K, const TYPE *A, const int lda, TYPE *X, const int incX )
x ← A−1x, x ← A−T x, x ← A−Hx S,D,C,Z
void cblas ♦tpsv ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE TransA,
const enum CBLAS DIAG Diag, const int N, const TYPE *Ap, TYPE *X, const int incX )
x ← A−1x, x ← A−T x, x ← A−Hx S,D,C,Z
void cblas ♦ger ( const enum CBLAS ORDER Order, const int M, const int N, SCALAR alpha, const TYPE *X, const int incX, const
TYPE *Y, const int incY, TYPE *A, const int lda )
A ← xyT + A, A − m × n S,D
void cblas ♦geru ( const enum CBLAS ORDER Order, const int M, const int N, const void *alpha, const void *X, const int incX, const
void *Y, const int incY, void *A, const int lda )
A ← xyT + A, A − m × n C,Z
void cblas ♦gerc ( const enum CBLAS ORDER Order, const int M, const int N, const void *alpha, const void *X, const int incX, const
void *Y, const int incY, void *A, const int lda )
A ← xyH + A, A − m × n C,Z
void cblas ♦her ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const UTYPE alpha, const void
*X, const int incX, void *A, const int lda )
A ← xxH + A C,Z
void cblas ♦hpr ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const UTYPE alpha, const void
*X, const int incX, void *A )
A ← xxH + A C,Z
void cblas ♦her2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *X,
const int incX, const void *Y, const int incY, void *A, const int lda )
A ← xyH + y(x)H + A C,Z
void cblas ♦hpr2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, const void *alpha, const void *X,
const int incX, const void *Y, const int incY, void *Ap )
A ← xyH + y(x)H + A C,Z
void cblas ♦syr ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, TYPE *A, const int lda )
A ← xxT + A S,D
void cblas ♦spr ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, TYPE *Ap )
A ← xxT + A S,D
void cblas ♦syr2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, const TYPE *Y, const int incY, TYPE *A, const int lda )
A ← xyT + yxT + A S,D
void cblas ♦spr2 ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const int N, SCALAR alpha, const TYPE *X,
const int incX, const TYPE *Y, const int incY, TYPE *A )
A ← xyT + yxT + A S,
 * Level 3 BLAS
void cblas ♦gemm ( const enum CBLAS ORDER Order, const enum CBLAS TRANSPOSE TransA, const enum CBLAS TRANSPOSE
TransB, const int M, const int N, const int K, const SCALAR alpha, const TYPE *A, const int lda, const TYPE *B,
const int ldb, const SCALAR beta, TYPE *C, const int ldc )
C ← op(A)op(B) + C,
op(X) = X,XT ,XH, C − m × n
S,D,C,Z
void cblas ♦symm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const int M,
const int N, SCALAR alpha, const TYPE *A, const int lda, const TYPE *B, const int ldb, SCALAR beta, TYPE *C,
const int ldc )
C ← AB + C, C ← BA + C, C −
m × n, A = AT
S,D,C,Z
void cblas ♦hemm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const int M,
const int N, const void *alpha, const void *A, const int lda, const void *B, const int ldb, const void *beta, void *C,
const int ldc )
C ← AB + C, C ← BA + C, C −
m × n,A = AH
C,Z
void cblas ♦syrk ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, SCALAR alpha, const TYPE *A, const int lda, SCALAR beta, TYPE *C, const int ldc )
C ← AAT + C, C ← ATA + C,
C − n × n
S,D,C,Z
void cblas ♦herk ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, const UTYPE alpha, const void *A, const int lda, const UTYPE beta, void *C, const int ldc )
C ← AAH + C, C ← AHA + C,
C − n × n
C,Z
void cblas ♦syr2k ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, SCALAR alpha, const TYPE *A, const int lda, const TYPE *B, const int ldb, SCALAR beta, TYPE
*C, const int ldc )
C ← ABT + ¯BAT +C, C ← ATB+
¯BTA + C, C − n × n
S,D,C,Z
void cblas ♦her2k ( const enum CBLAS ORDER Order, const enum CBLAS UPLO Uplo, const enum CBLAS TRANSPOSE Trans, const
int N, const int K, const void *alpha, const void *A, const int lda, const void *B, const int ldb, const UTYPE beta,
void *C, const int ldc )
C ← ABH+¯BAH+C, C ← AHB+
¯BHA + C, C − n × n
C,Z
void cblas ♦trmm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const enum
CBLAS TRANSPOSE TransA, const enum CBLAS DIAG Diag, const int M, const int N, SCALAR alpha, const TYPE
*A, const int lda, TYPE *B, const int ldb )
B ← op(A)B, B ← Bop(A), op(A) =
A,AT ,AH, B − m × n
S,D,C,Z
void cblas ♦trsm ( const enum CBLAS ORDER Order, const enum CBLAS SIDE Side, const enum CBLAS UPLO Uplo, const enum
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
  // arate, krate, FUNC, SPECDAT
#include <csoundCore.h> 
  // PVSDAT
#include <pstream.h>
}

#include <OpcodeBase.hpp>


