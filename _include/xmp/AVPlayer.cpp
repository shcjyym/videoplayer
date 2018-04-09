#include "AVPlayer.h"
#include <cmath>
#include <vector>
#include "../vlc/vlc.h"
#pragma comment(lib, "../_lib/vlc/libvlc.lib")
#pragma comment(lib, "../_lib/vlc/libvlccore.lib")


CAVPlayer::CAVPlayer(void) :
m_pVLC_Inst(NULL),
m_pVLC_Player(NULL),
m_hWnd(NULL),
m_pfnPlaying(NULL),
m_pfnPosChanged(NULL),
m_pfnEndReached(NULL)
{
}

CAVPlayer::~CAVPlayer(void)
{
    Release();
}

void CAVPlayer::Init()
{
    if (! m_pVLC_Inst)
    {
        m_pVLC_Inst = libvlc_new(0, NULL);
    }
}

void CAVPlayer::Release()
{
    Stop();
    if (m_pVLC_Inst)
    {
        libvlc_release (m_pVLC_Inst);
        m_pVLC_Inst   = NULL;
    }
}

bool CAVPlayer::Play(const std::string &strPath)
{
    if (! m_pVLC_Inst)
    {
        Init();
    }
    if(strPath.empty() || ! m_pVLC_Inst)
    {
        return false;
    }
	bool bURL = false;
	std::vector<std::string> vctURL;
	// 目前所支持的几种网络url
	vctURL.push_back("http");
	vctURL.push_back("https");
	vctURL.push_back("ftp");
	vctURL.push_back("rstp");
	for (unsigned i = 0; i < vctURL.size(); i++)
	{
		if (!strPath.compare(0, vctURL[i].size(), vctURL[i]))
		{
			bURL = true;
			break;
		}
	}

    Stop();
    bool bRet = false;
    libvlc_media_t *m;

	// 判断是网络视频还是本地视频，对应不同地址格式
	if (bURL)
	{
		m = libvlc_media_new_location(m_pVLC_Inst, strPath.c_str());
	}
	else
	{
		m = libvlc_media_new_path(m_pVLC_Inst, strPath.c_str());
	}

    if (m)
    {
        if (m_pVLC_Player = libvlc_media_player_new_from_media(m))
        {
            libvlc_media_player_set_hwnd(m_pVLC_Player, m_hWnd);
            //libvlc_media_player_play(m_pVLC_Player);//取消播放，只初始化播放器的媒体
            // 事件管理
            libvlc_event_manager_t *vlc_evt_man = libvlc_media_player_event_manager(m_pVLC_Player);
            libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerPlaying, ::OnVLC_Event, this);
            libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerPositionChanged, ::OnVLC_Event, this);
            libvlc_event_attach(vlc_evt_man, libvlc_MediaPlayerEndReached, ::OnVLC_Event, this);
            bRet = true;
        }
        libvlc_media_release(m);
    }
    return bRet;
}

void CAVPlayer::Stop()
{
    if (m_pVLC_Player)
    {
		libvlc_media_player_set_position(m_pVLC_Player, 0);
        libvlc_media_player_stop (m_pVLC_Player);
        libvlc_media_player_release (m_pVLC_Player);
        m_pVLC_Player = NULL;
    }
}

void CAVPlayer::Play()
{
    if (m_pVLC_Player)
    {
        libvlc_media_player_play(m_pVLC_Player);
    }
}

void CAVPlayer::Pause()
{
    if (m_pVLC_Player)
    {
        libvlc_media_player_pause(m_pVLC_Player);
    }
}

void CAVPlayer::Volume(int iVol)
{
    if (iVol < 0)
    {
        return;
    }
    if (m_pVLC_Player)
    {
        libvlc_audio_set_volume(m_pVLC_Player,int(iVol));
    }
}

void CAVPlayer::VolumeIncrease()
{
    if (m_pVLC_Player)
    {
        int iVol = libvlc_audio_get_volume(m_pVLC_Player);
        Volume((int)ceil(iVol * 1.1));
    }
}

void CAVPlayer::VolumeReduce()
{
    if (m_pVLC_Player)
    {
        int iVol = libvlc_audio_get_volume(m_pVLC_Player);
        Volume((int)floor(iVol * 0.9));
    }
}

int CAVPlayer::GetPos()
{   
    if (m_pVLC_Player)
    {
        return (int)(1000 * libvlc_media_player_get_position(m_pVLC_Player));
    }
    return 0;
}

void CAVPlayer::SeekTo(int iPos)
{
    if (iPos < 0 || iPos > 1000)
    {
        return;
    }
    if (m_pVLC_Player)
    {
        libvlc_media_player_set_position(m_pVLC_Player, iPos/(float)1000.0);
    }
}

void CAVPlayer::SeekForward()
{
    if (m_pVLC_Player)
    {
        libvlc_time_t i_time = libvlc_media_player_get_time(m_pVLC_Player) + 5000;
        if (i_time > GetTotalTime())
        {
            i_time = GetTotalTime();
        }
        libvlc_media_player_set_time(m_pVLC_Player, i_time);
    }
}

void CAVPlayer::Refresh()
{
	if (m_pVLC_Player)
	{
		libvlc_time_t i_time = 1000;
		libvlc_time_t i_ftime_low = 5000;
		libvlc_time_t i_ftime_high = 5100;//refresh函数的最大问题在于判断所需时间的长度是否足够，漏掉信号的处理方式以及验证方案
		if (i_time < 0)
		{
			i_time = 0;
		}
		if (libvlc_media_player_get_time(m_pVLC_Player) <= i_ftime_high && libvlc_media_player_get_time(m_pVLC_Player) >= i_ftime_low)//有待改进
		{
			libvlc_media_player_set_time(m_pVLC_Player, i_time);
		}
	}
}

void CAVPlayer::SeekBackward()
{
    if (m_pVLC_Player)
    {
        libvlc_time_t i_time = libvlc_media_player_get_time(m_pVLC_Player) - 5000;

        if (i_time < 0)
        {
            i_time = 0;
        }

        libvlc_media_player_set_time(m_pVLC_Player, i_time);
    }
}

void CAVPlayer::SetHWND( HWND hwnd )
{
    if (::IsWindow(hwnd))
    {
        m_hWnd = hwnd;
    }
}

HWND CAVPlayer::GetHWND()
{
    return m_hWnd;
}

bool CAVPlayer::IsOpen()
{
    return NULL != m_pVLC_Player;
}

bool CAVPlayer::IsPlaying()
{
    if (m_pVLC_Player)
    {
        return (1 == libvlc_media_player_is_playing(m_pVLC_Player));
    }

    return false;
}

__int64 CAVPlayer::GetTotalTime()
{
    if (m_pVLC_Player)
    {
        return libvlc_media_player_get_length(m_pVLC_Player);
    }

    return 0;
}

__int64 CAVPlayer::GetTime()
{
    if (m_pVLC_Player)
    {
        return libvlc_media_player_get_time(m_pVLC_Player);
    }

    return 0;
}

int CAVPlayer::GetVolume()
{
    if (m_pVLC_Player)
    {
        return libvlc_audio_get_volume(m_pVLC_Player);
    }

    return 0;
}

void CAVPlayer::SetTime(int time)
{
	if (m_pVLC_Player)
	{
		libvlc_time_t total_time = libvlc_media_player_get_length(m_pVLC_Player);
		if (time > 0 && time < total_time) 
		{
			libvlc_media_player_set_time(m_pVLC_Player, time);
		}
	}
}

void CAVPlayer::SetCbPlaying( pfnCallback pfn )
{
    m_pfnPlaying = pfn;
}

void CAVPlayer::SetCbPosChanged( pfnCallback pfn )
{
    m_pfnPosChanged = pfn;
}

void CAVPlayer::SetCbEndReached( pfnCallback pfn )
{
    m_pfnEndReached = pfn;
}

void OnVLC_Event( const libvlc_event_t *event, void *data )
{
    CAVPlayer *pAVPlayer = (CAVPlayer *) data;
    pfnCallback pfn = NULL;

    if (! pAVPlayer)
    {
        return;
    }

    switch(event->type)
    {
    case libvlc_MediaPlayerPlaying:
        pfn = pAVPlayer->m_pfnPlaying;
        break;
    case libvlc_MediaPlayerPositionChanged:
        pfn = pAVPlayer->m_pfnPosChanged;
        break;
    case libvlc_MediaPlayerEndReached:
        pfn = pAVPlayer->m_pfnEndReached;
        break;
    default:
        break;
    }

    if (pfn)
    {
        pfn(data);// 此回调函数还可以传入其他参数
    }   
}
