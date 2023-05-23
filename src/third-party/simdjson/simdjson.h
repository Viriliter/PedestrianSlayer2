/* auto-generated on 2021-03-18 11:31:38 -0400. Do not edit! */
/* begin file include/simdjson.h */
#ifndef SIMDJSON_H
#define SIMDJSON_H

/**
 * @mainpage
 *
 * Check the [README.md](https://github.com/lemire/simdjson/blob/master/README.md#simdjson--parsing-gigabytes-of-json-per-second).
 *
 * Sample code. See https://github.com/simdjson/simdjson/blob/master/doc/basics.md for more examples.

    #include "simdjson.h"

    int main(void) {
      // load from `twitter.json` file:
      simdjson::dom::parser parser;
      simdjson::dom::element tweets = parser.load("twitter.json");
      std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;

      // Parse and iterate through an array of objects
      auto abstract_json = R"( [
        {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
        {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
        ] )"_padded;

      for (simdjson::dom::object obj : parser.parse(abstract_json)) {
        for(const auto key_value : obj) {
          cout << "key: " << key_value.key << " : ";
          simdjson::dom::object innerobj = key_value.value;
          cout << "a: " << double(innerobj["a"]) << ", ";
          cout << "b: " << double(innerobj["b"]) << ", ";
          cout << "c: " << int64_t(innerobj["c"]) << endl;
        }
      }
    }
 */

/* begin file include/simdjson/dom.h */
#ifndef SIMDJSON_DOM_H
#define SIMDJSON_DOM_H

/* begin file include/simdjson/base.h */
#ifndef SIMDJSON_BASE_H
#define SIMDJSON_BASE_H

/* begin file include/simdjson/compiler_check.h */
#ifndef SIMDJSON_COMPILER_CHECK_H
#define SIMDJSON_COMPILER_CHECK_H

#ifndef __cplusplus
#error simdjson requires a C++ compiler
#endif

#ifndef SIMDJSON_CPLUSPLUS
#if defined(_MSVC_LANG) && !defined(__clang__)
#define SIMDJSON_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
#else
#define SIMDJSON_CPLUSPLUS __cplusplus
#endif
#endif

// C++ 17
#if !defined(SIMDJSON_CPLUSPLUS17) && (SIMDJSON_CPLUSPLUS >= 201703L)
#define SIMDJSON_CPLUSPLUS17 1
#endif

// C++ 14
#if !defined(SIMDJSON_CPLUSPLUS14) && (SIMDJSON_CPLUSPLUS >= 201402L)
#define SIMDJSON_CPLUSPLUS14 1
#endif

// C++ 11
#if !defined(SIMDJSON_CPLUSPLUS11) && (SIMDJSON_CPLUSPLUS >= 201103L)
#define SIMDJSON_CPLUSPLUS11 1
#endif

#ifndef SIMDJSON_CPLUSPLUS11
#error simdjson requires a compiler compliant with the C++11 standard
#endif

#endif // SIMDJSON_COMPILER_CHECK_H
/* end file include/simdjson/compiler_check.h */
/* begin file include/simdjson/common_defs.h */
#ifndef SIMDJSON_COMMON_DEFS_H
#define SIMDJSON_COMMON_DEFS_H

#include <cassert>
/* begin file include/simdjson/portability.h */
#ifndef SIMDJSON_PORTABILITY_H
#define SIMDJSON_PORTABILITY_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cfloat>
#include <cassert>
#ifndef _WIN32
// strcasecmp, strncasecmp
#include <strings.h>
#endif

#ifdef _MSC_VER
#define SIMDJSON_VISUAL_STUDIO 1
/**
 * We want to differentiate carefully between
 * clang under visual studio and regular visual
 * studio.
 *
 * Under clang for Windows, we enable:
 *  * target pragmas so that part and only part of the
 *     code gets compiled for advanced instructions.
 *
 */
#ifdef __clang__
// clang under visual studio
#define SIMDJSON_CLANG_VISUAL_STUDIO 1
#else
// just regular visual studio (best guess)
#define SIMDJSON_REGULAR_VISUAL_STUDIO 1
#endif // __clang__
#endif // _MSC_VER

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
// https://en.wikipedia.org/wiki/C_alternative_tokens
// This header should have no effect, except maybe
// under Visual Studio.
#include <iso646.h>
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
#define SIMDJSON_IS_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define SIMDJSON_IS_ARM64 1
#elif defined(__PPC64__) || defined(_M_PPC64)
#define SIMDJSON_IS_PPC64 1
#else
#define SIMDJSON_IS_32BITS 1

// We do not support 32-bit platforms, but it can be
// handy to identify them.
#if defined(_M_IX86) || defined(__i386__)
#define SIMDJSON_IS_X86_32BITS 1
#elif defined(__arm__) || defined(_M_ARM)
#define SIMDJSON_IS_ARM_32BITS 1
#elif defined(__PPC__) || defined(_M_PPC)
#define SIMDJSON_IS_PPC_32BITS 1
#endif

#endif // defined(__x86_64__) || defined(_M_AMD64)

#ifdef SIMDJSON_IS_32BITS
#ifndef SIMDJSON_NO_PORTABILITY_WARNING
#pragma message("The simdjson library is designed \
for 64-bit processors and it seems that you are not \
compiling for a known 64-bit platform. All fast kernels \
will be disabled and performance may be poor. Please \
use a 64-bit target such as x64, 64-bit ARM or 64-bit PPC.")
#endif // SIMDJSON_NO_PORTABILITY_WARNING
#endif // SIMDJSON_IS_32BITS

// this is almost standard?
#undef STRINGIFY_IMPLEMENTATION_
#undef STRINGIFY
#define STRINGIFY_IMPLEMENTATION_(a) #a
#define STRINGIFY(a) STRINGIFY_IMPLEMENTATION_(a)

// Our fast kernels require 64-bit systems.
//
// On 32-bit x86, we lack 64-bit popcnt, lzcnt, blsr instructions.
// Furthermore, the number of SIMD registers is reduced.
//
// On 32-bit ARM, we would have smaller registers.
//
// The simdjson users should still have the fallback kernel. It is
// slower, but it should run everywhere.

//
// Enable valid runtime implementations, and select SIMDJSON_BUILTIN_IMPLEMENTATION
//

// We are going to use runtime dispatch.
#ifdef SIMDJSON_IS_X86_64
#ifdef __clang__
// clang does not have GCC push pop
// warning: clang attribute push can't be used within a namespace in clang up
// til 8.0 so SIMDJSON_TARGET_REGION and SIMDJSON_UNTARGET_REGION must be *outside* of a
// namespace.
#define SIMDJSON_TARGET_REGION(T)                                                       \
  _Pragma(STRINGIFY(                                                           \
      clang attribute push(__attribute__((target(T))), apply_to = function)))
#define SIMDJSON_UNTARGET_REGION _Pragma("clang attribute pop")
#elif defined(__GNUC__)
// GCC is easier
#define SIMDJSON_TARGET_REGION(T)                                                       \
  _Pragma("GCC push_options") _Pragma(STRINGIFY(GCC target(T)))
#define SIMDJSON_UNTARGET_REGION _Pragma("GCC pop_options")
#endif // clang then gcc

#endif // x86

// Default target region macros don't do anything.
#ifndef SIMDJSON_TARGET_REGION
#define SIMDJSON_TARGET_REGION(T)
#define SIMDJSON_UNTARGET_REGION
#endif

// Is threading enabled?
#if defined(_REENTRANT) || defined(_MT)
#ifndef SIMDJSON_THREADS_ENABLED
#define SIMDJSON_THREADS_ENABLED
#endif
#endif

// workaround for large stack sizes under -O0.
// https://github.com/simdjson/simdjson/issues/691
#ifdef __APPLE__
#ifndef __OPTIMIZE__
// Apple systems have small stack sizes in secondary threads.
// Lack of compiler optimization may generate high stack usage.
// Users may want to disable threads for safety, but only when
// in debug mode which we detect by the fact that the __OPTIMIZE__
// macro is not defined.
#undef SIMDJSON_THREADS_ENABLED
#endif
#endif


#if defined(__clang__)
#define NO_SANITIZE_UNDEFINED __attribute__((no_sanitize("undefined")))
#elif defined(__GNUC__)
#define NO_SANITIZE_UNDEFINED __attribute__((no_sanitize_undefined))
#else
#define NO_SANITIZE_UNDEFINED
#endif

#ifdef SIMDJSON_VISUAL_STUDIO
// This is one case where we do not distinguish between
// regular visual studio and clang under visual studio.
// clang under Windows has _stricmp (like visual studio) but not strcasecmp (as clang normally has)
#define simdjson_strcasecmp _stricmp
#define simdjson_strncasecmp _strnicmp
#else
// The strcasecmp, strncasecmp, and strcasestr functions do not work with multibyte strings (e.g. UTF-8).
// So they are only useful for ASCII in our context.
// https://www.gnu.org/software/libunistring/manual/libunistring.html#char-_002a-strings
#define simdjson_strcasecmp strcasecmp
#define simdjson_strncasecmp strncasecmp
#endif

#ifdef NDEBUG

#ifdef SIMDJSON_VISUAL_STUDIO
#define SIMDJSON_UNREACHABLE() __assume(0)
#define SIMDJSON_ASSUME(COND) __assume(COND)
#else
#define SIMDJSON_UNREACHABLE() __builtin_unreachable();
#define SIMDJSON_ASSUME(COND) do { if (!(COND)) __builtin_unreachable(); } while (0)
#endif

#else // NDEBUG

#define SIMDJSON_UNREACHABLE() assert(0);
#define SIMDJSON_ASSUME(COND) assert(COND)

#endif

#endif // SIMDJSON_PORTABILITY_H
/* end file include/simdjson/portability.h */

namespace simdjson {

namespace internal {
/**
 * @private
 * Our own implementation of the C++17 to_chars function.
 * Defined in src/to_chars
 */
char *to_chars(char *first, const char *last, double value);
/**
 * @private
 * A number parsing routine.
 * Defined in src/from_chars
 */
double from_chars(const char *first) noexcept;
}

#ifndef SIMDJSON_EXCEPTIONS
#if __cpp_exceptions
#define SIMDJSON_EXCEPTIONS 1
#else
#define SIMDJSON_EXCEPTIONS 0
#endif
#endif

/** The maximum document size supported by simdjson. */
constexpr size_t SIMDJSON_MAXSIZE_BYTES = 0xFFFFFFFF;

/**
 * The amount of padding needed in a buffer to parse JSON.
 *
 * the input buf should be readable up to buf + SIMDJSON_PADDING
 * this is a stopgap; there should be a better description of the
 * main loop and its behavior that abstracts over this
 * See https://github.com/lemire/simdjson/issues/174
 */
constexpr size_t SIMDJSON_PADDING = 32;

/**
 * By default, simdjson supports this many nested objects and arrays.
 *
 * This is the default for parser::max_depth().
 */
constexpr size_t DEFAULT_MAX_DEPTH = 1024;

} // namespace simdjson

#if defined(__GNUC__)
  // Marks a block with a name so that MCA analysis can see it.
  #define SIMDJSON_BEGIN_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-BEGIN " #name);
  #define SIMDJSON_END_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-END " #name);
  #define SIMDJSON_DEBUG_BLOCK(name, block) BEGIN_DEBUG_BLOCK(name); block; END_DEBUG_BLOCK(name);
#else
  #define SIMDJSON_BEGIN_DEBUG_BLOCK(name)
  #define SIMDJSON_END_DEBUG_BLOCK(name)
  #define SIMDJSON_DEBUG_BLOCK(name, block)
#endif

// Align to N-byte boundary
#define SIMDJSON_ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define SIMDJSON_ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define SIMDJSON_ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n)-1)) == 0)

#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO)

  #define simdjson_really_inline __forceinline
  #define simdjson_never_inline __declspec(noinline)

  #define simdjson_unused
  #define simdjson_warn_unused

  #ifndef simdjson_likely
  #define simdjson_likely(x) x
  #endif
  #ifndef simdjson_unlikely
  #define simdjson_unlikely(x) x
  #endif

  #define SIMDJSON_PUSH_DISABLE_WARNINGS __pragma(warning( push ))
  #define SIMDJSON_PUSH_DISABLE_ALL_WARNINGS __pragma(warning( push, 0 ))
  #define SIMDJSON_DISABLE_VS_WARNING(WARNING_NUMBER) __pragma(warning( disable : WARNING_NUMBER ))
  // Get rid of Intellisense-only warnings (Code Analysis)
  // Though __has_include is C++17, it is supported in Visual Studio 2017 or better (_MSC_VER>=1910).
  #ifdef __has_include
  #if __has_include(<CppCoreCheck\Warnings.h>)
  #include <CppCoreCheck\Warnings.h>
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS SIMDJSON_DISABLE_VS_WARNING(ALL_CPPCORECHECK_WARNINGS)
  #endif
  #endif

  #ifndef SIMDJSON_DISABLE_UNDESIRED_WARNINGS
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS
  #endif

  #define SIMDJSON_DISABLE_DEPRECATED_WARNING SIMDJSON_DISABLE_VS_WARNING(4996)
  #define SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
  #define SIMDJSON_POP_DISABLE_WARNINGS __pragma(warning( pop ))

#else // SIMDJSON_REGULAR_VISUAL_STUDIO

  #define simdjson_really_inline inline __attribute__((always_inline))
  #define simdjson_never_inline inline __attribute__((noinline))

  #define simdjson_unused __attribute__((unused))
  #define simdjson_warn_unused __attribute__((warn_unused_result))

  #ifndef simdjson_likely
  #define simdjson_likely(x) __builtin_expect(!!(x), 1)
  #endif
  #ifndef simdjson_unlikely
  #define simdjson_unlikely(x) __builtin_expect(!!(x), 0)
  #endif

  #define SIMDJSON_PUSH_DISABLE_WARNINGS _Pragma("GCC diagnostic push")
  // gcc doesn't seem to disable all warnings with all and extra, add warnings here as necessary
  #define SIMDJSON_PUSH_DISABLE_ALL_WARNINGS SIMDJSON_PUSH_DISABLE_WARNINGS \
    SIMDJSON_DISABLE_GCC_WARNING(-Weffc++) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wall) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wconversion) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wextra) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wattributes) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wimplicit-fallthrough) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wnon-virtual-dtor) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wreturn-type) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wshadow) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wunused-parameter) \
    SIMDJSON_DISABLE_GCC_WARNING(-Wunused-variable)
  #define SIMDJSON_PRAGMA(P) _Pragma(#P)
  #define SIMDJSON_DISABLE_GCC_WARNING(WARNING) SIMDJSON_PRAGMA(GCC diagnostic ignored #WARNING)
  #if defined(SIMDJSON_CLANG_VISUAL_STUDIO)
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS SIMDJSON_DISABLE_GCC_WARNING(-Wmicrosoft-include)
  #else
  #define SIMDJSON_DISABLE_UNDESIRED_WARNINGS
  #endif
  #define SIMDJSON_DISABLE_DEPRECATED_WARNING SIMDJSON_DISABLE_GCC_WARNING(-Wdeprecated-declarations)
  #define SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING SIMDJSON_DISABLE_GCC_WARNING(-Wstrict-overflow)
  #define SIMDJSON_POP_DISABLE_WARNINGS _Pragma("GCC diagnostic pop")



#endif // MSC_VER

#if defined(SIMDJSON_VISUAL_STUDIO)
    /**
     * Windows users need to do some extra work when building
     * or using a dynamic library (DLL). When building, we need
     * to set SIMDJSON_DLLIMPORTEXPORT to __declspec(dllexport).
     * When *using* the DLL, the user needs to set
     * SIMDJSON_DLLIMPORTEXPORT __declspec(dllimport).
     *
     * Static libraries not need require such work.
     *
     * It does not matter here whether you are using
     * the regular visual studio or clang under visual
     * studio, you still need to handle these issues.
     *
     * Non-Windows sytems do not have this complexity.
     */
    #if SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY
    // We set SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY when we build a DLL under Windows.
    // It should never happen that both SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY and
    // SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY are set.
    #define SIMDJSON_DLLIMPORTEXPORT __declspec(dllexport)
    #elif SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY
    // Windows user who call a dynamic library should set SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY to 1.
    #define SIMDJSON_DLLIMPORTEXPORT __declspec(dllimport)
    #else
    // We assume by default static linkage
    #define SIMDJSON_DLLIMPORTEXPORT
    #endif

/**
 * Workaround for the vcpkg package manager. Only vcpkg should
 * ever touch the next line. The SIMDJSON_USING_LIBRARY macro is otherwise unused.
 */
#if SIMDJSON_USING_LIBRARY
#define SIMDJSON_DLLIMPORTEXPORT __declspec(dllimport)
#endif
/**
 * End of workaround for the vcpkg package manager.
 */
#else
    #define SIMDJSON_DLLIMPORTEXPORT
#endif

// C++17 requires string_view.
#if SIMDJSON_CPLUSPLUS17
#define SIMDJSON_HAS_STRING_VIEW
#include <string_view> // by the standard, this has to be safe.
#endif

// This macro (__cpp_lib_string_view) has to be defined
// for C++17 and better, but if it is otherwise defined,
// we are going to assume that string_view is available
// even if we do not have C++17 support.
#ifdef __cpp_lib_string_view
#define SIMDJSON_HAS_STRING_VIEW
#endif

// Some systems have string_view even if we do not have C++17 support,
// and even if __cpp_lib_string_view is undefined, it is the case
// with Apple clang version 11.
// We must handle it. *This is important.*
#ifndef SIMDJSON_HAS_STRING_VIEW
#if defined __has_include
// do not combine the next #if with the previous one (unsafe)
#if __has_include (<string_view>)
// now it is safe to trigger the include
#include <string_view> // though the file is there, it does not follow that we got the implementation
#if defined(_LIBCPP_STRING_VIEW)
// Ah! So we under libc++ which under its Library Fundamentals Technical Specification, which preceeded C++17,
// included string_view.
// This means that we have string_view *even though* we may not have C++17.
#define SIMDJSON_HAS_STRING_VIEW
#endif // _LIBCPP_STRING_VIEW
#endif // __has_include (<string_view>)
#endif // defined __has_include
#endif // def SIMDJSON_HAS_STRING_VIEW
// end of complicated but important routine to try to detect string_view.

//
// Backfill std::string_view using nonstd::string_view on systems where
// we expect that string_view is missing. Important: if we get this wrong,
// we will end up with two string_view definitions and potential trouble.
// That is why we work so hard above to avoid it.
//
#ifndef SIMDJSON_HAS_STRING_VIEW
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
/* begin file include/simdjson/nonstd/string_view.hpp */
// Copyright 2017-2019 by Martin Moene
//
// string-view lite, a C++17-like string_view for C++98 and later.
// For more information see https://github.com/martinmoene/string-view-lite
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef NONSTD_SV_LITE_H_INCLUDED
#define NONSTD_SV_LITE_H_INCLUDED

#define string_view_lite_MAJOR  1
#define string_view_lite_MINOR  4
#define string_view_lite_PATCH  0

#define string_view_lite_VERSION  nssv_STRINGIFY(string_view_lite_MAJOR) "." nssv_STRINGIFY(string_view_lite_MINOR) "." nssv_STRINGIFY(string_view_lite_PATCH)

#define nssv_STRINGIFY(  x )  nssv_STRINGIFY_( x )
#define nssv_STRINGIFY_( x )  #x

// string-view lite configuration:

#define nssv_STRING_VIEW_DEFAULT  0
#define nssv_STRING_VIEW_NONSTD   1
#define nssv_STRING_VIEW_STD      2

#if !defined( nssv_CONFIG_SELECT_STRING_VIEW )
# define nssv_CONFIG_SELECT_STRING_VIEW  ( nssv_HAVE_STD_STRING_VIEW ? nssv_STRING_VIEW_STD : nssv_STRING_VIEW_NONSTD )
#endif

#if defined( nssv_CONFIG_SELECT_STD_STRING_VIEW ) || defined( nssv_CONFIG_SELECT_NONSTD_STRING_VIEW )
# error nssv_CONFIG_SELECT_STD_STRING_VIEW and nssv_CONFIG_SELECT_NONSTD_STRING_VIEW are deprecated and removed, please use nssv_CONFIG_SELECT_STRING_VIEW=nssv_STRING_VIEW_...
#endif

#ifndef  nssv_CONFIG_STD_SV_OPERATOR
# define nssv_CONFIG_STD_SV_OPERATOR  0
#endif

#ifndef  nssv_CONFIG_USR_SV_OPERATOR
# define nssv_CONFIG_USR_SV_OPERATOR  1
#endif

#ifdef   nssv_CONFIG_CONVERSION_STD_STRING
# define nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS   nssv_CONFIG_CONVERSION_STD_STRING
# define nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS  nssv_CONFIG_CONVERSION_STD_STRING
#endif

#ifndef  nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS
# define nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS  1
#endif

#ifndef  nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS
# define nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS  1
#endif

// Control presence of exception handling (try and auto discover):

#ifndef nssv_CONFIG_NO_EXCEPTIONS
# if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#  define nssv_CONFIG_NO_EXCEPTIONS  0
# else
#  define nssv_CONFIG_NO_EXCEPTIONS  1
# endif
#endif

// C++ language version detection (C++20 is speculative):
// Note: VC14.0/1900 (VS2015) lacks too much from C++14.

#ifndef   nssv_CPLUSPLUS
# if defined(_MSVC_LANG ) && !defined(__clang__)
#  define nssv_CPLUSPLUS  (_MSC_VER == 1900 ? 201103L : _MSVC_LANG )
# else
#  define nssv_CPLUSPLUS  __cplusplus
# endif
#endif

#define nssv_CPP98_OR_GREATER  ( nssv_CPLUSPLUS >= 199711L )
#define nssv_CPP11_OR_GREATER  ( nssv_CPLUSPLUS >= 201103L )
#define nssv_CPP11_OR_GREATER_ ( nssv_CPLUSPLUS >= 201103L )
#define nssv_CPP14_OR_GREATER  ( nssv_CPLUSPLUS >= 201402L )
#define nssv_CPP17_OR_GREATER  ( nssv_CPLUSPLUS >= 201703L )
#define nssv_CPP20_OR_GREATER  ( nssv_CPLUSPLUS >= 202000L )

// use C++17 std::string_view if available and requested:

#if nssv_CPP17_OR_GREATER && defined(__has_include )
# if __has_include( <string_view> )
#  define nssv_HAVE_STD_STRING_VIEW  1
# else
#  define nssv_HAVE_STD_STRING_VIEW  0
# endif
#else
# define  nssv_HAVE_STD_STRING_VIEW  0
#endif

#define  nssv_USES_STD_STRING_VIEW  ( (nssv_CONFIG_SELECT_STRING_VIEW == nssv_STRING_VIEW_STD) || ((nssv_CONFIG_SELECT_STRING_VIEW == nssv_STRING_VIEW_DEFAULT) && nssv_HAVE_STD_STRING_VIEW) )

#define nssv_HAVE_STARTS_WITH ( nssv_CPP20_OR_GREATER || !nssv_USES_STD_STRING_VIEW )
#define nssv_HAVE_ENDS_WITH     nssv_HAVE_STARTS_WITH

//
// Use C++17 std::string_view:
//

#if nssv_USES_STD_STRING_VIEW

#include <string_view>

// Extensions for std::string:

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {

template< class CharT, class Traits, class Allocator = std::allocator<CharT> >
std::basic_string<CharT, Traits, Allocator>
to_string( std::basic_string_view<CharT, Traits> v, Allocator const & a = Allocator() )
{
    return std::basic_string<CharT,Traits, Allocator>( v.begin(), v.end(), a );
}

template< class CharT, class Traits, class Allocator >
std::basic_string_view<CharT, Traits>
to_string_view( std::basic_string<CharT, Traits, Allocator> const & s )
{
    return std::basic_string_view<CharT, Traits>( s.data(), s.size() );
}

// Literal operators sv and _sv:

#if nssv_CONFIG_STD_SV_OPERATOR

using namespace std::literals::string_view_literals;

#endif

#if nssv_CONFIG_USR_SV_OPERATOR

inline namespace literals {
inline namespace string_view_literals {


constexpr std::string_view operator "" _sv( const char* str, size_t len ) noexcept  // (1)
{
    return std::string_view{ str, len };
}

constexpr std::u16string_view operator "" _sv( const char16_t* str, size_t len ) noexcept  // (2)
{
    return std::u16string_view{ str, len };
}

constexpr std::u32string_view operator "" _sv( const char32_t* str, size_t len ) noexcept  // (3)
{
    return std::u32string_view{ str, len };
}

constexpr std::wstring_view operator "" _sv( const wchar_t* str, size_t len ) noexcept  // (4)
{
    return std::wstring_view{ str, len };
}

}} // namespace literals::string_view_literals

#endif // nssv_CONFIG_USR_SV_OPERATOR

} // namespace nonstd

#endif // nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {

using std::string_view;
using std::wstring_view;
using std::u16string_view;
using std::u32string_view;
using std::basic_string_view;

// literal "sv" and "_sv", see above

using std::operator==;
using std::operator!=;
using std::operator<;
using std::operator<=;
using std::operator>;
using std::operator>=;

using std::operator<<;

} // namespace nonstd

#else // nssv_HAVE_STD_STRING_VIEW

//
// Before C++17: use string_view lite:
//

// Compiler versions:
//
// MSVC++  6.0  _MSC_VER == 1200  nssv_COMPILER_MSVC_VERSION ==  60  (Visual Studio 6.0)
// MSVC++  7.0  _MSC_VER == 1300  nssv_COMPILER_MSVC_VERSION ==  70  (Visual Studio .NET 2002)
// MSVC++  7.1  _MSC_VER == 1310  nssv_COMPILER_MSVC_VERSION ==  71  (Visual Studio .NET 2003)
// MSVC++  8.0  _MSC_VER == 1400  nssv_COMPILER_MSVC_VERSION ==  80  (Visual Studio 2005)
// MSVC++  9.0  _MSC_VER == 1500  nssv_COMPILER_MSVC_VERSION ==  90  (Visual Studio 2008)
// MSVC++ 10.0  _MSC_VER == 1600  nssv_COMPILER_MSVC_VERSION == 100  (Visual Studio 2010)
// MSVC++ 11.0  _MSC_VER == 1700  nssv_COMPILER_MSVC_VERSION == 110  (Visual Studio 2012)
// MSVC++ 12.0  _MSC_VER == 1800  nssv_COMPILER_MSVC_VERSION == 120  (Visual Studio 2013)
// MSVC++ 14.0  _MSC_VER == 1900  nssv_COMPILER_MSVC_VERSION == 140  (Visual Studio 2015)
// MSVC++ 14.1  _MSC_VER >= 1910  nssv_COMPILER_MSVC_VERSION == 141  (Visual Studio 2017)
// MSVC++ 14.2  _MSC_VER >= 1920  nssv_COMPILER_MSVC_VERSION == 142  (Visual Studio 2019)

#if defined(_MSC_VER ) && !defined(__clang__)
# define nssv_COMPILER_MSVC_VER      (_MSC_VER )
# define nssv_COMPILER_MSVC_VERSION  (_MSC_VER / 10 - 10 * ( 5 + (_MSC_VER < 1900 ) ) )
#else
# define nssv_COMPILER_MSVC_VER      0
# define nssv_COMPILER_MSVC_VERSION  0
#endif

#define nssv_COMPILER_VERSION( major, minor, patch )  ( 10 * ( 10 * (major) + (minor) ) + (patch) )

#if defined(__clang__)
# define nssv_COMPILER_CLANG_VERSION  nssv_COMPILER_VERSION(__clang_major__, __clang_minor__, __clang_patchlevel__)
#else
# define nssv_COMPILER_CLANG_VERSION    0
#endif

#if defined(__GNUC__) && !defined(__clang__)
# define nssv_COMPILER_GNUC_VERSION  nssv_COMPILER_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
# define nssv_COMPILER_GNUC_VERSION    0
#endif

// half-open range [lo..hi):
#define nssv_BETWEEN( v, lo, hi ) ( (lo) <= (v) && (v) < (hi) )

// Presence of language and library features:

#ifdef _HAS_CPP0X
# define nssv_HAS_CPP0X  _HAS_CPP0X
#else
# define nssv_HAS_CPP0X  0
#endif

// Unless defined otherwise below, consider VC14 as C++11 for variant-lite:

#if nssv_COMPILER_MSVC_VER >= 1900
# undef  nssv_CPP11_OR_GREATER
# define nssv_CPP11_OR_GREATER  1
#endif

#define nssv_CPP11_90   (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1500)
#define nssv_CPP11_100  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1600)
#define nssv_CPP11_110  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1700)
#define nssv_CPP11_120  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1800)
#define nssv_CPP11_140  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1900)
#define nssv_CPP11_141  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1910)

#define nssv_CPP14_000  (nssv_CPP14_OR_GREATER)
#define nssv_CPP17_000  (nssv_CPP17_OR_GREATER)

// Presence of C++11 language features:

#define nssv_HAVE_CONSTEXPR_11          nssv_CPP11_140
#define nssv_HAVE_EXPLICIT_CONVERSION   nssv_CPP11_140
#define nssv_HAVE_INLINE_NAMESPACE      nssv_CPP11_140
#define nssv_HAVE_NOEXCEPT              nssv_CPP11_140
#define nssv_HAVE_NULLPTR               nssv_CPP11_100
#define nssv_HAVE_REF_QUALIFIER         nssv_CPP11_140
#define nssv_HAVE_UNICODE_LITERALS      nssv_CPP11_140
#define nssv_HAVE_USER_DEFINED_LITERALS nssv_CPP11_140
#define nssv_HAVE_WCHAR16_T             nssv_CPP11_100
#define nssv_HAVE_WCHAR32_T             nssv_CPP11_100

#if ! ( ( nssv_CPP11_OR_GREATER && nssv_COMPILER_CLANG_VERSION ) || nssv_BETWEEN( nssv_COMPILER_CLANG_VERSION, 300, 400 ) )
# define nssv_HAVE_STD_DEFINED_LITERALS  nssv_CPP11_140
#else
# define nssv_HAVE_STD_DEFINED_LITERALS  0
#endif

// Presence of C++14 language features:

#define nssv_HAVE_CONSTEXPR_14          nssv_CPP14_000

// Presence of C++17 language features:

#define nssv_HAVE_NODISCARD             nssv_CPP17_000

// Presence of C++ library features:

#define nssv_HAVE_STD_HASH              nssv_CPP11_120

// C++ feature usage:

#if nssv_HAVE_CONSTEXPR_11
# define nssv_constexpr  constexpr
#else
# define nssv_constexpr  /*constexpr*/
#endif

#if  nssv_HAVE_CONSTEXPR_14
# define nssv_constexpr14  constexpr
#else
# define nssv_constexpr14  /*constexpr*/
#endif

#if nssv_HAVE_EXPLICIT_CONVERSION
# define nssv_explicit  explicit
#else
# define nssv_explicit  /*explicit*/
#endif

#if nssv_HAVE_INLINE_NAMESPACE
# define nssv_inline_ns  inline
#else
# define nssv_inline_ns  /*inline*/
#endif

#if nssv_HAVE_NOEXCEPT
# define nssv_noexcept  noexcept
#else
# define nssv_noexcept  /*noexcept*/
#endif

//#if nssv_HAVE_REF_QUALIFIER
//# define nssv_ref_qual  &
//# define nssv_refref_qual  &&
//#else
//# define nssv_ref_qual  /*&*/
//# define nssv_refref_qual  /*&&*/
//#endif

#if nssv_HAVE_NULLPTR
# define nssv_nullptr  nullptr
#else
# define nssv_nullptr  NULL
#endif

#if nssv_HAVE_NODISCARD
# define nssv_nodiscard  [[nodiscard]]
#else
# define nssv_nodiscard  /*[[nodiscard]]*/
#endif

// Additional includes:

#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>
#include <ostream>
#include <string>   // std::char_traits<>

#if ! nssv_CONFIG_NO_EXCEPTIONS
# include <stdexcept>
#endif

#if nssv_CPP11_OR_GREATER
# include <type_traits>
#endif

// Clang, GNUC, MSVC warning suppression macros:

#if defined(__clang__)
# pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wuser-defined-literals"
#elif defined(__GNUC__)
# pragma  GCC  diagnostic push
# pragma  GCC  diagnostic ignored "-Wliteral-suffix"
#endif // __clang__

#if nssv_COMPILER_MSVC_VERSION >= 140
# define nssv_SUPPRESS_MSGSL_WARNING(expr)        [[gsl::suppress(expr)]]
# define nssv_SUPPRESS_MSVC_WARNING(code, descr)  __pragma(warning(suppress: code) )
# define nssv_DISABLE_MSVC_WARNINGS(codes)        __pragma(warning(push))  __pragma(warning(disable: codes))
#else
# define nssv_SUPPRESS_MSGSL_WARNING(expr)
# define nssv_SUPPRESS_MSVC_WARNING(code, descr)
# define nssv_DISABLE_MSVC_WARNINGS(codes)
#endif

#if defined(__clang__)
# define nssv_RESTORE_WARNINGS()  _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
# define nssv_RESTORE_WARNINGS()  _Pragma("GCC diagnostic pop")
#elif nssv_COMPILER_MSVC_VERSION >= 140
# define nssv_RESTORE_WARNINGS()  __pragma(warning(pop ))
#else
# define nssv_RESTORE_WARNINGS()
#endif

// Suppress the following MSVC (GSL) warnings:
// - C4455, non-gsl   : 'operator ""sv': literal suffix identifiers that do not
//                      start with an underscore are reserved
// - C26472, gsl::t.1 : don't use a static_cast for arithmetic conversions;
//                      use brace initialization, gsl::narrow_cast or gsl::narow
// - C26481: gsl::b.1 : don't use pointer arithmetic. Use span instead

nssv_DISABLE_MSVC_WARNINGS( 4455 26481 26472 )
//nssv_DISABLE_CLANG_WARNINGS( "-Wuser-defined-literals" )
//nssv_DISABLE_GNUC_WARNINGS( -Wliteral-suffix )

namespace nonstd { namespace sv_lite {

#if nssv_CPP11_OR_GREATER

namespace detail {

#if nssv_CPP14_OR_GREATER

template< typename CharT >
inline constexpr std::size_t length( CharT * s, std::size_t result = 0 )
{
    CharT * v = s;
    std::size_t r = result;
    while ( *v != '\0' ) {
       ++v;
       ++r;
    }
    return r;
}

#else // nssv_CPP14_OR_GREATER

// Expect tail call optimization to make length() non-recursive:

template< typename CharT >
inline constexpr std::size_t length( CharT * s, std::size_t result = 0 )
{
    return *s == '\0' ? result : length( s + 1, result + 1 );
}

#endif // nssv_CPP14_OR_GREATER

} // namespace detail

#endif // nssv_CPP11_OR_GREATER

template
<
    class CharT,
    class Traits = std::char_traits<CharT>
>
class basic_string_view;

//
// basic_string_view:
//

template
<
    class CharT,
    class Traits /* = std::char_traits<CharT> */
>
class basic_string_view
{
public:
    // Member types:

    typedef Traits traits_type;
    typedef CharT  value_type;

    typedef CharT       * pointer;
    typedef CharT const * const_pointer;
    typedef CharT       & reference;
    typedef CharT const & const_reference;

    typedef const_pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator< const_iterator > reverse_iterator;
    typedef	std::reverse_iterator< const_iterator > const_reverse_iterator;

    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  difference_type;

    // 24.4.2.1 Construction and assignment:

    nssv_constexpr basic_string_view() nssv_noexcept
        : data_( nssv_nullptr )
        , size_( 0 )
    {}

#if nssv_CPP11_OR_GREATER
    nssv_constexpr basic_string_view( basic_string_view const & other ) nssv_noexcept = default;
#else
    nssv_constexpr basic_string_view( basic_string_view const & other ) nssv_noexcept
        : data_( other.data_)
        , size_( other.size_)
    {}
#endif

    nssv_constexpr basic_string_view( CharT const * s, size_type count ) nssv_noexcept // non-standard noexcept
        : data_( s )
        , size_( count )
    {}

    nssv_constexpr basic_string_view( CharT const * s) nssv_noexcept // non-standard noexcept
        : data_( s )
#if nssv_CPP17_OR_GREATER
        , size_( Traits::length(s) )
#elif nssv_CPP11_OR_GREATER
        , size_( detail::length(s) )
#else
        , size_( Traits::length(s) )
#endif
    {}

    // Assignment:

#if nssv_CPP11_OR_GREATER
    nssv_constexpr14 basic_string_view & operator=( basic_string_view const & other ) nssv_noexcept = default;
#else
    nssv_constexpr14 basic_string_view & operator=( basic_string_view const & other ) nssv_noexcept
    {
        data_ = other.data_;
        size_ = other.size_;
        return *this;
    }
#endif

    // 24.4.2.2 Iterator support:

    nssv_constexpr const_iterator begin()  const nssv_noexcept { return data_;         }
    nssv_constexpr const_iterator end()    const nssv_noexcept { return data_ + size_; }

    nssv_constexpr const_iterator cbegin() const nssv_noexcept { return begin(); }
    nssv_constexpr const_iterator cend()   const nssv_noexcept { return end();   }

    nssv_constexpr const_reverse_iterator rbegin()  const nssv_noexcept { return const_reverse_iterator( end() );   }
    nssv_constexpr const_reverse_iterator rend()    const nssv_noexcept { return const_reverse_iterator( begin() ); }

    nssv_constexpr const_reverse_iterator crbegin() const nssv_noexcept { return rbegin(); }
    nssv_constexpr const_reverse_iterator crend()   const nssv_noexcept { return rend();   }

    // 24.4.2.3 Capacity:

    nssv_constexpr size_type size()     const nssv_noexcept { return size_; }
    nssv_constexpr size_type length()   const nssv_noexcept { return size_; }
    nssv_constexpr size_type max_size() const nssv_noexcept { return (std::numeric_limits< size_type >::max)(); }

    // since C++20
    nssv_nodiscard nssv_constexpr bool empty() const nssv_noexcept
    {
        return 0 == size_;
    }

    // 24.4.2.4 Element access:

    nssv_constexpr const_reference operator[]( size_type pos ) const
    {
        return data_at( pos );
    }

    nssv_constexpr14 const_reference at( size_type pos ) const
    {
#if nssv_CONFIG_NO_EXCEPTIONS
        assert( pos < size() );
#else
        if ( pos >= size() )
        {
            throw std::out_of_range("nonstd::string_view::at()");
        }
#endif
        return data_at( pos );
    }

    nssv_constexpr const_reference front() const { return data_at( 0 );          }
    nssv_constexpr const_reference back()  const { return data_at( size() - 1 ); }

    nssv_constexpr const_pointer   data()  const nssv_noexcept { return data_; }

    // 24.4.2.5 Modifiers:

    nssv_constexpr14 void remove_prefix( size_type n )
    {
        assert( n <= size() );
        data_ += n;
        size_ -= n;
    }

    nssv_constexpr14 void remove_suffix( size_type n )
    {
        assert( n <= size() );
        size_ -= n;
    }

    nssv_constexpr14 void swap( basic_string_view & other ) nssv_noexcept
    {
        using std::swap;
        swap( data_, other.data_ );
        swap( size_, other.size_ );
    }

    // 24.4.2.6 String operations:

    size_type copy( CharT * dest, size_type n, size_type pos = 0 ) const
    {
#if nssv_CONFIG_NO_EXCEPTIONS
        assert( pos <= size() );
#else
        if ( pos > size() )
        {
            throw std::out_of_range("nonstd::string_view::copy()");
        }
#endif
        const size_type rlen = (std::min)( n, size() - pos );

        (void) Traits::copy( dest, data() + pos, rlen );

        return rlen;
    }

    nssv_constexpr14 basic_string_view substr( size_type pos = 0, size_type n = npos ) const
    {
#if nssv_CONFIG_NO_EXCEPTIONS
        assert( pos <= size() );
#else
        if ( pos > size() )
        {
            throw std::out_of_range("nonstd::string_view::substr()");
        }
#endif
        return basic_string_view( data() + pos, (std::min)( n, size() - pos ) );
    }

    // compare(), 6x:

    nssv_constexpr14 int compare( basic_string_view other ) const nssv_noexcept // (1)
    {
        if ( const int result = Traits::compare( data(), other.data(), (std::min)( size(), other.size() ) ) )
        {
            return result;
        }

        return size() == other.size() ? 0 : size() < other.size() ? -1 : 1;
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, basic_string_view other ) const // (2)
    {
        return substr( pos1, n1 ).compare( other );
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, basic_string_view other, size_type pos2, size_type n2 ) const // (3)
    {
        return substr( pos1, n1 ).compare( other.substr( pos2, n2 ) );
    }

    nssv_constexpr int compare( CharT const * s ) const // (4)
    {
        return compare( basic_string_view( s ) );
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, CharT const * s ) const // (5)
    {
        return substr( pos1, n1 ).compare( basic_string_view( s ) );
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, CharT const * s, size_type n2 ) const // (6)
    {
        return substr( pos1, n1 ).compare( basic_string_view( s, n2 ) );
    }

    // 24.4.2.7 Searching:

    // starts_with(), 3x, since C++20:

    nssv_constexpr bool starts_with( basic_string_view v ) const nssv_noexcept  // (1)
    {
        return size() >= v.size() && compare( 0, v.size(), v ) == 0;
    }

    nssv_constexpr bool starts_with( CharT c ) const nssv_noexcept  // (2)
    {
        return starts_with( basic_string_view( &c, 1 ) );
    }

    nssv_constexpr bool starts_with( CharT const * s ) const  // (3)
    {
        return starts_with( basic_string_view( s ) );
    }

    // ends_with(), 3x, since C++20:

    nssv_constexpr bool ends_with( basic_string_view v ) const nssv_noexcept  // (1)
    {
        return size() >= v.size() && compare( size() - v.size(), npos, v ) == 0;
    }

    nssv_constexpr bool ends_with( CharT c ) const nssv_noexcept  // (2)
    {
        return ends_with( basic_string_view( &c, 1 ) );
    }

    nssv_constexpr bool ends_with( CharT const * s ) const  // (3)
    {
        return ends_with( basic_string_view( s ) );
    }

    // find(), 4x:

    nssv_constexpr14 size_type find( basic_string_view v, size_type pos = 0 ) const nssv_noexcept  // (1)
    {
        return assert( v.size() == 0 || v.data() != nssv_nullptr )
            , pos >= size()
            ? npos
            : to_pos( std::search( cbegin() + pos, cend(), v.cbegin(), v.cend(), Traits::eq ) );
    }

    nssv_constexpr14 size_type find( CharT c, size_type pos = 0 ) const nssv_noexcept  // (2)
    {
        return find( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr14 size_type find( CharT const * s, size_type pos, size_type n ) const  // (3)
    {
        return find( basic_string_view( s, n ), pos );
    }

    nssv_constexpr14 size_type find( CharT const * s, size_type pos = 0 ) const  // (4)
    {
        return find( basic_string_view( s ), pos );
    }

    // rfind(), 4x:

    nssv_constexpr14 size_type rfind( basic_string_view v, size_type pos = npos ) const nssv_noexcept  // (1)
    {
        if ( size() < v.size() )
        {
            return npos;
        }

        if ( v.empty() )
        {
            return (std::min)( size(), pos );
        }

        const_iterator last   = cbegin() + (std::min)( size() - v.size(), pos ) + v.size();
        const_iterator result = std::find_end( cbegin(), last, v.cbegin(), v.cend(), Traits::eq );

        return result != last ? size_type( result - cbegin() ) : npos;
    }

    nssv_constexpr14 size_type rfind( CharT c, size_type pos = npos ) const nssv_noexcept  // (2)
    {
        return rfind( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr14 size_type rfind( CharT const * s, size_type pos, size_type n ) const  // (3)
    {
        return rfind( basic_string_view( s, n ), pos );
    }

    nssv_constexpr14 size_type rfind( CharT const * s, size_type pos = npos ) const  // (4)
    {
        return rfind( basic_string_view( s ), pos );
    }

    // find_first_of(), 4x:

    nssv_constexpr size_type find_first_of( basic_string_view v, size_type pos = 0 ) const nssv_noexcept  // (1)
    {
        return pos >= size()
            ? npos
            : to_pos( std::find_first_of( cbegin() + pos, cend(), v.cbegin(), v.cend(), Traits::eq ) );
    }

    nssv_constexpr size_type find_first_of( CharT c, size_type pos = 0 ) const nssv_noexcept  // (2)
    {
        return find_first_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_first_of( CharT const * s, size_type pos, size_type n ) const  // (3)
    {
        return find_first_of( basic_string_view( s, n ), pos );
    }

    nssv_constexpr size_type find_first_of(  CharT const * s, size_type pos = 0 ) const  // (4)
    {
        return find_first_of( basic_string_view( s ), pos );
    }

    // find_last_of(), 4x:

    nssv_constexpr size_type find_last_of( basic_string_view v, size_type pos = npos ) const nssv_noexcept  // (1)
    {
        return empty()
            ? npos
            : pos >= size()
            ? find_last_of( v, size() - 1 )
            : to_pos( std::find_first_of( const_reverse_iterator( cbegin() + pos + 1 ), crend(), v.cbegin(), v.cend(), Traits::eq ) );
    }

    nssv_constexpr size_type find_last_of( CharT c, size_type pos = npos ) const nssv_noexcept  // (2)
    {
        return find_last_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_last_of( CharT const * s, size_type pos, size_type count ) const  // (3)
    {
        return find_last_of( basic_string_view( s, count ), pos );
    }

    nssv_constexpr size_type find_last_of( CharT const * s, size_type pos = npos ) const  // (4)
    {
        return find_last_of( basic_string_view( s ), pos );
    }

    // find_first_not_of(), 4x:

    nssv_constexpr size_type find_first_not_of( basic_string_view v, size_type pos = 0 ) const nssv_noexcept  // (1)
    {
        return pos >= size()
            ? npos
            : to_pos( std::find_if( cbegin() + pos, cend(), not_in_view( v ) ) );
    }

    nssv_constexpr size_type find_first_not_of( CharT c, size_type pos = 0 ) const nssv_noexcept  // (2)
    {
        return find_first_not_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_first_not_of( CharT const * s, size_type pos, size_type count ) const  // (3)
    {
        return find_first_not_of( basic_string_view( s, count ), pos );
    }

    nssv_constexpr size_type find_first_not_of( CharT const * s, size_type pos = 0 ) const  // (4)
    {
        return find_first_not_of( basic_string_view( s ), pos );
    }

    // find_last_not_of(), 4x:

    nssv_constexpr size_type find_last_not_of( basic_string_view v, size_type pos = npos ) const nssv_noexcept  // (1)
    {
        return empty()
            ? npos
            : pos >= size()
            ? find_last_not_of( v, size() - 1 )
            : to_pos( std::find_if( const_reverse_iterator( cbegin() + pos + 1 ), crend(), not_in_view( v ) ) );
    }

    nssv_constexpr size_type find_last_not_of( CharT c, size_type pos = npos ) const nssv_noexcept  // (2)
    {
        return find_last_not_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_last_not_of( CharT const * s, size_type pos, size_type count ) const  // (3)
    {
        return find_last_not_of( basic_string_view( s, count ), pos );
    }

    nssv_constexpr size_type find_last_not_of( CharT const * s, size_type pos = npos ) const  // (4)
    {
        return find_last_not_of( basic_string_view( s ), pos );
    }

    // Constants:

#if nssv_CPP17_OR_GREATER
    static nssv_constexpr size_type npos = size_type(-1);
#elif nssv_CPP11_OR_GREATER
    enum : size_type { npos = size_type(-1) };
#else
    enum { npos = size_type(-1) };
#endif

private:
    struct not_in_view
    {
        const basic_string_view v;

        nssv_constexpr explicit not_in_view( basic_string_view v ) : v( v ) {}

        nssv_constexpr bool operator()( CharT c ) const
        {
            return npos == v.find_first_of( c );
        }
    };

    nssv_constexpr size_type to_pos( const_iterator it ) const
    {
        return it == cend() ? npos : size_type( it - cbegin() );
    }

    nssv_constexpr size_type to_pos( const_reverse_iterator it ) const
    {
        return it == crend() ? npos : size_type( crend() - it - 1 );
    }

    nssv_constexpr const_reference data_at( size_type pos ) const
    {
#if nssv_BETWEEN( nssv_COMPILER_GNUC_VERSION, 1, 500 )
        return data_[pos];
#else
        return assert( pos < size() ), data_[pos];
#endif
    }

private:
    const_pointer data_;
    size_type     size_;

public:
#if nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS

    template< class Allocator >
    basic_string_view( std::basic_string<CharT, Traits, Allocator> const & s ) nssv_noexcept
        : data_( s.data() )
        , size_( s.size() )
    {}

#if nssv_HAVE_EXPLICIT_CONVERSION

    template< class Allocator >
    explicit operator std::basic_string<CharT, Traits, Allocator>() const
    {
        return to_string( Allocator() );
    }

#endif // nssv_HAVE_EXPLICIT_CONVERSION

#if nssv_CPP11_OR_GREATER

    template< class Allocator = std::allocator<CharT> >
    std::basic_string<CharT, Traits, Allocator>
    to_string( Allocator const & a = Allocator() ) const
    {
        return std::basic_string<CharT, Traits, Allocator>( begin(), end(), a );
    }

#else

    std::basic_string<CharT, Traits>
    to_string() const
    {
        return std::basic_string<CharT, Traits>( begin(), end() );
    }

    template< class Allocator >
    std::basic_string<CharT, Traits, Allocator>
    to_string( Allocator const & a ) const
    {
        return std::basic_string<CharT, Traits, Allocator>( begin(), end(), a );
    }

#endif // nssv_CPP11_OR_GREATER

#endif // nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS
};

//
// Non-member functions:
//

// 24.4.3 Non-member comparison functions:
// lexicographically compare two string views (function template):

template< class CharT, class Traits >
nssv_constexpr bool operator== (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) == 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator!= (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) != 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator< (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator<= (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator> (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator>= (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0 ; }

// Let S be basic_string_view<CharT, Traits>, and sv be an instance of S.
// Implementations shall provide sufficient additional overloads marked
// constexpr and noexcept so that an object t with an implicit conversion
// to S can be compared according to Table 67.

#if ! nssv_CPP11_OR_GREATER || nssv_BETWEEN( nssv_COMPILER_MSVC_VERSION, 100, 141 )

// accomodate for older compilers:

// ==

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) == 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) == 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.size() == rhs.size() && lhs.compare( rhs ) == 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return lhs.size() == rhs.size() && lhs.compare( rhs ) == 0; }

// !=

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) != 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) != 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.size() != rhs.size() && lhs.compare( rhs ) != 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return lhs.size() != rhs.size() || rhs.compare( lhs ) != 0; }

// <

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) > 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) > 0; }

// <=

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) >= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) >= 0; }

// >

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) < 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) < 0; }

// >=

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) <= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) <= 0; }

#else // newer compilers:

#define nssv_BASIC_STRING_VIEW_I(T,U)  typename std::decay< basic_string_view<T,U> >::type

#if nssv_BETWEEN( nssv_COMPILER_MSVC_VERSION, 140, 150 )
# define nssv_MSVC_ORDER(x)  , int=x
#else
# define nssv_MSVC_ORDER(x)  /*, int=x*/
#endif

// ==

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator==(
         basic_string_view  <CharT, Traits> lhs,
    nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) == 0; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator==(
    nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
         basic_string_view  <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.size() == rhs.size() && lhs.compare( rhs ) == 0; }

// !=

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator!= (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.size() != rhs.size() || lhs.compare( rhs ) != 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator!= (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) != 0 ; }

// <

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator< (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator< (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0 ; }

// <=

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator<= (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator<= (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0 ; }

// >

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator> (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator> (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0 ; }

// >=

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator>= (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator>= (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0 ; }

#undef nssv_MSVC_ORDER
#undef nssv_BASIC_STRING_VIEW_I

#endif // compiler-dependent approach to comparisons

// 24.4.4 Inserters and extractors:

namespace detail {

template< class Stream >
void write_padding( Stream & os, std::streamsize n )
{
    for ( std::streamsize i = 0; i < n; ++i )
        os.rdbuf()->sputc( os.fill() );
}

template< class Stream, class View >
Stream & write_to_stream( Stream & os, View const & sv )
{
    typename Stream::sentry sentry( os );

    if ( !os )
        return os;

    const std::streamsize length = static_cast<std::streamsize>( sv.length() );

    // Whether, and how, to pad:
    const bool      pad = ( length < os.width() );
    const bool left_pad = pad && ( os.flags() & std::ios_base::adjustfield ) == std::ios_base::right;

    if ( left_pad )
        write_padding( os, os.width() - length );

    // Write span characters:
    os.rdbuf()->sputn( sv.begin(), length );

    if ( pad && !left_pad )
        write_padding( os, os.width() - length );

    // Reset output stream width:
    os.width( 0 );

    return os;
}

} // namespace detail

template< class CharT, class Traits >
std::basic_ostream<CharT, Traits> &
operator<<(
    std::basic_ostream<CharT, Traits>& os,
    basic_string_view <CharT, Traits> sv )
{
    return detail::write_to_stream( os, sv );
}

// Several typedefs for common character types are provided:

typedef basic_string_view<char>      string_view;
typedef basic_string_view<wchar_t>   wstring_view;
#if nssv_HAVE_WCHAR16_T
typedef basic_string_view<char16_t>  u16string_view;
typedef basic_string_view<char32_t>  u32string_view;
#endif

}} // namespace nonstd::sv_lite

//
// 24.4.6 Suffix for basic_string_view literals:
//

#if nssv_HAVE_USER_DEFINED_LITERALS

namespace nonstd {
nssv_inline_ns namespace literals {
nssv_inline_ns namespace string_view_literals {

#if nssv_CONFIG_STD_SV_OPERATOR && nssv_HAVE_STD_DEFINED_LITERALS

nssv_constexpr nonstd::sv_lite::string_view operator "" sv( const char* str, size_t len ) nssv_noexcept  // (1)
{
    return nonstd::sv_lite::string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u16string_view operator "" sv( const char16_t* str, size_t len ) nssv_noexcept  // (2)
{
    return nonstd::sv_lite::u16string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u32string_view operator "" sv( const char32_t* str, size_t len ) nssv_noexcept  // (3)
{
    return nonstd::sv_lite::u32string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::wstring_view operator "" sv( const wchar_t* str, size_t len ) nssv_noexcept  // (4)
{
    return nonstd::sv_lite::wstring_view{ str, len };
}

#endif // nssv_CONFIG_STD_SV_OPERATOR && nssv_HAVE_STD_DEFINED_LITERALS

#if nssv_CONFIG_USR_SV_OPERATOR

nssv_constexpr nonstd::sv_lite::string_view operator "" _sv( const char* str, size_t len ) nssv_noexcept  // (1)
{
    return nonstd::sv_lite::string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u16string_view operator "" _sv( const char16_t* str, size_t len ) nssv_noexcept  // (2)
{
    return nonstd::sv_lite::u16string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u32string_view operator "" _sv( const char32_t* str, size_t len ) nssv_noexcept  // (3)
{
    return nonstd::sv_lite::u32string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::wstring_view operator "" _sv( const wchar_t* str, size_t len ) nssv_noexcept  // (4)
{
    return nonstd::sv_lite::wstring_view{ str, len };
}

#endif // nssv_CONFIG_USR_SV_OPERATOR

}}} // namespace nonstd::literals::string_view_literals

#endif

//
// Extensions for std::string:
//

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {
namespace sv_lite {

// Exclude MSVC 14 (19.00): it yields ambiguous to_string():

#if nssv_CPP11_OR_GREATER && nssv_COMPILER_MSVC_VERSION != 140

template< class CharT, class Traits, class Allocator = std::allocator<CharT> >
std::basic_string<CharT, Traits, Allocator>
to_string( basic_string_view<CharT, Traits> v, Allocator const & a = Allocator() )
{
    return std::basic_string<CharT,Traits, Allocator>( v.begin(), v.end(), a );
}

#else

template< class CharT, class Traits >
std::basic_string<CharT, Traits>
to_string( basic_string_view<CharT, Traits> v )
{
    return std::basic_string<CharT, Traits>( v.begin(), v.end() );
}

template< class CharT, class Traits, class Allocator >
std::basic_string<CharT, Traits, Allocator>
to_string( basic_string_view<CharT, Traits> v, Allocator const & a )
{
    return std::basic_string<CharT, Traits, Allocator>( v.begin(), v.end(), a );
}

#endif // nssv_CPP11_OR_GREATER

template< class CharT, class Traits, class Allocator >
basic_string_view<CharT, Traits>
to_string_view( std::basic_string<CharT, Traits, Allocator> const & s )
{
    return basic_string_view<CharT, Traits>( s.data(), s.size() );
}

}} // namespace nonstd::sv_lite

#endif // nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

//
// make types and algorithms available in namespace nonstd:
//

namespace nonstd {

using sv_lite::basic_string_view;
using sv_lite::string_view;
using sv_lite::wstring_view;

#if nssv_HAVE_WCHAR16_T
using sv_lite::u16string_view;
#endif
#if nssv_HAVE_WCHAR32_T
using sv_lite::u32string_view;
#endif

// literal "sv"

using sv_lite::operator==;
using sv_lite::operator!=;
using sv_lite::operator<;
using sv_lite::operator<=;
using sv_lite::operator>;
using sv_lite::operator>=;

using sv_lite::operator<<;

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS
using sv_lite::to_string;
using sv_lite::to_string_view;
#endif

} // namespace nonstd

// 24.4.5 Hash support (C++11):

// Note: The hash value of a string view object is equal to the hash value of
// the corresponding string object.

#if nssv_HAVE_STD_HASH

#include <functional>

namespace std {

template<>
struct hash< nonstd::string_view >
{
public:
    std::size_t operator()( nonstd::string_view v ) const nssv_noexcept
    {
        return std::hash<std::string>()( std::string( v.data(), v.size() ) );
    }
};

template<>
struct hash< nonstd::wstring_view >
{
public:
    std::size_t operator()( nonstd::wstring_view v ) const nssv_noexcept
    {
        return std::hash<std::wstring>()( std::wstring( v.data(), v.size() ) );
    }
};

template<>
struct hash< nonstd::u16string_view >
{
public:
    std::size_t operator()( nonstd::u16string_view v ) const nssv_noexcept
    {
        return std::hash<std::u16string>()( std::u16string( v.data(), v.size() ) );
    }
};

template<>
struct hash< nonstd::u32string_view >
{
public:
    std::size_t operator()( nonstd::u32string_view v ) const nssv_noexcept
    {
        return std::hash<std::u32string>()( std::u32string( v.data(), v.size() ) );
    }
};

} // namespace std

#endif // nssv_HAVE_STD_HASH

nssv_RESTORE_WARNINGS()

#endif // nssv_HAVE_STD_STRING_VIEW
#endif // NONSTD_SV_LITE_H_INCLUDED
/* end file include/simdjson/nonstd/string_view.hpp */
SIMDJSON_POP_DISABLE_WARNINGS

namespace std {
  using string_view = nonstd::string_view;
}
#endif // SIMDJSON_HAS_STRING_VIEW
#undef SIMDJSON_HAS_STRING_VIEW // We are not going to need this macro anymore.

/// If EXPR is an error, returns it.
#define SIMDJSON_TRY(EXPR) { auto _err = (EXPR); if (_err) { return _err; } }

#ifndef SIMDJSON_DEVELOPMENT_CHECKS
#ifndef NDEBUG
#define SIMDJSON_DEVELOPMENT_CHECKS
#endif
#endif



#if SIMDJSON_CPLUSPLUS17
// if we have C++, then fallthrough is a default attribute
# define simdjson_fallthrough [[fallthrough]]
// check if we have __attribute__ support
#elif defined(__has_attribute)
// check if we have the __fallthrough__ attribute
#if __has_attribute(__fallthrough__)
// we are good to go:
# define simdjson_fallthrough                    __attribute__((__fallthrough__))
#endif
#endif
// on some systems, we simply do not have support for fallthrough, so use a default:
#ifndef simdjson_fallthrough
# define simdjson_fallthrough do {} while (0)  /* fallthrough */
#endif


#endif // SIMDJSON_COMMON_DEFS_H
/* end file include/simdjson/common_defs.h */

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

// Public API
/* begin file include/simdjson/simdjson_version.h */
// /include/simdjson/simdjson_version.h automatically generated by release.py,
// do not change by hand
#ifndef SIMDJSON_SIMDJSON_VERSION_H
#define SIMDJSON_SIMDJSON_VERSION_H

/** The version of simdjson being used (major.minor.revision) */
#define SIMDJSON_VERSION 0.9.2

namespace simdjson {
enum {
  /**
   * The major version (MAJOR.minor.revision) of simdjson being used.
   */
  SIMDJSON_VERSION_MAJOR = 0,
  /**
   * The minor version (major.MINOR.revision) of simdjson being used.
   */
  SIMDJSON_VERSION_MINOR = 9,
  /**
   * The revision (major.minor.REVISION) of simdjson being used.
   */
  SIMDJSON_VERSION_REVISION = 2
};
} // namespace simdjson

#endif // SIMDJSON_SIMDJSON_VERSION_H
/* end file include/simdjson/simdjson_version.h */
/* begin file include/simdjson/error.h */
#ifndef SIMDJSON_ERROR_H
#define SIMDJSON_ERROR_H

#include <string>

namespace simdjson {

/**
 * All possible errors returned by simdjson.
 */
enum error_code {
  SUCCESS = 0,              ///< No error
  CAPACITY,                 ///< This parser can't support a document that big
  MEMALLOC,                 ///< Error allocating memory, most likely out of memory
  TAPE_ERROR,               ///< Something went wrong while writing to the tape (stage 2), this is a generic error
  DEPTH_ERROR,              ///< Your document exceeds the user-specified depth limitation
  STRING_ERROR,             ///< Problem while parsing a string
  T_ATOM_ERROR,             ///< Problem while parsing an atom starting with the letter 't'
  F_ATOM_ERROR,             ///< Problem while parsing an atom starting with the letter 'f'
  N_ATOM_ERROR,             ///< Problem while parsing an atom starting with the letter 'n'
  NUMBER_ERROR,             ///< Problem while parsing a number
  UTF8_ERROR,               ///< the input is not valid UTF-8
  UNINITIALIZED,            ///< unknown error, or uninitialized document
  EMPTY,                    ///< no structural element found
  UNESCAPED_CHARS,          ///< found unescaped characters in a string.
  UNCLOSED_STRING,          ///< missing quote at the end
  UNSUPPORTED_ARCHITECTURE, ///< unsupported architecture
  INCORRECT_TYPE,           ///< JSON element has a different type than user expected
  NUMBER_OUT_OF_RANGE,      ///< JSON number does not fit in 64 bits
  INDEX_OUT_OF_BOUNDS,      ///< JSON array index too large
  NO_SUCH_FIELD,            ///< JSON field not found in object
  IO_ERROR,                 ///< Error reading a file
  INVALID_JSON_POINTER,     ///< Invalid JSON pointer reference
  INVALID_URI_FRAGMENT,     ///< Invalid URI fragment
  UNEXPECTED_ERROR,         ///< indicative of a bug in simdjson
  PARSER_IN_USE,            ///< parser is already in use.
  OUT_OF_ORDER_ITERATION,   ///< tried to iterate an array or object out of order
  INSUFFICIENT_PADDING,     ///< The JSON doesn't have enough padding for simdjson to safely parse it.
  NUM_ERROR_CODES
};

/**
 * Get the error message for the given error code.
 *
 *   dom::parser parser;
 *   dom::element doc;
 *   auto error = parser.parse("foo",3).get(doc);
 *   if (error) { printf("Error: %s\n", error_message(error)); }
 *
 * @return The error message.
 */
inline const char *error_message(error_code error) noexcept;

/**
 * Write the error message to the output stream
 */
inline std::ostream& operator<<(std::ostream& out, error_code error) noexcept;

/**
 * Exception thrown when an exception-supporting simdjson method is called
 */
struct simdjson_error : public std::exception {
  /**
   * Create an exception from a simdjson error code.
   * @param error The error code
   */
  simdjson_error(error_code error) noexcept : _error{error} { }
  /** The error message */
  const char *what() const noexcept { return error_message(error()); }
  /** The error code */
  error_code error() const noexcept { return _error; }
private:
  /** The error code that was used */
  error_code _error;
};

namespace internal {

/**
 * The result of a simdjson operation that could fail.
 *
 * Gives the option of reading error codes, or throwing an exception by casting to the desired result.
 *
 * This is a base class for implementations that want to add functions to the result type for
 * chaining.
 *
 * Override like:
 *
 *   struct simdjson_result<T> : public internal::simdjson_result_base<T> {
 *     simdjson_result() noexcept : internal::simdjson_result_base<T>() {}
 *     simdjson_result(error_code error) noexcept : internal::simdjson_result_base<T>(error) {}
 *     simdjson_result(T &&value) noexcept : internal::simdjson_result_base<T>(std::forward(value)) {}
 *     simdjson_result(T &&value, error_code error) noexcept : internal::simdjson_result_base<T>(value, error) {}
 *     // Your extra methods here
 *   }
 *
 * Then any method returning simdjson_result<T> will be chainable with your methods.
 */
template<typename T>
struct simdjson_result_base : protected std::pair<T, error_code> {

  /**
   * Create a new empty result with error = UNINITIALIZED.
   */
  simdjson_really_inline simdjson_result_base() noexcept;

  /**
   * Create a new error result.
   */
  simdjson_really_inline simdjson_result_base(error_code error) noexcept;

  /**
   * Create a new successful result.
   */
  simdjson_really_inline simdjson_result_base(T &&value) noexcept;

  /**
   * Create a new result with both things (use if you don't want to branch when creating the result).
   */
  simdjson_really_inline simdjson_result_base(T &&value, error_code error) noexcept;

  /**
   * Move the value and the error to the provided variables.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   * @param error The variable to assign the error to. Set to SUCCESS if there is no error.
   */
  simdjson_really_inline void tie(T &value, error_code &error) && noexcept;

  /**
   * Move the value to the provided variable.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   */
  simdjson_really_inline error_code get(T &value) && noexcept;

  /**
   * Move the value to the provided variable.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   */
  simdjson_really_inline const T &value(error_code &error) const & noexcept;

  /**
   * The error.
   */
  simdjson_really_inline error_code error() const noexcept;

#if SIMDJSON_EXCEPTIONS

  /**
   * Get the result value.
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T& value() & noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T&& value() && noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T&& take_value() && noexcept(false);

  /**
   * Cast to the value (will throw on error).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline operator T&&() && noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  /**
   * Get the result value. This function is safe if and only
   * the error() method returns a value that evoluates to false.
   */
  simdjson_really_inline const T& value_unsafe() const& noexcept;

  /**
   * Take the result value (move it). This function is safe if and only
   * the error() method returns a value that evoluates to false.
   */
  simdjson_really_inline T&& value_unsafe() && noexcept;

}; // struct simdjson_result_base

} // namespace internal

/**
 * The result of a simdjson operation that could fail.
 *
 * Gives the option of reading error codes, or throwing an exception by casting to the desired result.
 */
template<typename T>
struct simdjson_result : public internal::simdjson_result_base<T> {
  /**
   * @private Create a new empty result with error = UNINITIALIZED.
   */
  simdjson_really_inline simdjson_result() noexcept;
  /**
   * @private Create a new error result.
   */
  simdjson_really_inline simdjson_result(T &&value) noexcept;
  /**
   * @private Create a new successful result.
   */
  simdjson_really_inline simdjson_result(error_code error_code) noexcept;
  /**
   * @private Create a new result with both things (use if you don't want to branch when creating the result).
   */
  simdjson_really_inline simdjson_result(T &&value, error_code error) noexcept;

  /**
   * Move the value and the error to the provided variables.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   * @param error The variable to assign the error to. Set to SUCCESS if there is no error.
   */
  simdjson_really_inline void tie(T &value, error_code &error) && noexcept;

  /**
   * Move the value to the provided variable.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   */
  simdjson_warn_unused simdjson_really_inline error_code get(T &value) && noexcept;

  /**
   * The error.
   */
  simdjson_really_inline error_code error() const noexcept;

#if SIMDJSON_EXCEPTIONS

  /**
   * Get the result value.
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T& value() & noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T&& value() && noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T&& take_value() && noexcept(false);

  /**
   * Cast to the value (will throw on error).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline operator T&&() && noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  /**
   * Get the result value. This function is safe if and only
   * the error() method returns a value that evoluates to false.
   */
  simdjson_really_inline const T& value_unsafe() const& noexcept;

  /**
   * Take the result value (move it). This function is safe if and only
   * the error() method returns a value that evoluates to false.
   */
  simdjson_really_inline T&& value_unsafe() && noexcept;

}; // struct simdjson_result

#if SIMDJSON_EXCEPTIONS

template<typename T>
inline std::ostream& operator<<(std::ostream& out, simdjson_result<T> value) noexcept { return out << value.value(); }

#endif // SIMDJSON_EXCEPTIONS

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
/**
 * @deprecated This is an alias and will be removed, use error_code instead
 */
using ErrorValues [[deprecated("This is an alias and will be removed, use error_code instead")]] = error_code;

/**
 * @deprecated Error codes should be stored and returned as `error_code`, use `error_message()` instead.
 */
[[deprecated("Error codes should be stored and returned as `error_code`, use `error_message()` instead.")]]
inline const std::string error_message(int error) noexcept;
#endif // SIMDJSON_DISABLE_DEPRECATED_API
} // namespace simdjson

#endif // SIMDJSON_ERROR_H
/* end file include/simdjson/error.h */
/* begin file include/simdjson/minify.h */
#ifndef SIMDJSON_MINIFY_H
#define SIMDJSON_MINIFY_H

/* begin file include/simdjson/padded_string.h */
#ifndef SIMDJSON_PADDED_STRING_H
#define SIMDJSON_PADDED_STRING_H

#include <cstring>
#include <memory>
#include <string>
#include <ostream>

namespace simdjson {

class padded_string_view;

/**
 * String with extra allocation for ease of use with parser::parse()
 *
 * This is a move-only class, it cannot be copied.
 */
struct padded_string final {

  /**
   * Create a new, empty padded string.
   */
  explicit inline padded_string() noexcept;
  /**
   * Create a new padded string buffer.
   *
   * @param length the size of the string.
   */
  explicit inline padded_string(size_t length) noexcept;
  /**
   * Create a new padded string by copying the given input.
   *
   * @param data the buffer to copy
   * @param length the number of bytes to copy
   */
  explicit inline padded_string(const char *data, size_t length) noexcept;
  /**
   * Create a new padded string by copying the given input.
   *
   * @param str_ the string to copy
   */
  inline padded_string(const std::string & str_ ) noexcept;
  /**
   * Create a new padded string by copying the given input.
   *
   * @param sv_ the string to copy
   */
  inline padded_string(std::string_view sv_) noexcept;
  /**
   * Move one padded string into another.
   *
   * The original padded string will be reduced to zero capacity.
   *
   * @param o the string to move.
   */
  inline padded_string(padded_string &&o) noexcept;
  /**
   * Move one padded string into another.
   *
   * The original padded string will be reduced to zero capacity.
   *
   * @param o the string to move.
   */
  inline padded_string &operator=(padded_string &&o) noexcept;
  inline void swap(padded_string &o) noexcept;
  ~padded_string() noexcept;

  /**
   * The length of the string.
   *
   * Does not include padding.
   */
  size_t size() const noexcept;

  /**
   * The length of the string.
   *
   * Does not include padding.
   */
  size_t length() const noexcept;

  /**
   * The string data.
   **/
  const char *data() const noexcept;
  const uint8_t *u8data() const noexcept { return static_cast<const uint8_t*>(static_cast<const void*>(data_ptr));}

  /**
   * The string data.
   **/
  char *data() noexcept;

  /**
   * Create a std::string_view with the same content.
   */
  operator std::string_view() const;

  /**
   * Create a padded_string_view with the same content.
   */
  operator padded_string_view() const noexcept;

  /**
   * Load this padded string from a file.
   *
   * @return IO_ERROR on error. Be mindful that on some 32-bit systems,
   * the file size might be limited to 2 GB.
   *
   * @param path the path to the file.
   **/
  inline static simdjson_result<padded_string> load(const std::string &path) noexcept;

private:
  padded_string &operator=(const padded_string &o) = delete;
  padded_string(const padded_string &o) = delete;

  size_t viable_size{0};
  char *data_ptr{nullptr};

}; // padded_string

/**
 * Send padded_string instance to an output stream.
 *
 * @param out The output stream.
 * @param s The padded_string instance.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, const padded_string& s) { return out << s.data(); }

#if SIMDJSON_EXCEPTIONS
/**
 * Send padded_string instance to an output stream.
 *
 * @param out The output stream.
 * @param s The padded_string instance.
  * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, simdjson_result<padded_string> &s) noexcept(false) { return out << s.value(); }
#endif

} // namespace simdjson

// This is deliberately outside of simdjson so that people get it without having to use the namespace
inline simdjson::padded_string operator "" _padded(const char *str, size_t len) {
  return simdjson::padded_string(str, len);
}

namespace simdjson {
namespace internal {

// The allocate_padded_buffer function is a low-level function to allocate memory
// with padding so we can read past the "length" bytes safely. It is used by
// the padded_string class automatically. It returns nullptr in case
// of error: the caller should check for a null pointer.
// The length parameter is the maximum size in bytes of the string.
// The caller is responsible to free the memory (e.g., delete[] (...)).
inline char *allocate_padded_buffer(size_t length) noexcept;

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_H
/* end file include/simdjson/padded_string.h */
#include <string>
#include <ostream>
#include <sstream>

namespace simdjson {



/**
 *
 * Minify the input string assuming that it represents a JSON string, does not parse or validate.
 * This function is much faster than parsing a JSON string and then writing a minified version of it.
 * However, it does not validate the input. It will merely return an error in simple cases (e.g., if
 * there is a string that was never terminated).
 *
 *
 * @param buf the json document to minify.
 * @param len the length of the json document.
 * @param dst the buffer to write the minified document to. *MUST* be allocated up to len bytes.
 * @param dst_len the number of bytes written. Output only.
 * @return the error code, or SUCCESS if there was no error.
 */
simdjson_warn_unused error_code minify(const char *buf, size_t len, char *dst, size_t &dst_len) noexcept;

} // namespace simdjson

#endif // SIMDJSON_MINIFY_H
/* end file include/simdjson/minify.h */
/* begin file include/simdjson/padded_string_view.h */
#ifndef SIMDJSON_PADDED_STRING_VIEW_H
#define SIMDJSON_PADDED_STRING_VIEW_H


#include <cstring>
#include <memory>
#include <string>
#include <ostream>

namespace simdjson {

/**
 * User-provided string that promises it has extra padded bytes at the end for use with parser::parse().
 */
class padded_string_view : public std::string_view {
private:
  size_t _capacity;

public:
  /** Create an empty padded_string_view. */
  inline padded_string_view() noexcept = default;

  /**
   * Promise the given buffer has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * @param s The string.
   * @param len The length of the string (not including padding).
   * @param capacity The allocated length of the string, including padding.
   */
  explicit inline padded_string_view(const char* s, size_t len, size_t capacity) noexcept;
  /** overload explicit inline padded_string_view(const char* s, size_t len) noexcept */
  explicit inline padded_string_view(const uint8_t* s, size_t len, size_t capacity) noexcept;

  /**
   * Promise the given string has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * The capacity of the string will be used to determine its padding.
   *
   * @param s The string.
   */
  explicit inline padded_string_view(const std::string &s) noexcept;

  /**
   * Promise the given string_view has at least SIMDJSON_PADDING extra bytes allocated to it.
   *
   * @param s The string.
   * @param capacity The allocated length of the string, including padding.
   */
  explicit inline padded_string_view(std::string_view s, size_t capacity) noexcept;

  /** The number of allocated bytes. */
  inline size_t capacity() const noexcept;

  /** The amount of padding on the string (capacity() - length()) */
  inline size_t padding() const noexcept;

}; // padded_string_view

#if SIMDJSON_EXCEPTIONS
/**
 * Send padded_string instance to an output stream.
 *
 * @param out The output stream.
 * @param s The padded_string_view.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, simdjson_result<padded_string_view> &s) noexcept(false) { return out << s.value(); }
#endif

} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_VIEW_H
/* end file include/simdjson/padded_string_view.h */
/* begin file include/simdjson/implementation.h */
#ifndef SIMDJSON_IMPLEMENTATION_H
#define SIMDJSON_IMPLEMENTATION_H

/* begin file include/simdjson/internal/dom_parser_implementation.h */
#ifndef SIMDJSON_INTERNAL_DOM_PARSER_IMPLEMENTATION_H
#define SIMDJSON_INTERNAL_DOM_PARSER_IMPLEMENTATION_H

#include <memory>

namespace simdjson {

namespace dom {
class document;
} // namespace dom

namespace internal {

/**
 * An implementation of simdjson's DOM parser for a particular CPU architecture.
 *
 * This class is expected to be accessed only by pointer, and never move in memory (though the
 * pointer can move).
 */
class dom_parser_implementation {
public:

  /**
   * @private For internal implementation use
   *
   * Run a full JSON parse on a single document (stage1 + stage2).
   *
   * Guaranteed only to be called when capacity > document length.
   *
   * Overridden by each implementation.
   *
   * @param buf The json document to parse. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param len The length of the json document.
   * @return The error code, or SUCCESS if there was no error.
   */
  simdjson_warn_unused virtual error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 1 of the document parser.
   *
   * Guaranteed only to be called when capacity > document length.
   *
   * Overridden by each implementation.
   *
   * @param buf The json document to parse.
   * @param len The length of the json document.
   * @param streaming Whether this is being called by parser::parse_many.
   * @return The error code, or SUCCESS if there was no error.
   */
  simdjson_warn_unused virtual error_code stage1(const uint8_t *buf, size_t len, bool streaming) noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 2 of the document parser.
   *
   * Called after stage1().
   *
   * Overridden by each implementation.
   *
   * @param doc The document to output to.
   * @return The error code, or SUCCESS if there was no error.
   */
  simdjson_warn_unused virtual error_code stage2(dom::document &doc) noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Stage 2 of the document parser for parser::parse_many.
   *
   * Guaranteed only to be called after stage1().
   * Overridden by each implementation.
   *
   * @param doc The document to output to.
   * @return The error code, SUCCESS if there was no error, or EMPTY if all documents have been parsed.
   */
  simdjson_warn_unused virtual error_code stage2_next(dom::document &doc) noexcept = 0;

  /**
   * Change the capacity of this parser.
   *
   * The capacity can never exceed SIMDJSON_MAXSIZE_BYTES (e.g., 4 GB)
   * and an CAPACITY error is returned if it is attempted.
   *
   * Generally used for reallocation.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth.
   * @return The error code, or SUCCESS if there was no error.
   */
  virtual error_code set_capacity(size_t capacity) noexcept = 0;

  /**
   * Change the max depth of this parser.
   *
   * Generally used for reallocation.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth.
   * @return The error code, or SUCCESS if there was no error.
   */
  virtual error_code set_max_depth(size_t max_depth) noexcept = 0;

  /**
   * Deallocate this parser.
   */
  virtual ~dom_parser_implementation() = default;

  /** Number of structural indices passed from stage 1 to stage 2 */
  uint32_t n_structural_indexes{0};
  /** Structural indices passed from stage 1 to stage 2 */
  std::unique_ptr<uint32_t[]> structural_indexes{};
  /** Next structural index to parse */
  uint32_t next_structural_index{0};

  /**
   * The largest document this parser can support without reallocating.
   *
   * @return Current capacity, in bytes.
   */
  simdjson_really_inline size_t capacity() const noexcept;

  /**
   * The maximum level of nested object and arrays supported by this parser.
   *
   * @return Maximum depth, in bytes.
   */
  simdjson_really_inline size_t max_depth() const noexcept;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  simdjson_warn_unused inline error_code allocate(size_t capacity, size_t max_depth) noexcept;

protected:
  /**
   * The maximum document length this parser supports.
   *
   * Buffers are large enough to handle any document up to this length.
   */
  size_t _capacity{0};

  /**
   * The maximum depth (number of nested objects and arrays) supported by this parser.
   *
   * Defaults to DEFAULT_MAX_DEPTH.
   */
  size_t _max_depth{0};

  // Declaring these so that subclasses can use them to implement their constructors.
  simdjson_really_inline dom_parser_implementation() noexcept;
  simdjson_really_inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  simdjson_really_inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;

  simdjson_really_inline dom_parser_implementation(const dom_parser_implementation &) noexcept = delete;
  simdjson_really_inline dom_parser_implementation &operator=(const dom_parser_implementation &other) noexcept = delete;
}; // class dom_parser_implementation

simdjson_really_inline dom_parser_implementation::dom_parser_implementation() noexcept = default;
simdjson_really_inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
simdjson_really_inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

simdjson_really_inline size_t dom_parser_implementation::capacity() const noexcept {
  return _capacity;
}

simdjson_really_inline size_t dom_parser_implementation::max_depth() const noexcept {
  return _max_depth;
}

simdjson_warn_unused
inline error_code dom_parser_implementation::allocate(size_t capacity, size_t max_depth) noexcept {
  if (this->max_depth() != max_depth) {
    error_code err = set_max_depth(max_depth);
    if (err) { return err; }
  }
  if (_capacity != capacity) {
    error_code err = set_capacity(capacity);
    if (err) { return err; }
  }
  return SUCCESS;
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_DOM_PARSER_IMPLEMENTATION_H
/* end file include/simdjson/internal/dom_parser_implementation.h */
/* begin file include/simdjson/internal/isadetection.h */
/* From
https://github.com/endorno/pytorch/blob/master/torch/lib/TH/generic/simd/simd.h
Highly modified.

Copyright (c) 2016-     Facebook, Inc            (Adam Paszke)
Copyright (c) 2014-     Facebook, Inc            (Soumith Chintala)
Copyright (c) 2011-2014 Idiap Research Institute (Ronan Collobert)
Copyright (c) 2012-2014 Deepmind Technologies    (Koray Kavukcuoglu)
Copyright (c) 2011-2012 NEC Laboratories America (Koray Kavukcuoglu)
Copyright (c) 2011-2013 NYU                      (Clement Farabet)
Copyright (c) 2006-2010 NEC Laboratories America (Ronan Collobert, Leon Bottou,
Iain Melvin, Jason Weston) Copyright (c) 2006      Idiap Research Institute
(Samy Bengio) Copyright (c) 2001-2004 Idiap Research Institute (Ronan Collobert,
Samy Bengio, Johnny Mariethoz)

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the names of Facebook, Deepmind Technologies, NYU, NEC Laboratories
America and IDIAP Research Institute nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SIMDJSON_INTERNAL_ISADETECTION_H
#define SIMDJSON_INTERNAL_ISADETECTION_H

#include <cstdint>
#include <cstdlib>
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)
#include <cpuid.h>
#endif

namespace simdjson {
namespace internal {


enum instruction_set {
  DEFAULT = 0x0,
  NEON = 0x1,
  AVX2 = 0x4,
  SSE42 = 0x8,
  PCLMULQDQ = 0x10,
  BMI1 = 0x20,
  BMI2 = 0x40,
  ALTIVEC = 0x80
};

#if defined(__PPC64__)

static inline uint32_t detect_supported_architectures() {
  return instruction_set::ALTIVEC;
}

#elif defined(__arm__) || defined(__aarch64__) // incl. armel, armhf, arm64

#if defined(__ARM_NEON)

static inline uint32_t detect_supported_architectures() {
  return instruction_set::NEON;
}

#else // ARM without NEON

static inline uint32_t detect_supported_architectures() {
  return instruction_set::DEFAULT;
}

#endif

#elif defined(__x86_64__) || defined(_M_AMD64) // x64


namespace {
// Can be found on Intel ISA Reference for CPUID
constexpr uint32_t cpuid_avx2_bit = 1 << 5;      ///< @private Bit 5 of EBX for EAX=0x7
constexpr uint32_t cpuid_bmi1_bit = 1 << 3;      ///< @private bit 3 of EBX for EAX=0x7
constexpr uint32_t cpuid_bmi2_bit = 1 << 8;      ///< @private bit 8 of EBX for EAX=0x7
constexpr uint32_t cpuid_sse42_bit = 1 << 20;    ///< @private bit 20 of ECX for EAX=0x1
constexpr uint32_t cpuid_pclmulqdq_bit = 1 << 1; ///< @private bit  1 of ECX for EAX=0x1
}



static inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                         uint32_t *edx) {
#if defined(_MSC_VER)
  int cpu_info[4];
  __cpuid(cpu_info, *eax);
  *eax = cpu_info[0];
  *ebx = cpu_info[1];
  *ecx = cpu_info[2];
  *edx = cpu_info[3];
#elif defined(HAVE_GCC_GET_CPUID) && defined(USE_GCC_GET_CPUID)
  uint32_t level = *eax;
  __get_cpuid(level, eax, ebx, ecx, edx);
#else
  uint32_t a = *eax, b, c = *ecx, d;
  asm volatile("cpuid\n\t" : "+a"(a), "=b"(b), "+c"(c), "=d"(d));
  *eax = a;
  *ebx = b;
  *ecx = c;
  *edx = d;
#endif
}

static inline uint32_t detect_supported_architectures() {
  uint32_t eax, ebx, ecx, edx;
  uint32_t host_isa = 0x0;

  // ECX for EAX=0x7
  eax = 0x7;
  ecx = 0x0;
  cpuid(&eax, &ebx, &ecx, &edx);
  if (ebx & cpuid_avx2_bit) {
    host_isa |= instruction_set::AVX2;
  }
  if (ebx & cpuid_bmi1_bit) {
    host_isa |= instruction_set::BMI1;
  }

  if (ebx & cpuid_bmi2_bit) {
    host_isa |= instruction_set::BMI2;
  }

  // EBX for EAX=0x1
  eax = 0x1;
  cpuid(&eax, &ebx, &ecx, &edx);

  if (ecx & cpuid_sse42_bit) {
    host_isa |= instruction_set::SSE42;
  }

  if (ecx & cpuid_pclmulqdq_bit) {
    host_isa |= instruction_set::PCLMULQDQ;
  }

  return host_isa;
}
#else // fallback


static inline uint32_t detect_supported_architectures() {
  return instruction_set::DEFAULT;
}


#endif // end SIMD extension detection code

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_ISADETECTION_H
/* end file include/simdjson/internal/isadetection.h */
#include <string>
#include <atomic>
#include <vector>

namespace simdjson {

/**
 * Validate the UTF-8 string.
 *
 * @param buf the string to validate.
 * @param len the length of the string in bytes.
 * @return true if the string is valid UTF-8.
 */
simdjson_warn_unused bool validate_utf8(const char * buf, size_t len) noexcept;


/**
 * Validate the UTF-8 string.
 *
 * @param sv the string_view to validate.
 * @return true if the string is valid UTF-8.
 */
simdjson_really_inline simdjson_warn_unused bool validate_utf8(const std::string_view sv) noexcept {
  return validate_utf8(sv.data(), sv.size());
}

/**
 * Validate the UTF-8 string.
 *
 * @param p the string to validate.
 * @return true if the string is valid UTF-8.
 */
simdjson_really_inline simdjson_warn_unused bool validate_utf8(const std::string& s) noexcept {
  return validate_utf8(s.data(), s.size());
}

namespace dom {
  class document;
} // namespace dom

/**
 * An implementation of simdjson for a particular CPU architecture.
 *
 * Also used to maintain the currently active implementation. The active implementation is
 * automatically initialized on first use to the most advanced implementation supported by the host.
 */
class implementation {
public:

  /**
   * The name of this implementation.
   *
   *     const implementation *impl = simdjson::active_implementation;
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &name() const { return _name; }

  /**
   * The description of this implementation.
   *
   *     const implementation *impl = simdjson::active_implementation;
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &description() const { return _description; }

  /**
   * The instruction sets this implementation is compiled against
   * and the current CPU match. This function may poll the current CPU/system
   * and should therefore not be called too often if performance is a concern.
   *
   *
   * @return true if the implementation can be safely used on the current system (determined at runtime)
   */
  bool supported_by_runtime_system() const;

  /**
   * @private For internal implementation use
   *
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `internal::instruction_set::` values
   */
  virtual uint32_t required_instruction_sets() const { return _required_instruction_sets; };

  /**
   * @private For internal implementation use
   *
   *     const implementation *impl = simdjson::active_implementation;
   *     cout << "simdjson is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @param capacity The largest document that will be passed to the parser.
   * @param max_depth The maximum JSON object/array nesting this parser is expected to handle.
   * @param dst The place to put the resulting parser implementation.
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_depth,
    std::unique_ptr<internal::dom_parser_implementation> &dst
  ) const noexcept = 0;

  /**
   * @private For internal implementation use
   *
   * Minify the input string assuming that it represents a JSON string, does not parse or validate.
   *
   * Overridden by each implementation.
   *
   * @param buf the json document to minify.
   * @param len the length of the json document.
   * @param dst the buffer to write the minified document to. *MUST* be allocated up to len + SIMDJSON_PADDING bytes.
   * @param dst_len the number of bytes written. Output only.
   * @return the error code, or SUCCESS if there was no error.
   */
  simdjson_warn_unused virtual error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept = 0;


  /**
   * Validate the UTF-8 string.
   *
   * Overridden by each implementation.
   *
   * @param buf the string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid UTF-8.
   */
  simdjson_warn_unused virtual bool validate_utf8(const char *buf, size_t len) const noexcept = 0;

protected:
  /** @private Construct an implementation with the given name and description. For subclasses. */
  simdjson_really_inline implementation(
    std::string_view name,
    std::string_view description,
    uint32_t required_instruction_sets
  ) :
    _name(name),
    _description(description),
    _required_instruction_sets(required_instruction_sets)
  {
  }
  virtual ~implementation()=default;

private:
  /**
   * The name of this implementation.
   */
  const std::string _name;

  /**
   * The description of this implementation.
   */
  const std::string _description;

  /**
   * Instruction sets required for this implementation.
   */
  const uint32_t _required_instruction_sets;
};

/** @private */
namespace internal {

/**
 * The list of available implementations compiled into simdjson.
 */
class available_implementation_list {
public:
  /** Get the list of available implementations compiled into simdjson */
  simdjson_really_inline available_implementation_list() {}
  /** Number of implementations */
  size_t size() const noexcept;
  /** STL const begin() iterator */
  const implementation * const *begin() const noexcept;
  /** STL const end() iterator */
  const implementation * const *end() const noexcept;

  /**
   * Get the implementation with the given name.
   *
   * Case sensitive.
   *
   *     const implementation *impl = simdjson::available_implementations["westmere"];
   *     if (!impl) { exit(1); }
   *     if (!imp->supported_by_runtime_system()) { exit(1); }
   *     simdjson::active_implementation = impl;
   *
   * @param name the implementation to find, e.g. "westmere", "haswell", "arm64"
   * @return the implementation, or nullptr if the parse failed.
   */
  const implementation * operator[](const std::string_view &name) const noexcept {
    for (const implementation * impl : *this) {
      if (impl->name() == name) { return impl; }
    }
    return nullptr;
  }

  /**
   * Detect the most advanced implementation supported by the current host.
   *
   * This is used to initialize the implementation on startup.
   *
   *     const implementation *impl = simdjson::available_implementation::detect_best_supported();
   *     simdjson::active_implementation = impl;
   *
   * @return the most advanced supported implementation for the current host, or an
   *         implementation that returns UNSUPPORTED_ARCHITECTURE if there is no supported
   *         implementation. Will never return nullptr.
   */
  const implementation *detect_best_supported() const noexcept;
};

template<typename T>
class atomic_ptr {
public:
  atomic_ptr(T *_ptr) : ptr{_ptr} {}

  operator const T*() const { return ptr.load(); }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr.load(); }

  operator T*() { return ptr.load(); }
  T& operator*() { return *ptr; }
  T* operator->() { return ptr.load(); }
  atomic_ptr& operator=(T *_ptr) { ptr = _ptr; return *this; }

private:
  std::atomic<T*> ptr;
};

} // namespace internal

/**
 * The list of available implementations compiled into simdjson.
 */
extern SIMDJSON_DLLIMPORTEXPORT const internal::available_implementation_list available_implementations;

/**
  * The active implementation.
  *
  * Automatically initialized on first use to the most advanced implementation supported by this hardware.
  */
extern SIMDJSON_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> active_implementation;

} // namespace simdjson

#endif // SIMDJSON_IMPLEMENTATION_H
/* end file include/simdjson/implementation.h */

// Inline functions
/* begin file include/simdjson/error-inl.h */
#ifndef SIMDJSON_INLINE_ERROR_H
#define SIMDJSON_INLINE_ERROR_H

#include <cstring>
#include <string>
#include <utility>

namespace simdjson {
namespace internal {
  // We store the error code so we can validate the error message is associated with the right code
  struct error_code_info {
    error_code code;
    const char* message; // do not use a fancy std::string where a simple C string will do (no alloc, no destructor)
  };
  // These MUST match the codes in error_code. We check this constraint in basictests.
  extern SIMDJSON_DLLIMPORTEXPORT const error_code_info error_codes[];
} // namespace internal


inline const char *error_message(error_code error) noexcept {
  // If you're using error_code, we're trusting you got it from the enum.
  return internal::error_codes[int(error)].message;
}

// deprecated function
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
inline const std::string error_message(int error) noexcept {
  if (error < 0 || error >= error_code::NUM_ERROR_CODES) {
    return internal::error_codes[UNEXPECTED_ERROR].message;
  }
  return internal::error_codes[error].message;
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline std::ostream& operator<<(std::ostream& out, error_code error) noexcept {
  return out << error_message(error);
}

namespace internal {

//
// internal::simdjson_result_base<T> inline implementation
//

template<typename T>
simdjson_really_inline void simdjson_result_base<T>::tie(T &value, error_code &error) && noexcept {
  error = this->second;
  if (!error) {
    value = std::forward<simdjson_result_base<T>>(*this).first;
  }
}

template<typename T>
simdjson_warn_unused simdjson_really_inline error_code simdjson_result_base<T>::get(T &value) && noexcept {
  error_code error;
  std::forward<simdjson_result_base<T>>(*this).tie(value, error);
  return error;
}

template<typename T>
simdjson_really_inline error_code simdjson_result_base<T>::error() const noexcept {
  return this->second;
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_really_inline T& simdjson_result_base<T>::value() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return this->first;
}

template<typename T>
simdjson_really_inline T&& simdjson_result_base<T>::value() && noexcept(false) {
  return std::forward<simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
simdjson_really_inline T&& simdjson_result_base<T>::take_value() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<T>(this->first);
}

template<typename T>
simdjson_really_inline simdjson_result_base<T>::operator T&&() && noexcept(false) {
  return std::forward<simdjson_result_base<T>>(*this).take_value();
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_really_inline const T& simdjson_result_base<T>::value_unsafe() const& noexcept {
  return this->first;
}

template<typename T>
simdjson_really_inline T&& simdjson_result_base<T>::value_unsafe() && noexcept {
  return std::forward<T>(this->first);
}

template<typename T>
simdjson_really_inline simdjson_result_base<T>::simdjson_result_base(T &&value, error_code error) noexcept
    : std::pair<T, error_code>(std::forward<T>(value), error) {}
template<typename T>
simdjson_really_inline simdjson_result_base<T>::simdjson_result_base(error_code error) noexcept
    : simdjson_result_base(T{}, error) {}
template<typename T>
simdjson_really_inline simdjson_result_base<T>::simdjson_result_base(T &&value) noexcept
    : simdjson_result_base(std::forward<T>(value), SUCCESS) {}
template<typename T>
simdjson_really_inline simdjson_result_base<T>::simdjson_result_base() noexcept
    : simdjson_result_base(T{}, UNINITIALIZED) {}

} // namespace internal

///
/// simdjson_result<T> inline implementation
///

template<typename T>
simdjson_really_inline void simdjson_result<T>::tie(T &value, error_code &error) && noexcept {
  std::forward<internal::simdjson_result_base<T>>(*this).tie(value, error);
}

template<typename T>
simdjson_warn_unused simdjson_really_inline error_code simdjson_result<T>::get(T &value) && noexcept {
  return std::forward<internal::simdjson_result_base<T>>(*this).get(value);
}

template<typename T>
simdjson_really_inline error_code simdjson_result<T>::error() const noexcept {
  return internal::simdjson_result_base<T>::error();
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_really_inline T& simdjson_result<T>::value() & noexcept(false) {
  return internal::simdjson_result_base<T>::value();
}

template<typename T>
simdjson_really_inline T&& simdjson_result<T>::value() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).value();
}

template<typename T>
simdjson_really_inline T&& simdjson_result<T>::take_value() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
simdjson_really_inline simdjson_result<T>::operator T&&() && noexcept(false) {
  return std::forward<internal::simdjson_result_base<T>>(*this).take_value();
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_really_inline const T& simdjson_result<T>::value_unsafe() const& noexcept {
  return internal::simdjson_result_base<T>::value_unsafe();
}

template<typename T>
simdjson_really_inline T&& simdjson_result<T>::value_unsafe() && noexcept {
  return std::forward<internal::simdjson_result_base<T>>(*this).value_unsafe();
}

template<typename T>
simdjson_really_inline simdjson_result<T>::simdjson_result(T &&value, error_code error) noexcept
    : internal::simdjson_result_base<T>(std::forward<T>(value), error) {}
template<typename T>
simdjson_really_inline simdjson_result<T>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<T>(error) {}
template<typename T>
simdjson_really_inline simdjson_result<T>::simdjson_result(T &&value) noexcept
    : internal::simdjson_result_base<T>(std::forward<T>(value)) {}
template<typename T>
simdjson_really_inline simdjson_result<T>::simdjson_result() noexcept
    : internal::simdjson_result_base<T>() {}

} // namespace simdjson

#endif // SIMDJSON_INLINE_ERROR_H
/* end file include/simdjson/error-inl.h */
/* begin file include/simdjson/padded_string-inl.h */
#ifndef SIMDJSON_INLINE_PADDED_STRING_H
#define SIMDJSON_INLINE_PADDED_STRING_H


#include <climits>
#include <cstring>
#include <memory>
#include <string>

namespace simdjson {
namespace internal {

// The allocate_padded_buffer function is a low-level function to allocate memory
// with padding so we can read past the "length" bytes safely. It is used by
// the padded_string class automatically. It returns nullptr in case
// of error: the caller should check for a null pointer.
// The length parameter is the maximum size in bytes of the string.
// The caller is responsible to free the memory (e.g., delete[] (...)).
inline char *allocate_padded_buffer(size_t length) noexcept {
  const size_t totalpaddedlength = length + SIMDJSON_PADDING;
  if(totalpaddedlength<length) {
    // overflow
    return nullptr;
  }
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  // avoid getting out of memory
  if (totalpaddedlength>(1UL<<20)) {
    return nullptr;
  }
#endif

  char *padded_buffer = new (std::nothrow) char[totalpaddedlength];
  if (padded_buffer == nullptr) {
    return nullptr;
  }
  // We write zeroes in the padded region to avoid having uninitized
  // garbage. If nothing else, garbage getting read might trigger a
  // warning in a memory checking.
  std::memset(padded_buffer + length, 0, totalpaddedlength - length);
  return padded_buffer;
} // allocate_padded_buffer()

} // namespace internal


inline padded_string::padded_string() noexcept {}
inline padded_string::padded_string(size_t length) noexcept
    : viable_size(length), data_ptr(internal::allocate_padded_buffer(length)) {
}
inline padded_string::padded_string(const char *data, size_t length) noexcept
    : viable_size(length), data_ptr(internal::allocate_padded_buffer(length)) {
  if ((data != nullptr) and (data_ptr != nullptr)) {
    std::memcpy(data_ptr, data, length);
  }
}
// note: do not pass std::string arguments by value
inline padded_string::padded_string(const std::string & str_ ) noexcept
    : viable_size(str_.size()), data_ptr(internal::allocate_padded_buffer(str_.size())) {
  if (data_ptr != nullptr) {
    std::memcpy(data_ptr, str_.data(), str_.size());
  }
}
// note: do pass std::string_view arguments by value
inline padded_string::padded_string(std::string_view sv_) noexcept
    : viable_size(sv_.size()), data_ptr(internal::allocate_padded_buffer(sv_.size())) {
  if(simdjson_unlikely(!data_ptr)) {
    //allocation failed or zero size
    viable_size=0;
    return;
  }
  if (sv_.size()) {
    std::memcpy(data_ptr, sv_.data(), sv_.size());
  }
}
inline padded_string::padded_string(padded_string &&o) noexcept
    : viable_size(o.viable_size), data_ptr(o.data_ptr) {
  o.data_ptr = nullptr; // we take ownership
}

inline padded_string &padded_string::operator=(padded_string &&o) noexcept {
  delete[] data_ptr;
  data_ptr = o.data_ptr;
  viable_size = o.viable_size;
  o.data_ptr = nullptr; // we take ownership
  o.viable_size = 0;
  return *this;
}

inline void padded_string::swap(padded_string &o) noexcept {
  size_t tmp_viable_size = viable_size;
  char *tmp_data_ptr = data_ptr;
  viable_size = o.viable_size;
  data_ptr = o.data_ptr;
  o.data_ptr = tmp_data_ptr;
  o.viable_size = tmp_viable_size;
}

inline padded_string::~padded_string() noexcept {
  delete[] data_ptr;
}

inline size_t padded_string::size() const noexcept { return viable_size; }

inline size_t padded_string::length() const noexcept { return viable_size; }

inline const char *padded_string::data() const noexcept { return data_ptr; }

inline char *padded_string::data() noexcept { return data_ptr; }

inline padded_string::operator std::string_view() const { return std::string_view(data(), length()); }

inline padded_string::operator padded_string_view() const noexcept {
  return padded_string_view(data(), length(), length() + SIMDJSON_PADDING);
}

inline simdjson_result<padded_string> padded_string::load(const std::string &filename) noexcept {
  // Open the file
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(filename.c_str(), "rb");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  if(std::fseek(fp, 0, SEEK_END) < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
#if defined(SIMDJSON_VISUAL_STUDIO) && !SIMDJSON_IS_32BITS
  __int64 llen = _ftelli64(fp);
  if(llen == -1L) {
    std::fclose(fp);
    return IO_ERROR;
  }
#else
  long llen = std::ftell(fp);
  if((llen < 0) || (llen == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }
#endif

  // Allocate the padded_string
  size_t len = static_cast<size_t>(llen);
  padded_string s(len);
  if (s.data() == nullptr) {
    std::fclose(fp);
    return MEMALLOC;
  }

  // Read the padded_string
  std::rewind(fp);
  size_t bytes_read = std::fread(s.data(), 1, len, fp);
  if (std::fclose(fp) != 0 || bytes_read != len) {
    return IO_ERROR;
  }

  return s;
}

} // namespace simdjson

#endif // SIMDJSON_INLINE_PADDED_STRING_H
/* end file include/simdjson/padded_string-inl.h */
/* begin file include/simdjson/padded_string_view-inl.h */
#ifndef SIMDJSON_PADDED_STRING_VIEW_INL_H
#define SIMDJSON_PADDED_STRING_VIEW_INL_H


#include <climits>
#include <cstring>
#include <memory>
#include <string>

namespace simdjson {

inline padded_string_view::padded_string_view(const char* s, size_t len, size_t capacity) noexcept
  : std::string_view(s, len), _capacity(capacity)
{
}

inline padded_string_view::padded_string_view(const uint8_t* s, size_t len, size_t capacity) noexcept
  : padded_string_view(reinterpret_cast<const char*>(s), len, capacity)
{
}

inline padded_string_view::padded_string_view(const std::string &s) noexcept
  : std::string_view(s), _capacity(s.capacity())
{
}

inline padded_string_view::padded_string_view(std::string_view s, size_t capacity) noexcept
  : std::string_view(s), _capacity(capacity)
{
}

inline size_t padded_string_view::capacity() const noexcept { return _capacity; }

inline size_t padded_string_view::padding() const noexcept { return capacity() - length(); }

} // namespace simdjson

#endif // SIMDJSON_PADDED_STRING_VIEW_INL_H
/* end file include/simdjson/padded_string_view-inl.h */

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_BASE_H
/* end file include/simdjson/base.h */

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

/* begin file include/simdjson/dom/array.h */
#ifndef SIMDJSON_DOM_ARRAY_H
#define SIMDJSON_DOM_ARRAY_H

/* begin file include/simdjson/internal/tape_ref.h */
#ifndef SIMDJSON_INTERNAL_TAPE_REF_H
#define SIMDJSON_INTERNAL_TAPE_REF_H

/* begin file include/simdjson/internal/tape_type.h */
#ifndef SIMDJSON_INTERNAL_TAPE_TYPE_H
#define SIMDJSON_INTERNAL_TAPE_TYPE_H

namespace simdjson {
namespace internal {

/**
 * The possible types in the tape.
 */
enum class tape_type {
  ROOT = 'r',
  START_ARRAY = '[',
  START_OBJECT = '{',
  END_ARRAY = ']',
  END_OBJECT = '}',
  STRING = '"',
  INT64 = 'l',
  UINT64 = 'u',
  DOUBLE = 'd',
  TRUE_VALUE = 't',
  FALSE_VALUE = 'f',
  NULL_VALUE = 'n'
}; // enum class tape_type

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_TAPE_TYPE_H
/* end file include/simdjson/internal/tape_type.h */

namespace simdjson {

namespace dom {
  class document;
}

namespace internal {

constexpr const uint64_t JSON_VALUE_MASK = 0x00FFFFFFFFFFFFFF;
constexpr const uint32_t JSON_COUNT_MASK = 0xFFFFFF;

/**
 * A reference to an element on the tape. Internal only.
 */
class tape_ref {
public:
  simdjson_really_inline tape_ref() noexcept;
  simdjson_really_inline tape_ref(const dom::document *doc, size_t json_index) noexcept;
  inline size_t after_element() const noexcept;
  simdjson_really_inline tape_type tape_ref_type() const noexcept;
  simdjson_really_inline uint64_t tape_value() const noexcept;
  simdjson_really_inline bool is_double() const noexcept;
  simdjson_really_inline bool is_int64() const noexcept;
  simdjson_really_inline bool is_uint64() const noexcept;
  simdjson_really_inline bool is_false() const noexcept;
  simdjson_really_inline bool is_true() const noexcept;
  simdjson_really_inline bool is_null_on_tape() const noexcept;// different name to avoid clash with is_null.
  simdjson_really_inline uint32_t matching_brace_index() const noexcept;
  simdjson_really_inline uint32_t scope_count() const noexcept;
  template<typename T>
  simdjson_really_inline T next_tape_value() const noexcept;
  simdjson_really_inline uint32_t get_string_length() const noexcept;
  simdjson_really_inline const char * get_c_str() const noexcept;
  inline std::string_view get_string_view() const noexcept;
  simdjson_really_inline bool is_document_root() const noexcept;

  /** The document this element references. */
  const dom::document *doc;

  /** The index of this element on `doc.tape[]` */
  size_t json_index;
};

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_TAPE_REF_H
/* end file include/simdjson/internal/tape_ref.h */

namespace simdjson {

namespace internal {
template<typename T>
class string_builder;
}
namespace dom {

class document;
class element;

/**
 * JSON array.
 */
class array {
public:
  /** Create a new, invalid array */
  simdjson_really_inline array() noexcept;

  class iterator {
  public:
    using value_type = element;
    using difference_type = std::ptrdiff_t;

    /**
     * Get the actual value
     */
    inline value_type operator*() const noexcept;
    /**
     * Get the next value.
     *
     * Part of the std::iterator interface.
     */
    inline iterator& operator++() noexcept;
    /**
     * Get the next value.
     *
     * Part of the  std::iterator interface.
     */
    inline iterator operator++(int) noexcept;
    /**
     * Check if these values come from the same place in the JSON.
     *
     * Part of the std::iterator interface.
     */
    inline bool operator!=(const iterator& other) const noexcept;
    inline bool operator==(const iterator& other) const noexcept;

    inline bool operator<(const iterator& other) const noexcept;
    inline bool operator<=(const iterator& other) const noexcept;
    inline bool operator>=(const iterator& other) const noexcept;
    inline bool operator>(const iterator& other) const noexcept;

    iterator() noexcept = default;
    iterator(const iterator&) noexcept = default;
    iterator& operator=(const iterator&) noexcept = default;
  private:
    simdjson_really_inline iterator(const internal::tape_ref &tape) noexcept;
    internal::tape_ref tape;
    friend class array;
  };

  /**
   * Return the first array element.
   *
   * Part of the std::iterable interface.
   */
  inline iterator begin() const noexcept;
  /**
   * One past the last array element.
   *
   * Part of the std::iterable interface.
   */
  inline iterator end() const noexcept;
  /**
   * Get the size of the array (number of immediate children).
   * It is a saturated value with a maximum of 0xFFFFFF: if the value
   * is 0xFFFFFF then the size is 0xFFFFFF or greater.
   */
  inline size_t size() const noexcept;
  /**
   * Get the total number of slots used by this array on the tape.
   *
   * Note that this is not the same thing as `size()`, which reports the
   * number of actual elements within an array (not counting its children).
   *
   * Since an element can use 1 or 2 slots on the tape, you can only use this
   * to figure out the total size of an array (including its children,
   * recursively) if you know its structure ahead of time.
   **/
  inline size_t number_of_slots() const noexcept;
  /**
   * Get the value associated with the given JSON pointer.  We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard, interpreting the current node
   * as the root of its own JSON document.
   *
   *   dom::parser parser;
   *   array a = parser.parse(R"([ { "foo": { "a": [ 10, 20, 30 ] }} ])"_padded);
   *   a.at_pointer("/0/foo/a/1") == 20
   *   a.at_pointer("0")["foo"]["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at_pointer(std::string_view json_pointer) const noexcept;

  /**
   * Get the value at the given index. This function has linear-time complexity and
   * is equivalent to the following:
   *
   *    size_t i=0;
   *    for (auto element : *this) {
   *      if (i == index) { return element; }
   *      i++;
   *    }
   *    return INDEX_OUT_OF_BOUNDS;
   *
   * Avoid calling the at() function repeatedly.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline simdjson_result<element> at(size_t index) const noexcept;

private:
  simdjson_really_inline array(const internal::tape_ref &tape) noexcept;
  internal::tape_ref tape;
  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::internal::string_builder;
};


} // namespace dom

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::array> : public internal::simdjson_result_base<dom::array> {
public:
  simdjson_really_inline simdjson_result() noexcept; ///< @private
  simdjson_really_inline simdjson_result(dom::array value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> at_pointer(std::string_view json_pointer) const noexcept;
  inline simdjson_result<dom::element> at(size_t index) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::array::iterator begin() const noexcept(false);
  inline dom::array::iterator end() const noexcept(false);
  inline size_t size() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};



} // namespace simdjson

#if defined(__cpp_lib_ranges)
#include <ranges>

namespace std {
namespace ranges {
template<>
inline constexpr bool enable_view<simdjson::dom::array> = true;
#if SIMDJSON_EXCEPTIONS
template<>
inline constexpr bool enable_view<simdjson::simdjson_result<simdjson::dom::array>> = true;
#endif // SIMDJSON_EXCEPTIONS
} // namespace ranges
} // namespace std
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_DOM_ARRAY_H
/* end file include/simdjson/dom/array.h */
/* begin file include/simdjson/dom/document_stream.h */
#ifndef SIMDJSON_DOCUMENT_STREAM_H
#define SIMDJSON_DOCUMENT_STREAM_H

/* begin file include/simdjson/dom/parser.h */
#ifndef SIMDJSON_DOM_PARSER_H
#define SIMDJSON_DOM_PARSER_H

/* begin file include/simdjson/dom/document.h */
#ifndef SIMDJSON_DOM_DOCUMENT_H
#define SIMDJSON_DOM_DOCUMENT_H

#include <memory>
#include <ostream>

namespace simdjson {
namespace dom {

class element;

/**
 * A parsed JSON document.
 *
 * This class cannot be copied, only moved, to avoid unintended allocations.
 */
class document {
public:
  /**
   * Create a document container with zero capacity.
   *
   * The parser will allocate capacity as needed.
   */
  document() noexcept = default;
  ~document() noexcept = default;

  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed and it is invalidated.
   */
  document(document &&other) noexcept = default;
  /** @private */
  document(const document &) = delete; // Disallow copying
  /**
   * Take another document's buffers.
   *
   * @param other The document to take. Its capacity is zeroed.
   */
  document &operator=(document &&other) noexcept = default;
  /** @private */
  document &operator=(const document &) = delete; // Disallow copying

  /**
   * Get the root element of this document as a JSON array.
   */
  element root() const noexcept;

  /**
   * @private Dump the raw tape for debugging.
   *
   * @param os the stream to output to.
   * @return false if the tape is likely wrong (e.g., you did not parse a valid JSON).
   */
  bool dump_raw_tape(std::ostream &os) const noexcept;

  /** @private Structural values. */
  std::unique_ptr<uint64_t[]> tape{};

  /** @private String values.
   *
   * Should be at least byte_capacity.
   */
  std::unique_ptr<uint8_t[]> string_buf{};
  /** @private Allocate memory to support
   * input JSON documents of up to len bytes.
   *
   * When calling this function, you lose
   * all the data.
   *
   * The memory allocation is strict: you
   * can you use this function to increase
   * or lower the amount of allocated memory.
   * Passsing zero clears the memory.
   */
  error_code allocate(size_t len) noexcept;
  /** @private Capacity in bytes, in terms
   * of how many bytes of input JSON we can
   * support.
   */
  size_t capacity() const noexcept;


private:
  size_t allocated_capacity{0};
  friend class parser;
}; // class document

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_DOM_DOCUMENT_H
/* end file include/simdjson/dom/document.h */
#include <memory>
#include <ostream>
#include <string>

namespace simdjson {

namespace dom {

class document_stream;
class element;

/** The default batch size for parser.parse_many() and parser.load_many() */
static constexpr size_t DEFAULT_BATCH_SIZE = 1000000;
/**
 * Some adversary might try to set the batch size to 0 or 1, which might cause problems.
 * We set a minimum of 32B since anything else is highly likely to be an error. In practice,
 * most users will want a much larger batch size.
 *
 * All non-negative MINIMAL_BATCH_SIZE values should be 'safe' except that, obviously, no JSON
 * document can ever span 0 or 1 byte and that very large values would create memory allocation issues.
 */
static constexpr size_t MINIMAL_BATCH_SIZE = 32;

/**
 * It is wasteful to allocate memory for tiny documents (e.g., 4 bytes).
 */
static constexpr size_t MINIMAL_DOCUMENT_CAPACITY = 32;

/**
 * A persistent document parser.
 *
 * The parser is designed to be reused, holding the internal buffers necessary to do parsing,
 * as well as memory for a single document. The parsed document is overwritten on each parse.
 *
 * This class cannot be copied, only moved, to avoid unintended allocations.
 *
 * @note Moving a parser instance may invalidate "dom::element" instances. If you need to
 * preserve both the "dom::element" instances and the parser, consider wrapping the parser
 * instance in a std::unique_ptr instance:
 *
 *   std::unique_ptr<dom::parser> parser(new dom::parser{});
 *   auto error = parser->load(f).get(root);
 *
 * You can then move std::unique_ptr safely.
 *
 * @note This is not thread safe: one parser cannot produce two documents at the same time!
 */
class parser {
public:
  /**
   * Create a JSON parser.
   *
   * The new parser will have zero capacity.
   *
   * @param max_capacity The maximum document length the parser can automatically handle. The parser
   *    will allocate more capacity on an as needed basis (when it sees documents too big to handle)
   *    up to this amount. The parser still starts with zero capacity no matter what this number is:
   *    to allocate an initial capacity, call allocate() after constructing the parser.
   *    Defaults to SIMDJSON_MAXSIZE_BYTES (the largest single document simdjson can process).
   */
  simdjson_really_inline explicit parser(size_t max_capacity = SIMDJSON_MAXSIZE_BYTES) noexcept;
  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  simdjson_really_inline parser(parser &&other) noexcept;
  parser(const parser &) = delete; ///< @private Disallow copying
  /**
   * Take another parser's buffers and state.
   *
   * @param other The parser to take. Its capacity is zeroed.
   */
  simdjson_really_inline parser &operator=(parser &&other) noexcept;
  parser &operator=(const parser &) = delete; ///< @private Disallow copying

  /** Deallocate the JSON parser. */
  ~parser()=default;

  /**
   * Load a JSON document from a file and return a reference to it.
   *
   *   dom::parser parser;
   *   const element doc = parser.load("jsonexamples/twitter.json");
   *
   * The function is eager: the file's content is loaded in memory inside the parser instance
   * and immediately parsed. The file can be deleted after the  `parser.load` call.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * Moving the parser instance is safe, but it invalidates the element instances. You may store
   * the parser instance without moving it by wrapping it inside an `unique_ptr` instance like
   * so: `std::unique_ptr<dom::parser> parser(new dom::parser{});`.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than the file length, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param path The path to load.
   * @return The document, or an error:
   *         - IO_ERROR if there was an error opening or reading the file.
   *           Be mindful that on some 32-bit systems,
   *           the file size might be limited to 2 GB.
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<element> load(const std::string &path) & noexcept;
  inline simdjson_result<element> load(const std::string &path) &&  = delete ;
  /**
   * Parse a JSON document and return a temporary reference to it.
   *
   *   dom::parser parser;
   *   element doc_root = parser.parse(buf, len);
   *
   * The function eagerly parses the input: the input can be modified and discarded after
   * the `parser.parse(buf, len)` call has completed.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * The JSON document still lives in the parser: this is the most efficient way to parse JSON
   * documents because it reuses the same buffers, but you *must* use the document before you
   * destroy the parser or call parse() again.
   *
   * Moving the parser instance is safe, but it invalidates the element instances. You may store
   * the parser instance without moving it by wrapping it inside an `unique_ptr` instance like
   * so: `std::unique_ptr<dom::parser> parser(new dom::parser{});`.
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated.
   *
   * If realloc_if_needed is true (the default), it is assumed that the buffer does *not* have enough padding,
   * and it is copied into an enlarged temporary buffer before parsing. Thus the following is safe:
   *
   *   const char *json      = R"({"key":"value"})";
   *   const size_t json_len = std::strlen(json);
   *   simdjson::dom::parser parser;
   *   simdjson::dom::element element = parser.parse(json, json_len);
   *
   * If you set realloc_if_needed to false (e.g., parser.parse(json, json_len, false)),
   * you must provide a buffer with at least SIMDJSON_PADDING extra bytes at the end.
   * The benefit of setting realloc_if_needed to false is that you avoid a temporary
   * memory allocation and a copy.
   *
   * The padded bytes may be read. It is not important how you initialize
   * these bytes though we recommend a sensible default like null character values or spaces.
   * For example, the following low-level code is safe:
   *
   *   const char *json      = R"({"key":"value"})";
   *   const size_t json_len = std::strlen(json);
   *   std::unique_ptr<char[]> padded_json_copy{new char[json_len + SIMDJSON_PADDING]};
   *   std::memcpy(padded_json_copy.get(), json, json_len);
   *   std::memset(padded_json_copy.get() + json_len, '\0', SIMDJSON_PADDING);
   *   simdjson::dom::parser parser;
   *   simdjson::dom::element element = parser.parse(padded_json_copy.get(), json_len, false);
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than len, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param buf The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, unless
   *            realloc_if_needed is true.
   * @param len The length of the JSON.
   * @param realloc_if_needed Whether to reallocate and enlarge the JSON buffer to add padding.
   * @return An element pointing at the root of the document, or an error:
   *         - MEMALLOC if realloc_if_needed is true or the parser does not have enough capacity,
   *           and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<element> parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  inline simdjson_result<element> parse(const uint8_t *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_really_inline simdjson_result<element> parse(const char *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  simdjson_really_inline simdjson_result<element> parse(const char *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_really_inline simdjson_result<element> parse(const std::string &s) & noexcept;
  simdjson_really_inline simdjson_result<element> parse(const std::string &s) && =delete;
  /** @overload parse(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_really_inline simdjson_result<element> parse(const padded_string &s) & noexcept;
  simdjson_really_inline simdjson_result<element> parse(const padded_string &s) && =delete;

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  simdjson_really_inline simdjson_result<element> parse(const char *buf) noexcept = delete;

  /**
   * Parse a JSON document into a provide document instance and return a temporary reference to it.
   * It is similar to the function `parse` except that instead of parsing into the internal
   * `document` instance associated with the parser, it allows the user to provide a document
   * instance.
   *
   *   dom::parser parser;
   *   dom::document doc;
   *   element doc_root = parser.parse_into_document(doc, buf, len);
   *
   * The function eagerly parses the input: the input can be modified and discarded after
   * the `parser.parse(buf, len)` call has completed.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * After the call to parse_into_document, the parser is no longer needed.
   *
   * The JSON document lives in the document instance: you must keep the document
   * instance alive while you navigate through it (i.e., used the returned value from
   * parse_into_document). You are encourage to reuse the document instance
   * many times with new data to avoid reallocations:
   *
   *   dom::document doc;
   *   element doc_root1 = parser.parse_into_document(doc, buf1, len);
   *   //... doc_root1 is a pointer inside doc
   *   element doc_root2 = parser.parse_into_document(doc, buf1, len);
   *   //... doc_root2 is a pointer inside doc
   *   // at this point doc_root1 is no longer safe
   *
   * Moving the document instance is safe, but it invalidates the element instances. After
   * moving a document, you can recover safe access to the document root with its `root()` method.
   *
   * @param doc The document instance where the parsed data will be stored (on success).
   * @param buf The JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes, unless
   *            realloc_if_needed is true.
   * @param len The length of the JSON.
   * @param realloc_if_needed Whether to reallocate and enlarge the JSON buffer to add padding.
   * @return An element pointing at the root of document, or an error:
   *         - MEMALLOC if realloc_if_needed is true or the parser does not have enough capacity,
   *           and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and len > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<element> parse_into_document(document& doc, const uint8_t *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  inline simdjson_result<element> parse_into_document(document& doc, const uint8_t *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse_into_document(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_really_inline simdjson_result<element> parse_into_document(document& doc, const char *buf, size_t len, bool realloc_if_needed = true) & noexcept;
  simdjson_really_inline simdjson_result<element> parse_into_document(document& doc, const char *buf, size_t len, bool realloc_if_needed = true) && =delete;
  /** @overload parse_into_document(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_really_inline simdjson_result<element> parse_into_document(document& doc, const std::string &s) & noexcept;
  simdjson_really_inline simdjson_result<element> parse_into_document(document& doc, const std::string &s) && =delete;
  /** @overload parse_into_document(const uint8_t *buf, size_t len, bool realloc_if_needed) */
  simdjson_really_inline simdjson_result<element> parse_into_document(document& doc, const padded_string &s) & noexcept;
  simdjson_really_inline simdjson_result<element> parse_into_document(document& doc, const padded_string &s) && =delete;

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  simdjson_really_inline simdjson_result<element> parse_into_document(document& doc, const char *buf) noexcept = delete;

  /**
   * Load a file containing many JSON documents.
   *
   *   dom::parser parser;
   *   for (const element doc : parser.load_many(path)) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * The file is loaded in memory and can be safely deleted after the `parser.load_many(path)`
   * function has returned. The memory is held by the `parser` instance.
   *
   * The function is lazy: it may be that no more than one JSON document at a time is parsed.
   * And, possibly, no document many have been parsed when the `parser.load_many(path)` function
   * returned.
   *
   * ### Format
   *
   * The file must contain a series of one or more JSON documents, concatenated into a single
   * buffer, separated by whitespace. It effectively parses until it has a fully valid document,
   * then starts parsing the next document at that point. (It does this with more parallelism and
   * lookahead than you might think, though.)
   *
   * Documents that consist of an object or array may omit the whitespace between them, concatenating
   * with no separator. documents that consist of a single primitive (i.e. documents that are not
   * arrays or objects) MUST be separated with whitespace.
   *
   * The documents must not exceed batch_size bytes (by default 1MB) or they will fail to parse.
   * Setting batch_size to excessively large or excesively small values may impact negatively the
   * performance.
   *
   * ### Error Handling
   *
   * All errors are returned during iteration: if there is a global error such as memory allocation,
   * it will be yielded as the first result. Iteration always stops after the first error.
   *
   * As with all other simdjson methods, non-exception error handling is readily available through
   * the same interface, requiring you to check the error before using the document:
   *
   *   dom::parser parser;
   *   dom::document_stream docs;
   *   auto error = parser.load_many(path).get(docs);
   *   if (error) { cerr << error << endl; exit(1); }
   *   for (auto doc : docs) {
   *     std::string_view title;
   *     if ((error = doc["title"].get(title)) { cerr << error << endl; exit(1); }
   *     cout << title << endl;
   *   }
   *
   * ### Threads
   *
   * When compiled with SIMDJSON_THREADS_ENABLED, this method will use a single thread under the
   * hood to do some lookahead.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param path File name pointing at the concatenated JSON to parse.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 1MB (as simdjson::dom::DEFAULT_BATCH_SIZE), which has been a reasonable sweet
   *                   spot in our tests.
   *                   If you set the batch_size to a value smaller than simdjson::dom::MINIMAL_BATCH_SIZE
   *                   (currently 32B), it will be replaced by simdjson::dom::MINIMAL_BATCH_SIZE.
   * @return The stream, or an error. An empty input will yield 0 documents rather than an EMPTY error. Errors:
   *         - IO_ERROR if there was an error opening or reading the file.
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails.
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<document_stream> load_many(const std::string &path, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;

  /**
   * Parse a buffer containing many JSON documents.
   *
   *   dom::parser parser;
   *   for (element doc : parser.parse_many(buf, len)) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * No copy of the input buffer is made.
   *
   * The function is lazy: it may be that no more than one JSON document at a time is parsed.
   * And, possibly, no document many have been parsed when the `parser.load_many(path)` function
   * returned.
   *
   * The caller is responsabile to ensure that the input string data remains unchanged and is
   * not deleted during the loop. In particular, the following is unsafe and will not compile:
   *
   *   auto docs = parser.parse_many("[\"temporary data\"]"_padded);
   *   // here the string "[\"temporary data\"]" may no longer exist in memory
   *   // the parser instance may not have even accessed the input yet
   *   for (element doc : docs) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * The following is safe:
   *
   *   auto json = "[\"temporary data\"]"_padded;
   *   auto docs = parser.parse_many(json);
   *   for (element doc : docs) {
   *     cout << std::string(doc["title"]) << endl;
   *   }
   *
   * ### Format
   *
   * The buffer must contain a series of one or more JSON documents, concatenated into a single
   * buffer, separated by whitespace. It effectively parses until it has a fully valid document,
   * then starts parsing the next document at that point. (It does this with more parallelism and
   * lookahead than you might think, though.)
   *
   * documents that consist of an object or array may omit the whitespace between them, concatenating
   * with no separator. documents that consist of a single primitive (i.e. documents that are not
   * arrays or objects) MUST be separated with whitespace.
   *
   * The documents must not exceed batch_size bytes (by default 1MB) or they will fail to parse.
   * Setting batch_size to excessively large or excesively small values may impact negatively the
   * performance.
   *
   * ### Error Handling
   *
   * All errors are returned during iteration: if there is a global error such as memory allocation,
   * it will be yielded as the first result. Iteration always stops after the first error.
   *
   * As with all other simdjson methods, non-exception error handling is readily available through
   * the same interface, requiring you to check the error before using the document:
   *
   *   dom::parser parser;
   *   dom::document_stream docs;
   *   auto error = parser.load_many(path).get(docs);
   *   if (error) { cerr << error << endl; exit(1); }
   *   for (auto doc : docs) {
   *     std::string_view title;
   *     if ((error = doc["title"].get(title)) { cerr << error << endl; exit(1); }
   *     cout << title << endl;
   *   }
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated.
   *
   * ### Threads
   *
   * When compiled with SIMDJSON_THREADS_ENABLED, this method will use a single thread under the
   * hood to do some lookahead.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param buf The concatenated JSON to parse. Must have at least len + SIMDJSON_PADDING allocated bytes.
   * @param len The length of the concatenated JSON.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream, or an error. An empty input will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<document_stream> parse_many(const uint8_t *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> parse_many(const char *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> parse_many(const std::string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  inline simdjson_result<document_stream> parse_many(const std::string &&s, size_t batch_size) = delete;// unsafe
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> parse_many(const padded_string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  inline simdjson_result<document_stream> parse_many(const padded_string &&s, size_t batch_size) = delete;// unsafe

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  simdjson_result<document_stream> parse_many(const char *buf, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept = delete;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  simdjson_warn_unused inline error_code allocate(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
  /**
   * @private deprecated because it returns bool instead of error_code, which is our standard for
   * failures. Use allocate() instead.
   *
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return true if successful, false if allocation failed.
   */
  [[deprecated("Use allocate() instead.")]]
  simdjson_warn_unused inline bool allocate_capacity(size_t capacity, size_t max_depth = DEFAULT_MAX_DEPTH) noexcept;
#endif // SIMDJSON_DISABLE_DEPRECATED_API
  /**
   * The largest document this parser can support without reallocating.
   *
   * @return Current capacity, in bytes.
   */
  simdjson_really_inline size_t capacity() const noexcept;

  /**
   * The largest document this parser can automatically support.
   *
   * The parser may reallocate internal buffers as needed up to this amount.
   *
   * @return Maximum capacity, in bytes.
   */
  simdjson_really_inline size_t max_capacity() const noexcept;

  /**
   * The maximum level of nested object and arrays supported by this parser.
   *
   * @return Maximum depth, in bytes.
   */
  simdjson_really_inline size_t max_depth() const noexcept;

  /**
   * Set max_capacity. This is the largest document this parser can automatically support.
   *
   * The parser may reallocate internal buffers as needed up to this amount as documents are passed
   * to it.
   *
   * Note: To avoid limiting the memory to an absurd value, such as zero or two bytes,
   * iff you try to set max_capacity to a value lower than MINIMAL_DOCUMENT_CAPACITY,
   * then the maximal capacity is set to MINIMAL_DOCUMENT_CAPACITY.
   *
   * This call will not allocate or deallocate, even if capacity is currently above max_capacity.
   *
   * @param max_capacity The new maximum capacity, in bytes.
   */
  simdjson_really_inline void set_max_capacity(size_t max_capacity) noexcept;

#ifdef SIMDJSON_THREADS_ENABLED
  /**
   * The parser instance can use threads when they are available to speed up some
   * operations. It is enabled by default. Changing this attribute will change the
   * behavior of the parser for future operations.
   */
  bool threaded{true};
#endif
  /** @private Use the new DOM API instead */
  class Iterator;
  /** @private Use simdjson_error instead */
  using InvalidJSON [[deprecated("Use simdjson_error instead")]] = simdjson_error;

  /** @private [for benchmarking access] The implementation to use */
  std::unique_ptr<internal::dom_parser_implementation> implementation{};

  /** @private Use `if (parser.parse(...).error())` instead */
  bool valid{false};
  /** @private Use `parser.parse(...).error()` instead */
  error_code error{UNINITIALIZED};

  /** @private Use `parser.parse(...).value()` instead */
  document doc{};

  /** @private returns true if the document parsed was valid */
  [[deprecated("Use the result of parser.parse() instead")]]
  inline bool is_valid() const noexcept;

  /**
   * @private return an error code corresponding to the last parsing attempt, see
   * simdjson.h will return UNITIALIZED if no parsing was attempted
   */
  [[deprecated("Use the result of parser.parse() instead")]]
  inline int get_error_code() const noexcept;

  /** @private return the string equivalent of "get_error_code" */
  [[deprecated("Use error_message() on the result of parser.parse() instead, or cout << error")]]
  inline std::string get_error_message() const noexcept;

  /** @private */
  [[deprecated("Use cout << on the result of parser.parse() instead")]]
  inline bool print_json(std::ostream &os) const noexcept;

  /** @private Private and deprecated: use `parser.parse(...).doc.dump_raw_tape()` instead */
  inline bool dump_raw_tape(std::ostream &os) const noexcept;


private:
  /**
   * The maximum document length this parser will automatically support.
   *
   * The parser will not be automatically allocated above this amount.
   */
  size_t _max_capacity;

  /**
   * The loaded buffer (reused each time load() is called)
   */
  std::unique_ptr<char[]> loaded_bytes;

  /** Capacity of loaded_bytes buffer. */
  size_t _loaded_bytes_capacity{0};

  // all nodes are stored on the doc.tape using a 64-bit word.
  //
  // strings, double and ints are stored as
  //  a 64-bit word with a pointer to the actual value
  //
  //
  //
  // for objects or arrays, store [ or {  at the beginning and } and ] at the
  // end. For the openings ([ or {), we annotate them with a reference to the
  // location on the doc.tape of the end, and for then closings (} and ]), we
  // annotate them with a reference to the location of the opening
  //
  //

  /**
   * Ensure we have enough capacity to handle at least desired_capacity bytes,
   * and auto-allocate if not. This also allocates memory if needed in the
   * internal document.
   */
  inline error_code ensure_capacity(size_t desired_capacity) noexcept;
  /**
   * Ensure we have enough capacity to handle at least desired_capacity bytes,
   * and auto-allocate if not. This also allocates memory if needed in the
   * provided document.
   */
  inline error_code ensure_capacity(document& doc, size_t desired_capacity) noexcept;

  /** Read the file into loaded_bytes */
  inline simdjson_result<size_t> read_file(const std::string &path) noexcept;

  friend class parser::Iterator;
  friend class document_stream;


}; // class parser

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_DOM_PARSER_H
/* end file include/simdjson/dom/parser.h */
#ifdef SIMDJSON_THREADS_ENABLED
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

namespace simdjson {
namespace dom {


#ifdef SIMDJSON_THREADS_ENABLED
/** @private Custom worker class **/
struct stage1_worker {
  stage1_worker() noexcept = default;
  stage1_worker(const stage1_worker&) = delete;
  stage1_worker(stage1_worker&&) = delete;
  stage1_worker operator=(const stage1_worker&) = delete;
  ~stage1_worker();
  /**
   * We only start the thread when it is needed, not at object construction, this may throw.
   * You should only call this once.
   **/
  void start_thread();
  /**
   * Start a stage 1 job. You should first call 'run', then 'finish'.
   * You must call start_thread once before.
   */
  void run(document_stream * ds, dom::parser * stage1, size_t next_batch_start);
  /** Wait for the run to finish (blocking). You should first call 'run', then 'finish'. **/
  void finish();

private:

  /**
   * Normally, we would never stop the thread. But we do in the destructor.
   * This function is only safe assuming that you are not waiting for results. You
   * should have called run, then finish, and be done.
   **/
  void stop_thread();

  std::thread thread{};
  /** These three variables define the work done by the thread. **/
  dom::parser * stage1_thread_parser{};
  size_t _next_batch_start{};
  document_stream * owner{};
  /**
   * We have two state variables. This could be streamlined to one variable in the future but
   * we use two for clarity.
   */
  bool has_work{false};
  bool can_work{true};

  /**
   * We lock using a mutex.
   */
  std::mutex locking_mutex{};
  std::condition_variable cond_var{};
};
#endif

/**
 * A forward-only stream of documents.
 *
 * Produced by parser::parse_many.
 *
 */
class document_stream {
public:
  /**
   * Construct an uninitialized document_stream.
   *
   *  ```c++
   *  document_stream docs;
   *  error = parser.parse_many(json).get(docs);
   *  ```
   */
  simdjson_really_inline document_stream() noexcept;
  /** Move one document_stream to another. */
  simdjson_really_inline document_stream(document_stream &&other) noexcept = default;
  /** Move one document_stream to another. */
  simdjson_really_inline document_stream &operator=(document_stream &&other) noexcept = default;

  simdjson_really_inline ~document_stream() noexcept;

  /**
   * An iterator through a forward-only stream of documents.
   */
  class iterator {
  public:
    using value_type = simdjson_result<element>;
    using reference  = value_type;

    using difference_type   = std::ptrdiff_t;

    using iterator_category = std::input_iterator_tag;

    /**
     * Default contructor.
     */
    simdjson_really_inline iterator() noexcept;
    /**
     * Get the current document (or error).
     */
    simdjson_really_inline reference operator*() noexcept;
    /**
     * Advance to the next document (prefix).
     */
    inline iterator& operator++() noexcept;
    /**
     * Check if we're at the end yet.
     * @param other the end iterator to compare to.
     */
    simdjson_really_inline bool operator!=(const iterator &other) const noexcept;
    /**
     * @private
     *
     * Gives the current index in the input document in bytes.
     *
     *   document_stream stream = parser.parse_many(json,window);
     *   for(auto i = stream.begin(); i != stream.end(); ++i) {
     *      auto doc = *i;
     *      size_t index = i.current_index();
     *   }
     *
     * This function (current_index()) is experimental and the usage
     * may change in future versions of simdjson: we find the API somewhat
     * awkward and we would like to offer something friendlier.
     */
     simdjson_really_inline size_t current_index() const noexcept;
    /**
     * @private
     *
     * Gives a view of the current document.
     *
     *   document_stream stream = parser.parse_many(json,window);
     *   for(auto i = stream.begin(); i != stream.end(); ++i) {
     *      auto doc = *i;
     *      std::string_view v = i->source();
     *   }
     *
     * The returned string_view instance is simply a map to the (unparsed)
     * source string: it may thus include white-space characters and all manner
     * of padding.
     *
     * This function (source()) is experimental and the usage
     * may change in future versions of simdjson: we find the API somewhat
     * awkward and we would like to offer something friendlier.
     */
     simdjson_really_inline std::string_view source() const noexcept;

  private:
    simdjson_really_inline iterator(document_stream *s, bool finished) noexcept;
    /** The document_stream we're iterating through. */
    document_stream* stream;
    /** Whether we're finished or not. */
    bool finished;
    friend class document_stream;
  };

  /**
   * Start iterating the documents in the stream.
   */
  simdjson_really_inline iterator begin() noexcept;
  /**
   * The end of the stream, for iterator comparison purposes.
   */
  simdjson_really_inline iterator end() noexcept;

private:

  document_stream &operator=(const document_stream &) = delete; // Disallow copying
  document_stream(const document_stream &other) = delete; // Disallow copying

  /**
   * Construct a document_stream. Does not allocate or parse anything until the iterator is
   * used.
   *
   * @param parser is a reference to the parser instance used to generate this document_stream
   * @param buf is the raw byte buffer we need to process
   * @param len is the length of the raw byte buffer in bytes
   * @param batch_size is the size of the windows (must be strictly greater or equal to the largest JSON document)
   */
  simdjson_really_inline document_stream(
    dom::parser &parser,
    const uint8_t *buf,
    size_t len,
    size_t batch_size
  ) noexcept;

  /**
   * Parse the first document in the buffer. Used by begin(), to handle allocation and
   * initialization.
   */
  inline void start() noexcept;

  /**
   * Parse the next document found in the buffer previously given to document_stream.
   *
   * The content should be a valid JSON document encoded as UTF-8. If there is a
   * UTF-8 BOM, the caller is responsible for omitting it, UTF-8 BOM are
   * discouraged.
   *
   * You do NOT need to pre-allocate a parser.  This function takes care of
   * pre-allocating a capacity defined by the batch_size defined when creating the
   * document_stream object.
   *
   * The function returns simdjson::EMPTY if there is no more data to be parsed.
   *
   * The function returns simdjson::SUCCESS (as integer = 0) in case of success
   * and indicates that the buffer has successfully been parsed to the end.
   * Every document it contained has been parsed without error.
   *
   * The function returns an error code from simdjson/simdjson.h in case of failure
   * such as simdjson::CAPACITY, simdjson::MEMALLOC, simdjson::DEPTH_ERROR and so forth;
   * the simdjson::error_message function converts these error codes into a string).
   *
   * You can also check validity by calling parser.is_valid(). The same parser can
   * and should be reused for the other documents in the buffer.
   */
  inline void next() noexcept;

  /**
   * Pass the next batch through stage 1 and return when finished.
   * When threads are enabled, this may wait for the stage 1 thread to finish.
   */
  inline void load_batch() noexcept;

  /** Get the next document index. */
  inline size_t next_batch_start() const noexcept;

  /** Pass the next batch through stage 1 with the given parser. */
  inline error_code run_stage1(dom::parser &p, size_t batch_start) noexcept;

  dom::parser *parser;
  const uint8_t *buf;
  size_t len;
  size_t batch_size;
  /** The error (or lack thereof) from the current document. */
  error_code error;
  size_t batch_start{0};
  size_t doc_index{};

#ifdef SIMDJSON_THREADS_ENABLED
  /** Indicates whether we use threads. Note that this needs to be a constant during the execution of the parsing. */
  bool use_thread;

  inline void load_from_stage1_thread() noexcept;

  /** Start a thread to run stage 1 on the next batch. */
  inline void start_stage1_thread() noexcept;

  /** Wait for the stage 1 thread to finish and capture the results. */
  inline void finish_stage1_thread() noexcept;

  /** The error returned from the stage 1 thread. */
  error_code stage1_thread_error{UNINITIALIZED};
  /** The thread used to run stage 1 against the next batch in the background. */
  friend struct stage1_worker;
  std::unique_ptr<stage1_worker> worker{new(std::nothrow) stage1_worker()};
  /**
   * The parser used to run stage 1 in the background. Will be swapped
   * with the regular parser when finished.
   */
  dom::parser stage1_thread_parser{};
#endif // SIMDJSON_THREADS_ENABLED

  friend class dom::parser;
  friend struct simdjson_result<dom::document_stream>;
  friend struct internal::simdjson_result_base<dom::document_stream>;

}; // class document_stream

} // namespace dom

template<>
struct simdjson_result<dom::document_stream> : public internal::simdjson_result_base<dom::document_stream> {
public:
  simdjson_really_inline simdjson_result() noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result(dom::document_stream &&value) noexcept; ///< @private

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline dom::document_stream::iterator begin() noexcept(false);
  simdjson_really_inline dom::document_stream::iterator end() noexcept(false);
#else // SIMDJSON_EXCEPTIONS
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
  [[deprecated("parse_many() and load_many() may return errors. Use document_stream stream; error = parser.parse_many().get(doc); instead.")]]
  simdjson_really_inline dom::document_stream::iterator begin() noexcept;
  [[deprecated("parse_many() and load_many() may return errors. Use document_stream stream; error = parser.parse_many().get(doc); instead.")]]
  simdjson_really_inline dom::document_stream::iterator end() noexcept;
#endif // SIMDJSON_DISABLE_DEPRECATED_API
#endif // SIMDJSON_EXCEPTIONS
}; // struct simdjson_result<dom::document_stream>

} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_STREAM_H
/* end file include/simdjson/dom/document_stream.h */
/* begin file include/simdjson/dom/element.h */
#ifndef SIMDJSON_DOM_ELEMENT_H
#define SIMDJSON_DOM_ELEMENT_H

#include <ostream>

namespace simdjson {
namespace internal {
template<typename T>
class string_builder;
}
namespace dom {
class array;
class document;
class object;

/**
 * The actual concrete type of a JSON element
 * This is the type it is most easily cast to with get<>.
 */
enum class element_type {
  ARRAY = '[',     ///< dom::array
  OBJECT = '{',    ///< dom::object
  INT64 = 'l',     ///< int64_t
  UINT64 = 'u',    ///< uint64_t: any integer that fits in uint64_t but *not* int64_t
  DOUBLE = 'd',    ///< double: Any number with a "." or "e" that fits in double.
  STRING = '"',    ///< std::string_view
  BOOL = 't',      ///< bool
  NULL_VALUE = 'n' ///< null
};

/**
 * A JSON element.
 *
 * References an element in a JSON document, representing a JSON null, boolean, string, number,
 * array or object.
 */
class element {
public:
  /** Create a new, invalid element. */
  simdjson_really_inline element() noexcept;

  /** The type of this element. */
  simdjson_really_inline element_type type() const noexcept;

  /**
   * Cast this element to an array.
   *
   * @returns An object that can be used to iterate the array, or:
   *          INCORRECT_TYPE if the JSON element is not an array.
   */
  inline simdjson_result<array> get_array() const noexcept;
  /**
   * Cast this element to an object.
   *
   * @returns An object that can be used to look up or iterate the object's fields, or:
   *          INCORRECT_TYPE if the JSON element is not an object.
   */
  inline simdjson_result<object> get_object() const noexcept;
  /**
   * Cast this element to a null-terminated C string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * The length of the string is given by get_string_length(). Because JSON strings
   * may contain null characters, it may be incorrect to use strlen to determine the
   * string length.
   *
   * It is possible to get a single string_view instance which represents both the string
   * content and its length: see get_string().
   *
   * @returns A pointer to a null-terminated UTF-8 string. This string is stored in the parser and will
   *          be invalidated the next time it parses a document or when it is destroyed.
   *          Returns INCORRECT_TYPE if the JSON element is not a string.
   */
  inline simdjson_result<const char *> get_c_str() const noexcept;
  /**
   * Gives the length in bytes of the string.
   *
   * It is possible to get a single string_view instance which represents both the string
   * content and its length: see get_string().
   *
   * @returns A string length in bytes.
   *          Returns INCORRECT_TYPE if the JSON element is not a string.
   */
  inline simdjson_result<size_t> get_string_length() const noexcept;
  /**
   * Cast this element to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next time it
   *          parses a document or when it is destroyed.
   *          Returns INCORRECT_TYPE if the JSON element is not a string.
   */
  inline simdjson_result<std::string_view> get_string() const noexcept;
  /**
   * Cast this element to a signed integer.
   *
   * @returns A signed 64-bit integer.
   *          Returns INCORRECT_TYPE if the JSON element is not an integer, or NUMBER_OUT_OF_RANGE
   *          if it is negative.
   */
  inline simdjson_result<int64_t> get_int64() const noexcept;
  /**
   * Cast this element to an unsigned integer.
   *
   * @returns An unsigned 64-bit integer.
   *          Returns INCORRECT_TYPE if the JSON element is not an integer, or NUMBER_OUT_OF_RANGE
   *          if it is too large.
   */
  inline simdjson_result<uint64_t> get_uint64() const noexcept;
  /**
   * Cast this element to a double floating-point.
   *
   * @returns A double value.
   *          Returns INCORRECT_TYPE if the JSON element is not a number.
   */
  inline simdjson_result<double> get_double() const noexcept;
  /**
   * Cast this element to a bool.
   *
   * @returns A bool value.
   *          Returns INCORRECT_TYPE if the JSON element is not a boolean.
   */
  inline simdjson_result<bool> get_bool() const noexcept;

  /**
   * Whether this element is a json array.
   *
   * Equivalent to is<array>().
   */
  inline bool is_array() const noexcept;
  /**
   * Whether this element is a json object.
   *
   * Equivalent to is<object>().
   */
  inline bool is_object() const noexcept;
  /**
   * Whether this element is a json string.
   *
   * Equivalent to is<std::string_view>() or is<const char *>().
   */
  inline bool is_string() const noexcept;
  /**
   * Whether this element is a json number that fits in a signed 64-bit integer.
   *
   * Equivalent to is<int64_t>().
   */
  inline bool is_int64() const noexcept;
  /**
   * Whether this element is a json number that fits in an unsigned 64-bit integer.
   *
   * Equivalent to is<uint64_t>().
   */
  inline bool is_uint64() const noexcept;
  /**
   * Whether this element is a json number that fits in a double.
   *
   * Equivalent to is<double>().
   */
  inline bool is_double() const noexcept;

  /**
   * Whether this element is a json number.
   *
   * Both integers and floating points will return true.
   */
  inline bool is_number() const noexcept;

  /**
   * Whether this element is a json `true` or `false`.
   *
   * Equivalent to is<bool>().
   */
  inline bool is_bool() const noexcept;
  /**
   * Whether this element is a json `null`.
   */
  inline bool is_null() const noexcept;

  /**
   * Tell whether the value can be cast to provided type (T).
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: dom::array
   * - Object: dom::object
   *
   * @tparam T bool, double, uint64_t, int64_t, std::string_view, const char *, dom::array, dom::object
   */
  template<typename T>
  simdjson_really_inline bool is() const noexcept;

  /**
   * Get the value as the provided type (T).
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: dom::array
   * - Object: dom::object
   *
   * You may use get_double(), get_bool(), get_uint64(), get_int64(),
   * get_object(), get_array() or get_string() instead.
   *
   * @tparam T bool, double, uint64_t, int64_t, std::string_view, const char *, dom::array, dom::object
   *
   * @returns The value cast to the given type, or:
   *          INCORRECT_TYPE if the value cannot be cast to the given type.
   */

  template<typename T>
  inline simdjson_result<T> get() const noexcept {
    // Unless the simdjson library provides an inline implementation, calling this method should
    // immediately fail.
    static_assert(!sizeof(T), "The get method with given type is not implemented by the simdjson library.");
  }

  /**
   * Get the value as the provided type (T).
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: dom::array
   * - Object: dom::object
   *
   * @tparam T bool, double, uint64_t, int64_t, std::string_view, const char *, dom::array, dom::object
   *
   * @param value The variable to set to the value. May not be set if there is an error.
   *
   * @returns The error that occurred, or SUCCESS if there was no error.
   */
  template<typename T>
  simdjson_warn_unused simdjson_really_inline error_code get(T &value) const noexcept;

  /**
   * Get the value as the provided type (T), setting error if it's not the given type.
   *
   * Supported types:
   * - Boolean: bool
   * - Number: double, uint64_t, int64_t
   * - String: std::string_view, const char *
   * - Array: dom::array
   * - Object: dom::object
   *
   * @tparam T bool, double, uint64_t, int64_t, std::string_view, const char *, dom::array, dom::object
   *
   * @param value The variable to set to the given type. value is undefined if there is an error.
   * @param error The variable to store the error. error is set to error_code::SUCCEED if there is an error.
   */
  template<typename T>
  inline void tie(T &value, error_code &error) && noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Read this element as a boolean.
   *
   * @return The boolean value
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a boolean.
   */
  inline operator bool() const noexcept(false);

  /**
   * Read this element as a null-terminated UTF-8 string.
   *
   * Be mindful that JSON allows strings to contain null characters.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return The string value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a string.
   */
  inline explicit operator const char*() const noexcept(false);

  /**
   * Read this element as a null-terminated UTF-8 string.
   *
   * Does *not* convert other types to a string; requires that the JSON type of the element was
   * an actual string.
   *
   * @return The string value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a string.
   */
  inline operator std::string_view() const noexcept(false);

  /**
   * Read this element as an unsigned integer.
   *
   * @return The integer value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an integer
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits or is negative
   */
  inline operator uint64_t() const noexcept(false);
  /**
   * Read this element as an signed integer.
   *
   * @return The integer value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an integer
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits
   */
  inline operator int64_t() const noexcept(false);
  /**
   * Read this element as an double.
   *
   * @return The double value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not a number
   * @exception simdjson_error(NUMBER_OUT_OF_RANGE) if the integer doesn't fit in 64 bits or is negative
   */
  inline operator double() const noexcept(false);
  /**
   * Read this element as a JSON array.
   *
   * @return The JSON array.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an array
   */
  inline operator array() const noexcept(false);
  /**
   * Read this element as a JSON object (key/value pairs).
   *
   * @return The JSON object.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an object
   */
  inline operator object() const noexcept(false);

  /**
   * Iterate over each element in this array.
   *
   * @return The beginning of the iteration.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an array
   */
  inline dom::array::iterator begin() const noexcept(false);

  /**
   * Iterate over each element in this array.
   *
   * @return The end of the iteration.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON element is not an array
   */
  inline dom::array::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get_uint64().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get_uint64().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const char *key) const noexcept;

  /**
   * Get the value associated with the given JSON pointer.  We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard.
   *
   *   dom::parser parser;
   *   element doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded);
   *   doc.at_pointer("/foo/a/1") == 20
   *   doc.at_pointer("/foo")["a"].at(1) == 20
   *   doc.at_pointer("")["foo"]["a"].at(1) == 20
   *
   * It is allowed for a key to be the empty string:
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "": { "a": [ 10, 20, 30 ] }})"_padded);
   *   obj.at_pointer("//a/1") == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at_pointer(const std::string_view json_pointer) const noexcept;

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
  /**
   *
   * Version 0.4 of simdjson used an incorrect interpretation of the JSON Pointer standard
   * and allowed the following :
   *
   *   dom::parser parser;
   *   element doc = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded);
   *   doc.at("foo/a/1") == 20
   *
   * Though it is intuitive, it is not compliant with RFC 6901
   * https://tools.ietf.org/html/rfc6901
   *
   * For standard compliance, use the at_pointer function instead.
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  [[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
  inline simdjson_result<element> at(const std::string_view json_pointer) const noexcept;
#endif // SIMDJSON_DISABLE_DEPRECATED_API

  /**
   * Get the value at the given index.
   *
   * @return The value at the given index, or:
   *         - INDEX_OUT_OF_BOUNDS if the array index is larger than an array length
   */
  inline simdjson_result<element> at(size_t index) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get_uint64().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key(std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key in a case-insensitive manner.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key_case_insensitive(std::string_view key) const noexcept;

  /** @private for debugging. Prints out the root element. */
  inline bool dump_raw_tape(std::ostream &out) const noexcept;

private:
  simdjson_really_inline element(const internal::tape_ref &tape) noexcept;
  internal::tape_ref tape;
  friend class document;
  friend class object;
  friend class array;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::internal::string_builder;

};

} // namespace dom

/** The result of a JSON navigation that may fail. */
template<>
struct simdjson_result<dom::element> : public internal::simdjson_result_base<dom::element> {
public:
  simdjson_really_inline simdjson_result() noexcept; ///< @private
  simdjson_really_inline simdjson_result(dom::element &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result<dom::element_type> type() const noexcept;
  template<typename T>
  simdjson_really_inline bool is() const noexcept;
  template<typename T>
  simdjson_really_inline simdjson_result<T> get() const noexcept;
  template<typename T>
  simdjson_warn_unused simdjson_really_inline error_code get(T &value) const noexcept;

  simdjson_really_inline simdjson_result<dom::array> get_array() const noexcept;
  simdjson_really_inline simdjson_result<dom::object> get_object() const noexcept;
  simdjson_really_inline simdjson_result<const char *> get_c_str() const noexcept;
  simdjson_really_inline simdjson_result<size_t> get_string_length() const noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() const noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() const noexcept;
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() const noexcept;
  simdjson_really_inline simdjson_result<double> get_double() const noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() const noexcept;

  simdjson_really_inline bool is_array() const noexcept;
  simdjson_really_inline bool is_object() const noexcept;
  simdjson_really_inline bool is_string() const noexcept;
  simdjson_really_inline bool is_int64() const noexcept;
  simdjson_really_inline bool is_uint64() const noexcept;
  simdjson_really_inline bool is_double() const noexcept;
  simdjson_really_inline bool is_number() const noexcept;
  simdjson_really_inline bool is_bool() const noexcept;
  simdjson_really_inline bool is_null() const noexcept;

  simdjson_really_inline simdjson_result<dom::element> operator[](std::string_view key) const noexcept;
  simdjson_really_inline simdjson_result<dom::element> operator[](const char *key) const noexcept;
  simdjson_really_inline simdjson_result<dom::element> at_pointer(const std::string_view json_pointer) const noexcept;
  [[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
  simdjson_really_inline simdjson_result<dom::element> at(const std::string_view json_pointer) const noexcept;
  simdjson_really_inline simdjson_result<dom::element> at(size_t index) const noexcept;
  simdjson_really_inline simdjson_result<dom::element> at_key(std::string_view key) const noexcept;
  simdjson_really_inline simdjson_result<dom::element> at_key_case_insensitive(std::string_view key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator bool() const noexcept(false);
  simdjson_really_inline explicit operator const char*() const noexcept(false);
  simdjson_really_inline operator std::string_view() const noexcept(false);
  simdjson_really_inline operator uint64_t() const noexcept(false);
  simdjson_really_inline operator int64_t() const noexcept(false);
  simdjson_really_inline operator double() const noexcept(false);
  simdjson_really_inline operator dom::array() const noexcept(false);
  simdjson_really_inline operator dom::object() const noexcept(false);

  simdjson_really_inline dom::array::iterator begin() const noexcept(false);
  simdjson_really_inline dom::array::iterator end() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};


} // namespace simdjson

#endif // SIMDJSON_DOM_DOCUMENT_H
/* end file include/simdjson/dom/element.h */
/* begin file include/simdjson/dom/object.h */
#ifndef SIMDJSON_DOM_OBJECT_H
#define SIMDJSON_DOM_OBJECT_H


namespace simdjson {
namespace internal {
template<typename T>
class string_builder;
}
namespace dom {

class document;
class element;
class key_value_pair;

/**
 * JSON object.
 */
class object {
public:
  /** Create a new, invalid object */
  simdjson_really_inline object() noexcept;

  class iterator {
  public:
    using value_type = key_value_pair;
    using difference_type = std::ptrdiff_t;

    /**
     * Get the actual key/value pair
     */
    inline const value_type operator*() const noexcept;
    /**
     * Get the next key/value pair.
     *
     * Part of the std::iterator interface.
     *
     */
    inline iterator& operator++() noexcept;
    /**
     * Get the next key/value pair.
     *
     * Part of the std::iterator interface.
     *
     */
    inline iterator operator++(int) noexcept;
    /**
     * Check if these values come from the same place in the JSON.
     *
     * Part of the std::iterator interface.
     */
    inline bool operator!=(const iterator& other) const noexcept;
    inline bool operator==(const iterator& other) const noexcept;

    inline bool operator<(const iterator& other) const noexcept;
    inline bool operator<=(const iterator& other) const noexcept;
    inline bool operator>=(const iterator& other) const noexcept;
    inline bool operator>(const iterator& other) const noexcept;
    /**
     * Get the key of this key/value pair.
     */
    inline std::string_view key() const noexcept;
    /**
     * Get the length (in bytes) of the key in this key/value pair.
     * You should expect this function to be faster than key().size().
     */
    inline uint32_t key_length() const noexcept;
    /**
     * Returns true if the key in this key/value pair is equal
     * to the provided string_view.
     */
    inline bool key_equals(std::string_view o) const noexcept;
    /**
     * Returns true if the key in this key/value pair is equal
     * to the provided string_view in a case-insensitive manner.
     * Case comparisons may only be handled correctly for ASCII strings.
     */
    inline bool key_equals_case_insensitive(std::string_view o) const noexcept;
    /**
     * Get the key of this key/value pair.
     */
    inline const char *key_c_str() const noexcept;
    /**
     * Get the value of this key/value pair.
     */
    inline element value() const noexcept;

    iterator() noexcept = default;
    iterator(const iterator&) noexcept = default;
    iterator& operator=(const iterator&) noexcept = default;
  private:
    simdjson_really_inline iterator(const internal::tape_ref &tape) noexcept;

    internal::tape_ref tape;

    friend class object;
  };

  /**
   * Return the first key/value pair.
   *
   * Part of the std::iterable interface.
   */
  inline iterator begin() const noexcept;
  /**
   * One past the last key/value pair.
   *
   * Part of the std::iterable interface.
   */
  inline iterator end() const noexcept;
  /**
   * Get the size of the object (number of keys).
   * It is a saturated value with a maximum of 0xFFFFFF: if the value
   * is 0xFFFFFF then the size is 0xFFFFFF or greater.
   */
  inline size_t size() const noexcept;
  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get_uint64().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get_uint64().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   *         - INCORRECT_TYPE if this is not an object
   */
  inline simdjson_result<element> operator[](const char *key) const noexcept;

  /**
   * Get the value associated with the given JSON pointer. We use the RFC 6901
   * https://tools.ietf.org/html/rfc6901 standard, interpreting the current node
   * as the root of its own JSON document.
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "foo": { "a": [ 10, 20, 30 ] }})"_padded);
   *   obj.at_pointer("/foo/a/1") == 20
   *   obj.at_pointer("/foo")["a"].at(1) == 20
   *
   * It is allowed for a key to be the empty string:
   *
   *   dom::parser parser;
   *   object obj = parser.parse(R"({ "": { "a": [ 10, 20, 30 ] }})"_padded);
   *   obj.at_pointer("//a/1") == 20
   *   obj.at_pointer("/")["a"].at(1) == 20
   *
   * @return The value associated with the given JSON pointer, or:
   *         - NO_SUCH_FIELD if a field does not exist in an object
   *         - INDEX_OUT_OF_BOUNDS if an array index is larger than an array length
   *         - INCORRECT_TYPE if a non-integer is used to access an array
   *         - INVALID_JSON_POINTER if the JSON pointer is invalid and cannot be parsed
   */
  inline simdjson_result<element> at_pointer(std::string_view json_pointer) const noexcept;

  /**
   * Get the value associated with the given key.
   *
   * The key will be matched against **unescaped** JSON:
   *
   *   dom::parser parser;
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\n"].get_uint64().first == 1
   *   parser.parse(R"({ "a\n": 1 })"_padded)["a\\n"].get_uint64().error() == NO_SUCH_FIELD
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key(std::string_view key) const noexcept;

  /**
   * Get the value associated with the given key in a case-insensitive manner.
   * It is only guaranteed to work over ASCII inputs.
   *
   * Note: The key will be matched against **unescaped** JSON.
   *
   * This function has linear-time complexity: the keys are checked one by one.
   *
   * @return The value associated with this field, or:
   *         - NO_SUCH_FIELD if the field does not exist in the object
   */
  inline simdjson_result<element> at_key_case_insensitive(std::string_view key) const noexcept;

private:
  simdjson_really_inline object(const internal::tape_ref &tape) noexcept;

  internal::tape_ref tape;

  friend class element;
  friend struct simdjson_result<element>;
  template<typename T>
  friend class simdjson::internal::string_builder;
};

/**
 * Key/value pair in an object.
 */
class key_value_pair {
public:
  /** key in the key-value pair **/
  std::string_view key;
  /** value in the key-value pair **/
  element value;

private:
  simdjson_really_inline key_value_pair(std::string_view _key, element _value) noexcept;
  friend class object;
};

} // namespace dom

/** The result of a JSON conversion that may fail. */
template<>
struct simdjson_result<dom::object> : public internal::simdjson_result_base<dom::object> {
public:
  simdjson_really_inline simdjson_result() noexcept; ///< @private
  simdjson_really_inline simdjson_result(dom::object value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  inline simdjson_result<dom::element> operator[](std::string_view key) const noexcept;
  inline simdjson_result<dom::element> operator[](const char *key) const noexcept;
  inline simdjson_result<dom::element> at_pointer(std::string_view json_pointer) const noexcept;
  inline simdjson_result<dom::element> at_key(std::string_view key) const noexcept;
  inline simdjson_result<dom::element> at_key_case_insensitive(std::string_view key) const noexcept;

#if SIMDJSON_EXCEPTIONS
  inline dom::object::iterator begin() const noexcept(false);
  inline dom::object::iterator end() const noexcept(false);
  inline size_t size() const noexcept(false);
#endif // SIMDJSON_EXCEPTIONS
};

} // namespace simdjson

#if defined(__cpp_lib_ranges)
#include <ranges>

namespace std {
namespace ranges {
template<>
inline constexpr bool enable_view<simdjson::dom::object> = true;
#if SIMDJSON_EXCEPTIONS
template<>
inline constexpr bool enable_view<simdjson::simdjson_result<simdjson::dom::object>> = true;
#endif // SIMDJSON_EXCEPTIONS
} // namespace ranges
} // namespace std
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_DOM_OBJECT_H
/* end file include/simdjson/dom/object.h */
/* begin file include/simdjson/dom/serialization.h */
#ifndef SIMDJSON_SERIALIZATION_H
#define SIMDJSON_SERIALIZATION_H

#include <vector>

namespace simdjson {

/**
 * The string_builder template and mini_formatter class
 * are not part of  our public API and are subject to change
 * at any time!
 */
namespace internal {

class mini_formatter;

/**
 * @private The string_builder template allows us to construct
 * a string from a document element. It is parametrized
 * by a "formatter" which handles the details. Thus
 * the string_builder template could support both minification
 * and prettification, and various other tradeoffs.
 */
template <class formatter = mini_formatter>
class string_builder {
public:
  /** Construct an initially empty builder, would print the empty string **/
  string_builder() = default;
  /** Append an element to the builder (to be printed) **/
  inline void append(simdjson::dom::element value);
  /** Append an array to the builder (to be printed) **/
  inline void append(simdjson::dom::array value);
  /** Append an objet to the builder (to be printed) **/
  inline void append(simdjson::dom::object value);
  /** Reset the builder (so that it would print the empty string) **/
  simdjson_really_inline void clear();
  /**
   * Get access to the string. The string_view is owned by the builder
   * and it is invalid to use it after the string_builder has been
   * destroyed.
   * However you can make a copy of the string_view on memory that you
   * own.
   */
  simdjson_really_inline std::string_view str() const;
  /** Append a key_value_pair to the builder (to be printed) **/
  simdjson_really_inline void append(simdjson::dom::key_value_pair value);
private:
  formatter format{};
};

/**
 * @private This is the class that we expect to use with the string_builder
 * template. It tries to produce a compact version of the JSON element
 * as quickly as possible.
 */
class mini_formatter {
public:
  mini_formatter() = default;
  /** Add a comma **/
  simdjson_really_inline void comma();
  /** Start an array, prints [ **/
  simdjson_really_inline void start_array();
  /** End an array, prints ] **/
  simdjson_really_inline void end_array();
  /** Start an array, prints { **/
  simdjson_really_inline void start_object();
  /** Start an array, prints } **/
  simdjson_really_inline void end_object();
  /** Prints a true **/
  simdjson_really_inline void true_atom();
  /** Prints a false **/
  simdjson_really_inline void false_atom();
  /** Prints a null **/
  simdjson_really_inline void null_atom();
  /** Prints a number **/
  simdjson_really_inline void number(int64_t x);
  /** Prints a number **/
  simdjson_really_inline void number(uint64_t x);
  /** Prints a number **/
  simdjson_really_inline void number(double x);
  /** Prints a key (string + colon) **/
  simdjson_really_inline void key(std::string_view unescaped);
  /** Prints a string. The string is escaped as needed. **/
  simdjson_really_inline void string(std::string_view unescaped);
  /** Clears out the content. **/
  simdjson_really_inline void clear();
  /**
   * Get access to the buffer, it is own by the instance, but
   * the user can make a copy.
   **/
  simdjson_really_inline std::string_view str() const;

private:
  // implementation details (subject to change)
  /** Prints one character **/
  simdjson_really_inline void one_char(char c);
  /** Backing buffer **/
  std::vector<char> buffer{}; // not ideal!
};

} // internal

namespace dom {

/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The element.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::dom::element value) {
    simdjson::internal::string_builder<> sb;
    sb.append(value);
    return (out << sb.str());
}
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::dom::element> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The array.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::dom::array value)  {
    simdjson::internal::string_builder<> sb;
    sb.append(value);
    return (out << sb.str());
}
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::dom::array> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The objet.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::dom::object value)   {
    simdjson::internal::string_builder<> sb;
    sb.append(value);
    return (out << sb.str());
}
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out,  simdjson::simdjson_result<simdjson::dom::object> x) {
    if (x.error()) { throw  simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#endif
} // namespace dom

/**
 * Converts JSON to a string.
 *
 *   dom::parser parser;
 *   element doc = parser.parse("   [ 1 , 2 , 3 ] "_padded);
 *   cout << to_string(doc) << endl; // prints [1,2,3]
 *
 */
template <class T>
std::string to_string(T x)   {
    // in C++, to_string is standard: http://www.cplusplus.com/reference/string/to_string/
    // Currently minify and to_string are identical but in the future, they may
    // differ.
    simdjson::internal::string_builder<> sb;
    sb.append(x);
    std::string_view answer = sb.str();
    return std::string(answer.data(), answer.size());
}
#if SIMDJSON_EXCEPTIONS
template <class T>
std::string to_string(simdjson_result<T> x) {
    if (x.error()) { throw simdjson_error(x.error()); }
    return to_string(x.value());
}
#endif

/**
 * Minifies a JSON element or document, printing the smallest possible valid JSON.
 *
 *   dom::parser parser;
 *   element doc = parser.parse("   [ 1 , 2 , 3 ] "_padded);
 *   cout << minify(doc) << endl; // prints [1,2,3]
 *
 */
template <class T>
std::string minify(T x)  {
  return to_string(x);
}

#if SIMDJSON_EXCEPTIONS
template <class T>
std::string minify(simdjson_result<T> x) {
    if (x.error()) { throw simdjson_error(x.error()); }
    return to_string(x.value());
}
#endif


} // namespace simdjson


#endif
/* end file include/simdjson/dom/serialization.h */

// Deprecated API
/* begin file include/simdjson/dom/jsonparser.h */
// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_DOM_JSONPARSER_H
#define SIMDJSON_DOM_JSONPARSER_H

/* begin file include/simdjson/dom/parsedjson.h */
// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_DOM_PARSEDJSON_H
#define SIMDJSON_DOM_PARSEDJSON_H


namespace simdjson {

/**
 * @deprecated Use `dom::parser` instead.
 */
using ParsedJson [[deprecated("Use dom::parser instead")]] = dom::parser;

} // namespace simdjson

#endif // SIMDJSON_DOM_PARSEDJSON_H
/* end file include/simdjson/dom/parsedjson.h */
/* begin file include/simdjson/jsonioutil.h */
#ifndef SIMDJSON_JSONIOUTIL_H
#define SIMDJSON_JSONIOUTIL_H


namespace simdjson {

#if SIMDJSON_EXCEPTIONS
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
[[deprecated("Use padded_string::load() instead")]]
inline padded_string get_corpus(const char *path) {
  return padded_string::load(path);
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API
#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_JSONIOUTIL_H
/* end file include/simdjson/jsonioutil.h */

namespace simdjson {

//
// C API (json_parse and build_parsed_json) declarations
//

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const uint8_t *buf, size_t len, dom::parser &parser, bool realloc_if_needed = true) noexcept {
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}
[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const char *buf, size_t len, dom::parser &parser, bool realloc_if_needed = true) noexcept {
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}
[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const std::string &s, dom::parser &parser, bool realloc_if_needed = true) noexcept {
  error_code code = parser.parse(s.data(), s.length(), realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}
[[deprecated("Use parser.parse() instead")]]
inline int json_parse(const padded_string &s, dom::parser &parser) noexcept {
  error_code code = parser.parse(s).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return code;
}

[[deprecated("Use parser.parse() instead")]]
simdjson_warn_unused inline dom::parser build_parsed_json(const uint8_t *buf, size_t len, bool realloc_if_needed = true) noexcept {
  dom::parser parser;
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}
[[deprecated("Use parser.parse() instead")]]
simdjson_warn_unused inline dom::parser build_parsed_json(const char *buf, size_t len, bool realloc_if_needed = true) noexcept {
  dom::parser parser;
  error_code code = parser.parse(buf, len, realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}
[[deprecated("Use parser.parse() instead")]]
simdjson_warn_unused inline dom::parser build_parsed_json(const std::string &s, bool realloc_if_needed = true) noexcept {
  dom::parser parser;
  error_code code = parser.parse(s.data(), s.length(), realloc_if_needed).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}
[[deprecated("Use parser.parse() instead")]]
simdjson_warn_unused inline dom::parser build_parsed_json(const padded_string &s) noexcept {
  dom::parser parser;
  error_code code = parser.parse(s).error();
  // The deprecated json_parse API is a signal that the user plans to *use* the error code / valid
  // bits in the parser instead of heeding the result code. The normal parser unsets those in
  // anticipation of making the error code ephemeral.
  // Here we put the code back into the parser, until we've removed this method.
  parser.valid = code == SUCCESS;
  parser.error = code;
  return parser;
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

/** @private We do not want to allow implicit conversion from C string to std::string. */
int json_parse(const char *buf, dom::parser &parser) noexcept = delete;
/** @private We do not want to allow implicit conversion from C string to std::string. */
dom::parser build_parsed_json(const char *buf) noexcept = delete;

} // namespace simdjson

#endif // SIMDJSON_DOM_JSONPARSER_H
/* end file include/simdjson/dom/jsonparser.h */
/* begin file include/simdjson/dom/parsedjson_iterator.h */
// TODO Remove this -- deprecated API and files

#ifndef SIMDJSON_DOM_PARSEDJSON_ITERATOR_H
#define SIMDJSON_DOM_PARSEDJSON_ITERATOR_H

#include <cstring>
#include <string>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

/* begin file include/simdjson/internal/jsonformatutils.h */
#ifndef SIMDJSON_INTERNAL_JSONFORMATUTILS_H
#define SIMDJSON_INTERNAL_JSONFORMATUTILS_H

#include <iomanip>
#include <iostream>
#include <sstream>

namespace simdjson {
namespace internal {

class escape_json_string;

inline std::ostream& operator<<(std::ostream& out, const escape_json_string &str);

class escape_json_string {
public:
  escape_json_string(std::string_view _str) noexcept : str{_str} {}
  operator std::string() const noexcept { std::stringstream s; s << *this; return s.str(); }
private:
  std::string_view str;
  friend std::ostream& operator<<(std::ostream& out, const escape_json_string &unescaped);
};

inline std::ostream& operator<<(std::ostream& out, const escape_json_string &unescaped) {
  for (size_t i=0; i<unescaped.str.length(); i++) {
    switch (unescaped.str[i]) {
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\"':
      out << "\\\"";
      break;
    case '\t':
      out << "\\t";
      break;
    case '\\':
      out << "\\\\";
      break;
    default:
      if (static_cast<unsigned char>(unescaped.str[i]) <= 0x1F) {
        // TODO can this be done once at the beginning, or will it mess up << char?
        std::ios::fmtflags f(out.flags());
        out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(unescaped.str[i]);
        out.flags(f);
      } else {
        out << unescaped.str[i];
      }
    }
  }
  return out;
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JSONFORMATUTILS_H
/* end file include/simdjson/internal/jsonformatutils.h */

#ifndef SIMDJSON_DISABLE_DEPRECATED_API

namespace simdjson {
/** @private **/
class [[deprecated("Use the new DOM navigation API instead (see doc/basics.md)")]] dom::parser::Iterator {
public:
  inline Iterator(const dom::parser &parser) noexcept(false);
  inline Iterator(const Iterator &o) noexcept;
  inline ~Iterator() noexcept;

  inline Iterator& operator=(const Iterator&) = delete;

  inline bool is_ok() const;

  // useful for debugging purposes
  inline size_t get_tape_location() const;

  // useful for debugging purposes
  inline size_t get_tape_length() const;

  // returns the current depth (start at 1 with 0 reserved for the fictitious
  // root node)
  inline size_t get_depth() const;

  // A scope is a series of nodes at the same depth, typically it is either an
  // object ({) or an array ([). The root node has type 'r'.
  inline uint8_t get_scope_type() const;

  // move forward in document order
  inline bool move_forward();

  // retrieve the character code of what we're looking at:
  // [{"slutfn are the possibilities
  inline uint8_t get_type() const {
      return current_type; // short functions should be inlined!
  }

  // get the int64_t value at this node; valid only if get_type is "l"
  inline int64_t get_integer() const {
      if (location + 1 >= tape_length) {
      return 0; // default value in case of error
      }
      return static_cast<int64_t>(doc.tape[location + 1]);
  }

  // get the value as uint64; valid only if  if get_type is "u"
  inline uint64_t get_unsigned_integer() const {
      if (location + 1 >= tape_length) {
      return 0; // default value in case of error
      }
      return doc.tape[location + 1];
  }

  // get the string value at this node (NULL ended); valid only if get_type is "
  // note that tabs, and line endings are escaped in the returned value (see
  // print_with_escapes) return value is valid UTF-8, it may contain NULL chars
  // within the string: get_string_length determines the true string length.
  inline const char *get_string() const {
      return reinterpret_cast<const char *>(
          doc.string_buf.get() + (current_val & internal::JSON_VALUE_MASK) + sizeof(uint32_t));
  }

  // return the length of the string in bytes
  inline uint32_t get_string_length() const {
      uint32_t answer;
      std::memcpy(&answer,
          reinterpret_cast<const char *>(doc.string_buf.get() +
                                          (current_val & internal::JSON_VALUE_MASK)),
          sizeof(uint32_t));
      return answer;
  }

  // get the double value at this node; valid only if
  // get_type() is "d"
  inline double get_double() const {
      if (location + 1 >= tape_length) {
      return std::numeric_limits<double>::quiet_NaN(); // default value in
                                                      // case of error
      }
      double answer;
      std::memcpy(&answer, &doc.tape[location + 1], sizeof(answer));
      return answer;
  }

  inline bool is_object_or_array() const { return is_object() || is_array(); }

  inline bool is_object() const { return get_type() == '{'; }

  inline bool is_array() const { return get_type() == '['; }

  inline bool is_string() const { return get_type() == '"'; }

  // Returns true if the current type of the node is an signed integer.
  // You can get its value with `get_integer()`.
  inline bool is_integer() const { return get_type() == 'l'; }

  // Returns true if the current type of the node is an unsigned integer.
  // You can get its value with `get_unsigned_integer()`.
  //
  // NOTE:
  // Only a large value, which is out of range of a 64-bit signed integer, is
  // represented internally as an unsigned node. On the other hand, a typical
  // positive integer, such as 1, 42, or 1000000, is as a signed node.
  // Be aware this function returns false for a signed node.
  inline bool is_unsigned_integer() const { return get_type() == 'u'; }
  // Returns true if the current type of the node is a double floating-point number.
  inline bool is_double() const { return get_type() == 'd'; }
  // Returns true if the current type of the node is a number (integer or floating-point).
  inline bool is_number() const {
      return is_integer() || is_unsigned_integer() || is_double();
  }
  // Returns true if the current type of the node is a bool with true value.
  inline bool is_true() const { return get_type() == 't'; }
  // Returns true if the current type of the node is a bool with false value.
  inline bool is_false() const { return get_type() == 'f'; }
  // Returns true if the current type of the node is null.
  inline bool is_null() const { return get_type() == 'n'; }
  // Returns true if the type byte represents an object of an array
  static bool is_object_or_array(uint8_t type) {
      return ((type == '[') || (type == '{'));
  }

  // when at {, go one level deep, looking for a given key
  // if successful, we are left pointing at the value,
  // if not, we are still pointing at the object ({)
  // (in case of repeated keys, this only finds the first one).
  // We seek the key using C's strcmp so if your JSON strings contain
  // NULL chars, this would trigger a false positive: if you expect that
  // to be the case, take extra precautions.
  // Furthermore, we do the comparison character-by-character
  // without taking into account Unicode equivalence.
  inline bool move_to_key(const char *key);

  // as above, but case insensitive lookup (strcmpi instead of strcmp)
  inline bool move_to_key_insensitive(const char *key);

  // when at {, go one level deep, looking for a given key
  // if successful, we are left pointing at the value,
  // if not, we are still pointing at the object ({)
  // (in case of repeated keys, this only finds the first one).
  // The string we search for can contain NULL values.
  // Furthermore, we do the comparison character-by-character
  // without taking into account Unicode equivalence.
  inline bool move_to_key(const char *key, uint32_t length);

  // when at a key location within an object, this moves to the accompanying
  // value (located next to it). This is equivalent but much faster than
  // calling "next()".
  inline void move_to_value();

  // when at [, go one level deep, and advance to the given index.
  // if successful, we are left pointing at the value,
  // if not, we are still pointing at the array ([)
  inline bool move_to_index(uint32_t index);

  // Moves the iterator to the value corresponding to the json pointer.
  // Always search from the root of the document.
  // if successful, we are left pointing at the value,
  // if not, we are still pointing the same value we were pointing before the
  // call. The json pointer follows the rfc6901 standard's syntax:
  // https://tools.ietf.org/html/rfc6901 However, the standard says "If a
  // referenced member name is not unique in an object, the member that is
  // referenced is undefined, and evaluation fails". Here we just return the
  // first corresponding value. The length parameter is the length of the
  // jsonpointer string ('pointer').
  inline bool move_to(const char *pointer, uint32_t length);

  // Moves the iterator to the value corresponding to the json pointer.
  // Always search from the root of the document.
  // if successful, we are left pointing at the value,
  // if not, we are still pointing the same value we were pointing before the
  // call. The json pointer implementation follows the rfc6901 standard's
  // syntax: https://tools.ietf.org/html/rfc6901 However, the standard says
  // "If a referenced member name is not unique in an object, the member that
  // is referenced is undefined, and evaluation fails". Here we just return
  // the first corresponding value.
  inline bool move_to(const std::string &pointer) {
      return move_to(pointer.c_str(), uint32_t(pointer.length()));
  }

  private:
  // Almost the same as move_to(), except it searches from the current
  // position. The pointer's syntax is identical, though that case is not
  // handled by the rfc6901 standard. The '/' is still required at the
  // beginning. However, contrary to move_to(), the URI Fragment Identifier
  // Representation is not supported here. Also, in case of failure, we are
  // left pointing at the closest value it could reach. For these reasons it
  // is private. It exists because it is used by move_to().
  inline bool relative_move_to(const char *pointer, uint32_t length);

  public:
  // throughout return true if we can do the navigation, false
  // otherwise

  // Withing a given scope (series of nodes at the same depth within either an
  // array or an object), we move forward.
  // Thus, given [true, null, {"a":1}, [1,2]], we would visit true, null, {
  // and [. At the object ({) or at the array ([), you can issue a "down" to
  // visit their content. valid if we're not at the end of a scope (returns
  // true).
  inline bool next();

  // Within a given scope (series of nodes at the same depth within either an
  // array or an object), we move backward.
  // Thus, given [true, null, {"a":1}, [1,2]], we would visit ], }, null, true
  // when starting at the end of the scope. At the object ({) or at the array
  // ([), you can issue a "down" to visit their content.
  // Performance warning: This function is implemented by starting again
  // from the beginning of the scope and scanning forward. You should expect
  // it to be relatively slow.
  inline bool prev();

  // Moves back to either the containing array or object (type { or [) from
  // within a contained scope.
  // Valid unless we are at the first level of the document
  inline bool up();

  // Valid if we're at a [ or { and it starts a non-empty scope; moves us to
  // start of that deeper scope if it not empty. Thus, given [true, null,
  // {"a":1}, [1,2]], if we are at the { node, we would move to the "a" node.
  inline bool down();

  // move us to the start of our current scope,
  // a scope is a series of nodes at the same level
  inline void to_start_scope();

  inline void rewind() {
      while (up())
      ;
  }



  // print the node we are currently pointing at
  inline bool print(std::ostream &os, bool escape_strings = true) const;

  private:
  const document &doc;
  size_t max_depth{};
  size_t depth{};
  size_t location{}; // our current location on a tape
  size_t tape_length{};
  uint8_t current_type{};
  uint64_t current_val{};
  typedef struct {
      size_t start_of_scope;
      uint8_t scope_type;
  } scopeindex_t;

  scopeindex_t *depth_index{};
};

} // namespace simdjson
#endif // SIMDJSON_DISABLE_DEPRECATED_API

#endif // SIMDJSON_DOM_PARSEDJSON_ITERATOR_H
/* end file include/simdjson/dom/parsedjson_iterator.h */

// Inline functions
/* begin file include/simdjson/dom/array-inl.h */
#ifndef SIMDJSON_INLINE_ARRAY_H
#define SIMDJSON_INLINE_ARRAY_H

// Inline implementations go in here.

#include <utility>

namespace simdjson {

//
// simdjson_result<dom::array> inline implementation
//
simdjson_really_inline simdjson_result<dom::array>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::array>() {}
simdjson_really_inline simdjson_result<dom::array>::simdjson_result(dom::array value) noexcept
    : internal::simdjson_result_base<dom::array>(std::forward<dom::array>(value)) {}
simdjson_really_inline simdjson_result<dom::array>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::array>(error) {}

#if SIMDJSON_EXCEPTIONS

inline dom::array::iterator simdjson_result<dom::array>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::array::iterator simdjson_result<dom::array>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}
inline size_t simdjson_result<dom::array>::size() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.size();
}

#endif // SIMDJSON_EXCEPTIONS

inline simdjson_result<dom::element> simdjson_result<dom::array>::at_pointer(std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::array>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}

namespace dom {

//
// array inline implementation
//
simdjson_really_inline array::array() noexcept : tape{} {}
simdjson_really_inline array::array(const internal::tape_ref &_tape) noexcept : tape{_tape} {}
inline array::iterator array::begin() const noexcept {
  return internal::tape_ref(tape.doc, tape.json_index + 1);
}
inline array::iterator array::end() const noexcept {
  return internal::tape_ref(tape.doc, tape.after_element() - 1);
}
inline size_t array::size() const noexcept {
  return tape.scope_count();
}
inline size_t array::number_of_slots() const noexcept {
  return tape.matching_brace_index() - tape.json_index;
}
inline simdjson_result<element> array::at_pointer(std::string_view json_pointer) const noexcept {
  if(json_pointer.empty()) { // an empty string means that we return the current node
      return element(this->tape); // copy the current node
  } else if(json_pointer[0] != '/') { // otherwise there is an error
      return INVALID_JSON_POINTER;
  }
  json_pointer = json_pointer.substr(1);
  // - means "the append position" or "the element after the end of the array"
  // We don't support this, because we're returning a real element, not a position.
  if (json_pointer == "-") { return INDEX_OUT_OF_BOUNDS; }

  // Read the array index
  size_t array_index = 0;
  size_t i;
  for (i = 0; i < json_pointer.length() && json_pointer[i] != '/'; i++) {
    uint8_t digit = uint8_t(json_pointer[i] - '0');
    // Check for non-digit in array index. If it's there, we're trying to get a field in an object
    if (digit > 9) { return INCORRECT_TYPE; }
    array_index = array_index*10 + digit;
  }

  // 0 followed by other digits is invalid
  if (i > 1 && json_pointer[0] == '0') { return INVALID_JSON_POINTER; } // "JSON pointer array index has other characters after 0"

  // Empty string is invalid; so is a "/" with no digits before it
  if (i == 0) { return INVALID_JSON_POINTER; } // "Empty string in JSON pointer array index"

  // Get the child
  auto child = array(tape).at(array_index);
  // If there is an error, it ends here
  if(child.error()) {
    return child;
  }
  // If there is a /, we're not done yet, call recursively.
  if (i < json_pointer.length()) {
    child = child.at_pointer(json_pointer.substr(i));
  }
  return child;
}

inline simdjson_result<element> array::at(size_t index) const noexcept {
  size_t i=0;
  for (auto element : *this) {
    if (i == index) { return element; }
    i++;
  }
  return INDEX_OUT_OF_BOUNDS;
}

//
// array::iterator inline implementation
//
simdjson_really_inline array::iterator::iterator(const internal::tape_ref &_tape) noexcept : tape{_tape} { }
inline element array::iterator::operator*() const noexcept {
  return element(tape);
}
inline array::iterator& array::iterator::operator++() noexcept {
  tape.json_index = tape.after_element();
  return *this;
}
inline array::iterator array::iterator::operator++(int) noexcept {
  array::iterator out = *this;
  ++*this;
  return out;
}
inline bool array::iterator::operator!=(const array::iterator& other) const noexcept {
  return tape.json_index != other.tape.json_index;
}
inline bool array::iterator::operator==(const array::iterator& other) const noexcept {
  return tape.json_index == other.tape.json_index;
}
inline bool array::iterator::operator<(const array::iterator& other) const noexcept {
  return tape.json_index < other.tape.json_index;
}
inline bool array::iterator::operator<=(const array::iterator& other) const noexcept {
  return tape.json_index <= other.tape.json_index;
}
inline bool array::iterator::operator>=(const array::iterator& other) const noexcept {
  return tape.json_index >= other.tape.json_index;
}
inline bool array::iterator::operator>(const array::iterator& other) const noexcept {
  return tape.json_index > other.tape.json_index;
}

} // namespace dom


} // namespace simdjson

/* begin file include/simdjson/dom/element-inl.h */
#ifndef SIMDJSON_INLINE_ELEMENT_H
#define SIMDJSON_INLINE_ELEMENT_H

#include <cstring>
#include <utility>

namespace simdjson {

//
// simdjson_result<dom::element> inline implementation
//
simdjson_really_inline simdjson_result<dom::element>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::element>() {}
simdjson_really_inline simdjson_result<dom::element>::simdjson_result(dom::element &&value) noexcept
    : internal::simdjson_result_base<dom::element>(std::forward<dom::element>(value)) {}
simdjson_really_inline simdjson_result<dom::element>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::element>(error) {}
inline simdjson_result<dom::element_type> simdjson_result<dom::element>::type() const noexcept {
  if (error()) { return error(); }
  return first.type();
}

template<typename T>
simdjson_really_inline bool simdjson_result<dom::element>::is() const noexcept {
  return !error() && first.is<T>();
}
template<typename T>
simdjson_really_inline simdjson_result<T> simdjson_result<dom::element>::get() const noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}
template<typename T>
simdjson_warn_unused simdjson_really_inline error_code simdjson_result<dom::element>::get(T &value) const noexcept {
  if (error()) { return error(); }
  return first.get<T>(value);
}

simdjson_really_inline simdjson_result<dom::array> simdjson_result<dom::element>::get_array() const noexcept {
  if (error()) { return error(); }
  return first.get_array();
}
simdjson_really_inline simdjson_result<dom::object> simdjson_result<dom::element>::get_object() const noexcept {
  if (error()) { return error(); }
  return first.get_object();
}
simdjson_really_inline simdjson_result<const char *> simdjson_result<dom::element>::get_c_str() const noexcept {
  if (error()) { return error(); }
  return first.get_c_str();
}
simdjson_really_inline simdjson_result<size_t> simdjson_result<dom::element>::get_string_length() const noexcept {
  if (error()) { return error(); }
  return first.get_string_length();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<dom::element>::get_string() const noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<dom::element>::get_int64() const noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<dom::element>::get_uint64() const noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<dom::element>::get_double() const noexcept {
  if (error()) { return error(); }
  return first.get_double();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<dom::element>::get_bool() const noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}

simdjson_really_inline bool simdjson_result<dom::element>::is_array() const noexcept {
  return !error() && first.is_array();
}
simdjson_really_inline bool simdjson_result<dom::element>::is_object() const noexcept {
  return !error() && first.is_object();
}
simdjson_really_inline bool simdjson_result<dom::element>::is_string() const noexcept {
  return !error() && first.is_string();
}
simdjson_really_inline bool simdjson_result<dom::element>::is_int64() const noexcept {
  return !error() && first.is_int64();
}
simdjson_really_inline bool simdjson_result<dom::element>::is_uint64() const noexcept {
  return !error() && first.is_uint64();
}
simdjson_really_inline bool simdjson_result<dom::element>::is_double() const noexcept {
  return !error() && first.is_double();
}
simdjson_really_inline bool simdjson_result<dom::element>::is_number() const noexcept {
  return !error() && first.is_number();
}
simdjson_really_inline bool simdjson_result<dom::element>::is_bool() const noexcept {
  return !error() && first.is_bool();
}

simdjson_really_inline bool simdjson_result<dom::element>::is_null() const noexcept {
  return !error() && first.is_null();
}

simdjson_really_inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<dom::element> simdjson_result<dom::element>::operator[](const char *key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<dom::element> simdjson_result<dom::element>::at_pointer(const std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
[[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
simdjson_really_inline simdjson_result<dom::element> simdjson_result<dom::element>::at(const std::string_view json_pointer) const noexcept {
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
  if (error()) { return error(); }
  return first.at(json_pointer);
SIMDJSON_POP_DISABLE_WARNINGS
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API
simdjson_really_inline simdjson_result<dom::element> simdjson_result<dom::element>::at(size_t index) const noexcept {
  if (error()) { return error(); }
  return first.at(index);
}
simdjson_really_inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
simdjson_really_inline simdjson_result<dom::element> simdjson_result<dom::element>::at_key_case_insensitive(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key_case_insensitive(key);
}

#if SIMDJSON_EXCEPTIONS

simdjson_really_inline simdjson_result<dom::element>::operator bool() const noexcept(false) {
  return get<bool>();
}
simdjson_really_inline simdjson_result<dom::element>::operator const char *() const noexcept(false) {
  return get<const char *>();
}
simdjson_really_inline simdjson_result<dom::element>::operator std::string_view() const noexcept(false) {
  return get<std::string_view>();
}
simdjson_really_inline simdjson_result<dom::element>::operator uint64_t() const noexcept(false) {
  return get<uint64_t>();
}
simdjson_really_inline simdjson_result<dom::element>::operator int64_t() const noexcept(false) {
  return get<int64_t>();
}
simdjson_really_inline simdjson_result<dom::element>::operator double() const noexcept(false) {
  return get<double>();
}
simdjson_really_inline simdjson_result<dom::element>::operator dom::array() const noexcept(false) {
  return get<dom::array>();
}
simdjson_really_inline simdjson_result<dom::element>::operator dom::object() const noexcept(false) {
  return get<dom::object>();
}

simdjson_really_inline dom::array::iterator simdjson_result<dom::element>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
simdjson_really_inline dom::array::iterator simdjson_result<dom::element>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}

#endif // SIMDJSON_EXCEPTIONS

namespace dom {

//
// element inline implementation
//
simdjson_really_inline element::element() noexcept : tape{} {}
simdjson_really_inline element::element(const internal::tape_ref &_tape) noexcept : tape{_tape} { }

inline element_type element::type() const noexcept {
  auto tape_type = tape.tape_ref_type();
  return tape_type == internal::tape_type::FALSE_VALUE ? element_type::BOOL : static_cast<element_type>(tape_type);
}

inline simdjson_result<bool> element::get_bool() const noexcept {
  if(tape.is_true()) {
    return true;
  } else if(tape.is_false()) {
    return false;
  }
  return INCORRECT_TYPE;
}
inline simdjson_result<const char *> element::get_c_str() const noexcept {
  switch (tape.tape_ref_type()) {
    case internal::tape_type::STRING: {
      return tape.get_c_str();
    }
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<size_t> element::get_string_length() const noexcept {
  switch (tape.tape_ref_type()) {
    case internal::tape_type::STRING: {
      return tape.get_string_length();
    }
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<std::string_view> element::get_string() const noexcept {
  switch (tape.tape_ref_type()) {
    case internal::tape_type::STRING:
      return tape.get_string_view();
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<uint64_t> element::get_uint64() const noexcept {
  if(simdjson_unlikely(!tape.is_uint64())) { // branch rarely taken
    if(tape.is_int64()) {
      int64_t result = tape.next_tape_value<int64_t>();
      if (result < 0) {
        return NUMBER_OUT_OF_RANGE;
      }
      return uint64_t(result);
    }
    return INCORRECT_TYPE;
  }
  return tape.next_tape_value<int64_t>();
}
inline simdjson_result<int64_t> element::get_int64() const noexcept {
  if(simdjson_unlikely(!tape.is_int64())) { // branch rarely taken
    if(tape.is_uint64()) {
      uint64_t result = tape.next_tape_value<uint64_t>();
      // Wrapping max in parens to handle Windows issue: https://stackoverflow.com/questions/11544073/how-do-i-deal-with-the-max-macro-in-windows-h-colliding-with-max-in-std
      if (result > uint64_t((std::numeric_limits<int64_t>::max)())) {
        return NUMBER_OUT_OF_RANGE;
      }
      return static_cast<int64_t>(result);
    }
    return INCORRECT_TYPE;
  }
  return tape.next_tape_value<int64_t>();
}
inline simdjson_result<double> element::get_double() const noexcept {
  // Performance considerations:
  // 1. Querying tape_ref_type() implies doing a shift, it is fast to just do a straight
  //   comparison.
  // 2. Using a switch-case relies on the compiler guessing what kind of code generation
  //    we want... But the compiler cannot know that we expect the type to be "double"
  //    most of the time.
  // We can expect get<double> to refer to a double type almost all the time.
  // It is important to craft the code accordingly so that the compiler can use this
  // information. (This could also be solved with profile-guided optimization.)
  if(simdjson_unlikely(!tape.is_double())) { // branch rarely taken
    if(tape.is_uint64()) {
      return double(tape.next_tape_value<uint64_t>());
    } else if(tape.is_int64()) {
      return double(tape.next_tape_value<int64_t>());
    }
    return INCORRECT_TYPE;
  }
  // this is common:
  return tape.next_tape_value<double>();
}
inline simdjson_result<array> element::get_array() const noexcept {
  switch (tape.tape_ref_type()) {
    case internal::tape_type::START_ARRAY:
      return array(tape);
    default:
      return INCORRECT_TYPE;
  }
}
inline simdjson_result<object> element::get_object() const noexcept {
  switch (tape.tape_ref_type()) {
    case internal::tape_type::START_OBJECT:
      return object(tape);
    default:
      return INCORRECT_TYPE;
  }
}

template<typename T>
simdjson_warn_unused simdjson_really_inline error_code element::get(T &value) const noexcept {
  return get<T>().get(value);
}
// An element-specific version prevents recursion with simdjson_result::get<element>(value)
template<>
simdjson_warn_unused simdjson_really_inline error_code element::get<element>(element &value) const noexcept {
  value = element(tape);
  return SUCCESS;
}
template<typename T>
inline void element::tie(T &value, error_code &error) && noexcept {
  error = get<T>(value);
}

template<typename T>
simdjson_really_inline bool element::is() const noexcept {
  auto result = get<T>();
  return !result.error();
}

template<> inline simdjson_result<array> element::get<array>() const noexcept { return get_array(); }
template<> inline simdjson_result<object> element::get<object>() const noexcept { return get_object(); }
template<> inline simdjson_result<const char *> element::get<const char *>() const noexcept { return get_c_str(); }
template<> inline simdjson_result<std::string_view> element::get<std::string_view>() const noexcept { return get_string(); }
template<> inline simdjson_result<int64_t> element::get<int64_t>() const noexcept { return get_int64(); }
template<> inline simdjson_result<uint64_t> element::get<uint64_t>() const noexcept { return get_uint64(); }
template<> inline simdjson_result<double> element::get<double>() const noexcept { return get_double(); }
template<> inline simdjson_result<bool> element::get<bool>() const noexcept { return get_bool(); }

inline bool element::is_array() const noexcept { return is<array>(); }
inline bool element::is_object() const noexcept { return is<object>(); }
inline bool element::is_string() const noexcept { return is<std::string_view>(); }
inline bool element::is_int64() const noexcept { return is<int64_t>(); }
inline bool element::is_uint64() const noexcept { return is<uint64_t>(); }
inline bool element::is_double() const noexcept { return is<double>(); }
inline bool element::is_bool() const noexcept { return is<bool>(); }
inline bool element::is_number() const noexcept { return is_int64() || is_uint64() || is_double(); }

inline bool element::is_null() const noexcept {
  return tape.is_null_on_tape();
}

#if SIMDJSON_EXCEPTIONS

inline element::operator bool() const noexcept(false) { return get<bool>(); }
inline element::operator const char*() const noexcept(false) { return get<const char *>(); }
inline element::operator std::string_view() const noexcept(false) { return get<std::string_view>(); }
inline element::operator uint64_t() const noexcept(false) { return get<uint64_t>(); }
inline element::operator int64_t() const noexcept(false) { return get<int64_t>(); }
inline element::operator double() const noexcept(false) { return get<double>(); }
inline element::operator array() const noexcept(false) { return get<array>(); }
inline element::operator object() const noexcept(false) { return get<object>(); }

inline array::iterator element::begin() const noexcept(false) {
  return get<array>().begin();
}
inline array::iterator element::end() const noexcept(false) {
  return get<array>().end();
}

#endif // SIMDJSON_EXCEPTIONS

inline simdjson_result<element> element::operator[](std::string_view key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> element::operator[](const char *key) const noexcept {
  return at_key(key);
}

inline simdjson_result<element> element::at_pointer(std::string_view json_pointer) const noexcept {
  switch (tape.tape_ref_type()) {
    case internal::tape_type::START_OBJECT:
      return object(tape).at_pointer(json_pointer);
    case internal::tape_type::START_ARRAY:
      return array(tape).at_pointer(json_pointer);
    default: {
      if(!json_pointer.empty()) { // a non-empty string is invalid on an atom
        return INVALID_JSON_POINTER;
      }
      // an empty string means that we return the current node
      dom::element copy(*this);
      return simdjson_result<element>(std::move(copy));
    }
  }
}
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
[[deprecated("For standard compliance, use at_pointer instead, and prefix your pointers with a slash '/', see RFC6901 ")]]
inline simdjson_result<element> element::at(std::string_view json_pointer) const noexcept {
  // version 0.4 of simdjson allowed non-compliant pointers
  auto std_pointer = (json_pointer.empty() ? "" : "/") + std::string(json_pointer.begin(), json_pointer.end());
  return at_pointer(std_pointer);
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline simdjson_result<element> element::at(size_t index) const noexcept {
  return get<array>().at(index);
}
inline simdjson_result<element> element::at_key(std::string_view key) const noexcept {
  return get<object>().at_key(key);
}
inline simdjson_result<element> element::at_key_case_insensitive(std::string_view key) const noexcept {
  return get<object>().at_key_case_insensitive(key);
}

inline bool element::dump_raw_tape(std::ostream &out) const noexcept {
  return tape.doc->dump_raw_tape(out);
}


inline std::ostream& operator<<(std::ostream& out, element_type type) {
  switch (type) {
    case element_type::ARRAY:
      return out << "array";
    case element_type::OBJECT:
      return out << "object";
    case element_type::INT64:
      return out << "int64_t";
    case element_type::UINT64:
      return out << "uint64_t";
    case element_type::DOUBLE:
      return out << "double";
    case element_type::STRING:
      return out << "string";
    case element_type::BOOL:
      return out << "bool";
    case element_type::NULL_VALUE:
      return out << "null";
    default:
      return out << "unexpected content!!!"; // abort() usage is forbidden in the library
  }
}

} // namespace dom

} // namespace simdjson

#endif // SIMDJSON_INLINE_ELEMENT_H
/* end file include/simdjson/dom/element-inl.h */

#if defined(__cpp_lib_ranges)
static_assert(std::ranges::view<simdjson::dom::array>);
static_assert(std::ranges::sized_range<simdjson::dom::array>);
#if SIMDJSON_EXCEPTIONS
static_assert(std::ranges::view<simdjson::simdjson_result<simdjson::dom::array>>);
static_assert(std::ranges::sized_range<simdjson::simdjson_result<simdjson::dom::array>>);
#endif // SIMDJSON_EXCEPTIONS
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_INLINE_ARRAY_H
/* end file include/simdjson/dom/array-inl.h */
/* begin file include/simdjson/dom/document_stream-inl.h */
#ifndef SIMDJSON_INLINE_DOCUMENT_STREAM_H
#define SIMDJSON_INLINE_DOCUMENT_STREAM_H

#include <algorithm>
#include <limits>
#include <stdexcept>
namespace simdjson {
namespace dom {

#ifdef SIMDJSON_THREADS_ENABLED
inline void stage1_worker::finish() {
  // After calling "run" someone would call finish() to wait
  // for the end of the processing.
  // This function will wait until either the thread has done
  // the processing or, else, the destructor has been called.
  std::unique_lock<std::mutex> lock(locking_mutex);
  cond_var.wait(lock, [this]{return has_work == false;});
}

inline stage1_worker::~stage1_worker() {
  // The thread may never outlive the stage1_worker instance
  // and will always be stopped/joined before the stage1_worker
  // instance is gone.
  stop_thread();
}

inline void stage1_worker::start_thread() {
  std::unique_lock<std::mutex> lock(locking_mutex);
  if(thread.joinable()) {
    return; // This should never happen but we never want to create more than one thread.
  }
  thread = std::thread([this]{
      while(true) {
        std::unique_lock<std::mutex> thread_lock(locking_mutex);
        // We wait for either "run" or "stop_thread" to be called.
        cond_var.wait(thread_lock, [this]{return has_work || !can_work;});
        // If, for some reason, the stop_thread() method was called (i.e., the
        // destructor of stage1_worker is called, then we want to immediately destroy
        // the thread (and not do any more processing).
        if(!can_work) {
          break;
        }
        this->owner->stage1_thread_error = this->owner->run_stage1(*this->stage1_thread_parser,
              this->_next_batch_start);
        this->has_work = false;
        // The condition variable call should be moved after thread_lock.unlock() for performance
        // reasons but thread sanitizers may report it as a data race if we do.
        // See https://stackoverflow.com/questions/35775501/c-should-condition-variable-be-notified-under-lock
        cond_var.notify_one(); // will notify "finish"
        thread_lock.unlock();
      }
    }
  );
}


inline void stage1_worker::stop_thread() {
  std::unique_lock<std::mutex> lock(locking_mutex);
  // We have to make sure that all locks can be released.
  can_work = false;
  has_work = false;
  cond_var.notify_all();
  lock.unlock();
  if(thread.joinable()) {
    thread.join();
  }
}

inline void stage1_worker::run(document_stream * ds, dom::parser * stage1, size_t next_batch_start) {
  std::unique_lock<std::mutex> lock(locking_mutex);
  owner = ds;
  _next_batch_start = next_batch_start;
  stage1_thread_parser = stage1;
  has_work = true;
  // The condition variable call should be moved after thread_lock.unlock() for performance
  // reasons but thread sanitizers may report it as a data race if we do.
  // See https://stackoverflow.com/questions/35775501/c-should-condition-variable-be-notified-under-lock
  cond_var.notify_one(); // will notify the thread lock that we have work
  lock.unlock();
}
#endif

simdjson_really_inline document_stream::document_stream(
  dom::parser &_parser,
  const uint8_t *_buf,
  size_t _len,
  size_t _batch_size
) noexcept
  : parser{&_parser},
    buf{_buf},
    len{_len},
    batch_size{_batch_size <= MINIMAL_BATCH_SIZE ? MINIMAL_BATCH_SIZE : _batch_size},
    error{SUCCESS}
#ifdef SIMDJSON_THREADS_ENABLED
    , use_thread(_parser.threaded) // we need to make a copy because _parser.threaded can change
#endif
{
#ifdef SIMDJSON_THREADS_ENABLED
  if(worker.get() == nullptr) {
    error = MEMALLOC;
  }
#endif
}

simdjson_really_inline document_stream::document_stream() noexcept
  : parser{nullptr},
    buf{nullptr},
    len{0},
    batch_size{0},
    error{UNINITIALIZED}
#ifdef SIMDJSON_THREADS_ENABLED
    , use_thread(false)
#endif
{
}

simdjson_really_inline document_stream::~document_stream() noexcept {
#ifdef SIMDJSON_THREADS_ENABLED
  worker.reset();
#endif
}

simdjson_really_inline document_stream::iterator::iterator() noexcept
  : stream{nullptr}, finished{true} {
}

simdjson_really_inline document_stream::iterator document_stream::begin() noexcept {
  start();
  // If there are no documents, we're finished.
  return iterator(this, error == EMPTY);
}

simdjson_really_inline document_stream::iterator document_stream::end() noexcept {
  return iterator(this, true);
}

simdjson_really_inline document_stream::iterator::iterator(document_stream* _stream, bool is_end) noexcept
  : stream{_stream}, finished{is_end} {
}

simdjson_really_inline document_stream::iterator::reference document_stream::iterator::operator*() noexcept {
  // Note that in case of error, we do not yet mark
  // the iterator as "finished": this detection is done
  // in the operator++ function since it is possible
  // to call operator++ repeatedly while omitting
  // calls to operator*.
  if (stream->error) { return stream->error; }
  return stream->parser->doc.root();
}

simdjson_really_inline document_stream::iterator& document_stream::iterator::operator++() noexcept {
  // If there is an error, then we want the iterator
  // to be finished, no matter what. (E.g., we do not
  // keep generating documents with errors, or go beyond
  // a document with errors.)
  //
  // Users do not have to call "operator*()" when they use operator++,
  // so we need to end the stream in the operator++ function.
  //
  // Note that setting finished = true is essential otherwise
  // we would enter an infinite loop.
  if (stream->error) { finished = true; }
  // Note that stream->error() is guarded against error conditions
  // (it will immediately return if stream->error casts to false).
  // In effect, this next function does nothing when (stream->error)
  // is true (hence the risk of an infinite loop).
  stream->next();
  // If that was the last document, we're finished.
  // It is the only type of error we do not want to appear
  // in operator*.
  if (stream->error == EMPTY) { finished = true; }
  // If we had any other kind of error (not EMPTY) then we want
  // to pass it along to the operator* and we cannot mark the result
  // as "finished" just yet.
  return *this;
}

simdjson_really_inline bool document_stream::iterator::operator!=(const document_stream::iterator &other) const noexcept {
  return finished != other.finished;
}

inline void document_stream::start() noexcept {
  if (error) { return; }
  error = parser->ensure_capacity(batch_size);
  if (error) { return; }
  // Always run the first stage 1 parse immediately
  batch_start = 0;
  error = run_stage1(*parser, batch_start);
  if(error == EMPTY) {
    // In exceptional cases, we may start with an empty block
    batch_start = next_batch_start();
    if (batch_start >= len) { return; }
    error = run_stage1(*parser, batch_start);
  }
  if (error) { return; }
#ifdef SIMDJSON_THREADS_ENABLED
  if (use_thread && next_batch_start() < len) {
    // Kick off the first thread if needed
    error = stage1_thread_parser.ensure_capacity(batch_size);
    if (error) { return; }
    worker->start_thread();
    start_stage1_thread();
    if (error) { return; }
  }
#endif // SIMDJSON_THREADS_ENABLED

  next();
}

simdjson_really_inline size_t document_stream::iterator::current_index() const noexcept {
  return stream->doc_index;
}

simdjson_really_inline std::string_view document_stream::iterator::source() const noexcept {
  size_t next_doc_index = stream->batch_start + stream->parser->implementation->structural_indexes[stream->parser->implementation->next_structural_index];
  return std::string_view(reinterpret_cast<const char*>(stream->buf) + current_index(), next_doc_index - current_index() - 1);
}


inline void document_stream::next() noexcept {
  // We always enter at once once in an error condition.
  if (error) { return; }

  // Load the next document from the batch
  doc_index = batch_start + parser->implementation->structural_indexes[parser->implementation->next_structural_index];
  error = parser->implementation->stage2_next(parser->doc);
  // If that was the last document in the batch, load another batch (if available)
  while (error == EMPTY) {
    batch_start = next_batch_start();
    if (batch_start >= len) { break; }

#ifdef SIMDJSON_THREADS_ENABLED
    if(use_thread) {
      load_from_stage1_thread();
    } else {
      error = run_stage1(*parser, batch_start);
    }
#else
    error = run_stage1(*parser, batch_start);
#endif
    if (error) { continue; } // If the error was EMPTY, we may want to load another batch.
    // Run stage 2 on the first document in the batch
    doc_index = batch_start + parser->implementation->structural_indexes[parser->implementation->next_structural_index];
    error = parser->implementation->stage2_next(parser->doc);
  }
}

inline size_t document_stream::next_batch_start() const noexcept {
  return batch_start + parser->implementation->structural_indexes[parser->implementation->n_structural_indexes];
}

inline error_code document_stream::run_stage1(dom::parser &p, size_t _batch_start) noexcept {
  // If this is the final batch, pass partial = false
  size_t remaining = len - _batch_start;
  if (remaining <= batch_size) {
    return p.implementation->stage1(&buf[_batch_start], remaining, false);
  } else {
    return p.implementation->stage1(&buf[_batch_start], batch_size, true);
  }
}

#ifdef SIMDJSON_THREADS_ENABLED

inline void document_stream::load_from_stage1_thread() noexcept {
  worker->finish();
  // Swap to the parser that was loaded up in the thread. Make sure the parser has
  // enough memory to swap to, as well.
  std::swap(*parser, stage1_thread_parser);
  error = stage1_thread_error;
  if (error) { return; }

  // If there's anything left, start the stage 1 thread!
  if (next_batch_start() < len) {
    start_stage1_thread();
  }
}

inline void document_stream::start_stage1_thread() noexcept {
  // we call the thread on a lambda that will update
  // this->stage1_thread_error
  // there is only one thread that may write to this value
  // TODO this is NOT exception-safe.
  this->stage1_thread_error = UNINITIALIZED; // In case something goes wrong, make sure it's an error
  size_t _next_batch_start = this->next_batch_start();

  worker->run(this, & this->stage1_thread_parser, _next_batch_start);
}

#endif // SIMDJSON_THREADS_ENABLED

} // namespace dom

simdjson_really_inline simdjson_result<dom::document_stream>::simdjson_result() noexcept
  : simdjson_result_base() {
}
simdjson_really_inline simdjson_result<dom::document_stream>::simdjson_result(error_code error) noexcept
  : simdjson_result_base(error) {
}
simdjson_really_inline simdjson_result<dom::document_stream>::simdjson_result(dom::document_stream &&value) noexcept
  : simdjson_result_base(std::forward<dom::document_stream>(value)) {
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::begin() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
simdjson_really_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::end() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}
#else // SIMDJSON_EXCEPTIONS
#ifndef SIMDJSON_DISABLE_DEPRECATED_API
simdjson_really_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::begin() noexcept {
  first.error = error();
  return first.begin();
}
simdjson_really_inline dom::document_stream::iterator simdjson_result<dom::document_stream>::end() noexcept {
  first.error = error();
  return first.end();
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API
#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson
#endif // SIMDJSON_INLINE_DOCUMENT_STREAM_H
/* end file include/simdjson/dom/document_stream-inl.h */
/* begin file include/simdjson/dom/document-inl.h */
#ifndef SIMDJSON_INLINE_DOCUMENT_H
#define SIMDJSON_INLINE_DOCUMENT_H

// Inline implementations go in here.

#include <ostream>
#include <cstring>

namespace simdjson {
namespace dom {

//
// document inline implementation
//
inline element document::root() const noexcept {
  return element(internal::tape_ref(this, 1));
}
simdjson_warn_unused
inline size_t document::capacity() const noexcept {
  return allocated_capacity;
}

simdjson_warn_unused
inline error_code document::allocate(size_t capacity) noexcept {
  if (capacity == 0) {
    string_buf.reset();
    tape.reset();
    allocated_capacity = 0;
    return SUCCESS;
  }

  // a pathological input like "[[[[..." would generate capacity tape elements, so
  // need a capacity of at least capacity + 1, but it is also possible to do
  // worse with "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6"
  //where capacity + 1 tape elements are
  // generated, see issue https://github.com/lemire/simdjson/issues/345
  size_t tape_capacity = SIMDJSON_ROUNDUP_N(capacity + 3, 64);
  // a document with only zero-length strings... could have capacity/3 string
  // and we would need capacity/3 * 5 bytes on the string buffer
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * capacity / 3 + SIMDJSON_PADDING, 64);
  string_buf.reset( new (std::nothrow) uint8_t[string_capacity]);
  tape.reset(new (std::nothrow) uint64_t[tape_capacity]);
  if(!(string_buf && tape)) {
    allocated_capacity = 0;
    string_buf.reset();
    tape.reset();
    return MEMALLOC;
  }
  // Technically the allocated_capacity might be larger than capacity
  // so the next line is pessimistic.
  allocated_capacity = capacity;
  return SUCCESS;
}

inline bool document::dump_raw_tape(std::ostream &os) const noexcept {
  uint32_t string_length;
  size_t tape_idx = 0;
  uint64_t tape_val = tape[tape_idx];
  uint8_t type = uint8_t(tape_val >> 56);
  os << tape_idx << " : " << type;
  tape_idx++;
  size_t how_many = 0;
  if (type == 'r') {
    how_many = size_t(tape_val & internal::JSON_VALUE_MASK);
  } else {
    // Error: no starting root node?
    return false;
  }
  os << "\t// pointing to " << how_many << " (right after last node)\n";
  uint64_t payload;
  for (; tape_idx < how_many; tape_idx++) {
    os << tape_idx << " : ";
    tape_val = tape[tape_idx];
    payload = tape_val & internal::JSON_VALUE_MASK;
    type = uint8_t(tape_val >> 56);
    switch (type) {
    case '"': // we have a string
      os << "string \"";
      std::memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      os << internal::escape_json_string(std::string_view(
        reinterpret_cast<const char *>(string_buf.get() + payload + sizeof(uint32_t)),
        string_length
      ));
      os << '"';
      os << '\n';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
      break;
    case 'u': // we have a long uint
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "unsigned integer " << tape[++tape_idx] << "\n";
      break;
    case 'd': // we have a double
      os << "float ";
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      double answer;
      std::memcpy(&answer, &tape[++tape_idx], sizeof(answer));
      os << answer << '\n';
      break;
    case 'n': // we have a null
      os << "null\n";
      break;
    case 't': // we have a true
      os << "true\n";
      break;
    case 'f': // we have a false
      os << "false\n";
      break;
    case '{': // we have an object
      os << "{\t// pointing to next tape location " << uint32_t(payload)
         << " (first node after the scope), "
         << " saturated count "
         << ((payload >> 32) & internal::JSON_COUNT_MASK)<< "\n";
      break;    case '}': // we end an object
      os << "}\t// pointing to previous tape location " << uint32_t(payload)
         << " (start of the scope)\n";
      break;
    case '[': // we start an array
      os << "[\t// pointing to next tape location " << uint32_t(payload)
         << " (first node after the scope), "
         << " saturated count "
         << ((payload >> 32) & internal::JSON_COUNT_MASK)<< "\n";
      break;
    case ']': // we end an array
      os << "]\t// pointing to previous tape location " << uint32_t(payload)
         << " (start of the scope)\n";
      break;
    case 'r': // we start and end with the root node
      // should we be hitting the root node?
      return false;
    default:
      return false;
    }
  }
  tape_val = tape[tape_idx];
  payload = tape_val & internal::JSON_VALUE_MASK;
  type = uint8_t(tape_val >> 56);
  os << tape_idx << " : " << type << "\t// pointing to " << payload
     << " (start root)\n";
  return true;
}

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_INLINE_DOCUMENT_H
/* end file include/simdjson/dom/document-inl.h */
/* begin file include/simdjson/dom/object-inl.h */
#ifndef SIMDJSON_INLINE_OBJECT_H
#define SIMDJSON_INLINE_OBJECT_H

#include <cstring>
#include <string>

namespace simdjson {

//
// simdjson_result<dom::object> inline implementation
//
simdjson_really_inline simdjson_result<dom::object>::simdjson_result() noexcept
    : internal::simdjson_result_base<dom::object>() {}
simdjson_really_inline simdjson_result<dom::object>::simdjson_result(dom::object value) noexcept
    : internal::simdjson_result_base<dom::object>(std::forward<dom::object>(value)) {}
simdjson_really_inline simdjson_result<dom::object>::simdjson_result(error_code error) noexcept
    : internal::simdjson_result_base<dom::object>(error) {}

inline simdjson_result<dom::element> simdjson_result<dom::object>::operator[](std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::operator[](const char *key) const noexcept {
  if (error()) { return error(); }
  return first[key];
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_pointer(std::string_view json_pointer) const noexcept {
  if (error()) { return error(); }
  return first.at_pointer(json_pointer);
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_key(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key(key);
}
inline simdjson_result<dom::element> simdjson_result<dom::object>::at_key_case_insensitive(std::string_view key) const noexcept {
  if (error()) { return error(); }
  return first.at_key_case_insensitive(key);
}

#if SIMDJSON_EXCEPTIONS

inline dom::object::iterator simdjson_result<dom::object>::begin() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.begin();
}
inline dom::object::iterator simdjson_result<dom::object>::end() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.end();
}
inline size_t simdjson_result<dom::object>::size() const noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first.size();
}

#endif // SIMDJSON_EXCEPTIONS

namespace dom {

//
// object inline implementation
//
simdjson_really_inline object::object() noexcept : tape{} {}
simdjson_really_inline object::object(const internal::tape_ref &_tape) noexcept : tape{_tape} { }
inline object::iterator object::begin() const noexcept {
  return internal::tape_ref(tape.doc, tape.json_index + 1);
}
inline object::iterator object::end() const noexcept {
  return internal::tape_ref(tape.doc, tape.after_element() - 1);
}
inline size_t object::size() const noexcept {
  return tape.scope_count();
}

inline simdjson_result<element> object::operator[](std::string_view key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> object::operator[](const char *key) const noexcept {
  return at_key(key);
}
inline simdjson_result<element> object::at_pointer(std::string_view json_pointer) const noexcept {
  if(json_pointer.empty()) { // an empty string means that we return the current node
      return element(this->tape); // copy the current node
  } else if(json_pointer[0] != '/') { // otherwise there is an error
      return INVALID_JSON_POINTER;
  }
  json_pointer = json_pointer.substr(1);
  size_t slash = json_pointer.find('/');
  std::string_view key = json_pointer.substr(0, slash);
  // Grab the child with the given key
  simdjson_result<element> child;

  // If there is an escape character in the key, unescape it and then get the child.
  size_t escape = key.find('~');
  if (escape != std::string_view::npos) {
    // Unescape the key
    std::string unescaped(key);
    do {
      switch (unescaped[escape+1]) {
        case '0':
          unescaped.replace(escape, 2, "~");
          break;
        case '1':
          unescaped.replace(escape, 2, "/");
          break;
        default:
          return INVALID_JSON_POINTER; // "Unexpected ~ escape character in JSON pointer");
      }
      escape = unescaped.find('~', escape+1);
    } while (escape != std::string::npos);
    child = at_key(unescaped);
  } else {
    child = at_key(key);
  }
  if(child.error()) {
    return child; // we do not continue if there was an error
  }
  // If there is a /, we have to recurse and look up more of the path
  if (slash != std::string_view::npos) {
    child = child.at_pointer(json_pointer.substr(slash));
  }
  return child;
}

inline simdjson_result<element> object::at_key(std::string_view key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (field.key_equals(key)) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}
// In case you wonder why we need this, please see
// https://github.com/simdjson/simdjson/issues/323
// People do seek keys in a case-insensitive manner.
inline simdjson_result<element> object::at_key_case_insensitive(std::string_view key) const noexcept {
  iterator end_field = end();
  for (iterator field = begin(); field != end_field; ++field) {
    if (field.key_equals_case_insensitive(key)) {
      return field.value();
    }
  }
  return NO_SUCH_FIELD;
}

//
// object::iterator inline implementation
//
simdjson_really_inline object::iterator::iterator(const internal::tape_ref &_tape) noexcept : tape{_tape} { }
inline const key_value_pair object::iterator::operator*() const noexcept {
  return key_value_pair(key(), value());
}
inline bool object::iterator::operator!=(const object::iterator& other) const noexcept {
  return tape.json_index != other.tape.json_index;
}
inline bool object::iterator::operator==(const object::iterator& other) const noexcept {
  return tape.json_index == other.tape.json_index;
}
inline bool object::iterator::operator<(const object::iterator& other) const noexcept {
  return tape.json_index < other.tape.json_index;
}
inline bool object::iterator::operator<=(const object::iterator& other) const noexcept {
  return tape.json_index <= other.tape.json_index;
}
inline bool object::iterator::operator>=(const object::iterator& other) const noexcept {
  return tape.json_index >= other.tape.json_index;
}
inline bool object::iterator::operator>(const object::iterator& other) const noexcept {
  return tape.json_index > other.tape.json_index;
}
inline object::iterator& object::iterator::operator++() noexcept {
  tape.json_index++;
  tape.json_index = tape.after_element();
  return *this;
}
inline object::iterator object::iterator::operator++(int) noexcept {
  object::iterator out = *this;
  ++*this;
  return out;
}
inline std::string_view object::iterator::key() const noexcept {
  return tape.get_string_view();
}
inline uint32_t object::iterator::key_length() const noexcept {
  return tape.get_string_length();
}
inline const char* object::iterator::key_c_str() const noexcept {
  return reinterpret_cast<const char *>(&tape.doc->string_buf[size_t(tape.tape_value()) + sizeof(uint32_t)]);
}
inline element object::iterator::value() const noexcept {
  return element(internal::tape_ref(tape.doc, tape.json_index + 1));
}

/**
 * Design notes:
 * Instead of constructing a string_view and then comparing it with a
 * user-provided strings, it is probably more performant to have dedicated
 * functions taking as a parameter the string we want to compare against
 * and return true when they are equal. That avoids the creation of a temporary
 * std::string_view. Though it is possible for the compiler to avoid entirely
 * any overhead due to string_view, relying too much on compiler magic is
 * problematic: compiler magic sometimes fail, and then what do you do?
 * Also, enticing users to rely on high-performance function is probably better
 * on the long run.
 */

inline bool object::iterator::key_equals(std::string_view o) const noexcept {
  // We use the fact that the key length can be computed quickly
  // without access to the string buffer.
  const uint32_t len = key_length();
  if(o.size() == len) {
    // We avoid construction of a temporary string_view instance.
    return (memcmp(o.data(), key_c_str(), len) == 0);
  }
  return false;
}

inline bool object::iterator::key_equals_case_insensitive(std::string_view o) const noexcept {
  // We use the fact that the key length can be computed quickly
  // without access to the string buffer.
  const uint32_t len = key_length();
  if(o.size() == len) {
      // See For case-insensitive string comparisons, avoid char-by-char functions
      // https://lemire.me/blog/2020/04/30/for-case-insensitive-string-comparisons-avoid-char-by-char-functions/
      // Note that it might be worth rolling our own strncasecmp function, with vectorization.
      return (simdjson_strncasecmp(o.data(), key_c_str(), len) == 0);
  }
  return false;
}
//
// key_value_pair inline implementation
//
inline key_value_pair::key_value_pair(std::string_view _key, element _value) noexcept :
  key(_key), value(_value) {}

} // namespace dom

} // namespace simdjson

#if defined(__cpp_lib_ranges)
static_assert(std::ranges::view<simdjson::dom::object>);
static_assert(std::ranges::sized_range<simdjson::dom::object>);
#if SIMDJSON_EXCEPTIONS
static_assert(std::ranges::view<simdjson::simdjson_result<simdjson::dom::object>>);
static_assert(std::ranges::sized_range<simdjson::simdjson_result<simdjson::dom::object>>);
#endif // SIMDJSON_EXCEPTIONS
#endif // defined(__cpp_lib_ranges)

#endif // SIMDJSON_INLINE_OBJECT_H
/* end file include/simdjson/dom/object-inl.h */
/* begin file include/simdjson/dom/parsedjson_iterator-inl.h */
#ifndef SIMDJSON_INLINE_PARSEDJSON_ITERATOR_H
#define SIMDJSON_INLINE_PARSEDJSON_ITERATOR_H

#include <cstring>

#ifndef SIMDJSON_DISABLE_DEPRECATED_API

namespace simdjson {

// VS2017 reports deprecated warnings when you define a deprecated class's methods.
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING

// Because of template weirdness, the actual class definition is inline in the document class
simdjson_warn_unused bool dom::parser::Iterator::is_ok() const {
  return location < tape_length;
}

// useful for debugging purposes
size_t dom::parser::Iterator::get_tape_location() const {
  return location;
}

// useful for debugging purposes
size_t dom::parser::Iterator::get_tape_length() const {
  return tape_length;
}

// returns the current depth (start at 1 with 0 reserved for the fictitious root
// node)
size_t dom::parser::Iterator::get_depth() const {
  return depth;
}

// A scope is a series of nodes at the same depth, typically it is either an
// object ({) or an array ([). The root node has type 'r'.
uint8_t dom::parser::Iterator::get_scope_type() const {
  return depth_index[depth].scope_type;
}

bool dom::parser::Iterator::move_forward() {
  if (location + 1 >= tape_length) {
    return false; // we are at the end!
  }

  if ((current_type == '[') || (current_type == '{')) {
    // We are entering a new scope
    depth++;
    assert(depth < max_depth);
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
  } else if ((current_type == ']') || (current_type == '}')) {
    // Leaving a scope.
    depth--;
  } else if (is_number()) {
    // these types use 2 locations on the tape, not just one.
    location += 1;
  }

  location += 1;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
  return true;
}

void dom::parser::Iterator::move_to_value() {
  // assume that we are on a key, so move by 1.
  location += 1;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
}

bool dom::parser::Iterator::move_to_key(const char *key) {
    if (down()) {
      do {
        const bool right_key = (strcmp(get_string(), key) == 0);
        move_to_value();
        if (right_key) {
          return true;
        }
      } while (next());
      up();
    }
    return false;
}

bool dom::parser::Iterator::move_to_key_insensitive(
    const char *key) {
    if (down()) {
      do {
        const bool right_key = (simdjson_strcasecmp(get_string(), key) == 0);
        move_to_value();
        if (right_key) {
          return true;
        }
      } while (next());
      up();
    }
    return false;
}

bool dom::parser::Iterator::move_to_key(const char *key,
                                                       uint32_t length) {
  if (down()) {
    do {
      bool right_key = ((get_string_length() == length) &&
                        (memcmp(get_string(), key, length) == 0));
      move_to_value();
      if (right_key) {
        return true;
      }
    } while (next());
    up();
  }
  return false;
}

bool dom::parser::Iterator::move_to_index(uint32_t index) {
  if (down()) {
    uint32_t i = 0;
    for (; i < index; i++) {
      if (!next()) {
        break;
      }
    }
    if (i == index) {
      return true;
    }
    up();
  }
  return false;
}

bool dom::parser::Iterator::prev() {
  size_t target_location = location;
  to_start_scope();
  size_t npos = location;
  if (target_location == npos) {
    return false; // we were already at the start
  }
  size_t oldnpos;
  // we have that npos < target_location here
  do {
    oldnpos = npos;
    if ((current_type == '[') || (current_type == '{')) {
      // we need to jump
      npos = uint32_t(current_val);
    } else {
      npos = npos + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
    }
  } while (npos < target_location);
  location = oldnpos;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
  return true;
}

bool dom::parser::Iterator::up() {
  if (depth == 1) {
    return false; // don't allow moving back to root
  }
  to_start_scope();
  // next we just move to the previous value
  depth--;
  location -= 1;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
  return true;
}

bool dom::parser::Iterator::down() {
  if (location + 1 >= tape_length) {
    return false;
  }
  if ((current_type == '[') || (current_type == '{')) {
    size_t npos = uint32_t(current_val);
    if (npos == location + 2) {
      return false; // we have an empty scope
    }
    depth++;
    assert(depth < max_depth);
    location = location + 1;
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
    current_val = doc.tape[location];
    current_type = uint8_t(current_val >> 56);
    return true;
  }
  return false;
}

void dom::parser::Iterator::to_start_scope() {
  location = depth_index[depth].start_of_scope;
  current_val = doc.tape[location];
  current_type = uint8_t(current_val >> 56);
}

bool dom::parser::Iterator::next() {
  size_t npos;
  if ((current_type == '[') || (current_type == '{')) {
    // we need to jump
    npos = uint32_t(current_val);
  } else {
    npos = location + (is_number() ? 2 : 1);
  }
  uint64_t next_val = doc.tape[npos];
  uint8_t next_type = uint8_t(next_val >> 56);
  if ((next_type == ']') || (next_type == '}')) {
    return false; // we reached the end of the scope
  }
  location = npos;
  current_val = next_val;
  current_type = next_type;
  return true;
}
dom::parser::Iterator::Iterator(const dom::parser &pj) noexcept(false)
    : doc(pj.doc)
{
#if SIMDJSON_EXCEPTIONS
  if (!pj.valid) { throw simdjson_error(pj.error); }
#else
  if (!pj.valid) { return; } //  abort() usage is forbidden in the library
#endif

  max_depth = pj.max_depth();
  depth_index = new scopeindex_t[max_depth + 1];
  depth_index[0].start_of_scope = location;
  current_val = doc.tape[location++];
  current_type = uint8_t(current_val >> 56);
  depth_index[0].scope_type = current_type;
  tape_length = size_t(current_val & internal::JSON_VALUE_MASK);
  if (location < tape_length) {
    // If we make it here, then depth_capacity must >=2, but the compiler
    // may not know this.
    current_val = doc.tape[location];
    current_type = uint8_t(current_val >> 56);
    depth++;
    assert(depth < max_depth);
    depth_index[depth].start_of_scope = location;
    depth_index[depth].scope_type = current_type;
  }
}
dom::parser::Iterator::Iterator(
    const dom::parser::Iterator &o) noexcept
    : doc(o.doc),
    max_depth(o.depth),
    depth(o.depth),
    location(o.location),
    tape_length(o.tape_length),
    current_type(o.current_type),
    current_val(o.current_val)
{
  depth_index = new scopeindex_t[max_depth+1];
  std::memcpy(depth_index, o.depth_index, (depth + 1) * sizeof(depth_index[0]));
}

dom::parser::Iterator::~Iterator() noexcept {
  if (depth_index) { delete[] depth_index; }
}

bool dom::parser::Iterator::print(std::ostream &os, bool escape_strings) const {
  if (!is_ok()) {
    return false;
  }
  switch (current_type) {
  case '"': // we have a string
    os << '"';
    if (escape_strings) {
      os << internal::escape_json_string(std::string_view(get_string(), get_string_length()));
    } else {
      // was: os << get_string();, but given that we can include null chars, we
      // have to do something crazier:
      std::copy(get_string(), get_string() + get_string_length(), std::ostream_iterator<char>(os));
    }
    os << '"';
    break;
  case 'l': // we have a long int
    os << get_integer();
    break;
  case 'u':
    os << get_unsigned_integer();
    break;
  case 'd':
    os << get_double();
    break;
  case 'n': // we have a null
    os << "null";
    break;
  case 't': // we have a true
    os << "true";
    break;
  case 'f': // we have a false
    os << "false";
    break;
  case '{': // we have an object
  case '}': // we end an object
  case '[': // we start an array
  case ']': // we end an array
    os << char(current_type);
    break;
  default:
    return false;
  }
  return true;
}

bool dom::parser::Iterator::move_to(const char *pointer,
                                                   uint32_t length) {
  char *new_pointer = nullptr;
  if (pointer[0] == '#') {
    // Converting fragment representation to string representation
    new_pointer = new char[length];
    uint32_t new_length = 0;
    for (uint32_t i = 1; i < length; i++) {
      if (pointer[i] == '%' && pointer[i + 1] == 'x') {
#if __cpp_exceptions
        try {
#endif
          int fragment =
              std::stoi(std::string(&pointer[i + 2], 2), nullptr, 16);
          if (fragment == '\\' || fragment == '"' || (fragment <= 0x1F)) {
            // escaping the character
            new_pointer[new_length] = '\\';
            new_length++;
          }
          new_pointer[new_length] = char(fragment);
          i += 3;
#if __cpp_exceptions
        } catch (std::invalid_argument &) {
          delete[] new_pointer;
          return false; // the fragment is invalid
        }
#endif
      } else {
        new_pointer[new_length] = pointer[i];
      }
      new_length++;
    }
    length = new_length;
    pointer = new_pointer;
  }

  // saving the current state
  size_t depth_s = depth;
  size_t location_s = location;
  uint8_t current_type_s = current_type;
  uint64_t current_val_s = current_val;

  rewind(); // The json pointer is used from the root of the document.

  bool found = relative_move_to(pointer, length);
  delete[] new_pointer;

  if (!found) {
    // since the pointer has found nothing, we get back to the original
    // position.
    depth = depth_s;
    location = location_s;
    current_type = current_type_s;
    current_val = current_val_s;
  }

  return found;
}

bool dom::parser::Iterator::relative_move_to(const char *pointer,
                                                            uint32_t length) {
  if (length == 0) {
    // returns the whole document
    return true;
  }

  if (pointer[0] != '/') {
    // '/' must be the first character
    return false;
  }

  // finding the key in an object or the index in an array
  std::string key_or_index;
  uint32_t offset = 1;

  // checking for the "-" case
  if (is_array() && pointer[1] == '-') {
    if (length != 2) {
      // the pointer must be exactly "/-"
      // there can't be anything more after '-' as an index
      return false;
    }
    key_or_index = '-';
    offset = length; // will skip the loop coming right after
  }

  // We either transform the first reference token to a valid json key
  // or we make sure it is a valid index in an array.
  for (; offset < length; offset++) {
    if (pointer[offset] == '/') {
      // beginning of the next key or index
      break;
    }
    if (is_array() && (pointer[offset] < '0' || pointer[offset] > '9')) {
      // the index of an array must be an integer
      // we also make sure std::stoi won't discard whitespaces later
      return false;
    }
    if (pointer[offset] == '~') {
      // "~1" represents "/"
      if (pointer[offset + 1] == '1') {
        key_or_index += '/';
        offset++;
        continue;
      }
      // "~0" represents "~"
      if (pointer[offset + 1] == '0') {
        key_or_index += '~';
        offset++;
        continue;
      }
    }
    if (pointer[offset] == '\\') {
      if (pointer[offset + 1] == '\\' || pointer[offset + 1] == '"' ||
          (pointer[offset + 1] <= 0x1F)) {
        key_or_index += pointer[offset + 1];
        offset++;
        continue;
      }
      return false; // invalid escaped character
    }
    if (pointer[offset] == '\"') {
      // unescaped quote character. this is an invalid case.
      // lets do nothing and assume most pointers will be valid.
      // it won't find any corresponding json key anyway.
      // return false;
    }
    key_or_index += pointer[offset];
  }

  bool found = false;
  if (is_object()) {
    if (move_to_key(key_or_index.c_str(), uint32_t(key_or_index.length()))) {
      found = relative_move_to(pointer + offset, length - offset);
    }
  } else if (is_array()) {
    if (key_or_index == "-") { // handling "-" case first
      if (down()) {
        while (next())
          ; // moving to the end of the array
        // moving to the nonexistent value right after...
        size_t npos;
        if ((current_type == '[') || (current_type == '{')) {
          // we need to jump
          npos = uint32_t(current_val);
        } else {
          npos =
              location + ((current_type == 'd' || current_type == 'l') ? 2 : 1);
        }
        location = npos;
        current_val = doc.tape[npos];
        current_type = uint8_t(current_val >> 56);
        return true; // how could it fail ?
      }
    } else { // regular numeric index
      // The index can't have a leading '0'
      if (key_or_index[0] == '0' && key_or_index.length() > 1) {
        return false;
      }
      // it cannot be empty
      if (key_or_index.length() == 0) {
        return false;
      }
      // we already checked the index contains only valid digits
      uint32_t index = std::stoi(key_or_index);
      if (move_to_index(index)) {
        found = relative_move_to(pointer + offset, length - offset);
      }
    }
  }

  return found;
}

SIMDJSON_POP_DISABLE_WARNINGS
} // namespace simdjson

#endif // SIMDJSON_DISABLE_DEPRECATED_API


#endif // SIMDJSON_INLINE_PARSEDJSON_ITERATOR_H
/* end file include/simdjson/dom/parsedjson_iterator-inl.h */
/* begin file include/simdjson/dom/parser-inl.h */
#ifndef SIMDJSON_INLINE_PARSER_H
#define SIMDJSON_INLINE_PARSER_H

#include <cstdio>
#include <climits>

namespace simdjson {
namespace dom {

//
// parser inline implementation
//
simdjson_really_inline parser::parser(size_t max_capacity) noexcept
  : _max_capacity{max_capacity},
    loaded_bytes(nullptr) {
}
simdjson_really_inline parser::parser(parser &&other) noexcept = default;
simdjson_really_inline parser &parser::operator=(parser &&other) noexcept = default;

inline bool parser::is_valid() const noexcept { return valid; }
inline int parser::get_error_code() const noexcept { return error; }
inline std::string parser::get_error_message() const noexcept { return error_message(error); }

inline bool parser::dump_raw_tape(std::ostream &os) const noexcept {
  return valid ? doc.dump_raw_tape(os) : false;
}

inline simdjson_result<size_t> parser::read_file(const std::string &path) noexcept {
  // Open the file
  SIMDJSON_PUSH_DISABLE_WARNINGS
  SIMDJSON_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  std::FILE *fp = std::fopen(path.c_str(), "rb");
  SIMDJSON_POP_DISABLE_WARNINGS

  if (fp == nullptr) {
    return IO_ERROR;
  }

  // Get the file size
  if(std::fseek(fp, 0, SEEK_END) < 0) {
    std::fclose(fp);
    return IO_ERROR;
  }
#if defined(SIMDJSON_VISUAL_STUDIO) && !SIMDJSON_IS_32BITS
  __int64 len = _ftelli64(fp);
  if(len == -1L) {
    std::fclose(fp);
    return IO_ERROR;
  }
#else
  long len = std::ftell(fp);
  if((len < 0) || (len == LONG_MAX)) {
    std::fclose(fp);
    return IO_ERROR;
  }
#endif

  // Make sure we have enough capacity to load the file
  if (_loaded_bytes_capacity < size_t(len)) {
    loaded_bytes.reset( internal::allocate_padded_buffer(len) );
    if (!loaded_bytes) {
      std::fclose(fp);
      return MEMALLOC;
    }
    _loaded_bytes_capacity = len;
  }

  // Read the string
  std::rewind(fp);
  size_t bytes_read = std::fread(loaded_bytes.get(), 1, len, fp);
  if (std::fclose(fp) != 0 || bytes_read != size_t(len)) {
    return IO_ERROR;
  }

  return bytes_read;
}

inline simdjson_result<element> parser::load(const std::string &path) & noexcept {
  size_t len;
  auto _error = read_file(path).get(len);
  if (_error) { return _error; }
  return parse(loaded_bytes.get(), len, false);
}

inline simdjson_result<document_stream> parser::load_many(const std::string &path, size_t batch_size) noexcept {
  size_t len;
  auto _error = read_file(path).get(len);
  if (_error) { return _error; }
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  return document_stream(*this, reinterpret_cast<const uint8_t*>(loaded_bytes.get()), len, batch_size);
}

inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const uint8_t *buf, size_t len, bool realloc_if_needed) & noexcept {
  // Important: we need to ensure that document has enough capacity.
  // Important: It is possible that provided_doc is actually the internal 'doc' within the parser!!!
  error_code _error = ensure_capacity(provided_doc, len);
  if (_error) { return _error; }
  std::unique_ptr<uint8_t[]> tmp_buf;
  if (realloc_if_needed) {
    tmp_buf.reset(reinterpret_cast<uint8_t *>( internal::allocate_padded_buffer(len) ));
    if (tmp_buf.get() == nullptr) { return MEMALLOC; }
    std::memcpy(static_cast<void *>(tmp_buf.get()), buf, len);
  }
  _error = implementation->parse(realloc_if_needed ? tmp_buf.get() : buf, len, provided_doc);

  if (_error) { return _error; }

  return provided_doc.root();
}

simdjson_really_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const char *buf, size_t len, bool realloc_if_needed) & noexcept {
  return parse_into_document(provided_doc, reinterpret_cast<const uint8_t *>(buf), len, realloc_if_needed);
}
simdjson_really_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const std::string &s) & noexcept {
  return parse_into_document(provided_doc, s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
simdjson_really_inline simdjson_result<element> parser::parse_into_document(document& provided_doc, const padded_string &s) & noexcept {
  return parse_into_document(provided_doc, s.data(), s.length(), false);
}


inline simdjson_result<element> parser::parse(const uint8_t *buf, size_t len, bool realloc_if_needed) & noexcept {
  return parse_into_document(doc, buf, len, realloc_if_needed);
}

simdjson_really_inline simdjson_result<element> parser::parse(const char *buf, size_t len, bool realloc_if_needed) & noexcept {
  return parse(reinterpret_cast<const uint8_t *>(buf), len, realloc_if_needed);
}
simdjson_really_inline simdjson_result<element> parser::parse(const std::string &s) & noexcept {
  return parse(s.data(), s.length(), s.capacity() - s.length() < SIMDJSON_PADDING);
}
simdjson_really_inline simdjson_result<element> parser::parse(const padded_string &s) & noexcept {
  return parse(s.data(), s.length(), false);
}

inline simdjson_result<document_stream> parser::parse_many(const uint8_t *buf, size_t len, size_t batch_size) noexcept {
  if(batch_size < MINIMAL_BATCH_SIZE) { batch_size = MINIMAL_BATCH_SIZE; }
  return document_stream(*this, buf, len, batch_size);
}
inline simdjson_result<document_stream> parser::parse_many(const char *buf, size_t len, size_t batch_size) noexcept {
  return parse_many(reinterpret_cast<const uint8_t *>(buf), len, batch_size);
}
inline simdjson_result<document_stream> parser::parse_many(const std::string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}
inline simdjson_result<document_stream> parser::parse_many(const padded_string &s, size_t batch_size) noexcept {
  return parse_many(s.data(), s.length(), batch_size);
}

simdjson_really_inline size_t parser::capacity() const noexcept {
  return implementation ? implementation->capacity() : 0;
}
simdjson_really_inline size_t parser::max_capacity() const noexcept {
  return _max_capacity;
}
simdjson_really_inline size_t parser::max_depth() const noexcept {
  return implementation ? implementation->max_depth() : DEFAULT_MAX_DEPTH;
}

simdjson_warn_unused
inline error_code parser::allocate(size_t capacity, size_t max_depth) noexcept {
  //
  // Reallocate implementation if needed
  //
  error_code err;
  if (implementation) {
    err = implementation->allocate(capacity, max_depth);
  } else {
    err = simdjson::active_implementation->create_dom_parser_implementation(capacity, max_depth, implementation);
  }
  if (err) { return err; }
  return SUCCESS;
}

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
simdjson_warn_unused
inline bool parser::allocate_capacity(size_t capacity, size_t max_depth) noexcept {
  return !allocate(capacity, max_depth);
}
#endif // SIMDJSON_DISABLE_DEPRECATED_API

inline error_code parser::ensure_capacity(size_t desired_capacity) noexcept {
  return ensure_capacity(doc, desired_capacity);
}


inline error_code parser::ensure_capacity(document& target_document, size_t desired_capacity) noexcept {
  // 1. It is wasteful to allocate a document and a parser for documents spanning less than MINIMAL_DOCUMENT_CAPACITY bytes.
  // 2. If we allow desired_capacity = 0 then it is possible to exit this function with implementation == nullptr.
  if(desired_capacity < MINIMAL_DOCUMENT_CAPACITY) { desired_capacity = MINIMAL_DOCUMENT_CAPACITY; }
  // If we don't have enough capacity, (try to) automatically bump it.
  // If the document needs allocation, do it too.
  // Both in one if statement to minimize unlikely branching.
  //
  // Note: we must make sure that this function is called if capacity() == 0. We do so because we
  // ensure that desired_capacity > 0.
  if (simdjson_unlikely(capacity() < desired_capacity || target_document.capacity() < desired_capacity)) {
    if (desired_capacity > max_capacity()) {
      return error = CAPACITY;
    }
    error_code err1 = target_document.capacity() < desired_capacity ? target_document.allocate(desired_capacity) : SUCCESS;
    error_code err2 = capacity() < desired_capacity ? allocate(desired_capacity, max_depth()) : SUCCESS;
    if(err1 != SUCCESS) { return error = err1; }
    if(err2 != SUCCESS) { return error = err2; }
  }
  return SUCCESS;
}

simdjson_really_inline void parser::set_max_capacity(size_t max_capacity) noexcept {
  if(max_capacity < MINIMAL_DOCUMENT_CAPACITY) {
    _max_capacity = max_capacity;
  } else {
    _max_capacity = MINIMAL_DOCUMENT_CAPACITY;
  }
}

} // namespace dom
} // namespace simdjson

#endif // SIMDJSON_INLINE_PARSER_H
/* end file include/simdjson/dom/parser-inl.h */
/* begin file include/simdjson/internal/tape_ref-inl.h */
#ifndef SIMDJSON_INLINE_TAPE_REF_H
#define SIMDJSON_INLINE_TAPE_REF_H

#include <cstring>

namespace simdjson {
namespace internal {

//
// tape_ref inline implementation
//
simdjson_really_inline tape_ref::tape_ref() noexcept : doc{nullptr}, json_index{0} {}
simdjson_really_inline tape_ref::tape_ref(const dom::document *_doc, size_t _json_index) noexcept : doc{_doc}, json_index{_json_index} {}


simdjson_really_inline bool tape_ref::is_document_root() const noexcept {
  return json_index == 1; // should we ever change the structure of the tape, this should get updated.
}

// Some value types have a specific on-tape word value. It can be faster
// to check the type by doing a word-to-word comparison instead of extracting the
// most significant 8 bits.

simdjson_really_inline bool tape_ref::is_double() const noexcept {
  constexpr uint64_t tape_double = uint64_t(tape_type::DOUBLE)<<56;
  return doc->tape[json_index] == tape_double;
}
simdjson_really_inline bool tape_ref::is_int64() const noexcept {
  constexpr uint64_t tape_int64 = uint64_t(tape_type::INT64)<<56;
  return doc->tape[json_index] == tape_int64;
}
simdjson_really_inline bool tape_ref::is_uint64() const noexcept {
  constexpr uint64_t tape_uint64 = uint64_t(tape_type::UINT64)<<56;
  return doc->tape[json_index] == tape_uint64;
}
simdjson_really_inline bool tape_ref::is_false() const noexcept {
  constexpr uint64_t tape_false = uint64_t(tape_type::FALSE_VALUE)<<56;
  return doc->tape[json_index] == tape_false;
}
simdjson_really_inline bool tape_ref::is_true() const noexcept {
  constexpr uint64_t tape_true = uint64_t(tape_type::TRUE_VALUE)<<56;
  return doc->tape[json_index] == tape_true;
}
simdjson_really_inline bool tape_ref::is_null_on_tape() const noexcept {
  constexpr uint64_t tape_null = uint64_t(tape_type::NULL_VALUE)<<56;
  return doc->tape[json_index] == tape_null;
}

inline size_t tape_ref::after_element() const noexcept {
  switch (tape_ref_type()) {
    case tape_type::START_ARRAY:
    case tape_type::START_OBJECT:
      return matching_brace_index();
    case tape_type::UINT64:
    case tape_type::INT64:
    case tape_type::DOUBLE:
      return json_index + 2;
    default:
      return json_index + 1;
  }
}
simdjson_really_inline tape_type tape_ref::tape_ref_type() const noexcept {
  return static_cast<tape_type>(doc->tape[json_index] >> 56);
}
simdjson_really_inline uint64_t internal::tape_ref::tape_value() const noexcept {
  return doc->tape[json_index] & internal::JSON_VALUE_MASK;
}
simdjson_really_inline uint32_t internal::tape_ref::matching_brace_index() const noexcept {
  return uint32_t(doc->tape[json_index]);
}
simdjson_really_inline uint32_t internal::tape_ref::scope_count() const noexcept {
  return uint32_t((doc->tape[json_index] >> 32) & internal::JSON_COUNT_MASK);
}

template<typename T>
simdjson_really_inline T tape_ref::next_tape_value() const noexcept {
  static_assert(sizeof(T) == sizeof(uint64_t), "next_tape_value() template parameter must be 64-bit");
  // Though the following is tempting...
  //  return *reinterpret_cast<const T*>(&doc->tape[json_index + 1]);
  // It is not generally safe. It is safer, and often faster to rely
  // on memcpy. Yes, it is uglier, but it is also encapsulated.
  T x;
  std::memcpy(&x,&doc->tape[json_index + 1],sizeof(uint64_t));
  return x;
}

simdjson_really_inline uint32_t internal::tape_ref::get_string_length() const noexcept {
  size_t string_buf_index = size_t(tape_value());
  uint32_t len;
  std::memcpy(&len, &doc->string_buf[string_buf_index], sizeof(len));
  return len;
}

simdjson_really_inline const char * internal::tape_ref::get_c_str() const noexcept {
  size_t string_buf_index = size_t(tape_value());
  return reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]);
}

inline std::string_view internal::tape_ref::get_string_view() const noexcept {
  return std::string_view(
      get_c_str(),
      get_string_length()
  );
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INLINE_TAPE_REF_H
/* end file include/simdjson/internal/tape_ref-inl.h */
/* begin file include/simdjson/dom/serialization-inl.h */

#ifndef SIMDJSON_SERIALIZATION_INL_H
#define SIMDJSON_SERIALIZATION_INL_H


#include <cinttypes>
#include <type_traits>

namespace simdjson {
namespace dom {
inline bool parser::print_json(std::ostream &os) const noexcept {
  if (!valid) { return false; }
  simdjson::internal::string_builder<> sb;
  sb.append(doc.root());
  std::string_view answer = sb.str();
  os << answer;
  return true;
}
}
/***
 * Number utility functions
 **/


namespace {
/**@private
 * Escape sequence like \b or \u0001
 * We expect that most compilers will use 8 bytes for this data structure.
 **/
struct escape_sequence {
    uint8_t length;
    const char string[7]; // technically, we only ever need 6 characters, we pad to 8
};
/**@private
 * This converts a signed integer into a character sequence.
 * The caller is responsible for providing enough memory (at least
 * 20 characters.)
 * Though various runtime libraries provide itoa functions,
 * it is not part of the C++ standard. The C++17 standard
 * adds the to_chars functions which would do as well, but
 * we want to support C++11.
 */
char *fast_itoa(char *output, int64_t value) noexcept {
  // This is a standard implementation of itoa.
  char buffer[20];
  uint64_t value_positive;
  // In general, negating a signed integer is unsafe.
  if(value < 0) {
    *output++ = '-';
    // Doing value_positive = -value; while avoiding
    // undefined behavior warnings.
    // It assumes two complement's which is universal at this
    // point in time.
    std::memcpy(&value_positive, &value, sizeof(value));
    value_positive = (~value_positive) + 1; // this is a negation
  } else {
    value_positive = value;
  }
  // We work solely with value_positive. It *might* be easier
  // for an optimizing compiler to deal with an unsigned variable
  // as far as performance goes.
  const char *const end_buffer = buffer + 20;
  char *write_pointer = buffer + 19;
  // A faster approach is possible if we expect large integers:
  // unroll the loop (work in 100s, 1000s) and use some kind of
  // memoization.
  while(value_positive >= 10) {
    *write_pointer-- = char('0' + (value_positive % 10));
    value_positive /= 10;
  }
  *write_pointer = char('0' + value_positive);
  size_t len = end_buffer - write_pointer;
  std::memcpy(output, write_pointer, len);
  return output + len;
}
/**@private
 * This converts an unsigned integer into a character sequence.
 * The caller is responsible for providing enough memory (at least
 * 19 characters.)
 * Though various runtime libraries provide itoa functions,
 * it is not part of the C++ standard. The C++17 standard
 * adds the to_chars functions which would do as well, but
 * we want to support C++11.
 */
char *fast_itoa(char *output, uint64_t value) noexcept {
  // This is a standard implementation of itoa.
  char buffer[20];
  const char *const end_buffer = buffer + 20;
  char *write_pointer = buffer + 19;
  // A faster approach is possible if we expect large integers:
  // unroll the loop (work in 100s, 1000s) and use some kind of
  // memoization.
  while(value >= 10) {
    *write_pointer-- = char('0' + (value % 10));
    value /= 10;
  };
  *write_pointer = char('0' + value);
  size_t len = end_buffer - write_pointer;
  std::memcpy(output, write_pointer, len);
  return output + len;
}
} // anonymous namespace
namespace internal {

/***
 * Minifier/formatter code.
 **/

simdjson_really_inline void mini_formatter::number(uint64_t x) {
  char number_buffer[24];
  char *newp = fast_itoa(number_buffer, x);
  buffer.insert(buffer.end(), number_buffer, newp);
}

simdjson_really_inline void mini_formatter::number(int64_t x) {
  char number_buffer[24];
  char *newp = fast_itoa(number_buffer, x);
  buffer.insert(buffer.end(), number_buffer, newp);
}

simdjson_really_inline void mini_formatter::number(double x) {
  char number_buffer[24];
  // Currently, passing the nullptr to the second argument is
  // safe because our implementation does not check the second
  // argument.
  char *newp = internal::to_chars(number_buffer, nullptr, x);
  buffer.insert(buffer.end(), number_buffer, newp);
}

simdjson_really_inline void mini_formatter::start_array() { one_char('['); }
simdjson_really_inline void mini_formatter::end_array() { one_char(']'); }
simdjson_really_inline void mini_formatter::start_object() { one_char('{'); }
simdjson_really_inline void mini_formatter::end_object() { one_char('}'); }
simdjson_really_inline void mini_formatter::comma() { one_char(','); }


simdjson_really_inline void mini_formatter::true_atom() {
  const char * s = "true";
  buffer.insert(buffer.end(), s, s + 4);
}
simdjson_really_inline void mini_formatter::false_atom() {
  const char * s = "false";
  buffer.insert(buffer.end(), s, s + 5);
}
simdjson_really_inline void mini_formatter::null_atom() {
  const char * s = "null";
  buffer.insert(buffer.end(), s, s + 4);
}
simdjson_really_inline void mini_formatter::one_char(char c) { buffer.push_back(c); }
simdjson_really_inline void mini_formatter::key(std::string_view unescaped) {
  string(unescaped);
  one_char(':');
}
simdjson_really_inline void mini_formatter::string(std::string_view unescaped) {
  one_char('\"');
  size_t i = 0;
  // Fast path for the case where we have no control character, no ", and no backslash.
  // This should include most keys.
  constexpr static bool needs_escaping[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for(;i + 8 <= unescaped.length(); i += 8) {
    // Poor's man vectorization. This could get much faster if we used SIMD.
    if(needs_escaping[uint8_t(unescaped[i])] | needs_escaping[uint8_t(unescaped[i+1])]
      | needs_escaping[uint8_t(unescaped[i+2])] | needs_escaping[uint8_t(unescaped[i+3])]
      | needs_escaping[uint8_t(unescaped[i+4])] | needs_escaping[uint8_t(unescaped[i+5])]
      | needs_escaping[uint8_t(unescaped[i+6])] | needs_escaping[uint8_t(unescaped[i+7])]
      ) { break; }
  }
  for(;i < unescaped.length(); i++) {
    if(needs_escaping[uint8_t(unescaped[i])]) { break; }
  }
  // The following is also possible and omits a 256-byte table, but it is slower:
  // for (; (i < unescaped.length()) && (uint8_t(unescaped[i]) > 0x1F)
  //      && (unescaped[i] != '\"') && (unescaped[i] != '\\'); i++) {}

  // At least for long strings, the following should be fast. We could
  // do better by integrating the checks and the insertion.
  buffer.insert(buffer.end(), unescaped.data(), unescaped.data() + i);
  // We caught a control character if we enter this loop (slow).
  // Note that we are do not restart from the beginning, but rather we continue
  // from the point where we encountered something that requires escaping.
  for (; i < unescaped.length(); i++) {
    switch (unescaped[i]) {
    case '\"':
      {
        const char * s = "\\\"";
        buffer.insert(buffer.end(), s, s + 2);
      }
      break;
    case '\\':
      {
        const char * s = "\\\\";
        buffer.insert(buffer.end(), s, s + 2);
      }
      break;
    default:
      if (uint8_t(unescaped[i]) <= 0x1F) {
        // If packed, this uses 8 * 32 bytes.
        // Note that we expect most compilers to embed this code in the data
        // section.
        constexpr static escape_sequence escaped[32] = {
          {6, "\\u0000"}, {6, "\\u0001"}, {6, "\\u0002"}, {6, "\\u0003"},
          {6, "\\u0004"}, {6, "\\u0005"}, {6, "\\u0006"}, {6, "\\u0007"},
          {2, "\\b"},     {2, "\\t"},     {2, "\\n"},     {6, "\\u000b"},
          {2, "\\f"},     {2, "\\r"},     {6, "\\u000e"}, {6, "\\u000f"},
          {6, "\\u0010"}, {6, "\\u0011"}, {6, "\\u0012"}, {6, "\\u0013"},
          {6, "\\u0014"}, {6, "\\u0015"}, {6, "\\u0016"}, {6, "\\u0017"},
          {6, "\\u0018"}, {6, "\\u0019"}, {6, "\\u001a"}, {6, "\\u001b"},
          {6, "\\u001c"}, {6, "\\u001d"}, {6, "\\u001e"}, {6, "\\u001f"}};
        auto u = escaped[uint8_t(unescaped[i])];
        buffer.insert(buffer.end(), u.string, u.string + u.length);
      } else {
        one_char(unescaped[i]);
      }
    } // switch
  }   // for
  one_char('\"');
}

inline void mini_formatter::clear() {
  buffer.clear();
}

simdjson_really_inline std::string_view mini_formatter::str() const {
  return std::string_view(buffer.data(), buffer.size());
}


/***
 * String building code.
 **/

template <class serializer>
inline void string_builder<serializer>::append(simdjson::dom::element value) {
  // using tape_type = simdjson::internal::tape_type;
  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  internal::tape_ref iter(value.tape);
  do {
    // print commas after each value
    if (after_value) {
      format.comma();
    }
    // If we are in an object, print the next key and :, and skip to the next
    // value.
    if (is_object[depth]) {
      format.key(iter.get_string_view());
      iter.json_index++;
    }
    switch (iter.tape_ref_type()) {

    // Arrays
    case tape_type::START_ARRAY: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (simdjson_unlikely(depth >= MAX_DEPTH)) {
        append(simdjson::dom::array(iter));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the ]
        depth--;
        break;
      }

      // Output start [
      format.start_array();
      iter.json_index++;

      // Handle empty [] (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_ARRAY) {
        format.end_array();
        depth--;
        break;
      }

      is_object[depth] = false;
      after_value = false;
      continue;
    }

    // Objects
    case tape_type::START_OBJECT: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (simdjson_unlikely(depth >= MAX_DEPTH)) {
        append(simdjson::dom::object(iter));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the }
        depth--;
        break;
      }

      // Output start {
      format.start_object();
      iter.json_index++;

      // Handle empty {} (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_OBJECT) {
        format.end_object();
        depth--;
        break;
      }

      is_object[depth] = true;
      after_value = false;
      continue;
    }

    // Scalars
    case tape_type::STRING:
      format.string(iter.get_string_view());
      break;
    case tape_type::INT64:
      format.number(iter.next_tape_value<int64_t>());
      iter.json_index++; // numbers take up 2 spots, so we need to increment
                         // extra
      break;
    case tape_type::UINT64:
      format.number(iter.next_tape_value<uint64_t>());
      iter.json_index++; // numbers take up 2 spots, so we need to increment
                         // extra
      break;
    case tape_type::DOUBLE:
      format.number(iter.next_tape_value<double>());
      iter.json_index++; // numbers take up 2 spots, so we need to increment
                         // extra
      break;
    case tape_type::TRUE_VALUE:
      format.true_atom();
      break;
    case tape_type::FALSE_VALUE:
      format.false_atom();
      break;
    case tape_type::NULL_VALUE:
      format.null_atom();
      break;

    // These are impossible
    case tape_type::END_ARRAY:
    case tape_type::END_OBJECT:
    case tape_type::ROOT:
      SIMDJSON_UNREACHABLE();
    }
    iter.json_index++;
    after_value = true;

    // Handle multiple ends in a row
    while (depth != 0 && (iter.tape_ref_type() == tape_type::END_ARRAY ||
                          iter.tape_ref_type() == tape_type::END_OBJECT)) {
      if (iter.tape_ref_type() == tape_type::END_ARRAY) {
        format.end_array();
      } else {
        format.end_object();
      }
      depth--;
      iter.json_index++;
    }

    // Stop when we're at depth 0
  } while (depth != 0);
}

template <class serializer>
inline void string_builder<serializer>::append(simdjson::dom::object value) {
  format.start_object();
  auto pair = value.begin();
  auto end = value.end();
  if (pair != end) {
    append(*pair);
    for (++pair; pair != end; ++pair) {
      format.comma();
      append(*pair);
    }
  }
  format.end_object();
}

template <class serializer>
inline void string_builder<serializer>::append(simdjson::dom::array value) {
  format.start_array();
  auto iter = value.begin();
  auto end = value.end();
  if (iter != end) {
    append(*iter);
    for (++iter; iter != end; ++iter) {
      format.comma();
      append(*iter);
    }
  }
  format.end_array();
}

template <class serializer>
simdjson_really_inline void string_builder<serializer>::append(simdjson::dom::key_value_pair kv) {
  format.key(kv.key);
  append(kv.value);
}

template <class serializer>
simdjson_really_inline void string_builder<serializer>::clear() {
  format.clear();
}

template <class serializer>
simdjson_really_inline std::string_view string_builder<serializer>::str() const {
  return format.str();
}


} // namespace internal
} // namespace simdjson

#endif
/* end file include/simdjson/dom/serialization-inl.h */

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_DOM_H
/* end file include/simdjson/dom.h */
/* begin file include/simdjson/builtin.h */
#ifndef SIMDJSON_BUILTIN_H
#define SIMDJSON_BUILTIN_H

/* begin file include/simdjson/implementations.h */
#ifndef SIMDJSON_IMPLEMENTATIONS_H
#define SIMDJSON_IMPLEMENTATIONS_H

/* begin file include/simdjson/implementation-base.h */
#ifndef SIMDJSON_IMPLEMENTATION_BASE_H
#define SIMDJSON_IMPLEMENTATION_BASE_H

/**
 * @file
 *
 * Includes common stuff needed for implementations.
 */


// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDJSON_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
/* begin file include/simdjson/internal/jsoncharutils_tables.h */
#ifndef SIMDJSON_INTERNAL_JSONCHARUTILS_TABLES_H
#define SIMDJSON_INTERNAL_JSONCHARUTILS_TABLES_H


#ifdef JSON_TEST_STRINGS
void found_string(const uint8_t *buf, const uint8_t *parsed_begin,
                  const uint8_t *parsed_end);
void found_bad_string(const uint8_t *buf);
#endif

namespace simdjson {
namespace internal {
// structural chars here are
// they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c (and NULL)
// we are also interested in the four whitespace characters
// space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d

extern SIMDJSON_DLLIMPORTEXPORT const bool structural_or_whitespace_negated[256];
extern SIMDJSON_DLLIMPORTEXPORT const bool structural_or_whitespace[256];
extern SIMDJSON_DLLIMPORTEXPORT const uint32_t digit_to_val32[886];

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JSONCHARUTILS_TABLES_H
/* end file include/simdjson/internal/jsoncharutils_tables.h */
/* begin file include/simdjson/internal/numberparsing_tables.h */
#ifndef SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H
#define SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H


namespace simdjson {
namespace internal {
/**
 * The smallest non-zero float (binary64) is 2^-1074.
 * We take as input numbers of the form w x 10^q where w < 2^64.
 * We have that w * 10^-343  <  2^(64-344) 5^-343 < 2^-1076.
 * However, we have that
 * (2^64-1) * 10^-342 =  (2^64-1) * 2^-342 * 5^-342 > 2^-1074.
 * Thus it is possible for a number of the form w * 10^-342 where
 * w is a 64-bit value to be a non-zero floating-point number.
 *********
 * Any number of form w * 10^309 where w>= 1 is going to be
 * infinite in binary64 so we never need to worry about powers
 * of 5 greater than 308.
 */
constexpr int smallest_power = -342;
constexpr int largest_power = 308;

/**
 * Represents a 128-bit value.
 * low: least significant 64 bits.
 * high: most significant 64 bits.
 */
struct value128 {
  uint64_t low;
  uint64_t high;
};


// Precomputed powers of ten from 10^0 to 10^22. These
// can be represented exactly using the double type.
extern SIMDJSON_DLLIMPORTEXPORT const double power_of_ten[];


/**
 * When mapping numbers from decimal to binary,
 * we go from w * 10^q to m * 2^p but we have
 * 10^q = 5^q * 2^q, so effectively
 * we are trying to match
 * w * 2^q * 5^q to m * 2^p. Thus the powers of two
 * are not a concern since they can be represented
 * exactly using the binary notation, only the powers of five
 * affect the binary significand.
 */


// The truncated powers of five from 5^-342 all the way to 5^308
// The mantissa is truncated to 128 bits, and
// never rounded up. Uses about 10KB.
extern SIMDJSON_DLLIMPORTEXPORT const uint64_t power_of_five_128[];
} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_NUMBERPARSING_TABLES_H
/* end file include/simdjson/internal/numberparsing_tables.h */
/* begin file include/simdjson/internal/simdprune_tables.h */
#ifndef SIMDJSON_INTERNAL_SIMDPRUNE_TABLES_H
#define SIMDJSON_INTERNAL_SIMDPRUNE_TABLES_H

#include <cstdint>

namespace simdjson { // table modified and copied from
namespace internal { // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetTable

extern SIMDJSON_DLLIMPORTEXPORT const unsigned char BitsSetTable256mul2[256];

extern SIMDJSON_DLLIMPORTEXPORT const uint8_t pshufb_combine_table[272];

// 256 * 8 bytes = 2kB, easily fits in cache.
extern SIMDJSON_DLLIMPORTEXPORT const uint64_t thintable_epi8[256];

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_SIMDPRUNE_TABLES_H
/* end file include/simdjson/internal/simdprune_tables.h */

#endif // SIMDJSON_IMPLEMENTATION_BASE_H
/* end file include/simdjson/implementation-base.h */

//
// First, figure out which implementations can be run. Doing it here makes it so we don't have to worry about the order
// in which we include them.
//

#ifndef SIMDJSON_IMPLEMENTATION_ARM64
#define SIMDJSON_IMPLEMENTATION_ARM64 (SIMDJSON_IS_ARM64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_ARM64 SIMDJSON_IMPLEMENTATION_ARM64 && SIMDJSON_IS_ARM64

// Default Haswell to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef SIMDJSON_IMPLEMENTATION_HASWELL
#define SIMDJSON_IMPLEMENTATION_HASWELL (SIMDJSON_IS_X86_64)
#endif
// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdjson/simdjson/issues/1247
#define SIMDJSON_CAN_ALWAYS_RUN_HASWELL ((SIMDJSON_IMPLEMENTATION_HASWELL) && (SIMDJSON_IS_X86_64) && (__AVX2__))

// Default Westmere to on if this is x86-64, unless we'll always select Haswell.
#ifndef SIMDJSON_IMPLEMENTATION_WESTMERE
#define SIMDJSON_IMPLEMENTATION_WESTMERE (SIMDJSON_IS_X86_64 && !SIMDJSON_REQUIRES_HASWELL)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_WESTMERE (SIMDJSON_IMPLEMENTATION_WESTMERE && SIMDJSON_IS_X86_64 && __SSE4_2__ && __PCLMUL__)

#ifndef SIMDJSON_IMPLEMENTATION_PPC64
#define SIMDJSON_IMPLEMENTATION_PPC64 (SIMDJSON_IS_PPC64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_PPC64 SIMDJSON_IMPLEMENTATION_PPC64 && SIMDJSON_IS_PPC64

// Default Fallback to on unless a builtin implementation has already been selected.
#ifndef SIMDJSON_IMPLEMENTATION_FALLBACK
#define SIMDJSON_IMPLEMENTATION_FALLBACK 1 // (!SIMDJSON_CAN_ALWAYS_RUN_ARM64 && !SIMDJSON_CAN_ALWAYS_RUN_HASWELL && !SIMDJSON_CAN_ALWAYS_RUN_WESTMERE && !SIMDJSON_CAN_ALWAYS_RUN_PPC64)
#endif
#define SIMDJSON_CAN_ALWAYS_RUN_FALLBACK SIMDJSON_IMPLEMENTATION_FALLBACK

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

// Implementations
/* begin file include/simdjson/arm64.h */
#ifndef SIMDJSON_ARM64_H
#define SIMDJSON_ARM64_H


#if SIMDJSON_IMPLEMENTATION_ARM64

namespace simdjson {
/**
 * Implementation for NEON (ARMv8).
 */
namespace arm64 {
} // namespace arm64
} // namespace simdjson

/* begin file include/simdjson/arm64/implementation.h */
#ifndef SIMDJSON_ARM64_IMPLEMENTATION_H
#define SIMDJSON_ARM64_IMPLEMENTATION_H


namespace simdjson {
namespace arm64 {

namespace {
using namespace simdjson;
using namespace simdjson::dom;
}

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation("arm64", "ARM NEON", internal::instruction_set::NEON) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_IMPLEMENTATION_H
/* end file include/simdjson/arm64/implementation.h */

/* begin file include/simdjson/arm64/begin.h */
// redefining SIMDJSON_IMPLEMENTATION to "arm64"
// #define SIMDJSON_IMPLEMENTATION arm64
/* end file include/simdjson/arm64/begin.h */

// Declarations
/* begin file include/simdjson/generic/dom_parser_implementation.h */

namespace simdjson {
namespace arm64 {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  inline dom_parser_implementation() noexcept;
  inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation &operator=(const dom_parser_implementation &) = delete;

  simdjson_warn_unused error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  simdjson_warn_unused error_code stage2(dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage2_next(dom::document &doc) noexcept final;
  inline simdjson_warn_unused error_code set_capacity(size_t capacity) noexcept final;
  inline simdjson_warn_unused error_code set_max_depth(size_t max_depth) noexcept final;
private:
  simdjson_really_inline simdjson_warn_unused error_code set_capacity_stage1(size_t capacity);

};

} // namespace arm64
} // namespace simdjson

namespace simdjson {
namespace arm64 {

inline dom_parser_implementation::dom_parser_implementation() noexcept = default;
inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

// Leaving these here so they can be inlined if so desired
inline simdjson_warn_unused error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  if(capacity > SIMDJSON_MAXSIZE_BYTES) { return CAPACITY; }
  // Stage 1 index output
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!structural_indexes) { _capacity = 0; return MEMALLOC; }
  structural_indexes[0] = 0;
  n_structural_indexes = 0;

  _capacity = capacity;
  return SUCCESS;
}

inline simdjson_warn_unused error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  // Stage 2 stacks
  open_containers.reset(new (std::nothrow) open_container[max_depth]);
  is_array.reset(new (std::nothrow) bool[max_depth]);
  if (!is_array || !open_containers) { _max_depth = 0; return MEMALLOC; }

  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace arm64
} // namespace simdjson
/* end file include/simdjson/generic/dom_parser_implementation.h */
/* begin file include/simdjson/arm64/intrinsics.h */
#ifndef SIMDJSON_ARM64_INTRINSICS_H
#define SIMDJSON_ARM64_INTRINSICS_H

// This should be the correct header whether
// you use visual studio or other compilers.
#include <arm_neon.h>

#endif //  SIMDJSON_ARM64_INTRINSICS_H
/* end file include/simdjson/arm64/intrinsics.h */
/* begin file include/simdjson/arm64/bitmanipulation.h */
#ifndef SIMDJSON_ARM64_BITMANIPULATION_H
#define SIMDJSON_ARM64_BITMANIPULATION_H

namespace simdjson {
namespace arm64 {
namespace {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
simdjson_really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long ret;
  // Search the mask data from least significant bit (LSB)
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB)
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int count_ones(uint64_t input_num) {
   return vaddv_u8(vcnt_u8(vcreate_u8(input_num)));
}

simdjson_really_inline bool add_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  *result = value1 + value2;
  return *result < value1;
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   reinterpret_cast<unsigned long long *>(result));
#endif
}

} // unnamed namespace
} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_BITMANIPULATION_H
/* end file include/simdjson/arm64/bitmanipulation.h */
/* begin file include/simdjson/arm64/bitmask.h */
#ifndef SIMDJSON_ARM64_BITMASK_H
#define SIMDJSON_ARM64_BITMASK_H

namespace simdjson {
namespace arm64 {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(uint64_t bitmask) {
  /////////////
  // We could do this with PMULL, but it is apparently slow.
  //
  //#ifdef __ARM_FEATURE_CRYPTO // some ARM processors lack this extension
  //return vmull_p64(-1ULL, bitmask);
  //#else
  // Analysis by @sebpop:
  // When diffing the assembly for src/stage1_find_marks.cpp I see that the eors are all spread out
  // in between other vector code, so effectively the extra cycles of the sequence do not matter
  // because the GPR units are idle otherwise and the critical path is on the FP side.
  // Also the PMULL requires two extra fmovs: GPR->FP (3 cycles in N1, 5 cycles in A72 )
  // and FP->GPR (2 cycles on N1 and 5 cycles on A72.)
  ///////////
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // unnamed namespace
} // namespace arm64
} // namespace simdjson

#endif
/* end file include/simdjson/arm64/bitmask.h */
/* begin file include/simdjson/arm64/simd.h */
#ifndef SIMDJSON_ARM64_SIMD_H
#define SIMDJSON_ARM64_SIMD_H

#include <type_traits>


namespace simdjson {
namespace arm64 {
namespace {
namespace simd {

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
namespace {
// Start of private section with Visual Studio workaround


/**
 * make_uint8x16_t initializes a SIMD register (uint8x16_t).
 * This is needed because, incredibly, the syntax uint8x16_t x = {1,2,3...}
 * is not recognized under Visual Studio! This is a workaround.
 * Using a std::initializer_list<uint8_t>  as a parameter resulted in
 * inefficient code. With the current approach, if the parameters are
 * compile-time constants,
 * GNU GCC compiles it to ldr, the same as uint8x16_t x = {1,2,3...}.
 * You should not use this function except for compile-time constants:
 * it is not efficient.
 */
simdjson_really_inline uint8x16_t make_uint8x16_t(uint8_t x1,  uint8_t x2,  uint8_t x3,  uint8_t x4,
                                         uint8_t x5,  uint8_t x6,  uint8_t x7,  uint8_t x8,
                                         uint8_t x9,  uint8_t x10, uint8_t x11, uint8_t x12,
                                         uint8_t x13, uint8_t x14, uint8_t x15, uint8_t x16) {
  // Doing a load like so end ups generating worse code.
  // uint8_t array[16] = {x1, x2, x3, x4, x5, x6, x7, x8,
  //                     x9, x10,x11,x12,x13,x14,x15,x16};
  // return vld1q_u8(array);
  uint8x16_t x{};
  // incredibly, Visual Studio does not allow x[0] = x1
  x = vsetq_lane_u8(x1, x, 0);
  x = vsetq_lane_u8(x2, x, 1);
  x = vsetq_lane_u8(x3, x, 2);
  x = vsetq_lane_u8(x4, x, 3);
  x = vsetq_lane_u8(x5, x, 4);
  x = vsetq_lane_u8(x6, x, 5);
  x = vsetq_lane_u8(x7, x, 6);
  x = vsetq_lane_u8(x8, x, 7);
  x = vsetq_lane_u8(x9, x, 8);
  x = vsetq_lane_u8(x10, x, 9);
  x = vsetq_lane_u8(x11, x, 10);
  x = vsetq_lane_u8(x12, x, 11);
  x = vsetq_lane_u8(x13, x, 12);
  x = vsetq_lane_u8(x14, x, 13);
  x = vsetq_lane_u8(x15, x, 14);
  x = vsetq_lane_u8(x16, x, 15);
  return x;
}


// We have to do the same work for make_int8x16_t
simdjson_really_inline int8x16_t make_int8x16_t(int8_t x1,  int8_t x2,  int8_t x3,  int8_t x4,
                                       int8_t x5,  int8_t x6,  int8_t x7,  int8_t x8,
                                       int8_t x9,  int8_t x10, int8_t x11, int8_t x12,
                                       int8_t x13, int8_t x14, int8_t x15, int8_t x16) {
  // Doing a load like so end ups generating worse code.
  // int8_t array[16] = {x1, x2, x3, x4, x5, x6, x7, x8,
  //                     x9, x10,x11,x12,x13,x14,x15,x16};
  // return vld1q_s8(array);
  int8x16_t x{};
  // incredibly, Visual Studio does not allow x[0] = x1
  x = vsetq_lane_s8(x1, x, 0);
  x = vsetq_lane_s8(x2, x, 1);
  x = vsetq_lane_s8(x3, x, 2);
  x = vsetq_lane_s8(x4, x, 3);
  x = vsetq_lane_s8(x5, x, 4);
  x = vsetq_lane_s8(x6, x, 5);
  x = vsetq_lane_s8(x7, x, 6);
  x = vsetq_lane_s8(x8, x, 7);
  x = vsetq_lane_s8(x9, x, 8);
  x = vsetq_lane_s8(x10, x, 9);
  x = vsetq_lane_s8(x11, x, 10);
  x = vsetq_lane_s8(x12, x, 11);
  x = vsetq_lane_s8(x13, x, 12);
  x = vsetq_lane_s8(x14, x, 13);
  x = vsetq_lane_s8(x15, x, 14);
  x = vsetq_lane_s8(x16, x, 15);
  return x;
}

// End of private section with Visual Studio workaround
} // namespace
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO


  template<typename T>
  struct simd8;

  //
  // Base class of simd8<uint8_t> and simd8<bool>, both of which use uint8x16_t internally.
  //
  template<typename T, typename Mask=simd8<bool>>
  struct base_u8 {
    uint8x16_t value;
    static const int SIZE = sizeof(value);

    // Conversion from/to SIMD register
    simdjson_really_inline base_u8(const uint8x16_t _value) : value(_value) {}
    simdjson_really_inline operator const uint8x16_t&() const { return this->value; }
    simdjson_really_inline operator uint8x16_t&() { return this->value; }

    // Bit operations
    simdjson_really_inline simd8<T> operator|(const simd8<T> other) const { return vorrq_u8(*this, other); }
    simdjson_really_inline simd8<T> operator&(const simd8<T> other) const { return vandq_u8(*this, other); }
    simdjson_really_inline simd8<T> operator^(const simd8<T> other) const { return veorq_u8(*this, other); }
    simdjson_really_inline simd8<T> bit_andnot(const simd8<T> other) const { return vbicq_u8(*this, other); }
    simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }
    simdjson_really_inline simd8<T>& operator|=(const simd8<T> other) { auto this_cast = static_cast<simd8<T>*>(this); *this_cast = *this_cast | other; return *this_cast; }
    simdjson_really_inline simd8<T>& operator&=(const simd8<T> other) { auto this_cast = static_cast<simd8<T>*>(this); *this_cast = *this_cast & other; return *this_cast; }
    simdjson_really_inline simd8<T>& operator^=(const simd8<T> other) { auto this_cast = static_cast<simd8<T>*>(this); *this_cast = *this_cast ^ other; return *this_cast; }

    simdjson_really_inline Mask operator==(const simd8<T> other) const { return vceqq_u8(*this, other); }

    template<int N=1>
    simdjson_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return vextq_u8(prev_chunk, *this, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base_u8<bool> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    static simdjson_really_inline simd8<bool> splat(bool _value) { return vmovq_n_u8(uint8_t(-(!!_value))); }

    simdjson_really_inline simd8(const uint8x16_t _value) : base_u8<bool>(_value) {}
    // False constructor
    simdjson_really_inline simd8() : simd8(vdupq_n_u8(0)) {}
    // Splat constructor
    simdjson_really_inline simd8(bool _value) : simd8(splat(_value)) {}

    // We return uint32_t instead of uint16_t because that seems to be more efficient for most
    // purposes (cutting it down to uint16_t costs performance in some compilers).
    simdjson_really_inline uint32_t to_bitmask() const {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
      const uint8x16_t bit_mask =  make_uint8x16_t(0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                                   0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80);
#else
      const uint8x16_t bit_mask =  {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                    0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
#endif
      auto minput = *this & bit_mask;
      uint8x16_t tmp = vpaddq_u8(minput, minput);
      tmp = vpaddq_u8(tmp, tmp);
      tmp = vpaddq_u8(tmp, tmp);
      return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
    }
    simdjson_really_inline bool any() const { return vmaxvq_u8(*this) != 0; }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base_u8<uint8_t> {
    static simdjson_really_inline uint8x16_t splat(uint8_t _value) { return vmovq_n_u8(_value); }
    static simdjson_really_inline uint8x16_t zero() { return vdupq_n_u8(0); }
    static simdjson_really_inline uint8x16_t load(const uint8_t* values) { return vld1q_u8(values); }

    simdjson_really_inline simd8(const uint8x16_t _value) : base_u8<uint8_t>(_value) {}
    // Zero constructor
    simdjson_really_inline simd8() : simd8(zero()) {}
    // Array constructor
    simdjson_really_inline simd8(const uint8_t values[16]) : simd8(load(values)) {}
    // Splat constructor
    simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Member-by-member initialization
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(make_uint8x16_t(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
#else
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(uint8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
#endif

    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    simdjson_really_inline void store(uint8_t dst[16]) const { return vst1q_u8(dst, *this); }

    // Saturated math
    simdjson_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return vqaddq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return vqsubq_u8(*this, other); }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_really_inline simd8<uint8_t> operator+(const simd8<uint8_t> other) const { return vaddq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t> operator-(const simd8<uint8_t> other) const { return vsubq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t>& operator+=(const simd8<uint8_t> other) { *this = *this + other; return *this; }
    simdjson_really_inline simd8<uint8_t>& operator-=(const simd8<uint8_t> other) { *this = *this - other; return *this; }

    // Order-specific operations
    simdjson_really_inline uint8_t max_val() const { return vmaxvq_u8(*this); }
    simdjson_really_inline uint8_t min_val() const { return vminvq_u8(*this); }
    simdjson_really_inline simd8<uint8_t> max_val(const simd8<uint8_t> other) const { return vmaxq_u8(*this, other); }
    simdjson_really_inline simd8<uint8_t> min_val(const simd8<uint8_t> other) const { return vminq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return vcleq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return vcgeq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return vcltq_u8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return vcgtq_u8(*this, other); }
    // Same as >, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    simdjson_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this > other); }
    // Same as <, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    simdjson_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this < other); }

    // Bit-specific operations
    simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return vtstq_u8(*this, bits); }
    simdjson_really_inline bool any_bits_set_anywhere() const { return this->max_val() != 0; }
    simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return (*this & bits).any_bits_set_anywhere(); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shr() const { return vshrq_n_u8(*this, N); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shl() const { return vshlq_n_u8(*this, N); }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }


    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 16 - count_ones(mask) bytes of the result are significant but 16 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint16_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    simdjson_really_inline void compress(uint16_t mask, L * output) const {
      using internal::thintable_epi8;
      using internal::BitsSetTable256mul2;
      using internal::pshufb_combine_table;
      // this particular implementation was inspired by work done by @animetosho
      // we do it in two steps, first 8 bytes and then second 8 bytes
      uint8_t mask1 = uint8_t(mask); // least significant 8 bits
      uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      uint64x2_t shufmask64 = {thintable_epi8[mask1], thintable_epi8[mask2]};
      uint8x16_t shufmask = vreinterpretq_u8_u64(shufmask64);
      // we increment by 0x08 the second half of the mask
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
      uint8x16_t inc = make_uint8x16_t(0, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08);
#else
      uint8x16_t inc = {0, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};
#endif
      shufmask = vaddq_u8(shufmask, inc);
      // this is the version "nearly pruned"
      uint8x16_t pruned = vqtbl1q_u8(*this, shufmask);
      // we still need to put the two halves together.
      // we compute the popcount of the first half:
      int pop1 = BitsSetTable256mul2[mask1];
      // then load the corresponding mask, what it does is to write
      // only the first pop1 bytes from the first 8 bytes, and then
      // it fills in with the bytes from the second 8 bytes + some filling
      // at the end.
      uint8x16_t compactmask = vld1q_u8(reinterpret_cast<const uint8_t *>(pshufb_combine_table + pop1 * 8));
      uint8x16_t answer = vqtbl1q_u8(pruned, compactmask);
      vst1q_u8(reinterpret_cast<uint8_t*>(output), answer);
    }

    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }

    template<typename T>
    simdjson_really_inline simd8<uint8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_u8(*this, simd8<uint8_t>(original));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> {
    int8x16_t value;

    static simdjson_really_inline simd8<int8_t> splat(int8_t _value) { return vmovq_n_s8(_value); }
    static simdjson_really_inline simd8<int8_t> zero() { return vdupq_n_s8(0); }
    static simdjson_really_inline simd8<int8_t> load(const int8_t values[16]) { return vld1q_s8(values); }

    // Conversion from/to SIMD register
    simdjson_really_inline simd8(const int8x16_t _value) : value{_value} {}
    simdjson_really_inline operator const int8x16_t&() const { return this->value; }
    simdjson_really_inline operator int8x16_t&() { return this->value; }

    // Zero constructor
    simdjson_really_inline simd8() : simd8(zero()) {}
    // Splat constructor
    simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(make_int8x16_t(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
#else
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(int8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
#endif
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Store to array
    simdjson_really_inline void store(int8_t dst[16]) const { return vst1q_s8(dst, *this); }

    // Explicit conversion to/from unsigned
    //
    // Under Visual Studio/ARM64 uint8x16_t and int8x16_t are apparently the same type.
    // In theory, we could check this occurence with std::same_as and std::enabled_if but it is C++14
    // and relatively ugly and hard to read.
#ifndef SIMDJSON_REGULAR_VISUAL_STUDIO
    simdjson_really_inline explicit simd8(const uint8x16_t other): simd8(vreinterpretq_s8_u8(other)) {}
#endif
    simdjson_really_inline explicit operator simd8<uint8_t>() const { return vreinterpretq_u8_s8(this->value); }

    // Math
    simdjson_really_inline simd8<int8_t> operator+(const simd8<int8_t> other) const { return vaddq_s8(*this, other); }
    simdjson_really_inline simd8<int8_t> operator-(const simd8<int8_t> other) const { return vsubq_s8(*this, other); }
    simdjson_really_inline simd8<int8_t>& operator+=(const simd8<int8_t> other) { *this = *this + other; return *this; }
    simdjson_really_inline simd8<int8_t>& operator-=(const simd8<int8_t> other) { *this = *this - other; return *this; }

    // Order-sensitive comparisons
    simdjson_really_inline simd8<int8_t> max_val(const simd8<int8_t> other) const { return vmaxq_s8(*this, other); }
    simdjson_really_inline simd8<int8_t> min_val(const simd8<int8_t> other) const { return vminq_s8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return vcgtq_s8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return vcltq_s8(*this, other); }
    simdjson_really_inline simd8<bool> operator==(const simd8<int8_t> other) const { return vceqq_s8(*this, other); }

    template<int N=1>
    simdjson_really_inline simd8<int8_t> prev(const simd8<int8_t> prev_chunk) const {
      return vextq_s8(prev_chunk, *this, 16 - N);
    }

    // Perform a lookup assuming no value is larger than 16
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }

    template<typename T>
    simdjson_really_inline simd8<int8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_s8(*this, simd8<uint8_t>(original));
    }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 4, "ARM kernel should use four registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T>& other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    simdjson_really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    simdjson_really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    simdjson_really_inline simd8<T> reduce_or() const {
      return (this->chunks[0] | this->chunks[1]) | (this->chunks[2] | this->chunks[3]);
    }


    simdjson_really_inline void compress(uint64_t mask, T * output) const {
      this->chunks[0].compress(uint16_t(mask), output);
      this->chunks[1].compress(uint16_t(mask >> 16), output + 16 - count_ones(mask & 0xFFFF));
      this->chunks[2].compress(uint16_t(mask >> 32), output + 32 - count_ones(mask & 0xFFFFFFFF));
      this->chunks[3].compress(uint16_t(mask >> 48), output + 48 - count_ones(mask & 0xFFFFFFFFFFFF));
    }

    simdjson_really_inline uint64_t to_bitmask() const {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
      const uint8x16_t bit_mask = make_uint8x16_t(
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      );
#else
      const uint8x16_t bit_mask = {
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      };
#endif
      // Add each of the elements next to each other, successively, to stuff each 8 byte mask into one.
      uint8x16_t sum0 = vpaddq_u8(this->chunks[0] & bit_mask, this->chunks[1] & bit_mask);
      uint8x16_t sum1 = vpaddq_u8(this->chunks[2] & bit_mask, this->chunks[3] & bit_mask);
      sum0 = vpaddq_u8(sum0, sum1);
      sum0 = vpaddq_u8(sum0, sum0);
      return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
    }

    simdjson_really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] == mask,
        this->chunks[1] == mask,
        this->chunks[2] == mask,
        this->chunks[3] == mask
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] <= mask,
        this->chunks[1] <= mask,
        this->chunks[2] <= mask,
        this->chunks[3] <= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simd
} // unnamed namespace
} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_SIMD_H
/* end file include/simdjson/arm64/simd.h */
/* begin file include/simdjson/generic/jsoncharutils.h */

namespace simdjson {
namespace arm64 {
namespace {
namespace jsoncharutils {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = internal::digit_to_val32[630 + src[0]];
  uint32_t v2 = internal::digit_to_val32[420 + src[1]];
  uint32_t v3 = internal::digit_to_val32[210 + src[2]];
  uint32_t v4 = internal::digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

using internal::value128;

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace jsoncharutils
} // unnamed namespace
} // namespace arm64
} // namespace simdjson
/* end file include/simdjson/generic/jsoncharutils.h */
/* begin file include/simdjson/generic/atomparsing.h */
namespace simdjson {
namespace arm64 {
namespace {
/// @private
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
simdjson_warn_unused
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | jsoncharutils::is_not_structural_or_whitespace(src[5])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // unnamed namespace
} // namespace arm64
} // namespace simdjson
/* end file include/simdjson/generic/atomparsing.h */
/* begin file include/simdjson/arm64/stringparsing.h */
#ifndef SIMDJSON_ARM64_STRINGPARSING_H
#define SIMDJSON_ARM64_STRINGPARSING_H


namespace simdjson {
namespace arm64 {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_really_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_really_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_really_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + sizeof(v0));
  v0.store(dst);
  v1.store(dst + sizeof(v0));

  // Getting a 64-bit bitmask is much cheaper than multiple 16-bit bitmasks on ARM; therefore, we
  // smash them together into a 64-byte mask and get the bitmask from there.
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    uint32_t(bs_and_quote),      // bs_bits
    uint32_t(bs_and_quote >> 32) // quote_bits
  };
}

} // unnamed namespace
} // namespace arm64
} // namespace simdjson

/* begin file include/simdjson/generic/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace simdjson {
namespace arm64 {
namespace {
/// @private
namespace stringparsing {

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
simdjson_warn_unused
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // jsoncharutils::hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = jsoncharutils::codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

/**
 * Unescape a string from src to dst, stopping at a final unescaped quote. E.g., if src points at 'joe"', then
 * dst needs to have four free bytes.
 */
simdjson_warn_unused simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

simdjson_unused simdjson_warn_unused simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (*(src++) != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view(reinterpret_cast<const char *>(current_string_buf_loc), end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // unnamed namespace
} // namespace arm64
} // namespace simdjson
/* end file include/simdjson/generic/stringparsing.h */

#endif // SIMDJSON_ARM64_STRINGPARSING_H
/* end file include/simdjson/arm64/stringparsing.h */
/* begin file include/simdjson/arm64/numberparsing.h */
#ifndef SIMDJSON_ARM64_NUMBERPARSING_H
#define SIMDJSON_ARM64_NUMBERPARSING_H

namespace simdjson {
namespace arm64 {
namespace {

// we don't have SSE, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  uint64_t val;
  std::memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

} // unnamed namespace
} // namespace arm64
} // namespace simdjson

#define SWAR_NUMBER_PARSING

/* begin file include/simdjson/generic/numberparsing.h */
#include <limits>

namespace simdjson {
namespace arm64 {
namespace {
/// @private
namespace numberparsing {



#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

namespace {
// Convert a mantissa, an exponent and a sign bit into an ieee64 double.
// The real_exponent needs to be in [0, 2046] (technically real_exponent = 2047 would be acceptable).
// The mantissa should be in [0,1<<53). The bit at index (1ULL << 52) while be zeroed.
simdjson_really_inline double to_double(uint64_t mantissa, uint64_t real_exponent, bool negative) {
    double d;
    mantissa &= ~(1ULL << 52);
    mantissa |= real_exponent << 52;
    mantissa |= ((static_cast<uint64_t>(negative)) << 63);
    std::memcpy(&d, &mantissa, sizeof(d));
    return d;
}
}
// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [smallest_power,
// largest_power] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / simdjson::internal::power_of_ten[-power];
    } else {
      d = d * simdjson::internal::power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It was
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }


  // The exponent is 1024 + 63 + power
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power when power >= 0
  // and it is equal to
  //  ceil(log(5**-power)/log(2)) + power when power < 0
  //
  // The 65536 is (1<<16) and corresponds to
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to
  // floor(log(5**power)/log(2))
  //
  // Note that this is not magic: 152170/(1<<16) is
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;


  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;


  // We are going to need to do some 64-bit arithmetic to get a precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= smallest_power
  // and power <= largest_power
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  //
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  const uint32_t index = 2 * uint32_t(power - simdjson::internal::smallest_power);
  // Optimization: It may be that materializing the index as a variable might confuse some compilers and prevent effective complex-addressing loads. (Done for code clarity.)
  //
  // The full_multiplication function computes the 128-bit product of two 64-bit words
  // with a returned value of type value128 with a "low component" corresponding to the
  // 64-bit least significant bits of the product and with a "high component" corresponding
  // to the 64-bit most significant bits of the product.
  simdjson::internal::value128 firstproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index]);
  // Both i and power_of_five_128[index] have their most significant bit set to 1 which
  // implies that the either the most or the second most significant bit of the product
  // is 1. We pack values in this manner for efficiency reasons: it maximizes the use
  // we make of the product. It also makes it easy to reason aboutthe product: there
  // 0 or 1 leading zero in the product.

  // Unless the least significant 9 bits of the high (64-bit) part of the full
  // product are all 1s, then we know that the most significant 55 bits are
  // exact and no further work is needed. Having 55 bits is necessary because
  // we need 53 bits for the mantissa but we have to have one rounding bit and
  // we can waste a bit if the most significant bit of the product is zero.
  if((firstproduct.high & 0x1FF) == 0x1FF) {
    // We want to compute i * 5^q, but only care about the top 55 bits at most.
    // Consider the scenario where q>=0. Then 5^q may not fit in 64-bits. Doing
    // the full computation is wasteful. So we do what is called a "truncated
    // multiplication".
    // We take the most significant 64-bits, and we put them in
    // power_of_five_128[index]. Usually, that's good enough to approximate i * 5^q
    // to the desired approximation using one multiplication. Sometimes it does not suffice.
    // Then we store the next most significant 64 bits in power_of_five_128[index + 1], and
    // then we get a better approximation to i * 5^q. In very rare cases, even that
    // will not suffice, though it is seemingly very hard to find such a scenario.
    //
    // That's for when q>=0. The logic for q<0 is somewhat similar but it is somewhat
    // more complicated.
    //
    // There is an extra layer of complexity in that we need more than 55 bits of
    // accuracy in the round-to-even scenario.
    //
    // The full_multiplication function computes the 128-bit product of two 64-bit words
    // with a returned value of type value128 with a "low component" corresponding to the
    // 64-bit least significant bits of the product and with a "high component" corresponding
    // to the 64-bit most significant bits of the product.
    simdjson::internal::value128 secondproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index + 1]);
    firstproduct.low += secondproduct.high;
    if(secondproduct.high > firstproduct.low) { firstproduct.high++; }
    // At this point, we might need to add at most one to firstproduct, but this
    // can only change the value of firstproduct.high if firstproduct.low is maximal.
    if(simdjson_unlikely(firstproduct.low  == 0xFFFFFFFFFFFFFFFF)) {
      // This is very unlikely, but if so, we need to do much more work!
      return false;
    }
  }
  uint64_t lower = firstproduct.low;
  uint64_t upper = firstproduct.high;
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).
  int64_t real_exponent = exponent - lz;
  if (simdjson_unlikely(real_exponent <= 0)) { // we have a subnormal?
    // Here have that real_exponent <= 0 so -real_exponent >= 0
    if(-real_exponent + 1 >= 64) { // if we have more than 64 bits below the minimum exponent, you have a zero for sure.
      d = 0.0;
      return true;
    }
    // next line is safe because -real_exponent + 1 < 0
    mantissa >>= -real_exponent + 1;
    // Thankfully, we can't have both "round-to-even" and subnormals because
    // "round-to-even" only occurs for powers close to 0.
    mantissa += (mantissa & 1); // round up
    mantissa >>= 1;
    // There is a weird scenario where we don't have a subnormal but just.
    // Suppose we start with 2.2250738585072013e-308, we end up
    // with 0x3fffffffffffff x 2^-1023-53 which is technically subnormal
    // whereas 0x40000000000000 x 2^-1023-53  is normal. Now, we need to round
    // up 0x3fffffffffffff x 2^-1023-53  and once we do, we are no longer
    // subnormal, but we can only know this after rounding.
    // So we only declare a subnormal if we are smaller than the threshold.
    real_exponent = (mantissa < (uint64_t(1) << 52)) ? 0 : 1;
    d = to_double(mantissa, real_exponent, negative);
    return true;
  }
  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  //
  // The round-to-even cases take the form of a number 2m+1 which is in (2^53,2^54]
  // times a power of two. That is, it is right between a number with binary significand
  // m and another number with binary significand m+1; and it must be the case
  // that it cannot be represented by a float itself.
  //
  // We must have that w * 10 ^q == (2m+1) * 2^p for some power of two 2^p.
  // Recall that 10^q = 5^q * 2^q.
  // When q >= 0, we must have that (2m+1) is divible by 5^q, so 5^q <= 2^54. We have that
  //  5^23 <=  2^54 and it is the last power of five to qualify, so q <= 23.
  // When q<0, we have  w  >=  (2m+1) x 5^{-q}.  We must have that w<2^{64} so
  // (2m+1) x 5^{-q} < 2^{64}. We have that 2m+1>2^{53}. Hence, we must have
  // 2^{53} x 5^{-q} < 2^{64}.
  // Hence we have 5^{-q} < 2^{11}$ or q>= -4.
  //
  // We require lower <= 1 and not lower == 0 because we could not prove that
  // that lower == 0 is implied; but we could prove that lower <= 1 is a necessary and sufficient test.
  if (simdjson_unlikely((lower <= 1) && (power >= -4) && (power <= 23) && ((mantissa & 3) == 1))) {
    if((mantissa  << (upperbit + 64 - 53 - 2)) ==  upper) {
      mantissa &= ~1;             // flip it so that we do not round up
    }
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    real_exponent++;
  }
  mantissa &= ~(1ULL << 52);
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely(real_exponent > 2046)) {
    // We have an infinte value!!! We could actually throw an error here if we could.
    return false;
  }
  d = to_double(mantissa, real_exponent, negative);
  return true;
}

// We call a fallback floating-point parser that might be slow. Note
// it will accept JSON numbers, but the JSON spec. is more restrictive so
// before you call parse_float_fallback, you need to have validated the input
// string with the JSON grammar.
// It will return an error (false) if the parsed number is infinite.
// The string parsing itself always succeeds. We know that there is at least
// one digit.
static bool parse_float_fallback(const uint8_t *ptr, double *outDouble) {
  *outDouble = simdjson::internal::from_chars(reinterpret_cast<const char *>(ptr));
  // We do not accept infinite values.

  // Detecting finite values in a portable manner is ridiculously hard, ideally
  // we would want to do:
  // return !std::isfinite(*outDouble);
  // but that mysteriously fails under legacy/old libc++ libraries, see
  // https://github.com/simdjson/simdjson/issues/1286
  //
  // Therefore, fall back to this solution (the extra parens are there
  // to handle that max may be a macro on windows).
  return !(*outDouble > (std::numeric_limits<double>::max)() || *outDouble < std::numeric_limits<double>::lowest());
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  std::memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(simdjson_unused const uint8_t * src, W writer) {
  double d;
  if (parse_float_fallback(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(simdjson_unused const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(simdjson_unused const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow.
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time.
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline size_t significant_digits(const uint8_t * start_digits, size_t digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - size_t(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, size_t digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  //
  // 9999999999999999999 < 2**64 so we can accomodate 19 digits.
  // If we have a decimal separator, then digit_count - 1 is the number of digits, but we
  // may not have a decimal separator!
  if (simdjson_unlikely(digit_count > 19 && significant_digits(start_digits, digit_count) > 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < simdjson::internal::smallest_power) || (exponent > simdjson::internal::largest_power)) {
    //
    // Important: smallest_power is such that it leads to a zero value.
    // Observe that 18446744073709551615e-343 == 0, i.e. (2**64 - 1) e -343 is zero
    // so something x 10^-343 goes to zero, but not so with  something x 10^-342.
    static_assert(simdjson::internal::smallest_power <= -342, "smallest_power is not small enough");
    //
    if((exponent < simdjson::internal::smallest_power) || (i == 0)) {
      WRITE_DOUBLE(0, src, writer);
      return SUCCESS;
    } else { // (exponent > largest_power) and (i != 0)
      // We have, for sure, an infinite value and simdjson refuses to parse infinite values.
      return INVALID_NUMBER(src);
    }
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_fallback(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * const src) noexcept { return 0; }

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool dirty_end = jsoncharutils::is_not_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (dirty_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// Inlineable functions
namespace {

// This table can be used to characterize the final character of an integer
// string. For JSON structural character and allowable white space characters,
// we return SUCCESS. For 'e', '.' and 'E', we return INCORRECT_TYPE. Otherwise
// we return NUMBER_ERROR.
// Optimization note: we could easily reduce the size of the table by half (to 128)
// at the cost of an extra branch.
// Optimization note: we want the values to use at most 8 bits (not, e.g., 32 bits):
static_assert(error_code(uint8_t(NUMBER_ERROR))== NUMBER_ERROR, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(SUCCESS))== SUCCESS, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(INCORRECT_TYPE))== INCORRECT_TYPE, "bad NUMBER_ERROR cast");

const uint8_t integer_string_finisher[256] = {
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   SUCCESS,      NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, INCORRECT_TYPE,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, SUCCESS,        NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR};

// Parse any number from 0 to 18,446,744,073,709,551,615
simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;
  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > 20))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > 20)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if (integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }

  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > longest_digit_count))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > longest_digit_count)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if(integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INCORRECT_TYPE; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return negative ? (~i+1) : i;
}

simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src ) { return INCORRECT_TYPE; }
  if ( (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. More than 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 > 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src > 19;
    }
  } else {
    overflow = p-src > 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
  }

  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  overflow = overflow || exponent < simdjson::internal::smallest_power || exponent > simdjson::internal::largest_power;

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_fallback(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // unnamed namespace
} // namespace arm64
} // namespace simdjson
/* end file include/simdjson/generic/numberparsing.h */

#endif // SIMDJSON_ARM64_NUMBERPARSING_H
/* end file include/simdjson/arm64/numberparsing.h */
/* begin file include/simdjson/arm64/end.h */
/* end file include/simdjson/arm64/end.h */

#endif // SIMDJSON_IMPLEMENTATION_ARM64

#endif // SIMDJSON_ARM64_H
/* end file include/simdjson/arm64.h */
/* begin file include/simdjson/fallback.h */
#ifndef SIMDJSON_FALLBACK_H
#define SIMDJSON_FALLBACK_H


#if SIMDJSON_IMPLEMENTATION_FALLBACK

namespace simdjson {
/**
 * Fallback implementation (runs on any machine).
 */
namespace fallback {
} // namespace fallback
} // namespace simdjson

/* begin file include/simdjson/fallback/implementation.h */
#ifndef SIMDJSON_FALLBACK_IMPLEMENTATION_H
#define SIMDJSON_FALLBACK_IMPLEMENTATION_H


namespace simdjson {
namespace fallback {

namespace {
using namespace simdjson;
using namespace simdjson::dom;
}

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation(
      "fallback",
      "Generic fallback implementation",
      0
  ) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_IMPLEMENTATION_H
/* end file include/simdjson/fallback/implementation.h */

/* begin file include/simdjson/fallback/begin.h */
// redefining SIMDJSON_IMPLEMENTATION to "fallback"
// #define SIMDJSON_IMPLEMENTATION fallback
/* end file include/simdjson/fallback/begin.h */

// Declarations
/* begin file include/simdjson/generic/dom_parser_implementation.h */

namespace simdjson {
namespace fallback {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  inline dom_parser_implementation() noexcept;
  inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation &operator=(const dom_parser_implementation &) = delete;

  simdjson_warn_unused error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  simdjson_warn_unused error_code stage2(dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage2_next(dom::document &doc) noexcept final;
  inline simdjson_warn_unused error_code set_capacity(size_t capacity) noexcept final;
  inline simdjson_warn_unused error_code set_max_depth(size_t max_depth) noexcept final;
private:
  simdjson_really_inline simdjson_warn_unused error_code set_capacity_stage1(size_t capacity);

};

} // namespace fallback
} // namespace simdjson

namespace simdjson {
namespace fallback {

inline dom_parser_implementation::dom_parser_implementation() noexcept = default;
inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

// Leaving these here so they can be inlined if so desired
inline simdjson_warn_unused error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  if(capacity > SIMDJSON_MAXSIZE_BYTES) { return CAPACITY; }
  // Stage 1 index output
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!structural_indexes) { _capacity = 0; return MEMALLOC; }
  structural_indexes[0] = 0;
  n_structural_indexes = 0;

  _capacity = capacity;
  return SUCCESS;
}

inline simdjson_warn_unused error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  // Stage 2 stacks
  open_containers.reset(new (std::nothrow) open_container[max_depth]);
  is_array.reset(new (std::nothrow) bool[max_depth]);
  if (!is_array || !open_containers) { _max_depth = 0; return MEMALLOC; }

  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace fallback
} // namespace simdjson
/* end file include/simdjson/generic/dom_parser_implementation.h */
/* begin file include/simdjson/fallback/bitmanipulation.h */
#ifndef SIMDJSON_FALLBACK_BITMANIPULATION_H
#define SIMDJSON_FALLBACK_BITMANIPULATION_H

#include <limits>

namespace simdjson {
namespace fallback {
namespace {

#if defined(_MSC_VER) && !defined(_M_ARM64) && !defined(_M_X64)
static inline unsigned char _BitScanForward64(unsigned long* ret, uint64_t x) {
  unsigned long x0 = (unsigned long)x, top, bottom;
  _BitScanForward(&top, (unsigned long)(x >> 32));
  _BitScanForward(&bottom, x0);
  *ret = x0 ? bottom : 32 + top;
  return x != 0;
}
static unsigned char _BitScanReverse64(unsigned long* ret, uint64_t x) {
  unsigned long x1 = (unsigned long)(x >> 32), top, bottom;
  _BitScanReverse(&top, x1);
  _BitScanReverse(&bottom, (unsigned long)x);
  *ret = x1 ? top + 32 : bottom;
  return x != 0;
}
#endif

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB)
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// _MSC_VER
}

} // unnamed namespace
} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_BITMANIPULATION_H
/* end file include/simdjson/fallback/bitmanipulation.h */
/* begin file include/simdjson/generic/jsoncharutils.h */

namespace simdjson {
namespace fallback {
namespace {
namespace jsoncharutils {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = internal::digit_to_val32[630 + src[0]];
  uint32_t v2 = internal::digit_to_val32[420 + src[1]];
  uint32_t v3 = internal::digit_to_val32[210 + src[2]];
  uint32_t v4 = internal::digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

using internal::value128;

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace jsoncharutils
} // unnamed namespace
} // namespace fallback
} // namespace simdjson
/* end file include/simdjson/generic/jsoncharutils.h */
/* begin file include/simdjson/generic/atomparsing.h */
namespace simdjson {
namespace fallback {
namespace {
/// @private
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
simdjson_warn_unused
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | jsoncharutils::is_not_structural_or_whitespace(src[5])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // unnamed namespace
} // namespace fallback
} // namespace simdjson
/* end file include/simdjson/generic/atomparsing.h */
/* begin file include/simdjson/fallback/stringparsing.h */
#ifndef SIMDJSON_FALLBACK_STRINGPARSING_H
#define SIMDJSON_FALLBACK_STRINGPARSING_H


namespace simdjson {
namespace fallback {
namespace {

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 1;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return c == '"'; }
  simdjson_really_inline bool has_backslash() { return c == '\\'; }
  simdjson_really_inline int quote_index() { return c == '"' ? 0 : 1; }
  simdjson_really_inline int backslash_index() { return c == '\\' ? 0 : 1; }

  uint8_t c;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // store to dest unconditionally - we can overwrite the bits we don't like later
  dst[0] = src[0];
  return { src[0] };
}

} // unnamed namespace
} // namespace fallback
} // namespace simdjson

/* begin file include/simdjson/generic/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace simdjson {
namespace fallback {
namespace {
/// @private
namespace stringparsing {

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
simdjson_warn_unused
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // jsoncharutils::hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = jsoncharutils::codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

/**
 * Unescape a string from src to dst, stopping at a final unescaped quote. E.g., if src points at 'joe"', then
 * dst needs to have four free bytes.
 */
simdjson_warn_unused simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

simdjson_unused simdjson_warn_unused simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (*(src++) != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view(reinterpret_cast<const char *>(current_string_buf_loc), end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // unnamed namespace
} // namespace fallback
} // namespace simdjson
/* end file include/simdjson/generic/stringparsing.h */

#endif // SIMDJSON_FALLBACK_STRINGPARSING_H
/* end file include/simdjson/fallback/stringparsing.h */
/* begin file include/simdjson/fallback/numberparsing.h */
#ifndef SIMDJSON_FALLBACK_NUMBERPARSING_H
#define SIMDJSON_FALLBACK_NUMBERPARSING_H

#ifdef JSON_TEST_NUMBERS // for unit testing
void found_invalid_number(const uint8_t *buf);
void found_integer(int64_t result, const uint8_t *buf);
void found_unsigned_integer(uint64_t result, const uint8_t *buf);
void found_float(double result, const uint8_t *buf);
#endif

namespace simdjson {
namespace fallback {
namespace {
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const char *chars) {
  uint64_t val;
  memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}
static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  return parse_eight_digits_unrolled(reinterpret_cast<const char *>(chars));
}

} // unnamed namespace
} // namespace fallback
} // namespace simdjson

#define SWAR_NUMBER_PARSING
/* begin file include/simdjson/generic/numberparsing.h */
#include <limits>

namespace simdjson {
namespace fallback {
namespace {
/// @private
namespace numberparsing {



#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

namespace {
// Convert a mantissa, an exponent and a sign bit into an ieee64 double.
// The real_exponent needs to be in [0, 2046] (technically real_exponent = 2047 would be acceptable).
// The mantissa should be in [0,1<<53). The bit at index (1ULL << 52) while be zeroed.
simdjson_really_inline double to_double(uint64_t mantissa, uint64_t real_exponent, bool negative) {
    double d;
    mantissa &= ~(1ULL << 52);
    mantissa |= real_exponent << 52;
    mantissa |= ((static_cast<uint64_t>(negative)) << 63);
    std::memcpy(&d, &mantissa, sizeof(d));
    return d;
}
}
// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [smallest_power,
// largest_power] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / simdjson::internal::power_of_ten[-power];
    } else {
      d = d * simdjson::internal::power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It was
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }


  // The exponent is 1024 + 63 + power
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power when power >= 0
  // and it is equal to
  //  ceil(log(5**-power)/log(2)) + power when power < 0
  //
  // The 65536 is (1<<16) and corresponds to
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to
  // floor(log(5**power)/log(2))
  //
  // Note that this is not magic: 152170/(1<<16) is
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;


  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;


  // We are going to need to do some 64-bit arithmetic to get a precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= smallest_power
  // and power <= largest_power
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  //
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  const uint32_t index = 2 * uint32_t(power - simdjson::internal::smallest_power);
  // Optimization: It may be that materializing the index as a variable might confuse some compilers and prevent effective complex-addressing loads. (Done for code clarity.)
  //
  // The full_multiplication function computes the 128-bit product of two 64-bit words
  // with a returned value of type value128 with a "low component" corresponding to the
  // 64-bit least significant bits of the product and with a "high component" corresponding
  // to the 64-bit most significant bits of the product.
  simdjson::internal::value128 firstproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index]);
  // Both i and power_of_five_128[index] have their most significant bit set to 1 which
  // implies that the either the most or the second most significant bit of the product
  // is 1. We pack values in this manner for efficiency reasons: it maximizes the use
  // we make of the product. It also makes it easy to reason aboutthe product: there
  // 0 or 1 leading zero in the product.

  // Unless the least significant 9 bits of the high (64-bit) part of the full
  // product are all 1s, then we know that the most significant 55 bits are
  // exact and no further work is needed. Having 55 bits is necessary because
  // we need 53 bits for the mantissa but we have to have one rounding bit and
  // we can waste a bit if the most significant bit of the product is zero.
  if((firstproduct.high & 0x1FF) == 0x1FF) {
    // We want to compute i * 5^q, but only care about the top 55 bits at most.
    // Consider the scenario where q>=0. Then 5^q may not fit in 64-bits. Doing
    // the full computation is wasteful. So we do what is called a "truncated
    // multiplication".
    // We take the most significant 64-bits, and we put them in
    // power_of_five_128[index]. Usually, that's good enough to approximate i * 5^q
    // to the desired approximation using one multiplication. Sometimes it does not suffice.
    // Then we store the next most significant 64 bits in power_of_five_128[index + 1], and
    // then we get a better approximation to i * 5^q. In very rare cases, even that
    // will not suffice, though it is seemingly very hard to find such a scenario.
    //
    // That's for when q>=0. The logic for q<0 is somewhat similar but it is somewhat
    // more complicated.
    //
    // There is an extra layer of complexity in that we need more than 55 bits of
    // accuracy in the round-to-even scenario.
    //
    // The full_multiplication function computes the 128-bit product of two 64-bit words
    // with a returned value of type value128 with a "low component" corresponding to the
    // 64-bit least significant bits of the product and with a "high component" corresponding
    // to the 64-bit most significant bits of the product.
    simdjson::internal::value128 secondproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index + 1]);
    firstproduct.low += secondproduct.high;
    if(secondproduct.high > firstproduct.low) { firstproduct.high++; }
    // At this point, we might need to add at most one to firstproduct, but this
    // can only change the value of firstproduct.high if firstproduct.low is maximal.
    if(simdjson_unlikely(firstproduct.low  == 0xFFFFFFFFFFFFFFFF)) {
      // This is very unlikely, but if so, we need to do much more work!
      return false;
    }
  }
  uint64_t lower = firstproduct.low;
  uint64_t upper = firstproduct.high;
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).
  int64_t real_exponent = exponent - lz;
  if (simdjson_unlikely(real_exponent <= 0)) { // we have a subnormal?
    // Here have that real_exponent <= 0 so -real_exponent >= 0
    if(-real_exponent + 1 >= 64) { // if we have more than 64 bits below the minimum exponent, you have a zero for sure.
      d = 0.0;
      return true;
    }
    // next line is safe because -real_exponent + 1 < 0
    mantissa >>= -real_exponent + 1;
    // Thankfully, we can't have both "round-to-even" and subnormals because
    // "round-to-even" only occurs for powers close to 0.
    mantissa += (mantissa & 1); // round up
    mantissa >>= 1;
    // There is a weird scenario where we don't have a subnormal but just.
    // Suppose we start with 2.2250738585072013e-308, we end up
    // with 0x3fffffffffffff x 2^-1023-53 which is technically subnormal
    // whereas 0x40000000000000 x 2^-1023-53  is normal. Now, we need to round
    // up 0x3fffffffffffff x 2^-1023-53  and once we do, we are no longer
    // subnormal, but we can only know this after rounding.
    // So we only declare a subnormal if we are smaller than the threshold.
    real_exponent = (mantissa < (uint64_t(1) << 52)) ? 0 : 1;
    d = to_double(mantissa, real_exponent, negative);
    return true;
  }
  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  //
  // The round-to-even cases take the form of a number 2m+1 which is in (2^53,2^54]
  // times a power of two. That is, it is right between a number with binary significand
  // m and another number with binary significand m+1; and it must be the case
  // that it cannot be represented by a float itself.
  //
  // We must have that w * 10 ^q == (2m+1) * 2^p for some power of two 2^p.
  // Recall that 10^q = 5^q * 2^q.
  // When q >= 0, we must have that (2m+1) is divible by 5^q, so 5^q <= 2^54. We have that
  //  5^23 <=  2^54 and it is the last power of five to qualify, so q <= 23.
  // When q<0, we have  w  >=  (2m+1) x 5^{-q}.  We must have that w<2^{64} so
  // (2m+1) x 5^{-q} < 2^{64}. We have that 2m+1>2^{53}. Hence, we must have
  // 2^{53} x 5^{-q} < 2^{64}.
  // Hence we have 5^{-q} < 2^{11}$ or q>= -4.
  //
  // We require lower <= 1 and not lower == 0 because we could not prove that
  // that lower == 0 is implied; but we could prove that lower <= 1 is a necessary and sufficient test.
  if (simdjson_unlikely((lower <= 1) && (power >= -4) && (power <= 23) && ((mantissa & 3) == 1))) {
    if((mantissa  << (upperbit + 64 - 53 - 2)) ==  upper) {
      mantissa &= ~1;             // flip it so that we do not round up
    }
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    real_exponent++;
  }
  mantissa &= ~(1ULL << 52);
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely(real_exponent > 2046)) {
    // We have an infinte value!!! We could actually throw an error here if we could.
    return false;
  }
  d = to_double(mantissa, real_exponent, negative);
  return true;
}

// We call a fallback floating-point parser that might be slow. Note
// it will accept JSON numbers, but the JSON spec. is more restrictive so
// before you call parse_float_fallback, you need to have validated the input
// string with the JSON grammar.
// It will return an error (false) if the parsed number is infinite.
// The string parsing itself always succeeds. We know that there is at least
// one digit.
static bool parse_float_fallback(const uint8_t *ptr, double *outDouble) {
  *outDouble = simdjson::internal::from_chars(reinterpret_cast<const char *>(ptr));
  // We do not accept infinite values.

  // Detecting finite values in a portable manner is ridiculously hard, ideally
  // we would want to do:
  // return !std::isfinite(*outDouble);
  // but that mysteriously fails under legacy/old libc++ libraries, see
  // https://github.com/simdjson/simdjson/issues/1286
  //
  // Therefore, fall back to this solution (the extra parens are there
  // to handle that max may be a macro on windows).
  return !(*outDouble > (std::numeric_limits<double>::max)() || *outDouble < std::numeric_limits<double>::lowest());
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  std::memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(simdjson_unused const uint8_t * src, W writer) {
  double d;
  if (parse_float_fallback(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(simdjson_unused const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(simdjson_unused const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow.
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time.
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline size_t significant_digits(const uint8_t * start_digits, size_t digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - size_t(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, size_t digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  //
  // 9999999999999999999 < 2**64 so we can accomodate 19 digits.
  // If we have a decimal separator, then digit_count - 1 is the number of digits, but we
  // may not have a decimal separator!
  if (simdjson_unlikely(digit_count > 19 && significant_digits(start_digits, digit_count) > 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < simdjson::internal::smallest_power) || (exponent > simdjson::internal::largest_power)) {
    //
    // Important: smallest_power is such that it leads to a zero value.
    // Observe that 18446744073709551615e-343 == 0, i.e. (2**64 - 1) e -343 is zero
    // so something x 10^-343 goes to zero, but not so with  something x 10^-342.
    static_assert(simdjson::internal::smallest_power <= -342, "smallest_power is not small enough");
    //
    if((exponent < simdjson::internal::smallest_power) || (i == 0)) {
      WRITE_DOUBLE(0, src, writer);
      return SUCCESS;
    } else { // (exponent > largest_power) and (i != 0)
      // We have, for sure, an infinite value and simdjson refuses to parse infinite values.
      return INVALID_NUMBER(src);
    }
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_fallback(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * const src) noexcept { return 0; }

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool dirty_end = jsoncharutils::is_not_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (dirty_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// Inlineable functions
namespace {

// This table can be used to characterize the final character of an integer
// string. For JSON structural character and allowable white space characters,
// we return SUCCESS. For 'e', '.' and 'E', we return INCORRECT_TYPE. Otherwise
// we return NUMBER_ERROR.
// Optimization note: we could easily reduce the size of the table by half (to 128)
// at the cost of an extra branch.
// Optimization note: we want the values to use at most 8 bits (not, e.g., 32 bits):
static_assert(error_code(uint8_t(NUMBER_ERROR))== NUMBER_ERROR, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(SUCCESS))== SUCCESS, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(INCORRECT_TYPE))== INCORRECT_TYPE, "bad NUMBER_ERROR cast");

const uint8_t integer_string_finisher[256] = {
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   SUCCESS,      NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, INCORRECT_TYPE,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, SUCCESS,        NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR};

// Parse any number from 0 to 18,446,744,073,709,551,615
simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;
  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > 20))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > 20)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if (integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }

  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > longest_digit_count))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > longest_digit_count)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if(integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INCORRECT_TYPE; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return negative ? (~i+1) : i;
}

simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src ) { return INCORRECT_TYPE; }
  if ( (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. More than 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 > 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src > 19;
    }
  } else {
    overflow = p-src > 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
  }

  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  overflow = overflow || exponent < simdjson::internal::smallest_power || exponent > simdjson::internal::largest_power;

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_fallback(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // unnamed namespace
} // namespace fallback
} // namespace simdjson
/* end file include/simdjson/generic/numberparsing.h */

#endif // SIMDJSON_FALLBACK_NUMBERPARSING_H
/* end file include/simdjson/fallback/numberparsing.h */
/* begin file include/simdjson/fallback/end.h */
/* end file include/simdjson/fallback/end.h */

#endif // SIMDJSON_IMPLEMENTATION_FALLBACK
#endif // SIMDJSON_FALLBACK_H
/* end file include/simdjson/fallback.h */
/* begin file include/simdjson/haswell.h */
#ifndef SIMDJSON_HASWELL_H
#define SIMDJSON_HASWELL_H


#if SIMDJSON_IMPLEMENTATION_HASWELL

#if SIMDJSON_CAN_ALWAYS_RUN_HASWELL
#define SIMDJSON_TARGET_HASWELL
#define SIMDJSON_UNTARGET_HASWELL
#else
#define SIMDJSON_TARGET_HASWELL SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt")
#define SIMDJSON_UNTARGET_HASWELL SIMDJSON_UNTARGET_REGION
#endif

namespace simdjson {
/**
 * Implementation for Haswell (Intel AVX2).
 */
namespace haswell {
} // namespace haswell
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_HASWELL
//
/* begin file include/simdjson/haswell/implementation.h */
#ifndef SIMDJSON_HASWELL_IMPLEMENTATION_H
#define SIMDJSON_HASWELL_IMPLEMENTATION_H


// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_HASWELL
namespace simdjson {
namespace haswell {

using namespace simdjson;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation(
      "haswell",
      "Intel/AMD AVX2",
      internal::instruction_set::AVX2 | internal::instruction_set::PCLMULQDQ | internal::instruction_set::BMI1 | internal::instruction_set::BMI2
  ) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_IMPLEMENTATION_H
/* end file include/simdjson/haswell/implementation.h */
/* begin file include/simdjson/haswell/intrinsics.h */
#ifndef SIMDJSON_HASWELL_INTRINSICS_H
#define SIMDJSON_HASWELL_INTRINSICS_H


#ifdef SIMDJSON_VISUAL_STUDIO
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h>  // visual studio or clang
#else
#include <x86intrin.h> // elsewhere
#endif // SIMDJSON_VISUAL_STUDIO

#ifdef SIMDJSON_CLANG_VISUAL_STUDIO
/**
 * You are not supposed, normally, to include these
 * headers directly. Instead you should either include intrin.h
 * or x86intrin.h. However, when compiling with clang
 * under Windows (i.e., when _MSC_VER is set), these headers
 * only get included *if* the corresponding features are detected
 * from macros:
 * e.g., if __AVX2__ is set... in turn,  we normally set these
 * macros by compiling against the corresponding architecture
 * (e.g., arch:AVX2, -mavx2, etc.) which compiles the whole
 * software with these advanced instructions. In simdjson, we
 * want to compile the whole program for a generic target,
 * and only target our specific kernels. As a workaround,
 * we directly include the needed headers. These headers would
 * normally guard against such usage, but we carefully included
 * <x86intrin.h>  (or <intrin.h>) before, so the headers
 * are fooled.
 */
#include <bmiintrin.h>   // for _blsr_u64
#include <lzcntintrin.h> // for  __lzcnt64
#include <immintrin.h>   // for most things (AVX2, AVX512, _popcnt64)
#include <smmintrin.h>
#include <tmmintrin.h>
#include <avxintrin.h>
#include <avx2intrin.h>
#include <wmmintrin.h>   // for  _mm_clmulepi64_si128
// unfortunately, we may not get _blsr_u64, but, thankfully, clang
// has it as a macro.
#ifndef _blsr_u64
// we roll our own
SIMDJSON_TARGET_HASWELL
static simdjson_really_inline uint64_t _blsr_u64(uint64_t n) {
  return (n - 1) & n;
}
SIMDJSON_UNTARGET_HASWELL
#endif //  _blsr_u64
#endif // SIMDJSON_CLANG_VISUAL_STUDIO

#endif // SIMDJSON_HASWELL_INTRINSICS_H
/* end file include/simdjson/haswell/intrinsics.h */

//
// The rest need to be inside the region
//
/* begin file include/simdjson/haswell/begin.h */
// redefining SIMDJSON_IMPLEMENTATION to "haswell"
// #define SIMDJSON_IMPLEMENTATION haswell
SIMDJSON_TARGET_HASWELL
/* end file include/simdjson/haswell/begin.h */

// Declarations
/* begin file include/simdjson/generic/dom_parser_implementation.h */

namespace simdjson {
namespace haswell {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  inline dom_parser_implementation() noexcept;
  inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation &operator=(const dom_parser_implementation &) = delete;

  simdjson_warn_unused error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  simdjson_warn_unused error_code stage2(dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage2_next(dom::document &doc) noexcept final;
  inline simdjson_warn_unused error_code set_capacity(size_t capacity) noexcept final;
  inline simdjson_warn_unused error_code set_max_depth(size_t max_depth) noexcept final;
private:
  simdjson_really_inline simdjson_warn_unused error_code set_capacity_stage1(size_t capacity);

};

} // namespace haswell
} // namespace simdjson

namespace simdjson {
namespace haswell {

inline dom_parser_implementation::dom_parser_implementation() noexcept = default;
inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

// Leaving these here so they can be inlined if so desired
inline simdjson_warn_unused error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  if(capacity > SIMDJSON_MAXSIZE_BYTES) { return CAPACITY; }
  // Stage 1 index output
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!structural_indexes) { _capacity = 0; return MEMALLOC; }
  structural_indexes[0] = 0;
  n_structural_indexes = 0;

  _capacity = capacity;
  return SUCCESS;
}

inline simdjson_warn_unused error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  // Stage 2 stacks
  open_containers.reset(new (std::nothrow) open_container[max_depth]);
  is_array.reset(new (std::nothrow) bool[max_depth]);
  if (!is_array || !open_containers) { _max_depth = 0; return MEMALLOC; }

  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace haswell
} // namespace simdjson
/* end file include/simdjson/generic/dom_parser_implementation.h */
/* begin file include/simdjson/haswell/bitmanipulation.h */
#ifndef SIMDJSON_HASWELL_BITMANIPULATION_H
#define SIMDJSON_HASWELL_BITMANIPULATION_H

namespace simdjson {
namespace haswell {
namespace {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
simdjson_really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  return (int)_tzcnt_u64(input_num);
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  ////////
  // You might expect the next line to be equivalent to
  // return (int)_tzcnt_u64(input_num);
  // but the generated code differs and might be less efficient?
  ////////
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return _blsr_u64(input_num);
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
  return int(_lzcnt_u64(input_num));
}

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
simdjson_really_inline unsigned __int64 count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
}
#else
simdjson_really_inline long long int count_ones(uint64_t input_num) {
  return _popcnt64(input_num);
}
#endif

simdjson_really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   reinterpret_cast<unsigned long long *>(result));
#endif
}

} // unnamed namespace
} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_BITMANIPULATION_H
/* end file include/simdjson/haswell/bitmanipulation.h */
/* begin file include/simdjson/haswell/bitmask.h */
#ifndef SIMDJSON_HASWELL_BITMASK_H
#define SIMDJSON_HASWELL_BITMASK_H

namespace simdjson {
namespace haswell {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processor supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // unnamed namespace
} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_BITMASK_H
/* end file include/simdjson/haswell/bitmask.h */
/* begin file include/simdjson/haswell/simd.h */
#ifndef SIMDJSON_HASWELL_SIMD_H
#define SIMDJSON_HASWELL_SIMD_H


namespace simdjson {
namespace haswell {
namespace {
namespace simd {

  // Forward-declared so they can be used by splat and friends.
  template<typename Child>
  struct base {
    __m256i value;

    // Zero constructor
    simdjson_really_inline base() : value{__m256i()} {}

    // Conversion from SIMD register
    simdjson_really_inline base(const __m256i _value) : value(_value) {}

    // Conversion to SIMD register
    simdjson_really_inline operator const __m256i&() const { return this->value; }
    simdjson_really_inline operator __m256i&() { return this->value; }

    // Bit operations
    simdjson_really_inline Child operator|(const Child other) const { return _mm256_or_si256(*this, other); }
    simdjson_really_inline Child operator&(const Child other) const { return _mm256_and_si256(*this, other); }
    simdjson_really_inline Child operator^(const Child other) const { return _mm256_xor_si256(*this, other); }
    simdjson_really_inline Child bit_andnot(const Child other) const { return _mm256_andnot_si256(other, *this); }
    simdjson_really_inline Child& operator|=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast | other; return *this_cast; }
    simdjson_really_inline Child& operator&=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast & other; return *this_cast; }
    simdjson_really_inline Child& operator^=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint32_t bitmask_t;
    typedef uint64_t bitmask2_t;

    simdjson_really_inline base8() : base<simd8<T>>() {}
    simdjson_really_inline base8(const __m256i _value) : base<simd8<T>>(_value) {}

    simdjson_really_inline Mask operator==(const simd8<T> other) const { return _mm256_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<T>::value);

    template<int N=1>
    simdjson_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm256_alignr_epi8(*this, _mm256_permute2x128_si256(prev_chunk, *this, 0x21), 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static simdjson_really_inline simd8<bool> splat(bool _value) { return _mm256_set1_epi8(uint8_t(-(!!_value))); }

    simdjson_really_inline simd8<bool>() : base8() {}
    simdjson_really_inline simd8<bool>(const __m256i _value) : base8<bool>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    simdjson_really_inline int to_bitmask() const { return _mm256_movemask_epi8(*this); }
    simdjson_really_inline bool any() const { return !_mm256_testz_si256(*this, *this); }
    simdjson_really_inline simd8<bool> operator~() const { return *this ^ true; }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static simdjson_really_inline simd8<T> splat(T _value) { return _mm256_set1_epi8(_value); }
    static simdjson_really_inline simd8<T> zero() { return _mm256_setzero_si256(); }
    static simdjson_really_inline simd8<T> load(const T values[32]) {
      return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static simdjson_really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    simdjson_really_inline base8_numeric() : base8<T>() {}
    simdjson_really_inline base8_numeric(const __m256i _value) : base8<T>(_value) {}

    // Store to array
    simdjson_really_inline void store(T dst[32]) const { return _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), *this); }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_really_inline simd8<T> operator+(const simd8<T> other) const { return _mm256_add_epi8(*this, other); }
    simdjson_really_inline simd8<T> operator-(const simd8<T> other) const { return _mm256_sub_epi8(*this, other); }
    simdjson_really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *static_cast<simd8<T>*>(this); }
    simdjson_really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *static_cast<simd8<T>*>(this); }

    // Override to distinguish from bool version
    simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm256_shuffle_epi8(lookup_table, *this);
    }

    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 32 - count_ones(mask) bytes of the result are significant but 32 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint32_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    simdjson_really_inline void compress(uint32_t mask, L * output) const {
      using internal::thintable_epi8;
      using internal::BitsSetTable256mul2;
      using internal::pshufb_combine_table;
      // this particular implementation was inspired by work done by @animetosho
      // we do it in four steps, first 8 bytes and then second 8 bytes...
      uint8_t mask1 = uint8_t(mask); // least significant 8 bits
      uint8_t mask2 = uint8_t(mask >> 8); // second least significant 8 bits
      uint8_t mask3 = uint8_t(mask >> 16); // ...
      uint8_t mask4 = uint8_t(mask >> 24); // ...
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      __m256i shufmask =  _mm256_set_epi64x(thintable_epi8[mask4], thintable_epi8[mask3],
        thintable_epi8[mask2], thintable_epi8[mask1]);
      // we increment by 0x08 the second half of the mask and so forth
      shufmask =
      _mm256_add_epi8(shufmask, _mm256_set_epi32(0x18181818, 0x18181818,
         0x10101010, 0x10101010, 0x08080808, 0x08080808, 0, 0));
      // this is the version "nearly pruned"
      __m256i pruned = _mm256_shuffle_epi8(*this, shufmask);
      // we still need to put the  pieces back together.
      // we compute the popcount of the first words:
      int pop1 = BitsSetTable256mul2[mask1];
      int pop3 = BitsSetTable256mul2[mask3];

      // then load the corresponding mask
      // could be done with _mm256_loadu2_m128i but many standard libraries omit this intrinsic.
      __m256i v256 = _mm256_castsi128_si256(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(pshufb_combine_table + pop1 * 8)));
      __m256i compactmask = _mm256_insertf128_si256(v256,
         _mm_loadu_si128(reinterpret_cast<const __m128i *>(pshufb_combine_table + pop3 * 8)), 1);
      __m256i almostthere =  _mm256_shuffle_epi8(pruned, compactmask);
      // We just need to write out the result.
      // This is the tricky bit that is hard to do
      // if we want to return a SIMD register, since there
      // is no single-instruction approach to recombine
      // the two 128-bit lanes with an offset.
      __m128i v128;
      v128 = _mm256_castsi256_si128(almostthere);
      _mm_storeu_si128( reinterpret_cast<__m128i *>(output), v128);
      v128 = _mm256_extractf128_si256(almostthere, 1);
      _mm_storeu_si128( reinterpret_cast<__m128i *>(output + 16 - count_ones(mask & 0xFFFF)), v128);
    }

    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> : base8_numeric<int8_t> {
    simdjson_really_inline simd8() : base8_numeric<int8_t>() {}
    simdjson_really_inline simd8(const __m256i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const int8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15,
      int8_t v16, int8_t v17, int8_t v18, int8_t v19, int8_t v20, int8_t v21, int8_t v22, int8_t v23,
      int8_t v24, int8_t v25, int8_t v26, int8_t v27, int8_t v28, int8_t v29, int8_t v30, int8_t v31
    ) : simd8(_mm256_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15,
      v16,v17,v18,v19,v20,v21,v22,v23,
      v24,v25,v26,v27,v28,v29,v30,v31
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    simdjson_really_inline simd8<int8_t> max_val(const simd8<int8_t> other) const { return _mm256_max_epi8(*this, other); }
    simdjson_really_inline simd8<int8_t> min_val(const simd8<int8_t> other) const { return _mm256_min_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm256_cmpgt_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm256_cmpgt_epi8(other, *this); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    simdjson_really_inline simd8() : base8_numeric<uint8_t>() {}
    simdjson_really_inline simd8(const __m256i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const uint8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15,
      uint8_t v16, uint8_t v17, uint8_t v18, uint8_t v19, uint8_t v20, uint8_t v21, uint8_t v22, uint8_t v23,
      uint8_t v24, uint8_t v25, uint8_t v26, uint8_t v27, uint8_t v28, uint8_t v29, uint8_t v30, uint8_t v31
    ) : simd8(_mm256_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15,
      v16,v17,v18,v19,v20,v21,v22,v23,
      v24,v25,v26,v27,v28,v29,v30,v31
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    simdjson_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm256_adds_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm256_subs_epu8(*this, other); }

    // Order-specific operations
    simdjson_really_inline simd8<uint8_t> max_val(const simd8<uint8_t> other) const { return _mm256_max_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> min_val(const simd8<uint8_t> other) const { return _mm256_min_epu8(other, *this); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    simdjson_really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max_val(*this) == other; }
    simdjson_really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min_val(*this) == other; }
    simdjson_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    simdjson_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->lt_bits(other).any_bits_set(); }

    // Bit-specific operations
    simdjson_really_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    simdjson_really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    simdjson_really_inline bool is_ascii() const { return _mm256_movemask_epi8(*this) == 0; }
    simdjson_really_inline bool bits_not_set_anywhere() const { return _mm256_testz_si256(*this, *this); }
    simdjson_really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    simdjson_really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm256_testz_si256(*this, bits); }
    simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm256_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm256_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    simdjson_really_inline int get_bit() const { return _mm256_movemask_epi8(_mm256_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 2, "Haswell kernel should use two registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T>& other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1) : chunks{chunk0, chunk1} {}
    simdjson_really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+32)} {}

    simdjson_really_inline void compress(uint64_t mask, T * output) const {
      uint32_t mask1 = uint32_t(mask);
      uint32_t mask2 = uint32_t(mask >> 32);
      this->chunks[0].compress(mask1, output);
      this->chunks[1].compress(mask2, output + 32 - count_ones(mask1));
    }

    simdjson_really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
    }

    simdjson_really_inline uint64_t to_bitmask() const {
      uint64_t r_lo = uint32_t(this->chunks[0].to_bitmask());
      uint64_t r_hi =                       this->chunks[1].to_bitmask();
      return r_lo | (r_hi << 32);
    }

    simdjson_really_inline simd8<T> reduce_or() const {
      return this->chunks[0] | this->chunks[1];
    }

    simdjson_really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return simd8x64<T>(
        this->chunks[0] | mask,
        this->chunks[1] | mask
      );
    }

    simdjson_really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] == mask,
        this->chunks[1] == mask
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t eq(const simd8x64<uint8_t> &other) const {
      return  simd8x64<bool>(
        this->chunks[0] == other.chunks[0],
        this->chunks[1] == other.chunks[1]
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] <= mask,
        this->chunks[1] <= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simd

} // unnamed namespace
} // namespace haswell
} // namespace simdjson

#endif // SIMDJSON_HASWELL_SIMD_H
/* end file include/simdjson/haswell/simd.h */
/* begin file include/simdjson/generic/jsoncharutils.h */

namespace simdjson {
namespace haswell {
namespace {
namespace jsoncharutils {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = internal::digit_to_val32[630 + src[0]];
  uint32_t v2 = internal::digit_to_val32[420 + src[1]];
  uint32_t v3 = internal::digit_to_val32[210 + src[2]];
  uint32_t v4 = internal::digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

using internal::value128;

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace jsoncharutils
} // unnamed namespace
} // namespace haswell
} // namespace simdjson
/* end file include/simdjson/generic/jsoncharutils.h */
/* begin file include/simdjson/generic/atomparsing.h */
namespace simdjson {
namespace haswell {
namespace {
/// @private
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
simdjson_warn_unused
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | jsoncharutils::is_not_structural_or_whitespace(src[5])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // unnamed namespace
} // namespace haswell
} // namespace simdjson
/* end file include/simdjson/generic/atomparsing.h */
/* begin file include/simdjson/haswell/stringparsing.h */
#ifndef SIMDJSON_HASWELL_STRINGPARSING_H
#define SIMDJSON_HASWELL_STRINGPARSING_H


namespace simdjson {
namespace haswell {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_really_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }
  simdjson_really_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_really_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 15 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v(src);
  // store to dest unconditionally - we can overwrite the bits we don't like later
  v.store(dst);
  return {
      static_cast<uint32_t>((v == '\\').to_bitmask()),     // bs_bits
      static_cast<uint32_t>((v == '"').to_bitmask()), // quote_bits
  };
}

} // unnamed namespace
} // namespace haswell
} // namespace simdjson

/* begin file include/simdjson/generic/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace simdjson {
namespace haswell {
namespace {
/// @private
namespace stringparsing {

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
simdjson_warn_unused
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // jsoncharutils::hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = jsoncharutils::codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

/**
 * Unescape a string from src to dst, stopping at a final unescaped quote. E.g., if src points at 'joe"', then
 * dst needs to have four free bytes.
 */
simdjson_warn_unused simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

simdjson_unused simdjson_warn_unused simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (*(src++) != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view(reinterpret_cast<const char *>(current_string_buf_loc), end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // unnamed namespace
} // namespace haswell
} // namespace simdjson
/* end file include/simdjson/generic/stringparsing.h */

#endif // SIMDJSON_HASWELL_STRINGPARSING_H
/* end file include/simdjson/haswell/stringparsing.h */
/* begin file include/simdjson/haswell/numberparsing.h */
#ifndef SIMDJSON_HASWELL_NUMBERPARSING_H
#define SIMDJSON_HASWELL_NUMBERPARSING_H

namespace simdjson {
namespace haswell {
namespace {

static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  // this actually computes *16* values so we are being wasteful.
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  const __m128i mul_1_10000 =
      _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);
  const __m128i input = _mm_sub_epi8(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars)), ascii0);
  const __m128i t1 = _mm_maddubs_epi16(input, mul_1_10);
  const __m128i t2 = _mm_madd_epi16(t1, mul_1_100);
  const __m128i t3 = _mm_packus_epi32(t2, t2);
  const __m128i t4 = _mm_madd_epi16(t3, mul_1_10000);
  return _mm_cvtsi128_si32(
      t4); // only captures the sum of the first 8 digits, drop the rest
}

} // unnamed namespace
} // namespace haswell
} // namespace simdjson

#define SWAR_NUMBER_PARSING

/* begin file include/simdjson/generic/numberparsing.h */
#include <limits>

namespace simdjson {
namespace haswell {
namespace {
/// @private
namespace numberparsing {



#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

namespace {
// Convert a mantissa, an exponent and a sign bit into an ieee64 double.
// The real_exponent needs to be in [0, 2046] (technically real_exponent = 2047 would be acceptable).
// The mantissa should be in [0,1<<53). The bit at index (1ULL << 52) while be zeroed.
simdjson_really_inline double to_double(uint64_t mantissa, uint64_t real_exponent, bool negative) {
    double d;
    mantissa &= ~(1ULL << 52);
    mantissa |= real_exponent << 52;
    mantissa |= ((static_cast<uint64_t>(negative)) << 63);
    std::memcpy(&d, &mantissa, sizeof(d));
    return d;
}
}
// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [smallest_power,
// largest_power] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / simdjson::internal::power_of_ten[-power];
    } else {
      d = d * simdjson::internal::power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It was
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }


  // The exponent is 1024 + 63 + power
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power when power >= 0
  // and it is equal to
  //  ceil(log(5**-power)/log(2)) + power when power < 0
  //
  // The 65536 is (1<<16) and corresponds to
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to
  // floor(log(5**power)/log(2))
  //
  // Note that this is not magic: 152170/(1<<16) is
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;


  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;


  // We are going to need to do some 64-bit arithmetic to get a precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= smallest_power
  // and power <= largest_power
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  //
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  const uint32_t index = 2 * uint32_t(power - simdjson::internal::smallest_power);
  // Optimization: It may be that materializing the index as a variable might confuse some compilers and prevent effective complex-addressing loads. (Done for code clarity.)
  //
  // The full_multiplication function computes the 128-bit product of two 64-bit words
  // with a returned value of type value128 with a "low component" corresponding to the
  // 64-bit least significant bits of the product and with a "high component" corresponding
  // to the 64-bit most significant bits of the product.
  simdjson::internal::value128 firstproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index]);
  // Both i and power_of_five_128[index] have their most significant bit set to 1 which
  // implies that the either the most or the second most significant bit of the product
  // is 1. We pack values in this manner for efficiency reasons: it maximizes the use
  // we make of the product. It also makes it easy to reason aboutthe product: there
  // 0 or 1 leading zero in the product.

  // Unless the least significant 9 bits of the high (64-bit) part of the full
  // product are all 1s, then we know that the most significant 55 bits are
  // exact and no further work is needed. Having 55 bits is necessary because
  // we need 53 bits for the mantissa but we have to have one rounding bit and
  // we can waste a bit if the most significant bit of the product is zero.
  if((firstproduct.high & 0x1FF) == 0x1FF) {
    // We want to compute i * 5^q, but only care about the top 55 bits at most.
    // Consider the scenario where q>=0. Then 5^q may not fit in 64-bits. Doing
    // the full computation is wasteful. So we do what is called a "truncated
    // multiplication".
    // We take the most significant 64-bits, and we put them in
    // power_of_five_128[index]. Usually, that's good enough to approximate i * 5^q
    // to the desired approximation using one multiplication. Sometimes it does not suffice.
    // Then we store the next most significant 64 bits in power_of_five_128[index + 1], and
    // then we get a better approximation to i * 5^q. In very rare cases, even that
    // will not suffice, though it is seemingly very hard to find such a scenario.
    //
    // That's for when q>=0. The logic for q<0 is somewhat similar but it is somewhat
    // more complicated.
    //
    // There is an extra layer of complexity in that we need more than 55 bits of
    // accuracy in the round-to-even scenario.
    //
    // The full_multiplication function computes the 128-bit product of two 64-bit words
    // with a returned value of type value128 with a "low component" corresponding to the
    // 64-bit least significant bits of the product and with a "high component" corresponding
    // to the 64-bit most significant bits of the product.
    simdjson::internal::value128 secondproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index + 1]);
    firstproduct.low += secondproduct.high;
    if(secondproduct.high > firstproduct.low) { firstproduct.high++; }
    // At this point, we might need to add at most one to firstproduct, but this
    // can only change the value of firstproduct.high if firstproduct.low is maximal.
    if(simdjson_unlikely(firstproduct.low  == 0xFFFFFFFFFFFFFFFF)) {
      // This is very unlikely, but if so, we need to do much more work!
      return false;
    }
  }
  uint64_t lower = firstproduct.low;
  uint64_t upper = firstproduct.high;
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).
  int64_t real_exponent = exponent - lz;
  if (simdjson_unlikely(real_exponent <= 0)) { // we have a subnormal?
    // Here have that real_exponent <= 0 so -real_exponent >= 0
    if(-real_exponent + 1 >= 64) { // if we have more than 64 bits below the minimum exponent, you have a zero for sure.
      d = 0.0;
      return true;
    }
    // next line is safe because -real_exponent + 1 < 0
    mantissa >>= -real_exponent + 1;
    // Thankfully, we can't have both "round-to-even" and subnormals because
    // "round-to-even" only occurs for powers close to 0.
    mantissa += (mantissa & 1); // round up
    mantissa >>= 1;
    // There is a weird scenario where we don't have a subnormal but just.
    // Suppose we start with 2.2250738585072013e-308, we end up
    // with 0x3fffffffffffff x 2^-1023-53 which is technically subnormal
    // whereas 0x40000000000000 x 2^-1023-53  is normal. Now, we need to round
    // up 0x3fffffffffffff x 2^-1023-53  and once we do, we are no longer
    // subnormal, but we can only know this after rounding.
    // So we only declare a subnormal if we are smaller than the threshold.
    real_exponent = (mantissa < (uint64_t(1) << 52)) ? 0 : 1;
    d = to_double(mantissa, real_exponent, negative);
    return true;
  }
  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  //
  // The round-to-even cases take the form of a number 2m+1 which is in (2^53,2^54]
  // times a power of two. That is, it is right between a number with binary significand
  // m and another number with binary significand m+1; and it must be the case
  // that it cannot be represented by a float itself.
  //
  // We must have that w * 10 ^q == (2m+1) * 2^p for some power of two 2^p.
  // Recall that 10^q = 5^q * 2^q.
  // When q >= 0, we must have that (2m+1) is divible by 5^q, so 5^q <= 2^54. We have that
  //  5^23 <=  2^54 and it is the last power of five to qualify, so q <= 23.
  // When q<0, we have  w  >=  (2m+1) x 5^{-q}.  We must have that w<2^{64} so
  // (2m+1) x 5^{-q} < 2^{64}. We have that 2m+1>2^{53}. Hence, we must have
  // 2^{53} x 5^{-q} < 2^{64}.
  // Hence we have 5^{-q} < 2^{11}$ or q>= -4.
  //
  // We require lower <= 1 and not lower == 0 because we could not prove that
  // that lower == 0 is implied; but we could prove that lower <= 1 is a necessary and sufficient test.
  if (simdjson_unlikely((lower <= 1) && (power >= -4) && (power <= 23) && ((mantissa & 3) == 1))) {
    if((mantissa  << (upperbit + 64 - 53 - 2)) ==  upper) {
      mantissa &= ~1;             // flip it so that we do not round up
    }
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    real_exponent++;
  }
  mantissa &= ~(1ULL << 52);
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely(real_exponent > 2046)) {
    // We have an infinte value!!! We could actually throw an error here if we could.
    return false;
  }
  d = to_double(mantissa, real_exponent, negative);
  return true;
}

// We call a fallback floating-point parser that might be slow. Note
// it will accept JSON numbers, but the JSON spec. is more restrictive so
// before you call parse_float_fallback, you need to have validated the input
// string with the JSON grammar.
// It will return an error (false) if the parsed number is infinite.
// The string parsing itself always succeeds. We know that there is at least
// one digit.
static bool parse_float_fallback(const uint8_t *ptr, double *outDouble) {
  *outDouble = simdjson::internal::from_chars(reinterpret_cast<const char *>(ptr));
  // We do not accept infinite values.

  // Detecting finite values in a portable manner is ridiculously hard, ideally
  // we would want to do:
  // return !std::isfinite(*outDouble);
  // but that mysteriously fails under legacy/old libc++ libraries, see
  // https://github.com/simdjson/simdjson/issues/1286
  //
  // Therefore, fall back to this solution (the extra parens are there
  // to handle that max may be a macro on windows).
  return !(*outDouble > (std::numeric_limits<double>::max)() || *outDouble < std::numeric_limits<double>::lowest());
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  std::memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(simdjson_unused const uint8_t * src, W writer) {
  double d;
  if (parse_float_fallback(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(simdjson_unused const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(simdjson_unused const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow.
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time.
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline size_t significant_digits(const uint8_t * start_digits, size_t digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - size_t(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, size_t digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  //
  // 9999999999999999999 < 2**64 so we can accomodate 19 digits.
  // If we have a decimal separator, then digit_count - 1 is the number of digits, but we
  // may not have a decimal separator!
  if (simdjson_unlikely(digit_count > 19 && significant_digits(start_digits, digit_count) > 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < simdjson::internal::smallest_power) || (exponent > simdjson::internal::largest_power)) {
    //
    // Important: smallest_power is such that it leads to a zero value.
    // Observe that 18446744073709551615e-343 == 0, i.e. (2**64 - 1) e -343 is zero
    // so something x 10^-343 goes to zero, but not so with  something x 10^-342.
    static_assert(simdjson::internal::smallest_power <= -342, "smallest_power is not small enough");
    //
    if((exponent < simdjson::internal::smallest_power) || (i == 0)) {
      WRITE_DOUBLE(0, src, writer);
      return SUCCESS;
    } else { // (exponent > largest_power) and (i != 0)
      // We have, for sure, an infinite value and simdjson refuses to parse infinite values.
      return INVALID_NUMBER(src);
    }
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_fallback(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * const src) noexcept { return 0; }

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool dirty_end = jsoncharutils::is_not_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (dirty_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// Inlineable functions
namespace {

// This table can be used to characterize the final character of an integer
// string. For JSON structural character and allowable white space characters,
// we return SUCCESS. For 'e', '.' and 'E', we return INCORRECT_TYPE. Otherwise
// we return NUMBER_ERROR.
// Optimization note: we could easily reduce the size of the table by half (to 128)
// at the cost of an extra branch.
// Optimization note: we want the values to use at most 8 bits (not, e.g., 32 bits):
static_assert(error_code(uint8_t(NUMBER_ERROR))== NUMBER_ERROR, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(SUCCESS))== SUCCESS, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(INCORRECT_TYPE))== INCORRECT_TYPE, "bad NUMBER_ERROR cast");

const uint8_t integer_string_finisher[256] = {
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   SUCCESS,      NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, INCORRECT_TYPE,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, SUCCESS,        NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR};

// Parse any number from 0 to 18,446,744,073,709,551,615
simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;
  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > 20))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > 20)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if (integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }

  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > longest_digit_count))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > longest_digit_count)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if(integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INCORRECT_TYPE; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return negative ? (~i+1) : i;
}

simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src ) { return INCORRECT_TYPE; }
  if ( (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. More than 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 > 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src > 19;
    }
  } else {
    overflow = p-src > 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
  }

  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  overflow = overflow || exponent < simdjson::internal::smallest_power || exponent > simdjson::internal::largest_power;

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_fallback(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // unnamed namespace
} // namespace haswell
} // namespace simdjson
/* end file include/simdjson/generic/numberparsing.h */

#endif // SIMDJSON_HASWELL_NUMBERPARSING_H
/* end file include/simdjson/haswell/numberparsing.h */
/* begin file include/simdjson/haswell/end.h */
SIMDJSON_UNTARGET_HASWELL
/* end file include/simdjson/haswell/end.h */

#endif // SIMDJSON_IMPLEMENTATION_HASWELL
#endif // SIMDJSON_HASWELL_COMMON_H
/* end file include/simdjson/haswell.h */
/* begin file include/simdjson/ppc64.h */
#ifndef SIMDJSON_PPC64_H
#define SIMDJSON_PPC64_H


#if SIMDJSON_IMPLEMENTATION_PPC64

namespace simdjson {
/**
 * Implementation for ALTIVEC (PPC64).
 */
namespace ppc64 {
} // namespace ppc64
} // namespace simdjson

/* begin file include/simdjson/ppc64/implementation.h */
#ifndef SIMDJSON_PPC64_IMPLEMENTATION_H
#define SIMDJSON_PPC64_IMPLEMENTATION_H


namespace simdjson {
namespace ppc64 {

namespace {
using namespace simdjson;
using namespace simdjson::dom;
} // namespace

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation()
      : simdjson::implementation("ppc64", "PPC64 ALTIVEC",
                                 internal::instruction_set::ALTIVEC) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
      size_t capacity, size_t max_length,
      std::unique_ptr<internal::dom_parser_implementation> &dst)
      const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len,
                                         uint8_t *dst,
                                         size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf,
                                          size_t len) const noexcept final;
};

} // namespace ppc64
} // namespace simdjson

#endif // SIMDJSON_PPC64_IMPLEMENTATION_H
/* end file include/simdjson/ppc64/implementation.h */

/* begin file include/simdjson/ppc64/begin.h */
// redefining SIMDJSON_IMPLEMENTATION to "ppc64"
// #define SIMDJSON_IMPLEMENTATION ppc64
/* end file include/simdjson/ppc64/begin.h */

// Declarations
/* begin file include/simdjson/generic/dom_parser_implementation.h */

namespace simdjson {
namespace ppc64 {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  inline dom_parser_implementation() noexcept;
  inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation &operator=(const dom_parser_implementation &) = delete;

  simdjson_warn_unused error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  simdjson_warn_unused error_code stage2(dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage2_next(dom::document &doc) noexcept final;
  inline simdjson_warn_unused error_code set_capacity(size_t capacity) noexcept final;
  inline simdjson_warn_unused error_code set_max_depth(size_t max_depth) noexcept final;
private:
  simdjson_really_inline simdjson_warn_unused error_code set_capacity_stage1(size_t capacity);

};

} // namespace ppc64
} // namespace simdjson

namespace simdjson {
namespace ppc64 {

inline dom_parser_implementation::dom_parser_implementation() noexcept = default;
inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

// Leaving these here so they can be inlined if so desired
inline simdjson_warn_unused error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  if(capacity > SIMDJSON_MAXSIZE_BYTES) { return CAPACITY; }
  // Stage 1 index output
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!structural_indexes) { _capacity = 0; return MEMALLOC; }
  structural_indexes[0] = 0;
  n_structural_indexes = 0;

  _capacity = capacity;
  return SUCCESS;
}

inline simdjson_warn_unused error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  // Stage 2 stacks
  open_containers.reset(new (std::nothrow) open_container[max_depth]);
  is_array.reset(new (std::nothrow) bool[max_depth]);
  if (!is_array || !open_containers) { _max_depth = 0; return MEMALLOC; }

  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace ppc64
} // namespace simdjson
/* end file include/simdjson/generic/dom_parser_implementation.h */
/* begin file include/simdjson/ppc64/intrinsics.h */
#ifndef SIMDJSON_PPC64_INTRINSICS_H
#define SIMDJSON_PPC64_INTRINSICS_H


// This should be the correct header whether
// you use visual studio or other compilers.
#include <altivec.h>

// These are defined by altivec.h in GCC toolchain, it is safe to undef them.
#ifdef bool
#undef bool
#endif

#ifdef vector
#undef vector
#endif

#endif //  SIMDJSON_PPC64_INTRINSICS_H
/* end file include/simdjson/ppc64/intrinsics.h */
/* begin file include/simdjson/ppc64/bitmanipulation.h */
#ifndef SIMDJSON_PPC64_BITMANIPULATION_H
#define SIMDJSON_PPC64_BITMANIPULATION_H

namespace simdjson {
namespace ppc64 {
namespace {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
simdjson_really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long ret;
  // Search the mask data from least significant bit (LSB)
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else  // SIMDJSON_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num - 1);
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB)
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
simdjson_really_inline int count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num); // Visual Studio wants two underscores
}
#else
simdjson_really_inline int count_ones(uint64_t input_num) {
  return __builtin_popcountll(input_num);
}
#endif

simdjson_really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                         uint64_t *result) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  *result = value1 + value2;
  return *result < value1;
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   reinterpret_cast<unsigned long long *>(result));
#endif
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#endif // SIMDJSON_PPC64_BITMANIPULATION_H
/* end file include/simdjson/ppc64/bitmanipulation.h */
/* begin file include/simdjson/ppc64/bitmask.h */
#ifndef SIMDJSON_PPC64_BITMASK_H
#define SIMDJSON_PPC64_BITMASK_H

namespace simdjson {
namespace ppc64 {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is
// encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(uint64_t bitmask) {
  // You can use the version below, however gcc sometimes miscompiles
  // vec_pmsum_be, it happens somewhere around between 8 and 9th version.
  // The performance boost was not noticeable, falling back to a usual
  // implementation.
  //   __vector unsigned long long all_ones = {~0ull, ~0ull};
  //   __vector unsigned long long mask = {bitmask, 0};
  //   // Clang and GCC return different values for pmsum for ull so cast it to one.
  //   // Generally it is not specified by ALTIVEC ISA what is returned by
  //   // vec_pmsum_be.
  // #if defined(__LITTLE_ENDIAN__)
  //   return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[0]);
  // #else
  //   return (uint64_t)(((__vector unsigned long long)vec_pmsum_be(all_ones, mask))[1]);
  // #endif
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#endif
/* end file include/simdjson/ppc64/bitmask.h */
/* begin file include/simdjson/ppc64/simd.h */
#ifndef SIMDJSON_PPC64_SIMD_H
#define SIMDJSON_PPC64_SIMD_H

#include <type_traits>

namespace simdjson {
namespace ppc64 {
namespace {
namespace simd {

using __m128i = __vector unsigned char;

template <typename Child> struct base {
  __m128i value;

  // Zero constructor
  simdjson_really_inline base() : value{__m128i()} {}

  // Conversion from SIMD register
  simdjson_really_inline base(const __m128i _value) : value(_value) {}

  // Conversion to SIMD register
  simdjson_really_inline operator const __m128i &() const {
    return this->value;
  }
  simdjson_really_inline operator __m128i &() { return this->value; }

  // Bit operations
  simdjson_really_inline Child operator|(const Child other) const {
    return vec_or(this->value, (__m128i)other);
  }
  simdjson_really_inline Child operator&(const Child other) const {
    return vec_and(this->value, (__m128i)other);
  }
  simdjson_really_inline Child operator^(const Child other) const {
    return vec_xor(this->value, (__m128i)other);
  }
  simdjson_really_inline Child bit_andnot(const Child other) const {
    return vec_andc(this->value, (__m128i)other);
  }
  simdjson_really_inline Child &operator|=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast | other;
    return *this_cast;
  }
  simdjson_really_inline Child &operator&=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast & other;
    return *this_cast;
  }
  simdjson_really_inline Child &operator^=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast ^ other;
    return *this_cast;
  }
};

// Forward-declared so they can be used by splat and friends.
template <typename T> struct simd8;

template <typename T, typename Mask = simd8<bool>>
struct base8 : base<simd8<T>> {
  typedef uint16_t bitmask_t;
  typedef uint32_t bitmask2_t;

  simdjson_really_inline base8() : base<simd8<T>>() {}
  simdjson_really_inline base8(const __m128i _value) : base<simd8<T>>(_value) {}

  simdjson_really_inline Mask operator==(const simd8<T> other) const {
    return (__m128i)vec_cmpeq(this->value, (__m128i)other);
  }

  static const int SIZE = sizeof(base<simd8<T>>::value);

  template <int N = 1>
  simdjson_really_inline simd8<T> prev(simd8<T> prev_chunk) const {
    __m128i chunk = this->value;
#ifdef __LITTLE_ENDIAN__
    chunk = (__m128i)vec_reve(this->value);
    prev_chunk = (__m128i)vec_reve((__m128i)prev_chunk);
#endif
    chunk = (__m128i)vec_sld((__m128i)prev_chunk, (__m128i)chunk, 16 - N);
#ifdef __LITTLE_ENDIAN__
    chunk = (__m128i)vec_reve((__m128i)chunk);
#endif
    return chunk;
  }
};

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd8<bool> : base8<bool> {
  static simdjson_really_inline simd8<bool> splat(bool _value) {
    return (__m128i)vec_splats((unsigned char)(-(!!_value)));
  }

  simdjson_really_inline simd8<bool>() : base8() {}
  simdjson_really_inline simd8<bool>(const __m128i _value)
      : base8<bool>(_value) {}
  // Splat constructor
  simdjson_really_inline simd8<bool>(bool _value)
      : base8<bool>(splat(_value)) {}

  simdjson_really_inline int to_bitmask() const {
    __vector unsigned long long result;
    const __m128i perm_mask = {0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40,
                               0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x08, 0x00};

    result = ((__vector unsigned long long)vec_vbpermq((__m128i)this->value,
                                                       (__m128i)perm_mask));
#ifdef __LITTLE_ENDIAN__
    return static_cast<int>(result[1]);
#else
    return static_cast<int>(result[0]);
#endif
  }
  simdjson_really_inline bool any() const {
    return !vec_all_eq(this->value, (__m128i)vec_splats(0));
  }
  simdjson_really_inline simd8<bool> operator~() const {
    return this->value ^ (__m128i)splat(true);
  }
};

template <typename T> struct base8_numeric : base8<T> {
  static simdjson_really_inline simd8<T> splat(T value) {
    (void)value;
    return (__m128i)vec_splats(value);
  }
  static simdjson_really_inline simd8<T> zero() { return splat(0); }
  static simdjson_really_inline simd8<T> load(const T values[16]) {
    return (__m128i)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(values)));
  }
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  static simdjson_really_inline simd8<T> repeat_16(T v0, T v1, T v2, T v3, T v4,
                                                   T v5, T v6, T v7, T v8, T v9,
                                                   T v10, T v11, T v12, T v13,
                                                   T v14, T v15) {
    return simd8<T>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                    v14, v15);
  }

  simdjson_really_inline base8_numeric() : base8<T>() {}
  simdjson_really_inline base8_numeric(const __m128i _value)
      : base8<T>(_value) {}

  // Store to array
  simdjson_really_inline void store(T dst[16]) const {
    vec_vsx_st(this->value, 0, reinterpret_cast<__m128i *>(dst));
  }

  // Override to distinguish from bool version
  simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

  // Addition/subtraction are the same for signed and unsigned
  simdjson_really_inline simd8<T> operator+(const simd8<T> other) const {
    return (__m128i)((__m128i)this->value + (__m128i)other);
  }
  simdjson_really_inline simd8<T> operator-(const simd8<T> other) const {
    return (__m128i)((__m128i)this->value - (__m128i)other);
  }
  simdjson_really_inline simd8<T> &operator+=(const simd8<T> other) {
    *this = *this + other;
    return *static_cast<simd8<T> *>(this);
  }
  simdjson_really_inline simd8<T> &operator-=(const simd8<T> other) {
    *this = *this - other;
    return *static_cast<simd8<T> *>(this);
  }

  // Perform a lookup assuming the value is between 0 and 16 (undefined behavior
  // for out of range values)
  template <typename L>
  simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
    return (__m128i)vec_perm((__m128i)lookup_table, (__m128i)lookup_table, this->value);
  }

  // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted
  // as a bitset). Passing a 0 value for mask would be equivalent to writing out
  // every byte to output. Only the first 16 - count_ones(mask) bytes of the
  // result are significant but 16 bytes get written. Design consideration: it
  // seems like a function with the signature simd8<L> compress(uint32_t mask)
  // would be sensible, but the AVX ISA makes this kind of approach difficult.
  template <typename L>
  simdjson_really_inline void compress(uint16_t mask, L *output) const {
    using internal::BitsSetTable256mul2;
    using internal::pshufb_combine_table;
    using internal::thintable_epi8;
    // this particular implementation was inspired by work done by @animetosho
    // we do it in two steps, first 8 bytes and then second 8 bytes
    uint8_t mask1 = uint8_t(mask);      // least significant 8 bits
    uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
    // next line just loads the 64-bit values thintable_epi8[mask1] and
    // thintable_epi8[mask2] into a 128-bit register, using only
    // two instructions on most compilers.
#ifdef __LITTLE_ENDIAN__
    __m128i shufmask = (__m128i)(__vector unsigned long long){
        thintable_epi8[mask1], thintable_epi8[mask2]};
#else
    __m128i shufmask = (__m128i)(__vector unsigned long long){
        thintable_epi8[mask2], thintable_epi8[mask1]};
    shufmask = (__m128i)vec_reve((__m128i)shufmask);
#endif
    // we increment by 0x08 the second half of the mask
    shufmask = ((__m128i)shufmask) +
               ((__m128i)(__vector int){0, 0, 0x08080808, 0x08080808});

    // this is the version "nearly pruned"
    __m128i pruned = vec_perm(this->value, this->value, shufmask);
    // we still need to put the two halves together.
    // we compute the popcount of the first half:
    int pop1 = BitsSetTable256mul2[mask1];
    // then load the corresponding mask, what it does is to write
    // only the first pop1 bytes from the first 8 bytes, and then
    // it fills in with the bytes from the second 8 bytes + some filling
    // at the end.
    __m128i compactmask =
        vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(pshufb_combine_table + pop1 * 8));
    __m128i answer = vec_perm(pruned, (__m128i)vec_splats(0), compactmask);
    vec_vsx_st(answer, 0, reinterpret_cast<__m128i *>(output));
  }

  template <typename L>
  simdjson_really_inline simd8<L>
  lookup_16(L replace0, L replace1, L replace2, L replace3, L replace4,
            L replace5, L replace6, L replace7, L replace8, L replace9,
            L replace10, L replace11, L replace12, L replace13, L replace14,
            L replace15) const {
    return lookup_16(simd8<L>::repeat_16(
        replace0, replace1, replace2, replace3, replace4, replace5, replace6,
        replace7, replace8, replace9, replace10, replace11, replace12,
        replace13, replace14, replace15));
  }
};

// Signed bytes
template <> struct simd8<int8_t> : base8_numeric<int8_t> {
  simdjson_really_inline simd8() : base8_numeric<int8_t>() {}
  simdjson_really_inline simd8(const __m128i _value)
      : base8_numeric<int8_t>(_value) {}
  // Splat constructor
  simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  simdjson_really_inline simd8(const int8_t *values) : simd8(load(values)) {}
  // Member-by-member initialization
  simdjson_really_inline simd8(int8_t v0, int8_t v1, int8_t v2, int8_t v3,
                               int8_t v4, int8_t v5, int8_t v6, int8_t v7,
                               int8_t v8, int8_t v9, int8_t v10, int8_t v11,
                               int8_t v12, int8_t v13, int8_t v14, int8_t v15)
      : simd8((__m128i)(__vector signed char){v0, v1, v2, v3, v4, v5, v6, v7,
                                              v8, v9, v10, v11, v12, v13, v14,
                                              v15}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  simdjson_really_inline static simd8<int8_t>
  repeat_16(int8_t v0, int8_t v1, int8_t v2, int8_t v3, int8_t v4, int8_t v5,
            int8_t v6, int8_t v7, int8_t v8, int8_t v9, int8_t v10, int8_t v11,
            int8_t v12, int8_t v13, int8_t v14, int8_t v15) {
    return simd8<int8_t>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                         v13, v14, v15);
  }

  // Order-sensitive comparisons
  simdjson_really_inline simd8<int8_t>
  max_val(const simd8<int8_t> other) const {
    return (__m128i)vec_max((__vector signed char)this->value,
                            (__vector signed char)(__m128i)other);
  }
  simdjson_really_inline simd8<int8_t>
  min_val(const simd8<int8_t> other) const {
    return (__m128i)vec_min((__vector signed char)this->value,
                            (__vector signed char)(__m128i)other);
  }
  simdjson_really_inline simd8<bool>
  operator>(const simd8<int8_t> other) const {
    return (__m128i)vec_cmpgt((__vector signed char)this->value,
                              (__vector signed char)(__m128i)other);
  }
  simdjson_really_inline simd8<bool>
  operator<(const simd8<int8_t> other) const {
    return (__m128i)vec_cmplt((__vector signed char)this->value,
                              (__vector signed char)(__m128i)other);
  }
};

// Unsigned bytes
template <> struct simd8<uint8_t> : base8_numeric<uint8_t> {
  simdjson_really_inline simd8() : base8_numeric<uint8_t>() {}
  simdjson_really_inline simd8(const __m128i _value)
      : base8_numeric<uint8_t>(_value) {}
  // Splat constructor
  simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  simdjson_really_inline simd8(const uint8_t *values) : simd8(load(values)) {}
  // Member-by-member initialization
  simdjson_really_inline
  simd8(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5,
        uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9, uint8_t v10,
        uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15)
      : simd8((__m128i){v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                        v13, v14, v15}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  simdjson_really_inline static simd8<uint8_t>
  repeat_16(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4,
            uint8_t v5, uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9,
            uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14,
            uint8_t v15) {
    return simd8<uint8_t>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                          v13, v14, v15);
  }

  // Saturated math
  simdjson_really_inline simd8<uint8_t>
  saturating_add(const simd8<uint8_t> other) const {
    return (__m128i)vec_adds(this->value, (__m128i)other);
  }
  simdjson_really_inline simd8<uint8_t>
  saturating_sub(const simd8<uint8_t> other) const {
    return (__m128i)vec_subs(this->value, (__m128i)other);
  }

  // Order-specific operations
  simdjson_really_inline simd8<uint8_t>
  max_val(const simd8<uint8_t> other) const {
    return (__m128i)vec_max(this->value, (__m128i)other);
  }
  simdjson_really_inline simd8<uint8_t>
  min_val(const simd8<uint8_t> other) const {
    return (__m128i)vec_min(this->value, (__m128i)other);
  }
  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  simdjson_really_inline simd8<uint8_t>
  gt_bits(const simd8<uint8_t> other) const {
    return this->saturating_sub(other);
  }
  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  simdjson_really_inline simd8<uint8_t>
  lt_bits(const simd8<uint8_t> other) const {
    return other.saturating_sub(*this);
  }
  simdjson_really_inline simd8<bool>
  operator<=(const simd8<uint8_t> other) const {
    return other.max_val(*this) == other;
  }
  simdjson_really_inline simd8<bool>
  operator>=(const simd8<uint8_t> other) const {
    return other.min_val(*this) == other;
  }
  simdjson_really_inline simd8<bool>
  operator>(const simd8<uint8_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }
  simdjson_really_inline simd8<bool>
  operator<(const simd8<uint8_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }

  // Bit-specific operations
  simdjson_really_inline simd8<bool> bits_not_set() const {
    return (__m128i)vec_cmpeq(this->value, (__m128i)vec_splats(uint8_t(0)));
  }
  simdjson_really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const {
    return (*this & bits).bits_not_set();
  }
  simdjson_really_inline simd8<bool> any_bits_set() const {
    return ~this->bits_not_set();
  }
  simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const {
    return ~this->bits_not_set(bits);
  }
  simdjson_really_inline bool bits_not_set_anywhere() const {
    return vec_all_eq(this->value, (__m128i)vec_splats(0));
  }
  simdjson_really_inline bool any_bits_set_anywhere() const {
    return !bits_not_set_anywhere();
  }
  simdjson_really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const {
    return vec_all_eq(vec_and(this->value, (__m128i)bits),
                      (__m128i)vec_splats(0));
  }
  simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const {
    return !bits_not_set_anywhere(bits);
  }
  template <int N> simdjson_really_inline simd8<uint8_t> shr() const {
    return simd8<uint8_t>(
        (__m128i)vec_sr(this->value, (__m128i)vec_splat_u8(N)));
  }
  template <int N> simdjson_really_inline simd8<uint8_t> shl() const {
    return simd8<uint8_t>(
        (__m128i)vec_sl(this->value, (__m128i)vec_splat_u8(N)));
  }
};

template <typename T> struct simd8x64 {
  static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
  static_assert(NUM_CHUNKS == 4,
                "PPC64 kernel should use four registers per 64-byte block.");
  const simd8<T> chunks[NUM_CHUNKS];

  simd8x64(const simd8x64<T> &o) = delete; // no copy allowed
  simd8x64<T> &
  operator=(const simd8<T>& other) = delete; // no assignment allowed
  simd8x64() = delete;                      // no default constructor allowed

  simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1,
                                  const simd8<T> chunk2, const simd8<T> chunk3)
      : chunks{chunk0, chunk1, chunk2, chunk3} {}
  simdjson_really_inline simd8x64(const T ptr[64])
      : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr + 16),
               simd8<T>::load(ptr + 32), simd8<T>::load(ptr + 48)} {}

  simdjson_really_inline void store(T ptr[64]) const {
    this->chunks[0].store(ptr + sizeof(simd8<T>) * 0);
    this->chunks[1].store(ptr + sizeof(simd8<T>) * 1);
    this->chunks[2].store(ptr + sizeof(simd8<T>) * 2);
    this->chunks[3].store(ptr + sizeof(simd8<T>) * 3);
  }

  simdjson_really_inline simd8<T> reduce_or() const {
    return (this->chunks[0] | this->chunks[1]) |
           (this->chunks[2] | this->chunks[3]);
  }

  simdjson_really_inline void compress(uint64_t mask, T *output) const {
    this->chunks[0].compress(uint16_t(mask), output);
    this->chunks[1].compress(uint16_t(mask >> 16),
                             output + 16 - count_ones(mask & 0xFFFF));
    this->chunks[2].compress(uint16_t(mask >> 32),
                             output + 32 - count_ones(mask & 0xFFFFFFFF));
    this->chunks[3].compress(uint16_t(mask >> 48),
                             output + 48 - count_ones(mask & 0xFFFFFFFFFFFF));
  }

  simdjson_really_inline uint64_t to_bitmask() const {
    uint64_t r0 = uint32_t(this->chunks[0].to_bitmask());
    uint64_t r1 = this->chunks[1].to_bitmask();
    uint64_t r2 = this->chunks[2].to_bitmask();
    uint64_t r3 = this->chunks[3].to_bitmask();
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  simdjson_really_inline uint64_t eq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] == mask, this->chunks[1] == mask,
                          this->chunks[2] == mask, this->chunks[3] == mask)
        .to_bitmask();
  }

  simdjson_really_inline uint64_t eq(const simd8x64<uint8_t> &other) const {
    return simd8x64<bool>(this->chunks[0] == other.chunks[0],
                          this->chunks[1] == other.chunks[1],
                          this->chunks[2] == other.chunks[2],
                          this->chunks[3] == other.chunks[3])
        .to_bitmask();
  }

  simdjson_really_inline uint64_t lteq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] <= mask, this->chunks[1] <= mask,
                          this->chunks[2] <= mask, this->chunks[3] <= mask)
        .to_bitmask();
  }
}; // struct simd8x64<T>

} // namespace simd
} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#endif // SIMDJSON_PPC64_SIMD_INPUT_H
/* end file include/simdjson/ppc64/simd.h */
/* begin file include/simdjson/generic/jsoncharutils.h */

namespace simdjson {
namespace ppc64 {
namespace {
namespace jsoncharutils {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = internal::digit_to_val32[630 + src[0]];
  uint32_t v2 = internal::digit_to_val32[420 + src[1]];
  uint32_t v3 = internal::digit_to_val32[210 + src[2]];
  uint32_t v4 = internal::digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

using internal::value128;

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace jsoncharutils
} // unnamed namespace
} // namespace ppc64
} // namespace simdjson
/* end file include/simdjson/generic/jsoncharutils.h */
/* begin file include/simdjson/generic/atomparsing.h */
namespace simdjson {
namespace ppc64 {
namespace {
/// @private
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
simdjson_warn_unused
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | jsoncharutils::is_not_structural_or_whitespace(src[5])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // unnamed namespace
} // namespace ppc64
} // namespace simdjson
/* end file include/simdjson/generic/atomparsing.h */
/* begin file include/simdjson/ppc64/stringparsing.h */
#ifndef SIMDJSON_PPC64_STRINGPARSING_H
#define SIMDJSON_PPC64_STRINGPARSING_H


namespace simdjson {
namespace ppc64 {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote
  copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() {
    return ((bs_bits - 1) & quote_bits) != 0;
  }
  simdjson_really_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_really_inline int quote_index() {
    return trailing_zeroes(quote_bits);
  }
  simdjson_really_inline int backslash_index() {
    return trailing_zeroes(bs_bits);
  }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote
backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1),
                "backslash and quote finder must process fewer than "
                "SIMDJSON_PADDING bytes");
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + sizeof(v0));
  v0.store(dst);
  v1.store(dst + sizeof(v0));

  // Getting a 64-bit bitmask is much cheaper than multiple 16-bit bitmasks on
  // PPC; therefore, we smash them together into a 64-byte mask and get the
  // bitmask from there.
  uint64_t bs_and_quote =
      simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
      uint32_t(bs_and_quote),      // bs_bits
      uint32_t(bs_and_quote >> 32) // quote_bits
  };
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

/* begin file include/simdjson/generic/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace simdjson {
namespace ppc64 {
namespace {
/// @private
namespace stringparsing {

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
simdjson_warn_unused
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // jsoncharutils::hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = jsoncharutils::codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

/**
 * Unescape a string from src to dst, stopping at a final unescaped quote. E.g., if src points at 'joe"', then
 * dst needs to have four free bytes.
 */
simdjson_warn_unused simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

simdjson_unused simdjson_warn_unused simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (*(src++) != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view(reinterpret_cast<const char *>(current_string_buf_loc), end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // unnamed namespace
} // namespace ppc64
} // namespace simdjson
/* end file include/simdjson/generic/stringparsing.h */

#endif // SIMDJSON_PPC64_STRINGPARSING_H
/* end file include/simdjson/ppc64/stringparsing.h */
/* begin file include/simdjson/ppc64/numberparsing.h */
#ifndef SIMDJSON_PPC64_NUMBERPARSING_H
#define SIMDJSON_PPC64_NUMBERPARSING_H

#include <byteswap.h>

namespace simdjson {
namespace ppc64 {
namespace {

// we don't have appropriate instructions, so let us use a scalar function
// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
static simdjson_really_inline uint32_t
parse_eight_digits_unrolled(const uint8_t *chars) {
  uint64_t val;
  std::memcpy(&val, chars, sizeof(uint64_t));
#ifdef __BIG_ENDIAN__
  val = bswap_64(val);
#endif
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#define SWAR_NUMBER_PARSING

/* begin file include/simdjson/generic/numberparsing.h */
#include <limits>

namespace simdjson {
namespace ppc64 {
namespace {
/// @private
namespace numberparsing {



#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

namespace {
// Convert a mantissa, an exponent and a sign bit into an ieee64 double.
// The real_exponent needs to be in [0, 2046] (technically real_exponent = 2047 would be acceptable).
// The mantissa should be in [0,1<<53). The bit at index (1ULL << 52) while be zeroed.
simdjson_really_inline double to_double(uint64_t mantissa, uint64_t real_exponent, bool negative) {
    double d;
    mantissa &= ~(1ULL << 52);
    mantissa |= real_exponent << 52;
    mantissa |= ((static_cast<uint64_t>(negative)) << 63);
    std::memcpy(&d, &mantissa, sizeof(d));
    return d;
}
}
// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [smallest_power,
// largest_power] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / simdjson::internal::power_of_ten[-power];
    } else {
      d = d * simdjson::internal::power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It was
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }


  // The exponent is 1024 + 63 + power
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power when power >= 0
  // and it is equal to
  //  ceil(log(5**-power)/log(2)) + power when power < 0
  //
  // The 65536 is (1<<16) and corresponds to
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to
  // floor(log(5**power)/log(2))
  //
  // Note that this is not magic: 152170/(1<<16) is
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;


  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;


  // We are going to need to do some 64-bit arithmetic to get a precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= smallest_power
  // and power <= largest_power
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  //
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  const uint32_t index = 2 * uint32_t(power - simdjson::internal::smallest_power);
  // Optimization: It may be that materializing the index as a variable might confuse some compilers and prevent effective complex-addressing loads. (Done for code clarity.)
  //
  // The full_multiplication function computes the 128-bit product of two 64-bit words
  // with a returned value of type value128 with a "low component" corresponding to the
  // 64-bit least significant bits of the product and with a "high component" corresponding
  // to the 64-bit most significant bits of the product.
  simdjson::internal::value128 firstproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index]);
  // Both i and power_of_five_128[index] have their most significant bit set to 1 which
  // implies that the either the most or the second most significant bit of the product
  // is 1. We pack values in this manner for efficiency reasons: it maximizes the use
  // we make of the product. It also makes it easy to reason aboutthe product: there
  // 0 or 1 leading zero in the product.

  // Unless the least significant 9 bits of the high (64-bit) part of the full
  // product are all 1s, then we know that the most significant 55 bits are
  // exact and no further work is needed. Having 55 bits is necessary because
  // we need 53 bits for the mantissa but we have to have one rounding bit and
  // we can waste a bit if the most significant bit of the product is zero.
  if((firstproduct.high & 0x1FF) == 0x1FF) {
    // We want to compute i * 5^q, but only care about the top 55 bits at most.
    // Consider the scenario where q>=0. Then 5^q may not fit in 64-bits. Doing
    // the full computation is wasteful. So we do what is called a "truncated
    // multiplication".
    // We take the most significant 64-bits, and we put them in
    // power_of_five_128[index]. Usually, that's good enough to approximate i * 5^q
    // to the desired approximation using one multiplication. Sometimes it does not suffice.
    // Then we store the next most significant 64 bits in power_of_five_128[index + 1], and
    // then we get a better approximation to i * 5^q. In very rare cases, even that
    // will not suffice, though it is seemingly very hard to find such a scenario.
    //
    // That's for when q>=0. The logic for q<0 is somewhat similar but it is somewhat
    // more complicated.
    //
    // There is an extra layer of complexity in that we need more than 55 bits of
    // accuracy in the round-to-even scenario.
    //
    // The full_multiplication function computes the 128-bit product of two 64-bit words
    // with a returned value of type value128 with a "low component" corresponding to the
    // 64-bit least significant bits of the product and with a "high component" corresponding
    // to the 64-bit most significant bits of the product.
    simdjson::internal::value128 secondproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index + 1]);
    firstproduct.low += secondproduct.high;
    if(secondproduct.high > firstproduct.low) { firstproduct.high++; }
    // At this point, we might need to add at most one to firstproduct, but this
    // can only change the value of firstproduct.high if firstproduct.low is maximal.
    if(simdjson_unlikely(firstproduct.low  == 0xFFFFFFFFFFFFFFFF)) {
      // This is very unlikely, but if so, we need to do much more work!
      return false;
    }
  }
  uint64_t lower = firstproduct.low;
  uint64_t upper = firstproduct.high;
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).
  int64_t real_exponent = exponent - lz;
  if (simdjson_unlikely(real_exponent <= 0)) { // we have a subnormal?
    // Here have that real_exponent <= 0 so -real_exponent >= 0
    if(-real_exponent + 1 >= 64) { // if we have more than 64 bits below the minimum exponent, you have a zero for sure.
      d = 0.0;
      return true;
    }
    // next line is safe because -real_exponent + 1 < 0
    mantissa >>= -real_exponent + 1;
    // Thankfully, we can't have both "round-to-even" and subnormals because
    // "round-to-even" only occurs for powers close to 0.
    mantissa += (mantissa & 1); // round up
    mantissa >>= 1;
    // There is a weird scenario where we don't have a subnormal but just.
    // Suppose we start with 2.2250738585072013e-308, we end up
    // with 0x3fffffffffffff x 2^-1023-53 which is technically subnormal
    // whereas 0x40000000000000 x 2^-1023-53  is normal. Now, we need to round
    // up 0x3fffffffffffff x 2^-1023-53  and once we do, we are no longer
    // subnormal, but we can only know this after rounding.
    // So we only declare a subnormal if we are smaller than the threshold.
    real_exponent = (mantissa < (uint64_t(1) << 52)) ? 0 : 1;
    d = to_double(mantissa, real_exponent, negative);
    return true;
  }
  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  //
  // The round-to-even cases take the form of a number 2m+1 which is in (2^53,2^54]
  // times a power of two. That is, it is right between a number with binary significand
  // m and another number with binary significand m+1; and it must be the case
  // that it cannot be represented by a float itself.
  //
  // We must have that w * 10 ^q == (2m+1) * 2^p for some power of two 2^p.
  // Recall that 10^q = 5^q * 2^q.
  // When q >= 0, we must have that (2m+1) is divible by 5^q, so 5^q <= 2^54. We have that
  //  5^23 <=  2^54 and it is the last power of five to qualify, so q <= 23.
  // When q<0, we have  w  >=  (2m+1) x 5^{-q}.  We must have that w<2^{64} so
  // (2m+1) x 5^{-q} < 2^{64}. We have that 2m+1>2^{53}. Hence, we must have
  // 2^{53} x 5^{-q} < 2^{64}.
  // Hence we have 5^{-q} < 2^{11}$ or q>= -4.
  //
  // We require lower <= 1 and not lower == 0 because we could not prove that
  // that lower == 0 is implied; but we could prove that lower <= 1 is a necessary and sufficient test.
  if (simdjson_unlikely((lower <= 1) && (power >= -4) && (power <= 23) && ((mantissa & 3) == 1))) {
    if((mantissa  << (upperbit + 64 - 53 - 2)) ==  upper) {
      mantissa &= ~1;             // flip it so that we do not round up
    }
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    real_exponent++;
  }
  mantissa &= ~(1ULL << 52);
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely(real_exponent > 2046)) {
    // We have an infinte value!!! We could actually throw an error here if we could.
    return false;
  }
  d = to_double(mantissa, real_exponent, negative);
  return true;
}

// We call a fallback floating-point parser that might be slow. Note
// it will accept JSON numbers, but the JSON spec. is more restrictive so
// before you call parse_float_fallback, you need to have validated the input
// string with the JSON grammar.
// It will return an error (false) if the parsed number is infinite.
// The string parsing itself always succeeds. We know that there is at least
// one digit.
static bool parse_float_fallback(const uint8_t *ptr, double *outDouble) {
  *outDouble = simdjson::internal::from_chars(reinterpret_cast<const char *>(ptr));
  // We do not accept infinite values.

  // Detecting finite values in a portable manner is ridiculously hard, ideally
  // we would want to do:
  // return !std::isfinite(*outDouble);
  // but that mysteriously fails under legacy/old libc++ libraries, see
  // https://github.com/simdjson/simdjson/issues/1286
  //
  // Therefore, fall back to this solution (the extra parens are there
  // to handle that max may be a macro on windows).
  return !(*outDouble > (std::numeric_limits<double>::max)() || *outDouble < std::numeric_limits<double>::lowest());
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  std::memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(simdjson_unused const uint8_t * src, W writer) {
  double d;
  if (parse_float_fallback(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(simdjson_unused const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(simdjson_unused const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow.
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time.
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline size_t significant_digits(const uint8_t * start_digits, size_t digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - size_t(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, size_t digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  //
  // 9999999999999999999 < 2**64 so we can accomodate 19 digits.
  // If we have a decimal separator, then digit_count - 1 is the number of digits, but we
  // may not have a decimal separator!
  if (simdjson_unlikely(digit_count > 19 && significant_digits(start_digits, digit_count) > 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < simdjson::internal::smallest_power) || (exponent > simdjson::internal::largest_power)) {
    //
    // Important: smallest_power is such that it leads to a zero value.
    // Observe that 18446744073709551615e-343 == 0, i.e. (2**64 - 1) e -343 is zero
    // so something x 10^-343 goes to zero, but not so with  something x 10^-342.
    static_assert(simdjson::internal::smallest_power <= -342, "smallest_power is not small enough");
    //
    if((exponent < simdjson::internal::smallest_power) || (i == 0)) {
      WRITE_DOUBLE(0, src, writer);
      return SUCCESS;
    } else { // (exponent > largest_power) and (i != 0)
      // We have, for sure, an infinite value and simdjson refuses to parse infinite values.
      return INVALID_NUMBER(src);
    }
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_fallback(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * const src) noexcept { return 0; }

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool dirty_end = jsoncharutils::is_not_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (dirty_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// Inlineable functions
namespace {

// This table can be used to characterize the final character of an integer
// string. For JSON structural character and allowable white space characters,
// we return SUCCESS. For 'e', '.' and 'E', we return INCORRECT_TYPE. Otherwise
// we return NUMBER_ERROR.
// Optimization note: we could easily reduce the size of the table by half (to 128)
// at the cost of an extra branch.
// Optimization note: we want the values to use at most 8 bits (not, e.g., 32 bits):
static_assert(error_code(uint8_t(NUMBER_ERROR))== NUMBER_ERROR, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(SUCCESS))== SUCCESS, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(INCORRECT_TYPE))== INCORRECT_TYPE, "bad NUMBER_ERROR cast");

const uint8_t integer_string_finisher[256] = {
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   SUCCESS,      NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, INCORRECT_TYPE,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, SUCCESS,        NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR};

// Parse any number from 0 to 18,446,744,073,709,551,615
simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;
  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > 20))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > 20)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if (integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }

  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > longest_digit_count))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > longest_digit_count)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if(integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INCORRECT_TYPE; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return negative ? (~i+1) : i;
}

simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src ) { return INCORRECT_TYPE; }
  if ( (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. More than 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 > 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src > 19;
    }
  } else {
    overflow = p-src > 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
  }

  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  overflow = overflow || exponent < simdjson::internal::smallest_power || exponent > simdjson::internal::largest_power;

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_fallback(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // unnamed namespace
} // namespace ppc64
} // namespace simdjson
/* end file include/simdjson/generic/numberparsing.h */

#endif // SIMDJSON_PPC64_NUMBERPARSING_H
/* end file include/simdjson/ppc64/numberparsing.h */
/* begin file include/simdjson/ppc64/end.h */
/* end file include/simdjson/ppc64/end.h */

#endif // SIMDJSON_IMPLEMENTATION_PPC64

#endif // SIMDJSON_PPC64_H
/* end file include/simdjson/ppc64.h */
/* begin file include/simdjson/westmere.h */
#ifndef SIMDJSON_WESTMERE_H
#define SIMDJSON_WESTMERE_H


#if SIMDJSON_IMPLEMENTATION_WESTMERE

#if SIMDJSON_CAN_ALWAYS_RUN_WESTMERE
#define SIMDJSON_TARGET_WESTMERE
#define SIMDJSON_UNTARGET_WESTMERE
#else
#define SIMDJSON_TARGET_WESTMERE SIMDJSON_TARGET_REGION("sse4.2,pclmul")
#define SIMDJSON_UNTARGET_WESTMERE SIMDJSON_UNTARGET_REGION
#endif

namespace simdjson {
/**
 * Implementation for Westmere (Intel SSE4.2).
 */
namespace westmere {
} // namespace westmere
} // namespace simdjson

//
// These two need to be included outside SIMDJSON_TARGET_WESTMERE
//
/* begin file include/simdjson/westmere/implementation.h */
#ifndef SIMDJSON_WESTMERE_IMPLEMENTATION_H
#define SIMDJSON_WESTMERE_IMPLEMENTATION_H


// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_WESTMERE
namespace simdjson {
namespace westmere {

namespace {
using namespace simdjson;
using namespace simdjson::dom;
}

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation("westmere", "Intel/AMD SSE4.2", internal::instruction_set::SSE42 | internal::instruction_set::PCLMULQDQ) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_IMPLEMENTATION_H
/* end file include/simdjson/westmere/implementation.h */
/* begin file include/simdjson/westmere/intrinsics.h */
#ifndef SIMDJSON_WESTMERE_INTRINSICS_H
#define SIMDJSON_WESTMERE_INTRINSICS_H

#ifdef SIMDJSON_VISUAL_STUDIO
// under clang within visual studio, this will include <x86intrin.h>
#include <intrin.h> // visual studio or clang
#else
#include <x86intrin.h> // elsewhere
#endif // SIMDJSON_VISUAL_STUDIO


#ifdef SIMDJSON_CLANG_VISUAL_STUDIO
/**
 * You are not supposed, normally, to include these
 * headers directly. Instead you should either include intrin.h
 * or x86intrin.h. However, when compiling with clang
 * under Windows (i.e., when _MSC_VER is set), these headers
 * only get included *if* the corresponding features are detected
 * from macros:
 */
#include <smmintrin.h>  // for _mm_alignr_epi8
#include <wmmintrin.h>  // for  _mm_clmulepi64_si128
#endif



#endif // SIMDJSON_WESTMERE_INTRINSICS_H
/* end file include/simdjson/westmere/intrinsics.h */

//
// The rest need to be inside the region
//
/* begin file include/simdjson/westmere/begin.h */
// redefining SIMDJSON_IMPLEMENTATION to "westmere"
// #define SIMDJSON_IMPLEMENTATION westmere
SIMDJSON_TARGET_WESTMERE
/* end file include/simdjson/westmere/begin.h */

// Declarations
/* begin file include/simdjson/generic/dom_parser_implementation.h */

namespace simdjson {
namespace westmere {

// expectation: sizeof(open_container) = 64/8.
struct open_container {
  uint32_t tape_index; // where, on the tape, does the scope ([,{) begins
  uint32_t count; // how many elements in the scope
}; // struct open_container

static_assert(sizeof(open_container) == 64/8, "Open container must be 64 bits");

class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /** Tape location of each open { or [ */
  std::unique_ptr<open_container[]> open_containers{};
  /** Whether each open container is a [ or { */
  std::unique_ptr<bool[]> is_array{};
  /** Buffer passed to stage 1 */
  const uint8_t *buf{};
  /** Length passed to stage 1 */
  size_t len{0};
  /** Document passed to stage 2 */
  dom::document *doc{};

  inline dom_parser_implementation() noexcept;
  inline dom_parser_implementation(dom_parser_implementation &&other) noexcept;
  inline dom_parser_implementation &operator=(dom_parser_implementation &&other) noexcept;
  dom_parser_implementation(const dom_parser_implementation &) = delete;
  dom_parser_implementation &operator=(const dom_parser_implementation &) = delete;

  simdjson_warn_unused error_code parse(const uint8_t *buf, size_t len, dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  simdjson_warn_unused error_code stage2(dom::document &doc) noexcept final;
  simdjson_warn_unused error_code stage2_next(dom::document &doc) noexcept final;
  inline simdjson_warn_unused error_code set_capacity(size_t capacity) noexcept final;
  inline simdjson_warn_unused error_code set_max_depth(size_t max_depth) noexcept final;
private:
  simdjson_really_inline simdjson_warn_unused error_code set_capacity_stage1(size_t capacity);

};

} // namespace westmere
} // namespace simdjson

namespace simdjson {
namespace westmere {

inline dom_parser_implementation::dom_parser_implementation() noexcept = default;
inline dom_parser_implementation::dom_parser_implementation(dom_parser_implementation &&other) noexcept = default;
inline dom_parser_implementation &dom_parser_implementation::operator=(dom_parser_implementation &&other) noexcept = default;

// Leaving these here so they can be inlined if so desired
inline simdjson_warn_unused error_code dom_parser_implementation::set_capacity(size_t capacity) noexcept {
  if(capacity > SIMDJSON_MAXSIZE_BYTES) { return CAPACITY; }
  // Stage 1 index output
  size_t max_structures = SIMDJSON_ROUNDUP_N(capacity, 64) + 2 + 7;
  structural_indexes.reset( new (std::nothrow) uint32_t[max_structures] );
  if (!structural_indexes) { _capacity = 0; return MEMALLOC; }
  structural_indexes[0] = 0;
  n_structural_indexes = 0;

  _capacity = capacity;
  return SUCCESS;
}

inline simdjson_warn_unused error_code dom_parser_implementation::set_max_depth(size_t max_depth) noexcept {
  // Stage 2 stacks
  open_containers.reset(new (std::nothrow) open_container[max_depth]);
  is_array.reset(new (std::nothrow) bool[max_depth]);
  if (!is_array || !open_containers) { _max_depth = 0; return MEMALLOC; }

  _max_depth = max_depth;
  return SUCCESS;
}

} // namespace westmere
} // namespace simdjson
/* end file include/simdjson/generic/dom_parser_implementation.h */
/* begin file include/simdjson/westmere/bitmanipulation.h */
#ifndef SIMDJSON_WESTMERE_BITMANIPULATION_H
#define SIMDJSON_WESTMERE_BITMANIPULATION_H

namespace simdjson {
namespace westmere {
namespace {

// We sometimes call trailing_zero on inputs that are zero,
// but the algorithms do not end up using the returned value.
// Sadly, sanitizers are not smart enough to figure it out.
NO_SANITIZE_UNDEFINED
simdjson_really_inline int trailing_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long ret;
  // Search the mask data from least significant bit (LSB)
  // to the most significant bit (MSB) for a set bit (1).
  _BitScanForward64(&ret, input_num);
  return (int)ret;
#else // SIMDJSON_REGULAR_VISUAL_STUDIO
  return __builtin_ctzll(input_num);
#endif // SIMDJSON_REGULAR_VISUAL_STUDIO
}

/* result might be undefined when input_num is zero */
simdjson_really_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num-1);
}

/* result might be undefined when input_num is zero */
simdjson_really_inline int leading_zeroes(uint64_t input_num) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  unsigned long leading_zero = 0;
  // Search the mask data from most significant bit (MSB)
  // to least significant bit (LSB) for a set bit (1).
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif// SIMDJSON_REGULAR_VISUAL_STUDIO
}

#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
simdjson_really_inline unsigned __int64 count_ones(uint64_t input_num) {
  // note: we do not support legacy 32-bit Windows
  return __popcnt64(input_num);// Visual Studio wants two underscores
}
#else
simdjson_really_inline long long int count_ones(uint64_t input_num) {
  return _popcnt64(input_num);
}
#endif

simdjson_really_inline bool add_overflow(uint64_t value1, uint64_t value2,
                                uint64_t *result) {
#ifdef SIMDJSON_REGULAR_VISUAL_STUDIO
  return _addcarry_u64(0, value1, value2,
                       reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2,
                                   reinterpret_cast<unsigned long long *>(result));
#endif
}

} // unnamed namespace
} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_BITMANIPULATION_H
/* end file include/simdjson/westmere/bitmanipulation.h */
/* begin file include/simdjson/westmere/bitmask.h */
#ifndef SIMDJSON_WESTMERE_BITMASK_H
#define SIMDJSON_WESTMERE_BITMASK_H

namespace simdjson {
namespace westmere {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
simdjson_really_inline uint64_t prefix_xor(const uint64_t bitmask) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return _mm_cvtsi128_si64(result);
}

} // unnamed namespace
} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_BITMASK_H
/* end file include/simdjson/westmere/bitmask.h */
/* begin file include/simdjson/westmere/simd.h */
#ifndef SIMDJSON_WESTMERE_SIMD_H
#define SIMDJSON_WESTMERE_SIMD_H


namespace simdjson {
namespace westmere {
namespace {
namespace simd {

  template<typename Child>
  struct base {
    __m128i value;

    // Zero constructor
    simdjson_really_inline base() : value{__m128i()} {}

    // Conversion from SIMD register
    simdjson_really_inline base(const __m128i _value) : value(_value) {}

    // Conversion to SIMD register
    simdjson_really_inline operator const __m128i&() const { return this->value; }
    simdjson_really_inline operator __m128i&() { return this->value; }

    // Bit operations
    simdjson_really_inline Child operator|(const Child other) const { return _mm_or_si128(*this, other); }
    simdjson_really_inline Child operator&(const Child other) const { return _mm_and_si128(*this, other); }
    simdjson_really_inline Child operator^(const Child other) const { return _mm_xor_si128(*this, other); }
    simdjson_really_inline Child bit_andnot(const Child other) const { return _mm_andnot_si128(other, *this); }
    simdjson_really_inline Child& operator|=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast | other; return *this_cast; }
    simdjson_really_inline Child& operator&=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast & other; return *this_cast; }
    simdjson_really_inline Child& operator^=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    simdjson_really_inline base8() : base<simd8<T>>() {}
    simdjson_really_inline base8(const __m128i _value) : base<simd8<T>>(_value) {}

    simdjson_really_inline Mask operator==(const simd8<T> other) const { return _mm_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<simd8<T>>::value);

    template<int N=1>
    simdjson_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm_alignr_epi8(*this, prev_chunk, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static simdjson_really_inline simd8<bool> splat(bool _value) { return _mm_set1_epi8(uint8_t(-(!!_value))); }

    simdjson_really_inline simd8<bool>() : base8() {}
    simdjson_really_inline simd8<bool>(const __m128i _value) : base8<bool>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    simdjson_really_inline int to_bitmask() const { return _mm_movemask_epi8(*this); }
    simdjson_really_inline bool any() const { return !_mm_testz_si128(*this, *this); }
    simdjson_really_inline simd8<bool> operator~() const { return *this ^ true; }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static simdjson_really_inline simd8<T> splat(T _value) { return _mm_set1_epi8(_value); }
    static simdjson_really_inline simd8<T> zero() { return _mm_setzero_si128(); }
    static simdjson_really_inline simd8<T> load(const T values[16]) {
      return _mm_loadu_si128(reinterpret_cast<const __m128i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static simdjson_really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    simdjson_really_inline base8_numeric() : base8<T>() {}
    simdjson_really_inline base8_numeric(const __m128i _value) : base8<T>(_value) {}

    // Store to array
    simdjson_really_inline void store(T dst[16]) const { return _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), *this); }

    // Override to distinguish from bool version
    simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_really_inline simd8<T> operator+(const simd8<T> other) const { return _mm_add_epi8(*this, other); }
    simdjson_really_inline simd8<T> operator-(const simd8<T> other) const { return _mm_sub_epi8(*this, other); }
    simdjson_really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *static_cast<simd8<T>*>(this); }
    simdjson_really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *static_cast<simd8<T>*>(this); }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm_shuffle_epi8(lookup_table, *this);
    }

    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 16 - count_ones(mask) bytes of the result are significant but 16 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint32_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    simdjson_really_inline void compress(uint16_t mask, L * output) const {
      using internal::thintable_epi8;
      using internal::BitsSetTable256mul2;
      using internal::pshufb_combine_table;
      // this particular implementation was inspired by work done by @animetosho
      // we do it in two steps, first 8 bytes and then second 8 bytes
      uint8_t mask1 = uint8_t(mask); // least significant 8 bits
      uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      __m128i shufmask =  _mm_set_epi64x(thintable_epi8[mask2], thintable_epi8[mask1]);
      // we increment by 0x08 the second half of the mask
      shufmask =
      _mm_add_epi8(shufmask, _mm_set_epi32(0x08080808, 0x08080808, 0, 0));
      // this is the version "nearly pruned"
      __m128i pruned = _mm_shuffle_epi8(*this, shufmask);
      // we still need to put the two halves together.
      // we compute the popcount of the first half:
      int pop1 = BitsSetTable256mul2[mask1];
      // then load the corresponding mask, what it does is to write
      // only the first pop1 bytes from the first 8 bytes, and then
      // it fills in with the bytes from the second 8 bytes + some filling
      // at the end.
      __m128i compactmask =
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(pshufb_combine_table + pop1 * 8));
      __m128i answer = _mm_shuffle_epi8(pruned, compactmask);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(output), answer);
    }

    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> : base8_numeric<int8_t> {
    simdjson_really_inline simd8() : base8_numeric<int8_t>() {}
    simdjson_really_inline simd8(const __m128i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    simdjson_really_inline simd8<int8_t> max_val(const simd8<int8_t> other) const { return _mm_max_epi8(*this, other); }
    simdjson_really_inline simd8<int8_t> min_val(const simd8<int8_t> other) const { return _mm_min_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(*this, other); }
    simdjson_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(other, *this); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    simdjson_really_inline simd8() : base8_numeric<uint8_t>() {}
    simdjson_really_inline simd8(const __m128i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const uint8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    simdjson_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm_adds_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm_subs_epu8(*this, other); }

    // Order-specific operations
    simdjson_really_inline simd8<uint8_t> max_val(const simd8<uint8_t> other) const { return _mm_max_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> min_val(const simd8<uint8_t> other) const { return _mm_min_epu8(*this, other); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    simdjson_really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max_val(*this) == other; }
    simdjson_really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min_val(*this) == other; }
    simdjson_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    simdjson_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }

    // Bit-specific operations
    simdjson_really_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    simdjson_really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    simdjson_really_inline bool is_ascii() const { return _mm_movemask_epi8(*this) == 0; }
    simdjson_really_inline bool bits_not_set_anywhere() const { return _mm_testz_si128(*this, *this); }
    simdjson_really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    simdjson_really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm_testz_si128(*this, bits); }
    simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    simdjson_really_inline int get_bit() const { return _mm_movemask_epi8(_mm_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 4, "Westmere kernel should use four registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T>& other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    simdjson_really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    simdjson_really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    simdjson_really_inline simd8<T> reduce_or() const {
      return (this->chunks[0] | this->chunks[1]) | (this->chunks[2] | this->chunks[3]);
    }

    simdjson_really_inline void compress(uint64_t mask, T * output) const {
      this->chunks[0].compress(uint16_t(mask), output);
      this->chunks[1].compress(uint16_t(mask >> 16), output + 16 - count_ones(mask & 0xFFFF));
      this->chunks[2].compress(uint16_t(mask >> 32), output + 32 - count_ones(mask & 0xFFFFFFFF));
      this->chunks[3].compress(uint16_t(mask >> 48), output + 48 - count_ones(mask & 0xFFFFFFFFFFFF));
    }

    simdjson_really_inline uint64_t to_bitmask() const {
      uint64_t r0 = uint32_t(this->chunks[0].to_bitmask() );
      uint64_t r1 =          this->chunks[1].to_bitmask() ;
      uint64_t r2 =          this->chunks[2].to_bitmask() ;
      uint64_t r3 =          this->chunks[3].to_bitmask() ;
      return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
    }

    simdjson_really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] == mask,
        this->chunks[1] == mask,
        this->chunks[2] == mask,
        this->chunks[3] == mask
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t eq(const simd8x64<uint8_t> &other) const {
      return  simd8x64<bool>(
        this->chunks[0] == other.chunks[0],
        this->chunks[1] == other.chunks[1],
        this->chunks[2] == other.chunks[2],
        this->chunks[3] == other.chunks[3]
      ).to_bitmask();
    }

    simdjson_really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] <= mask,
        this->chunks[1] <= mask,
        this->chunks[2] <= mask,
        this->chunks[3] <= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simd
} // unnamed namespace
} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
/* end file include/simdjson/westmere/simd.h */
/* begin file include/simdjson/generic/jsoncharutils.h */

namespace simdjson {
namespace westmere {
namespace {
namespace jsoncharutils {

// return non-zero if not a structural or whitespace char
// zero otherwise
simdjson_really_inline uint32_t is_not_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace_negated[c];
}

simdjson_really_inline uint32_t is_structural_or_whitespace(uint8_t c) {
  return internal::structural_or_whitespace[c];
}

// returns a value with the high 16 bits set if not valid
// otherwise returns the conversion of the 4 hex digits at src into the bottom
// 16 bits of the 32-bit return register
//
// see
// https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
static inline uint32_t hex_to_u32_nocheck(
    const uint8_t *src) { // strictly speaking, static inline is a C-ism
  uint32_t v1 = internal::digit_to_val32[630 + src[0]];
  uint32_t v2 = internal::digit_to_val32[420 + src[1]];
  uint32_t v3 = internal::digit_to_val32[210 + src[2]];
  uint32_t v4 = internal::digit_to_val32[0 + src[3]];
  return v1 | v2 | v3 | v4;
}

// given a code point cp, writes to c
// the utf-8 code, outputting the length in
// bytes, if the length is zero, the code point
// is invalid
//
// This can possibly be made faster using pdep
// and clz and table lookups, but JSON documents
// have few escaped code points, and the following
// function looks cheap.
//
// Note: we assume that surrogates are treated separately
//
simdjson_really_inline size_t codepoint_to_utf8(uint32_t cp, uint8_t *c) {
  if (cp <= 0x7F) {
    c[0] = uint8_t(cp);
    return 1; // ascii
  }
  if (cp <= 0x7FF) {
    c[0] = uint8_t((cp >> 6) + 192);
    c[1] = uint8_t((cp & 63) + 128);
    return 2; // universal plane
    //  Surrogates are treated elsewhere...
    //} //else if (0xd800 <= cp && cp <= 0xdfff) {
    //  return 0; // surrogates // could put assert here
  } else if (cp <= 0xFFFF) {
    c[0] = uint8_t((cp >> 12) + 224);
    c[1] = uint8_t(((cp >> 6) & 63) + 128);
    c[2] = uint8_t((cp & 63) + 128);
    return 3;
  } else if (cp <= 0x10FFFF) { // if you know you have a valid code point, this
                               // is not needed
    c[0] = uint8_t((cp >> 18) + 240);
    c[1] = uint8_t(((cp >> 12) & 63) + 128);
    c[2] = uint8_t(((cp >> 6) & 63) + 128);
    c[3] = uint8_t((cp & 63) + 128);
    return 4;
  }
  // will return 0 when the code point was too large.
  return 0; // bad r
}

#ifdef SIMDJSON_IS_32BITS // _umul128 for x86, arm
// this is a slow emulation routine for 32-bit
//
static simdjson_really_inline uint64_t __emulu(uint32_t x, uint32_t y) {
  return x * (uint64_t)y;
}
static simdjson_really_inline uint64_t _umul128(uint64_t ab, uint64_t cd, uint64_t *hi) {
  uint64_t ad = __emulu((uint32_t)(ab >> 32), (uint32_t)cd);
  uint64_t bd = __emulu((uint32_t)ab, (uint32_t)cd);
  uint64_t adbc = ad + __emulu((uint32_t)ab, (uint32_t)(cd >> 32));
  uint64_t adbc_carry = !!(adbc < ad);
  uint64_t lo = bd + (adbc << 32);
  *hi = __emulu((uint32_t)(ab >> 32), (uint32_t)(cd >> 32)) + (adbc >> 32) +
        (adbc_carry << 32) + !!(lo < bd);
  return lo;
}
#endif

using internal::value128;

simdjson_really_inline value128 full_multiplication(uint64_t value1, uint64_t value2) {
  value128 answer;
#if defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
#ifdef _M_ARM64
  // ARM64 has native support for 64-bit multiplications, no need to emultate
  answer.high = __umulh(value1, value2);
  answer.low = value1 * value2;
#else
  answer.low = _umul128(value1, value2, &answer.high); // _umul128 not available on ARM64
#endif // _M_ARM64
#else // defined(SIMDJSON_REGULAR_VISUAL_STUDIO) || defined(SIMDJSON_IS_32BITS)
  __uint128_t r = (static_cast<__uint128_t>(value1)) * value2;
  answer.low = uint64_t(r);
  answer.high = uint64_t(r >> 64);
#endif
  return answer;
}

} // namespace jsoncharutils
} // unnamed namespace
} // namespace westmere
} // namespace simdjson
/* end file include/simdjson/generic/jsoncharutils.h */
/* begin file include/simdjson/generic/atomparsing.h */
namespace simdjson {
namespace westmere {
namespace {
/// @private
namespace atomparsing {

// The string_to_uint32 is exclusively used to map literal strings to 32-bit values.
// We use memcpy instead of a pointer cast to avoid undefined behaviors since we cannot
// be certain that the character pointer will be properly aligned.
// You might think that using memcpy makes this function expensive, but you'd be wrong.
// All decent optimizing compilers (GCC, clang, Visual Studio) will compile string_to_uint32("false");
// to the compile-time constant 1936482662.
simdjson_really_inline uint32_t string_to_uint32(const char* str) { uint32_t val; std::memcpy(&val, str, sizeof(uint32_t)); return val; }


// Again in str4ncmp we use a memcpy to avoid undefined behavior. The memcpy may appear expensive.
// Yet all decent optimizing compilers will compile memcpy to a single instruction, just about.
simdjson_warn_unused
simdjson_really_inline uint32_t str4ncmp(const uint8_t *src, const char* atom) {
  uint32_t srcval; // we want to avoid unaligned 32-bit loads (undefined in C/C++)
  static_assert(sizeof(uint32_t) <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be larger than 4 bytes");
  std::memcpy(&srcval, src, sizeof(uint32_t));
  return srcval ^ string_to_uint32(atom);
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src) {
  return (str4ncmp(src, "true") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_true_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_true_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "true"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src) {
  return (str4ncmp(src+1, "alse") | jsoncharutils::is_not_structural_or_whitespace(src[5])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_false_atom(const uint8_t *src, size_t len) {
  if (len > 5) { return is_valid_false_atom(src); }
  else if (len == 5) { return !str4ncmp(src+1, "alse"); }
  else { return false; }
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src) {
  return (str4ncmp(src, "null") | jsoncharutils::is_not_structural_or_whitespace(src[4])) == 0;
}

simdjson_warn_unused
simdjson_really_inline bool is_valid_null_atom(const uint8_t *src, size_t len) {
  if (len > 4) { return is_valid_null_atom(src); }
  else if (len == 4) { return !str4ncmp(src, "null"); }
  else { return false; }
}

} // namespace atomparsing
} // unnamed namespace
} // namespace westmere
} // namespace simdjson
/* end file include/simdjson/generic/atomparsing.h */
/* begin file include/simdjson/westmere/stringparsing.h */
#ifndef SIMDJSON_WESTMERE_STRINGPARSING_H
#define SIMDJSON_WESTMERE_STRINGPARSING_H

namespace simdjson {
namespace westmere {
namespace {

using namespace simd;

// Holds backslashes and quotes locations.
struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_really_inline static backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_really_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_really_inline bool has_backslash() { return bs_bits != 0; }
  simdjson_really_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_really_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_really_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  // this can read up to 31 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  simd8<uint8_t> v0(src);
  simd8<uint8_t> v1(src + 16);
  v0.store(dst);
  v1.store(dst + 16);
  uint64_t bs_and_quote = simd8x64<bool>(v0 == '\\', v1 == '\\', v0 == '"', v1 == '"').to_bitmask();
  return {
    uint32_t(bs_and_quote),      // bs_bits
    uint32_t(bs_and_quote >> 32) // quote_bits
  };
}

} // unnamed namespace
} // namespace westmere
} // namespace simdjson

/* begin file include/simdjson/generic/stringparsing.h */
// This file contains the common code every implementation uses
// It is intended to be included multiple times and compiled multiple times

namespace simdjson {
namespace westmere {
namespace {
/// @private
namespace stringparsing {

// begin copypasta
// These chars yield themselves: " \ /
// b -> backspace, f -> formfeed, n -> newline, r -> cr, t -> horizontal tab
// u not handled in this table as it's complex
static const uint8_t escape_map[256] = {
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x0.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0x22, 0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0x2f,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x4.
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0x5c, 0, 0,    0, // 0x5.
    0, 0, 0x08, 0, 0,    0, 0x0c, 0, 0, 0, 0, 0, 0,    0, 0x0a, 0, // 0x6.
    0, 0, 0x0d, 0, 0x09, 0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0, // 0x7.

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,

    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
    0, 0, 0,    0, 0,    0, 0,    0, 0, 0, 0, 0, 0,    0, 0,    0,
};

// handle a unicode codepoint
// write appropriate values into dest
// src will advance 6 bytes or 12 bytes
// dest will advance a variable amount (return via pointer)
// return true if the unicode codepoint was valid
// We work in little-endian then swap at write time
simdjson_warn_unused
simdjson_really_inline bool handle_unicode_codepoint(const uint8_t **src_ptr,
                                            uint8_t **dst_ptr) {
  // jsoncharutils::hex_to_u32_nocheck fills high 16 bits of the return value with 1s if the
  // conversion isn't valid; we defer the check for this to inside the
  // multilingual plane check
  uint32_t code_point = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);
  *src_ptr += 6;
  // check for low surrogate for characters outside the Basic
  // Multilingual Plane.
  if (code_point >= 0xd800 && code_point < 0xdc00) {
    if (((*src_ptr)[0] != '\\') || (*src_ptr)[1] != 'u') {
      return false;
    }
    uint32_t code_point_2 = jsoncharutils::hex_to_u32_nocheck(*src_ptr + 2);

    // if the first code point is invalid we will get here, as we will go past
    // the check for being outside the Basic Multilingual plane. If we don't
    // find a \u immediately afterwards we fail out anyhow, but if we do,
    // this check catches both the case of the first code point being invalid
    // or the second code point being invalid.
    if ((code_point | code_point_2) >> 16) {
      return false;
    }

    code_point =
        (((code_point - 0xd800) << 10) | (code_point_2 - 0xdc00)) + 0x10000;
    *src_ptr += 6;
  }
  size_t offset = jsoncharutils::codepoint_to_utf8(code_point, *dst_ptr);
  *dst_ptr += offset;
  return offset > 0;
}

/**
 * Unescape a string from src to dst, stopping at a final unescaped quote. E.g., if src points at 'joe"', then
 * dst needs to have four free bytes.
 */
simdjson_warn_unused simdjson_really_inline uint8_t *parse_string(const uint8_t *src, uint8_t *dst) {
  while (1) {
    // Copy the next n bytes, and find the backslash and quote in them.
    auto bs_quote = backslash_and_quote::copy_and_find(src, dst);
    // If the next thing is the end quote, copy and return
    if (bs_quote.has_quote_first()) {
      // we encountered quotes first. Move dst to point to quotes and exit
      return dst + bs_quote.quote_index();
    }
    if (bs_quote.has_backslash()) {
      /* find out where the backspace is */
      auto bs_dist = bs_quote.backslash_index();
      uint8_t escape_char = src[bs_dist + 1];
      /* we encountered backslash first. Handle backslash */
      if (escape_char == 'u') {
        /* move src/dst up to the start; they will be further adjusted
           within the unicode codepoint handling code. */
        src += bs_dist;
        dst += bs_dist;
        if (!handle_unicode_codepoint(&src, &dst)) {
          return nullptr;
        }
      } else {
        /* simple 1:1 conversion. Will eat bs_dist+2 characters in input and
         * write bs_dist+1 characters to output
         * note this may reach beyond the part of the buffer we've actually
         * seen. I think this is ok */
        uint8_t escape_result = escape_map[escape_char];
        if (escape_result == 0u) {
          return nullptr; /* bogus escape value is an error */
        }
        dst[bs_dist] = escape_result;
        src += bs_dist + 2;
        dst += bs_dist + 1;
      }
    } else {
      /* they are the same. Since they can't co-occur, it means we
       * encountered neither. */
      src += backslash_and_quote::BYTES_PROCESSED;
      dst += backslash_and_quote::BYTES_PROCESSED;
    }
  }
  /* can't be reached */
  return nullptr;
}

simdjson_unused simdjson_warn_unused simdjson_really_inline error_code parse_string_to_buffer(const uint8_t *src, uint8_t *&current_string_buf_loc, std::string_view &s) {
  if (*(src++) != '"') { return STRING_ERROR; }
  auto end = stringparsing::parse_string(src, current_string_buf_loc);
  if (!end) { return STRING_ERROR; }
  s = std::string_view(reinterpret_cast<const char *>(current_string_buf_loc), end-current_string_buf_loc);
  current_string_buf_loc = end;
  return SUCCESS;
}

} // namespace stringparsing
} // unnamed namespace
} // namespace westmere
} // namespace simdjson
/* end file include/simdjson/generic/stringparsing.h */

#endif // SIMDJSON_WESTMERE_STRINGPARSING_H
/* end file include/simdjson/westmere/stringparsing.h */
/* begin file include/simdjson/westmere/numberparsing.h */
#ifndef SIMDJSON_WESTMERE_NUMBERPARSING_H
#define SIMDJSON_WESTMERE_NUMBERPARSING_H

namespace simdjson {
namespace westmere {
namespace {

static simdjson_really_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars) {
  // this actually computes *16* values so we are being wasteful.
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  const __m128i mul_1_10000 =
      _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);
  const __m128i input = _mm_sub_epi8(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(chars)), ascii0);
  const __m128i t1 = _mm_maddubs_epi16(input, mul_1_10);
  const __m128i t2 = _mm_madd_epi16(t1, mul_1_100);
  const __m128i t3 = _mm_packus_epi32(t2, t2);
  const __m128i t4 = _mm_madd_epi16(t3, mul_1_10000);
  return _mm_cvtsi128_si32(
      t4); // only captures the sum of the first 8 digits, drop the rest
}

} // unnamed namespace
} // namespace westmere
} // namespace simdjson

#define SWAR_NUMBER_PARSING

/* begin file include/simdjson/generic/numberparsing.h */
#include <limits>

namespace simdjson {
namespace westmere {
namespace {
/// @private
namespace numberparsing {



#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

namespace {
// Convert a mantissa, an exponent and a sign bit into an ieee64 double.
// The real_exponent needs to be in [0, 2046] (technically real_exponent = 2047 would be acceptable).
// The mantissa should be in [0,1<<53). The bit at index (1ULL << 52) while be zeroed.
simdjson_really_inline double to_double(uint64_t mantissa, uint64_t real_exponent, bool negative) {
    double d;
    mantissa &= ~(1ULL << 52);
    mantissa |= real_exponent << 52;
    mantissa |= ((static_cast<uint64_t>(negative)) << 63);
    std::memcpy(&d, &mantissa, sizeof(d));
    return d;
}
}
// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [smallest_power,
// largest_power] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / simdjson::internal::power_of_ten[-power];
    } else {
      d = d * simdjson::internal::power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It was
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }


  // The exponent is 1024 + 63 + power
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power when power >= 0
  // and it is equal to
  //  ceil(log(5**-power)/log(2)) + power when power < 0
  //
  // The 65536 is (1<<16) and corresponds to
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to
  // floor(log(5**power)/log(2))
  //
  // Note that this is not magic: 152170/(1<<16) is
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;


  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;


  // We are going to need to do some 64-bit arithmetic to get a precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= smallest_power
  // and power <= largest_power
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  //
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  const uint32_t index = 2 * uint32_t(power - simdjson::internal::smallest_power);
  // Optimization: It may be that materializing the index as a variable might confuse some compilers and prevent effective complex-addressing loads. (Done for code clarity.)
  //
  // The full_multiplication function computes the 128-bit product of two 64-bit words
  // with a returned value of type value128 with a "low component" corresponding to the
  // 64-bit least significant bits of the product and with a "high component" corresponding
  // to the 64-bit most significant bits of the product.
  simdjson::internal::value128 firstproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index]);
  // Both i and power_of_five_128[index] have their most significant bit set to 1 which
  // implies that the either the most or the second most significant bit of the product
  // is 1. We pack values in this manner for efficiency reasons: it maximizes the use
  // we make of the product. It also makes it easy to reason aboutthe product: there
  // 0 or 1 leading zero in the product.

  // Unless the least significant 9 bits of the high (64-bit) part of the full
  // product are all 1s, then we know that the most significant 55 bits are
  // exact and no further work is needed. Having 55 bits is necessary because
  // we need 53 bits for the mantissa but we have to have one rounding bit and
  // we can waste a bit if the most significant bit of the product is zero.
  if((firstproduct.high & 0x1FF) == 0x1FF) {
    // We want to compute i * 5^q, but only care about the top 55 bits at most.
    // Consider the scenario where q>=0. Then 5^q may not fit in 64-bits. Doing
    // the full computation is wasteful. So we do what is called a "truncated
    // multiplication".
    // We take the most significant 64-bits, and we put them in
    // power_of_five_128[index]. Usually, that's good enough to approximate i * 5^q
    // to the desired approximation using one multiplication. Sometimes it does not suffice.
    // Then we store the next most significant 64 bits in power_of_five_128[index + 1], and
    // then we get a better approximation to i * 5^q. In very rare cases, even that
    // will not suffice, though it is seemingly very hard to find such a scenario.
    //
    // That's for when q>=0. The logic for q<0 is somewhat similar but it is somewhat
    // more complicated.
    //
    // There is an extra layer of complexity in that we need more than 55 bits of
    // accuracy in the round-to-even scenario.
    //
    // The full_multiplication function computes the 128-bit product of two 64-bit words
    // with a returned value of type value128 with a "low component" corresponding to the
    // 64-bit least significant bits of the product and with a "high component" corresponding
    // to the 64-bit most significant bits of the product.
    simdjson::internal::value128 secondproduct = jsoncharutils::full_multiplication(i, simdjson::internal::power_of_five_128[index + 1]);
    firstproduct.low += secondproduct.high;
    if(secondproduct.high > firstproduct.low) { firstproduct.high++; }
    // At this point, we might need to add at most one to firstproduct, but this
    // can only change the value of firstproduct.high if firstproduct.low is maximal.
    if(simdjson_unlikely(firstproduct.low  == 0xFFFFFFFFFFFFFFFF)) {
      // This is very unlikely, but if so, we need to do much more work!
      return false;
    }
  }
  uint64_t lower = firstproduct.low;
  uint64_t upper = firstproduct.high;
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).
  int64_t real_exponent = exponent - lz;
  if (simdjson_unlikely(real_exponent <= 0)) { // we have a subnormal?
    // Here have that real_exponent <= 0 so -real_exponent >= 0
    if(-real_exponent + 1 >= 64) { // if we have more than 64 bits below the minimum exponent, you have a zero for sure.
      d = 0.0;
      return true;
    }
    // next line is safe because -real_exponent + 1 < 0
    mantissa >>= -real_exponent + 1;
    // Thankfully, we can't have both "round-to-even" and subnormals because
    // "round-to-even" only occurs for powers close to 0.
    mantissa += (mantissa & 1); // round up
    mantissa >>= 1;
    // There is a weird scenario where we don't have a subnormal but just.
    // Suppose we start with 2.2250738585072013e-308, we end up
    // with 0x3fffffffffffff x 2^-1023-53 which is technically subnormal
    // whereas 0x40000000000000 x 2^-1023-53  is normal. Now, we need to round
    // up 0x3fffffffffffff x 2^-1023-53  and once we do, we are no longer
    // subnormal, but we can only know this after rounding.
    // So we only declare a subnormal if we are smaller than the threshold.
    real_exponent = (mantissa < (uint64_t(1) << 52)) ? 0 : 1;
    d = to_double(mantissa, real_exponent, negative);
    return true;
  }
  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  //
  // The round-to-even cases take the form of a number 2m+1 which is in (2^53,2^54]
  // times a power of two. That is, it is right between a number with binary significand
  // m and another number with binary significand m+1; and it must be the case
  // that it cannot be represented by a float itself.
  //
  // We must have that w * 10 ^q == (2m+1) * 2^p for some power of two 2^p.
  // Recall that 10^q = 5^q * 2^q.
  // When q >= 0, we must have that (2m+1) is divible by 5^q, so 5^q <= 2^54. We have that
  //  5^23 <=  2^54 and it is the last power of five to qualify, so q <= 23.
  // When q<0, we have  w  >=  (2m+1) x 5^{-q}.  We must have that w<2^{64} so
  // (2m+1) x 5^{-q} < 2^{64}. We have that 2m+1>2^{53}. Hence, we must have
  // 2^{53} x 5^{-q} < 2^{64}.
  // Hence we have 5^{-q} < 2^{11}$ or q>= -4.
  //
  // We require lower <= 1 and not lower == 0 because we could not prove that
  // that lower == 0 is implied; but we could prove that lower <= 1 is a necessary and sufficient test.
  if (simdjson_unlikely((lower <= 1) && (power >= -4) && (power <= 23) && ((mantissa & 3) == 1))) {
    if((mantissa  << (upperbit + 64 - 53 - 2)) ==  upper) {
      mantissa &= ~1;             // flip it so that we do not round up
    }
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    real_exponent++;
  }
  mantissa &= ~(1ULL << 52);
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely(real_exponent > 2046)) {
    // We have an infinte value!!! We could actually throw an error here if we could.
    return false;
  }
  d = to_double(mantissa, real_exponent, negative);
  return true;
}

// We call a fallback floating-point parser that might be slow. Note
// it will accept JSON numbers, but the JSON spec. is more restrictive so
// before you call parse_float_fallback, you need to have validated the input
// string with the JSON grammar.
// It will return an error (false) if the parsed number is infinite.
// The string parsing itself always succeeds. We know that there is at least
// one digit.
static bool parse_float_fallback(const uint8_t *ptr, double *outDouble) {
  *outDouble = simdjson::internal::from_chars(reinterpret_cast<const char *>(ptr));
  // We do not accept infinite values.

  // Detecting finite values in a portable manner is ridiculously hard, ideally
  // we would want to do:
  // return !std::isfinite(*outDouble);
  // but that mysteriously fails under legacy/old libc++ libraries, see
  // https://github.com/simdjson/simdjson/issues/1286
  //
  // Therefore, fall back to this solution (the extra parens are there
  // to handle that max may be a macro on windows).
  return !(*outDouble > (std::numeric_limits<double>::max)() || *outDouble < std::numeric_limits<double>::lowest());
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  std::memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(simdjson_unused const uint8_t * src, W writer) {
  double d;
  if (parse_float_fallback(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(simdjson_unused const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(simdjson_unused const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow.
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time.
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline size_t significant_digits(const uint8_t * start_digits, size_t digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - size_t(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, size_t digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  //
  // 9999999999999999999 < 2**64 so we can accomodate 19 digits.
  // If we have a decimal separator, then digit_count - 1 is the number of digits, but we
  // may not have a decimal separator!
  if (simdjson_unlikely(digit_count > 19 && significant_digits(start_digits, digit_count) > 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < simdjson::internal::smallest_power) || (exponent > simdjson::internal::largest_power)) {
    //
    // Important: smallest_power is such that it leads to a zero value.
    // Observe that 18446744073709551615e-343 == 0, i.e. (2**64 - 1) e -343 is zero
    // so something x 10^-343 goes to zero, but not so with  something x 10^-342.
    static_assert(simdjson::internal::smallest_power <= -342, "smallest_power is not small enough");
    //
    if((exponent < simdjson::internal::smallest_power) || (i == 0)) {
      WRITE_DOUBLE(0, src, writer);
      return SUCCESS;
    } else { // (exponent > largest_power) and (i != 0)
      // We have, for sure, an infinite value and simdjson refuses to parse infinite values.
      return INVALID_NUMBER(src);
    }
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_fallback(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t * const src) noexcept { return 0; }
simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * const src) noexcept { return 0; }

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool dirty_end = jsoncharutils::is_not_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (dirty_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// Inlineable functions
namespace {

// This table can be used to characterize the final character of an integer
// string. For JSON structural character and allowable white space characters,
// we return SUCCESS. For 'e', '.' and 'E', we return INCORRECT_TYPE. Otherwise
// we return NUMBER_ERROR.
// Optimization note: we could easily reduce the size of the table by half (to 128)
// at the cost of an extra branch.
// Optimization note: we want the values to use at most 8 bits (not, e.g., 32 bits):
static_assert(error_code(uint8_t(NUMBER_ERROR))== NUMBER_ERROR, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(SUCCESS))== SUCCESS, "bad NUMBER_ERROR cast");
static_assert(error_code(uint8_t(INCORRECT_TYPE))== INCORRECT_TYPE, "bad NUMBER_ERROR cast");

const uint8_t integer_string_finisher[256] = {
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   SUCCESS,      NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, SUCCESS,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, INCORRECT_TYPE,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, SUCCESS,        NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, INCORRECT_TYPE, NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, SUCCESS,      NUMBER_ERROR,
    SUCCESS,      NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR, NUMBER_ERROR,   NUMBER_ERROR, NUMBER_ERROR, NUMBER_ERROR,
    NUMBER_ERROR};

// Parse any number from 0 to 18,446,744,073,709,551,615
simdjson_unused simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;
  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > 20))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > 20)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if (integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }

  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
simdjson_unused simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  // Optimization note: size_t is expected to be unsigned.
  size_t digit_count = size_t(p - start_digits);
  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  size_t longest_digit_count = negative ? 19 : 20;
  // Optimization note: the compiler can probably merge
  // ((digit_count == 0) || (digit_count > longest_digit_count))
  // into a single  branch since digit_count is unsigned.
  if ((digit_count == 0) || (digit_count > longest_digit_count)) { return INCORRECT_TYPE; }
  // Here digit_count > 0.
  if (('0' == *start_digits) && (digit_count > 1)) { return NUMBER_ERROR; }
  // We can do the following...
  // if (!jsoncharutils::is_structural_or_whitespace(*p)) {
  //  return (*p == '.' || *p == 'e' || *p == 'E') ? INCORRECT_TYPE : NUMBER_ERROR;
  // }
  // as a single table lookup:
  if(integer_string_finisher[*p] != SUCCESS) { return error_code(integer_string_finisher[*p]); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INCORRECT_TYPE; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INCORRECT_TYPE; }
  }

  return negative ? (~i+1) : i;
}

simdjson_unused simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src ) { return INCORRECT_TYPE; }
  if ( (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. More than 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 > 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src > 19;
    }
  } else {
    overflow = p-src > 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
  }

  if (jsoncharutils::is_not_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  overflow = overflow || exponent < simdjson::internal::smallest_power || exponent > simdjson::internal::largest_power;

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_fallback(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // unnamed namespace
} // namespace westmere
} // namespace simdjson
/* end file include/simdjson/generic/numberparsing.h */

#endif //  SIMDJSON_WESTMERE_NUMBERPARSING_H
/* end file include/simdjson/westmere/numberparsing.h */
/* begin file include/simdjson/westmere/end.h */
SIMDJSON_UNTARGET_WESTMERE
/* end file include/simdjson/westmere/end.h */

#endif // SIMDJSON_IMPLEMENTATION_WESTMERE
#endif // SIMDJSON_WESTMERE_COMMON_H
/* end file include/simdjson/westmere.h */

// Builtin implementation

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_IMPLEMENTATIONS_H
/* end file include/simdjson/implementations.h */

// Determine the best builtin implementation
#ifndef SIMDJSON_BUILTIN_IMPLEMENTATION
#if SIMDJSON_CAN_ALWAYS_RUN_HASWELL
#define SIMDJSON_BUILTIN_IMPLEMENTATION haswell
#elif SIMDJSON_CAN_ALWAYS_RUN_WESTMERE
#define SIMDJSON_BUILTIN_IMPLEMENTATION westmere
#elif SIMDJSON_CAN_ALWAYS_RUN_ARM64
#define SIMDJSON_BUILTIN_IMPLEMENTATION arm64
#elif SIMDJSON_CAN_ALWAYS_RUN_PPC64
#define SIMDJSON_BUILTIN_IMPLEMENTATION ppc64
#elif SIMDJSON_CAN_ALWAYS_RUN_FALLBACK
#define SIMDJSON_BUILTIN_IMPLEMENTATION fallback
#else
#error "All possible implementations (including fallback) have been disabled! simdjson will not run."
#endif
#endif // SIMDJSON_BUILTIN_IMPLEMENTATION

// redefining SIMDJSON_IMPLEMENTATION to "SIMDJSON_BUILTIN_IMPLEMENTATION"
// #define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION

// ondemand is only compiled as part of the builtin implementation at present

// Interface declarations
/* begin file include/simdjson/generic/implementation_simdjson_result_base.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {

// This is a near copy of include/error.h's implementation_simdjson_result_base, except it doesn't use std::pair
// so we can avoid inlining errors
// TODO reconcile these!
/**
 * The result of a simdjson operation that could fail.
 *
 * Gives the option of reading error codes, or throwing an exception by casting to the desired result.
 *
 * This is a base class for implementations that want to add functions to the result type for
 * chaining.
 *
 * Override like:
 *
 *   struct simdjson_result<T> : public internal::implementation_simdjson_result_base<T> {
 *     simdjson_result() noexcept : internal::implementation_simdjson_result_base<T>() {}
 *     simdjson_result(error_code error) noexcept : internal::implementation_simdjson_result_base<T>(error) {}
 *     simdjson_result(T &&value) noexcept : internal::implementation_simdjson_result_base<T>(std::forward(value)) {}
 *     simdjson_result(T &&value, error_code error) noexcept : internal::implementation_simdjson_result_base<T>(value, error) {}
 *     // Your extra methods here
 *   }
 *
 * Then any method returning simdjson_result<T> will be chainable with your methods.
 */
template<typename T>
struct implementation_simdjson_result_base {

  /**
   * Create a new empty result with error = UNINITIALIZED.
   */
  simdjson_really_inline implementation_simdjson_result_base() noexcept = default;

  /**
   * Create a new error result.
   */
  simdjson_really_inline implementation_simdjson_result_base(error_code error) noexcept;

  /**
   * Create a new successful result.
   */
  simdjson_really_inline implementation_simdjson_result_base(T &&value) noexcept;

  /**
   * Create a new result with both things (use if you don't want to branch when creating the result).
   */
  simdjson_really_inline implementation_simdjson_result_base(T &&value, error_code error) noexcept;

  /**
   * Move the value and the error to the provided variables.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   * @param error The variable to assign the error to. Set to SUCCESS if there is no error.
   */
  simdjson_really_inline void tie(T &value, error_code &error) && noexcept;

  /**
   * Move the value to the provided variable.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   */
  simdjson_really_inline error_code get(T &value) && noexcept;

  /**
   * The error.
   */
  simdjson_really_inline error_code error() const noexcept;

#if SIMDJSON_EXCEPTIONS

  /**
   * Get the result value.
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T& value() & noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T&& value() && noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline T&& take_value() && noexcept(false);

  /**
   * Cast to the value (will throw on error).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_really_inline operator T&&() && noexcept(false);

  /**
   * Get the result value. This function is safe if and only
   * the error() method returns a value that evoluates to false.
   */
  simdjson_really_inline const T& value_unsafe() const& noexcept;

  /**
   * Take the result value (move it). This function is safe if and only
   * the error() method returns a value that evoluates to false.
   */
  simdjson_really_inline T&& value_unsafe() && noexcept;

#endif // SIMDJSON_EXCEPTIONS

  T first{};
  error_code second{UNINITIALIZED};
}; // struct implementation_simdjson_result_base

} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson
/* end file include/simdjson/generic/implementation_simdjson_result_base.h */
/* begin file include/simdjson/generic/ondemand.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
/**
 * A fast, simple, DOM-like interface that parses JSON as you use it.
 *
 * Designed for maximum speed and a lower memory profile.
 */
namespace ondemand {

/** Represents the depth of a JSON value (number of nested arrays/objects). */
using depth_t = int32_t;

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

/* begin file include/simdjson/generic/ondemand/json_type.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

/**
 * The type of a JSON value.
 */
enum class json_type {
    // Start at 1 to catch uninitialized / default values more easily
    array=1, ///< A JSON array   ( [ 1, 2, 3 ... ] )
    object,  ///< A JSON object  ( { "a": 1, "b" 2, ... } )
    number,  ///< A JSON number  ( 1 or -2.3 or 4.5e6 ...)
    string,  ///< A JSON string  ( "a" or "hello world\n" ...)
    boolean, ///< A JSON boolean (true or false)
    null     ///< A JSON null    (null)
};

/**
 * Write the JSON type to the output stream
 *
 * @param out The output stream.
 * @param type The json_type.
 */
inline std::ostream& operator<<(std::ostream& out, json_type type) noexcept;

#if SIMDJSON_EXCEPTIONS
/**
 * Send JSON type to an output stream.
 *
 * @param out The output stream.
 * @param type The json_type.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, simdjson_result<json_type> &type) noexcept(false);
#endif

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/json_type.h */
/* begin file include/simdjson/generic/ondemand/token_position.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

/** @private Position in the JSON buffer indexes */
using token_position = const uint32_t *;

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson
/* end file include/simdjson/generic/ondemand/token_position.h */
/* begin file include/simdjson/generic/ondemand/logger.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class json_iterator;
class value_iterator;

namespace logger {

#if SIMDJSON_VERBOSE_LOGGING
  static constexpr const bool LOG_ENABLED = true;
#else
  static constexpr const bool LOG_ENABLED = false;
#endif

static simdjson_really_inline void log_headers() noexcept;
static simdjson_really_inline void log_line(const json_iterator &iter, token_position index, depth_t depth, const char *title_prefix, const char *title, std::string_view detail) noexcept;
static simdjson_really_inline void log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept;
static simdjson_really_inline void log_event(const json_iterator &iter, const char *type, std::string_view detail="", int delta=0, int depth_delta=0) noexcept;
static simdjson_really_inline void log_value(const json_iterator &iter, token_position index, depth_t depth, const char *type, std::string_view detail="") noexcept;
static simdjson_really_inline void log_value(const json_iterator &iter, const char *type, std::string_view detail="", int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_start_value(const json_iterator &iter, token_position index, depth_t depth, const char *type, std::string_view detail="") noexcept;
static simdjson_really_inline void log_start_value(const json_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_end_value(const json_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_error(const json_iterator &iter, token_position index, depth_t depth, const char *error, const char *detail="") noexcept;
static simdjson_really_inline void log_error(const json_iterator &iter, const char *error, const char *detail="", int delta=-1, int depth_delta=0) noexcept;

static simdjson_really_inline void log_event(const value_iterator &iter, const char *type, std::string_view detail="", int delta=0, int depth_delta=0) noexcept;
static simdjson_really_inline void log_value(const value_iterator &iter, const char *type, std::string_view detail="", int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_start_value(const value_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_end_value(const value_iterator &iter, const char *type, int delta=-1, int depth_delta=0) noexcept;
static simdjson_really_inline void log_error(const value_iterator &iter, const char *error, const char *detail="", int delta=-1, int depth_delta=0) noexcept;

} // namespace logger
} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson
/* end file include/simdjson/generic/ondemand/logger.h */
/* begin file include/simdjson/generic/ondemand/raw_json_string.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class object;
class parser;
class json_iterator;

/**
 * A string escaped per JSON rules, terminated with quote ("). They are used to represent
 * unescaped keys inside JSON documents.
 *
 * (In other words, a pointer to the beginning of a string, just after the start quote, inside a
 * JSON file.)
 *
 * This class is deliberately simplistic and has little functionality. You can
 * compare a raw_json_string instance with an unescaped C string, but
 * that is pretty much all you can do.
 *
 * They originate typically from field instance which in turn represent key-value pairs from
 * object instances. From a field instance, you get the raw_json_string instance by calling key().
 * You can, if you want a more usable string_view instance, call the unescaped_key() method
 * on the field instance.
 */
class raw_json_string {
public:
  /**
   * Create a new invalid raw_json_string.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline raw_json_string() noexcept = default;

  /**
   * Create a new invalid raw_json_string pointed at the given location in the JSON.
   *
   * The given location must be just *after* the beginning quote (") in the JSON file.
   *
   * It *must* be terminated by a ", and be a valid JSON string.
   */
  simdjson_really_inline raw_json_string(const uint8_t * _buf) noexcept;
  /**
   * Get the raw pointer to the beginning of the string in the JSON (just after the ").
   *
   * It is possible for this function to return a null pointer if the instance
   * has outlived its existence.
   */
  simdjson_really_inline const char * raw() const noexcept;

  /**
   * This compares the current instance to the std::string_view target: returns true if
   * they are byte-by-byte equal (no escaping is done) on target.size() characters,
   * and if the raw_json_string instance has a quote character at byte index target.size().
   * We never read more than length + 1 bytes in the raw_json_string instance.
   * If length is smaller than target.size(), this will return false.
   *
   * The std::string_view instance may contain any characters. However, the caller
   * is responsible for setting length so that length bytes may be read in the
   * raw_json_string.
   *
   * Performance: the comparison may be done using memcmp which may be efficient
   * for long strings.
   */
  simdjson_really_inline bool unsafe_is_equal(size_t length, std::string_view target) const noexcept;

  /**
   * This compares the current instance to the std::string_view target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   * The std::string_view instance should not contain unescaped quote characters:
   * the caller is responsible for this check. See is_free_from_unescaped_quote.
   *
   * Performance: the comparison is done byte-by-byte which might be inefficient for
   * long strings.
   *
   * If target is a compile-time constant, and your compiler likes you,
   * you should be able to do the following without performance penatly...
   *
   *   static_assert(raw_json_string::is_free_from_unescaped_quote(target), "");
   *   s.unsafe_is_equal(target);
   */
  simdjson_really_inline bool unsafe_is_equal(std::string_view target) const noexcept;

  /**
   * This compares the current instance to the C string target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   * The provided C string should not contain an unescaped quote character:
   * the caller is responsible for this check. See is_free_from_unescaped_quote.
   *
   * If target is a compile-time constant, and your compiler likes you,
   * you should be able to do the following without performance penatly...
   *
   *   static_assert(raw_json_string::is_free_from_unescaped_quote(target), "");
   *   s.unsafe_is_equal(target);
   */
  simdjson_really_inline bool unsafe_is_equal(const char* target) const noexcept;

  /**
   * This compares the current instance to the std::string_view target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   */
  simdjson_really_inline bool is_equal(std::string_view target) const noexcept;

  /**
   * This compares the current instance to the C string target: returns true if
   * they are byte-by-byte equal (no escaping is done).
   */
  simdjson_really_inline bool is_equal(const char* target) const noexcept;

  /**
   * Returns true if target is free from unescaped quote. If target is known at
   * compile-time, we might expect the computation to happen at compile time with
   * many compilers (not all!).
   */
  static simdjson_really_inline bool is_free_from_unescaped_quote(std::string_view target) noexcept;
  static simdjson_really_inline bool is_free_from_unescaped_quote(const char* target) noexcept;

private:


  /**
   * This will set the inner pointer to zero, effectively making
   * this instance unusable.
   */
  simdjson_really_inline void consume() noexcept { buf = nullptr; }

  /**
   * Checks whether the inner pointer is non-null and thus usable.
   */
  simdjson_really_inline simdjson_warn_unused bool alive() const noexcept { return buf != nullptr; }

  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc.
   *
   * ## IMPORTANT: string_view lifetime
   *
   * The string_view is only valid as long as the bytes in dst.
   *
   * @param dst A pointer to a buffer at least large enough to write this string as well as a \0.
   *            dst will be updated to the next unused location (just after the \0 written out at
   *            the end of this string).
   * @return A string_view pointing at the unescaped string in dst
   * @error STRING_ERROR if escapes are incorrect.
   */
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(uint8_t *&dst) const noexcept;
  /**
   * Unescape this JSON string, replacing \\ with \, \n with newline, etc.
   *
   * ## IMPORTANT: string_view lifetime
   *
   * The string_view is only valid until the next parse() call on the parser.
   *
   * @param iter A json_iterator, which contains a buffer where the string will be written.
   */
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(json_iterator &iter) const noexcept;

  const uint8_t * buf{};
  friend class object;
  friend class field;
  friend struct simdjson_result<raw_json_string>;
};

simdjson_unused simdjson_really_inline std::ostream &operator<<(std::ostream &, const raw_json_string &) noexcept;

/**
 * Comparisons between raw_json_string and std::string_view instances are potentially unsafe: the user is responsible
 * for providing a string with no unescaped quote. Note that unescaped quotes cannot be present in valid JSON strings.
 */
simdjson_unused simdjson_really_inline bool operator==(const raw_json_string &a, std::string_view c) noexcept;
simdjson_unused simdjson_really_inline bool operator==(std::string_view c, const raw_json_string &a) noexcept;
simdjson_unused simdjson_really_inline bool operator!=(const raw_json_string &a, std::string_view c) noexcept;
simdjson_unused simdjson_really_inline bool operator!=(std::string_view c, const raw_json_string &a) noexcept;


} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private

  simdjson_really_inline simdjson_result<const char *> raw() const noexcept;
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(uint8_t *&dst) const noexcept;
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescape(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator &iter) const noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/raw_json_string.h */
/* begin file include/simdjson/generic/ondemand/token_iterator.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

/**
 * Iterates through JSON tokens (`{` `}` `[` `]` `,` `:` `"<string>"` `123` `true` `false` `null`)
 * detected by stage 1.
 *
 * @private This is not intended for external use.
 */
class token_iterator {
public:
  /**
   * Create a new invalid token_iterator.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline token_iterator() noexcept = default;
  simdjson_really_inline token_iterator(token_iterator &&other) noexcept = default;
  simdjson_really_inline token_iterator &operator=(token_iterator &&other) noexcept = default;
  simdjson_really_inline token_iterator(const token_iterator &other) noexcept = default;
  simdjson_really_inline token_iterator &operator=(const token_iterator &other) noexcept = default;

  /**
   * Advance to the next token (returning the current one).
   *
   * Does not check or update depth/expect_value. Caller is responsible for that.
   */
  simdjson_really_inline const uint8_t *advance() noexcept;

  /**
   * Get the JSON text for a given token (relative).
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param delta The relative position of the token to retrieve. e.g. 0 = current token,
   *              1 = next token, -1 = prev token.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_really_inline const uint8_t *peek(int32_t delta=0) const noexcept;
  /**
   * Get the maximum length of the JSON text for a given token.
   *
   * The length will include any whitespace at the end of the token.
   *
   * @param delta The relative position of the token to retrieve. e.g. 0 = current token,
   *              1 = next token, -1 = prev token.
   */
  simdjson_really_inline uint32_t peek_length(int32_t delta=0) const noexcept;

  /**
   * Get the JSON text for a given token.
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param position The position of the token.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_really_inline const uint8_t *peek(token_position position) const noexcept;
  /**
   * Get the maximum length of the JSON text for a given token.
   *
   * The length will include any whitespace at the end of the token.
   *
   * @param position The position of the token.
   */
  simdjson_really_inline uint32_t peek_length(token_position position) const noexcept;

  /**
   * Save the current index to be restored later.
   */
  simdjson_really_inline token_position position() const noexcept;
  /**
   * Reset to a previously saved index.
   */
  simdjson_really_inline void set_position(token_position target_checkpoint) noexcept;

  // NOTE: we don't support a full C++ iterator interface, because we expect people to make
  // different calls to advance the iterator based on *their own* state.

  simdjson_really_inline bool operator==(const token_iterator &other) const noexcept;
  simdjson_really_inline bool operator!=(const token_iterator &other) const noexcept;
  simdjson_really_inline bool operator>(const token_iterator &other) const noexcept;
  simdjson_really_inline bool operator>=(const token_iterator &other) const noexcept;
  simdjson_really_inline bool operator<(const token_iterator &other) const noexcept;
  simdjson_really_inline bool operator<=(const token_iterator &other) const noexcept;

protected:
  simdjson_really_inline token_iterator(const uint8_t *buf, token_position index) noexcept;

  /**
   * Get the index of the JSON text for a given token (relative).
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param delta The relative position of the token to retrieve. e.g. 0 = current token,
   *              1 = next token, -1 = prev token.
   */
  simdjson_really_inline uint32_t peek_index(int32_t delta=0) const noexcept;
  /**
   * Get the index of the JSON text for a given token.
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param position The position of the token.
   *
   */
  simdjson_really_inline uint32_t peek_index(token_position position) const noexcept;

  const uint8_t *buf{};
  token_position index{};

  friend class json_iterator;
  friend class value_iterator;
  friend class object;
  friend simdjson_really_inline void logger::log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept;
  friend simdjson_really_inline void logger::log_line(const json_iterator &iter, token_position index, depth_t depth, const char *title_prefix, const char *title, std::string_view detail) noexcept;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;
  simdjson_really_inline ~simdjson_result() noexcept = default; ///< @private
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/token_iterator.h */
/* begin file include/simdjson/generic/ondemand/json_iterator.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class document;
class object;
class array;
class value;
class raw_json_string;
class parser;

/**
 * Iterates through JSON tokens, keeping track of depth and string buffer.
 *
 * @private This is not intended for external use.
 */
class json_iterator {
protected:
  token_iterator token{};
  ondemand::parser *parser{};
  /**
   * Next free location in the string buffer.
   *
   * Used by raw_json_string::unescape() to have a place to unescape strings to.
   */
  uint8_t *_string_buf_loc{};
  /**
   * JSON error, if there is one.
   *
   * INCORRECT_TYPE and NO_SUCH_FIELD are *not* stored here, ever.
   *
   * PERF NOTE: we *hope* this will be elided into control flow, as it is only used (a) in the first
   * iteration of the loop, or (b) for the final iteration after a missing comma is found in ++. If
   * this is not elided, we should make sure it's at least not using up a register. Failing that,
   * we should store it in document so there's only one of them.
   */
  error_code error{SUCCESS};
  /**
   * Depth of the current token in the JSON.
   *
   * - 0 = finished with document
   * - 1 = document root value (could be [ or {, not yet known)
   * - 2 = , or } inside root array/object
   * - 3 = key or value inside root array/object.
   */
  depth_t _depth{};

public:
  simdjson_really_inline json_iterator() noexcept = default;
  simdjson_really_inline json_iterator(json_iterator &&other) noexcept;
  simdjson_really_inline json_iterator &operator=(json_iterator &&other) noexcept;
  simdjson_really_inline json_iterator(const json_iterator &other) noexcept = delete;
  simdjson_really_inline json_iterator &operator=(const json_iterator &other) noexcept = delete;

  /**
   * Skips a JSON value, whether it is a scalar, array or object.
   */
  simdjson_warn_unused simdjson_really_inline error_code skip_child(depth_t parent_depth) noexcept;

  /**
   * Tell whether the iterator is still at the start
   */
  simdjson_really_inline bool at_root() const noexcept;

  /**
   * Get the root value iterator
   */
  simdjson_really_inline token_position root_checkpoint() const noexcept;

  /**
   * Assert if the iterator is not at the start
   */
  simdjson_really_inline void assert_at_root() const noexcept;

  /**
   * Tell whether the iterator is at the EOF mark
   */
  simdjson_really_inline bool at_eof() const noexcept;

  /**
   * Tell whether the iterator is live (has not been moved).
   */
  simdjson_really_inline bool is_alive() const noexcept;

  /**
   * Abandon this iterator, setting depth to 0 (as if the document is finished).
   */
  simdjson_really_inline void abandon() noexcept;

  /**
   * Advance the current token.
   */
  simdjson_really_inline const uint8_t *advance() noexcept;

  /**
   * Get the JSON text for a given token (relative).
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param delta The relative position of the token to retrieve. e.g. 0 = next token, -1 = prev token.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_really_inline const uint8_t *peek(int32_t delta=0) const noexcept;
  /**
   * Get the maximum length of the JSON text for the current token (or relative).
   *
   * The length will include any whitespace at the end of the token.
   *
   * @param delta The relative position of the token to retrieve. e.g. 0 = next token, -1 = prev token.
   */
  simdjson_really_inline uint32_t peek_length(int32_t delta=0) const noexcept;
  /**
   * Get the JSON text for a given token.
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * @param index The position of the token to retrieve.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_really_inline const uint8_t *peek(token_position position) const noexcept;
  /**
   * Get the maximum length of the JSON text for the current token (or relative).
   *
   * The length will include any whitespace at the end of the token.
   *
   * @param index The position of the token to retrieve.
   */
  simdjson_really_inline uint32_t peek_length(token_position position) const noexcept;
  /**
   * Get the JSON text for the last token in the document.
   *
   * This is not null-terminated; it is a view into the JSON.
   *
   * TODO consider a string_view, assuming the length will get stripped out by the optimizer when
   * it isn't used ...
   */
  simdjson_really_inline const uint8_t *peek_last() const noexcept;

  /**
   * Ascend one level.
   *
   * Validates that the depth - 1 == parent_depth.
   *
   * @param parent_depth the expected parent depth.
   */
  simdjson_really_inline void ascend_to(depth_t parent_depth) noexcept;

  /**
   * Descend one level.
   *
   * Validates that the new depth == child_depth.
   *
   * @param child_depth the expected child depth.
   */
  simdjson_really_inline void descend_to(depth_t parent_depth) noexcept;
  simdjson_really_inline void descend_to(depth_t parent_depth, int32_t delta) noexcept;

  /**
   * Get current depth.
   */
  simdjson_really_inline depth_t depth() const noexcept;

  /**
   * Get current (writeable) location in the string buffer.
   */
  simdjson_really_inline uint8_t *&string_buf_loc() noexcept;

  /**
   * Report an error, preventing further iteration.
   *
   * @param error The error to report. Must not be SUCCESS, UNINITIALIZED, INCORRECT_TYPE, or NO_SUCH_FIELD.
   * @param message An error message to report with the error.
   */
  simdjson_really_inline error_code report_error(error_code error, const char *message) noexcept;

  /**
   * Log error, but don't stop iteration.
   * @param error The error to report. Must be INCORRECT_TYPE, or NO_SUCH_FIELD.
   * @param message An error message to report with the error.
   */
  simdjson_really_inline error_code optional_error(error_code error, const char *message) noexcept;

  template<int N> simdjson_warn_unused simdjson_really_inline bool copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t (&tmpbuf)[N]) noexcept;
  template<int N> simdjson_warn_unused simdjson_really_inline bool peek_to_buffer(uint8_t (&tmpbuf)[N]) noexcept;
  template<int N> simdjson_warn_unused simdjson_really_inline bool advance_to_buffer(uint8_t (&tmpbuf)[N]) noexcept;

  simdjson_really_inline token_position position() const noexcept;
  simdjson_really_inline void reenter_child(token_position position, depth_t child_depth) noexcept;
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  simdjson_really_inline token_position start_position(depth_t depth) const noexcept;
  simdjson_really_inline void set_start_position(depth_t depth, token_position position) noexcept;
#endif

protected:
  simdjson_really_inline json_iterator(const uint8_t *buf, ondemand::parser *parser) noexcept;
  simdjson_really_inline token_position last_document_position() const noexcept;

  friend class document;
  friend class object;
  friend class array;
  friend class value;
  friend class raw_json_string;
  friend class parser;
  friend class value_iterator;
  friend simdjson_really_inline void logger::log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept;
  friend simdjson_really_inline void logger::log_line(const json_iterator &iter, token_position index, depth_t depth, const char *title_prefix, const char *title, std::string_view detail) noexcept;
}; // json_iterator

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private

  simdjson_really_inline simdjson_result() noexcept = default;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/json_iterator.h */
/* begin file include/simdjson/generic/ondemand/value_iterator.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class document;
class object;
class array;
class value;
class raw_json_string;
class parser;

/**
 * Iterates through a single JSON value at a particular depth.
 *
 * Does not keep track of the type of value: provides methods for objects, arrays and scalars and expects
 * the caller to call the right ones.
 *
 * @private This is not intended for external use.
 */
class value_iterator {
protected:
  /** The underlying JSON iterator */
  json_iterator *_json_iter{};
  /** The depth of this value */
  depth_t _depth{};
  /**
   * The starting token index for this value
   *
   * PERF NOTE: this is a safety check; we expect this to be elided in release builds.
   */
  token_position _start_position{};

public:
  simdjson_really_inline value_iterator() noexcept = default;

  /**
   * Denote that we're starting a document.
   */
  simdjson_really_inline void start_document() noexcept;

  /**
   * Skips a non-iterated or partially-iterated JSON value, whether it is a scalar, array or object.
   *
   * Optimized for scalars.
   */
  simdjson_warn_unused simdjson_really_inline error_code skip_child() noexcept;

  /**
   * Tell whether the iterator is at the EOF mark
   */
  simdjson_really_inline bool at_eof() const noexcept;

  /**
   * Tell whether the iterator is at the start of the value
   */
  simdjson_really_inline bool at_start() const noexcept;

  /**
   * Tell whether the value is open--if the value has not been used, or the array/object is still open.
   */
  simdjson_really_inline bool is_open() const noexcept;

  /**
   * Tell whether the value is at an object's first field (just after the {).
   */
  simdjson_really_inline bool at_first_field() const noexcept;

  /**
   * Abandon all iteration.
   */
  simdjson_really_inline void abandon() noexcept;

  /**
   * Get the child value as a value_iterator.
   */
  simdjson_really_inline value_iterator child_value() const noexcept;

  /**
   * Get the depth of this value.
   */
  simdjson_really_inline depth_t depth() const noexcept;

  /**
   * Get the JSON type of this value.
   *
   * @error TAPE_ERROR when the JSON value is a bad token like "}" "," or "alse".
   */
  simdjson_really_inline simdjson_result<json_type> type() noexcept;

  /**
   * @addtogroup object Object iteration
   *
   * Methods to iterate and find object fields. These methods generally *assume* the value is
   * actually an object; the caller is responsible for keeping track of that fact.
   *
   * @{
   */

  /**
   * Start an object iteration.
   *
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCORRECT_TYPE if there is no opening {
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_object() noexcept;
  /**
   * Start an object iteration from the root.
   *
   * @returns Whether the object had any fields (returns false for empty).
   * @error INCORRECT_TYPE if there is no opening {
   * @error TAPE_ERROR if there is no matching } at end of document
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_root_object() noexcept;

  /**
   * Start an object iteration after the user has already checked and moved past the {.
   *
   * Does not move the iterator.
   *
   * @returns Whether the object had any fields (returns false for empty).
   */
  simdjson_warn_unused simdjson_really_inline bool started_object() noexcept;

  /**
   * Moves to the next field in an object.
   *
   * Looks for , and }. If } is found, the object is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   *
   * @return whether there is another field in the object.
   * @error TAPE_ERROR If there is a comma missing between fields.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> has_next_field() noexcept;

  /**
   * Get the current field's key.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> field_key() noexcept;

  /**
   * Pass the : in the field and move to its value.
   */
  simdjson_warn_unused simdjson_really_inline error_code field_value() noexcept;

  /**
   * Find the next field with the given key.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   *
   * This means the iterator must be sitting at the next key:
   *
   * ```
   * { "a": 1, "b": 2 }
   *           ^
   * ```
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  simdjson_warn_unused simdjson_really_inline error_code find_field(const std::string_view key) noexcept;

  /**
   * Find the next field with the given key, *without* unescaping. This assumes object order: it
   * will not find the field if it was already passed when looking for some *other* field.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   *
   * This means the iterator must be sitting at the next key:
   *
   * ```
   * { "a": 1, "b": 2 }
   *           ^
   * ```
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> find_field_raw(const std::string_view key) noexcept;

  /**
   * Find the field with the given key without regard to order, and *without* unescaping.
   *
   * This is an unordered object lookup: if the field is not found initially, it will cycle around and scan from the beginning.
   *
   * Assumes you have called next_field() or otherwise matched the previous value.
   *
   * This means the iterator must be sitting at the next key:
   *
   * ```
   * { "a": 1, "b": 2 }
   *           ^
   * ```
   *
   * Key is *raw JSON,* meaning it will be matched against the verbatim JSON without attempting to
   * unescape it. This works well for typical ASCII and UTF-8 keys (almost all of them), but may
   * fail to match some keys with escapes (\u, \n, etc.).
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> find_field_unordered_raw(const std::string_view key) noexcept;

  /** @} */

  /**
   * @addtogroup array Array iteration
   * Methods to iterate over array elements. These methods generally *assume* the value is actually
   * an object; the caller is responsible for keeping track of that fact.
   * @{
   */

  /**
   * Check for an opening [ and start an array iteration.
   *
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCORRECT_TYPE If there is no [.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_array() noexcept;
  /**
   * Check for an opening [ and start an array iteration while at the root.
   *
   * @returns Whether the array had any elements (returns false for empty).
   * @error INCORRECT_TYPE If there is no [.
   * @error TAPE_ERROR if there is no matching ] at end of document
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> start_root_array() noexcept;

  /**
   * Start an array iteration after the user has already checked and moved past the [.
   *
   * Does not move the iterator.
   *
   * @returns Whether the array had any elements (returns false for empty).
   */
  simdjson_warn_unused simdjson_really_inline bool started_array() noexcept;

  /**
   * Moves to the next element in an array.
   *
   * Looks for , and ]. If ] is found, the array is finished and the iterator advances past it.
   * Otherwise, it advances to the next value.
   *
   * @return Whether there is another element in the array.
   * @error TAPE_ERROR If there is a comma missing between elements.
   */
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> has_next_element() noexcept;

  /**
   * Get a child value iterator.
   */
  simdjson_warn_unused simdjson_really_inline value_iterator child() const noexcept;

  /** @} */

  /**
   * @defgroup scalar Scalar values
   * @addtogroup scalar
   * @{
   */

  simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> get_string() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<double> get_double() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

  simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> get_root_string() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> get_root_raw_json_string() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> get_root_uint64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> get_root_int64() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<double> get_root_double() noexcept;
  simdjson_warn_unused simdjson_really_inline simdjson_result<bool> get_root_bool() noexcept;
  simdjson_really_inline bool is_root_null() noexcept;

  simdjson_really_inline error_code error() const noexcept;
  simdjson_really_inline uint8_t *&string_buf_loc() noexcept;
  simdjson_really_inline const json_iterator &json_iter() const noexcept;
  simdjson_really_inline json_iterator &json_iter() noexcept;

  simdjson_really_inline void assert_is_valid() const noexcept;
  simdjson_really_inline bool is_valid() const noexcept;

  /** @} */

protected:
  simdjson_really_inline value_iterator(json_iterator *json_iter, depth_t depth, token_position start_index) noexcept;

  simdjson_really_inline bool parse_null(const uint8_t *json) const noexcept;
  simdjson_really_inline simdjson_result<bool> parse_bool(const uint8_t *json) const noexcept;

  simdjson_really_inline const uint8_t *peek_start() const noexcept;
  simdjson_really_inline uint32_t peek_start_length() const noexcept;
  simdjson_really_inline const uint8_t *advance_start(const char *type) const noexcept;
  simdjson_really_inline error_code advance_container_start(const char *type, const uint8_t *&json) const noexcept;
  simdjson_really_inline const uint8_t *advance_root_scalar(const char *type) const noexcept;
  simdjson_really_inline const uint8_t *advance_non_root_scalar(const char *type) const noexcept;

  simdjson_really_inline error_code incorrect_type_error(const char *message) const noexcept;

  simdjson_really_inline bool is_at_start() const noexcept;
  simdjson_really_inline bool is_at_container_start() const noexcept;
  simdjson_really_inline bool is_at_iterator_start() const noexcept;
  simdjson_really_inline void assert_at_start() const noexcept;
  simdjson_really_inline void assert_at_container_start() const noexcept;
  simdjson_really_inline void assert_at_root() const noexcept;
  simdjson_really_inline void assert_at_child() const noexcept;
  simdjson_really_inline void assert_at_next() const noexcept;
  simdjson_really_inline void assert_at_non_root_start() const noexcept;

  friend class document;
  friend class object;
  friend class array;
  friend class value;
}; // value_iterator

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/value_iterator.h */
/* begin file include/simdjson/generic/ondemand/array_iterator.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class array;
class value;
class document;

/**
 * A forward-only JSON array.
 *
 * This is an input_iterator, meaning:
 * - It is forward-only
 * - * must be called exactly once per element.
 * - ++ must be called exactly once in between each * (*, ++, *, ++, * ...)
 */
class array_iterator {
public:
  /** Create a new, invalid array iterator. */
  simdjson_really_inline array_iterator() noexcept = default;

  //
  // Iterator interface
  //

  /**
   * Get the current element.
   *
   * Part of the std::iterator interface.
   */
  simdjson_really_inline simdjson_result<value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  /**
   * Check if we are at the end of the JSON.
   *
   * Part of the std::iterator interface.
   *
   * @return true if there are no more elements in the JSON array.
   */
  simdjson_really_inline bool operator==(const array_iterator &) const noexcept;
  /**
   * Check if there are more elements in the JSON array.
   *
   * Part of the std::iterator interface.
   *
   * @return true if there are more elements in the JSON array.
   */
  simdjson_really_inline bool operator!=(const array_iterator &) const noexcept;
  /**
   * Move to the next element.
   *
   * Part of the std::iterator interface.
   */
  simdjson_really_inline array_iterator &operator++() noexcept;

private:
  value_iterator iter{};

  simdjson_really_inline array_iterator(const value_iterator &iter) noexcept;

  friend class array;
  friend class value;
  friend struct simdjson_result<array_iterator>;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  //
  // Iterator interface
  //

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  simdjson_really_inline bool operator==(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> &) const noexcept;
  simdjson_really_inline bool operator!=(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> &) const noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> &operator++() noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/array_iterator.h */
/* begin file include/simdjson/generic/ondemand/object_iterator.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class field;

class object_iterator {
public:
  /**
   * Create a new invalid object_iterator.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline object_iterator() noexcept = default;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  // MUST ONLY BE CALLED ONCE PER ITERATION.
  simdjson_really_inline simdjson_result<field> operator*() noexcept;
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_really_inline bool operator==(const object_iterator &) const noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_really_inline bool operator!=(const object_iterator &) const noexcept;
  // Checks for ']' and ','
  simdjson_really_inline object_iterator &operator++() noexcept;

private:
  /**
   * The underlying JSON iterator.
   *
   * PERF NOTE: expected to be elided in favor of the parent document: this is set when the object
   * is first used, and never changes afterwards.
   */
  value_iterator iter{};

  simdjson_really_inline object_iterator(const value_iterator &iter) noexcept;
  friend struct simdjson_result<object_iterator>;
  friend class object;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  //
  // Iterator interface
  //

  // Reads key and value, yielding them to the user.
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field> operator*() noexcept; // MUST ONLY BE CALLED ONCE PER ITERATION.
  // Assumes it's being compared with the end. true if depth < iter->depth.
  simdjson_really_inline bool operator==(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> &) const noexcept;
  // Assumes it's being compared with the end. true if depth >= iter->depth.
  simdjson_really_inline bool operator!=(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> &) const noexcept;
  // Checks for ']' and ','
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> &operator++() noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/object_iterator.h */
/* begin file include/simdjson/generic/ondemand/array.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class value;
class document;

/**
 * A forward-only JSON array.
 */
class array {
public:
  /**
   * Create a new invalid array.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline array() noexcept = default;

  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator> begin() noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator> end() noexcept;

protected:
  /**
   * Begin array iteration.
   *
   * @param iter The iterator. Must be where the initial [ is expected. Will be *moved* into the
   *        resulting array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   */
  static simdjson_really_inline simdjson_result<array> start(value_iterator &iter) noexcept;
  /**
   * Begin array iteration from the root.
   *
   * @param iter The iterator. Must be where the initial [ is expected. Will be *moved* into the
   *        resulting array.
   * @error INCORRECT_TYPE if the iterator is not at [.
   * @error TAPE_ERROR if there is no closing ] at the end of the document.
   */
  static simdjson_really_inline simdjson_result<array> start_root(value_iterator &iter) noexcept;
  /**
   * Begin array iteration.
   *
   * This version of the method should be called after the initial [ has been verified, and is
   * intended for use by switch statements that check the type of a value.
   *
   * @param iter The iterator. Must be after the initial [. Will be *moved* into the resulting array.
   */
  static simdjson_really_inline array started(value_iterator &iter) noexcept;

  /**
   * Create an array at the given Internal array creation. Call array::start() or array::started() instead of this.
   *
   * @param iter The iterator. Must either be at the start of the first element with iter.is_alive()
   *        == true, or past the [] with is_alive() == false if the array is empty. Will be *moved*
   *        into the resulting array.
   */
  simdjson_really_inline array(const value_iterator &iter) noexcept;

  /**
   * Iterator marking current position.
   *
   * iter.is_alive() == false indicates iteration is complete.
   */
  value_iterator iter{};

  friend class value;
  friend class document;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<array>;
  friend class array_iterator;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> begin() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> end() noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/array.h */
/* begin file include/simdjson/generic/ondemand/document.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class parser;
class array;
class object;
class value;
class raw_json_string;
class array_iterator;

/**
 * A JSON document iteration.
 *
 * Used by tokens to get text, and string buffer location.
 *
 * You must keep the document around during iteration.
 */
class document {
public:
  /**
   * Create a new invalid document.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline document() noexcept = default;
  simdjson_really_inline document(const document &other) noexcept = delete;
  simdjson_really_inline document(document &&other) noexcept = default;
  simdjson_really_inline document &operator=(const document &other) noexcept = delete;
  simdjson_really_inline document &operator=(document &&other) noexcept = default;

  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @returns INCORRECT_TYPE If the JSON value is not an array.
   */
  simdjson_really_inline simdjson_result<array> get_array() & noexcept;
  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   */
  simdjson_really_inline simdjson_result<object> get_object() & noexcept;
  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit integer.
   */
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @returns INCORRECT_TYPE If the JSON value is not a valid floating-point number.
   */
  simdjson_really_inline simdjson_result<double> get_double() noexcept;
  /**
   * Cast this JSON value to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_really_inline simdjson_result<std::string_view> get_string() noexcept;
  /**
   * Cast this JSON value to a raw_json_string.
   *
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;
  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @returns INCORRECT_TYPE if the JSON value is not true or false.
   */
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  /**
   * Checks if this JSON value is null.
   *
   * @returns Whether the value is null.
   */
  simdjson_really_inline bool is_null() noexcept;

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool
   *
   * You may use get_double(), get_bool(), get_uint64(), get_int64(),
   * get_object(), get_array(), get_raw_json_string(), or get_string() instead.
   *
   * @returns A value of the given type, parsed from the JSON.
   * @returns INCORRECT_TYPE If the JSON value is not the given type.
   */
  template<typename T> simdjson_really_inline simdjson_result<T> get() & noexcept {
    // Unless the simdjson library provides an inline implementation, calling this method should
    // immediately fail.
    static_assert(!sizeof(T), "The get method with given type is not implemented by the simdjson library.");
  }
  /** @overload template<typename T> simdjson_result<T> get() & noexcept */
  template<typename T> simdjson_really_inline simdjson_result<T> get() && noexcept {
    // Unless the simdjson library provides an inline implementation, calling this method should
    // immediately fail.
    static_assert(!sizeof(T), "The get method with given type is not implemented by the simdjson library.");
  }

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool
   *
   * @param out This is set to a value of the given type, parsed from the JSON. If there is an error, this may not be initialized.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   * @returns SUCCESS If the parse succeeded and the out parameter was set to the value.
   */
  template<typename T> simdjson_really_inline error_code get(T &out) & noexcept;
  /** @overload template<typename T> error_code get(T &out) & noexcept */
  template<typename T> simdjson_really_inline error_code get(T &out) && noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an array.
   */
  simdjson_really_inline operator array() & noexcept(false);
  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an object.
   */
  simdjson_really_inline operator object() & noexcept(false);
  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_really_inline operator uint64_t() noexcept(false);
  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit integer.
   */
  simdjson_really_inline operator int64_t() noexcept(false);
  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a valid floating-point number.
   */
  simdjson_really_inline operator double() noexcept(false);
  /**
   * Cast this JSON value to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_really_inline operator std::string_view() noexcept(false);
  /**
   * Cast this JSON value to a raw_json_string.
   *
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_really_inline operator raw_json_string() noexcept(false);
  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not true or false.
   */
  simdjson_really_inline operator bool() noexcept(false);
#endif

  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator> begin() & noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator> end() & noexcept;

  /**
   * Look up a field by name on an object (order-sensitive).
   *
   * The following code reads z, then y, then x, and thus will not retrieve x or y if fed the
   * JSON `{ "x": 1, "y": 2, "z": 3 }`:
   *
   * ```c++
   * simdjson::ondemand::parser parser;
   * auto obj = parser.parse(R"( { "x": 1, "y": 2, "z": 3 } )"_padded);
   * double z = obj.find_field("z");
   * double y = obj.find_field("y");
   * double x = obj.find_field("x");
   * ```
   *
   * **Raw Keys:** The lookup will be done against the *raw* key, and will not unescape keys.
   * e.g. `object["a"]` will match `{ "a": 1 }`, but will *not* match `{ "\u0061": 1 }`.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<value> find_field(std::string_view key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> find_field(const char *key) & noexcept;

  /**
   * Look up a field by name on an object, without regard to key order.
   *
   * **Performance Notes:** This is a bit less performant than find_field(), though its effect varies
   * and often appears negligible. It starts out normally, starting out at the last field; but if
   * the field is not found, it scans from the beginning of the object to see if it missed it. That
   * missing case has a non-cache-friendly bump and lots of extra scanning, especially if the object
   * in question is large. The fact that the extra code is there also bumps the executable size.
   *
   * It is the default, however, because it would be highly surprising (and hard to debug) if the
   * default behavior failed to look up a field just because it was in the wrong order--and many
   * APIs assume this. Therefore, you must be explicit if you want to treat objects as out of order.
   *
   * Use find_field() if you are sure fields will be in order (or are willing to treat it as if the
   * field wasn't there when they aren't).
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> find_field_unordered(const char *key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> operator[](const char *key) & noexcept;

  /**
   * Get the type of this JSON value.
   *
   * NOTE: If you're only expecting a value to be one type (a typical case), it's generally
   * better to just call .get_double, .get_string, etc. and check for INCORRECT_TYPE (or just
   * let it throw an exception).
   *
   * @error TAPE_ERROR when the JSON value is a bad token like "}" "," or "alse".
   */
  simdjson_really_inline simdjson_result<json_type> type() noexcept;

  /**
   * Get the raw JSON for this token.
   *
   * The string_view will always point into the input buffer.
   *
   * The string_view will start at the beginning of the token, and include the entire token
   * *as well as all spaces until the next token (or EOF).* This means, for example, that a
   * string token always begins with a " and is always terminated by the final ", possibly
   * followed by a number of spaces.
   *
   * The string_view is *not* null-terminated. If this is a scalar (string, number,
   * boolean, or null), the character after the end of the string_view may be the padded buffer.
   *
   * Tokens include:
   * - {
   * - [
   * - "a string (possibly with UTF-8 or backslashed characters like \\\")".
   * - -1.2e-100
   * - true
   * - false
   * - null
   */
  simdjson_really_inline simdjson_result<std::string_view> raw_json_token() noexcept;

protected:
  simdjson_really_inline document(ondemand::json_iterator &&iter) noexcept;
  simdjson_really_inline const uint8_t *text(uint32_t idx) const noexcept;

  simdjson_really_inline value_iterator resume_value_iterator() noexcept;
  simdjson_really_inline value_iterator get_root_value_iterator() noexcept;
  simdjson_really_inline value resume_value() noexcept;
  static simdjson_really_inline document start(ondemand::json_iterator &&iter) noexcept;

  //
  // Fields
  //
  json_iterator iter{}; ///< Current position in the document
  static constexpr depth_t DOCUMENT_DEPTH = 0; ///< document depth is always 0

  friend struct simdjson_result<document>;
  friend class array_iterator;
  friend class value;
  friend class ondemand::parser;
  friend class object;
  friend class array;
  friend class field;
  friend class token;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array> get_array() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object> get_object() & noexcept;
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_really_inline simdjson_result<double> get_double() noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

  template<typename T> simdjson_really_inline simdjson_result<T> get() & noexcept;
  template<typename T> simdjson_really_inline simdjson_result<T> get() && noexcept;

  template<typename T> simdjson_really_inline error_code get(T &out) & noexcept;
  template<typename T> simdjson_really_inline error_code get(T &out) && noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array() & noexcept(false);
  simdjson_really_inline operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object() & noexcept(false);
  simdjson_really_inline operator uint64_t() noexcept(false);
  simdjson_really_inline operator int64_t() noexcept(false);
  simdjson_really_inline operator double() noexcept(false);
  simdjson_really_inline operator std::string_view() noexcept(false);
  simdjson_really_inline operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false);
  simdjson_really_inline operator bool() noexcept(false);
#endif

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> begin() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> end() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field(const char *key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> operator[](const char *key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(const char *key) & noexcept;

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type> type() noexcept;

  /** @copydoc simdjson_really_inline std::string_view document::raw_json_token() const noexcept */
  simdjson_really_inline simdjson_result<std::string_view> raw_json_token() noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/document.h */
/* begin file include/simdjson/generic/ondemand/value.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class array;
class document;
class field;
class object;
class raw_json_string;

/**
 * An ephemeral JSON value returned during iteration.
 */
class value {
public:
  /**
   * Create a new invalid value.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline value() noexcept = default;

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool
   *
   * You may use get_double(), get_bool(), get_uint64(), get_int64(),
   * get_object(), get_array(), get_raw_json_string(), or get_string() instead.
   *
   * @returns A value of the given type, parsed from the JSON.
   * @returns INCORRECT_TYPE If the JSON value is not the given type.
   */
  template<typename T> simdjson_really_inline simdjson_result<T> get() noexcept {
    // Unless the simdjson library provides an inline implementation, calling this method should
    // immediately fail.
    static_assert(!sizeof(T), "The get method with given type is not implemented by the simdjson library.");
  }

  /**
   * Get this value as the given type.
   *
   * Supported types: object, array, raw_json_string, string_view, uint64_t, int64_t, double, bool
   *
   * @param out This is set to a value of the given type, parsed from the JSON. If there is an error, this may not be initialized.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   * @returns SUCCESS If the parse succeeded and the out parameter was set to the value.
   */
  template<typename T> simdjson_really_inline error_code get(T &out) noexcept;

  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @returns INCORRECT_TYPE If the JSON value is not an array.
   */
  simdjson_really_inline simdjson_result<array> get_array() noexcept;

  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @returns INCORRECT_TYPE If the JSON value is not an object.
   */
  simdjson_really_inline simdjson_result<object> get_object() noexcept;

  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;

  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @returns INCORRECT_TYPE If the JSON value is not a 64-bit integer.
   */
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;

  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @returns INCORRECT_TYPE If the JSON value is not a valid floating-point number.
   */
  simdjson_really_inline simdjson_result<double> get_double() noexcept;

  /**
   * Cast this JSON value to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * Equivalent to get<std::string_view>().
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_really_inline simdjson_result<std::string_view> get_string() noexcept;

  /**
   * Cast this JSON value to a raw_json_string.
   *
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @returns INCORRECT_TYPE if the JSON value is not a string.
   */
  simdjson_really_inline simdjson_result<raw_json_string> get_raw_json_string() noexcept;

  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @returns INCORRECT_TYPE if the JSON value is not true or false.
   */
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;

  /**
   * Checks if this JSON value is null.
   *
   * @returns Whether the value is null.
   */
  simdjson_really_inline bool is_null() noexcept;

#if SIMDJSON_EXCEPTIONS
  /**
   * Cast this JSON value to an array.
   *
   * @returns An object that can be used to iterate the array.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an array.
   */
  simdjson_really_inline operator array() noexcept(false);
  /**
   * Cast this JSON value to an object.
   *
   * @returns An object that can be used to look up or iterate fields.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not an object.
   */
  simdjson_really_inline operator object() noexcept(false);
  /**
   * Cast this JSON value to an unsigned integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit unsigned integer.
   */
  simdjson_really_inline operator uint64_t() noexcept(false);
  /**
   * Cast this JSON value to a signed integer.
   *
   * @returns A signed 64-bit integer.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a 64-bit integer.
   */
  simdjson_really_inline operator int64_t() noexcept(false);
  /**
   * Cast this JSON value to a double.
   *
   * @returns A double.
   * @exception simdjson_error(INCORRECT_TYPE) If the JSON value is not a valid floating-point number.
   */
  simdjson_really_inline operator double() noexcept(false);
  /**
   * Cast this JSON value to a string.
   *
   * The string is guaranteed to be valid UTF-8.
   *
   * Equivalent to get<std::string_view>().
   *
   * @returns An UTF-8 string. The string is stored in the parser and will be invalidated the next
   *          time it parses a document or when it is destroyed.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_really_inline operator std::string_view() noexcept(false);
  /**
   * Cast this JSON value to a raw_json_string.
   *
   * The string is guaranteed to be valid UTF-8, and may have escapes in it (e.g. \\ or \n).
   *
   * @returns A pointer to the raw JSON for the given string.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not a string.
   */
  simdjson_really_inline operator raw_json_string() noexcept(false);
  /**
   * Cast this JSON value to a bool.
   *
   * @returns A bool value.
   * @exception simdjson_error(INCORRECT_TYPE) if the JSON value is not true or false.
   */
  simdjson_really_inline operator bool() noexcept(false);
#endif

  /**
   * Begin array iteration.
   *
   * Part of the std::iterable interface.
   *
   * @returns INCORRECT_TYPE If the JSON value is not an array.
   */
  simdjson_really_inline simdjson_result<array_iterator> begin() & noexcept;
  /**
   * Sentinel representing the end of the array.
   *
   * Part of the std::iterable interface.
   */
  simdjson_really_inline simdjson_result<array_iterator> end() & noexcept;

  /**
   * Look up a field by name on an object (order-sensitive).
   *
   * The following code reads z, then y, then x, and thus will not retrieve x or y if fed the
   * JSON `{ "x": 1, "y": 2, "z": 3 }`:
   *
   * ```c++
   * simdjson::ondemand::parser parser;
   * auto obj = parser.parse(R"( { "x": 1, "y": 2, "z": 3 } )"_padded);
   * double z = obj.find_field("z");
   * double y = obj.find_field("y");
   * double x = obj.find_field("x");
   * ```
   * If you have multiple fields with a matching key ({"x": 1,  "x": 1}) be mindful
   * that only one field is returned.

   * **Raw Keys:** The lookup will be done against the *raw* key, and will not unescape keys.
   * e.g. `object["a"]` will match `{ "a": 1 }`, but will *not* match `{ "\u0061": 1 }`.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<value> find_field(std::string_view key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<value> find_field(const char *key) noexcept;

  /**
   * Look up a field by name on an object, without regard to key order.
   *
   * **Performance Notes:** This is a bit less performant than find_field(), though its effect varies
   * and often appears negligible. It starts out normally, starting out at the last field; but if
   * the field is not found, it scans from the beginning of the object to see if it missed it. That
   * missing case has a non-cache-friendly bump and lots of extra scanning, especially if the object
   * in question is large. The fact that the extra code is there also bumps the executable size.
   *
   * It is the default, however, because it would be highly surprising (and hard to debug) if the
   * default behavior failed to look up a field just because it was in the wrong order--and many
   * APIs assume this. Therefore, you must be explicit if you want to treat objects as out of order.
   *
   * If you have multiple fields with a matching key ({"x": 1,  "x": 1}) be mindful
   * that only one field is returned.
   *
   * Use find_field() if you are sure fields will be in order (or are willing to treat it as if the
   * field wasn't there when they aren't).
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<value> find_field_unordered(const char *key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<value> operator[](const char *key) noexcept;

  /**
   * Get the type of this JSON value.
   *
   * NOTE: If you're only expecting a value to be one type (a typical case), it's generally
   * better to just call .get_double, .get_string, etc. and check for INCORRECT_TYPE (or just
   * let it throw an exception).
   *
   * @return The type of JSON value (json_type::array, json_type::object, json_type::string,
   *     json_type::number, json_type::boolean, or json_type::null).
   * @error TAPE_ERROR when the JSON value is a bad token like "}" "," or "alse".
   */
  simdjson_really_inline simdjson_result<json_type> type() noexcept;

  /**
   * Get the raw JSON for this token.
   *
   * The string_view will always point into the input buffer.
   *
   * The string_view will start at the beginning of the token, and include the entire token
   * *as well as all spaces until the next token (or EOF).* This means, for example, that a
   * string token always begins with a " and is always terminated by the final ", possibly
   * followed by a number of spaces.
   *
   * The string_view is *not* null-terminated. However, if this is a scalar (string, number,
   * boolean, or null), the character after the end of the string_view is guaranteed to be
   * a non-space token.
   *
   * Tokens include:
   * - {
   * - [
   * - "a string (possibly with UTF-8 or backslashed characters like \\\")".
   * - -1.2e-100
   * - true
   * - false
   * - null
   */
  simdjson_really_inline std::string_view raw_json_token() noexcept;

protected:
  /**
   * Create a value.
   */
  simdjson_really_inline value(const value_iterator &iter) noexcept;

  /**
   * Skip this value, allowing iteration to continue.
   */
  simdjson_really_inline void skip() noexcept;

  /**
   * Start a value at the current position.
   *
   * (It should already be started; this is just a self-documentation method.)
   */
  static simdjson_really_inline value start(const value_iterator &iter) noexcept;

  /**
   * Resume a value.
   */
  static simdjson_really_inline value resume(const value_iterator &iter) noexcept;

  /**
   * Get the object, starting or resuming it as necessary
   */
  simdjson_really_inline simdjson_result<object> start_or_resume_object() noexcept;

  // simdjson_really_inline void log_value(const char *type) const noexcept;
  // simdjson_really_inline void log_error(const char *message) const noexcept;

  value_iterator iter{};

  friend class document;
  friend class array_iterator;
  friend class field;
  friend class object;
  friend struct simdjson_result<value>;
  friend struct simdjson_result<document>;
  friend struct simdjson_result<field>;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array> get_array() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object> get_object() noexcept;

  simdjson_really_inline simdjson_result<uint64_t> get_uint64() noexcept;
  simdjson_really_inline simdjson_result<int64_t> get_int64() noexcept;
  simdjson_really_inline simdjson_result<double> get_double() noexcept;
  simdjson_really_inline simdjson_result<std::string_view> get_string() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> get_raw_json_string() noexcept;
  simdjson_really_inline simdjson_result<bool> get_bool() noexcept;
  simdjson_really_inline bool is_null() noexcept;

  template<typename T> simdjson_really_inline simdjson_result<T> get() noexcept;

  template<typename T> simdjson_really_inline error_code get(T &out) noexcept;

#if SIMDJSON_EXCEPTIONS
  simdjson_really_inline operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array() noexcept(false);
  simdjson_really_inline operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object() noexcept(false);
  simdjson_really_inline operator uint64_t() noexcept(false);
  simdjson_really_inline operator int64_t() noexcept(false);
  simdjson_really_inline operator double() noexcept(false);
  simdjson_really_inline operator std::string_view() noexcept(false);
  simdjson_really_inline operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false);
  simdjson_really_inline operator bool() noexcept(false);
#endif

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> begin() & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> end() & noexcept;

  /**
   * Look up a field by name on an object (order-sensitive).
   *
   * The following code reads z, then y, then x, and thus will not retrieve x or y if fed the
   * JSON `{ "x": 1, "y": 2, "z": 3 }`:
   *
   * ```c++
   * simdjson::ondemand::parser parser;
   * auto obj = parser.parse(R"( { "x": 1, "y": 2, "z": 3 } )"_padded);
   * double z = obj.find_field("z");
   * double y = obj.find_field("y");
   * double x = obj.find_field("x");
   * ```
   *
   * **Raw Keys:** The lookup will be done against the *raw* key, and will not unescape keys.
   * e.g. `object["a"]` will match `{ "a": 1 }`, but will *not* match `{ "\u0061": 1 }`.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field(const char *key) noexcept;

  /**
   * Look up a field by name on an object, without regard to key order.
   *
   * **Performance Notes:** This is a bit less performant than find_field(), though its effect varies
   * and often appears negligible. It starts out normally, starting out at the last field; but if
   * the field is not found, it scans from the beginning of the object to see if it missed it. That
   * missing case has a non-cache-friendly bump and lots of extra scanning, especially if the object
   * in question is large. The fact that the extra code is there also bumps the executable size.
   *
   * It is the default, however, because it would be highly surprising (and hard to debug) if the
   * default behavior failed to look up a field just because it was in the wrong order--and many
   * APIs assume this. Therefore, you must be explicit if you want to treat objects as out of order.
   *
   * Use find_field() if you are sure fields will be in order (or are willing to treat it as if the
   * field wasn't there when they aren't).
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(const char *key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) noexcept;
  /** @overload simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) noexcept; */
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> operator[](const char *key) noexcept;

  /**
   * Get the type of this JSON value.
   *
   * NOTE: If you're only expecting a value to be one type (a typical case), it's generally
   * better to just call .get_double, .get_string, etc. and check for INCORRECT_TYPE (or just
   * let it throw an exception).
   */
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type> type() noexcept;

  /** @copydoc simdjson_really_inline std::string_view value::raw_json_token() const noexcept */
  simdjson_really_inline simdjson_result<std::string_view> raw_json_token() noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/value.h */
/* begin file include/simdjson/generic/ondemand/field.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

/**
 * A JSON field (key/value pair) in an object.
 *
 * Returned from object iteration.
 *
 * Extends from std::pair<raw_json_string, value> so you can use C++ algorithms that rely on pairs.
 */
class field : public std::pair<raw_json_string, value> {
public:
  /**
   * Create a new invalid field.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline field() noexcept;

  /**
   * Get the key as a string_view (for higher speed, consider raw_key).
   * We deliberately use a more cumbersome name (unescaped_key) to force users
   * to think twice about using it.
   *
   * This consumes the key: once you have called unescaped_key(), you cannot
   * call it again nor can you call key().
   */
  simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> unescaped_key() noexcept;
  /**
   * Get the key as a raw_json_string. Can be used for direct comparison with
   * an unescaped C string: e.g., key() == "test".
   */
  simdjson_really_inline raw_json_string key() const noexcept;
  /**
   * Get the field value.
   */
  simdjson_really_inline ondemand::value &value() & noexcept;
  /**
   * @overload ondemand::value &ondemand::value() & noexcept
   */
  simdjson_really_inline ondemand::value value() && noexcept;

protected:
  simdjson_really_inline field(raw_json_string key, ondemand::value &&value) noexcept;
  static simdjson_really_inline simdjson_result<field> start(value_iterator &parent_iter) noexcept;
  static simdjson_really_inline simdjson_result<field> start(const value_iterator &parent_iter, raw_json_string key) noexcept;
  friend struct simdjson_result<field>;
  friend class object_iterator;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  simdjson_really_inline simdjson_result<std::string_view> unescaped_key() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> key() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> value() noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/field.h */
/* begin file include/simdjson/generic/ondemand/object.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

/**
 * A forward-only JSON object field iterator.
 */
class object {
public:
  /**
   * Create a new invalid object.
   *
   * Exists so you can declare a variable and later assign to it before use.
   */
  simdjson_really_inline object() noexcept = default;

  simdjson_really_inline simdjson_result<object_iterator> begin() noexcept;
  simdjson_really_inline simdjson_result<object_iterator> end() noexcept;

  /**
   * Look up a field by name on an object (order-sensitive).
   *
   * The following code reads z, then y, then x, and thus will not retrieve x or y if fed the
   * JSON `{ "x": 1, "y": 2, "z": 3 }`:
   *
   * ```c++
   * simdjson::ondemand::parser parser;
   * auto obj = parser.parse(R"( { "x": 1, "y": 2, "z": 3 } )"_padded);
   * double z = obj.find_field("z");
   * double y = obj.find_field("y");
   * double x = obj.find_field("x");
   * ```
   * If you have multiple fields with a matching key ({"x": 1,  "x": 1}) be mindful
   * that only one field is returned.
   *
   * **Raw Keys:** The lookup will be done against the *raw* key, and will not unescape keys.
   * e.g. `object["a"]` will match `{ "a": 1 }`, but will *not* match `{ "\u0061": 1 }`.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<value> find_field(std::string_view key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> find_field(std::string_view key) && noexcept;

  /**
   * Look up a field by name on an object, without regard to key order.
   *
   * **Performance Notes:** This is a bit less performant than find_field(), though its effect varies
   * and often appears negligible. It starts out normally, starting out at the last field; but if
   * the field is not found, it scans from the beginning of the object to see if it missed it. That
   * missing case has a non-cache-friendly bump and lots of extra scanning, especially if the object
   * in question is large. The fact that the extra code is there also bumps the executable size.
   *
   * It is the default, however, because it would be highly surprising (and hard to debug) if the
   * default behavior failed to look up a field just because it was in the wrong order--and many
   * APIs assume this. Therefore, you must be explicit if you want to treat objects as out of order.
   *
   * Use find_field() if you are sure fields will be in order (or are willing to treat it as if the
   * field wasn't there when they aren't).
   *
   * If you have multiple fields with a matching key ({"x": 1,  "x": 1}) be mindful
   * that only one field is returned.
   *
   * @param key The key to look up.
   * @returns The value of the field, or NO_SUCH_FIELD if the field is not in the object.
   */
  simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) && noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) & noexcept;
  /** @overload simdjson_really_inline simdjson_result<value> find_field_unordered(std::string_view key) & noexcept; */
  simdjson_really_inline simdjson_result<value> operator[](std::string_view key) && noexcept;

protected:
  static simdjson_really_inline simdjson_result<object> start(value_iterator &iter) noexcept;
  static simdjson_really_inline simdjson_result<object> start_root(value_iterator &iter) noexcept;
  static simdjson_really_inline object started(value_iterator &iter) noexcept;
  static simdjson_really_inline object resume(const value_iterator &iter) noexcept;
  simdjson_really_inline object(const value_iterator &iter) noexcept;

  simdjson_warn_unused simdjson_really_inline error_code find_field_raw(const std::string_view key) noexcept;

  value_iterator iter{};

  friend class value;
  friend class document;
  friend struct simdjson_result<object>;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;

  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> begin() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> end() noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field(std::string_view key) && noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> find_field_unordered(std::string_view key) && noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) & noexcept;
  simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> operator[](std::string_view key) && noexcept;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/object.h */
/* begin file include/simdjson/generic/ondemand/parser.h */

namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

class array;
class object;
class value;
class raw_json_string;

/**
 * A JSON fragment iterator.
 *
 * This holds the actual iterator as well as the buffer for writing strings.
 */
class parser {
public:
  /**
   * Create a JSON parser.
   *
   * The new parser will have zero capacity.
   */
  inline parser() noexcept = default;

  inline parser(parser &&other) noexcept = default;
  simdjson_really_inline parser(const parser &other) = delete;
  simdjson_really_inline parser &operator=(const parser &other) = delete;

  /** Deallocate the JSON parser. */
  inline ~parser() noexcept = default;

  /**
   * Start iterating an on-demand JSON document.
   *
   *   ondemand::parser parser;
   *   document doc = parser.iterate(json);
   *
   * ### IMPORTANT: Buffer Lifetime
   *
   * Because parsing is done while you iterate, you *must* keep the JSON buffer around at least as
   * long as the document iteration.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * Only one iteration at a time can happen per parser, and the parser *must* be kept alive during
   * iteration to ensure intermediate buffers can be accessed. Any document must be destroyed before
   * you call parse() again or destroy the parser.
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated.
   *
   * @param json The JSON to parse.
   * @param len The length of the JSON.
   * @param capacity The number of bytes allocated in the JSON (must be at least len+SIMDJSON_PADDING).
   *
   * @return The document, or an error:
   *         - INSUFFICIENT_PADDING if the input has less than SIMDJSON_PADDING extra bytes.
   *         - MEMALLOC if realloc_if_needed the parser does not have enough capacity, and memory
   *           allocation fails.
   *         - EMPTY if the document is all whitespace.
   *         - UTF8_ERROR if the document is not valid UTF-8.
   *         - UNESCAPED_CHARS if a string contains control characters that must be escaped
   *         - UNCLOSED_STRING if there is an unclosed string in the document.
   */
  simdjson_warn_unused simdjson_result<document> iterate(padded_string_view json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const char *json, size_t len, size_t capacity) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const uint8_t *json, size_t len, size_t capacity) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(std::string_view json, size_t capacity) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const std::string &json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const simdjson_result<padded_string> &json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const simdjson_result<padded_string_view> &json) & noexcept;
  /** @overload simdjson_result<document> iterate(padded_string_view json) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(padded_string &&json) & noexcept = delete;

  /**
   * @private
   *
   * Start iterating an on-demand JSON document.
   *
   *   ondemand::parser parser;
   *   json_iterator doc = parser.iterate(json);
   *
   * ### IMPORTANT: Buffer Lifetime
   *
   * Because parsing is done while you iterate, you *must* keep the JSON buffer around at least as
   * long as the document iteration.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * Only one iteration at a time can happen per parser, and the parser *must* be kept alive during
   * iteration to ensure intermediate buffers can be accessed. Any document must be destroyed before
   * you call parse() again or destroy the parser.
   *
   * ### REQUIRED: Buffer Padding
   *
   * The buffer must have at least SIMDJSON_PADDING extra allocated bytes. It does not matter what
   * those bytes are initialized to, as long as they are allocated.
   *
   * @param json The JSON to parse.
   * @param len The length of the JSON.
   * @param allocated The number of bytes allocated in the JSON (must be at least len+SIMDJSON_PADDING).
   *
   * @return The iterator, or an error:
   *         - INSUFFICIENT_PADDING if the input has less than SIMDJSON_PADDING extra bytes.
   *         - MEMALLOC if realloc_if_needed the parser does not have enough capacity, and memory
   *           allocation fails.
   *         - EMPTY if the document is all whitespace.
   *         - UTF8_ERROR if the document is not valid UTF-8.
   *         - UNESCAPED_CHARS if a string contains control characters that must be escaped
   *         - UNCLOSED_STRING if there is an unclosed string in the document.
   */
  simdjson_warn_unused simdjson_result<json_iterator> iterate_raw(padded_string_view json) & noexcept;

  /** The capacity of this parser (the largest document it can process). */
  simdjson_really_inline size_t capacity() const noexcept;
  /** The maximum depth of this parser (the most deeply nested objects and arrays it can process). */
  simdjson_really_inline size_t max_depth() const noexcept;

private:
  /** @private [for benchmarking access] The implementation to use */
  std::unique_ptr<internal::dom_parser_implementation> implementation{};
  size_t _capacity{0};
  size_t _max_depth{DEFAULT_MAX_DEPTH};
  std::unique_ptr<uint8_t[]> string_buf{};
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  std::unique_ptr<token_position[]> start_positions{};
#endif

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  simdjson_warn_unused error_code allocate(size_t capacity, size_t max_depth=DEFAULT_MAX_DEPTH) noexcept;

  friend class json_iterator;
};

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser> : public SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;
};

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/parser.h */
/* end file include/simdjson/generic/ondemand.h */

// Inline definitions
/* begin file include/simdjson/generic/implementation_simdjson_result_base-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {

//
// internal::implementation_simdjson_result_base<T> inline implementation
//

template<typename T>
simdjson_really_inline void implementation_simdjson_result_base<T>::tie(T &value, error_code &error) && noexcept {
  error = this->second;
  if (!error) {
    value = std::forward<implementation_simdjson_result_base<T>>(*this).first;
  }
}

template<typename T>
simdjson_warn_unused simdjson_really_inline error_code implementation_simdjson_result_base<T>::get(T &value) && noexcept {
  error_code error;
  std::forward<implementation_simdjson_result_base<T>>(*this).tie(value, error);
  return error;
}

template<typename T>
simdjson_really_inline error_code implementation_simdjson_result_base<T>::error() const noexcept {
  return this->second;
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_really_inline T& implementation_simdjson_result_base<T>::value() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return this->first;
}

template<typename T>
simdjson_really_inline T&& implementation_simdjson_result_base<T>::value() && noexcept(false) {
  return std::forward<implementation_simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
simdjson_really_inline T&& implementation_simdjson_result_base<T>::take_value() && noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return std::forward<T>(this->first);
}

template<typename T>
simdjson_really_inline implementation_simdjson_result_base<T>::operator T&&() && noexcept(false) {
  return std::forward<implementation_simdjson_result_base<T>>(*this).take_value();
}

template<typename T>
simdjson_really_inline const T& implementation_simdjson_result_base<T>::value_unsafe() const& noexcept {
  return this->first;
}

template<typename T>
simdjson_really_inline T&& implementation_simdjson_result_base<T>::value_unsafe() && noexcept {
  return std::forward<T>(this->first);
}

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
simdjson_really_inline implementation_simdjson_result_base<T>::implementation_simdjson_result_base(T &&value, error_code error) noexcept
    : first{std::forward<T>(value)}, second{error} {}
template<typename T>
simdjson_really_inline implementation_simdjson_result_base<T>::implementation_simdjson_result_base(error_code error) noexcept
    : implementation_simdjson_result_base(T{}, error) {}
template<typename T>
simdjson_really_inline implementation_simdjson_result_base<T>::implementation_simdjson_result_base(T &&value) noexcept
    : implementation_simdjson_result_base(std::forward<T>(value), SUCCESS) {}

} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson
/* end file include/simdjson/generic/implementation_simdjson_result_base-inl.h */
/* begin file include/simdjson/generic/ondemand-inl.h */
/* begin file include/simdjson/generic/ondemand/json_type-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

inline std::ostream& operator<<(std::ostream& out, json_type type) noexcept {
    switch (type) {
        case json_type::array: out << "array"; break;
        case json_type::object: out << "object"; break;
        case json_type::number: out << "number"; break;
        case json_type::string: out << "string"; break;
        case json_type::boolean: out << "boolean"; break;
        case json_type::null: out << "null"; break;
        default: SIMDJSON_UNREACHABLE();
    }
    return out;
}

#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson_result<json_type> &type) noexcept(false) {
    return out << type.value();
}
#endif

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type>::simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type>(error) {}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/json_type-inl.h */
/* begin file include/simdjson/generic/ondemand/logger-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {
namespace logger {

static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
static constexpr const int LOG_EVENT_LEN = 20;
static constexpr const int LOG_BUFFER_LEN = 30;
static constexpr const int LOG_SMALL_BUFFER_LEN = 10;
static int log_depth = 0; // Not threadsafe. Log only.

// Helper to turn unprintable or newline characters into spaces
static simdjson_really_inline char printable_char(char c) {
  if (c >= 0x20) {
    return c;
  } else {
    return ' ';
  }
}

simdjson_really_inline void log_event(const json_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_line(iter, "", type, detail, delta, depth_delta);
}

simdjson_really_inline void log_value(const json_iterator &iter, token_position index, depth_t depth, const char *type, std::string_view detail) noexcept {
  log_line(iter, index, depth, "", type, detail);
}
simdjson_really_inline void log_value(const json_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_line(iter, "", type, detail, delta, depth_delta);
}

simdjson_really_inline void log_start_value(const json_iterator &iter, token_position index, depth_t depth, const char *type, std::string_view detail) noexcept {
  log_line(iter, index, depth, "+", type, detail);
  if (LOG_ENABLED) { log_depth++; }
}
simdjson_really_inline void log_start_value(const json_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  log_line(iter, "+", type, "", delta, depth_delta);
  if (LOG_ENABLED) { log_depth++; }
}

simdjson_really_inline void log_end_value(const json_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  if (LOG_ENABLED) { log_depth--; }
  log_line(iter, "-", type, "", delta, depth_delta);
}

simdjson_really_inline void log_error(const json_iterator &iter, const char *error, const char *detail, int delta, int depth_delta) noexcept {
  log_line(iter, "ERROR: ", error, detail, delta, depth_delta);
}
simdjson_really_inline void log_error(const json_iterator &iter, token_position index, depth_t depth, const char *error, const char *detail) noexcept {
  log_line(iter, index, depth, "ERROR: ", error, detail);
}

simdjson_really_inline void log_event(const value_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_event(iter.json_iter(), type, detail, delta, depth_delta);
}

simdjson_really_inline void log_value(const value_iterator &iter, const char *type, std::string_view detail, int delta, int depth_delta) noexcept {
  log_value(iter.json_iter(), type, detail, delta, depth_delta);
}

simdjson_really_inline void log_start_value(const value_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  log_start_value(iter.json_iter(), type, delta, depth_delta);
}

simdjson_really_inline void log_end_value(const value_iterator &iter, const char *type, int delta, int depth_delta) noexcept {
  log_end_value(iter.json_iter(), type, delta, depth_delta);
}

simdjson_really_inline void log_error(const value_iterator &iter, const char *error, const char *detail, int delta, int depth_delta) noexcept {
  log_error(iter.json_iter(), error, detail, delta, depth_delta);
}

simdjson_really_inline void log_headers() noexcept {
  if (LOG_ENABLED) {
    log_depth = 0;
    printf("\n");
    printf("| %-*s ", LOG_EVENT_LEN,        "Event");
    printf("| %-*s ", LOG_BUFFER_LEN,       "Buffer");
    printf("| %-*s ", LOG_SMALL_BUFFER_LEN, "Next");
    // printf("| %-*s ", 5,                    "Next#");
    printf("| %-*s ", 5,                    "Depth");
    printf("| Detail ");
    printf("|\n");

    printf("|%.*s", LOG_EVENT_LEN+2, DASHES);
    printf("|%.*s", LOG_BUFFER_LEN+2, DASHES);
    printf("|%.*s", LOG_SMALL_BUFFER_LEN+2, DASHES);
    // printf("|%.*s", 5+2, DASHES);
    printf("|%.*s", 5+2, DASHES);
    printf("|--------");
    printf("|\n");
    fflush(stdout);
  }
}

simdjson_really_inline void log_line(const json_iterator &iter, const char *title_prefix, const char *title, std::string_view detail, int delta, int depth_delta) noexcept {
  log_line(iter, iter.token.index+delta, depth_t(iter.depth()+depth_delta), title_prefix, title, detail);
}
simdjson_really_inline void log_line(const json_iterator &iter, token_position index, depth_t depth, const char *title_prefix, const char *title, std::string_view detail) noexcept {
  if (LOG_ENABLED) {
    const int indent = depth*2;
    const auto buf = iter.token.buf;
    printf("| %*s%s%-*s ",
      indent, "",
      title_prefix,
      LOG_EVENT_LEN - indent - int(strlen(title_prefix)), title
      );
    {
      // Print the current structural.
      printf("| ");
      auto current_structural = &buf[*index];
      for (int i=0;i<LOG_BUFFER_LEN;i++) {
        printf("%c", printable_char(current_structural[i]));
      }
      printf(" ");
    }
    {
      // Print the next structural.
      printf("| ");
      auto next_structural = &buf[*(index+1)];
      for (int i=0;i<LOG_SMALL_BUFFER_LEN;i++) {
        printf("%c", printable_char(next_structural[i]));
      }
      printf(" ");
    }
    // printf("| %5u ", *(index+1));
    printf("| %5u ", depth);
    printf("| %.*s ", int(detail.size()), detail.data());
    printf("|\n");
    fflush(stdout);
  }
}

} // namespace logger
} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson
/* end file include/simdjson/generic/ondemand/logger-inl.h */
/* begin file include/simdjson/generic/ondemand/raw_json_string-inl.h */
namespace simdjson {

namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline raw_json_string::raw_json_string(const uint8_t * _buf) noexcept : buf{_buf} {}

simdjson_really_inline const char * raw_json_string::raw() const noexcept { return reinterpret_cast<const char *>(buf); }
simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> raw_json_string::unescape(uint8_t *&dst) const noexcept {
  uint8_t *end = stringparsing::parse_string(buf, dst);
  if (!end) { return STRING_ERROR; }
  std::string_view result(reinterpret_cast<const char *>(dst), end-dst);
  dst = end;
  return result;
}

simdjson_really_inline bool raw_json_string::is_free_from_unescaped_quote(std::string_view target) noexcept {
  size_t pos{0};
  // if the content has no escape character, just scan through it quickly!
  for(;pos < target.size() && target[pos] != '\\';pos++) {}
  // slow path may begin.
  bool escaping{false};
  for(;pos < target.size();pos++) {
    if((target[pos] == '"') && !escaping) {
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  return true;
}

simdjson_really_inline bool raw_json_string::is_free_from_unescaped_quote(const char* target) noexcept {
  size_t pos{0};
  // if the content has no escape character, just scan through it quickly!
  for(;target[pos] && target[pos] != '\\';pos++) {}
  // slow path may begin.
  bool escaping{false};
  for(;target[pos];pos++) {
    if((target[pos] == '"') && !escaping) {
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  return true;
}


simdjson_really_inline bool raw_json_string::unsafe_is_equal(size_t length, std::string_view target) const noexcept {
  // If we are going to call memcmp, then we must know something about the length of the raw_json_string.
  return (length >= target.size()) && (raw()[target.size()] == '"') && !memcmp(raw(), target.data(), target.size());
}

simdjson_really_inline bool raw_json_string::unsafe_is_equal(std::string_view target) const noexcept {
  // Assumptions: does not contain unescaped quote characters, and
  // the raw content is quote terminated within a valid JSON string.
  if(target.size() <= SIMDJSON_PADDING) {
    return (raw()[target.size()] == '"') && !memcmp(raw(), target.data(), target.size());
  }
  const char * r{raw()};
  size_t pos{0};
  for(;pos < target.size();pos++) {
    if(r[pos] != target[pos]) { return false; }
  }
  if(r[pos] != '"') { return false; }
  return true;
}

simdjson_really_inline bool raw_json_string::is_equal(std::string_view target) const noexcept {
  const char * r{raw()};
  size_t pos{0};
  bool escaping{false};
  for(;pos < target.size();pos++) {
    if(r[pos] != target[pos]) { return false; }
    // if target is a compile-time constant and it is free from
    // quotes, then the next part could get optimized away through
    // inlining.
    if((target[pos] == '"') && !escaping) {
      // We have reached the end of the raw_json_string but
      // the target is not done.
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  if(r[pos] != '"') { return false; }
  return true;
}


simdjson_really_inline bool raw_json_string::unsafe_is_equal(const char * target) const noexcept {
  // Assumptions: 'target' does not contain unescaped quote characters, is null terminated and
  // the raw content is quote terminated within a valid JSON string.
  const char * r{raw()};
  size_t pos{0};
  for(;target[pos];pos++) {
    if(r[pos] != target[pos]) { return false; }
  }
  if(r[pos] != '"') { return false; }
  return true;
}

simdjson_really_inline bool raw_json_string::is_equal(const char* target) const noexcept {
  // Assumptions: does not contain unescaped quote characters, and
  // the raw content is quote terminated within a valid JSON string.
  const char * r{raw()};
  size_t pos{0};
  bool escaping{false};
  for(;target[pos];pos++) {
    if(r[pos] != target[pos]) { return false; }
    // if target is a compile-time constant and it is free from
    // quotes, then the next part could get optimized away through
    // inlining.
    if((target[pos] == '"') && !escaping) {
      // We have reached the end of the raw_json_string but
      // the target is not done.
      return false;
    } else if(target[pos] == '\\') {
      escaping = !escaping;
    } else {
      escaping = false;
    }
  }
  if(r[pos] != '"') { return false; }
  return true;
}

simdjson_unused simdjson_really_inline bool operator==(const raw_json_string &a, std::string_view c) noexcept {
  return a.unsafe_is_equal(c);
}

simdjson_unused simdjson_really_inline bool operator==(std::string_view c, const raw_json_string &a) noexcept {
  return a == c;
}

simdjson_unused simdjson_really_inline bool operator!=(const raw_json_string &a, std::string_view c) noexcept {
  return !(a == c);
}

simdjson_unused simdjson_really_inline bool operator!=(std::string_view c, const raw_json_string &a) noexcept {
  return !(a == c);
}


simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> raw_json_string::unescape(json_iterator &iter) const noexcept {
  return unescape(iter.string_buf_loc());
}


simdjson_unused simdjson_really_inline std::ostream &operator<<(std::ostream &out, const raw_json_string &str) noexcept {
  bool in_escape = false;
  const char *s = str.raw();
  while (true) {
    switch (*s) {
      case '\\': in_escape = !in_escape; break;
      case '"': if (in_escape) { in_escape = false; } else { return out; } break;
      default: if (in_escape) { in_escape = false; }
    }
    out << *s;
    s++;
  }
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>::simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>(error) {}

simdjson_really_inline simdjson_result<const char *> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>::raw() const noexcept {
  if (error()) { return error(); }
  return first.raw();
}
simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>::unescape(uint8_t *&dst) const noexcept {
  if (error()) { return error(); }
  return first.unescape(dst);
}
simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string>::unescape(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator &iter) const noexcept {
  if (error()) { return error(); }
  return first.unescape(iter);
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/raw_json_string-inl.h */
/* begin file include/simdjson/generic/ondemand/token_iterator-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline token_iterator::token_iterator(const uint8_t *_buf, token_position _index) noexcept
  : buf{_buf}, index{_index}
{
}

simdjson_really_inline const uint8_t *token_iterator::advance() noexcept {
  return &buf[*(index++)];
}

simdjson_really_inline const uint8_t *token_iterator::peek(token_position position) const noexcept {
  return &buf[*position];
}
simdjson_really_inline uint32_t token_iterator::peek_index(token_position position) const noexcept {
  return *position;
}
simdjson_really_inline uint32_t token_iterator::peek_length(token_position position) const noexcept {
  return *(position+1) - *position;
}

simdjson_really_inline const uint8_t *token_iterator::peek(int32_t delta) const noexcept {
  return &buf[*(index+delta)];
}
simdjson_really_inline uint32_t token_iterator::peek_index(int32_t delta) const noexcept {
  return *(index+delta);
}
simdjson_really_inline uint32_t token_iterator::peek_length(int32_t delta) const noexcept {
  return *(index+delta+1) - *(index+delta);
}

simdjson_really_inline token_position token_iterator::position() const noexcept {
  return index;
}
simdjson_really_inline void token_iterator::set_position(token_position target_checkpoint) noexcept {
  index = target_checkpoint;
}

simdjson_really_inline bool token_iterator::operator==(const token_iterator &other) const noexcept {
  return index == other.index;
}
simdjson_really_inline bool token_iterator::operator!=(const token_iterator &other) const noexcept {
  return index != other.index;
}
simdjson_really_inline bool token_iterator::operator>(const token_iterator &other) const noexcept {
  return index > other.index;
}
simdjson_really_inline bool token_iterator::operator>=(const token_iterator &other) const noexcept {
  return index >= other.index;
}
simdjson_really_inline bool token_iterator::operator<(const token_iterator &other) const noexcept {
  return index < other.index;
}
simdjson_really_inline bool token_iterator::operator<=(const token_iterator &other) const noexcept {
  return index <= other.index;
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator>::simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::token_iterator>(error) {}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/token_iterator-inl.h */
/* begin file include/simdjson/generic/ondemand/json_iterator-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline json_iterator::json_iterator(json_iterator &&other) noexcept
  : token(std::forward<token_iterator>(other.token)),
    parser{other.parser},
    _string_buf_loc{other._string_buf_loc},
    _depth{other._depth}
{
  other.parser = nullptr;
}
simdjson_really_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept {
  token = other.token;
  parser = other.parser;
  _string_buf_loc = other._string_buf_loc;
  _depth = other._depth;
  other.parser = nullptr;
  return *this;
}

simdjson_really_inline json_iterator::json_iterator(const uint8_t *buf, ondemand::parser *_parser) noexcept
  : token(buf, _parser->implementation->structural_indexes.get()),
    parser{_parser},
    _string_buf_loc{parser->string_buf.get()},
    _depth{1}
{
  logger::log_headers();
}

// GCC 7 warns when the first line of this function is inlined away into oblivion due to the caller
// relating depth and parent_depth, which is a desired effect. The warning does not show up if the
// skip_child() function is not marked inline).
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_warn_unused simdjson_really_inline error_code json_iterator::skip_child(depth_t parent_depth) noexcept {
  if (depth() <= parent_depth) { return SUCCESS; }

  switch (*advance()) {
    // TODO consider whether matching braces is a requirement: if non-matching braces indicates
    // *missing* braces, then future lookups are not in the object/arrays they think they are,
    // violating the rule "validate enough structure that the user can be confident they are
    // looking at the right values."
    // PERF TODO we can eliminate the switch here with a lookup of how much to add to depth

    // For the first open array/object in a value, we've already incremented depth, so keep it the same
    // We never stop at colon, but if we did, it wouldn't affect depth
    case '[': case '{': case ':':
      logger::log_start_value(*this, "skip");
      break;
    // If there is a comma, we have just finished a value in an array/object, and need to get back in
    case ',':
      logger::log_value(*this, "skip");
      break;
    // ] or } means we just finished a value and need to jump out of the array/object
    case ']': case '}':
      logger::log_end_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
      break;
    case '"':
      if(*peek() == ':') {
        // we are at a key!!! This is
        // only possible if someone searched
        // for a key and the key was not found.
        logger::log_value(*this, "key");
        advance(); // eat up the ':'
        break; // important!!!
      }
      simdjson_fallthrough;
    // Anything else must be a scalar value
    default:
      // For the first scalar, we will have incremented depth already, so we decrement it here.
      logger::log_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
      break;
  }

  // Now that we've considered the first value, we only increment/decrement for arrays/objects
  auto end = &parser->implementation->structural_indexes[parser->implementation->n_structural_indexes];
  while (token.index <= end) {
    switch (*advance()) {
      case '[': case '{':
        logger::log_start_value(*this, "skip");
        _depth++;
        break;
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      // PERF TODO we can eliminate the switch here with a lookup of how much to add to depth
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        _depth--;
        if (depth() <= parent_depth) { return SUCCESS; }
        break;
      default:
        logger::log_value(*this, "skip", "");
        break;
    }
  }

  return report_error(TAPE_ERROR, "not enough close braces");
}

SIMDJSON_POP_DISABLE_WARNINGS

simdjson_really_inline bool json_iterator::at_root() const noexcept {
  return token.position() == root_checkpoint();
}

simdjson_really_inline token_position json_iterator::root_checkpoint() const noexcept {
  return parser->implementation->structural_indexes.get();
}

simdjson_really_inline void json_iterator::assert_at_root() const noexcept {
  SIMDJSON_ASSUME( _depth == 1 );
  // Visual Studio Clang treats unique_ptr.get() as "side effecting."
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME( token.index == parser->implementation->structural_indexes.get() );
#endif
}

simdjson_really_inline bool json_iterator::at_eof() const noexcept {
  return token.index == &parser->implementation->structural_indexes[parser->implementation->n_structural_indexes];
}

simdjson_really_inline bool json_iterator::is_alive() const noexcept {
  return parser;
}

simdjson_really_inline void json_iterator::abandon() noexcept {
  parser = nullptr;
  _depth = 0;
}

simdjson_really_inline const uint8_t *json_iterator::advance() noexcept {
  return token.advance();
}

simdjson_really_inline const uint8_t *json_iterator::peek(int32_t delta) const noexcept {
  return token.peek(delta);
}

simdjson_really_inline uint32_t json_iterator::peek_length(int32_t delta) const noexcept {
  return token.peek_length(delta);
}

simdjson_really_inline const uint8_t *json_iterator::peek(token_position position) const noexcept {
  return token.peek(position);
}

simdjson_really_inline uint32_t json_iterator::peek_length(token_position position) const noexcept {
  return token.peek_length(position);
}

simdjson_really_inline token_position json_iterator::last_document_position() const noexcept {
  // The following line fails under some compilers...
  // SIMDJSON_ASSUME(parser->implementation->n_structural_indexes > 0);
  // since it has side-effects.
  uint32_t n_structural_indexes{parser->implementation->n_structural_indexes};
  SIMDJSON_ASSUME(n_structural_indexes > 0);
  return &parser->implementation->structural_indexes[n_structural_indexes - 1];
}
simdjson_really_inline const uint8_t *json_iterator::peek_last() const noexcept {
  return token.peek(last_document_position());
}

simdjson_really_inline void json_iterator::ascend_to(depth_t parent_depth) noexcept {
  SIMDJSON_ASSUME(parent_depth >= 0 && parent_depth < INT32_MAX - 1);
  SIMDJSON_ASSUME(_depth == parent_depth + 1);
  _depth = parent_depth;
}

simdjson_really_inline void json_iterator::descend_to(depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
  _depth = child_depth;
}

simdjson_really_inline depth_t json_iterator::depth() const noexcept {
  return _depth;
}

simdjson_really_inline uint8_t *&json_iterator::string_buf_loc() noexcept {
  return _string_buf_loc;
}

simdjson_really_inline error_code json_iterator::report_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error != SUCCESS && _error != UNINITIALIZED && _error != INCORRECT_TYPE && _error != NO_SUCH_FIELD);
  logger::log_error(*this, message);
  error = _error;
  return error;
}

simdjson_really_inline token_position json_iterator::position() const noexcept {
  return token.position();
}
simdjson_really_inline void json_iterator::reenter_child(token_position position, depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME(position >= parser->start_positions[child_depth]);
#endif
#endif
  token.set_position(position);
  _depth = child_depth;
}

#ifdef SIMDJSON_DEVELOPMENT_CHECKS
simdjson_really_inline token_position json_iterator::start_position(depth_t depth) const noexcept {
  return parser->start_positions[depth];
}
simdjson_really_inline void json_iterator::set_start_position(depth_t depth, token_position position) noexcept {
  parser->start_positions[depth] = position;
}

#endif


simdjson_really_inline error_code json_iterator::optional_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error == INCORRECT_TYPE || _error == NO_SUCH_FIELD);
  logger::log_error(*this, message);
  return _error;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t (&tmpbuf)[N]) noexcept {
  // Truncate whitespace to fit the buffer.
  if (max_len > N-1) {
    if (jsoncharutils::is_not_structural_or_whitespace(json[N-1])) { return false; }
    max_len = N-1;
  }

  // Copy to the buffer.
  std::memcpy(tmpbuf, json, max_len);
  tmpbuf[max_len] = ' ';
  return true;
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::peek_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  auto max_len = token.peek_length();
  auto json = token.peek();
  return copy_to_buffer(json, max_len, tmpbuf);
}

template<int N>
simdjson_warn_unused simdjson_really_inline bool json_iterator::advance_to_buffer(uint8_t (&tmpbuf)[N]) noexcept {
  auto max_len = peek_length();
  auto json = advance();
  return copy_to_buffer(json, max_len, tmpbuf);
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_iterator>(error) {}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/json_iterator-inl.h */
/* begin file include/simdjson/generic/ondemand/value_iterator-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline value_iterator::value_iterator(json_iterator *json_iter, depth_t depth, token_position start_index) noexcept
  : _json_iter{json_iter},
    _depth{depth},
    _start_position{start_index}
{
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_object() noexcept {
  const uint8_t *json;
  SIMDJSON_TRY( advance_container_start("object", json) );
  if (*json != '{') { return incorrect_type_error("Not an object"); }
  return started_object();
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_root_object() noexcept {
  bool result;
  SIMDJSON_TRY( start_object().get(result) );
  if (*_json_iter->peek_last() != '}') { return _json_iter->report_error(TAPE_ERROR, "object invalid: { at beginning of document unmatched by } at end of document"); }
  return result;
}

simdjson_warn_unused simdjson_really_inline bool value_iterator::started_object() noexcept {
  assert_at_container_start();
  if (*_json_iter->peek() == '}') {
    logger::log_value(*_json_iter, "empty object");
    _json_iter->advance();
    _json_iter->ascend_to(depth()-1);
    return false;
  }
  logger::log_start_value(*_json_iter, "object");
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  _json_iter->set_start_position(_depth, _start_position);
#endif
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::has_next_field() noexcept {
  assert_at_next();

  switch (*_json_iter->advance()) {
    case '}':
      logger::log_end_value(*_json_iter, "object");
      _json_iter->ascend_to(depth()-1);
      return false;
    case ',':
      return true;
    default:
      return _json_iter->report_error(TAPE_ERROR, "Missing comma between object fields");
  }
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::find_field_raw(const std::string_view key) noexcept {
  error_code error;
  bool has_value;
  //
  // Initially, the object can be in one of a few different places:
  //
  // 1. The start of the object, at the first field:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //      ^ (depth 2, index 1)
  //    ```
  //
  if (at_first_field()) {
    has_value = true;

  //
  // 2. When a previous search did not yield a value or the object is empty:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                     ^ (depth 0)
  //    { }
  //        ^ (depth 0, index 2)
  //    ```
  //
  } else if (!is_open()) {
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
    // If we're past the end of the object, we're being iterated out of order.
    // Note: this isn't perfect detection. It's possible the user is inside some other object; if so,
    // this object iterator will blithely scan that object for fields.
    if (_json_iter->depth() < depth() - 1) { return OUT_OF_ORDER_ITERATION; }
#endif
    has_value = false;

  // 3. When a previous search found a field or an iterator yielded a value:
  //
  //    ```
  //    // When a field was not fully consumed (or not even touched at all)
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //           ^ (depth 2)
  //    // When a field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                   ^ (depth 1)
  //    // When the last field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                   ^ (depth 1)
  //    ```
  //
  } else {
    if ((error = skip_child() )) { abandon(); return error; }
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
    if (_json_iter->start_position(_depth) != _start_position) { return OUT_OF_ORDER_ITERATION; }
#endif
  }
  while (has_value) {
    // Get the key and colon, stopping at the value.
    raw_json_string actual_key;
    // size_t max_key_length = _json_iter->peek_length() - 2; // -2 for the two quotes
    if ((error = field_key().get(actual_key) )) { abandon(); return error; };

    if ((error = field_value() )) { abandon(); return error; }
    // If it matches, stop and return
    // We could do it this way if we wanted to allow arbitrary
    // key content (including escaped quotes).
    //if (actual_key.unsafe_is_equal(max_key_length, key)) {
    // Instead we do the following which may trigger buffer overruns if the
    // user provides an adversarial key (containing a well placed unescaped quote
    // character and being longer than the number of bytes remaining in the JSON
    // input).
    if (actual_key.unsafe_is_equal(key)) {
      logger::log_event(*this, "match", key, -2);
      return true;
    }

    // No match: skip the value and see if , or } is next
    logger::log_event(*this, "no match", key, -2);
    SIMDJSON_TRY( skip_child() ); // Skip the value entirely
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
  }

  // If the loop ended, we're out of fields to look at.
  return false;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::find_field_unordered_raw(const std::string_view key) noexcept {
  error_code error;
  bool has_value;
  //
  // Initially, the object can be in one of a few different places:
  //
  // 1. The start of the object, at the first field:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //      ^ (depth 2, index 1)
  //    ```
  //
  if (at_first_field()) {
    // If we're at the beginning of the object, we definitely have a field
    has_value = true;

  // 2. When a previous search did not yield a value or the object is empty:
  //
  //    ```
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                     ^ (depth 0)
  //    { }
  //        ^ (depth 0, index 2)
  //    ```
  //
  } else if (!is_open()) {
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
    // If we're past the end of the object, we're being iterated out of order.
    // Note: this isn't perfect detection. It's possible the user is inside some other object; if so,
    // this object iterator will blithely scan that object for fields.
    if (_json_iter->depth() < depth() - 1) { return OUT_OF_ORDER_ITERATION; }
#endif
    has_value = false;

  // 3. When a previous search found a field or an iterator yielded a value:
  //
  //    ```
  //    // When a field was not fully consumed (or not even touched at all)
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //           ^ (depth 2)
  //    // When a field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                   ^ (depth 1)
  //    // When the last field was fully consumed
  //    { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                   ^ (depth 1)
  //    ```
  //
  } else {
    // Finish the previous value and see if , or } is next
    if ((error = skip_child() )) { abandon(); return error; }
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
    if (_json_iter->start_position(_depth) != _start_position) { return OUT_OF_ORDER_ITERATION; }
#endif
  }

  // After initial processing, we will be in one of two states:
  //
  // ```
  // // At the beginning of a field
  // { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //   ^ (depth 1)
  // { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                  ^ (depth 1)
  // // At the end of the object
  // { "a": [ 1, 2 ], "b": [ 3, 4 ] }
  //                                  ^ (depth 0)
  // ```
  //

  // First, we scan from that point to the end.
  // If we don't find a match, we loop back around, and scan from the beginning to that point.
  token_position search_start = _json_iter->position();

  // Next, we find a match starting from the current position.
  while (has_value) {
    SIMDJSON_ASSUME( _json_iter->_depth == _depth ); // We must be at the start of a field

    // Get the key and colon, stopping at the value.
    raw_json_string actual_key;
    // size_t max_key_length = _json_iter->peek_length() - 2; // -2 for the two quotes

    if ((error = field_key().get(actual_key) )) { abandon(); return error; };
    if ((error = field_value() )) { abandon(); return error; }

    // If it matches, stop and return
    // We could do it this way if we wanted to allow arbitrary
    // key content (including escaped quotes).
    // if (actual_key.unsafe_is_equal(max_key_length, key)) {
    // Instead we do the following which may trigger buffer overruns if the
    // user provides an adversarial key (containing a well placed unescaped quote
    // character and being longer than the number of bytes remaining in the JSON
    // input).
    if (actual_key.unsafe_is_equal(key)) {
      logger::log_event(*this, "match", key, -2);
      return true;
    }

    // No match: skip the value and see if , or } is next
    logger::log_event(*this, "no match", key, -2);
    SIMDJSON_TRY( skip_child() );
    if ((error = has_next_field().get(has_value) )) { abandon(); return error; }
  }

  // If we reach the end without finding a match, search the rest of the fields starting at the
  // beginning of the object.
  // (We have already run through the object before, so we've already validated its structure. We
  // don't check errors in this bit.)
  _json_iter->reenter_child(_start_position + 1, _depth);

  has_value = started_object();
  while (_json_iter->position() < search_start) {
    SIMDJSON_ASSUME(has_value); // we should reach search_start before ever reaching the end of the object
    SIMDJSON_ASSUME( _json_iter->_depth == _depth ); // We must be at the start of a field

    // Get the key and colon, stopping at the value.
    raw_json_string actual_key;
    // size_t max_key_length = _json_iter->peek_length() - 2; // -2 for the two quotes

    error = field_key().get(actual_key); SIMDJSON_ASSUME(!error);
    error = field_value(); SIMDJSON_ASSUME(!error);

    // If it matches, stop and return
    // We could do it this way if we wanted to allow arbitrary
    // key content (including escaped quotes).
    // if (actual_key.unsafe_is_equal(max_key_length, key)) {
    // Instead we do the following which may trigger buffer overruns if the
    // user provides an adversarial key (containing a well placed unescaped quote
    // character and being longer than the number of bytes remaining in the JSON
    // input).
    if (actual_key.unsafe_is_equal(key)) {
      logger::log_event(*this, "match", key, -2);
      return true;
    }

    // No match: skip the value and see if , or } is next
    logger::log_event(*this, "no match", key, -2);
    SIMDJSON_TRY( skip_child() );
    error = has_next_field().get(has_value); SIMDJSON_ASSUME(!error);
  }

  // If the loop ended, we're out of fields to look at.
  return false;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::field_key() noexcept {
  assert_at_next();

  const uint8_t *key = _json_iter->advance();
  if (*(key++) != '"') { return _json_iter->report_error(TAPE_ERROR, "Object key is not a string"); }
  return raw_json_string(key);
}

simdjson_warn_unused simdjson_really_inline error_code value_iterator::field_value() noexcept {
  assert_at_next();

  if (*_json_iter->advance() != ':') { return _json_iter->report_error(TAPE_ERROR, "Missing colon in object field"); }
  _json_iter->descend_to(depth()+1);
  return SUCCESS;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_array() noexcept {
  const uint8_t *json;
  SIMDJSON_TRY( advance_container_start("array", json) );
  if (*json != '[') { return incorrect_type_error("Not an array"); }
  return started_array();
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::start_root_array() noexcept {
  bool result;
  SIMDJSON_TRY( start_array().get(result) );
  if (*_json_iter->peek_last() != ']') { return _json_iter->report_error(TAPE_ERROR, "array invalid: [ at beginning of document unmatched by ] at end of document"); }
  return result;
}

simdjson_warn_unused simdjson_really_inline bool value_iterator::started_array() noexcept {
  assert_at_container_start();
  if (*_json_iter->peek() == ']') {
    logger::log_value(*_json_iter, "empty array");
    _json_iter->advance();
    _json_iter->ascend_to(depth()-1);
    return false;
  }
  logger::log_start_value(*_json_iter, "array");
  _json_iter->descend_to(depth()+1);
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  _json_iter->set_start_position(_depth, _start_position);
#endif
  return true;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::has_next_element() noexcept {
  assert_at_next();

  switch (*_json_iter->advance()) {
    case ']':
      logger::log_end_value(*_json_iter, "array");
      _json_iter->ascend_to(depth()-1);
      return false;
    case ',':
      _json_iter->descend_to(depth()+1);
      return true;
    default:
      return _json_iter->report_error(TAPE_ERROR, "Missing comma between array elements");
  }
}

simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::parse_bool(const uint8_t *json) const noexcept {
  auto not_true = atomparsing::str4ncmp(json, "true");
  auto not_false = atomparsing::str4ncmp(json, "fals") | (json[4] ^ 'e');
  bool error = (not_true && not_false) || jsoncharutils::is_not_structural_or_whitespace(json[not_true ? 5 : 4]);
  if (error) { return incorrect_type_error("Not a boolean"); }
  return simdjson_result<bool>(!not_true);
}
simdjson_really_inline bool value_iterator::parse_null(const uint8_t *json) const noexcept {
  return !atomparsing::str4ncmp(json, "null") && jsoncharutils::is_structural_or_whitespace(json[4]);
}

simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> value_iterator::get_string() noexcept {
  return get_raw_json_string().unescape(_json_iter->string_buf_loc());
}
simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::get_raw_json_string() noexcept {
  auto json = advance_start("string");
  if (*json != '"') { return incorrect_type_error("Not a string"); }
  return raw_json_string(json+1);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::get_uint64() noexcept {
  return numberparsing::parse_unsigned(advance_non_root_scalar("uint64"));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::get_int64() noexcept {
  return numberparsing::parse_integer(advance_non_root_scalar("int64"));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::get_double() noexcept {
  return numberparsing::parse_double(advance_non_root_scalar("double"));
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::get_bool() noexcept {
  return parse_bool(advance_non_root_scalar("bool"));
}
simdjson_really_inline bool value_iterator::is_null() noexcept {
  return parse_null(advance_non_root_scalar("null"));
}

constexpr const uint32_t MAX_INT_LENGTH = 1024;

simdjson_warn_unused simdjson_really_inline simdjson_result<std::string_view> value_iterator::get_root_string() noexcept {
  return get_string();
}
simdjson_warn_unused simdjson_really_inline simdjson_result<raw_json_string> value_iterator::get_root_raw_json_string() noexcept {
  return get_raw_json_string();
}
simdjson_warn_unused simdjson_really_inline simdjson_result<uint64_t> value_iterator::get_root_uint64() noexcept {
  auto max_len = peek_start_length();
  auto json = advance_root_scalar("uint64");
  uint8_t tmpbuf[20+1]; // <20 digits> is the longest possible unsigned integer
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { logger::log_error(*_json_iter, _start_position, depth(), "Root number more than 20 characters"); return NUMBER_ERROR; }
  return numberparsing::parse_unsigned(tmpbuf);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<int64_t> value_iterator::get_root_int64() noexcept {
  auto max_len = peek_start_length();
  auto json = advance_root_scalar("int64");
  uint8_t tmpbuf[20+1]; // -<19 digits> is the longest possible integer
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { logger::log_error(*_json_iter, _start_position, depth(), "Root number more than 20 characters"); return NUMBER_ERROR; }
  return numberparsing::parse_integer(tmpbuf);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<double> value_iterator::get_root_double() noexcept {
  auto max_len = peek_start_length();
  auto json = advance_root_scalar("double");
  // Per https://www.exploringbinary.com/maximum-number-of-decimal-digits-in-binary-floating-point-numbers/, 1074 is the maximum number of significant fractional digits. Add 8 more digits for the biggest number: -0.<fraction>e-308.
  uint8_t tmpbuf[1074+8+1];
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { logger::log_error(*_json_iter, _start_position, depth(), "Root number more than 1082 characters"); return NUMBER_ERROR; }
  return numberparsing::parse_double(tmpbuf);
}
simdjson_warn_unused simdjson_really_inline simdjson_result<bool> value_iterator::get_root_bool() noexcept {
  auto max_len = peek_start_length();
  auto json = advance_root_scalar("bool");
  uint8_t tmpbuf[5+1];
  if (!_json_iter->copy_to_buffer(json, max_len, tmpbuf)) { return incorrect_type_error("Not a boolean"); }
  return parse_bool(tmpbuf);
}
simdjson_really_inline bool value_iterator::is_root_null() noexcept {
  auto max_len = peek_start_length();
  auto json = advance_root_scalar("null");
  return max_len >= 4 && !atomparsing::str4ncmp(json, "null") &&
         (max_len == 4 || jsoncharutils::is_structural_or_whitespace(json[5]));
}

simdjson_warn_unused simdjson_really_inline error_code value_iterator::skip_child() noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_position );
  SIMDJSON_ASSUME( _json_iter->_depth >= _depth );

  return _json_iter->skip_child(depth());
}

simdjson_really_inline value_iterator value_iterator::child() const noexcept {
  assert_at_child();
  return { _json_iter, depth()+1, _json_iter->token.position() };
}

// GCC 7 warns when the first line of this function is inlined away into oblivion due to the caller
// relating depth and iterator depth, which is a desired effect. It does not happen if is_open is
// marked non-inline.
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_really_inline bool value_iterator::is_open() const noexcept {
  return _json_iter->depth() >= depth();
}
SIMDJSON_POP_DISABLE_WARNINGS

simdjson_really_inline bool value_iterator::at_eof() const noexcept {
  return _json_iter->at_eof();
}

simdjson_really_inline bool value_iterator::at_start() const noexcept {
  return _json_iter->token.index == _start_position;
}

simdjson_really_inline bool value_iterator::at_first_field() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_position );
  return _json_iter->token.index == _start_position + 1;
}

simdjson_really_inline void value_iterator::abandon() noexcept {
  _json_iter->abandon();
}

simdjson_warn_unused simdjson_really_inline depth_t value_iterator::depth() const noexcept {
  return _depth;
}
simdjson_warn_unused simdjson_really_inline error_code value_iterator::error() const noexcept {
  return _json_iter->error;
}
simdjson_warn_unused simdjson_really_inline uint8_t *&value_iterator::string_buf_loc() noexcept {
  return _json_iter->string_buf_loc();
}
simdjson_warn_unused simdjson_really_inline const json_iterator &value_iterator::json_iter() const noexcept {
  return *_json_iter;
}
simdjson_warn_unused simdjson_really_inline json_iterator &value_iterator::json_iter() noexcept {
  return *_json_iter;
}

simdjson_really_inline const uint8_t *value_iterator::peek_start() const noexcept {
  return _json_iter->peek(_start_position);
}
simdjson_really_inline uint32_t value_iterator::peek_start_length() const noexcept {
  return _json_iter->peek_length(_start_position);
}

simdjson_really_inline const uint8_t *value_iterator::advance_start(const char *type) const noexcept {
  logger::log_value(*_json_iter, _start_position, depth(), type);
  // If we're not at the position anymore, we don't want to advance the cursor.
  if (!is_at_start()) { return peek_start(); }

  // Get the JSON and advance the cursor, decreasing depth to signify that we have retrieved the value.
  assert_at_start();
  auto result = _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return result;
}
simdjson_really_inline error_code value_iterator::advance_container_start(const char *type, const uint8_t *&json) const noexcept {
  logger::log_start_value(*_json_iter, _start_position, depth(), type);

  // If we're not at the position anymore, we don't want to advance the cursor.
  if (!is_at_start()) {
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
    if (!is_at_iterator_start()) { return OUT_OF_ORDER_ITERATION; }
#endif
    json = peek_start();
    return SUCCESS;
  }

  // Get the JSON and advance the cursor, decreasing depth to signify that we have retrieved the value.
  assert_at_start();
  json = _json_iter->advance();
  return SUCCESS;
}
simdjson_really_inline const uint8_t *value_iterator::advance_root_scalar(const char *type) const noexcept {
  logger::log_value(*_json_iter, _start_position, depth(), type);
  if (!is_at_start()) { return peek_start(); }

  assert_at_root();
  auto result = _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return result;
}
simdjson_really_inline const uint8_t *value_iterator::advance_non_root_scalar(const char *type) const noexcept {
  logger::log_value(*_json_iter, _start_position, depth(), type);
  if (!is_at_start()) { return peek_start(); }

  assert_at_non_root_start();
  auto result = _json_iter->advance();
  _json_iter->ascend_to(depth()-1);
  return result;
}

simdjson_really_inline error_code value_iterator::incorrect_type_error(const char *message) const noexcept {
  logger::log_error(*_json_iter, _start_position, depth(), message);
  return INCORRECT_TYPE;
}

simdjson_really_inline bool value_iterator::is_at_start() const noexcept {
  return _json_iter->token.index == _start_position;
}
simdjson_really_inline bool value_iterator::is_at_container_start() const noexcept {
  return _json_iter->token.index == _start_position + 1;
}
simdjson_really_inline bool value_iterator::is_at_iterator_start() const noexcept {
  // We can legitimately be either at the first value ([1]), or after the array if it's empty ([]).
  auto delta = _json_iter->token.index - _start_position;
  return delta == 1 || delta == 2;
}

simdjson_really_inline void value_iterator::assert_at_start() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index == _start_position );
  SIMDJSON_ASSUME( _json_iter->_depth == _depth );
  SIMDJSON_ASSUME( _depth > 0 );
}

simdjson_really_inline void value_iterator::assert_at_container_start() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index == _start_position + 1 );
  SIMDJSON_ASSUME( _json_iter->_depth == _depth );
  SIMDJSON_ASSUME( _depth > 0 );
}

simdjson_really_inline void value_iterator::assert_at_next() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_position );
  SIMDJSON_ASSUME( _json_iter->_depth == _depth );
  SIMDJSON_ASSUME( _depth > 0 );
}

simdjson_really_inline void value_iterator::assert_at_child() const noexcept {
  SIMDJSON_ASSUME( _json_iter->token.index > _start_position );
  SIMDJSON_ASSUME( _json_iter->_depth == _depth + 1 );
  SIMDJSON_ASSUME( _depth > 0 );
}

simdjson_really_inline void value_iterator::assert_at_root() const noexcept {
  assert_at_start();
  SIMDJSON_ASSUME( _depth == 1 );
}

simdjson_really_inline void value_iterator::assert_at_non_root_start() const noexcept {
  assert_at_start();
  SIMDJSON_ASSUME( _depth > 1 );
}

simdjson_really_inline void value_iterator::assert_is_valid() const noexcept {
  SIMDJSON_ASSUME( _json_iter != nullptr );
}

simdjson_really_inline bool value_iterator::is_valid() const noexcept {
  return _json_iter != nullptr;
}


simdjson_really_inline simdjson_result<json_type> value_iterator::type() noexcept {
  switch (*peek_start()) {
    case '{':
      return json_type::object;
    case '[':
      return json_type::array;
    case '"':
      return json_type::string;
    case 'n':
      return json_type::null;
    case 't': case 'f':
      return json_type::boolean;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return json_type::number;
    default:
      return TAPE_ERROR;
  }
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator>::simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value_iterator>(error) {}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/value_iterator-inl.h */
/* begin file include/simdjson/generic/ondemand/array_iterator-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline array_iterator::array_iterator(const value_iterator &_iter) noexcept
  : iter{_iter}
{}

simdjson_really_inline simdjson_result<value> array_iterator::operator*() noexcept {
  if (iter.error()) { iter.abandon(); return iter.error(); }
  return value(iter.child());
}
simdjson_really_inline bool array_iterator::operator==(const array_iterator &other) const noexcept {
  return !(*this != other);
}
simdjson_really_inline bool array_iterator::operator!=(const array_iterator &) const noexcept {
  return iter.is_open();
}
simdjson_really_inline array_iterator &array_iterator::operator++() noexcept {
  error_code error;
  // PERF NOTE this is a safety rail ... users should exit loops as soon as they receive an error, so we'll never get here.
  // However, it does not seem to make a perf difference, so we add it out of an abundance of caution.
  if ((error = iter.error()) ) { return *this; }
  if ((error = iter.skip_child() )) { return *this; }
  if ((error = iter.has_next_element().error() )) { return *this; }
  return *this;
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>::simdjson_result(
  SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator &&value
) noexcept
  : SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>(value))
{
  first.iter.assert_is_valid();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>::simdjson_result(error_code error) noexcept
  : SIMDJSON_BUILTIN_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>::operator*() noexcept {
  if (error()) { return error(); }
  return *first;
}
simdjson_really_inline bool simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>::operator==(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> &other) const noexcept {
  if (!first.iter.is_valid()) { return !error(); }
  return first == other.first;
}
simdjson_really_inline bool simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>::operator!=(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> &other) const noexcept {
  if (!first.iter.is_valid()) { return error(); }
  return first != other.first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> &simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator>::operator++() noexcept {
  // Clear the error if there is one, so we don't yield it twice
  if (error()) { second = SUCCESS; return *this; }
  ++(first);
  return *this;
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/array_iterator-inl.h */
/* begin file include/simdjson/generic/ondemand/object_iterator-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

//
// object_iterator
//

simdjson_really_inline object_iterator::object_iterator(const value_iterator &_iter) noexcept
  : iter{_iter}
{}

simdjson_really_inline simdjson_result<field> object_iterator::operator*() noexcept {
  error_code error = iter.error();
  if (error) { iter.abandon(); return error; }
  auto result = field::start(iter);
  // TODO this is a safety rail ... users should exit loops as soon as they receive an error.
  // Nonetheless, let's see if performance is OK with this if statement--the compiler may give it to us for free.
  if (result.error()) { iter.abandon(); }
  return result;
}
simdjson_really_inline bool object_iterator::operator==(const object_iterator &other) const noexcept {
  return !(*this != other);
}
simdjson_really_inline bool object_iterator::operator!=(const object_iterator &) const noexcept {
  return iter.is_open();
}

simdjson_really_inline object_iterator &object_iterator::operator++() noexcept {
  // TODO this is a safety rail ... users should exit loops as soon as they receive an error.
  // Nonetheless, let's see if performance is OK with this if statement--the compiler may give it to us for free.
  if (!iter.is_open()) { return *this; } // Iterator will be released if there is an error

  simdjson_unused error_code error;
  if ((error = iter.skip_child() )) { return *this; }

  simdjson_unused bool has_value;
  if ((error = iter.has_next_field().get(has_value) )) { return *this; };
  return *this;
}

//
// ### Live States
//
// While iterating or looking up values, depth >= iter.depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the object is first found and the iterator is just past the {.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the , or } before the next value. In this state,
//   depth == iter.depth, at_start == false, and error == SUCCESS.
// - Unfinished Business: When we hand an array/object to the user which they do not fully
//   iterate over, we need to finish that iteration by skipping child values until we reach the
//   Next state. In this state, depth > iter.depth, at_start == false, and error == SUCCESS.
//
// ## Error States
//
// In error states, we will yield exactly one more value before stopping. iter.depth == depth
// and at_start is always false. We decrement after yielding the error, moving to the Finished
// state.
//
// - Chained Error: When the object iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet field may be missing or not be an
//   object--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   iter.depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between fields,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, iter.depth == depth, and at_start == false.
//
// Errors that occur while reading a field to give to the user (such as when the key is not a
// string or the field is missing a colon) are yielded immediately. Depth is then decremented,
// moving to the Finished state without transitioning through an Error state at all.
//
// ## Terminal State
//
// The terminal state has iter.depth < depth. at_start is always false.
//
// - Finished: When we have reached a }, we are finished. We signal this by decrementing depth.
//   In this state, iter.depth < depth, at_start == false, and error == SUCCESS.
//

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>::simdjson_result(
  SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>(value))
{
  first.iter.assert_is_valid();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>::simdjson_result(error_code error) noexcept
  : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>({}, error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>::operator*() noexcept {
  if (error()) { return error(); }
  return *first;
}
// If we're iterating and there is an error, return the error once.
simdjson_really_inline bool simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>::operator==(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> &other) const noexcept {
  if (!first.iter.is_valid()) { return !error(); }
  return first == other.first;
}
// If we're iterating and there is an error, return the error once.
simdjson_really_inline bool simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>::operator!=(const simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> &other) const noexcept {
  if (!first.iter.is_valid()) { return error(); }
  return first != other.first;
}
// Checks for ']' and ','
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> &simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator>::operator++() noexcept {
  // Clear the error if there is one, so we don't yield it twice
  if (error()) { second = SUCCESS; return *this; }
  ++first;
  return *this;
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/object_iterator-inl.h */
/* begin file include/simdjson/generic/ondemand/array-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

//
// ### Live States
//
// While iterating or looking up values, depth >= iter->depth. at_start may vary. Error is
// always SUCCESS:
//
// - Start: This is the state when the array is first found and the iterator is just past the `{`.
//   In this state, at_start == true.
// - Next: After we hand a scalar value to the user, or an array/object which they then fully
//   iterate over, the iterator is at the `,` before the next value (or `]`). In this state,
//   depth == iter->depth, at_start == false, and error == SUCCESS.
// - Unfinished Business: When we hand an array/object to the user which they do not fully
//   iterate over, we need to finish that iteration by skipping child values until we reach the
//   Next state. In this state, depth > iter->depth, at_start == false, and error == SUCCESS.
//
// ## Error States
//
// In error states, we will yield exactly one more value before stopping. iter->depth == depth
// and at_start is always false. We decrement after yielding the error, moving to the Finished
// state.
//
// - Chained Error: When the array iterator is part of an error chain--for example, in
//   `for (auto tweet : doc["tweets"])`, where the tweet element may be missing or not be an
//   array--we yield that error in the loop, exactly once. In this state, error != SUCCESS and
//   iter->depth == depth, and at_start == false. We decrement depth when we yield the error.
// - Missing Comma Error: When the iterator ++ method discovers there is no comma between elements,
//   we flag that as an error and treat it exactly the same as a Chained Error. In this state,
//   error == TAPE_ERROR, iter->depth == depth, and at_start == false.
//
// ## Terminal State
//
// The terminal state has iter->depth < depth. at_start is always false.
//
// - Finished: When we have reached a `]` or have reported an error, we are finished. We signal this
//   by decrementing depth. In this state, iter->depth < depth, at_start == false, and
//   error == SUCCESS.
//

simdjson_really_inline array::array(const value_iterator &_iter) noexcept
  : iter{_iter}
{
}

simdjson_really_inline simdjson_result<array> array::start(value_iterator &iter) noexcept {
  // We don't need to know if the array is empty to start iteration, but we do want to know if there
  // is an error--thus `simdjson_unused`.
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_array().get(has_value) );
  return array(iter);
}
simdjson_really_inline simdjson_result<array> array::start_root(value_iterator &iter) noexcept {
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_root_array().get(has_value) );
  return array(iter);
}
simdjson_really_inline array array::started(value_iterator &iter) noexcept {
  simdjson_unused bool has_value = iter.started_array();
  return array(iter);
}

simdjson_really_inline simdjson_result<array_iterator> array::begin() noexcept {
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  if (!iter.is_at_iterator_start()) { return OUT_OF_ORDER_ITERATION; }
#endif
  return array_iterator(iter);
}
simdjson_really_inline simdjson_result<array_iterator> array::end() noexcept {
  return array_iterator(iter);
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array>::simdjson_result(
  SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array &&value
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array>(
      std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array>::simdjson_result(
  error_code error
) noexcept
  : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array>::begin() noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array>::end() noexcept {
  if (error()) { return error(); }
  return first.end();
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/array-inl.h */
/* begin file include/simdjson/generic/ondemand/document-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline document::document(ondemand::json_iterator &&_iter) noexcept
  : iter{std::forward<json_iterator>(_iter)}
{
  logger::log_start_value(iter, "document");
}

simdjson_really_inline document document::start(json_iterator &&iter) noexcept {
  return document(std::forward<json_iterator>(iter));
}

simdjson_really_inline value_iterator document::resume_value_iterator() noexcept {
  return value_iterator(&iter, 1, iter.root_checkpoint());
}
simdjson_really_inline value_iterator document::get_root_value_iterator() noexcept {
  return resume_value_iterator();
}
simdjson_really_inline value document::resume_value() noexcept {
  return resume_value_iterator();
}
simdjson_really_inline simdjson_result<array> document::get_array() & noexcept {
  auto value = get_root_value_iterator();
  return array::start_root(value);
}
simdjson_really_inline simdjson_result<object> document::get_object() & noexcept {
  auto value = get_root_value_iterator();
  return object::start_root(value);
}
simdjson_really_inline simdjson_result<uint64_t> document::get_uint64() noexcept {
  return get_root_value_iterator().get_root_uint64();
}
simdjson_really_inline simdjson_result<int64_t> document::get_int64() noexcept {
  return get_root_value_iterator().get_root_int64();
}
simdjson_really_inline simdjson_result<double> document::get_double() noexcept {
  return get_root_value_iterator().get_root_double();
}
simdjson_really_inline simdjson_result<std::string_view> document::get_string() noexcept {
  return get_root_value_iterator().get_root_string();
}
simdjson_really_inline simdjson_result<raw_json_string> document::get_raw_json_string() noexcept {
  return get_root_value_iterator().get_root_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> document::get_bool() noexcept {
  return get_root_value_iterator().get_root_bool();
}
simdjson_really_inline bool document::is_null() noexcept {
  return get_root_value_iterator().is_root_null();
}

template<> simdjson_really_inline simdjson_result<array> document::get() & noexcept { return get_array(); }
template<> simdjson_really_inline simdjson_result<object> document::get() & noexcept { return get_object(); }
template<> simdjson_really_inline simdjson_result<raw_json_string> document::get() & noexcept { return get_raw_json_string(); }
template<> simdjson_really_inline simdjson_result<std::string_view> document::get() & noexcept { return get_string(); }
template<> simdjson_really_inline simdjson_result<double> document::get() & noexcept { return get_double(); }
template<> simdjson_really_inline simdjson_result<uint64_t> document::get() & noexcept { return get_uint64(); }
template<> simdjson_really_inline simdjson_result<int64_t> document::get() & noexcept { return get_int64(); }
template<> simdjson_really_inline simdjson_result<bool> document::get() & noexcept { return get_bool(); }

template<> simdjson_really_inline simdjson_result<raw_json_string> document::get() && noexcept { return get_raw_json_string(); }
template<> simdjson_really_inline simdjson_result<std::string_view> document::get() && noexcept { return get_string(); }
template<> simdjson_really_inline simdjson_result<double> document::get() && noexcept { return std::forward<document>(*this).get_double(); }
template<> simdjson_really_inline simdjson_result<uint64_t> document::get() && noexcept { return std::forward<document>(*this).get_uint64(); }
template<> simdjson_really_inline simdjson_result<int64_t> document::get() && noexcept { return std::forward<document>(*this).get_int64(); }
template<> simdjson_really_inline simdjson_result<bool> document::get() && noexcept { return std::forward<document>(*this).get_bool(); }

template<typename T> simdjson_really_inline error_code document::get(T &out) & noexcept {
  return get<T>().get(out);
}
template<typename T> simdjson_really_inline error_code document::get(T &out) && noexcept {
  return std::forward<document>(*this).get<T>().get(out);
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline document::operator array() & noexcept(false) { return get_array(); }
simdjson_really_inline document::operator object() & noexcept(false) { return get_object(); }
simdjson_really_inline document::operator uint64_t() noexcept(false) { return get_uint64(); }
simdjson_really_inline document::operator int64_t() noexcept(false) { return get_int64(); }
simdjson_really_inline document::operator double() noexcept(false) { return get_double(); }
simdjson_really_inline document::operator std::string_view() noexcept(false) { return get_string(); }
simdjson_really_inline document::operator raw_json_string() noexcept(false) { return get_raw_json_string(); }
simdjson_really_inline document::operator bool() noexcept(false) { return get_bool(); }
#endif

simdjson_really_inline simdjson_result<array_iterator> document::begin() & noexcept {
  return get_array().begin();
}
simdjson_really_inline simdjson_result<array_iterator> document::end() & noexcept {
  return {};
}

simdjson_really_inline simdjson_result<value> document::find_field(std::string_view key) & noexcept {
  return resume_value().find_field(key);
}
simdjson_really_inline simdjson_result<value> document::find_field(const char *key) & noexcept {
  return resume_value().find_field(key);
}
simdjson_really_inline simdjson_result<value> document::find_field_unordered(std::string_view key) & noexcept {
  return resume_value().find_field_unordered(key);
}
simdjson_really_inline simdjson_result<value> document::find_field_unordered(const char *key) & noexcept {
  return resume_value().find_field_unordered(key);
}
simdjson_really_inline simdjson_result<value> document::operator[](std::string_view key) & noexcept {
  return resume_value()[key];
}
simdjson_really_inline simdjson_result<value> document::operator[](const char *key) & noexcept {
  return resume_value()[key];
}

simdjson_really_inline simdjson_result<json_type> document::type() noexcept {
  return get_root_value_iterator().type();
}

simdjson_really_inline simdjson_result<std::string_view> document::raw_json_token() noexcept {
  auto _iter = get_root_value_iterator();
  return std::string_view(reinterpret_cast<const char*>(_iter.peek_start()), _iter.peek_start_length());
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::simdjson_result(
  SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(
      std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(
      error
    )
{
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::end() & noexcept {
  return {};
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::find_field_unordered(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::find_field_unordered(const char *key) & noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator[](std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator[](const char *key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::find_field(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::find_field(const char *key) & noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_array() & noexcept {
  if (error()) { return error(); }
  return first.get_array();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_object() & noexcept {
  if (error()) { return error(); }
  return first.get_object();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_double() noexcept {
  if (error()) { return error(); }
  return first.get_double();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_string() noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get_bool() noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::is_null() noexcept {
  if (error()) { return error(); }
  return first.is_null();
}

template<typename T>
simdjson_really_inline simdjson_result<T> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get() & noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}
template<typename T>
simdjson_really_inline simdjson_result<T> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(first).get<T>();
}
template<typename T>
simdjson_really_inline error_code simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get(T &out) & noexcept {
  if (error()) { return error(); }
  return first.get<T>(out);
}
template<typename T>
simdjson_really_inline error_code simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get(T &out) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(first).get<T>(out);
}

template<> simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>() & noexcept = delete;
template<> simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>() && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(first);
}
template<> simdjson_really_inline error_code simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document &out) & noexcept = delete;
template<> simdjson_really_inline error_code simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::get<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document &out) && noexcept {
  if (error()) { return error(); }
  out = std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>(first);
  return SUCCESS;
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::type() noexcept {
  if (error()) { return error(); }
  return first.type();
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object() & noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator uint64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator int64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator double() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator std::string_view() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::operator bool() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
#endif

simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::document>::raw_json_token() noexcept {
  if (error()) { return error(); }
  return first.raw_json_token();
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/document-inl.h */
/* begin file include/simdjson/generic/ondemand/value-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline value::value(const value_iterator &_iter) noexcept
  : iter{_iter}
{
}
simdjson_really_inline value value::start(const value_iterator &iter) noexcept {
  return iter;
}
simdjson_really_inline value value::resume(const value_iterator &iter) noexcept {
  return iter;
}

simdjson_really_inline simdjson_result<array> value::get_array() noexcept {
  return array::start(iter);
}
simdjson_really_inline simdjson_result<object> value::get_object() noexcept {
  return object::start(iter);
}
simdjson_really_inline simdjson_result<object> value::start_or_resume_object() noexcept {
  if (iter.at_start()) {
    return get_object();
  } else {
    return object::resume(iter);
  }
}

simdjson_really_inline simdjson_result<raw_json_string> value::get_raw_json_string() noexcept {
  return iter.get_raw_json_string();
}
simdjson_really_inline simdjson_result<std::string_view> value::get_string() noexcept {
  return iter.get_string();
}
simdjson_really_inline simdjson_result<double> value::get_double() noexcept {
  return iter.get_double();
}
simdjson_really_inline simdjson_result<uint64_t> value::get_uint64() noexcept {
  return iter.get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> value::get_int64() noexcept {
  return iter.get_int64();
}
simdjson_really_inline simdjson_result<bool> value::get_bool() noexcept {
  return iter.get_bool();
}
simdjson_really_inline bool value::is_null() noexcept {
  return iter.is_null();
}

template<> simdjson_really_inline simdjson_result<array> value::get() noexcept { return get_array(); }
template<> simdjson_really_inline simdjson_result<object> value::get() noexcept { return get_object(); }
template<> simdjson_really_inline simdjson_result<raw_json_string> value::get() noexcept { return get_raw_json_string(); }
template<> simdjson_really_inline simdjson_result<std::string_view> value::get() noexcept { return get_string(); }
template<> simdjson_really_inline simdjson_result<double> value::get() noexcept { return get_double(); }
template<> simdjson_really_inline simdjson_result<uint64_t> value::get() noexcept { return get_uint64(); }
template<> simdjson_really_inline simdjson_result<int64_t> value::get() noexcept { return get_int64(); }
template<> simdjson_really_inline simdjson_result<bool> value::get() noexcept { return get_bool(); }

template<typename T> simdjson_really_inline error_code value::get(T &out) noexcept {
  return get<T>().get(out);
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline value::operator array() noexcept(false) {
  return get_array();
}
simdjson_really_inline value::operator object() noexcept(false) {
  return get_object();
}
simdjson_really_inline value::operator uint64_t() noexcept(false) {
  return get_uint64();
}
simdjson_really_inline value::operator int64_t() noexcept(false) {
  return get_int64();
}
simdjson_really_inline value::operator double() noexcept(false) {
  return get_double();
}
simdjson_really_inline value::operator std::string_view() noexcept(false) {
  return get_string();
}
simdjson_really_inline value::operator raw_json_string() noexcept(false) {
  return get_raw_json_string();
}
simdjson_really_inline value::operator bool() noexcept(false) {
  return get_bool();
}
#endif

simdjson_really_inline simdjson_result<array_iterator> value::begin() & noexcept {
  return get_array().begin();
}
simdjson_really_inline simdjson_result<array_iterator> value::end() & noexcept {
  return {};
}

simdjson_really_inline simdjson_result<value> value::find_field(std::string_view key) noexcept {
  return start_or_resume_object().find_field(key);
}
simdjson_really_inline simdjson_result<value> value::find_field(const char *key) noexcept {
  return start_or_resume_object().find_field(key);
}

simdjson_really_inline simdjson_result<value> value::find_field_unordered(std::string_view key) noexcept {
  return start_or_resume_object().find_field_unordered(key);
}
simdjson_really_inline simdjson_result<value> value::find_field_unordered(const char *key) noexcept {
  return start_or_resume_object().find_field_unordered(key);
}

simdjson_really_inline simdjson_result<value> value::operator[](std::string_view key) noexcept {
  return start_or_resume_object()[key];
}
simdjson_really_inline simdjson_result<value> value::operator[](const char *key) noexcept {
  return start_or_resume_object()[key];
}

simdjson_really_inline simdjson_result<json_type> value::type() noexcept {
  return iter.type();
}

simdjson_really_inline std::string_view value::raw_json_token() noexcept {
  return std::string_view(reinterpret_cast<const char*>(iter.peek_start()), iter.peek_start_length());
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::simdjson_result(
  SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>(
      std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::begin() & noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::end() & noexcept {
  if (error()) { return error(); }
  return {};
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::find_field(std::string_view key) noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::find_field(const char *key) noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::find_field_unordered(std::string_view key) noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::find_field_unordered(const char *key) noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator[](std::string_view key) noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator[](const char *key) noexcept {
  if (error()) { return error(); }
  return first[key];
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_array() noexcept {
  if (error()) { return error(); }
  return first.get_array();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_object() noexcept {
  if (error()) { return error(); }
  return first.get_object();
}
simdjson_really_inline simdjson_result<uint64_t> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_uint64() noexcept {
  if (error()) { return error(); }
  return first.get_uint64();
}
simdjson_really_inline simdjson_result<int64_t> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_int64() noexcept {
  if (error()) { return error(); }
  return first.get_int64();
}
simdjson_really_inline simdjson_result<double> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_double() noexcept {
  if (error()) { return error(); }
  return first.get_double();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_string() noexcept {
  if (error()) { return error(); }
  return first.get_string();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_raw_json_string() noexcept {
  if (error()) { return error(); }
  return first.get_raw_json_string();
}
simdjson_really_inline simdjson_result<bool> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get_bool() noexcept {
  if (error()) { return error(); }
  return first.get_bool();
}
simdjson_really_inline bool simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::is_null() noexcept {
  if (error()) { return false; }
  return first.is_null();
}

template<typename T> simdjson_really_inline simdjson_result<T> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get() noexcept {
  if (error()) { return error(); }
  return first.get<T>();
}
template<typename T> simdjson_really_inline error_code simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get(T &out) noexcept {
  if (error()) { return error(); }
  return first.get<T>(out);
}

template<> simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>() noexcept  {
  if (error()) { return error(); }
  return std::move(first);
}
template<> simdjson_really_inline error_code simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::get<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value &out) noexcept {
  if (error()) { return error(); }
  out = first;
  return SUCCESS;
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::json_type> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::type() noexcept {
  if (error()) { return error(); }
  return first.type();
}

#if SIMDJSON_EXCEPTIONS
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::array() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator uint64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator int64_t() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator double() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator std::string_view() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::operator bool() noexcept(false) {
  if (error()) { throw simdjson_error(error()); }
  return first;
}
#endif

simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value>::raw_json_token() noexcept {
  if (error()) { return error(); }
  return first.raw_json_token();
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/value-inl.h */
/* begin file include/simdjson/generic/ondemand/field-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

// clang 6 doesn't think the default constructor can be noexcept, so we make it explicit
simdjson_really_inline field::field() noexcept : std::pair<raw_json_string, ondemand::value>() {}

simdjson_really_inline field::field(raw_json_string key, ondemand::value &&value) noexcept
  : std::pair<raw_json_string, ondemand::value>(key, std::forward<ondemand::value>(value))
{
}

simdjson_really_inline simdjson_result<field> field::start(value_iterator &parent_iter) noexcept {
  raw_json_string key;
  SIMDJSON_TRY( parent_iter.field_key().get(key) );
  SIMDJSON_TRY( parent_iter.field_value() );
  return field::start(parent_iter, key);
}

simdjson_really_inline simdjson_result<field> field::start(const value_iterator &parent_iter, raw_json_string key) noexcept {
    return field(key, parent_iter.child());
}

simdjson_really_inline simdjson_warn_unused simdjson_result<std::string_view> field::unescaped_key() noexcept {
  SIMDJSON_ASSUME(first.buf != nullptr); // We would like to call .alive() by Visual Studio won't let us.
  simdjson_result<std::string_view> answer = first.unescape(second.iter.string_buf_loc());
  first.consume();
  return answer;
}

simdjson_really_inline raw_json_string field::key() const noexcept {
  SIMDJSON_ASSUME(first.buf != nullptr); // We would like to call .alive() by Visual Studio won't let us.
  return first;
}

simdjson_really_inline value &field::value() & noexcept {
  return second;
}

simdjson_really_inline value field::value() && noexcept {
  return std::forward<field>(*this).second;
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>::simdjson_result(
  SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field &&value
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>(
      std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>(value)
    )
{
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>::simdjson_result(
  error_code error
) noexcept :
    implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>(error)
{
}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::raw_json_string> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>::key() noexcept {
  if (error()) { return error(); }
  return first.key();
}
simdjson_really_inline simdjson_result<std::string_view> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>::unescaped_key() noexcept {
  if (error()) { return error(); }
  return first.unescaped_key();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::field>::value() noexcept {
  if (error()) { return error(); }
  return std::move(first.value());
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/field-inl.h */
/* begin file include/simdjson/generic/ondemand/object-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_really_inline simdjson_result<value> object::find_field_unordered(const std::string_view key) & noexcept {
  bool has_value;
  SIMDJSON_TRY( iter.find_field_unordered_raw(key).get(has_value) );
  if (!has_value) { return NO_SUCH_FIELD; }
  return value(iter.child());
}
simdjson_really_inline simdjson_result<value> object::find_field_unordered(const std::string_view key) && noexcept {
  bool has_value;
  SIMDJSON_TRY( iter.find_field_unordered_raw(key).get(has_value) );
  if (!has_value) { return NO_SUCH_FIELD; }
  return value(iter.child());
}
simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) & noexcept {
  return find_field_unordered(key);
}
simdjson_really_inline simdjson_result<value> object::operator[](const std::string_view key) && noexcept {
  return std::forward<object>(*this).find_field_unordered(key);
}
simdjson_really_inline simdjson_result<value> object::find_field(const std::string_view key) & noexcept {
  bool has_value;
  SIMDJSON_TRY( iter.find_field_raw(key).get(has_value) );
  if (!has_value) { return NO_SUCH_FIELD; }
  return value(iter.child());
}
simdjson_really_inline simdjson_result<value> object::find_field(const std::string_view key) && noexcept {
  bool has_value;
  SIMDJSON_TRY( iter.find_field_raw(key).get(has_value) );
  if (!has_value) { return NO_SUCH_FIELD; }
  return value(iter.child());
}

simdjson_really_inline simdjson_result<object> object::start(value_iterator &iter) noexcept {
  // We don't need to know if the object is empty to start iteration, but we do want to know if there
  // is an error--thus `simdjson_unused`.
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_object().get(has_value) );
  return object(iter);
}
simdjson_really_inline simdjson_result<object> object::start_root(value_iterator &iter) noexcept {
  simdjson_unused bool has_value;
  SIMDJSON_TRY( iter.start_root_object().get(has_value) );
  return object(iter);
}
simdjson_really_inline object object::started(value_iterator &iter) noexcept {
  simdjson_unused bool has_value = iter.started_object();
  return iter;
}
simdjson_really_inline object object::resume(const value_iterator &iter) noexcept {
  return iter;
}

simdjson_really_inline object::object(const value_iterator &_iter) noexcept
  : iter{_iter}
{
}

simdjson_really_inline simdjson_result<object_iterator> object::begin() noexcept {
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  if (!iter.is_at_iterator_start()) { return OUT_OF_ORDER_ITERATION; }
#endif
  return object_iterator(iter);
}
simdjson_really_inline simdjson_result<object_iterator> object::end() noexcept {
  return object_iterator(iter);
}

} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>(error) {}

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::begin() noexcept {
  if (error()) { return error(); }
  return first.begin();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object_iterator> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::end() noexcept {
  if (error()) { return error(); }
  return first.end();
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::find_field_unordered(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::find_field_unordered(std::string_view key) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>(first).find_field_unordered(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::operator[](std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::operator[](std::string_view key) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>(first)[key];
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::find_field(std::string_view key) & noexcept {
  if (error()) { return error(); }
  return first.find_field(key);
}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::value> simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>::find_field(std::string_view key) && noexcept {
  if (error()) { return error(); }
  return std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::object>(first).find_field(key);
}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/object-inl.h */
/* begin file include/simdjson/generic/ondemand/parser-inl.h */
namespace simdjson {
namespace SIMDJSON_BUILTIN_IMPLEMENTATION {
namespace ondemand {

simdjson_warn_unused simdjson_really_inline error_code parser::allocate(size_t new_capacity, size_t new_max_depth) noexcept {
  if (string_buf && new_capacity == capacity() && new_max_depth == max_depth()) { return SUCCESS; }

  // string_capacity copied from document::allocate
  _capacity = 0;
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * new_capacity / 3 + SIMDJSON_PADDING, 64);
  string_buf.reset(new (std::nothrow) uint8_t[string_capacity]);
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  start_positions.reset(new (std::nothrow) token_position[new_max_depth]);
#endif
  if (implementation) {
    SIMDJSON_TRY( implementation->set_capacity(new_capacity) );
    SIMDJSON_TRY( implementation->set_max_depth(new_max_depth) );
  } else {
    SIMDJSON_TRY( simdjson::active_implementation->create_dom_parser_implementation(new_capacity, new_max_depth, implementation) );
  }
  _capacity = new_capacity;
  _max_depth = new_max_depth;
  return SUCCESS;
}

simdjson_warn_unused simdjson_really_inline simdjson_result<document> parser::iterate(padded_string_view json) & noexcept {
  if (json.padding() < SIMDJSON_PADDING) { return INSUFFICIENT_PADDING; }

  // Allocate if needed
  if (capacity() < json.length() || !string_buf) {
    SIMDJSON_TRY( allocate(json.length(), max_depth()) );
  }

  // Run stage 1.
  SIMDJSON_TRY( implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.length(), false) );
  return document::start({ reinterpret_cast<const uint8_t *>(json.data()), this });
}

simdjson_warn_unused simdjson_really_inline simdjson_result<document> parser::iterate(const char *json, size_t len, size_t allocated) & noexcept {
  return iterate(padded_string_view(json, len, allocated));
}

simdjson_warn_unused simdjson_really_inline simdjson_result<document> parser::iterate(const uint8_t *json, size_t len, size_t allocated) & noexcept {
  return iterate(padded_string_view(json, len, allocated));
}

simdjson_warn_unused simdjson_really_inline simdjson_result<document> parser::iterate(std::string_view json, size_t allocated) & noexcept {
  return iterate(padded_string_view(json, allocated));
}

simdjson_warn_unused simdjson_really_inline simdjson_result<document> parser::iterate(const std::string &json) & noexcept {
  return iterate(padded_string_view(json));
}

simdjson_warn_unused simdjson_really_inline simdjson_result<document> parser::iterate(const simdjson_result<padded_string_view> &result) & noexcept {
  // We don't presently have a way to temporarily get a const T& from a simdjson_result<T> without throwing an exception
  SIMDJSON_TRY( result.error() );
  padded_string_view json = result.value_unsafe();
  return iterate(json);
}

simdjson_warn_unused simdjson_really_inline simdjson_result<document> parser::iterate(const simdjson_result<padded_string> &result) & noexcept {
  // We don't presently have a way to temporarily get a const T& from a simdjson_result<T> without throwing an exception
  SIMDJSON_TRY( result.error() );
  const padded_string &json = result.value_unsafe();
  return iterate(json);
}

simdjson_warn_unused simdjson_really_inline simdjson_result<json_iterator> parser::iterate_raw(padded_string_view json) & noexcept {
  if (json.padding() < SIMDJSON_PADDING) { return INSUFFICIENT_PADDING; }

  // Allocate if needed
  if (capacity() < json.length()) {
    SIMDJSON_TRY( allocate(json.length(), max_depth()) );
  }

  // Run stage 1.
  SIMDJSON_TRY( implementation->stage1(reinterpret_cast<const uint8_t *>(json.data()), json.length(), false) );
  return json_iterator(reinterpret_cast<const uint8_t *>(json.data()), this);
}

simdjson_really_inline size_t parser::capacity() const noexcept {
  return _capacity;
}
simdjson_really_inline size_t parser::max_depth() const noexcept {
  return _max_depth;
}


} // namespace ondemand
} // namespace SIMDJSON_BUILTIN_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser>::simdjson_result(SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser>(std::forward<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser>(value)) {}
simdjson_really_inline simdjson_result<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand::parser>(error) {}

} // namespace simdjson
/* end file include/simdjson/generic/ondemand/parser-inl.h */
/* end file include/simdjson/generic/ondemand-inl.h */


namespace simdjson {
  /**
   * Represents the best statically linked simdjson implementation that can be used by the compiling
   * program.
   *
   * Detects what options the program is compiled against, and picks the minimum implementation that
   * will work on any computer that can run the program. For example, if you compile with g++
   * -march=westmere, it will pick the westmere implementation. The haswell implementation will
   * still be available, and can be selected at runtime, but the builtin implementation (and any
   * code that uses it) will use westmere.
   */
  namespace builtin = SIMDJSON_BUILTIN_IMPLEMENTATION;
  /**
   * @copydoc simdjson::SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand
   */
  namespace ondemand = SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand;
  /**
   * Function which returns a pointer to an implementation matching the "builtin" implementation.
   * The builtin implementation is the best statically linked simdjson implementation that can be used by the compiling
   * program. If you compile with g++ -march=haswell, this will return the haswell implementation.
   * It is handy to be able to check what builtin was used: builtin_implementation()->name().
   */
  const implementation * builtin_implementation();
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_H
/* end file include/simdjson/builtin.h */

#endif // SIMDJSON_H
/* end file include/simdjson.h */
