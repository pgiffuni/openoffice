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



#ifndef _SHELLEXEC_HXX_
#define _SHELLEXEC_HXX_

#include <cppuhelper/implbase2.hxx>
#include <osl/mutex.hxx>
#include <rtl/strbuf.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#ifndef _COM_SUN_STAR_SYS_SHELL_XSYSTEMSHELLEXECUTE_HPP_
#include <com/sun/star/system/XSystemShellExecute.hpp>
#endif

//----------------------------------------------------------
// class declaration		
//----------------------------------------------------------

class ShellExec : public ::cppu::WeakImplHelper2< com::sun::star::system::XSystemShellExecute, com::sun::star::lang::XServiceInfo >
{
    ::rtl::OString m_aDesktopEnvironment;
    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext >
    m_xContext;
    
public:
    ShellExec(const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext >& xContext);

    //------------------------------------------------
    // XSystemShellExecute
    //------------------------------------------------ 

    virtual void SAL_CALL execute( const ::rtl::OUString& aCommand, const ::rtl::OUString& aParameter, sal_Int32 nFlags ) 
        throw (::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::system::SystemShellExecuteException, ::com::sun::star::uno::RuntimeException);

    //------------------------------------------------
    // XServiceInfo
    //------------------------------------------------ 
    
    virtual ::rtl::OUString SAL_CALL getImplementationName(	 )
        throw(::com::sun::star::uno::RuntimeException);

    virtual sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) 
        throw(::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) 
        throw(::com::sun::star::uno::RuntimeException);    
}; 


// helper function - needed for urltest
void escapeForShell( rtl::OStringBuffer & rBuffer, const rtl::OString & rURL);

#endif 
