// xec
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#include "ThreadName.hpp"

#include <cstring>
#include <cstdio>
#include <cassert>

#if defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include <string>
using tid = HANDLE;
#else
using tid = pthread_t;
#endif

namespace xec
{

namespace
{
int doSetName(tid h, std::string_view name) {
    // max length for thread names with pthread is 16 symbols
    // even though we don't have the same limitation on Widows, we assert anyway for multiplatofm-ness
    // note that the length is safe-guarded below
    // you can remove this assertion and the thread names will be trimmed on unix
    // however it's here so we have a notification if we violate the rule
    assert(name.length() < 16);

#if defined(_WIN32)
    std::wstring ww;
    ww.reserve(name.length());
    for (auto c : name) {
        ww.push_back(wchar_t(c));
    }
    auto res = SetThreadDescription(h, ww.c_str());
    // hresult hax
    if (SUCCEEDED(res)) return 0;
    return res;
#else
    char name16[16];
    auto len = std::min(name.length()+1, size_t(16));
    snprintf(name16, len, "%s", name.data());
    return pthread_setname_np(h, name16);
#endif
}
}

int SetThreadName(std::thread& t, std::string_view name) {
    return doSetName(t.native_handle(), name);
}

int SetThisThreadName(std::string_view name) {
#if defined(_WIN32)
    return doSetName(GetCurrentThread(), name);
#else
    return doSetName(pthread_self(), name);
#endif
}

}
