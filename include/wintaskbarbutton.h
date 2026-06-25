// # Copyright © 2026 Fenris Creations.
#pragma once
#ifndef WINTASKBARBUTTON_H
#define WINTASKBARBUTTON_H
#if WIN64
#include <qwindowdefs_win.h>

class ITaskbarList3;
class QIcon;

class WinTaskbarButton
{
public:
    WinTaskbarButton();
    ~WinTaskbarButton();

    void setWindow(HWND handle);
    void setOverlayIcon(int warnings, int errors);
private:
    void generateIcons();

    ITaskbarList3* m_taskbarList{nullptr};
    HWND m_handle{nullptr};
    HICON m_icons[100];
};
#endif // WIN64
#endif //WINTASKBARBUTTON_H
