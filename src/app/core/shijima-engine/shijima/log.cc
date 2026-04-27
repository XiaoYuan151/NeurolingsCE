// 
// libshijima - C++ library for shimeji desktop mascots
// Copyright (C) 2024-2025 pixelomer
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

#include "log.hpp"
#include <iostream>

namespace shijima {

#ifdef SHIJIMA_LOGGING_ENABLED

void default_log(std::string const& log);
void default_log_with_level(uint16_t level, std::string const& log);

static uint16_t log_level = 0;
static std::function<void(uint16_t, std::string const&)> active_logger =
    default_log_with_level;

void set_log_level(uint16_t level) {
    log_level = level;
}
uint16_t get_log_level() {
    return log_level;
}
void log(uint16_t level, std::string const& log) {
    if (log_level & level) {
        active_logger(level, log);
    }
}
void log(std::string const& msg) {
    active_logger(SHIJIMA_LOG_EVERYTHING, msg);
}
void default_log(std::string const& log) {
    std::cerr << log << std::endl;
}
void default_log_with_level(uint16_t, std::string const& log) {
    default_log(log);
}
void set_logger(std::function<void(std::string const&)> logger) {
    active_logger = [logger](uint16_t, std::string const& log) {
        logger(log);
    };
}
void set_logger(std::function<void(uint16_t, std::string const&)> logger) {
    active_logger = logger ? logger : default_log_with_level;
}

#endif /* defined(SHIJIMA_LOGGING_ENABLED) */

}
