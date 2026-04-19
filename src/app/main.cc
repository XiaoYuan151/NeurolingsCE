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

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QMessageBox>
#include "shijima-qt/AppLog.hpp"
#include <shijima/log.hpp>
#include "Platform/Platform.hpp"
#include "shijima-qt/ShijimaManager.hpp"
#include "shijima-qt/AssetLoader.hpp"
#include "shijima-qt/cli.hpp"
#include <httplib.h>
#include "ElaApplication.h"
#include <exception>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
LONG WINAPI appUnhandledExceptionFilter(EXCEPTION_POINTERS *info) {
    std::ostringstream oss;
    if (info != nullptr && info->ExceptionRecord != nullptr) {
        oss << "Unhandled SEH exception code=0x"
            << std::hex << std::uppercase
            << info->ExceptionRecord->ExceptionCode
            << " address=" << info->ExceptionRecord->ExceptionAddress;
    }
    else {
        oss << "Unhandled SEH exception with null EXCEPTION_POINTERS";
    }
    AppLog::writeCrash("crash", oss.str());
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void appTerminateHandler() {
    try {
        auto ex = std::current_exception();
        if (ex != nullptr) {
            std::rethrow_exception(ex);
        }
        AppLog::writeCrash("crash", "std::terminate called without an active exception");
    }
    catch (std::exception const& ex) {
        AppLog::writeCrash("crash", std::string("std::terminate: ") + ex.what());
    }
    catch (...) {
        AppLog::writeCrash("crash", "std::terminate: unknown exception");
    }
    std::abort();
}

}

int main(int argc, char **argv) {
    if (argc > 1) {
        QCoreApplication app(argc, argv);
        app.setApplicationName(QStringLiteral(APP_NAME));
        AppLog::initialize(&app);
        std::set_terminate(appTerminateHandler);
#ifdef _WIN32
        SetUnhandledExceptionFilter(appUnhandledExceptionFilter);
#endif
        int ret = shijimaRunCli(argc, argv);
        AppLog::shutdown();
        return ret;
    }
    Platform::initialize(argc, argv);
    #ifdef SHIJIMA_LOGGING_ENABLED
        shijima::set_log_level(SHIJIMA_LOG_PARSER | SHIJIMA_LOG_WARNINGS);
    #endif
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(APP_NAME));
    app.setApplicationDisplayName(QStringLiteral(APP_DISPLAY_NAME));
    AppLog::initialize(&app);
    std::set_terminate(appTerminateHandler);
#ifdef _WIN32
    SetUnhandledExceptionFilter(appUnhandledExceptionFilter);
#endif
    eApp->init();
    {
        QIcon appIcon { QStringLiteral(":/neurolingsce.ico") };
        if (appIcon.isNull()) {
            appIcon = QIcon { QStringLiteral(":/neurolingsce.png") };
        }
        if (!appIcon.isNull()) {
            app.setWindowIcon(appIcon);
        }
    }
    try {
        httplib::Client pingClient { "http://127.0.0.1:32456" };
        pingClient.set_connection_timeout(0, 500000);
        pingClient.set_read_timeout(0, 500000);
        auto pingResult = pingClient.Get("/shijima/api/v1/ping");
        if (pingResult != nullptr) {
            APP_LOG_ERROR("startup") << "Single-instance guard rejected startup; ping endpoint already responded";
            throw std::runtime_error(QCoreApplication::translate("main", APP_NAME " is already running!").toStdString());
        }
        APP_LOG_INFO("startup") << "Application startup checks passed";
        ShijimaManager::defaultManager()->show();
    }
    catch (std::exception &ex) {
        APP_LOG_ERROR("startup") << "Failed to start application: " << ex.what();
        QMessageBox *msg = new QMessageBox {};
        msg->setText(QCoreApplication::translate("main", APP_NAME " failed to start. Reason: ") +
            QString::fromUtf8(ex.what()));
        msg->setStandardButtons(QMessageBox::StandardButton::Close);
        msg->setAttribute(Qt::WA_DeleteOnClose);
        msg->show();
    }
    int ret = app.exec();
    ShijimaManager::finalize();
    AssetLoader::finalize();
    AppLog::shutdown();
    return ret;
}
