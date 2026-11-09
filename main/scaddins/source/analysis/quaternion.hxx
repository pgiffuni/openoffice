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

#ifndef QUATERNION_HXX
#define QUATERNION_HXX

#include <sal/types.h>
#include <rtl/ustring.hxx>
#include <boost/qvm/all.hpp>

#include "analysisdefs.hxx"

class Quaternion
{
    boost::qvm::quat<double>    Num;

public:
    inline                      Quaternion( double fReal = 0.0, double fI = 0.0, double fJ = 0.0, double fK = 0.0 );
                                Quaternion( const STRING& rQuatAsString );

    static sal_Bool             ParseString( const STRING& rQuatAsString, Quaternion& rReturn );
    STRING                      GetString() const;

    inline double               Real( void ) const;
    inline double               I( void ) const;
    inline double               J( void ) const;
    inline double               K( void ) const;

    double                      Abs( void ) const;

    static Quaternion           Conjugate( const Quaternion& rQ );
    static Quaternion           Inverse( const Quaternion& rQ );
    static Quaternion           Product( const Quaternion& rQ1, const Quaternion& rQ2 );
    static Quaternion           Normalize( const Quaternion& rQ );
    static STRING               Rotate( const Quaternion& rQ, double fX, double fY, double fZ );
    static Quaternion           Slerp( const Quaternion& rQ1, const Quaternion& rQ2, double fT );
};

inline Quaternion::Quaternion( double fReal, double fI, double fJ, double fK )
    : Num{ { fReal, fI, fJ, fK } }
{
}

inline double Quaternion::Real( void ) const
{
    return Num.a[0];
}

inline double Quaternion::I( void ) const
{
    return Num.a[1];
}

inline double Quaternion::J( void ) const
{
    return Num.a[2];
}

inline double Quaternion::K( void ) const
{
    return Num.a[3];
}

#endif