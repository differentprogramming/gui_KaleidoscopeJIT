
// Interactive.h : main header file for the Interactive application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CInteractiveApp:
// See Interactive.cpp for the implementation of this class
//

class CInteractiveApp : public CWinAppEx
{
public:
	CInteractiveApp();
	bool CloseDocumentByTitle(wchar_t* title);


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnFormatFont();

};

extern CInteractiveApp theApp;
