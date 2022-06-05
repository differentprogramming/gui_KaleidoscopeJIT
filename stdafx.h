
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once
#if (_MSC_VER >= 1915)
#define no_init_all deprecated
#endif
//#ifndef WIN32_LEAN_AND_MEAN
//#	define WIN32_LEAN_AND_MEAN
//#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions





#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars



#include "afxdialogex.h"

#include "afxwinappex.h"
#include "afxdialogex.h"




#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#include <stdio.h>
#include <tchar.h>
#include <stdarg.h>
#include <wchar.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
#include <stdexcept>
#include <utility>
//#include <boost/any.hpp>
#include <vector>
#include <list>
//#include <boost/regex.hpp>
#include <boost/variant.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/flyweight.hpp>
#include <unordered_map>  
#include <map>
#include <sstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <functional>
#include <map>
#include <atomic>
#include <iomanip>
#include "tinyutf8.h"
#include <propkey.h>

#include "..\Solid Ch3\dirent_for_windows.h"
#include "..\Solid Ch3\spooky.h"
#include "InputDialog.h"
#include "ChildFrm.h"
#include "Resource.h"
#include "Interactive.h"
#include "InteractiveDoc.h"
#include "MainFrm.h"
#include "OutputWnd.h"
#include "Interactive.h"
#include "MainFrm.h"

#include "InteractiveView.h"
#include "..\Solid Ch3\hashtable.h"

#undef ERROR
#undef IN