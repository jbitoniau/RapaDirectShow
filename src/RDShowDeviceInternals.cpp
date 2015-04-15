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
#include "RDShowDeviceInternals.h"

#include <assert.h>
#include <string>
#include <sstream>
#include <algorithm>

// http://msdn.microsoft.com/en-us/library/windows/desktop/dd407331(v=vs.85).aspx
// http://www.codeproject.com/Articles/34663/DirectShow-Examples-for-Using-SampleGrabber-for-Gr

// http://msrds.googlecode.com/svn/trunk/wavelets/haar_src/src/dshow/
// http://msrds.googlecode.com/svn/trunk/wavelets/haar_src/src/dshow/VideoCapture.cpp


// http://stackoverflow.com/questions/5031740/finding-out-when-the-samplegrabber-is-ready-in-directshow

// Base Microsoft Playcap example customised to use a sample grabber
// http://www.planet-source-code.com/vb/scripts/ShowCode.asp?txtCodeId=13364&lngWId=3

namespace RDShow
{

/*
	DeviceInternals::VideoMediaType
*/
DeviceInternals::VideoMediaType::VideoMediaType()
	: width(0),
	  height(0),
	  needVerticalFlip(false),
	  frameInterval(0),
	  subType()
{
}

bool DeviceInternals::VideoMediaType::operator==( const DeviceInternals::VideoMediaType& other ) const
{	
	return	width==other.width &&
			height==other.height &&
			needVerticalFlip==other.needVerticalFlip && 
			frameInterval==other.frameInterval &&
			subType==other.subType;
}

bool DeviceInternals::VideoMediaType::operator!=( const DeviceInternals::VideoMediaType& other ) const
{
	return	!( (*this)==other );
}

std::string DeviceInternals::VideoMediaType::getSubTypeName( const GUID& mediaType )
{
	// Specific non-FourCC types
	if ( mediaType==MEDIASUBTYPE_RGB1 )	
		return "RGB1";
	if ( mediaType==MEDIASUBTYPE_RGB4 )	
		return "";
	if ( mediaType==MEDIASUBTYPE_RGB8 )	
		return "RGB8";
	if ( mediaType==MEDIASUBTYPE_RGB565 )	
		return "RGB565";
	if ( mediaType==MEDIASUBTYPE_RGB555 )	
		return "RGB555";
	if ( mediaType==MEDIASUBTYPE_RGB24 )	
		return "RGB24";
	if ( mediaType==MEDIASUBTYPE_RGB32 )	
		return "RGB32";
	
	if ( mediaType==MEDIASUBTYPE_ARGB1555 )	
		return "ARGB1555";
	if ( mediaType==MEDIASUBTYPE_ARGB4444 )	
		return "ARGB4444";
	if ( mediaType==MEDIASUBTYPE_ARGB32 )	
		return "ARGB32";
	if ( mediaType==MEDIASUBTYPE_A2R10G10B10 )	
		return "A2R10G10B10";
	if ( mediaType==MEDIASUBTYPE_A2B10G10R10 )	
		return "MEDIASUBTYPE_A2B10G10R10";

	// For the other GUIDs, we to extract the FourCC 
	DWORD fcc = mediaType.Data1;
	char* fccAsChars = reinterpret_cast<char*>( &fcc );
	std::string ret( fccAsChars, sizeof(DWORD) );

	return ret;
}

float DeviceInternals::VideoMediaType::getFrameRate() const
{
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd387907(v=vs.85).aspx
	// "The MinFrameInterval and MaxFrameInterval members of VIDEO_STREAM_CONFIG_CAPS are the minimum and maximum length of each video frame, 
	// which you can translate into frame rates as follows: frames per second = 10,000,000 / frame duration"
	float frameRate = 10000000.f / static_cast<float>(frameInterval); 
	return frameRate;
}

std::string DeviceInternals::VideoMediaType::toString() const
{
	std::stringstream stream;
	stream.precision(2);
	stream << std::fixed;


	stream << "width:" << width << " height:" << height << " needVerticalFlip:" << needVerticalFlip << " frameInterval:" << frameInterval<< " (fps:" << getFrameRate() << ") subType:" << getSubTypeName();
	return stream.str();
}


/*
	DeviceInternals::SampleGrabberCallback 
*/
class DeviceInternals::SampleGrabberCallback : public ISampleGrabberCB
{
public:
	SampleGrabberCallback( CRITICAL_SECTION* criticalSection )
		: mCriticalSection( criticalSection ),
		  mReferenceCounter(1),
		  mImageBuffer(NULL),
		  mImageSequenceNumber(0)
	{
	}

    STDMETHODIMP_(ULONG) AddRef() 
	{ 
		return InterlockedIncrement( &mReferenceCounter );
	}

    STDMETHODIMP_(ULONG) Release() 
	{
		ULONG counter = InterlockedDecrement(&mReferenceCounter);
		if ( counter==0 )
			delete this;
		return counter;
	}
		
	STDMETHODIMP QueryInterface( REFIID /*riid*/, void** /*ppvObject*/ )
    {
	return E_NOTIMPL;
        /*if (NULL == ppvObject) return E_POINTER;
        if (riid == __uuidof(IUnknown))
        {
            *ppvObject = static_cast<IUnknown*>(this);
			return S_OK;
        }
        if (riid == __uuidof(ISampleGrabberCB))
        {
            *ppvObject = static_cast<ISampleGrabberCB*>(this);
			return S_OK;
        }
        //return E_NOTIMPL;
		return E_NOINTERFACE;		// http://social.msdn.microsoft.com/Forums/en-US/windowsdirectshowdevelopment/thread/3d5ada34-13d5-48f4-82fc-04018b6e0dbc/
		*/
    }

    STDMETHODIMP SampleCB( double /*Time*/, IMediaSample* /*pSample*/ )
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP BufferCB( double /*Time*/, BYTE *pBuffer, long BufferLen )
    {
		CriticalSectionEnterer criticalSectionRAII( *mCriticalSection );

		if ( BufferLen==0 )		// Called with 0 when we stop the capture graph
			return S_OK;
		
	/*	HDC hdc=GetDC(g_hwnd);
		SetStretchBltMode(hdc, HALFTONE);
		StretchDIBits(hdc, 0, 0, g_WWidth, g_WHeight, 0, 0,
						g_pVih->bmiHeader.biWidth, g_pVih->bmiHeader.biHeight,
						pBuffer, (BITMAPINFO*)&g_pVih->bmiHeader, DIB_RGB_COLORS, SRCCOPY);
	*/
		if ( !mImageBuffer )
			mImageBuffer = new MemoryBuffer( BufferLen );
		assert( BufferLen==static_cast<long>(mImageBuffer->getSizeInBytes()) );

		unsigned char* destBytes = mImageBuffer->getBytes();
		memcpy( destBytes, pBuffer, BufferLen ); 

		mImageSequenceNumber++;

        return S_OK;
    }

	MemoryBuffer* getImageBuffer() const
	{
		return mImageBuffer;
	}

	void deleteImageBuffer()
	{
		delete mImageBuffer;
		mImageBuffer = NULL;
	}

	unsigned int getImageSequenceNumber() const 
	{
		return mImageSequenceNumber;
	}

	void resetImageSequenceNumber() 
	{
		mImageSequenceNumber = 0;
	}

private:
	CRITICAL_SECTION*	mCriticalSection;
	LONG				mReferenceCounter;
	MemoryBuffer*		mImageBuffer;
	unsigned int		mImageSequenceNumber;
};

DeviceInternals::DeviceInternals( COMObjectSharedPtr<IMoniker> moniker )
	: mCriticalSection(), 
	  mSupportedVideoMediaTypes(),
	  mMoniker(moniker),
	  mGraphBuilder(),
	  mCaptureGraphBuilder2(),
	  mSampleGrabberFilter(),
	  mSourceFilter(),
	  mSampleGrabberCallback(NULL),
//	  mImageBuffer(NULL)
	  mIsCapturing(false)
{
	InitializeCriticalSection( &mCriticalSection );

	mSampleGrabberCallback = new SampleGrabberCallback( &mCriticalSection );

	bool ret = initialize();
	assert( ret );
}

DeviceInternals::~DeviceInternals()
{
	if ( isCapturing() )
		stopCapture();
	
	mSampleGrabberFilter->SetCallback( NULL, 1 );
	delete mSampleGrabberCallback;
	mSampleGrabberCallback = NULL;

	mSampleGrabberFilter.reset();
	mGraphBuilder.reset();
	mCaptureGraphBuilder2.reset();
	
	DeleteCriticalSection( &mCriticalSection );
}


// Release the format block for a media type.
void _FreeMediaType(AM_MEDIA_TYPE& mt)		//	http://msdn.microsoft.com/en-us/library/windows/desktop/dd375432(v=vs.85).aspx
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}
// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt != NULL)
    {
        _FreeMediaType(*pmt); 
        CoTaskMemFree(pmt);
    }
}

bool DeviceInternals::initialize()
{
	// Graph Builder
	IGraphBuilder* graphBuilderRaw = NULL;
	HRESULT hr = CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&graphBuilderRaw );
	COMObjectSharedPtr<IGraphBuilder> graphBuilder( graphBuilderRaw );
	if ( FAILED(hr) )
		return false;

	// Capture Graph Builder
	ICaptureGraphBuilder2* captureGraphBuilder2Raw = NULL;
	hr = CoCreateInstance( CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder2Raw );
	COMObjectSharedPtr<ICaptureGraphBuilder2> captureGraphBuilder2( captureGraphBuilder2Raw );
	if ( FAILED(hr) )
		return false;
    
	// Attach the filter graph to the capture graph
	hr = captureGraphBuilder2->SetFiltergraph( graphBuilder.get() );
    if ( FAILED(hr) )
		return false;		

	// Bind the Moniker to a Filter object
	IBaseFilter* sourceFilterRaw = NULL;
    hr = mMoniker->BindToObject( 0, 0, IID_IBaseFilter, (void**)&sourceFilterRaw);
	COMObjectSharedPtr<IBaseFilter> sourceFilter( sourceFilterRaw );
	if ( FAILED(hr) )
		return false;

    // Add Capture filter to the graph
    hr = graphBuilder->AddFilter( sourceFilter.get(), L"Video Capture" );
    if ( FAILED(hr) ) 
        return false;		
		
	// Get capture source information
	IAMStreamConfig* streamConfigRaw = NULL;
//	hr = captureGraphBuilder2->FindInterface( &PIN_CATEGORY_CAPTURE, 0, sourceFilter.get(), IID_IAMStreamConfig, (void**)&streamConfigRaw );
	//hr = sourceFilter->QueryInterface( IID_IAMStreamConfig, (void**)&streamConfigRaw );
	IPin* pinRaw = NULL;
	hr = captureGraphBuilder2->FindPin( sourceFilter.get(), PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, NULL, FALSE, 0, &pinRaw );
	if ( FAILED(hr) )
		return false;

	COMObjectSharedPtr<IPin> pin( pinRaw );
	hr = pin->QueryInterface( IID_IAMStreamConfig, (void**)&streamConfigRaw );
	if ( FAILED(hr) )
		return false;
	
	COMObjectSharedPtr<IAMStreamConfig> streamConfig( streamConfigRaw );
	if ( FAILED(hr) )
		return false;

	int numCaps = 0;
	int videoCapsSize = 0;
	hr = streamConfig->GetNumberOfCapabilities( &numCaps, &videoCapsSize );
	if ( FAILED(hr) )
		return false;
	if ( videoCapsSize!=sizeof(VIDEO_STREAM_CONFIG_CAPS) ) 
		return false;

	for ( int i=0; i<numCaps; ++i )
	{
		VIDEO_STREAM_CONFIG_CAPS videoCaps;	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd407352(v=vs.85).aspx
		AM_MEDIA_TYPE* mediaType = NULL;	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd373477(v=vs.85).aspx
		
		// Get the MediaType and VideoStreamConfigCaps objects.
		// The VideoStreamConfigCaps is pretty deprecated
		hr = streamConfig->GetStreamCaps( i, &mediaType, (BYTE*)&videoCaps );
		if ( FAILED(hr) )
			continue;
	
		LONG width = 0;
		LONG height = 0;
		bool needVerticalFlip = false;
		LONGLONG frameInterval = 0;
		GUID subtype;

		// The subtype comes from the MediaType
		subtype = mediaType->subtype;
		
		// We use the "pbFormat" member of the MediaType to identify the resolution
		if ( mediaType->formattype == FORMAT_VideoInfo )
		{
			assert( mediaType->cbFormat==sizeof(VIDEOINFOHEADER) );	// Something's wrong with the data encapsulated in the MediaType
			VIDEOINFOHEADER* videoInfoHeader = reinterpret_cast<VIDEOINFOHEADER*>( mediaType->pbFormat );
		
			/// Direction of the image (bottom up or to-down...
			// upside down or not...
			// http://msdn.microsoft.com/en-us/library/windows/desktop/dd318229(v=vs.85).aspx
/*
		biHeight
Specifies the height of the bitmap, in pixels.
For uncompressed RGB bitmaps, if biHeight is positive, the bitmap is a bottom-up DIB with the origin at the lower left corner. If biHeight is negative, the bitmap is a top-down DIB with the origin at the upper left corner.
For YUV bitmaps, the bitmap is always top-down, regardless of the sign of biHeight. Decoders should offer YUV formats with postive biHeight, but for backward compatibility they should accept YUV formats with either positive or negative biHeight.
For compressed formats, biHeight must be positive, regardless of image orientation.
*/

// This guy didn't manage to know when flip is needed
// http://social.msdn.microsoft.com/Forums/en-US/windowsdirectshowdevelopment/thread/938e0736-34d5-4faf-ad1c-eb150c6c299b/

			width = videoInfoHeader->bmiHeader.biWidth;
			height = abs( videoInfoHeader->bmiHeader.biHeight );
			if ( subtype==MEDIASUBTYPE_YUY2 )
				needVerticalFlip = false;
			else
				needVerticalFlip = ( videoInfoHeader->bmiHeader.biHeight > 0 );
		}
	
	    // http://msdn.microsoft.com/en-us/library/windows/desktop/dd387907(v=vs.85).aspx
		//assert( videoCaps.MinCroppingSize==videoCaps.MaxCroppingSize == InputSize == MinOutputSize == MaxOutputSize );
		//assert( MinFrameInterval==MaxFrameInterval );

		// The resolution information in the VideoStreamConfigCaps should match the MediaType VideoInfoHeader
		assert( width==videoCaps.MinCroppingSize.cx );
		assert( height==videoCaps.MinCroppingSize.cy );
		assert( width==videoCaps.MaxCroppingSize.cx );
		assert( height==videoCaps.MaxCroppingSize.cy );
		assert( width==videoCaps.MinOutputSize.cx );
		assert( height==videoCaps.MinOutputSize.cy );
		assert( width==videoCaps.MaxOutputSize.cx );
		assert( height==videoCaps.MaxOutputSize.cy );
		//assert( width==videoCaps.InputSize.cx );
		//assert( height==videoCaps.InputSize.cy );

		// We read the frameInterval from the VideoStreamConfigCaps object
		frameInterval = videoCaps.MinFrameInterval;
		
		VideoMediaType supportedMediaType;
		supportedMediaType.width = width;
		supportedMediaType.height = height;
		supportedMediaType.needVerticalFlip = needVerticalFlip;
		supportedMediaType.frameInterval = frameInterval;
		supportedMediaType.subType = subtype;
		
	//printf("%s\n", supportedMediaType.toString().c_str() );
		
		mSupportedVideoMediaTypes.push_back( supportedMediaType );

		_DeleteMediaType( mediaType );
		mediaType = NULL;
	}

	// Create the Sample Grabber Filter
	IBaseFilter* sampleGrabberAsBaseFilterRaw = NULL;
	hr = CoCreateInstance( CLSID_SampleGrabber, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&sampleGrabberAsBaseFilterRaw );
	COMObjectSharedPtr<IBaseFilter> sampleGrabberAsBaseFilter( sampleGrabberAsBaseFilterRaw );
	if ( FAILED(hr) )
		return false;		

	hr = graphBuilder->AddFilter( sampleGrabberAsBaseFilter.get(), L"Sample Grabber");
    if ( FAILED(hr) ) 
		return false;

	// Create the Null Renderer Filter
	IBaseFilter* nullFilterRaw = NULL;
	hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&nullFilterRaw );
	COMObjectSharedPtr<IBaseFilter> nullFilter( nullFilterRaw );
	if ( FAILED(hr) )
		return false;

	hr = graphBuilder->AddFilter( nullFilter.get(), L"Null Renderer" );
    if ( FAILED(hr) ) 
	    return false;
    
	// Configure Sample Grabber
	ISampleGrabber* sampleGrabberFilterRaw = NULL;
	hr = sampleGrabberAsBaseFilter->QueryInterface( IID_ISampleGrabber, (void**)&sampleGrabberFilterRaw );
	COMObjectSharedPtr<ISampleGrabber> sampleGrabberFilter( sampleGrabberFilterRaw );	
	if ( FAILED(hr) )
		return false;

	hr = sampleGrabberFilter->SetOneShot( FALSE );
	if ( FAILED(hr) )
		return false;

	hr = sampleGrabberFilter->SetBufferSamples( FALSE );
	if ( FAILED(hr) )
		return false;
	
	hr = sampleGrabberFilter->SetCallback( mSampleGrabberCallback, 1 );			// 1 means the callback called is ISampleGrabberCB::BufferCB()
	if ( FAILED(hr) )
		return false;

	// Connect Filters: Source -> SampleGrabber -> Null
hr = captureGraphBuilder2->RenderStream( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, sourceFilter.get(), sampleGrabberAsBaseFilter.get(), nullFilter.get() );
//	hr = captureGraphBuilder2->RenderStream( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, sourceFilter.get(), sampleGrabberAsBaseFilter.get(), nullFilter.get() );
	if ( FAILED(hr) )	// E_FAIL ?
		return false;

	// Store objects as members
	mGraphBuilder = graphBuilder;
	mCaptureGraphBuilder2 = captureGraphBuilder2;
	mSampleGrabberFilter = sampleGrabberFilter;
	mSourceFilter = sourceFilter;
	return true;
}

bool DeviceInternals::startCapture( std::size_t indexMediaType )
{
	if ( isCapturing() )
		return false;

	// Get the VideoMediaType corresponding to the index
	if ( indexMediaType>=mSupportedVideoMediaTypes.size() )
		return false;
	const VideoMediaType& videoMediaType = mSupportedVideoMediaTypes[indexMediaType];


	// Set capture source information
	IAMStreamConfig* streamConfigRaw = NULL;
//	hr = captureGraphBuilder2->FindInterface( &PIN_CATEGORY_CAPTURE, 0, sourceFilter.get(), IID_IAMStreamConfig, (void**)&streamConfigRaw );
	//hr = sourceFilter->QueryInterface( IID_IAMStreamConfig, (void**)&streamConfigRaw );
	IPin* pinRaw = NULL;
	HRESULT hr = mCaptureGraphBuilder2->FindPin( mSourceFilter.get(), PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, NULL, FALSE, 0, &pinRaw );
	if ( FAILED(hr) )
		return false;

	COMObjectSharedPtr<IPin> pin( pinRaw );
	hr = pin->QueryInterface( IID_IAMStreamConfig, (void**)&streamConfigRaw );
	if ( FAILED(hr) )
		return false;
	
	COMObjectSharedPtr<IAMStreamConfig> streamConfig( streamConfigRaw );
	if ( FAILED(hr) )
		return false;

	// Fetch the indexMediaTypeth MediaType and VideoCaps
	VIDEO_STREAM_CONFIG_CAPS videoCaps;	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd407352(v=vs.85).aspx
	AM_MEDIA_TYPE* mediaType = NULL;	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd373477(v=vs.85).aspx
	hr = streamConfig->GetStreamCaps( static_cast<int>(indexMediaType), &mediaType, (BYTE*)&videoCaps );
	if ( FAILED(hr) )
		return false;

	// Double check its the same as our internally stored VideoMediaType
	assert( videoCaps.MinCroppingSize.cx==videoMediaType.width );
	assert( videoCaps.MinCroppingSize.cy==videoMediaType.height );
	assert( videoCaps.MinFrameInterval==videoMediaType.frameInterval );
	assert( mediaType->subtype==videoMediaType.subType );

streamConfig->SetFormat( mediaType );

	// Set the SampleGrabberFilter to use it
	hr = mSampleGrabberFilter->SetMediaType( mediaType );
    if ( FAILED(hr) ) 
		return false; 
	_DeleteMediaType( mediaType );
	mediaType = NULL;

/*	::Sleep(100);
	// Store the media type for later use.
	AM_MEDIA_TYPE mediaTypeInUse;
	hr = mSampleGrabberFilter->GetConnectedMediaType( &mediaTypeInUse );		// !!!!!!!!!!!!!!!
	if ( FAILED(hr) )
		return false;
	assert( mediaType->subtype==mediaTypeInUse.subtype );
*/

/*	// Examine the format block.
	if ( (g_mt.formattype == FORMAT_VideoInfo) && 
		 (g_mt.cbFormat >= sizeof(VIDEOINFOHEADER)) &&
		 (g_mt.pbFormat != NULL) ) 
	{ 
		g_pVih = (VIDEOINFOHEADER*)g_mt.pbFormat; 
	}
	else 
	{ 
		return false;
*/
	// Check that the MemoryBuffer to receive the image data hasn't been created yet
 	assert( !mSampleGrabberCallback->getImageBuffer() );

	
	// Get Media Control interface from GraphBuilder
	IMediaControl* mediaControlRaw = NULL;
	hr = mGraphBuilder->QueryInterface( IID_IMediaControl, (LPVOID*)&mediaControlRaw );
	COMObjectSharedPtr<IMediaControl> mediaControl( mediaControlRaw );
	if ( FAILED(hr) )
		return false;
	
	// Start the capture graph
	hr = mediaControl->Run();
	if ( FAILED(hr) ) 
		return false; 

	mIsCapturing = true;

	return true;
}

bool DeviceInternals::stopCapture()
{
	if ( !isCapturing() )
		return true;
		
	// Get Media Control interface from GraphBuilder
	IMediaControl* mediaControlRaw = NULL;
	HRESULT hr = mGraphBuilder->QueryInterface( IID_IMediaControl, (LPVOID*)&mediaControlRaw );
	COMObjectSharedPtr<IMediaControl> mediaControl( mediaControlRaw );
	if ( FAILED(hr) )
		return false;

	// Stop the capture graph
	hr = mediaControl->Stop();
	if ( FAILED(hr) ) 
		return false; 

	mSampleGrabberCallback->deleteImageBuffer();
	mSampleGrabberCallback->resetImageSequenceNumber();

	mIsCapturing = false;
	return true;
}

bool DeviceInternals::getCapturedImage( MemoryBuffer& buffer, unsigned int& sequenceNumber, LONGLONG& timestamp ) const
{
	CriticalSectionEnterer criticalSectionRAII( mCriticalSection );

	// Check that we're currently capturing and that a first image was already grabbed
	const MemoryBuffer* capturedImageBuffer = mSampleGrabberCallback->getImageBuffer();
	if ( !capturedImageBuffer )
		return false;

	// Initialize output parameters
	sequenceNumber = mSampleGrabberCallback->getImageSequenceNumber();
	timestamp = 0;

	// Check that the destination buffer matches the size
/*	if ( buffer.getSizeInBytes()!=capturedImageBuffer->getSizeInBytes() )
		return false;
	// Fill the output parameters
	bool ret = buffer.copyFrom( *capturedImageBuffer );
	assert( ret );
*/
	unsigned int numBytes = std::min( buffer.getSizeInBytes(), capturedImageBuffer->getSizeInBytes() );
	memcpy( buffer.getBytes(), (void*)capturedImageBuffer->getBytes(), numBytes );

	
//	sequenceNumber = mCapturedImageNumber;
//	timestamp = mCapturedImageTimestamp;
	
	return true;
}

}