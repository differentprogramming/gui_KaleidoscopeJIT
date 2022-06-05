
#include "stdafx.h"


extern COutputWnd *output_window;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputBar


COutputWnd::COutputWnd()
{
}

COutputWnd::~COutputWnd()
{
}

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create tabs window:
//	if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 1))
//	{
//		TRACE0("Failed to create output tab window\n");
//		return -1;      // fail to create
//	}

	// Create output panes:
	const DWORD dwStyle = ES_MULTILINE | 
		WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL
		| ES_WANTRETURN
		//| LBS_MULTICOLUMN
		;

	if (!m_wndOutputBuild.Create(dwStyle, rectDummy, this, 2) //||
		//!m_wndOutputDebug.Create(dwStyle, rectDummy, &m_wndTabs, 3) ||
		//!m_wndOutputFind.Create(dwStyle, rectDummy, &m_wndTabs, 4)
		)
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}

	UpdateFonts();

	CString strTabName;
	BOOL bNameValid;

	// Attach list windows to tab:
//	bNameValid = strTabName.LoadString(IDS_BUILD_TAB);
//	ASSERT(bNameValid);
//	m_wndTabs.AddTab(&m_wndOutputBuild, strTabName, (UINT)0);

//	bNameValid = strTabName.LoadString(IDS_DEBUG_TAB);
//	ASSERT(bNameValid);
//	m_wndTabs.AddTab(&m_wndOutputDebug, strTabName, (UINT)1);
//	bNameValid = strTabName.LoadString(IDS_FIND_TAB);
//	ASSERT(bNameValid);
//	m_wndTabs.AddTab(&m_wndOutputFind, strTabName, (UINT)2);

	// Fill output tabs with some dummy text (nothing magic here)
	FillBuildWindow();
//	FillDebugWindow();
//	FillFindWindow();
	m_wndOutputBuild.SetLimitText(1000000);
	m_wndOutputBuild.append_text(L"oh hai> ");
	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// Tab control should cover the whole client area:
	m_wndOutputBuild.SetWindowPos (NULL, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void COutputWnd::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&afxGlobalData.fontRegular);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i ++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}


void COutputList::append_text(LPCTSTR txt)
{
	int len = GetWindowTextLength();
	SetSel(len, len);
	ReplaceSel(txt);
	end_of_last_output = GetWindowTextLength();
}

//std::vector<std::string> interactive_inputs;
LPSTR UnicodeToUTF8(LPCTSTR s);
//int test_libtcc();
void COutputList::OnChar(UINT c, UINT rep_cnt, UINT flags)
{
	CEdit::OnChar(c, rep_cnt, flags);
	int len = GetWindowTextLength();
	if (len < end_of_last_output) {
		CString windowtext;
		end_of_last_output = len;
		GetWindowTextW(windowtext);
		while (end_of_last_output > 0
			&& windowtext.GetAt(end_of_last_output) != L'\r'
			) --end_of_last_output;
	}
	if (c == '\r') {
		int len = GetWindowTextLength();
		if (len > end_of_last_output) {
			//		SetSel(end_of_last_output, len);
			CString all;
			GetWindowTextW(all);
			CString rest(all.Mid(end_of_last_output));
//			if (rest == "libtcc\r\n") {
//				test_libtcc();
//			}
//			else 
			{

				LPSTR line = UnicodeToUTF8(rest.LockBuffer());
				rest.LockBuffer();

				output_window->AddLineWindow(nullptr,"input line", line,-1);
				delete[] line;
				append_text(L"\n\r");
			}
		}
	}
}


LPTSTR UTF8ToUnicode(LPCSTR t)
{
	int tsize = MultiByteToWideChar(
		CP_UTF8,
		0,
		t,
		-1,
		NULL,
		0
	);
	LPTSTR tbuffer = new WCHAR[tsize + 1];

	MultiByteToWideChar(
		CP_UTF8,
		0,
		t,
		-1,
		tbuffer,
		tsize + 1
	);
	return tbuffer;
}

LPTSTR UTF8ToUnicodeAndTranslateLF(LPCSTR t)
{
	int count = 1;
	LPCSTR n = t;
	while (*n) {
		++count;
		if (*n == '\n') ++count;
		++n;
	}
	LPSTR copy = new char[count];
	LPSTR c = copy;
	n = t;
	while (*n) {
		if (*n == '\n') *c++ = '\r';
		*c++ = *n++;
	}
	*c = 0;
	LPTSTR r = UTF8ToUnicode(copy);
	delete[] copy;
	return r;
}

void COutputWnd::append_string(LPCSTR txt)
{
	LPTSTR tbuffer = UTF8ToUnicodeAndTranslateLF(txt);

	m_wndOutputBuild.append_text(tbuffer);
	delete[] tbuffer;

}
void init_parser();
void parse(LPSTR filename, LPSTR t);
//LPCTSTR digits = _T("0123456789abcdef");
//LPTSTR chars = new TCHAR[10000];
//void init_interpreter();
//extern LVar base_env;
#include "Interactive.h"
extern CInteractiveApp theApp;

bool CInteractiveApp::CloseDocumentByTitle(wchar_t* title)
{
	bool found = false;

	POSITION p = theApp.GetFirstDocTemplatePosition();

	while (!found && p)
	{
		CDocTemplate* d = theApp.GetNextDocTemplate(p);
		POSITION p2 = d->GetFirstDocPosition();
		while (!found && p2)
		{
			CDocument* pDocument;
			if ((pDocument = d->GetNextDoc(p2)) != NULL) {
				CString what = pDocument->GetTitle();
				if (wcsncmp(what.GetString(), title, MAX_PATH) == 0) {
					//						d->RemoveDocument(pDocument);
					pDocument->OnCloseDocument();
					//delete pDocument;
					found = true;
				}
			}
		}
	}
	return found;
}

DWORD Spawn(wchar_t* path, wchar_t* command, wchar_t* target, wchar_t* params)
{
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = L"open";
	ShExecInfo.lpFile = L"clang";
	ShExecInfo.lpParameters = params;
	ShExecInfo.lpDirectory = path;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

	DWORD ret=0;
	GetExitCodeProcess(
		ShExecInfo.hProcess,
		&ret
	);

	CloseHandle(ShExecInfo.hProcess);
	return ret;
}


//===========================================================================
// Execute a console(!) applicatin. Can redirect stdio.
//===========================================================================
// IN: application Full(!) name of the application to execute
// IN: cmd_line Optional arguments for the app - can be NULL
// IN: work_dir Optional - can be NULL (means the current dir)
// IN: flags See GenOsse.h
// RET: 0 == success
//===========================================================================
// Redirection of stdio:
// The required/created files must/will reside in "work_dir".
//===========================================================================
// WIN32:
// The code creates the redirection files and hides the console window.
// MAC:
// The console app is itself responsible to create the redirect files
// and to hide the window.
//===========================================================================
// Once you called the app, check the redirected files for data.
//===========================================================================
DWORD os_execconsole(const wchar_t* work_dir,
	const wchar_t* application,
	const wchar_t* cmd_line)
{
	// Param check
	if (!application) { return -1; }


	// Will contain the expanded redirction file names
	//const wchar_t stdin_file[] = { L"stdin.txt" };
	//const wchar_t stdout_file[] = { L"stdout.txt" };
	const wchar_t stderr_file[] = { L"stderr.txt" };

	wchar_t old_path[MAX_PATH+1];
	GetCurrentDirectory(MAX_PATH, old_path);
	// Change into the workdir, if given
	SetCurrentDirectory(work_dir);
	//-----------------------------------------------------------------------------


// WIN32 parameters
	SECURITY_ATTRIBUTES sa = { sizeof(sa) }; // Open files in inheritable mode
	STARTUPINFOW si = { sizeof(si) }; // Input for CreateProcess()
	PROCESS_INFORMATION pi = { 0 }; // Output of CreateProcess()

	sa.bInheritHandle = TRUE; // Allow inheritance
	sa.lpSecurityDescriptor = NULL; // handles to the child process

	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.dwFlags |=  STARTF_USESTDHANDLES;
	//si.wShowWindow = (flags & OS_EC_HIDDEN) ? SW_HIDE : 0;

	// Open/create the stdio-redirections with the inherited security handle
	//if (flags & OS_EC_REDIRECT_STDIN)
	//	if ((si.hStdOutput = CreateFileA(stdin_file, GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, 0, NULL)) == NULL)
	//		return GetLastError();

	//if (flags & OS_EC_REDIRECT_STDOUT)
	//	if ((si.hStdOutput = CreateFileA(stdout_file, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, NULL)) == NULL)
	//		return GetLastError();

	//if (flags & OS_EC_REDIRECT_STDERR)
		if ((si.hStdError = CreateFileW(stderr_file, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, NULL)) == NULL)
			return GetLastError();

	// Build the command line to call
	wchar_t cmd[4 * MAX_PATH];
	wsprintf(cmd, L"%s %s", application, (cmd_line ? cmd_line : L""));

	// Call the process now! Hot! Only 1.29 USD/min!
	int err = CreateProcessW(NULL, // name of process - is done in "cmd"
		cmd, // the complete command to execute
		NULL, // lpProcessAttr
		NULL, // lpThreadAttr
		TRUE, // bInheritHandles
		0, // dwCreationFlags
		NULL, // lpEnvironment
		NULL, // lpCurrDir
		&si, // StartupInfo - contains the StdHandles!
		&pi); // lpProcessInfo
	if (err == FALSE)
	{
		// Oops. "err" is only a boolean flag - get here the correct error value;
		err = GetLastError();
	}
	else
	{
		//if (flags & OS_EC_WAIT)
		//{
			switch (err = WaitForSingleObject(pi.hProcess, INFINITE))
			{
			case WAIT_ABANDONED: err = 0; break;
			case WAIT_OBJECT_0: err = 0; break;
			case WAIT_TIMEOUT: break;
			case WAIT_FAILED: break;
			default: break;
			}
		//}
		//else
		//{
		//	err = 0;
		//}
	}

	// Close the opened files, if the call was blocking
	//if (err || (flags & OS_EC_WAIT))
	//{
		//if (si.hStdInput) { CloseHandle(si.hStdInput); }
		//if (si.hStdOutput) { CloseHandle(si.hStdOutput); }
		if (si.hStdError) { CloseHandle(si.hStdError); }
	//}

	DWORD ret = 0;
	GetExitCodeProcess(
		pi.hProcess,
		&ret
	);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	SetCurrentDirectory(old_path);
	return ret;
}
	//-----------------------------------------------------------------------------
std::string mainish(LPSTR source);

void COutputWnd::AddLineWindow(wchar_t * path, LPSTR filename, LPSTR t, int windowNumber)
{

	std::stringstream message;

	tiny_utf8::string fname = filename;
	tiny_utf8::string target="";
	bool is_c = false;
	bool is_ll = false;
	auto r = fname.rbegin();
	if (*r == (char32_t)'h' || *r == (char32_t)'c') {
		++r;
		if (*r = (char32_t)'.') is_c = true;
	}
	else if (*r == (char32_t)'p') {
		++r;
		if (*r == (char32_t)'p') {
			++r;
			if (*r == (char32_t)'h' || *r == (char32_t)'c') {
				++r;
				if (*r = (char32_t)'.') is_c = true;
			}
		}
	}
	else if (*r == (char32_t)'l') {
		++r;
		if (*r == (char32_t)'l') {
			++r;
			if (*r = (char32_t)'.') is_ll = true;
		}
	}

	if (is_c) {
		fname[r.get_index()]=0;
		

		LPTSTR utarget = UTF8ToUnicode(fname.cpp_str().c_str());

		message << filename << " parse as C or C++ '" << t << "'\n";
		std::ostringstream command;
		

		LPTSTR cl = UTF8ToUnicode(filename);
		wchar_t params[2048] = L"/C clang -S -emit-llvm -O2 -fno-discard-value-names -fno-strict-overflow -fno-strict-aliasing -Xclang -disable-lifetime-markers -Xclang -disable-llvm-optzns -mllvm -opaque-pointers ";
		wcsncat(params, cl, 2048 - wcslen(params));
		wcsncat(params, L" -o ", 2048- wcslen(params));
		wcsncat(params, utarget, 2048 - wcslen(params));
		wcsncat(params, L".ll", 2048 - wcslen(params));
		//wcsncat(params, path, 2048 - wcslen(params));
		//wcsncat(params, L"error_output.txt", 2048 - wcslen(params));

		os_execconsole(path, L"cmd", params);
		
		wcsncpy(params, path, 2048);
		wcsncat(params, L"stderr.txt", 2048 - wcslen(params));


		FILE* f = _wfopen(params, L"r");
		if (f) {
			_fseeki64(f, 0, SEEK_END);

			ULONGLONG len = _ftelli64(f);
			LPSTR buffer1 = new CHAR[(int)len + 1];
			_fseeki64(f, 0, SEEK_SET);
			len=fread(buffer1, 1, (int)len, f);
			buffer1[(int)len] = 0;
			fclose(f);

			LPTSTR tbuffer = UTF8ToUnicodeAndTranslateLF(buffer1);

			m_wndOutputBuild.append_text(tbuffer);
			delete[] tbuffer;
			delete[] buffer1;
		}
		wcsncpy(params, utarget, 2048);
		wcsncat(params, L".ll", 2048 - wcslen(params));


		theApp.CloseDocumentByTitle(params);

		//ShellExecuteW(NULL,L"open", L"\"C:\\Program Files\\LLVM\\bin\\clang\"", params,path,0);
		delete[] cl;
		wcsncpy(params, path, 2048);
		wcsncat(params, utarget, 2048 - wcslen(params));
		wcsncat(params, L".ll", 2048 - wcslen(params));
		theApp.OpenDocumentFile(params);
		delete[] utarget;
	}else if (is_ll){
	}
	else {
		static bool intialized = false;
		if (!intialized) {
			init_parser();
			intialized = true;
		}

		message << filename << " parse '" << mainish(t) << "'\n";

		LPTSTR tbuffer = UTF8ToUnicodeAndTranslateLF(message.str().c_str());

		m_wndOutputBuild.append_text(tbuffer);
		delete[] tbuffer;
	}
	//size_t len = _tcslen(t);
/*	size_t len = strlen(t);

	LPTSTR a = &chars[0];
	for (int i = 0;i < len;++i) {
		//a[0] = digits[(t[i] >> 12) & 15];
		//a[1] = digits[(t[i] >> 8) & 15];
		a[0] = digits[(t[i] >> 4) & 15];
		a[1] = digits[t[i] & 15];
		a += 2;
	}
	a[0] = 0x0d;
	a[1] = 0x0a;
	a[2] = 0;
	a[3] = 0;
*/
//std::stringstream message;
//message << "parse '" << t << "'\n";
//LPTSTR tbuffer = UTF8ToUnicodeAndTranslateLF(message.str().c_str());

//m_wndOutputBuild.append_text(tbuffer);
//delete[] tbuffer;

//	init_parser();
	
//	parse(filename, t);
//	if (windowNumber >= 0) { message << "In window #" << windowNumber << " "; }
//	int len = strlen(t);
//	for (int i = len - 1; i >= 0; --i) message << t[i];
//	try {
//		LVar tokens(tokenize(filename, t));
//	//	message << tokens << endl;
//		message << eval_progn(tokens,base_env) << endl;
//	}
//	catch (token_error &e) {
//		message << e.what() << endl;
//	}
//	catch (std::runtime_error &e) {
//		message << e.what() << endl;
//	}
	//LPTSTR tbuffer = UTF8ToUnicodeAndTranslateLF(message.str().c_str());

	//m_wndOutputBuild.append_text(tbuffer);
	//delete[] tbuffer;
}


void COutputWnd::FillBuildWindow()
{
	m_wndOutputBuild.SetTabStops(9);
//	m_wndOutputBuild.append_text(_T("Output is being displayed here.\r\n"));
//	m_wndOutputBuild.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputBuild.AddString(_T("but you can change the way it is displayed as you wish..."));
}

//void COutputWnd::FillDebugWindow()
//{
//	m_wndOutputDebug.AddString(_T("Debug output is being displayed here."));
//	m_wndOutputDebug.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputDebug.AddString(_T("but you can change the way it is displayed as you wish..."));
//}

//void COutputWnd::FillFindWindow()
//{
//	m_wndOutputFind.AddString(_T("Find output is being displayed here."));
//	m_wndOutputFind.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputFind.AddString(_T("but you can change the way it is displayed as you wish..."));
//}

void COutputWnd::UpdateFonts()
{
	m_wndOutputBuild.SetFont(&afxGlobalData.fontRegular);
//	m_wndOutputDebug.SetFont(&afxGlobalData.fontRegular);
//	m_wndOutputFind.SetFont(&afxGlobalData.fontRegular);
}

/////////////////////////////////////////////////////////////////////////////
// COutputList1

COutputList::COutputList()
	:end_of_last_output(0)
{
}

COutputList::~COutputList()
{
}

BEGIN_MESSAGE_MAP(COutputList, CEdit)
	ON_WM_CONTEXTMENU()
	ON_WM_CHAR()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
	ON_WM_WINDOWPOSCHANGING()
	ON_COMMAND(ID_EDIT_CUT, &COutputList::OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, &COutputList::OnEditPaste)
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// COutputList message handlers

void COutputList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_OUTPUT_POPUP);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void COutputList::OnEditCopy()
{
	//MessageBox(_T("Copy output"));
	CEdit::Copy();
}

void COutputList::OnEditClear()
{
	int sellen = GetWindowTextLength();
	SetSel(0, sellen);
	Clear();
}

void COutputList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != NULL && pParentBar != NULL)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();

	}
}


void COutputList::OnEditCut()
{
	CEdit::Cut();
}


void COutputList::OnEditPaste()
{
	CEdit::Paste();
}
