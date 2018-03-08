#pragma once
#include "duilib.h"
#include <deque>
#include "../_include/xmp/AVPlayer.h"
#include "PlayList.h"

class CDuiFrameWnd: public CXMLWnd
{
public:
    explicit CDuiFrameWnd(LPCTSTR pszXMLName);
    ~CDuiFrameWnd();

    DUI_DECLARE_MESSAGE_MAP()
    virtual void InitWindow();
    virtual CControlUI* CreateControl(LPCTSTR pstrClassName);
    virtual void Notify(TNotifyUI& msg);
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void OnClick(TNotifyUI& msg);
    virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam); 

    void OnDisplayChange(HWND hwnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen);
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);
    LRESULT OnPlaying(HWND hwnd, WPARAM wParam, LPARAM lParam);// �ļ�ͷ��ȡ��ϣ���ʼ����
    LRESULT OnPosChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);// ���ȸı䣬�������������Ľ���
    LRESULT OnEndReached(HWND hwnd, WPARAM wParam, LPARAM lParam);// �ļ��������
    bool    OnPosChanged(void* param);// ���ȸı䣬�û������ı����
    bool    OnVolumeChanged(void* param);// �����ı�

    void Play(LPCTSTR pszPath);// ����·��ΪpszPath���ļ�
    void Play(bool bPlay);// ���Ż���ͣ
    void Stop();// ֹͣ
    void OpenFileDialog();// ���ļ�����
    void ShowPlaylist(bool bShow);// ��ʾ�����б�
    void AddFile(const std::vector<string_t> &vctString);// ����ļ��������б�

private:
    CAVPlayer       m_cAVPlayer;// ��������
    CDuiString      m_strPath;// ��ǰ�ļ���·��
    CSliderUI       *m_Slider;// �ļ����Ž���
    CLabelUI        *m_VideoTime;// �ļ�����ʱ��
    WINDOWPLACEMENT m_OldWndPlacement;// ���洰��ԭ����λ��
    bool            m_FullScreen;// �Ƿ���ȫ��ģʽ
   
    void ShowPlayButton(bool bShow);// ��ʾ���Ű�ť
    void ShowPlayWnd(bool bShow);// ��ʾ���Ŵ���
    void ShowControlsForPlay(bool bShow);// ����ʼ���Ż�ֹͣʱ����ʾ������һЩ�ؼ�

    void AdaptWindowSize(UINT cxScreen);// ������Ļ�ֱ����Զ��������ڴ�С
    void FullScreen(bool bFull);// ȫ��
};