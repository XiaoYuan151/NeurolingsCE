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

#include "shijima-qt/MascotPackage.hpp"
#include "shijima-qt/AppLog.hpp"

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QTemporaryDir>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <shimejifinder/analyze.hpp>
#include <shimejifinder/extractor.hpp>
#include <shimejifinder/libunarr/archive.hpp>
#include <shimejifinder/memory_extractor.hpp>

namespace {

class ExactPathExtractor : public shimejifinder::extractor {
public:
    explicit ExactPathExtractor(QString root): m_root(QDir::cleanPath(root)) {}

    void begin_write(shimejifinder::extract_target const& target) override {
        QString relative = QString::fromStdString(target.extract_name());
        QString filePath = QDir(m_root).absoluteFilePath(relative);
        std::filesystem::path path = filePath.toStdString();
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out;
        out.open(path, std::ios::out | std::ios::binary);
        m_activeWrites.emplace_back(std::move(out));
    }

    void write_next(size_t offset, const void *buf, size_t size) override {
        for (auto &stream : m_activeWrites) {
            stream.seekp(offset);
            stream.write(static_cast<const char *>(buf), size);
        }
    }

    void end_write() override {
        for (auto &stream : m_activeWrites) {
            stream.close();
        }
        m_activeWrites.clear();
    }

    void finalize() override {
        m_activeWrites.clear();
    }

private:
    QString m_root;
    std::vector<std::ofstream> m_activeWrites;
};

QString normalizedArchivePath(QString path) {
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QStringList cleanParts;
    for (auto const& part : parts) {
        if (part == QStringLiteral(".") || part == QStringLiteral("..") ||
            part.contains(QLatin1Char(':')))
        {
            return {};
        }
        cleanParts.append(part);
    }
    return cleanParts.join(QLatin1Char('/'));
}

bool isSupportedPackagePath(QString const& path) {
    QString lower = path.toLower();
    return lower == QStringLiteral("info.json") ||
        lower == QStringLiteral("bubble_context.txt") ||
        lower == QStringLiteral("actions.xml") ||
        lower == QStringLiteral("behaviors.xml") ||
        (lower.startsWith(QStringLiteral("img/")) &&
            lower.endsWith(QStringLiteral(".png"))) ||
        lower.startsWith(QStringLiteral("sound/"));
}

bool openPackage(QString const& packagePath, shimejifinder::libunarr::archive &archive,
    QString &errorMessage)
{
    try {
        archive.open(packagePath.toStdString());
        return true;
    }
    catch (std::exception const& ex) {
        errorMessage = QString::fromUtf8(ex.what());
        return false;
    }
}

QByteArray readPackageFile(QString const& packagePath, QString const& wantedPath,
    QString &errorMessage)
{
    shimejifinder::libunarr::archive archive;
    if (!openPackage(packagePath, archive, errorMessage)) {
        return {};
    }

    QString normalizedWanted = normalizedArchivePath(wantedPath).toLower();
    bool found = false;
    for (size_t i = 0; i < archive.size(); ++i) {
        auto entry = archive.at(i);
        QString entryPath = normalizedArchivePath(QString::fromStdString(entry->path()));
        if (entryPath.toLower() == normalizedWanted) {
            entry->add_target(shimejifinder::extract_target(
                normalizedWanted.toStdString()));
            found = true;
            break;
        }
    }
    if (!found) {
        errorMessage = QStringLiteral("Package is missing %1").arg(wantedPath);
        return {};
    }

    shimejifinder::memory_extractor extractor;
    try {
        static_cast<shimejifinder::archive&>(archive).extract(&extractor);
    }
    catch (std::exception const& ex) {
        errorMessage = QString::fromUtf8(ex.what());
        return {};
    }

    auto key = normalizedWanted.toStdString();
    if (!extractor.contains(key)) {
        errorMessage = QStringLiteral("Could not read %1").arg(wantedPath);
        return {};
    }
    auto const& data = extractor.data(key);
    return QByteArray(data.data(), (qsizetype)data.size());
}

bool inspectEntries(QString const& packagePath, MascotMetadata &metadata,
    QString &errorMessage)
{
    QByteArray infoJson = readPackageFile(packagePath, QStringLiteral("info.json"),
        errorMessage);
    if (infoJson.isEmpty()) {
        return false;
    }
    try {
        metadata = MascotPackage::metadataFromJson(infoJson);
    }
    catch (std::exception const& ex) {
        errorMessage = QString::fromUtf8(ex.what());
        return false;
    }

    shimejifinder::libunarr::archive archive;
    if (!openPackage(packagePath, archive, errorMessage)) {
        return false;
    }
    bool hasActions = false;
    bool hasBehaviors = false;
    bool hasImage = false;
    for (size_t i = 0; i < archive.size(); ++i) {
        QString path = normalizedArchivePath(QString::fromStdString(archive.at(i)->path()));
        QString lower = path.toLower();
        hasActions = hasActions || lower == QStringLiteral("actions.xml");
        hasBehaviors = hasBehaviors || lower == QStringLiteral("behaviors.xml");
        hasImage = hasImage || (lower.startsWith(QStringLiteral("img/")) &&
            lower.endsWith(QStringLiteral(".png")));
    }
    if (!hasActions || !hasBehaviors || !hasImage) {
        errorMessage = QStringLiteral("Package must contain actions.xml, behaviors.xml, and img/*.png");
        return false;
    }
    return true;
}

quint32 crc32(QByteArray const& bytes) {
    static quint32 table[256] = {};
    static bool initialized = false;
    if (!initialized) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1) ? (0xedb88320U ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        initialized = true;
    }

    quint32 c = 0xffffffffU;
    for (auto ch : bytes) {
        c = table[(c ^ static_cast<quint8>(ch)) & 0xff] ^ (c >> 8);
    }
    return c ^ 0xffffffffU;
}

void write16(QFile &file, quint16 value) {
    char data[2] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
    };
    file.write(data, 2);
}

void write32(QFile &file, quint32 value) {
    char data[4] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 24) & 0xff),
    };
    file.write(data, 4);
}

struct ZipEntry {
    QString name;
    QByteArray data;
    quint32 crc = 0;
    quint32 offset = 0;
};

bool addFileEntry(QString const& root, QString const& filePath,
    std::vector<ZipEntry> &entries, QString &errorMessage)
{
    QString relative = QDir(root).relativeFilePath(filePath).replace(
        QLatin1Char('\\'), QLatin1Char('/'));
    relative = normalizedArchivePath(relative);
    if (relative.isEmpty() || !isSupportedPackagePath(relative)) {
        return true;
    }

    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        errorMessage = QStringLiteral("Could not read %1").arg(filePath);
        return false;
    }

    ZipEntry entry;
    entry.name = relative;
    entry.data = file.readAll();
    entry.crc = crc32(entry.data);
    entries.push_back(entry);
    return true;
}

void writeLocalEntry(QFile &file, ZipEntry &entry) {
    QByteArray name = entry.name.toUtf8();
    entry.offset = static_cast<quint32>(file.pos());
    write32(file, 0x04034b50);
    write16(file, 20);
    write16(file, 0x0800);
    write16(file, 0);
    write16(file, 0);
    write16(file, 0);
    write32(file, entry.crc);
    write32(file, static_cast<quint32>(entry.data.size()));
    write32(file, static_cast<quint32>(entry.data.size()));
    write16(file, static_cast<quint16>(name.size()));
    write16(file, 0);
    file.write(name);
    file.write(entry.data);
}

void writeCentralEntry(QFile &file, ZipEntry const& entry) {
    QByteArray name = entry.name.toUtf8();
    write32(file, 0x02014b50);
    write16(file, 20);
    write16(file, 20);
    write16(file, 0x0800);
    write16(file, 0);
    write16(file, 0);
    write16(file, 0);
    write32(file, entry.crc);
    write32(file, static_cast<quint32>(entry.data.size()));
    write32(file, static_cast<quint32>(entry.data.size()));
    write16(file, static_cast<quint16>(name.size()));
    write16(file, 0);
    write16(file, 0);
    write16(file, 0);
    write16(file, 0);
    write32(file, 0);
    write32(file, entry.offset);
    file.write(name);
}

void ensureLegacyMetadata(QString const& sourcePath, QString const& fallbackName) {
    QFile info(QDir(sourcePath).absoluteFilePath(QStringLiteral("info.json")));
    if (info.exists()) {
        return;
    }
    MascotMetadata metadata;
    metadata.name = fallbackName;
    QSaveFile out(info.fileName());
    if (out.open(QFile::WriteOnly)) {
        out.write(MascotPackage::metadataToJson(metadata));
        out.commit();
    }
}

void tryExtractBubbleContext(QString const& archivePath, QString const& mascotName,
    QString const& targetPath)
{
    if (QFile::exists(QDir(targetPath).absoluteFilePath(QStringLiteral("bubble_context.txt")))) {
        return;
    }

    shimejifinder::libunarr::archive archive;
    QString error;
    if (!openPackage(archivePath, archive, error)) {
        return;
    }

    QStringList matches;
    QString mascotLower = mascotName.toLower();
    for (size_t i = 0; i < archive.size(); ++i) {
        QString path = normalizedArchivePath(QString::fromStdString(archive.at(i)->path()));
        QString lower = path.toLower();
        if (!lower.endsWith(QStringLiteral("bubble_context.txt"))) {
            continue;
        }
        if (lower == QStringLiteral("bubble_context.txt") ||
            lower.contains(QStringLiteral("/") + mascotLower + QStringLiteral("/")) ||
            lower.startsWith(mascotLower + QStringLiteral("/")) ||
            lower.startsWith(mascotLower + QStringLiteral(".mascot/")))
        {
            matches.append(path);
        }
    }
    if (matches.isEmpty()) {
        return;
    }

    QByteArray bytes = readPackageFile(archivePath, matches.first(), error);
    if (bytes.isEmpty()) {
        return;
    }
    QSaveFile out(QDir(targetPath).absoluteFilePath(QStringLiteral("bubble_context.txt")));
    if (out.open(QFile::WriteOnly)) {
        out.write(bytes);
        out.commit();
    }
}

}

namespace MascotPackage {

MascotMetadata defaultMetadata() {
    MascotMetadata metadata;
    metadata.name = QStringLiteral("Default");
    metadata.version = QStringLiteral("1.0");
    metadata.description = QStringLiteral("Default mascot for the application.");
    metadata.author = QStringLiteral("pixelomer[https://github.com/pixelomer]");
    return metadata;
}

MascotMetadata metadataFromJson(QByteArray const& bytes) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        throw std::runtime_error("Invalid info.json");
    }
    QJsonObject object = doc.object();
    MascotMetadata metadata;
    metadata.name = object.value(QStringLiteral("name")).toString().trimmed();
    metadata.version = object.value(QStringLiteral("version")).toString();
    metadata.description = object.value(QStringLiteral("description")).toString();
    metadata.author = object.value(QStringLiteral("author")).toString();
    if (metadata.name.isEmpty()) {
        throw std::runtime_error("info.json must contain a non-empty name");
    }
    return metadata;
}

QByteArray metadataToJson(MascotMetadata const& metadata) {
    QJsonObject object;
    object["name"] = metadata.name;
    object["version"] = metadata.version;
    object["description"] = metadata.description;
    object["author"] = metadata.author;
    return QJsonDocument(object).toJson(QJsonDocument::Indented);
}

QString sanitizedPackageBaseName(QString const& name) {
    QString result = name.trimmed();
    static QString const invalid = QStringLiteral("<>:\"/\\|?*");
    for (auto ch : invalid) {
        result.replace(ch, QLatin1Char('_'));
    }
    for (qsizetype i = 0; i < result.size(); ++i) {
        if (result[i].unicode() < 32) {
            result[i] = QLatin1Char('_');
        }
    }
    result = result.trimmed();
    while (result.endsWith(QLatin1Char('.'))) {
        result.chop(1);
    }
    if (result.isEmpty()) {
        result = QStringLiteral("Mascot");
    }
    return result;
}

QString packagePathForName(QString const& storagePath, QString const& name) {
    return QDir(storagePath).absoluteFilePath(
        sanitizedPackageBaseName(name) + QStringLiteral(".mascot"));
}

QString cachePathForName(QString const& cacheRootPath, QString const& name) {
    return QDir(cacheRootPath).absoluteFilePath(sanitizedPackageBaseName(name));
}

bool inspectPackage(QString const& packagePath, MascotMetadata &metadata,
    QString &errorMessage)
{
    return inspectEntries(packagePath, metadata, errorMessage);
}

bool extractPackage(QString const& packagePath, QString const& outputPath,
    QString &errorMessage)
{
    shimejifinder::libunarr::archive archive;
    if (!openPackage(packagePath, archive, errorMessage)) {
        return false;
    }

    QDir outputDir(outputPath);
    if (outputDir.exists()) {
        outputDir.removeRecursively();
    }
    outputDir.mkpath(QStringLiteral("."));

    bool added = false;
    for (size_t i = 0; i < archive.size(); ++i) {
        auto entry = archive.at(i);
        QString path = normalizedArchivePath(QString::fromStdString(entry->path()));
        if (path.isEmpty() || !isSupportedPackagePath(path)) {
            continue;
        }
        entry->add_target(shimejifinder::extract_target(path.toStdString()));
        added = true;
    }
    if (!added) {
        errorMessage = QStringLiteral("Package does not contain any supported files");
        return false;
    }

    ExactPathExtractor extractor(outputPath);
    try {
        static_cast<shimejifinder::archive&>(archive).extract(&extractor);
        return true;
    }
    catch (std::exception const& ex) {
        errorMessage = QString::fromUtf8(ex.what());
        return false;
    }
}

bool writePackageFromDirectory(QString const& sourcePath,
    QString const& packagePath, QString &errorMessage)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isDir()) {
        errorMessage = QStringLiteral("Source mascot directory does not exist");
        return false;
    }

    std::vector<ZipEntry> entries;
    QDirIterator iter(sourceInfo.absoluteFilePath(), QDir::Files,
        QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        if (!addFileEntry(sourceInfo.absoluteFilePath(), iter.next(), entries,
            errorMessage))
        {
            return false;
        }
    }
    std::sort(entries.begin(), entries.end(), [](ZipEntry const& lhs,
        ZipEntry const& rhs) {
        return lhs.name < rhs.name;
    });

    bool hasInfo = false;
    bool hasActions = false;
    bool hasBehaviors = false;
    bool hasImage = false;
    for (auto const& entry : entries) {
        QString lower = entry.name.toLower();
        hasInfo = hasInfo || lower == QStringLiteral("info.json");
        hasActions = hasActions || lower == QStringLiteral("actions.xml");
        hasBehaviors = hasBehaviors || lower == QStringLiteral("behaviors.xml");
        hasImage = hasImage || lower.startsWith(QStringLiteral("img/"));
    }
    if (!hasInfo || !hasActions || !hasBehaviors || !hasImage) {
        errorMessage = QStringLiteral("Mascot package source is missing required files");
        return false;
    }

    QFileInfo packageInfo(packagePath);
    QDir().mkpath(packageInfo.absolutePath());
    QFile file(packagePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        errorMessage = QStringLiteral("Could not write %1").arg(packagePath);
        return false;
    }

    for (auto &entry : entries) {
        writeLocalEntry(file, entry);
    }
    quint32 centralOffset = static_cast<quint32>(file.pos());
    for (auto const& entry : entries) {
        writeCentralEntry(file, entry);
    }
    quint32 centralSize = static_cast<quint32>(file.pos()) - centralOffset;
    write32(file, 0x06054b50);
    write16(file, 0);
    write16(file, 0);
    write16(file, static_cast<quint16>(entries.size()));
    write16(file, static_cast<quint16>(entries.size()));
    write32(file, centralSize);
    write32(file, centralOffset);
    write16(file, 0);
    return true;
}

bool installPackage(QString const& packagePath, QString const& storagePath,
    QString &installedName, QString &errorMessage)
{
    MascotMetadata metadata;
    if (!inspectPackage(packagePath, metadata, errorMessage)) {
        return false;
    }
    QDir storageDir(storagePath);
    storageDir.mkpath(QStringLiteral("."));
    QString targetPath = packagePathForName(storagePath, metadata.name);
    QFileInfo sourceInfo(packagePath);
    QFileInfo targetInfo(targetPath);
    if (sourceInfo.absoluteFilePath() != targetInfo.absoluteFilePath()) {
        QFile::remove(targetPath);
        if (!QFile::copy(sourceInfo.absoluteFilePath(), targetPath)) {
            errorMessage = QStringLiteral("Could not copy package into mascot storage");
            return false;
        }
    }
    installedName = metadata.name;
    return true;
}

bool packageLegacyDirectory(QString const& sourcePath,
    QString const& storagePath, QString const& fallbackName,
    QString &installedName, QString &errorMessage)
{
    ensureLegacyMetadata(sourcePath, fallbackName);
    QFile infoFile(QDir(sourcePath).absoluteFilePath(QStringLiteral("info.json")));
    if (!infoFile.open(QFile::ReadOnly)) {
        errorMessage = QStringLiteral("Could not read generated info.json");
        return false;
    }
    MascotMetadata metadata;
    try {
        metadata = metadataFromJson(infoFile.readAll());
    }
    catch (...) {
        metadata.name = fallbackName;
        QSaveFile out(infoFile.fileName());
        if (out.open(QFile::WriteOnly)) {
            out.write(metadataToJson(metadata));
            out.commit();
        }
    }
    QString targetPath = packagePathForName(storagePath, metadata.name);
    QFile::remove(targetPath);
    if (!writePackageFromDirectory(sourcePath, targetPath, errorMessage)) {
        return false;
    }
    installedName = metadata.name;
    return true;
}

std::set<std::string> importArchive(QString const& archivePath,
    QString const& storagePath)
{
    std::set<std::string> imported;
    QFileInfo archiveInfo(archivePath);
    QString error;

    if (archiveInfo.suffix().compare(QStringLiteral("mascot"),
        Qt::CaseInsensitive) == 0)
    {
        QString installedName;
        if (installPackage(archiveInfo.absoluteFilePath(), storagePath,
            installedName, error))
        {
            imported.insert(installedName.toStdString());
            return imported;
        }
    }

    try {
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            return imported;
        }
        auto archive = shimejifinder::analyze(archiveInfo.absoluteFilePath().toStdString());
        archive->extract(tempDir.path().toStdString());
        for (auto const& name : archive->shimejis()) {
            QString qName = QString::fromStdString(name);
            QString sourcePath = QDir(tempDir.path()).absoluteFilePath(
                qName + QStringLiteral(".mascot"));
            tryExtractBubbleContext(archiveInfo.absoluteFilePath(), qName, sourcePath);
            QString installedName;
            if (packageLegacyDirectory(sourcePath, storagePath, qName,
                installedName, error))
            {
                imported.insert(installedName.toStdString());
            }
            else {
                APP_LOG_WARN("import") << "Could not package legacy mascot name=\""
                    << qName.toStdString() << "\": " << error.toStdString();
            }
        }
    }
    catch (std::exception const& ex) {
        APP_LOG_ERROR("import") << "Legacy import failed for path=\""
            << archivePath.toStdString() << "\": " << ex.what();
    }
    return imported;
}

void migrateLegacyDirectories(QString const& storagePath) {
    QDirIterator iter(storagePath, QDir::Dirs | QDir::NoDotAndDotDot,
        QDirIterator::NoIteratorFlags);
    while (iter.hasNext()) {
        QFileInfo entry = iter.nextFileInfo();
        QString dirname = entry.fileName();
        if (!dirname.endsWith(QStringLiteral(".mascot"), Qt::CaseInsensitive) ||
            dirname.length() <= 7)
        {
            continue;
        }
        QString fallbackName = dirname.sliced(0, dirname.length() - 7);
        QString tempPackage = QDir(storagePath).absoluteFilePath(
            fallbackName + QStringLiteral(".mascot.migrating"));
        QString error;
        ensureLegacyMetadata(entry.absoluteFilePath(), fallbackName);
        if (!writePackageFromDirectory(entry.absoluteFilePath(), tempPackage, error)) {
            APP_LOG_WARN("mascot") << "Could not migrate legacy mascot directory path=\""
                << entry.absoluteFilePath().toStdString() << "\": "
                << error.toStdString();
            QFile::remove(tempPackage);
            continue;
        }
        QDir legacyDir(entry.absoluteFilePath());
        if (!legacyDir.removeRecursively()) {
            QFile::remove(tempPackage);
            continue;
        }
        QFile::rename(tempPackage, entry.absoluteFilePath());
    }
}

}
