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


#ifndef _IDLC_ASTINTERFACEMEMBER_HXX_
#define _IDLC_ASTINTERFACEMEMBER_HXX_

#include <idlc/astinterface.hxx>

class AstInterfaceMember : public AstDeclaration
{
public:
	AstInterfaceMember(const sal_uInt32 flags, AstInterface* pRealInterface, 
					   const ::rtl::OString& name, AstScope* pScope)
		: AstDeclaration(NT_interface_member, name, pScope)
		, m_flags(flags)
		, m_pRealInterface(pRealInterface)
		{}
	virtual ~AstInterfaceMember() {}

	AstInterface* getRealInterface()
		{ return m_pRealInterface; }
	sal_Bool isOptional() 
		{ return ((m_flags & AF_OPTIONAL) == AF_OPTIONAL); }
private:
	const sal_uInt32	m_flags;
	AstInterface*		m_pRealInterface;
};

#endif // _IDLC_ASTINTERFACEMEMBER_HXX_

