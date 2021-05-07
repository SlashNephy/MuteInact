// 非アクティブTVTestを黙らせる
#include <Windows.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,11)
#include "TVTestPlugin.h"

class CMuteInact : public TVTest::CTVTestPlugin
{
public:
    CMuteInact()
        : m_lpClassName(L"TVTest MuteInact Receiver")
        , m_hwnd(nullptr)
    {
    }

    bool GetPluginInfo(TVTest::PluginInfo *pInfo)
    {
        pInfo->Type = TVTest::PLUGIN_TYPE_NORMAL;
        pInfo->Flags = 0;
        pInfo->pszPluginName = L"MuteInact";
        pInfo->pszCopyright = L"Public Domain";
        pInfo->pszDescription = L"非アクティブTVTestを黙らせる (1.0.0)";
        return true;
    }

    bool Initialize()
    {
        WNDCLASS wc = {};
        wc.lpfnWndProc = ReceiverWndProc;
        wc.hInstance = g_hinstDLL;
        wc.lpszClassName = m_lpClassName;
        RegisterClass(&wc);
        m_pApp->SetEventCallback(EventCallback, this);
        return true;
    }

    bool Finalize()
    {
        DisablePlugin();
        return true;
    }

    bool EnablePlugin()
    {
        if (!m_hwnd)
        {
            m_hwnd = CreateWindowEx(0, m_lpClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hinstDLL, this);
            if (!m_hwnd)
            {
                return false;
            }
            m_pApp->SetWindowMessageCallback(WindowMsgCallback, this);
            MuteOthers();
        }
        return true;
    }

    bool DisablePlugin()
    {
        if (m_hwnd)
        {
            m_pApp->SetWindowMessageCallback(nullptr);
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            UnmuteOther();
        }
        return true;
    }

    static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
    {
        if (Event == TVTest::EVENT_PLUGINENABLE)
        {
            return lParam1 ? ((CMuteInact*)pClientData)->EnablePlugin() : ((CMuteInact*)pClientData)->DisablePlugin();
        }
        return 0;
    }

    static BOOL CALLBACK WindowMsgCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pUserData)
    {
        if (uMsg == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE)
        {
            ((CMuteInact*)pUserData)->MuteOthers();
        }
        return FALSE;
    }

    static LRESULT CALLBACK ReceiverWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CMuteInact *pThis = (CMuteInact*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        switch (uMsg)
        {
        case WM_CREATE:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
            return 0;
        case WM_APP:
            if ((wParam != 0) != pThis->m_pApp->GetMute())
            {
                pThis->m_pApp->SetMute(wParam != 0);
                if (wParam)
                {
                    // ミュート時刻を記録する
                    WCHAR szText[32];
                    wsprintf(szText, L"MuteTick=%u", GetTickCount());
                    SetWindowText(hwnd, szText);
                }
            }
            return TRUE;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

private:
    void MuteOthers()
    {
        HWND hwnd = nullptr;
        while ((hwnd = FindWindowEx(HWND_MESSAGE, hwnd, m_lpClassName, nullptr)) != nullptr)
        {
            PostMessage(hwnd, WM_APP, hwnd != m_hwnd, 0);
        }
    }

    void UnmuteOther()
    {
        // なるべくミュート時刻が最近のものを1つだけ復活させる
        HWND hwnd = nullptr;
        HWND hwndMin = nullptr;
        DWORD dwMin = MAXDWORD;
        DWORD dwTick = GetTickCount();
        while ((hwnd = FindWindowEx(HWND_MESSAGE, hwnd, m_lpClassName, nullptr)) != nullptr)
        {
            WCHAR szText[32];
            DWORD dw = MAXDWORD;
            if (GetWindowText(hwnd, szText, 32) > 9)
            {
                dw = dwTick - wcstoul(szText + 9, nullptr, 10);
            }
            if (dw <= dwMin)
            {
                dwMin = dw;
                hwndMin = hwnd;
            }
        }
        if (hwndMin)
        {
            PostMessage(hwndMin, WM_APP, 0, 0);
        }
    }

    const LPCWSTR m_lpClassName;
    HWND m_hwnd;
};

TVTest::CTVTestPlugin *CreatePluginClass()
{
    return new CMuteInact;
}
