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
#include "RDShowUnicode.h"

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX 
#include <windows.h>

namespace RDShow
{
/*
std::string	Unicode::TCHARToUTF8( const TCHAR* tcharString )
{
#ifdef _UNICODE
	return UTF16toUTF8String( tcharString );
#else
	return UTF16toUTF8String( MBCStoUTF16String( tcharString ) );
#endif
}*/

std::wstring Unicode::MBCStoUTF16String( const std::string& mbcsString )
{
	if( mbcsString.empty() )    
		return std::wstring();
	// ACP means ANSI Code Page. The conversion depends on the computer current code page settings.
	// See http://msdn.microsoft.com/en-us/library/dd319072%28VS.85%29.aspx
	size_t retStringSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsString.c_str(), static_cast<int>(mbcsString.length()), 0, 0);
    std::wstring retString( retStringSize, L'\0' );
	::MultiByteToWideChar(CP_UTF8, 0, mbcsString.c_str(), static_cast<int>(mbcsString.length()), &retString[0], static_cast<int>(retString.length()));
    return retString;
}

std::string Unicode::UTF16toUTF8String(const std::wstring& utf16String)
{
	if ( utf16String.empty() )
		return std::string();
	int retStringSizeInByte = ::WideCharToMultiByte(CP_UTF8, 0, utf16String.c_str(), static_cast<int>(utf16String.length()), 0, 0, 0, 0);
	std::string retString( retStringSizeInByte, '\0' );
	::WideCharToMultiByte(CP_UTF8, 0, utf16String.c_str(), static_cast<int>(utf16String.length()), &retString[0], retStringSizeInByte, 0, 0);
	return retString;	
}

std::wstring Unicode::UTF8toUTF16String(const std::string& utf8String)
{
	if( utf8String.empty() )    
		return std::wstring();
	size_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), static_cast<int>(utf8String.length()), 0, 0);
    std::wstring ret( reqLength, L'\0' );
	::MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), static_cast<int>(utf8String.length()), &ret[0], static_cast<int>(ret.length()));
    return ret;
}

}