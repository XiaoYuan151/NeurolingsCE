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

#include "InternalCli.hpp"
#include "shijima-qt/AppLog.hpp"

#include <httplib.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>

#include <algorithm>

namespace {

CliError makeError(QString const& code, QString const& message, int exitCode = 1,
    int httpStatus = 0, QString const& details = {})
{
    CliError error;
    error.code = code;
    error.error = message;
    error.details = details;
    error.exitCode = exitCode;
    error.httpStatus = httpStatus;
    return error;
}

void configureClient(httplib::Client &client, CliGlobalOptions const& global) {
    client.set_connection_timeout(global.connectTimeoutMs / 1000,
        (global.connectTimeoutMs % 1000) * 1000);
    client.set_read_timeout(global.readTimeoutMs / 1000,
        (global.readTimeoutMs % 1000) * 1000);
}

QJsonObject patchToJson(MascotPatch const& patch) {
    QJsonObject object;
    if (patch.hasCompleteAnchor()) {
        QJsonObject anchor;
        anchor["x"] = patch.anchorX.value();
        anchor["y"] = patch.anchorY.value();
        object["anchor"] = anchor;
    }
    if (patch.behavior.has_value()) {
        object["behavior"] = patch.behavior.value();
    }
    return object;
}

QJsonObject spawnRequestToJson(SpawnMascotRequest const& request) {
    QJsonObject object = patchToJson(request.patch);
    if (request.name.has_value()) {
        object["name"] = request.name.value();
    }
    if (request.dataId.has_value()) {
        object["data_id"] = request.dataId.value();
    }
    return object;
}

bool parseJsonObject(httplib::Result const& res, QJsonObject &object,
    CliError &error)
{
    if (res == nullptr) {
        error = makeError(QStringLiteral("not_running"),
            QStringLiteral("Request failed. Is NeurolingsCE running?"));
        return false;
    }
    QByteArray bytes(res->body.data(), (qsizetype)res->body.size());
    QJsonParseError parseError;
    auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = makeError(QStringLiteral("invalid_response"),
            QStringLiteral("Failed to parse response from HTTP API"), 1,
            res->status, parseError.errorString());
        return false;
    }
    object = document.object();
    if (res->status >= 400 || object.contains("error")) {
        error = makeError(
            object.value("code").toString(QStringLiteral("api_error")),
            object.value("error").toString(QStringLiteral("API request failed")),
            1, res->status);
        return false;
    }
    return true;
}

bool parseMascotArray(QJsonObject const& object, char const *key,
    QList<MascotInfo> &mascots, CliError &error)
{
    auto value = object.value(key);
    if (!value.isArray()) {
        error = makeError(QStringLiteral("invalid_response"),
            QStringLiteral("Malformed response from HTTP API"));
        return false;
    }
    for (auto const& item : value.toArray()) {
        MascotInfo mascot;
        QString parseError;
        if (!mascotInfoFromJson(item, mascot, &parseError)) {
            error = makeError(QStringLiteral("invalid_response"),
                QStringLiteral("Malformed mascot payload"), 1, 0, parseError);
            return false;
        }
        mascots.append(mascot);
    }
    return true;
}

bool parseLoadedMascotArray(QJsonObject const& object, char const *key,
    QList<LoadedMascotInfo> &mascots, CliError &error)
{
    auto value = object.value(key);
    if (!value.isArray()) {
        error = makeError(QStringLiteral("invalid_response"),
            QStringLiteral("Malformed response from HTTP API"));
        return false;
    }
    for (auto const& item : value.toArray()) {
        LoadedMascotInfo mascot;
        QString parseError;
        if (!loadedMascotInfoFromJson(item, mascot, &parseError)) {
            error = makeError(QStringLiteral("invalid_response"),
                QStringLiteral("Malformed loaded mascot payload"), 1, 0,
                parseError);
            return false;
        }
        mascots.append(mascot);
    }
    return true;
}

bool parseMascotObject(QJsonObject const& object, char const *key,
    MascotInfo &mascot, CliError &error)
{
    QString parseError;
    if (!mascotInfoFromJson(object.value(key), mascot, &parseError)) {
        error = makeError(QStringLiteral("invalid_response"),
            QStringLiteral("Malformed mascot payload"), 1, 0, parseError);
        return false;
    }
    return true;
}

QString chooseBehavior(QStringList const& behaviors) {
    if (behaviors.isEmpty()) {
        return {};
    }
    int index = QRandomGenerator::global()->bounded(0, behaviors.size());
    return behaviors[index];
}

bool resolveMascotId(httplib::Client &client, CliCommand const& command,
    int &mascotId, CliError &error)
{
    bool ok = false;
    int parsedId = command.idToken.toInt(&ok);
    if (ok) {
        if (!command.selectors.isEmpty() || !command.selector.isEmpty()) {
            error = makeError(QStringLiteral("invalid_arguments"),
                QStringLiteral("You can't specify a numeric ID and a selector at the same time"),
                2);
            return false;
        }
        if (parsedId < 0) {
            error = makeError(QStringLiteral("invalid_arguments"),
                QStringLiteral("ID must be greater than or equal to 0"), 2);
            return false;
        }
        mascotId = parsedId;
        return true;
    }

    static const QStringList autoIds = {
        QStringLiteral("oldest"),
        QStringLiteral("newest"),
        QStringLiteral("random"),
    };
    if (!autoIds.contains(command.idToken)) {
        error = makeError(QStringLiteral("invalid_arguments"),
            QStringLiteral("Invalid auto ID."), 2, 0,
            QStringLiteral("Expected one of: oldest, newest, random"));
        return false;
    }

    QStringList selectors = !command.selectors.isEmpty()
        ? command.selectors
        : QStringList { command.selector };
    if (selectors.isEmpty()) {
        selectors.append(QString {});
    }

    for (auto const& selector : selectors) {
        httplib::Params params;
        if (!selector.isEmpty()) {
            params.insert({ "selector", selector.toStdString() });
        }
        auto res = client.Get("/shijima/api/v1/mascots", params, {});
        QJsonObject object;
        if (!parseJsonObject(res, object, error)) {
            return false;
        }
        QList<MascotInfo> mascots;
        if (!parseMascotArray(object, "mascots", mascots, error)) {
            return false;
        }
        if (mascots.isEmpty()) {
            continue;
        }
        if (command.idToken == QStringLiteral("oldest")) {
            mascotId = mascots.first().id;
            return true;
        }
        if (command.idToken == QStringLiteral("newest")) {
            mascotId = mascots.last().id;
            return true;
        }
        mascotId = mascots[QRandomGenerator::global()->bounded(0, mascots.size())].id;
        return true;
    }

    error = makeError(QStringLiteral("not_found"),
        QStringLiteral("Failed to determine ID (are any mascots spawned?)"));
    return false;
}

}

CliExecutionResult executeCliCommand(CliCommand const& command) {
    CliExecutionResult result;
    httplib::Client client(command.global.host.toStdString(), command.global.port);
    configureClient(client, command.global);

    auto fail = [&](CliError const& error) {
        result.error = error;
        APP_LOG_ERROR("cli") << error.error.toStdString();
        if (!error.details.isEmpty()) {
            APP_LOG_WARN("cli") << error.details.toStdString();
        }
        return result;
    };

    if (command.kind == CliCommandKind::ListMascots) {
        httplib::Params params;
        if (!command.selector.isEmpty()) {
            params.insert({ "selector", command.selector.toStdString() });
        }
        auto res = client.Get("/shijima/api/v1/mascots", params, {});
        QJsonObject object;
        CliError error;
        if (!parseJsonObject(res, object, error)) {
            return fail(error);
        }
        if (!parseMascotArray(object, "mascots", result.mascots, error)) {
            return fail(error);
        }
        return result;
    }

    if (command.kind == CliCommandKind::ListLoadedMascots) {
        auto res = client.Get("/shijima/api/v1/loadedMascots");
        QJsonObject object;
        CliError error;
        if (!parseJsonObject(res, object, error)) {
            return fail(error);
        }
        if (!parseLoadedMascotArray(object, "loaded_mascots",
            result.loadedMascots, error))
        {
            return fail(error);
        }
        return result;
    }

    if (command.kind == CliCommandKind::SpawnMascot) {
        SpawnMascotRequest request = command.spawnRequest;
        auto behavior = chooseBehavior(command.behaviors);
        if (!behavior.isEmpty()) {
            request.patch.behavior = behavior;
        }
        QJsonDocument document(spawnRequestToJson(request));
        auto json = document.toJson(QJsonDocument::Compact);
        auto res = client.Post("/shijima/api/v1/mascots",
            std::string(json.constData(), (size_t)json.size()),
            "application/json");
        QJsonObject object;
        CliError error;
        if (!parseJsonObject(res, object, error)) {
            return fail(error);
        }
        MascotInfo mascot;
        if (!parseMascotObject(object, "mascot", mascot, error)) {
            return fail(error);
        }
        result.mascot = mascot;
        return result;
    }

    if (command.kind == CliCommandKind::AlterMascot) {
        CliError error;
        int mascotId = -1;
        if (!resolveMascotId(client, command, mascotId, error)) {
            return fail(error);
        }
        MascotPatch patch = command.patch;
        auto behavior = chooseBehavior(command.behaviors);
        if (!behavior.isEmpty()) {
            patch.behavior = behavior;
        }
        QJsonDocument document(patchToJson(patch));
        auto json = document.toJson(QJsonDocument::Compact);
        auto res = client.Put("/shijima/api/v1/mascots/" + std::to_string(mascotId),
            std::string(json.constData(), (size_t)json.size()),
            "application/json");
        QJsonObject object;
        if (!parseJsonObject(res, object, error)) {
            return fail(error);
        }
        MascotInfo mascot;
        if (!parseMascotObject(object, "mascot", mascot, error)) {
            return fail(error);
        }
        result.mascot = mascot;
        return result;
    }

    if (command.kind == CliCommandKind::DismissMascot) {
        CliError error;
        int mascotId = -1;
        if (!resolveMascotId(client, command, mascotId, error)) {
            return fail(error);
        }
        auto res = client.Delete("/shijima/api/v1/mascots/" + std::to_string(mascotId));
        QJsonObject object;
        if (!parseJsonObject(res, object, error)) {
            return fail(error);
        }
        return result;
    }

    QJsonObject requestObject;
    if (!command.selector.isEmpty()) {
        requestObject["selector"] = command.selector;
    }
    QJsonDocument document(requestObject);
    auto json = document.toJson(QJsonDocument::Compact);
    auto res = client.Delete("/shijima/api/v1/mascots",
        std::string(json.constData(), (size_t)json.size()), "application/json");
    QJsonObject object;
    CliError error;
    if (!parseJsonObject(res, object, error)) {
        return fail(error);
    }
    return result;
}
