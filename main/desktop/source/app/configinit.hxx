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

#ifndef _DESKTOP_CONFIGINIT_HXX_
#define _DESKTOP_CONFIGINIT_HXX_

#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <rtl/ustring.hxx>

/** creates a ConfigurationProvider instance
Important: exceptions thrown from that method will contain a readily
displayable message.

    @return 
        The default configuration provider for the application or<br/>
        <NULL/>, if startup was canceled

    @throw com::sun::star::configuration::CannotLoadConfigurationException
        if the configuration provider can't be created

    @throw com::sun::star::lang::ServiceNotRegisteredException
        if the ConfigurationProvider service is unknown
        
    @throw com::sun::star::lang::WrappedTargetException
        if the configuration backend could be created,
        but incurred a failure later
        
*/
extern
com::sun::star::uno::Reference< com::sun::star::lang::XMultiServiceFactory >
    CreateApplicationConfigurationProvider( );

//-----------------------------------------------------------------------------
#include <com/sun/star/task/XInteractionHandler.hpp>

/**
    sets an InteractionHandler for configuration errors in the current context.

  */
    class ConfigurationErrorHandler
    {
    public:
        typedef com::sun::star::uno::Reference< com::sun::star::task::XInteractionHandler > InteractionHandler;
        
        /// Constructor: Uses the default interaction handler
        ConfigurationErrorHandler()
        : m_pContext(0), m_xHandler()
        {}

        /// Constructor: Uses an externally provided interaction handler
        ConfigurationErrorHandler(const InteractionHandler & xHandler)
        : m_pContext(0), m_xHandler( xHandler )
        {}

        ~ConfigurationErrorHandler();

        static InteractionHandler getDefaultInteractionHandler();

        /// installs the handler into the current context
        void activate();
        /// deinstalls the handler from the current context, restoring the previous context
        void deactivate();
    private: 
        class Context;
        Context * m_pContext;
        InteractionHandler m_xHandler;
    private: 
        // not implemented - suppress copy
        ConfigurationErrorHandler(const ConfigurationErrorHandler&);
        void operator=(const ConfigurationErrorHandler&);
    };

//-----------------------------------------------------------------------------
#endif
