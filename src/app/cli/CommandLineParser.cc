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

QSet<QString> const& legacyCommands() {
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

QSet<QString> const& documentCommands() {
    static const QSet<QString> commands = {
        QStringLiteral("--help"),
        QStringLiteral("-h"),
        QStringLiteral("--summon"),
        QStringLiteral("-s"),
        QStringLiteral("--close"),
        QStringLiteral("--close-all"),
        QStringLiteral("--stop"),
        QStringLiteral("--mascot"),
        QStringLiteral("-m"),
        QStringLiteral("--list"),
        QStringLiteral("-l"),
        QStringLiteral("--version"),
        QStringLiteral("-v"),
    };
    return commands;
}

bool isLegacyCommand(QString const& token) {
    return legacyCommands().contains(token);
}

bool isDocumentCommand(QString const& token) {
    return documentCommands().contains(token);
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

QString helpSynopsis(char const *argv0) {
    return QStringLiteral(
        "Usage: %1 [--quiet] [--json] "
        "[--connect-timeout-ms MS] [--read-timeout-ms MS] <command>")
        .arg(QString::fromUtf8(argv0));
}

QString documentHelpText(char const *argv0) {
    QString executable = QString::fromUtf8(argv0);
    return QStringLiteral(
        "%1\n"
        "\n"
        "Document commands:\n"
        "  %2 --help|-h\n"
        "  %2 --version|-v\n"
        "  %2 --list|-l\n"
        "  %2 --summon|-s mascot --name NAME [label]\n"
        "  %2 --summon|-s mascot --data-id ID [label]\n"
        "  %2 --summon|-s random [label]\n"
        "  %2 --close LABEL\n"
        "  %2 --close-all\n"
        "  %2 --stop\n"
        "  %2 --mascot|-m list\n"
        "  %2 --mascot|-m add ZIP\n"
        "  %2 --mascot|-m remove MASCOT\n"
        "\n"
        "Global options:\n"
        "  --quiet  --json  --connect-timeout-ms MS  --read-timeout-ms MS\n"
        "\n"
        "Transport notes:\n"
        "  Runtime commands auto-start a local runtime when needed.\n"
        "  Commands use local IPC and do not use HTTP.\n"
        "  --host and --port are no longer supported.\n"
        "\n"
        "Legacy commands remain supported:\n"
        "  list, list-loaded, spawn, alter, dismiss, dismiss-all")
        .arg(helpSynopsis(argv0), executable);
}

QString commandUsage(char const *argv0, QString const& commandName) {
    QString executable = QString::fromUtf8(argv0);
    if (commandName == QStringLiteral("list")) {
        return QStringLiteral("Usage: %1 [globals...] list [--selector JS] [--json]")
            .arg(executable);
    }
    if (commandName == QStringLiteral("list-loaded")) {
        return QStringLiteral("Usage: %1 [globals...] list-loaded [--sort-by-id] [--json]")
            .arg(executable);
    }
    if (commandName == QStringLiteral("spawn")) {
        return QStringLiteral(
            "Usage: %1 [globals...] spawn (--name NAME | --data-id ID) "
            "[--behavior NAME]... [--x X --y Y] [--json]").arg(executable);
    }
    if (commandName == QStringLiteral("alter")) {
        return QStringLiteral(
            "Usage: %1 [globals...] alter --id ID_OR_AUTO [--selector JS]... "
            "[--behavior NAME]... [--x X --y Y] [--json]").arg(executable);
    }
    if (commandName == QStringLiteral("dismiss")) {
        return QStringLiteral(
            "Usage: %1 [globals...] dismiss --id ID_OR_AUTO [--selector JS]")
            .arg(executable);
    }
    if (commandName == QStringLiteral("dismiss-all")) {
        return QStringLiteral("Usage: %1 [globals...] dismiss-all [--selector JS]")
            .arg(executable);
    }
    if (commandName == QStringLiteral("--summon")) {
        return QStringLiteral(
            "Usage: %1 [globals...] --summon|-s mascot (--name NAME | --data-id ID) [label]\n"
            "       %1 [globals...] --summon|-s random [label]").arg(executable);
    }
    if (commandName == QStringLiteral("--close")) {
        return QStringLiteral("Usage: %1 [globals...] --close LABEL").arg(executable);
    }
    if (commandName == QStringLiteral("--close-all")) {
        return QStringLiteral("Usage: %1 [globals...] --close-all").arg(executable);
    }
    if (commandName == QStringLiteral("--stop")) {
        return QStringLiteral("Usage: %1 [globals...] --stop").arg(executable);
    }
    if (commandName == QStringLiteral("--mascot")) {
        return QStringLiteral(
            "Usage: %1 [globals...] --mascot|-m list\n"
            "       %1 [globals...] --mascot|-m add ZIP\n"
            "       %1 [globals...] --mascot|-m remove MASCOT").arg(executable);
    }
    if (commandName == QStringLiteral("--list")) {
        return QStringLiteral("Usage: %1 [globals...] --list|-l").arg(executable);
    }
    if (commandName == QStringLiteral("--version")) {
        return QStringLiteral("Usage: %1 [globals...] --version|-v").arg(executable);
    }
    return documentHelpText(argv0);
}

CliError parseError(CliGlobalOptions const&, QString const& message,
    char const *argv0, QString const& commandName = {},
    QString const& details = {})
{
    CliError error;
    error.code = QStringLiteral("invalid_arguments");
    error.error = message;
    error.details = details;
    error.usage = commandName.isEmpty() ? documentHelpText(argv0)
        : commandUsage(argv0, commandName);
    error.exitCode = 2;
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

bool parseOptionalLabel(QString const& value, std::optional<int> &out) {
    int label = 0;
    if (!parseIntValue(value, label) || label < 0) {
        return false;
    }
    out = label;
    return true;
}

void setBooleanGlobal(QString const& token, CliGlobalOptions &global) {
    if (token == QStringLiteral("--quiet")) {
        global.quiet = true;
    }
    else if (token == QStringLiteral("--json")) {
        global.json = true;
    }
}

bool applyGlobalOption(QString const& token, QString const& value,
    CliGlobalOptions &global)
{
    if (token == QStringLiteral("--host") || token == QStringLiteral("--port")) {
        (void)value;
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

bool validateAnchor(MascotPatch const& patch) {
    return !patch.hasAnchor() || patch.hasCompleteAnchor();
}

CliCommandKind documentCommandKind(QString const& token) {
    if (token == QStringLiteral("--help") || token == QStringLiteral("-h")) {
        return CliCommandKind::Help;
    }
    if (token == QStringLiteral("--version") || token == QStringLiteral("-v")) {
        return CliCommandKind::Version;
    }
    if (token == QStringLiteral("--list") || token == QStringLiteral("-l")) {
        return CliCommandKind::DocumentList;
    }
    if (token == QStringLiteral("--summon") || token == QStringLiteral("-s")) {
        return CliCommandKind::DocumentSummon;
    }
    if (token == QStringLiteral("--close")) {
        return CliCommandKind::DocumentClose;
    }
    if (token == QStringLiteral("--stop")) {
        return CliCommandKind::DocumentStop;
    }
    if (token == QStringLiteral("--mascot") || token == QStringLiteral("-m")) {
        return CliCommandKind::DocumentMascot;
    }
    return CliCommandKind::DocumentCloseAll;
}

CliCommandKind legacyCommandKind(QString const& token) {
    if (token == QStringLiteral("list")) {
        return CliCommandKind::ListMascots;
    }
    if (token == QStringLiteral("list-loaded")) {
        return CliCommandKind::ListLoadedMascots;
    }
    if (token == QStringLiteral("spawn")) {
        return CliCommandKind::SpawnMascot;
    }
    if (token == QStringLiteral("alter")) {
        return CliCommandKind::AlterMascot;
    }
    if (token == QStringLiteral("dismiss")) {
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
        if (isLegacyCommand(token) || isDocumentCommand(token)) {
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
    CliParseResult result {};
    int index = 1;
    while (index < argc) {
        QString token = QString::fromUtf8(argv[index]);
        if (isLegacyCommand(token) || isDocumentCommand(token)) {
            break;
        }
        if (isBooleanGlobal(token)) {
            setBooleanGlobal(token, result.global);
            ++index;
            continue;
        }
        if (isValuedGlobal(token)) {
            if (index + 1 >= argc) {
                result.error = parseError(result.global,
                    QStringLiteral("Missing value for %1").arg(token), argv[0]);
                result.hasError = true;
                return result;
            }
            QString value = QString::fromUtf8(argv[index + 1]);
            if (!applyGlobalOption(token, value, result.global)) {
                result.error = parseError(result.global,
                    QStringLiteral("Invalid value for %1").arg(token), argv[0]);
                result.hasError = true;
                return result;
            }
            if (token == QStringLiteral("--host") || token == QStringLiteral("--port")) {
                result.error = parseError(result.global,
                    QStringLiteral("%1 is not supported by the local IPC CLI")
                        .arg(token),
                    argv[0], {}, QStringLiteral("Use the local running instance instead of host/port routing."));
                result.hasError = true;
                return result;
            }
            index += 2;
            continue;
        }
        result.error = parseError(result.global,
            QStringLiteral("Unknown global option: %1").arg(token), argv[0]);
        result.hasError = true;
        return result;
    }

    if (index >= argc) {
        result.error = parseError(result.global, QStringLiteral("Missing command"),
            argv[0]);
        result.hasError = true;
        return result;
    }

    QString commandToken = QString::fromUtf8(argv[index++]);
    CliCommand &command = result.command;
    result.hasCommand = true;
    command.global = result.global;
    command.commandName = commandToken;

    if (isDocumentCommand(commandToken)) {
        command.documentStyle = true;
        command.kind = documentCommandKind(commandToken);

        if (command.kind == CliCommandKind::Help ||
            command.kind == CliCommandKind::Version ||
            command.kind == CliCommandKind::DocumentList ||
            command.kind == CliCommandKind::DocumentCloseAll ||
            command.kind == CliCommandKind::DocumentStop)
        {
            if (index < argc) {
                result.error = parseError(command.global,
                    QStringLiteral("Unexpected argument: %1")
                        .arg(QString::fromUtf8(argv[index])),
                    argv[0], commandToken);
                result.hasError = true;
                return result;
            }
            result.global = command.global;
            return result;
        }

        if (command.kind == CliCommandKind::DocumentMascot) {
            if (index >= argc) {
                result.error = parseError(command.global,
                    QStringLiteral("Missing mascot command"), argv[0],
                    commandToken);
                result.hasError = true;
                return result;
            }
            command.mascotAction = QString::fromUtf8(argv[index++]);
            if (command.mascotAction == QStringLiteral("list")) {
                if (index < argc) {
                    result.error = parseError(command.global,
                        QStringLiteral("Unexpected argument: %1")
                            .arg(QString::fromUtf8(argv[index])),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                result.global = command.global;
                return result;
            }
            if (command.mascotAction == QStringLiteral("add")) {
                if (index >= argc) {
                    result.error = parseError(command.global,
                        QStringLiteral("Missing mascot archive path"),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                command.mascotArchivePath = QString::fromUtf8(argv[index++]);
                if (index < argc) {
                    result.error = parseError(command.global,
                        QStringLiteral("Unexpected argument: %1")
                            .arg(QString::fromUtf8(argv[index])),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                result.global = command.global;
                return result;
            }
            if (command.mascotAction == QStringLiteral("remove")) {
                if (index >= argc) {
                    result.error = parseError(command.global,
                        QStringLiteral("Missing mascot template name"),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                command.mascotTemplateName = QString::fromUtf8(argv[index++]);
                if (index < argc) {
                    result.error = parseError(command.global,
                        QStringLiteral("Unexpected argument: %1")
                            .arg(QString::fromUtf8(argv[index])),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                result.global = command.global;
                return result;
            }
            result.error = parseError(command.global,
                QStringLiteral("Mascot command must be list, add, or remove"),
                argv[0], commandToken);
            result.hasError = true;
            return result;
        }

        if (command.kind == CliCommandKind::DocumentClose) {
            if (index >= argc) {
                result.error = parseError(command.global,
                    QStringLiteral("Missing CLI label"), argv[0], commandToken);
                result.hasError = true;
                return result;
            }
            if (!parseOptionalLabel(QString::fromUtf8(argv[index++]), command.cliLabel)) {
                result.error = parseError(command.global,
                    QStringLiteral("CLI label must be a non-negative integer"),
                    argv[0], commandToken);
                result.hasError = true;
                return result;
            }
            if (index < argc) {
                result.error = parseError(command.global,
                    QStringLiteral("Unexpected argument: %1")
                        .arg(QString::fromUtf8(argv[index])),
                    argv[0], commandToken);
                result.hasError = true;
                return result;
            }
            result.global = command.global;
            return result;
        }

        if (index >= argc) {
            result.error = parseError(command.global,
                QStringLiteral("Missing summon mode"), argv[0], commandToken);
            result.hasError = true;
            return result;
        }
        command.summonMode = QString::fromUtf8(argv[index++]);
        if (command.summonMode != QStringLiteral("mascot") &&
            command.summonMode != QStringLiteral("random"))
        {
            result.error = parseError(command.global,
                QStringLiteral("Summon mode must be mascot or random"),
                argv[0], commandToken);
            result.hasError = true;
            return result;
        }

        while (index < argc) {
            QString token = QString::fromUtf8(argv[index]);
            if (token == QStringLiteral("--name")) {
                ++index;
                if (index >= argc) {
                    result.error = parseError(command.global,
                        QStringLiteral("Missing value for --name"),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                command.spawnRequest.name = QString::fromUtf8(argv[index++]);
                continue;
            }
            if (token == QStringLiteral("--data-id")) {
                ++index;
                if (index >= argc) {
                    result.error = parseError(command.global,
                        QStringLiteral("Missing value for --data-id"),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                int dataId = 0;
                if (!parseIntValue(QString::fromUtf8(argv[index++]), dataId)) {
                    result.error = parseError(command.global,
                        QStringLiteral("Invalid value for --data-id"),
                        argv[0], commandToken);
                    result.hasError = true;
                    return result;
                }
                command.spawnRequest.dataId = dataId;
                continue;
            }
            std::optional<int> label;
            if (!parseOptionalLabel(token, label)) {
                result.error = parseError(command.global,
                    QStringLiteral("Unexpected argument: %1").arg(token),
                    argv[0], commandToken);
                result.hasError = true;
                return result;
            }
            command.cliLabel = label;
            ++index;
            if (index < argc) {
                result.error = parseError(command.global,
                    QStringLiteral("Unexpected argument: %1")
                        .arg(QString::fromUtf8(argv[index])),
                    argv[0], commandToken);
                result.hasError = true;
                return result;
            }
        }

        if (command.summonMode == QStringLiteral("mascot")) {
            if (command.spawnRequest.name.has_value() ==
                command.spawnRequest.dataId.has_value())
            {
                result.error = parseError(command.global,
                    QStringLiteral("You must specify one of --name or --data-id"),
                    argv[0], commandToken);
                result.hasError = true;
                return result;
            }
        }
        else if (command.spawnRequest.name.has_value() ||
            command.spawnRequest.dataId.has_value())
        {
            result.error = parseError(command.global,
                QStringLiteral("random summon does not accept --name or --data-id"),
                argv[0], commandToken);
            result.hasError = true;
            return result;
        }

        result.global = command.global;
        return result;
    }

    command.kind = legacyCommandKind(commandToken);
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
                    commandToken);
                result.hasError = true;
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
                        commandToken);
                    result.hasError = true;
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
                        commandToken);
                    result.hasError = true;
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
                        commandToken);
                    result.hasError = true;
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
                        commandToken);
                    result.hasError = true;
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
                        commandToken);
                    result.hasError = true;
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
            QStringLiteral("Unknown option: %1").arg(token), argv[0], commandToken);
        result.hasError = true;
        return result;
    }

    if (command.kind == CliCommandKind::ListLoadedMascots &&
        command.global.json && command.sortById)
    {
        result.error = parseError(command.global,
            QStringLiteral("--json and --sort-by-id cannot be used together."),
            argv[0], commandToken);
        result.hasError = true;
        return result;
    }

    if (command.kind == CliCommandKind::SpawnMascot) {
        if (command.spawnRequest.name.has_value() ==
            command.spawnRequest.dataId.has_value())
        {
            result.error = parseError(command.global,
                QStringLiteral("You must specify one of name or data-id."),
                argv[0], commandToken);
            result.hasError = true;
            return result;
        }
        if (!validateAnchor(command.spawnRequest.patch)) {
            result.error = parseError(command.global,
                QStringLiteral("X and Y must be specified together"),
                argv[0], commandToken);
            result.hasError = true;
            return result;
        }
    }
    else if (command.kind == CliCommandKind::AlterMascot) {
        if (command.idToken.isEmpty()) {
            result.error = parseError(command.global,
                QStringLiteral("Missing required option --id"), argv[0], commandToken);
            result.hasError = true;
            return result;
        }
        if (!validateAnchor(command.patch)) {
            result.error = parseError(command.global,
                QStringLiteral("X and Y must be specified together"),
                argv[0], commandToken);
            result.hasError = true;
            return result;
        }
    }
    else if (command.kind == CliCommandKind::DismissMascot &&
        command.idToken.isEmpty())
    {
        result.error = parseError(command.global,
            QStringLiteral("Missing required option --id"), argv[0], commandToken);
        result.hasError = true;
        return result;
    }

    result.global = command.global;
    return result;
}
