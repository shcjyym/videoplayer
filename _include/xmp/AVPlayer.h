#ifndef __AVPlayer_H__
#define __AVPlayer_H__
#include <iostream>
#include <windows.h>

typedef void (* pfnCallback) (void *data);// VLC�¼��Ļص�����ָ��
struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_event_t;

class CAVPlayer
{
    // VLC���¼�����
    friend void OnVLC_Event(const libvlc_event_t *event, void *data);

public:
    CAVPlayer(void);
    ~CAVPlayer(void);

    bool Play(const std::string &strPath);// ����·��ΪstrPath���ļ�
    void Play ();// ����
    void Pause();// ��ͣ
    void Stop ();// ֹͣ

    void Volume(int iVol);// ��������ΪiVol
    void VolumeIncrease();// ��������
    void VolumeReduce();// ������С 

    void SeekTo(int iPos);// ����ָ��λ��iPos
    void SeekForward();// ���
    void SeekBackward();// ����
	void Refresh();// ˢ�£��˹�����δʹ�ã�
	void SetTime(int time);// ����ʱ��

    void SetHWND(HWND hwnd);// ������Ƶ��ʾ�Ĵ��ھ��
    HWND GetHWND();// ��ȡ��Ƶ��ʾ�Ĵ��ھ��

    bool    IsOpen();// �ļ��Ƿ��
    bool    IsPlaying();// �ļ��Ƿ����ڲ���
    int     GetPos();// ��ȡ�ļ���ǰ���ŵ�λ��
    __int64 GetTotalTime();// ��ȡ��ʱ��
    __int64 GetTime();// ��ȡʱ��
    int     GetVolume();// ��ȡ����

    void SetCbPlaying(pfnCallback pfn);// �ļ�ͷ��ȡ���ʱ�Ļص�
    void SetCbPosChanged(pfnCallback pfn);// �ļ�λ�øı�ʱ�Ļص�
    void SetCbEndReached(pfnCallback pfn);// �ļ�ͷ��ȡ���ʱ�Ļص�

private:
    libvlc_instance_t       *m_pVLC_Inst;// VLCʵ��
    libvlc_media_player_t   *m_pVLC_Player;// VLC������
    HWND                    m_hWnd;// ��Ƶ��ʾ�Ĵ��ھ��
    pfnCallback             m_pfnPlaying;// �ļ���ȡ��ϣ�׼������
    pfnCallback             m_pfnPosChanged;// �ļ�λ�øı�ʱ�Ļص�����
    pfnCallback             m_pfnEndReached;// �ļ�������ϵĻص�����

    void Init();// ��ʼ��
    void Release();// �����ڴ�
};
#endif
