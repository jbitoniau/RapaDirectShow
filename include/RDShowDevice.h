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

#include <string>
#include <vector>
#include "RDShowImage.h"
#include "RDShowCaptureSettings.h"
#include "RDShowCapturedImage.h"

namespace RDShow
{

class DeviceManager;
class DeviceInternals;	

/*
	Device

	Strings are UTF-8 encoded.
*/
class Device
{
public:
	DeviceManager*					getParentDeviceManager() const			{ return mParentDeviceManager; }
	const std::string&				getName() const							{ return mName; }
	const std::string&				getPath() const							{ return mPath; }
	const CaptureSettingsList&		getSupportedCaptureSettingsList() const	{ return mSupportedCaptureSettingsList; }

	bool							isCapturing() const;
	bool							startCapture( std::size_t captureSettingsIndex );
	bool							getStartedCaptureSettingsIndex( unsigned int& index ) const;
	const CapturedImage*			getCapturedImage() const				{ return mCapturedImage; }
	void							stopCapture();

	void							update();

	class Listener
	{
	public:
		virtual ~Listener() {}
		virtual void onDeviceStarted( Device* /*device*/ ) {}
		virtual void onDeviceCapturedImage( Device* /*device*/ ) {}
		virtual void onDeviceStopping( Device* /*device*/ ) {}
	};

	void							addListener( Listener* listener );
	bool							removeListener( Listener* listener );

protected:
	friend class DeviceManager;
	Device( DeviceManager* parentDeviceManager, void* monikerSharedPtrAsVoidPtr, const std::string& name, const std::string& path );
	virtual ~Device();

private:
	DeviceManager*					mParentDeviceManager;
	std::string						mName;
	std::string						mPath;

	CaptureSettingsList				mSupportedCaptureSettingsList;
	std::vector<std::size_t>		mMediaTypeIndices;			// For each CaptureSettings, contains the index of the corresponding MediaType

	DeviceInternals*				mInternals;
	
	unsigned int					mStartedCaptureSettingsIndex;
	CapturedImage*					mCapturedImage;
	
	typedef	std::vector<Listener*> Listeners; 
	Listeners						mListeners;
};

typedef std::vector<Device*> Devices;

}