#include "MainWnd.h"

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    ::CoInitialize(NULL);
    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() + _T("skin"));
    CDuiFrameWnd *pWnd = new CDuiFrameWnd(_T("XMP.xml"));
	pWnd->Create(NULL, _T("²¥·ÅÆ÷"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES);
	pWnd->ShowModal();
    delete pWnd;
    ::CoUninitialize();
    return 0;
}
