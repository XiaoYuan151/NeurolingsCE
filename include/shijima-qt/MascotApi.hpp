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

#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>

#include <optional>

struct MascotInfo {
    int id = -1;
    int dataId = -1;
    QString name;
    std::optional<QString> activeBehavior;
    double anchorX = 0.0;
    double anchorY = 0.0;
};

struct LoadedMascotInfo {
    int id = -1;
    QString name;
};

struct MascotPatch {
    std::optional<double> anchorX;
    std::optional<double> anchorY;
    std::optional<QString> behavior;

    bool hasAnchor() const;
    bool hasCompleteAnchor() const;
};

struct SpawnMascotRequest {
    std::optional<QString> name;
    std::optional<int> dataId;
    MascotPatch patch;
};

struct ListMascotsRequest {
    QString selector;
};

struct DismissAllMascotsRequest {
    QString selector;
};

struct ApiPingInfo {
    bool ok = true;
    QString app;
    QString apiVersion;
};

struct MascotCommandStatus {
    int status = 200;
    QString code;
    QString error;

    bool ok() const;

    static MascotCommandStatus success();
    static MascotCommandStatus failure(int status, QString const& code,
        QString const& error);
};

QJsonObject mascotInfoToJson(MascotInfo const& mascot);
QJsonObject loadedMascotInfoToJson(LoadedMascotInfo const& mascot);
QJsonObject pingInfoToJson(ApiPingInfo const& ping);
QJsonObject errorToJson(MascotCommandStatus const& status);

bool mascotInfoFromJson(QJsonValue const& value, MascotInfo &mascot,
    QString *errorMessage = nullptr);
bool loadedMascotInfoFromJson(QJsonValue const& value,
    LoadedMascotInfo &mascot, QString *errorMessage = nullptr);
