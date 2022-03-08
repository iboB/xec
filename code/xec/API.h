// xec
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once
#include <splat/symbol_export.h>

#if XEC_SHARED
#   if BUILDING_XEC
#       define XEC_API SYMBOL_EXPORT
#   else
#       define XEC_API SYMBOL_IMPORT
#   endif
#else
#   define XEC_API
#endif
