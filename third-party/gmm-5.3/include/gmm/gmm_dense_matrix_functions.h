/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================

 Copyright (C) 2014-2017 Konstantinos Poulios

 This file is a part of GetFEM++

 GetFEM++  is  free software;  you  can  redistribute  it  and/or modify it
 under  the  terms  of the  GNU  Lesser General Public License as published
 by  the  Free Software Foundation;  either version 3 of the License,  or
 (at your option) any later version along with the GCC Runtime Library
 Exception either version 3.1 or (at your option) any later version.
 This program  is  distributed  in  the  hope  that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License and GCC Runtime Library Exception for more details.
 You  should  have received a copy of the GNU Lesser General Public License
 along  with  this program;  if not, write to the Free Software Foundation,
 Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

 As a special exception, you  may use  this file  as it is a part of a free
 software  library  without  restriction.  Specifically,  if   other  files
 instantiate  templates  or  use macros or inline functions from this file,
 or  you compile this  file  and  link  it  with other files  to produce an
 executable, this file  does  not  by itself cause the resulting executable
 to be covered  by the GNU Lesser General Public License.  This   exception
 does not  however  invalidate  any  other  reasons why the executable file
 might be covered by the GNU Lesser General Public License.

===========================================================================*/

/**@file gmm_dense_matrix_functions.h
   @author  Konstantinos Poulios <poulios.konstantinos@gmail.com>
   @date December 10, 2014.
   @brief Common matrix functions for dense matrices.
*/
#ifndef GMM_DENSE_MATRIX_FUNCTIONS_H
#define GMM_DENSE_MATRIX_FUNCTIONS_H


namespace gmm {


  /**
     Matrix square root for upper triangular matrices (from GNU Octave).
  */
  template <typename T>
  void sqrtm_utri_inplace(dense_matrix<T>& A)
  {
    typedef typename number_traits<T>::magnitude_type R;
    bool singular = false;

    // The following code is equivalent to this triple loop:
    //
    //   n = rows (A);
    //   for j = 1:n
    //     A(j,j) = sqrt (A(j,j));
    //     for i = j-1:-1:1
    //       A(i,j) /= (A(i,i) + A(j,j));
    //       k = 1:i-1;
    //   t storing a    A(k,j) -= A(k,i) * A(i,j);
    //     endfor
    //   endfor

    R tol = R(0); // default_tol(R()) * gmm::mat_maxnorm(A);

    const size_type n = mat_nrows(A);
    for (int j=0; j < int(n); j++) {
      typename dense_matrix<T>::iterator colj = A.begin() + j*n;
      if (gmm::abs(colj[j]) > tol)
        colj[j] = gmm::sqrt(colj[j]);
      else
        singular = true;

      for (int i=j-1; i >= 0; i--) {
        typename dense_matrix<T>::const_iterator coli = A.begin() + i*n;
        T colji = colj[i] = safe_divide(colj[i], (coli[i] + colj[j]));
        for (int k = 0; k < i; k++)
          colj[k] -= coli[k] * colji;
      }
    }

    if (singular)
      GMM_WARNING1("Matrix is singular, may not have a square root");
  }


  template <typename T>
  void sqrtm(const dense_matrix<std::complex<T> >& A,
             dense_matrix<std::complex<T> >& SQRTMA)
  {
    GMM_ASSERT1(gmm::mat_nrows(A) == gmm::mat_ncols(A),
                "Matrix square root requires a square matrix");
    gmm::resize(SQRTMA, gmm::mat_nrows(A), gmm::mat_ncols(A));
    dense_matrix<std::complex<T> > S(A), Q(A), TMP(A);
    #if defined(GMM_USES_LAPACK)
    schur(TMP, S, Q);
    #else
    GMM_ASSERT1(false, "Please recompile with lapack and blas librairies "
                "to use sqrtm matrix function.");
    #endif
    sqrtm_utri_inplace(S);
    gmm::mult(Q, S, TMP);
    gmm::mult(TMP, gmm::transposed(Q), SQRTMA);
  }

  template <typename T>
  void sqrtm(const dense_matrix<T>& A,
             dense_matrix<std::complex<T> >& SQRTMA)
  {
    dense_matrix<std::complex<T> > cA(mat_nrows(A), mat_ncols(A));
    gmm::copy(A, gmm::real_part(cA));
    sqrtm(cA, SQRTMA);
  }

  template <typename T>
  void sqrtm(const dense_matrix<T>& A, dense_matrix<T>& SQRTMA)
  {
    dense_matrix<std::complex<T> > cA(mat_nrows(A), mat_ncols(A));
    gmm::copy(A, gmm::real_part(cA));
    dense_matrix<std::complex<T> > cSQRTMA(cA);
    sqrtm(cA, cSQRTMA);
    gmm::resize(SQRTMA, gmm::mat_nrows(A), gmm::mat_ncols(A));
    gmm::copy(gmm::real_part(cSQRTMA), SQRTMA);
//    dense_matrix<std::complex<T1> >::const_reference
//      it = cSQRTMA.begin(), ite = cSQRTMA.end();
//    dense_matrix<std::complex<T1> >::reference
//      rit = SQRTMA.begin();
//    for (; it != ite; ++it, ++rit) *rit = it->real();
  }


  /**
   Matrix logarithm for upper triangular matrices (from GNU/Octave)
  */
  template <typename T>
  void logm_utri_inplace(dense_matrix<T>& S)
  {
    typedef typename number_traits<T>::magnitude_type R;

    size_type n = gmm::mat_nrows(S);
    GMM_ASSERT1(n == gmm::mat_ncols(S),
                "Matrix logarithm is not defined for non-square matrices");
    for (size_type i=0; i < n-1; ++i)
      if (gmm::abs(S(i+1,i)) > default_tol(T())) {
        GMM_ASSERT1(false, "An upper triangular matrix is expected");
        break;
      }
    for (size_type i=0; i < n-1; ++i)
      if (gmm::real(S(i,i)) <= -default_tol(R()) &&
          gmm::abs(gmm::imag(S(i,i))) <= default_tol(R())) {
        GMM_ASSERT1(false, "Principal matrix logarithm is not defined "
                           "for matrices with negative eigenvalues");
        break;
      }

    // Algorithm 11.9 in "Function of matrices", by N. Higham
    R theta[] = { R(0),R(0),R(1.61e-2),R(5.38e-2),R(1.13e-1),R(1.86e-1),R(2.6429608311114350e-1) };

    R scaling(1);
    size_type p(0), m(6), opt_iters(100);
    for (size_type k=0; k < opt_iters; ++k, scaling *= R(2)) {
      dense_matrix<T> auxS(S);
      for (size_type i = 0; i < n; ++i) auxS(i,i) -= R(1);
      R tau = gmm::mat_norm1(auxS);
      if (tau <= theta[6]) {
        ++p;
        size_type j1(6), j2(6);
        for (size_type j=0; j < 6; ++j)
          if (tau <= theta[j]) { j1 = j; break; }
        for (size_type j=0; j < j1; ++j)
          if (tau <= 2*theta[j]) { j2 = j; break; }
        if (j1 - j2 <= 1 || p == 2) { m = j1; break; }
      }
      sqrtm_utri_inplace(S);
      if (k == opt_iters-1)
        GMM_WARNING1 ("Maximum number of square roots exceeded; "
                      "the calculated matrix logarithm may still be accurate");
    }

    for (size_type i = 0; i < n; ++i) S(i,i) -= R(1);

    if (m > 0) {

      std::vector<R> nodes, wts;
      switch(m) {
      case 0: {
        R nodes_[] = { R(0.5) };
        R wts_[] = { R(1) };
        nodes.assign(nodes_, nodes_+m+1);
        wts.assign(wts_, wts_+m+1);
        } break;
      case 1: {
        R nodes_[] = { R(0.211324865405187),R(0.788675134594813) };
        R wts_[] = { R(0.5),R(0.5) };
        nodes.assign(nodes_, nodes_+m+1);
        wts.assign(wts_, wts_+m+1);
        } break;
      case 2: {
        R nodes_[] = { R(0.112701665379258),R(0.500000000000000),R(0.887298334620742) };
        R wts_[] = { R(0.277777777777778),R(0.444444444444444),R(0.277777777777778) };
        nodes.assign(nodes_, nodes_+m+1);
        wts.assign(wts_, wts_+m+1);
        } break;
      case 3: {
        R nodes_[] = { R(0.0694318442029737),R(0.3300094782075718),R(0.6699905217924281),R(0.9305681557970263) };
        R wts_[] = { R(0.173927422568727),R(0.326072577431273),R(0.326072577431273),R(0.173927422568727) };
        nodes.assign(nodes_, nodes_+m+1);
        wts.assign(wts_, wts_+m+1);
        } break;
      case 4: {
        R nodes_[] = { R(0.0469100770306681),R(0.2307653449471584),R(0.5000000000000000),R(0.7692346550528415),R(0.9530899229693319) };
        R wts_[] = { R(0.118463442528095),R(0.239314335249683),R(0.284444444444444),R(0.239314335249683),R(0.118463442528094) };
        nodes.assign(nodes_, nodes_+m+1);
        wts.assign(wts_, wts_+m+1);
        } break;
      case 5: {
        R nodes_[] = { R(0.0337652428984240),R(0.1693953067668678),R(0.3806904069584015),R(0.6193095930415985),R(0.8306046932331322),R(0.9662347571015761) };
        R wts_[] = { R(0.0856622461895853),R(0.1803807865240693),R(0.2339569672863452),R(0.2339569672863459),R(0.1803807865240693),R(0.0856622461895852) };
        nodes.assign(nodes_, nodes_+m+1);
        wts.assign(wts_, wts_+m+1);
        } break;
      case 6: {
        R nodes_[] = { R(0.0254460438286208),R(0.1292344072003028),R(0.2970774243113015),R(0.4999999999999999),R(0.7029225756886985),R(0.8707655927996973),R(0.9745539561713792) };
        R wts_[] = { R(0.0647424830844348),R(0.1398526957446384),R(0.1909150252525594),R(0.2089795918367343),R(0.1909150252525595),R(0.1398526957446383),R(0.0647424830844349) };
        nodes.assign(nodes_, nodes_+m+1);
        wts.assign(wts_, wts_+m+1);
        } break;
      }

      dense_matrix<T> auxS1(S), auxS2(S);
      std::vector<T> auxvec(n);
      gmm::clear(S);
      for (size_type j=0; j <= m; ++j) {
        gmm::copy(gmm::scaled(auxS1, nodes[j]), auxS2);
        gmm::add(gmm::identity_matrix(), auxS2);
        // S += wts[i] * auxS1 * inv(auxS2)
        for (size_type i=0; i < n; ++i) {
          gmm::copy(gmm::mat_row(auxS1, i), auxvec);
          gmm::lower_tri_solve(gmm::transposed(auxS2), auxvec, false);
          gmm::add(gmm::scaled(auxvec, wts[j]), gmm::mat_row(S, i));
        }
      }
    }
    gmm::scale(S, scaling);
  }

  /**
   Matrix logarithm (from GNU/Octave)
  */
  template <typename T>
  void logm(const dense_matrix<T>& A, dense_matrix<T>& LOGMA)
  {
    typedef typename number_traits<T>::magnitude_type R;
    size_type n = gmm::mat_nrows(A);
    GMM_ASSERT1(n == gmm::mat_ncols(A),
                "Matrix logarithm is not defined for non-square matrices");
    dense_matrix<T> S(A), Q(A);
    #if defined(GMM_USES_LAPACK)
    schur(A, S, Q); // A = Q * S * Q^T
    #else
    GMM_ASSERT1(false, "Please recompile with lapack and blas librairies "
                "to use logm matrix function.");
    #endif

    bool convert_to_complex(false);
    if (!is_complex(T()))
      for (size_type i=0; i < n-1; ++i)
        if (gmm::abs(S(i+1,i)) > default_tol(T())) {
          convert_to_complex = true;
          break;
        }

    gmm::resize(LOGMA, n, n);
    if (convert_to_complex) {
      dense_matrix<std::complex<R> > cS(n,n), cQ(n,n), auxmat(n,n);
      gmm::copy(gmm::real_part(S), gmm::real_part(cS));
      gmm::copy(gmm::real_part(Q), gmm::real_part(cQ));
      block2x2_reduction(cS, cQ, default_tol(R())*R(3));
      for (size_type j=0; j < n-1; ++j)
        for (size_type i=j+1; i < n; ++i)
          cS(i,j) = T(0);
      logm_utri_inplace(cS);
      gmm::mult(cQ, cS, auxmat);
      gmm::mult(auxmat, gmm::transposed(cQ), cS);
      // Remove small complex values which may have entered calculation
      gmm::copy(gmm::real_part(cS), LOGMA);
//      GMM_ASSERT1(gmm::mat_norm1(gmm::imag_part(cS)) < n*default_tol(T()),
//                  "Internal error, imag part should be zero");
    } else {
      dense_matrix<T> auxmat(n,n);
      logm_utri_inplace(S);
      gmm::mult(Q, S, auxmat);
      gmm::mult(auxmat, gmm::transposed(Q), LOGMA);
    }

  }

}

#endif

