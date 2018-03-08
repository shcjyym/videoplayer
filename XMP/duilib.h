#pragma once
#include "../duilib/UIlib.h"
using namespace DuiLib;

#ifdef _DEBUG
#   ifdef _UNICODE
#       pragma comment(lib, "../_lib/DuiLib_ud.lib")
#   else
#       pragma comment(lib, "../_lib/DuiLib_d.lib")
#   endif
#else
#   ifdef _UNICODE
#       pragma comment(lib, "../_lib/DuiLib_u.lib")
#   else
#       pragma comment(lib, "../_lib/DuiLib.lib")
#   endif
#endif


// ��XML���ɽ���Ĵ��ڻ���
class CXMLWnd : public WindowImplBase
{
public:
    explicit CXMLWnd(LPCTSTR pszXMLName) 
        : m_XMLName(pszXMLName){}

public:
    virtual LPCTSTR GetWindowClassName() const
    {
        return _T("XMLWnd");
    }

    virtual CDuiString GetSkinFile()
    {
        return m_XMLName;
    }

    virtual CDuiString GetSkinFolder()
    {
        return _T("");
    }

protected:
    CDuiString m_XMLName;
};


// ��HWND��ʾ��CControlUI����
class CWndUI: public CControlUI
{
public:
    CWndUI(): m_hWnd(NULL){}

    virtual void SetVisible(bool bVisible = true)
    {
        __super::SetVisible(bVisible);
        ::ShowWindow(m_hWnd, bVisible);
    }

    virtual void SetInternVisible(bool bVisible = true)
    {
        __super::SetInternVisible(bVisible);
        ::ShowWindow(m_hWnd, bVisible);
    }

    virtual void SetPos(RECT rc)
    {
        __super::SetPos(rc);
        ::SetWindowPos(m_hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    BOOL Attach(HWND hWndNew)
    {
        if (! ::IsWindow(hWndNew))
        {
            return FALSE;
        }

        m_hWnd = hWndNew;
        return TRUE;
    }

    HWND Detach()
    {
        HWND hWnd = m_hWnd;
        m_hWnd = NULL;
        return hWnd;
    }

    HWND GetHWND()
    {
        return m_hWnd;
    }

protected:
    HWND m_hWnd;
};
