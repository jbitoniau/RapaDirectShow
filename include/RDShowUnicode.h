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

//#include <tchar.h>
#include <string>

namespace RDShow
{

/*
	Unicode

	A utility class that can perform conversion between UTF-16 and UTF-8 string.
	
	On Windows, two systems exist in terms of characters: UNICODE and MBCS.
	You choose one or the other in the project settings under "Character Set".

	When UNICODE is defined, the Windows system functions you are supposed to 
	use are the ones manipulating UTF-16 encoded wide strings (in the form of 
	wchar_t* or equivalent typedef/macro). For example: MessageBoxW()

	When MBCS is defined, the system functions use narrow strings (char* or 
	equivalent typedef/macro). For example: MessageBoxA().
	The encoding of the string is a Windows code-page based	thing 
	(not too sure about the details, for example, which code-page is used).
	The encoding is not UNICODE though.

	To simplify the programmer's life, Windows can hide which system is used
	in its API through the use of TCHAR string macros (TCHAR*). The programmer
	in that case simply has to call the MessageBox() method in our example.
	
	Anyhow, all this mess is Windows-specific. To write portable code, it's
	recommended to use std::string everywhere in your code and consider that
	the content is UTF-8 encoded. Then when it comes to calling Windows 
	system function you should make appropriate string conversion before and
	after (if the function returns a string). This is all explained
	extremely well in http://www.utf8everywhere.org/
*/
class Unicode
{
public:
	//static std::string	TCHARToUTF8( const TCHAR* tcharString );
	static std::wstring	MBCStoUTF16String( const std::string& mbcsString );
	static std::string	UTF16toUTF8String( const std::wstring& utf16String );
	static std::wstring	UTF8toUTF16String( const std::string& utf8String );
};

}