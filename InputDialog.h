#pragma once


// CInputDialog dialog

class CInputDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CInputDialog)

public:
	CString prompt;
	CString input;

	CInputDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInputDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_INPUT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
//	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
};
