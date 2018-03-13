#include<winsock2.h>
#include "MainWnd.h"
#include "source.h"
#include <algorithm>
#include <time.h>
#include <comutil.h>
#include <process.h> 
#include <commdlg.h>
#include "MenuWnd.h"
#include <winapifamily.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "comsupp.lib")


#define WM_USER_PLAYING         WM_USER + 1// 开始播放文件
#define WM_USER_POS_CHANGED     WM_USER + 2// 文件播放位置改变
#define WM_USER_END_REACHED     WM_USER + 3// 播放完毕

//文件类型
const TCHAR STR_FILE_FILTER[] =
_T("All Files(*.*)\0*.*\0")
_T("Movie Files(*.rmvb,*.mpeg,etc)\0*.rm;*.rmvb;*.flv;*.f4v;*.avi;*.3gp;*.mp4;*.wmv;*.mpeg;*.mpga;*.asf;*.dat;*.mov;*.dv;*.mkv;*.mpg;*.trp;*.ts;*.vob;*.xv;*.m4v;*.dpg;\0");

// 查找 
const TCHAR STR_FILE_MOVIE[] =
_T("Movie Files(*.rmvb,*.mpeg,etc)|*.rm;*.rmvb;*.flv;*.f4v;*.avi;*.3gp;*.mp4;*.wmv;*.mpeg;*.mpga;*.asf;*.dat;*.mov;*.dv;*.mkv;*.mpg;*.trp;*.ts;*.vob;*.xv;*.m4v;*.dpg;|");

// TreeView控件播放列表的文件路径节点
const UINT_PTR U_TAG_PLAYLIST = 1; 

std::string UnicodeConvert(const std::wstring& strWide, UINT uCodePage)
{
	std::string strANSI;
	int iLen = ::WideCharToMultiByte(uCodePage, 0, strWide.c_str(), -1, NULL, 0, NULL, NULL);

	if (iLen > 1)
	{
		strANSI.resize(iLen - 1);
		::WideCharToMultiByte(uCodePage, 0, strWide.c_str(), -1, &strANSI[0], iLen, NULL, NULL);
	}

	return strANSI;
}

std::string UnicodeToUTF8(const std::wstring& strWide)
{
	return UnicodeConvert(strWide, CP_UTF8);
}

bool FindFile(LPCTSTR pstrPath, LPCTSTR pstrExtFilter)
{
    if (! pstrPath || ! pstrExtFilter)
    {
        return false;
    }
    TCHAR szExt[_MAX_EXT] = _T("");

    _tsplitpath_s(pstrPath, NULL, 0, NULL, 0, NULL, 0, szExt, _MAX_EXT);
    _tcslwr_s(szExt, _MAX_EXT);
    if(_tcslen(szExt))  
    {
        _tcscat_s(szExt, _MAX_EXT, _T(";"));//加上;判断是否匹配
        return NULL != _tcsstr(pstrExtFilter, szExt);
    }
    return false;
}

bool Movie(LPCTSTR pstrPath)
{
    return FindFile(pstrPath, STR_FILE_MOVIE);
}

bool WantedFile(LPCTSTR pstrPath)
{
    return Movie(pstrPath);
}

void CbPlayer(void *data, UINT uMsg)
{
    CAVPlayer *pAVPlayer = (CAVPlayer *) data;

    if (pAVPlayer)
    {
        HWND hWnd = pAVPlayer->GetHWND();

        if (::IsWindow(hWnd) && ::IsWindow(::GetParent(hWnd)))
        {
            ::PostMessage(::GetParent(hWnd), uMsg, (WPARAM)data, 0);
        }
    }
}

void CbPlaying(void *data)
{
	CbPlayer(data, WM_USER_PLAYING);
}

void CbPosChanged(void *data)
{
	CbPlayer(data, WM_USER_POS_CHANGED);
}

void CbEndReached(void *data)
{
	CbPlayer(data, WM_USER_END_REACHED);
}

CDuiFrameWnd::CDuiFrameWnd( LPCTSTR pszXMLName )
: CXMLWnd(pszXMLName),
m_Slider(NULL),
m_FullScreen(FALSE)
{
}

CDuiFrameWnd::~CDuiFrameWnd()
{
}

SOCKET serSocket; //建立服务器
sockaddr_in serAddr; //服务器地址
sockaddr_in remoteAddr; //远程控制客户端地址
DWORD WINAPI CDuiFrameWnd::ThreadProc(LPVOID lpParameter)
{
	
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		closesocket(serSocket);
	}
	
	int nAddrLen = sizeof(remoteAddr);
	char recvData[255];
	int ret = recvfrom(serSocket, recvData, 255, 0, (sockaddr*)&remoteAddr, &nAddrLen);
	char * sendData = "一个来自服务端的UDP数据包";
	sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
	
	//closesocket(serSocket);//单击连接后按钮禁灰，不关闭套接字
	return 0;
}


DUI_BEGIN_MESSAGE_MAP(CDuiFrameWnd, CNotifyPump)
DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK,OnClick)
DUI_END_MESSAGE_MAP()

void CDuiFrameWnd::InitWindow()
{
    SetIcon(IDI_ICON1);
    // 根据分辨率自动调节窗口大小
    MONITORINFO myMonitor = {};
	myMonitor.cbSize = sizeof(myMonitor);
    ::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTONEAREST), &myMonitor);
    AdaptWindowSize(myMonitor.rcMonitor.right - myMonitor.rcMonitor.left);
    ::GetWindowPlacement(*this, &m_OldWndPlacement);

    // 初始化CActiveXUI控件
    std::vector<CDuiString> vctName;
    CActiveXUI* pActiveXUI;

    for (UINT i = 0; i < vctName.size(); i++)
    {
        pActiveXUI = static_cast<CActiveXUI*>(m_PaintManager.FindControl(vctName[i]));

        if(pActiveXUI) 
        {
            pActiveXUI->SetDelayCreate(false);
            pActiveXUI->CreateControl(CLSID_WebBrowser);
        }
    }

    // 几个常用控件做为成员变量
    CSliderUI* pSilderVol = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("sliderVol")));
    m_Slider = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("sliderPlay")));
	m_VideoTime = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("labelPlayTime")));

    if (! pSilderVol || !m_Slider || !m_VideoTime)
    {
        return;
    }

    pSilderVol->OnNotify    += MakeDelegate(this, &CDuiFrameWnd::OnVolumeChanged);
	m_Slider->OnNotify += MakeDelegate(this, &CDuiFrameWnd::OnPosChanged);

    // 设置播放器的窗口句柄和回调函数
    CWndUI *pWnd = static_cast<CWndUI*>(m_PaintManager.FindControl(_T("wndMedia")));
    if (pWnd)
    {
        m_cAVPlayer.SetHWND(pWnd->GetHWND()); 
        m_cAVPlayer.SetCbPlaying(CbPlaying);
        m_cAVPlayer.SetCbPosChanged(CbPosChanged);
        m_cAVPlayer.SetCbEndReached(CbEndReached);
    }
	m_cAVPlayer.Play("test1.avi");//添加此处预加载通信部分dll ??? 有待改正 ???
}

CControlUI* CDuiFrameWnd::CreateControl( LPCTSTR pstrClassName )
{
    CDuiString XML;
    CDialogBuilder builder;
    if (_tcsicmp(pstrClassName, _T("Caption")) == 0)
    {
		XML = _T("Caption.xml");
    }
    else if (_tcsicmp(pstrClassName, _T("PlayPanel")) == 0)
    {
		XML = _T("PlayPanel.xml");
    }
    else if (_tcsicmp(pstrClassName, _T("Playlist")) == 0)
    {
		XML = _T("Playlist.xml");
    }
    else if (_tcsicmp(pstrClassName, _T("WndMediaDisplay")) == 0)
    {
        CWndUI *pUI = new CWndUI;   
        HWND   hWnd = CreateWindow(_T("#32770"), _T("WndMediaDisplay"), WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_PaintManager.GetPaintWindow(), (HMENU)0, NULL, NULL);
        pUI->Attach(hWnd);  
        return pUI;
    }

    if (!XML.IsEmpty())
    {
        CControlUI* pUI = builder.Create(XML.GetData(), NULL, NULL, &m_PaintManager, NULL);
		return pUI;
    }
    return NULL;
}

void CDuiFrameWnd::OnClick( TNotifyUI& msg )
{
    if( msg.pSender->GetName() == _T("btnPlaylistShow") ) 
    {
        ShowPlaylist(true);
    }
    else if( msg.pSender->GetName() == _T("btnPlaylistHide") ) 
    {
        ShowPlaylist(false);
    }
	else if (msg.pSender->GetName() == _T("btnGetConnect"))
	{
		CControlUI *pbtnGetConnect = m_PaintManager.FindControl(_T("btnGetConnect"));
		pbtnGetConnect->SetEnabled(0);
		hThread = ::CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
	}
	else if (msg.pSender->GetName() == _T("btnCloseConnect"))
	{
		closesocket(serSocket);
		CControlUI *pbtnGetConnect = m_PaintManager.FindControl(_T("btnGetConnect"));
		pbtnGetConnect->SetEnabled(-1);
	}
	else if (msg.pSender->GetName() == _T("btnOpen") || msg.pSender->GetName() == _T("btnOpenMini"))
	{
		OpenFileDialog();
	}
    else if( msg.pSender->GetName() == _T("btnPlay") ) 
    {
		char * sendData = "2";
		int nAddrLen = sizeof(remoteAddr);
		sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
        Play(true);
    }
    else if( msg.pSender->GetName() == _T("btnPause") ) 
    {
		char * sendData = "3";
		int nAddrLen = sizeof(remoteAddr);
		sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
        Play(false);
    }
    else if( msg.pSender->GetName() == _T("btnStop") ) 
    {
        Stop();
    }
	else if (msg.pSender->GetName() == _T("btnScreenNormal"))
	{
		FullScreen(false);
	}
    else if( msg.pSender->GetName() == _T("btnScreenFull") ) 
    {
        FullScreen(true);
    }
	else if (msg.pSender->GetName() == _T("btnVolume"))
	{
		m_cAVPlayer.Volume(0);
		m_PaintManager.FindControl(_T("btnVolumeZero"))->SetVisible(true);
		msg.pSender->SetVisible(false);
	}
	else if (msg.pSender->GetName() == _T("btnVolumeZero"))
	{
		CSliderUI* pUI = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("sliderVol")));
		m_cAVPlayer.Volume(pUI->GetValue());
		m_PaintManager.FindControl(_T("btnVolume"))->SetVisible(true);
		msg.pSender->SetVisible(false);
	}

    __super::OnClick(msg);
}

void CDuiFrameWnd::Notify( TNotifyUI& msg )
{
    if( msg.sType == DUI_MSGTYPE_DBCLICK)
    {
		if (IsInStaticControl(msg.pSender))
		{
			if (!msg.pSender->GetParent())
			{
				FullScreen(!m_FullScreen);
			}
		}
    }
    __super::Notify(msg);
}

LRESULT CDuiFrameWnd::HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LRESULT lRes = __super::HandleMessage(uMsg, wParam, lParam);

    switch (uMsg)
    {
        HANDLE_MSG (*this, WM_DISPLAYCHANGE, OnDisplayChange);
        HANDLE_MSG (*this, WM_GETMINMAXINFO, OnGetMinMaxInfo);

    case WM_USER_PLAYING:
        return OnPlaying(*this, wParam, lParam);
    case WM_USER_POS_CHANGED:
        return OnPosChanged(*this, wParam, lParam);
    case WM_USER_END_REACHED:
        return OnEndReached(*this, wParam, lParam);       
    }

    return lRes;
}

LRESULT CDuiFrameWnd::OnNcHitTest( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if (m_FullScreen)
    {
        return HTCLIENT;
    }
    return __super::OnNcHitTest(uMsg, wParam, lParam, bHandled);
}

LRESULT CDuiFrameWnd::ResponseDefaultKeyEvent(WPARAM wParam)
{
    if (wParam == VK_ESCAPE)
    {
        if (m_FullScreen)
        {
            FullScreen(false);
        }
    }
    return __super::ResponseDefaultKeyEvent(wParam);
}

void CDuiFrameWnd::AdaptWindowSize( UINT cxScreen )
{
    int X = 968, Y = 600;
    int WidthList = 236, WidthSearchEdit = 193;
    SIZE FixSearchBtn = {201, 0};
    if(cxScreen <= 1024)      // 800*600  1024*768  
    {
        X = 765;
        Y = 460;
    } 
    else if(cxScreen <= 1280) // 1152*864  1280*800  1280*960  1280*1024
    {
        X = 968;
        Y = 600;
    }
    else if(cxScreen <= 1366) // 1360*768 1366*768
    {
        X = 1028;
        Y = 626;
        WidthList        += 21;
        WidthSearchEdit  += 21;
        FixSearchBtn.cx += 21;
    }
    else                      // 1440*900
    {
        X = 1200;
        Y = 720;
        WidthList        += 66;
        WidthSearchEdit  += 66;
        FixSearchBtn.cx += 66;
    }
    CControlUI *pctnPlaylist = m_PaintManager.FindControl(_T("ctnPlaylist"));
    CControlUI *peditSearch  = m_PaintManager.FindControl(_T("editSearch"));
    CControlUI *pbtnSearch   = m_PaintManager.FindControl(_T("btnSearch"));
    if (pctnPlaylist && peditSearch && pbtnSearch)
    {
		pctnPlaylist->SetFixedWidth(WidthList);
		peditSearch->SetFixedWidth(WidthSearchEdit);
		pbtnSearch->SetFixedXY(FixSearchBtn);
    }
	::SetWindowPos(m_PaintManager.GetPaintWindow(), NULL, 0, 0, X, Y, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
	CenterWindow();
}

void CDuiFrameWnd::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	if (m_FullScreen)
	{
		lpMinMaxInfo->ptMaxTrackSize.x = GetSystemMetrics(SM_CXSCREEN) + 2 * (GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CXBORDER));
		lpMinMaxInfo->ptMaxTrackSize.y = GetSystemMetrics(SM_CYSCREEN) + 2 * (GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYBORDER));
	}
}

void CDuiFrameWnd::OnDisplayChange( HWND hwnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen )
{
    AdaptWindowSize(cxScreen);
}

void CDuiFrameWnd::ShowPlayButton(bool bShow)
{
	CControlUI *pbtnPlay = m_PaintManager.FindControl(_T("btnPlay"));
	CControlUI *pbtnPause = m_PaintManager.FindControl(_T("btnPause"));
	if (pbtnPlay && pbtnPause)
	{
		pbtnPlay->SetVisible(bShow);
		pbtnPause->SetVisible(!bShow);
	}
}

void CDuiFrameWnd::ShowPlaylist( bool bShow )
{
    CControlUI *pctnPlaylist = m_PaintManager.FindControl(_T("ctnPlaylist"));
    CControlUI *pbtnHide     = m_PaintManager.FindControl(_T("btnPlaylistHide"));
    CControlUI *pbtnShow     = m_PaintManager.FindControl(_T("btnPlaylistShow"));
    if (pctnPlaylist && pbtnHide && pbtnShow)
    {
        pctnPlaylist->SetVisible(bShow);
        pbtnHide->SetVisible(bShow);
        pbtnShow->SetVisible(! bShow);
    }
}

void CDuiFrameWnd::ShowPlayWnd( bool bShow )
{
    CControlUI *pbtnWnd     = m_PaintManager.FindControl(_T("wndMedia"));
    CControlUI *pbtnStop    = m_PaintManager.FindControl(_T("btnStop"));
    CControlUI *pbtnScreen  = m_PaintManager.FindControl(_T("btnScreenFull"));
    CControlUI *pctnClient  = m_PaintManager.FindControl(_T("ctnClient"));
    CControlUI *pctnMusic   = m_PaintManager.FindControl(_T("ctnMusic"));
    CControlUI *pctnSlider  = m_PaintManager.FindControl(_T("ctnSlider"));

    if (pbtnWnd && pbtnStop && pbtnScreen && pctnClient && pctnMusic && pctnSlider)
    {
        pbtnStop->SetEnabled(bShow);
        pbtnScreen->SetEnabled(bShow);
        pctnClient->SetVisible(! bShow);
        pctnSlider->SetVisible(bShow);
        // 打开文件时
        if (bShow)
        {
            pbtnWnd->SetVisible(bShow);
            pctnMusic->SetVisible(! bShow);
        }
        // 关闭文件时
        else  
        {
            pctnMusic->SetVisible(false);
            pbtnWnd->SetVisible(false);
        }
    }
}

void CDuiFrameWnd::ShowConnectButton(bool bShow)
{
	CControlUI *pbtnGetConnect = m_PaintManager.FindControl(_T("btnGetConnect"));
	CControlUI *pbtnCloseConnect = m_PaintManager.FindControl(_T("btnCloseConnect"));
	if (pbtnGetConnect && pbtnCloseConnect)
	{
		pbtnGetConnect->SetVisible(bShow);
		pbtnCloseConnect->SetVisible(bShow);
	}
}

void CDuiFrameWnd::ShowControlsForPlay( bool bShow )
{
	m_VideoTime->SetText(_T(""));
    ShowPlayWnd(bShow);
    ShowPlaylist(! bShow);
}

void CDuiFrameWnd::OpenFileDialog()
{
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH] = _T("");
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = *this;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = STR_FILE_FILTER;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{
		std::vector<string_t> vctString(1, szFile);
		AddFile(vctString);
	}
}

void CDuiFrameWnd::AddFile( const std::vector<string_t> &vctString)
{
    CTreeNodeUI *pNodeTmp, *pNodePlaylist;
    CDuiString  strTmp;
    TCHAR       szName[_MAX_FNAME];
    TCHAR       szExt[_MAX_EXT];
    unsigned    i, uWantedCount;
    pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodePlaylist")));
    if (! pNodePlaylist)
    {
        return;
    }
    for(i = 0, uWantedCount = 0; i < vctString.size(); i++)
    {
        if (WantedFile(vctString[i].c_str()))
        {
            _tsplitpath_s(vctString[i].c_str(), NULL, 0, NULL, 0, szName, _MAX_FNAME, szExt, _MAX_EXT);
            strTmp.Format(_T("%s%s"), szName, szExt);   // 文件名
            uWantedCount++;
        }        
    }
	Play(strTmp);//第一次打开显示时候不应该加入
}

void CDuiFrameWnd::Play( bool bPlay )
{
    if (m_cAVPlayer.IsOpen())
    {
        ShowPlayButton(! bPlay);
        if (bPlay)
        {
            m_cAVPlayer.Play();
        } 
        else
        {
            m_cAVPlayer.Pause();
        }
    }
}

void CDuiFrameWnd::Play( LPCTSTR pszPath )
{
    if (! pszPath)
    {
        return;
    }
    m_strPath = pszPath;
    if (m_cAVPlayer.Play(UnicodeToUTF8(pszPath)))
    {
        ShowControlsForPlay(true);
    }
}

void CDuiFrameWnd::Stop()
{
	m_cAVPlayer.Stop();
	ShowControlsForPlay(false);
	CControlUI *pbtnPlay = m_PaintManager.FindControl(_T("btnPlay"));
	CControlUI *pbtnPause = m_PaintManager.FindControl(_T("btnPause"));
	pbtnPlay->SetVisible(true);
	pbtnPause->SetVisible(false);
	// 进度条归零问题，目前停止过后进度条停在原先位置，现在的方案：在Stop()函数内部加入回调，让POS位置归零，问题在于归零需要将nPosChanged
	// 参数传回，需要在cAVPlayer.cpp中重新加入，加入区域应在初始化阶段，即为打开文件期间进行的媒体初始化。
}

void CDuiFrameWnd::FullScreen( bool bFull )
{
    CControlUI* pbtnFull   = m_PaintManager.FindControl(_T("btnScreenFull"));
    CControlUI* pbtnNormal = m_PaintManager.FindControl(_T("btnScreenNormal"));
    CControlUI* pUICaption = m_PaintManager.FindControl(_T("ctnCaption"));
    int iBorderX = GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CXBORDER);
    int iBorderY = GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYBORDER);

    if (pbtnFull && pbtnNormal && pUICaption)
    {
		m_FullScreen = bFull;

        if (bFull)
        {
            ::GetWindowPlacement(*this, &m_OldWndPlacement);
            if (::IsZoomed(*this))
            {
                ::ShowWindow(*this, SW_SHOWDEFAULT);
            }
            ::SetWindowPos(*this, HWND_TOPMOST, -iBorderX, -iBorderY, GetSystemMetrics(SM_CXSCREEN) + 2 * iBorderX, GetSystemMetrics(SM_CYSCREEN) + 2 * iBorderY, 0);
            ShowPlaylist(false);
        } 
        else
        {
            ::SetWindowPlacement(*this, &m_OldWndPlacement);
            ::SetWindowPos(*this, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        }

        pbtnNormal->SetVisible(bFull);
        pUICaption->SetVisible(! bFull);
        pbtnFull->SetVisible(! bFull);
    }
}

LRESULT CDuiFrameWnd::OnPlaying(HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	return TRUE;
}

LRESULT CDuiFrameWnd::OnPosChanged(HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    CDuiString  strTime;
    struct tm   tmTotal, tmCurrent;
	TCHAR       szTotal[MAX_PATH], szCurrent[MAX_PATH];
	//m_cAVPlayer.Refresh();
    time_t      timeCurrent = m_cAVPlayer.GetTime() / 1000;
	time_t      timeTotal = m_cAVPlayer.GetTotalTime() / 1000;
    gmtime_s(&tmTotal, &timeTotal);
    gmtime_s(&tmCurrent, &timeCurrent);
    _tcsftime(szTotal,   MAX_PATH, _T("%X"), &tmTotal);
    _tcsftime(szCurrent, MAX_PATH, _T("%X"), &tmCurrent);
    strTime.Format(_T("%s / %s"), szCurrent, szTotal);
	m_VideoTime->SetText(strTime);
	m_Slider->SetValue(m_cAVPlayer.GetPos());
    return TRUE;
}

LRESULT CDuiFrameWnd::OnEndReached(HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	Stop();
    return TRUE;
}

bool CDuiFrameWnd::OnPosChanged(void* param)
{
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == _T("valuechanged"))
	{
		m_cAVPlayer.SeekTo((static_cast<CSliderUI*>(pMsg->pSender))->GetValue() + 1); // 获取的值少了1，导致设置的值也少了1，所以这里+1
	}

	return true;
}

bool CDuiFrameWnd::OnVolumeChanged( void* param )
{
    TNotifyUI* pMsg = (TNotifyUI*)param;
    if( pMsg->sType == _T("valuechanged") )
    {
        m_cAVPlayer.Volume((static_cast<CSliderUI*>(pMsg->pSender))->GetValue());
    }

    return true;
}
