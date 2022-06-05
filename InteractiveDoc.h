
// InteractiveDoc.h : interface of the CInteractiveDoc class
//


#pragma once


class CInteractiveDoc : public CDocument
{
protected: // create from serialization only
	CInteractiveDoc();
	DECLARE_DYNCREATE(CInteractiveDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	CPtrList& get_view_list() { return m_viewList; }
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CInteractiveDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
