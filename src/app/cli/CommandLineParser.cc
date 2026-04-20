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

#include <QSet>

namespace {

QSet<QString> const& cliCommands() {
    static const QSet<QString> commands = {
        QStringLiteral("list"),
        QStringLiteral("list-loaded"),
        QStringLiteral("spawn"),
        QStringLiteral("alter"),
        QStringLiteral("dismiss"),
        QStringLiteral("dismiss-all"),
    };
    return commands;
}

bool isCliCommand(QString const& token) {
    return cliCommands().contains(token);
}

bool isBooleanGlobal(QString const& token) {
    return token == QStringLiteral("--quiet") || token == QStringLiteral("--json");
}

bool isValuedGlobal(QString const& token) {
    return token == QStringLiteral("--host") ||
        token == QStringLiteral("--port") ||
        token == QStringLiteral("--connect-timeout-ms") ||
        token == QStringLiteral("--read-timeout-ms");
}

QString generalUsage(char const *argv0) {
    return QStringLiteral(
        "Usage: %1 [--quiet] [--json] [--host HOST] [--port PORT] "
        "[--connect-timeout-ms MS] [--read-timeout-ms MS] <command> [options...]")
        .arg(QString::fromUtf8(argv0));
}

QString commandUsage(char const *argv0, QString const& command) {
    QString executable = QString::fromUtf8(argv0);
    if (command == QStringLiteral("list")) {
        return QStringLiteral("Usage: %1 [globals...] list [--selector JS] [--json]")
            .arg(executable);
    }
    if (command == QStringLiteral("list-loaded")) {
        return QStringLiteral("Usage: %1 [globals...] list-loaded [--sort-by-id] [--json]")
            .arg(executable);
    }
    if (command == QStringLiteral("spawn")) {
        return QStringLiteral(
            "Usage: %1 [globals...] spawn (--name NAME | --data-id ID) "
            "[--behavior NAME]... [--x X --y Y] [--json]").arg(executable);
    }
    if (command == QStringLiteral("alter")) {
        return QStringLiteral(
            "Usage: %1 [globals...] alter --id ID_OR_AUTO [--selector JS]... "
            "[--behavior NAME]... [--x X --y Y] [--json]").arg(executable);
    }
    if (command == QStringLiteral("dismiss")) {
        return QStringLiteral(
            "Usage: %1 [globals...] dismiss --id ID_OR_AUTO [--selector JS]")
            .arg(executable);
    }
    if (command == QStringLiteral("dismiss-all")) {
        return QStringLiteral(
            "Usage: %1 [globals...] dismiss-all [--selector JS]").arg(executable);
    }
    return generalUsage(argv0);
}

CliError parseError(CliGlobalOptions const& global, QString const& message,
    char const *argv0, QString const& command = {}, QString const& details = {})
{
    CliError error;
    error.code = QStringLiteral("invalid_arguments");
    error.error = message;
    error.details = details;
    error.usage = command.isEmpty() ? generalUsage(argv0) : commandUsage(argv0, command);
    error.exitCode = 2;
    Q_UNUSED(global);
    return error;
}

bool parseIntValue(QString const& value, int &out) {
    bool ok = false;
    int parsed = value.toInt(&ok);
    if (!ok) {
        return false;
    }
    out = parsed;
    return true;
}

bool parseDoubleValue(QString const& value, double &out) {
    bool ok = false;
    double parsed = value.toDouble(&ok);
    if (!ok) {
        return false;
    }
    out = parsed;
    return true;
}

bool applyGlobalOption(QString const& token, QString const& value,
    CliGlobalOptions &global)
{
    if (token == QStringLiteral("--host")) {
        global.host = value;
        return true;
    }
    if (token == QStringLiteral("--port")) {
        int port = 0;
        if (!parseIntValue(value, port) || port <= 0) {
            return false;
        }
        global.port = port;
        return true;
    }
    if (token == QStringLiteral("--connect-timeout-ms")) {
        int timeout = 0;
        if (!parseIntValue(value, timeout) || timeout < 0) {
            return false;
        }
        global.connectTimeoutMs = timeout;
        return true;
    }
    if (token == QStringLiteral("--read-timeout-ms")) {
        int timeout = 0;
        if (!parseIntValue(value, timeout) || timeout < 0) {
            return false;
        }
        global.readTimeoutMs = timeout;
        return true;
    }
    return false;
}

void setBooleanOption(QString const& token, CliGlobalOptions &global) {
    if (token == QStringLiteral("--quiet")) {
        global.quiet = true;
    }
    else if (token == QStringLiteral("--json")) {
        global.json = true;
    }
}

bool validateAnchor(MascotPatch const& patch) {
    return !patch.hasAnchor() || patch.hasCompleteAnchor();
}

CliCommandKind commandKindFor(QString const& command) {
    if (command == QStringLiteral("list")) {
        return CliCommandKind::ListMascots;
    }
    if (command == QStringLiteral("list-loaded")) {
        return CliCommandKind::ListLoadedMascots;
    }
    if (command == QStringLiteral("spawn")) {
        return CliCommandKind::SpawnMascot;
    }
    if (command == QStringLiteral("alter")) {
        return CliCommandKind::AlterMascot;
    }
    if (command == QStringLiteral("dismiss")) {
        return CliCommandKind::DismissMascot;
    }
    return CliCommandKind::DismissAllMascots;
}

}

bool isCliInvocation(int argc, char **argv) {
    if (argc <= 1) {
        return false;
    }
    int index = 1;
    while (index < argc) {
        QString token = QString::fromUtf8(argv[index]);
        if (isCliCommand(token)) {
            return true;
        }
        if (isBooleanGlobal(token)) {
            ++index;
            continue;
        }
        if (isValuedGlobal(token)) {
            if (index + 1 >= argc) {
                return false;
            }
            index += 2;
            continue;
        }
        return false;
    }
    return false;
}

CliParseResult parseCliArguments(int argc, char **argv) {
    CliParseResult result;
    int index = 1;
    while (index < argc) {
        QString token = QString::fromUtf8(argv[index]);
        if (isCliCommand(token)) {
            break;
        }
        if (isBooleanGlobal(token)) {
            setBooleanOption(token, result.global);
            ++index;
            continue;
        }
        if (isValuedGlobal(token)) {
            if (index + 1 >= argc) {
                result.error = parseError(result.global,
                    QStringLiteral("Missing value for %1").arg(token), argv[0]);
                return result;
            }
            QString value = QString::fromUtf8(argv[index + 1]);
            if (!applyGlobalOption(token, value, result.global)) {
                result.error = parseError(result.global,
                    QStringLiteral("Invalid value for %1").arg(token), argv[0]);
                return result;
            }
            index += 2;
            continue;
        }
        result.error = parseError(result.global,
            QStringLiteral("Unknown global option: %1").arg(token), argv[0]);
        return result;
    }

    if (index >= argc) {
        result.error = parseError(result.global, QStringLiteral("Missing command"),
            argv[0]);
        return result;
    }

    QString commandName = QString::fromUtf8(argv[index++]);
    CliCommand command;
    command.global = result.global;
    command.kind = commandKindFor(commandName);

    while (index < argc) {
        QString token = QString::fromUtf8(argv[index++]);
        if (token == QStringLiteral("--json")) {
            command.global.json = true;
            continue;
        }

        auto requireValue = [&](QString const& option, QString &value) -> bool {
            if (index >= argc) {
                result.error = parseError(command.global,
                    QStringLiteral("Missing value for %1").arg(option), argv[0],
                    commandName);
                return false;
            }
            value = QString::fromUtf8(argv[index++]);
            return true;
        };

        if (command.kind == CliCommandKind::ListMascots) {
            if (token == QStringLiteral("--selector")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.selector = value;
                continue;
            }
        }
        else if (command.kind == CliCommandKind::ListLoadedMascots) {
            if (token == QStringLiteral("--sort-by-id")) {
                command.sortById = true;
                continue;
            }
        }
        else if (command.kind == CliCommandKind::SpawnMascot) {
            if (token == QStringLiteral("--name")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.spawnRequest.name = value;
                continue;
            }
            if (token == QStringLiteral("--data-id")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                int dataId = 0;
                if (!parseIntValue(value, dataId)) {
                    result.error = parseError(command.global,
                        QStringLiteral("Invalid value for --data-id"), argv[0],
                        commandName);
                    return result;
                }
                command.spawnRequest.dataId = dataId;
                continue;
            }
            if (token == QStringLiteral("--behavior")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.behaviors.append(value);
                continue;
            }
            if (token == QStringLiteral("--x")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                double x = 0.0;
                if (!parseDoubleValue(value, x)) {
                    result.error = parseError(command.global,
                        QStringLiteral("Invalid value for --x"), argv[0],
                        commandName);
                    return result;
                }
                command.spawnRequest.patch.anchorX = x;
                continue;
            }
            if (token == QStringLiteral("--y")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                double y = 0.0;
                if (!parseDoubleValue(value, y)) {
                    result.error = parseError(command.global,
                        QStringLiteral("Invalid value for --y"), argv[0],
                        commandName);
                    return result;
                }
                command.spawnRequest.patch.anchorY = y;
                continue;
            }
        }
        else if (command.kind == CliCommandKind::AlterMascot) {
            if (token == QStringLiteral("--id")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.idToken = value;
                continue;
            }
            if (token == QStringLiteral("--selector")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.selectors.append(value);
                continue;
            }
            if (token == QStringLiteral("--behavior")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.behaviors.append(value);
                continue;
            }
            if (token == QStringLiteral("--x")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                double x = 0.0;
                if (!parseDoubleValue(value, x)) {
                    result.error = parseError(command.global,
                        QStringLiteral("Invalid value for --x"), argv[0],
                        commandName);
                    return result;
                }
                command.patch.anchorX = x;
                continue;
            }
            if (token == QStringLiteral("--y")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                double y = 0.0;
                if (!parseDoubleValue(value, y)) {
                    result.error = parseError(command.global,
                        QStringLiteral("Invalid value for --y"), argv[0],
                        commandName);
                    return result;
                }
                command.patch.anchorY = y;
                continue;
            }
        }
        else if (command.kind == CliCommandKind::DismissMascot) {
            if (token == QStringLiteral("--id")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.idToken = value;
                continue;
            }
            if (token == QStringLiteral("--selector")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.selector = value;
                continue;
            }
        }
        else if (command.kind == CliCommandKind::DismissAllMascots) {
            if (token == QStringLiteral("--selector")) {
                QString value;
                if (!requireValue(token, value)) {
                    return result;
                }
                command.selector = value;
                continue;
            }
        }

        result.error = parseError(command.global,
            QStringLiteral("Unknown option: %1").arg(token), argv[0], commandName);
        return result;
    }

    if (command.kind == CliCommandKind::ListLoadedMascots &&
        command.global.json && command.sortById)
    {
        result.error = parseError(command.global,
            QStringLiteral("--json and --sort-by-id cannot be used together."),
            argv[0], commandName);
        return result;
    }

    if (command.kind == CliCommandKind::SpawnMascot) {
        if (command.spawnRequest.name.has_value() ==
            command.spawnRequest.dataId.has_value())
        {
            result.error = parseError(command.global,
                QStringLiteral("You must specify one of name or data-id."),
                argv[0], commandName);
            return result;
        }
        if (!validateAnchor(command.spawnRequest.patch)) {
            result.error = parseError(command.global,
                QStringLiteral("X and Y must be specified together"),
                argv[0], commandName);
            return result;
        }
    }
    else if (command.kind == CliCommandKind::AlterMascot) {
        if (command.idToken.isEmpty()) {
            result.error = parseError(command.global,
                QStringLiteral("Missing required option --id"), argv[0], commandName);
            return result;
        }
        if (!validateAnchor(command.patch)) {
            result.error = parseError(command.global,
                QStringLiteral("X and Y must be specified together"),
                argv[0], commandName);
            return result;
        }
    }
    else if (command.kind == CliCommandKind::DismissMascot &&
        command.idToken.isEmpty())
    {
        result.error = parseError(command.global,
            QStringLiteral("Missing required option --id"), argv[0], commandName);
        return result;
    }

    result.global = command.global;
    result.command = command;
    return result;
}
