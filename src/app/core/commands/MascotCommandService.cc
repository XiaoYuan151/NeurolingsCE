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

#include "shijima-qt/MascotCommandService.hpp"

#include "shijima-qt/MascotData.hpp"
#include "shijima-qt/ShijimaManager.hpp"
#include "shijima-qt/ui/mascot/ShijimaWidget.hpp"
#include <QBuffer>
#include <QByteArray>
#include <QMetaObject>

namespace {

MascotInfo buildMascotInfo(ShijimaWidget *widget) {
    MascotInfo info;
    info.id = widget->mascotId();
    info.dataId = widget->mascotData()->id();
    info.name = widget->mascotData()->name();
    info.anchorX = widget->mascot().state->anchor.x;
    info.anchorY = widget->mascot().state->anchor.y;
    auto activeBehavior = widget->mascot().active_behavior();
    if (activeBehavior != nullptr) {
        info.activeBehavior = QString::fromStdString(activeBehavior->name);
    }
    return info;
}

CliLabelInfo buildCliLabelInfo(int label, int mascotId) {
    CliLabelInfo info;
    info.label = label;
    info.mascotId = mascotId;
    return info;
}

LoadedMascotInfo buildLoadedMascotInfo(MascotData *data) {
    LoadedMascotInfo info;
    info.id = data->id();
    info.name = data->name();
    return info;
}

bool selectorEval(ShijimaWidget *mascot, QString const& selector) {
    if (selector.isEmpty()) {
        return true;
    }
    bool eval;
    try {
        mascot->mascot().script_ctx->state = mascot->mascot().state;
        eval = mascot->mascot().script_ctx->eval_bool(selector.toStdString());
    }
    catch (...) {
        eval = false;
    }
    return eval;
}

void applyPatchToWidget(MascotPatch const& patch, ShijimaWidget *widget) {
    if (patch.hasCompleteAnchor()) {
        widget->mascot().state->anchor.x = patch.anchorX.value();
        widget->mascot().state->anchor.y = patch.anchorY.value();
    }
    if (patch.behavior.has_value()) {
        auto behaviorName = patch.behavior.value().toStdString();
        auto behavior = widget->mascot()
            .initial_behavior_list().find(behaviorName, false);
        if (behavior != nullptr) {
            widget->mascot().next_behavior(behaviorName);
        }
    }
}

QString resolveMascotName(ShijimaManager *manager,
    SpawnMascotRequest const& request)
{
    if (request.name.has_value()) {
        auto name = request.name.value();
        if (manager->loadedMascots().contains(name)) {
            return name;
        }
        return {};
    }
    if (request.dataId.has_value()) {
        int dataId = request.dataId.value();
        if (manager->loadedMascotsById().contains(dataId)) {
            return manager->loadedMascotsById()[dataId]->name();
        }
    }
    return {};
}

MascotCommandStatus mascotNotFoundStatus() {
    return MascotCommandStatus::failure(404, QStringLiteral("mascot_not_found"),
        QStringLiteral("No such mascot"));
}

MascotCommandStatus loadedMascotNotFoundStatus() {
    return MascotCommandStatus::failure(404,
        QStringLiteral("loaded_mascot_not_found"),
        QStringLiteral("No such loaded mascot"));
}

MascotCommandStatus mascotTemplateNotFoundStatus() {
    return MascotCommandStatus::failure(404,
        QStringLiteral("mascot_template_not_found"),
        QStringLiteral("No such mascot template"));
}

MascotCommandStatus cliLabelNotFoundStatus() {
    return MascotCommandStatus::failure(404,
        QStringLiteral("cli_label_not_found"),
        QStringLiteral("No such CLI label"));
}

}

MascotCommandService::MascotCommandService(ShijimaManager *manager):
    m_manager(manager) {}

MascotCommandStatus MascotCommandService::listMascots(
    ListMascotsRequest const& request, QList<MascotInfo> &out) const
{
    out.clear();
    m_manager->onTickSync([&out, &request](ShijimaManager *manager) {
        for (auto mascot : manager->mascots()) {
            if (!selectorEval(mascot, request.selector)) {
                continue;
            }
            auto info = buildMascotInfo(mascot);
            info.cliLabel = manager->cliLabelForMascot(info.id);
            out.append(info);
        }
    });
    return MascotCommandStatus::success();
}

MascotCommandStatus MascotCommandService::spawnMascot(
    SpawnMascotRequest const& request, MascotInfo &out) const
{
    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, &request, &out](ShijimaManager *manager) {
        auto mascotName = resolveMascotName(manager, request);
        if (mascotName.isEmpty()) {
            status = MascotCommandStatus::failure(400,
                QStringLiteral("invalid_mascot"),
                QStringLiteral("Invalid mascot name or data ID"));
            return;
        }
        auto widget = manager->spawn(mascotName.toStdString());
        if (widget == nullptr) {
            status = MascotCommandStatus::failure(500,
                QStringLiteral("spawn_failed"),
                QStringLiteral("Failed to spawn mascot"));
            return;
        }
        applyPatchToWidget(request.patch, widget);
        out = buildMascotInfo(widget);
        out.cliLabel = manager->cliLabelForMascot(out.id);
    });
    return status;
}

MascotCommandStatus MascotCommandService::registerCliLabel(
    RegisterCliLabelRequest const& request, CliLabelInfo &out) const
{
    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, &request, &out](ShijimaManager *manager) {
        QString errorMessage;
        int assignedLabel = -1;
        if (!manager->assignCliLabel(request.mascotId, request.label,
            assignedLabel, errorMessage))
        {
            int httpStatus = 400;
            QString code = QStringLiteral("invalid_cli_label");
            if (errorMessage == QStringLiteral("No such mascot")) {
                status = mascotNotFoundStatus();
                return;
            }
            if (errorMessage == QStringLiteral("CLI label is already in use")) {
                code = QStringLiteral("cli_label_conflict");
            }
            status = MascotCommandStatus::failure(httpStatus, code, errorMessage);
            return;
        }
        out = buildCliLabelInfo(assignedLabel, request.mascotId);
    });
    return status;
}

MascotCommandStatus MascotCommandService::getCliLabel(int cliLabel,
    CliLabelInfo &out) const
{
    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, cliLabel, &out](ShijimaManager *manager) {
        auto mascotId = manager->mascotIdForCliLabel(cliLabel);
        if (!mascotId.has_value()) {
            status = cliLabelNotFoundStatus();
            return;
        }
        out = buildCliLabelInfo(cliLabel, mascotId.value());
    });
    return status;
}

MascotCommandStatus MascotCommandService::alterMascot(int mascotId,
    MascotPatch const& patch, MascotInfo &out) const
{
    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, mascotId, &patch, &out](ShijimaManager *manager) {
        if (manager->mascotsById().count(mascotId) != 1) {
            status = mascotNotFoundStatus();
            return;
        }
        auto widget = manager->mascotsById().at(mascotId);
        applyPatchToWidget(patch, widget);
        out = buildMascotInfo(widget);
        out.cliLabel = manager->cliLabelForMascot(mascotId);
    });
    return status;
}

MascotCommandStatus MascotCommandService::getMascot(int mascotId,
    MascotInfo &out) const
{
    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, mascotId, &out](ShijimaManager *manager) {
        if (manager->mascotsById().count(mascotId) != 1) {
            status = mascotNotFoundStatus();
            return;
        }
        out = buildMascotInfo(manager->mascotsById().at(mascotId));
        out.cliLabel = manager->cliLabelForMascot(mascotId);
    });
    return status;
}

MascotCommandStatus MascotCommandService::dismissMascot(int mascotId) const {
    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, mascotId](ShijimaManager *manager) {
        if (manager->mascotsById().count(mascotId) != 1) {
            status = mascotNotFoundStatus();
            return;
        }
        manager->clearCliLabelForMascot(mascotId);
        manager->mascotsById().at(mascotId)->markForDeletion();
    });
    return status;
}

MascotCommandStatus MascotCommandService::dismissAllMascots(
    DismissAllMascotsRequest const& request) const
{
    m_manager->onTickSync([&request](ShijimaManager *manager) {
        for (auto mascot : manager->mascots()) {
            if (!selectorEval(mascot, request.selector)) {
                continue;
            }
            manager->clearCliLabelForMascot(mascot->mascotId());
            mascot->markForDeletion();
        }
        if (request.selector.isEmpty()) {
            manager->clearCliLabels();
        }
    });
    return MascotCommandStatus::success();
}

MascotCommandStatus MascotCommandService::listLoadedMascots(
    QList<LoadedMascotInfo> &out) const
{
    out.clear();
    m_manager->onTickSync([&out](ShijimaManager *manager) {
        for (auto mascot : manager->loadedMascots()) {
            out.append(buildLoadedMascotInfo(mascot));
        }
    });
    return MascotCommandStatus::success();
}

MascotCommandStatus MascotCommandService::importMascotTemplate(
    QString const& archivePath, QList<LoadedMascotInfo> &out) const
{
    out.clear();
    if (archivePath.isEmpty()) {
        return MascotCommandStatus::failure(400,
            QStringLiteral("invalid_archive"),
            QStringLiteral("Archive path is required"));
    }

    auto changed = m_manager->import(archivePath);
    if (changed.empty()) {
        return MascotCommandStatus::failure(400,
            QStringLiteral("import_failed"),
            QStringLiteral("Could not import any mascots from the specified archive"));
    }

    m_manager->onTickSync([&out, &changed](ShijimaManager *manager) {
        manager->reloadMascots(changed);
        for (auto const& mascotName : changed) {
            auto name = QString::fromStdString(mascotName);
            if (!manager->loadedMascots().contains(name)) {
                continue;
            }
            out.append(buildLoadedMascotInfo(manager->loadedMascots()[name]));
        }
    });

    if (out.isEmpty()) {
        return MascotCommandStatus::failure(500,
            QStringLiteral("import_failed"),
            QStringLiteral("Imported mascot archive but no templates were loaded"));
    }
    return MascotCommandStatus::success();
}

MascotCommandStatus MascotCommandService::removeMascotTemplate(
    QString const& mascotName) const
{
    if (mascotName.isEmpty()) {
        return MascotCommandStatus::failure(400,
            QStringLiteral("invalid_mascot_template"),
            QStringLiteral("Mascot template name is required"));
    }

    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, &mascotName](ShijimaManager *manager) {
        QString errorMessage;
        if (manager->removeMascotTemplate(mascotName, errorMessage)) {
            return;
        }
        if (errorMessage == QStringLiteral("No such mascot template")) {
            status = mascotTemplateNotFoundStatus();
            return;
        }
        QString code = QStringLiteral("remove_failed");
        int httpStatus = 400;
        if (errorMessage == QStringLiteral("Mascot template cannot be deleted")) {
            code = QStringLiteral("mascot_template_not_deletable");
        }
        status = MascotCommandStatus::failure(httpStatus, code, errorMessage);
    });
    return status;
}

MascotCommandStatus MascotCommandService::stopRuntime() const {
    QMetaObject::invokeMethod(m_manager, [manager = m_manager]() {
        manager->quitAction();
    }, Qt::QueuedConnection);
    return MascotCommandStatus::success();
}

MascotCommandStatus MascotCommandService::getLoadedMascot(int mascotId,
    LoadedMascotInfo &out) const
{
    auto status = MascotCommandStatus::success();
    m_manager->onTickSync([&status, mascotId, &out](ShijimaManager *manager) {
        if (!manager->loadedMascotsById().contains(mascotId)) {
            status = loadedMascotNotFoundStatus();
            return;
        }
        out = buildLoadedMascotInfo(manager->loadedMascotsById()[mascotId]);
    });
    return status;
}

MascotCommandStatus MascotCommandService::getLoadedMascotPreviewPng(int mascotId,
    QByteArray &pngBytes) const
{
    auto status = MascotCommandStatus::success();
    pngBytes.clear();
    m_manager->onTickSync([&status, mascotId, &pngBytes](ShijimaManager *manager) {
        if (!manager->loadedMascotsById().contains(mascotId)) {
            status = loadedMascotNotFoundStatus();
            return;
        }
        auto data = manager->loadedMascotsById()[mascotId];
        auto &preview = data->preview();
        auto pixmap = preview.availableSizes().empty()
            ? preview.pixmap(128, 128)
            : preview.pixmap(preview.availableSizes()[0]);
        QBuffer buffer(&pngBytes);
        buffer.open(QBuffer::WriteOnly);
        pixmap.save(&buffer, "PNG");
        buffer.close();
    });
    return status;
}

ApiPingInfo MascotCommandService::ping() const {
    ApiPingInfo ping;
    ping.ok = true;
    ping.app = QStringLiteral(APP_NAME);
    ping.apiVersion = QStringLiteral("v1");
    return ping;
}
