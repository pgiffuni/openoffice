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


// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"



#include <string>
#include <rtl/string.hxx>

#include "bigfloat.hxx"



namespace
{

/** Single character classification used while scanning user input. */
enum ScBigFloatChar
{
    scbfInvalid,        // not part of a number at all
    scbfDigit,
    scbfSign,
    scbfDecimalPoint,
    scbfExponentMark,
    scbfGroupSeparator
};

/** Classify one character of the input, @see ScBigFloatFromString(). */
ScBigFloatChar lcl_Classify( sal_Unicode cChar,
                            const rtl::OUString& rDecimalSep,
                            const rtl::OUString& rGroupSep,
                                sal_Unicode& rLocaleDecimal )
{
    if (cChar >= '0' && cChar <= '9')
        return scbfDigit;
    if (cChar == '+' || cChar == '-')
        return scbfSign;
    if (cChar == 'e' || cChar == 'E')
        return scbfExponentMark;

    rLocaleDecimal = 0;
    // A '.' is always accepted as the decimal point, also in a locale that
    // uses it as the grouping character: BIGFLOAT() is primarily the way to
    // write an exact constant, and the formula language spells the decimal
    // point with a dot. The locale's own separators are accepted in addition.
    if (cChar == '.')
        return scbfDecimalPoint;
    if (rDecimalSep.getLength() > 0 && cChar == rDecimalSep[0])
    {
        rLocaleDecimal = cChar;
        return scbfDecimalPoint;
    }
    if (rGroupSep.getLength() > 0 && cChar == rGroupSep[0])
        return scbfGroupSeparator;
    return scbfInvalid;
}

}  // namespace



bool ScBigFloatIsFinite( const ScBigFloat& rValue )
{
    return !ScBigFloatIsNaN( rValue ) &&
           !::boost::multiprecision::isinf( rValue );
}


bool ScBigFloatIsNaN( const ScBigFloat& rValue )
{
    return ::boost::multiprecision::isnan( rValue );
}


bool ScBigFloatIsZero( const ScBigFloat& rValue )
{
    return rValue == ScBigFloat( 0 );
}


ScBigFloat ScBigFloatFromDouble( double fValue )
{
    return ScBigFloat( fValue );
}


double ScBigFloatToDouble( const ScBigFloat& rValue )
{
    // Explicit and lossy on purpose. Only for APIs that have no other way to
    // express a number, never inside a high precision calculation.
    return static_cast< double >( rValue );
}


bool ScBigFloatFromString( const rtl::OUString& rText,
                            const rtl::OUString& rDecimalSep,
                            const rtl::OUString& rThousandSep,
                                ScBigFloat& rOut )
{
    // The value is built up as plain ASCII text with '.' as the decimal
    // separator and handed to Boost as such. Nothing here goes through a
    // double, which is the entire point of this function.
    std::string aNumber;
    sal_Int32 nStart = 0;
    sal_Int32 nEnd = rText.getLength();
    while (nStart < nEnd && (rText[ nStart ] == ' ' || rText[ nStart ] == '\t'))
        ++nStart;
    while (nEnd > nStart && (rText[ nEnd - 1 ] == ' ' || rText[ nEnd - 1 ] == '\t'))
        --nEnd;
    if (nStart >= nEnd)
        return false;

    bool bMantissaDigit = false;    // at least one digit before the exponent
    bool bDecimalPoint = false;
    bool bSignSeen = false;
    int nExponent = 0;              // 0 = none, 1 = 'e' seen, 2 = digit seen
    for (sal_Int32 i = nStart; i < nEnd; ++i)
    {
        sal_Unicode cLocaleDecimal = 0;
        ScBigFloatChar eClass = lcl_Classify( rText[ i ], rDecimalSep,
                rThousandSep, cLocaleDecimal );
        switch (eClass)
        {
            case scbfGroupSeparator:
                // Grouping is only meaningful in front of the decimal point.
                if (!bDecimalPoint && nExponent == 0)
                    break;
                return false;

            case scbfSign:
                // A sign is only allowed in front of the number or in front of
                // the exponent, and only once there.
                if (bSignSeen)
                    return false;
                if (nExponent == 0 && (bMantissaDigit || bDecimalPoint))
                    return false;
                bSignSeen = true;
                aNumber += static_cast<char>( rText[ i ] );
                break;

            case scbfDigit:
                aNumber += static_cast<char>( rText[ i ] );
                if (nExponent > 0)
                    nExponent = 2;
                else
                    bMantissaDigit = true;
                break;

            case scbfDecimalPoint:
                if (bDecimalPoint || nExponent > 0)
                    return false;   // second point, or point in the exponent
                bDecimalPoint = true;
                aNumber += '.';
                break;

            case scbfExponentMark:
                if (!bMantissaDigit || nExponent > 0)
                    return false;   // no mantissa digits, or second exponent
                aNumber += 'e';
                nExponent = 1;
                break;

            case scbfInvalid:
            default:
                return false;
        }
    }

    // The mantissa must have at least one digit, and an exponent must be
    // complete. (".", "+", "1e", "1e+" are all rejected here.)
    if (!bMantissaDigit || nExponent == 1)
        return false;

    try
    {
        // Boost's string constructor parses the decimal exactly, it does not
        // go through a native floating point value. It throws on input it
        // cannot represent, e.g. an exponent far outside its range.
        rOut = ScBigFloat( aNumber );
    }
    catch ( const std::exception& )
    {
        return false;
    }
    return true;
}


rtl::OUString ScBigFloatToString( const ScBigFloat& rValue )
{
    // str(0) yields the stored value with all its significant digits and no
    // trailing zeros, switching to exponent notation only for magnitudes that
    // do not fit a plain decimal representation.
    const std::string aStdStr( rValue.str( 0 ));
    return rtl::OUString( aStdStr.c_str(), aStdStr.length(),
            RTL_TEXTENCODING_ASCII_US );
}


rtl::OUString ScBigFloatToString( const ScBigFloat& rValue, sal_Unicode cDecimalSep )
{
    rtl::OUString aResult( ScBigFloatToString( rValue ));
    if (aResult.isEmpty() || cDecimalSep == '.')
        return aResult;

    // The mantissa may only hold digits and the decimal point, so the first
    // 'e' can only be the exponent marker. Calc writes exponents in upper
    // case, e.g. 1E-60.
    aResult = aResult.replace( 'e', 'E' );
    return aResult.replace( '.', cDecimalSep );
}
