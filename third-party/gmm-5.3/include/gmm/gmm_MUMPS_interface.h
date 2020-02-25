/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================

 Copyright (C) 2003-2017 Yves Renard, Julien Pommier

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

/**@file gmm_MUMPS_interface.h
   @author Yves Renard <Yves.Renard@insa-lyon.fr>,
   @author Julien Pommier <Julien.Pommier@insa-toulouse.fr>
   @date December 8, 2005.
   @brief Interface with MUMPS (LU direct solver for sparse matrices).
*/
#if defined(GMM_USES_MUMPS) || defined(HAVE_DMUMPS_C_H)

#ifndef GMM_MUMPS_INTERFACE_H
#define GMM_MUMPS_INTERFACE_H

#include "gmm_kernel.h"


extern "C" {

#include <smumps_c.h>
#undef F_INT
#undef F_DOUBLE
#undef F_DOUBLE2
#include <dmumps_c.h>
#undef F_INT
#undef F_DOUBLE
#undef F_DOUBLE2
#include <cmumps_c.h>
#undef F_INT
#undef F_DOUBLE
#undef F_DOUBLE2
#include <zmumps_c.h>
#undef F_INT
#undef F_DOUBLE
#undef F_DOUBLE2

}

namespace gmm {

#define ICNTL(I) icntl[(I)-1]
#define INFO(I) info[(I)-1]
#define INFOG(I) infog[(I)-1]
#define RINFOG(I) rinfog[(I)-1]

  template <typename T> struct ij_sparse_matrix {
    std::vector<int> irn;
    std::vector<int> jcn;
    std::vector<T> a;
    bool sym;
    
    template <typename L> void store(const L& l, size_type i) {
       typename linalg_traits<L>::const_iterator it = vect_const_begin(l),
         ite = vect_const_end(l);
       for (; it != ite; ++it) {
         int ir = (int)i + 1, jc = (int)it.index() + 1;
         if (*it != T(0) && (!sym || ir >= jc)) 
         { irn.push_back(ir); jcn.push_back(jc); a.push_back(*it); }
       }
    }

    template <typename L> void build_from(const L& l, row_major) {
      for (size_type i = 0; i < mat_nrows(l); ++i)
        store(mat_const_row(l, i), i);
    }

    template <typename L> void build_from(const L& l, col_major) {
      for (size_type i = 0; i < mat_ncols(l); ++i)
        store(mat_const_col(l, i), i);
      irn.swap(jcn);
    }

    template <typename L> ij_sparse_matrix(const L& A, bool sym_) {
      size_type nz = nnz(A);
      sym = sym_;
      irn.reserve(nz); jcn.reserve(nz); a.reserve(nz);
      build_from(A,  typename principal_orientation_type<typename
                 linalg_traits<L>::sub_orientation>::potype());
    }
  };

  /* ********************************************************************* */
  /*   MUMPS solve interface                                               */
  /* ********************************************************************* */

  template <typename T> struct mumps_interf {};

  template <> struct mumps_interf<float> {
    typedef SMUMPS_STRUC_C  MUMPS_STRUC_C;
    typedef float value_type;

    static void mumps_c(MUMPS_STRUC_C &id) { smumps_c(&id); }
  };

  template <> struct mumps_interf<double> {
    typedef DMUMPS_STRUC_C  MUMPS_STRUC_C;
    typedef double value_type;
    static void mumps_c(MUMPS_STRUC_C &id) { dmumps_c(&id); }
  };

  template <> struct mumps_interf<std::complex<float> > {
    typedef CMUMPS_STRUC_C  MUMPS_STRUC_C;
    typedef mumps_complex value_type;
    static void mumps_c(MUMPS_STRUC_C &id) { cmumps_c(&id); }
  };

  template <> struct mumps_interf<std::complex<double> > {
    typedef ZMUMPS_STRUC_C  MUMPS_STRUC_C;
    typedef mumps_double_complex value_type;
    static void mumps_c(MUMPS_STRUC_C &id) { zmumps_c(&id); }
  };


  template <typename MUMPS_STRUCT>
  static inline bool mumps_error_check(MUMPS_STRUCT &id) {
    if (id.INFO(1) < 0) {
      switch (id.INFO(1)) {
        case -2:
          GMM_ASSERT1(false, "Solve with MUMPS failed: NZ = " << id.INFO(2)
                      << " is out of range");
        case -6 : case -10 :
          GMM_WARNING1("Solve with MUMPS failed: matrix is singular");
          return false;
        case -9:
          GMM_ASSERT1(false, "Solve with MUMPS failed: error "
                      << id.INFO(1) << ", increase ICNTL(14)");
        case -13 :
          GMM_ASSERT1(false, "Solve with MUMPS failed: not enough memory");
        default :
          GMM_ASSERT1(false, "Solve with MUMPS failed with error "
                      << id.INFO(1));
      }
    }
    return true;
  }


  /** MUMPS solve interface  
   *  Works only with sparse or skyline matrices
   */
  template <typename MAT, typename VECTX, typename VECTB>
  bool MUMPS_solve(const MAT &A, const VECTX &X_, const VECTB &B,
                   bool sym = false, bool distributed = false) {
    VECTX &X = const_cast<VECTX &>(X_);

    typedef typename linalg_traits<MAT>::value_type T;
    typedef typename mumps_interf<T>::value_type MUMPS_T;
    GMM_ASSERT2(gmm::mat_nrows(A) == gmm::mat_ncols(A), "Non-square matrix");
  
    std::vector<T> rhs(gmm::vect_size(B)); gmm::copy(B, rhs);

    ij_sparse_matrix<T> AA(A, sym);
  
    const int JOB_INIT = -1;
    const int JOB_END = -2;
    const int USE_COMM_WORLD = -987654;

    typename mumps_interf<T>::MUMPS_STRUC_C id;

    int rank(0);
#ifdef GMM_USES_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
    
    id.job = JOB_INIT;
    id.par = 1;
    id.sym = sym ? 2 : 0;
    id.comm_fortran = USE_COMM_WORLD;
    mumps_interf<T>::mumps_c(id);
    
    if (rank == 0 || distributed) {
      id.n = int(gmm::mat_nrows(A));
      if (distributed) {
        id.nz_loc = int(AA.irn.size());
        id.irn_loc = &(AA.irn[0]);
        id.jcn_loc = &(AA.jcn[0]);
        id.a_loc = (MUMPS_T*)(&(AA.a[0]));
      } else {
        id.nz = int(AA.irn.size());
        id.irn = &(AA.irn[0]);
        id.jcn = &(AA.jcn[0]);
        id.a = (MUMPS_T*)(&(AA.a[0]));
      }
      if (rank == 0)
        id.rhs = (MUMPS_T*)(&(rhs[0]));
    }

    id.ICNTL(1) = -1; // output stream for error messages
    id.ICNTL(2) = -1; // output stream for other messages
    id.ICNTL(3) = -1; // output stream for global information
    id.ICNTL(4) = 0;  // verbosity level

    if (distributed)
      id.ICNTL(5) = 0;  // assembled input matrix (default)

    id.ICNTL(14) += 80; /* small boost to the workspace size as we have encountered some problem
                           who did not fit in the default settings of mumps.. 
                           by default, ICNTL(14) = 15 or 20
                        */
    //cout << "ICNTL(14): " << id.ICNTL(14) << "\n";

    if (distributed)
      id.ICNTL(18) = 3; // strategy for distributed input matrix

    // id.ICNTL(22) = 1;   /* enables out-of-core support */

    id.job = 6;
    mumps_interf<T>::mumps_c(id);
    bool ok = mumps_error_check(id);

    id.job = JOB_END;
    mumps_interf<T>::mumps_c(id);

#ifdef GMM_USES_MPI
    MPI_Bcast(&(rhs[0]),id.n,gmm::mpi_type(T()),0,MPI_COMM_WORLD);
#endif

    gmm::copy(rhs, X);

    return ok;

  }



  /** MUMPS solve interface for distributed matrices 
   *  Works only with sparse or skyline matrices
   */
  template <typename MAT, typename VECTX, typename VECTB>
  bool MUMPS_distributed_matrix_solve(const MAT &A, const VECTX &X_,
                                      const VECTB &B, bool sym = false) {
    return MUMPS_solve(A, X_, B, sym, true);
  }



  template<typename T>
  inline T real_or_complex(std::complex<T> a) { return a.real(); }
  template<typename T>
  inline T real_or_complex(T &a) { return a; }


  /** Evaluate matrix determinant with MUMPS  
   *  Works only with sparse or skyline matrices
   */
  template <typename MAT, typename T = typename linalg_traits<MAT>::value_type>
  T MUMPS_determinant(const MAT &A, int &exponent,
                      bool sym = false, bool distributed = false) {
    exponent = 0;
    typedef typename mumps_interf<T>::value_type MUMPS_T;
    typedef typename number_traits<T>::magnitude_type R;
    GMM_ASSERT2(gmm::mat_nrows(A) == gmm::mat_ncols(A), "Non-square matrix");
  
    ij_sparse_matrix<T> AA(A, sym);
  
    const int JOB_INIT = -1;
    const int JOB_END = -2;
    const int USE_COMM_WORLD = -987654;

    typename mumps_interf<T>::MUMPS_STRUC_C id;

    int rank(0);
#ifdef GMM_USES_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
    
    id.job = JOB_INIT;
    id.par = 1;
    id.sym = sym ? 2 : 0;
    id.comm_fortran = USE_COMM_WORLD;
    mumps_interf<T>::mumps_c(id);
    
    if (rank == 0 || distributed) {
      id.n = int(gmm::mat_nrows(A));
      if (distributed) {
        id.nz_loc = int(AA.irn.size());
        id.irn_loc = &(AA.irn[0]);
        id.jcn_loc = &(AA.jcn[0]);
        id.a_loc = (MUMPS_T*)(&(AA.a[0]));
      } else {
        id.nz = int(AA.irn.size());
        id.irn = &(AA.irn[0]);
        id.jcn = &(AA.jcn[0]);
        id.a = (MUMPS_T*)(&(AA.a[0]));
      }
    }

    id.ICNTL(1) = -1; // output stream for error messages
    id.ICNTL(2) = -1; // output stream for other messages
    id.ICNTL(3) = -1; // output stream for global information
    id.ICNTL(4) = 0;  // verbosity level

    if (distributed)
      id.ICNTL(5) = 0;  // assembled input matrix (default)

//    id.ICNTL(14) += 80; // small boost to the workspace size 

    if (distributed)
      id.ICNTL(18) = 3; // strategy for distributed input matrix

    id.ICNTL(31) = 1;   // only factorization, no solution to follow
    id.ICNTL(33) = 1;   // request determinant calculation

    id.job = 4; // abalysis (job=1) + factorization (job=2)
    mumps_interf<T>::mumps_c(id);
    mumps_error_check(id);

    T det = real_or_complex(std::complex<R>(id.RINFOG(12),id.RINFOG(13)));
    exponent = id.INFOG(34);

    id.job = JOB_END;
    mumps_interf<T>::mumps_c(id);

    return det;
  }

#undef ICNTL
#undef INFO
#undef INFOG
#undef RINFOG

}

  
#endif // GMM_MUMPS_INTERFACE_H

#endif // GMM_USES_MUMPS
