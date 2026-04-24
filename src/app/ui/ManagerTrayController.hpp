#pragma once

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

#include <functional>

#include <QObject>
#include <QString>

class QMenu;
class QSystemTrayIcon;
class ShijimaManager;

class ManagerTrayController : public QObject {
public:
    explicit ManagerTrayController(ShijimaManager *manager);
    ~ManagerTrayController() override;

    static bool isAvailable();
    void refreshMenu();
    void showMessage(QString const& title, QString const& message,
        std::function<void()> onClick = {});

private:
    ShijimaManager *m_manager = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    std::function<void()> m_messageClickHandler;
};
