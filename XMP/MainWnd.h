#pragma once
#include "duilib.h"
#include <deque>
#include "../_include/xmp/AVPlayer.h"
#include "PlayList.h"

//����ģʽ����
enum PlayMode
{
	EM_PLAY_MODE_QUEUE,
	EM_PLAY_MODE_RANDOM,        
	EM_PLAY_MODE_SINGLE_LISTCIRCLE
};

class CDuiFrameWnd: public CXMLWnd
{
public:
    explicit CDuiFrameWnd(LPCTSTR pszXMLName);
    ~CDuiFrameWnd();

    DUI_DECLARE_MESSAGE_MAP()
    virtual void InitWindow();//
	static DWORD WINAPI CmuThreadProc(LPVOID lpParameter);// ���豸��ͨ�ź���
	static DWORD WINAPI SynThread(LPVOID lpParameter);// ���豸��ͬ������
    virtual CControlUI* CreateControl(LPCTSTR pstrClassName);// ������XML��ؼ���Ϣ
    virtual void Notify(TNotifyUI& msg);// ��Ϣ���ʹ�����
    virtual void OnClick(TNotifyUI& msg);// �����ؼ���Ӧ
    virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam);// ������Ӧ
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);// �Զ�����Ϣ
	virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);// �϶��ؼ�

    void OnDisplayChange(HWND hwnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen);// ��ʾ���仯
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);// �����Ϣ
    LRESULT OnPlaying(HWND hwnd, WPARAM wParam, LPARAM lParam);// �ļ�ͷ��ȡ��ϣ���ʼ����
    LRESULT OnPosChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);// ���ȸı䣬ý�崫�ؽ���
    LRESULT OnEndReached(HWND hwnd, WPARAM wParam, LPARAM lParam);// ������ϱ�־
	LRESULT OnAddIP(HWND hwnd, WPARAM wParam, LPARAM lParam);// ���IP��Ϣ
	LRESULT OnSynTime(HWND hwnd, WPARAM wParam, LPARAM lParam);// ͬ��ʱ��
	bool OnPosChanged(void* param);// ���ȸı䣬�û������ı����
	bool OnVolumeChanged(void* param);// �����ı�

    void Play(LPCTSTR pszPath);// ����·��ΪpszPath���ļ�
    void Play(bool bPlay);// ���Ż���ͣ
    void Stop();// ֹͣ
    void OpenFileDialog();// ���ļ�����
    void ShowPlaylist(bool bShow);// ��ʾ�����б�
    void AddFile(const std::vector<string_t> &vctString, bool bInit = false);// ����ļ��������б�
	void AddConnectID(LPCTSTR str, int i);// ���ͨ������IP��ַ
	void CloseConnect();// �ر�ͨ������
	CDuiString GetNextPath(bool bNext);// ��ȡ��һ������·����bNextΪtrue������һ����Ϊfalse������һ��

private:
	CAVPlayer       m_cAVPlayer;// ��������
    CDuiString      m_strPath;// ��ǰ�ļ���·��
    CSliderUI       *m_Slider;// �ļ����Ž���
    CLabelUI        *m_VideoTime;// �ļ�����ʱ��
	CPlaylist         m_cPlayList;// �洢�����б��·��
    WINDOWPLACEMENT m_OldWndPlacement;// ���洰��ԭ����λ��
    bool            m_FullScreen;// �Ƿ���ȫ��ģʽ
	HANDLE      hThread_Communication;// ͨ���߳̾��
	HANDLE      hThread_Synchronize;// ͬ���߳̾��
    void ShowPlayButton(bool bShow);// ��ʾ���Ű�ť
    void ShowPlayWnd(bool bShow);// ��ʾ���Ŵ���
	void ShowConnectButton(bool bShow);// ��ʾͨ�����Ӱ�ť
    void ShowControlsForPlay(bool bShow);// ����ʼ���Ż�ֹͣʱ����ʾ������һЩ�ؼ�
    void AdaptWindowSize(UINT cxScreen);// ������Ļ�ֱ����Զ��������ڴ�С
    void FullScreen(bool bFull);// ȫ��
	void GetPlaylistInfo(int &iIndexTreeStart, int &iFileCount);// ��ȡ�����б���Tree�ؼ��е���ʼλ�á��ļ�����
	int m_playlistIndex;// �������
	int  GetPlaylistIndex(int iIndexTree); // Tree�ؼ���Playlist�±�
	PlayMode   m_playMode;// ����ģʽѡ���
	std::deque<unsigned>   m_rand;// ����������
	int during_time;// ���ų���ʱ��
};