// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "API.h"

#include <thread>
#include <string_view>
#include <string>

namespace xec {
// Debug helpers which give names to threads so that they are easily identifiable in the debugger
// Return 0 on success, non-zero otherwise
XEC_API int SetThreadName(std::thread& t, std::string_view name);
XEC_API int SetThisThreadName(std::string_view name);
XEC_API std::string GetThreadName(std::thread& t);
XEC_API std::string GetThisThreadName();
}
