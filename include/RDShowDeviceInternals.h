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
#pragma once

#include <vector>
#include "RDShowCriticalSectionEnterer.h"
#include "RDShowCOMObjectSharedPtr.h"
#include "RDShowCaptureSettings.h"
#include "RDShowMemoryBuffer.h"

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <windows.h>
#include <dshow.h>
// See http://social.msdn.microsoft.com/forums/en-US/windowssdk/thread/ed097d2c-3d68-4f48-8448-277eaaf68252/
#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include "qedit.h"

namespace RDShow
{

class DeviceInternals 
{
public:
	DeviceInternals( COMObjectSharedPtr<IMoniker> moniker );
	virtual ~DeviceInternals();

	class VideoMediaType
	{
	public:
		VideoMediaType();					
		LONG				width ;
		LONG				height;
		bool				needVerticalFlip;
		LONGLONG			frameInterval;
		GUID				subType;
		
				
		std::string			getSubTypeName() const		{ return getSubTypeName(subType); }
		static std::string	getSubTypeName( const GUID& mediaType );
		
		float				getFrameRate() const;

		std::string			toString() const;

		bool				operator==( const VideoMediaType& other ) const;
		bool				operator!=( const VideoMediaType& other ) const;
	};
	typedef std::vector<VideoMediaType> VideoMediaTypes;
	const VideoMediaTypes&	getSupportedVideoMediaTypes() const { return mSupportedVideoMediaTypes; }

	//void					update();	

	bool					startCapture( std::size_t indexMediaType );
	bool					stopCapture();
	bool					isCapturing() const				{ return mIsCapturing; }
	bool					getCapturedImage( MemoryBuffer& buffer, unsigned int& sequenceNumber, LONGLONG& timestamp ) const;


private:
	bool					initialize();

	mutable CRITICAL_SECTION					mCriticalSection;
	VideoMediaTypes								mSupportedVideoMediaTypes;

	COMObjectSharedPtr<IMoniker>				mMoniker;
	COMObjectSharedPtr<IGraphBuilder>			mGraphBuilder;
	COMObjectSharedPtr<ICaptureGraphBuilder2>	mCaptureGraphBuilder2;
	COMObjectSharedPtr<ISampleGrabber>			mSampleGrabberFilter;
	COMObjectSharedPtr<IBaseFilter>				mSourceFilter;
	
	class SampleGrabberCallback;
	SampleGrabberCallback*						mSampleGrabberCallback;
	bool										mIsCapturing;
};


}