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



#include <rtl/string.hxx>

#include "bigfloattoken.hxx"



ScBigFloatToken::ScBigFloatToken( const ScBigFloat& rValue ) :
    ScToken( formula::svBigFloat ),
    maValue( rValue ),
    maString( ScBigFloatToString( rValue ))
{
}


ScBigFloatToken::ScBigFloatToken( const ScBigFloatToken& r ) :
    ScToken( r ),
    maValue( r.maValue ),
    maString( r.maString )
{
}


sal_Bool ScBigFloatToken::operator==( const formula::FormulaToken& rToken ) const
{
    // FormulaToken::operator== compares the type first, so a BigFloat token is
    // never equal to a double or string token, whatever the values are.
    return FormulaToken::operator==( rToken ) &&
        maValue == static_cast< const ScBigFloatToken& >( rToken ).maValue;
}


const String& ScBigFloatToken::GetString() const
{
    return maString;
}


double ScBigFloatToken::GetDouble() const
{
    return ScBigFloatToDouble( maValue );
}


const ScBigFloatToken* ScGetBigFloatToken( const formula::FormulaToken* pToken )
{
    // The type is checked before the cast, so this is safe for any token.
    if (pToken && pToken->GetType() == formula::svBigFloat)
        return static_cast< const ScBigFloatToken* >( pToken );
    return NULL;
}


const ScBigFloat* ScGetBigFloatValue( const formula::FormulaToken* pToken )
{
    const ScBigFloatToken* pBigToken = ScGetBigFloatToken( pToken );
    return pBigToken ? &pBigToken->GetBigFloat() : NULL;
}
