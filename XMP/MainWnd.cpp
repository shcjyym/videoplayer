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

#define WM_USER_PLAYING               WM_USER + 1// ��ʼ�����ļ�
#define WM_USER_POS_CHANGED    WM_USER + 2// �ļ�����λ�øı�
#define WM_USER_END_REACHED     WM_USER + 3// �������
#define WM_USER_ADD_IP                 WM_USER + 4

//�ļ�����
const TCHAR STR_FILE_FILTER[] =
_T("All Files(*.*)\0*.*\0")
_T("Movie Files(*.rmvb,*.mpeg,etc)\0*.rm;*.rmvb;*.flv;*.f4v;*.avi;*.3gp;*.mp4;*.wmv;*.mpeg;*.mpga;*.asf;*.dat;*.mov;*.dv;*.mkv;*.mpg;*.trp;*.ts;*.vob;*.xv;*.m4v;*.dpg;\0");

// ����
const TCHAR STR_FILE_MOVIE[] =
_T("Movie Files(*.rmvb,*.mpeg,etc)|*.rm;*.rmvb;*.flv;*.f4v;*.avi;*.3gp;*.mp4;*.wmv;*.mpeg;*.mpga;*.asf;*.dat;*.mov;*.dv;*.mkv;*.mpg;*.trp;*.ts;*.vob;*.xv;*.m4v;*.dpg;|");

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
        _tcscat_s(szExt, _MAX_EXT, _T(";"));//����;�ж��Ƿ�ƥ��
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

SOCKET serSocket; // ����������
sockaddr_in serAddr; // ��������ַ
sockaddr_in remoteAddr; // Զ�̿��ƿͻ��˵�ַ
CDuiString address_ip; // �б�����ʾ����
char recvData[255]; // ��ȡ����������
int connect_num=1;// ������Ŀ
DWORD WINAPI CDuiFrameWnd::CmuThreadProc(LPVOID lpParameter)
{
	CDuiFrameWnd* pDlg;
	pDlg = (CDuiFrameWnd*)lpParameter;
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
	int ret = recvfrom(serSocket, recvData, 255, 0, (sockaddr *)&remoteAddr, &nAddrLen);
	char * sendData = "һ�����Է���˵�UDP���ݰ�";
	sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
	if (recvData[0] == '1') {
		size_t len = strlen(recvData) + 1;
		size_t converted = 0;
		wchar_t *WStr;
		WStr = (wchar_t*)malloc(len * sizeof(wchar_t));
		mbstowcs_s(&converted, WStr, len, recvData, _TRUNCATE);
		//��char���͵�recvDataת��Ϊwchar_t���͵�WStr,������ʾΪ����
		address_ip.Format(_T("%s"), WStr);
		::PostMessage(pDlg->m_hWnd, WM_USER_ADD_IP, 0, 0);
	}
	//closesocket(serSocket);//�������Ӻ�ť���ң����ر��׽���
	return 0;
}

DWORD WINAPI CDuiFrameWnd::SynThread(LPVOID lpParameter)
{
	while (1)
	{
		Sleep(1000);
		char *temp = "88";
		int nAddrLen = sizeof(remoteAddr);
		sendto(serSocket, temp, strlen(temp), 0, (sockaddr *)&remoteAddr, nAddrLen);
	}
}

DUI_BEGIN_MESSAGE_MAP(CDuiFrameWnd, CNotifyPump)
      DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK,OnClick)
DUI_END_MESSAGE_MAP()

void CDuiFrameWnd::InitWindow()
{
	SetIcon(IDI_ICON1);

	// ���ݷֱ����Զ����ڴ��ڴ�С
	MONITORINFO Monitor = {};
	Monitor.cbSize = sizeof(Monitor);
	::GetMonitorInfo(::MonitorFromWindow(*this, MONITOR_DEFAULTTONEAREST), &Monitor);
	AdaptWindowSize(Monitor.rcMonitor.right - Monitor.rcMonitor.left);
	::GetWindowPlacement(*this, &m_OldWndPlacement);

	// ��ʼ��CActiveXUI�ؼ�
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

	// �������ÿؼ���Ϊ��Ա����
	CSliderUI* pSilderVol = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("sliderVol")));
	m_Slider = static_cast<CSliderUI*>(m_PaintManager.FindControl(_T("sliderPlay")));
	m_VideoTime = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("labelPlayTime")));

	if (!pSilderVol || !m_Slider || !m_VideoTime)
	{
		return;
	}

	pSilderVol->OnNotify += MakeDelegate(this, &CDuiFrameWnd::OnVolumeChanged);
	m_Slider->OnNotify += MakeDelegate(this, &CDuiFrameWnd::OnPosChanged);

	// ���ò������Ĵ��ھ���ͻص�����
	CWndUI *pWnd = static_cast<CWndUI*>(m_PaintManager.FindControl(_T("wndMedia")));
	if (pWnd)
	{
		m_cAVPlayer.SetHWND(pWnd->GetHWND());
		m_cAVPlayer.SetCbPlaying(CbPlaying);
		m_cAVPlayer.SetCbPosChanged(CbPosChanged);
		m_cAVPlayer.SetCbEndReached(CbEndReached);
	}
	m_cAVPlayer.Play("test1.avi");//��Ӵ˴��Լ���ͨ�Ų�����Ҫ��dll�ļ�������ӵĽ�����޷��ڲ�����Ƶǰ����ͨ�ţ��˴���Ҫ����
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
		pbtnGetConnect->SetEnabled(0);
		hThread_Communication = ::CreateThread(NULL, 0, CmuThreadProc, this, 0, NULL);
	}
	else if (msg.pSender->GetName() == _T("btnCloseConnect"))
	{
		CloseConnect();
	}
	else if (msg.pSender->GetName() == _T("btnOpen") || msg.pSender->GetName() == _T("btnOpenMini"))
	{
		OpenFileDialog();
	}
    else if( msg.pSender->GetName() == _T("btnPlay"))
    {
		char * sendData = "2";
		int nAddrLen = sizeof(remoteAddr);
		sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
		hThread_Synchronize = ::CreateThread(NULL, 0, SynThread, this, 0, NULL);
        Play(true);
    }
    else if( msg.pSender->GetName() == _T("btnPause") ) 
    {
		char * sendData = "3";
		int nAddrLen = sizeof(remoteAddr);
		sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
        Play(false);
    }
    else if( msg.pSender->GetName() == _T("btnStop"))
    {
		char * sendData = "4";
		int nAddrLen = sizeof(remoteAddr);
		sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
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
		char sendData[10];
		itoa(adjust_time, sendData, 10);
		int nAddrLen = sizeof(remoteAddr);
		sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
		m_cAVPlayer.SetTime(adjust_time);
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
				Stop();// �лص���Ϊ��ʼ״̬����Ƶֹͣ����
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
	// ���ݵ�����ʾ�豸����������ͻ�����ʾ������С
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
        // ���ļ�ʱ
        if (bShow)
        {
            pbtnWnd->SetVisible(bShow);
        }
        // �ر��ļ�ʱ
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
    CDuiString  strTmp;
    TCHAR       szName[_MAX_FNAME];
    TCHAR       szExt[_MAX_EXT];
    unsigned    i, uWantedCount;
    for(i = 0, uWantedCount = 0; i < vctString.size(); i++)
    {
        if (WantedFile(vctString[i].c_str()))
        {
            _tsplitpath_s(vctString[i].c_str(), NULL, 0, NULL, 0, szName, _MAX_FNAME, szExt, _MAX_EXT);
            strTmp.Format(_T("%s%s"), szName, szExt);// �ļ�����ʽ��д
            uWantedCount++;
        }
    }
	Play(strTmp);
}

void CDuiFrameWnd::AddConnectID(LPCTSTR str, int i)
{
	CTreeNodeUI *pNodeTmp, *pNodePlaylist;
	pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodePlaylist")));
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
	pNodePlaylist = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(_T("nodePlaylist")));
	for (int m = 1; m < connect_num; m++) {
		LPCTSTR str_num = (LPCTSTR)&m;
		pNodeTemp = static_cast<CTreeNodeUI*>(m_PaintManager.FindControl(str_num));
		pNodePlaylist->Remove(pNodeTemp);
	}
	closesocket(serSocket);
	CControlUI *pbtnGetConnect = m_PaintManager.FindControl(_T("btnGetConnect"));
	pbtnGetConnect->SetEnabled(-1);
	connect_num = 1;
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
	m_Slider->SetValue(m_cAVPlayer.GetPos());
	// ��������Stop()�����ڲ�����ص�����POSλ�ù��㣬�������ڹ�����Ҫ��nPosChanged
	// �������أ���Ҫ��cAVPlayer.cpp�����¼��룬��������Ӧ�ڳ�ʼ���׶Σ���Ϊ���ļ��ڼ���е�ý���ʼ����
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
	_tcsftime(szTotal, MAX_PATH, _T("%X"), &tmTotal);
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

LRESULT CDuiFrameWnd::OnAddIP(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	AddConnectID(address_ip, connect_num);
	connect_num++;
	return TRUE;
}

bool CDuiFrameWnd::OnPosChanged(void* param)
{
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == _T("valuechanged"))
	{
		m_cAVPlayer.SeekTo((static_cast<CSliderUI*>(pMsg->pSender))->GetValue() + 1); // ��ȡ��ֵ����1���������õ�ֵҲ����1����������+1
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
