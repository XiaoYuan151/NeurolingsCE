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
#include "../commands/MascotCommandDispatcher.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocalServer>
#include <QLocalSocket>

namespace {

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

    auto response = MascotCommandDispatcher::dispatchRequest(request, service);
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
