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

#include "precompiled_sc.hxx"

#include "gtest/gtest.h"

#include <type_traits>

#include "bigfloat.hxx"
#include "bigfloattoken.hxx"

namespace
{

const rtl::OUString aDot = rtl::OUString::createFromAscii(".");
const rtl::OUString aComma = rtl::OUString::createFromAscii(",");
const rtl::OUString aSpaceGroup = rtl::OUString::createFromAscii(" ");
const rtl::OUString aNoSep;

rtl::OUString Ascii( const char* pText )
{
    return rtl::OUString::createFromAscii( pText );
}

}  // namespace

TEST(BigFloatTest, ExactDecimalConstruction)
{
    // Reference values are generated independently, they are not what a double
    // would print.
    ScBigFloat aValue;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1.234567890123456789012345678901234567890"), aDot, aNoSep, aValue));
    EXPECT_EQ(Ascii("1.23456789012345678901234567890123456789"),
            ScBigFloatToString(aValue));

    ASSERT_TRUE(ScBigFloatFromString(Ascii("3.1415926535897932384626433832795028841971693993751"), aDot, aNoSep, aValue));
    EXPECT_EQ(Ascii("3.1415926535897932384626433832795028841971693993751"),
            ScBigFloatToString(aValue));
}

TEST(BigFloatTest, OneThird)
{
    const ScBigFloat aThird = ScBigFloat(1) / ScBigFloat(3);
    EXPECT_EQ(Ascii("0.333333333333333333333333333333333333333333333333"),
            ScBigFloatToString(aThird).copy(0, 50));
}

TEST(BigFloatTest, SqrtTwo)
{
    const ScBigFloat aRoot = boost::multiprecision::sqrt(ScBigFloat(2));
    EXPECT_EQ(Ascii("1.414213562373095048801688724209698078569671875376"),
            ScBigFloatToString(aRoot).copy(0, 50));
}

TEST(BigFloatTest, CatastrophicCancellationSurvives)
{
    // The whole point of the feature: two values that are indistinguishable
    // as doubles differ by exactly one here.
    ScBigFloat aBig, aBigMinusOne;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("10000000000000000000000000000000000001"), aDot, aNoSep, aBig));
    ASSERT_TRUE(ScBigFloatFromString(Ascii("10000000000000000000000000000000000000"), aDot, aNoSep, aBigMinusOne));
    EXPECT_EQ(Ascii("1"), ScBigFloatToString(aBig - aBigMinusOne));
    // ... and the double path cannot see the difference at all.
    EXPECT_DOUBLE_EQ(0.0, 1e37 - 1e37);
}

TEST(BigFloatTest, MultiplicationOfLongValues)
{
    ScBigFloat a, b;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1234567890123456789012345"), aDot, aNoSep, a));
    ASSERT_TRUE(ScBigFloatFromString(Ascii("9876543210987654321098765"), aDot, aNoSep, b));
    EXPECT_EQ(Ascii("12193263113702179522618496034720321071359549253925"),
            ScBigFloatToString(a * b));
}

TEST(BigFloatTest, Division)
{
    ScBigFloat a, b;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1"), aDot, aNoSep, a));
    ASSERT_TRUE(ScBigFloatFromString(Ascii("7"), aDot, aNoSep, b));
    EXPECT_EQ(Ascii("0.142857142857142857142857142857142857142857142857"),
            ScBigFloatToString(a / b).copy(0, 50));
}

TEST(BigFloatTest, LargeAndSmallExponents)
{
    ScBigFloat a;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1e40"), aDot, aNoSep, a));
    EXPECT_EQ(Ascii("10000000000000000000000000000000000000000"), ScBigFloatToString(a));
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1.5e-40"), aDot, aNoSep, a));
    EXPECT_EQ(Ascii("1.5e-40"), ScBigFloatToString(a));
    EXPECT_EQ(Ascii("1.5E-40"), ScBigFloatToString(a, ','));
}

TEST(BigFloatTest, RejectsInvalidInput)
{
    const char* pBad[] = { "", " ", "+", "-", ".", "1e", "1e+", "12x34", "1.2.3",
                           "1e999999999999999999", "1,5" };
    for (size_t i = 0; i < sizeof(pBad) / sizeof(pBad[0]); ++i)
    {
        ScBigFloat aUnchanged(42);
        EXPECT_FALSE(ScBigFloatFromString(Ascii(pBad[i]), aDot, aNoSep, aUnchanged))
                << "input <" << pBad[i] << "> was not rejected";
    }
}

TEST(BigFloatTest, LocaleSeparators)
{
    ScBigFloat aValue;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1 234,5"), aComma, aSpaceGroup, aValue));
    EXPECT_EQ(Ascii("1234.5"), ScBigFloatToString(aValue));
    EXPECT_EQ(Ascii("1234,5"), ScBigFloatToString(aValue, ','));
}

TEST(BigFloatTest, DisplayUsesLocaleDecimalSeparatorAndUpperCaseExponent)
{
    ScBigFloat aValue;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1.5e-7"), aDot, aNoSep, aValue));
    EXPECT_EQ(Ascii("1.5E-7"), ScBigFloatToString(aValue, ','));
}

TEST(BigFloatTest, NoImplicitConversionToDouble)
{
    // Narrowing is never implicit, that is what would silently destroy the
    // digits. The other direction, a double promoted to a BigFloat, is
    // allowed by Boost.Multiprecision and is lossless, so it is left alone.
    static_assert(!std::is_convertible<ScBigFloat, double>::value,
            "a BigFloat must not convert to double implicitly");
    EXPECT_DOUBLE_EQ(3.0, ScBigFloatToDouble(ScBigFloat(3)));
    EXPECT_EQ(Ascii("0.1000000000000000055511151231257827021181583404541015625"),
            ScBigFloatToString(ScBigFloatFromDouble(0.1)));
}

TEST(BigFloatTest, FiniteAndZero)
{
    ScBigFloat aInf = ScBigFloat(1);
    aInf /= ScBigFloat(0);
    EXPECT_FALSE(ScBigFloatIsFinite(aInf));
    EXPECT_TRUE(ScBigFloatIsFinite(ScBigFloat(1)));
    EXPECT_TRUE(ScBigFloatIsZero(ScBigFloat(0)));
    EXPECT_TRUE(ScBigFloatIsZero(ScBigFloat("-0.0")));
    EXPECT_FALSE(ScBigFloatIsZero(ScBigFloat("0.1")));
}

TEST(BigFloatTest, NaNAndInfinityAreDistinguishable)
{
    // The interpreter maps these to two different Calc errors, and it must be
    // able to tell them apart. A NaN carries no Calc error code in its bits, so
    // decoding one out of its payload would invent an arbitrary error code.
    ScBigFloat aInf = ScBigFloat(1);
    aInf /= ScBigFloat(0);
    ScBigFloat aNaN = aInf / aInf;
    ScBigFloat aZero(0);
    ScBigFloat aZeroDivZero = aZero / aZero;

    EXPECT_TRUE(ScBigFloatIsNaN(aNaN));
    EXPECT_TRUE(ScBigFloatIsNaN(aZeroDivZero));
    EXPECT_FALSE(ScBigFloatIsNaN(aInf));
    EXPECT_FALSE(ScBigFloatIsNaN(ScBigFloat(1)));
    EXPECT_FALSE(ScBigFloatIsNaN(ScBigFloat(0)));
    EXPECT_FALSE(ScBigFloatIsFinite(aNaN));
    EXPECT_FALSE(ScBigFloatIsFinite(aInf));
}

TEST(BigFloatTokenTest, CloneAndEquality)
{
    ScBigFloat aValue;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1.000000000000000000000000000000000000001"), aDot, aNoSep, aValue));

    ScBigFloatToken aToken(aValue);
    EXPECT_EQ(formula::svBigFloat, aToken.GetType());
    EXPECT_EQ(Ascii("1.000000000000000000000000000000000000001"), aToken.GetString());

    FormulaTokenRef xClone = aToken.Clone();
    EXPECT_EQ(formula::svBigFloat, xClone->GetType());
    EXPECT_TRUE(aToken == *xClone);
    EXPECT_EQ(aToken.GetString(), xClone->GetString());

    // Different values are not equal, and a BigFloat token is never equal to
    // a plain double token.
    ScBigFloatToken aOther(ScBigFloat("2"));
    EXPECT_FALSE(aToken == aOther);
    FormulaDoubleToken aDouble(1.0);
    EXPECT_FALSE(aToken == aDouble);
}

TEST(BigFloatTokenTest, ValueAccessors)
{
    ScBigFloat aValue("0.5");
    ScBigFloatToken aToken(aValue);
    const ScBigFloat* pGot = ScGetBigFloatValue(&aToken);
    ASSERT_TRUE(pGot != NULL);
    EXPECT_TRUE(*pGot == aValue);
    // GetDouble() is the explicit, lossy way out.
    EXPECT_DOUBLE_EQ(0.5, aToken.GetDouble());
    // A token that is not a BigFloat yields NULL, not a wrong value.
    FormulaDoubleToken aDouble(1.0);
    EXPECT_TRUE(ScGetBigFloatValue(&aDouble) == NULL);
}

/**************************************************************
 *
 *  The test below is the round trip the design note requires: decimal text ->
 *  BigFloat -> token -> text -> BigFloat, at 20, 30, 40 and 50 digits.
 *
 *************************************************************/
TEST(BigFloatTest, RoundTrip)
{
    const char* pTexts[] = {
        "1.2345678901234567891",
        "1.2345678901234567890123456789",
        "1.2345678901234567890123456789012345678",
        "1.2345678901234567890123456789012345678901234567" };
    for (size_t i = 0; i < sizeof(pTexts) / sizeof(pTexts[0]); ++i)
    {
        ScBigFloat aValue, aAgain;
        ASSERT_TRUE(ScBigFloatFromString(Ascii(pTexts[i]), aDot, aNoSep, aValue))
                << pTexts[i];
        ScBigFloatToken aToken(aValue);
        ASSERT_TRUE(ScBigFloatFromString(aToken.GetString(), aDot, aNoSep, aAgain))
                << pTexts[i];
        EXPECT_EQ(aValue, aAgain) << pTexts[i];
        EXPECT_EQ(Ascii(pTexts[i]), aToken.GetString()) << pTexts[i];
    }
}

TEST(BigFloatTest, TrailingZerosAreNotPartOfTheValue)
{
    // 1.50 and 1.5 are the same number, the display drops the zero. That is a
    // property of the text, not a loss of digits.
    ScBigFloat aValue, aAgain;
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1.50"), aDot, aNoSep, aValue));
    EXPECT_EQ(Ascii("1.5"), ScBigFloatToString(aValue));
    ASSERT_TRUE(ScBigFloatFromString(Ascii("1.5"), aDot, aNoSep, aAgain));
    EXPECT_EQ(aValue, aAgain);
}
