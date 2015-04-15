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
#include "RDShowDeviceManager.h"

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

#include "RDShowCOMObjectSharedPtr.h"
#include "RDShowDevice.h"
#include "RDShowUnicode.h"
#include <vector>
#include <assert.h>
#include <algorithm>

// http://msdn.microsoft.com/en-us/library/windows/desktop/dd407331(v=vs.85).aspx
// http://www.codeproject.com/Articles/34663/DirectShow-Examples-for-Using-SampleGrabber-for-Gr

namespace RDShow
{

/*
	DeviceManager::Internals
*/
class DeviceManager::Internals
{
public:
	Internals( DeviceManager* parentDeviceManager );
	~Internals();

	void			update();
	const Devices&	getDevices() const	{ return mDevices; }

	void			addListener( Listener* listener );
	bool			removeListener( Listener* listener );

protected:
	static void		wideCharStringToMultiByteString( const wchar_t* wideCharString, std::string& multiByteString );

	struct DeviceInfo
	{
		std::string friendlyName;		
		std::string devicePath;		
		COMObjectSharedPtr<IMoniker> moniker;
	};
	typedef std::vector<DeviceInfo>	DeviceInfos;

	static bool		getDeviceInfo( COMObjectSharedPtr<IMoniker>& moniker, DeviceInfo& deviceInfo );
	static bool		enumerateDevices( DeviceInfos& deviceInfos );
	void			updateDeviceList();

	void			createDevice( DeviceInfo& deviceInfo );
	void			deleteDevice( Device* device );

	static LRESULT CALLBACK wndProcHook( int nCode, WPARAM wParam, LPARAM lParam );

private:
	DeviceManager*  mParentDeviceManager;
	bool			mUpdateDeviceListAtNextUpdate;
	Devices			mDevices;

	typedef	std::vector<DeviceManager::Listener*> Listeners; 
	Listeners		mListeners;

	typedef std::vector< DeviceManager::Internals* > InternalObjects;
	static InternalObjects mInternalObjects;
	HHOOK			mHookHandle;
};

// The list of all the DeviceManager::Internals instances that exist in the application
// This is needed because of the static nature of the WindowProc hook
DeviceManager::Internals::InternalObjects DeviceManager::Internals::mInternalObjects;

DeviceManager::Internals::Internals( DeviceManager* parentDeviceManager )
	: mParentDeviceManager( parentDeviceManager ),
	  mUpdateDeviceListAtNextUpdate(true),
	  mDevices(),
	  mListeners(),
	  mHookHandle(0)
{
	// Initialize the COM library 
	// We dont check the result on purpose here. See documentation: 
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms695279(v=vs.85).aspx
	CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

	// Add this instance to the static list
	mInternalObjects.push_back( this );
	
	// Register hook when this is the first instance of DeviceManager::Internals created
	// See this article for extra information
	// http://www.codeproject.com/Articles/14500/Detecting-Hardware-Insertion-and-or-Removal
	if ( mInternalObjects.size()==1 )
	{
		DWORD threadID = GetCurrentThreadId();
		HINSTANCE hInstance = GetModuleHandle(NULL) ;
		mHookHandle = SetWindowsHookEx( WH_CALLWNDPROC, wndProcHook, hInstance, threadID );
		assert( mHookHandle );
	}
}

DeviceManager::Internals::~Internals()
{
	// Delete all devices
	Devices devices = mDevices;		// The copy is on purpose here
	for ( std::size_t i=0; i<devices.size(); ++i )
		deleteDevice( devices[i] );
	assert( mDevices.empty() );

	// Shutdown COM 
	CoUninitialize();

	// Remove this instance from the static list
	InternalObjects::iterator itr = std::find( mInternalObjects.begin(), mInternalObjects.end(), this );
	assert( itr!=mInternalObjects.end() );
	mInternalObjects.erase(itr);

	// If there's no more instances, remove the hook
	if ( mInternalObjects.empty() )
	{
		BOOL ret = UnhookWindowsHookEx( mHookHandle );
		assert( ret );
		mHookHandle = 0;
	}
}

LRESULT CALLBACK DeviceManager::Internals::wndProcHook( int nCode, WPARAM wParam, LPARAM lParam )
{
/*	std::wstringstream stream;
	stream << std::hex;
	stream << L"code=" << nCode << L" wparam=" << params.wParam << L" lparam=" << params.lParam << L" message=" << params.message << L" hwnd=" << params.hwnd << L"\n";
	OutputDebugString(stream.str().c_str());
*/
	// Note:
	// We only process the message if it is sent by the current thread.
	// By "current" I guess that they mean the same thread as the one called the hook
	// registration function... but that's a guess :(
	// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms644975(v=vs.85).aspx
	// My goal is to use this information to avoid having to handle concurrency with 
	// a critical section or whatnot.
	// Because if this hook is called by different thread, there's a micro chance that the
	// array of InternalObjects I'm using changes while I'm using it
	if ( wParam!=0 )
	{
		const CWPSTRUCT& params = *reinterpret_cast<CWPSTRUCT*>(lParam);
		if ( params.message==WM_DEVICECHANGE )
		{
			for ( std::size_t i=0; i<mInternalObjects.size(); ++i )
				mInternalObjects[i]->mUpdateDeviceListAtNextUpdate = true;
		}
	}

	// Process event
	return CallNextHookEx( NULL, nCode, wParam, lParam );
}

void DeviceManager::Internals::update()
{
	if ( mUpdateDeviceListAtNextUpdate )
	{
		updateDeviceList();
		mUpdateDeviceListAtNextUpdate = false;
	}

	for ( Devices::iterator itr=mDevices.begin(); itr!=mDevices.end(); ++itr )
	{
		Device* device = *itr;
		device->update();
	}
}

bool DeviceManager::Internals::getDeviceInfo( COMObjectSharedPtr<IMoniker>& moniker, DeviceInfo& deviceInfo )
{
	IPropertyBag* propertyBagRaw = NULL;
	HRESULT hr = moniker->BindToStorage( 0, 0, IID_PPV_ARGS(&propertyBagRaw) );
	COMObjectSharedPtr<IPropertyBag> propertyBag( propertyBagRaw );
	if ( FAILED(hr) )
		return false;

	VARIANT variant;
	VariantInit( &variant );
	hr = propertyBag->Read( L"FriendlyName", &variant, 0 );
	if ( FAILED(hr) )
		return false;
	deviceInfo.friendlyName = Unicode::UTF16toUTF8String( variant.bstrVal );
	VariantClear( &variant ); 

	hr = propertyBag->Read(L"DevicePath", &variant, 0);
	// PS3 Eye camera fail to return the DevicePath (on purpose?). We continue anyway
	if ( FAILED(hr) )
		deviceInfo.devicePath = "NoDevicePath";
	else
 		deviceInfo.devicePath = Unicode::UTF16toUTF8String( variant.bstrVal );
	VariantClear(&variant); 
	
	deviceInfo.moniker = moniker;

	return true;
}

bool DeviceManager::Internals::enumerateDevices( DeviceInfos& deviceInfos )
{
	deviceInfos.clear();

	// Selecting a Capture Device
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd377566(v=vs.85).aspx

	// Create the system device enumerator
	ICreateDevEnum* devEnumRaw = NULL;
	HRESULT hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void**)&devEnumRaw );
    COMObjectSharedPtr<ICreateDevEnum> devEnum( devEnumRaw );
	if ( FAILED(hr) )
		return false;
	
	// Create an enumerator for the video capture devices
	IEnumMoniker* enumMonikerRaw = NULL;
    hr = devEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &enumMonikerRaw, 0 );
	COMObjectSharedPtr<IEnumMoniker> enumMoniker( enumMonikerRaw );
	if ( FAILED(hr) )
		return false;	

	// If there are no enumerators for the requested type, then 
	// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
	if ( !enumMoniker.get() )
		return false;

	IMoniker* monikerRaw = NULL;
	while ( enumMoniker->Next( 1, &monikerRaw, NULL)==S_OK )
    {
		COMObjectSharedPtr<IMoniker> moniker( monikerRaw );

		DeviceInfo deviceInfo;
		if ( getDeviceInfo( moniker, deviceInfo ) )
			deviceInfos.push_back( deviceInfo );
	}

	return true;
}

void DeviceManager::Internals::updateDeviceList()
{
	// Get up-to-date list of Devices
	DeviceInfos deviceInfos;
	DeviceManager::Internals::enumerateDevices( deviceInfos );

	// Determine freshly added Devices
	DeviceInfos newDeviceInfos;
	for ( std::size_t i=0; i<deviceInfos.size(); ++i )
	{
		const std::string& path = deviceInfos[i].devicePath;
		bool found = false;
		for ( std::size_t j=0; j<mDevices.size(); ++j )
		{
			if ( mDevices[j]->getPath()==path )
			{
				found = true;
				break;
			}
		}

		if ( !found )
			newDeviceInfos.push_back( deviceInfos[i] );
	}
	
	// Determine freshly removed Devices
	Devices removedDevices;
	for ( std::size_t i=0; i<mDevices.size(); ++i )
	{
		Device* device = mDevices[i];
		bool found = false;

		for ( std::size_t j=0; j<deviceInfos.size(); ++j )
		{	
			const std::string& path = deviceInfos[j].devicePath;
			if ( device->getPath()==path )
			{
				found = true;
				break;
			}
		}
		if ( !found )
			removedDevices.push_back( device );
	}

	// Create and add the new Devices
	for ( std::size_t i=0; i<newDeviceInfos.size(); ++i )
		createDevice( newDeviceInfos[i] );
	
	// Remove detached Devices
	for ( std::size_t i=0; i<removedDevices.size(); ++i )
		deleteDevice( removedDevices[i] );
}

void DeviceManager::Internals::createDevice( DeviceInfo& deviceInfo )
{
	// Notify 
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceAdding( mParentDeviceManager );

	void* monikerSharedPtrAsVoidPtr = reinterpret_cast<void*>(&deviceInfo.moniker);
	Device* device = new Device( mParentDeviceManager, monikerSharedPtrAsVoidPtr, deviceInfo.friendlyName, deviceInfo.devicePath );
	mDevices.push_back( device );

	// Notify 
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceAdded( mParentDeviceManager, device );
}

void DeviceManager::Internals::deleteDevice( Device* device )
{
	Devices::iterator itr = std::find( mDevices.begin(), mDevices.end(), device );
	if ( itr==mDevices.end() )
		return;

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceRemoving( mParentDeviceManager, device );

	mDevices.erase( itr );
	delete device;

	// Notify
	for ( Listeners::const_iterator itr=mListeners.begin(); itr!=mListeners.end(); ++itr )
		(*itr)->onDeviceRemoved( mParentDeviceManager, device );
}

void DeviceManager::Internals::addListener( Listener* listener )
{
	assert(listener);
	mListeners.push_back(listener);
}

bool DeviceManager::Internals::removeListener( Listener* listener )
{
	Listeners::iterator itr = std::find( mListeners.begin(), mListeners.end(), listener );
	if ( itr==mListeners.end() )
		return false;
	mListeners.erase( itr );
	return true;
}

/*
	DeviceManager
*/
DeviceManager::DeviceManager()
	: mInternals(NULL)
{
	mInternals = new Internals(this);
}

DeviceManager::~DeviceManager()
{
	delete mInternals;
	mInternals = NULL;
}

void DeviceManager::update()
{
	mInternals->update();
}

const Devices& DeviceManager::getDevices() const
{
	return mInternals->getDevices();
}

void DeviceManager::addListener( Listener* listener )
{
	mInternals->addListener(listener);
}

bool DeviceManager::removeListener( Listener* listener )
{
	return mInternals->removeListener(listener);
}





/*

DeviceManager::DeviceManager()
{
	CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

	DeviceInfos deviceInfos;
	enumerateDevices( deviceInfos );

	for ( std::size_t i=0; i<deviceInfos.size(); ++i )
	{
		DeviceInfo& deviceInfo = deviceInfos[i];
		void* monikerSharedPtrAsVoidPtr = reinterpret_cast<void*>(&deviceInfo.moniker);
		Device* device = new Device( monikerSharedPtrAsVoidPtr, deviceInfo.friendlyName, deviceInfo.devicePath );
		mDevices.push_back( device );
	}
}	

DeviceManager::~DeviceManager()
{
	CoUninitialize();
}

void DeviceManager::update()
{
	for ( std::size_t i=0; i<mDevices.size(); ++i )
	{
		Device* device = mDevices[i];
		device->update();
	}
}

void DeviceManager::updateDeviceList()
{
}

bool DeviceManager::getDeviceInfo( COMObjectSharedPtr<IMoniker>& moniker, DeviceInfo& deviceInfo )
{
	IPropertyBag* propertyBagRaw = NULL;
	HRESULT hr = moniker->BindToStorage( 0, 0, IID_PPV_ARGS(&propertyBagRaw) );
	COMObjectSharedPtr<IPropertyBag> propertyBag( propertyBagRaw );
	if ( FAILED(hr) )
		return false;

	VARIANT variant;
	VariantInit( &variant );
	hr = propertyBag->Read( L"FriendlyName", &variant, 0 );
	if ( FAILED(hr) )
		return false;
	deviceInfo.friendlyName = variant.bstrVal;
	//printf("%S\n", var.bstrVal);
	VariantClear( &variant ); 

	hr = propertyBag->Read(L"DevicePath", &variant, 0);
	// PS3 Eye camera fail to return the DevicePath (on purpose?). We continue anyway
	if ( FAILED(hr) )
		deviceInfo.devicePath = L"NoDevicePath";
	else
 		deviceInfo.devicePath = variant.bstrVal;
	VariantClear(&variant); 
	
	deviceInfo.moniker = moniker;

	return true;
}

bool DeviceManager::enumerateDevices( DeviceInfos& deviceInfos )
{
	// Selecting a Capture Device
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd377566(v=vs.85).aspx

	// Create the system device enumerator
	ICreateDevEnum* devEnumRaw = NULL;
	HRESULT hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void**)&devEnumRaw );
    COMObjectSharedPtr<ICreateDevEnum> devEnum( devEnumRaw );
	if ( FAILED(hr) )
		return false;
	
	// Create an enumerator for the video capture devices
	IEnumMoniker* enumMonikerRaw = NULL;
    hr = devEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &enumMonikerRaw, 0 );
	COMObjectSharedPtr<IEnumMoniker> enumMoniker( enumMonikerRaw );
	if ( FAILED(hr) )
		return false;	

	// If there are no enumerators for the requested type, then 
	// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
	if ( !enumMoniker.get() )
		return false;

	IMoniker* monikerRaw = NULL;
	while ( enumMoniker->Next( 1, &monikerRaw, NULL)==S_OK )
    {
		COMObjectSharedPtr<IMoniker> moniker( monikerRaw );

		DeviceInfo deviceInfo;
		if ( getDeviceInfo( moniker, deviceInfo ) )
			deviceInfos.push_back( deviceInfo );
	}

	return true;
}
*/
}
