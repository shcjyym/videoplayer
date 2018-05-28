#include<winsock2.h>
#include <ws2tcpip.h>
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


const UINT_PTR TAG = 1;

#define WM_USER_PLAYING               WM_USER + 1// 正在播放文件
#define WM_USER_POS_CHANGED    WM_USER + 2// 文件播放位置改变
#define WM_USER_END_REACHED     WM_USER + 3// 播放完毕
#define WM_USER_ADD_IP                 WM_USER + 4// 通信连接后显示设备IP地址
#define WM_USER_SYN_TIME             WM_USER + 5// 同步时间数据反馈

//文件类型
const TCHAR STR_FILE_FILTER[] =
//_T("All Files(*.*)\0*.*\0")
_T("Movie Files(*.rmvb,*.mpeg,etc)|*.avi;*.mp4;*.mov;*.rmvb;*.flv;*.3gp;*.wmv;*.mpeg;*.mpga;*.mkv;*.mpg;*.ts;|");

// 查找
const TCHAR STR_FILE_MOVIE[] =
_T("Movie Files(*.rmvb,*.mpeg,etc)|*.avi;*.mp4;*.mov;*.rmvb;*.flv;*.3gp;*.wmv;*.mpeg;*.mpga;*.mkv;*.mpg;*.ts;|");

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
    return FindFile(pstrPath, STR_FILE_FILTER);
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

// 产生随机播放序列
void Rand(std::deque<unsigned int> &queRand, unsigned int uRandNum)
{
	unsigned Old = queRand.size();
	unsigned New = Old + uRandNum;
	queRand.resize(New);
	for (unsigned i = Old; i < New; i++)
	{
		queRand[i] = i;
	}
	for (unsigned i = Old; i < New; i++)
	{
		std::swap(queRand[i], queRand[rand() % New]);
	}
}

CDuiFrameWnd::CDuiFrameWnd( LPCTSTR pszXMLName )
: CXMLWnd(pszXMLName),
m_Slider(NULL),
m_FullScreen(FALSE),
m_playlistIndex(-1),
m_playMode(EM_PLAY_MODE_RANDOM)
{
}

CDuiFrameWnd::~CDuiFrameWnd()
{
}

SOCKET serSocket; // 建立服务器
sockaddr_in serAddr; // 服务器地址
//sockaddr_in remoteAddr; // 远程控制客户端地址
CDuiString address_ip; // 列表中显示内容
char recvData[255]; // 获取的数据内容
char buf[1000]; // 发送数据内容
int connect_num=1; // 连接数目
int synTime = 0; // 当前时间
float rate = 1;
bool flag = true;

struct Command
{
	uint16_t head;
	uint16_t type;
	uint16_t order;
	SYSTEMTIME time;
	uint16_t tail;
};

void Sendto(int head, int type, int order)
{
	Command Cmd;
	Cmd.head = 0x01;
	Cmd.type = 0x01;
	Cmd.order = 0x01;
	GetLocalTime(&Cmd.time);
	Cmd.tail = 0xf0;

	Cmd.head = head;
	Cmd.type = type;
	Cmd.order = order;

	char *p = buf;
	*((uint16_t*)p) = Cmd.head;
	p += sizeof(uint16_t);
	*((uint16_t*)p) = Cmd.type;
	p += sizeof(uint16_t);
	*((uint16_t*)p) = Cmd.order;
	p += sizeof(uint16_t);
	//获取尽可能接近的命令发送时间信息
	GetLocalTime(&Cmd.time);
	*((SYSTEMTIME*)p) = Cmd.time;
	p += sizeof(SYSTEMTIME);
	*((uint16_t*)p) = Cmd.tail;

	sendto(serSocket, buf, sizeof(buf), 0, (struct sockaddr*)&serAddr, sizeof(serAddr));
};

DWORD WINAPI CDuiFrameWnd::CmuThreadProc(LPVOID lpParameter)
{
	CDuiFrameWnd* pDlg;
	pDlg = (CDuiFrameWnd*)lpParameter;
	Command Cmd;
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	int n = 1;
	serSocket = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(serSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&n, sizeof(n));
	serAddr.sin_addr.S_un.S_addr = inet_addr("234.2.2.2");
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	/*if (bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		closesocket(serSocket);
	}
	int nAddrLen = sizeof(remoteAddr);
	int ret = recvfrom(serSocket, recvData, 255, 0, (sockaddr *)&remoteAddr, &nAddrLen);*/
	Sendto(1, 1, 8);
	/*char * sendData = "well connected";
	sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&serAddr, sizeof(sockaddr));
	if (recvData[0] == '1') {
		size_t len = strlen(recvData) + 1;
		size_t converted = 0;
		wchar_t *WStr;
		WStr = (wchar_t*)malloc(len * sizeof(wchar_t));
		mbstowcs_s(&converted, WStr, len, recvData, _TRUNCATE);
		//把char类型的recvData转化为wchar_t类型的WStr,否则显示为乱码
		address_ip.Format(_T("%s"), WStr);
		::PostMessage(pDlg->m_hWnd, WM_USER_ADD_IP, 0, 0);
	}*/
	//closesocket(serSocket);//单击连接后按钮禁灰，不关闭套接字
	return 0;
}

DWORD WINAPI CDuiFrameWnd::SynThread(LPVOID lpParameter)
{
	CDuiFrameWnd* pDlg;
	pDlg = (CDuiFrameWnd*)lpParameter;
	while (flag == true)
	{
		Sendto(1, 4, synTime);
		::PostMessage(pDlg->m_hWnd, WM_USER_SYN_TIME, 0, 0);

		DWORD dwStart = GetTickCount();
		DWORD dwEnd = dwStart;
		do
		{
			dwEnd = GetTickCount() - dwStart;
		} while (dwEnd < 3000);
	}

	return 0;
}


DUI_BEGIN_MESSAGE_MAP(CDuiFrameWnd, CNotifyPump)
      DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK,OnClick)
DUI_END_MESSAGE_MAP()

void CDuiFrameWnd::InitWindow()
{
	SetIcon(IDI_ICON1);

	// 根据分辨率自动调节窗口大小
	MONITORINFO Monitor = {};
	Monitor.cbSize = sizeof(Monitor);
	::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTONEAREST), &Monitor);
	AdaptWindowSize(Monitor.rcMonitor.right - Monitor.rcMonitor.left);
	::GetWindowPlacement(*this, &m_OldWndPlacement);

	// 初始化CActiveXUI控件
	std::vector<CDuiString> vctName;
	CActiveXUI* pActiveXUI;
	vctName.push_back(_T("ActiveXWeb"));
	for (UINT i = 0; i < vctName.size(); i++)
	{
		pActiveXUI = static_cast<CActiveXUI*>(m_PaintManager.FindControl(vctName[i]));
		if (pActiveXUI)
		{
			pActiveXUI->SetDelayCreate(false);
			pActiveXUI->CreateControl(CLSID_WebBrowser);
		}
	}

	// 几个常用控件做为成员变量
	CSliderUI* pSilderVol = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("sliderVol")));
	m_Slider = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("sliderPlay")));
	m_VideoTime = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("labelPlayTime")));

	if (!pSilderVol || !m_Slider || !m_VideoTime)
	{
		return;
	}

	pSilderVol->OnNotify += MakeDelegate(this, &CDuiFrameWnd::OnVolumeChanged);
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

	CControlUI *pbtnCloseConnect = m_PaintManager.FindControl(_T("btnCloseConnect"));
	pbtnCloseConnect->SetEnabled(0);// 初始化关闭连接按钮

	m_cAVPlayer.Play("test1.avi");// 添加此处以加载通信部分需要的dll文件。不添加的结果是无法在播放视频前建立通信，此处需要修正
	Stop();
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
    if( msg.pSender->GetName() == _T("btnPlaylistShow"))
    {
        ShowPlaylist(true);
    }
    else if( msg.pSender->GetName() == _T("btnPlaylistHide"))
    {
        ShowPlaylist(false);
    }
	else if (msg.pSender->GetName() == _T("btnGetConnect"))
	{
		CControlUI *pbtnGetConnect = m_PaintManager.FindControl(_T("btnGetConnect"));
		CControlUI *pbtnCloseConnect = m_PaintManager.FindControl(_T("btnCloseConnect"));
		flag = true;
		pbtnGetConnect->SetEnabled(0);
		pbtnCloseConnect->SetEnabled(-1);
		hThread_Communication = ::CreateThread(NULL, 0, CmuThreadProc, this, 0, NULL);
		hThread_Synchronize = ::CreateThread(NULL, 0, SynThread, this, 0, NULL);// 关闭原因：发送的实时时间存在误差
	}
	else if (msg.pSender->GetName() == _T("btnCloseConnect"))
	{
		CloseConnect();
	}
	else if (msg.pSender->GetName() == _T("btnOpen") || msg.pSender->GetName() == _T("btnOpenMini"))
	{
		Stop();
		OpenFileDialog();
	}
    else if( msg.pSender->GetName() == _T("btnPlay"))
    {
		Sendto(1, 2, 1);
		Play(true);
    }
	else if (msg.pSender->GetName() == _T("btnPause"))
	{
		Sendto(1, 2, 2);
		Play(false);
	}
    else if( msg.pSender->GetName() == _T("btnStop"))
    {
		Sendto(1, 2, 3);
        Stop();
    }
	else if (msg.pSender->GetName() == _T("btnFastBackward"))
	{
		m_cAVPlayer.SeekBackward();
		::PostMessage(*this, WM_USER_POS_CHANGED, 0, m_cAVPlayer.GetPos());
	}
	else if (msg.pSender->GetName() == _T("btnFastForward"))
	{
		m_cAVPlayer.SeekForward();
		::PostMessage(*this, WM_USER_POS_CHANGED, 0, m_cAVPlayer.GetPos());
	}
	else if (msg.pSender->GetName() == _T("btnPlaySlow"))
	{
		if (rate == 1) {
			rate = 0.5;
		}
		else if (rate == 2)
		{
			rate = 1;
		}
		m_cAVPlayer.SetRate(rate);
	}
	else if (msg.pSender->GetName() == _T("btnPlayFast"))
	{
		if (rate == 0.5) {
			rate = 1;
		}
		else if (rate == 1)
		{
			rate = 2;
		}
		m_cAVPlayer.SetRate(rate);
	}
	else if (msg.pSender->GetName() == _T("btnScreenNormal"))
	{
		FullScreen(false);
	}
    else if( msg.pSender->GetName() == _T("btnScreenFull"))
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
	else if (msg.pSender->GetName() == _T("btnRefresh"))
	{
		CEditUI* pUI = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("editURL")));
		Play(pUI->GetText());
	}
	else if (msg.pSender->GetName() == _T("btnAdjust"))
	{
		CEditUI* pUI = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("editATime")));
		int adjust_time = _ttoi(pUI->GetText());
		Sendto(1, 3, adjust_time);
		m_cAVPlayer.SetTime(adjust_time);
	}
	else if (msg.pSender->GetName() == _T("btnSetInit"))
	{
		during_time = 0;
		CEditUI* pUI = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("editPlayDTime")));
		pUI->SetText(_T("播放持续时间(ms)"));
	}
	else if (msg.pSender->GetName() == _T("btnSetOk"))
	{
		CEditUI* pUI = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("editPlayDTime")));
		during_time = _ttoi(pUI->GetText());
	}
	else if (msg.pSender->GetName() == _T("btnPlayMode"))
	{
		CMenuWnd *pMenu = new CMenuWnd(_T("menu.xml"));
		POINT    pt = { msg.ptMouse.x, msg.ptMouse.y };
		CDuiRect rc = msg.pSender->GetPos();
		pt.x = rc.left;
		pt.y = rc.bottom;
		pMenu->Init(&m_PaintManager, pt);
		pMenu->ShowWindow(TRUE);
	}
	else if (msg.pSender->GetName() == _T("btnAddPlaylist"))
	{
		OpenFileDialog();
	}
	else if (msg.pSender->GetName() == _T("btnDeletePlaylist"))
	{
		CTreeNodeUI  *pNodePlaylist, *pNodeTemp;
		pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodePlaylist")));
		pNodeTemp = pNodePlaylist->GetChildNode(m_playlistIndex - 1);
		pNodePlaylist->Remove(pNodeTemp);
		m_cPlayList.Delete(m_playlistIndex - 2);
	}

	__super::OnClick(msg);
}

void CDuiFrameWnd::Notify( TNotifyUI& msg )
{
	if (msg.sType == DUI_MSGTYPE_ITEMACTIVATE)
	{
		CTreeViewUI* pTree = static_cast<CTreeViewUI*>(m_PaintManager.FindControl(_T("treePlaylist")));
		if (pTree && -1 != pTree->GetItemIndex(msg.pSender) && TAG == msg.pSender->GetTag())
		{
			Stop();
			Sendto(1, 1, 1);
			m_playlistIndex = pTree->GetItemIndex(msg.pSender);
			Play(m_cPlayList.GetPlaylist(GetPlaylistIndex(m_playlistIndex)).c_str());
		}
	}
	else if (msg.sType == DUI_MSGTYPE_ITEMCLICK)
	{
		CDuiString strName = msg.pSender->GetName();
		CTreeViewUI* pTree = static_cast<CTreeViewUI*>(m_PaintManager.FindControl(_T("treePlaylist")));
		m_playlistIndex = pTree->GetItemIndex(msg.pSender);
		if (strName == _T("menuSingleCircle"))
		{
			m_playMode = EM_PLAY_MODE_SINGLE_LISTCIRCLE;
		}  
		else if (strName == _T("menuSequence"))
		{
			m_playMode = EM_PLAY_MODE_QUEUE;
		}
		else if (strName == _T("menuRandom"))
		{
			m_playMode = EM_PLAY_MODE_RANDOM;
		}
	}
	else if( msg.sType == DUI_MSGTYPE_DBCLICK)
    {
		if (IsInStaticControl(msg.pSender))
		{
			if (!msg.pSender->GetParent())
			{
				FullScreen(!m_FullScreen);
			}
		}
    }
	else if (msg.sType == DUI_MSGTYPE_SELECTCHANGED)
	{
		CDuiString    strName = msg.pSender->GetName();
		CTabLayoutUI* pTab = static_cast<CTabLayoutUI*>(m_PaintManager.FindControl(_T("tabCaption")));
		std::vector<CDuiString> vctString;
		vctString.push_back(_T("tabClient"));
		vctString.push_back(_T("tabWeb"));
		std::vector<CDuiString>::iterator it = std::find(vctString.begin(), vctString.end(), strName);
		if (vctString.end() != it)
		{
			int iIndex = it - vctString.begin();
			pTab->SelectItem(iIndex);
			if (iIndex == 0)
			{
				Stop();// 切回调整为初始状态，视频停止播放
			}
			if (iIndex > 0)
			{
				std::vector<CDuiString> vctName, vctURL;
				CActiveXUI* pActiveXUI;
				vctName.push_back(_T("ActiveXWeb"));
				vctURL.push_back(_T("http://www.baidu.com"));
				iIndex--;
				pActiveXUI = static_cast<CActiveXUI*>(m_PaintManager.FindControl(vctName[iIndex]));

				if (pActiveXUI)
				{
					IWebBrowser2* pWebBrowser = NULL;
					pActiveXUI->GetControl(IID_IWebBrowser2, (void**)&pWebBrowser);
					if (pWebBrowser)
					{
						_bstr_t bstrTmp;
						BSTR    bstr;
						pWebBrowser->get_LocationURL(&bstr);
						bstrTmp.Attach(bstr);

						if (!bstrTmp.length())
						{
							pWebBrowser->Navigate(_bstr_t(vctURL[iIndex]), NULL, NULL, NULL, NULL);
						}
					}
				}
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
	case WM_USER_ADD_IP:
		return OnAddIP(*this, wParam, lParam);
	case WM_USER_SYN_TIME:
		return OnSynTime(*this, wParam, lParam);
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
	SIZE FixSearchBtn = { 201, 0 };
	// 根据电脑显示设备的屏宽调整客户端显示画幅大小
    if(cxScreen <= 1024)
    {
        X = 765;
        Y = 460;
    } 
    else if(cxScreen <= 1280)
    {
        X = 968;
        Y = 600;
    }
    else if(cxScreen <= 1366)
    {
        X = 1028;
        Y = 626;
		WidthList += 21;
		WidthSearchEdit += 21;
		FixSearchBtn.cx += 21;
    }
    else
	{
		X = 1200;
        Y = 720;
		WidthList += 66;
		WidthSearchEdit += 66;
		FixSearchBtn.cx += 66;
    }
	CControlUI *pctnPlaylist = m_PaintManager.FindControl(_T("ctnPlaylist"));
    if (pctnPlaylist)
    {
		pctnPlaylist->SetFixedWidth(WidthList);
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
    CControlUI *pctnSlider  = m_PaintManager.FindControl(_T("ctnSlider"));
	CControlUI *pctnURL = m_PaintManager.FindControl(_T("ctnURL"));

	if (pbtnWnd && pbtnStop && pbtnScreen && pctnClient  && pctnSlider && pctnURL)
    {
        pbtnStop->SetEnabled(bShow);
        pbtnScreen->SetEnabled(bShow);
        pctnClient->SetVisible(! bShow);
        pctnSlider->SetVisible(bShow);
		pctnURL->SetVisible(!bShow);
        // 打开文件时
        if (bShow)
        {
            pbtnWnd->SetVisible(bShow);
        }
        // 关闭文件时
        else  
        {
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

void CDuiFrameWnd::AddFile( const std::vector<string_t> &vctString, bool bInit)
{
    CDuiString  strTmp;
	CTreeNodeUI *pNodeTmp, *pNodePlaylist;
    TCHAR       szName[_MAX_FNAME];
    TCHAR       szExt[_MAX_EXT];
    unsigned    i, uWantedCount;
	pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodePlaylist")));
    for(i = 0, uWantedCount = 0; i < vctString.size(); i++)
    {
        if (WantedFile(vctString[i].c_str()))
        {
            _tsplitpath_s(vctString[i].c_str(), NULL, 0, NULL, 0, szName, _MAX_FNAME, szExt, _MAX_EXT);
            strTmp.Format(_T("%s%s"), szName, szExt);
			pNodeTmp = new CTreeNodeUI;
			pNodeTmp->SetItemTextColor(0xFFC8C6CB);
			pNodeTmp->SetItemHotTextColor(0xFFC8C6CB);
			pNodeTmp->SetSelItemTextColor(0xFFC8C6CB);
			pNodeTmp->SetTag(TAG);
			pNodeTmp->SetItemText(strTmp);
			pNodeTmp->SetAttribute(_T("height"), _T("22"));
			pNodeTmp->SetAttribute(_T("inset"), _T("7,0,0,0"));
			pNodeTmp->SetAttribute(_T("itemattr"), _T("valign=\"vcenter\" font=\"4\""));
			pNodeTmp->SetAttribute(_T("folderattr"), _T("width=\"0\" float=\"true\""));
			pNodePlaylist->Add(pNodeTmp);
            uWantedCount++;
			if (!bInit)
			{
				m_cPlayList.Add(vctString[i]);// 完整路径
			}
        }
    }
}

void CDuiFrameWnd::AddConnectID(LPCTSTR str, int i)
{
	CTreeNodeUI *pNodeTmp, *pNodePlaylist;
	pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodeConnectlist")));
	if (!pNodePlaylist)
	{
		return;
	}
	LPCTSTR str_num = (LPCTSTR)&i;
	pNodeTmp = new CTreeNodeUI;
	pNodeTmp->SetItemTextColor(0xFFC8C6CB);
	pNodeTmp->SetItemHotTextColor(0xFFC8C6CB);
	pNodeTmp->SetSelItemTextColor(0xFFC8C6CB);
	pNodeTmp->SetItemText(str);
	pNodeTmp->SetName(str_num);
	pNodeTmp->SetAttribute(_T("height"), _T("22"));
	pNodeTmp->SetAttribute(_T("inset"), _T("7,0,0,0"));
	pNodeTmp->SetAttribute(_T("itemattr"), _T("valign=\"vcenter\" font=\"4\""));
	pNodeTmp->SetAttribute(_T("folderattr"), _T("width=\"0\" float=\"true\""));
	pNodePlaylist->Add(pNodeTmp);
}

void CDuiFrameWnd::CloseConnect() 
{
	CTreeNodeUI  *pNodePlaylist, *pNodeTemp;
	pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodeConnectlist")));
	for (int m = 1; m < connect_num; m++) {
		LPCTSTR str_num = (LPCTSTR)&m;
		pNodeTemp = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(str_num));
		pNodePlaylist->Remove(pNodeTemp);
	}
	flag = false;
	closesocket(serSocket);
	CControlUI *pbtnGetConnect = m_PaintManager.FindControl(_T("btnGetConnect"));
	CControlUI *pbtnCloseConnect = m_PaintManager.FindControl(_T("btnCloseConnect"));
	pbtnGetConnect->SetEnabled(-1);
	pbtnCloseConnect->SetEnabled(0);
	connect_num = 1;
}

void CDuiFrameWnd::Play( bool bPlay )
{
    if (m_cAVPlayer.IsOpen())
    {
        if (bPlay)
        {
            m_cAVPlayer.Play();
        } 
        else
        {
            m_cAVPlayer.Pause();
        }
		ShowPlayButton(!bPlay);
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
	m_Slider->SetValue(m_cAVPlayer.GetPos());
	// 方案：在Stop()函数内部加入回调，让POS位置归零，问题在于归零需要将nPosChanged
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
	time_t      timeCurrent = m_cAVPlayer.GetTime() / 1000;
	time_t      timeTotal = m_cAVPlayer.GetTotalTime() / 1000;
	gmtime_s(&tmTotal, &timeTotal);
	gmtime_s(&tmCurrent, &timeCurrent);
	_tcsftime(szTotal, MAX_PATH, _T("%X"), &tmTotal);
	_tcsftime(szCurrent, MAX_PATH, _T("%X"), &tmCurrent);
	strTime.Format(_T("%s / %s"), szCurrent, szTotal);
	synTime = m_cAVPlayer.GetTime();
	m_VideoTime->SetText(strTime);
	m_Slider->SetValue(m_cAVPlayer.GetPos());
	if (synTime > during_time&&during_time > 0)// 消息传回过程中加入过多判断可能负荷过大
	{
		Play(GetNextPath(true));
		Play(true);
	}
	return TRUE;
}

LRESULT CDuiFrameWnd::OnEndReached(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	Play(GetNextPath(true));
	Play(true);
	return TRUE;
}

LRESULT CDuiFrameWnd::OnAddIP(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	AddConnectID(address_ip, connect_num);
	connect_num++;
	return TRUE;
}

LRESULT CDuiFrameWnd::OnSynTime(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	//m_cAVPlayer.SetTime(synTime); // 如此调整不合理，会导致界面崩溃卡顿
	return TRUE;
}

bool CDuiFrameWnd::OnPosChanged(void* param)
{
	TNotifyUI* pMsg = (TNotifyUI*)param;

	if (pMsg->sType == _T("valuechanged"))
	{
		m_cAVPlayer.SeekTo((static_cast<CSliderUI*>(pMsg->pSender))->GetValue() + 1); // 获取的值少了1，导致设置的值也少了1
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

int CDuiFrameWnd::GetPlaylistIndex(int iIndexTree)
{
	int iNodeStart = -1; // 第一个文件的tree下标
	int iFileCount = 0;
	GetPlaylistInfo(iNodeStart, iFileCount);
	if (iFileCount <= 0)
	{
		return -1;
	}
	return iIndexTree - iNodeStart;
}

void CDuiFrameWnd::GetPlaylistInfo(int &iIndexTreeStart, int &iFileCount)
{
	iIndexTreeStart = -1;
	iFileCount = 0;
	CTreeNodeUI *pNodeTmp;
	CTreeNodeUI *pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodePlaylist")));
	int iNodeTotal = pNodePlaylist->GetCountChild();
	int i;

	for (i = 0; i < iNodeTotal; i++)
	{
		pNodeTmp = pNodePlaylist->GetChildNode(i);
		if (TAG == pNodeTmp->GetTag())
		{
			break;
		}
	}
	if (i == iNodeTotal)
	{
		return;
	}
	CTreeViewUI* pTree = static_cast<CTreeViewUI*>(m_PaintManager.FindControl(_T("treePlaylist")));
	if (pTree)
	{
		iIndexTreeStart = pTree->GetItemIndex(pNodeTmp);
		iFileCount = iNodeTotal - i;
	}
}

DuiLib::CDuiString CDuiFrameWnd::GetNextPath(bool bNext)
{
	int NodeStart = -1;
	int FileCount = 0;
	int IndexPlay = m_playlistIndex;
	GetPlaylistInfo(NodeStart, FileCount);

	if (FileCount <= 0)
	{
		return _T("");
	}

	if (-1 == IndexPlay)
	{
		IndexPlay = NodeStart;
	}

	if (EM_PLAY_MODE_RANDOM == m_playMode)
	{
		if (!m_rand.size())
		{
			Rand(m_rand, FileCount);
		}

		IndexPlay = NodeStart + m_rand.front();
		m_rand.pop_front();
	}
	else if (EM_PLAY_MODE_QUEUE == m_playMode)
	{
		if (bNext)
		{
			IndexPlay++;

			if (IndexPlay >= NodeStart + FileCount)
			{
				IndexPlay = NodeStart;
			}
		}
		else
		{
			IndexPlay--;

			if (IndexPlay < NodeStart)
			{
				IndexPlay = NodeStart + FileCount - 1;
			}
		}
	}
	CTreeViewUI *pTree = static_cast<CTreeViewUI*>(m_PaintManager.FindControl(_T("treePlaylist")));
	if (pTree)
	{
		pTree->SelectItem(IndexPlay, true);
	}
	m_playlistIndex = IndexPlay;

	return m_cPlayList.GetPlaylist(IndexPlay - NodeStart).c_str();// 完整路径
}
