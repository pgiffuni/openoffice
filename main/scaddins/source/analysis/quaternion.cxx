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

#include "quaternion.hxx"

#include <stdio.h>
#include <math.h>
#include <rtl/math.hxx>
#include "analysishelper.hxx"

using namespace ::rtl;

sal_Bool Quaternion::ParseString( const STRING& rStr, Quaternion& rQuat )
{
    rQuat.Num = boost::qvm::quat<double>{ { 0.0, 0.0, 0.0, 0.0 } };

    const sal_Unicode* p = rStr.getStr();

    if( rStr.isEmpty() )
        return sal_False;

    double a = 0.0, b = 0.0, c = 0.0, d = 0.0;
    sal_Bool bGotReal = sal_False;
    sal_Bool bGotI = sal_False;
    sal_Bool bGotJ = sal_False;
    sal_Bool bGotK = sal_False;

    while( *p )
    {
        sal_Bool bNeg = sal_False;
        if( *p == '+' )
            p++;
        else if( *p == '-' )
        {
            bNeg = sal_True;
            p++;
        }

        if( *p == 0 )
            return sal_False;

        double f;

        if( *p == 'i' || *p == 'j' || *p == 'k' )
        {
            f = bNeg ? -1.0 : 1.0;
        }
        else
        {
            if( !ParseDouble( p, f ) )
                return sal_False;
            if( bNeg )
                f = -f;
        }

        if( *p == 'i' )
        {
            if( bGotI )
                return sal_False;
            b = f;
            bGotI = sal_True;
            p++;
        }
        else if( *p == 'j' )
        {
            if( bGotJ )
                return sal_False;
            c = f;
            bGotJ = sal_True;
            p++;
        }
        else if( *p == 'k' )
        {
            if( bGotK )
                return sal_False;
            d = f;
            bGotK = sal_True;
            p++;
        }
        else
        {
            if( bGotReal || bGotI || bGotJ || bGotK )
                return sal_False;
            a = f;
            bGotReal = sal_True;
        }
    }

    if( !bGotReal && !bGotI && !bGotJ && !bGotK )
        return sal_False;

    rQuat.Num = boost::qvm::quat<double>{ { a, b, c, d } };
    return sal_True;
}


Quaternion::Quaternion( const STRING& rStr )
{
    if( !ParseString( rStr, *this ) )
        THROW_IAE;
}


STRING Quaternion::GetString() const
{
    CHK_FINITE(Real());
    CHK_FINITE(I());
    CHK_FINITE(J());
    CHK_FINITE(K());

    double a = Real();
    double b = I();
    double c = J();
    double d = K();

    static const OUString aI( OUString::createFromAscii("i") );
    static const OUString aJ( OUString::createFromAscii("j") );
    static const OUString aK( OUString::createFromAscii("k") );
    static const OUString aPlus( OUString::createFromAscii("+") );
    static const OUString aMinus( OUString::createFromAscii("-") );

    STRING aRet;
    sal_Bool bFirst = sal_True;

    if( a != 0.0 )
    {
        aRet = ::GetString( a );
        bFirst = sal_False;
    }

    if( b != 0.0 )
    {
        if( b == 1.0 )
        {
            if( !bFirst )
                aRet += aPlus;
        }
        else if( b == -1.0 )
        {
            aRet += aMinus;
        }
        else
        {
            if( !bFirst )
            {
                if( b > 0.0 )
                    aRet += aPlus;
                aRet += ::GetString( b, true );
            }
            else
                aRet += ::GetString( b );
        }
        aRet += aI;
        bFirst = sal_False;
    }

    if( c != 0.0 )
    {
        if( c == 1.0 )
        {
            if( !bFirst )
                aRet += aPlus;
        }
        else if( c == -1.0 )
        {
            aRet += aMinus;
        }
        else
        {
            if( !bFirst )
            {
                if( c > 0.0 )
                    aRet += aPlus;
                aRet += ::GetString( c, true );
            }
            else
                aRet += ::GetString( c );
        }
        aRet += aJ;
        bFirst = sal_False;
    }

    if( d != 0.0 )
    {
        if( d == 1.0 )
        {
            if( !bFirst )
                aRet += aPlus;
        }
        else if( d == -1.0 )
        {
            aRet += aMinus;
        }
        else
        {
            if( !bFirst )
            {
                if( d > 0.0 )
                    aRet += aPlus;
                aRet += ::GetString( d, true );
            }
            else
                aRet += ::GetString( d );
        }
        aRet += aK;
        bFirst = sal_False;
    }

    if( a == 0.0 && b == 0.0 && c == 0.0 && d == 0.0 )
        return ::GetString( 0.0 );

    return aRet;
}


double Quaternion::Abs( void ) const
{
    return boost::qvm::mag( Num );
}


Quaternion Quaternion::Conjugate( const Quaternion& rQ )
{
    Quaternion aResult;
    aResult.Num = boost::qvm::conjugate( rQ.Num );
    return aResult;
}


Quaternion Quaternion::Inverse( const Quaternion& rQ )
{
    Quaternion aResult;
    aResult.Num = boost::qvm::inverse( rQ.Num );
    return aResult;
}


Quaternion Quaternion::Product( const Quaternion& rQ1, const Quaternion& rQ2 )
{
    Quaternion aResult;
    aResult.Num = rQ1.Num * rQ2.Num;
    return aResult;
}


Quaternion Quaternion::Normalize( const Quaternion& rQ )
{
    Quaternion aResult( rQ );
    boost::qvm::normalize( aResult.Num );
    return aResult;
}


STRING Quaternion::Rotate( const Quaternion& rQ, double fX, double fY, double fZ )
{
    boost::qvm::vec<double, 3> aVec{ { fX, fY, fZ } };
    boost::qvm::vec<double, 3> aResult = rQ.Num * aVec;
    static const OUString aComma( OUString::createFromAscii(",") );
    return ::GetString( aResult.a[0] ) + aComma + ::GetString( aResult.a[1] ) + aComma + ::GetString( aResult.a[2] );
}


Quaternion Quaternion::Slerp( const Quaternion& rQ1, const Quaternion& rQ2, double fT )
{
    Quaternion aResult;
    aResult.Num = boost::qvm::slerp180( rQ1.Num, rQ2.Num, fT );
    return aResult;
}