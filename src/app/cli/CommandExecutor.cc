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
#include "shijima-qt/ShijimaLocalApi.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QThread>

#include <algorithm>
#include <exception>
#include <map>

#include <shimejifinder/analyze.hpp>

namespace {

CliError makeError(QString const& code, QString const& message, int exitCode = 1,
    int status = 0, QString const& details = {})
{
    CliError error;
    error.code = code;
    error.error = message;
    error.details = details;
    error.exitCode = exitCode;
    error.httpStatus = status;
    return error;
}

ShijimaLocalApiClientOptions localApiOptions(CliGlobalOptions const& global) {
    ShijimaLocalApiClientOptions options;
    options.connectTimeoutMs = global.connectTimeoutMs;
    options.readTimeoutMs = global.readTimeoutMs;
    return options;
}

ShijimaLocalApiClientOptions startupRequestOptions(CliGlobalOptions const& global) {
    ShijimaLocalApiClientOptions options = localApiOptions(global);
    options.connectTimeoutMs = std::max(options.connectTimeoutMs, 1000);
    options.readTimeoutMs = std::max(options.readTimeoutMs, 5000);
    return options;
}

QStringList runtimeExecutableCandidates() {
    QString executableSuffix;
#ifdef _WIN32
    executableSuffix = QStringLiteral(".exe");
#endif
    QStringList names {
        QStringLiteral(APP_NAME),
        QStringLiteral("NeurolingsCE"),
        QStringLiteral("shijima-qt"),
    };
    names.removeDuplicates();

    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();
    for (auto const& name : names) {
        QString fileName = name;
        if (!executableSuffix.isEmpty() &&
            !fileName.endsWith(executableSuffix, Qt::CaseInsensitive))
        {
            fileName += executableSuffix;
        }
        candidates.append(QDir(appDir).absoluteFilePath(fileName));
    }
    return candidates;
}

bool findRuntimeExecutable(QString &path, CliError &error) {
    auto cliPath = QFileInfo { QCoreApplication::applicationFilePath() }
        .canonicalFilePath();
    for (auto const& candidate : runtimeExecutableCandidates()) {
        QFileInfo info { candidate };
        if (!info.exists() || !info.isFile() || !info.isExecutable()) {
            continue;
        }
        if (info.canonicalFilePath() == cliPath) {
            continue;
        }
        path = info.absoluteFilePath();
        return true;
    }

    error = makeError(QStringLiteral("runtime_not_found"),
        QStringLiteral("Could not find the NeurolingsCE runtime executable next to the CLI"),
        1, 0, runtimeExecutableCandidates().join(QStringLiteral("; ")));
    return false;
}

bool waitForRuntime(CliGlobalOptions const& global, int timeoutMs) {
    ShijimaLocalApiClientOptions options = localApiOptions(global);
    options.connectTimeoutMs = std::min(options.connectTimeoutMs, 250);
    options.readTimeoutMs = std::min(options.readTimeoutMs, 500);

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() <= timeoutMs) {
        if (shijimaLocalApiPing(options)) {
            return true;
        }
        QThread::msleep(100);
    }
    return false;
}

bool ensureRuntimeStarted(CliCommand const& command, CliError &error) {
    if (waitForRuntime(command.global, 0)) {
        return true;
    }

    QString runtimePath;
    if (!findRuntimeExecutable(runtimePath, error)) {
        return false;
    }

    APP_LOG_INFO("cli") << "Starting NeurolingsCE runtime path=\""
        << runtimePath.toStdString() << "\"";
    if (!QProcess::startDetached(runtimePath,
        QStringList { QStringLiteral("--neurolingsce-cli-runtime") },
        QFileInfo(runtimePath).absolutePath()))
    {
        error = makeError(QStringLiteral("runtime_start_failed"),
            QStringLiteral("Could not start the NeurolingsCE runtime executable"),
            1, 0, runtimePath);
        return false;
    }

    int startupTimeoutMs = std::max(10000,
        command.global.connectTimeoutMs + command.global.readTimeoutMs);
    if (!waitForRuntime(command.global, startupTimeoutMs)) {
        error = makeError(QStringLiteral("runtime_start_timeout"),
            QStringLiteral("NeurolingsCE runtime was started but did not become ready"),
            1, 0, runtimePath);
        return false;
    }
    return true;
}

bool sendRequest(CliCommand const& command, QJsonObject const& requestObject,
    QJsonObject &responseObject, CliError &error)
{
    QString transportError;
    bool requestOk = shijimaLocalApiRequest(requestObject, responseObject,
        transportError, localApiOptions(command.global));
    if (!requestOk) {
        if (!shijimaLocalApiPing(localApiOptions(command.global))) {
            if (!ensureRuntimeStarted(command, error)) {
                return false;
            }
            transportError.clear();
            requestOk = shijimaLocalApiRequest(requestObject, responseObject,
                transportError, startupRequestOptions(command.global));
        }
    }
    if (!requestOk) {
        if (transportError.isEmpty()) {
            transportError = QStringLiteral("NeurolingsCE runtime did not respond to the CLI request");
        }
        error = makeError(QStringLiteral("transport_error"), transportError);
        return false;
    }
    if (responseObject.contains("error")) {
        error = makeError(
            responseObject.value("code").toString(QStringLiteral("ipc_error")),
            responseObject.value("error").toString(QStringLiteral("IPC request failed")),
            1, responseObject.value("status").toInt());
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
            QStringLiteral("Malformed response from local IPC"));
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
            QStringLiteral("Malformed response from local IPC"));
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

bool parseCliLabelObject(QJsonObject const& object, CliLabelInfo &labelInfo,
    CliError &error)
{
    QString parseError;
    if (!cliLabelInfoFromJson(object, labelInfo, &parseError)) {
        error = makeError(QStringLiteral("invalid_response"),
            QStringLiteral("Malformed CLI label payload"), 1, 0, parseError);
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

QString mascotStoragePath() {
    QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dataPath.isEmpty()) {
        return {};
    }
    return QDir::cleanPath(dataPath + QDir::separator() + QStringLiteral("mascots"));
}

bool ensureMascotStorage(QString &storagePath, CliError &error) {
    storagePath = mascotStoragePath();
    if (storagePath.isEmpty()) {
        error = makeError(QStringLiteral("storage_unavailable"),
            QStringLiteral("Could not determine mascot storage directory"));
        return false;
    }

    QDir dir { storagePath };
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        error = makeError(QStringLiteral("storage_unavailable"),
            QStringLiteral("Could not create mascot storage directory"), 1, 0,
            storagePath);
        return false;
    }

    if (QFile readme { dir.absoluteFilePath(QStringLiteral("README.txt")) };
        readme.open(QFile::WriteOnly | QFile::NewOnly | QFile::Text))
    {
        readme.write(""
"Manually importing shimeji by copying its contents into this folder may\n"
"cause problems. Prefer NeurolingsCE-cli --mascot add <zip> or the GUI import\n"
"dialog unless you have a good reason not to.\n"
        );
    }
    return true;
}

QString normalizeMascotTemplateName(QString name) {
    name = name.trimmed();
    if (name.endsWith(QStringLiteral(".mascot"), Qt::CaseInsensitive)) {
        name.chop(7);
    }
    return name;
}

bool hasPathSeparator(QString const& value) {
    return value.contains(QLatin1Char('/')) ||
        value.contains(QLatin1Char('\\')) ||
        value == QStringLiteral(".") ||
        value == QStringLiteral("..");
}

bool listStandaloneLoadedMascots(QList<LoadedMascotInfo> &mascots,
    CliError &error)
{
    QString storagePath;
    if (!ensureMascotStorage(storagePath, error)) {
        return false;
    }

    mascots.clear();
    mascots.append(LoadedMascotInfo { 0, QStringLiteral("Default Mascot") });

    QStringList names;
    QDirIterator iter { storagePath, QDir::Dirs | QDir::NoDotAndDotDot,
        QDirIterator::NoIteratorFlags };
    while (iter.hasNext()) {
        QFileInfo entry = iter.nextFileInfo();
        QString dirname = entry.fileName();
        if (!dirname.endsWith(QStringLiteral(".mascot"), Qt::CaseInsensitive) ||
            dirname.length() <= 7)
        {
            continue;
        }
        names.append(dirname.sliced(0, dirname.length() - 7));
    }
    names.sort(Qt::CaseInsensitive);

    int id = 1;
    for (auto const& name : names) {
        mascots.append(LoadedMascotInfo { id++, name });
    }
    return true;
}

bool importStandaloneMascotTemplate(QString const& archivePath,
    QList<LoadedMascotInfo> &importedMascots, CliError &error)
{
    QFileInfo archiveInfo { archivePath };
    if (!archiveInfo.exists() || !archiveInfo.isFile()) {
        error = makeError(QStringLiteral("invalid_arguments"),
            QStringLiteral("Mascot archive does not exist"), 2, 0, archivePath);
        return false;
    }

    QString storagePath;
    if (!ensureMascotStorage(storagePath, error)) {
        return false;
    }

    std::set<std::string> changed;
    try {
        auto archive = shimejifinder::analyze(archiveInfo.absoluteFilePath().toStdString());
        if (!archive) {
            error = makeError(QStringLiteral("import_failed"),
                QStringLiteral("Could not analyze mascot archive"), 1, 0,
                archiveInfo.absoluteFilePath());
            return false;
        }
        archive->extract(storagePath.toStdString());
        changed = archive->shimejis();
    }
    catch (std::exception const& ex) {
        error = makeError(QStringLiteral("import_failed"),
            QStringLiteral("Could not import mascot archive"), 1, 0,
            QString::fromUtf8(ex.what()));
        return false;
    }

    if (changed.empty()) {
        error = makeError(QStringLiteral("import_failed"),
            QStringLiteral("Could not import any mascots from the specified archive"));
        return false;
    }

    QList<LoadedMascotInfo> allMascots;
    if (!listStandaloneLoadedMascots(allMascots, error)) {
        return false;
    }

    std::map<QString, LoadedMascotInfo> byName;
    for (auto const& mascot : allMascots) {
        byName[mascot.name] = mascot;
    }

    importedMascots.clear();
    for (auto const& name : changed) {
        auto qName = QString::fromStdString(name);
        auto it = byName.find(qName);
        if (it != byName.end()) {
            importedMascots.append(it->second);
        }
        else {
            importedMascots.append(LoadedMascotInfo { -1, qName });
        }
    }
    return true;
}

bool removeStandaloneMascotTemplate(QString const& requestedName,
    QString &removedName, CliError &error)
{
    QString name = normalizeMascotTemplateName(requestedName);
    if (name.isEmpty() || hasPathSeparator(name)) {
        error = makeError(QStringLiteral("invalid_arguments"),
            QStringLiteral("Mascot template name must be a plain template name"),
            2);
        return false;
    }
    if (name == QStringLiteral("Default Mascot")) {
        error = makeError(QStringLiteral("mascot_template_not_deletable"),
            QStringLiteral("Mascot template cannot be deleted"));
        return false;
    }

    QString storagePath;
    if (!ensureMascotStorage(storagePath, error)) {
        return false;
    }

    QDir storageDir { storagePath };
    QFileInfo storageInfo { storagePath };
    QFileInfo targetInfo {
        storageDir.absoluteFilePath(name + QStringLiteral(".mascot"))
    };
    if (!targetInfo.exists() || !targetInfo.isDir()) {
        error = makeError(QStringLiteral("mascot_template_not_found"),
            QStringLiteral("No such mascot template"));
        return false;
    }

    QString storageCanonical = storageInfo.canonicalFilePath();
    QString targetCanonical = targetInfo.canonicalFilePath();
    QString storagePrefix = QDir::cleanPath(storageCanonical) + QDir::separator();
    if (storageCanonical.isEmpty() || targetCanonical.isEmpty() ||
        !QDir::cleanPath(targetCanonical).startsWith(storagePrefix))
    {
        error = makeError(QStringLiteral("invalid_template_path"),
            QStringLiteral("Refusing to delete a mascot outside the storage directory"));
        return false;
    }

    QDir targetDir { targetCanonical };
    if (!targetDir.removeRecursively()) {
        error = makeError(QStringLiteral("remove_failed"),
            QStringLiteral("Could not remove mascot template"), 1, 0,
            targetCanonical);
        return false;
    }

    removedName = name;
    return true;
}

bool listLoadedMascots(CliCommand const& command,
    QList<LoadedMascotInfo> &mascots, CliError &error)
{
    QJsonObject object;
    if (!sendRequest(command, QJsonObject {
        { QStringLiteral("command"), QStringLiteral("list_loaded_mascots") },
    }, object, error))
    {
        return false;
    }
    return parseLoadedMascotArray(object, "loaded_mascots", mascots, error);
}

bool registerCliLabel(CliCommand const& command, int mascotId,
    std::optional<int> preferredLabel, CliLabelInfo &labelInfo,
    CliError &error)
{
    QJsonObject requestObject {
        { QStringLiteral("command"), QStringLiteral("register_cli_label") },
        { QStringLiteral("mascot_id"), mascotId },
    };
    if (preferredLabel.has_value()) {
        requestObject[QStringLiteral("label")] = preferredLabel.value();
    }
    QJsonObject object;
    if (!sendRequest(command, requestObject, object, error)) {
        return false;
    }
    return parseCliLabelObject(object, labelInfo, error);
}

bool resolveCliLabel(CliCommand const& command, int cliLabel,
    CliLabelInfo &labelInfo, CliError &error)
{
    QJsonObject object;
    if (!sendRequest(command, QJsonObject {
        { QStringLiteral("command"), QStringLiteral("get_cli_label") },
        { QStringLiteral("label"), cliLabel },
    }, object, error))
    {
        return false;
    }
    return parseCliLabelObject(object, labelInfo, error);
}

bool resolveMascotId(CliCommand const& command, int &mascotId,
    CliError &error)
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
        QJsonObject requestObject {
            { QStringLiteral("command"), QStringLiteral("list_mascots") },
        };
        if (!selector.isEmpty()) {
            requestObject[QStringLiteral("selector")] = selector;
        }
        QJsonObject object;
        if (!sendRequest(command, requestObject, object, error)) {
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

    if (command.kind == CliCommandKind::Help ||
        command.kind == CliCommandKind::Version)
    {
        return result;
    }

    auto fail = [&](CliError const& error) {
        result.error = error;
        APP_LOG_ERROR("cli") << error.error.toStdString();
        if (!error.details.isEmpty()) {
            APP_LOG_WARN("cli") << error.details.toStdString();
        }
        return result;
    };

    if (command.kind == CliCommandKind::DocumentStop) {
        QJsonObject object;
        QString transportError;
        bool requestOk = shijimaLocalApiRequest(QJsonObject {
            { QStringLiteral("command"), QStringLiteral("stop_runtime") },
        }, object, transportError, localApiOptions(command.global));
        if (!requestOk) {
            if (!shijimaLocalApiPing(localApiOptions(command.global))) {
                return result;
            }
            return fail(makeError(QStringLiteral("transport_error"),
                transportError.isEmpty()
                    ? QStringLiteral("NeurolingsCE runtime did not respond to the CLI request")
                    : transportError));
        }
        if (object.contains(QStringLiteral("error"))) {
            return fail(makeError(
                object.value(QStringLiteral("code")).toString(QStringLiteral("ipc_error")),
                object.value(QStringLiteral("error")).toString(QStringLiteral("IPC request failed")),
                1, object.value(QStringLiteral("status")).toInt()));
        }
        return result;
    }

    if (command.kind == CliCommandKind::DocumentList ||
        command.kind == CliCommandKind::ListMascots)
    {
        QJsonObject requestObject {
            { QStringLiteral("command"), QStringLiteral("list_mascots") },
        };
        if (!command.selector.isEmpty()) {
            requestObject[QStringLiteral("selector")] = command.selector;
        }
        QJsonObject object;
        CliError error;
        if (!sendRequest(command, requestObject, object, error)) {
            return fail(error);
        }
        if (!parseMascotArray(object, "mascots", result.mascots, error)) {
            return fail(error);
        }
        return result;
    }

    if (command.kind == CliCommandKind::ListLoadedMascots) {
        CliError error;
        if (!listLoadedMascots(command, result.loadedMascots, error)) {
            return fail(error);
        }
        return result;
    }

    if (command.kind == CliCommandKind::DocumentMascot) {
        CliError error;
        if (command.mascotAction == QStringLiteral("list")) {
            if (!listStandaloneLoadedMascots(result.loadedMascots, error)) {
                return fail(error);
            }
            return result;
        }

        if (command.mascotAction == QStringLiteral("add")) {
            if (!importStandaloneMascotTemplate(command.mascotArchivePath,
                result.loadedMascots, error))
            {
                return fail(error);
            }
        }
        else {
            if (!removeStandaloneMascotTemplate(command.mascotTemplateName,
                result.removedTemplateName, error))
            {
                return fail(error);
            }
        }
        return result;
    }

    if (command.kind == CliCommandKind::DocumentSummon ||
        command.kind == CliCommandKind::SpawnMascot)
    {
        SpawnMascotRequest request = command.spawnRequest;
        CliError error;

        if (command.kind == CliCommandKind::DocumentSummon &&
            command.summonMode == QStringLiteral("random"))
        {
            QList<LoadedMascotInfo> loadedMascots;
            if (!listLoadedMascots(command, loadedMascots, error)) {
                return fail(error);
            }
            if (loadedMascots.isEmpty()) {
                return fail(makeError(QStringLiteral("not_found"),
                    QStringLiteral("No loaded mascots are available")));
            }
            int index = QRandomGenerator::global()->bounded(0, loadedMascots.size());
            request.dataId = loadedMascots[index].id;
            request.name.reset();
        }

        auto behavior = chooseBehavior(command.behaviors);
        if (!behavior.isEmpty()) {
            request.patch.behavior = behavior;
        }

        QJsonObject object;
        if (!sendRequest(command, QJsonObject {
            { QStringLiteral("command"), QStringLiteral("spawn_mascot") },
            { QStringLiteral("request"), spawnMascotRequestToJson(request) },
        }, object, error))
        {
            return fail(error);
        }
        MascotInfo mascot;
        if (!parseMascotObject(object, "mascot", mascot, error)) {
            return fail(error);
        }
        if (command.kind == CliCommandKind::DocumentSummon) {
            CliLabelInfo labelInfo;
            if (!registerCliLabel(command, mascot.id, command.cliLabel, labelInfo, error)) {
                return fail(error);
            }
            mascot.cliLabel = labelInfo.label;
        }
        result.mascot = mascot;
        return result;
    }

    if (command.kind == CliCommandKind::AlterMascot) {
        CliError error;
        int mascotId = -1;
        if (!resolveMascotId(command, mascotId, error)) {
            return fail(error);
        }
        MascotPatch patch = command.patch;
        auto behavior = chooseBehavior(command.behaviors);
        if (!behavior.isEmpty()) {
            patch.behavior = behavior;
        }
        QJsonObject object;
        if (!sendRequest(command, QJsonObject {
            { QStringLiteral("command"), QStringLiteral("alter_mascot") },
            { QStringLiteral("mascot_id"), mascotId },
            { QStringLiteral("patch"), mascotPatchToJson(patch) },
        }, object, error))
        {
            return fail(error);
        }
        MascotInfo mascot;
        if (!parseMascotObject(object, "mascot", mascot, error)) {
            return fail(error);
        }
        result.mascot = mascot;
        return result;
    }

    if (command.kind == CliCommandKind::DocumentClose) {
        CliError error;
        CliLabelInfo labelInfo;
        if (!resolveCliLabel(command, command.cliLabel.value(), labelInfo, error)) {
            return fail(error);
        }
        QJsonObject object;
        if (!sendRequest(command, QJsonObject {
            { QStringLiteral("command"), QStringLiteral("dismiss_mascot") },
            { QStringLiteral("mascot_id"), labelInfo.mascotId },
        }, object, error))
        {
            return fail(error);
        }
        return result;
    }

    if (command.kind == CliCommandKind::DismissMascot) {
        CliError error;
        int mascotId = -1;
        if (!resolveMascotId(command, mascotId, error)) {
            return fail(error);
        }
        QJsonObject object;
        if (!sendRequest(command, QJsonObject {
            { QStringLiteral("command"), QStringLiteral("dismiss_mascot") },
            { QStringLiteral("mascot_id"), mascotId },
        }, object, error))
        {
            return fail(error);
        }
        return result;
    }

    QJsonObject requestObject {
        { QStringLiteral("command"), QStringLiteral("dismiss_all_mascots") },
    };
    if (!command.selector.isEmpty()) {
        requestObject[QStringLiteral("selector")] = command.selector;
    }
    QJsonObject object;
    CliError error;
    if (!sendRequest(command, requestObject, object, error)) {
        return fail(error);
    }
    return result;
}
