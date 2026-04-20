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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

void printMascot(MascotInfo const& mascot) {
    std::cout << "[" << mascot.id << "] "
        << mascot.name.toStdString() << std::endl;
    std::cout << "  Data ID: " << mascot.dataId << std::endl;
    std::cout << "  Active behavior: "
        << mascot.activeBehavior.value_or(QString {}).toStdString() << std::endl;
    std::cout << "  Anchor: {" << mascot.anchorX << ", "
        << mascot.anchorY << "}" << std::endl;
}

QJsonObject successJson(CliCommand const& command,
    CliExecutionResult const& result)
{
    QJsonObject object;
    if (command.kind == CliCommandKind::ListMascots) {
        QJsonArray array;
        for (auto const& mascot : result.mascots) {
            array.append(mascotInfoToJson(mascot));
        }
        object["mascots"] = array;
    }
    else if (command.kind == CliCommandKind::ListLoadedMascots) {
        QJsonArray array;
        for (auto const& mascot : result.loadedMascots) {
            array.append(loadedMascotInfoToJson(mascot));
        }
        object["loaded_mascots"] = array;
    }
    else if (result.mascot.has_value()) {
        object["mascot"] = mascotInfoToJson(result.mascot.value());
    }
    return object;
}

QJsonObject errorJson(CliError const& error) {
    QJsonObject object;
    object["error"] = error.error;
    object["code"] = error.code;
    if (!error.details.isEmpty()) {
        object["details"] = error.details;
    }
    if (!error.usage.isEmpty()) {
        object["usage"] = error.usage;
    }
    if (error.httpStatus > 0) {
        object["status"] = error.httpStatus;
    }
    return object;
}

void writeJson(QJsonObject const& object) {
    QJsonDocument document(object);
    std::cout << document.toJson(QJsonDocument::Compact).constData()
        << std::endl;
}

}

int writeCliError(CliGlobalOptions const& global, CliError const& error) {
    if (global.json) {
        writeJson(errorJson(error));
    }
    else {
        std::cerr << "ERROR: " << error.error.toStdString() << std::endl;
        if (!error.details.isEmpty()) {
            std::cerr << error.details.toStdString() << std::endl;
        }
        if (!error.usage.isEmpty()) {
            std::cerr << error.usage.toStdString() << std::endl;
        }
    }
    return error.exitCode;
}

int writeCliOutput(CliCommand const& command, CliExecutionResult const& result) {
    if (result.error.has_value()) {
        return writeCliError(command.global, result.error.value());
    }

    if (command.global.quiet) {
        return EXIT_SUCCESS;
    }

    if (command.global.json) {
        writeJson(successJson(command, result));
        return EXIT_SUCCESS;
    }

    if (command.kind == CliCommandKind::ListMascots) {
        for (auto const& mascot : result.mascots) {
            printMascot(mascot);
        }
        return EXIT_SUCCESS;
    }

    if (command.kind == CliCommandKind::ListLoadedMascots) {
        std::vector<LoadedMascotInfo> mascots(result.loadedMascots.begin(),
            result.loadedMascots.end());
        if (command.sortById) {
            std::sort(mascots.begin(), mascots.end(),
                [](LoadedMascotInfo const& lhs, LoadedMascotInfo const& rhs) {
                    return lhs.id < rhs.id;
                });
        }
        for (auto const& mascot : mascots) {
            std::cout << "[" << mascot.id << "] "
                << mascot.name.toStdString() << std::endl;
        }
        return EXIT_SUCCESS;
    }

    if (result.mascot.has_value()) {
        printMascot(result.mascot.value());
    }
    return EXIT_SUCCESS;
}
