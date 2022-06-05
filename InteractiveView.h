
// InteractiveView.h : interface of the CInteractiveView class
//

#pragma once


class CInteractiveView : public CEditView
{
protected: // create from serialization only
	static std::atomic_int WindowCounter;
	int myWindowNumber;

	CInteractiveView();
	DECLARE_DYNCREATE(CInteractiveView)

// Attributes
public:
	CInteractiveDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	void DisplayStatus();
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CInteractiveView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMenu();
	afx_msg void OnAccelerator32772();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

#ifndef _DEBUG  // debug version in InteractiveView.cpp
inline CInteractiveDoc* CInteractiveView::GetDocument() const
   { return reinterpret_cast<CInteractiveDoc*>(m_pDocument); }
#endif

