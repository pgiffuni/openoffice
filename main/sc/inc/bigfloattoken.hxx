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


#ifndef SC_BIGFLOATTOKEN_HXX
#define SC_BIGFLOATTOKEN_HXX

#include "bigfloat.hxx"
#include "token.hxx"



/** Formula token that carries a high precision decimal number.

    This is the second numeric value category next to formula::svDouble. It
    travels through the interpreter stack and the ScFormulaResult token slot
    exactly like the string, empty cell and matrix results do, which is what
    keeps the memory and performance characteristics of ordinary Calc
    calculations unchanged: a double result is still stored directly in the
    result union and never allocates a token.

    There is deliberately no implicit conversion to double. GetDouble() exists
    because formula::FormulaToken declares it as the virtual that
    ScFormulaResult uses to narrow a result for the legacy double APIs, and it
    is a lossy conversion by definition. */
class SC_DLLPUBLIC ScBigFloatToken : public ScToken
{
private:
                                ScBigFloat     maValue;
                                /** Decimal text of maValue, kept so that
                                    GetString() can hand out a reference. */
                                String          maString;

public:
                                ScBigFloatToken( const ScBigFloat& rValue );
                                ScBigFloatToken( const ScBigFloatToken& r );

    virtual formula::FormulaToken*  Clone() const { return new ScBigFloatToken( *this); }
    virtual sal_Bool                operator==( const formula::FormulaToken& rToken ) const;

    /** The exact value, no conversion involved. */
    const ScBigFloat &     GetBigFloat() const { return maValue; }

    /** The decimal representation, see ScBigFloatToString(). This is a
        controlled conversion to text for display and serialization, it is not
        what the cell stores. */
    virtual const String & GetString() const;

    /** Explicit, lossy narrowing to a native double. Only reached when a
        caller explicitly asks a BigFloat token for a double, never inside a
        high precision calculation. */
    virtual double         GetDouble() const;
};


/** The token itself, or NULL if it is anything else. Callers that only want
    the number use ScGetBigFloatValue() below. */
SC_DLLPUBLIC const ScBigFloatToken*    ScGetBigFloatToken( const formula::FormulaToken* pToken );

/** The high precision value of a formula token, or NULL if the token is
    anything else. Safe for every token, no cast is needed by the caller. */
SC_DLLPUBLIC const ScBigFloat*     ScGetBigFloatValue( const formula::FormulaToken* pToken );

#endif  // SC_BIGFLOATTOKEN_HXX
