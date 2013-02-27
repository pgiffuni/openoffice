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



#ifndef _SV_BITMAP_HXX
#define _SV_BITMAP_HXX

#include <vcl/sv.h>
#include <vcl/dllapi.h>
#include <vcl/mapmod.hxx>
#include <tools/rc.hxx>
#include <vcl/region.hxx>

#ifdef WNT
#define _STLP_HAS_NATIVE_FLOAT_ABS
#endif

#include <boost/math/special_functions/sinc.hpp>

using namespace boost::math::policies;
typedef policy<
	promote_double<false>
> SincPolicy;

// -----------
// - Defines -
// -----------

#define BMP_MIRROR_NONE				0x00000000UL
#define BMP_MIRROR_HORZ				0x00000001UL
#define BMP_MIRROR_VERT				0x00000002UL

// -----------------------------------------------------------------------------

#define BMP_SCALE_NONE				    0x00000000UL
#define BMP_SCALE_FAST				    0x00000001UL
#define BMP_SCALE_INTERPOLATE		    0x00000002UL
#define BMP_SCALE_SUPER                 0x00000003UL
#define BMP_SCALE_LANCZOS			    0x00000004UL
#define BMP_SCALE_BICUBIC               0x00000005UL
#define BMP_SCALE_BILINEAR              0x00000006UL
#define BMP_SCALE_BOX                   0x00000007UL

// new default assigns for having slots for best quality and
// an alternative with a good compromize between speed and quality.
// Currently BMP_SCALE_BESTQUALITY maps to BMP_SCALE_LANCZOS and
// BMP_SCALE_FASTESTINTERPOLATE to BMP_SCALE_SUPER
#define BMP_SCALE_BESTQUALITY           0x000000feUL
#define BMP_SCALE_FASTESTINTERPOLATE    0x000000ffUL

// -----------------------------------------------------------------------------

#define BMP_DITHER_NONE				0x00000000UL
#define BMP_DITHER_MATRIX			0x00000001UL
#define BMP_DITHER_FLOYD			0x00000002UL
#define BMP_DITHER_FLOYD_16			0x00000004UL

// -----------------------------------------------------------------------------

#define BMP_VECTORIZE_NONE			BMP_VECTORIZE_OUTER
#define BMP_VECTORIZE_INNER			0x00000001UL
#define BMP_VECTORIZE_OUTER			0x00000002UL
#define BMP_VECTORIZE_BOUND_ONLY	0x00000004UL
#define BMP_VECTORIZE_REDUCE_EDGES	0x00000008UL

// -----------------------------------------------------------------------------

#define BMP_COL_TRANS					Color( 252, 3, 251 )
#define BMP_COLOR_MONOCHROME_THRESHOLD	128

// ---------
// - Enums -
// ---------

enum BmpConversion
{
    BMP_CONVERSION_NONE = 0,
    BMP_CONVERSION_1BIT_THRESHOLD = 1,
    BMP_CONVERSION_1BIT_MATRIX = 2,
    BMP_CONVERSION_4BIT_GREYS = 3,
    BMP_CONVERSION_4BIT_COLORS = 4,
    BMP_CONVERSION_8BIT_GREYS = 5,
    BMP_CONVERSION_8BIT_COLORS = 6,
    BMP_CONVERSION_24BIT = 7,
    BMP_CONVERSION_4BIT_TRANS = 8,
    BMP_CONVERSION_8BIT_TRANS = 9,
    BMP_CONVERSION_GHOSTED = 10
};

// ------------------------------------------------------------------------

enum BmpCombine
{
    BMP_COMBINE_COPY = 0,
    BMP_COMBINE_INVERT = 1,
    BMP_COMBINE_AND = 2,
    BMP_COMBINE_NAND = 3,
    BMP_COMBINE_OR = 4,
    BMP_COMBINE_NOR = 5,
    BMP_COMBINE_XOR = 6,
    BMP_COMBINE_NXOR = 7
};

// ------------------------------------------------------------------------

enum BmpReduce
{
	BMP_REDUCE_SIMPLE = 0,
	BMP_REDUCE_POPULAR = 1,
	BMP_REDUCE_MEDIAN = 2
};

// ------------------------------------------------------------------------

enum BmpEmboss
{
	BMP_EMBOSS_TOPLEFT = 0,
	BMP_EMBOSS_TOP = 1,
	BMP_EMBOSS_TOPRIGHT = 2,
	BMP_EMBOSS_LEFT = 3,
	BMP_EMBOSS_MIDDLE = 4,
	BMP_EMBOSS_RIGHT = 5,
	BMP_EMBOSS_BOTTOMLEFT = 6,
	BMP_EMBOSS_BOTTOM = 7,
	BMP_EMBOSS_BOTTOMRIGHT = 8
};

// ------------------------------------------------------------------------

enum BmpFilter
{
    BMP_FILTER_SMOOTH = 0,
    BMP_FILTER_SHARPEN = 1,
    BMP_FILTER_REMOVENOISE = 2,
	BMP_FILTER_SOBEL_GREY = 3,
	BMP_FILTER_EMBOSS_GREY = 4,
	BMP_FILTER_SOLARIZE = 5,
	BMP_FILTER_SEPIA = 6,
	BMP_FILTER_MOSAIC = 7,
	BMP_FILTER_POPART = 8,

	BMP_FILTER_UNKNOWN = 65535
};

// ------------------------------------------------------------------------

enum BmpColorMode
{
    BMP_COLOR_NORMAL = 0,
    BMP_COLOR_HIGHCONTRAST = 1,
	BMP_COLOR_MONOCHROME_BLACK = 2,
	BMP_COLOR_MONOCHROME_WHITE = 3
};

// --------------------
// - FilterParameters -
// --------------------

class VCL_DLLPUBLIC BmpFilterParam
{
	friend class Bitmap;
	friend class BitmapEx;
	friend class Animation;

private:
	BmpFilter	    meFilter;
	sal_uLong		mnProgressStart;
	sal_uLong		mnProgressEnd;

public:
	struct MosaicTileSize
	{
		sal_uLong mnTileWidth;
		sal_uLong mnTileHeight;
	};

	struct EmbossAngles
	{
		sal_uInt16 mnAzimuthAngle100;
		sal_uInt16 mnElevationAngle100;
	};

private:
	union
	{
		sal_uInt16	mnSepiaPercent;
		sal_uInt8	mcSolarGreyThreshold;

		MosaicTileSize maMosaicTileSize;
		EmbossAngles maEmbossAngles;
	};

public:

	BmpFilterParam( sal_uLong nProgressStart = 0, sal_uLong nProgressEnd = 0 ) :
		meFilter( BMP_FILTER_UNKNOWN ), mnProgressStart( nProgressStart ), mnProgressEnd( nProgressEnd ) {}

	BmpFilterParam( sal_uInt8 cSolarGreyThreshold, sal_uLong nProgressStart = 0, sal_uLong nProgressEnd = 0 ) :
		meFilter( BMP_FILTER_SOLARIZE ), mnProgressStart( nProgressStart ), mnProgressEnd( nProgressEnd ),
		mcSolarGreyThreshold( cSolarGreyThreshold ) {}

	BmpFilterParam( sal_uInt16 nSepiaPercent, sal_uLong nProgressStart = 0, sal_uLong nProgressEnd = 0 ) :
		meFilter( BMP_FILTER_SEPIA ), mnProgressStart( nProgressStart ), mnProgressEnd( nProgressEnd ),
		mnSepiaPercent( nSepiaPercent ) {}

	BmpFilterParam( const Size& rMosaicTileSize, sal_uLong nProgressStart = 0, sal_uLong nProgressEnd = 0 ) :
		meFilter( BMP_FILTER_MOSAIC ), mnProgressStart( nProgressStart ), mnProgressEnd( nProgressEnd )
		{
			maMosaicTileSize.mnTileWidth = rMosaicTileSize.Width();
			maMosaicTileSize.mnTileHeight= rMosaicTileSize.Height();
		}
	BmpFilterParam( sal_uInt16 nEmbossAzimuthAngle100, sal_uInt16 nEmbossElevationAngle100,
					sal_uLong nProgressStart = 0, sal_uLong nProgressEnd = 0 ) :
		meFilter( BMP_FILTER_EMBOSS_GREY ), mnProgressStart( nProgressStart ), mnProgressEnd( nProgressEnd )
		{
			maEmbossAngles.mnAzimuthAngle100 = nEmbossAzimuthAngle100;
			maEmbossAngles.mnElevationAngle100 = nEmbossElevationAngle100;
		}
};

// --------------------
// Resample Kernels
// --------------------

class Kernel
{
public:
    Kernel() {}
    virtual ~Kernel() {}

    virtual double GetWidth() const = 0;
    virtual double Calculate(double x) const = 0;
};

class Lanczos3Kernel : public Kernel
{
public:
    Lanczos3Kernel( void) {}

    virtual double GetWidth() const
    { 
        return 3.0; 
    }
    
    virtual double Calculate(double x) const
    {
        return (-3.0 <= x && 3.0 > x) ? SincFilter(x) * SincFilter( x / 3.0 ) : 0.0;
    }

    inline double SincFilter(double x) const
    {
        if(0.0 == x)
        {
            return 1.0;
        }

        x *= M_PI;
        return boost::math::sinc_pi(x, SincPolicy());
    }
};

class BicubicKernel : public Kernel 
{
public:
    BicubicKernel( void) {}

    virtual double GetWidth() const
    { 
        return 2.0; 
    }
    
    virtual double Calculate(double x) const
    {
        if(0.0 > x)
        {
            x = -x;
        }

        if(1.0 >= x)
        {
            return (1.5 * x - 2.5) * x * x + 1.0;
        }
        else if(2.0 > x)
        {
            return ((-0.5 * x + 2.5) * x - 4.0) * x + 2.0;
        }

        return 0.0;
    }
};

class BilinearKernel : public Kernel 
{
public:
    BilinearKernel( void) {}

    virtual double GetWidth() const
    { 
        return 1.0; 
    }

    virtual double Calculate(double x) const
    {
        if(0.0 > x)
        {
            x = -x;
        }

        if(1.0 > x)
        {
            return 1.0 - x;
        }
        
        return 0.0;
    }
};

class BoxKernel : public Kernel 
{
public:
    BoxKernel( void) {}

    virtual double GetWidth() const
    { 
        return 0.5; 
    }

    virtual double Calculate(double x) const
    {
        if(-0.5 <= x && 0.5 > x)
        {
            return 1.0;
        }

        return 0.0;
    }
};

// ----------
// - Bitmap -
// ----------

class   BitmapReadAccess;
class   BitmapWriteAccess;
class   BitmapPalette;
class   ImpBitmap;
class   Color;
class   ResId;
class	GDIMetaFile;
class	AlphaMask;
class   OutputDevice;
class   SalBitmap;

struct BitmapSystemData
{
    #if defined( WNT ) || defined( OS2 )
    void* pDIB; // device independent byte buffer
    void* pDDB; // if not NULL then this is actually an HBITMAP
    #elif defined( QUARTZ )
    void* rImageContext;     //Image context (CGContextRef)
    #else
    void* aPixmap;
    #endif
    int mnWidth;
    int mnHeight;
};

class VCL_DLLPUBLIC Bitmap
{
private:

    ImpBitmap*              mpImpBmp;
    MapMode                 maPrefMapMode;
    Size                    maPrefSize;

//#if 0 // _SOLAR__PRIVATE

public:

    SAL_DLLPRIVATE void                 ImplReleaseRef();
    SAL_DLLPRIVATE void                 ImplMakeUnique();
                   ImpBitmap*           ImplGetImpBitmap() const;
    SAL_DLLPRIVATE void                 ImplSetImpBitmap( ImpBitmap* pImpBmp );
    SAL_DLLPRIVATE void                 ImplAssignWithSize( const Bitmap& rBitmap );

    SAL_DLLPRIVATE void                     ImplAdaptBitCount(Bitmap& rNew) const;
    SAL_DLLPRIVATE sal_Bool                 ImplScaleFast( const double& rScaleX, const double& rScaleY );
    SAL_DLLPRIVATE sal_Bool                 ImplScaleInterpolate( const double& rScaleX, const double& rScaleY );
    SAL_DLLPRIVATE sal_Bool                 ImplScaleSuper( const double& rScaleX, const double& rScaleY );
    SAL_DLLPRIVATE sal_Bool                 ImplScaleConvolution( const double& rScaleX, const double& rScaleY, const Kernel& aKernel);
    SAL_DLLPRIVATE sal_Bool                 ImplMakeMono( sal_uInt8 cThreshold );
    SAL_DLLPRIVATE sal_Bool                 ImplMakeMonoDither();
    SAL_DLLPRIVATE sal_Bool                 ImplMakeGreyscales( sal_uInt16 nGreyscales );
    SAL_DLLPRIVATE sal_Bool                 ImplConvertUp( sal_uInt16 nBitCount, Color* pExtColor = NULL );
    SAL_DLLPRIVATE sal_Bool                 ImplConvertDown( sal_uInt16 nBitCount, Color* pExtColor = NULL );
	SAL_DLLPRIVATE sal_Bool					ImplConvertGhosted();
	SAL_DLLPRIVATE sal_Bool					ImplDitherMatrix();
	SAL_DLLPRIVATE sal_Bool					ImplDitherFloyd();
	SAL_DLLPRIVATE sal_Bool					ImplDitherFloyd16();
	SAL_DLLPRIVATE sal_Bool					ImplReduceSimple( sal_uInt16 nColorCount );
	SAL_DLLPRIVATE sal_Bool					ImplReducePopular( sal_uInt16 nColorCount );
	SAL_DLLPRIVATE sal_Bool					ImplReduceMedian( sal_uInt16 nColorCount );
	SAL_DLLPRIVATE void					ImplMedianCut( sal_uLong* pColBuf, BitmapPalette& rPal, 
										   long nR1, long nR2, long nG1, long nG2, long nB1, long nB2,
										   long nColors, long nPixels, long& rIndex );
	SAL_DLLPRIVATE sal_Bool					ImplConvolute3( const long* pMatrix, long nDivisor, 
											const BmpFilterParam* pFilterParam, const Link* pProgress );
	SAL_DLLPRIVATE sal_Bool					ImplMedianFilter( const BmpFilterParam* pFilterParam, const Link* pProgress );
	SAL_DLLPRIVATE sal_Bool					ImplSobelGrey( const BmpFilterParam* pFilterParam, const Link* pProgress );
	SAL_DLLPRIVATE sal_Bool					ImplEmbossGrey( const BmpFilterParam* pFilterParam, const Link* pProgress );
	SAL_DLLPRIVATE sal_Bool					ImplSolarize( const BmpFilterParam* pFilterParam, const Link* pProgress );
	SAL_DLLPRIVATE sal_Bool					ImplSepia( const BmpFilterParam* pFilterParam, const Link* pProgress );
	SAL_DLLPRIVATE sal_Bool					ImplMosaic( const BmpFilterParam* pFilterParam, const Link* pProgress );
	SAL_DLLPRIVATE sal_Bool					ImplPopArt( const BmpFilterParam* pFilterParam, const Link* pProgress );

//#endif // PRIVATE

public:

                            Bitmap();
                            Bitmap( const Bitmap& rBitmap );
                            Bitmap( const Size& rSizePixel, sal_uInt16 nBitCount, const BitmapPalette* pPal = NULL );
                            Bitmap( const ResId& rResId );
                            Bitmap( SalBitmap* pSalBitmap );
    virtual                 ~Bitmap();

    Bitmap&                 operator=( const Bitmap& rBitmap );
    inline sal_Bool             operator!() const;
    inline sal_Bool             operator==( const Bitmap& rBitmap ) const;
    inline sal_Bool             operator!=( const Bitmap& rBitmap ) const;

	inline sal_Bool				IsSameInstance( const Bitmap& rBmp ) const;
	sal_Bool					IsEqual( const Bitmap& rBmp ) const;

	inline sal_Bool				IsEmpty() const;
	void					SetEmpty();

    inline const MapMode&   GetPrefMapMode() const;
    inline void             SetPrefMapMode( const MapMode& rMapMode );

    inline const Size&      GetPrefSize() const;
    inline void             SetPrefSize( const Size& rSize );

    Size                    GetSizePixel() const;
	void					SetSizePixel( const Size& rNewSize, sal_uInt32 nScaleFlag = BMP_SCALE_FASTESTINTERPOLATE );

    /**
     * The pixel size of a bitmap's source (e.g. an image file)
     * and the pixel size of its resulting bitmap can differ,
     * e.g. when the image reader has its preview mode enabled.
     */
    Size                    GetSourceSizePixel() const;
    void                    SetSourceSizePixel( const Size& );


    sal_uInt16                  GetBitCount() const;
    inline sal_uLong            GetColorCount() const;
    inline sal_uLong            GetSizeBytes() const;
	sal_Bool					HasGreyPalette() const;
    /** get system dependent bitmap data
    
        @param rData
        The system dependent BitmapSystemData structure to be filled
        
        @return sal_True if the bitmap has a valid system object (e.g. not empty)
    */
    bool                    GetSystemData( BitmapSystemData& rData ) const;

	sal_uLong					GetChecksum() const;

    Bitmap                  CreateDisplayBitmap( OutputDevice* pDisplay );
    Bitmap                  GetColorTransformedBitmap( BmpColorMode eColorMode ) const;

	static const BitmapPalette& GetGreyPalette( int nEntries );

public:

    sal_Bool MakeMono( sal_uInt8 cThreshold );


    /** Convert bitmap format

    	@param eConversion
        The format this bitmap should be converted to.

        @return sal_True, if the conversion was completed successfully.
     */
    sal_Bool                    Convert( BmpConversion eConversion );

    /** Reduce number of colors for the bitmap

    	@param nNewColorCount
        Maximal number of bitmap colors after the reduce operation

        @param eReduce
        Algorithm to use for color reduction

        @return sal_True, if the color reduction operation was completed successfully.
     */
	sal_Bool					ReduceColors( sal_uInt16 nNewColorCount, 
										  BmpReduce eReduce = BMP_REDUCE_SIMPLE );

    /** Apply a dither algorithm to the bitmap

    	This method dithers the bitmap inplace, i.e. a true color
    	bitmap is converted to a paletted bitmap, reducing the color
    	deviation by error diffusion.

    	@param nDitherFlags
        The algorithm to be used for dithering

        @param pDitherPal
        A custom palette to be used when dithering (not yet implemented, leave NULL)
     */
	sal_Bool					Dither( sal_uLong nDitherFlags = BMP_DITHER_MATRIX );

    /** Crop the bitmap

    	@param rRectPixel
        A rectangle specifying the crop amounts on all four sides of
        the bitmap. If the upper left corner of the bitmap is assigned
        (0,0), then this method cuts out the given rectangle from the
        bitmap. Note that the rectangle is clipped to the bitmap's
        dimension, i.e. negative left,top rectangle coordinates or
        exceeding width or height is ignored.

        @return sal_True, if cropping was performed successfully. If
        nothing had to be cropped, because e.g. the crop rectangle
        included the bitmap, sal_False is returned, too!
     */
    sal_Bool                    Crop( const Rectangle& rRectPixel );

    /** Expand the bitmap by pixel padding

    	@param nDX 
        Number of pixel to pad at the right border of the bitmap

    	@param nDY
        Number of scanlines to pad at the bottom border of the bitmap

        @param pInitColor
        Color to use for padded pixel

        @return sal_True, if padding was performed successfully. sal_False is
        not only returned when the operation failed, but also if
        nothing had to be done, e.g. because nDX and nDY were zero.
     */
    sal_Bool                    Expand( sal_uLong nDX, sal_uLong nDY, 
									const Color* pInitColor = NULL );

    /** Copy a rectangular area from another bitmap

    	@param rRectDst
        Destination rectangle in this bitmap. This is clipped to the
        bitmap dimensions.

        @param rRectSrc
        Source rectangle in pBmpSrc. This is clipped to the source
        bitmap dimensions. Note further that no scaling takes place
        during this copy operation, i.e. only the minimum of source
        and destination rectangle's width and height are used.

        @param pBmpSrc
        The source bitmap to copy from. If this argument is NULL, or
        equal to the object this method is called on, copying takes
        place within the same bitmap.

        @return sal_True, if the operation completed successfully. sal_False
        is not only returned when the operation failed, but also if
        nothing had to be done, e.g. because one of the rectangles are
        empty.
     */
    sal_Bool                    CopyPixel( const Rectangle& rRectDst,
									   const Rectangle& rRectSrc,
									   const Bitmap* pBmpSrc = NULL );

    /** Perform boolean operations with another bitmap

    	@param rMask
        The mask bitmap in the selected combine operation
        
        @param eCombine
        The combine operation to perform on the bitmap

        @return sal_True, if the operation was completed successfully.
     */
    sal_Bool                    CombineSimple( const Bitmap& rMask,
										   BmpCombine eCombine );

    /** Alpha-blend the given bitmap against a specified uniform
      	background color.

		@attention This method might convert paletted bitmaps to
		truecolor, to be able to represent every necessary color. Note
		that during alpha blending, lots of colors not originally
		included in the bitmap can be generated.

        @param rAlpha
        Alpha mask to blend with

        @param rBackgroundColor
        Background color to use for every pixel during alpha blending

        @return sal_True, if blending was successful, sal_False otherwise
     */
    sal_Bool 					Blend( const AlphaMask& rAlpha, 
                                   const Color& 	rBackgroundColor );

    /** Fill the entire bitmap with the given color

    	@param rFillColor
        Color value to use for filling

        @return sal_True, if the operation was completed successfully.
     */
    sal_Bool                    Erase( const Color& rFillColor );

    /** Perform the Invert operation on every pixel

        @return sal_True, if the operation was completed successfully.
     */
    sal_Bool                    Invert();
    
    /** Mirror the bitmap

    	@param nMirrorFlags
        About which axis (horizontal, vertical, or both) to mirror

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Mirror( sal_uLong nMirrorFlags );

    /** Scale the bitmap

    	@param rNewSize
        The resulting size of the scaled bitmap

        @param nScaleFlag
        The algorithm to be used for scaling

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Scale( const Size& rNewSize, sal_uInt32 nScaleFlag = BMP_SCALE_FASTESTINTERPOLATE );

    /** Scale the bitmap

    	@param rScaleX
        The scale factor in x direction.

    	@param rScaleY
        The scale factor in y direction.

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Scale( const double& rScaleX, const double& rScaleY, sal_uInt32 nScaleFlag = BMP_SCALE_FASTESTINTERPOLATE );

    // Adapt the BitCount of rNew to BitCount of lolal, including grey or color paltette
    // Can be used to create alpha/mask bitmaps after their processing in 24bit
    void AdaptBitCount(Bitmap& rNew) const;

    /** Rotate bitmap by the specified angle

    	@param nAngle10
        The rotation angle in tenth of a degree. The bitmap is always rotated around its center.

        @param rFillColor
        The color to use for filling blank areas. During rotation, the
        bitmap is enlarged such that the whole rotation result fits
        in. The empty spaces around that rotated original bitmap are
        then filled with this color.

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Rotate( long nAngle10, const Color& rFillColor );

    /** Create on-off mask from bitmap

    	This method creates a bitmask from the bitmap, where every
    	pixel that equals rTransColor is set transparent, the rest
    	opaque.

        @param rTransColor
        Color value where the bitmask should be transparent

        @param nTol
        Tolerance value. Specifies the maximal difference between
        rTransColor and the individual pixel values, such that the
        corresponding pixel is still regarded transparent.

        @return the resulting bitmask.
     */
    Bitmap                  CreateMask( const Color& rTransColor, sal_uLong nTol = 0UL ) const;

    /** Create region of similar colors in a given rectangle

    	@param rColor
        All pixel which have this color are included in the calculated region

        @param rRect
        The rectangle within which matching pixel are looked for. This
        rectangle is always clipped to the bitmap dimensions.

        @return the generated region.
     */
	Region					CreateRegion( const Color& rColor, const Rectangle& rRect ) const;

    /** Replace all pixel where the given mask is on with the specified color

    	@param rMask
        Mask specifying which pixel should be replaced
        
        @param rReplaceColor
        Color to be placed in all changed pixel

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Replace( const Bitmap& rMask, const Color& rReplaceColor );

    /** Merge bitmap with given background color according to specified alpha mask

    	@param rAlpha
        Alpha mask specifying the amount of background color to merge in
        
        @param rMergeColor
        Background color to be used for merging

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Replace( const AlphaMask& rAlpha, const Color& rMergeColor );

    /** Replace all pixel having the search color with the specified color

    	@param rSearchColor
        Color specifying which pixel should be replaced
        
        @param rReplaceColor
        Color to be placed in all changed pixel

        @param nTol
        Tolerance value. Specifies the maximal difference between
        rSearchColor and the individual pixel values, such that the
        corresponding pixel is still regarded a match.

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Replace( const Color& rSearchColor, const Color& rReplaceColor, sal_uLong nTol = 0 );

    /** Replace all pixel having one the search colors with the corresponding replace color

    	@param pSearchColor
        Array of colors specifying which pixel should be replaced
        
        @param pReplaceColor
        Array of colors to be placed in all changed pixel

        @param nColorCount
        Size of the aforementioned color arrays

        @param nTol
        Tolerance value. Specifies the maximal difference between
        pSearchColor colors and the individual pixel values, such that
        the corresponding pixel is still regarded a match.

        @return sal_True, if the operation was completed successfully.        
     */
    sal_Bool                    Replace( const Color* pSearchColors, const Color* rReplaceColors, 
									 sal_uLong nColorCount, sal_uLong* pTols = NULL );

    /** Convert the bitmap to a PolyPolygon    	

    	This works by putting continuous areas of the same color into
    	a polygon, by tracing its bounding line.

    	@param rPolyPoly
        The resulting PolyPolygon

        @param nFlags
        Whether the inline or the outline of the color areas should be
        represented by the polygon

        @param pProgress
        A callback for showing the progress of the vectorization

        @return sal_True, if the operation was completed successfully.        
     */
	sal_Bool					Vectorize( PolyPolygon& rPolyPoly, 
									   sal_uLong nFlags = BMP_VECTORIZE_OUTER,
									   const Link* pProgress = NULL );

    /** Convert the bitmap to a meta file

    	This works by putting continuous areas of the same color into
    	polygons painted in this color, by tracing the area's bounding
    	line.

    	@param rMtf
        The resulting meta file

        @param cReduce
        If non-null, minimal size of bound rects for individual polygons. Smaller ones are ignored.

        @param nFlags
        Whether the inline or the outline of the color areas should be
        represented by the polygon

        @param pProgress
        A callback for showing the progress of the vectorization

        @return sal_True, if the operation was completed successfully.        
     */
	sal_Bool					Vectorize( GDIMetaFile& rMtf, sal_uInt8 cReduce = 0,
									   sal_uLong nFlags = BMP_VECTORIZE_INNER,
									   const Link* pProgress = NULL );

    /** Change various global color characteristics

    	@param nLuminancePercent
        Percent of luminance change, valid range [-100,100]. Values outside this range are clipped to the valid range.

    	@param nContrastPercent
        Percent of contrast change, valid range [-100,100]. Values outside this range are clipped to the valid range.

    	@param nChannelRPercent
        Percent of red channel change, valid range [-100,100]. Values outside this range are clipped to the valid range.

    	@param nChannelGPercent
        Percent of green channel change, valid range [-100,100]. Values outside this range are clipped to the valid range.

    	@param nChannelBPercent
        Percent of blue channel change, valid range [-100,100]. Values outside this range are clipped to the valid range.

        @param fGamma
        Exponent of the gamma function applied to the bitmap. The
        value 1.0 results in no change, the valid range is
        (0.0,10.0]. Values outside this range are regarded as 1.0.

        @param bInvert
        If sal_True, invert the channel values with the logical 'not' operator

        @return sal_True, if the operation was completed successfully.        
     */
	sal_Bool					Adjust( short nLuminancePercent = 0,
									short nContrastPercent = 0,
									short nChannelRPercent = 0, 
									short nChannelGPercent = 0, 
									short nChannelBPercent = 0,
									double fGamma = 1.0,
									sal_Bool bInvert = sal_False );

    /** Apply specified filter to the bitmap

    	@param eFilter
        The filter algorithm to apply

        @param pFilterParam
        Various parameter for the different bitmap filter algorithms

        @param pProgress
        A callback for showing the progress of the vectorization
        
        @return sal_True, if the operation was completed successfully.        
     */
	sal_Bool					Filter( BmpFilter eFilter, 
									const BmpFilterParam* pFilterParam = NULL,
									const Link* pProgress = NULL );

    BitmapReadAccess*       AcquireReadAccess();
    BitmapWriteAccess*      AcquireWriteAccess();
    void                    ReleaseAccess( BitmapReadAccess* pAccess );
};

// -----------
// - Inlines -
// -----------

inline sal_Bool Bitmap::operator!() const
{
    return( mpImpBmp == NULL );
}

// ------------------------------------------------------------------

inline sal_Bool Bitmap::operator==( const Bitmap& rBitmap ) const
{
    return( rBitmap.mpImpBmp == mpImpBmp );
}

// ------------------------------------------------------------------

inline sal_Bool Bitmap::operator!=( const Bitmap& rBitmap ) const
{
    return( rBitmap.mpImpBmp != mpImpBmp );
}

// ------------------------------------------------------------------

inline sal_Bool Bitmap::IsSameInstance( const Bitmap& rBitmap ) const
{
    return( rBitmap.mpImpBmp == mpImpBmp );
}

// ------------------------------------------------------------------

inline sal_Bool	Bitmap::IsEmpty() const
{
    return( mpImpBmp == NULL );
}

// ------------------------------------------------------------------

inline const MapMode& Bitmap::GetPrefMapMode() const
{
    return maPrefMapMode;
}

// ------------------------------------------------------------------

inline void Bitmap::SetPrefMapMode( const MapMode& rMapMode )
{
    maPrefMapMode = rMapMode;
}

// ------------------------------------------------------------------

inline const Size& Bitmap::GetPrefSize() const
{
    return maPrefSize;
}

// ------------------------------------------------------------------

inline void Bitmap::SetPrefSize( const Size& rSize )
{
    maPrefSize = rSize;
}

// ------------------------------------------------------------------

inline sal_uLong Bitmap::GetColorCount() const
{
    return( 1UL << (sal_uLong) GetBitCount() );
}

// ------------------------------------------------------------------

inline sal_uLong Bitmap::GetSizeBytes() const
{
    const Size aSizePix( GetSizePixel() );
    return( ( (sal_uLong) aSizePix.Width() * aSizePix.Height() * GetBitCount() ) >> 3UL );
}

#endif // _SV_BITMAP_HXX
