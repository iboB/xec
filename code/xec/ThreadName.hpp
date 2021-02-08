// xec
// Copyright (c) 2020-2021 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include "API.h"

#include <thread>
#include <string_view>

namespace xec
{
// Debug helpers which give names to threads so that they are easily identifiable in the debugger
// Return 0 on success, non-zero otherwise
XEC_API int SetThreadName(std::thread& t, std::string_view name);
XEC_API int SetThisThreadName(std::string_view name);
}
