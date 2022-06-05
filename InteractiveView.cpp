
// InteractiveView.cpp : implementation of the CInteractiveView class
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CInteractiveView

IMPLEMENT_DYNCREATE(CInteractiveView, CEditView)

BEGIN_MESSAGE_MAP(CInteractiveView, CEditView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CEditView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CEditView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CInteractiveView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_Menu, &CInteractiveView::OnMenu)
	ON_COMMAND(ID_ACCELERATOR32772, &CInteractiveView::OnAccelerator32772)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CREATE()
END_MESSAGE_MAP()

// CInteractiveView construction/destruction

CInteractiveView::CInteractiveView()
{
	myWindowNumber = ++WindowCounter;
}

CInteractiveView::~CInteractiveView()
{
}

std::atomic_int CInteractiveView::WindowCounter = 0;


BOOL CInteractiveView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CEditView::PreCreateWindow(cs);
}

extern CMFCStatusBar *status_bar;

void CInteractiveView::DisplayStatus()
{
	int lineIndex = GetEditCtrl().LineIndex(-1);
	// get the line number
	int lineNum = GetEditCtrl().LineFromChar(lineIndex) + 1;
	// get the column
	int start, end;
	GetEditCtrl().GetSel(start, end);
	int col = end > lineIndex ? end - lineIndex + 1 : 1;
	CString str;
	str.Format(_T("Line %d of %d column %d"), lineNum, GetEditCtrl().GetLineCount(), col);

	if (status_bar != nullptr) status_bar->SetPaneText(0, str, TRUE);

}

// CInteractiveView drawing

void CInteractiveView::OnDraw(CDC* /*pDC*/)
{
	CInteractiveDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CInteractiveView printing


void CInteractiveView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CInteractiveView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CInteractiveView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CInteractiveView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CInteractiveView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CInteractiveView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CInteractiveView diagnostics

#ifdef _DEBUG
void CInteractiveView::AssertValid() const
{
	CEditView::AssertValid();
}

void CInteractiveView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}

CInteractiveDoc* CInteractiveView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CInteractiveDoc)));
	return (CInteractiveDoc*)m_pDocument;
}
#endif //_DEBUG

LPTSTR UTF8ToUnicode(LPCSTR t);
LPSTR UnicodeToUTF8(LPCTSTR s)
{
	int bsize = WideCharToMultiByte(
		CP_UTF8,
		0,
		s,
		-1,
		NULL,
		0,
		NULL,
		NULL
	);
	LPSTR mbuffer = new CHAR[bsize + 1];

	WideCharToMultiByte(
		CP_UTF8,
		0,
		s,
		-1,
		mbuffer,
		bsize + 1,
		NULL,
		NULL
	);
	return mbuffer;
}

// CInteractiveView message handlers
#include "OutputWnd.h"
extern COutputWnd *output_window;
void CInteractiveView::OnMenu()
{
	if (output_window != nullptr)
	{
		
		CString filenamebuffer =
			GetDocument()->GetPathName();

		if (filenamebuffer.GetLength() > 0) {
			CFile theFile;



			theFile.Open(filenamebuffer.LockBuffer(), CFile::modeCreate | CFile::modeWrite);
			filenamebuffer.UnlockBuffer();
			GetDocument()->Serialize(CArchive(&theFile, CArchive::store));
			theFile.Close();
		}

		wchar_t* path = _wcsdup(filenamebuffer.LockBuffer());

		for (int i=wcslen(path)-1; i>=0; --i){
			if (path[i] == L'\\') {
				path[i + 1] = 0;
				break;
			}
		}
		filenamebuffer= GetDocument()->GetTitle();

		LPSTR mbuffer = UnicodeToUTF8(LockBuffer());
		UnlockBuffer();

		output_window->AddLineWindow(path, UnicodeToUTF8(filenamebuffer.LockBuffer()), mbuffer, myWindowNumber);
		filenamebuffer.UnlockBuffer();
		free(path);
		delete[] mbuffer;
	}
	// TODO: Add your command handler code here
}


void CInteractiveView::OnAccelerator32772()
{
	// TODO: Add your command handler code here
	OnMenu();
}

void CInteractiveView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	CEditView::OnChar(nChar, nRepCnt, nFlags);

}


void CInteractiveView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	CEditView::OnKeyDown(nChar, nRepCnt, nFlags);
	DisplayStatus();
}


void CInteractiveView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	CEditView::OnKeyUp(nChar, nRepCnt, nFlags);
}

extern bool first_font;
extern CFont m_font;

int CInteractiveView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEditView::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!first_font) {
		SetFont(&m_font);
//		UpdateWindow();
	}
	else {
		UINT mRead = 0;
		LOGFONT *mLogFont;
		if (theApp.GetProfileBinary(_T("1.0.0.1"), _T("Font"), reinterpret_cast<LPBYTE *>(&mLogFont), &mRead)) {
			if (m_font.CreateFontIndirect(mLogFont)) {
				first_font = false;
				SetFont(&m_font);
				delete[] mLogFont;
			}
		}
	}

	return 0;
}
