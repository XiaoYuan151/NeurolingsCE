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
    std::optional<int> cliLabel;
    double anchorX = 0.0;
    double anchorY = 0.0;
};

struct LoadedMascotInfo {
    int id = -1;
    QString name;
    QString version;
    QString description;
    QString author;
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

struct RegisterCliLabelRequest {
    int mascotId = -1;
    std::optional<int> label;
};

struct CliLabelInfo {
    int label = -1;
    int mascotId = -1;
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
QJsonObject mascotPatchToJson(MascotPatch const& patch);
QJsonObject spawnMascotRequestToJson(SpawnMascotRequest const& request);
QJsonObject cliLabelInfoToJson(CliLabelInfo const& info);
QJsonObject pingInfoToJson(ApiPingInfo const& ping);
QJsonObject errorToJson(MascotCommandStatus const& status);

bool mascotInfoFromJson(QJsonValue const& value, MascotInfo &mascot,
    QString *errorMessage = nullptr);
bool mascotPatchFromJson(QJsonValue const& value, MascotPatch &patch,
    QString *errorMessage = nullptr);
bool spawnMascotRequestFromJson(QJsonValue const& value,
    SpawnMascotRequest &request, QString *errorMessage = nullptr);
bool cliLabelInfoFromJson(QJsonValue const& value, CliLabelInfo &info,
    QString *errorMessage = nullptr);
bool loadedMascotInfoFromJson(QJsonValue const& value,
    LoadedMascotInfo &mascot, QString *errorMessage = nullptr);
