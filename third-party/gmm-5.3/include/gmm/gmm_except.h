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

/** @file gmm_except.h
    @author Yves Renard <Yves.Renard@insa-lyon.fr>
    @author Julien Pommier <Julien.Pommier@insa-toulouse.fr>
    @date September 01, 2002.
    @brief Definition of basic exceptions.
*/

#ifndef GMM_EXCEPT_H__
#define GMM_EXCEPT_H__

#include <sstream>
#include "gmm_std.h"
#include "gmm_feedback_management.h"


//provides external implementation of gmm_exception and logging.

namespace gmm {

/* *********************************************************************** */
/*        GetFEM++ generic errors.                                         */
/* *********************************************************************** */

  // std logic_error with error level information
  class gmm_error: public std::logic_error {
  public:
    gmm_error(const std::string& what_arg, int errorLevel = 1):
      std::logic_error (what_arg), errorLevel_(errorLevel) {}
    int errLevel() {return errorLevel_;}

  private:
    int errorLevel_;
  };

#ifdef GETFEM_HAVE_PRETTY_FUNCTION
#  define GMM_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif _MSC_VER
#  define GMM_PRETTY_FUNCTION __FUNCTION__
#else 
#  define GMM_PRETTY_FUNCTION ""
#endif


  // Errors : GMM_THROW should not be used on its own.
  //          GMM_ASSERT1 : Non-maskable errors. Typically for in/ouput and
  //               when the test do not significantly reduces the performance.
  //          GMM_ASSERT2 : All tests which are potentially performance
  //               consuming. Not hidden by default. Hidden when NDEBUG is
  //               defined.
  //          GMM_ASSERT3 : For internal checks. Hidden by default. Active
  //               only when DEBUG_MODE is defined.
  // __EXCEPTIONS is defined by gcc, _CPPUNWIND is defined by visual c++
#if defined(__EXCEPTIONS) || defined(_CPPUNWIND)
  inline void short_error_throw(const char *file, int line, const char *func,
                                const char *errormsg) {
    std::stringstream msg__;
    msg__ << "Error in " << file << ", line " << line << " " << func
          << ": \n" << errormsg << std::ends;
    throw gmm::gmm_error(msg__.str());
  }
# define GMM_THROW_(type, errormsg) {                         \
    std::stringstream msg__;                                  \
    msg__ << "Error in " << __FILE__ << ", line "             \
          << __LINE__ << " " << GMM_PRETTY_FUNCTION << ": \n" \
          << errormsg << std::ends;                           \
    throw (type)(msg__.str());                                \
  }

# define GMM_THROW_AT_LEVEL(errormsg, level)                  \
  {                                                           \
  std::stringstream msg;                                      \
  msg << "Error in " << __FILE__ << ", line "                 \
  << __LINE__ << " " << GMM_PRETTY_FUNCTION << ": \n"         \
  << errormsg << std::ends;                                   \
  throw gmm::gmm_error(msg.str(), level);                     \
  }

#else
#ifndef _MSC_VER
# define abort_no_return() ::abort()
#else
// apparently ::abort() on windows is not declared with __declspec(noreturn) so the compiler spits a lot of warnings when abort is used.
# define abort_no_return() { assert("GMM ABORT"==0); throw "GMM ABORT"; }
#endif

  inline void short_error_throw(const char *file, int line, const char *func,
                                const char *errormsg) {
    std::stringstream msg__;
    msg__ << "Error in " << file << ", line " << line << " " << func
          << ": \n" << errormsg << std::ends;
    std::cerr << msg__.str() << std::endl;
    abort_no_return();
  }

# define GMM_THROW_(type, errormsg) {                         \
    std::stringstream msg__;                                  \
    msg__ << "Error in " << __FILE__ << ", line "             \
          << __LINE__ << " " << GMM_PRETTY_FUNCTION << ": \n" \
          << errormsg;                                        \
    std::cerr << msg__.str() << std::endl;                    \
    abort_no_return();                                        \
  }

# define GMM_THROW_AT_LEVEL(errormsg, level)                  \
  {                                                           \
  std::stringstream msg__;                                    \
  msg__ << "Error in " << __FILE__ << ", line "               \
  << __LINE__ << " " << GMM_PRETTY_FUNCTION << ": \n"         \
  << errormsg <<  " at level " << level;                      \
  std::cerr << msg__.str()  << std::endl;                     \
  abort_no_return();                                          \
  }
#endif


inline void GMM_THROW() {}
#define GMM_THROW(a, b) { GMM_THROW_(a,b); gmm::GMM_THROW(); }

# define GMM_THROW_DEFAULT(errormsg) GMM_THROW_AT_LEVEL(errormsg, 1)

// This allows not to compile some assertions 
#ifndef GMM_ASSERT_LEVEL
#if defined(NDEBUG)
# define GMM_ASSERT_LEVEL 1
#elif defined(DEBUG_MODE)
# define GMM_ASSERT_LEVEL 3
#else
# define GMM_ASSERT_LEVEL 2
#endif
#endif


# define GMM_ASSERT1(test, errormsg) { if (!(test)) GMM_THROW_AT_LEVEL(errormsg, 1); }

#if GMM_ASSERT_LEVEL < 2
# define GMM_ASSERT2(test, errormsg) {}
# define GMM_ASSERT3(test, errormsg) {}
#elif GMM_ASSERT_LEVEL < 3
# define GMM_ASSERT2(test, errormsg){ if (!(test)) GMM_THROW_AT_LEVEL(errormsg, 2); }
# define GMM_ASSERT3(test, errormsg){}
#else
# define GMM_ASSERT2(test, errormsg){ if (!(test)) GMM_THROW_AT_LEVEL(errormsg, 2); }
# define GMM_ASSERT3(test, errormsg){ if (!(test)) GMM_THROW_AT_LEVEL(errormsg, 3); }
#endif

/* *********************************************************************** */
/*        GetFEM++ warnings.                                               */
/* *********************************************************************** */

  // This allows not to compile some Warnings
#ifndef GMM_WARNING_LEVEL
# define GMM_WARNING_LEVEL 4
#endif

  // Warning levels : 0 always printed
  //                  1 very important : specify a possible error in the code.
  //                  2 important : specify a default of optimization for inst.
  //                  3 remark
  //                  4 ignored by default.

#define GMM_WARNING_MSG(level_, thestr)  {                                                \
  std::stringstream msg__;                                                                \
  msg__ << "Level " << level_ << " Warning in " << __FILE__ << ", line "                  \
  << __LINE__ << ": " << thestr;                                                          \
  gmm::feedback_manager::manage()->send(msg__.str(), gmm::FeedbackType::WARNING, level_); \
}

#define GMM_WARNING0(thestr) GMM_WARNING_MSG(0, thestr)


#if GMM_WARNING_LEVEL > 0
# define GMM_WARNING1(thestr)                                           \
  { if (1 <= gmm::feedback_manager::warning_level()) GMM_WARNING_MSG(1, thestr) }
#else
# define GMM_WARNING1(thestr) {}
#endif

#if GMM_WARNING_LEVEL > 1
# define GMM_WARNING2(thestr)                                           \
  { if (2 <= gmm::feedback_manager::warning_level()) GMM_WARNING_MSG(2, thestr) }
#else
# define GMM_WARNING2(thestr) {}
#endif

#if GMM_WARNING_LEVEL > 2
# define GMM_WARNING3(thestr)                                           \
  { if (3 <= gmm::feedback_manager::warning_level()) GMM_WARNING_MSG(3, thestr) }
#else
# define GMM_WARNING3(thestr) {}
#endif

#if GMM_WARNING_LEVEL > 3
# define GMM_WARNING4(thestr)                                           \
  { if (4 <= gmm::feedback_manager::warning_level()) GMM_WARNING_MSG(4, thestr) }
#else
# define GMM_WARNING4(thestr) {}
#endif

/* *********************************************************************** */
/*        GetFEM++ traces.                                                 */
/* *********************************************************************** */

  // This allow not too compile some Warnings
#ifndef GMM_TRACES_LEVEL
# define GMM_TRACES_LEVEL 4
#endif

  // Traces levels : 0 always printed
  //                 1 Susceptible to occur once in a program.
  //                 2 Susceptible to occur occasionnaly in a program (10).
  //                 3 Susceptible to occur often (100).
  //                 4 Susceptible to occur very often (>1000).

#define GMM_TRACE_MSG_MPI     // for Parallelized version
#define GMM_TRACE_MSG(level_, thestr)  {                                        \
  GMM_TRACE_MSG_MPI {                                                           \
    std::stringstream msg__;                                                    \
    msg__ << "Trace " << level_ << " in " << __FILE__ << ", line "              \
          << __LINE__ << ": " << thestr;                                        \
    gmm::feedback_manager::send(msg__.str(), gmm::FeedbackType::TRACE, level_); \
  }                                                                             \
}

#define GMM_TRACE_SIMPLE_MSG(level_, thestr)  {                               \
  GMM_TRACE_MSG_MPI {                                                         \
  std::stringstream msg__;                                                    \
  msg__ << "Trace " << level_ << ": " << thestr;                              \
  gmm::feedback_manager::send(msg__.str(), gmm::FeedbackType::TRACE, level_); \
  }                                                                           \
}

#define GMM_TRACE0(thestr) GMM_TRACE_MSG(0, thestr)
#define GMM_SIMPLE_TRACE0(thestr) GMM_TRACE_MSG_SIMPLE(0, thestr)

#if GMM_TRACES_LEVEL > 0
# define GMM_TRACE1(thestr)                                                          \
  { if (1 <= gmm::feedback_manager::traces_level()) GMM_TRACE_MSG(1, thestr) }
# define GMM_SIMPLE_TRACE1(thestr)                                                  \
  { if (1 <= gmm::feedback_manager::traces_level()) GMM_TRACE_SIMPLE_MSG(1, thestr) }
#else
# define GMM_TRACE1(thestr) {}
# define GMM_SIMPLE_TRACE1(thestr) {}
#endif

#if GMM_TRACES_LEVEL > 1
# define GMM_TRACE2(thestr)                                                           \
  { if (2 <= gmm::feedback_manager::traces_level()) GMM_TRACE_MSG(2, thestr) }
# define GMM_SIMPLE_TRACE2(thestr)                                                   \
  { if (2 <= gmm::feedback_manager::traces_level()) GMM_TRACE_SIMPLE_MSG(2, thestr) }
#else
# define GMM_TRACE2(thestr) {}
# define GMM_SIMPLE_TRACE2(thestr) {}
#endif

#if GMM_TRACES_LEVEL > 2
# define GMM_TRACE3(thestr)                                                        \
  { if (3 <= gmm::feedback_manager::traces_level()) GMM_TRACE_MSG(3, thestr) }
# define GMM_SIMPLE_TRACE3(thestr)                                                \
  { if (3 <= gmm::feedback_manager::traces_level()) GMM_TRACE_SIMPLE_MSG(3, thestr) }
#else
# define GMM_TRACE3(thestr) {}
# define GMM_SIMPLE_TRACE3(thestr) {}
#endif

#if GMM_TRACES_LEVEL > 3
# define GMM_TRACE4(thestr)                                                         \
  { if (4 <= gmm::feedback_manager::traces_level()) GMM_TRACE_MSG(4, thestr) }
# define GMM_SIMPLE_TRACE4(thestr)                                                 \
  { if (4 <= gmm::feedback_manager::traces_level()) GMM_TRACE_SIMPLE_MSG(4, thestr) }
#else
# define GMM_TRACE4(thestr) {}
# define GMM_SIMPLE_TRACE4(thestr) {}
#endif

#define GMM_STANDARD_CATCH_ERROR                                           \
  catch(gmm::gmm_error e)                                                  \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "|    A GMM error has been detected !!!     |\n";         \
    strStream << "============================================\n";         \
    strStream << e.what() << std::endl << std::endl;                       \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, e.errLevel());   \
    gmm::feedback_manager::terminating_action();                           \
  }                                                                        \
  catch(std::logic_error e)                                                \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "|       An error has been detected !!!     |\n";         \
    strStream << "============================================\n";         \
    strStream << e.what() << std::endl << std::endl;                       \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, 0);              \
    gmm::feedback_manager::terminating_action();                           \
  }                                                                        \
  catch(std::runtime_error e)                                              \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "| A runtime error has been detected !!!    |\n";         \
    strStream << "============================================\n";         \
    strStream << e.what() << std::endl << std::endl;                       \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, 0);              \
    gmm::feedback_manager::terminating_action();                           \
  }                                                                        \
  catch(std::bad_alloc)                                                    \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "| A bad allocation has been detected !!!   |\n";         \
    strStream << "============================================\n";         \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, 0);              \
    gmm::feedback_manager::terminating_action();                           \
  }                                                                        \
  catch(std::bad_typeid)                                                   \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "|  A bad typeid has been detected !!!      |\n";         \
    strStream << "============================================\n";         \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, 0);              \
    gmm::feedback_manager::terminating_action();                           \
  }                                                                        \
  catch(std::bad_exception)                                                \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "|  A bad exception has been detected !!!   |\n";         \
    strStream << "============================================\n";         \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, 0);              \
    gmm::feedback_manager::terminating_action();                           \
  }                                                                        \
  catch(std::bad_cast)                                                     \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "|      A bad_cast has been detected !!!    |\n";         \
    strStream << "============================================\n";         \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, 0);              \
    gmm::feedback_manager::terminating_action();                           \
  }                                                                        \
  catch(...)                                                               \
  {                                                                        \
    std::stringstream strStream;                                           \
    strStream << "============================================\n";         \
    strStream << "|  An unknown error has been detected !!!  |\n";         \
    strStream << "============================================\n";         \
    gmm::feedback_manager::send(strStream.str(),                           \
                               gmm::FeedbackType::ASSERT, 0);              \
    gmm::feedback_manager::terminating_action();                           \
  }
  //   catch(ios_base::failure) {
  //     std::cerr << "============================================\n";
  //     std::cerr << "| A ios_base::failure has been detected !!!|\n";
  //     std::cerr << "============================================\n";
  //     exit(1);
  //   }

#if defined(__GNUC__) && (__GNUC__ > 3)
# define GMM_SET_EXCEPTION_DEBUG                                \
  std::set_terminate(__gnu_cxx::__verbose_terminate_handler);
#else
# define GMM_SET_EXCEPTION_DEBUG
#endif

} // namespace gmm

#endif /* GMM_EXCEPT_H__ */
