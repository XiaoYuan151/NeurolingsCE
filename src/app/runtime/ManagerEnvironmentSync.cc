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

#include "shijima-qt/ShijimaManager.hpp"
#include "shijima-qt/ui/mascot/ShijimaWidget.hpp"
#include "ManagerRuntimeState.hpp"
#include "../ui/ManagerUiState.hpp"
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

void ShijimaManager::screenAdded(QScreen *screen) {
    m_runtime->environment.screenAdded(screen);
}

void ShijimaManager::screenRemoved(QScreen *screen) {
    m_runtime->environment.screenRemoved(screen,
        QGuiApplication::primaryScreen(), m_runtime->sessions);
}

void ShijimaManager::setWindowedMode(bool windowedMode) {
    if (!!this->windowedMode() == !!windowedMode) {
        return;
    }

    m_ui->windowedModeAction->setChecked(windowedMode);
    for (auto mascot : m_runtime->sessions.mascots()) {
        mascot->close();
        mascot->setParent(nullptr);
    }

    if (windowedMode) {
        QWidget *parent;
#if defined(_WIN32)
        parent = nullptr;
#else
        parent = this;
#endif
        m_ui->sandboxWidget = new QWidget { parent, Qt::Window };
        m_ui->sandboxWidget->setAttribute(Qt::WA_StyledBackground, true);
        m_ui->sandboxWidget->resize(640, 480);
        m_ui->sandboxWidget->setObjectName("sandboxWindow");
        m_ui->sandboxWidget->show();
        updateSandboxBackground();
    }
    else {
        m_ui->sandboxWidget->close();
        delete m_ui->sandboxWidget;
        m_ui->sandboxWidget = nullptr;
    }

    updateEnvironment();
    std::shared_ptr<shijima::mascot::environment> env;
    if (windowedMode) {
        env = m_runtime->environment.environmentForScreen(nullptr);
    }
    else {
        env = m_runtime->environment.environmentForScreen(mascotScreen());
    }

    for (auto &mascot : m_runtime->sessions.mascots()) {
        bool inspectorWasVisible = mascot->inspectorVisible();
        auto newMascot = new ShijimaWidget(*mascot, windowedMode,
            mascotParent());
        newMascot->setEnv(env);
        delete mascot;
        mascot = newMascot;
        m_runtime->sessions.index(mascot);
        mascot->mascot().reset_position();
        mascot->show();
        if (inspectorWasVisible) {
            mascot->showInspector();
        }
    }
}

void ShijimaManager::updateEnvironment(QScreen *screen) {
    m_runtime->environment.updateScreen(screen, m_ui->sandboxWidget,
        windowedMode());
}

void ShijimaManager::updateEnvironment() {
    m_runtime->environment.updateAll(m_ui->sandboxWidget, windowedMode(),
        QGuiApplication::screens());
}

bool ShijimaManager::windowedMode() {
    return m_ui->sandboxWidget != nullptr;
}

QWidget *ShijimaManager::mascotParent() {
    if (windowedMode()) {
        return m_ui->sandboxWidget;
    }
    return nullptr;
}

QScreen *ShijimaManager::mascotScreen() {
    if (windowedMode()) {
        return nullptr;
    }

    QScreen *screen = this->screen();
    if (screen == nullptr) {
        screen = qApp->primaryScreen();
    }
    return screen;
}
