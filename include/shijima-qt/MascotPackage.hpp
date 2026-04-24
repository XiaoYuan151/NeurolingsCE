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

#include <QByteArray>
#include <QString>

#include <set>
#include <string>

struct MascotMetadata {
    QString name;
    QString version;
    QString description;
    QString author;
};

namespace MascotPackage {

MascotMetadata defaultMetadata();
MascotMetadata metadataFromJson(QByteArray const& bytes);
QByteArray metadataToJson(MascotMetadata const& metadata);
QString sanitizedPackageBaseName(QString const& name);
QString packagePathForName(QString const& storagePath, QString const& name);
QString cachePathForName(QString const& cacheRootPath, QString const& name);

bool inspectPackage(QString const& packagePath, MascotMetadata &metadata,
    QString &errorMessage);
bool extractPackage(QString const& packagePath, QString const& outputPath,
    QString &errorMessage);
bool writePackageFromDirectory(QString const& sourcePath,
    QString const& packagePath, QString &errorMessage);
bool installPackage(QString const& packagePath, QString const& storagePath,
    QString &installedName, QString &errorMessage);
bool packageLegacyDirectory(QString const& sourcePath,
    QString const& storagePath, QString const& fallbackName,
    QString &installedName, QString &errorMessage);
std::set<std::string> importArchive(QString const& archivePath,
    QString const& storagePath);
void migrateLegacyDirectories(QString const& storagePath);

}
