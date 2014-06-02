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
#include "precompiled_xmloff.hxx"

#include "PageMasterExportPropMapper.hxx"
#include <xmloff/xmltoken.hxx>
#include <comphelper/types.hxx>
#include <com/sun/star/table/BorderLine.hpp>
#include <xmloff/PageMasterStyleMap.hxx>
#include <tools/debug.hxx>
#include <rtl/ustrbuf.hxx>
#include <comphelper/extract.hxx>

//UUUU
#include <xmloff/txtprmap.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::comphelper;
using namespace ::xmloff::token;

//______________________________________________________________________________

inline sal_Bool lcl_HasSameLineWidth( const table::BorderLine& rLine1, const table::BorderLine& rLine2 )
{
	return	(rLine1.InnerLineWidth == rLine2.InnerLineWidth) &&
			(rLine1.OuterLineWidth == rLine2.OuterLineWidth) &&
			(rLine1.LineDistance == rLine2.LineDistance);
}

inline sal_Bool operator==( const table::BorderLine& rLine1, const table::BorderLine& rLine2 )
{
	return	(rLine1.Color == rLine2.Color) &&
			lcl_HasSameLineWidth( rLine1, rLine2 );
}

inline void lcl_RemoveState( XMLPropertyState* pState )
{
	pState->mnIndex = -1;
	pState->maValue.clear();
}

void lcl_RemoveStateIfZero16( XMLPropertyState* pState )
{
	sal_Int16	nValue = sal_Int16();
	if( (pState->maValue >>= nValue) && !nValue )
		lcl_RemoveState( pState );
}

void lcl_AddState(::std::vector< XMLPropertyState >& rPropState, sal_Int32 nIndex, const rtl::OUString& rProperty, uno::Reference< beans::XPropertySet >& xProps)
{
    if(::cppu::any2bool(xProps->getPropertyValue(rProperty)))
        rPropState.push_back(XMLPropertyState (nIndex, cppu::bool2any(sal_True)));
}

//______________________________________________________________________________
// helper struct to handle equal XMLPropertyState's for page, header and footer

struct XMLPropertyStateBuffer
{
    XMLPropertyState*       pPMMarginAll;
    XMLPropertyState*       pPMMarginTop;
    XMLPropertyState*       pPMMarginBottom;
    XMLPropertyState*       pPMMarginLeft;
    XMLPropertyState*       pPMMarginRight;

	XMLPropertyState*		pPMBorderAll;
	XMLPropertyState*		pPMBorderTop;
	XMLPropertyState*		pPMBorderBottom;
	XMLPropertyState*		pPMBorderLeft;
	XMLPropertyState*		pPMBorderRight;

	XMLPropertyState*		pPMBorderWidthAll;
	XMLPropertyState*		pPMBorderWidthTop;
	XMLPropertyState*		pPMBorderWidthBottom;
	XMLPropertyState*		pPMBorderWidthLeft;
	XMLPropertyState*		pPMBorderWidthRight;

	XMLPropertyState*		pPMPaddingAll;
	XMLPropertyState*		pPMPaddingTop;
	XMLPropertyState*		pPMPaddingBottom;
	XMLPropertyState*		pPMPaddingLeft;
	XMLPropertyState*		pPMPaddingRight;

							XMLPropertyStateBuffer();
	void					ContextFilter( ::std::vector< XMLPropertyState >& rPropState );
};

XMLPropertyStateBuffer::XMLPropertyStateBuffer()
    :   pPMMarginAll( NULL )
    ,   pPMMarginTop( NULL )
    ,   pPMMarginBottom( NULL )
    ,   pPMMarginLeft( NULL )
    ,   pPMMarginRight( NULL )
    ,
		pPMBorderAll( NULL ),
		pPMBorderTop( NULL ),
		pPMBorderBottom( NULL ),
		pPMBorderLeft( NULL ),
		pPMBorderRight( NULL ),

		pPMBorderWidthAll( NULL ),
		pPMBorderWidthTop( NULL ),
		pPMBorderWidthBottom( NULL ),
		pPMBorderWidthLeft( NULL ),
		pPMBorderWidthRight( NULL ),

		pPMPaddingAll( NULL ),
		pPMPaddingTop( NULL ),
		pPMPaddingBottom( NULL ),
		pPMPaddingLeft( NULL ),
		pPMPaddingRight( NULL )
{
}

void XMLPropertyStateBuffer::ContextFilter( ::std::vector< XMLPropertyState >& )
{
    if (pPMMarginAll)
    {
        lcl_RemoveState(pPMMarginAll); // #i117696# do not write fo:margin
    }

	if( pPMBorderAll )
	{
		if( pPMBorderTop && pPMBorderBottom && pPMBorderLeft && pPMBorderRight )
		{
			table::BorderLine aLineTop, aLineBottom, aLineLeft, aLineRight;

			pPMBorderTop->maValue >>= aLineTop;
			pPMBorderBottom->maValue >>= aLineBottom;
			pPMBorderLeft->maValue >>= aLineLeft;
			pPMBorderRight->maValue >>= aLineRight;

			if( (aLineTop == aLineBottom) && (aLineBottom == aLineLeft) && (aLineLeft == aLineRight) )
			{
				lcl_RemoveState( pPMBorderTop );
				lcl_RemoveState( pPMBorderBottom );
				lcl_RemoveState( pPMBorderLeft );
				lcl_RemoveState( pPMBorderRight );
			}
			else
				lcl_RemoveState( pPMBorderAll );
		}
		else
			lcl_RemoveState( pPMBorderAll );
	}

	if( pPMBorderWidthAll )
	{
		if( pPMBorderWidthTop && pPMBorderWidthBottom && pPMBorderWidthLeft && pPMBorderWidthRight )
		{
			table::BorderLine aLineTop, aLineBottom, aLineLeft, aLineRight;

			pPMBorderWidthTop->maValue >>= aLineTop;
			pPMBorderWidthBottom->maValue >>= aLineBottom;
			pPMBorderWidthLeft->maValue >>= aLineLeft;
			pPMBorderWidthRight->maValue >>= aLineRight;

			if( lcl_HasSameLineWidth( aLineTop, aLineBottom ) &&
				lcl_HasSameLineWidth( aLineBottom, aLineLeft ) &&
				lcl_HasSameLineWidth( aLineLeft, aLineRight ) )
			{
				lcl_RemoveState( pPMBorderWidthTop );
				lcl_RemoveState( pPMBorderWidthBottom );
				lcl_RemoveState( pPMBorderWidthLeft );
				lcl_RemoveState( pPMBorderWidthRight );
			}
			else
				lcl_RemoveState( pPMBorderWidthAll );
		}
		else
			lcl_RemoveState( pPMBorderWidthAll );
	}

	if( pPMPaddingAll )
	{
		if( pPMPaddingTop && pPMPaddingBottom && pPMPaddingLeft && pPMPaddingRight )
		{
			sal_Int32 nTop = 0, nBottom = 0, nLeft = 0, nRight = 0;

			pPMPaddingTop->maValue >>= nTop;
			pPMPaddingBottom->maValue >>= nBottom;
			pPMPaddingLeft->maValue >>= nLeft;
			pPMPaddingRight->maValue >>= nRight;

			if( (nTop == nBottom) && (nBottom == nLeft) && (nLeft == nRight) )
			{
				lcl_RemoveState( pPMPaddingTop );
				lcl_RemoveState( pPMPaddingBottom );
				lcl_RemoveState( pPMPaddingLeft );
				lcl_RemoveState( pPMPaddingRight );
			}
			else
				lcl_RemoveState( pPMPaddingAll );
		}
		else
			lcl_RemoveState( pPMPaddingAll );
	}
}

//______________________________________________________________________________

XMLPageMasterExportPropMapper::XMLPageMasterExportPropMapper(
		const UniReference< XMLPropertySetMapper >& rMapper,
		SvXMLExport& rExport ) :
	SvXMLExportPropertyMapper( rMapper ),
	aBackgroundImageExport( rExport ),
	aTextColumnsExport( rExport ),
	aFootnoteSeparatorExport( rExport )
{
}

XMLPageMasterExportPropMapper::~XMLPageMasterExportPropMapper()
{
}

void XMLPageMasterExportPropMapper::handleElementItem(
        SvXMLExport&,
		const XMLPropertyState& rProperty,
		sal_uInt16 /*nFlags*/,
		const ::std::vector< XMLPropertyState >* pProperties,
		sal_uInt32 nIdx ) const
{
	XMLPageMasterExportPropMapper* pThis = (XMLPageMasterExportPropMapper*) this;

	sal_uInt32 nContextId = getPropertySetMapper()->GetEntryContextId( rProperty.mnIndex );
	switch( nContextId )
	{
		case CTF_PM_GRAPHICURL:
		case CTF_PM_HEADERGRAPHICURL:
		case CTF_PM_FOOTERGRAPHICURL:
			{
				DBG_ASSERT( pProperties && (nIdx >= 2), "property vector missing" );
				sal_Int32 nPos;
				sal_Int32 nFilter;
				switch( nContextId  )
				{
				case CTF_PM_GRAPHICURL:
					nPos = CTF_PM_GRAPHICPOSITION;
					nFilter = CTF_PM_GRAPHICFILTER;
					break;
				case CTF_PM_HEADERGRAPHICURL:
					nPos = CTF_PM_HEADERGRAPHICPOSITION;
					nFilter = CTF_PM_HEADERGRAPHICFILTER;
					break;
				case CTF_PM_FOOTERGRAPHICURL:
					nPos = CTF_PM_FOOTERGRAPHICPOSITION;
					nFilter = CTF_PM_FOOTERGRAPHICFILTER;
					break;
				default:
					nPos = 0;  // TODO What values should this be?
					nFilter = 0;
				}
				const Any*	pPos	= NULL;
				const Any*	pFilter	= NULL;
				if( pProperties && (nIdx >= 2) )
				{
					const XMLPropertyState& rPos = (*pProperties)[nIdx - 2];
					DBG_ASSERT( getPropertySetMapper()->GetEntryContextId( rPos.mnIndex ) == nPos,
								"invalid property map: pos expected" );
					if( getPropertySetMapper()->GetEntryContextId( rPos.mnIndex ) == nPos )
						pPos = &rPos.maValue;

					const XMLPropertyState& rFilter = (*pProperties)[nIdx - 1];
					DBG_ASSERT( getPropertySetMapper()->GetEntryContextId( rFilter.mnIndex ) == nFilter,
								"invalid property map: filter expected" );
					if( getPropertySetMapper()->GetEntryContextId( rFilter.mnIndex ) == nFilter )
						pFilter = &rFilter.maValue;
				}
				sal_uInt32 nPropIndex = rProperty.mnIndex;
				pThis->aBackgroundImageExport.exportXML( rProperty.maValue, pPos, pFilter, NULL,
					getPropertySetMapper()->GetEntryNameSpace( nPropIndex ),
					getPropertySetMapper()->GetEntryXMLName( nPropIndex ) );
			}
			break;
		case CTF_PM_TEXTCOLUMNS:
			pThis->aTextColumnsExport.exportXML( rProperty.maValue );
			break;
		case CTF_PM_FTN_LINE_WEIGTH:
			pThis->aFootnoteSeparatorExport.exportXML( pProperties, nIdx, 
													   getPropertySetMapper());
			break;
	}
}

void XMLPageMasterExportPropMapper::handleSpecialItem(
		SvXMLAttributeList&,
		const XMLPropertyState&,
		const SvXMLUnitConverter&,
		const SvXMLNamespaceMap&,
		const ::std::vector< XMLPropertyState >*,
		sal_uInt32 /*nIdx*/) const
{
}

void XMLPageMasterExportPropMapper::ContextFilter(
		::std::vector< XMLPropertyState >& rPropState,
		Reference< XPropertySet > rPropSet ) const
{
	XMLPropertyStateBuffer	aPageBuffer;
	XMLPropertyStateBuffer	aHeaderBuffer;
	XMLPropertyStateBuffer	aFooterBuffer;

	XMLPropertyState*		pPMHeaderHeight		= NULL;
	XMLPropertyState*		pPMHeaderMinHeight	= NULL;
	XMLPropertyState*		pPMHeaderDynamic	= NULL;

	XMLPropertyState*		pPMFooterHeight		= NULL;
	XMLPropertyState*		pPMFooterMinHeight	= NULL;
	XMLPropertyState*		pPMFooterDynamic	= NULL;

	XMLPropertyState*		pPMScaleTo			= NULL;
	XMLPropertyState*		pPMScaleToPages		= NULL;
	XMLPropertyState*		pPMScaleToX 		= NULL;
	XMLPropertyState*		pPMScaleToY	    	= NULL;
	XMLPropertyState*		pPMStandardMode    	= NULL;
	XMLPropertyState*		pPMGridBaseWidth   	= NULL;
	XMLPropertyState*		pPMGridSnapToChars 	= NULL;

    XMLPropertyState*       pPrint              = NULL;

    //UUUU
    XMLPropertyState* pRepeatOffsetX = NULL;
    XMLPropertyState* pRepeatOffsetY = NULL;
    XMLPropertyState* pHeaderRepeatOffsetX = NULL;
    XMLPropertyState* pHeaderRepeatOffsetY = NULL;
    XMLPropertyState* pFooterRepeatOffsetX = NULL;
    XMLPropertyState* pFooterRepeatOffsetY = NULL;

    UniReference < XMLPropertySetMapper > aPropMapper(getPropertySetMapper());

	for( ::std::vector< XMLPropertyState >::iterator aIter = rPropState.begin(); aIter != rPropState.end(); ++aIter )
	{
        XMLPropertyState *pProp = &(*aIter);
		sal_Int16 nContextId	= aPropMapper->GetEntryContextId( pProp->mnIndex );
		sal_Int16 nFlag			= nContextId & CTF_PM_FLAGMASK;
		sal_Int16 nSimpleId		= nContextId & (~CTF_PM_FLAGMASK | XML_PM_CTF_START);
        sal_Int16 nPrintId      = nContextId & CTF_PM_PRINTMASK;

		XMLPropertyStateBuffer* pBuffer;
		switch( nFlag )
		{
			case CTF_PM_HEADERFLAG:			pBuffer = &aHeaderBuffer;	break;
			case CTF_PM_FOOTERFLAG:			pBuffer = &aFooterBuffer;	break;
			default:						pBuffer = &aPageBuffer;		break;
		}

		switch( nSimpleId )
		{
            case CTF_PM_MARGINALL:          pBuffer->pPMMarginAll           = pProp;    break;
            case CTF_PM_MARGINTOP:          pBuffer->pPMMarginTop           = pProp;    break;
            case CTF_PM_MARGINBOTTOM:       pBuffer->pPMMarginBottom        = pProp;    break;
            case CTF_PM_MARGINLEFT:         pBuffer->pPMMarginLeft          = pProp;    break;
            case CTF_PM_MARGINRIGHT:        pBuffer->pPMMarginRight         = pProp;    break;
			case CTF_PM_BORDERALL:			pBuffer->pPMBorderAll			= pProp;	break;
			case CTF_PM_BORDERTOP:			pBuffer->pPMBorderTop			= pProp;	break;
			case CTF_PM_BORDERBOTTOM:		pBuffer->pPMBorderBottom		= pProp;	break;
			case CTF_PM_BORDERLEFT:			pBuffer->pPMBorderLeft			= pProp;	break;
			case CTF_PM_BORDERRIGHT:		pBuffer->pPMBorderRight			= pProp;	break;
			case CTF_PM_BORDERWIDTHALL:		pBuffer->pPMBorderWidthAll		= pProp;	break;
			case CTF_PM_BORDERWIDTHTOP:		pBuffer->pPMBorderWidthTop		= pProp;	break;
			case CTF_PM_BORDERWIDTHBOTTOM:	pBuffer->pPMBorderWidthBottom	= pProp;	break;
			case CTF_PM_BORDERWIDTHLEFT:	pBuffer->pPMBorderWidthLeft		= pProp;	break;
			case CTF_PM_BORDERWIDTHRIGHT:	pBuffer->pPMBorderWidthRight	= pProp;	break;
			case CTF_PM_PADDINGALL:			pBuffer->pPMPaddingAll			= pProp;	break;
			case CTF_PM_PADDINGTOP:			pBuffer->pPMPaddingTop			= pProp;	break;
			case CTF_PM_PADDINGBOTTOM:		pBuffer->pPMPaddingBottom		= pProp;	break;
			case CTF_PM_PADDINGLEFT:		pBuffer->pPMPaddingLeft			= pProp;	break;
			case CTF_PM_PADDINGRIGHT:		pBuffer->pPMPaddingRight		= pProp;	break;
		}

		switch( nContextId )
		{
			case CTF_PM_HEADERHEIGHT:		pPMHeaderHeight		= pProp;	break;
			case CTF_PM_HEADERMINHEIGHT:	pPMHeaderMinHeight	= pProp;	break;
			case CTF_PM_HEADERDYNAMIC:		pPMHeaderDynamic	= pProp;	break;
			case CTF_PM_FOOTERHEIGHT:		pPMFooterHeight		= pProp;	break;
			case CTF_PM_FOOTERMINHEIGHT:	pPMFooterMinHeight	= pProp;	break;
			case CTF_PM_FOOTERDYNAMIC:		pPMFooterDynamic	= pProp;	break;
			case CTF_PM_SCALETO:			pPMScaleTo			= pProp;	break;
			case CTF_PM_SCALETOPAGES:		pPMScaleToPages		= pProp;	break;
            case CTF_PM_SCALETOX:   		pPMScaleToX 		= pProp;	break;
            case CTF_PM_SCALETOY:   		pPMScaleToY 		= pProp;	break;
            case CTF_PM_STANDARD_MODE:		pPMStandardMode		= pProp;	break;
            case CTP_PM_GRID_BASE_WIDTH:		pPMGridBaseWidth	= pProp;	break;
            case CTP_PM_GRID_SNAP_TO_CHARS:		pPMGridSnapToChars	= pProp;	break;

            //UUUU
            case CTF_PM_REPEAT_OFFSET_X:
                pRepeatOffsetX = pProp;
                break;

            //UUUU
            case CTF_PM_REPEAT_OFFSET_Y:
                pRepeatOffsetY = pProp;
                break;

            //UUUU
            case CTF_PM_HEADERREPEAT_OFFSET_X:
                pHeaderRepeatOffsetX = pProp;
                break;

            //UUUU
            case CTF_PM_HEADERREPEAT_OFFSET_Y:
                pHeaderRepeatOffsetY = pProp;
                break;

            //UUUU
            case CTF_PM_FOOTERREPEAT_OFFSET_X:
                pFooterRepeatOffsetX = pProp;
                break;

            //UUUU
            case CTF_PM_FOOTERREPEAT_OFFSET_Y:
                pFooterRepeatOffsetY = pProp;
                break;

            //UUUU Sort out empty entries
            case CTF_PM_FILLGRADIENTNAME:
            case CTF_PM_FILLHATCHNAME:
            case CTF_PM_FILLBITMAPNAME:
            case CTF_PM_FILLTRANSNAME:

            case CTF_PM_HEADERFILLGRADIENTNAME:
            case CTF_PM_HEADERFILLHATCHNAME:
            case CTF_PM_HEADERFILLBITMAPNAME:
            case CTF_PM_HEADERFILLTRANSNAME:

            case CTF_PM_FOOTERFILLGRADIENTNAME:
            case CTF_PM_FOOTERFILLHATCHNAME:
            case CTF_PM_FOOTERFILLBITMAPNAME:
            case CTF_PM_FOOTERFILLTRANSNAME:
            {
                rtl::OUString aStr;

                if( (pProp->maValue >>= aStr) && 0 == aStr.getLength() )
                {
                    pProp->mnIndex = -1;
                }

                break;
            }
        }

        if (nPrintId == CTF_PM_PRINTMASK)
        {
            pPrint = pProp;
            lcl_RemoveState(pPrint);
        }
    }

    //UUUU These entries need to be reduced to a single one for XML export. 
    // Both would be exported as 'draw:tile-repeat-offset' following a percent
    // value and a 'vertical' or 'horizontal' tag as mark. If both would be active
    // and both would be exported this would create an XML error (same property twice)
    if(pRepeatOffsetX && pRepeatOffsetY)
    {
        sal_Int32 nOffset(0);

        if((pRepeatOffsetX->maValue >>= nOffset) && (!nOffset))
        {
            pRepeatOffsetX->mnIndex = -1;
        }
        else
        {
            pRepeatOffsetY->mnIndex = -1;
        }
    }

    //UUUU Same as above for Header
    if(pHeaderRepeatOffsetX && pHeaderRepeatOffsetY)
    {
        sal_Int32 nOffset(0);

        if((pHeaderRepeatOffsetX->maValue >>= nOffset) && (!nOffset))
        {
            pHeaderRepeatOffsetX->mnIndex = -1;
        }
        else
        {
            pHeaderRepeatOffsetY->mnIndex = -1;
        }
    }

    //UUUU Same as above for Footer
    if(pFooterRepeatOffsetX && pFooterRepeatOffsetY)
    {
        sal_Int32 nOffset(0);

        if((pFooterRepeatOffsetX->maValue >>= nOffset) && (!nOffset))
        {
            pFooterRepeatOffsetX->mnIndex = -1;
        }
        else
        {
            pFooterRepeatOffsetY->mnIndex = -1;
        }
    }

	if( pPMStandardMode && !getBOOL(pPMStandardMode->maValue) )
	{
		lcl_RemoveState(pPMStandardMode);
		if( pPMGridBaseWidth )
			lcl_RemoveState(pPMGridBaseWidth);
		if( pPMGridSnapToChars )
			lcl_RemoveState(pPMGridSnapToChars);
	}

	if( pPMGridBaseWidth && pPMStandardMode )
		lcl_RemoveState(pPMStandardMode);	
	
	aPageBuffer.ContextFilter( rPropState );
	aHeaderBuffer.ContextFilter( rPropState );
	aFooterBuffer.ContextFilter( rPropState );

	if( pPMHeaderHeight && (!pPMHeaderDynamic || (pPMHeaderDynamic && getBOOL( pPMHeaderDynamic->maValue ))) )
		lcl_RemoveState( pPMHeaderHeight );
	if( pPMHeaderMinHeight && pPMHeaderDynamic && !getBOOL( pPMHeaderDynamic->maValue ) )
		lcl_RemoveState( pPMHeaderMinHeight );
	if( pPMHeaderDynamic )
		lcl_RemoveState( pPMHeaderDynamic );

	if( pPMFooterHeight && (!pPMFooterDynamic || (pPMFooterDynamic && getBOOL( pPMFooterDynamic->maValue ))) )
		lcl_RemoveState( pPMFooterHeight );
	if( pPMFooterMinHeight && pPMFooterDynamic && !getBOOL( pPMFooterDynamic->maValue ) )
		lcl_RemoveState( pPMFooterMinHeight );
	if( pPMFooterDynamic )
		lcl_RemoveState( pPMFooterDynamic );

	if( pPMScaleTo )
		lcl_RemoveStateIfZero16( pPMScaleTo );
	if( pPMScaleToPages )
		lcl_RemoveStateIfZero16( pPMScaleToPages );
	if( pPMScaleToX )
		lcl_RemoveStateIfZero16( pPMScaleToX );
	if( pPMScaleToY )
		lcl_RemoveStateIfZero16( pPMScaleToY );

    if (pPrint)
    {
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_ANNOTATIONS), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintAnnotations")), rPropSet);
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_CHARTS), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintCharts")), rPropSet);
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_DRAWING), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintDrawing")), rPropSet);
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_FORMULAS), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintFormulas")), rPropSet);
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_GRID), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintGrid")), rPropSet);
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_HEADERS), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintHeaders")), rPropSet);
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_OBJECTS), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintObjects")), rPropSet);
        lcl_AddState(rPropState, aPropMapper->FindEntryIndex(CTF_PM_PRINT_ZEROVALUES), rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("PrintZeroValues")), rPropSet);
    }

	SvXMLExportPropertyMapper::ContextFilter(rPropState,rPropSet);
}

