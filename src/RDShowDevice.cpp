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
#include "RDShowDevice.h"

#include <assert.h>
#include <algorithm>
#include "RDShowDeviceInternals.h"

namespace RDShow
{

Device::Device( DeviceManager* parentDeviceManager, void* monikerSharedPtrAsVoidPtr, const std::string& name, const std::string& path )
	: mParentDeviceManager(parentDeviceManager),
	  mName(name),
	  mPath(path),
	  mSupportedCaptureSettingsList(),
	  mMediaTypeIndices(),
	  mInternals(NULL),
	  mStartedCaptureSettingsIndex(0),
	  mCapturedImage(NULL)
{
	COMObjectSharedPtr<IMoniker>& monikerSharedPtr = *(reinterpret_cast< COMObjectSharedPtr<IMoniker>* >( monikerSharedPtrAsVoidPtr ));
	mInternals = new DeviceInternals( monikerSharedPtr );

	// Convert the supported VideoMediaTypes of the DeviceInternals into a CaptureSettingsList
	// We only keep the types that our Image class can handle
	CaptureSettingsList settingsList;
	const DeviceInternals::VideoMediaTypes&	mediaTypes = mInternals->getSupportedVideoMediaTypes();
	for ( std::size_t index=0; index<mediaTypes.size(); ++index )
	{
		const DeviceInternals::VideoMediaType mediaType = mediaTypes[index];
		
		bool supported = true;
		ImageFormat::Encoding encoding = ImageFormat::RGB24;
		
	//printf("%s\n", mediaType.toString().c_str() );
		if ( mediaType.subType==MEDIASUBTYPE_RGB24 )		// RGB24 in the MediaType world actually means BGR24 (reading each byte in sequence gives: blue, green, red)
			encoding = ImageFormat::BGR24;
		else if ( mediaType.subType==MEDIASUBTYPE_RGB32 )	// Same for RGB32, the sequence is blue, green, red, unused
			encoding = ImageFormat::BGRX32;
		else if ( mediaType.subType==MEDIASUBTYPE_YUY2 )
			encoding = ImageFormat::YUYV;
		else 
			supported = false;

		if ( supported )
		{
			ImageFormat imageFormat = ImageFormat( mediaType.width, mediaType.height, encoding );
			CaptureSettings settings( imageFormat, mediaType.getFrameRate() );
			mSupportedCaptureSettingsList.push_back( settings );
			mMediaTypeIndices.push_back(index);
		}
	}
}

Device::~Device()
{
	if ( isCapturing() )
		stopCapture();
	delete mInternals;
	mInternals = NULL;
}

bool Device::isCapturing() const
{
	return mInternals->isCapturing();
}
/*
bool Device::startCapture( const CaptureSettings& captureSettings )
{
	if ( isCapturing() )
		return false;

	for ( std::size_t index=0; index<mSupportedCaptureSettingsList.size(); ++index )
	{
		if ( captureSettings==mSupportedCaptureSettingsList[index] )
			return startCapture( index );
	}
	return false;
}*/

bool Device::startCapture( std::size_t captureSettingsIndex )
{
	if ( isCapturing() )
		return false;

	if ( captureSettingsIndex>=mSupportedCaptureSettingsList.size() )
		return false;

	// Remember which CaptureSettings we've started
	mStartedCaptureSettingsIndex = captureSettingsIndex;
	
	// Prepare the Image that will receive the data when the update method is called
	const CaptureSettings& captureSettings = mSupportedCaptureSettingsList[mStartedCaptureSettingsIndex];
	mCapturedImage = new CapturedImage( captureSettings.getImageFormat() );
	
	// Find the MediaType corresponding to the index of the CaptureSettings to use
	assert( mStartedCaptureSettingsIndex<mMediaTypeIndices.size() );
	int mediaTypeIndex = mMediaTypeIndices[mStartedCaptureSettingsIndex];
	
	//const DeviceInternals::VideoMediaType& mediaType = mInternals->getSupportedVideoMediaTypes()[mediaTypeIndex];
	// Prepare an image for vertical flip if necessary
	/*assert( !mTempImage );
	if ( mediaType.stride<0 )
		mTempImage = new Image( captureSettings.getImageFormat() );
	*/

	// Start the capture
	bool ret = mInternals->startCapture( mediaTypeIndex );
	if ( ret )
	{
		// Notify
		for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
			(*itr)->onDeviceStarted( this );
	}
/*	else
	{
		delete mTempImage;
		mTempImage = NULL;
	}
*/
	return ret;
}

bool Device::getStartedCaptureSettingsIndex( unsigned int& index ) const
{
	index = 0;
	if ( !isCapturing() )
		return false;
	index = mStartedCaptureSettingsIndex;
	return true;
}

/*bool Device::getStartedCaptureSettings( CaptureSettings& settings ) const
{
	settings = CaptureSettings();
	if ( !isCapturing() )
		return false;
	settings = mSupportedCaptureSettingsList[mStartedCaptureSettingsIndex];
	return true;
}*/

void Device::stopCapture()
{
	if ( !isCapturing() )
		return; 

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceStopping( this );

	mInternals->stopCapture();

	// Delete the CaptureImage that receives the data
	delete mCapturedImage;
	mCapturedImage = NULL;
	
	// Also delete the TempImage when that is only created when a vertical flip is needed
/*	delete mTempImage;
	mTempImage = NULL;
*/
	mStartedCaptureSettingsIndex = 0;
}

void Device::update()
{
	if ( !isCapturing() )
		return;

	assert( mCapturedImage );

	MemoryBuffer& buffer = mCapturedImage->getImage().getBuffer();
	unsigned int sequenceNumber = 0;
	LONGLONG timestamp = 0;
	bool ret = mInternals->getCapturedImage( buffer, sequenceNumber, timestamp );
	if ( !ret )
		return;
	
	// Set the sequence number
	mCapturedImage->setSequenceNumber( sequenceNumber );
	
	// Set the timestamp
	// The timestamp coming form the Internals object is in 100 nanosecond units
	// http://msdn.microsoft.com/fr-fr/library/windows/desktop/dd374658(v=vs.85).aspx
	float timestampInSec = static_cast<float>(timestamp) /  1e7f;		
	mCapturedImage->setTimestampInSec( timestampInSec );	

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceCapturedImage( this );
}

void Device::addListener( Listener* listener )
{
	assert(listener);
	mListeners.push_back(listener);
}

bool Device::removeListener( Listener* listener )
{
	Listeners::iterator itr = std::find( mListeners.begin(), mListeners.end(), listener );
	if ( itr==mListeners.end() )
		return false;
	mListeners.erase( itr );
	return true;
}

}