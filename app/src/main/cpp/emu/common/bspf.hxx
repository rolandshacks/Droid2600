//============================================================================
//
//  BBBBB    SSSS   PPPPP   FFFFFF
//  BB  BB  SS  SS  PP  PP  FF
//  BB  BB  SS      PP  PP  FF
//  BBBBB    SSSS   PPPPP   FFFF    --  "Brad's Simple Portability Framework"
//  BB  BB      SS  PP      FF
//  BB  BB  SS  SS  PP      FF
//  BBBBB    SSSS   PP      FF
//
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: bspf.hxx 3316 2016-08-24 23:57:07Z stephena $
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott
  @version $Id: bspf.hxx 3316 2016-08-24 23:57:07Z stephena $
*/

#include <cstdint>
// Types for 8/16/32/64-bit signed and unsigned integers
using Int8 = int8_t;
using uInt8 = uint8_t;
using Int16 = int16_t;
using uInt16 = uint16_t;
using Int32 = int32_t;
using uInt32 = uint32_t;
using Int64 = int64_t;
using uInt64 = uint64_t;

// The following code should provide access to the standard C++ objects and
// types: cout, cerr, string, ostream, istream, etc.
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <utility>
#include <vector>
#include "UniquePtr.hxx"  // only until C++14 compilers are more common

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::istream;
using std::ostream;
using std::fstream;
using std::iostream;
using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::unique_ptr;
using std::shared_ptr;
using std::make_ptr;
using std::make_shared;
using std::array;
using std::vector;
using std::make_pair;
using std::runtime_error;
using std::memcpy;

// Common array types
using IntArray = std::vector<Int32>;
using BoolArray = std::vector<bool>;
using ByteArray = std::vector<uInt8>;
using StringList = std::vector<std::string>;
using BytePtr = std::unique_ptr<uInt8[]>;

static const string EmptyString("");

namespace BSPF
{
  // Defines to help with path handling
#if defined(BSPF_UNIX) || defined(BSPF_MAC_OSX)
static const string PATH_SEPARATOR = "/";
#elif defined(BSPF_WINDOWS)
static const string PATH_SEPARATOR = "\\";
#pragma warning(2:4264)  // no override available for virtual member function from base 'class'; function is hidden
#pragma warning(2:4265)  // class has virtual functions, but destructor is not virtual
#pragma warning(2:4266)  // no override available for virtual member function from base 'type'; function is hidden
#else
#error Update src/common/bspf.hxx for path separator
#endif

// CPU architecture type
// This isn't complete yet, but takes care of all the major platforms
#if defined(__i386__) || defined(_M_IX86)
static const string ARCH = "i386";
#elif defined(__x86_64__) || defined(_WIN64)
static const string ARCH = "x86_64";
#elif defined(__powerpc__) || defined(__ppc__)
static const string ARCH = "ppc";
#else
static const string ARCH = "NOARCH";
#endif

// Combines 'max' and 'min', and clamps value to the upper/lower value
// if it is outside the specified range
template<typename T> inline T clamp(T a, T l, T u)
{
    return (a < l) ? l : (a > u) ? u : a;
}

// Compare two strings, ignoring case
inline int compareIgnoreCase(const string& s1, const string& s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
    return _stricmp(s1.c_str(), s2.c_str());
#else
    return strcasecmp(s1.c_str(), s2.c_str());
#endif
}
inline int compareIgnoreCase(const char* s1, const char* s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
    return _stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

// Test whether the first string starts with the second one (case insensitive)
inline bool startsWithIgnoreCase(const string& s1, const string& s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
    return _strnicmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
#else
    return strncasecmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
#endif
}
inline bool startsWithIgnoreCase(const char* s1, const char* s2)
{
#if defined BSPF_WINDOWS && !defined __GNUG__
    return _strnicmp(s1, s2, strlen(s2)) == 0;
#else
    return strncasecmp(s1, s2, strlen(s2)) == 0;
#endif
}

// Test whether two strings are equal (case insensitive)
inline bool equalsIgnoreCase(const string& s1, const string& s2)
{
    return compareIgnoreCase(s1, s2) == 0;
}

// Find location (if any) of the second string within the first,
// starting from 'startpos' in the first string
inline size_t findIgnoreCase(const string& s1, const string& s2, int startpos = 0)
{
    auto pos = std::search(s1.begin() + startpos, s1.end(),
        s2.begin(), s2.end(), [](char ch1, char ch2) {
        return toupper(uInt8(ch1)) == toupper(uInt8(ch2));
    });
    return pos == s1.end() ? string::npos : size_t(pos - (s1.begin() + startpos));
}

// Test whether the first string ends with the second one (case insensitive)
inline bool endsWithIgnoreCase(const string& s1, const string& s2)
{
    if (s1.length() >= s2.length())
    {
        const char* end = s1.c_str() + s1.length() - s2.length();
        return compareIgnoreCase(end, s2.c_str()) == 0;
    }
    return false;
}

// Test whether the first string contains the second one (case insensitive)
inline bool containsIgnoreCase(const string& s1, const string& s2)
{
    return findIgnoreCase(s1, s2) != string::npos;
}
} // namespace BSPF

///// ANDROID LOGGING

#ifndef __DEF_ANDROID_LOGGING
#define __DEF_ANDROID_LOGGING

#include <android/log.h>

#define  ANDROID_LOG_TAG    "droid2600"

#define  LOG(...)  __android_log_print(ANDROID_LOG_INFO, ANDROID_LOG_TAG, __VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, ANDROID_LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, ANDROID_LOG_TAG, __VA_ARGS__)

#endif

#endif
