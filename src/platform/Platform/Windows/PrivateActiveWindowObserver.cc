// 
// Shijima-Qt - Cross-platform shimeji simulation app for desktop
// Copyright (C) 2025 pixelomer
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 

#include "PrivateActiveWindowObserver.hpp"
#include <windows.h>
#include <process.h>
#include <QString>
#include <string>

namespace Platform {

PrivateActiveWindowObserver::PrivateActiveWindowObserver() {}

ActiveWindow PrivateActiveWindowObserver::getActiveWindow() {
    HWND foregroundWindow = GetForegroundWindow();
    if (foregroundWindow == NULL) {
        return m_activeWindow;
    }
    DWORD newPid;
    if (GetWindowThreadProcessId(foregroundWindow, &newPid) == 0) {
        return m_activeWindow = {};
    }
    if ((long)newPid == _getpid()) {
        return m_activeWindow;
    }
    RECT rect;
    if (!GetWindowRect(foregroundWindow, &rect)) {
        return m_activeWindow = {};
    }
    UINT dpi = GetDpiForWindow(foregroundWindow);
    if (dpi == 0) {
        return m_activeWindow = {};
    }
    double scaleRatio = 96.0 / dpi;
    QString uid = QString::fromStdString(std::to_string(newPid) + "-"
        + std::to_string((unsigned long long)foregroundWindow));
    return m_activeWindow = { uid, (long)newPid,
        rect.left * scaleRatio,
        rect.top * scaleRatio,
        (rect.right - rect.left) * scaleRatio,
        (rect.bottom - rect.top) * scaleRatio };
}

}
