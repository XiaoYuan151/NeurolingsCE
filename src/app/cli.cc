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

#include "shijima-qt/cli.hpp"
#include "shijima-qt/AppLog.hpp"

#include "cli/InternalCli.hpp"

bool shijimaShouldRunCli(int argc, char **argv) {
    return isCliInvocation(argc, argv);
}

int shijimaRunCli(int argc, char **argv) {
    auto parsed = parseCliArguments(argc, argv);
    if (parsed.hasError) {
        APP_LOG_ERROR("cli") << parsed.error.error.toStdString();
        return writeCliError(parsed.global, parsed.error);
    }
    APP_LOG_INFO("cli") << "CLI command started";
    auto result = executeCliCommand(parsed.command);
    int ret = writeCliOutput(parsed.command, result);
    APP_LOG_INFO("cli") << "CLI command finished exit_code=" << ret;
    return ret;
}
