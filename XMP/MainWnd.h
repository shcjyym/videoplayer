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
    virtual void InitWindow();//
	static DWORD WINAPI CmuThreadProc(LPVOID lpParameter);// 与设备端通信函数
	static DWORD WINAPI SynThread(LPVOID lpParameter);// 与设备端同步函数
    virtual CControlUI* CreateControl(LPCTSTR pstrClassName);// 加载其余XML控件信息
    virtual void Notify(TNotifyUI& msg);//
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void OnClick(TNotifyUI& msg);// 单击控件响应
    virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);// 鼠标响应
    virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam);// 键盘响应

    void OnDisplayChange(HWND hwnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen);//
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);//
    LRESULT OnPlaying(HWND hwnd, WPARAM wParam, LPARAM lParam);// 文件头读取完毕，开始播放
    LRESULT OnPosChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);// 进度改变，播放器传回来的进度
    LRESULT OnEndReached(HWND hwnd, WPARAM wParam, LPARAM lParam);// 文件播放完毕
	LRESULT OnAddIP(HWND hwnd, WPARAM wParam, LPARAM lParam);// 添加IP信息
    bool    OnPosChanged(void* param);// 进度改变，用户主动改变进度
    bool    OnVolumeChanged(void* param);// 音量改变

    void Play(LPCTSTR pszPath);// 播放路径为pszPath的文件
    void Play(bool bPlay);// 播放或暂停
    void Stop();// 停止
    void OpenFileDialog();// 打开文件窗口
    void ShowPlaylist(bool bShow);// 显示播放列表
    void AddFile(const std::vector<string_t> &vctString);// 添加文件到播放列表
	void AddConnectID(LPCTSTR str, int i);// 添加通信连接IP地址
	void CloseConnect();// 关闭通信连接

private:
	CAVPlayer       m_cAVPlayer;// 播放器类
    CDuiString      m_strPath;// 当前文件的路径
    CSliderUI       *m_Slider;// 文件播放进度
    CLabelUI        *m_VideoTime;// 文件播放时间
    WINDOWPLACEMENT m_OldWndPlacement;// 保存窗口原来的位置
    bool            m_FullScreen;// 是否在全屏模式
	HANDLE      hThread_Communication;// 通信线程句柄
	HANDLE      hThread_Synchronize;// 同步线程句柄
    void ShowPlayButton(bool bShow);// 显示播放按钮
    void ShowPlayWnd(bool bShow);// 显示播放窗口
	void ShowConnectButton(bool bShow);// 显示通信连接按钮
    void ShowControlsForPlay(bool bShow);// 当开始播放或停止时，显示或隐藏一些控件

    void AdaptWindowSize(UINT cxScreen);// 根据屏幕分辨率自动调整窗口大小
    void FullScreen(bool bFull);// 全屏
};