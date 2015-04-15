/*
   The MIT License (MIT) (http://opensource.org/licenses/MIT)
   
   Copyright (c) 2015 Jacques Menuet
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#include "RDShowImageConverter.h"

#include <assert.h>

namespace RDShow
{

ImageConverter::ImageConverter( const ImageFormat& outputImageFormat )
	: mImage(NULL)	
{
	mImage = new Image( outputImageFormat );
}

ImageConverter::~ImageConverter()
{
	delete mImage;
	mImage = NULL;
}

bool ImageConverter::update( const Image& sourceImage )
{
	if ( sourceImage.getFormat()==mImage->getFormat() )
		return mImage->getBuffer().copyFrom( sourceImage.getBuffer() );
	return convertImage( sourceImage, *mImage );
}

bool ImageConverter::convertBGR24ImageToRGB24Image( const Image& sourceImage, Image& destImage )
{
	// Pre-checks
	if ( sourceImage.getFormat().getEncoding()!=ImageFormat::BGR24 )
		return false;
	if ( destImage.getFormat().getEncoding()!=ImageFormat::RGB24 )
		return false;

	unsigned int width = sourceImage.getFormat().getWidth();
	unsigned int height = sourceImage.getFormat().getHeight();
	if ( destImage.getFormat().getWidth()!=width || destImage.getFormat().getHeight()!=height )
		 return false;

	const unsigned char* sourceBytes = sourceImage.getBuffer().getBytes();
	unsigned char* destBytes = destImage.getBuffer().getBytes();
	for ( unsigned int y=0; y<height; ++y )
	{
		for ( unsigned int x=0; x<width; ++x )
		{
			unsigned char blue = sourceBytes[0];
			unsigned char green = sourceBytes[1];
			unsigned char red = sourceBytes[2];
			sourceBytes += 3;	

			destBytes[0] = red;
			destBytes[1] = green;
			destBytes[2] = blue;
			destBytes += 3;
		}
	}
	return true;
}

bool ImageConverter::convertBGRX32ImageToRGB24Image( const Image& sourceImage, Image& destImage )
{
	// Pre-checks
	if ( sourceImage.getFormat().getEncoding()!=ImageFormat::BGRX32 )
		return false;
	if ( destImage.getFormat().getEncoding()!=ImageFormat::RGB24 )
		return false;

	unsigned int width = sourceImage.getFormat().getWidth();
	unsigned int height = sourceImage.getFormat().getHeight();
	if ( destImage.getFormat().getWidth()!=width || destImage.getFormat().getHeight()!=height )
		 return false;
	
	const unsigned char* sourceBytes = sourceImage.getBuffer().getBytes();
	unsigned char* destBytes = destImage.getBuffer().getBytes();
	for ( unsigned int y=0; y<height; ++y )
	{
		for ( unsigned int x=0; x<width; ++x )
		{
			unsigned char blue = sourceBytes[0];
			unsigned char green = sourceBytes[1];
			unsigned char red = sourceBytes[2];
			//unsigned char unused = sourceBytes[3];
			sourceBytes += 4;	

			destBytes[0] = red;
			destBytes[1] = green;
			destBytes[2] = blue;
			destBytes += 3;
		}
	}
	return true;
}

bool ImageConverter::convertBGRX32ImageToBGR24Image( const Image& sourceImage, Image& destImage )
{
	// Pre-checks
	if ( sourceImage.getFormat().getEncoding()!=ImageFormat::BGRX32 )
		return false;
	if ( destImage.getFormat().getEncoding()!=ImageFormat::BGR24 )
		return false;

	unsigned int width = sourceImage.getFormat().getWidth();
	unsigned int height = sourceImage.getFormat().getHeight();
	if ( destImage.getFormat().getWidth()!=width || destImage.getFormat().getHeight()!=height )
		 return false;
	
	const unsigned char* sourceBytes = sourceImage.getBuffer().getBytes();
	unsigned char* destBytes = destImage.getBuffer().getBytes();
	for ( unsigned int y=0; y<height; ++y )
	{
		for ( unsigned int x=0; x<width; ++x )
		{
			unsigned char blue = sourceBytes[0];
			unsigned char green = sourceBytes[1];
			unsigned char red = sourceBytes[2];
			//unsigned char unused = sourceBytes[3];
			sourceBytes += 4;	

			destBytes[0] = blue;
			destBytes[1] = green;
			destBytes[2] = red;
			destBytes += 3;
		}
	}
	return true;
}


#define CLIP_INT_TO_UCHAR(value) ( (value)<0 ? 0 : ( (value)>255 ? 255 : static_cast<unsigned char>(value) ) ) 

bool ImageConverter::convertYUYVImageToRGB24Image( const Image& sourceImage, Image& destImage )
{
	// Pre-checks
	if ( sourceImage.getFormat().getEncoding()!=ImageFormat::YUYV )
		return false;
	if ( destImage.getFormat().getEncoding()!=ImageFormat::RGB24 )
		return false;

	unsigned int width = sourceImage.getFormat().getWidth();
	unsigned int height = sourceImage.getFormat().getHeight();
	if ( destImage.getFormat().getWidth()!=width || destImage.getFormat().getHeight()!=height )
		 return false;

	// General information about YUV color space can be found here:
	// http://en.wikipedia.org/wiki/YUV 
	// or here:
	// http://www.fourcc.org/yuv.php

	// The following conversion code comes from here:
	// http://stackoverflow.com/questions/4491649/how-to-convert-yuy2-to-a-bitmap-in-c
	// http://msdn.microsoft.com/en-us/library/aa904813(VS.80).aspx#yuvformats_2
	const unsigned char* sourceBytes = sourceImage.getBuffer().getBytes();
	unsigned char* destBytes = destImage.getBuffer().getBytes();
	for ( unsigned int y=0; y<height; ++y )
	{
		for ( unsigned int i=0; i<width/2; ++i )
		{
			int y0 = sourceBytes[0];
			int u0 = sourceBytes[1];
			int y1 = sourceBytes[2];
			int v0 = sourceBytes[3];
			sourceBytes += 4;	
			
			int c = y0 - 16;
			int d = u0 - 128;
			int e = v0 - 128;
			destBytes[0] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			destBytes[1] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[2] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			
			c = y1 - 16;
			destBytes[3] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			destBytes[4] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[5] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			destBytes += 6;
		}
	}
	return true;
}


bool ImageConverter::convertYUYVImageToBGR24Image( const Image& sourceImage, Image& destImage )
{
	// Pre-checks
	if ( sourceImage.getFormat().getEncoding()!=ImageFormat::YUYV )
		return false;
	if ( destImage.getFormat().getEncoding()!=ImageFormat::BGR24 )
		return false;

	unsigned int width = sourceImage.getFormat().getWidth();
	unsigned int height = sourceImage.getFormat().getHeight();
	if ( destImage.getFormat().getWidth()!=width || destImage.getFormat().getHeight()!=height )
		 return false;

	// General information about YUV color space can be found here:
	// http://en.wikipedia.org/wiki/YUV 
	// or here:
	// http://www.fourcc.org/yuv.php

	// The following conversion code comes from here:
	// http://stackoverflow.com/questions/4491649/how-to-convert-yuy2-to-a-bitmap-in-c
	// http://msdn.microsoft.com/en-us/library/aa904813(VS.80).aspx#yuvformats_2
	const unsigned char* sourceBytes = sourceImage.getBuffer().getBytes();
	unsigned char* destBytes = destImage.getBuffer().getBytes();
	for ( unsigned int y=0; y<height; ++y )
	{
		for ( unsigned int i=0; i<width/2; ++i )
		{
			int y0 = sourceBytes[0];
			int u0 = sourceBytes[1];
			int y1 = sourceBytes[2];
			int v0 = sourceBytes[3];
			sourceBytes += 4;	
			
			int c = y0 - 16;
			int d = u0 - 128;
			int e = v0 - 128;
			destBytes[0] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			destBytes[1] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[2] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			
			c = y1 - 16;
			destBytes[3] = CLIP_INT_TO_UCHAR(( 298 * c + 516 * d           + 128) >> 8);		// Blue
			destBytes[4] = CLIP_INT_TO_UCHAR(( 298 * c - 100 * d - 208 * e + 128) >> 8);		// Green
			destBytes[5] = CLIP_INT_TO_UCHAR(( 298 * c           + 409 * e + 128) >> 8);		// Red
			destBytes += 6;
		}
	}
	return true;
}

bool ImageConverter::convertImage( const Image& sourceImage, Image& destinationImage )
{
	if ( sourceImage.getFormat()==destinationImage.getFormat() )
		return false;

	ImageFormat::Encoding sourceEncoding = sourceImage.getFormat().getEncoding();
	ImageFormat::Encoding destinationEncoding = destinationImage.getFormat().getEncoding();

	if ( sourceEncoding==ImageFormat::BGR24 && destinationEncoding==ImageFormat::RGB24 )
		return convertBGR24ImageToRGB24Image( sourceImage, destinationImage );
	else if ( sourceEncoding==ImageFormat::BGRX32 && destinationEncoding==ImageFormat::RGB24 )
		return convertBGRX32ImageToRGB24Image( sourceImage, destinationImage );
	else if ( sourceEncoding==ImageFormat::BGRX32 && destinationEncoding==ImageFormat::BGR24 )
		return convertBGRX32ImageToBGR24Image( sourceImage, destinationImage );
	else if ( sourceEncoding==ImageFormat::YUYV && destinationEncoding==ImageFormat::RGB24 )
		return convertYUYVImageToRGB24Image( sourceImage, destinationImage );
	else if ( sourceEncoding==ImageFormat::YUYV && destinationEncoding==ImageFormat::BGR24 )
		return convertYUYVImageToBGR24Image( sourceImage, destinationImage );
	return false;
}

}


