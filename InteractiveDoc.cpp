
// InteractiveDoc.cpp : implementation of the CInteractiveDoc class
//

#include "stdafx.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CInteractiveDoc

IMPLEMENT_DYNCREATE(CInteractiveDoc, CDocument)

BEGIN_MESSAGE_MAP(CInteractiveDoc, CDocument)
END_MESSAGE_MAP()


// CInteractiveDoc construction/destruction

CInteractiveDoc::CInteractiveDoc()
{
	// TODO: add one-time construction code here

}

CInteractiveDoc::~CInteractiveDoc()
{
}

BOOL CInteractiveDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	CEditView* v = (CEditView*)m_viewList.GetHead();
	v->SetTabStops(9);

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CInteractiveDoc serialization
LPTSTR UTF8ToUnicode(LPCSTR t);
LPSTR UnicodeToUTF8(LPCTSTR s);

void CInteractiveDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		auto view = (CEditView*)m_viewList.GetHead();
		LPSTR buffer = UnicodeToUTF8(view->LockBuffer());
		view->UnlockBuffer();
		ar.Write(buffer, strlen(buffer));
		//	SerializeRaw(ar);
		delete[] buffer;
		//this->m_viewList.Serialize(ar);
	}
	else
	{
		CEditView* v = (CEditView*)m_viewList.GetHead();
			
		CFile *f = ar.GetFile();
		ULONGLONG len = f->GetLength();
		LPSTR buffer1 = new CHAR[(int)len+1];
		f->Read(buffer1, (int)len);
		buffer1[(int)len] = 0;
		for (int i = 1; i < len; ++i) {
			if (buffer1[i] == '\n') {
				if (buffer1[i + 1] == '\r' || buffer1[i - 1] == '\r') break;

				int count = 0;
				for (int j = 0; j < len; ++j)if (buffer1[j] == '\n') ++count;
				int k = 0;
				LPSTR buffern = new CHAR[(int)len + 2+count];
				for (int j = 0; j < len; ++j, ++k) {
					buffern[k] = buffer1[j];
					if (buffer1[j] == '\n') {
						buffern[k] = '\r';
						buffern[++k] = '\n';
					}
				}
				delete[] buffer1;
				buffer1 = buffern;
				buffer1[k] = 0;

				break;
			}

		}
		LPTSTR buffer2 = UTF8ToUnicode(buffer1);
		delete[] buffer1;
		int sellen = v->GetWindowTextLength();
		v->GetEditCtrl().SetSel(0, sellen);
		v->GetEditCtrl().ReplaceSel(buffer2);
		delete[] buffer2;
		v->SetTabStops(9);
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CInteractiveDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CInteractiveDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CInteractiveDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CInteractiveDoc diagnostics

#ifdef _DEBUG
void CInteractiveDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CInteractiveDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CInteractiveDoc commands
