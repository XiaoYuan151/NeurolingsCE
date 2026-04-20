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

#include <QCoreApplication>

#include "shijima-qt/AppLog.hpp"
#include "shijima-qt/cli.hpp"

#include <exception>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
LONG WINAPI cliUnhandledExceptionFilter(EXCEPTION_POINTERS *info) {
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

void cliTerminateHandler() {
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
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral(APP_NAME));
    AppLog::initialize(&app);
    std::set_terminate(cliTerminateHandler);
#ifdef _WIN32
    SetUnhandledExceptionFilter(cliUnhandledExceptionFilter);
#endif
    int ret = shijimaRunCli(argc, argv);
    AppLog::shutdown();
    return ret;
}
