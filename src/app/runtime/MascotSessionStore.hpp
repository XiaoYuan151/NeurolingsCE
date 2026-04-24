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

#include <list>
#include <map>
#include <optional>

#include <QMap>
#include <QString>

class ShijimaWidget;

class MascotSessionStore {
public:
    std::list<ShijimaWidget *> const& mascots() const;
    std::list<ShijimaWidget *>& mascots();
    std::map<int, ShijimaWidget *> const& mascotsById() const;
    std::map<int, ShijimaWidget *>& mascotsById();

    bool empty() const;
    int size() const;
    void add(ShijimaWidget *mascot);
    void index(ShijimaWidget *mascot);
    void removeIndex(int mascotId);
    void clear();

    void markAllForDeletion();
    void markAllForDeletion(QString const& name);
    void markAllButOneForDeletion(ShijimaWidget *widget);
    void markAllButOneForDeletion(QString const& name);

    std::optional<int> cliLabelForMascot(int mascotId) const;
    std::optional<int> mascotIdForCliLabel(int cliLabel) const;
    bool assignCliLabel(int mascotId, std::optional<int> preferredLabel,
        int &assignedLabel, QString &errorMessage);
    void clearCliLabelForMascot(int mascotId);
    void clearCliLabels();

private:
    // Widgets are QObject-owned/deleted by the runtime; this store owns indexes.
    std::list<ShijimaWidget *> m_mascots;
    std::map<int, ShijimaWidget *> m_mascotsById;

    // CLI labels are transient handles for the current process, not settings.
    QMap<int, int> m_cliLabelToMascotId;
    QMap<int, int> m_cliLabelByMascotId;
    int m_nextCliLabel = 0;
};
