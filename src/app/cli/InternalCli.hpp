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

#include "shijima-qt/MascotApi.hpp"

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

enum class CliCommandKind {
    ListMascots,
    ListLoadedMascots,
    SpawnMascot,
    AlterMascot,
    DismissMascot,
    DismissAllMascots,
};

struct CliGlobalOptions {
    bool quiet = false;
    bool json = false;
    QString host = QStringLiteral("127.0.0.1");
    int port = 32456;
    int connectTimeoutMs = 500;
    int readTimeoutMs = 500;
};

struct CliCommand {
    CliGlobalOptions global;
    CliCommandKind kind = CliCommandKind::ListMascots;
    QString idToken;
    QString selector;
    QStringList selectors;
    QStringList behaviors;
    bool sortById = false;
    SpawnMascotRequest spawnRequest;
    MascotPatch patch;
};

struct CliError {
    QString code;
    QString error;
    QString details;
    QString usage;
    int exitCode = 1;
    int httpStatus = 0;
};

struct CliExecutionResult {
    QList<MascotInfo> mascots;
    QList<LoadedMascotInfo> loadedMascots;
    std::optional<MascotInfo> mascot;
    std::optional<CliError> error;
};

struct CliParseResult {
    CliGlobalOptions global;
    std::optional<CliCommand> command;
    std::optional<CliError> error;
};

bool isCliInvocation(int argc, char **argv);
CliParseResult parseCliArguments(int argc, char **argv);
CliExecutionResult executeCliCommand(CliCommand const& command);
int writeCliOutput(CliCommand const& command, CliExecutionResult const& result);
int writeCliError(CliGlobalOptions const& global, CliError const& error);
