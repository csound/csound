/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================

 Copyright (C) 2002-2017 Yves Renard

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

/**@file gmm_std.h
@author  Yves Renard <Yves.Renard@insa-lyon.fr>,
@author  Julien Pommier <Julien.Pommier@insa-toulouse.fr>
@date June 01, 1995.
@brief basic setup for gmm (includes, typedefs etc.)
*/
#ifndef GMM_STD_H__
#define GMM_STD_H__

#ifndef __USE_STD_IOSTREAM
# define __USE_STD_IOSTREAM
#endif

#ifndef __USE_BSD
# define __USE_BSD
#endif

#ifndef __USE_ISOC99
# define __USE_ISOC99
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1400 // Secure versions for VC++
# define GMM_SECURE_CRT
# define SECURE_NONCHAR_SSCANF sscanf_s
# define SECURE_NONCHAR_FSCANF fscanf_s
# define SECURE_STRNCPY(a, la, b, lb) strncpy_s(a, la, b, lb)
# define SECURE_FOPEN(F, filename, mode) (*(F) = 0,  fopen_s(F, filename, mode))
# define SECURE_SPRINTF1(S, l, st, p1) sprintf_s(S, l, st, p1) 
# define SECURE_SPRINTF2(S, l, st, p1, p2) sprintf_s(S, l, st, p1, p2) 
# define SECURE_SPRINTF4(S, l, st, p1, p2, p3, p4) sprintf_s(S, l, st, p1, p2, p3, p4)
# define SECURE_STRDUP(s) _strdup(s)
# ifndef _SCL_SECURE_NO_DEPRECATE
#   error Add the option /D_SCL_SECURE_NO_DEPRECATE to the compilation command
# endif
#else
# define SECURE_NONCHAR_SSCANF sscanf
# define SECURE_NONCHAR_FSCANF fscanf
# define SECURE_STRNCPY(a, la, b, lb) strncpy(a, b, lb)
# define SECURE_FOPEN(F, filename, mode) ((*(F)) = fopen(filename, mode))
# define SECURE_SPRINTF1(S, l, st, p1) sprintf(S, st, p1)
# define SECURE_SPRINTF2(S, l, st, p1, p2) sprintf(S, st, p1, p2)
# define SECURE_SPRINTF4(S, l, st, p1, p2, p3, p4) sprintf(S, st, p1, p2, p3, p4) 
# define SECURE_STRDUP(s) strdup(s)
#endif

inline void GMM_NOPERATION_(int) { }
#define GMM_NOPERATION(a) { GMM_NOPERATION_(abs(&(a) != &(a))); }

/* ********************************************************************** */
/*	Compilers detection.						  */
/* ********************************************************************** */

/* for VISUAL C++ ...
#if defined(_MSC_VER) //  && !defined(__MWERKS__)
#define _GETFEM_MSVCPP_ _MSC_VER
#endif
*/

#if defined(__GNUC__)
#  if (__GNUC__ < 4)
#    error : PLEASE UPDATE g++ TO AT LEAST 4.8 VERSION
#  endif
#endif

/* ********************************************************************** */
/*	C++ Standard Headers.						  */
/* ********************************************************************** */
#include <clocale>
#include <cstdlib>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <cctype>
#include <cassert>
#include <climits>
#include <iostream>
//#include <ios>
#include <fstream>
#include <ctime>
#include <exception>
#include <typeinfo>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <vector>
#include <deque>
#include <string>
#include <complex>
#include <limits>
#include <sstream>
#include <numeric>
#include <memory>
#include <array>
#include <locale.h>

#include <gmm/gmm_arch_config.h>

namespace std {
#if defined(__GNUC__) && (__cplusplus <= 201103L)
  template<typename _Tp>
    struct _MakeUniq
    { typedef unique_ptr<_Tp> __single_object; };
  template<typename _Tp>
    struct _MakeUniq<_Tp[]>
    { typedef unique_ptr<_Tp[]> __array; };
  template<typename _Tp, size_t _Bound>
    struct _MakeUniq<_Tp[_Bound]>
    { struct __invalid_type { }; };
  /// std::make_unique for single objects
  template<typename _Tp, typename... _Args>
    inline typename _MakeUniq<_Tp>::__single_object
    make_unique(_Args&&... __args)
    { return unique_ptr<_Tp>(new _Tp(std::forward<_Args>(__args)...)); }
  /// std::make_unique for arrays of unknown bound
  template<typename _Tp>
    inline typename _MakeUniq<_Tp>::__array
    make_unique(size_t __num)
    { return unique_ptr<_Tp>(new typename remove_extent<_Tp>::type[__num]()); }
  /// Disable std::make_unique for arrays of known bound
  template<typename _Tp, typename... _Args>
    inline typename _MakeUniq<_Tp>::__invalid_type
    make_unique(_Args&&...) = delete;
#endif


  // Should simply be replaced by std::shared_ptr<T[]> when it will be supported
  // by the STL
  template <typename T> class shared_array_ptr : shared_ptr<T> {
  public:
    shared_array_ptr() {}
    shared_array_ptr(T *q) : std::shared_ptr<T>(q, default_delete<T[]>()) {}
    template <typename Y> shared_array_ptr(const std::shared_ptr<Y> &p, T *q)
      : std::shared_ptr<T>(p, q) {}
    T *get() const { return shared_ptr<T>::get(); }
    T& operator*() const { return shared_ptr<T>::operator*(); }
    T* operator->() const { return shared_ptr<T>::operator->(); }
  };
  
  template <typename T> shared_array_ptr<T> make_shared_array(size_t num)
  { return shared_array_ptr<T>(new T[num]); }
}




#ifdef GMM_HAVE_OPENMP

#include <omp.h>
	/**number of OpenMP threads*/
	inline size_t num_threads(){return omp_get_max_threads();}
	/**index of the current thread*/
	inline size_t this_thread() {return omp_get_thread_num();}
	/**is the program running in the parallel section*/
	inline bool me_is_multithreaded_now(){return static_cast<bool>(omp_in_parallel());}
#else
	inline size_t num_threads(){return size_t(1);}
	inline size_t this_thread() {return size_t(0);}
	inline bool me_is_multithreaded_now(){return false;}
#endif

namespace gmm {

	using std::endl; using std::cout; using std::cerr;
        using std::ends; using std::cin; using std::isnan;

#ifdef _WIN32

	class standard_locale {
		std::string cloc;
		std::locale cinloc;
	public :
		inline standard_locale(void) : cinloc(cin.getloc())
		{
			if (!me_is_multithreaded_now()){
				 cloc=setlocale(LC_NUMERIC, 0);
				 setlocale(LC_NUMERIC,"C");
			}
		}

		inline ~standard_locale() {
			if (!me_is_multithreaded_now())
					setlocale(LC_NUMERIC, cloc.c_str());

		}
	};
#else
	/**this is the above solutions for linux, but it still needs to be tested.*/
	//class standard_locale {
	//	locale_t oldloc;
	//	locale_t temploc;

	//public :
	//	inline standard_locale(void) : oldloc(uselocale((locale_t)0))
	//	{
	//			temploc = newlocale(LC_NUMERIC, "C", NULL);
    //              uselocale(temploc);
	//	}

	//	inline ~standard_locale()
	//	{
	//		    uselocale(oldloc);
	//			freelocale(temploc);
	//	}
	//};


  class standard_locale {
    std::string cloc;
    std::locale cinloc;

  public :
    inline standard_locale(void)
      : cloc(setlocale(LC_NUMERIC, 0)), cinloc(cin.getloc())
    { setlocale(LC_NUMERIC,"C"); cin.imbue(std::locale("C")); }
    inline ~standard_locale()
    { setlocale(LC_NUMERIC, cloc.c_str()); cin.imbue(cinloc); }
  };


#endif

  class stream_standard_locale {
    std::locale cloc;
    std::ios &io;

  public :
    inline stream_standard_locale(std::ios &i)
      : cloc(i.getloc()), io(i) { io.imbue(std::locale("C")); }
    inline ~stream_standard_locale() { io.imbue(cloc); }
  };




  /* ******************************************************************* */
  /*       Clock functions.                                              */
  /* ******************************************************************* */

# if  defined(HAVE_SYS_TIMES)
  inline double uclock_sec(void) {
    static double ttclk = 0.;
    if (ttclk == 0.) ttclk = sysconf(_SC_CLK_TCK);
    tms t; times(&t); return double(t.tms_utime) / ttclk;
  }
# else
  inline double uclock_sec(void)
  { return double(clock())/double(CLOCKS_PER_SEC); }
# endif

  /* ******************************************************************** */
  /*	Fixed size integer types.                     			  */
  /* ******************************************************************** */
  // Remark : the test program dynamic_array tests the length of
  //          resulting integers

  template <size_t s> struct fixed_size_integer_generator {
    typedef void int_base_type;
    typedef void uint_base_type;
  };

  template <> struct fixed_size_integer_generator<sizeof(char)> {
    typedef signed char int_base_type;
    typedef unsigned char uint_base_type;
  };

  template <> struct fixed_size_integer_generator<sizeof(short int)
    - ((sizeof(short int) == sizeof(char)) ? 78 : 0)> {
  typedef signed short int int_base_type;
  typedef unsigned short int uint_base_type;
};

template <> struct fixed_size_integer_generator<sizeof(int)
  - ((sizeof(int) == sizeof(short int)) ? 59 : 0)> {
    typedef signed int int_base_type;
    typedef unsigned int uint_base_type;
  };

template <> struct fixed_size_integer_generator<sizeof(long)
  - ((sizeof(int) == sizeof(long)) ? 93 : 0)> {
    typedef signed long int_base_type;
    typedef unsigned long uint_base_type;
  };

template <> struct fixed_size_integer_generator<sizeof(long long)
  - ((sizeof(long long) == sizeof(long)) ? 99 : 0)> {
    typedef signed long long int_base_type;
    typedef unsigned long long uint_base_type;
  };

typedef fixed_size_integer_generator<1>::int_base_type int8_type;
typedef fixed_size_integer_generator<1>::uint_base_type uint8_type;
typedef fixed_size_integer_generator<2>::int_base_type int16_type;
typedef fixed_size_integer_generator<2>::uint_base_type uint16_type;
typedef fixed_size_integer_generator<4>::int_base_type int32_type;
typedef fixed_size_integer_generator<4>::uint_base_type uint32_type;
typedef fixed_size_integer_generator<8>::int_base_type int64_type;
typedef fixed_size_integer_generator<8>::uint_base_type uint64_type;

// #if INT_MAX == 32767
//   typedef signed int    int16_type;
//   typedef unsigned int uint16_type;
// #elif  SHRT_MAX == 32767
//   typedef signed short int    int16_type;
//   typedef unsigned short int uint16_type;
// #else
// # error "impossible to build a 16 bits integer"
// #endif

// #if INT_MAX == 2147483647
//   typedef signed int    int32_type;
//   typedef unsigned int uint32_type;
// #elif  SHRT_MAX == 2147483647
//   typedef signed short int    int32_type;
//   typedef unsigned short int uint32_type;
// #elif LONG_MAX == 2147483647
//   typedef signed long int    int32_type;
//   typedef unsigned long int uint32_type;
// #else
// # error "impossible to build a 32 bits integer"
// #endif

// #if INT_MAX == 9223372036854775807L || INT_MAX == 9223372036854775807
//   typedef signed int    int64_type;
//   typedef unsigned int uint64_type;
// #elif LONG_MAX == 9223372036854775807L || LONG_MAX == 9223372036854775807
//   typedef signed long int    int64_type;
//   typedef unsigned long int uint64_type;
// #elif LLONG_MAX == 9223372036854775807LL || LLONG_MAX == 9223372036854775807L || LLONG_MAX == 9223372036854775807
//   typedef signed long long int int64_type;
//   typedef unsigned long long int uint64_type;
// #else
// # error "impossible to build a 64 bits integer"
// #endif

#if defined(__GNUC__) && !defined(__ICC)
/*
   g++ can issue a warning at each usage of a function declared with this special attribute
   (also works with typedefs and variable declarations)
*/
# define IS_DEPRECATED __attribute__ ((__deprecated__))
/*
  the specified function is inlined at any optimization level
*/
# define ALWAYS_INLINE __attribute__((always_inline))
#else
# define IS_DEPRECATED
# define ALWAYS_INLINE
#endif

}

  /* ******************************************************************** */
  /*	Import/export classes and interfaces from a shared library          */
  /* ******************************************************************** */

#if defined(EXPORTED_TO_SHARED_LIB)
#  if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#     define APIDECL __declspec(dllexport)
#  elif defined(__GNUC__)
#     define __attribute__((visibility("default")))
#  else
#     define APIDECL
#  endif
#   if defined(IMPORTED_FROM_SHARED_LIB)
#	  error INTENTIONAL COMPILCATION ERROR, DLL IMPORT AND EXPORT ARE INCOMPITABLE
#   endif
#endif

#if defined(IMPORTED_FROM_SHARED_LIB)
#  if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#     define APIDECL __declspec(dllimport)
#  else
#     define APIDECL
#  endif
#   if defined(EXPORTED_TO_SHARED_LIB)
#	  error INTENTIONAL COMPILCATION ERROR, DLL IMPORT AND EXPORT ARE INCOMPITABLE
#   endif
#endif

#ifndef EXPORTED_TO_SHARED_LIB
#  ifndef IMPORTED_FROM_SHARED_LIB
#    define APIDECL  //empty, used during static linking
#  endif
#endif

#endif /* GMM_STD_H__ */
