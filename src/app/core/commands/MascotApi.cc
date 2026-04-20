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

#include "shijima-qt/MascotApi.hpp"

namespace {

QJsonObject anchorToJson(double x, double y) {
    QJsonObject anchor;
    anchor["x"] = x;
    anchor["y"] = y;
    return anchor;
}

bool parseAnchor(QJsonValue const& value, double &x, double &y,
    QString *errorMessage)
{
    if (!value.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("anchor must be an object");
        }
        return false;
    }
    auto object = value.toObject();
    auto xValue = object.value("x");
    auto yValue = object.value("y");
    if (!xValue.isDouble() || !yValue.isDouble()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("anchor must contain numeric x and y");
        }
        return false;
    }
    x = xValue.toDouble();
    y = yValue.toDouble();
    return true;
}

}

bool MascotPatch::hasAnchor() const {
    return anchorX.has_value() || anchorY.has_value();
}

bool MascotPatch::hasCompleteAnchor() const {
    return anchorX.has_value() && anchorY.has_value();
}

bool MascotCommandStatus::ok() const {
    return status >= 200 && status < 300;
}

MascotCommandStatus MascotCommandStatus::success() {
    return {};
}

MascotCommandStatus MascotCommandStatus::failure(int statusValue,
    QString const& codeValue, QString const& errorValue)
{
    MascotCommandStatus status;
    status.status = statusValue;
    status.code = codeValue;
    status.error = errorValue;
    return status;
}

QJsonObject mascotInfoToJson(MascotInfo const& mascot) {
    QJsonObject object;
    object["id"] = mascot.id;
    object["data_id"] = mascot.dataId;
    object["name"] = mascot.name;
    object["anchor"] = anchorToJson(mascot.anchorX, mascot.anchorY);
    if (mascot.activeBehavior.has_value()) {
        object["active_behavior"] = mascot.activeBehavior.value();
    }
    else {
        object["active_behavior"] = QJsonValue::Null;
    }
    return object;
}

QJsonObject loadedMascotInfoToJson(LoadedMascotInfo const& mascot) {
    QJsonObject object;
    object["id"] = mascot.id;
    object["name"] = mascot.name;
    return object;
}

QJsonObject pingInfoToJson(ApiPingInfo const& ping) {
    QJsonObject object;
    object["ok"] = ping.ok;
    object["app"] = ping.app;
    object["api_version"] = ping.apiVersion;
    return object;
}

QJsonObject errorToJson(MascotCommandStatus const& status) {
    QJsonObject object;
    object["error"] = status.error;
    if (!status.code.isEmpty()) {
        object["code"] = status.code;
    }
    object["status"] = status.status;
    return object;
}

bool mascotInfoFromJson(QJsonValue const& value, MascotInfo &mascot,
    QString *errorMessage)
{
    if (!value.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("mascot entry must be an object");
        }
        return false;
    }
    auto object = value.toObject();
    auto idValue = object.value("id");
    auto dataIdValue = object.value("data_id");
    auto nameValue = object.value("name");
    if (!idValue.isDouble() || !dataIdValue.isDouble() || !nameValue.isString()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("mascot entry is missing required fields");
        }
        return false;
    }
    double anchorX = 0.0;
    double anchorY = 0.0;
    if (!parseAnchor(object.value("anchor"), anchorX, anchorY, errorMessage)) {
        return false;
    }
    mascot.id = idValue.toInt();
    mascot.dataId = dataIdValue.toInt();
    mascot.name = nameValue.toString();
    mascot.anchorX = anchorX;
    mascot.anchorY = anchorY;
    auto behaviorValue = object.value("active_behavior");
    if (behaviorValue.isString()) {
        mascot.activeBehavior = behaviorValue.toString();
    }
    else {
        mascot.activeBehavior.reset();
    }
    return true;
}

bool loadedMascotInfoFromJson(QJsonValue const& value,
    LoadedMascotInfo &mascot, QString *errorMessage)
{
    if (!value.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("loaded mascot entry must be an object");
        }
        return false;
    }
    auto object = value.toObject();
    auto idValue = object.value("id");
    auto nameValue = object.value("name");
    if (!idValue.isDouble() || !nameValue.isString()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("loaded mascot entry is missing required fields");
        }
        return false;
    }
    mascot.id = idValue.toInt();
    mascot.name = nameValue.toString();
    return true;
}
