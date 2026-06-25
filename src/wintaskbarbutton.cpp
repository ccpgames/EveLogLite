// Copyright © 2026 Fenris Creations.
#include "wintaskbarbutton.h"

#if WIN64
#include <QIcon>
#include <ShObjIdl.h>
#include <iostream>

WinTaskbarButton::WinTaskbarButton()
{
    HRESULT hr = CoCreateInstance(
        CLSID_TaskbarList,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_taskbarList)
        );

    if( !SUCCEEDED(hr) ) {
        m_taskbarList = nullptr;
    }

    generateIcons();
}

HICON createIcon(int warnings, int errors)
{
    constexpr int width{16};
    constexpr int height{16};

    constexpr COLORREF black = RGB(0, 0, 0);
    constexpr COLORREF white = RGB(255, 255, 255);
    constexpr COLORREF warningColor = RGB(255, 255, 0);
    constexpr COLORREF errorColor = RGB(255, 0, 0);

    HBITMAP hMonoBitmap = CreateBitmap(width, height, 1, 1, nullptr);
    HBITMAP hColorBitmap = CreateCompatibleBitmap(GetDC(nullptr), width, height);
    HDC hdc = CreateCompatibleDC(nullptr);

    HGDIOBJ oldBmp = SelectObject(hdc, hColorBitmap);
    HBRUSH blackBrush = CreateSolidBrush(black);
    RECT rect = {0, 0, width, height};
    FillRect(hdc, &rect, blackBrush);  // The black color will become transparent when combined with a white mask.
    SelectObject(hdc, oldBmp);

    oldBmp = SelectObject(hdc, hColorBitmap);
    RECT warningRect = {0, 16, 8, 16 - warnings};
    RECT errorRect = {8, 16, 16, 16 - errors};

    HBRUSH yellowBrush = CreateSolidBrush(warningColor);
    FillRect(hdc, &warningRect, yellowBrush);
    DeleteObject(yellowBrush);

    HBRUSH redBrush = CreateSolidBrush(errorColor);
    FillRect(hdc, &errorRect, redBrush);
    DeleteObject(redBrush);

    SelectObject(hdc, hMonoBitmap);
    HBRUSH whiteBrush = CreateSolidBrush(white);
    FillRect(hdc, &rect, whiteBrush);
    DeleteObject(whiteBrush);

    blackBrush = CreateSolidBrush(black);
    FillRect(hdc, &warningRect, blackBrush);
    FillRect(hdc, &errorRect, blackBrush);
    DeleteObject(blackBrush);

    DeleteDC(hdc);

    ICONINFO iconInfo;
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = hMonoBitmap;
    iconInfo.hbmColor = hColorBitmap;

    HICON hIcon = CreateIconIndirect(&iconInfo);

    DeleteObject(hMonoBitmap);
    DeleteObject(hColorBitmap);
    return hIcon;
}

void WinTaskbarButton::generateIcons()
{
    for (int warnings = 0; warnings < 10; ++warnings)
    {
        for (int errors = 0; errors < 10; ++errors)
        {
            m_icons[10 * warnings + errors] = createIcon(warnings, errors);
        }
    }
}

WinTaskbarButton::~WinTaskbarButton()
{
    if(m_taskbarList)
    {
        m_taskbarList->Release();
        m_taskbarList = nullptr;
    }
    for(int i = 0; i < 100; ++i)
    {
        DestroyIcon(m_icons[i]);
    }
}

void WinTaskbarButton::setWindow(HWND handle)
{
    m_handle = handle;
}

void WinTaskbarButton::setOverlayIcon(int warnings, int errors)
{
    if( !m_taskbarList )
    {
        return;
    }
    m_taskbarList->SetOverlayIcon(m_handle, m_icons[warnings * 10 + errors], L"An overlay icon showing warning and error count");
}

#endif // WIN64
