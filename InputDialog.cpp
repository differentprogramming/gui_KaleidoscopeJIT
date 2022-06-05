// InputDialog.cpp : implementation file
//

#include "stdafx.h"



// CInputDialog dialog

IMPLEMENT_DYNAMIC(CInputDialog, CDialogEx)

CInputDialog::CInputDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_INPUT_DIALOG, pParent)
{

}

CInputDialog::~CInputDialog()
{
}

void CInputDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInputDialog, CDialogEx)
//	ON_WM_CREATE()
	ON_BN_CLICKED(IDOK, &CInputDialog::OnBnClickedOk)
END_MESSAGE_MAP()


// CInputDialog message handlers


void CInputDialog::OnBnClickedOk()
{
	CWnd *label = GetDlgItem(IDC_INPUT_EDIT);
	label->GetWindowText(input);
	CDialogEx::OnOK();
}


BOOL CInputDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CWnd *label = GetDlgItem(IDC_PROMPT_TEXT);
	label->SetWindowText(prompt);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
