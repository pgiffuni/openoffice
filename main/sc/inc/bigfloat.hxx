/**************************************************************
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 *************************************************************/


#ifndef SC_BIGFLOAT_HXX
#define SC_BIGFLOAT_HXX

#include <sal/types.h>
#include <rtl/ustring.hxx>

// Boost.Multiprecision (and Boost.Math, which it pulls in) contains an
// unconditional #warning for language standards below C++14. Calc is built
// with -std=gnu++11 and -Werror, which turns that into a hard build error.
// The warning is silenced here, around the Boost includes only. The spellings
// have to be distinguished per compiler, clang rejects the GCC option name
// under -Werror.
#if defined(_MSC_VER)
#  pragma warning(push, 0)
#elif defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-W#warnings"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcpp"
#endif

#include <boost/multiprecision/cpp_dec_float.hpp>

#if defined(_MSC_VER)
#  pragma warning(pop)
#elif defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

/** Number of decimal digits carried by a ScBigFloat. This is the one and only
    place where the precision of the high precision number type is decided;
    changing it here changes it everywhere. Note that the number of digits
    actually held is somewhat larger than this (it is a decimal *precision*,
    not a fixed digit count), see the size note in main/sc/doc/bigfloat.md. */
#define SC_BIGFLOAT_DIGITS 50

/** Calc's high precision decimal number.

    This is the second numeric representation next to the native double. It is
    *not* a replacement for double: ordinary Calc values stay double, and a
    BigFloat only ever exists inside a formula token, i.e. for results that a
    formula explicitly produced as high precision.

    There is deliberately no implicit conversion to or from double in either
    direction. Use ScBigFloatFromDouble() to promote and ScBigFloatToDouble()
    when a legacy API asks for a double and the loss of precision is accepted
    knowingly. */
typedef ::boost::multiprecision::number<
        ::boost::multiprecision::cpp_dec_float< SC_BIGFLOAT_DIGITS > >     ScBigFloat;

/** True if the value is neither NaN nor infinity. */
bool        ScBigFloatIsFinite( const ScBigFloat& rValue );

/** True if the value is NaN. */
bool        ScBigFloatIsNaN( const ScBigFloat& rValue );

/** True if the value is exactly zero. */
bool        ScBigFloatIsZero( const ScBigFloat& rValue );

/** Explicit promotion of a native double to high precision. The double is
    converted to its exact binary value, *not* to its decimal display. */
ScBigFloat  ScBigFloatFromDouble( double fValue );

/** Explicit narrowing of a high precision value to a native double. Digits
    beyond the double precision are lost, this is only for the legacy APIs that
    have no other way to express a number. */
double      ScBigFloatToDouble( const ScBigFloat& rValue );

/** Parses a decimal number from text without ever going through a double.

    @param rText          the number, e.g. "1.2345678901234567890123456789"
    @param rDecimalSep    the locale's decimal separator, may be empty for '.'
    @param rThousandSep   the locale's grouping separator, may be empty
    @param rOut           receives the parsed value, unchanged on failure
    @return true if the whole text is a valid number.

    In contrast to rtl::math::stringToDouble() this preserves every digit the
    user supplied, and it is the only supported way to create a ScBigFloat from
    user input. */
bool        ScBigFloatFromString( const rtl::OUString& rText,
                            const rtl::OUString& rDecimalSep,
                            const rtl::OUString& rThousandSep,
                                ScBigFloat& rOut );

/** The exact decimal representation of the value, with '.' as the decimal
    separator. Trailing zeros of the stored value are dropped, and an exponent
    form is used for very small or very large magnitudes, e.g.
    "1.23456789012345678901234567890123456789" or "1.2345e-07". */
rtl::OUString   ScBigFloatToString( const ScBigFloat& rValue );

/** Same as above, but with the decimal separator replaced by cDecimalSep, for
    display in a document with a non-English locale. The exponent marker, if
    any, is upper cased to match Calc's number formatting. */
rtl::OUString   ScBigFloatToString( const ScBigFloat& rValue, sal_Unicode cDecimalSep );

#endif  // SC_BIGFLOAT_HXX
