
#pragma once

/////////////////////////////////////////////////////////////////////////////
// COutputList window

class COutputList : public CEdit
{
// Construction
public:
	COutputList();
	int end_of_last_output;
// Implementation
public:
	virtual ~COutputList();
	void append_text(LPCTSTR txt);
protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();
	void OnChar(UINT c, UINT rep_cnt, UINT flags);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
};

class COutputWnd : public CDockablePane
{
// Construction
public:
	COutputWnd();

	void UpdateFonts();

// Attributes
protected:
	CMFCTabCtrl	m_wndTabs;

	COutputList m_wndOutputBuild;
//	COutputList m_wndOutputDebug;
//	COutputList m_wndOutputFind;

protected:
	void FillBuildWindow();
//	void FillDebugWindow();
//	void FillFindWindow();

	void AdjustHorzScroll(CListBox& wndListBox);
// Implementation
public:
	virtual ~COutputWnd();
	void AddLineWindow(wchar_t*path, LPSTR filename, LPSTR t, int windowNumber);
	void append_string(LPCSTR txt);
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

