#pragma once
#include "duilib.h"
#include <deque>
#include "../_include/xmp/AVPlayer.h"
#include "PlayList.h"

//播放模式定义
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
	static DWORD WINAPI CmuThreadProc(LPVOID lpParameter);// 与设备端通信函数
	static DWORD WINAPI SynThread(LPVOID lpParameter);// 与设备端同步函数
    virtual CControlUI* CreateControl(LPCTSTR pstrClassName);// 加载主XML外控件信息
    virtual void Notify(TNotifyUI& msg);// 消息类型处理函数
    virtual void OnClick(TNotifyUI& msg);// 单击控件响应
    virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam);// 键盘响应
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);// 自定义消息
	virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);// 拖动控件

    void OnDisplayChange(HWND hwnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen);// 显示器变化
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);// 最大化信息
    LRESULT OnPlaying(HWND hwnd, WPARAM wParam, LPARAM lParam);// 文件头读取完毕，开始播放
    LRESULT OnPosChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);// 进度改变，媒体传回进度
    LRESULT OnEndReached(HWND hwnd, WPARAM wParam, LPARAM lParam);// 播放完毕标志
	LRESULT OnAddIP(HWND hwnd, WPARAM wParam, LPARAM lParam);// 添加IP信息
	LRESULT OnSynTime(HWND hwnd, WPARAM wParam, LPARAM lParam);// 同步时间
	bool OnPosChanged(void* param);// 进度改变，用户主动改变进度
	bool OnVolumeChanged(void* param);// 音量改变

    void Play(LPCTSTR pszPath);// 播放路径为pszPath的文件
    void Play(bool bPlay);// 播放或暂停
    void Stop();// 停止
    void OpenFileDialog();// 打开文件窗口
    void ShowPlaylist(bool bShow);// 显示播放列表
    void AddFile(const std::vector<string_t> &vctString, bool bInit = false);// 添加文件到播放列表
	void AddConnectID(LPCTSTR str, int i);// 添加通信连接IP地址
	void CloseConnect();// 关闭通信连接
	CDuiString GetNextPath(bool bNext);// 获取下一个播放路径，bNext为true代表下一个，为false代表上一个

private:
	CAVPlayer       m_cAVPlayer;// 播放器类
    CDuiString      m_strPath;// 当前文件的路径
    CSliderUI       *m_Slider;// 文件播放进度
    CLabelUI        *m_VideoTime;// 文件播放时间
	CPlaylist         m_cPlayList;// 存储播放列表的路径
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
	void GetPlaylistInfo(int &iIndexTreeStart, int &iFileCount);// 获取播放列表在Tree控件中的起始位置、文件数量
	int m_playlistIndex;// 辅助标记
	int  GetPlaylistIndex(int iIndexTree); // Tree控件的Playlist下标
	PlayMode   m_playMode;// 播放模式选择号
	std::deque<unsigned>   m_rand;// 随机播放序号
	int during_time;// 播放持续时间
};