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
#include "RDShowDevice.h"

#include <stdio.h>
#include <assert.h>

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <windows.h>

int main()
{
	RDShow::DeviceManager* deviceManager = new RDShow::DeviceManager();
	deviceManager->update();
	
	if ( deviceManager->getDevices().empty() )
		return 0;

	RDShow::Device* device = deviceManager->getDevices()[0];
	int i=0;
	while (i<50)
	{
		deviceManager->update();

		if ( i==5 || i==30)
		{
			bool ret = device->startCapture( 0 );
			printf("start:%d", ret);
		}
		
		::Sleep(100);
		
		printf(".");
		if ( i==20 || i==40 )
		{
			device->stopCapture();
			printf("stop" );
		}
		i++;
	}
	delete deviceManager;
	deviceManager = NULL;

	return 0;
}