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
#include "ManagerUiHelpers.hpp"
#include "ManagerTrayController.hpp"
#include <functional>
#include <memory>
#include <QApplication>
#include <QCoreApplication>
#include <QMenu>
#include <QMetaObject>
#include <QStyle>
#include <QSystemTrayIcon>

namespace {

QIcon makeTrayIconFallback(QWidget *widget) {
    QIcon ico { QStringLiteral(":/neurolingsce.ico") };
    if (!ico.isNull()) {
        return ico;
    }

    ico = QIcon { QStringLiteral(":/neurolingsce.png") };
    if (!ico.isNull()) {
        return ico;
    }

    if (qApp != nullptr) {
        QIcon appIcon = qApp->windowIcon();
        if (!appIcon.isNull()) {
            return appIcon;
        }
    }

    if (widget != nullptr) {
        QIcon windowIcon = widget->windowIcon();
        if (!windowIcon.isNull()) {
            return windowIcon;
        }
    }

    QIcon themed = QIcon::fromTheme("shijima-qt");
    if (!themed.isNull()) {
        return themed;
    }
    if (qApp != nullptr && qApp->style() != nullptr) {
        return qApp->style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    return {};
}

}

ManagerTrayController::ManagerTrayController(ShijimaManager *manager):
    QObject(manager),
    m_manager(manager)
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip(QCoreApplication::translate("ShijimaManager", APP_NAME));
    m_trayIcon->setIcon(makeTrayIconFallback(manager));

    m_trayMenu = new QMenu(manager);
    m_trayIcon->setContextMenu(m_trayMenu);
    refreshMenu();

    QObject::connect(m_trayIcon, &QSystemTrayIcon::activated,
        [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                m_manager->setManagerVisible(!m_manager->isVisible());
                refreshMenu();
            }
        });
    QObject::connect(m_trayIcon, &QSystemTrayIcon::messageClicked,
        m_manager, [this]() {
            if (!m_messageClickHandler) {
                return;
            }
            auto handler = m_messageClickHandler;
            m_messageClickHandler = {};
            handler();
        });

    m_trayIcon->show();
}

ManagerTrayController::~ManagerTrayController() {
    // The controller is owned by the manager UI state. Clear click handlers
    // before deleting tray objects so stale notifications cannot call back.
    m_messageClickHandler = {};
    if (m_trayIcon != nullptr) {
        m_trayIcon->hide();
        m_trayIcon->setContextMenu(nullptr);
        delete m_trayIcon;
        m_trayIcon = nullptr;
    }
    if (m_trayMenu != nullptr) {
        m_trayMenu->hide();
        delete m_trayMenu;
        m_trayMenu = nullptr;
    }
}

bool ManagerTrayController::isAvailable() {
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void ManagerTrayController::refreshMenu() {
    if (m_trayMenu == nullptr || m_manager == nullptr) {
        return;
    }

    m_trayMenu->clear();

    QAction *toggleAction = m_trayMenu->addAction(
        m_manager->isVisible()
            ? QCoreApplication::translate("ShijimaManager", "Hide")
            : QCoreApplication::translate("ShijimaManager", "Show"));
    QObject::connect(toggleAction, &QAction::triggered, [this]() {
        m_manager->setManagerVisible(!m_manager->isVisible());
        refreshMenu();
    });

    QMenu *spawnMenu = m_trayMenu->addMenu(
        QCoreApplication::translate("ShijimaManager", "Spawn"));
    auto names = m_manager->loadedMascots().keys();
    names.sort(Qt::CaseInsensitive);
    for (auto const& name : names) {
        QAction *action = spawnMenu->addAction(name);
        QObject::connect(action, &QAction::triggered, [this, name]() {
            m_manager->spawn(name.toStdString());
        });
    }
    if (names.isEmpty()) {
        QAction *empty = spawnMenu->addAction(
            QCoreApplication::translate("ShijimaManager", "(none)"));
        empty->setEnabled(false);
    }

    m_trayMenu->addSeparator();

    QAction *killAllAction = m_trayMenu->addAction(
        QCoreApplication::translate("ShijimaManager", "Kill all"));
    QObject::connect(killAllAction, &QAction::triggered, [this]() {
        m_manager->killAll();
    });

    QAction *quitAction = m_trayMenu->addAction(
        QCoreApplication::translate("ShijimaManager", "Quit"));
    QObject::connect(quitAction, &QAction::triggered, [manager = m_manager]() {
        QMetaObject::invokeMethod(manager, [manager]() {
            manager->quitAction();
        }, Qt::QueuedConnection);
    });
}

void ManagerTrayController::showMessage(QString const& title,
    QString const& message, std::function<void()> onClick)
{
    if (m_trayIcon == nullptr) {
        return;
    }

    m_messageClickHandler = std::move(onClick);
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 10000);
}

namespace ShijimaManagerUiInternal {

void refreshTrayMenu(ManagerTrayController *controller) {
    if (controller != nullptr) {
        controller->refreshMenu();
    }
}

void setupTrayIcon(ShijimaManager *manager,
    std::unique_ptr<ManagerTrayController>& controller)
{
    if (manager == nullptr || !ManagerTrayController::isAvailable()) {
        return;
    }
    if (controller != nullptr) {
        return;
    }

    controller = std::make_unique<ManagerTrayController>(manager);
}

void showTrayMessage(QString const& title, QString const& message,
    ManagerTrayController *controller, std::function<void()> onClick)
{
    if (controller != nullptr) {
        controller->showMessage(title, message, std::move(onClick));
    }
}

void teardownTrayIcon(std::unique_ptr<ManagerTrayController>& controller) {
    controller.reset();
}

}
