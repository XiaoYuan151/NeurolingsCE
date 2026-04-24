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

#include <memory>
#include <vector>

#include <QMap>
#include <QString>

#include <shijima/mascot/factory.hpp>

class MascotData;

class MascotTemplateStore {
public:
    MascotTemplateStore();
    ~MascotTemplateStore();

    MascotData *registerTemplate(std::unique_ptr<MascotData> data);
    std::unique_ptr<MascotData> takeTemplate(QString const& name);

    QMap<QString, MascotData *> const& loadedMascots() const;
    QMap<int, MascotData *> const& loadedMascotsById() const;
    QMap<QString, MascotData *>& loadedMascots();
    QMap<int, MascotData *>& loadedMascotsById();
    shijima::mascot::factory& factory();
    shijima::mascot::factory const& factory() const;

private:
    // The maps are raw pointer facades for compatibility; ownership stays here.
    std::vector<std::unique_ptr<MascotData>> m_ownedTemplates;
    QMap<QString, MascotData *> m_loadedMascots;
    QMap<int, MascotData *> m_loadedMascotsById;
    shijima::mascot::factory m_factory;
};
