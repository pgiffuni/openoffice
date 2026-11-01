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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "setjmp.h"
#include "jpeglib.h"
#include "jerror.h"
#include "jpeg.h"
#include "rtl/alloc.h"
#include "osl/diagnose.h"

struct my_error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

void jpeg_svstream_src (j_decompress_ptr cinfo, void* infile);
void jpeg_svstream_dest (j_compress_ptr cinfo, void* outfile);

METHODDEF( void )
my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

METHODDEF( void )
my_output_message (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
	OSL_TRACE( "JPEG: %s", buffer );
}

/* TODO: when incompatible changes are possible again
   the preview size hint should be redone */
static int nPreviewWidth = 0;
static int nPreviewHeight = 0;
void SetJpegPreviewSizeHint( int nWidth, int nHeight )
{
    nPreviewWidth = nWidth;
    nPreviewHeight = nHeight;
}

static unsigned int ReadExifU16( const JOCTET * pData, int bLittleEndian )
{
	if ( bLittleEndian )
		return (unsigned int) pData[0] | ( (unsigned int) pData[1] << 8 );
	return ( (unsigned int) pData[0] << 8 ) | (unsigned int) pData[1];
}

static unsigned long ReadExifU32( const JOCTET * pData, int bLittleEndian )
{
	if ( bLittleEndian )
		return (unsigned long) pData[0] |
			   ( (unsigned long) pData[1] << 8 ) |
			   ( (unsigned long) pData[2] << 16 ) |
			   ( (unsigned long) pData[3] << 24 );
	return ( (unsigned long) pData[0] << 24 ) |
		   ( (unsigned long) pData[1] << 16 ) |
		   ( (unsigned long) pData[2] << 8 ) |
		   (unsigned long) pData[3];
}

sal_Bool JPEGIsNativePassthroughSafe( const sal_uInt8* pData, sal_uLong nSize )
{
	const JOCTET* pCurrent;
	const JOCTET* pEnd;

	if ( pData == NULL || nSize < 2 || pData[0] != 0xff || pData[1] != 0xd8 )
		return sal_False;

	pCurrent = pData + 2;
	pEnd = pData + nSize;
	while ( pCurrent < pEnd )
	{
		unsigned int nMarker;
		size_t nLength;

		if ( *pCurrent != 0xff )
			return sal_False;
		while ( pCurrent < pEnd && *pCurrent == 0xff )
			++pCurrent;
		if ( pCurrent >= pEnd )
			break;
		nMarker = *pCurrent++;
		if ( nMarker == 0xda || nMarker == 0xd9 )
			break;
		if ( nMarker == 0x01 || ( nMarker >= 0xd0 && nMarker <= 0xd7 ) )
			continue;
		if ( pEnd - pCurrent < 2 )
			return sal_False;
		nLength = ( (size_t) pCurrent[0] << 8 ) | pCurrent[1];
		if ( nLength < 2 || nLength - 2 > (size_t) ( pEnd - pCurrent - 2 ) )
			return sal_False;
		if ( nMarker == JPEG_APP0 + 1 && nLength >= 16 &&
			memcmp( pCurrent + 2, "Exif\0\0", 6 ) == 0 )
		{
			const JOCTET* pTiff = pCurrent + 8;
			size_t nTiffLength = nLength - 8;
			int bLittleEndian;
			unsigned long nIfdOffset;
			unsigned int nEntryCount;
			unsigned int nEntry;

			if ( nTiffLength < 8 )
				return sal_False;
			if ( pTiff[0] == 'I' && pTiff[1] == 'I' )
				bLittleEndian = 1;
			else if ( pTiff[0] == 'M' && pTiff[1] == 'M' )
				bLittleEndian = 0;
			else
				return sal_False;
			if ( ReadExifU16( pTiff + 2, bLittleEndian ) != 42 )
				return sal_False;
			nIfdOffset = ReadExifU32( pTiff + 4, bLittleEndian );
			if ( nIfdOffset > nTiffLength || nTiffLength - nIfdOffset < 2 )
				return sal_False;
			nEntryCount = ReadExifU16( pTiff + nIfdOffset, bLittleEndian );
			if ( nEntryCount > ( nTiffLength - nIfdOffset - 2 ) / 12 )
				return sal_False;
			for ( nEntry = 0; nEntry < nEntryCount; ++nEntry )
			{
				const JOCTET* pEntry = pTiff + nIfdOffset + 2 + nEntry * 12;
				if ( ReadExifU16( pEntry, bLittleEndian ) == 0x0112 )
				{
					if ( ReadExifU16( pEntry + 2, bLittleEndian ) != 3 ||
						ReadExifU32( pEntry + 4, bLittleEndian ) != 1 )
						return sal_False;
					return ReadExifU16( pEntry + 8, bLittleEndian ) == 1;
				}
			}
		}
		pCurrent += nLength;
	}

	return sal_True;
}

static unsigned long GetExifOrientation( j_decompress_ptr cinfo )
{
	jpeg_saved_marker_ptr pMarker;

	for ( pMarker = cinfo->marker_list; pMarker != NULL; pMarker = pMarker->next )
	{
		const JOCTET * pTiff;
		size_t nTiffLength;
		unsigned long nIfdOffset;
		unsigned int nEntryCount;
		unsigned int nEntry;
		int bLittleEndian;

		if ( pMarker->marker != JPEG_APP0 + 1 || pMarker->data_length < 14 ||
			memcmp( pMarker->data, "Exif\0\0", 6 ) != 0 )
			continue;

		pTiff = pMarker->data + 6;
		nTiffLength = pMarker->data_length - 6;
		if ( pTiff[0] == 'I' && pTiff[1] == 'I' )
			bLittleEndian = 1;
		else if ( pTiff[0] == 'M' && pTiff[1] == 'M' )
			bLittleEndian = 0;
		else
			continue;

		if ( ReadExifU16( pTiff + 2, bLittleEndian ) != 42 )
			continue;

		nIfdOffset = ReadExifU32( pTiff + 4, bLittleEndian );
		if ( nIfdOffset > nTiffLength || nTiffLength - nIfdOffset < 2 )
			continue;

		nEntryCount = ReadExifU16( pTiff + nIfdOffset, bLittleEndian );
		if ( nEntryCount > ( nTiffLength - nIfdOffset - 2 ) / 12 )
			continue;

		for ( nEntry = 0; nEntry < nEntryCount; ++nEntry )
		{
			const JOCTET * pEntry = pTiff + nIfdOffset + 2 + nEntry * 12;
			unsigned int nTag = ReadExifU16( pEntry, bLittleEndian );
			unsigned int nType = ReadExifU16( pEntry + 2, bLittleEndian );
			unsigned long nCount = ReadExifU32( pEntry + 4, bLittleEndian );
			unsigned int nOrientation;

			if ( nTag != 0x0112 || nType != 3 || nCount != 1 )
				continue;

			nOrientation = ReadExifU16( pEntry + 8, bLittleEndian );
			if ( nOrientation >= 1 && nOrientation <= 8 )
				return nOrientation;
		}
	}

	return 1;
}

void ReadJPEG( void* pJPEGReader, void* pIStm, long* pLines )
{
	struct jpeg_decompress_struct *pcinfo;
	struct my_error_mgr *pjerr;
	struct JPEGCreateBitmapParam	aCreateBitmapParam;
	HPBYTE							pDIB;
	HPBYTE							pTmp;
	long							nWidth;
	long							nHeight;
	long							nAlignedWidth;
	JSAMPLE *						range_limit;
	HPBYTE volatile pScanLineBuffer = NULL;
	size_t							nScanLineBufferComponents = 0;
	volatile long bDecompCreated = 0;

	*pLines = 0;
	memset( &aCreateBitmapParam, 0, sizeof( aCreateBitmapParam ) );
	pcinfo = (struct jpeg_decompress_struct *) malloc( sizeof( *pcinfo ) );
	pjerr = (struct my_error_mgr *) malloc( sizeof( *pjerr ) );
	if ( pcinfo == NULL || pjerr == NULL )
	{
		free( pcinfo );
		free( pjerr );
		return;
	}

	/* Falls der Stream nicht ausreicht (IO_PENDING)
	 wird ueber ein longjmp in der Schleife nach Exit
	 gesprungen, wir geben dann die Anzahl
	 der bisher bearbeiteten Scanlines zurueck*/
	if ( setjmp( pjerr->setjmp_buffer ) )
		goto Exit;

	pcinfo->err = jpeg_std_error( &pjerr->pub );
	pjerr->pub.error_exit = my_error_exit;
	pjerr->pub.output_message = my_output_message;

	jpeg_create_decompress( pcinfo );
	bDecompCreated = 1;
	jpeg_svstream_src( pcinfo, pIStm );
	jpeg_save_markers( pcinfo, JPEG_APP0 + 1, 0xFFFF );
	jpeg_read_header( pcinfo, sal_True );
	aCreateBitmapParam.nExifOrientation = GetExifOrientation( pcinfo );

	pcinfo->scale_num = 1;
	pcinfo->scale_denom = 1;
	pcinfo->output_gamma = 1.0;
	pcinfo->raw_data_out = sal_False;
	pcinfo->quantize_colors = sal_False;
	if ( pcinfo->jpeg_color_space == JCS_YCbCr )
		pcinfo->out_color_space = JCS_RGB;
	else if ( pcinfo->jpeg_color_space == JCS_YCCK )
		pcinfo->out_color_space = JCS_CMYK;

	OSL_ASSERT( pcinfo->out_color_space == JCS_CMYK ||
				pcinfo->out_color_space == JCS_GRAYSCALE ||
				pcinfo->out_color_space == JCS_RGB );

	/* Change scale for preview import. */
	if ( nPreviewWidth || nPreviewHeight )
	{
		if ( nPreviewWidth == 0 )
		{
			nPreviewWidth = ( pcinfo->image_width * nPreviewHeight ) /
				pcinfo->image_height;
			if ( nPreviewWidth <= 0 )
				nPreviewWidth = 1;
		}
		else if ( nPreviewHeight == 0 )
		{
			nPreviewHeight = ( pcinfo->image_height * nPreviewWidth ) /
				pcinfo->image_width;
			if ( nPreviewHeight <= 0 )
				nPreviewHeight = 1;
		}

		for ( pcinfo->scale_denom = 1; pcinfo->scale_denom < 8;
			  pcinfo->scale_denom *= 2 )
		{
			if ( pcinfo->image_width < nPreviewWidth * pcinfo->scale_denom )
				break;
			if ( pcinfo->image_height < nPreviewHeight * pcinfo->scale_denom )
				break;
		}

		if ( pcinfo->scale_denom > 1 )
		{
			pcinfo->dct_method = JDCT_FASTEST;
			pcinfo->do_fancy_upsampling = sal_False;
			pcinfo->do_block_smoothing = sal_False;
		}
	}

	jpeg_start_decompress( pcinfo );

	nWidth = pcinfo->output_width;
	nHeight = pcinfo->output_height;
	aCreateBitmapParam.nWidth = nWidth;
	aCreateBitmapParam.nHeight = nHeight;

	aCreateBitmapParam.density_unit = pcinfo->density_unit;
	aCreateBitmapParam.X_density = pcinfo->X_density;
	aCreateBitmapParam.Y_density = pcinfo->Y_density;
	aCreateBitmapParam.bGray = pcinfo->output_components == 1;
	pDIB = CreateBitmap( pJPEGReader, &aCreateBitmapParam );
	nAlignedWidth = aCreateBitmapParam.nAlignedWidth;
	range_limit = pcinfo->sample_range_limit;

	if ( pcinfo->out_color_space == JCS_CMYK )
	{
		if ( (size_t) pcinfo->output_width > (size_t) -1 / 4 )
			ERREXIT1( pcinfo, JERR_OUT_OF_MEMORY, 0 );
		nScanLineBufferComponents = (size_t) pcinfo->output_width * 4;
		pScanLineBuffer = rtl_allocateMemory( nScanLineBufferComponents );
		if ( pScanLineBuffer == NULL )
			ERREXIT1( pcinfo, JERR_OUT_OF_MEMORY, 0 );
	}

	if ( pDIB )
	{
		if ( aCreateBitmapParam.bTopDown )
			pTmp = pDIB;
		else
		{
			pTmp = pDIB + ( nHeight - 1 ) * nAlignedWidth;
			nAlignedWidth = -nAlignedWidth;
		}

		for ( *pLines = 0; *pLines < nHeight; (*pLines)++ )
		{
			if ( pScanLineBuffer != NULL )
			{
				size_t i;
				size_t j;
				jpeg_read_scanlines( pcinfo, (JSAMPARRAY) &pScanLineBuffer, 1 );
				/* Convert CMYK to RGB. */
				for ( i = 0, j = 0; i < nScanLineBufferComponents; i += 4, j += 3 )
				{
					int nC = 255 - pScanLineBuffer[i];
					int nM = 255 - pScanLineBuffer[i + 1];
					int nY = 255 - pScanLineBuffer[i + 2];
					int nK = 255 - pScanLineBuffer[i + 3];

					pTmp[j] = range_limit[ 255L - ( nC + nK ) ];
					pTmp[j + 1] = range_limit[ 255L - ( nM + nK ) ];
					pTmp[j + 2] = range_limit[ 255L - ( nY + nK ) ];
				}
			}
			else
				jpeg_read_scanlines( pcinfo, (JSAMPARRAY) &pTmp, 1 );

			/* PENDING ??? */
			if ( pcinfo->err->msg_code == JWRN_JPEG_EOF )
				break;

			pTmp += nAlignedWidth;
		}
	}

	if ( pDIB )
	{
		jpeg_finish_decompress( pcinfo );
	}
	else
	{
		jpeg_abort_decompress( pcinfo );
	}

Exit:

	if ( pScanLineBuffer != NULL )
		rtl_freeMemory( pScanLineBuffer );
	if ( bDecompCreated )
	{
		bDecompCreated = 0;
		jpeg_destroy_decompress( pcinfo );
	}
	free( pjerr );
	free( pcinfo );
}

long WriteJPEG( void* pJPEGWriter, void* pOStm,
				long nWidth, long nHeight, long bGreys,
				long nQualityPercent, void* pCallbackData )
{
	struct jpeg_compress_struct *pcinfo;
	struct my_error_mgr *pjerr;
	void*						pScanline;
	long						nY;
	volatile long				bCompCreated = 0;
	volatile long				bRet = 0;

	if ( nWidth <= 0 || nHeight <= 0 ||
		 (unsigned long) nWidth > 65535UL || (unsigned long) nHeight > 65535UL )
		return 0;

	pcinfo = (struct jpeg_compress_struct *) malloc( sizeof( *pcinfo ) );
	pjerr = (struct my_error_mgr *) malloc( sizeof( *pjerr ) );
	if ( pcinfo == NULL || pjerr == NULL )
	{
		free( pcinfo );
		free( pjerr );
		return 0;
	}

	if ( setjmp( pjerr->setjmp_buffer ) )
		goto Exit;

	pcinfo->err = jpeg_std_error( &pjerr->pub );
	pjerr->pub.error_exit = my_error_exit;
	pjerr->pub.output_message = my_output_message;

	jpeg_create_compress( pcinfo );
	bCompCreated = 1;

	jpeg_svstream_dest( pcinfo, pOStm );

	pcinfo->image_width = (JDIMENSION) nWidth;
	pcinfo->image_height = (JDIMENSION) nHeight;
	if ( bGreys )
	{
		pcinfo->input_components = 1;
		pcinfo->in_color_space = JCS_GRAYSCALE;
	}
	else
	{
		pcinfo->input_components = 3;
		pcinfo->in_color_space = JCS_RGB;
	}

	jpeg_set_defaults( pcinfo );
	jpeg_set_quality( pcinfo, (int) nQualityPercent, sal_False );

	if ( ( nWidth > 128 ) || ( nHeight > 128 ) )
		jpeg_simple_progression( pcinfo );

	jpeg_start_compress( pcinfo, sal_True );

	for( nY = 0; nY < nHeight; nY++ )
	{
		pScanline = GetScanline( pJPEGWriter, nY );

		if ( !pScanline ||
			 jpeg_write_scanlines( pcinfo, (JSAMPARRAY) &pScanline, 1 ) != 1 )
			goto Exit;

		if( JPEGCallback( pCallbackData, nY * 100L / nHeight ) )
			goto Exit;
	}

	jpeg_finish_compress( pcinfo );
	bRet = 1;

Exit:

	if ( bCompCreated )
	{
		bCompCreated = 0;
		jpeg_destroy_compress( pcinfo );
	}
	free( pjerr );
	free( pcinfo );

	return bRet;
}
