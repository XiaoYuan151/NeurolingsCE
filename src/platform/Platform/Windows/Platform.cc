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

#include "../Platform.hpp"
#include <QWidget>
#include <windows.h>

namespace Platform {

void initialize(int argc, char **argv) {
    freopen("shijima_stdout.txt", "a", stdout);
    freopen("shijima_stderr.txt", "a", stderr);
}

void showOnAllDesktops(QWidget *widget) {
    HWND window = (HWND)widget->winId();
    LONG_PTR exstyle = GetWindowLongPtr(window, GWL_EXSTYLE);
    if (exstyle != 0) {
        exstyle |= WS_EX_TOOLWINDOW;
        SetWindowLongPtr(window, GWL_EXSTYLE, exstyle);
    }
}

void refreshTopmost(QWidget *widget) {
    HWND window = (HWND)widget->winId();
    if (window == NULL || !IsWindow(window)) {
        return;
    }

    // Reassert the topmost z-order without stealing focus so the mascot
    // can move back above the taskbar after the taskbar is clicked.
    SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

bool useWindowMasks() {
    return false;
}

}
