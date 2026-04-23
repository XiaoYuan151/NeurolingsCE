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

#include "shijima-qt/MascotCommandService.hpp"

#include <QJsonObject>
#include <QString>

#include <atomic>
#include <memory>
#include <thread>

class ShijimaManager;

struct ShijimaLocalApiClientOptions {
    int connectTimeoutMs = 500;
    int readTimeoutMs = 500;
};

QString shijimaLocalApiServerName();
bool shijimaLocalApiRequest(QJsonObject const& request, QJsonObject &response,
    QString &transportError,
    ShijimaLocalApiClientOptions const& options = {});
bool shijimaLocalApiPing(
    ShijimaLocalApiClientOptions const& options = {});
bool shijimaLocalApiShowManager(
    ShijimaLocalApiClientOptions const& options = {});

class ShijimaLocalApi {
private:
    std::unique_ptr<std::thread> m_thread;
    MascotCommandService m_service;
    QString m_serverName;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
public:
    explicit ShijimaLocalApi(ShijimaManager *manager);

    void start(QString const& serverName = shijimaLocalApiServerName());
    void stop();
    bool running() const;
    QString const& serverName() const;
};
