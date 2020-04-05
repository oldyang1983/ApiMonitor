﻿
// ApiMonitorUIDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "ApiMonitorUI.h"
#include "ApiMonitorUIDlg.h"
#include "afxdialogex.h"
#include "ApiMonitor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ID_REFRESH_API_CALL_LOG_TIMER 1
#define WM_TREE_ADD_MODULE WM_USER+100  

void Reply(const uint8_t *readData, uint32_t readDataSize, uint8_t *writeData, uint32_t *writeDataSize, const uint32_t maxWriteBuffer, void* userData);


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CApiMonitorUIDlg 对话框


CApiMonitorUIDlg::CApiMonitorUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_APIMONITORUI_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CApiMonitorUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDE_FILE_PATH, m_editFilePath);
    DDX_Control(pDX, IDC_TREE_MODULES, m_treeModuleList);
    DDX_Control(pDX, IDC_LIST_API_LOGS, m_listApiCalls);
}



std::wstring CApiMonitorUIDlg::ToWString(const std::string & str)
{
    int sz = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
    std::vector<wchar_t> vec(sz + 1);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, vec.data(), sz);
    return std::wstring(vec.data());
}

CString CApiMonitorUIDlg::ToCString(const std::string & str)
{
#ifdef UNICODE
    CString cs;
    cs.GetBuffer(str.size() * 2);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, cs.GetBuffer(), str.size() * 2);
    return cs;
#else
    return str.c_str();
#endif
}

CString CApiMonitorUIDlg::ToCString(long long i, bool hex)
{
    CString cs;
    cs.Format((hex ? _T("0x%llx") : _T("%lld")), i);
    return cs;
}

void CApiMonitorUIDlg::UpdateModuleList(void* pv)
{
    SendMessage(WM_TREE_ADD_MODULE, (WPARAM)pv, 0);
}

void CApiMonitorUIDlg::AppendApiCallLog(void * pv)
{
    ApiLogItem* al = reinterpret_cast<ApiLogItem*>(pv);
    al->mApiNameW = ToWString(al->mApiName);
    al->mModuleNameW = ToWString(al->mModuleName);
    m_ApiLogs.push_back(*al);
}

BEGIN_MESSAGE_MAP(CApiMonitorUIDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CApiMonitorUIDlg::OnBnClickedButton1)
    ON_MESSAGE(WM_TREE_ADD_MODULE, &CApiMonitorUIDlg::OnTreeListAddModule)
    ON_WM_TIMER()
    ON_WM_SIZE()
END_MESSAGE_MAP()


// CApiMonitorUIDlg 消息处理程序

BOOL CApiMonitorUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

    m_treeModuleList.ModifyStyle(NULL, TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS);

    SetTimer(ID_REFRESH_API_CALL_LOG_TIMER, 1000, NULL);

    m_listApiCalls.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listApiCalls.InsertColumn(0, _T("No."),     LVCFMT_LEFT, 50,  -1);
    m_listApiCalls.InsertColumn(1, _T("TID"),     LVCFMT_LEFT, 50,  -1);
    m_listApiCalls.InsertColumn(2, _T("RetAddr"), LVCFMT_LEFT, 70,  -1);
    m_listApiCalls.InsertColumn(3, _T("Module"),  LVCFMT_LEFT, 120, -1);
    m_listApiCalls.InsertColumn(4, _T("Name"),    LVCFMT_LEFT, 150, -1);
    m_listApiCalls.InsertColumn(5, _T("Count"),   LVCFMT_LEFT, 50,  -1);
    //HDITEM hdi;
    //hdi.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT | HDI_IMAGE;
    //hdi.fmt = HDF_STRING | HDF_CENTER;
    //TCHAR* colums[6] = { _T("No."), _T("TID"), _T("RetAddr"),_T("Module"), _T("Name"), _T("Count") };
    //int width[6] = { 50,50,70,120,150,50 };
    //for (int i = 0; i < 6; i++)
    //{
    //    hdi.cxy = width[i];
    //    hdi.iImage = i % 3;
    //    hdi.pszText = colums[i];
    //    m_listApiCalls.GetHeaderCtrl()->InsertItem(i, &hdi);
    //}
    m_listApiCalls.Invalidate();

    m_Controller = new PipeController();
    m_Controller->mMsgHandler = Reply;
    m_Controller->mUserData = this;

    m_Monitor = new Monitor();
    m_Monitor->SetPipeHandler(m_Controller);

    m_editFilePath.SetWindowText(L"C:\\Projects\\ApiMonitor\\bin\\Win32\\Release\\TestExe.exe");
    UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CApiMonitorUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CApiMonitorUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CApiMonitorUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void Reply(const uint8_t *readData, uint32_t readDataSize, uint8_t *writeData, uint32_t *writeDataSize, const uint32_t maxWriteBuffer, void* userData)
{
    CApiMonitorUIDlg* dlg = (CApiMonitorUIDlg*)userData;
    PipeController* pc = (PipeController*)dlg->m_Controller;

    printf("data arrive. size=%d\n", readDataSize);
    if (readDataSize < sizeof(PipeDefine::PipeMsg) + sizeof(size_t))
    {
        // 过短消息
        printf("too short.");
        return;
    }

    PipeDefine::Message* msg = (PipeDefine::Message*)readData;
    switch (msg->type)
    {
    case PipeDefine::Pipe_C_Req_Inited: {
        PipeDefine::msg::Init m;
        std::vector<char, Allocator::allocator<char>> str(msg->Content, msg->Content + msg->ContentSize);
        m.Unserial(str);
        m.dummy += 1;
        str = m.Serial();
        PipeDefine::Message* msg2 = (PipeDefine::Message*)writeData;
        msg2->type = PipeDefine::Pipe_S_Ack_Inited;
        msg2->tid = msg->tid;
        msg2->ContentSize = str.size();
        memcpy_s(msg2->Content, maxWriteBuffer, str.data(), str.size());
        *writeDataSize = msg2->HeaderLength + msg2->ContentSize;
        break;
    }
    case PipeDefine::Pipe_C_Req_ModuleApiList: {
        PipeDefine::msg::ModuleApis m;
        std::vector<char, Allocator::allocator<char>> str(msg->Content, msg->Content + msg->ContentSize);
        m.Unserial(str);
        ModuleInfoItem me;
        me.mName = m.module_name;
        me.mPath = m.module_path;
        me.mBase = m.module_base;
        //printf("module name: %s, base: %llx, path: %s\n", m.module_name.c_str(), m.module_base, m.module_path.c_str());
        for (size_t i = 0; i < m.apis.size(); ++i)
        {
            ModuleInfoItem::ApiEntry ae;
            ae.mName         = m.apis[i].name;
            ae.mVa           = m.apis[i].va;
            ae.mIsForward    = m.apis[i].forward_api;
            ae.mIsDataExport = m.apis[i].data_export;
            ae.mForwardto    = m.apis[i].forwardto;
            me.mApis.push_back(ae);
            //if (m.apis[i].forward_api)
            //    printf("  (%05u) name: %s, va: 0x%llx, rva: 0x%llx, dataExp: %s, forward-to: %s\n", i, m.apis[i].name.c_str(), m.apis[i].va, m.apis[i].rva,
            //    (m.apis[i].data_export ? "yes" : "no"), m.apis[i].forwardto.c_str());
            //else
            //    printf("  (%05u) name: %s, va: 0x%llx, rva: 0x%llx, dataExp: %s, forward: no\n", i, m.apis[i].name.c_str(), m.apis[i].va, m.apis[i].rva,
            //    (m.apis[i].data_export ? "yes" : "no"));
            if (!_stricmp(m.module_name.c_str(), "kernel32.dll") && m.apis[i].name == "OutputDebugStringA")
                pc->outputdbgstr = m.apis[i].va;
        }
        pc->mModuleApis.push_back(me);
        dlg->UpdateModuleList(&me);

        PipeDefine::msg::ApiFilter filter;
        filter.module_name = m.module_name;
        for (size_t i = 0; i < me.mApis.size(); ++i)
        {
            PipeDefine::msg::ApiFilter::Api filter_api;
            filter_api.api_name = me.mApis[i].mName;
            filter_api.filter = me.mApis[i].mIsHook;        // 由 UI 更新
            filter.apis.push_back(filter_api);
        }
        str = filter.Serial();
        PipeDefine::Message* msg2 = (PipeDefine::Message*)writeData;
        msg2->type = PipeDefine::Pipe_S_Ack_FilterApi;
        msg2->tid = msg->tid;
        msg2->ContentSize = str.size();
        memcpy_s(msg2->Content, maxWriteBuffer, str.data(), str.size());
        *writeDataSize = msg2->HeaderLength + msg2->ContentSize;
        break;
    }
    case PipeDefine::Pipe_C_Req_ApiInvoked: {
        PipeDefine::msg::ApiInvoked m;
        std::vector<char, Allocator::allocator<char>> str(msg->Content, msg->Content + msg->ContentSize);
        m.Unserial(str);
        ApiLogItem al;
        al.mModuleName = m.module_name;
        al.mApiName = m.api_name;
        al.mCallFrom = m.call_from;
        al.mTimes = m.times;
        al.mTid = msg->tid;
        dlg->AppendApiCallLog(&al);
        //printf("Api Invoked: %s, %s, tid: %d, call from: 0x%llx, time: %d\n", m.module_name.c_str(), m.api_name.c_str(), msg->tid, m.call_from, m.times);
        if (pc->mConditionReady)
        {
            pc->mConditionReady = false;
            auto msg = pc->Lock();
            str = msg->Serial();
            PipeDefine::Message* msg2 = (PipeDefine::Message*)writeData;
            msg2->type = PipeDefine::Pipe_S_Req_SetBreakCondition;
            msg2->tid = -1;
            msg2->ContentSize = str.size();
            memcpy_s(msg2->Content, maxWriteBuffer, str.data(), str.size());
            *writeDataSize = msg2->HeaderLength + msg2->ContentSize;
            pc->UnLock();
            printf("condition sent!\n");
        }
        break;
    }
    }
}


void CApiMonitorUIDlg::OnBnClickedButton1()
{
    CString path;
    m_editFilePath.GetWindowText(path);
    if (m_RunningMonitorThread.joinable())
    {
        AfxMessageBox(_T("Already running"));
        return;
    }

    m_RunningMonitorThread = std::thread([this, path]() {
        return m_Monitor->LoadFile(path.GetString());
    });
}

LRESULT CApiMonitorUIDlg::OnTreeListAddModule(WPARAM wParam, LPARAM lParam)
{
    ModuleInfoItem* mii = (ModuleInfoItem*)wParam;

    HTREEITEM hRoot = m_treeModuleList.GetRootItem();
    if (hRoot == NULL)
    {
        hRoot = m_treeModuleList.InsertItem(_T("Modules"));
        m_treeModuleList.Expand(hRoot, TVE_EXPAND);
    }

    HTREEITEM hMod = m_treeModuleList.InsertItem(ToCString(mii->mName), NULL, NULL, hRoot);
    for (size_t i = 0; i < mii->mApis.size(); ++i)
    {
        m_treeModuleList.InsertItem(ToCString(mii->mApis[i].mName), NULL, NULL, hMod);
    }

    m_treeModuleList.EnsureVisible(hMod);
    return 0;
}

void CApiMonitorUIDlg::OnTimer(UINT nIDEvent)
{
    if (nIDEvent != ID_REFRESH_API_CALL_LOG_TIMER)
        return;

    size_t totalCount = m_listApiCalls.GetItemCount();
    size_t count = m_ApiLogs.size();

    if (totalCount >= count)
        return;

    int idx = 0;
    for (size_t i = totalCount; i < count; ++i)
    {
        m_ApiLogs[i].mIndex = i;
        idx = m_listApiCalls.InsertItem(i, ToCString(m_ApiLogs[i].mIndex));
        m_listApiCalls.SetItem(idx, 1, LVIF_TEXT, ToCString(m_ApiLogs[i].mTid), 0, 0, 0, 0);
        m_listApiCalls.SetItem(idx, 2, LVIF_TEXT, ToCString(m_ApiLogs[i].mCallFrom, true), 0, 0, 0, 0);
        m_listApiCalls.SetItem(idx, 3, LVIF_TEXT, m_ApiLogs[i].mModuleNameW.c_str(), 0, 0, 0, 0);
        m_listApiCalls.SetItem(idx, 4, LVIF_TEXT, m_ApiLogs[i].mApiNameW.c_str(), 0, 0, 0, 0);
        m_listApiCalls.SetItem(idx, 5, LVIF_TEXT, ToCString(m_ApiLogs[i].mTimes), 0, 0, 0, 0);
    }
    m_listApiCalls.EnsureVisible(idx, TRUE);
}

void CApiMonitorUIDlg::OnSize(UINT nType, int cx, int cy)
{
    //UpdateData(TRUE);

    const int BORDER = 10;

    RECT rc;
    rc.top = 50;
    rc.left = BORDER;
    rc.right = cx / 3 - BORDER / 2;
    rc.bottom = cy - BORDER;
    m_treeModuleList.SetWindowPos(NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);

    rc.top = 50;
    rc.left = cx / 3 + BORDER / 2;
    rc.right = cx - BORDER;
    rc.bottom = cy - BORDER;
    m_listApiCalls.SetWindowPos(NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);

    //UpdateData(FALSE);
}