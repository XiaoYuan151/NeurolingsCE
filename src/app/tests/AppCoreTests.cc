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

#include "../core/commands/MascotCommandDispatcher.hpp"
#include "shijima-qt/MascotApi.hpp"
#include "shijima-qt/MascotPackage.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <cstdlib>
#include <iostream>

namespace {

int g_failures = 0;

void expect(bool condition, char const *message) {
    if (condition) {
        return;
    }
    std::cerr << "FAIL: " << message << std::endl;
    ++g_failures;
}

struct FakeMascotService {
    QString lastSelector;
    QString importedArchive;
    QString removedTemplate;
    int alteredMascotId = -1;
    int dismissedMascotId = -1;

    MascotCommandStatus listMascots(ListMascotsRequest const& request,
        QList<MascotInfo> &out)
    {
        lastSelector = request.selector;
        out.append(MascotInfo { 7, 3, QStringLiteral("Default"),
            QStringLiteral("Fall"), 2, 12.5, 42.0 });
        return MascotCommandStatus::success();
    }

    MascotCommandStatus spawnMascot(SpawnMascotRequest const& request,
        MascotInfo &out)
    {
        out.id = 8;
        out.dataId = request.dataId.value_or(3);
        out.name = request.name.value_or(QStringLiteral("Default"));
        out.anchorX = request.patch.anchorX.value_or(0.0);
        out.anchorY = request.patch.anchorY.value_or(0.0);
        out.activeBehavior = request.patch.behavior;
        return MascotCommandStatus::success();
    }

    MascotCommandStatus registerCliLabel(RegisterCliLabelRequest const& request,
        CliLabelInfo &out)
    {
        out.mascotId = request.mascotId;
        out.label = request.label.value_or(5);
        return MascotCommandStatus::success();
    }

    MascotCommandStatus getCliLabel(int cliLabel, CliLabelInfo &out) {
        out.label = cliLabel;
        out.mascotId = 8;
        return MascotCommandStatus::success();
    }

    MascotCommandStatus alterMascot(int mascotId, MascotPatch const& patch,
        MascotInfo &out)
    {
        alteredMascotId = mascotId;
        out.id = mascotId;
        out.dataId = 3;
        out.name = QStringLiteral("Default");
        out.anchorX = patch.anchorX.value_or(0.0);
        out.anchorY = patch.anchorY.value_or(0.0);
        out.activeBehavior = patch.behavior;
        return MascotCommandStatus::success();
    }

    MascotCommandStatus getMascot(int, MascotInfo &) {
        return MascotCommandStatus::success();
    }

    MascotCommandStatus dismissMascot(int mascotId) {
        dismissedMascotId = mascotId;
        return MascotCommandStatus::success();
    }

    MascotCommandStatus dismissAllMascots(DismissAllMascotsRequest const&) {
        return MascotCommandStatus::success();
    }

    MascotCommandStatus listLoadedMascots(QList<LoadedMascotInfo> &out) {
        out.append(LoadedMascotInfo { 3, QStringLiteral("Default"),
            QStringLiteral("1.0"), QStringLiteral("Bundled"), QStringLiteral("pixelomer") });
        return MascotCommandStatus::success();
    }

    MascotCommandStatus importMascotTemplate(QString const& archivePath,
        QList<LoadedMascotInfo> &out)
    {
        importedArchive = archivePath;
        out.append(LoadedMascotInfo { 4, QStringLiteral("Imported") });
        return MascotCommandStatus::success();
    }

    MascotCommandStatus removeMascotTemplate(QString const& mascotName) {
        removedTemplate = mascotName;
        return MascotCommandStatus::success();
    }

    MascotCommandStatus stopRuntime() {
        return MascotCommandStatus::success();
    }

    MascotCommandStatus showManagerWindow() {
        return MascotCommandStatus::success();
    }

    MascotCommandStatus getLoadedMascot(int, LoadedMascotInfo &) {
        return MascotCommandStatus::success();
    }

    MascotCommandStatus getLoadedMascotPreviewPng(int, QByteArray &) {
        return MascotCommandStatus::success();
    }

    ApiPingInfo ping() const {
        ApiPingInfo info;
        info.ok = true;
        info.app = QStringLiteral("NeurolingsCE");
        info.apiVersion = QStringLiteral("v1");
        return info;
    }
};

void testMascotPatchParsing() {
    MascotPatch patch;
    QString error;
    expect(!mascotPatchFromJson(QJsonObject {
        { QStringLiteral("anchor"), QJsonObject {
            { QStringLiteral("x"), 12.0 },
        } },
    }, patch, &error), "partial anchor should be rejected");

    expect(mascotPatchFromJson(QJsonObject {
        { QStringLiteral("anchor"), QJsonObject {
            { QStringLiteral("x"), 12.0 },
            { QStringLiteral("y"), 34.0 },
        } },
        { QStringLiteral("behavior"), QStringLiteral("Fall") },
    }, patch, &error), "complete patch should parse");
    expect(patch.hasCompleteAnchor(), "complete patch should have anchor");
    expect(patch.behavior.value_or(QString()) == QStringLiteral("Fall"),
        "patch should preserve behavior");
}

void testJsonRoundTrips() {
    MascotInfo mascot;
    mascot.id = 10;
    mascot.dataId = 2;
    mascot.name = QStringLiteral("Default");
    mascot.activeBehavior = QStringLiteral("Sit");
    mascot.cliLabel = 4;
    mascot.anchorX = 1.5;
    mascot.anchorY = 2.5;

    MascotInfo parsedMascot;
    QString error;
    expect(mascotInfoFromJson(mascotInfoToJson(mascot), parsedMascot, &error),
        "mascot info should round-trip");
    expect(parsedMascot.id == mascot.id && parsedMascot.dataId == mascot.dataId,
        "mascot IDs should round-trip");
    expect(parsedMascot.activeBehavior == mascot.activeBehavior,
        "active behavior should round-trip");
    expect(parsedMascot.cliLabel == mascot.cliLabel,
        "CLI label should round-trip");

    LoadedMascotInfo loaded { 3, QStringLiteral("Jenny"),
        QStringLiteral("1.2"), QStringLiteral("Test mascot"),
        QStringLiteral("author") };
    LoadedMascotInfo parsedLoaded;
    expect(loadedMascotInfoFromJson(loadedMascotInfoToJson(loaded), parsedLoaded,
        &error), "loaded mascot info should round-trip");
    expect(parsedLoaded.name == loaded.name && parsedLoaded.version == loaded.version,
        "loaded mascot metadata should round-trip");
}

void testStatusJson() {
    auto success = MascotCommandStatus::success();
    expect(success.ok(), "success status should be OK");

    auto failure = MascotCommandStatus::failure(404,
        QStringLiteral("missing"), QStringLiteral("Not found"));
    auto json = errorToJson(failure);
    expect(!failure.ok(), "failure status should not be OK");
    expect(json.value(QStringLiteral("status")).toInt() == 404,
        "failure JSON should include status");
    expect(json.value(QStringLiteral("code")).toString() == QStringLiteral("missing"),
        "failure JSON should include code");
}

void testMascotPackageNames() {
    expect(MascotPackage::sanitizedPackageBaseName(QStringLiteral("  Bad<>:\"/\\|?*.  ")) ==
        QStringLiteral("Bad_________"), "package name should strip invalid characters and trailing dots");
    expect(MascotPackage::sanitizedPackageBaseName(QString()) ==
        QStringLiteral("Mascot"), "empty package name should use fallback");
    expect(MascotPackage::sanitizedPackageBaseName(QStringLiteral("../Name")) ==
        QStringLiteral(".._Name"), "path separators should not survive package names");
}

void testCommandDispatcher() {
    FakeMascotService service;

    auto missing = MascotCommandDispatcher::dispatchRequest({}, service);
    expect(missing.value(QStringLiteral("code")).toString() == QStringLiteral("bad_request"),
        "dispatcher should reject missing command");

    auto unknown = MascotCommandDispatcher::dispatchRequest(QJsonObject {
        { QStringLiteral("command"), QStringLiteral("unknown") },
    }, service);
    expect(unknown.value(QStringLiteral("error")).toString() == QStringLiteral("Unknown command"),
        "dispatcher should reject unknown command");

    auto ping = MascotCommandDispatcher::dispatchRequest(QJsonObject {
        { QStringLiteral("command"), QStringLiteral("ping") },
    }, service);
    expect(ping.value(QStringLiteral("ok")).toBool(false),
        "dispatcher should return ping payload");

    auto list = MascotCommandDispatcher::dispatchRequest(QJsonObject {
        { QStringLiteral("command"), QStringLiteral("list_mascots") },
        { QStringLiteral("selector"), QStringLiteral("mascot.anchor.x > 0") },
    }, service);
    expect(service.lastSelector == QStringLiteral("mascot.anchor.x > 0"),
        "dispatcher should pass list selector");
    expect(list.value(QStringLiteral("mascots")).toArray().size() == 1,
        "dispatcher should serialize listed mascots");

    auto spawn = MascotCommandDispatcher::dispatchRequest(QJsonObject {
        { QStringLiteral("command"), QStringLiteral("spawn_mascot") },
        { QStringLiteral("request"), QJsonObject {
            { QStringLiteral("name"), QStringLiteral("Default") },
            { QStringLiteral("anchor"), QJsonObject {
                { QStringLiteral("x"), 5.0 },
                { QStringLiteral("y"), 6.0 },
            } },
        } },
    }, service);
    expect(spawn.value(QStringLiteral("mascot")).toObject()
        .value(QStringLiteral("anchor")).toObject()
        .value(QStringLiteral("x")).toDouble() == 5.0,
        "dispatcher should parse spawn request");
}

}

int main() {
    testMascotPatchParsing();
    testJsonRoundTrips();
    testStatusJson();
    testMascotPackageNames();
    testCommandDispatcher();

    if (g_failures > 0) {
        std::cerr << g_failures << " test(s) failed" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "All app core tests passed" << std::endl;
    return EXIT_SUCCESS;
}
