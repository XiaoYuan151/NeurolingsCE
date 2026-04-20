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

#include "shijima-qt/ShijimaLocalApi.hpp"

#include "shijima-qt/AppLog.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocalServer>
#include <QLocalSocket>

namespace {

QString const kCommandKey = QStringLiteral("command");
QString const kRequestKey = QStringLiteral("request");
QString const kPatchKey = QStringLiteral("patch");
QString const kSelectorKey = QStringLiteral("selector");
QString const kMascotIdKey = QStringLiteral("mascot_id");
QString const kLabelKey = QStringLiteral("label");
QString const kArchivePathKey = QStringLiteral("archive_path");
QString const kMascotNameKey = QStringLiteral("mascot_name");

QJsonObject invalidRequest(QString const& message) {
    QJsonObject object;
    object["error"] = message;
    object["code"] = QStringLiteral("bad_request");
    object["status"] = 400;
    return object;
}

QByteArray jsonMessage(QJsonObject const& object) {
    auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    bytes.append('\n');
    return bytes;
}

bool parseObject(QByteArray const& bytes, QJsonObject &object, QString &error) {
    QJsonParseError parseError;
    auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("Failed to parse IPC response: %1")
            .arg(parseError.errorString());
        return false;
    }
    object = document.object();
    return true;
}

bool readMessage(QLocalSocket &socket, int timeoutMs, QByteArray &message,
    QString &error)
{
    message.clear();
    while (true) {
        auto newlineIndex = message.indexOf('\n');
        if (newlineIndex >= 0) {
            message.truncate(newlineIndex);
            return true;
        }
        if (socket.state() != QLocalSocket::ConnectedState &&
            !socket.bytesAvailable())
        {
            error = QStringLiteral("IPC connection closed before a complete response was received");
            return false;
        }
        if (!socket.bytesAvailable() && !socket.waitForReadyRead(timeoutMs)) {
            error = QStringLiteral("Timed out waiting for IPC response");
            return false;
        }
        message.append(socket.readAll());
    }
}

bool writeMessage(QLocalSocket &socket, QJsonObject const& object,
    int timeoutMs, QString &error)
{
    auto bytes = jsonMessage(object);
    if (socket.write(bytes) != bytes.size()) {
        error = QStringLiteral("Failed to write IPC response");
        return false;
    }
    if (!socket.waitForBytesWritten(timeoutMs)) {
        error = QStringLiteral("Timed out sending IPC response");
        return false;
    }
    return true;
}

void closeSocket(QLocalSocket &socket) {
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState) {
        socket.waitForDisconnected(200);
    }
}

bool jsonInt(QJsonObject const& object, QString const& key, int &value) {
    auto token = object.value(key);
    if (!token.isDouble()) {
        return false;
    }
    value = token.toInt();
    return true;
}

QJsonObject dispatchRequest(QJsonObject const& request,
    MascotCommandService &service)
{
    auto commandValue = request.value(kCommandKey);
    if (!commandValue.isString()) {
        return invalidRequest(QStringLiteral("Missing command"));
    }
    auto command = commandValue.toString();

    if (command == QStringLiteral("ping")) {
        return pingInfoToJson(service.ping());
    }

    if (command == QStringLiteral("list_mascots")) {
        ListMascotsRequest listRequest;
        auto selectorValue = request.value(kSelectorKey);
        if (selectorValue.isString()) {
            listRequest.selector = selectorValue.toString();
        }
        QList<MascotInfo> mascots;
        auto status = service.listMascots(listRequest, mascots);
        if (!status.ok()) {
            return errorToJson(status);
        }
        QJsonArray array;
        for (auto const& mascot : mascots) {
            array.append(mascotInfoToJson(mascot));
        }
        QJsonObject object;
        object["mascots"] = array;
        return object;
    }

    if (command == QStringLiteral("list_loaded_mascots")) {
        QList<LoadedMascotInfo> mascots;
        auto status = service.listLoadedMascots(mascots);
        if (!status.ok()) {
            return errorToJson(status);
        }
        QJsonArray array;
        for (auto const& mascot : mascots) {
            array.append(loadedMascotInfoToJson(mascot));
        }
        QJsonObject object;
        object["loaded_mascots"] = array;
        return object;
    }

    if (command == QStringLiteral("import_mascot_template")) {
        auto archivePathValue = request.value(kArchivePathKey);
        if (!archivePathValue.isString()) {
            return invalidRequest(QStringLiteral("archive_path must be a string"));
        }
        QList<LoadedMascotInfo> mascots;
        auto status = service.importMascotTemplate(archivePathValue.toString(),
            mascots);
        if (!status.ok()) {
            return errorToJson(status);
        }
        QJsonArray array;
        for (auto const& mascot : mascots) {
            array.append(loadedMascotInfoToJson(mascot));
        }
        QJsonObject object;
        object["loaded_mascots"] = array;
        return object;
    }

    if (command == QStringLiteral("remove_mascot_template")) {
        auto mascotNameValue = request.value(kMascotNameKey);
        if (!mascotNameValue.isString()) {
            return invalidRequest(QStringLiteral("mascot_name must be a string"));
        }
        auto status = service.removeMascotTemplate(mascotNameValue.toString());
        return status.ok() ? QJsonObject {} : errorToJson(status);
    }

    if (command == QStringLiteral("spawn_mascot")) {
        SpawnMascotRequest spawnRequest;
        QString parseError;
        if (!spawnMascotRequestFromJson(request.value(kRequestKey), spawnRequest,
            &parseError))
        {
            return invalidRequest(parseError);
        }
        MascotInfo mascot;
        auto status = service.spawnMascot(spawnRequest, mascot);
        if (!status.ok()) {
            return errorToJson(status);
        }
        QJsonObject object;
        object["mascot"] = mascotInfoToJson(mascot);
        return object;
    }

    if (command == QStringLiteral("register_cli_label")) {
        RegisterCliLabelRequest labelRequest;
        if (!jsonInt(request, kMascotIdKey, labelRequest.mascotId) ||
            labelRequest.mascotId < 0)
        {
            return invalidRequest(QStringLiteral("mascot_id must be a non-negative integer"));
        }
        auto labelValue = request.value(kLabelKey);
        if (labelValue.isDouble()) {
            labelRequest.label = labelValue.toInt();
        }
        CliLabelInfo labelInfo;
        auto status = service.registerCliLabel(labelRequest, labelInfo);
        if (!status.ok()) {
            return errorToJson(status);
        }
        return cliLabelInfoToJson(labelInfo);
    }

    if (command == QStringLiteral("get_cli_label")) {
        int label = -1;
        if (!jsonInt(request, kLabelKey, label) || label < 0) {
            return invalidRequest(QStringLiteral("label must be a non-negative integer"));
        }
        CliLabelInfo labelInfo;
        auto status = service.getCliLabel(label, labelInfo);
        if (!status.ok()) {
            return errorToJson(status);
        }
        return cliLabelInfoToJson(labelInfo);
    }

    if (command == QStringLiteral("alter_mascot")) {
        int mascotId = -1;
        if (!jsonInt(request, kMascotIdKey, mascotId) || mascotId < 0) {
            return invalidRequest(QStringLiteral("mascot_id must be a non-negative integer"));
        }
        MascotPatch patch;
        QString parseError;
        if (!mascotPatchFromJson(request.value(kPatchKey), patch, &parseError)) {
            return invalidRequest(parseError);
        }
        MascotInfo mascot;
        auto status = service.alterMascot(mascotId, patch, mascot);
        if (!status.ok()) {
            QJsonObject object = errorToJson(status);
            object["mascot"] = QJsonValue::Null;
            return object;
        }
        QJsonObject object;
        object["mascot"] = mascotInfoToJson(mascot);
        return object;
    }

    if (command == QStringLiteral("dismiss_mascot")) {
        int mascotId = -1;
        if (!jsonInt(request, kMascotIdKey, mascotId) || mascotId < 0) {
            return invalidRequest(QStringLiteral("mascot_id must be a non-negative integer"));
        }
        auto status = service.dismissMascot(mascotId);
        return status.ok() ? QJsonObject {} : errorToJson(status);
    }

    if (command == QStringLiteral("dismiss_all_mascots")) {
        DismissAllMascotsRequest dismissRequest;
        auto selectorValue = request.value(kSelectorKey);
        if (selectorValue.isString()) {
            dismissRequest.selector = selectorValue.toString();
        }
        auto status = service.dismissAllMascots(dismissRequest);
        return status.ok() ? QJsonObject {} : errorToJson(status);
    }

    if (command == QStringLiteral("stop_runtime")) {
        auto status = service.stopRuntime();
        return status.ok() ? QJsonObject {
            { QStringLiteral("stopped"), true },
        } : errorToJson(status);
    }

    return invalidRequest(QStringLiteral("Unknown command"));
}

void handleConnection(QLocalSocket *socket, MascotCommandService &service) {
    QString error;
    QByteArray bytes;
    if (!readMessage(*socket, 2000, bytes, error)) {
        APP_LOG_WARN("ipc") << error.toStdString();
        writeMessage(*socket, invalidRequest(error), 2000, error);
        closeSocket(*socket);
        return;
    }

    QJsonObject request;
    if (!parseObject(bytes, request, error)) {
        APP_LOG_WARN("ipc") << error.toStdString();
        writeMessage(*socket, invalidRequest(error), 2000, error);
        closeSocket(*socket);
        return;
    }

    auto response = dispatchRequest(request, service);
    if (!writeMessage(*socket, response, 2000, error)) {
        APP_LOG_WARN("ipc") << error.toStdString();
    }
    socket->flush();
    closeSocket(*socket);
}

bool listenWithRecovery(QLocalServer &server, QString const& serverName) {
    if (server.listen(serverName)) {
        return true;
    }
    if (shijimaLocalApiPing({ 150, 150 })) {
        return false;
    }
    QLocalServer::removeServer(serverName);
    return server.listen(serverName);
}

}

ShijimaLocalApi::ShijimaLocalApi(ShijimaManager *manager):
    m_service(manager), m_serverName(shijimaLocalApiServerName()) {}

void ShijimaLocalApi::start(QString const& serverName) {
    stop();
    m_serverName = serverName;
    m_stopRequested.store(false);
    m_thread = std::make_unique<std::thread>([this]() {
        QLocalServer server;
        if (!listenWithRecovery(server, m_serverName)) {
            APP_LOG_ERROR("ipc") << "Failed to listen on local IPC server \""
                << m_serverName.toStdString() << "\": "
                << server.errorString().toStdString();
            return;
        }
        APP_LOG_INFO("ipc") << "Local IPC server listening on \""
            << m_serverName.toStdString() << "\"";
        m_running.store(true);
        while (!m_stopRequested.load()) {
            if (!server.waitForNewConnection(100)) {
                continue;
            }
            while (server.hasPendingConnections()) {
                std::unique_ptr<QLocalSocket> socket(server.nextPendingConnection());
                if (socket == nullptr) {
                    continue;
                }
                handleConnection(socket.get(), m_service);
            }
        }
        server.close();
        QLocalServer::removeServer(m_serverName);
        m_running.store(false);
        APP_LOG_INFO("ipc") << "Local IPC server stopped";
    });
}

void ShijimaLocalApi::stop() {
    m_stopRequested.store(true);
    if (m_thread != nullptr) {
        QLocalSocket wakeSocket;
        wakeSocket.connectToServer(m_serverName);
        wakeSocket.waitForConnected(100);
        wakeSocket.disconnectFromServer();
        m_thread->join();
        m_thread.reset();
    }
    m_running.store(false);
}

bool ShijimaLocalApi::running() const {
    return m_running.load();
}

QString const& ShijimaLocalApi::serverName() const {
    return m_serverName;
}
